#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>
#include <vector>

namespace fst {

//=============================================================================
// Label
//=============================================================================
struct LabelOptions {
    Style style;
    Color color = Color::transparent();  // Use theme default if transparent
    bool wrap = false;                   // Word wrap
    Alignment align = Alignment::Start;
};

void Label(std::string_view text, const LabelOptions& options = {});

// Convenience overloads
void LabelSecondary(std::string_view text);
void LabelHeading(std::string_view text);

} // namespace fst
