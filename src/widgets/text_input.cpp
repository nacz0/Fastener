/**
 * @file text_input.cpp
 * @brief Single-line text input widget implementation.
 */

#include "fastener/widgets/text_input.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <cmath>

namespace fst {

//=============================================================================
// TextInput Implementation (Explicit DI)
//=============================================================================

bool TextInput(Context& ctx, std::string_view id, std::string& value, const TextInputOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    // Generate unique ID
    WidgetId widgetId = ctx.makeId(id);
    
    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.inputHeight;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(ctx, widgetId, bounds, true);
    WidgetState state = getWidgetState(ctx, widgetId);
    
    bool changed = false;
    
    // Handle text input when focused
    if (state.focused && !options.readonly) {
        const std::string& textInput = ctx.input().textInput();
        if (!textInput.empty()) {
            if (options.maxLength == 0 || 
                value.length() + textInput.length() <= static_cast<size_t>(options.maxLength)) {
                value += textInput;
                changed = true;
            }
        }
        
        // Handle backspace (UTF-8 aware)
        if (ctx.input().isKeyPressed(Key::Backspace) && !value.empty()) {
            size_t len = value.length();
            // Skip continuation bytes (10xxxxxx pattern)
            while (len > 0 && (value[len - 1] & 0xC0) == 0x80) {
                len--;
            }
            if (len > 0) {
                len--;
            }
            value.resize(len);
            changed = true;
        }
    }
    
    // Draw background
    Color bgColor = options.readonly 
        ? theme.colors.inputBackground.darker(0.1f) 
        : theme.colors.inputBackground;
    
    float radius = options.style.borderRadius > 0 
        ? options.style.borderRadius 
        : theme.metrics.borderRadiusSmall;
    
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw border with state-dependent color
    Color borderColor;
    if (state.focused) {
        borderColor = theme.colors.inputFocused;
    } else if (state.hovered) {
        borderColor = theme.colors.borderHover;
    } else {
        borderColor = theme.colors.inputBorder;
    }
    dl.addRect(bounds, borderColor, radius);
    
    // Draw text or placeholder
    if (font) {
        Rect textRect = bounds.shrunk(theme.metrics.paddingSmall);
        dl.pushClipRect(textRect);
        
        std::string displayText;
        Color textColor;
        
        // Show placeholder when empty and unfocused
        if (value.empty() && !options.placeholder.empty() && !state.focused) {
            displayText = options.placeholder;
            textColor = theme.colors.textDisabled;
        } else {
            // Mask password characters
            displayText = options.password 
                ? std::string(value.length(), '*') 
                : value;
            textColor = theme.colors.text;
        }
        
        Vec2 textPos(
            textRect.x(),
            textRect.y() + (textRect.height() - font->lineHeight()) * 0.5f
        );
        
        dl.addText(font, textPos, displayText, textColor);
        
        // Draw blinking cursor when focused and editable
        if (state.focused && !options.readonly) {
            float cursorAlpha = (std::fmod(ctx.time() * 2.0f, 2.0f) < 1.0f) ? 1.0f : 0.0f;
            if (cursorAlpha > 0.5f) {
                Vec2 textSize = font->measureText(displayText);
                float cursorX = textRect.x() + textSize.x;
                Rect cursorRect(cursorX, textRect.y() + 2, 1, textRect.height() - 4);
                dl.addRectFilled(cursorRect, theme.colors.text);
            }
        }
        
        dl.popClipRect();
    }
    
    return changed;
}

bool TextInputWithLabel(Context& ctx, std::string_view label, std::string& value, 
                        const TextInputOptions& options) {
    return TextInput(ctx, label, value, options);
}



} // namespace fst

