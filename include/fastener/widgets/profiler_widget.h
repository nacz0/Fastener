#pragma once

#include <string>

namespace fst {

/**
 * @brief Shows a small performance overlay in a corner of the window.
 * @param open Pointer to boolean controlling visibility.
 */
void ShowProfilerOverlay(bool* open);

/**
 * @brief Shows a detailed performance profiler window with frame breakdown.
 * @param title Window title.
 * @param open Pointer to boolean controlling visibility.
 */
void ShowProfilerWindow(const char* title, bool* open);

} // namespace fst
