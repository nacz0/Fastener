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

//=============================================================================
// Slider
//=============================================================================
struct SliderOptions {
    Style style;
    bool showValue = true;
    int decimals = 2;         // Decimal places for value display
    bool disabled = false;
};

/**
 * @brief Renders a horizontal slider for adjusting a float value.
 * 
 * @param label Label displayed to the left of the slider
 * @param value Reference to the float value being adjusted
 * @param minVal Minimum allowed value
 * @param maxVal Maximum allowed value
 * @param options Slider styling and behavior options
 * @return true if the value was changed this frame
 */
[[nodiscard]] bool Slider(std::string_view label, float& value, float minVal, float maxVal, 
            const SliderOptions& options = {});

/**
 * @brief Renders a horizontal slider for adjusting an integer value.
 * 
 * @param label Label displayed to the left of the slider
 * @param value Reference to the integer value being adjusted
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @param options Slider styling and behavior options
 * @return true if the value was changed this frame
 */
[[nodiscard]] bool SliderInt(std::string_view label, int& value, int min, int max,
               const SliderOptions& options = {});

} // namespace fst
