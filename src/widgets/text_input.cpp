#include "fastener/widgets/text_input.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"
#include <algorithm>

namespace fst {

bool TextInput(const char* id, std::string& value, const TextInputOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    // Generate ID
    WidgetId widgetId = ctx->makeId(id);
    
    // Calculate size
    float width = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : theme.metrics.inputHeight;
    
    Rect bounds(0, 0, width, height);
    // TODO: Get from layout system
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(widgetId, bounds, true);
    WidgetState state = getWidgetState(widgetId);
    
    bool changed = false;
    
    // Handle text input when focused
    if (state.focused && !options.readonly) {
        const std::string& textInput = ctx->input().textInput();
        if (!textInput.empty()) {
            if (options.maxLength == 0 || value.length() + textInput.length() <= static_cast<size_t>(options.maxLength)) {
                value += textInput;
                changed = true;
            }
        }
        
        // Handle backspace
        if (ctx->input().isKeyPressed(Key::Backspace) && !value.empty()) {
            // Remove last UTF-8 character
            size_t len = value.length();
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
    Color bgColor = options.readonly ? 
        theme.colors.inputBackground.darker(0.1f) : theme.colors.inputBackground;
    
    float radius = options.style.borderRadius > 0 ? 
                   options.style.borderRadius : theme.metrics.borderRadiusSmall;
    
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw border
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
        
        if (value.empty() && !options.placeholder.empty() && !state.focused) {
            displayText = options.placeholder;
            textColor = theme.colors.textDisabled;
        } else {
            if (options.password) {
                displayText = std::string(value.length(), '*');
            } else {
                displayText = value;
            }
            textColor = theme.colors.text;
        }
        
        Vec2 textPos(
            textRect.x(),
            textRect.y() + (textRect.height() - font->lineHeight()) * 0.5f
        );
        
        dl.addText(font, textPos, displayText, textColor);
        
        // Draw cursor when focused
        if (state.focused && !options.readonly) {
            // Blinking cursor
            float cursorAlpha = (std::fmod(ctx->time() * 2.0f, 2.0f) < 1.0f) ? 1.0f : 0.0f;
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

bool TextInput(const std::string& id, std::string& value, const TextInputOptions& options) {
    return TextInput(id.c_str(), value, options);
}

bool TextInputWithLabel(const std::string& label, std::string& value, 
                        const TextInputOptions& options) {
    // TODO: Draw label + input in a row
    return TextInput(label, value, options);
}

} // namespace fst
