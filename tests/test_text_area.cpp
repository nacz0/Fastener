#include <gtest/gtest.h>
#include <fastener/core/context.h>
#include <fastener/platform/window.h>
#include <fastener/widgets/text_area.h>

using namespace fst;

class ClipboardWindowStub : public IPlatformWindow {
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
    std::string getClipboardText() const override { return clipboard; }
    void setClipboardText(const std::string& text) override { clipboard = text; }
    void* nativeHandle() const override { return nullptr; }
    InputState& input() override { return inputState; }
    const InputState& input() const override { return inputState; }

    std::string clipboard;
    InputState inputState;
};

TEST(TextAreaClipboardTest, CtrlAThenCtrlCUsesClipboard) {
    ClipboardWindowStub window;
    Context ctx(false);

    std::string value = "Hello World";
    TextAreaOptions options;
    options.style = Style().withSize(200, 100);

    window.input().beginFrame();
    ctx.beginFrame(window);
    WidgetId id = ctx.makeId("clip");
    ctx.setFocusedWidget(id);
    window.input().onModifiersChanged(false, true, false, false);
    window.input().onKeyDown(Key::A);
    TextArea(ctx, "clip", value, options);
    ctx.endFrame();

    window.input().beginFrame();
    ctx.beginFrame(window);
    ctx.setFocusedWidget(id);
    window.input().onModifiersChanged(false, true, false, false);
    window.input().onKeyDown(Key::C);
    TextArea(ctx, "clip", value, options);
    ctx.endFrame();

    EXPECT_EQ(window.clipboard, "Hello World");
}

TEST(TextAreaClipboardTest, CtrlAThenCtrlVPastesClipboard) {
    ClipboardWindowStub window;
    Context ctx(false);

    std::string value = "Hello World";
    window.clipboard = "Hi";

    TextAreaOptions options;
    options.style = Style().withSize(200, 100);

    window.input().beginFrame();
    ctx.beginFrame(window);
    WidgetId id = ctx.makeId("paste");
    ctx.setFocusedWidget(id);
    window.input().onModifiersChanged(false, true, false, false);
    window.input().onKeyDown(Key::A);
    TextArea(ctx, "paste", value, options);
    ctx.endFrame();

    window.input().beginFrame();
    ctx.beginFrame(window);
    ctx.setFocusedWidget(id);
    window.input().onModifiersChanged(false, true, false, false);
    window.input().onKeyDown(Key::V);
    TextArea(ctx, "paste", value, options);
    ctx.endFrame();

    EXPECT_EQ(value, "Hi");
}

TEST(TextAreaContextTest, CursorStateIsIsolatedForMatchingIds) {
    ClipboardWindowStub firstWindow;
    ClipboardWindowStub secondWindow;
    Context firstContext(false);
    Context secondContext(false);
    TextAreaOptions options;
    options.style = Style().withSize(200, 100);

    std::string firstValue = "AAAA";
    firstWindow.input().beginFrame();
    firstContext.beginFrame(firstWindow);
    WidgetId firstId = firstContext.makeId("context-shared-cursor");
    firstContext.setFocusedWidget(firstId);
    firstWindow.input().onKeyDown(Key::End);
    TextArea(firstContext, "context-shared-cursor", firstValue, options);
    firstContext.endFrame();

    std::string secondValue = "BBBB";
    secondWindow.input().beginFrame();
    secondContext.beginFrame(secondWindow);
    WidgetId secondId = secondContext.makeId("context-shared-cursor");
    secondContext.setFocusedWidget(secondId);
    secondWindow.input().onTextInput(U'x');
    TextArea(secondContext, "context-shared-cursor", secondValue, options);
    secondContext.endFrame();

    EXPECT_EQ(secondValue, "xBBBB");
}
