#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// Splitter
//=============================================================================
struct SplitterOptions {
    Style style;
    Direction direction = Direction::Vertical;
    float minSize1 = 50.0f;
    float minSize2 = 50.0f;
    float splitterWidth = 6.0f;
    bool disabled = false;
};

/**
 * Splitter widget that allows interactive resizing of two panels.
 * 
 * @param id Unique identifier for the widget
 * @param splitPosition Current position of the divider (X for Vertical, Y for Horizontal)
 * @param bounds The total area available for both panels and the splitter
 * @param options Splitter configuration
 * @return true if the splitPosition was changed
 */
bool Splitter(const std::string& id, float& splitPosition, const Rect& bounds, 
             const SplitterOptions& options = {});

bool Splitter(const char* id, float& splitPosition, const Rect& bounds,
             const SplitterOptions& options = {});

} // namespace fst
