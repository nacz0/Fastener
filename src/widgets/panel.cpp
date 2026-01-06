#include "fastener/widgets/panel.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

static int s_panelDepth = 0;

PanelScope::PanelScope(const std::string& id, const PanelOptions& options) 
    : m_visible(true), m_needsEnd(true) 
{
    m_visible = BeginPanel(id, options);
    m_needsEnd = true;
}

PanelScope::~PanelScope() {
    if (m_needsEnd) {
        EndPanel();
    }
}

bool BeginPanel(const std::string& id, const PanelOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    
    // Generate ID
    ctx->pushId(id.c_str());
    WidgetId widgetId = ctx->currentId();
    
    // Get bounds
    float width = options.style.width > 0 ? options.style.width : 300.0f;
    float height = options.style.height > 0 ? options.style.height : 200.0f;
    
    Rect bounds;
    if (options.style.x < 0.0f && options.style.y < 0.0f) {
        bounds = ctx->layout().allocate(width, height, options.style.flexGrow);
    } else {
        bounds = Rect(options.style.x, options.style.y, width, height);
    }
    
    // Draw panel background
    Color bgColor = options.style.backgroundColor.a > 0 ? 
                    options.style.backgroundColor : theme.colors.panelBackground;
    
    float radius = options.style.borderRadius > 0 ? 
                   options.style.borderRadius : theme.metrics.borderRadius;
    
    // Draw shadow
    if (options.style.hasShadow || s_panelDepth == 0) {
        dl.addShadow(bounds, theme.colors.shadow, theme.metrics.shadowSize, radius);
    }
    
    // Draw background
    dl.addRectFilled(bounds, bgColor, radius);
    
    // Draw border
    if (options.style.borderWidth > 0) {
        dl.addRect(bounds, theme.colors.border, radius);
    }
    
    // Draw title if provided
    float contentY = bounds.y();
    if (!options.title.empty()) {
        Font* font = ctx->font();
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
            
            // Separator line
            float sepY = titleRect.bottom() - 1;
            dl.addRectFilled(Rect(bounds.x(), sepY, bounds.width(), 1), theme.colors.border);
            
            contentY = titleRect.bottom();
        }
    }
    
    // Push clip rect for content
    Rect contentRect(
        bounds.x() + theme.metrics.paddingMedium,
        contentY + theme.metrics.paddingMedium,
        bounds.width() - theme.metrics.paddingMedium * 2,
        bounds.bottom() - contentY - theme.metrics.paddingMedium
    );
    dl.pushClipRect(contentRect);
    
    // Begin layout container for interior
    ctx->layout().beginContainer(contentRect, options.direction);
    if (options.spacing > 0) {
        ctx->layout().setSpacing(options.spacing);
    } else {
        ctx->layout().setSpacing(theme.metrics.paddingSmall);
    }
    
    s_panelDepth++;
    
    return true;
}

void EndPanel() {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    s_panelDepth--;
    
    // Pop clip rect
    ctx->drawList().popClipRect();
    
    // End layout container for interior
    ctx->layout().endContainer();
    
    ctx->popId();
}

} // namespace fst
