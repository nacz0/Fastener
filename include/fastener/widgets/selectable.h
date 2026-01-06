#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

namespace fst {

/**
 * @brief Options for customizing Selectable appearance.
 */
struct SelectableOptions {
    Style style;
    bool disabled = false;
    bool spanWidth = true;      ///< Span full available width
    float height = 0.0f;        ///< 0 = auto from text
};

/**
 * @brief Renders a selectable text item (like a list item or menu entry).
 * 
 * @param label Text to display
 * @param selected Reference to selection state
 * @param options Styling options
 * @return true if clicked/toggled this frame
 */
bool Selectable(const std::string& label, bool& selected, 
                const SelectableOptions& options = {});

bool Selectable(const char* label, bool& selected, 
                const SelectableOptions& options = {});

/**
 * @brief Selectable with external selection control (doesn't modify state).
 * 
 * @param label Text to display
 * @param selected Current selection state (read-only)
 * @param options Styling options
 * @return true if clicked this frame
 */
bool Selectable(const std::string& label, bool selected = false, 
                const SelectableOptions& options = {});

/**
 * @brief Selectable with icon prefix.
 * 
 * @param icon Icon character or emoji
 * @param label Text to display
 * @param selected Reference to selection state
 * @param options Styling options
 * @return true if clicked/toggled this frame
 */
bool SelectableWithIcon(const std::string& icon, const std::string& label, 
                        bool& selected, const SelectableOptions& options = {});

} // namespace fst
