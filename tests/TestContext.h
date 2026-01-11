#pragma once

/**
 * @file test_context.h
 * @brief RAII helper for widget unit tests with dependency injection.
 */

#include <fastener/core/context.h>
#include <fastener/ui/widget_scope.h>
#include <fastener/ui/widget_utils.h>
#include "MockDrawList.h"

namespace fst::testing {

/**
 * @brief Stub window implementation for testing (no actual window created).
 */
class StubWindow : public fst::IPlatformWindow {
public:
    int width() const override { return 800; }
    int height() const override { return 600; }
    bool isOpen() const override { return true; }
    void close() override {}
    void pollEvents() override {}
    void waitEvents() override {}
    void swapBuffers() override {}
    void makeContextCurrent() override {}
    Vec2 size() const override { return {800.0f, 600.0f}; }
    Vec2 framebufferSize() const override { return {800.0f, 600.0f}; }
    float dpiScale() const override { return 1.0f; }
    void setTitle(const std::string&) override {}
    void setSize(int, int) override {}
    void setPosition(int, int) override {}
    void minimize() override {}
    void maximize() override {}
    void restore() override {}
    void focus() override {}
    bool isMinimized() const override { return false; }
    bool isMaximized() const override { return false; }
    bool isFocused() const override { return true; }
    void setCursor(Cursor) override {}
    void hideCursor() override {}
    void showCursor() override {}
    std::string getClipboardText() const override { return ""; }
    void setClipboardText(const std::string&) override {}
    void* nativeHandle() const override { return nullptr; }
    InputState& input() override { return m_input; }
    const InputState& input() const override { return m_input; }
    
private:
    InputState m_input;
};

/**
 * @brief RAII test helper that sets up minimal Context for widget tests.
 * 
 * TestContext creates a Context without a renderer, sets up a MockDrawList,
 * and manages the context stack automatically. This enables clean unit testing
 * of widgets without requiring a real window or OpenGL context.
 * 
 * Usage:
 * @code
 *   TEST(MyWidget, ShouldRenderButton) {
 *       TestContext tc;
 *       auto& mockDl = tc.mockDrawList();
 *       EXPECT_CALL(mockDl, addRectFilled(_, _, _)).Times(1);
 *       
 *       tc.beginFrame();  // Optional - for widgets that need frame state
 *       Button("Test");   // Uses tc.context() via context stack
 *       tc.endFrame();
 *   }
 * @endcode
 */
class TestContext {
public:
    TestContext() : m_ctx(false) {  // No renderer
        Context::setTestDrawList(&m_mockDl);
        
        // Setup default mock behaviors
        ON_CALL(m_mockDl, resolveColor(::testing::_))
            .WillByDefault([](Color c) { return c; });
        ON_CALL(m_mockDl, currentClipRect())
            .WillByDefault([]() { return Rect(0, 0, 1920, 1080); });
        ON_CALL(m_mockDl, currentColor())
            .WillByDefault([]() { return Color::white(); });
        ON_CALL(m_mockDl, currentLayer())
            .WillByDefault(::testing::Return(DrawLayer::Default));
    }
    
    ~TestContext() {
        if (m_frameActive) {
            m_ctx.endFrame();
        }
        Context::setTestDrawList(nullptr);
    }
    
    // Non-copyable
    TestContext(const TestContext&) = delete;
    TestContext& operator=(const TestContext&) = delete;
    
    /**
     * @brief Begin a frame for testing widgets that need frame state.
     */
    void beginFrame() {
        m_ctx.beginFrame(m_window);
        m_frameActive = true;
    }
    
    /**
     * @brief End the current frame.
     */
    void endFrame() {
        if (m_frameActive) {
            m_ctx.endFrame();
            m_frameActive = false;
        }
    }
    
    /** @brief Get the test Context. */
    Context& context() { return m_ctx; }
    
    /** @brief Get the mock DrawList for setting expectations. */
    MockDrawList& mockDrawList() { return m_mockDl; }
    
    /** @brief Get the stub window. */
    StubWindow& window() { return m_window; }
    
    /** @brief Check if a frame is currently active. */
    bool isFrameActive() const { return m_frameActive; }

private:
    StubWindow m_window;
    Context m_ctx;
    MockDrawList m_mockDl;
    bool m_frameActive = false;
};

} // namespace fst::testing
