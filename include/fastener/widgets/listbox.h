#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>
#include <string_view>

namespace fst {

class Context;

/**
 * @brief Options for customizing Listbox appearance and behavior.
 */
struct ListboxOptions {
    Style style;
    bool disabled = false;
    float itemHeight = 0.0f;    ///< 0 = auto-calculate from font height
    float height = 150.0f;      ///< Total height of the listbox
    bool multiSelect = false;   ///< Allow multiple selection
};

/**
 * @brief Renders a scrollable list of selectable items.
 * 
 * @param label Label displayed before the listbox
 * @param selectedIndex Reference to the selected item index (-1 for none)
 * @param items Vector of item strings to display
 * @param options Listbox styling and behavior options
 * @return true if the selection was changed this frame
 */
/// Explicit DI version
bool Listbox(Context& ctx, std::string_view label, int& selectedIndex, 
             const std::vector<std::string>& items,
             const ListboxOptions& options = {});

/// Explicit DI version
bool ListboxMulti(Context& ctx, std::string_view label, std::vector<int>& selectedIndices,
                  const std::vector<std::string>& items,
                  const ListboxOptions& options = {});

} // namespace fst
