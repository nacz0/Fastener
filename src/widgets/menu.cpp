/**
 * @file menu.cpp
 * @brief Menu bar and context menu widget implementations.
 */

#include "fastener/widgets/menu.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include <algorithm>
#include <cmath>

namespace fst {

// Local context menu instance (data storage, not shared state)
static ContextMenu g_contextMenu;

// Helper to access menu state from current Context
static Context::MenuState& getMenuState(Context& ctx) {
    return ctx.menuState();
}


//=============================================================================
// MenuBar
//=============================================================================
MenuBar::MenuBar() = default;
MenuBar::~MenuBar() = default;

void MenuBar::addMenu(std::string_view label, const std::vector<MenuItem>& items) {
    TopMenu menu;
    menu.label = std::string(label);
    menu.items = items;
    m_menus.push_back(menu);
}

void MenuBar::clear() {
    m_menus.clear();
    m_openMenuIndex = -1;
}

float MenuBar::render(Context& ctx, const Rect& bounds) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();

    
    float barHeight = 28.0f;
    Rect barRect(bounds.x(), bounds.y(), bounds.width(), barHeight);
    
    // Draw bar background
    dl.addRectFilled(barRect, theme.colors.panelBackground);
    dl.addRectFilled(Rect(bounds.x(), barRect.bottom() - 1, bounds.width(), 1), 
                     theme.colors.border);
    
    // Reset hover
    m_hoveredIndex = -1;
    
    // Calculate menu positions
    float x = bounds.x() + 4;
    for (size_t i = 0; i < m_menus.size(); i++) {
        auto& menu = m_menus[i];
        float textWidth = font ? font->measureText(menu.label).x : 50.0f;
        menu.width = textWidth + 20.0f;
        menu.x = x;
        
        Rect menuRect(x, bounds.y(), menu.width, barHeight);
        bool isHovered = menuRect.contains(ctx.input().mousePos());

        bool isOpen = (static_cast<int>(i) == m_openMenuIndex);
        
        if (isHovered) {
            m_hoveredIndex = static_cast<int>(i);
        }
        
        // Background
        if (isOpen) {
            dl.addRectFilled(menuRect, theme.colors.selection);
        } else if (isHovered) {
            dl.addRectFilled(menuRect, theme.colors.buttonHover);
        }
        
        // Label
        if (font) {
            Color textColor = isOpen ? theme.colors.selectionText : theme.colors.text;
            Vec2 textPos(x + 10, bounds.y() + (barHeight - font->lineHeight()) / 2);
            dl.addText(font, textPos, menu.label, textColor);
        }
        
        // Handle click
        if (isHovered && ctx.input().isMousePressed(MouseButton::Left)) {

            if (isOpen) {
                m_openMenuIndex = -1;
            } else {
                m_openMenuIndex = static_cast<int>(i);
            }
        }
        
        // Open on hover if another menu is open
        if (isHovered && m_openMenuIndex >= 0 && !isOpen) {
            m_openMenuIndex = static_cast<int>(i);
        }
        
        x += menu.width;
    }
    
    // Store dropdown position for deferred rendering
    m_dropdownY = barRect.bottom();
    
    // Update global state for input blocking
    getMenuState(ctx).menuBarOpen = m_openMenuIndex >= 0;
    if (!getMenuState(ctx).menuBarOpen) {
        getMenuState(ctx).menuBarDropdownRect = Rect();
    }
    
    // Close on click outside
    if (m_openMenuIndex >= 0 && ctx.input().isMousePressed(MouseButton::Left)) {
        if (m_hoveredIndex < 0 && !getMenuState(ctx).menuBarDropdownRect.contains(ctx.input().mousePos())) {
            m_openMenuIndex = -1;
        }
    }

    
    return barHeight;
}

void MenuBar::renderPopups(Context& ctx) {
    if (m_openMenuIndex >= 0 && m_openMenuIndex < static_cast<int>(m_menus.size())) {
        auto& menu = m_menus[m_openMenuIndex];
        renderDropdown(ctx, menu, Vec2(menu.x, m_dropdownY));
    }
}

void MenuBar::renderDropdown(Context& ctx, const TopMenu& menu, const Vec2& pos) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();
    
    float itemHeight = 28.0f;
    float separatorHeight = 9.0f;
    float minWidth = 180.0f;
    float padding = 8.0f;
    
    // Calculate dropdown size
    float maxWidth = minWidth;
    float totalHeight = padding;
    
    // Dim background to obscure content underneath
    dl.setTexture(0);
    dl.addRectFilled(fst::Rect(0.0f, 0.0f, ctx.input().windowSize().x, ctx.input().windowSize().y), 
                     fst::Color(0, 0, 0, 40)); 
    
    for (const auto& item : menu.items) {
        if (item.type == MenuItemType::Separator) {
            totalHeight += separatorHeight;
        } else {
            totalHeight += itemHeight;
            if (font) {
                float textWidth = font->measureText(item.label).x;
                if (!item.shortcut.empty()) {
                    textWidth += font->measureText(item.shortcut).x + 40;
                }
                maxWidth = std::max(maxWidth, textWidth + 50);
            }
        }
    }
    totalHeight += padding;
    
    Rect dropdownRect(pos.x, pos.y, maxWidth, totalHeight);
    
    // Update global dropdown rect for occlusion testing
    getMenuState(ctx).menuBarDropdownRect = dropdownRect;
    ctx.addFloatingWindowRect(dropdownRect);

    
    // Reset texture to white texture for solid color background
    dl.setTexture(0);
    
    // Shadow
    dl.addShadow(dropdownRect, theme.colors.shadow, 8.0f, 4.0f);
    
    // Background - solid opaque fill
    dl.setTexture(0);
    dl.addRectFilled(dropdownRect, theme.colors.popupBackground, 4.0f);
    dl.addRect(dropdownRect, theme.colors.border, 4.0f);
    
    // Items
    float y = pos.y + padding;
    
    for (size_t i = 0; i < menu.items.size(); i++) {
        auto& item = const_cast<MenuItem&>(menu.items[i]);
        
        // Read checked state from pointer if bound
        if (item.type == MenuItemType::Checkbox && item.checkedPtr) {
            item.checked = *item.checkedPtr;
        }
        
        if (item.type == MenuItemType::Separator) {
            float sepY = y + separatorHeight / 2;
            dl.addRectFilled(Rect(pos.x + 8, sepY, maxWidth - 16, 1), theme.colors.border);
            y += separatorHeight;
            continue;
        }
        
        Rect itemRect(pos.x, y, maxWidth, itemHeight);
        bool isHovered = itemRect.contains(ctx.input().mousePos());

        
        // Hover background
        if (isHovered && item.enabled) {
            Rect hoverRect(pos.x + 2, y, maxWidth - 4, itemHeight);
            dl.addRectFilled(hoverRect, theme.colors.selection, 2.0f);
        }
        
        Color textColor = item.enabled ? 
            (isHovered ? theme.colors.selectionText : theme.colors.text) :
            theme.colors.textDisabled;
        
        float textX = pos.x + 30;
        
        // Checkbox/Radio indicator
        if (item.type == MenuItemType::Checkbox && item.checked) {
            float checkX = pos.x + 12;
            float checkY = y + itemHeight / 2;
            dl.addLine(Vec2(checkX - 3, checkY), Vec2(checkX, checkY + 3), textColor, 1.5f);
            dl.addLine(Vec2(checkX, checkY + 3), Vec2(checkX + 5, checkY - 4), textColor, 1.5f);
        }
        
        // Label
        if (font) {
            Vec2 textPos(textX, y + (itemHeight - font->lineHeight()) / 2);
            dl.addText(font, textPos, item.label, textColor);
            
            // Shortcut
            if (!item.shortcut.empty()) {
                float scWidth = font->measureText(item.shortcut).x;
                Vec2 scPos(pos.x + maxWidth - scWidth - 12, textPos.y);
                dl.addText(font, scPos, item.shortcut, theme.colors.textSecondary);
            }
        }
        
        // Handle submenu hover
        if (isHovered && item.type == MenuItemType::Submenu && item.enabled) {
            m_activeSubmenuIndex = static_cast<int>(i);
            m_activeSubmenuBounds = Rect(pos.x + maxWidth - 2, y, 0, 0); // Temporary
        } else if (isHovered && item.type != MenuItemType::Submenu) {
            // Clear submenu when hovering over non-submenu item
            m_activeSubmenuIndex = -1;
        }
        
        // Handle click
        if (isHovered && item.enabled && ctx.input().isMousePressed(MouseButton::Left)) {
            if (item.type != MenuItemType::Submenu) {
                // Toggle checkbox if clicked
                if (item.type == MenuItemType::Checkbox) {
                    item.checked = !item.checked;
                    if (item.checkedPtr) {
                        *item.checkedPtr = item.checked;
                    }
                }
                if (item.action) {
                    item.action();
                }
                m_openMenuIndex = -1;  // Close menu
            }
        }

        
        y += itemHeight;
    }
    
    // Render submenu if active
    if (m_activeSubmenuIndex >= 0 && m_activeSubmenuIndex < static_cast<int>(menu.items.size())) {
        const auto& item = menu.items[m_activeSubmenuIndex];
        if (item.type == MenuItemType::Submenu && !item.children.empty()) {
            renderSubmenu(ctx, item.children, m_activeSubmenuBounds.topLeft());
        }
    }

    
    // Clear submenu if mouse is far away
    if (!dropdownRect.expanded(10).contains(ctx.input().mousePos()) && 
        !m_activeSubmenuBounds.expanded(200).contains(ctx.input().mousePos())) {
        m_activeSubmenuIndex = -1;
    }
}

void MenuBar::renderSubmenu(Context& ctx, const std::vector<std::shared_ptr<MenuItem>>& items, const Vec2& pos) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();

    
    float itemHeight = 28.0f;
    float minWidth = 160.0f;
    float padding = 8.0f;
    
    float maxWidth = minWidth;
    float totalHeight = padding + items.size() * itemHeight + padding;
    
    for (const auto& item : items) {
        if (font) {
            float textWidth = font->measureText(item->label).x;
            maxWidth = std::max(maxWidth, textWidth + 50);
        }
    }
    
    Rect subRect(pos.x, pos.y, maxWidth, totalHeight);
    m_activeSubmenuBounds = subRect; // Update bounds for hover persistence
    ctx.addFloatingWindowRect(subRect);

    
    dl.setTexture(0);
    dl.addShadow(subRect, theme.colors.shadow, 8.0f, 4.0f);
    dl.addRectFilled(subRect, theme.colors.popupBackground, 4.0f);
    dl.addRect(subRect, theme.colors.border, 4.0f);
    
    float y = pos.y + padding;
    for (const auto& item : items) {
        Rect itemRect(pos.x + 2, y, maxWidth - 4, itemHeight);
        bool isHovered = itemRect.contains(ctx.input().mousePos());
        
        if (isHovered && item->enabled) {
            dl.addRectFilled(itemRect, theme.colors.selection, 2.0f);
        }
        
        Color textColor = item->enabled ? 
            (isHovered ? theme.colors.selectionText : theme.colors.text) :
            theme.colors.textDisabled;
        
        if (font) {
            dl.addText(font, Vec2(pos.x + 30, y + (itemHeight - font->lineHeight()) / 2), 
                       item->label, textColor);
        }
        
        if (isHovered && item->enabled && ctx.input().isMousePressed(MouseButton::Left)) {
            if (item->action) item->action();
            m_openMenuIndex = -1;
        }

        
        y += itemHeight;
    }
}

//=============================================================================
// ContextMenu
//=============================================================================
ContextMenu::ContextMenu() = default;
ContextMenu::~ContextMenu() = default;

void ContextMenu::setItems(const std::vector<MenuItem>& items) {
    m_items = items;
}

void ContextMenu::clear() {
    m_items.clear();
    m_visible = false;
}

void ContextMenu::show(const Vec2& position) {
    m_position = position;
    m_visible = true;
    m_hoveredIndex = -1;
    m_selectedItem = nullptr;
}

void ContextMenu::hide() {
    m_visible = false;
    m_selectedItem = nullptr;
}

void ContextMenu::render(Context& ctx) {
    if (!m_visible) return;
    
    renderItems(ctx, m_items, m_position, 0);
    
    // Close on click outside (simple check)
    if (ctx.input().isMousePressed(MouseButton::Left) || 
        ctx.input().isMousePressed(MouseButton::Right)) {
        // Check if any item was clicked - if not, close
        // For now, close after any click (items handle their own clicks)
    }
    
    // Close on escape
    if (ctx.input().isKeyPressed(Key::Escape)) {
        hide();
    }
}

float ContextMenu::renderItems(Context& ctx, const std::vector<MenuItem>& items, const Vec2& pos, int depth) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();

    
    float itemHeight = 26.0f;
    float separatorHeight = 7.0f;
    float minWidth = 160.0f;
    float padding = 4.0f;
    
    // Calculate size
    float maxWidth = minWidth;
    float totalHeight = padding;
    
    for (const auto& item : items) {
        if (item.type == MenuItemType::Separator) {
            totalHeight += separatorHeight;
        } else {
            totalHeight += itemHeight;
            if (font) {
                float textWidth = font->measureText(item.label).x;
                if (!item.shortcut.empty()) {
                    textWidth += font->measureText(item.shortcut).x + 30;
                }
                maxWidth = std::max(maxWidth, textWidth + 45);
            }
        }
    }
    totalHeight += padding;
    
    Rect menuRect(pos.x, pos.y, maxWidth, totalHeight);
    if (depth == 0) {
        getMenuState(ctx).contextMenuRect = menuRect;
    }
    ctx.addFloatingWindowRect(menuRect);

    
    // Dim background
    if (depth == 0) {
        dl.setTexture(0);
        dl.addRectFilled(fst::Rect(0.0f, 0.0f, ctx.input().windowSize().x, ctx.input().windowSize().y), 
                         fst::Color(0, 0, 0, 40));
    }

    
    // Reset texture to solid
    dl.setTexture(0);
    
    // Shadow & background
    dl.addShadow(menuRect, theme.colors.shadow, 6.0f, 4.0f);
    dl.addRectFilled(menuRect, theme.colors.popupBackground, 4.0f);
    dl.addRect(menuRect, theme.colors.border, 4.0f);
    
    // Items
    float y = pos.y + padding;
    bool anyHovered = false;
    
    for (size_t i = 0; i < items.size(); i++) {
        const auto& item = items[i];
        
        if (item.type == MenuItemType::Separator) {
            float sepY = y + separatorHeight / 2;
            dl.addRectFilled(Rect(pos.x + 6, sepY, maxWidth - 12, 1), theme.colors.border);
            y += separatorHeight;
            continue;
        }
        
        Rect itemRect(pos.x + 2, y, maxWidth - 4, itemHeight);
        bool isHovered = itemRect.contains(ctx.input().mousePos());

        
        if (isHovered && item.enabled) {
            anyHovered = true;
            dl.addRectFilled(itemRect, theme.colors.selection, 2.0f);
        }
        
        Color textColor = item.enabled ? 
            (isHovered ? theme.colors.selectionText : theme.colors.text) :
            theme.colors.textDisabled;
        
        float textX = pos.x + 28;
        
        // Checkbox indicator
        if (item.type == MenuItemType::Checkbox && item.checked) {
            float checkX = pos.x + 12;
            float checkY = y + itemHeight / 2;
            dl.addLine(Vec2(checkX - 3, checkY), Vec2(checkX, checkY + 2), textColor, 1.5f);
            dl.addLine(Vec2(checkX, checkY + 2), Vec2(checkX + 4, checkY - 3), textColor, 1.5f);
        }
        
        // Label
        if (font) {
            Vec2 textPos(textX, y + (itemHeight - font->lineHeight()) / 2);
            dl.addText(font, textPos, item.label, textColor);
            
            // Shortcut
            if (!item.shortcut.empty()) {
                float scWidth = font->measureText(item.shortcut).x;
                Vec2 scPos(pos.x + maxWidth - scWidth - 10, textPos.y);
                dl.addText(font, scPos, item.shortcut, theme.colors.textSecondary);
            }
        }
        
        // Submenu arrow
        if (item.type == MenuItemType::Submenu && !item.children.empty()) {
            float arrowX = pos.x + maxWidth - 12;
            float arrowY = y + itemHeight / 2;
            Vec2 p1(arrowX, arrowY - 3);
            Vec2 p2(arrowX + 4, arrowY);
            Vec2 p3(arrowX, arrowY + 3);
            dl.addTriangleFilled(p1, p2, p3, textColor);
        }
        
        // Handle submenu hover
        if (isHovered && item.type == MenuItemType::Submenu && item.enabled) {
            m_openSubmenu = static_cast<int>(i);
        }
        
        // Handle click
        if (isHovered && item.enabled && ctx.input().isMousePressed(MouseButton::Left)) {

            if (item.type != MenuItemType::Submenu) {
                m_selectedItem = &item;
                if (item.action) {
                    item.action();
                }
                hide();
            }
        }
        
        y += itemHeight;
    }
    
    // Render submenu
    if (m_openSubmenu >= 0 && m_openSubmenu < static_cast<int>(items.size())) {
        const auto& item = items[m_openSubmenu];
        if (item.type == MenuItemType::Submenu && !item.children.empty()) {
            renderSubmenu(ctx, item.children, Vec2(pos.x + maxWidth - 5, pos.y + padding + m_openSubmenu * itemHeight));
        }
    }
    
    // Close if clicked outside
    if (!anyHovered && !menuRect.contains(ctx.input().mousePos())) {
        if (ctx.input().isMousePressed(MouseButton::Left)) {
            hide();
        }
    }

    
    return totalHeight;
}

void ContextMenu::renderSubmenu(Context& ctx, const std::vector<std::shared_ptr<MenuItem>>& items, const Vec2& pos) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();

    
    float itemHeight = 26.0f;
    float padding = 4.0f;
    float minWidth = 160.0f;
    
    float maxWidth = minWidth;
    float totalHeight = padding + items.size() * itemHeight + padding;
    
    for (const auto& item : items) {
        if (font) {
            float textWidth = font->measureText(item->label).x;
            maxWidth = std::max(maxWidth, textWidth + 45);
        }
    }
    
    Rect subRect(pos.x, pos.y, maxWidth, totalHeight);
    ctx.addFloatingWindowRect(subRect);

    
    dl.setTexture(0);
    dl.addShadow(subRect, theme.colors.shadow, 6.0f, 4.0f);
    dl.addRectFilled(subRect, theme.colors.popupBackground, 4.0f);
    dl.addRect(subRect, theme.colors.border, 4.0f);
    
    float y = pos.y + padding;
    for (const auto& item : items) {
        Rect itemRect(pos.x + 2, y, maxWidth - 4, itemHeight);
        bool isHovered = itemRect.contains(ctx.input().mousePos());

        
        if (isHovered && item->enabled) {
            dl.addRectFilled(itemRect, theme.colors.selection, 2.0f);
        }
        
        Color textColor = item->enabled ? 
            (isHovered ? theme.colors.selectionText : theme.colors.text) :
            theme.colors.textDisabled;
        
        if (font) {
            dl.addText(font, Vec2(pos.x + 28, y + (itemHeight - font->lineHeight()) / 2), 
                       item->label, textColor);
        }
        
        if (isHovered && item->enabled && ctx.input().isMousePressed(MouseButton::Left)) {

            if (item->action) item->action();
            hide();
        }
        
        y += itemHeight;
    }
}

//=============================================================================
// Global helpers
//=============================================================================
void ShowContextMenu(Context& ctx, const std::vector<MenuItem>& items, const Vec2& position) {
    g_contextMenu.setItems(items);
    g_contextMenu.show(position);
    getMenuState(ctx).contextMenuActive = true;
}

void RenderContextMenu(Context& ctx) {
    if (getMenuState(ctx).contextMenuActive) {
        g_contextMenu.render(ctx);
        if (!g_contextMenu.isVisible()) {
            getMenuState(ctx).contextMenuActive = false;
        }
    }
}

bool IsContextMenuOpen(Context& ctx) {
    return getMenuState(ctx).contextMenuActive && g_contextMenu.isVisible();
}

void CloseContextMenu(Context& ctx) {
    g_contextMenu.hide();
    getMenuState(ctx).contextMenuActive = false;
    getMenuState(ctx).contextMenuRect = Rect();
}


bool IsMouseOverAnyMenu(Context& ctx) {
    Vec2 mousePos = ctx.input().mousePos();
    
    if (getMenuState(ctx).menuBarOpen && getMenuState(ctx).menuBarDropdownRect.contains(mousePos)) {
        return true;
    }
    
    if (getMenuState(ctx).contextMenuActive && getMenuState(ctx).contextMenuRect.contains(mousePos)) {
        return true;
    }
    
    // Also block if mouse is over the menu bar itself (y < 28)
    if (mousePos.y < 28.0f) {
        return true;
    }
    
    return false;
}


} // namespace fst
