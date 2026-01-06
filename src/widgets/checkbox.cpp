/**
 * @file checkbox.cpp
 * @brief Checkbox widget implementation.
 */

#include "fastener/widgets/checkbox.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Checkbox Implementation
//=============================================================================

/**
 * @brief Renders an interactive checkbox with optional label.
 * 
 * @param label Text displayed next to the checkbox
 * @param checked Reference to boolean state (toggled on click)
 * @param options Checkbox styling and behavior options
 * @return true if the checkbox state was changed this frame
 */
bool Checkbox(const char* label, bool& checked, const CheckboxOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;
    
    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Generate unique ID
    WidgetId id = wc.ctx->makeId(label);
    
    // Calculate dimensions
    float boxSize = theme.metrics.checkboxSize;
    Vec2 textSize = font ? font->measureText(label) : Vec2(0, boxSize);
    float totalWidth = boxSize + theme.metrics.paddingSmall + textSize.x;
    float height = std::max(boxSize, textSize.y);
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, totalWidth, height);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;
    
    // Toggle on click
    bool changed = false;
    if (interaction.clicked && !options.disabled) {
        checked = !checked;
        changed = true;
    }
    
    // Calculate checkbox box bounds
    Rect boxBounds(
        bounds.x(),
        bounds.y() + (height - boxSize) * 0.5f,
        boxSize,
        boxSize
    );
    
    // Determine background color
    Color bgColor;
    if (options.disabled) {
        bgColor = theme.colors.inputBackground.withAlpha(0.5f);
    } else if (checked) {
        bgColor = state.hovered ? theme.colors.primaryHover : theme.colors.primary;
    } else {
        bgColor = state.hovered 
            ? theme.colors.inputBackground.lighter(0.1f) 
            : theme.colors.inputBackground;
    }
    
    // Draw checkbox box
    float radius = theme.metrics.borderRadiusSmall;
    dl.addRectFilled(boxBounds, bgColor, radius);
    
    // Draw border for unchecked state
    if (!checked) {
        Color borderColor = state.focused 
            ? theme.colors.borderFocused 
            : theme.colors.inputBorder;
        dl.addRect(boxBounds, borderColor, radius);
    }
    
    // Draw checkmark when checked
    if (checked) {
        Color checkColor = options.disabled 
            ? theme.colors.primaryText.withAlpha(0.5f) 
            : theme.colors.primaryText;
        
        Vec2 center = boxBounds.center();
        auto pts = checkbox_utils::calculateCheckmark(center, boxSize);
        
        dl.addLine(pts.p1, pts.p2, checkColor, 2.0f);
        dl.addLine(pts.p2, pts.p3, checkColor, 2.0f);
    }
    
    // Draw label text
    if (font && label[0] != '\0') {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, textSize.y);
        Vec2 textPos(boxBounds.right() + theme.metrics.paddingSmall, textY);
        
        Color textColor = options.disabled 
            ? theme.colors.textDisabled 
            : theme.colors.text;
        dl.addText(font, textPos, label, nullptr, textColor);
    }
    
    return changed;
}

/**
 * @brief String overload for Checkbox.
 */
bool Checkbox(const std::string& label, bool& checked, const CheckboxOptions& options) {
    return Checkbox(label.c_str(), checked, options);
}

} // namespace fst
