#include <gtest/gtest.h>
#include <fastener/ui/layout.h>
#include <fastener/core/context.h>
#include <fastener/ui/theme.h>

using namespace fst;

class LayoutTest : public ::testing::Test {
protected:
    void SetUp() override {
        // We need a context for some global layout helpers
        // but for raw LayoutContext tests we can just use the class
    }
};

TEST_F(LayoutTest, HorizontalAllocation) {
    LayoutContext lc;
    Rect root(0, 0, 1000, 100);
    lc.beginContainer(root, LayoutDirection::Horizontal);
    lc.setSpacing(10);
    
    Rect r1 = lc.allocate(100, 50);
    EXPECT_FLOAT_EQ(r1.x(), 0.0f);
    EXPECT_FLOAT_EQ(r1.width(), 100.0f);
    
    Rect r2 = lc.allocate(100, 50);
    EXPECT_FLOAT_EQ(r2.x(), 110.0f); // 100 + 10 spacing
    EXPECT_FLOAT_EQ(r2.width(), 100.0f);
    
    lc.endContainer();
}

TEST_F(LayoutTest, VerticalAllocation) {
    LayoutContext lc;
    Rect root(0, 0, 200, 1000);
    lc.beginContainer(root, LayoutDirection::Vertical);
    lc.setSpacing(10);
    
    Rect r1 = lc.allocate(100, 50);
    EXPECT_FLOAT_EQ(r1.y(), 0.0f);
    EXPECT_FLOAT_EQ(r1.height(), 50.0f);
    
    Rect r2 = lc.allocate(100, 50);
    EXPECT_FLOAT_EQ(r2.y(), 60.0f); // 50 + 10 spacing
    EXPECT_FLOAT_EQ(r2.height(), 50.0f);
    
    lc.endContainer();
}

TEST_F(LayoutTest, Padding) {
    LayoutContext lc;
    Rect root(0, 0, 1000, 1000);
    lc.beginContainer(root, LayoutDirection::Vertical);
    lc.setPadding(10, 20, 30, 40); // top, right, bottom, left
    
    Rect r1 = lc.allocate(100, 50);
    EXPECT_FLOAT_EQ(r1.x(), 40.0f); // left padding
    EXPECT_FLOAT_EQ(r1.y(), 10.0f); // top padding
    
    lc.endContainer();
}

TEST_F(LayoutTest, NestedContainers) {
    LayoutContext lc;
    Rect root(0, 0, 1000, 1000);
    
    lc.beginContainer(root, LayoutDirection::Vertical);
    lc.setPadding(10, 10, 10, 10);
    
    Rect rowBounds = lc.allocate(800, 50);
    lc.beginContainer(rowBounds, LayoutDirection::Horizontal);
    lc.setSpacing(5);
    
    Rect item1 = lc.allocate(100, 40);
    EXPECT_FLOAT_EQ(item1.x(), 10.0f);
    EXPECT_FLOAT_EQ(item1.y(), 10.0f);
    
    Rect item2 = lc.allocate(100, 40);
    EXPECT_FLOAT_EQ(item2.x(), 115.0f); // 10 + 100 + 5
    
    lc.endContainer();
    lc.endContainer();
}

TEST_F(LayoutTest, AllocateRemaining) {
    LayoutContext lc;
    Rect root(10, 20, 100, 200);
    lc.beginContainer(root, LayoutDirection::Vertical);
    lc.setSpacing(10);
    lc.allocate(100, 50);
    
    Rect remaining = lc.allocateRemaining();
    EXPECT_FLOAT_EQ(remaining.y(), 20.0f + 50.0f + 10.0f);
    EXPECT_FLOAT_EQ(remaining.height(), 200.0f - 50.0f - 10.0f);
    
    lc.endContainer();
}
