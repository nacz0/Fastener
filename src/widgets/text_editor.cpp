#include "fastener/widgets/text_editor.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/platform/window.h"
#include <sstream>
#include <algorithm>

namespace fst {

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
}

void TextEditor::render(const Rect& bounds, const TextEditorOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return;

    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    Font* font = ctx->font();
    if (!font) return;

    InputState& input = ctx->input();

    dl.addRectFilled(bounds, theme.colors.windowBackground);
    dl.addRect(bounds, theme.colors.border);

    float rowHeight = font->lineHeight() * options.lineHeight;
    float charWidth = font->measureText("M").x; 
    float gutterWidth = options.showLineNumbers ? 50.0f : 0.0f;

    handleInput(bounds, rowHeight, charWidth, gutterWidth);

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
            if (ctx->window().isFocused() && static_cast<int>(ctx->time() * 2) % 2 == 0) {
                float cx = textBounds.x() + 5 + font->measureText(m_lines[i].substr(0, m_cursor.column)).x;
                dl.addLine(Vec2(cx, y + 2), Vec2(cx, y + rowHeight - 2), theme.colors.primary, 2.0f);
            }
        }

        // Text
        dl.addText(font, Vec2(textBounds.x() + 5, y + (rowHeight - font->lineHeight()) / 2), m_lines[i], theme.colors.text);
    }

    dl.popClipRect();
}

void TextEditor::setCursor(const TextPosition& pos) {
    m_cursor.line = std::clamp(pos.line, 0, static_cast<int>(m_lines.size() - 1));
    m_cursor.column = std::clamp(pos.column, 0, static_cast<int>(m_lines[m_cursor.line].length()));
}

void TextEditor::handleInput(const Rect& bounds, float rowHeight, float charWidth, float gutterWidth) {
    Context* ctx = Context::current();
    float dt = ctx->deltaTime();
    
    handleKeyboard(bounds, rowHeight, dt);
    handleMouse(bounds, rowHeight, charWidth, gutterWidth);
    
    InputState& input = ctx->input();
    if (bounds.contains(input.mousePos())) {
        m_scrollOffset.y = std::max(0.0f, m_scrollOffset.y - input.scrollDelta().y * rowHeight * 3.0f);
    }
}

void TextEditor::handleKeyboard(const Rect& bounds, float rowHeight, float deltaTime) {
    InputState& input = Context::current()->input();
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
    
    if (ctrl && input.isKeyPressed(Key::C)) copyToClipboard();
    if (ctrl && input.isKeyPressed(Key::V)) pasteFromClipboard();
    if (ctrl && input.isKeyPressed(Key::X)) cutToClipboard();
    if (ctrl && input.isKeyPressed(Key::A)) {
        m_selection.start = {0, 0};
        m_selection.end = {(int)m_lines.size() - 1, (int)m_lines.back().length()};
        m_cursor = m_selection.end;
    }

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
    if (input.isKeyPressed(Key::Enter)) {
        if (!m_selection.isEmpty()) deleteSelection();
        enter();
        ensureCursorVisible(bounds, rowHeight);
    }
}

void TextEditor::handleMouse(const Rect& bounds, float rowHeight, float charWidth, float gutterWidth) {
    InputState& input = Context::current()->input();
    Vec2 mp = input.mousePos();

    if (input.isMousePressed(MouseButton::Left) && bounds.contains(mp)) {
        m_isSelecting = true;
        m_cursor = screenToTextPos(mp, bounds, rowHeight, charWidth, gutterWidth);
        if (!input.modifiers().shift) {
            m_selection.clear();
            m_selectionAnchor = m_cursor;
        } else {
            m_selection.start = m_selectionAnchor;
            m_selection.end = m_cursor;
        }
    }
    if (m_isSelecting && input.isMouseDown(MouseButton::Left)) {
        m_cursor = screenToTextPos(mp, bounds, rowHeight, charWidth, gutterWidth);
        m_selection.start = m_selectionAnchor;
        m_selection.end = m_cursor;
        ensureCursorVisible(bounds, rowHeight);
    }
    if (input.isMouseReleased(MouseButton::Left)) m_isSelecting = false;
}

void TextEditor::insertText(const std::string& text) {
    m_lines[m_cursor.line].insert(m_cursor.column, text);
    m_cursor.column += (int)text.length();
}

void TextEditor::deleteSelection() {
    if (m_selection.isEmpty()) return;
    TextPosition minP = m_selection.min(), maxP = m_selection.max();
    if (minP.line == maxP.line) m_lines[minP.line].erase(minP.column, maxP.column - minP.column);
    else {
        m_lines[minP.line] = m_lines[minP.line].substr(0, minP.column) + m_lines[maxP.line].substr(maxP.column);
        m_lines.erase(m_lines.begin() + minP.line + 1, m_lines.begin() + maxP.line + 1);
    }
    m_cursor = minP;
    m_selection.clear();
}

void TextEditor::backspace() {
    if (m_cursor.column > 0) { m_lines[m_cursor.line].erase(m_cursor.column - 1, 1); m_cursor.column--; }
    else if (m_cursor.line > 0) {
        int oldLen = (int)m_lines[m_cursor.line - 1].length();
        m_lines[m_cursor.line - 1] += m_lines[m_cursor.line];
        m_lines.erase(m_lines.begin() + m_cursor.line);
        m_cursor.line--; m_cursor.column = oldLen;
    }
}

void TextEditor::enter() {
    std::string line = m_lines[m_cursor.line];
    size_t first = line.find_first_not_of(" \t");
    std::string indent = (m_cursor.column > (int)first) ? line.substr(0, first) : "";
    std::string rem = line.substr(m_cursor.column);
    m_lines[m_cursor.line].erase(m_cursor.column);
    m_lines.insert(m_lines.begin() + m_cursor.line + 1, indent + rem);
    m_cursor.line++; m_cursor.column = (int)indent.length();
}

void TextEditor::copyToClipboard() {
    if (!m_selection.isEmpty()) Context::current()->window().setClipboardText(getSelectedText());
}

void TextEditor::cutToClipboard() {
    if (!m_selection.isEmpty()) { copyToClipboard(); deleteSelection(); }
}

void TextEditor::pasteFromClipboard() {
    std::string text = Context::current()->window().getClipboardText();
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
    if (m_selection.isEmpty()) return "";
    TextPosition minP = m_selection.min(), maxP = m_selection.max();
    if (minP.line == maxP.line) return m_lines[minP.line].substr(minP.column, maxP.column - minP.column);
    std::string result = m_lines[minP.line].substr(minP.column) + "\n";
    for (int i = minP.line + 1; i < maxP.line; ++i) result += m_lines[i] + "\n";
    result += m_lines[maxP.line].substr(0, maxP.column);
    return result;
}

void TextEditor::ensureCursorVisible(const Rect& bounds, float rowHeight) {
    float y = m_cursor.line * rowHeight;
    if (y < m_scrollOffset.y) m_scrollOffset.y = y;
    else if (y + rowHeight > m_scrollOffset.y + bounds.height()) m_scrollOffset.y = y + rowHeight - bounds.height();
}

TextPosition TextEditor::screenToTextPos(const Vec2& mp, const Rect& bounds, float rowHeight, float charWidth, float gutterWidth) {
    float ly = mp.y - bounds.y() + m_scrollOffset.y;
    int line = std::clamp((int)(ly / rowHeight), 0, (int)m_lines.size() - 1);
    float lx = mp.x - bounds.x() - gutterWidth - 5 + m_scrollOffset.x;
    const std::string& s = m_lines[line];
    int col = 0;
    float curX = 0;
    Font* f = Context::current()->font();
    for (int i = 0; i < (int)s.length(); ++i) {
        float cw = f->measureText(s.substr(i, 1)).x;
        if (curX + cw/2 > lx) break;
        curX += cw; col++;
    }
    return {line, col};
}

} // namespace fst
