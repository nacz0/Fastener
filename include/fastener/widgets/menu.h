#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace fst {

//=============================================================================
// MenuItem Types
//=============================================================================
enum class MenuItemType {
    Normal,
    Separator,
    Submenu,
    Checkbox,
    Radio
};

//=============================================================================
// MenuItem
//=============================================================================
struct MenuItem {
    std::string id;
    std::string label;
    std::string shortcut;       // e.g. "Ctrl+S"
    std::string icon;           // Optional icon
    MenuItemType type = MenuItemType::Normal;
    bool enabled = true;
    bool checked = false;       // For Checkbox/Radio
    int radioGroup = 0;         // For Radio grouping
    
    std::vector<std::shared_ptr<MenuItem>> children;  // For Submenu
    std::function<void()> action;
    
    MenuItem() = default;
    MenuItem(const std::string& id, const std::string& label, 
             std::function<void()> action = nullptr)
        : id(id), label(label), action(action) {}
    
    // Static factory methods
    static MenuItem separator() {
        MenuItem item;
        item.type = MenuItemType::Separator;
        return item;
    }
    
    static MenuItem submenu(const std::string& id, const std::string& label) {
        MenuItem item(id, label);
        item.type = MenuItemType::Submenu;
        return item;
    }
    
    static MenuItem checkbox(const std::string& id, const std::string& label, 
                             bool checked, std::function<void()> action = nullptr) {
        MenuItem item(id, label, action);
        item.type = MenuItemType::Checkbox;
        item.checked = checked;
        return item;
    }
    
    // Add child (for submenus)
    MenuItem& add(const MenuItem& child) {
        children.push_back(std::make_shared<MenuItem>(child));
        return *this;
    }
    
    // Chainable setters
    MenuItem& withShortcut(const std::string& sc) { shortcut = sc; return *this; }
    MenuItem& withIcon(const std::string& ic) { icon = ic; return *this; }
    MenuItem& disabled() { enabled = false; return *this; }
};

//=============================================================================
// MenuBar - Horizontal menu bar at top of window
//=============================================================================
class MenuBar {
public:
    MenuBar();
    ~MenuBar();
    
    // Add top-level menu
    void addMenu(const std::string& label, const std::vector<MenuItem>& items);
    void clear();
    
    // Render - returns height of menu bar
    float render(const Rect& bounds);
    
    // Render popups (call at end of frame, after all other rendering)
    void renderPopups();
    
    // Check if menu is open
    bool isOpen() const { return m_openMenuIndex >= 0; }
    void closeAll() { m_openMenuIndex = -1; }
    
private:
    struct TopMenu {
        std::string label;
        std::vector<MenuItem> items;
        float x = 0;
        float width = 0;
    };
    
    std::vector<TopMenu> m_menus;
    int m_openMenuIndex = -1;
    int m_hoveredIndex = -1;
    float m_dropdownY = 0;
    
    // Submenu tracking
    struct OpenSubmenu {
        int itemIndex = -1;
        Rect bounds;
    };
    int m_activeSubmenuIndex = -1;
    Rect m_activeSubmenuBounds;
    
    void renderDropdown(const TopMenu& menu, const Vec2& pos);
    void renderSubmenu(const std::vector<std::shared_ptr<MenuItem>>& items, const Vec2& pos);
};

//=============================================================================
// ContextMenu - Popup menu shown on right-click
//=============================================================================
class ContextMenu {
public:
    ContextMenu();
    ~ContextMenu();
    
    // Set menu items
    void setItems(const std::vector<MenuItem>& items);
    void clear();
    
    // Show at position
    void show(const Vec2& position);
    void hide();
    bool isVisible() const { return m_visible; }
    
    // Render - call every frame when visible
    void render();
    
    // Get selected item (after click)
    const MenuItem* selectedItem() const { return m_selectedItem; }
    
private:
    std::vector<MenuItem> m_items;
    Vec2 m_position;
    bool m_visible = false;
    int m_hoveredIndex = -1;
    int m_openSubmenu = -1;
    const MenuItem* m_selectedItem = nullptr;
    
    float renderItems(const std::vector<MenuItem>& items, const Vec2& pos, 
                      int depth = 0);
    void renderSubmenu(const std::vector<std::shared_ptr<MenuItem>>& items, const Vec2& pos);
};

//=============================================================================
// Global context menu helper
//=============================================================================
void ShowContextMenu(const std::vector<MenuItem>& items, const Vec2& position);
void RenderContextMenu();  // Call at end of frame
bool IsContextMenuOpen();
void CloseContextMenu();

// Input blocking
bool IsMouseOverAnyMenu();

} // namespace fst
