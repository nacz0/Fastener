#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

namespace fst {

class Context;

/**
 * @brief Options for customizing Selectable appearance.
 */
struct SelectableOptions {
    Style style;
    bool disabled = false;
    bool spanWidth = true;      ///< Span full available width
    float height = 0.0f;        ///< 0 = auto from text
};

/// Explicit DI versions
[[nodiscard]] bool Selectable(Context& ctx, std::string_view label, bool& selected, 
                const SelectableOptions& options = {});

[[nodiscard]] bool SelectableWithIcon(Context& ctx, std::string_view icon, std::string_view label, 
                        bool& selected, const SelectableOptions& options = {});

} // namespace fst

