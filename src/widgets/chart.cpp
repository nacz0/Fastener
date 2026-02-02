/**
 * @file chart.cpp
 * @brief Chart widget implementation (line, bar, pie).
 */

#include "fastener/widgets/chart.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/widgets/tooltip.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace fst {

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct ChartRange {
    float min = 0.0f;
    float max = 1.0f;
};

float normalizeAngle(float angle) {
    float twoPi = 2.0f * kPi;
    while (angle < 0.0f) angle += twoPi;
    while (angle >= twoPi) angle -= twoPi;
    return angle;
}

bool angleInRange(float angle, float start, float end, bool clockwise) {
    angle = normalizeAngle(angle);
    start = normalizeAngle(start);
    end = normalizeAngle(end);

    if (!clockwise) {
        if (start <= end) {
            return angle >= start && angle < end;
        }
        return angle >= start || angle < end;
    }

    if (start >= end) {
        return angle <= start && angle > end;
    }
    return angle <= start || angle > end;
}

Color resolveColor(Color color, Color fallback) {
    return color.a > 0 ? color : fallback;
}

std::string formatValue(float value, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(std::max(0, decimals)) << value;
    return oss.str();
}

std::string labelForIndex(const ChartOptions& options, size_t index) {
    if (index < options.labels.size()) {
        return options.labels[index];
    }
    return {};
}

std::string tooltipTextForValue(const ChartOptions& options, size_t index, float value,
                                float fraction, bool includePercent) {
    std::string label = labelForIndex(options, index);
    std::string valueText = formatValue(value, options.valueDecimals);
    if (includePercent) {
        std::string percentText = formatValue(fraction * 100.0f, options.valueDecimals);
        valueText += " (" + percentText + "%)";
    }
    if (!label.empty()) {
        return label + ": " + valueText;
    }
    return valueText;
}

Vec2 clampTextPosition(const Vec2& pos, const Vec2& textSize, const Rect& bounds) {
    float x = std::clamp(pos.x, bounds.x(), bounds.right() - textSize.x);
    float y = std::clamp(pos.y, bounds.y(), bounds.bottom() - textSize.y);
    return Vec2(x, y);
}

Color sliceColorForIndex(const ChartOptions& options, const Theme& theme, size_t index) {
    if (!options.sliceColors.empty()) {
        return options.sliceColors[index % options.sliceColors.size()];
    }
    const Color palette[] = {
        theme.colors.primary,
        theme.colors.success,
        theme.colors.warning,
        theme.colors.error,
        theme.colors.info,
        theme.colors.secondary
    };
    const int paletteCount = static_cast<int>(sizeof(palette) / sizeof(palette[0]));
    return palette[index % paletteCount];
}

ChartRange resolveRange(const std::vector<float>& values, const ChartOptions& options) {
    ChartRange range;
    if (options.autoScale) {
        if (!values.empty()) {
            range.min = values.front();
            range.max = values.front();
            for (float v : values) {
                range.min = std::min(range.min, v);
                range.max = std::max(range.max, v);
            }
        }
        if (options.includeZero) {
            range.min = std::min(range.min, 0.0f);
            range.max = std::max(range.max, 0.0f);
        }
    } else {
        range.min = options.minValue;
        range.max = options.maxValue;
    }

    if (std::fabs(range.max - range.min) < 1e-6f) {
        range.max = range.min + 1.0f;
    }

    return range;
}

float normalizeValue(float value, const ChartRange& range) {
    float t = (value - range.min) / (range.max - range.min);
    return std::clamp(t, 0.0f, 1.0f);
}

Rect plotBounds(const Rect& bounds, float padding) {
    float pad = std::max(0.0f, padding);
    Rect plot = bounds.shrunk(pad);
    if (plot.width() < 0.0f) {
        plot.size.x = 0.0f;
    }
    if (plot.height() < 0.0f) {
        plot.size.y = 0.0f;
    }
    return plot;
}

void drawGrid(IDrawList& dl, const Rect& plot, int lines, Color color) {
    if (lines <= 0) return;
    for (int i = 1; i < lines; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(lines);
        float y = plot.y() + t * plot.height();
        dl.addLine(Vec2(plot.x(), y), Vec2(plot.right(), y), color, 1.0f);
    }
}

void drawAxes(IDrawList& dl, const Rect& plot, float baselineY, Color color) {
    dl.addLine(Vec2(plot.x(), plot.top()), Vec2(plot.x(), plot.bottom()), color, 1.0f);
    dl.addLine(Vec2(plot.x(), baselineY), Vec2(plot.right(), baselineY), color, 1.0f);
}

void drawLineChart(IDrawList& dl, const Rect& plot, const std::vector<float>& values,
                   const ChartOptions& options, const ChartRange& range, Color lineColor) {
    if (values.empty()) return;
    if (plot.width() <= 0.0f || plot.height() <= 0.0f) return;

    Vec2 prev{};
    bool hasPrev = false;
    const size_t count = values.size();

    for (size_t i = 0; i < count; ++i) {
        float t = (count == 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(count - 1);
        float x = plot.x() + t * plot.width();
        float y = plot.bottom() - normalizeValue(values[i], range) * plot.height();
        Vec2 pos(x, y);

        if (hasPrev) {
            dl.addLine(prev, pos, lineColor, options.lineThickness);
        }
        if (options.showPoints) {
            dl.addCircleFilled(pos, options.pointRadius, lineColor, 0);
        }

        prev = pos;
        hasPrev = true;
    }
}

void drawBarChart(IDrawList& dl, const Rect& plot, const std::vector<float>& values,
                  const ChartOptions& options, const ChartRange& range, Color barColor) {
    if (values.empty()) return;
    if (plot.width() <= 0.0f || plot.height() <= 0.0f) return;

    size_t count = values.size();
    float spacing = std::max(0.0f, options.barSpacing);
    float barWidth = (plot.width() - spacing * static_cast<float>(count - 1)) / static_cast<float>(count);
    if (barWidth < 1.0f) {
        barWidth = 1.0f;
    }

    float baselineValue = options.includeZero ? 0.0f : range.min;
    float baselineT = normalizeValue(baselineValue, range);
    float baselineY = plot.bottom() - baselineT * plot.height();

    for (size_t i = 0; i < count; ++i) {
        float x = plot.x() + static_cast<float>(i) * (barWidth + spacing);
        float valueT = normalizeValue(values[i], range);
        float valueY = plot.bottom() - valueT * plot.height();

        float top = std::min(valueY, baselineY);
        float height = std::fabs(baselineY - valueY);
        if (height <= 0.0f) continue;

        Rect barRect(x, top, barWidth, height);
        float radius = std::min(options.barCornerRadius, std::min(barWidth, height) * 0.5f);
        dl.addRectFilled(barRect, barColor, radius);
    }
}

void drawPieChart(IDrawList& dl, const Rect& plot, const std::vector<float>& values,
                  const ChartOptions& options, const Theme& theme) {
    if (values.empty()) return;

    float total = 0.0f;
    for (float v : values) {
        if (v > 0.0f) total += v;
    }
    if (total <= 0.0f) return;

    float diameter = std::min(plot.width(), plot.height());
    if (diameter <= 0.0f) return;

    Vec2 center(plot.x() + plot.width() * 0.5f, plot.y() + plot.height() * 0.5f);
    float radius = diameter * 0.5f;

    float startAngle = options.pieStartAngle * (kPi / 180.0f);
    float direction = options.pieClockwise ? -1.0f : 1.0f;
    int baseSegments = std::max(8, options.pieSegments);

    for (size_t i = 0; i < values.size(); ++i) {
        float value = values[i];
        if (value <= 0.0f) continue;

        float fraction = value / total;
        float angle = fraction * 2.0f * kPi * direction;
        float absAngle = std::fabs(angle);
        int segments = std::max(1, static_cast<int>(std::ceil(absAngle / (2.0f * kPi) * baseSegments)));
        float step = angle / static_cast<float>(segments);

        Color sliceColor = sliceColorForIndex(options, theme, i);

        for (int s = 0; s < segments; ++s) {
            float a0 = startAngle + step * static_cast<float>(s);
            float a1 = startAngle + step * static_cast<float>(s + 1);
            Vec2 p1(center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius);
            Vec2 p2(center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius);
            dl.addTriangleFilled(center, p1, p2, sliceColor);
        }

        startAngle += angle;
    }
}

} // namespace

void Chart(Context& ctx, std::string_view id, const std::vector<float>& values,
           const ChartOptions& options) {
    (void)id;
    auto wc = WidgetContext::make(ctx);
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    float width = options.style.width > 0.0f ? options.style.width : 240.0f;
    float height = options.style.height > 0.0f ? options.style.height : 160.0f;
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);

    if (options.showBackground) {
        WidgetState state{};
        drawWidgetBackground(ctx, bounds, options.style, state);
    }

    if (options.showBorder) {
        Style borderStyle = options.style;
        if (borderStyle.borderWidth <= 0.0f) {
            borderStyle.borderWidth = theme.metrics.borderWidth;
        }
        WidgetState state{};
        drawWidgetBorder(ctx, bounds, borderStyle, state);
    }

    Rect contentBounds = bounds;
    Rect legendRect = Rect::zero();
    bool legendEnabled = options.showLegend && font && !options.labels.empty();

    if (legendEnabled) {
        float maxLabelWidth = 0.0f;
        for (const auto& label : options.labels) {
            maxLabelWidth = std::max(maxLabelWidth, font->measureText(label).x);
        }
        float padding = std::max(0.0f, options.legendPadding);
        float swatch = std::max(0.0f, options.legendSwatchSize);
        float spacing = std::max(0.0f, options.legendItemSpacing);
        float legendWidth = swatch + spacing + maxLabelWidth + padding * 2.0f;

        if (legendWidth > 0.0f && legendWidth < bounds.width()) {
            legendRect = Rect(bounds.right() - legendWidth, bounds.y(), legendWidth, bounds.height());
            contentBounds.size.x = std::max(0.0f, contentBounds.width() - legendWidth);
        } else {
            legendEnabled = false;
        }
    }

    Rect plot = plotBounds(contentBounds, options.plotPadding);
    if (plot.width() <= 0.0f || plot.height() <= 0.0f) return;

    Color axisColor = resolveColor(options.axisColor, theme.colors.border);
    Color gridColor = resolveColor(options.gridColor, theme.colors.border.withAlpha(0.4f));
    Color lineColor = resolveColor(options.lineColor, theme.colors.primary);
    Color barColor = resolveColor(options.barColor, theme.colors.primary);

    ChartRange range{};
    float baselineY = plot.bottom();
    if (options.type != ChartType::Pie) {
        range = resolveRange(values, options);
        if (options.type == ChartType::Bar) {
            float baselineValue = options.includeZero ? 0.0f : range.min;
            float baselineT = normalizeValue(baselineValue, range);
            baselineY = plot.bottom() - baselineT * plot.height();
        }
    }

    dl.pushClipRect(plot);

    if (options.type != ChartType::Pie) {
        if (options.showGrid) {
            drawGrid(dl, plot, options.gridLines, gridColor);
        }

        if (options.showAxes) {
            drawAxes(dl, plot, baselineY, axisColor);
        }

        if (options.type == ChartType::Line) {
            drawLineChart(dl, plot, values, options, range, lineColor);
        } else if (options.type == ChartType::Bar) {
            drawBarChart(dl, plot, values, options, range, barColor);
        }
    } else {
        drawPieChart(dl, plot, values, options, theme);
    }

    dl.popClipRect();

    if (font && options.showLabels && !values.empty()) {
        Color labelColor = theme.colors.textSecondary;
        if (options.type == ChartType::Line) {
            size_t count = values.size();
            for (size_t i = 0; i < count; ++i) {
                float t = (count == 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(count - 1);
                float x = plot.x() + t * plot.width();
                float y = plot.bottom() - normalizeValue(values[i], range) * plot.height();
                std::string label = labelForIndex(options, i);
                if (label.empty()) {
                    label = formatValue(values[i], options.valueDecimals);
                }
                if (label.empty()) continue;

                Vec2 textSize = font->measureText(label);
                Vec2 textPos(x - textSize.x * 0.5f, y - textSize.y - options.labelOffset);
                textPos = clampTextPosition(textPos, textSize, contentBounds);
                dl.addText(font, textPos, label, labelColor);
            }
        } else if (options.type == ChartType::Bar) {
            size_t count = values.size();
            float spacing = std::max(0.0f, options.barSpacing);
            float barWidth = (plot.width() - spacing * static_cast<float>(count - 1)) / static_cast<float>(count);
            if (barWidth < 1.0f) {
                barWidth = 1.0f;
            }

            for (size_t i = 0; i < count; ++i) {
                float x = plot.x() + static_cast<float>(i) * (barWidth + spacing);
                float valueT = normalizeValue(values[i], range);
                float valueY = plot.bottom() - valueT * plot.height();

                float top = std::min(valueY, baselineY);
                float height = std::fabs(baselineY - valueY);
                if (height <= 0.0f) continue;

                std::string label = labelForIndex(options, i);
                if (label.empty()) {
                    label = formatValue(values[i], options.valueDecimals);
                }
                if (label.empty()) continue;

                Vec2 textSize = font->measureText(label);
                float labelY = top - options.labelOffset - textSize.y;
                if (labelY < contentBounds.y()) {
                    labelY = top + height + options.labelOffset;
                }
                Vec2 textPos(x + (barWidth - textSize.x) * 0.5f, labelY);
                textPos = clampTextPosition(textPos, textSize, contentBounds);
                dl.addText(font, textPos, label, labelColor);
            }
        } else {
            float total = 0.0f;
            for (float v : values) {
                if (v > 0.0f) total += v;
            }
            if (total > 0.0f) {
                float diameter = std::min(plot.width(), plot.height());
                Vec2 center(plot.x() + plot.width() * 0.5f, plot.y() + plot.height() * 0.5f);
                float radius = diameter * 0.5f;
                float startAngle = options.pieStartAngle * (kPi / 180.0f);
                float direction = options.pieClockwise ? -1.0f : 1.0f;

                for (size_t i = 0; i < values.size(); ++i) {
                    float value = values[i];
                    if (value <= 0.0f) continue;
                    float fraction = value / total;
                    float angle = fraction * 2.0f * kPi * direction;
                    float mid = startAngle + angle * 0.5f;

                    std::string label = labelForIndex(options, i);
                    if (label.empty()) {
                        label = formatValue(value, options.valueDecimals);
                    }
                    if (label.empty()) {
                        startAngle += angle;
                        continue;
                    }

                    Vec2 textSize = font->measureText(label);
                    Vec2 textPos(center.x + std::cos(mid) * radius * 0.6f - textSize.x * 0.5f,
                                 center.y + std::sin(mid) * radius * 0.6f - textSize.y * 0.5f);
                    textPos = clampTextPosition(textPos, textSize, contentBounds);
                    dl.addText(font, textPos, label, labelColor);

                    startAngle += angle;
                }
            }
        }
    }

    if (legendEnabled) {
        float padding = std::max(0.0f, options.legendPadding);
        float swatch = std::max(0.0f, options.legendSwatchSize);
        float spacing = std::max(0.0f, options.legendItemSpacing);
        float y = legendRect.y() + padding;

        for (size_t i = 0; i < options.labels.size(); ++i) {
            if (y > legendRect.bottom()) break;
            std::string label = options.labels[i];
            if (label.empty()) continue;

            float itemHeight = std::max(font->lineHeight(), swatch);
            Rect swatchRect(legendRect.x() + padding,
                            y + (itemHeight - swatch) * 0.5f,
                            swatch,
                            swatch);

            Color swatchColor = lineColor;
            if (options.type == ChartType::Pie) {
                swatchColor = sliceColorForIndex(options, theme, i);
            } else if (options.type == ChartType::Bar) {
                swatchColor = barColor;
            }

            dl.addRectFilled(swatchRect, swatchColor, 2.0f);

            Vec2 textPos(swatchRect.right() + spacing,
                         y + (itemHeight - font->lineHeight()) * 0.5f);
            dl.addText(font, textPos, label, theme.colors.text);

            y += itemHeight + spacing;
        }
    }

    if (options.showTooltips && font) {
        Vec2 mouse = ctx.input().mousePos();
        bool hoverAllowed = plot.contains(mouse) &&
                            !ctx.isPointClipped(mouse) &&
                            !ctx.isOccluded(mouse) &&
                            !ctx.isInputCaptured() &&
                            !ctx.input().isMouseConsumed();

        if (hoverAllowed) {
            int hoveredIndex = -1;
            if (options.type == ChartType::Line) {
                float bestDistSq = options.tooltipHitRadius * options.tooltipHitRadius;
                size_t count = values.size();
                for (size_t i = 0; i < count; ++i) {
                    float t = (count == 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(count - 1);
                    float x = plot.x() + t * plot.width();
                    float y = plot.bottom() - normalizeValue(values[i], range) * plot.height();
                    float dx = mouse.x - x;
                    float dy = mouse.y - y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= bestDistSq) {
                        bestDistSq = distSq;
                        hoveredIndex = static_cast<int>(i);
                    }
                }
            } else if (options.type == ChartType::Bar) {
                size_t count = values.size();
                if (count > 0) {
                    float spacing = std::max(0.0f, options.barSpacing);
                    float barWidth = (plot.width() - spacing * static_cast<float>(count - 1)) / static_cast<float>(count);
                    if (barWidth < 1.0f) {
                        barWidth = 1.0f;
                    }

                    for (size_t i = 0; i < count; ++i) {
                        float x = plot.x() + static_cast<float>(i) * (barWidth + spacing);
                        float valueT = normalizeValue(values[i], range);
                        float valueY = plot.bottom() - valueT * plot.height();

                        float top = std::min(valueY, baselineY);
                        float height = std::fabs(baselineY - valueY);
                        if (height <= 0.0f) continue;

                        Rect barRect(x, top, barWidth, height);
                        if (barRect.contains(mouse)) {
                            hoveredIndex = static_cast<int>(i);
                            break;
                        }
                    }
                }
            } else {
                float total = 0.0f;
                for (float v : values) {
                    if (v > 0.0f) total += v;
                }
                if (total > 0.0f) {
                    float diameter = std::min(plot.width(), plot.height());
                    Vec2 center(plot.x() + plot.width() * 0.5f, plot.y() + plot.height() * 0.5f);
                    float radius = diameter * 0.5f;
                    Vec2 delta(mouse.x - center.x, mouse.y - center.y);
                    float distance = delta.length();
                    if (distance <= radius) {
                        float angle = std::atan2(delta.y, delta.x);
                        float cursor = options.pieStartAngle * (kPi / 180.0f);

                        for (size_t i = 0; i < values.size(); ++i) {
                            float value = values[i];
                            if (value <= 0.0f) continue;
                            float fraction = value / total;
                            float sliceAngle = fraction * 2.0f * kPi * (options.pieClockwise ? -1.0f : 1.0f);
                            float end = cursor + sliceAngle;

                            if (angleInRange(angle, cursor, end, options.pieClockwise)) {
                                hoveredIndex = static_cast<int>(i);
                                break;
                            }

                            cursor = end;
                        }
                    }
                }
            }

            if (hoveredIndex >= 0 && static_cast<size_t>(hoveredIndex) < values.size()) {
                float value = values[static_cast<size_t>(hoveredIndex)];
                float fraction = 0.0f;
                if (options.type == ChartType::Pie) {
                    float total = 0.0f;
                    for (float v : values) {
                        if (v > 0.0f) total += v;
                    }
                    if (total > 0.0f) {
                        fraction = value / total;
                    }
                }

                std::string tooltipText = tooltipTextForValue(
                    options,
                    static_cast<size_t>(hoveredIndex),
                    value,
                    fraction,
                    options.type == ChartType::Pie);

                if (!tooltipText.empty()) {
                    ShowTooltip(ctx, tooltipText.c_str(), mouse);
                }
            }
        }
    }
}

} // namespace fst
