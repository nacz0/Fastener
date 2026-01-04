#include "fastener/widgets/menu.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include <algorithm>
#include <cmath>

namespace fst {

// Global context menu state
static ContextMenu g_contextMenu;
static bool g_contextMenuActive = false;

//=============================================================================
// MenuBar
//=============================================================================
MenuBar::MenuBar() = default;
MenuBar::~MenuBar() = default;

void MenuBar::addMenu(const std::string& label, const std::vector<MenuItem>& items) {
    TopMenu menu;
    menu.label = label;
    menu.items = items;
    m_menus.push_back(menu);
}

void MenuBar::clear() {
    m_menus.clear();
    m_openMenuIndex = -1;
}

float MenuBar::render(const Rect& bounds) {
    Context* ctx = Context::current();
    if (!ctx) return 0;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    Font* font = ctx->font();
    
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
        bool isHovered = menuRect.contains(ctx->input().mousePos());
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
        if (isHovered && ctx->input().isMousePressed(MouseButton::Left)) {
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
    
    // Render open dropdown
    if (m_openMenuIndex >= 0 && m_openMenuIndex < static_cast<int>(m_menus.size())) {
        auto& menu = m_menus[m_openMenuIndex];
        renderDropdown(menu, Vec2(menu.x, barRect.bottom()));
    }
    
    // Close on click outside
    if (m_openMenuIndex >= 0 && ctx->input().isMousePressed(MouseButton::Left)) {
        if (m_hoveredIndex < 0) {
            // Check if click is in dropdown area
            // For now, simple check
            Vec2 mousePos = ctx->input().mousePos();
            if (mousePos.y < barRect.bottom()) {
                // Already handled above
            } else {
                // Will close after action is taken
            }
        }
    }
    
    return barHeight;
}

void MenuBar::renderDropdown(const TopMenu& menu, const Vec2& pos) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    Font* font = ctx->font();
    
    float itemHeight = 28.0f;
    float separatorHeight = 9.0f;
    float minWidth = 180.0f;
    float padding = 8.0f;
    
    // Calculate dropdown size
    float maxWidth = minWidth;
    float totalHeight = padding;
    
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
    
    // Shadow
    dl.addShadow(dropdownRect, theme.colors.shadow, 8.0f, 4.0f);
    
    // Background
    dl.addRectFilled(dropdownRect, theme.colors.popupBackground, 4.0f);
    dl.addRect(dropdownRect, theme.colors.border, 4.0f);
    
    // Items
    float y = pos.y + padding;
    
    for (size_t i = 0; i < menu.items.size(); i++) {
        const auto& item = menu.items[i];
        
        if (item.type == MenuItemType::Separator) {
            float sepY = y + separatorHeight / 2;
            dl.addRectFilled(Rect(pos.x + 8, sepY, maxWidth - 16, 1), theme.colors.border);
            y += separatorHeight;
            continue;
        }
        
        Rect itemRect(pos.x, y, maxWidth, itemHeight);
        bool isHovered = itemRect.contains(ctx->input().mousePos());
        
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
        
        // Submenu arrow
        if (item.type == MenuItemType::Submenu) {
            float arrowX = pos.x + maxWidth - 15;
            float arrowY = y + itemHeight / 2;
            Vec2 p1(arrowX, arrowY - 4);
            Vec2 p2(arrowX + 5, arrowY);
            Vec2 p3(arrowX, arrowY + 4);
            dl.addTriangleFilled(p1, p2, p3, textColor);
        }
        
        // Handle click
        if (isHovered && item.enabled && ctx->input().isMousePressed(MouseButton::Left)) {
            if (item.action) {
                item.action();
            }
            m_openMenuIndex = -1;  // Close menu
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

void ContextMenu::render() {
    if (!m_visible) return;
    
    Context* ctx = Context::current();
    if (!ctx) return;
    
    renderItems(m_items, m_position, 0);
    
    // Close on click outside (simple check)
    if (ctx->input().isMousePressed(MouseButton::Left) || 
        ctx->input().isMousePressed(MouseButton::Right)) {
        // Check if any item was clicked - if not, close
        // For now, close after any click (items handle their own clicks)
    }
    
    // Close on escape
    if (ctx->input().isKeyPressed(Key::Escape)) {
        hide();
    }
}

float ContextMenu::renderItems(const std::vector<MenuItem>& items, const Vec2& pos, int depth) {
    Context* ctx = Context::current();
    if (!ctx) return 0;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    Font* font = ctx->font();
    
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
    
    // Clamp to screen (simple)
    // TODO: Get screen bounds
    
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
        bool isHovered = itemRect.contains(ctx->input().mousePos());
        
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
        
        // Handle click
        if (isHovered && item.enabled && ctx->input().isMousePressed(MouseButton::Left)) {
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
    
    // Close if clicked outside
    if (!anyHovered && !menuRect.contains(ctx->input().mousePos())) {
        if (ctx->input().isMousePressed(MouseButton::Left)) {
            hide();
        }
    }
    
    return totalHeight;
}

//=============================================================================
// Global helpers
//=============================================================================
void ShowContextMenu(const std::vector<MenuItem>& items, const Vec2& position) {
    g_contextMenu.setItems(items);
    g_contextMenu.show(position);
    g_contextMenuActive = true;
}

void RenderContextMenu() {
    if (g_contextMenuActive) {
        g_contextMenu.render();
        if (!g_contextMenu.isVisible()) {
            g_contextMenuActive = false;
        }
    }
}

bool IsContextMenuOpen() {
    return g_contextMenuActive && g_contextMenu.isVisible();
}

void CloseContextMenu() {
    g_contextMenu.hide();
    g_contextMenuActive = false;
}

} // namespace fst
