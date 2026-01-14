/**
 * @file toggle_switch.cpp
 * @brief Toggle switch widget implementation.
 */

#include "fastener/widgets/toggle_switch.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Toggle Switch Implementation
//=============================================================================

bool ToggleSwitch(Context& ctx, std::string_view label, bool& value, const ToggleSwitchOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Generate unique ID
    WidgetId id = ctx.makeId(label);
    
    // Calculate dimensions - pill shape
    float trackHeight = theme.metrics.checkboxSize;
    float trackWidth = trackHeight * 1.8f;
    float knobRadius = (trackHeight - 4.0f) * 0.5f;
    
    Vec2 textSize = font ? font->measureText(label) : Vec2(0, trackHeight);
    float totalWidth = trackWidth + theme.metrics.paddingSmall + textSize.x;
    float height = std::max(trackHeight, textSize.y);
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, totalWidth, height);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(ctx, id, bounds, true);
    WidgetState state = getWidgetState(ctx, id);
    state.disabled = options.disabled;
    
    // Toggle on click
    bool changed = false;
    if (interaction.clicked && !options.disabled) {
        value = !value;
        changed = true;
    }
    
    // Calculate track bounds (vertically centered)
    Rect trackBounds(
        bounds.x(),
        bounds.y() + (height - trackHeight) * 0.5f,
        trackWidth,
        trackHeight
    );
    
    // Determine track color
    Color trackColor;
    if (options.disabled) {
        trackColor = theme.colors.inputBackground.withAlpha(0.5f);
    } else if (value) {
        trackColor = state.hovered ? theme.colors.primaryHover : theme.colors.primary;
    } else {
        trackColor = state.hovered 
            ? theme.colors.secondary.lighter(0.1f) 
            : theme.colors.secondary;
    }
    
    // Draw track (pill shape)
    float trackRadius = trackHeight * 0.5f;
    dl.addRectFilled(trackBounds, trackColor, trackRadius);
    
    // Calculate knob position
    float knobPadding = 2.0f;
    float knobX = value 
        ? trackBounds.right() - knobRadius - knobPadding
        : trackBounds.x() + knobRadius + knobPadding;
    float knobY = trackBounds.center().y;
    
    // Determine knob color
    Color knobColor = options.disabled 
        ? theme.colors.text.withAlpha(0.5f)
        : Color::white();
    
    // Draw knob shadow (subtle depth effect)
    if (!options.disabled) {
        Color shadowColor = Color(0, 0, 0, 40);
        dl.addCircleFilled(Vec2(knobX + 1, knobY + 1), knobRadius, shadowColor, 16);
    }
    
    // Draw knob
    dl.addCircleFilled(Vec2(knobX, knobY), knobRadius, knobColor, 16);
    
    // Draw label text
    if (font && !label.empty()) {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, textSize.y);
        Vec2 textPos(trackBounds.right() + theme.metrics.paddingSmall, textY);
        
        Color textColor = options.disabled 
            ? theme.colors.textDisabled 
            : theme.colors.text;
        dl.addText(font, textPos, label, textColor);
    }
    
    return changed;
}

} // namespace fst
