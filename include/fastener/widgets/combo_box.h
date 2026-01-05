#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <vector>

namespace fst {

struct ComboBoxOptions {
    Style style;
    bool disabled = false;
    float dropdownMaxHeight = 200.0f;
};

bool ComboBox(const std::string& label, int& selectedIndex, 
              const std::vector<std::string>& items,
              const ComboBoxOptions& options = {});

bool ComboBox(const char* label, int& selectedIndex, 
              const std::vector<std::string>& items,
              const ComboBoxOptions& options = {});

} // namespace fst
