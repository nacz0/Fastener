#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

namespace fst {

class Context;

//=============================================================================
// CollapsingHeader
//=============================================================================

struct CollapsingHeaderOptions {
    Style style;
    bool defaultOpen = false;
};

/// Explicit DI version
[[nodiscard]] bool CollapsingHeader(Context& ctx, std::string_view label, bool& isOpen, 
                       const CollapsingHeaderOptions& options = {});

/// Uses context stack
[[nodiscard]] bool CollapsingHeader(std::string_view label, bool& isOpen, 
                       const CollapsingHeaderOptions& options = {});

} // namespace fst

