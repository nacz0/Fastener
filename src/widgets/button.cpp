#include "fastener/widgets/button.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"

namespace fst {

bool Button(const char* label, const ButtonOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    // Generate ID
    WidgetId id = ctx->makeId(label);
    
    // Calculate size
    Vec2 textSize = font ? font->measureText(label) : Vec2(100, 20);
    float width = options.style.width > 0 ? options.style.width : 
                  textSize.x + theme.metrics.paddingMedium * 2 + theme.metrics.paddingLarge * 2;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.buttonHeight;
    
    // Allocate space (for now, simple positioning)
    Rect bounds(0, 0, width, height);
    // TODO: Get position from layout system
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;
    
    // Determine colors
    Color bgColor, textColor;
    
    if (options.primary) {
        if (options.disabled) {
            bgColor = theme.colors.primary.withAlpha(0.5f);
            textColor = theme.colors.primaryText.withAlpha(0.5f);
        } else if (state.active) {
            bgColor = theme.colors.primaryActive;
            textColor = theme.colors.primaryText;
        } else if (state.hovered) {
            bgColor = theme.colors.primaryHover;
            textColor = theme.colors.primaryText;
        } else {
            bgColor = theme.colors.primary;
            textColor = theme.colors.primaryText;
        }
    } else {
        if (options.disabled) {
            bgColor = theme.colors.buttonBackground.withAlpha(0.5f);
            textColor = theme.colors.buttonText.withAlpha(0.5f);
        } else if (state.active) {
            bgColor = theme.colors.buttonActive;
            textColor = theme.colors.buttonText;
        } else if (state.hovered) {
            bgColor = theme.colors.buttonHover;
            textColor = theme.colors.buttonText;
        } else {
            bgColor = theme.colors.buttonBackground;
            textColor = theme.colors.buttonText;
        }
    }
    
    // Draw background
    float radius = options.style.borderRadius > 0 ? 
                   options.style.borderRadius : theme.metrics.borderRadius;
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw border if focused
    if (state.focused) {
        dl.addRect(bounds, theme.colors.borderFocused, radius);
    }
    
    // Draw text
    if (font) {
        Vec2 textPos;
        textPos.x = bounds.x() + (bounds.width() - textSize.x) * 0.5f;
        textPos.y = bounds.y() + (bounds.height() - textSize.y) * 0.5f;
        dl.addText(font, textPos, label, nullptr, textColor);
    }
    
    return interaction.clicked && !options.disabled;
}

bool Button(const std::string& label, const ButtonOptions& options) {
    return Button(label.c_str(), options);
}

bool ButtonPrimary(const char* label) {
    ButtonOptions options;
    options.primary = true;
    return Button(label, options);
}

bool ButtonPrimary(const std::string& label) {
    return ButtonPrimary(label.c_str());
}

} // namespace fst
