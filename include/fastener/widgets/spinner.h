#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

/**
 * @brief Options for customizing Spinner appearance.
 */
struct SpinnerOptions {
    Style style;
    float size = 24.0f;         ///< Diameter of the spinner
    float thickness = 3.0f;     ///< Line thickness
    float speed = 1.0f;         ///< Rotation speed multiplier
    Color color = Color(0, 0, 0, 0);  ///< 0 = use theme primary color
};

/**
 * @brief Renders an animated loading spinner.
 * 
 * @param id Unique identifier for the spinner
 * @param options Spinner styling options
 */
void Spinner(const std::string& id, const SpinnerOptions& options = {});
void Spinner(const char* id, const SpinnerOptions& options = {});

/**
 * @brief Renders a spinner with a label.
 * 
 * @param id Unique identifier
 * @param label Text to display next to spinner
 * @param options Spinner styling options
 */
void SpinnerWithLabel(const std::string& id, const std::string& label, 
                      const SpinnerOptions& options = {});

/**
 * @brief Dots-style loading indicator (alternative to spinner).
 * 
 * @param id Unique identifier
 * @param options Styling options (size controls dot size)
 */
void LoadingDots(const std::string& id, const SpinnerOptions& options = {});

} // namespace fst
