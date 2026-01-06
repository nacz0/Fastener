#include "fastener/widgets/checkbox.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

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
    
    // Determine bounds
    Rect bounds;
    if (options.style.x < 0.0f && options.style.y < 0.0f) {
        bounds = ctx->layout().allocate(totalWidth, height, options.style.flexGrow);
    } else {
        bounds = Rect(options.style.x, options.style.y, totalWidth, height);
    }
    
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
        auto pts = checkbox_utils::calculateCheckmark(center, boxSize);
        
        dl.addLine(pts.p1, pts.p2, checkColor, 2.0f);
        dl.addLine(pts.p2, pts.p3, checkColor, 2.0f);
    }
    
    // Draw label
    if (font && label[0] != '\0') {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, textSize.y);
        Vec2 textPos(
            boxBounds.right() + theme.metrics.paddingSmall,
            textY
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
