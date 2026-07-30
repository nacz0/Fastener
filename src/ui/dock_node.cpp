#include "fastener/ui/dock_node.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace fst {

DockNode* DockNode::child(int index) {
    return index >= 0 && index < 2 ? m_children[index].get() : nullptr;
}

const DockNode* DockNode::child(int index) const {
    return index >= 0 && index < 2 ? m_children[index].get() : nullptr;
}

bool DockNode::selectTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_dockedWindows.size())) {
        return false;
    }
    m_selectedTabIndex = index;
    return true;
}

bool DockNode::setSplitRatio(float ratio) {
    if (!isSplitNode() ||
        !std::isfinite(ratio) ||
        ratio <= 0.0f ||
        ratio >= 1.0f) {
        return false;
    }
    m_splitRatio = ratio;
    return true;
}

DockNode* DockNode::findNodeByWindowId(WidgetId windowId) {
    if (hasWindow(windowId)) {
        return this;
    }
    for (auto& childNode : m_children) {
        if (childNode) {
            if (DockNode* found = childNode->findNodeByWindowId(windowId)) {
                return found;
            }
        }
    }
    return nullptr;
}

const DockNode* DockNode::findNodeByWindowId(WidgetId windowId) const {
    if (hasWindow(windowId)) {
        return this;
    }
    for (const auto& childNode : m_children) {
        if (childNode) {
            if (const DockNode* found =
                    childNode->findNodeByWindowId(windowId)) {
                return found;
            }
        }
    }
    return nullptr;
}

DockNode* DockNode::findNodeById(Id nodeId) {
    if (m_id == nodeId) {
        return this;
    }
    for (auto& childNode : m_children) {
        if (childNode) {
            if (DockNode* found = childNode->findNodeById(nodeId)) {
                return found;
            }
        }
    }
    return nullptr;
}

const DockNode* DockNode::findNodeById(Id nodeId) const {
    if (m_id == nodeId) {
        return this;
    }
    for (const auto& childNode : m_children) {
        if (childNode) {
            if (const DockNode* found = childNode->findNodeById(nodeId)) {
                return found;
            }
        }
    }
    return nullptr;
}

void DockNode::addWindow(WidgetId windowId) {
    if (windowId == INVALID_WIDGET_ID || hasWindow(windowId)) {
        return;
    }
    m_dockedWindows.push_back(windowId);
    m_type = m_dockedWindows.size() > 1
        ? DockNodeType::TabContainer
        : DockNodeType::Leaf;
}

void DockNode::removeWindow(WidgetId windowId) {
    auto it =
        std::find(m_dockedWindows.begin(), m_dockedWindows.end(), windowId);
    if (it == m_dockedWindows.end()) {
        return;
    }

    m_dockedWindows.erase(it);
    if (m_selectedTabIndex >= static_cast<int>(m_dockedWindows.size())) {
        m_selectedTabIndex =
            std::max(0, static_cast<int>(m_dockedWindows.size()) - 1);
    }
    m_type = m_dockedWindows.size() > 1
        ? DockNodeType::TabContainer
        : DockNodeType::Leaf;
}

bool DockNode::hasWindow(WidgetId windowId) const {
    return std::find(
               m_dockedWindows.begin(),
               m_dockedWindows.end(),
               windowId) != m_dockedWindows.end();
}

DockNode* DockNode::splitNode(
    DockDirection direction,
    Id childId0,
    Id childId1,
    float ratio) {
    if (direction == DockDirection::None ||
        direction == DockDirection::Center ||
        m_flags.noSplit ||
        isSplitNode() ||
        m_children[0] ||
        m_children[1] ||
        childId0 == INVALID_ID ||
        childId1 == INVALID_ID ||
        childId0 == m_id ||
        childId1 == m_id ||
        childId0 == childId1 ||
        !std::isfinite(ratio) ||
        ratio <= 0.0f ||
        ratio >= 1.0f) {
        return nullptr;
    }

    auto newChild0 = std::make_unique<DockNode>(childId0);
    auto newChild1 = std::make_unique<DockNode>(childId1);
    newChild0->m_parent = this;
    newChild1->m_parent = this;

    DockNode* existingContent = nullptr;
    DockNode* newContent = nullptr;
    if (direction == DockDirection::Left ||
        direction == DockDirection::Top) {
        existingContent = newChild1.get();
        newContent = newChild0.get();
        m_splitRatio = ratio;
    } else {
        existingContent = newChild0.get();
        newContent = newChild1.get();
        m_splitRatio = 1.0f - ratio;
    }

    existingContent->m_dockedWindows = std::move(m_dockedWindows);
    existingContent->m_selectedTabIndex = m_selectedTabIndex;
    existingContent->m_type = existingContent->m_dockedWindows.size() > 1
        ? DockNodeType::TabContainer
        : DockNodeType::Leaf;

    m_dockedWindows.clear();
    m_selectedTabIndex = 0;
    newContent->m_type = DockNodeType::Leaf;
    m_type = (direction == DockDirection::Left ||
              direction == DockDirection::Right)
        ? DockNodeType::SplitHorizontal
        : DockNodeType::SplitVertical;
    m_children[0] = std::move(newChild0);
    m_children[1] = std::move(newChild1);
    return newContent;
}

void DockNode::mergeNodes() {
    if (!isSplitNode()) {
        return;
    }

    DockNode* nonEmptyChild = nullptr;
    int emptyCount = 0;
    for (auto& childNode : m_children) {
        if (childNode && childNode->isEmpty()) {
            ++emptyCount;
        } else if (childNode) {
            nonEmptyChild = childNode.get();
        }
    }

    if (emptyCount == 1 && nonEmptyChild) {
        auto childWindows = std::move(nonEmptyChild->m_dockedWindows);
        const int childSelected = nonEmptyChild->m_selectedTabIndex;
        const DockNodeType childType = nonEmptyChild->m_type;
        auto grandchild0 = std::move(nonEmptyChild->m_children[0]);
        auto grandchild1 = std::move(nonEmptyChild->m_children[1]);

        m_children[0].reset();
        m_children[1].reset();

        m_dockedWindows = std::move(childWindows);
        m_selectedTabIndex = childSelected;
        m_type = childType;
        m_children[0] = std::move(grandchild0);
        m_children[1] = std::move(grandchild1);
        if (m_children[0]) m_children[0]->m_parent = this;
        if (m_children[1]) m_children[1]->m_parent = this;
    } else if (emptyCount == 2) {
        m_type = DockNodeType::Leaf;
        m_children[0].reset();
        m_children[1].reset();
    }
}

void DockNode::updateLayout(const Rect& availableBounds) {
    m_bounds = availableBounds;
    if (!isSplitNode()) {
        return;
    }

    if (m_children[0]) {
        m_children[0]->updateLayout(getChildBounds(0));
    }
    if (m_children[1]) {
        m_children[1]->updateLayout(getChildBounds(1));
    }
}

Rect DockNode::getChildBounds(int childIndex) const {
    if (childIndex < 0 || childIndex > 1 || !isSplitNode()) {
        return m_bounds;
    }

    constexpr float splitterSize = 4.0f;
    if (m_type == DockNodeType::SplitHorizontal) {
        const float splitX =
            m_bounds.x() + m_bounds.width() * m_splitRatio;
        if (childIndex == 0) {
            return Rect(
                m_bounds.x(),
                m_bounds.y(),
                splitX - m_bounds.x() - splitterSize * 0.5f,
                m_bounds.height());
        }
        return Rect(
            splitX + splitterSize * 0.5f,
            m_bounds.y(),
            m_bounds.right() - splitX - splitterSize * 0.5f,
            m_bounds.height());
    }

    const float splitY =
        m_bounds.y() + m_bounds.height() * m_splitRatio;
    if (childIndex == 0) {
        return Rect(
            m_bounds.x(),
            m_bounds.y(),
            m_bounds.width(),
            splitY - m_bounds.y() - splitterSize * 0.5f);
    }
    return Rect(
        m_bounds.x(),
        splitY + splitterSize * 0.5f,
        m_bounds.width(),
        m_bounds.bottom() - splitY - splitterSize * 0.5f);
}

void DockNode::forEachNode(
    const std::function<void(DockNode*)>& callback) {
    callback(this);
    for (auto& childNode : m_children) {
        if (childNode) {
            childNode->forEachNode(callback);
        }
    }
}

void DockNode::forEachLeaf(
    const std::function<void(DockNode*)>& callback) {
    if (isLeafNode() || (!m_children[0] && !m_children[1])) {
        callback(this);
        return;
    }
    for (auto& childNode : m_children) {
        if (childNode) {
            childNode->forEachLeaf(callback);
        }
    }
}

std::string DockNode::debugPrint(int depth) const {
    std::stringstream ss;
    const std::string indent(static_cast<std::size_t>(depth) * 2, ' ');
    ss << indent << "DockNode[" << m_id << "] ";
    switch (m_type) {
        case DockNodeType::Unknown: ss << "Unknown"; break;
        case DockNodeType::SplitHorizontal: ss << "SplitH"; break;
        case DockNodeType::SplitVertical: ss << "SplitV"; break;
        case DockNodeType::TabContainer: ss << "TabContainer"; break;
        case DockNodeType::Leaf: ss << "Leaf"; break;
    }
    ss << " bounds(" << m_bounds.x() << ',' << m_bounds.y() << ','
       << m_bounds.width() << ',' << m_bounds.height() << ')';

    if (!m_dockedWindows.empty()) {
        ss << " windows[";
        for (std::size_t index = 0;
             index < m_dockedWindows.size();
             ++index) {
            if (index > 0) ss << ',';
            ss << m_dockedWindows[index];
        }
        ss << ']';
    }
    ss << '\n';

    for (const auto& childNode : m_children) {
        if (childNode) {
            ss << childNode->debugPrint(depth + 1);
        }
    }
    return ss.str();
}

} // namespace fst
