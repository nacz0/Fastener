#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace fst {

//=============================================================================
// TreeNode - Represents a single node in the tree
//=============================================================================
struct TreeNode {
    std::string id;
    std::string label;
    std::string icon;           // Optional icon identifier
    bool isExpanded = false;
    bool isSelected = false;
    bool isLeaf = false;        // No children possible
    void* userData = nullptr;   // User-attached data
    
    std::vector<std::shared_ptr<TreeNode>> children;
    TreeNode* parent = nullptr;
    
    TreeNode() = default;
    TreeNode(const std::string& id, const std::string& label, bool isLeaf = false)
        : id(id), label(label), isLeaf(isLeaf) {}
    
    // Add a child node
    std::shared_ptr<TreeNode> addChild(const std::string& childId, 
                                        const std::string& childLabel, 
                                        bool childIsLeaf = false) {
        auto child = std::make_shared<TreeNode>(childId, childLabel, childIsLeaf);
        child->parent = this;
        children.push_back(child);
        return child;
    }
    
    // Find node by ID (recursive)
    TreeNode* findById(const std::string& searchId) {
        if (id == searchId) return this;
        for (auto& child : children) {
            if (auto found = child->findById(searchId)) {
                return found;
            }
        }
        return nullptr;
    }
    
    // Check if node has children
    bool hasChildren() const { return !children.empty(); }
    
    // Get depth in tree
    int depth() const {
        int d = 0;
        TreeNode* p = parent;
        while (p) {
            d++;
            p = p->parent;
        }
        return d;
    }
};

//=============================================================================
// TreeView Options
//=============================================================================
struct TreeViewOptions {
    Style style;
    float indentWidth = 20.0f;
    float rowHeight = 24.0f;
    bool showLines = false;         // Show connecting lines
    bool showIcons = true;          // Show folder/file icons
    bool multiSelect = false;       // Allow multiple selection
    bool dragDrop = false;          // Enable drag and drop
};

//=============================================================================
// TreeView Events
//=============================================================================
struct TreeViewEvents {
    std::function<void(TreeNode*)> onSelect;
    std::function<void(TreeNode*)> onDoubleClick;
    std::function<void(TreeNode*)> onExpand;
    std::function<void(TreeNode*)> onCollapse;
    std::function<void(TreeNode*, TreeNode*)> onDrop;  // source, target
    std::function<void(TreeNode*)> onContextMenu;
};

//=============================================================================
// TreeView Widget
//=============================================================================
class TreeView {
public:
    TreeView();
    ~TreeView();
    
    // Root node access
    TreeNode* root() { return m_root.get(); }
    const TreeNode* root() const { return m_root.get(); }
    
    // Set a new root
    void setRoot(std::shared_ptr<TreeNode> root);
    
    // Selection
    TreeNode* selectedNode() const { return m_selectedNode; }
    void selectNode(TreeNode* node);
    void clearSelection();
    
    // Expansion
    void expandAll();
    void collapseAll();
    void expandTo(TreeNode* node);  // Expand all parents to make node visible
    
    // Scrolling
    void scrollToNode(TreeNode* node);
    
    // Rendering
    void render(const std::string& id, const Rect& bounds, 
                const TreeViewOptions& options = {},
                const TreeViewEvents& events = {});
    
private:
    std::shared_ptr<TreeNode> m_root;
    TreeNode* m_selectedNode = nullptr;
    TreeNode* m_hoveredNode = nullptr;
    float m_scrollY = 0.0f;
    float m_contentHeight = 0.0f;
    
    // Internal rendering
    float renderNode(TreeNode* node, const Rect& bounds, float y,
                     const TreeViewOptions& options,
                     const TreeViewEvents& events);
    
    // Helper to draw expand/collapse arrow
    void drawExpandArrow(const Vec2& pos, bool expanded, Color color);
    
    // Helper to draw folder/file icon
    void drawIcon(const Vec2& pos, bool isFolder, bool isExpanded, Color color);
};

// Convenience function for simple tree rendering
void TreeViewSimple(const std::string& id, TreeNode* root, const Rect& bounds,
                    std::function<void(TreeNode*)> onSelect = nullptr,
                    const TreeViewOptions& options = {});

} // namespace fst
