/**
 * @file test_context_di.cpp
 * @brief Unit tests for Context dependency injection infrastructure.
 */

#include <gtest/gtest.h>
#include <fastener/core/context.h>
#include <fastener/ui/widget_scope.h>
#include <fastener/ui/widget_utils.h>
#include "TestContext.h"

using namespace fst;
using namespace fst::testing;

//=============================================================================
// Context Stack Tests
//=============================================================================

// Suppress deprecation warnings for testing Context::current()
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

TEST(ContextStackTest, PushPop_Basic) {
    Context ctx1(false);
    Context ctx2(false);
    
    // Initially no context
    EXPECT_EQ(Context::current(), nullptr);
    
    // Push first context
    Context::pushContext(&ctx1);
    EXPECT_EQ(Context::current(), &ctx1);
    
    // Push second context - should be on top
    Context::pushContext(&ctx2);
    EXPECT_EQ(Context::current(), &ctx2);
    
    // Pop second context - first should be back
    Context::popContext();
    EXPECT_EQ(Context::current(), &ctx1);
    
    // Pop first context - none left
    Context::popContext();
    EXPECT_EQ(Context::current(), nullptr);
}

TEST(ContextStackTest, PopEmpty_Safe) {
    // Popping empty stack should not crash
    EXPECT_EQ(Context::current(), nullptr);
    Context::popContext();  // Should be safe
    EXPECT_EQ(Context::current(), nullptr);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//=============================================================================
// WidgetScope RAII Tests
//=============================================================================

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

TEST(WidgetScopeTest, RAII_PushOnConstruction) {
    Context ctx(false);
    
    EXPECT_EQ(Context::current(), nullptr);
    
    {
        WidgetScope scope(ctx);
        EXPECT_EQ(Context::current(), &ctx);
    }
    
    // After scope destruction, context should be popped
    EXPECT_EQ(Context::current(), nullptr);
}

TEST(WidgetScopeTest, NestedScopes) {
    Context ctx1(false);
    Context ctx2(false);
    
    {
        WidgetScope outer(ctx1);
        EXPECT_EQ(Context::current(), &ctx1);
        
        {
            WidgetScope inner(ctx2);
            EXPECT_EQ(Context::current(), &ctx2);
        }
        
        // Inner scope destroyed, back to outer
        EXPECT_EQ(Context::current(), &ctx1);
    }
    
    EXPECT_EQ(Context::current(), nullptr);
}

TEST(WidgetScopeTest, AccessViaContext) {
    Context ctx(false);
    WidgetScope scope(ctx);
    
    EXPECT_EQ(&scope.context(), &ctx);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

TEST(WidgetContextTest, GetWidgetContext_UsesStack) {
    Context ctx(false);
    WidgetScope scope(ctx);
    
    auto wc = getWidgetContext();
    
    EXPECT_TRUE(wc.valid());
    EXPECT_EQ(wc.ctx, &ctx);
}

TEST(WidgetContextTest, GetWidgetContext_EmptyStack) {
    // Without any context on stack
    auto wc = getWidgetContext();
    
    EXPECT_FALSE(wc.valid());
    EXPECT_EQ(wc.ctx, nullptr);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

TEST(TestContextTest, ContextStackIntegration) {
    // Ensure no context before test
    EXPECT_EQ(Context::current(), nullptr);
    
    {
        TestContext tc;
        tc.beginFrame();
        
        // Context should be on stack
        EXPECT_EQ(Context::current(), &tc.context());
        
        tc.endFrame();
    }
    
    // After TestContext destroyed, stack should be empty
    EXPECT_EQ(Context::current(), nullptr);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
