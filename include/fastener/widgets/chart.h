#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>
#include <vector>

namespace fst {

class Context;

enum class ChartType : uint8_t {
    Line,
    Bar,
    Pie
};

struct ChartOptions {
    Style style;
    ChartType type = ChartType::Line;

    bool showBackground = false;
    bool showBorder = false;
    bool showAxes = true;
    bool showGrid = true;
    int gridLines = 4;
    bool showLegend = false;
    bool showLabels = false;
    bool showTooltips = false;

    bool autoScale = true;
    bool includeZero = true;
    float minValue = 0.0f;
    float maxValue = 1.0f;

    float plotPadding = 8.0f;

    float lineThickness = 2.0f;
    bool showPoints = true;
    float pointRadius = 3.0f;
    float tooltipHitRadius = 8.0f;
    float labelOffset = 4.0f;

    float barSpacing = 4.0f;
    float barCornerRadius = 2.0f;

    float pieStartAngle = -90.0f;
    bool pieClockwise = true;
    int pieSegments = 48;

    float legendPadding = 8.0f;
    float legendItemSpacing = 6.0f;
    float legendSwatchSize = 10.0f;

    int valueDecimals = 2;

    Color lineColor = Color(0, 0, 0, 0);
    Color barColor = Color(0, 0, 0, 0);
    Color axisColor = Color(0, 0, 0, 0);
    Color gridColor = Color(0, 0, 0, 0);

    std::vector<Color> sliceColors;
    std::vector<std::string> labels;
};

void Chart(Context& ctx, std::string_view id, const std::vector<float>& values,
           const ChartOptions& options = {});

} // namespace fst
