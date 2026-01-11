/**
 * @file test_widget_rendering.cpp
 * @brief Unit tests for widget rendering using MockDrawList.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MockDrawList.h"
#include <fastener/core/context.h>
#include <fastener/core/types.h>
#include <fastener/ui/theme.h>
#include <fastener/ui/widget_utils.h>
#include <fastener/ui/drag_drop.h>
#include <fastener/platform/window.h>
#include <fastener/graphics/draw_list.h>

using namespace fst;
using namespace fst::testing;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Return;

//=============================================================================
// Test Stubs
//=============================================================================

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
    void setSize(int , int ) override {}
    void setPosition(int , int ) override {}
    void minimize() override {}
    void maximize() override {}
    void restore() override {}
    void focus() override {}
    bool isMinimized() const override { return false; }
    bool isMaximized() const override { return false; }
    bool isFocused() const override { return true; }
    void setCursor(Cursor ) override {}
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

//=============================================================================
// Test Fixture
//=============================================================================

class WidgetRenderingTest : public ::testing::Test {
protected:
    MockDrawList mockDl;
    Context* ctx = nullptr;
    StubWindow window;
    
    void SetUp() override {
        ctx = new Context(false);
        // Set up mock draw list for tests
        Context::setTestDrawList(&mockDl);
        
        // Set up default mock behaviors
        ON_CALL(mockDl, resolveColor(_))
            .WillByDefault([](Color c) { return c; });
        ON_CALL(mockDl, currentClipRect())
            .WillByDefault([]() { return Rect(0, 0, 1920, 1080); });
        ON_CALL(mockDl, currentColor())
            .WillByDefault([]() { return Color::white(); });
        ON_CALL(mockDl, currentLayer())
            .WillByDefault(Return(DrawLayer::Default));
    }
    
    void TearDown() override {
        // Clean up test draw list
        Context::setTestDrawList(nullptr);
        delete ctx;
    }
};

//=============================================================================
// Basic IDrawList Interface Tests
//=============================================================================

TEST_F(WidgetRenderingTest, MockDrawListBasicInterface) {
    // Verify that the mock can be called and tracked
    EXPECT_CALL(mockDl, addRectFilled(_, _, _)).Times(1);
    EXPECT_CALL(mockDl, addText(_, _, _, _)).Times(1);
    
    Rect bounds(0, 0, 100, 30);
    mockDl.addRectFilled(bounds, Color::red(), 5.0f);
    mockDl.addText(nullptr, Vec2(10, 5), "Test", Color::white());
}

TEST_F(WidgetRenderingTest, MockDrawList_AddLine) {
    EXPECT_CALL(mockDl, addLine(_, _, _, _)).Times(AtLeast(1));
    
    Vec2 p1(0, 0);
    Vec2 p2(100, 100);
    mockDl.addLine(p1, p2, Color::green(), 2.0f);
}

TEST_F(WidgetRenderingTest, MockDrawList_AddCircle) {
    EXPECT_CALL(mockDl, addCircleFilled(_, _, _, _)).Times(1);
    EXPECT_CALL(mockDl, addCircle(_, _, _, _)).Times(1);
    
    Vec2 center(50, 50);
    mockDl.addCircleFilled(center, 10.0f, Color::blue(), 16);
    mockDl.addCircle(center, 10.0f, Color::white(), 16);
}

//=============================================================================
// Color Resolution Tests
//=============================================================================

TEST_F(WidgetRenderingTest, ResolveColor_ReturnsGivenColor) {
    Color testColor(128, 64, 32, 255);
    
    EXPECT_CALL(mockDl, resolveColor(testColor))
        .WillOnce([](Color c) { return c; });
    
    Color resolved = mockDl.resolveColor(testColor);
    EXPECT_EQ(resolved.r, testColor.r);
    EXPECT_EQ(resolved.g, testColor.g);
    EXPECT_EQ(resolved.b, testColor.b);
    EXPECT_EQ(resolved.a, testColor.a);
}

//=============================================================================
// Clipping Tests
//=============================================================================

TEST_F(WidgetRenderingTest, ClipRect_PushPop) {
    Rect clipRect(10, 10, 200, 200);
    
    EXPECT_CALL(mockDl, pushClipRect(clipRect)).Times(1);
    EXPECT_CALL(mockDl, popClipRect()).Times(1);
    
    mockDl.pushClipRect(clipRect);
    mockDl.popClipRect();
}

//=============================================================================
// Drag & Drop Tests
//=============================================================================

TEST_F(WidgetRenderingTest, Context_OcclusionWorking) {
    ctx->beginFrame(window);
    ctx->addFloatingWindowRect(Rect(0, 0, 100, 100));
    EXPECT_EQ(ctx->currentFloatingRects().size(), 1);
    ctx->endFrame();
    
    ctx->beginFrame(window);
    EXPECT_EQ(ctx->prevFloatingRects().size(), 1);
    EXPECT_TRUE(ctx->isOccluded(Vec2(50, 50)));
    EXPECT_FALSE(ctx->isOccluded(Vec2(150, 150)));
    ctx->endFrame();
}

TEST_F(WidgetRenderingTest, DragDrop_OcclusionPreventsHighlight) {
    // Step 0: Ensure clean state
    CancelDragDrop(); 

    // Step 1: Start drag - Frame A: Press
    ctx->beginFrame(window);
    auto& input = ctx->input();
    
    input.onMouseMove(50, 50);
    ctx->setLastWidgetId(hashString("source"));
    ctx->setLastWidgetBounds(Rect(0, 0, 100, 100));
    input.onMouseDown(MouseButton::Left);
    
    BeginDragDropSource();
    ctx->endFrame();
    
    // Step 1: Start drag - Frame B: Move and Activate
    ctx->beginFrame(window);
    input.onMouseMove(70, 70); // Move enough to trigger drag
    ctx->setLastWidgetId(hashString("source"));
    ctx->setLastWidgetBounds(Rect(0, 0, 100, 100));
    
    bool started = BeginDragDropSource();
    EXPECT_TRUE(started) << "Drag should have started";
    SetDragDropPayload("test", nullptr, 0);
    EndDragDropSource();
    
    // Add a floating window that occludes (70, 70) for the NEXT frame
    ctx->addFloatingWindowRect(Rect(0, 0, 100, 100));
    ctx->endFrame();
    
    ASSERT_TRUE(IsDragDropActive());
    
    // Step 2: Test occlusion in third frame
    ctx->beginFrame(window);
    input.onMouseMove(70, 70); // Ensure mouse is still at (70, 70)
    
    // Verify all sub-conditions
    EXPECT_EQ(input.mousePos().x, 70.0f);
    EXPECT_EQ(input.mousePos().y, 70.0f);
    EXPECT_TRUE(Rect(40, 40, 80, 80).contains(input.mousePos())); // Bounds (40,40,40,40)
    EXPECT_EQ(ctx->prevFloatingRects().size(), 1);
    EXPECT_TRUE(ctx->isOccluded(Vec2(70, 70)));
    
    // Try to begin a target at (70, 70)
    bool isTarget = BeginDragDropTarget(Rect(40, 40, 40, 40));
    EXPECT_FALSE(isTarget) << "Target should be occluded by floating window";
    
    if (isTarget) EndDragDropTarget();
    
    ctx->endFrame();
}

TEST_F(WidgetRenderingTest, DragDrop_LateTargetUpdatesPreviewHighlight) {
    // Step 1: Start drag
    ctx->beginFrame(window);
    auto& input = ctx->input();
    input.onMouseMove(50, 50);
    
    ctx->setLastWidgetId(hashString("source"));
    ctx->setLastWidgetBounds(Rect(0, 0, 100, 100));
    input.onMouseDown(MouseButton::Left);
    input.onMouseMove(60, 60);
    
    BeginDragDropSource();
    SetDragDropPayload("test", nullptr, 0);
    EndDragDropSource();
    ctx->endFrame();
    
    // Step 2: Move to target and verify late update
    ctx->beginFrame(window);
    input.onMouseMove(150, 150); 
    
    // Source processed first
    BeginDragDropSource();
    EndDragDropSource();
    
    // Target processed second
    bool isTarget = BeginDragDropTarget(Rect(140, 140, 40, 40));
    if (isTarget) {
        AcceptDragDropPayload("test");
        EndDragDropTarget();
    }
    
    // Verify that highlight was drawn (happens during endFrame -> EndDragDropFrame)
    EXPECT_CALL(mockDl, addRect(_, _, _)).Times(AtLeast(1));
    
    ctx->endFrame();
}

//=============================================================================
// Integration Test - Verifying Test Mode Works
//=============================================================================

TEST_F(WidgetRenderingTest, TestModeDrawListInjection) {
    // Verify that Context::testDrawList() returns our mock
    EXPECT_EQ(Context::testDrawList(), &mockDl);
}
