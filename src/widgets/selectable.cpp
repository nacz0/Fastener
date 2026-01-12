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
// Selectable Implementation (Explicit DI)
//=============================================================================

bool Selectable(Context& ctx, std::string_view label, bool& selected, const SelectableOptions& options) {
    auto wc = WidgetContext::make(ctx);

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    WidgetId id = ctx.makeId(label);

    // Calculate dimensions
    float textWidth = font ? font->measureText(label).x : 100.0f;
    float height = options.height > 0 
        ? options.height 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);
    
    float width = options.style.width > 0 
        ? options.style.width 
        : (options.spanWidth ? 0.0f : textWidth + theme.metrics.paddingSmall * 2);

    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    if (options.spanWidth && bounds.width() < textWidth + theme.metrics.paddingSmall * 2) {
        bounds.size.x = textWidth + theme.metrics.paddingSmall * 2;
    }

    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(ctx, id, bounds, true);
    WidgetState state = getWidgetState(ctx, id);
    state.disabled = options.disabled;

    bool clicked = false;
    if (interaction.clicked && !options.disabled) {
        selected = !selected;
        clicked = true;
    }

    // Draw background
    if (selected) {
        dl.addRectFilled(bounds, theme.colors.selection, theme.metrics.borderRadiusSmall);
    } else if (state.hovered && !options.disabled) {
        dl.addRectFilled(bounds, theme.colors.buttonHover, theme.metrics.borderRadiusSmall);
    }

    // Draw text
    if (font) {
        Color textColor = options.disabled ? theme.colors.textDisabled : 
                         (selected ? theme.colors.selectionText : theme.colors.text);
        
        Vec2 textPos(
            bounds.x() + theme.metrics.paddingSmall,
            bounds.center().y - font->lineHeight() * 0.5f
        );
        
        dl.addText(font, textPos, label, textColor);
    }

    return clicked;
}


bool SelectableWithIcon(Context& ctx, std::string_view icon, std::string_view label, 
                        bool& selected, const SelectableOptions& options) {
    auto wc = WidgetContext::make(ctx);

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    std::string combinedId(icon);
    combinedId += label;
    WidgetId id = ctx.makeId(combinedId);

    float iconWidth = font ? font->measureText(icon).x : 16.0f;
    float textWidth = font ? font->measureText(label).x : 100.0f;
    float totalWidth = iconWidth + theme.metrics.paddingSmall + textWidth;
    float height = options.height > 0 
        ? options.height 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);

    float width = options.style.width > 0 
        ? options.style.width 
        : (options.spanWidth ? 0.0f : totalWidth + theme.metrics.paddingSmall * 2);

    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);

    WidgetInteraction interaction = handleWidgetInteraction(ctx, id, bounds, true);
    WidgetState state = getWidgetState(ctx, id);
    state.disabled = options.disabled;

    bool clicked = false;
    if (interaction.clicked && !options.disabled) {
        selected = !selected;
        clicked = true;
    }

    // Draw background
    if (selected) {
        dl.addRectFilled(bounds, theme.colors.selection, theme.metrics.borderRadiusSmall);
    } else if (state.hovered && !options.disabled) {
        dl.addRectFilled(bounds, theme.colors.buttonHover, theme.metrics.borderRadiusSmall);
    }

    // Draw icon and text
    if (font) {
        float y = bounds.center().y - font->lineHeight() * 0.5f;
        
        Color textColor = options.disabled ? theme.colors.textDisabled : 
                         (selected ? theme.colors.selectionText : theme.colors.text);
        Color iconColor = selected ? textColor : theme.colors.textSecondary;

        float x = bounds.x() + theme.metrics.paddingSmall;
        dl.addText(font, Vec2(x, y), icon, iconColor);
        x += iconWidth + theme.metrics.paddingSmall;
        dl.addText(font, Vec2(x, y), label, textColor);
    }

    return clicked;
}

} // namespace fst

