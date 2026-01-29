/**
 * @file panel.cpp
 * @brief Panel container widget implementation.
 */

#include "fastener/widgets/panel.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Panel State
//=============================================================================

/** @brief Tracks nested panel depth for shadow rendering decisions. */
static int s_panelDepth = 0;

//=============================================================================
// PanelScope Implementation
//=============================================================================

/**
 * @brief RAII helper for automatic BeginPanel/EndPanel pairing.
 */
PanelScope::PanelScope(Context& ctx, const std::string& id, const PanelOptions& options) 
    : m_ctx(&ctx), m_visible(true), m_needsEnd(true) 
{
    m_visible = BeginPanel(ctx, id, options);
}

PanelScope::~PanelScope() {
    if (m_visible && m_ctx) {
        EndPanel(*m_ctx);
    }
}

//=============================================================================
// Panel Functions
//=============================================================================

/**
 * @brief Begin a panel container with background and optional title.
 * 
 * Must be paired with EndPanel(). Use PanelScope for automatic cleanup.
 * 
 * @param ctx Explicit UI context
 * @param id Unique identifier string for the panel
 * @param options Panel styling and layout options
 * @return true (always visible, can be used for conditional content)
 */
bool BeginPanel(Context& ctx, const std::string& id, const PanelOptions& options) {
    // Get widget context
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    
    // Push panel ID onto stack
    ctx.pushId(id.c_str());
    
    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 300.0f;
    float height = options.style.height > 0 ? options.style.height : 200.0f;
    
    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, width, height);
    
    bool useBlur = options.style.blurRadius > 0.0f;
    
    // Determine background color
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
    
    float radius = options.style.borderRadius > 0 
        ? options.style.borderRadius 
        : theme.metrics.borderRadius;
    
    // Draw shadow for top-level panels or when explicitly requested
    if (options.style.hasShadow || s_panelDepth == 0) {
        dl.addShadow(bounds, theme.colors.shadow, theme.metrics.shadowSize, radius);
    }
    
    // Draw panel background
    if (useBlur) {
        dl.addBlurRect(bounds, options.style.blurRadius, radius);
    }
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw border if specified
    if (options.style.borderWidth > 0) {
        dl.addRect(bounds, theme.colors.border, radius);
    }
    
    // Draw title bar if title is provided
    float contentY = bounds.y();
    if (!options.title.empty()) {
        Font* font = wc.font;
        if (font) {
            float titleHeight = theme.metrics.paddingLarge * 2 + font->lineHeight();
            Rect titleRect(bounds.x(), bounds.y(), bounds.width(), titleHeight);
            
            // Title background (slightly darker)
            dl.addRectFilled(titleRect, bgColor.darker(0.05f), radius);
            
            // Title text
            Vec2 titlePos(
                bounds.x() + theme.metrics.paddingMedium,
                bounds.y() + theme.metrics.paddingLarge
            );
            dl.addText(font, titlePos, options.title, theme.colors.text);
            
            // Separator line under title
            float sepY = titleRect.bottom() - 1;
            dl.addRectFilled(Rect(bounds.x(), sepY, bounds.width(), 1), theme.colors.border);
            
            contentY = titleRect.bottom();
        }
    }
    
    // Calculate content area with padding
    Rect contentRect(
        bounds.x() + theme.metrics.paddingMedium,
        contentY + theme.metrics.paddingMedium,
        bounds.width() - theme.metrics.paddingMedium * 2,
        bounds.bottom() - contentY - theme.metrics.paddingMedium
    );
    
    // Set up clipping and layout for child widgets
    dl.pushClipRect(contentRect);
    
    ctx.layout().beginContainer(contentRect, options.direction);
    if (options.spacing > 0) {
        ctx.layout().setSpacing(options.spacing);
    } else {
        ctx.layout().setSpacing(theme.metrics.paddingSmall);
    }
    
    s_panelDepth++;
    
    return true;
}


/**
 * @brief End a panel container started with BeginPanel().
 */
void EndPanel(Context& ctx) {
    s_panelDepth--;
    
    // Restore clip rect
    ctx.drawList().popClipRect();
    
    // End layout container
    ctx.layout().endContainer();
    
    // Pop panel ID
    ctx.popId();
}

} // namespace fst
