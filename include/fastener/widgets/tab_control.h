#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>
#include <functional>

namespace fst {

//=============================================================================
// Tab Item
//=============================================================================
struct TabItem {
    std::string id;
    std::string label;
    std::string icon;           // Optional icon
    bool closable = true;       // Show close button
    bool modified = false;      // Show modified indicator (dot)
    void* userData = nullptr;   // User data
    
    TabItem() = default;
    TabItem(const std::string& id, const std::string& label, bool closable = true)
        : id(id), label(label), closable(closable) {}
};

//=============================================================================
// TabControl Options
//=============================================================================
struct TabControlOptions {
    Style style;
    float tabMinWidth = 80.0f;
    float tabMaxWidth = 200.0f;
    float tabHeight = 32.0f;
    bool showCloseButtons = true;
    bool allowReorder = true;      // Drag to reorder
    bool scrollable = true;        // Scroll when many tabs
    bool showAddButton = false;    // Show "+" button
};

//=============================================================================
// TabControl Events
//=============================================================================
struct TabControlEvents {
    std::function<void(int index, const TabItem&)> onSelect;
    std::function<void(int index, const TabItem&)> onClose;
    std::function<void(int fromIndex, int toIndex)> onReorder;
    std::function<void()> onAdd;   // When add button clicked
    std::function<void(int index, const TabItem&)> onContextMenu;
};

//=============================================================================
// TabControl Widget
//=============================================================================
class TabControl {
public:
    TabControl();
    ~TabControl();
    
    // Tab management
    void addTab(const TabItem& tab);
    void addTab(const std::string& id, const std::string& label, bool closable = true);
    void insertTab(int index, const TabItem& tab);
    void removeTab(int index);
    void removeTabById(const std::string& id);
    void clearTabs();
    
    // Access
    int tabCount() const { return static_cast<int>(m_tabs.size()); }
    TabItem* getTab(int index);
    TabItem* getTabById(const std::string& id);
    int findTabIndex(const std::string& id) const;
    
    // Selection
    int selectedIndex() const { return m_selectedIndex; }
    void selectTab(int index);
    void selectTabById(const std::string& id);
    TabItem* selectedTab();
    
    // Rendering - returns content area rect
    Rect render(const std::string& id, const Rect& bounds,
                const TabControlOptions& options = {},
                const TabControlEvents& events = {});
    
private:
    std::vector<TabItem> m_tabs;
    int m_selectedIndex = -1;
    float m_scrollOffset = 0.0f;
    int m_hoveredTab = -1;
    int m_hoveredClose = -1;
    int m_draggedTab = -1;
    float m_dragStartX = 0.0f;
    
    // Calculate tab widths
    std::vector<float> calculateTabWidths(float availableWidth, 
                                          const TabControlOptions& options) const;
    
    // Draw single tab
    void drawTab(int index, const Rect& tabRect, 
                 const TabControlOptions& options,
                 const TabControlEvents& events);
};

} // namespace fst
