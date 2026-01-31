#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include "fastener/graphics/svg.h"

namespace fst {

class Context;

//=============================================================================
// SvgImage
//=============================================================================

struct SvgImageOptions {
    Style style;
    Color tint = Color::white();
    float borderRadius = 0.0f;
    bool preserveAspectRatio = true;
};

/// Explicit DI versions
void SvgImage(Context& ctx, SvgDocument* document, const SvgImageOptions& options = {});
void SvgImage(Context& ctx, SvgDocument* document, Vec2 size, const SvgImageOptions& options = {});

} // namespace fst
