#include "fastener/core/input.h"
#include <cstring>

namespace fst {

bool InputState::isKeyDown(Key key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= static_cast<int>(Key::MaxKey)) return false;
    return m_keysDown[idx];
}

bool InputState::isKeyPressed(Key key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= static_cast<int>(Key::MaxKey)) return false;
    return m_keysPressed[idx];
}

bool InputState::isKeyReleased(Key key) const {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= static_cast<int>(Key::MaxKey)) return false;
    return m_keysReleased[idx];
}

bool InputState::isMouseDown(MouseButton button) const {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mouseDown[idx];
}

bool InputState::isMousePressed(MouseButton button) const {
    // Return false if mouse input was consumed (e.g., by toast/overlay)
    if (m_mouseConsumed) return false;
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mousePressed[idx];
}

bool InputState::isMousePressedRaw(MouseButton button) const {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mousePressed[idx];
}

bool InputState::isMouseReleased(MouseButton button) const {
    // Return false if mouse input was consumed (e.g., by toast/overlay)
    if (m_mouseConsumed) return false;
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mouseReleased[idx];
}

bool InputState::isMouseReleasedRaw(MouseButton button) const {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mouseReleased[idx];
}

bool InputState::isMouseDoubleClicked(MouseButton button) const {
    // Return false if mouse input was consumed (e.g., by toast/overlay)
    if (m_mouseConsumed) return false;
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mouseDoubleClicked[idx];
}

bool InputState::isMouseDoubleClickedRaw(MouseButton button) const {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return false;
    return m_mouseDoubleClicked[idx];
}

void InputState::beginFrame() {
    // Clear per-frame state
    m_keysPressed.fill(false);
    m_keysReleased.fill(false);
    m_mousePressed.fill(false);
    m_mouseReleased.fill(false);
    m_mouseDoubleClicked.fill(false);
    m_scrollDelta = Vec2::zero();
    m_textInput.clear();
    
    // Calculate mouse delta
    m_mouseDelta = m_mousePos - m_lastMousePos;
    m_lastMousePos = m_mousePos;
    m_mouseConsumed = false;
}

void InputState::endFrame() {
    // Nothing to do yet
}

void InputState::onKeyDown(Key key) {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= static_cast<int>(Key::MaxKey)) return;
    
    if (!m_keysDown[idx]) {
        m_keysPressed[idx] = true;
    }
    m_keysDown[idx] = true;
}

void InputState::onKeyUp(Key key) {
    int idx = static_cast<int>(key);
    if (idx < 0 || idx >= static_cast<int>(Key::MaxKey)) return;
    
    m_keysDown[idx] = false;
    m_keysReleased[idx] = true;
}

void InputState::onMouseDown(MouseButton button) {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return;
    
    if (!m_mouseDown[idx]) {
        m_mousePressed[idx] = true;
        
        // Check for double click
        float currentTime = m_frameTime;
        if (currentTime - m_lastClickTime[idx] < DOUBLE_CLICK_TIME) {
            m_mouseDoubleClicked[idx] = true;
        }
        m_lastClickTime[idx] = currentTime;
    }
    m_mouseDown[idx] = true;
}

void InputState::onMouseUp(MouseButton button) {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::MaxButton)) return;
    
    m_mouseDown[idx] = false;
    m_mouseReleased[idx] = true;
}

void InputState::onMouseMove(float x, float y) {
    m_mousePos = {x, y};
}

void InputState::onMouseScroll(float dx, float dy) {
    m_scrollDelta = {dx, dy};
}

void InputState::onTextInput(char32_t codepoint) {
    // Convert UTF-32 to UTF-8
    if (codepoint < 0x80) {
        m_textInput += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        m_textInput += static_cast<char>(0xC0 | (codepoint >> 6));
        m_textInput += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        m_textInput += static_cast<char>(0xE0 | (codepoint >> 12));
        m_textInput += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        m_textInput += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        m_textInput += static_cast<char>(0xF0 | (codepoint >> 18));
        m_textInput += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        m_textInput += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        m_textInput += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

void InputState::onModifiersChanged(bool shift, bool ctrl, bool alt, bool super) {
    m_modifiers.shift = shift;
    m_modifiers.ctrl = ctrl;
    m_modifiers.alt = alt;
    m_modifiers.super = super;
}

void InputState::onResize(float width, float height) {
    m_windowSize = {width, height};
}

void InputState::setFrameTime(float time) {
    m_frameTime = time;
}

} // namespace fst
