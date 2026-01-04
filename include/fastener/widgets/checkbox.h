#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// Checkbox
//=============================================================================
struct CheckboxOptions {
    Style style;
    bool disabled = false;
};

bool Checkbox(const std::string& label, bool& checked, const CheckboxOptions& options = {});
bool Checkbox(const char* label, bool& checked, const CheckboxOptions& options = {});

} // namespace fst
