#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string_view>

namespace fst {

class Context;

struct PillOptions {
    Style style;
    Color color = Color::transparent();
    Color textColor = Color::transparent();
    float horizontalPadding = 10.0f;
    float verticalPadding = 4.0f;
    bool outlined = false;
};

void Pill(Context& ctx, std::string_view text, const PillOptions& options = {});

} // namespace fst

