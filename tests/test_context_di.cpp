/**
 * @file test_context_di.cpp
 * @brief Unit tests for Context dependency injection infrastructure.
 */

#include <gtest/gtest.h>
#include <fastener/core/context.h>
#include <fastener/ui/layout.h>
#include <fastener/ui/widget_scope.h>
#include <fastener/ui/widget_utils.h>
#include <fastener/widgets/menu.h>
#include "TestContext.h"

using namespace fst;
using namespace fst::testing;

//=============================================================================
// WidgetScope RAII Tests
//=============================================================================

//=============================================================================
// WidgetContext Factory Tests
//=============================================================================

TEST(WidgetContextTest, Make_FromExplicitContext) {
    Context ctx(false);
    
    auto wc = WidgetContext::make(ctx);
    
    EXPECT_TRUE(wc.valid());
    EXPECT_EQ(wc.ctx, &ctx);
    EXPECT_EQ(wc.theme, &ctx.theme());
}

TEST(WidgetContextTest, GetWidgetContext_Overload) {
    Context ctx(false);
    
    auto wc = getWidgetContext(ctx);
    
    EXPECT_TRUE(wc.valid());
    EXPECT_EQ(wc.ctx, &ctx);
}

//=============================================================================
// TestContext Helper Tests
//=============================================================================

TEST(TestContextTest, BasicUsage) {
    TestContext tc;
    
    // Context should exist but no frame active yet
    EXPECT_FALSE(tc.isFrameActive());
    
    // Begin frame to enable widget testing
    tc.beginFrame();
    EXPECT_TRUE(tc.isFrameActive());
    
    // End frame
    tc.endFrame();
    EXPECT_FALSE(tc.isFrameActive());
}

TEST(TestContextTest, MockDrawListAvailable) {
    TestContext tc;
    
    MockDrawList& mockDl = tc.mockDrawList();
    
    // Should be able to set expectations
    EXPECT_CALL(mockDl, addRectFilled(::testing::_, ::testing::_, ::testing::_))
        .Times(1);
    
    mockDl.addRectFilled(Rect(0, 0, 100, 100), Color::red(), 5.0f);
}

TEST(ContextFrameGuardTest, DoubleBeginDoesNotRequireExtraEnd) {
    Context ctx(false);
    StubWindow window;

    ctx.beginFrame(window);
    ASSERT_TRUE(ctx.isFrameActive());

    ctx.beginFrame(window);
    EXPECT_TRUE(ctx.isFrameActive());

    ctx.endFrame();
    EXPECT_FALSE(ctx.isFrameActive());
}

TEST(ContextFrameGuardTest, EndWithoutBeginIsANoOp) {
    Context ctx(false);

    ctx.endFrame();

    EXPECT_FALSE(ctx.isFrameActive());
}

TEST(ContextFrameGuardTest, OutOfOrderEndCannotCloseAnotherContextsFrame) {
    Context first(false);
    Context second(false);
    StubWindow firstWindow;
    StubWindow secondWindow;

    first.beginFrame(firstWindow);
    second.beginFrame(secondWindow);

    first.endFrame();
    EXPECT_TRUE(first.isFrameActive());
    EXPECT_TRUE(second.isFrameActive());

    second.endFrame();
    first.endFrame();
    EXPECT_FALSE(first.isFrameActive());
    EXPECT_FALSE(second.isFrameActive());
}

TEST(ContextFrameGuardTest, WidgetScopeMustEndBeforeItsEnclosingFrame) {
    Context frameContext(false);
    Context scopedContext(false);
    StubWindow window;

    frameContext.beginFrame(window);
    {
        WidgetScope scope(scopedContext);
        frameContext.endFrame();
        EXPECT_TRUE(frameContext.isFrameActive());
    }

    frameContext.endFrame();
    EXPECT_FALSE(frameContext.isFrameActive());
}

TEST(ContextFrameGuardTest, EndFrameRecoversUnbalancedIdStack) {
    Context ctx(false);
    StubWindow window;

    ctx.beginFrame(window);
    ctx.pushId("UnclosedScope");
    ASSERT_NE(ctx.currentId(), WidgetId{0});

    ctx.endFrame();

    EXPECT_EQ(ctx.currentId(), WidgetId{0});
}

TEST(ContextFrameGuardTest, EndFrameRecoversUnbalancedLayoutStack) {
    Context ctx(false);
    StubWindow window;

    ctx.beginFrame(window);
    ctx.layout().beginContainer(
        Rect(100.0f, 100.0f, 200.0f, 200.0f),
        LayoutDirection::Horizontal);

    ctx.endFrame();

    EXPECT_EQ(ctx.layout().allocate(10.0f, 10.0f),
              Rect(0.0f, 0.0f, 10.0f, 10.0f));
}

TEST(ContextShutdownTest, WindowResourceReleaseMakesTargetContextCurrent) {
    Context ctx(false);
    StubWindow window;

    EXPECT_TRUE(ctx.releaseWindowResources(window));

    EXPECT_EQ(window.makeContextCurrentCalls(), 1);
}

TEST(ContextShutdownTest, ShutdownIsRejectedDuringActiveFrame) {
    Context ctx(false);
    StubWindow window;
    ctx.beginFrame(window);

    EXPECT_FALSE(ctx.shutdown(window));
    EXPECT_TRUE(ctx.isFrameActive());
    EXPECT_EQ(window.makeContextCurrentCalls(), 0);

    ctx.endFrame();
}

TEST(ContextShutdownTest, ShutdownIsIdempotentOutsideFrame) {
    Context ctx(false);
    StubWindow window;

    EXPECT_TRUE(ctx.shutdown(window));
    EXPECT_TRUE(ctx.shutdown(window));

    EXPECT_FALSE(ctx.isFrameActive());
    EXPECT_EQ(window.makeContextCurrentCalls(), 2);
}

TEST(ContextDeferredRenderTest, CommandsQueuedDuringFlushRunNextFrame) {
    Context ctx(false);
    StubWindow window;
    int executionOrder = 0;

    ctx.beginFrame(window);
    ctx.deferRender([&] {
        executionOrder = executionOrder * 10 + 1;
        ctx.deferRender([&] {
            executionOrder = executionOrder * 10 + 2;
        });
    });
    ctx.endFrame();

    EXPECT_EQ(executionOrder, 1);

    ctx.beginFrame(window);
    ctx.endFrame();

    EXPECT_EQ(executionOrder, 12);
}

TEST(ContextDeferredRenderTest, RecursiveEndFrameCannotCloseTheOuterFrame) {
    Context ctx(false);
    StubWindow window;
    int callbackCount = 0;

    ctx.beginFrame(window);
    ctx.deferRender([&] {
        ctx.endFrame();
        ++callbackCount;
    });
    ctx.endFrame();

    EXPECT_EQ(callbackCount, 1);
    EXPECT_FALSE(ctx.isFrameActive());

    ctx.beginFrame(window);
    EXPECT_TRUE(ctx.isFrameActive());
    ctx.endFrame();
}

TEST(ContextMenuContextTest, ClosingOneContextMenuDoesNotCloseAnotherContextsMenu) {
    Context first(false);
    Context second(false);

    ShowContextMenu(first, {MenuItem("first", "First action")}, Vec2(10.0f, 10.0f));
    ShowContextMenu(second, {MenuItem("second", "Second action")}, Vec2(20.0f, 20.0f));

    ASSERT_TRUE(IsContextMenuOpen(first));
    ASSERT_TRUE(IsContextMenuOpen(second));

    CloseContextMenu(second);

    EXPECT_TRUE(IsContextMenuOpen(first));
    EXPECT_FALSE(IsContextMenuOpen(second));

    CloseContextMenu(first);
}


//=============================================================================
// TestContext Helper Tests (Stack Integration removed)
//=============================================================================
