#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

//=============================================================================
// RadioButton
//=============================================================================

/**
 * @brief Options for RadioButton widget styling and behavior.
 */
struct RadioButtonOptions {
    Style style;
    bool disabled = false;
};

/**
 * @brief Renders a radio button for mutually exclusive selection.
 * 
 * @param label Text displayed next to the radio button
 * @param selectedIndex Reference to the currently selected index
 * @param index The index value this radio button represents
 * @param options Styling and behavior options
 * @return true if this radio button was selected this frame
 */
bool RadioButton(const char* label, int& selectedIndex, int index, 
                 const RadioButtonOptions& options = {});
bool RadioButton(const std::string& label, int& selectedIndex, int index,
                 const RadioButtonOptions& options = {});

} // namespace fst
