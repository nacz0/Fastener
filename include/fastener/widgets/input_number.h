#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// InputNumber
//=============================================================================

/**
 * @brief Options for InputNumber widget styling and behavior.
 */
struct InputNumberOptions {
    Style style;
    float step = 1.0f;      // Value change per button click
    int decimals = 0;       // Decimal places for display
    bool disabled = false;
};

/**
 * @brief Renders a numeric input with +/- buttons.
 * 
 * @param label Label displayed before the input
 * @param value Reference to the float value (modified on button click)
 * @param minVal Minimum allowed value
 * @param maxVal Maximum allowed value
 * @param options Styling and behavior options
 * @return true if the value was changed this frame
 */
bool InputNumber(const char* label, float& value, float minVal, float maxVal,
                 const InputNumberOptions& options = {});
bool InputNumber(const std::string& label, float& value, float minVal, float maxVal,
                 const InputNumberOptions& options = {});

/**
 * @brief Integer variant of InputNumber.
 */
bool InputNumberInt(const char* label, int& value, int minVal, int maxVal,
                    const InputNumberOptions& options = {});
bool InputNumberInt(const std::string& label, int& value, int minVal, int maxVal,
                    const InputNumberOptions& options = {});

} // namespace fst
