#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

struct ProgressBarOptions {
    Style style;
    bool showPercentage = true;
    bool indeterminate = false;  // Animated unknown progress
    Color fillColor = Color(0, 0, 0, 0);  // 0 = use theme primary
};

void ProgressBar(float progress, const ProgressBarOptions& options = {});
void ProgressBar(const std::string& label, float progress, const ProgressBarOptions& options = {});

} // namespace fst
