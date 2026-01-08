/**
 * @file input_number.cpp
 * @brief InputNumber widget implementation with +/- buttons.
 */

#include "fastener/widgets/input_number.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace fst {

//=============================================================================
// InputNumber Implementation
//=============================================================================

/**
 * @brief Renders a numeric input with +/- buttons.
 * 
 * @param label Label displayed before the input
 * @param value Reference to the float value (modified on button click)
 * @param minVal Minimum allowed value
 * @param maxVal Maximum allowed value
 * @param options Styling and behavior options
 * @return true if the value was changed this frame
 */
bool InputNumber(const char* label, float& value, float minVal, float maxVal,
                 const InputNumberOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;
    
    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Calculate dimensions
    float inputWidth = options.style.width > 0 ? options.style.width : 120.0f;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.inputHeight;
    float buttonWidth = height; // Square buttons
    
    // Calculate label width
    float labelWidth = 0;
    if (font && label[0] != '\0') {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }
    
    float totalWidth = labelWidth + inputWidth;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, totalWidth, height);
    
    // Calculate sub-regions
    Rect labelRect(bounds.x(), bounds.y(), labelWidth, height);
    Rect inputRect(bounds.x() + labelWidth, bounds.y(), inputWidth, height);
    Rect minusButtonRect(inputRect.x(), inputRect.y(), buttonWidth, height);
    Rect valueRect(inputRect.x() + buttonWidth, inputRect.y(), 
                   inputWidth - buttonWidth * 2, height);
    Rect plusButtonRect(inputRect.right() - buttonWidth, inputRect.y(), 
                        buttonWidth, height);
    
    // Generate unique IDs for sub-widgets
    wc.ctx->pushId(label);
    WidgetId minusId = wc.ctx->makeId("minus");
    WidgetId plusId = wc.ctx->makeId("plus");
    wc.ctx->popId();
    
    bool changed = false;
    
    // Handle minus button
    WidgetInteraction minusInteraction = handleWidgetInteraction(minusId, minusButtonRect, true);
    WidgetState minusState = getWidgetState(minusId);
    minusState.disabled = options.disabled || value <= minVal;
    
    if (minusInteraction.clicked && !minusState.disabled) {
        value = std::max(minVal, value - options.step);
        changed = true;
    }
    
    // Handle plus button
    WidgetInteraction plusInteraction = handleWidgetInteraction(plusId, plusButtonRect, true);
    WidgetState plusState = getWidgetState(plusId);
    plusState.disabled = options.disabled || value >= maxVal;
    
    if (plusInteraction.clicked && !plusState.disabled) {
        value = std::min(maxVal, value + options.step);
        changed = true;
    }
    
    // Clamp value
    value = std::clamp(value, minVal, maxVal);
    
    // Draw label
    if (font && label[0] != '\0') {
        float textY = layout_utils::verticalCenterY(bounds.y(), height, font->lineHeight());
        Color labelColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, Vec2(labelRect.x(), textY), label, nullptr, labelColor);
    }
    
    // Draw input background
    float radius = theme.metrics.borderRadiusSmall;
    dl.addRectFilled(inputRect, theme.colors.inputBackground, radius);
    dl.addRect(inputRect, theme.colors.inputBorder, radius);
    
    // Draw minus button
    Color minusBgColor = getStateColor(
        theme.colors.buttonBackground,
        theme.colors.buttonHover,
        theme.colors.buttonActive,
        minusState
    );
    dl.addRectFilled(minusButtonRect, minusBgColor, radius);
    dl.addRect(minusButtonRect, theme.colors.inputBorder, radius);
    
    // Draw minus sign
    if (font) {
        Vec2 minusTextSize = font->measureText("-");
        Vec2 minusTextPos = minusButtonRect.center() - minusTextSize * 0.5f;
        Color minusColor = minusState.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, minusTextPos, "-", minusColor);
    }
    
    // Draw plus button
    Color plusBgColor = getStateColor(
        theme.colors.buttonBackground,
        theme.colors.buttonHover,
        theme.colors.buttonActive,
        plusState
    );
    dl.addRectFilled(plusButtonRect, plusBgColor, radius);
    dl.addRect(plusButtonRect, theme.colors.inputBorder, radius);
    
    // Draw plus sign
    if (font) {
        Vec2 plusTextSize = font->measureText("+");
        Vec2 plusTextPos = plusButtonRect.center() - plusTextSize * 0.5f;
        Color plusColor = plusState.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, plusTextPos, "+", plusColor);
    }
    
    // Draw value
    if (font) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(options.decimals) << value;
        std::string valueStr = oss.str();
        
        Vec2 valueTextSize = font->measureText(valueStr);
        Vec2 valueTextPos = valueRect.center() - valueTextSize * 0.5f;
        Color valueColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, valueTextPos, valueStr, valueColor);
    }
    
    return changed;
}

/**
 * @brief String overload for InputNumber.
 */
bool InputNumber(const std::string& label, float& value, float minVal, float maxVal,
                 const InputNumberOptions& options) {
    return InputNumber(label.c_str(), value, minVal, maxVal, options);
}

/**
 * @brief Integer variant of InputNumber.
 */
bool InputNumberInt(const char* label, int& value, int minVal, int maxVal,
                    const InputNumberOptions& options) {
    float floatValue = static_cast<float>(value);
    
    InputNumberOptions intOptions = options;
    intOptions.decimals = 0;
    if (intOptions.step < 1.0f) intOptions.step = 1.0f;
    
    bool changed = InputNumber(label, floatValue, 
                               static_cast<float>(minVal), 
                               static_cast<float>(maxVal), 
                               intOptions);
    
    if (changed) {
        value = static_cast<int>(std::round(floatValue));
    }
    
    return changed;
}

/**
 * @brief String overload for InputNumberInt.
 */
bool InputNumberInt(const std::string& label, int& value, int minVal, int maxVal,
                    const InputNumberOptions& options) {
    return InputNumberInt(label.c_str(), value, minVal, maxVal, options);
}

} // namespace fst
