#pragma once

#include "fastener/core/types.h"

namespace fst {

class Context;

namespace detail {

bool allocateGridItem(Context& ctx, float width, float height, Rect& bounds);

} // namespace detail
} // namespace fst
