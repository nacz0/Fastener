/**
 * @file modal.cpp
 * @brief Modal/Dialog widget implementation.
 */

#include "fastener/widgets/modal.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>

namespace fst {

//=============================================================================
// Modal State
//=============================================================================

namespace {

struct ModalState {
    Rect contentBounds;
    Rect modalBounds;
    bool active = false;
    bool* openPtr = nullptr;
};

thread_local ModalState s_modalState;

} // anonymous namespace

//=============================================================================
// Modal Implementation
//=============================================================================

bool BeginModal(Context& ctx, const std::string& id, bool& isOpen, const ModalOptions& options) {
    if (!isOpen) {
        s_modalState.active = false;
        return false;
    }
    
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    ctx.pushId(id);
    
    // Get window size
    float windowW = static_cast<float>(ctx.window().width());
    float windowH = static_cast<float>(ctx.window().height());
    
    // Switch to overlay layer to render on top of everything
    dl.setLayer(DrawLayer::Overlay);
    
    // Draw backdrop (semi-transparent overlay)
    Rect backdropBounds(0, 0, windowW, windowH);
    
    // Push fullscreen clip rect to override any parent clipping
    dl.pushClipRect(backdropBounds);
    
    dl.addRectFilled(backdropBounds, Color(0, 0, 0, 150));
    
    // Block all input to widgets behind the modal backdrop
    // Handle backdrop as a widget to capture mouse events
    WidgetId backdropId = ctx.makeId("modal_backdrop");
    (void)handleWidgetInteraction(ctx, backdropId, backdropBounds, false);
    
    // Consume mouse to prevent hover on background widgets
    ctx.input().consumeMouse();
    
    // Register backdrop as floating window for occlusion in CURRENT frame
    // This helps block hover for widgets rendered AFTER the modal
    ctx.addFloatingWindowRect(backdropBounds);
    
    // Calculate modal dimensions
    float padding = options.padding > 0 ? options.padding : theme.metrics.paddingMedium;
    float titleHeight = 0;
    if (!options.title.empty() && font) {
        titleHeight = font->lineHeight() + padding * 2;
    }
    
    float modalWidth = options.width;
    float modalHeight = options.height > 0 ? options.height : 200.0f; // Default height
    
    // Center modal
    float modalX = (windowW - modalWidth) * 0.5f;
    float modalY = (windowH - modalHeight) * 0.5f;
    Rect modalBounds(modalX, modalY, modalWidth, modalHeight);
    s_modalState.modalBounds = modalBounds;
    
    // Draw modal shadow
    float shadowOffset = 8.0f;
    for (int i = 4; i > 0; --i) {
        float offset = static_cast<float>(i) * shadowOffset * 0.25f;
        uint8_t alpha = static_cast<uint8_t>(30 * (5 - i) / 4);
        Rect shadowRect(
            modalBounds.x() + offset,
            modalBounds.y() + offset,
            modalBounds.width(),
            modalBounds.height()
        );
        dl.addRectFilled(shadowRect, Color(0, 0, 0, alpha), theme.metrics.borderRadius);
    }
    
    // Draw modal background
    dl.addRectFilled(modalBounds, theme.colors.panelBackground, theme.metrics.borderRadius);
    dl.addRect(modalBounds, theme.colors.border, theme.metrics.borderRadius);
    
    // Store for backdrop click detection
    s_modalState.openPtr = &isOpen;
    
    // Handle click outside modal (backdrop click)
    if (options.closeOnBackdrop) {
        auto& input = ctx.input();
        if (input.isMousePressed(MouseButton::Left)) {
            Vec2 mousePos = input.mousePos();
            if (!modalBounds.contains(mousePos)) {
                isOpen = false;
                s_modalState.active = false;
                ctx.popId();
                return false;
            }
        }
    }
    
    // Draw title bar
    float contentY = modalBounds.y() + padding;
    if (!options.title.empty() && font) {
        Rect titleBarBounds(modalBounds.x(), modalBounds.y(), modalBounds.width(), titleHeight);
        
        // Title background
        dl.addRectFilled(
            titleBarBounds,
            theme.colors.panelBackground.lighter(0.05f),
            theme.metrics.borderRadius
        );
        
        // Title text
        Vec2 titlePos(
            titleBarBounds.x() + padding,
            titleBarBounds.center().y - font->lineHeight() * 0.5f
        );
        dl.addText(font, titlePos, options.title, theme.colors.text);
        
        // Close button (X)
        if (options.closeable) {
            float closeSize = font->lineHeight();
            Rect closeButtonBounds(
                titleBarBounds.right() - padding - closeSize,
                titleBarBounds.center().y - closeSize * 0.5f,
                closeSize,
                closeSize
            );
            
            WidgetId closeId = ctx.makeId("close");
            WidgetInteraction closeInteraction = handleWidgetInteraction(ctx, closeId, closeButtonBounds, true);
            WidgetState closeState = getWidgetState(ctx, closeId);
            
            if (closeInteraction.clicked) {
                isOpen = false;
                s_modalState.active = false;
                ctx.popId();
                return false;
            }
            
            // Draw X button
            Color xColor = closeState.hovered 
                ? theme.colors.textSecondary.lighter(0.2f) 
                : theme.colors.textSecondary;
            
            float xPadding = closeSize * 0.25f;
            dl.addLine(
                Vec2(closeButtonBounds.x() + xPadding, closeButtonBounds.y() + xPadding),
                Vec2(closeButtonBounds.right() - xPadding, closeButtonBounds.bottom() - xPadding),
                xColor, 2.0f
            );
            dl.addLine(
                Vec2(closeButtonBounds.right() - xPadding, closeButtonBounds.y() + xPadding),
                Vec2(closeButtonBounds.x() + xPadding, closeButtonBounds.bottom() - xPadding),
                xColor, 2.0f
            );
        }
        
        // Separator
        float sepY = titleBarBounds.bottom();
        dl.addLine(
            Vec2(modalBounds.x(), sepY),
            Vec2(modalBounds.right(), sepY),
            theme.colors.border,
            1.0f
        );
        
        contentY = sepY + padding;
    }
    
    // Set up content layout
    float contentHeight = modalBounds.bottom() - contentY - padding;
    Rect contentBounds(modalBounds.x() + padding, contentY, modalBounds.width() - padding * 2, contentHeight);
    
    // Set up clipping and layout
    dl.pushClipRect(contentBounds);
    ctx.layout().beginContainer(contentBounds, options.direction);
    ctx.layout().setSpacing(theme.metrics.itemSpacing);
    
    s_modalState.contentBounds = contentBounds;
    s_modalState.active = true;
    
    return true;
}

void EndModal(Context& ctx) {
    if (s_modalState.active) {
        ctx.drawList().popClipRect();  // Pop content clip rect
        ctx.layout().endContainer();
        ctx.drawList().popClipRect();  // Pop fullscreen clip rect
        ctx.drawList().setLayer(DrawLayer::Default);  // Restore default layer
        ctx.popId();
        s_modalState.active = false;
    }
}

bool ModalButton(Context& ctx, std::string_view label, bool primary) {
    auto wc = WidgetContext::make(ctx);
    
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    if (!font) return false;
    
    WidgetId id = ctx.makeId(label);
    
    // Calculate dimensions
    Vec2 textSize = font->measureText(label);
    float padding = theme.metrics.paddingMedium;
    float width = textSize.x + padding * 2;
    float height = textSize.y + padding;
    
    // Minimum width for buttons
    width = std::max(width, 80.0f);
    
    Style style;
    Rect bounds = allocateWidgetBounds(ctx, style, width, height);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(ctx, id, bounds, true);
    WidgetState state = getWidgetState(ctx, id);
    
    // Determine colors
    Color bgColor, textColor;
    if (primary) {
        bgColor = state.hovered ? theme.colors.primaryHover : theme.colors.primary;
        textColor = theme.colors.primaryText;
    } else {
        bgColor = state.hovered 
            ? theme.colors.secondary.lighter(0.1f) 
            : theme.colors.secondary;
        textColor = theme.colors.text;
    }
    
    // Draw button
    dl.addRectFilled(bounds, bgColor, theme.metrics.borderRadiusSmall);
    
    // Draw text centered
    Vec2 textPos(
        bounds.center().x - textSize.x * 0.5f,
        bounds.center().y - textSize.y * 0.5f
    );
    dl.addText(font, textPos, label, textColor);
    
    return interaction.clicked;
}

//=============================================================================
// ModalScope RAII
//=============================================================================

ModalScope::ModalScope(Context& ctx, const std::string& id, bool& isOpen, const ModalOptions& options)
    : m_ctx(&ctx), m_visible(false), m_needsEnd(true) {
    m_visible = BeginModal(ctx, id, isOpen, options);
}

ModalScope::~ModalScope() {
    if (m_needsEnd) {
        EndModal(*m_ctx);
    }
}

} // namespace fst
