#include <gtest/gtest.h>
#include <fastener/core/types.h>
#include <algorithm>
#include <cmath>

using namespace fst;

//=============================================================================
// Slider Value Calculations - extracted logic
//=============================================================================

namespace slider_utils {

// Calculate normalized position (0-1) from value
inline float valueToNormalized(float value, float minVal, float maxVal) {
    if (maxVal <= minVal) return 0.0f;
    return std::clamp((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);
}

// Calculate value from normalized position (0-1)
inline float normalizedToValue(float t, float minVal, float maxVal) {
    t = std::clamp(t, 0.0f, 1.0f);
    return minVal + t * (maxVal - minVal);
}

// Calculate thumb X position from value
inline float thumbPositionFromValue(float value, float minVal, float maxVal, 
                                     float trackLeft, float trackWidth) {
    float t = valueToNormalized(value, minVal, maxVal);
    return trackLeft + trackWidth * t;
}

// Calculate value from mouse X position
inline float valueFromMousePosition(float mouseX, float trackLeft, float trackWidth,
                                     float minVal, float maxVal) {
    float t = (mouseX - trackLeft) / trackWidth;
    return normalizedToValue(t, minVal, maxVal);
}

} // namespace slider_utils

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
// ProgressBar Calculations - extracted logic
//=============================================================================

namespace progress_utils {

// Calculate fill width from progress
inline float fillWidth(float progress, float trackWidth) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    return trackWidth * progress;
}

// Calculate indeterminate bar position (returns left edge)
inline float indeterminateBarPosition(float time, float speed, float trackLeft, 
                                       float trackWidth, float barWidth) {
    float totalRange = trackWidth + barWidth;
    float cycle = std::fmod(time * speed, 1.0f);
    return trackLeft - barWidth + cycle * totalRange;
}

} // namespace progress_utils

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
// Checkbox Calculations - extracted logic
//=============================================================================

namespace checkbox_utils {

// Calculate checkmark points
struct CheckmarkPoints {
    Vec2 p1, p2, p3;
};

inline CheckmarkPoints calculateCheckmark(Vec2 center, float boxSize) {
    float s = boxSize * 0.3f;
    CheckmarkPoints pts;
    pts.p1 = Vec2(center.x - s * 0.8f, center.y);
    pts.p2 = Vec2(center.x - s * 0.2f, center.y + s * 0.6f);
    pts.p3 = Vec2(center.x + s * 0.9f, center.y - s * 0.5f);
    return pts;
}

// Calculate label position
inline Vec2 labelPosition(float boxRight, float spacing, float boundsY, 
                          float boundsHeight, float textHeight) {
    return Vec2(
        boxRight + spacing,
        boundsY + (boundsHeight - textHeight) * 0.5f
    );
}

} // namespace checkbox_utils

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

TEST(CheckboxUtilsTest, LabelPosition_Centered) {
    Vec2 pos = checkbox_utils::labelPosition(30.0f, 5.0f, 10.0f, 20.0f, 14.0f);
    
    EXPECT_FLOAT_EQ(pos.x, 35.0f);  // boxRight + spacing
    EXPECT_FLOAT_EQ(pos.y, 13.0f);  // 10 + (20 - 14) * 0.5 = 10 + 3 = 13
}

//=============================================================================
// Layout Calculations - common widget sizing
//=============================================================================

namespace layout_utils {

// Calculate centered position within bounds
inline Vec2 centerInBounds(Vec2 itemSize, Rect bounds) {
    return Vec2(
        bounds.x() + (bounds.width() - itemSize.x) * 0.5f,
        bounds.y() + (bounds.height() - itemSize.y) * 0.5f
    );
}

// Calculate total widget width with label and optional value display
inline float totalWidthWithLabel(float contentWidth, float labelWidth, 
                                  float valueWidth, float padding) {
    float total = contentWidth;
    if (labelWidth > 0) total += labelWidth + padding;
    if (valueWidth > 0) total += valueWidth + padding;
    return total;
}

} // namespace layout_utils

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
