#include "fastener/widgets/label.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

void Label(const char* text, const LabelOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    if (!font) return;
    
    // Determine color
    Color textColor = options.color.a > 0 ? options.color : theme.colors.text;
    
    // Measure text
    Vec2 textSize = font->measureText(text);
    
    // Determine bounds
    float width = options.style.width > 0 ? options.style.width : textSize.x;
    float height = options.style.height > 0 ? options.style.height : textSize.y;
    
    Rect bounds;
    if (options.style.x < 0.0f && options.style.y < 0.0f) {
        bounds = ctx->layout().allocate(width, height, options.style.flexGrow);
    } else {
        bounds = Rect(options.style.x, options.style.y, width, height);
    }
    
    // Draw text
    dl.addText(font, bounds.pos, text, nullptr, textColor);
}

void Label(const std::string& text, const LabelOptions& options) {
    Label(text.c_str(), options);
}

void LabelSecondary(const std::string& text) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    LabelOptions options;
    options.color = ctx->theme().colors.textSecondary;
    Label(text, options);
}

void LabelHeading(const std::string& text) {
    // TODO: Use larger font
    Label(text);
}

} // namespace fst
