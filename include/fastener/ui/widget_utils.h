#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <algorithm>
#include <cmath>

namespace fst {

// Forward declarations
class Context;
class IDrawList;
class Font;
struct Theme;
class LayoutContext;

//=============================================================================
// Widget Context Helper
//=============================================================================

/**
 * @brief Aggregates common widget dependencies for cleaner initialization.
 * 
 * Use getWidgetContext() to obtain this struct at the start of each widget.
 * Returns an invalid context (ctx == nullptr) if Context::current() is not set.
 */
struct WidgetContext {
    Context* ctx;
    const Theme* theme;
    IDrawList* dl;
    Font* font;
    
    /** @brief Check if context is valid. */
    bool valid() const { return ctx != nullptr; }
};

/**
 * @brief Get the current widget context with all common dependencies.
 * @return WidgetContext struct; check valid() before using.
 */
WidgetContext getWidgetContext();

/**
 * @brief Allocate bounds for a widget using layout or explicit style.
 * 
 * If style.x and style.y are both < 0, allocates from the layout system.
 * Otherwise uses the explicit position from style.
 * 
 * @param style Widget style with position/size options
 * @param width Calculated/default width
 * @param height Calculated/default height
 * @return Allocated bounds rectangle
 */
Rect allocateWidgetBounds(const Style& style, float width, float height);

//=============================================================================
// State Color Helpers
//=============================================================================

/**
 * @brief Determine background color based on widget state.
 * 
 * @param baseColor Base/normal color
 * @param hoverColor Color when hovered
 * @param activeColor Color when active/pressed
 * @param state Current widget state
 * @param disabledAlpha Alpha multiplier for disabled state (0.0-1.0)
 * @return Appropriate color for current state
 */
Color getStateColor(Color baseColor, Color hoverColor, Color activeColor,
                    const struct WidgetState& state, float disabledAlpha = 0.5f);

//=============================================================================
// Slider Utilities
//=============================================================================
namespace slider_utils {

/// Calculate normalized position (0-1) from value
inline float valueToNormalized(float value, float minVal, float maxVal) {
    if (maxVal <= minVal) return 0.0f;
    return std::clamp((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);
}

/// Calculate value from normalized position (0-1)
inline float normalizedToValue(float t, float minVal, float maxVal) {
    t = std::clamp(t, 0.0f, 1.0f);
    return minVal + t * (maxVal - minVal);
}

/// Calculate thumb X position from value
inline float thumbPositionFromValue(float value, float minVal, float maxVal, 
                                     float trackLeft, float trackWidth) {
    float t = valueToNormalized(value, minVal, maxVal);
    return trackLeft + trackWidth * t;
}

/// Calculate value from mouse X position
inline float valueFromMousePosition(float mouseX, float trackLeft, float trackWidth,
                                     float minVal, float maxVal) {
    float t = (mouseX - trackLeft) / trackWidth;
    return normalizedToValue(t, minVal, maxVal);
}

} // namespace slider_utils

//=============================================================================
// ProgressBar Utilities
//=============================================================================
namespace progress_utils {

/// Calculate fill width from progress (0-1)
inline float fillWidth(float progress, float trackWidth) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    return trackWidth * progress;
}

/// Calculate indeterminate bar position (returns left edge)
inline float indeterminateBarPosition(float time, float speed, float trackLeft, 
                                       float trackWidth, float barWidth) {
    float totalRange = trackWidth + barWidth;
    float cycle = std::fmod(time * speed, 1.0f);
    return trackLeft - barWidth + cycle * totalRange;
}

} // namespace progress_utils

//=============================================================================
// Checkbox Utilities
//=============================================================================
namespace checkbox_utils {

/// Checkmark line endpoints
struct CheckmarkPoints {
    Vec2 p1, p2, p3;
};

/// Calculate checkmark points based on center and box size
inline CheckmarkPoints calculateCheckmark(Vec2 center, float boxSize) {
    float s = boxSize * 0.3f;
    CheckmarkPoints pts;
    pts.p1 = Vec2(center.x - s * 0.8f, center.y);
    pts.p2 = Vec2(center.x - s * 0.2f, center.y + s * 0.6f);
    pts.p3 = Vec2(center.x + s * 0.9f, center.y - s * 0.5f);
    return pts;
}

} // namespace checkbox_utils

//=============================================================================
// Layout Utilities
//=============================================================================
namespace layout_utils {

/// Calculate centered position within bounds
inline Vec2 centerInBounds(Vec2 itemSize, const Rect& bounds) {
    return Vec2(
        bounds.x() + (bounds.width() - itemSize.x) * 0.5f,
        bounds.y() + (bounds.height() - itemSize.y) * 0.5f
    );
}

/// Calculate vertical center position for text
inline float verticalCenterY(float boundsY, float boundsHeight, float textHeight) {
    return boundsY + (boundsHeight - textHeight) * 0.5f;
}

/// Calculate total widget width with optional label and value display
inline float totalWidthWithLabel(float contentWidth, float labelWidth, 
                                  float valueWidth, float padding) {
    float total = contentWidth;
    if (labelWidth > 0) total += labelWidth + padding;
    if (valueWidth > 0) total += valueWidth + padding;
    return total;
}

} // namespace layout_utils

} // namespace fst
