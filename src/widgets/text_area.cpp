/**
 * @file text_area.cpp
 * @brief Multi-line text input widget implementation.
 */

#include "fastener/widgets/text_area.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <sstream>
#include <vector>
#include <cmath>
#include <unordered_map>

namespace fst {

//=============================================================================
// TextArea State
//=============================================================================

/** @brief Per-textarea persistent state for cursor and scroll. */
struct TextAreaState {
    size_t cursorPos = 0;
    float scrollOffsetY = 0.0f;
    float scrollOffsetX = 0.0f;
    size_t selectionStart = 0;
    size_t selectionEnd = 0;
    size_t selectionAnchor = 0;
    bool selecting = false;
};

static std::unordered_map<WidgetId, TextAreaState> s_textAreaStates;

//=============================================================================
// Helper Functions
//=============================================================================

/** @brief Split text into lines. */
static std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    if (lines.empty() || (!text.empty() && text.back() == '\n')) {
        lines.push_back("");
    }
    return lines;
}

/** @brief Join lines back into text. */
static std::string joinLines(const std::vector<std::string>& lines) {
    std::string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        result += lines[i];
        if (i < lines.size() - 1) {
            result += '\n';
        }
    }
    return result;
}

/** @brief Get line and column from cursor position. */
static void getCursorLineCol(const std::string& text, size_t cursor, int& line, int& col) {
    line = 0;
    col = 0;
    for (size_t i = 0; i < cursor && i < text.length(); ++i) {
        if (text[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
}

/** @brief Get cursor position from line and column. */
static size_t getCursorFromLineCol(const std::string& text, int line, int col) {
    int currentLine = 0;
    int currentCol = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if (currentLine == line && currentCol == col) {
            return i;
        }
        if (text[i] == '\n') {
            if (currentLine == line) {
                return i; // End of target line
            }
            currentLine++;
            currentCol = 0;
        } else {
            currentCol++;
        }
    }
    return text.length();
}

static bool hasSelection(const TextAreaState& state) {
    return state.selectionStart != state.selectionEnd;
}

static void clearSelection(TextAreaState& state) {
    state.selectionStart = state.selectionEnd = state.cursorPos;
}

static void normalizeSelection(size_t& start, size_t& end) {
    if (start > end) std::swap(start, end);
}

static void deleteSelection(TextAreaState& state, std::string& value) {
    if (!hasSelection(state)) return;
    size_t start = state.selectionStart;
    size_t end = state.selectionEnd;
    normalizeSelection(start, end);
    if (end > start && end <= value.size()) {
        value.erase(start, end - start);
        state.cursorPos = start;
    }
    clearSelection(state);
}

//=============================================================================
// TextArea Implementation
//=============================================================================

bool TextArea(Context& ctx, const char* id, std::string& value, const TextAreaOptions& options) {
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();
    InputState& input = ctx.input();

    WidgetId widgetId = ctx.makeId(id);
    TextAreaState& state = s_textAreaStates[widgetId];


    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 300.0f;
    float height = options.style.height > 0 ? options.style.height : options.height;
    float lineHeight = font ? font->lineHeight() : 16.0f;

    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);



    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(ctx, widgetId, bounds, true);
    WidgetState widgetState = getWidgetState(ctx, widgetId);


    bool changed = false;

    // Handle mouse selection (press + drag)
    if (!options.readonly && bounds.contains(input.mousePos()) && !ctx.isOccluded(input.mousePos()) &&
        input.isMousePressed(MouseButton::Left)) {
        Vec2 clickPos = input.mousePos();
        float padding = theme.metrics.paddingSmall;
        float lineNumWidth = options.showLineNumbers ? 40.0f : 0.0f;

        int clickLine = (int)((clickPos.y - bounds.y() - padding + state.scrollOffsetY) / lineHeight);
        auto lines = splitLines(value);
        clickLine = std::clamp(clickLine, 0, std::max(0, (int)lines.size() - 1));

        int clickCol = 0;
        if (font && clickLine < (int)lines.size()) {
            float x = clickPos.x - bounds.x() - padding - lineNumWidth + state.scrollOffsetX;
            const std::string& line = lines[clickLine];
            for (size_t i = 0; i <= line.length(); ++i) {
                float charX = font->measureText(line.substr(0, i)).x;
                if (charX >= x) {
                    clickCol = (int)i;
                    break;
                }
                clickCol = (int)(i + 1);
            }
        }

        state.cursorPos = getCursorFromLineCol(value, clickLine, clickCol);
        state.selectionAnchor = state.cursorPos;
        if (!input.modifiers().shift) {
            clearSelection(state);
        } else {
            state.selectionStart = state.selectionAnchor;
            state.selectionEnd = state.cursorPos;
        }
        state.selecting = true;
    }

    if (state.selecting && input.isMouseDown(MouseButton::Left) && !options.readonly) {
        Vec2 dragPos = input.mousePos();
        float padding = theme.metrics.paddingSmall;
        float lineNumWidth = options.showLineNumbers ? 40.0f : 0.0f;
        int dragLine = (int)((dragPos.y - bounds.y() - padding + state.scrollOffsetY) / lineHeight);
        auto lines = splitLines(value);
        dragLine = std::clamp(dragLine, 0, std::max(0, (int)lines.size() - 1));
        int dragCol = 0;
        if (font && dragLine < (int)lines.size()) {
            float x = dragPos.x - bounds.x() - padding - lineNumWidth + state.scrollOffsetX;
            const std::string& line = lines[dragLine];
            for (size_t i = 0; i <= line.length(); ++i) {
                float charX = font->measureText(line.substr(0, i)).x;
                if (charX >= x) {
                    dragCol = (int)i;
                    break;
                }
                dragCol = (int)(i + 1);
            }
        }
        state.cursorPos = getCursorFromLineCol(value, dragLine, dragCol);
        state.selectionStart = state.selectionAnchor;
        state.selectionEnd = state.cursorPos;
    }
    if (input.isMouseReleased(MouseButton::Left)) {
        state.selecting = false;
    }

    // Handle text input when focused
    if (widgetState.focused && !options.readonly) {
        bool ctrl = input.modifiers().ctrl;
        bool shift = input.modifiers().shift;

        if (ctrl && input.isKeyPressed(Key::A)) {
            state.selectionStart = 0;
            state.selectionEnd = value.size();
            state.cursorPos = state.selectionEnd;
        }

        if (ctrl && input.isKeyPressed(Key::C)) {
            if (hasSelection(state)) {
                size_t start = state.selectionStart;
                size_t end = state.selectionEnd;
                normalizeSelection(start, end);
                ctx.window().setClipboardText(value.substr(start, end - start));
            }
        }

        if (ctrl && input.isKeyPressed(Key::X)) {
            if (hasSelection(state)) {
                size_t start = state.selectionStart;
                size_t end = state.selectionEnd;
                normalizeSelection(start, end);
                ctx.window().setClipboardText(value.substr(start, end - start));
                deleteSelection(state, value);
                changed = true;
            }
        }

        if (ctrl && input.isKeyPressed(Key::V)) {
            std::string clip = ctx.window().getClipboardText();
            if (!clip.empty()) {
                deleteSelection(state, value);
                value.insert(state.cursorPos, clip);
                state.cursorPos += clip.length();
                changed = true;
            }
        }

        const std::string& textInput = input.textInput();
        if (!textInput.empty() && !ctrl) {
            if (hasSelection(state)) {
                deleteSelection(state, value);
            }
            if (options.maxLength == 0 || 
                value.length() + textInput.length() <= static_cast<size_t>(options.maxLength)) {
                value.insert(state.cursorPos, textInput);
                state.cursorPos += textInput.length();
                changed = true;
            }
        }

        // Handle Enter key
        if (input.isKeyPressed(Key::Enter)) {
            if (hasSelection(state)) {
                deleteSelection(state, value);
            }
            value.insert(state.cursorPos, "\n");
            state.cursorPos++;
            changed = true;
        }

        // Handle Backspace
        if (input.isKeyPressed(Key::Backspace)) {
            if (hasSelection(state)) {
                deleteSelection(state, value);
                changed = true;
            } else if (state.cursorPos > 0) {
            size_t deletePos = state.cursorPos - 1;
            // Handle UTF-8 continuation bytes
            while (deletePos > 0 && (value[deletePos] & 0xC0) == 0x80) {
                deletePos--;
            }
            size_t deleteCount = state.cursorPos - deletePos;
            value.erase(deletePos, deleteCount);
            state.cursorPos = deletePos;
            changed = true;
            }
        }

        // Handle Delete
        if (input.isKeyPressed(Key::Delete)) {
            if (hasSelection(state)) {
                deleteSelection(state, value);
                changed = true;
            } else if (state.cursorPos < value.length()) {
            size_t deleteEnd = state.cursorPos + 1;
            // Handle UTF-8 continuation bytes
            while (deleteEnd < value.length() && (value[deleteEnd] & 0xC0) == 0x80) {
                deleteEnd++;
            }
            value.erase(state.cursorPos, deleteEnd - state.cursorPos);
            changed = true;
            }
        }

        // Arrow keys navigation
        if (input.isKeyPressed(Key::Left) && state.cursorPos > 0) {
            state.cursorPos--;
            while (state.cursorPos > 0 && (value[state.cursorPos] & 0xC0) == 0x80) {
                state.cursorPos--;
            }
            if (shift) {
                state.selectionEnd = state.cursorPos;
            } else {
                clearSelection(state);
            }
        }
        if (input.isKeyPressed(Key::Right) && state.cursorPos < value.length()) {
            state.cursorPos++;
            while (state.cursorPos < value.length() && (value[state.cursorPos] & 0xC0) == 0x80) {
                state.cursorPos++;
            }
            if (shift) {
                state.selectionEnd = state.cursorPos;
            } else {
                clearSelection(state);
            }
        }
        if (input.isKeyPressed(Key::Up)) {
            int line, col;
            getCursorLineCol(value, state.cursorPos, line, col);
            if (line > 0) {
                state.cursorPos = getCursorFromLineCol(value, line - 1, col);
            }
            if (shift) {
                state.selectionEnd = state.cursorPos;
            } else {
                clearSelection(state);
            }
        }
        if (input.isKeyPressed(Key::Down)) {
            int line, col;
            getCursorLineCol(value, state.cursorPos, line, col);
            auto lines = splitLines(value);
            if (line < (int)lines.size() - 1) {
                state.cursorPos = getCursorFromLineCol(value, line + 1, col);
            }
            if (shift) {
                state.selectionEnd = state.cursorPos;
            } else {
                clearSelection(state);
            }
        }
        
        // Home/End
        if (input.isKeyPressed(Key::Home)) {
            int line, col;
            getCursorLineCol(value, state.cursorPos, line, col);
            state.cursorPos = getCursorFromLineCol(value, line, 0);
            if (shift) {
                state.selectionEnd = state.cursorPos;
            } else {
                clearSelection(state);
            }
        }
        if (input.isKeyPressed(Key::End)) {
            int line, col;
            getCursorLineCol(value, state.cursorPos, line, col);
            auto lines = splitLines(value);
            if (line < (int)lines.size()) {
                state.cursorPos = getCursorFromLineCol(value, line, (int)lines[line].length());
            }
            if (shift) {
                state.selectionEnd = state.cursorPos;
            } else {
                clearSelection(state);
            }
        }
    }

    // Clamp cursor
    state.cursorPos = std::min(state.cursorPos, value.length());

    // Handle scroll
    if (bounds.contains(input.mousePos())) {
        state.scrollOffsetY -= input.scrollDelta().y * lineHeight;
    }


    auto lines = splitLines(value);
    float totalContentHeight = lineHeight * (float)lines.size();
    float maxScroll = std::max(0.0f, totalContentHeight - height + theme.metrics.paddingSmall * 2);
    state.scrollOffsetY = std::clamp(state.scrollOffsetY, 0.0f, maxScroll);

    // Draw background
    float radius = options.style.borderRadius > 0 
        ? options.style.borderRadius 
        : theme.metrics.borderRadiusSmall;
    
    Color bgColor = options.readonly 
        ? theme.colors.inputBackground.darker(0.1f) 
        : theme.colors.inputBackground;
    dl.addRectFilled(bounds, bgColor, radius);

    // Draw border
    Color borderColor;
    if (widgetState.focused) {
        borderColor = theme.colors.inputFocused;
    } else if (widgetState.hovered) {
        borderColor = theme.colors.borderHover;
    } else {
        borderColor = theme.colors.inputBorder;
    }
    dl.addRect(bounds, borderColor, radius);

    // Draw content
    if (font) {
        float padding = theme.metrics.paddingSmall;
        float lineNumWidth = options.showLineNumbers ? 40.0f : 0.0f;
        Rect contentRect = bounds.shrunk(padding);
        contentRect.pos.x += lineNumWidth;
        contentRect.size.x -= lineNumWidth;

        dl.pushClipRect(bounds.shrunk(1));

        // Get cursor line/col for visibility
        int cursorLine, cursorCol;
        getCursorLineCol(value, state.cursorPos, cursorLine, cursorCol);

        // Draw line numbers
        if (options.showLineNumbers) {
            for (int i = 0; i < (int)lines.size(); ++i) {
                float y = bounds.y() + padding + i * lineHeight - state.scrollOffsetY;
                if (y + lineHeight < bounds.y() || y > bounds.bottom()) continue;

                char lineNum[8];
                snprintf(lineNum, sizeof(lineNum), "%d", i + 1);
                Vec2 numPos(bounds.x() + padding, y);
                dl.addText(font, numPos, lineNum, theme.colors.textSecondary);
            }

            // Separator line
            float sepX = bounds.x() + padding + lineNumWidth - 4;
            dl.addLine(
                Vec2(sepX, bounds.y() + padding),
                Vec2(sepX, bounds.bottom() - padding),
                theme.colors.border.withAlpha((uint8_t)100)
            );
        }

        // Draw selection background
        if (hasSelection(state)) {
            size_t selStart = state.selectionStart;
            size_t selEnd = state.selectionEnd;
            normalizeSelection(selStart, selEnd);
            int selStartLine, selStartCol;
            int selEndLine, selEndCol;
            getCursorLineCol(value, selStart, selStartLine, selStartCol);
            getCursorLineCol(value, selEnd, selEndLine, selEndCol);

            for (int i = selStartLine; i <= selEndLine; ++i) {
                if (i < 0 || i >= (int)lines.size()) continue;
                float y = bounds.y() + padding + i * lineHeight - state.scrollOffsetY;
                if (y + lineHeight < bounds.y() || y > bounds.bottom()) continue;

                int lineStartCol = (i == selStartLine) ? selStartCol : 0;
                int lineEndCol = (i == selEndLine) ? selEndCol : (int)lines[i].length();
                lineStartCol = std::clamp(lineStartCol, 0, (int)lines[i].length());
                lineEndCol = std::clamp(lineEndCol, 0, (int)lines[i].length());

                std::string beforeStart = lines[i].substr(0, lineStartCol);
                std::string beforeEnd = lines[i].substr(0, lineEndCol);
                float startX = contentRect.x() + font->measureText(beforeStart).x;
                float endX = contentRect.x() + font->measureText(beforeEnd).x;
                if (endX < startX) std::swap(endX, startX);

                Rect selRect(startX, y, std::max(1.0f, endX - startX), lineHeight);
                dl.addRectFilled(selRect, theme.colors.selection.withAlpha(static_cast<uint8_t>(120)));
            }
        }

        // Draw text lines
        for (int i = 0; i < (int)lines.size(); ++i) {
            float y = bounds.y() + padding + i * lineHeight - state.scrollOffsetY;
            if (y + lineHeight < bounds.y() || y > bounds.bottom()) continue;

            Vec2 textPos(contentRect.x(), y);
            
            // Draw placeholder on first line if empty
            if (value.empty() && !options.placeholder.empty() && !widgetState.focused && i == 0) {
                dl.addText(font, textPos, options.placeholder, theme.colors.textDisabled);
            } else {
                dl.addText(font, textPos, lines[i], theme.colors.text);
            }
        }

        // Draw cursor
        if (widgetState.focused && !options.readonly) {
            float cursorAlpha = (std::fmod(ctx.time() * 2.0f, 2.0f) < 1.0f) ? 1.0f : 0.0f;

            if (cursorAlpha > 0.5f) {
                float y = bounds.y() + padding + cursorLine * lineHeight - state.scrollOffsetY;
                if (y >= bounds.y() && y + lineHeight <= bounds.bottom()) {
                    std::string lineText = cursorLine < (int)lines.size() ? lines[cursorLine] : "";
                    std::string textBeforeCursor = cursorCol <= (int)lineText.length() 
                        ? lineText.substr(0, cursorCol) 
                        : lineText;
                    float cursorX = contentRect.x() + font->measureText(textBeforeCursor).x;
                    Rect cursorRect(cursorX, y + 2, 1, lineHeight - 4);
                    dl.addRectFilled(cursorRect, theme.colors.text);
                }
            }
        }

        dl.popClipRect();
    }

    // Draw scrollbar if needed
    if (totalContentHeight > height) {
        float scrollbarWidth = 8.0f;
        Rect track(bounds.right() - scrollbarWidth - 2, bounds.y() + 2, scrollbarWidth, bounds.height() - 4);
        
        float thumbHeight = std::max(20.0f, (height / totalContentHeight) * track.height());
        float thumbY = track.y() + (state.scrollOffsetY / maxScroll) * (track.height() - thumbHeight);

        Rect thumb(track.x(), thumbY, track.width(), thumbHeight);
        Color thumbColor = track.contains(input.mousePos()) 
            ? theme.colors.scrollbarThumbHover 
            : theme.colors.scrollbarThumb;
        dl.addRectFilled(thumb, thumbColor, scrollbarWidth / 2);
    }

    return changed;
}

bool TextArea(Context& ctx, const std::string& id, std::string& value, const TextAreaOptions& options) {
    return TextArea(ctx, id.c_str(), value, options);
}


bool TextAreaWithLabel(Context& ctx, const std::string& label, std::string& value,
                       const TextAreaOptions& options) {
    const Theme& theme = ctx.theme();
    IDrawList& dl = *ctx.activeDrawList();
    Font* font = ctx.font();

    // Draw label
    if (font && !label.empty()) {
        float labelHeight = font->lineHeight() + theme.metrics.paddingSmall;
        float width = options.style.width > 0 ? options.style.width : 300.0f;
        float totalHeight = labelHeight + options.height;
        
        Rect bounds = allocateWidgetBounds(ctx, options.style, width, totalHeight);
        
        Vec2 labelPos(bounds.x(), bounds.y());
        dl.addText(font, labelPos, label, theme.colors.text);

        // Adjust options for text area below label
        TextAreaOptions areaOpts = options;
        areaOpts.style.width = bounds.width();
        areaOpts.style.height = options.height;
        
        // The TextArea will allocate its own bounds below
        return TextArea(ctx, label, value, areaOpts);
    }

    return TextArea(ctx, label, value, options);
}


} // namespace fst
