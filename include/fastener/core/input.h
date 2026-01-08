#pragma once

#include "fastener/core/types.h"
#include <array>
#include <string>

namespace fst {

//=============================================================================
// Key Codes
//=============================================================================
enum class Key : int {
    Unknown = -1,
    
    // Printable keys
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon = 59,
    Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,
    
    // Function keys
    Escape = 256,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock = 280,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Keypad
    KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
    KPDecimal,
    KPDivide,
    KPMultiply,
    KPSubtract,
    KPAdd,
    KPEnter,
    KPEqual,
    
    // Modifiers
    LeftShift = 340,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu,
    
    // Count
    MaxKey = 512
};

//=============================================================================
// Mouse Buttons
//=============================================================================
enum class MouseButton : int {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    
    MaxButton = 8
};

//=============================================================================
// Modifier Keys
//=============================================================================
struct Modifiers {
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool super = false;  // Windows/Command key
    
    bool none() const { return !shift && !ctrl && !alt && !super; }
    bool any() const { return shift || ctrl || alt || super; }
    
    bool operator==(const Modifiers& other) const {
        return shift == other.shift && ctrl == other.ctrl && 
               alt == other.alt && super == other.super;
    }
};

//=============================================================================
// Input State
//=============================================================================
class InputState {
public:
    InputState() = default;
    
    // Keyboard
    bool isKeyDown(Key key) const;
    bool isKeyPressed(Key key) const;   // Just pressed this frame
    bool isKeyReleased(Key key) const;  // Just released this frame
    
    // Mouse buttons
    bool isMouseDown(MouseButton button) const;
    bool isMousePressed(MouseButton button) const;
    bool isMouseReleased(MouseButton button) const;
    bool isMouseDoubleClicked(MouseButton button) const;
    
    // Mouse position & movement
    Vec2 mousePos() const { return m_mousePos; }
    Vec2 mouseDelta() const { return m_mouseDelta; }
    Vec2 scrollDelta() const { return m_scrollDelta; }
    Vec2 windowSize() const { return m_windowSize; }
    
    // Modifiers
    Modifiers modifiers() const { return m_modifiers; }
    
    // Text input (for this frame)
    const std::string& textInput() const { return m_textInput; }
    
    // Frame management (called by Context)
    void beginFrame();
    void endFrame();
    
    // Event handlers (called by Window)
    void onKeyDown(Key key);
    void onKeyUp(Key key);
    void onMouseDown(MouseButton button);
    void onMouseUp(MouseButton button);
    void onMouseMove(float x, float y);
    void onMouseScroll(float dx, float dy);
    void onTextInput(char32_t codepoint);
    void onModifiersChanged(bool shift, bool ctrl, bool alt, bool super);
    void onResize(float width, float height);
    void setFrameTime(float time);
    
private:
    // Keyboard state
    std::array<bool, static_cast<int>(Key::MaxKey)> m_keysDown{};
    std::array<bool, static_cast<int>(Key::MaxKey)> m_keysPressed{};
    std::array<bool, static_cast<int>(Key::MaxKey)> m_keysReleased{};
    
    // Mouse state
    std::array<bool, static_cast<int>(MouseButton::MaxButton)> m_mouseDown{};
    std::array<bool, static_cast<int>(MouseButton::MaxButton)> m_mousePressed{};
    std::array<bool, static_cast<int>(MouseButton::MaxButton)> m_mouseReleased{};
    std::array<bool, static_cast<int>(MouseButton::MaxButton)> m_mouseDoubleClicked{};
    std::array<float, static_cast<int>(MouseButton::MaxButton)> m_lastClickTime{};
    
    Vec2 m_mousePos;
    Vec2 m_lastMousePos;
    Vec2 m_mouseDelta;
    Vec2 m_scrollDelta;
    Vec2 m_windowSize;
    
    Modifiers m_modifiers;
    std::string m_textInput;
    float m_frameTime = 0.0f;
    
    static constexpr float DOUBLE_CLICK_TIME = 0.3f;  // seconds
};

} // namespace fst
