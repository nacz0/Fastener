#pragma once

#include <string>

namespace fst {
class Context;


/**
 * @brief Shows a small performance overlay in a corner of the window.
 * @param open Pointer to boolean controlling visibility.
 */
void ShowProfilerOverlay(Context& ctx, bool* open = nullptr);

/**
 * @brief Shows a detailed performance profiler window with frame breakdown.
 * @param title Window title.
 * @param open Pointer to boolean controlling visibility.
 */
void ShowProfilerWindow(Context& ctx, const char* title, bool* open = nullptr);

} // namespace fst
