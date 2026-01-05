#include <gtest/gtest.h>
#include <fastener/core/types.h>
#include <fastener/ui/widget_utils.h>

using namespace fst;

//=============================================================================
// Slider Utils Tests
//=============================================================================

TEST(SliderUtilsTest, ValueToNormalized_Middle) {
    float result = slider_utils::valueToNormalized(50.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(result, 0.5f);
}

TEST(SliderUtilsTest, ValueToNormalized_Min) {
    float result = slider_utils::valueToNormalized(0.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST(SliderUtilsTest, ValueToNormalized_Max) {
    float result = slider_utils::valueToNormalized(100.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(result, 1.0f);
}

TEST(SliderUtilsTest, ValueToNormalized_Clamped) {
    float below = slider_utils::valueToNormalized(-10.0f, 0.0f, 100.0f);
    float above = slider_utils::valueToNormalized(150.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(below, 0.0f);
    EXPECT_FLOAT_EQ(above, 1.0f);
}

TEST(SliderUtilsTest, ValueToNormalized_NegativeRange) {
    float result = slider_utils::valueToNormalized(-5.0f, -10.0f, 10.0f);
    EXPECT_FLOAT_EQ(result, 0.25f);  // -5 is 25% from -10 to 10
}

TEST(SliderUtilsTest, NormalizedToValue_Middle) {
    float result = slider_utils::normalizedToValue(0.5f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(result, 50.0f);
}

TEST(SliderUtilsTest, NormalizedToValue_Clamped) {
    float below = slider_utils::normalizedToValue(-0.5f, 0.0f, 100.0f);
    float above = slider_utils::normalizedToValue(1.5f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(below, 0.0f);
    EXPECT_FLOAT_EQ(above, 100.0f);
}

TEST(SliderUtilsTest, ThumbPosition) {
    // Track from 100 to 300 (width=200), value 50% -> position 200
    float pos = slider_utils::thumbPositionFromValue(50.0f, 0.0f, 100.0f, 100.0f, 200.0f);
    EXPECT_FLOAT_EQ(pos, 200.0f);
}

TEST(SliderUtilsTest, ValueFromMouse_Center) {
    // Track from 100 to 300, mouse at 200 -> 50%
    float value = slider_utils::valueFromMousePosition(200.0f, 100.0f, 200.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(value, 50.0f);
}

TEST(SliderUtilsTest, ValueFromMouse_OutOfBounds) {
    // Mouse before track start
    float before = slider_utils::valueFromMousePosition(50.0f, 100.0f, 200.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(before, 0.0f);
    
    // Mouse after track end
    float after = slider_utils::valueFromMousePosition(400.0f, 100.0f, 200.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(after, 100.0f);
}

//=============================================================================
// ProgressBar Utils Tests
//=============================================================================

TEST(ProgressBarUtilsTest, FillWidth_Zero) {
    float width = progress_utils::fillWidth(0.0f, 200.0f);
    EXPECT_FLOAT_EQ(width, 0.0f);
}

TEST(ProgressBarUtilsTest, FillWidth_Full) {
    float width = progress_utils::fillWidth(1.0f, 200.0f);
    EXPECT_FLOAT_EQ(width, 200.0f);
}

TEST(ProgressBarUtilsTest, FillWidth_Half) {
    float width = progress_utils::fillWidth(0.5f, 200.0f);
    EXPECT_FLOAT_EQ(width, 100.0f);
}

TEST(ProgressBarUtilsTest, FillWidth_Clamped) {
    float above = progress_utils::fillWidth(1.5f, 200.0f);
    float below = progress_utils::fillWidth(-0.5f, 200.0f);
    EXPECT_FLOAT_EQ(above, 200.0f);
    EXPECT_FLOAT_EQ(below, 0.0f);
}

TEST(ProgressBarUtilsTest, IndeterminatePosition_Start) {
    // At time=0, bar should be at trackLeft - barWidth (entering from left)
    float pos = progress_utils::indeterminateBarPosition(0.0f, 1.0f, 100.0f, 200.0f, 60.0f);
    EXPECT_FLOAT_EQ(pos, 40.0f);  // 100 - 60
}

TEST(ProgressBarUtilsTest, IndeterminatePosition_Middle) {
    // At cycle=0.5, bar should be at middle of animation
    float totalRange = 200.0f + 60.0f;  // trackWidth + barWidth
    float expectedPos = 100.0f - 60.0f + 0.5f * totalRange;  // 40 + 130 = 170
    
    // time * speed = 0.5
    float pos = progress_utils::indeterminateBarPosition(0.5f, 1.0f, 100.0f, 200.0f, 60.0f);
    EXPECT_FLOAT_EQ(pos, expectedPos);
}

//=============================================================================
// Checkbox Utils Tests
//=============================================================================

TEST(CheckboxUtilsTest, CheckmarkPoints) {
    Vec2 center(50.0f, 50.0f);
    float boxSize = 20.0f;
    
    auto pts = checkbox_utils::calculateCheckmark(center, boxSize);
    
    // Verify p1 is to the left of center
    EXPECT_LT(pts.p1.x, center.x);
    EXPECT_FLOAT_EQ(pts.p1.y, center.y);
    
    // Verify p2 is the lowest point (bottom of check)
    EXPECT_GT(pts.p2.y, pts.p1.y);
    EXPECT_GT(pts.p2.y, pts.p3.y);
    
    // Verify p3 is to the right of center
    EXPECT_GT(pts.p3.x, center.x);
}

//=============================================================================
// Layout Utils Tests  
//=============================================================================

TEST(LayoutUtilsTest, CenterInBounds) {
    Rect bounds(100.0f, 100.0f, 200.0f, 100.0f);
    Vec2 itemSize(50.0f, 20.0f);
    
    Vec2 centered = layout_utils::centerInBounds(itemSize, bounds);
    
    EXPECT_FLOAT_EQ(centered.x, 175.0f);  // 100 + (200 - 50) / 2
    EXPECT_FLOAT_EQ(centered.y, 140.0f);  // 100 + (100 - 20) / 2
}

TEST(LayoutUtilsTest, VerticalCenterY) {
    float y = layout_utils::verticalCenterY(10.0f, 100.0f, 20.0f);
    EXPECT_FLOAT_EQ(y, 50.0f);  // 10 + (100 - 20) / 2 = 10 + 40 = 50
}

TEST(LayoutUtilsTest, TotalWidthWithLabel) {
    // Content only
    float contentOnly = layout_utils::totalWidthWithLabel(200.0f, 0.0f, 0.0f, 10.0f);
    EXPECT_FLOAT_EQ(contentOnly, 200.0f);
    
    // With label
    float withLabel = layout_utils::totalWidthWithLabel(200.0f, 50.0f, 0.0f, 10.0f);
    EXPECT_FLOAT_EQ(withLabel, 260.0f);  // 200 + 50 + 10
    
    // With label and value
    float withBoth = layout_utils::totalWidthWithLabel(200.0f, 50.0f, 40.0f, 10.0f);
    EXPECT_FLOAT_EQ(withBoth, 310.0f);  // 200 + 50 + 10 + 40 + 10
}
