#include <gtest/gtest.h>
#include <fastener/core/input.h>

using namespace fst;

//=============================================================================
// InputState Key Bounds Tests
//=============================================================================

TEST(InputStateTest, KeyBoundsCheckingDown) {
    InputState input;
    
    // Valid keys should work
    input.onKeyDown(Key::A);
    EXPECT_TRUE(input.isKeyDown(Key::A));
    
    // Invalid key (negative) should return false, not crash
    EXPECT_FALSE(input.isKeyDown(Key::Unknown));  // -1
}

TEST(InputStateTest, KeyBoundsCheckingPressed) {
    InputState input;
    
    input.onKeyDown(Key::Space);
    EXPECT_TRUE(input.isKeyPressed(Key::Space));
    
    // Out of bounds should return false
    EXPECT_FALSE(input.isKeyPressed(Key::Unknown));
}

TEST(InputStateTest, KeyDownUp) {
    InputState input;
    
    // Key starts not pressed
    EXPECT_FALSE(input.isKeyDown(Key::Enter));
    
    // Press key
    input.onKeyDown(Key::Enter);
    EXPECT_TRUE(input.isKeyDown(Key::Enter));
    
    // Release key
    input.onKeyUp(Key::Enter);
    EXPECT_FALSE(input.isKeyDown(Key::Enter));
}

//=============================================================================
// InputState Mouse Bounds Tests
//=============================================================================

TEST(InputStateTest, MouseBoundsChecking) {
    InputState input;
    
    // Valid button
    input.onMouseDown(MouseButton::Left);
    EXPECT_TRUE(input.isMouseDown(MouseButton::Left));
    
    // Button should be marked as pressed this frame
    EXPECT_TRUE(input.isMousePressed(MouseButton::Left));
}

TEST(InputStateTest, MouseDownUp) {
    InputState input;
    
    EXPECT_FALSE(input.isMouseDown(MouseButton::Right));
    
    input.onMouseDown(MouseButton::Right);
    EXPECT_TRUE(input.isMouseDown(MouseButton::Right));
    
    input.onMouseUp(MouseButton::Right);
    EXPECT_FALSE(input.isMouseDown(MouseButton::Right));
}

//=============================================================================
// InputState Double-Click Tests
//=============================================================================

TEST(InputStateTest, DoubleClickDetection) {
    InputState input;
    
    // Set frame time so double-click can be detected
    input.setFrameTime(1.0f);
    
    // First click - not a double click
    input.onMouseDown(MouseButton::Left);
    EXPECT_FALSE(input.isMouseDoubleClicked(MouseButton::Left));
    input.onMouseUp(MouseButton::Left);
    
    // Begin new frame
    input.beginFrame();
    
    // Second click within DOUBLE_CLICK_TIME (0.3s)
    input.setFrameTime(1.2f);  // 0.2s later
    input.onMouseDown(MouseButton::Left);
    EXPECT_TRUE(input.isMouseDoubleClicked(MouseButton::Left));
}

TEST(InputStateTest, DoubleClickTimingTooSlow) {
    InputState input;
    
    // First click
    input.setFrameTime(1.0f);
    input.onMouseDown(MouseButton::Left);
    input.onMouseUp(MouseButton::Left);
    
    input.beginFrame();
    
    // Second click after DOUBLE_CLICK_TIME has passed
    input.setFrameTime(2.0f);  // 1.0s later - too slow
    input.onMouseDown(MouseButton::Left);
    EXPECT_FALSE(input.isMouseDoubleClicked(MouseButton::Left));
}

//=============================================================================
// InputState Mouse Position Tests
//=============================================================================

TEST(InputStateTest, MousePosition) {
    InputState input;
    
    input.onMouseMove(100.0f, 200.0f);
    EXPECT_FLOAT_EQ(input.mousePos().x, 100.0f);
    EXPECT_FLOAT_EQ(input.mousePos().y, 200.0f);
}

TEST(InputStateTest, MouseDelta) {
    InputState input;
    
    // Set initial position and run a frame so lastMousePos is set
    input.onMouseMove(100.0f, 100.0f);
    input.beginFrame();  // This sets lastMousePos = mousePos
    
    // Move mouse to new position
    input.onMouseMove(150.0f, 120.0f);
    input.beginFrame();  // This calculates delta = mousePos - lastMousePos
    
    // Delta should be the difference
    Vec2 delta = input.mouseDelta();
    EXPECT_FLOAT_EQ(delta.x, 50.0f);
    EXPECT_FLOAT_EQ(delta.y, 20.0f);
}

//=============================================================================
// InputState Scroll Tests
//=============================================================================

TEST(InputStateTest, ScrollDelta) {
    InputState input;
    
    input.onMouseScroll(0.0f, 3.0f);  // Scroll up 3 units
    Vec2 scroll = input.scrollDelta();
    
    EXPECT_FLOAT_EQ(scroll.x, 0.0f);
    EXPECT_FLOAT_EQ(scroll.y, 3.0f);
}

//=============================================================================
// InputState Modifiers Tests
//=============================================================================

TEST(InputStateTest, Modifiers) {
    InputState input;
    
    Modifiers m = input.modifiers();
    EXPECT_FALSE(m.shift);
    EXPECT_FALSE(m.ctrl);
    EXPECT_FALSE(m.alt);
    EXPECT_FALSE(m.super);
    
    input.onModifiersChanged(true, true, false, false);
    m = input.modifiers();
    EXPECT_TRUE(m.shift);
    EXPECT_TRUE(m.ctrl);
    EXPECT_FALSE(m.alt);
    EXPECT_FALSE(m.super);
}

TEST(InputStateTest, ModifiersNone) {
    Modifiers m;
    EXPECT_TRUE(m.none());
    EXPECT_FALSE(m.any());
    
    m.shift = true;
    EXPECT_FALSE(m.none());
    EXPECT_TRUE(m.any());
}

//=============================================================================
// InputState Text Input Tests
//=============================================================================

TEST(InputStateTest, TextInput) {
    InputState input;
    
    // ASCII character
    input.onTextInput('A');
    EXPECT_EQ(input.textInput(), "A");
}

TEST(InputStateTest, TextInputCleared) {
    InputState input;
    
    input.onTextInput('X');
    EXPECT_EQ(input.textInput(), "X");
    
    input.beginFrame();
    EXPECT_EQ(input.textInput(), "");  // Cleared at frame start
}
