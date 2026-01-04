#pragma once

#include "fastener/core/types.h"
#include "fastener/core/input.h"
#include <string>
#include <vector>
#include <functional>

namespace fst {

struct TextPosition {
    int line = 0;
    int column = 0;

    bool operator==(const TextPosition& other) const {
        return line == other.line && column == other.column;
    }
    bool operator!=(const TextPosition& other) const { return !(*this == other); }
    bool operator<(const TextPosition& other) const {
        if (line != other.line) return line < other.line;
        return column < other.column;
    }
};

struct TextSelection {
    TextPosition start;
    TextPosition end;

    bool isEmpty() const { return start == end; }
    void clear() { start = end = TextPosition(); }
    
    TextPosition min() const { return start < end ? start : end; }
    TextPosition max() const { return start < end ? end : start; }
};

struct TextEditorOptions {
    float fontSize = 14.0f;
    bool showLineNumbers = true;
    bool readOnly = false;
    bool wordWrap = false;
    float lineHeight = 1.2f;  // Relative to font height
};

class TextEditor {
public:
    TextEditor();
    ~TextEditor();

    void setText(const std::string& text);
    std::string getText() const;
    void clear();

    void render(const Rect& bounds, const TextEditorOptions& options = {});

    // State access
    const TextPosition& cursor() const { return m_cursor; }
    void setCursor(const TextPosition& pos);

private:
    std::vector<std::string> m_lines;
    TextPosition m_cursor;
    TextSelection m_selection;
    bool m_isSelecting = false;
    TextPosition m_selectionAnchor;
    
    Key m_lastRepeatKey = Key::Unknown;
    float m_repeatTimer = 0.0f;
    
    Vec2 m_scrollOffset = {0, 0};
    float m_contentWidth = 0;
    float m_contentHeight = 0;

    // Internal helpers
    void handleInput(const Rect& bounds, float rowHeight, float charWidth, float gutterWidth);
    void handleKeyboard(const Rect& bounds, float rowHeight, float deltaTime);
    void handleMouse(const Rect& bounds, float rowHeight, float charWidth, float gutterWidth);
    
    void insertText(const std::string& text);
    void deleteSelection();
    void backspace();
    void enter();
    
    void copyToClipboard();
    void pasteFromClipboard();
    void cutToClipboard();
    
    std::string getSelectedText() const;
    
    void ensureCursorVisible(const Rect& bounds, float rowHeight);
    TextPosition screenToTextPos(const Vec2& screenPos, const Rect& bounds, float rowHeight, float charWidth, float gutterWidth);
};

} // namespace fst
