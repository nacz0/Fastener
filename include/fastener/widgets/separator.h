#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

namespace fst {

class Context;

//=============================================================================
// Separator
//=============================================================================

struct SeparatorOptions {
    Style style;
    Color color;              // Default: from theme
    float thickness = 1.0f;
};

/// Explicit DI versions
void Separator(Context& ctx, const SeparatorOptions& options = {});
void SeparatorWithLabel(Context& ctx, std::string_view label, const SeparatorOptions& options = {});

} // namespace fst

