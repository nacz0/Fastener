#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>
#include <functional>

namespace fst {

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
bool Listbox(const std::string& label, int& selectedIndex, 
             const std::vector<std::string>& items,
             const ListboxOptions& options = {});

bool Listbox(const char* label, int& selectedIndex, 
             const std::vector<std::string>& items,
             const ListboxOptions& options = {});

/**
 * @brief Multi-select listbox variant.
 * 
 * @param label Label displayed before the listbox
 * @param selectedIndices Reference to vector of selected indices
 * @param items Vector of item strings to display
 * @param options Listbox styling and behavior options
 * @return true if the selection was changed this frame
 */
bool ListboxMulti(const std::string& label, std::vector<int>& selectedIndices,
                  const std::vector<std::string>& items,
                  const ListboxOptions& options = {});

} // namespace fst
