#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// ColorPicker
//=============================================================================
struct ColorPickerOptions {
    Style style;
    bool showAlpha = false;
    bool showNumeric = true;
    bool showHex = true;
};

bool ColorPicker(const char* label, Color& color, const ColorPickerOptions& options = {});
bool ColorPicker(const std::string& label, Color& color, const ColorPickerOptions& options = {});

} // namespace fst
