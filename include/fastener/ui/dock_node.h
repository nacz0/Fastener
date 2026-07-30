#pragma once

#include "fastener/core/types.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace fst {

class DockBuilder;
class DockContext;

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

    // Constructors
    DockNode() = default;
    explicit DockNode(Id nodeId) : m_id(nodeId) {}

    Id id() const { return m_id; }
    DockNodeType type() const { return m_type; }
    const DockNodeFlags& flags() const { return m_flags; }
    DockNode* parent() { return m_parent; }
    const DockNode* parent() const { return m_parent; }
    DockNode* child(int index);
    const DockNode* child(int index) const;
    const std::vector<WidgetId>& windows() const { return m_dockedWindows; }
    int selectedTabIndex() const { return m_selectedTabIndex; }
    const Rect& bounds() const { return m_bounds; }
    float splitRatio() const { return m_splitRatio; }
    bool selectTab(int index);
    bool setSplitRatio(float ratio);

    // Tree queries
    bool isRootNode() const { return m_parent == nullptr; }
    bool isLeafNode() const {
        return m_type == DockNodeType::Leaf ||
               m_type == DockNodeType::TabContainer;
    }
    bool isSplitNode() const {
        return m_type == DockNodeType::SplitHorizontal ||
               m_type == DockNodeType::SplitVertical;
    }
    bool isEmpty() const {
        return m_dockedWindows.empty() && !m_children[0] && !m_children[1];
    }

    // Window management
    DockNode* findNodeByWindowId(WidgetId windowId);
    const DockNode* findNodeByWindowId(WidgetId windowId) const;
    DockNode* findNodeById(Id nodeId);
    const DockNode* findNodeById(Id nodeId) const;

    bool hasWindow(WidgetId windowId) const;

    // Layout calculation
    void updateLayout(const Rect& availableBounds);
    Rect getChildBounds(int childIndex) const;

    // Traversal
    void forEachNode(const std::function<void(DockNode*)>& callback);
    void forEachLeaf(const std::function<void(DockNode*)>& callback);

    // Debug
    std::string debugPrint(int depth = 0) const;

private:
    friend class DockBuilder;
    friend class DockContext;

    Id m_id = INVALID_ID;
    DockNodeType m_type = DockNodeType::Unknown;
    DockNodeFlags m_flags;
    DockNode* m_parent = nullptr;
    std::unique_ptr<DockNode> m_children[2];
    std::vector<WidgetId> m_dockedWindows;
    int m_selectedTabIndex = 0;
    Rect m_bounds;
    float m_splitRatio = 0.5f;

    void addWindow(WidgetId windowId);
    void removeWindow(WidgetId windowId);
    DockNode* splitNode(
        DockDirection direction,
        Id childId0,
        Id childId1,
        float ratio = 0.5f);
    void mergeNodes();
};

} // namespace fst
