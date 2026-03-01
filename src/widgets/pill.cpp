/**
 * @file pill.cpp
 * @brief Pill widget implementation.
 */

#include "fastener/widgets/pill.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/layout.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget_utils.h"

namespace fst {

void Pill(Context& ctx, std::string_view text, const PillOptions& options) {
    auto wc = WidgetContext::make(ctx);

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;

    Vec2 textSize(0.0f, 0.0f);
    if (font && !text.empty()) {
        textSize = font->measureText(text);
    }

    // Respect explicit size from style, otherwise derive from text and padding.
    float width = options.style.width > 0.0f
        ? options.style.width
        : textSize.x + options.horizontalPadding * 2.0f;
    float height = options.style.height > 0.0f
        ? options.style.height
        : textSize.y + options.verticalPadding * 2.0f;

    if (width <= 0.0f) {
        width = 32.0f;
    }
    if (height <= 0.0f) {
        height = 20.0f;
    }

    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);

    Color bgColor = options.color.a > 0 ? options.color : theme.colors.secondary;
    Color fgColor = options.textColor.a > 0 ? options.textColor : theme.colors.text;
    float radius = bounds.height() * 0.5f;

    if (options.outlined) {
        dl.addRect(bounds, bgColor, radius);
    } else {
        dl.addRectFilled(bounds, bgColor, radius);
    }

    if (font && !text.empty()) {
        Vec2 textPos(
            bounds.center().x - textSize.x * 0.5f,
            bounds.center().y - textSize.y * 0.5f
        );
        dl.addText(font, textPos, text, fgColor);
    }
}

} // namespace fst
