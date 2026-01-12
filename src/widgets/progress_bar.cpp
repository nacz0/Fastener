/**
 * @file progress_bar.cpp
 * @brief ProgressBar widget implementation.
 */

#include "fastener/widgets/progress_bar.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace fst {

//=============================================================================
// ProgressBar Implementation (Explicit DI)
//=============================================================================

void ProgressBar(Context& ctx, float progress, const ProgressBarOptions& options) {
    ProgressBar(ctx, std::string_view{}, progress, options);
}

void ProgressBar(Context& ctx, std::string_view label, float progress, const ProgressBarOptions& options) {
    auto wc = WidgetContext::make(ctx);

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : 20.0f;
    float labelWidth = 0;

    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }

    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, labelWidth + width, height);

    // Draw label
    if (font && !label.empty()) {
        float labelY = layout_utils::verticalCenterY(bounds.y(), height, font->lineHeight());
        Vec2 labelPos(bounds.x(), labelY);
        dl.addText(font, labelPos, label, theme.colors.text);
    }

    // Calculate track bounds
    Rect trackBounds(bounds.x() + labelWidth, bounds.y(), width, height);
    float radius = height * 0.5f;

    // Draw track background
    dl.addRectFilled(trackBounds, theme.colors.secondary, radius);

    // Clamp progress value
    progress = std::clamp(progress, 0.0f, 1.0f);
    Color fillColor = options.fillColor.a > 0 ? options.fillColor : theme.colors.primary;

    if (options.indeterminate) {
        // Animated indeterminate mode
        float time = ctx.time();
        float barWidth = trackBounds.width() * 0.3f;
        float barLeft = progress_utils::indeterminateBarPosition(
            time, 0.7f, trackBounds.x(), trackBounds.width(), barWidth);
        float barRight = barLeft + barWidth;
        float innerLeft = trackBounds.x() + radius;
        float innerRight = trackBounds.right() - radius;

        // Clip to track area
        dl.pushClipRect(trackBounds);

        // Draw track segment clipped to moving bar window
        dl.pushClipRect(Rect(barLeft, trackBounds.y(), barWidth, trackBounds.height()));
        dl.addRectFilled(trackBounds, fillColor, radius);
        dl.popClipRect();

        // Draw rounded ends for capsule effect in middle area
        if (barLeft > innerLeft && barLeft < innerRight) {
            dl.addCircleFilled(Vec2(barLeft, trackBounds.y() + radius), radius, fillColor);
        }
        if (barRight > innerLeft && barRight < innerRight) {
            dl.addCircleFilled(Vec2(barRight, trackBounds.y() + radius), radius, fillColor);
        }

        dl.popClipRect();
    } else if (progress > 0.0f) {
        // Normal determinate progress fill
        float fillW = progress_utils::fillWidth(progress, trackBounds.width());
        Rect fillRect(trackBounds.x(), trackBounds.y(), fillW, trackBounds.height());
        dl.addRectFilled(fillRect, fillColor, radius);
    }

    // Draw percentage text centered on track
    if (options.showPercentage && font && !options.indeterminate) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << (progress * 100.0f) << "%";
        std::string text = oss.str();
        Vec2 textSize = font->measureText(text);
        Vec2 textPos(
            trackBounds.center().x - textSize.x * 0.5f,
            trackBounds.center().y - textSize.y * 0.5f
        );
        dl.addText(font, textPos, text, theme.colors.text);
    }
}

} // namespace fst

