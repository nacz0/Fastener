/**
 * @file selectable.cpp
 * @brief Selectable text/item widget implementation.
 */

#include "fastener/widgets/selectable.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Selectable Implementation
//=============================================================================

bool Selectable(const char* label, bool& selected, const SelectableOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = wc.ctx->input();

    WidgetId id = wc.ctx->makeId(label);

    // Calculate dimensions
    float textWidth = font ? font->measureText(label).x : 100.0f;
    float height = options.height > 0 
        ? options.height 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);
    
    float width = options.style.width > 0 
        ? options.style.width 
        : (options.spanWidth ? 0.0f : textWidth + theme.metrics.paddingSmall * 2);

    // Allocate bounds (spanWidth uses layout's available width)
    Rect bounds = allocateWidgetBounds(options.style, width, height);
    
    if (options.spanWidth && bounds.width() < textWidth + theme.metrics.paddingSmall * 2) {
        // Fallback if layout doesn't provide enough width
        bounds.size.x = textWidth + theme.metrics.paddingSmall * 2;
    }

    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    wc.ctx->setLastWidgetId(id);
    wc.ctx->setLastWidgetBounds(bounds);
    WidgetState widgetState = getWidgetState(id);
    widgetState.disabled = options.disabled;

    bool clicked = false;
    if (interaction.clicked && !options.disabled) {
        selected = !selected;
        clicked = true;
    }

    // Draw background
    Color bgColor = Color(0, 0, 0, 0); // Transparent by default
    if (selected) {
        bgColor = theme.colors.selection;
    } else if (widgetState.hovered && !options.disabled) {
        bgColor = theme.colors.selection.withAlpha((uint8_t)80);
    }

    if (bgColor.a > 0) {
        dl.addRectFilled(bounds, bgColor, theme.metrics.borderRadiusSmall);
    }

    // Draw text
    if (font) {
        Vec2 textPos(
            bounds.x() + theme.metrics.paddingSmall,
            bounds.center().y - font->lineHeight() * 0.5f
        );
        
        Color textColor;
        if (options.disabled) {
            textColor = theme.colors.textDisabled;
        } else if (selected) {
            textColor = theme.colors.selectionText;
        } else {
            textColor = theme.colors.text;
        }
        
        dl.addText(font, textPos, label, textColor);
    }

    return clicked;
}

bool Selectable(const std::string& label, bool& selected, const SelectableOptions& options) {
    return Selectable(label.c_str(), selected, options);
}

// Non-modifying overload
bool Selectable(const std::string& label, bool selected, const SelectableOptions& options) {
    bool sel = selected;
    return Selectable(label.c_str(), sel, options);
}

bool SelectableWithIcon(const std::string& icon, const std::string& label, 
                        bool& selected, const SelectableOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = wc.ctx->input();

    std::string fullLabel = icon + " " + label;
    WidgetId id = wc.ctx->makeId(fullLabel.c_str());

    float iconWidth = font ? font->measureText(icon).x : 16.0f;
    float textWidth = font ? font->measureText(label).x : 100.0f;
    float totalWidth = iconWidth + theme.metrics.paddingSmall + textWidth;
    float height = options.height > 0 
        ? options.height 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);

    float width = options.style.width > 0 
        ? options.style.width 
        : (options.spanWidth ? 0.0f : totalWidth + theme.metrics.paddingSmall * 2);

    Rect bounds = allocateWidgetBounds(options.style, width, height);

    if (options.spanWidth && bounds.width() < totalWidth + theme.metrics.paddingSmall * 2) {
        bounds.size.x = totalWidth + theme.metrics.paddingSmall * 2;
    }

    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    wc.ctx->setLastWidgetId(id);
    wc.ctx->setLastWidgetBounds(bounds);
    WidgetState widgetState = getWidgetState(id);
    widgetState.disabled = options.disabled;

    bool clicked = false;
    if (interaction.clicked && !options.disabled) {
        selected = !selected;
        clicked = true;
    }

    // Draw background
    Color bgColor = Color(0, 0, 0, 0);
    if (selected) {
        bgColor = theme.colors.selection;
    } else if (widgetState.hovered && !options.disabled) {
        bgColor = theme.colors.selection.withAlpha((uint8_t)80);
    }

    if (bgColor.a > 0) {
        dl.addRectFilled(bounds, bgColor, theme.metrics.borderRadiusSmall);
    }

    // Draw icon and text
    if (font) {
        float y = bounds.center().y - font->lineHeight() * 0.5f;
        
        Color textColor;
        if (options.disabled) {
            textColor = theme.colors.textDisabled;
        } else if (selected) {
            textColor = theme.colors.selectionText;
        } else {
            textColor = theme.colors.text;
        }

        // Icon (slightly dimmed if not selected)
        Color iconColor = selected ? textColor : theme.colors.textSecondary;
        Vec2 iconPos(bounds.x() + theme.metrics.paddingSmall, y);
        dl.addText(font, iconPos, icon, iconColor);

        // Label text
        Vec2 textPos(bounds.x() + theme.metrics.paddingSmall + iconWidth + theme.metrics.paddingSmall, y);
        dl.addText(font, textPos, label, textColor);
    }

    return clicked;
}

} // namespace fst
