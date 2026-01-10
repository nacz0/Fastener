#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>
#include <vector>

namespace fst {

struct ProgressBarOptions {
    Style style;
    bool showPercentage = true;
    bool indeterminate = false;  // Animated unknown progress
    Color fillColor = Color(0, 0, 0, 0);  // 0 = use theme primary
};

void ProgressBar(float progress, const ProgressBarOptions& options = {});
void ProgressBar(std::string_view label, float progress, const ProgressBarOptions& options = {});

} // namespace fst
