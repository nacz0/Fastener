/**
 * @file test_context_di.cpp
 * @brief Unit tests for Context dependency injection infrastructure.
 */

#include <gtest/gtest.h>
#include <fastener/core/context.h>
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
