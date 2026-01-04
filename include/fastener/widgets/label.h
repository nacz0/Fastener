#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>

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

void Label(const std::string& text, const LabelOptions& options = {});
void Label(const char* text, const LabelOptions& options = {});

// Convenience overloads
void LabelSecondary(const std::string& text);
void LabelHeading(const std::string& text);

} // namespace fst
