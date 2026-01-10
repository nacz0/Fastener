#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>
#include <vector>

namespace fst {

struct ComboBoxOptions {
    Style style;
    bool disabled = false;
    float dropdownMaxHeight = 200.0f;
};

[[nodiscard]] bool ComboBox(std::string_view label, int& selectedIndex,
               const std::vector<std::string>& items,
               const ComboBoxOptions& options = {});

} // namespace fst
