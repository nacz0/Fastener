#pragma once

#include "fastener/core/types.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace fst {

//=============================================================================
// DockNodeType - Type of dock node in the tree
//=============================================================================
enum class DockNodeType {
    Unknown,
    SplitHorizontal,  // Children side by side (left-right)
    SplitVertical,    // Children stacked (top-bottom)
    TabContainer,     // Children as tabs
    Leaf              // Single docked panel
};

//=============================================================================
// DockDirection - Direction for docking operations
//=============================================================================
enum class DockDirection {
    None,
    Left,
    Right,
    Top,
    Bottom,
    Center  // Tab docking
};

//=============================================================================
// DockNodeFlags - Configuration flags for dock nodes
//=============================================================================
struct DockNodeFlags {
    bool noSplit = false;           // Cannot be split
    bool noResize = false;          // Cannot resize with splitter
    bool noTabBar = false;          // Hide tab bar for single window
    bool keepAliveOnly = false;     // Keep alive when hidden
    bool passthruCentralNode = false; // For central empty area
};

//=============================================================================
// DockNode - Node in the dock tree hierarchy
//=============================================================================
class DockNode {
public:
    using Id = uint32_t;
    static constexpr Id INVALID_ID = 0;
    
    Id id = INVALID_ID;
    DockNodeType type = DockNodeType::Unknown;
    DockNodeFlags flags;
    
    // Tree structure
    DockNode* parent = nullptr;
    std::unique_ptr<DockNode> children[2];  // For split nodes
    
    // For TabContainer/Leaf nodes
    std::vector<WidgetId> dockedWindows;
    int selectedTabIndex = 0;
    
    // Layout
    Rect bounds;
    float splitRatio = 0.5f;  // Splitter position (0.0-1.0)
    
    // Constructors
    DockNode() = default;
    explicit DockNode(Id nodeId) : id(nodeId) {}
    
    // Tree queries
    bool isRootNode() const { return parent == nullptr; }
    bool isLeafNode() const { return type == DockNodeType::Leaf || type == DockNodeType::TabContainer; }
    bool isSplitNode() const { return type == DockNodeType::SplitHorizontal || type == DockNodeType::SplitVertical; }
    bool isEmpty() const { return dockedWindows.empty() && !children[0] && !children[1]; }
    
    // Window management
    DockNode* findNodeByWindowId(WidgetId windowId);
    const DockNode* findNodeByWindowId(WidgetId windowId) const;
    DockNode* findNodeById(Id nodeId);
    const DockNode* findNodeById(Id nodeId) const;
    
    void addWindow(WidgetId windowId);
    void removeWindow(WidgetId windowId);
    bool hasWindow(WidgetId windowId) const;
    
    // Split operations
    DockNode* splitNode(DockDirection direction, float ratio = 0.5f);
    void mergeNodes();
    
    // Layout calculation
    void updateLayout(const Rect& availableBounds);
    Rect getChildBounds(int childIndex) const;
    
    // Traversal
    void forEachNode(const std::function<void(DockNode*)>& callback);
    void forEachLeaf(const std::function<void(DockNode*)>& callback);
    
    // Debug
    std::string debugPrint(int depth = 0) const;
};

} // namespace fst
