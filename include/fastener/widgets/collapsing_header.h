#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// CollapsingHeader
//=============================================================================

/**
 * @brief Options for CollapsingHeader widget styling and behavior.
 */
struct CollapsingHeaderOptions {
    Style style;
    bool defaultOpen = false;
};

/**
 * @brief Renders a collapsible header that toggles visibility of content.
 * 
 * @param label Text displayed on the header
 * @param isOpen Reference to the open/closed state
 * @param options Styling and behavior options
 * @return true if the header is currently open (for conditional child rendering)
 */
[[nodiscard]] bool CollapsingHeader(const char* label, bool& isOpen, 
                      const CollapsingHeaderOptions& options = {});
[[nodiscard]] bool CollapsingHeader(const std::string& label, bool& isOpen,
                      const CollapsingHeaderOptions& options = {});

} // namespace fst
