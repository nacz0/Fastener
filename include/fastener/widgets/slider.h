#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

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

bool Slider(const std::string& label, float& value, float min, float max, 
            const SliderOptions& options = {});
bool Slider(const char* label, float& value, float min, float max,
            const SliderOptions& options = {});

// Integer slider
bool SliderInt(const std::string& label, int& value, int min, int max,
               const SliderOptions& options = {});

} // namespace fst
