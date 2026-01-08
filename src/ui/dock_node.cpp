#include "fastener/ui/dock_node.h"
#include <algorithm>
#include <sstream>

namespace fst {

//=============================================================================
// Tree queries
//=============================================================================

DockNode* DockNode::findNodeByWindowId(WidgetId windowId) {
    // Check this node
    auto it = std::find(dockedWindows.begin(), dockedWindows.end(), windowId);
    if (it != dockedWindows.end()) {
        return this;
    }
    
    // Check children
    for (int i = 0; i < 2; ++i) {
        if (children[i]) {
            DockNode* found = children[i]->findNodeByWindowId(windowId);
            if (found) {
                return found;
            }
        }
    }
    
    return nullptr;
}

const DockNode* DockNode::findNodeByWindowId(WidgetId windowId) const {
    return const_cast<DockNode*>(this)->findNodeByWindowId(windowId);
}

DockNode* DockNode::findNodeById(Id nodeId) {
    if (id == nodeId) {
        return this;
    }
    
    for (int i = 0; i < 2; ++i) {
        if (children[i]) {
            DockNode* found = children[i]->findNodeById(nodeId);
            if (found) {
                return found;
            }
        }
    }
    
    return nullptr;
}

const DockNode* DockNode::findNodeById(Id nodeId) const {
    return const_cast<DockNode*>(this)->findNodeById(nodeId);
}

//=============================================================================
// Window management
//=============================================================================

void DockNode::addWindow(WidgetId windowId) {
    if (!hasWindow(windowId)) {
        dockedWindows.push_back(windowId);
        if (type == DockNodeType::Unknown || type == DockNodeType::Leaf) {
            type = dockedWindows.size() > 1 ? DockNodeType::TabContainer : DockNodeType::Leaf;
        }
    }
}

void DockNode::removeWindow(WidgetId windowId) {
    auto it = std::find(dockedWindows.begin(), dockedWindows.end(), windowId);
    if (it != dockedWindows.end()) {
        dockedWindows.erase(it);
        
        // Adjust selected tab if needed
        if (selectedTabIndex >= static_cast<int>(dockedWindows.size())) {
            selectedTabIndex = static_cast<int>(dockedWindows.size()) - 1;
        }
        if (selectedTabIndex < 0) {
            selectedTabIndex = 0;
        }
        
        // Update type
        if (dockedWindows.empty()) {
            type = DockNodeType::Leaf;
        } else if (dockedWindows.size() == 1) {
            type = DockNodeType::Leaf;
        }
    }
}

bool DockNode::hasWindow(WidgetId windowId) const {
    return std::find(dockedWindows.begin(), dockedWindows.end(), windowId) != dockedWindows.end();
}

//=============================================================================
// Split operations
//=============================================================================



DockNode* DockNode::splitNode(DockDirection direction, Id childId0, Id childId1, float ratio) {
    if (direction == DockDirection::None || direction == DockDirection::Center) {
        return nullptr;
    }
    
    // Create two new child nodes
    auto newChild0 = std::make_unique<DockNode>(childId0);
    auto newChild1 = std::make_unique<DockNode>(childId1);
    
    newChild0->parent = this;
    newChild1->parent = this;
    
    // Move current contents to appropriate child
    DockNode* existingContent = nullptr;
    DockNode* newContent = nullptr;
    
    if (direction == DockDirection::Left || direction == DockDirection::Top) {
        existingContent = newChild1.get();  // Existing goes to right/bottom
        newContent = newChild0.get();       // New goes to left/top
        splitRatio = ratio;
    } else {
        existingContent = newChild0.get();  // Existing goes to left/top
        newContent = newChild1.get();       // New goes to right/bottom
        splitRatio = 1.0f - ratio;
    }
    
    // Transfer windows to existing content child
    existingContent->dockedWindows = std::move(dockedWindows);
    existingContent->selectedTabIndex = selectedTabIndex;
    existingContent->type = existingContent->dockedWindows.size() > 1 
        ? DockNodeType::TabContainer 
        : DockNodeType::Leaf;
    
    // Clear this node's windows
    dockedWindows.clear();
    selectedTabIndex = 0;
    
    // Set new content as empty leaf
    newContent->type = DockNodeType::Leaf;
    
    // Set this node as split
    type = (direction == DockDirection::Left || direction == DockDirection::Right) 
        ? DockNodeType::SplitHorizontal 
        : DockNodeType::SplitVertical;
    
    children[0] = std::move(newChild0);
    children[1] = std::move(newChild1);
    
    return newContent;
}

void DockNode::mergeNodes() {
    // Only merge if this is a split node with one empty child
    if (!isSplitNode()) {
        return;
    }
    
    DockNode* nonEmptyChild = nullptr;
    int emptyCount = 0;
    
    for (int i = 0; i < 2; ++i) {
        if (children[i] && children[i]->isEmpty()) {
            emptyCount++;
        } else if (children[i]) {
            nonEmptyChild = children[i].get();
        }
    }
    
    // If one child is empty, absorb the non-empty child's contents
    if (emptyCount == 1 && nonEmptyChild) {
        // Move data into local variables first to avoid use-after-free
        // when children pointers are reset
        auto childWindows = std::move(nonEmptyChild->dockedWindows);
        int childSelected = nonEmptyChild->selectedTabIndex;
        DockNodeType childType = nonEmptyChild->type;
        auto grandchild0 = std::move(nonEmptyChild->children[0]);
        auto grandchild1 = std::move(nonEmptyChild->children[1]);
        
        // Reset children - this destroys nonEmptyChild
        children[0].reset();
        children[1].reset();
        
        // Apply to self
        dockedWindows = std::move(childWindows);
        selectedTabIndex = childSelected;
        type = childType;
        children[0] = std::move(grandchild0);
        children[1] = std::move(grandchild1);
        
        // Update parent pointers for grandchildren
        if (children[0]) children[0]->parent = this;
        if (children[1]) children[1]->parent = this;
    }
    
    // If both children are empty, become an empty leaf
    if (emptyCount == 2) {
        type = DockNodeType::Leaf;
        children[0].reset();
        children[1].reset();
    }
}

//=============================================================================
// Layout calculation
//=============================================================================

void DockNode::updateLayout(const Rect& availableBounds) {
    bounds = availableBounds;
    
    if (!isSplitNode()) {
        return;
    }
    
    // Calculate child bounds based on split direction and ratio
    Rect child0Bounds = getChildBounds(0);
    Rect child1Bounds = getChildBounds(1);
    
    if (children[0]) {
        children[0]->updateLayout(child0Bounds);
    }
    if (children[1]) {
        children[1]->updateLayout(child1Bounds);
    }
}

Rect DockNode::getChildBounds(int childIndex) const {
    if (childIndex < 0 || childIndex > 1 || !isSplitNode()) {
        return bounds;
    }
    
    const float splitterSize = 4.0f;
    
    if (type == DockNodeType::SplitHorizontal) {
        // Left-right split
        float splitX = bounds.x() + bounds.width() * splitRatio;
        
        if (childIndex == 0) {
            // Left child
            return Rect(bounds.x(), bounds.y(), 
                       splitX - bounds.x() - splitterSize * 0.5f, 
                       bounds.height());
        } else {
            // Right child
            return Rect(splitX + splitterSize * 0.5f, bounds.y(),
                       bounds.right() - splitX - splitterSize * 0.5f,
                       bounds.height());
        }
    } else {
        // Top-bottom split
        float splitY = bounds.y() + bounds.height() * splitRatio;
        
        if (childIndex == 0) {
            // Top child
            return Rect(bounds.x(), bounds.y(),
                       bounds.width(),
                       splitY - bounds.y() - splitterSize * 0.5f);
        } else {
            // Bottom child
            return Rect(bounds.x(), splitY + splitterSize * 0.5f,
                       bounds.width(),
                       bounds.bottom() - splitY - splitterSize * 0.5f);
        }
    }
}

//=============================================================================
// Traversal
//=============================================================================

void DockNode::forEachNode(const std::function<void(DockNode*)>& callback) {
    callback(this);
    
    for (int i = 0; i < 2; ++i) {
        if (children[i]) {
            children[i]->forEachNode(callback);
        }
    }
}

void DockNode::forEachLeaf(const std::function<void(DockNode*)>& callback) {
    if (isLeafNode() || (!children[0] && !children[1])) {
        callback(this);
    } else {
        for (int i = 0; i < 2; ++i) {
            if (children[i]) {
                children[i]->forEachLeaf(callback);
            }
        }
    }
}

//=============================================================================
// Debug
//=============================================================================

std::string DockNode::debugPrint(int depth) const {
    std::stringstream ss;
    std::string indent(depth * 2, ' ');
    
    ss << indent << "DockNode[" << id << "] ";
    
    switch (type) {
        case DockNodeType::Unknown: ss << "Unknown"; break;
        case DockNodeType::SplitHorizontal: ss << "SplitH"; break;
        case DockNodeType::SplitVertical: ss << "SplitV"; break;
        case DockNodeType::TabContainer: ss << "TabContainer"; break;
        case DockNodeType::Leaf: ss << "Leaf"; break;
    }
    
    ss << " bounds(" << bounds.x() << "," << bounds.y() 
       << "," << bounds.width() << "," << bounds.height() << ")";
    
    if (!dockedWindows.empty()) {
        ss << " windows[";
        for (size_t i = 0; i < dockedWindows.size(); ++i) {
            if (i > 0) ss << ",";
            ss << dockedWindows[i];
        }
        ss << "]";
    }
    
    ss << "\n";
    
    for (int i = 0; i < 2; ++i) {
        if (children[i]) {
            ss << children[i]->debugPrint(depth + 1);
        }
    }
    
    return ss.str();
}

} // namespace fst
