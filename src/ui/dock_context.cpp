#include "fastener/ui/dock_context.h"
#include "fastener/core/context.h"
#include "fastener/core/input.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace fst {

//=============================================================================
// Implementation
//=============================================================================

struct DockContext::Impl {
    // Dock spaces indexed by string ID
    std::unordered_map<std::string, std::unique_ptr<DockNode>> dockSpaces;
    
    // String ID to node ID mapping
    std::unordered_map<std::string, DockNode::Id> dockSpaceIds;
    
    // Window to node mapping for quick lookups
    std::unordered_map<WidgetId, DockNode::Id> windowToNode;
    
    // Window titles for UI
    std::unordered_map<WidgetId, std::string> windowTitles;
    
    // Drag state
    DragState dragState;
    
    // ID generation
    DockNode::Id nextNodeId = 1;
    
    DockNode::Id generateId() {
        return nextNodeId++;
    }
};

//=============================================================================
// Constructor / Destructor
//=============================================================================

DockContext::DockContext() : m_impl(std::make_unique<Impl>()) {}
DockContext::~DockContext() = default;

//=============================================================================
// DockSpace Management
//=============================================================================

DockNode::Id DockContext::createDockSpace(const std::string& id, const Rect& bounds) {
    auto it = m_impl->dockSpaces.find(id);
    
    if (it != m_impl->dockSpaces.end()) {
        // Existing dock space - update bounds
        it->second->m_bounds = bounds;
        it->second->updateLayout(bounds);
        return it->second->m_id;
    }
    
    // Create new dock space
    auto nodeId = m_impl->generateId();
    auto node = std::make_unique<DockNode>(nodeId);
    node->m_type = DockNodeType::Leaf;
    node->m_bounds = bounds;
    
    m_impl->dockSpaceIds[id] = nodeId;
    m_impl->dockSpaces[id] = std::move(node);
    
    return nodeId;
}

DockNode* DockContext::getDockSpace(const std::string& id) {
    auto it = m_impl->dockSpaces.find(id);
    return it != m_impl->dockSpaces.end() ? it->second.get() : nullptr;
}

const DockNode* DockContext::getDockSpace(const std::string& id) const {
    auto it = m_impl->dockSpaces.find(id);
    return it != m_impl->dockSpaces.end() ? it->second.get() : nullptr;
}

DockNode* DockContext::getDockNode(DockNode::Id nodeId) {
    for (auto& [name, root] : m_impl->dockSpaces) {
        if (auto* found = root->findNodeById(nodeId)) {
            return found;
        }
    }
    return nullptr;
}

const DockNode* DockContext::getDockNode(DockNode::Id nodeId) const {
    return const_cast<DockContext*>(this)->getDockNode(nodeId);
}

void DockContext::removeDockSpace(const std::string& id) {
    auto it = m_impl->dockSpaces.find(id);
    if (it != m_impl->dockSpaces.end()) {
        // Remove all window mappings for this dock space
        it->second->forEachLeaf([this](DockNode* leaf) {
            for (auto winId : leaf->m_dockedWindows) {
                m_impl->windowToNode.erase(winId);
            }
        });
        
        m_impl->dockSpaceIds.erase(id);
        m_impl->dockSpaces.erase(it);
    }
}

//=============================================================================
// Window Docking
//=============================================================================

void DockContext::dockWindow(WidgetId windowId, DockNode::Id targetNodeId, 
                              DockDirection direction) {
    DockNode* targetNode = getDockNode(targetNodeId);
    const bool tabDock =
        direction == DockDirection::Center || direction == DockDirection::None;
    if (!targetNode ||
        !targetNode->isLeafNode() ||
        (!tabDock && targetNode->m_flags.noSplit)) {
        return;
    }

    DockNode* sourceNode = getWindowDockNode(windowId);
    const DockNode::Id sourceNodeId =
        sourceNode ? sourceNode->m_id : DockNode::INVALID_ID;

    // Docking a tab back into its current leaf is already satisfied. Splitting
    // a single-window leaf against itself would create an empty sibling that
    // immediately collapses, so preserve the existing layout as well.
    if (sourceNode == targetNode &&
        (tabDock || sourceNode->m_dockedWindows.size() == 1)) {
        return;
    }

    // Remove the source without collapsing its ancestors yet. Collapsing first
    // can destroy a sibling target node and invalidate the requested move.
    if (sourceNode) {
        sourceNode->removeWindow(windowId);
        m_impl->windowToNode.erase(windowId);
    }

    bool docked = false;
    if (tabDock) {
        // Tab docking - add to existing node
        targetNode->addWindow(windowId);
        m_impl->windowToNode[windowId] = targetNode->m_id;
        docked = true;
    } else {
        // Split docking - create new split
        DockNode::Id childId0 = generateNodeId();
        DockNode::Id childId1 = generateNodeId();
        DockNode* newNode = targetNode->splitNode(direction, childId0, childId1);
        if (newNode) {
            newNode->addWindow(windowId);
            m_impl->windowToNode[windowId] = newNode->m_id;
            
            // Also refresh mappings for the targetNode (which now has children)
            refreshMappings(targetNodeId);
            docked = true;
        }
    }

    if (!docked) {
        if (sourceNode) {
            sourceNode->addWindow(windowId);
            m_impl->windowToNode[windowId] = sourceNodeId;
        }
        return;
    }

    // Only now is it safe to collapse the empty source branch. Any destination
    // mappings moved into an absorbed parent are refreshed during the merge.
    collapseEmptyAncestors(sourceNodeId);
}

DockNode::Id DockContext::splitNode(
    DockNode::Id targetNodeId,
    DockDirection direction,
    float ratio) {
    DockNode* targetNode = getDockNode(targetNodeId);
    if (!targetNode || !targetNode->isLeafNode()) {
        return DockNode::INVALID_ID;
    }

    const DockNode::Id firstChildId = generateNodeId();
    const DockNode::Id secondChildId = generateNodeId();
    DockNode* newNode = targetNode->splitNode(
        direction,
        firstChildId,
        secondChildId,
        ratio);
    if (!newNode) {
        return DockNode::INVALID_ID;
    }
    refreshMappings(targetNodeId);
    return newNode->m_id;
}

bool DockContext::setNodeFlags(
    DockNode::Id nodeId,
    const DockNodeFlags& flags) {
    DockNode* node = getDockNode(nodeId);
    if (!node) {
        return false;
    }
    node->m_flags = flags;
    return true;
}

void DockContext::undockWindow(WidgetId windowId) {
    auto it = m_impl->windowToNode.find(windowId);
    if (it == m_impl->windowToNode.end()) {
        return;
    }
    
    DockNode::Id nodeId = it->second;
    m_impl->windowToNode.erase(it); // Erase first to avoid using invalid iterator

    DockNode* node = getDockNode(nodeId);
    if (node) {
        node->removeWindow(windowId);
        collapseEmptyAncestors(nodeId);
    }
}

void DockContext::collapseEmptyAncestors(DockNode::Id nodeId) {
    DockNode::Id currentId = nodeId;
    while (currentId != DockNode::INVALID_ID) {
        DockNode* current = getDockNode(currentId);
        if (!current || !current->isEmpty() || !current->m_parent) {
            break;
        }

        DockNode* parent = current->m_parent;
        const DockNode::Id parentId = parent->m_id;
        parent->mergeNodes();
        refreshMappings(parentId);
        currentId = parentId;
    }
}

void DockContext::refreshMappings(DockNode::Id nodeId) {
    if (nodeId == DockNode::INVALID_ID) {
        // Refresh all dock spaces
        for (auto& [name, root] : m_impl->dockSpaces) {
            refreshMappings(root->m_id);
        }
        return;
    }

    DockNode* node = getDockNode(nodeId);
    if (!node) return;

    // Update mappings for this node
    for (auto winId : node->m_dockedWindows) {
        m_impl->windowToNode[winId] = node->m_id;
    }

    // Recursively update children
    if (node->isSplitNode()) {
        if (node->m_children[0]) {
            refreshMappings(node->m_children[0]->m_id);
        }
        if (node->m_children[1]) {
            refreshMappings(node->m_children[1]->m_id);
        }
    }
}

bool DockContext::isWindowDocked(WidgetId windowId) const {
    return m_impl->windowToNode.find(windowId) != m_impl->windowToNode.end();
}

DockNode* DockContext::getWindowDockNode(WidgetId windowId) {
    auto it = m_impl->windowToNode.find(windowId);
    if (it != m_impl->windowToNode.end()) {
        return getDockNode(it->second);
    }
    return nullptr;
}

void DockContext::setWindowTitle(WidgetId windowId, const std::string& title) {
    m_impl->windowTitles[windowId] = title;
}

std::string DockContext::getWindowTitle(WidgetId windowId) const {
    auto it = m_impl->windowTitles.find(windowId);
    if (it != m_impl->windowTitles.end()) {
        return it->second;
    }
    return "Window";
}

const DockNode* DockContext::getWindowDockNode(WidgetId windowId) const {
    return const_cast<DockContext*>(this)->getWindowDockNode(windowId);
}

//=============================================================================
// Drag State
//=============================================================================

DockContext::DragState& DockContext::dragState() {
    return m_impl->dragState;
}

const DockContext::DragState& DockContext::dragState() const {
    return m_impl->dragState;
}

void DockContext::beginDrag(WidgetId windowId, const Vec2& mousePos) {
    m_impl->dragState.active = true;
    m_impl->dragState.windowId = windowId;
    m_impl->dragState.mousePos = mousePos;
    m_impl->dragState.hoveredNodeId = DockNode::INVALID_ID;
    m_impl->dragState.hoveredDirection = DockDirection::None;
}

void DockContext::updateDrag(const Vec2& mousePos, DockNode* /*hoveredNode*/, 
                               DockDirection /*direction*/) {
    if (!m_impl->dragState.active) return;
    
    m_impl->dragState.mousePos = mousePos;
    
    // Automatic hover detection
    m_impl->dragState.hoveredNodeId = DockNode::INVALID_ID;
    m_impl->dragState.hoveredDirection = DockDirection::None;
    
    for (auto& [name, root] : m_impl->dockSpaces) {
        root->forEachLeaf([&](DockNode* leaf) {
            if (leaf->m_bounds.contains(mousePos)) {
                m_impl->dragState.hoveredNodeId = leaf->m_id;
                
                const Rect& b = leaf->m_bounds;
                float relX = (mousePos.x - b.x()) / b.width();
                float relY = (mousePos.y - b.y()) / b.height();
                const float t = 0.25f;
                
                if (relX < t) m_impl->dragState.hoveredDirection = DockDirection::Left;
                else if (relX > 1.0f - t) m_impl->dragState.hoveredDirection = DockDirection::Right;
                else if (relY < t) m_impl->dragState.hoveredDirection = DockDirection::Top;
                else if (relY > 1.0f - t) m_impl->dragState.hoveredDirection = DockDirection::Bottom;
                else m_impl->dragState.hoveredDirection = DockDirection::Center;
            }
        });
        if (m_impl->dragState.hoveredNodeId != DockNode::INVALID_ID) break;
    }
}

void DockContext::endDrag(bool commit) {
    if (commit) {
        if (m_impl->dragState.hoveredNodeId != DockNode::INVALID_ID && 
            m_impl->dragState.hoveredDirection != DockDirection::None) {
            // Re-dock
            dockWindow(m_impl->dragState.windowId, 
                       m_impl->dragState.hoveredNodeId,
                       m_impl->dragState.hoveredDirection);
        } else {
            // Undock (become floating)
            undockWindow(m_impl->dragState.windowId);
        }
    }
    
    m_impl->dragState = DragState{};
}

//=============================================================================
// Frame Update
//=============================================================================

void DockContext::beginFrame(Context& ctx) {
    auto& input = ctx.input();
    
    // Update layouts for all dock spaces
    for (auto& [name, root] : m_impl->dockSpaces) {
        root->updateLayout(root->m_bounds);
    }

    // Update drag state if active
    if (m_impl->dragState.active) {
        updateDrag(input.mousePos());
        
        // Centralized release handling
        if (input.isMouseReleased(MouseButton::Left)) {
            endDrag(true);
        }
    }
}

void DockContext::endFrame() {
    // Cleanup empty nodes
    for (auto& [name, root] : m_impl->dockSpaces) {
        root->forEachNode([](DockNode* node) {
            if (node->isSplitNode()) {
                node->mergeNodes();
            }
        });
    }
}

//=============================================================================
// Persistence
//=============================================================================

std::string DockContext::serializeLayout() const {
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<float>::max_digits10);
    ss << "FST_DOCK_LAYOUT 1\n";
    ss << "SPACES " << m_impl->dockSpaces.size() << "\n";

    std::vector<std::string> names;
    names.reserve(m_impl->dockSpaces.size());
    for (const auto& entry : m_impl->dockSpaces) {
        names.push_back(entry.first);
    }
    std::sort(names.begin(), names.end());

    const auto flagsToMask = [](const DockNodeFlags& flags) {
        unsigned int mask = 0;
        if (flags.noSplit) mask |= 1u << 0;
        if (flags.noResize) mask |= 1u << 1;
        if (flags.noTabBar) mask |= 1u << 2;
        if (flags.keepAliveOnly) mask |= 1u << 3;
        if (flags.passthruCentralNode) mask |= 1u << 4;
        return mask;
    };

    std::function<void(const DockNode&)> writeNode =
        [&](const DockNode& node) {
            const int childCount =
                (node.m_children[0] ? 1 : 0) +
                (node.m_children[1] ? 1 : 0);
            ss << "NODE "
               << node.m_id << ' '
               << static_cast<int>(node.m_type) << ' '
               << flagsToMask(node.m_flags) << ' '
               << node.m_splitRatio << ' '
               << node.m_selectedTabIndex << ' '
               << node.m_bounds.x() << ' '
               << node.m_bounds.y() << ' '
               << node.m_bounds.width() << ' '
               << node.m_bounds.height() << ' '
               << node.m_dockedWindows.size();
            for (WidgetId windowId : node.m_dockedWindows) {
                ss << ' ' << windowId;
            }
            ss << ' ' << childCount << '\n';
            if (node.m_children[0]) writeNode(*node.m_children[0]);
            if (node.m_children[1]) writeNode(*node.m_children[1]);
        };

    for (const std::string& name : names) {
        const DockNode& root = *m_impl->dockSpaces.at(name);
        ss << "SPACE " << std::quoted(name) << '\n';
        writeNode(root);
    }
    ss << "END\n";
    return ss.str();
}

bool DockContext::deserializeLayout(const std::string& data) {
    constexpr std::size_t maxSpaces = 1024;
    constexpr std::size_t maxNodes = 100000;
    constexpr std::size_t maxWindowsPerNode = 100000;
    constexpr std::size_t maxTreeDepth = 256;
    constexpr unsigned int knownFlagMask = (1u << 5) - 1u;

    std::istringstream input(data);
    std::string token;
    unsigned int version = 0;
    if (!(input >> token >> version) ||
        token != "FST_DOCK_LAYOUT" ||
        version != 1) {
        return false;
    }

    std::size_t spaceCount = 0;
    if (!(input >> token >> spaceCount) ||
        token != "SPACES" ||
        spaceCount > maxSpaces) {
        return false;
    }

    auto candidate = std::make_unique<Impl>();
    std::unordered_set<DockNode::Id> nodeIds;
    std::unordered_set<WidgetId> windowIds;
    std::size_t nodeCount = 0;
    DockNode::Id maximumNodeId = DockNode::INVALID_ID;

    const auto maskToFlags = [](unsigned int mask) {
        DockNodeFlags flags;
        flags.noSplit = (mask & (1u << 0)) != 0;
        flags.noResize = (mask & (1u << 1)) != 0;
        flags.noTabBar = (mask & (1u << 2)) != 0;
        flags.keepAliveOnly = (mask & (1u << 3)) != 0;
        flags.passthruCentralNode = (mask & (1u << 4)) != 0;
        return flags;
    };

    bool valid = true;
    std::function<std::unique_ptr<DockNode>(DockNode*, std::size_t)> readNode;
    readNode = [&](DockNode* parent, std::size_t depth)
        -> std::unique_ptr<DockNode> {
        if (!valid || depth > maxTreeDepth || nodeCount >= maxNodes) {
            valid = false;
            return nullptr;
        }

        DockNode::Id nodeId = DockNode::INVALID_ID;
        int typeValue = 0;
        unsigned int flagsMask = 0;
        float splitRatio = 0.0f;
        int selectedTabIndex = 0;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        std::size_t windowCount = 0;
        if (!(input >> token >> nodeId >> typeValue >> flagsMask >>
              splitRatio >> selectedTabIndex >> x >> y >> width >> height >>
              windowCount) ||
            token != "NODE" ||
            nodeId == DockNode::INVALID_ID ||
            !nodeIds.insert(nodeId).second ||
            typeValue < static_cast<int>(DockNodeType::SplitHorizontal) ||
            typeValue > static_cast<int>(DockNodeType::Leaf) ||
            (flagsMask & ~knownFlagMask) != 0 ||
            !std::isfinite(splitRatio) ||
            !std::isfinite(x) ||
            !std::isfinite(y) ||
            !std::isfinite(width) ||
            !std::isfinite(height) ||
            width < 0.0f ||
            height < 0.0f ||
            windowCount > maxWindowsPerNode) {
            valid = false;
            return nullptr;
        }

        auto node = std::make_unique<DockNode>(nodeId);
        node->m_parent = parent;
        node->m_type = static_cast<DockNodeType>(typeValue);
        node->m_flags = maskToFlags(flagsMask);
        node->m_splitRatio = splitRatio;
        node->m_selectedTabIndex = selectedTabIndex;
        node->m_bounds = Rect(x, y, width, height);
        node->m_dockedWindows.reserve(windowCount);

        for (std::size_t index = 0; index < windowCount; ++index) {
            WidgetId windowId = INVALID_WIDGET_ID;
            if (!(input >> windowId) ||
                windowId == INVALID_WIDGET_ID ||
                !windowIds.insert(windowId).second) {
                valid = false;
                return nullptr;
            }
            node->m_dockedWindows.push_back(windowId);
            candidate->windowToNode.emplace(windowId, nodeId);
        }

        int childCount = 0;
        if (!(input >> childCount) || (childCount != 0 && childCount != 2)) {
            valid = false;
            return nullptr;
        }

        const bool splitNode =
            node->m_type == DockNodeType::SplitHorizontal ||
            node->m_type == DockNodeType::SplitVertical;
        const bool tabNode = node->m_type == DockNodeType::TabContainer;
        if ((splitNode && (childCount != 2 ||
                           !node->m_dockedWindows.empty() ||
                           splitRatio <= 0.0f ||
                           splitRatio >= 1.0f)) ||
            (!splitNode && childCount != 0) ||
            (tabNode && node->m_dockedWindows.size() < 2) ||
            (node->m_type == DockNodeType::Leaf &&
             node->m_dockedWindows.size() > 1) ||
            (node->m_dockedWindows.empty() && selectedTabIndex != 0) ||
            (!node->m_dockedWindows.empty() &&
             (selectedTabIndex < 0 ||
              selectedTabIndex >=
                  static_cast<int>(node->m_dockedWindows.size())))) {
            valid = false;
            return nullptr;
        }

        ++nodeCount;
        maximumNodeId = std::max(maximumNodeId, nodeId);
        if (childCount == 2) {
            node->m_children[0] = readNode(node.get(), depth + 1);
            node->m_children[1] = readNode(node.get(), depth + 1);
            if (!node->m_children[0] || !node->m_children[1]) {
                valid = false;
                return nullptr;
            }
        }
        return node;
    };

    for (std::size_t index = 0; index < spaceCount && valid; ++index) {
        std::string name;
        if (!(input >> token >> std::quoted(name)) ||
            token != "SPACE" ||
            name.empty() ||
            candidate->dockSpaces.find(name) != candidate->dockSpaces.end()) {
            valid = false;
            break;
        }

        std::unique_ptr<DockNode> root = readNode(nullptr, 0);
        if (!root) {
            valid = false;
            break;
        }
        candidate->dockSpaceIds.emplace(name, root->m_id);
        candidate->dockSpaces.emplace(std::move(name), std::move(root));
    }

    if (!valid ||
        !(input >> token) ||
        token != "END" ||
        (input >> token) ||
        maximumNodeId == std::numeric_limits<DockNode::Id>::max()) {
        return false;
    }

    candidate->nextNodeId = maximumNodeId + 1;
    if (candidate->nextNodeId == DockNode::INVALID_ID) {
        candidate->nextNodeId = 1;
    }
    m_impl = std::move(candidate);
    return true;
}

//=============================================================================
// ID Generation
//=============================================================================

DockNode::Id DockContext::generateNodeId() {
    return m_impl->generateId();
}

DockNode::Id DockContext::getNodeIdFromString(const std::string& str) const {
    auto it = m_impl->dockSpaceIds.find(str);
    return it != m_impl->dockSpaceIds.end() ? it->second : DockNode::INVALID_ID;
}

} // namespace fst
