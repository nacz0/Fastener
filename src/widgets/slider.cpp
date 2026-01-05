#include "fastener/widgets/slider.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace fst {

bool Slider(const char* label, float& value, float minVal, float maxVal, 
            const SliderOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    // Generate ID
    WidgetId id = ctx->makeId(label);
    
    // Calculate size
    float sliderWidth = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.sliderHeight;
    
    // Reserve space for label and value display
    float labelWidth = 0;
    float valueWidth = 0;
    
    if (font && label[0] != '\0') {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }
    
    if (options.showValue && font) {
        valueWidth = font->measureText("-999.99").x + theme.metrics.paddingMedium;
    }
    
    float totalWidth = layout_utils::totalWidthWithLabel(sliderWidth, labelWidth, valueWidth, 0);
    
    Rect bounds(0, 0, totalWidth, height);
    // TODO: Get from layout system
    
    // Slider track bounds
    Rect trackBounds(
        bounds.x() + labelWidth,
        bounds.y() + (height - theme.metrics.sliderHeight * 0.3f) * 0.5f,
        sliderWidth,
        theme.metrics.sliderHeight * 0.3f
    );
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    state.disabled = options.disabled;
    
    bool changed = false;
    
    // Handle dragging
    if ((interaction.clicked || interaction.dragging) && !options.disabled) {
        float mouseX = ctx->input().mousePos().x;
        float newValue = slider_utils::valueFromMousePosition(
            mouseX, trackBounds.left(), trackBounds.width(), minVal, maxVal);
        
        if (newValue != value) {
            value = newValue;
            changed = true;
        }
    }
    
    // Clamp value and get normalized position
    value = std::clamp(value, minVal, maxVal);
    float t = slider_utils::valueToNormalized(value, minVal, maxVal);
    
    // Draw label
    if (font && label[0] != '\0') {
        Vec2 labelPos(
            bounds.x(),
            bounds.y() + (height - font->lineHeight()) * 0.5f
        );
        Color labelColor = options.disabled ? 
            theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, labelPos, label, nullptr, labelColor);
    }
    
    // Draw track background
    float trackRadius = trackBounds.height() * 0.5f;
    dl.addRectFilled(trackBounds, theme.colors.secondary, trackRadius);
    
    // Draw filled portion
    if (t > 0.0f) {
        Rect filledRect(
            trackBounds.x(),
            trackBounds.y(),
            trackBounds.width() * t,
            trackBounds.height()
        );
        Color fillColor = options.disabled ? 
            theme.colors.primary.withAlpha(0.5f) : theme.colors.primary;
        dl.addRectFilled(filledRect, fillColor, trackRadius);
    }
    
    // Draw thumb
    float thumbRadius = theme.metrics.sliderHeight * 0.4f;
    Vec2 thumbCenter(
        trackBounds.x() + trackBounds.width() * t,
        bounds.center().y
    );
    
    Color thumbColor;
    if (options.disabled) {
        thumbColor = theme.colors.buttonBackground.withAlpha(0.5f);
    } else if (state.active) {
        thumbColor = theme.colors.primaryHover;
    } else if (state.hovered) {
        thumbColor = theme.colors.buttonHover;
    } else {
        thumbColor = theme.colors.buttonBackground;
    }
    
    dl.addCircleFilled(thumbCenter, thumbRadius, thumbColor);
    dl.addCircle(thumbCenter, thumbRadius, theme.colors.border);
    
    // Draw value
    if (options.showValue && font) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(options.decimals) << value;
        std::string valueStr = oss.str();
        
        Vec2 valuePos(
            trackBounds.right() + theme.metrics.paddingMedium,
            bounds.y() + (height - font->lineHeight()) * 0.5f
        );
        Color valueColor = options.disabled ? 
            theme.colors.textDisabled : theme.colors.textSecondary;
        dl.addText(font, valuePos, valueStr, valueColor);
    }
    
    return changed;
}

bool Slider(const std::string& label, float& value, float min, float max,
            const SliderOptions& options) {
    return Slider(label.c_str(), value, min, max, options);
}

bool SliderInt(const std::string& label, int& value, int min, int max,
               const SliderOptions& options) {
    float floatValue = static_cast<float>(value);
    
    SliderOptions intOptions = options;
    intOptions.decimals = 0;
    
    bool changed = Slider(label, floatValue, static_cast<float>(min), 
                          static_cast<float>(max), intOptions);
    
    if (changed) {
        value = static_cast<int>(std::round(floatValue));
    }
    
    return changed;
}

} // namespace fst
