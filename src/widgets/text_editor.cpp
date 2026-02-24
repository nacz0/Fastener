/**
 * @file text_editor.cpp
 * @brief Multi-line text editor widget implementation.
 * 
 * Provides a code editor with line numbers, selection, undo/redo,
 * clipboard support, and optional syntax highlighting via style hooks.
 */

#include "fastener/widgets/text_editor.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"

#include "fastener/platform/window.h"
#include <sstream>
#include <algorithm>
#include <cstdint>

namespace fst {

//=============================================================================
// TextEditor - Construction
//=============================================================================

TextEditor::TextEditor() {
    m_lines.push_back("");
}

TextEditor::~TextEditor() = default;

void TextEditor::setText(const std::string& text) {
    m_lines.clear();
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        m_lines.push_back(line);
    }
    if (m_lines.empty()) m_lines.push_back("");
    if (!text.empty() && text.back() == '\n') {
        m_lines.push_back("");
    }

    m_cursor = {0, 0};
    m_selection.clear();
    m_undoStack.clear();
    m_redoStack.clear();
}

std::string TextEditor::getText() const {
    std::string result;
    for (size_t i = 0; i < m_lines.size(); ++i) {
        result += m_lines[i];
        if (i < m_lines.size() - 1) result += "\n";
    }
    return result;
}

void TextEditor::clear() {
    m_lines.clear();
    m_lines.push_back("");
    m_cursor = {0, 0};
    m_selection.clear();
    m_undoStack.clear();
    m_redoStack.clear();
}

void TextEditor::render(Context& ctx, const Rect& bounds, const TextEditorOptions& options) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    Font* font = ctx.font();
    if (!font) return;
    
    InputState& input = ctx.input();

    const std::string widgetKey = "text_editor_" + std::to_string(reinterpret_cast<std::uintptr_t>(this));
    const WidgetId widgetId = ctx.makeId(widgetKey);
    (void)handleWidgetInteraction(ctx, widgetId, bounds, true);
    const WidgetState widgetState = getWidgetState(ctx, widgetId);



    dl.addRectFilled(bounds, theme.colors.windowBackground);
    dl.addRect(bounds, theme.colors.border);

    float rowHeight = font->lineHeight() * options.lineHeight;
    float charWidth = font->measureText("M").x; 
    float gutterWidth = options.showLineNumbers ? 50.0f : 0.0f;

    handleInput(ctx, bounds, rowHeight, charWidth, gutterWidth, widgetState.focused, options.suppressNavigationKeys);

    dl.pushClipRect(bounds);

    if (options.showLineNumbers) {
        Rect gutterRect = bounds;
        gutterRect.size.x = gutterWidth;
        dl.addRectFilled(gutterRect, theme.colors.panelBackground);
        dl.addLine(gutterRect.topRight(), gutterRect.bottomRight(), theme.colors.border);
    }

    Rect textBounds = bounds;
    textBounds.pos.x += gutterWidth;
    textBounds.size.x -= gutterWidth;

    int startLine = static_cast<int>(m_scrollOffset.y / rowHeight);
    int visibleLines = static_cast<int>(bounds.height() / rowHeight) + 1;
    int endLine = std::min(static_cast<int>(m_lines.size()), startLine + visibleLines);

    for (int i = startLine; i < endLine; ++i) {
        float y = bounds.y() + (i * rowHeight) - m_scrollOffset.y;
        if (y + rowHeight < bounds.y() || y > bounds.bottom()) continue;

        if (options.showLineNumbers) {
            std::string lineNum = std::to_string(i + 1);
            float tw = font->measureText(lineNum).x;
            dl.addText(font, Vec2(bounds.x() + gutterWidth - tw - 10, y + (rowHeight - font->lineHeight()) / 2),
                       lineNum, theme.colors.textSecondary);
        }

        if (!m_selection.isEmpty()) {
            TextPosition minP = m_selection.min();
            TextPosition maxP = m_selection.max();
            if (i >= minP.line && i <= maxP.line) {
                float x1 = 0, x2 = 0;
                if (i == minP.line) x1 = font->measureText(m_lines[i].substr(0, std::min((int)m_lines[i].length(), minP.column))).x;
                if (i == maxP.line) x2 = font->measureText(m_lines[i].substr(0, std::min((int)m_lines[i].length(), maxP.column))).x;
                else x2 = font->measureText(m_lines[i]).x + charWidth * 0.5f;
                dl.addRectFilled(Rect(textBounds.x() + 5 + x1, y, std::max(2.0f, x2 - x1), rowHeight), 
                                 Color(theme.colors.primary.r, theme.colors.primary.g, theme.colors.primary.b, 80));
            }
        }

        if (i == m_cursor.line) {
            dl.addRectFilled(Rect(textBounds.x(), y, textBounds.width(), rowHeight), Color(255, 255, 255, 10));
            if (widgetState.focused && ctx.window().isFocused() && static_cast<int>(ctx.time() * 2) % 2 == 0) {
                float cx = textBounds.x() + 5 + font->measureText(m_lines[i].substr(0, m_cursor.column)).x;
                dl.addLine(Vec2(cx, y + 2), Vec2(cx, y + rowHeight - 2), theme.colors.primary, 2.0f);
            }
        }


        // Text rendering (styled or plain)
        Vec2 textPos(textBounds.x() + 5, y + (rowHeight - font->lineHeight()) / 2);
        const std::string& lineText = m_lines[i];

        if (m_styleProvider) {
            std::vector<TextSegment> segments = m_styleProvider(i, lineText);
            
            // Sort segments by start column to ensure correct rendering order
            std::sort(segments.begin(), segments.end(), [](const TextSegment& a, const TextSegment& b) {
                return a.startColumn < b.startColumn;
            });

            int currentColumn = 0;
            float currentX = textPos.x;

            for (const auto& segment : segments) {
                // Render unstyled text before this segment
                if (segment.startColumn > currentColumn) {
                    std::string plain = lineText.substr(currentColumn, segment.startColumn - currentColumn);
                    dl.addText(font, Vec2(currentX, textPos.y), plain, theme.colors.text);
                    currentX += font->measureText(plain).x;
                }

                // Render styled segment
                if (segment.endColumn > segment.startColumn) {
                    int len = segment.endColumn - segment.startColumn;
                    std::string styled = lineText.substr(segment.startColumn, len);
                    dl.addText(font, Vec2(currentX, textPos.y), styled, segment.color);
                    currentX += font->measureText(styled).x;
                }

                currentColumn = std::max(currentColumn, segment.endColumn);
            }

            // Render remaining unstyled text
            if (currentColumn < (int)lineText.length()) {
                std::string plain = lineText.substr(currentColumn);
                dl.addText(font, Vec2(currentX, textPos.y), plain, theme.colors.text);
            }
        } else {
            dl.addText(font, textPos, lineText, theme.colors.text);
        }
    }

    dl.popClipRect();
}


void TextEditor::undo() {
    if (m_undoStack.empty()) return;
    m_isUndoingRedoing = true;
    EditAction action = m_undoStack.back();
    m_undoStack.pop_back();
    applyAction(action, true);
    m_redoStack.push_back(action);
    m_isUndoingRedoing = false;
}

void TextEditor::redo() {
    if (m_redoStack.empty()) return;
    m_isUndoingRedoing = true;
    EditAction action = m_redoStack.back();
    m_redoStack.pop_back();
    applyAction(action, false);
    m_undoStack.push_back(action);
    m_isUndoingRedoing = false;
}

void TextEditor::recordAction(EditActionType type, const std::string& text, TextPosition start, TextPosition end, TextPosition cursorBefore) {
    if (m_isUndoingRedoing) return;
    
    EditAction action;
    action.type = type;
    action.text = text;
    action.start = start;
    action.end = end;
    action.cursorBefore = cursorBefore;
    action.cursorAfter = m_cursor;

    m_undoStack.push_back(action);
    m_redoStack.clear();

    if (m_undoStack.size() > m_maxHistorySize) {
        m_undoStack.erase(m_undoStack.begin());
    }
}

void TextEditor::applyAction(const EditAction& action, bool undo) {
    bool isInsert = (action.type == EditActionType::Insert);
    if (undo) isInsert = !isInsert;

    if (isInsert) {
        // Apply insertion
        m_cursor = action.start;
        // Text is a single string but might contain newlines
        // insertText handles single lines, so we need a more robust insert for multi-line actions
        std::stringstream ss(action.text);
        std::string segment;
        bool first = true;
        while (std::getline(ss, segment, '\n')) {
            if (!segment.empty() && segment.back() == '\r') segment.pop_back();
            if (!first) enter();
            insertText(segment);
            first = false;
        }
    } else {
        // Apply deletion
        m_selection.start = action.start;
        m_selection.end = action.end;
        deleteSelection();
    }

    m_cursor = undo ? action.cursorBefore : action.cursorAfter;
    m_selection.clear();
}

void TextEditor::setCursor(const TextPosition& pos) {
    m_cursor.line = std::clamp(pos.line, 0, static_cast<int>(m_lines.size() - 1));
    m_cursor.column = std::clamp(pos.column, 0, static_cast<int>(m_lines[m_cursor.line].length()));
}

void TextEditor::handleInput(
    Context& ctx,
    const Rect& bounds,
    float rowHeight,
    float charWidth,
    float gutterWidth,
    bool focused,
    bool suppressNavigationKeys) {
    float dt = ctx.deltaTime();
    
    handleMouse(ctx, bounds, rowHeight, charWidth, gutterWidth);
    if (focused) {
        handleKeyboard(ctx, bounds, rowHeight, dt, suppressNavigationKeys);
    }
    
    InputState& input = ctx.input();
    if (bounds.contains(input.mousePos())) {
        m_scrollOffset.y = std::max(0.0f, m_scrollOffset.y - input.scrollDelta().y * rowHeight * 3.0f);
    }
}



void TextEditor::handleKeyboard(Context& ctx, const Rect& bounds, float rowHeight, float deltaTime, bool suppressNavigationKeys) {
    InputState& input = ctx.input();
    bool shift = input.modifiers().shift;
    bool ctrl = input.modifiers().ctrl;


    auto move = [&](TextPosition p, bool extend) {
        if (extend) {
            if (m_selection.isEmpty()) m_selection.start = m_cursor;
            m_cursor = p;
            m_selection.end = m_cursor;
        } else {
            m_cursor = p;
            m_selection.clear();
        }
        ensureCursorVisible(bounds, rowHeight);
    };

    auto shouldTrigger = [&](Key key) -> bool {
        if (input.isKeyPressed(key)) {
            m_lastRepeatKey = key;
            m_repeatTimer = 0.4f; // Initial delay
            return true;
        }
        if (m_lastRepeatKey == key && input.isKeyDown(key)) {
            m_repeatTimer -= deltaTime;
            if (m_repeatTimer <= 0) {
                m_repeatTimer = 0.05f; // Repeat rate
                return true;
            }
        }
        return false;
    };

    if (!suppressNavigationKeys) {
        if (shouldTrigger(Key::Down)) {
            TextPosition p = m_cursor;
            p.line = std::min(p.line + 1, (int)m_lines.size() - 1);
            p.column = std::min(p.column, (int)m_lines[p.line].length());
            move(p, shift);
        }
        if (shouldTrigger(Key::Up)) {
            TextPosition p = m_cursor;
            p.line = std::max(p.line - 1, 0);
            p.column = std::min(p.column, (int)m_lines[p.line].length());
            move(p, shift);
        }
    }
    if (shouldTrigger(Key::Left)) {
        TextPosition p = m_cursor;
        if (p.column > 0) p.column--;
        else if (p.line > 0) { p.line--; p.column = (int)m_lines[p.line].length(); }
        move(p, shift);
    }
    if (shouldTrigger(Key::Right)) {
        TextPosition p = m_cursor;
        if (p.column < (int)m_lines[p.line].length()) p.column++;
        else if (p.line < (int)m_lines.size() - 1) { p.line++; p.column = 0; }
        move(p, shift);
    }
    
    if (input.isKeyPressed(Key::Home)) move({m_cursor.line, 0}, shift);
    if (input.isKeyPressed(Key::End)) move({m_cursor.line, (int)m_lines[m_cursor.line].length()}, shift);
    
    if (ctrl && input.isKeyPressed(Key::C)) copyToClipboard(ctx);
    if (ctrl && input.isKeyPressed(Key::V)) pasteFromClipboard(ctx);
    if (ctrl && input.isKeyPressed(Key::X)) cutToClipboard(ctx);

    if (ctrl && input.isKeyPressed(Key::A)) {
        m_selection.start = {0, 0};
        m_selection.end = {(int)m_lines.size() - 1, (int)m_lines.back().length()};
        m_cursor = m_selection.end;
    }

    if (ctrl && shouldTrigger(Key::Z)) undo();
    if (ctrl && shouldTrigger(Key::Y)) redo();

    if (!input.textInput().empty() && !ctrl) {
        if (!m_selection.isEmpty()) deleteSelection();
        insertText(input.textInput());
        m_selection.clear();
        ensureCursorVisible(bounds, rowHeight);
    }
    
    if (shouldTrigger(Key::Backspace)) {
        if (!m_selection.isEmpty()) deleteSelection(); else backspace();
        ensureCursorVisible(bounds, rowHeight);
    }
    if (shouldTrigger(Key::Delete)) {
        if (!m_selection.isEmpty()) deleteSelection();
        else {
            if (m_cursor.column < (int)m_lines[m_cursor.line].length()) m_lines[m_cursor.line].erase(m_cursor.column, 1);
            else if (m_cursor.line < (int)m_lines.size() - 1) {
                m_lines[m_cursor.line] += m_lines[m_cursor.line+1];
                m_lines.erase(m_lines.begin() + m_cursor.line + 1);
            }
        }
        ensureCursorVisible(bounds, rowHeight);
    }
    if (!suppressNavigationKeys && input.isKeyPressed(Key::Enter)) {
        if (!m_selection.isEmpty()) deleteSelection();
        enter();
        ensureCursorVisible(bounds, rowHeight);
    }
}

void TextEditor::handleMouse(Context& ctx, const Rect& bounds, float rowHeight, float charWidth, float gutterWidth) {
    InputState& input = ctx.input();
    Vec2 mp = input.mousePos();
    

    
    if (input.isMousePressed(MouseButton::Left) && bounds.contains(mp)) {

        m_isSelecting = true;
        m_cursor = screenToTextPos(ctx, mp, bounds, rowHeight, charWidth, gutterWidth);
        if (!input.modifiers().shift) {
            m_selection.clear();
            m_selectionAnchor = m_cursor;
        } else {
            m_selection.start = m_selectionAnchor;
            m_selection.end = m_cursor;
        }
    }
    if (m_isSelecting && input.isMouseDown(MouseButton::Left)) {
        m_cursor = screenToTextPos(ctx, mp, bounds, rowHeight, charWidth, gutterWidth);
        m_selection.start = m_selectionAnchor;
        m_selection.end = m_cursor;
        ensureCursorVisible(bounds, rowHeight);
    }
    if (input.isMouseReleased(MouseButton::Left)) m_isSelecting = false;
}


void TextEditor::insertText(const std::string& text) {
    TextPosition start = m_cursor;
    m_lines[m_cursor.line].insert(m_cursor.column, text);
    m_cursor.column += (int)text.length();
    recordAction(EditActionType::Insert, text, start, m_cursor, start);
}

void TextEditor::deleteSelection() {
    if (m_selection.isEmpty()) return;
    TextPosition minP = m_selection.min(), maxP = m_selection.max();
    TextPosition cursorBefore = m_cursor;
    std::string deletedText = getTextRange(minP, maxP);
    
    if (minP.line == maxP.line) m_lines[minP.line].erase(minP.column, maxP.column - minP.column);
    else {
        m_lines[minP.line] = m_lines[minP.line].substr(0, minP.column) + m_lines[maxP.line].substr(maxP.column);
        m_lines.erase(m_lines.begin() + minP.line + 1, m_lines.begin() + maxP.line + 1);
    }
    m_cursor = minP;
    recordAction(EditActionType::Delete, deletedText, minP, maxP, cursorBefore);
    m_selection.clear();
}

void TextEditor::backspace() {
    TextPosition cursorBefore = m_cursor;
    if (m_cursor.column > 0) {
        TextPosition start = {m_cursor.line, m_cursor.column - 1};
        std::string deleted = getTextRange(start, m_cursor);
        m_lines[m_cursor.line].erase(m_cursor.column - 1, 1);
        m_cursor.column--;
        recordAction(EditActionType::Delete, deleted, start, cursorBefore, cursorBefore);
    }
    else if (m_cursor.line > 0) {
        TextPosition start = {m_cursor.line - 1, (int)m_lines[m_cursor.line - 1].length()};
        int oldLen = (int)m_lines[m_cursor.line - 1].length();
        m_lines[m_cursor.line - 1] += m_lines[m_cursor.line];
        m_lines.erase(m_lines.begin() + m_cursor.line);
        m_cursor.line--; m_cursor.column = oldLen;
        recordAction(EditActionType::Delete, "\n", start, cursorBefore, cursorBefore);
    }
}

void TextEditor::enter() {
    TextPosition cursorBefore = m_cursor;
    std::string line = m_lines[m_cursor.line];
    size_t first = line.find_first_not_of(" \t");
    std::string indent = (m_cursor.column > (int)first) ? line.substr(0, first) : "";
    std::string rem = line.substr(m_cursor.column);
    m_lines[m_cursor.line].erase(m_cursor.column);
    m_lines.insert(m_lines.begin() + m_cursor.line + 1, indent + rem);
    m_cursor.line++; m_cursor.column = (int)indent.length();
    recordAction(EditActionType::Insert, "\n" + indent, cursorBefore, m_cursor, cursorBefore);
}

void TextEditor::copyToClipboard(Context& ctx) {
    if (!m_selection.isEmpty()) ctx.window().setClipboardText(getSelectedText());
}


void TextEditor::cutToClipboard(Context& ctx) {
    if (!m_selection.isEmpty()) { copyToClipboard(ctx); deleteSelection(); }
}


void TextEditor::pasteFromClipboard(Context& ctx) {
    std::string text = ctx.window().getClipboardText();

    if (text.empty()) return;
    if (!m_selection.isEmpty()) deleteSelection();
    std::stringstream ss(text);
    std::string segment;
    bool first = true;
    while (std::getline(ss, segment, '\n')) {
        if (!segment.empty() && segment.back() == '\r') segment.pop_back();
        if (!first) enter();
        insertText(segment);
        first = false;
    }
}

std::string TextEditor::getSelectedText() const {
    return getTextRange(m_selection.min(), m_selection.max());
}

std::string TextEditor::getTextRange(TextPosition start, TextPosition end) const {
    if (start == end) return "";
    if (end < start) std::swap(start, end);

    if (start.line == end.line) return m_lines[start.line].substr(start.column, end.column - start.column);
    
    std::string result = m_lines[start.line].substr(start.column) + "\n";
    for (int i = start.line + 1; i < end.line; ++i) result += m_lines[i] + "\n";
    result += m_lines[end.line].substr(0, end.column);
    return result;
}

void TextEditor::ensureCursorVisible(const Rect& bounds, float rowHeight) {
    float y = m_cursor.line * rowHeight;
    if (y < m_scrollOffset.y) m_scrollOffset.y = y;
    else if (y + rowHeight > m_scrollOffset.y + bounds.height()) m_scrollOffset.y = y + rowHeight - bounds.height();
}

TextPosition TextEditor::screenToTextPos(Context& ctx, const Vec2& mp, const Rect& bounds, float rowHeight, float charWidth, float gutterWidth) {
    float ly = mp.y - bounds.y() + m_scrollOffset.y;
    int line = std::clamp((int)(ly / rowHeight), 0, (int)m_lines.size() - 1);
    float lx = mp.x - bounds.x() - gutterWidth - 5 + m_scrollOffset.x;
    const std::string& s = m_lines[line];
    int col = 0;
    float curX = 0;
    Font* f = ctx.font();

    for (int i = 0; i < (int)s.length(); ++i) {
        float cw = f->measureText(s.substr(i, 1)).x;
        if (curX + cw/2 > lx) break;
        curX += cw; col++;
    }
    return {line, col};
}

} // namespace fst
