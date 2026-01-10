/**
 * @file radio_button.cpp
 * @brief RadioButton widget implementation for mutually exclusive selection.
 */

#include "fastener/widgets/radio_button.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// RadioButton Implementation
//=============================================================================

/**
 * @brief Renders a radio button for mutually exclusive selection.
 * 
 * @param label Text displayed next to the radio button
 * @param selectedIndex Reference to the currently selected index
 * @param index The index value this radio button represents
 * @param options Styling and behavior options
 * @return true if this radio button was selected this frame
 */
bool RadioButton(std::string_view label, int& selectedIndex, int index,
                 const RadioButtonOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;
    
    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Generate unique ID
    WidgetId id = wc.ctx->makeId(label);
    
    // Calculate dimensions
    float circleSize = theme.metrics.checkboxSize;
    Vec2 textSize = font ? font->measureText(label) : Vec2(0, circleSize);
    float totalWidth = circleSize + theme.metrics.paddingSmall + textSize.x;
    float height = std::max(circleSize, textSize.y);
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, totalWidth, height);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;
    
    // Check if this radio button is selected
    bool isSelected = (selectedIndex == index);
    
    // Select on click
    bool changed = false;
    if (interaction.clicked && !options.disabled && !isSelected) {
        selectedIndex = index;
        changed = true;
    }
    
    // Calculate circle bounds
    Vec2 circleCenter(
        bounds.x() + circleSize * 0.5f,
        bounds.y() + height * 0.5f
    );
    float outerRadius = circleSize * 0.5f;
    float innerRadius = circleSize * 0.25f;
    
    // Determine colors
    Color outerColor;
    if (options.disabled) {
        outerColor = theme.colors.inputBackground.withAlpha(0.5f);
    } else if (isSelected) {
        outerColor = state.hovered ? theme.colors.primaryHover : theme.colors.primary;
    } else {
        outerColor = state.hovered 
            ? theme.colors.inputBackground.lighter(0.1f) 
            : theme.colors.inputBackground;
    }
    
    // Draw outer circle
    dl.addCircleFilled(circleCenter, outerRadius, outerColor);
    
    // Draw border for unselected state
    if (!isSelected) {
        Color borderColor = state.focused 
            ? theme.colors.borderFocused 
            : theme.colors.inputBorder;
        dl.addCircle(circleCenter, outerRadius, borderColor);
    }
    
    // Draw inner dot when selected
    if (isSelected) {
        Color dotColor = options.disabled 
            ? theme.colors.primaryText.withAlpha(0.5f) 
            : theme.colors.primaryText;
        dl.addCircleFilled(circleCenter, innerRadius, dotColor);
    }
    
    // Draw label text
    if (font && !label.empty()) {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, textSize.y);
        Vec2 textPos(bounds.x() + circleSize + theme.metrics.paddingSmall, textY);
        
        Color textColor = options.disabled 
            ? theme.colors.textDisabled 
            : theme.colors.text;
        dl.addText(font, textPos, label, textColor);
    }
    
    return changed;
}

} // namespace fst
