#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/graphics/texture.h"

namespace fst {

class Context;

//=============================================================================
// Image
//=============================================================================

struct ImageOptions {
    Style style;
    Color tint = Color(255, 255, 255, 255);  // White = no tint
    float borderRadius = 0.0f;
};

/// Explicit DI versions
void Image(Context& ctx, Texture* texture, const ImageOptions& options = {});
void Image(Context& ctx, Texture* texture, Vec2 size, const ImageOptions& options = {});

} // namespace fst

