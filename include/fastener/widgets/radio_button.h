#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

namespace fst {

class Context;

//=============================================================================
// RadioButton
//=============================================================================

struct RadioButtonOptions {
    Style style;
    bool disabled = false;
};

/// Explicit DI version
[[nodiscard]] bool RadioButton(Context& ctx, std::string_view label, int& selectedIndex, int index, 
                 const RadioButtonOptions& options = {});

/// Uses context stack
[[nodiscard]] bool RadioButton(std::string_view label, int& selectedIndex, int index, 
                 const RadioButtonOptions& options = {});

} // namespace fst

