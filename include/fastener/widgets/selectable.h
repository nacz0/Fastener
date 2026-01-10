#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

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
 * @param selected Reference to selection state (will be toggled on click)
 * @param options Styling options
 * @return true if clicked/toggled this frame
 */
[[nodiscard]] bool Selectable(std::string_view label, bool& selected, 
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
[[nodiscard]] bool SelectableWithIcon(std::string_view icon, std::string_view label, 
                        bool& selected, const SelectableOptions& options = {});

} // namespace fst
