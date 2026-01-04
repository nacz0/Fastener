#include "fastener/ui/widget.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/theme.h"

namespace fst {

WidgetState getWidgetState(WidgetId id) {
    Context* ctx = Context::current();
    if (!ctx) return {};
    
    WidgetState state;
    state.hovered = ctx->getHoveredWidget() == id;
    state.focused = ctx->getFocusedWidget() == id;
    state.active = ctx->getActiveWidget() == id;
    
    return state;
}

WidgetInteraction handleWidgetInteraction(WidgetId id, const Rect& bounds, bool focusable) {
    Context* ctx = Context::current();
    if (!ctx) return {};
    
    WidgetInteraction result;
    const InputState& input = ctx->input();
    
    // Check if mouse is over widget
    Vec2 mousePos = input.mousePos();
    bool isHovered = bounds.contains(mousePos);
    
    if (isHovered) {
        ctx->setHoveredWidget(id);
        result.hovered = true;
    }
    
    // Handle mouse clicks
    if (isHovered && input.isMousePressed(MouseButton::Left)) {
        ctx->setActiveWidget(id);
        if (focusable) {
            ctx->setFocusedWidget(id);
        }
    }
    
    if (ctx->getActiveWidget() == id) {
        if (input.isMouseReleased(MouseButton::Left)) {
            if (isHovered) {
                result.clicked = true;
            }
            ctx->clearActiveWidget();
        }
        
        if (input.isMouseDown(MouseButton::Left)) {
            result.dragging = true;
            result.dragDelta = input.mouseDelta();
        }
    }
    
    // Double click
    if (isHovered && input.isMouseDoubleClicked(MouseButton::Left)) {
        result.doubleClicked = true;
    }
    
    // Right click
    if (isHovered && input.isMousePressed(MouseButton::Right)) {
        result.rightClicked = true;
    }
    
    result.focused = ctx->getFocusedWidget() == id;
    
    return result;
}

void drawWidgetBackground(const Rect& bounds, const Style& style, const WidgetState& state) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    
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

void drawWidgetBorder(const Rect& bounds, const Style& style, const WidgetState& state) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    
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
