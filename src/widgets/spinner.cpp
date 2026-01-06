/**
 * @file spinner.cpp
 * @brief Animated loading spinner widget implementation.
 */

#include "fastener/widgets/spinner.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <cmath>

namespace fst {

//=============================================================================
// Helper Constants
//=============================================================================

static constexpr float PI = 3.14159265358979323846f;
static constexpr float TWO_PI = PI * 2.0f;

//=============================================================================
// Spinner Implementation
//=============================================================================

void Spinner(const char* id, const SpinnerOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    
    float size = options.size;
    float thickness = options.thickness;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, size, size);
    Vec2 center = bounds.center();
    float radius = (size - thickness) * 0.5f;

    // Get animation time
    float time = wc.ctx->time() * options.speed * 2.0f;

    // Choose color
    Color spinColor = options.color.a > 0 ? options.color : theme.colors.primary;

    // Draw spinning arc
    // We draw a partial circle that rotates and changes arc length
    float startAngle = time;
    float arcLength = 0.8f + 0.4f * std::sin(time * 1.2f); // Varies between 0.4 and 1.2 radians
    arcLength = std::clamp(arcLength, 0.3f, PI * 1.5f);
    
    // Number of segments for smooth arc
    int segments = std::max(8, (int)(arcLength * radius * 0.5f));
    float angleStep = arcLength / (float)segments;

    for (int i = 0; i < segments; ++i) {
        float angle1 = startAngle + i * angleStep;
        float angle2 = startAngle + (i + 1) * angleStep;

        Vec2 p1(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius);
        Vec2 p2(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius);

        // Fade from full opacity at start to lower at end
        float alpha = 1.0f - (float)i / (float)segments * 0.7f;
        Color segColor = spinColor.withAlpha((uint8_t)(spinColor.a * alpha));

        dl.addLine(p1, p2, segColor, thickness);
    }

}

void Spinner(const std::string& id, const SpinnerOptions& options) {
    Spinner(id.c_str(), options);
}

void SpinnerWithLabel(const std::string& id, const std::string& label, 
                      const SpinnerOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;

    float size = options.size;
    float textWidth = font ? font->measureText(label).x : 0;
    float totalWidth = size + theme.metrics.paddingSmall + textWidth;
    float height = std::max(size, font ? font->lineHeight() : size);

    Rect bounds = allocateWidgetBounds(options.style, totalWidth, height);

    // Draw spinner
    SpinnerOptions spinOpts = options;
    spinOpts.style.width = size;
    spinOpts.style.height = size;
    
    Vec2 center(bounds.x() + size * 0.5f, bounds.center().y);
    float radius = (size - options.thickness) * 0.5f;
    float time = wc.ctx->time() * options.speed * 2.0f;
    Color spinColor = options.color.a > 0 ? options.color : theme.colors.primary;

    float startAngle = time;
    float arcLength = 0.8f + 0.4f * std::sin(time * 1.2f);
    arcLength = std::clamp(arcLength, 0.3f, PI * 1.5f);
    int segments = std::max(8, (int)(arcLength * radius * 0.5f));
    float angleStep = arcLength / (float)segments;

    for (int i = 0; i < segments; ++i) {
        float angle1 = startAngle + i * angleStep;
        float angle2 = startAngle + (i + 1) * angleStep;
        Vec2 p1(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius);
        Vec2 p2(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius);
        float alpha = 1.0f - (float)i / (float)segments * 0.7f;
        Color segColor = spinColor.withAlpha((uint8_t)(spinColor.a * alpha));
        dl.addLine(p1, p2, segColor, options.thickness);
    }

    // Draw label
    if (font && !label.empty()) {
        Vec2 textPos(bounds.x() + size + theme.metrics.paddingSmall, 
                     bounds.center().y - font->lineHeight() * 0.5f);
        dl.addText(font, textPos, label, theme.colors.text);
    }
}

void LoadingDots(const std::string& id, const SpinnerOptions& options) {
    auto wc = getWidgetContext();
    if (!wc.valid()) return;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;

    float dotSize = options.size * 0.25f;
    float spacing = dotSize * 1.5f;
    float totalWidth = dotSize * 3 + spacing * 2;
    float height = options.size;

    Rect bounds = allocateWidgetBounds(options.style, totalWidth, height);
    
    float time = wc.ctx->time() * options.speed * 3.0f;
    Color baseColor = options.color.a > 0 ? options.color : theme.colors.primary;

    for (int i = 0; i < 3; ++i) {
        float phase = time - i * 0.3f;
        float scale = 0.5f + 0.5f * std::sin(phase);
        float yOffset = -4.0f * std::sin(phase);
        
        float x = bounds.x() + i * (dotSize + spacing) + dotSize * 0.5f;
        float y = bounds.center().y + yOffset;
        float r = dotSize * 0.5f * (0.6f + 0.4f * scale);

        Color dotColor = baseColor.withAlpha((uint8_t)(baseColor.a * (0.4f + 0.6f * scale)));
        dl.addCircleFilled(Vec2(x, y), r, dotColor);
    }
}

} // namespace fst
