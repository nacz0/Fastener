#include "fastener/ui/dock_context.h"
#include "fastener/core/context.h"
#include "fastener/core/input.h"
#include <unordered_map>
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
        it->second->bounds = bounds;
        it->second->updateLayout(bounds);
        return it->second->id;
    }
    
    // Create new dock space
    auto nodeId = m_impl->generateId();
    auto node = std::make_unique<DockNode>(nodeId);
    node->type = DockNodeType::Leaf;
    node->bounds = bounds;
    
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
            for (auto winId : leaf->dockedWindows) {
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
    // First undock if already docked elsewhere
    undockWindow(windowId);
    
    DockNode* targetNode = getDockNode(targetNodeId);
    if (!targetNode) {
        return;
    }
    
    if (direction == DockDirection::Center || direction == DockDirection::None) {
        // Tab docking - add to existing node
        targetNode->addWindow(windowId);
        m_impl->windowToNode[windowId] = targetNode->id;
    } else {
        // Split docking - create new split
        DockNode::Id childId0 = generateNodeId();
        DockNode::Id childId1 = generateNodeId();
        DockNode* newNode = targetNode->splitNode(direction, childId0, childId1);
        if (newNode) {
            newNode->addWindow(windowId);
            m_impl->windowToNode[windowId] = newNode->id;
            
            // Also refresh mappings for the targetNode (which now has children)
            refreshMappings(targetNodeId);
        }
    }
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
        
        // Merge parent if this node is now empty
        DockNode::Id currentId = nodeId;
        while (currentId != DockNode::INVALID_ID) {
            DockNode* current = getDockNode(currentId);
            if (!current || !current->isEmpty() || !current->parent) break;
            
            DockNode* parent = current->parent;
            DockNode::Id parentId = parent->id;
            
            parent->mergeNodes();
            refreshMappings(parentId);
            currentId = parentId;
        }
    }
}

void DockContext::refreshMappings(DockNode::Id nodeId) {
    if (nodeId == DockNode::INVALID_ID) {
        // Refresh all dock spaces
        for (auto& [name, root] : m_impl->dockSpaces) {
            refreshMappings(root->id);
        }
        return;
    }

    DockNode* node = getDockNode(nodeId);
    if (!node) return;

    // Update mappings for this node
    for (auto winId : node->dockedWindows) {
        m_impl->windowToNode[winId] = node->id;
    }

    // Recursively update children
    if (node->isSplitNode()) {
        if (node->children[0]) refreshMappings(node->children[0]->id);
        if (node->children[1]) refreshMappings(node->children[1]->id);
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
            if (leaf->bounds.contains(mousePos)) {
                m_impl->dragState.hoveredNodeId = leaf->id;
                
                const Rect& b = leaf->bounds;
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
        root->updateLayout(root->bounds);
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
    
    for (const auto& [name, root] : m_impl->dockSpaces) {
        ss << "[DockSpace:" << name << "]\n";
        ss << root->debugPrint();
        ss << "\n";
    }
    
    return ss.str();
}

bool DockContext::deserializeLayout(const std::string& data) {
    // TODO: Implement proper deserialization
    // For now, this is a placeholder
    (void)data;
    return false;
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
