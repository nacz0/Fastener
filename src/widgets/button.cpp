/**
 * @file button.cpp
 * @brief Button widget implementation.
 */

#include "fastener/widgets/button.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Button Implementation (Explicit DI)
//=============================================================================

bool Button(Context& ctx, std::string_view label, const ButtonOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Generate unique widget ID from label
    WidgetId id = ctx.makeId(label);
    
    // Calculate button dimensions
    Vec2 textSize = font ? font->measureText(label) : Vec2(100, 20);
    float width = options.style.width > 0 ? options.style.width : 
                  textSize.x + theme.metrics.paddingMedium * 2 + theme.metrics.paddingLarge * 2;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.buttonHeight;
    
    // Allocate bounds from layout or use explicit position
    Rect bounds = allocateWidgetBounds(options.style, width, height);
    
    // Handle mouse/keyboard interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;
    
    // Determine colors based on variant and state
    Color bgColor, textColor;
    
    if (options.primary) {
        // Primary button variant (accent color)
        bgColor = getStateColor(
            theme.colors.primary,
            theme.colors.primaryHover,
            theme.colors.primaryActive,
            state
        );
        textColor = state.disabled 
            ? theme.colors.primaryText.withAlpha(0.5f)
            : theme.colors.primaryText;
    } else {
        // Default button variant
        bgColor = getStateColor(
            theme.colors.buttonBackground,
            theme.colors.buttonHover,
            theme.colors.buttonActive,
            state
        );
        textColor = state.disabled 
            ? theme.colors.buttonText.withAlpha(0.5f)
            : theme.colors.buttonText;
    }
    
    // Draw button background
    float radius = options.style.borderRadius > 0 ? 
                   options.style.borderRadius : theme.metrics.borderRadius;
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw focus indicator border
    if (state.focused) {
        dl.addRect(bounds, theme.colors.borderFocused, radius);
    }
    
    // Draw centered label text
    if (font) {
        Vec2 textPos = layout_utils::centerInBounds(textSize, bounds);
        dl.addText(font, textPos, label, textColor);
    }
    
    return interaction.clicked && !options.disabled;
}

//=============================================================================
// Backward-compatible wrapper (uses context stack)
//=============================================================================

bool Button(std::string_view label, const ButtonOptions& options) {

    
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;
    return Button(*wc.ctx, label, options);
}


bool ButtonPrimary(std::string_view label) {
    ButtonOptions options;
    options.primary = true;
    return Button(label, options);
}

} // namespace fst

