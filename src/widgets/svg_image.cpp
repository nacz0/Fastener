/**
 * @file svg_image.cpp
 * @brief SvgImage widget implementation for SVG rendering.
 */

#include "fastener/widgets/svg_image.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// SvgImage Implementation (Explicit DI)
//=============================================================================

void SvgImage(Context& ctx, SvgDocument* document, const SvgImageOptions& options) {
    if (!document && options.style.width <= 0 && options.style.height <= 0) return;

    Vec2 size(0, 0);
    if (document && document->isValid()) {
        size = document->size();
    }

    SvgImage(ctx, document, size, options);
}

void SvgImage(Context& ctx, SvgDocument* document, Vec2 size, const SvgImageOptions& options) {
    auto wc = WidgetContext::make(ctx);
    IDrawList& dl = *wc.dl;

    float width = options.style.width > 0 ? options.style.width : size.x;
    float height = options.style.height > 0 ? options.style.height : size.y;

    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);

    bool rendered = false;
    if (document && document->isValid()) {
        SvgRenderOptions renderOpts;
        renderOpts.preserveAspectRatio = options.preserveAspectRatio;
        renderOpts.tint = options.tint;
        rendered = document->render(dl, bounds, renderOpts);
    }

    if (!rendered) {
        const Theme& theme = *wc.theme;
        dl.addRectFilled(bounds, theme.colors.secondary, options.borderRadius);
    }
}

} // namespace fst
