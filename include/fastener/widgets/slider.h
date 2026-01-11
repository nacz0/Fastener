#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

/**
 * @file slider.h
 * @brief Numeric value slider (float/int) with optional value display
 * 
 * @ai_hint Modifies value& in-place within [min, max]. Returns true while
 *          the user is actively dragging (not just on release).
 *          Use SliderInt for integer values to avoid float precision issues.
 *          Value display controlled by showValue option (default: true).
 * 
 * @example
 *   static float volume = 0.5f;
 *   fst::Slider("Volume", volume, 0.0f, 1.0f);
 *   
 *   static int count = 10;
 *   fst::SliderInt("Items", count, 0, 100, {.showValue = false});
 */

namespace fst {

class Context;

//=============================================================================
// Slider
//=============================================================================
struct SliderOptions {
    Style style;
    bool showValue = true;
    int decimals = 2;         // Decimal places for value display
    bool disabled = false;
};

/// Explicit DI versions
[[nodiscard]] bool Slider(Context& ctx, std::string_view label, float& value, float minVal, float maxVal, 
            const SliderOptions& options = {});

[[nodiscard]] bool SliderInt(Context& ctx, std::string_view label, int& value, int min, int max,
               const SliderOptions& options = {});

/// Uses context stack
[[nodiscard]] bool Slider(std::string_view label, float& value, float minVal, float maxVal, 
            const SliderOptions& options = {});

[[nodiscard]] bool SliderInt(std::string_view label, int& value, int min, int max,
               const SliderOptions& options = {});

} // namespace fst

