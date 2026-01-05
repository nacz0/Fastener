#include "fastener/widgets/progress_bar.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget_utils.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace fst {

void ProgressBar(float progress, const ProgressBarOptions& options) {
    ProgressBar("", progress, options);
}

void ProgressBar(const std::string& label, float progress, const ProgressBarOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return;

    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();

    float width = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : 20.0f;
    float labelWidth = 0;

    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }

    Rect bounds(options.style.x, options.style.y, labelWidth + width, height);
    // TODO: Get from layout system

    // Draw label
    if (font && !label.empty()) {
        float labelY = layout_utils::verticalCenterY(bounds.y(), height, font->lineHeight());
        Vec2 labelPos(bounds.x(), labelY);
        dl.addText(font, labelPos, label, theme.colors.text);
    }

    // Track bounds
    Rect trackBounds(bounds.x() + labelWidth, bounds.y(), width, height);
    float radius = height * 0.5f;

    // Draw track background
    dl.addRectFilled(trackBounds, theme.colors.secondary, radius);

    progress = std::clamp(progress, 0.0f, 1.0f);
    Color fillColor = options.fillColor.a > 0 ? options.fillColor : theme.colors.primary;

    if (options.indeterminate) {
        float time = ctx->time();
        float barWidth = trackBounds.width() * 0.3f;
        float barLeft = progress_utils::indeterminateBarPosition(
            time, 0.7f, trackBounds.x(), trackBounds.width(), barWidth);
        float barRight = barLeft + barWidth;
        float innerLeft = trackBounds.x() + radius;
        float innerRight = trackBounds.right() - radius;

        // Constraint to the track area
        dl.pushClipRect(trackBounds);

        // 1. Draw the "Track Segment": we draw the track shape in fill color,
        // but clip it to the moving bar window. This handles the curves at 
        // the very ends of the track perfectly.
        dl.pushClipRect(Rect(barLeft, trackBounds.y(), barWidth, trackBounds.height()));
        dl.addRectFilled(trackBounds, fillColor, radius);
        dl.popClipRect();

        // 2. Draw rounded ends to form a moving capsule when in the middle area.
        // We only draw them when they are in the straight part of the track;
        // near the ends, they naturally "merge" into the track's own curve.
        if (barLeft > innerLeft && barLeft < innerRight) {
            dl.addCircleFilled(Vec2(barLeft, trackBounds.y() + radius), radius, fillColor);
        }
        if (barRight > innerLeft && barRight < innerRight) {
            dl.addCircleFilled(Vec2(barRight, trackBounds.y() + radius), radius, fillColor);
        }

        dl.popClipRect();
    } else if (progress > 0.0f) {
        // Normal progress fill
        float fillW = progress_utils::fillWidth(progress, trackBounds.width());
        Rect fillRect(trackBounds.x(), trackBounds.y(), fillW, trackBounds.height());
        dl.addRectFilled(fillRect, fillColor, radius);
    }

    // Draw percentage text
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
