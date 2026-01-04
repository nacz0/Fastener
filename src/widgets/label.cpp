#include "fastener/widgets/label.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"

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
    
    // Calculate position (simple for now)
    Vec2 pos(0, 0);
    // TODO: Get from layout system
    
    // Draw text
    dl.addText(font, pos, text, nullptr, textColor);
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
