#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Widget Context Helpers
//=============================================================================



WidgetContext getWidgetContext(Context& ctx) {
    return WidgetContext::make(ctx);
}

WidgetContext WidgetContext::make(Context& ctx) {
    WidgetContext wc{};
    wc.ctx = &ctx;
    wc.theme = &ctx.theme();
    wc.dl = ctx.activeDrawList();
    wc.font = ctx.font();
    return wc;
}

Rect allocateWidgetBounds(Context& ctx, const Style& style, float width, float height) {
    if (style.x < 0.0f && style.y < 0.0f) {
        return ctx.layout().allocate(width, height, style.flexGrow);
    }
    return Rect(style.x, style.y, width, height);
}

Color getStateColor(Color baseColor, Color hoverColor, Color activeColor,
                    const WidgetState& state, float disabledAlpha) {
    if (state.disabled) {
        return baseColor.withAlpha(static_cast<uint8_t>(baseColor.a * disabledAlpha));
    }
    if (state.active) {
        return activeColor;
    }
    if (state.hovered) {
        return hoverColor;
    }
    return baseColor;
}

//=============================================================================
// Widget State Functions
//=============================================================================

WidgetState getWidgetState(Context& ctx, WidgetId id) {
    WidgetState state;
    state.hovered = ctx.getHoveredWidget() == id;
    state.focused = ctx.getFocusedWidget() == id;
    state.active = ctx.getActiveWidget() == id;
    
    return state;
}

WidgetInteraction handleWidgetInteraction(Context& ctx, WidgetId id, const Rect& bounds, bool focusable,
                                          bool ignoreOcclusion, bool ignoreConsumed) {
    WidgetInteraction result;
    const InputState& input = ctx.input();
    
    // Check if mouse is over widget
    Vec2 mousePos = input.mousePos();
    bool isHovered = bounds.contains(mousePos);
    
    // Check for clipping and existing capture (exclude ourselves if we are already active)
    bool clipped = ctx.isPointClipped(mousePos);
    bool captured = ctx.isInputCaptured() && !ctx.isCapturedBy(id);
    bool occluded = ignoreOcclusion ? false : ctx.isOccluded(mousePos);
    bool consumed = ctx.input().isMouseConsumed();
    if (ignoreConsumed) {
        consumed = false;
    }
    
    if (isHovered && !clipped && !captured && !occluded && !consumed) {
        ctx.setHoveredWidget(id);
        result.hovered = true;
    }

    // Handle mouse clicks
    if (isHovered && !clipped && !captured && !occluded && !consumed && input.isMousePressed(MouseButton::Left)) {
        ctx.setActiveWidget(id);
        if (focusable) {
            ctx.setFocusedWidget(id);
        }
    }
    
    if (ctx.getActiveWidget() == id) {
        if (input.isMouseReleased(MouseButton::Left)) {
            // Only register click if still hovered AND not blocked by overlays
            // This prevents click-through when an overlay (toast, modal) now covers the widget
            if (isHovered && !occluded && !consumed) {
                result.clicked = true;
            }
            ctx.clearActiveWidget();
        }
        
        if (input.isMouseDown(MouseButton::Left)) {
            result.dragging = true;
            result.dragDelta = input.mouseDelta();
        }
    }
    
    // Double click
    if (isHovered && !occluded && !consumed && input.isMouseDoubleClicked(MouseButton::Left)) {
        result.doubleClicked = true;
    }
    
    // Right click
    if (isHovered && !occluded && !consumed && input.isMousePressed(MouseButton::Right)) {
        result.rightClicked = true;
    }
    
    result.focused = ctx.getFocusedWidget() == id;
    
    // Store this widget's info for drag-drop source identification
    ctx.setLastWidgetId(id);
    ctx.setLastWidgetBounds(bounds);
    
    return result;
}

void drawWidgetBackground(Context& ctx, const Rect& bounds, const Style& style, const WidgetState& state) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    
    // Draw shadow
    if (style.hasShadow && style.shadowSize > 0) {
        Color shadowColor = style.shadowColor.a > 0 ? style.shadowColor : theme.colors.shadow;
        dl.addShadow(bounds, shadowColor, style.shadowSize, style.borderRadius);
    }
    
    // Determine background color
    Color bgColor = style.backgroundColor;
    if (bgColor.a == 0) {
        // Use theme default
        bgColor = theme.colors.panelBackground;
    }
    
    // Adjust for state
    if (state.active) {
        bgColor = bgColor.darker(0.1f);
    } else if (state.hovered) {
        bgColor = bgColor.lighter(0.05f);
    }
    
    // Draw background
    float radius = style.borderRadius > 0 ? style.borderRadius : theme.metrics.borderRadius;
    dl.addRectFilled(bounds, bgColor, radius);
}

void drawWidgetBorder(Context& ctx, const Rect& bounds, const Style& style, const WidgetState& state) {
    IDrawList& dl = *ctx.activeDrawList();
    const Theme& theme = ctx.theme();
    
    if (style.borderWidth <= 0) return;
    
    // Determine border color
    Color borderColor = style.borderColor;
    if (borderColor.a == 0) {
        if (state.focused) {
            borderColor = theme.colors.borderFocused;
        } else if (state.hovered) {
            borderColor = theme.colors.borderHover;
        } else {
            borderColor = theme.colors.border;
        }
    }
    
    float radius = style.borderRadius > 0 ? style.borderRadius : theme.metrics.borderRadius;
    dl.addRect(bounds, borderColor, radius);
}

} // namespace fst
