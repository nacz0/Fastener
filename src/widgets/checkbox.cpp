#include "fastener/widgets/checkbox.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"

namespace fst {

bool Checkbox(const char* label, bool& checked, const CheckboxOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    // Generate ID
    WidgetId id = ctx->makeId(label);
    
    // Calculate size
    float boxSize = theme.metrics.checkboxSize;
    Vec2 textSize = font ? font->measureText(label) : Vec2(0, boxSize);
    float totalWidth = boxSize + theme.metrics.paddingSmall + textSize.x;
    float height = std::max(boxSize, textSize.y);
    
    Rect bounds(0, 0, totalWidth, height);
    // TODO: Get from layout system
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;
    
    bool changed = false;
    if (interaction.clicked && !options.disabled) {
        checked = !checked;
        changed = true;
    }
    
    // Checkbox box bounds
    Rect boxBounds(
        bounds.x(),
        bounds.y() + (height - boxSize) * 0.5f,
        boxSize,
        boxSize
    );
    
    // Draw checkbox background
    Color bgColor;
    if (options.disabled) {
        bgColor = theme.colors.inputBackground.withAlpha(0.5f);
    } else if (checked) {
        bgColor = state.hovered ? theme.colors.primaryHover : theme.colors.primary;
    } else {
        bgColor = state.hovered ? 
            theme.colors.inputBackground.lighter(0.1f) : theme.colors.inputBackground;
    }
    
    float radius = theme.metrics.borderRadiusSmall;
    dl.addRectFilled(boxBounds, bgColor, radius);
    
    // Draw border
    if (!checked) {
        Color borderColor = state.focused ? 
            theme.colors.borderFocused : theme.colors.inputBorder;
        dl.addRect(boxBounds, borderColor, radius);
    }
    
    // Draw checkmark
    if (checked) {
        Color checkColor = theme.colors.primaryText;
        if (options.disabled) {
            checkColor = checkColor.withAlpha(0.5f);
        }
        
        // Draw simple checkmark using lines
        Vec2 center = boxBounds.center();
        float s = boxSize * 0.3f;
        
        // Checkmark: two lines forming a check
        Vec2 p1(center.x - s * 0.8f, center.y);
        Vec2 p2(center.x - s * 0.2f, center.y + s * 0.6f);
        Vec2 p3(center.x + s * 0.9f, center.y - s * 0.5f);
        
        dl.addLine(p1, p2, checkColor, 2.0f);
        dl.addLine(p2, p3, checkColor, 2.0f);
    }
    
    // Draw label
    if (font && label[0] != '\0') {
        Vec2 textPos(
            boxBounds.right() + theme.metrics.paddingSmall,
            bounds.y() + (height - textSize.y) * 0.5f
        );
        
        Color textColor = options.disabled ? 
            theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, textPos, label, nullptr, textColor);
    }
    
    return changed;
}

bool Checkbox(const std::string& label, bool& checked, const CheckboxOptions& options) {
    return Checkbox(label.c_str(), checked, options);
}

} // namespace fst
