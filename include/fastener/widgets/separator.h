#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

namespace fst {

//=============================================================================
// Separator
//=============================================================================

/**
 * @brief Options for Separator widget styling.
 */
struct SeparatorOptions {
    Style style;
    Color color;              // Default: from theme
    float thickness = 1.0f;
};

/**
 * @brief Renders a horizontal separator line.
 * 
 * @param options Styling options
 */
void Separator(const SeparatorOptions& options = {});

/**
 * @brief Renders a separator with centered text label.
 * 
 * @param label Text to display centered on the separator
 * @param options Styling options
 */
void SeparatorWithLabel(std::string_view label, const SeparatorOptions& options = {});

} // namespace fst
