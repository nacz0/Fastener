/**
 * @file card.cpp
 * @brief Card widget implementation.
 */

#include "fastener/widgets/card.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>

namespace fst {

//=============================================================================
// Card Implementation
//=============================================================================

bool BeginCard(Context& ctx, const std::string& id, const CardOptions& options) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    ctx.pushId(id);
    
    // Calculate dimensions
    float padding = options.padding > 0 ? options.padding : theme.metrics.paddingMedium;
    float width = options.style.width > 0 ? options.style.width : 300.0f;
    float height = options.style.height > 0 ? options.style.height : 200.0f;
    
    // Calculate title bar height if title exists
    float titleHeight = 0.0f;
    if (!options.title.empty() && font) {
        titleHeight = font->lineHeight() + padding;
    }
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    // Draw shadow (offset rectangles with decreasing opacity)
    if (options.shadow > 0) {
        float shadowOffset = options.shadow * 0.5f;
        int shadowLayers = static_cast<int>(options.shadow);
        for (int i = shadowLayers; i > 0; --i) {
            float offset = static_cast<float>(i) * shadowOffset * 0.3f;
            uint8_t alpha = static_cast<uint8_t>(20 * (shadowLayers - i + 1) / shadowLayers);
            Rect shadowRect(
                bounds.x() + offset,
                bounds.y() + offset,
                bounds.width(),
                bounds.height()
            );
            dl.addRectFilled(shadowRect, Color(0, 0, 0, alpha), theme.metrics.borderRadius);
        }
    }
    
    bool useBlur = options.style.blurRadius > 0.0f;
    
    Color bgColor = options.style.backgroundColor.a > 0
        ? options.style.backgroundColor
        : theme.colors.panelBackground;
    if (useBlur) {
        if (options.style.blurTint.a > 0) {
            bgColor = options.style.blurTint;
        } else if (bgColor.a == 255) {
            bgColor = bgColor.withAlpha(static_cast<uint8_t>(200));
        }
    }
    
    // Draw card background
    if (useBlur) {
        dl.addBlurRect(bounds, options.style.blurRadius, theme.metrics.borderRadius);
    }
    dl.addRectFilled(bounds, bgColor, theme.metrics.borderRadius);
    dl.addRect(bounds, theme.colors.border, theme.metrics.borderRadius);
    
    // Draw title bar if title exists
    float contentY = bounds.y() + padding;
    if (!options.title.empty() && font) {
        Rect titleBarBounds(bounds.x(), bounds.y(), bounds.width(), titleHeight);
        
        // Title bar background (slightly different shade)
        dl.addRectFilled(
            titleBarBounds,
            bgColor.lighter(0.05f),
            theme.metrics.borderRadius
        );
        
        // Title text
        Vec2 titlePos(
            titleBarBounds.x() + padding,
            titleBarBounds.center().y - font->lineHeight() * 0.5f
        );
        dl.addText(font, titlePos, options.title, theme.colors.text);
        
        // Separator line under title
        float sepY = titleBarBounds.bottom();
        dl.addLine(
            Vec2(bounds.x(), sepY),
            Vec2(bounds.right(), sepY),
            theme.colors.border,
            1.0f
        );
        
        contentY = sepY + padding;
    }
    
    // Set up content layout region
    float contentHeight = bounds.bottom() - contentY - padding;
    Rect contentBounds(bounds.x() + padding, contentY, bounds.width() - padding * 2, contentHeight);
    
    // Set up clipping and layout
    dl.pushClipRect(contentBounds);
    ctx.layout().beginContainer(contentBounds, options.direction);
    
    float spacing = options.spacing > 0 ? options.spacing : theme.metrics.itemSpacing;
    ctx.layout().setSpacing(spacing);
    
    return true;
}

void EndCard(Context& ctx) {
    ctx.drawList().popClipRect();
    ctx.layout().endContainer();
    ctx.popId();
}

//=============================================================================
// CardScope RAII
//=============================================================================

CardScope::CardScope(Context& ctx, const std::string& id, const CardOptions& options)
    : m_ctx(&ctx), m_visible(false), m_needsEnd(true) {
    m_visible = BeginCard(ctx, id, options);
}

CardScope::~CardScope() {
    if (m_needsEnd) {
        EndCard(*m_ctx);
    }
}

} // namespace fst
