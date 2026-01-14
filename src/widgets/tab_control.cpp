/**
 * @file tab_control.cpp
 * @brief Tab control widget implementation for tabbed interfaces.
 */

#include "fastener/widgets/tab_control.h"
#include "fastener/widgets/menu.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"

#include <algorithm>
#include <cmath>

namespace fst {

//=============================================================================
// TabControl Implementation
//=============================================================================

TabControl::TabControl() = default;
TabControl::~TabControl() = default;

void TabControl::addTab(const TabItem& tab) {
    m_tabs.push_back(tab);
    if (m_selectedIndex < 0) {
        m_selectedIndex = 0;
    }
}

void TabControl::addTab(const std::string& id, const std::string& label, bool closable) {
    addTab(TabItem(id, label, closable));
}

void TabControl::insertTab(int index, const TabItem& tab) {
    index = std::clamp(index, 0, static_cast<int>(m_tabs.size()));
    m_tabs.insert(m_tabs.begin() + index, tab);
    
    if (m_selectedIndex >= index) {
        m_selectedIndex++;
    }
    if (m_selectedIndex < 0 && !m_tabs.empty()) {
        m_selectedIndex = 0;
    }
}

void TabControl::removeTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return;
    
    m_tabs.erase(m_tabs.begin() + index);
    
    if (m_tabs.empty()) {
        m_selectedIndex = -1;
    } else if (m_selectedIndex >= static_cast<int>(m_tabs.size())) {
        m_selectedIndex = static_cast<int>(m_tabs.size()) - 1;
    } else if (m_selectedIndex > index) {
        m_selectedIndex--;
    }
}

void TabControl::removeTabById(const std::string& id) {
    int index = findTabIndex(id);
    if (index >= 0) {
        removeTab(index);
    }
}

void TabControl::clearTabs() {
    m_tabs.clear();
    m_selectedIndex = -1;
}

TabItem* TabControl::getTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return nullptr;
    return &m_tabs[index];
}

TabItem* TabControl::getTabById(const std::string& id) {
    return getTab(findTabIndex(id));
}

int TabControl::findTabIndex(const std::string& id) const {
    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++) {
        if (m_tabs[i].id == id) return i;
    }
    return -1;
}

void TabControl::selectTab(int index) {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        m_selectedIndex = index;
    }
}

void TabControl::selectTabById(const std::string& id) {
    selectTab(findTabIndex(id));
}

TabItem* TabControl::selectedTab() {
    return getTab(m_selectedIndex);
}

std::vector<float> TabControl::calculateTabWidths(Context& ctx, float availableWidth, 
                                                   const TabControlOptions& options) const {
    std::vector<float> widths;
    if (m_tabs.empty()) return widths;
    
    Font* font = ctx.font();

    
    // Calculate ideal width for each tab
    float totalIdeal = 0;
    for (const auto& tab : m_tabs) {
        float textWidth = font ? font->measureText(tab.label).x : 50.0f;
        float tabWidth = textWidth + 30.0f; // Padding + close button
        if (tab.closable && options.showCloseButtons) {
            tabWidth += 20.0f;
        }
        tabWidth = std::clamp(tabWidth, options.tabMinWidth, options.tabMaxWidth);
        widths.push_back(tabWidth);
        totalIdeal += tabWidth;
    }
    
    // If tabs fit, use ideal widths
    if (totalIdeal <= availableWidth) {
        return widths;
    }
    
    // Otherwise, shrink proportionally (but not below min)
    float minTotal = options.tabMinWidth * m_tabs.size();
    if (availableWidth <= minTotal) {
        std::fill(widths.begin(), widths.end(), options.tabMinWidth);
        return widths;
    }
    
    float scale = availableWidth / totalIdeal;
    for (auto& w : widths) {
        w = std::max(options.tabMinWidth, w * scale);
    }
    
    return widths;
}

Rect TabControl::render(Context& ctx, const std::string& id, const Rect& bounds,
                        const TabControlOptions& options,
                        const TabControlEvents& events) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();
    
    ctx.pushId(id.c_str());

    
    // Tab bar area
    float tabBarHeight = options.tabHeight;
    Rect tabBarRect(bounds.x(), bounds.y(), bounds.width(), tabBarHeight);
    Rect contentRect(bounds.x(), bounds.y() + tabBarHeight, 
                     bounds.width(), bounds.height() - tabBarHeight);
    
    // Draw tab bar background
    dl.addRectFilled(tabBarRect, theme.colors.panelBackground.darker(0.05f));
    
    // Calculate tab widths
    float availableWidth = tabBarRect.width();
    if (options.showAddButton) {
        availableWidth -= 32.0f;
    }
    
    std::vector<float> tabWidths = calculateTabWidths(ctx, availableWidth, options);

    
    // Reset hover states
    m_hoveredTab = -1;
    m_hoveredClose = -1;
    
    // Draw tabs
    float x = tabBarRect.x() - m_scrollOffset;
    
    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++) {
        float tabWidth = tabWidths[i];
        Rect tabRect(x, tabBarRect.y(), tabWidth, tabBarHeight);
        
        // Skip if completely outside view
        if (tabRect.right() > tabBarRect.x() && tabRect.left() < tabBarRect.right()) {
            drawTab(ctx, i, tabRect, options, events);
        }

        
        x += tabWidth;
    }
    
    // Draw add button
    if (options.showAddButton) {
        float addX = tabBarRect.right() - 32.0f;
        Rect addRect(addX, tabBarRect.y(), 32.0f, tabBarHeight);
        
        bool addHovered = addRect.contains(ctx.input().mousePos()) && !ctx.isOccluded(ctx.input().mousePos());

        if (addHovered) {
            dl.addRectFilled(addRect, theme.colors.buttonHover);
        }
        
        // Draw + icon
        Vec2 center = addRect.center();
        float size = 6.0f;
        Color iconColor = addHovered ? theme.colors.text : theme.colors.textSecondary;
        dl.addLine(Vec2(center.x - size, center.y), Vec2(center.x + size, center.y), iconColor, 2.0f);
        dl.addLine(Vec2(center.x, center.y - size), Vec2(center.x, center.y + size), iconColor, 2.0f);
        
        if (addHovered && ctx.input().isMousePressed(MouseButton::Left)) {

            if (events.onAdd) {
                events.onAdd();
            }
        }
    }
    
    // Draw bottom border
    dl.addRectFilled(Rect(bounds.x(), tabBarRect.bottom() - 1, bounds.width(), 1), 
                     theme.colors.border);
    
    // Handle scroll with mouse wheel
    if (tabBarRect.contains(ctx.input().mousePos()) && !ctx.isOccluded(ctx.input().mousePos())) {
        float scroll = ctx.input().scrollDelta().y;

        m_scrollOffset -= scroll * 30.0f;
        
        float totalWidth = 0;
        for (float w : tabWidths) totalWidth += w;
        float maxScroll = std::max(0.0f, totalWidth - availableWidth);
        m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maxScroll);
    }
    
    ctx.popId();
    
    return contentRect;
}


void TabControl::drawTab(Context& ctx, int index, const Rect& tabRect,
                         const TabControlOptions& options,
                         const TabControlEvents& events) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();

    
    const TabItem& tab = m_tabs[index];
    bool isSelected = (index == m_selectedIndex);
    bool isHovered = tabRect.contains(ctx.input().mousePos()) && !fst::IsMouseOverAnyMenu(ctx) && !ctx.isOccluded(ctx.input().mousePos());

    
    if (isHovered) {
        m_hoveredTab = index;
    }
    
    // Tab background
    Color bgColor;
    if (isSelected) {
        bgColor = theme.colors.panelBackground;
    } else if (isHovered) {
        bgColor = theme.colors.buttonHover;
    } else {
        bgColor = theme.colors.panelBackground.darker(0.05f);
    }
    
    // Draw tab with slight corner radius at top
    dl.addRectFilled(tabRect, bgColor, 4.0f);
    
    // Selected tab indicator
    if (isSelected) {
        dl.addRectFilled(Rect(tabRect.x(), tabRect.y(), tabRect.width(), 2), 
                         theme.colors.primary);
    }
    
    // Separator
    if (!isSelected && index < static_cast<int>(m_tabs.size()) - 1) {
        dl.addRectFilled(Rect(tabRect.right() - 1, tabRect.y() + 6, 
                              1, tabRect.height() - 12), 
                         theme.colors.border);
    }
    
    // Modified indicator
    float textX = tabRect.x() + 10;
    if (tab.modified) {
        Vec2 dotPos(tabRect.x() + 8, tabRect.center().y);
        dl.addCircleFilled(dotPos, 4, theme.colors.primary);
        textX += 10;
    }
    
    // Tab label
    if (font) {
        float maxTextWidth = tabRect.width() - 20;
        if (tab.closable && options.showCloseButtons) {
            maxTextWidth -= 20;
        }
        
        Color textColor = isSelected ? theme.colors.text : theme.colors.textSecondary;
        Vec2 textPos(textX, tabRect.y() + (tabRect.height() - font->lineHeight()) / 2);
        
        // Clip text if needed
        dl.pushClipRect(Rect(textX, tabRect.y(), maxTextWidth, tabRect.height()));
        dl.addText(font, textPos, tab.label, textColor);
        dl.popClipRect();
    }
    
    // Close button
    if (tab.closable && options.showCloseButtons) {
        float closeSize = 14.0f;
        Rect closeRect(tabRect.right() - closeSize - 6, 
                       tabRect.center().y - closeSize / 2,
                       closeSize, closeSize);
        
        bool closeHovered = closeRect.contains(ctx.input().mousePos()) && !ctx.isOccluded(ctx.input().mousePos());

        if (closeHovered) {
            m_hoveredClose = index;
            dl.addRectFilled(closeRect, theme.colors.buttonHover, 2.0f);
        }
        
        // X icon
        Vec2 center = closeRect.center();
        float xs = 4.0f;
        Color xColor = closeHovered ? theme.colors.text : theme.colors.textSecondary;
        dl.addLine(Vec2(center.x - xs, center.y - xs), Vec2(center.x + xs, center.y + xs), xColor, 1.5f);
        dl.addLine(Vec2(center.x + xs, center.y - xs), Vec2(center.x - xs, center.y + xs), xColor, 1.5f);
        
        // Close click
        if (closeHovered && ctx.input().isMousePressed(MouseButton::Left)) {

            if (events.onClose) {
                events.onClose(index, tab);
            }
        }
    }
    
    // Tab click (select)
    if (isHovered && m_hoveredClose != index && ctx.input().isMousePressed(MouseButton::Left)) {

        if (index != m_selectedIndex) {
            m_selectedIndex = index;
            if (events.onSelect) {
                events.onSelect(index, tab);
            }
        }
    }
    
    // Right click (context menu)
    if (isHovered && ctx.input().isMousePressed(MouseButton::Right)) {

        if (events.onContextMenu) {
            events.onContextMenu(index, tab);
        }
    }
}

} // namespace fst
