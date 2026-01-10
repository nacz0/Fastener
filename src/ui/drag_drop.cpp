#include "fastener/ui/drag_drop.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/core/input.h"
#include "fastener/ui/layout.h"

namespace fst {

//=============================================================================
// Global Drag Drop State
//=============================================================================

struct DragDropStateEx : DragDropState {
    bool pendingClear = false;
};
    
static DragDropStateEx s_dragDropState;
static WidgetId s_currentSourceWidget = INVALID_WIDGET_ID;
static Rect s_currentTargetRect;
static bool s_inSourceBlock = false;
static bool s_inTargetBlock = false;

//=============================================================================
// Internal Helpers
//=============================================================================

static void renderDragPreview() {
    auto* ctx = Context::current();
    if (!ctx || !s_dragDropState.active) return;
    
    auto& dl = ctx->drawList();
    const auto& theme = ctx->theme();
    const auto& input = ctx->input();
    
    // Switch to overlay layer for preview
    dl.setLayer(DrawList::Layer::Overlay);
    
    // Draw preview tooltip near cursor
    Vec2 pos = input.mousePos() + Vec2(15, 15);
    
    std::string text = s_dragDropState.payload.displayText;
    if (text.empty()) {
        text = "[" + s_dragDropState.payload.type + "]";
    }
    
    Font* font = ctx->font();
    if (font) {
        Vec2 textSize = font->measureText(text);
        float padding = 8.0f;
        
        Rect bgRect(pos.x, pos.y, textSize.x + padding * 2, textSize.y + padding * 2);
        
        // Background
        dl.addRectFilled(bgRect, theme.colors.panelBackground.withAlpha(0.9f), 4.0f);
        dl.addRect(bgRect, s_dragDropState.isOverValidTarget ? 
                   theme.colors.primary : theme.colors.border, 4.0f);
        
        // Text
        dl.addText(font, pos + Vec2(padding, padding), text, theme.colors.text);
    }
    
    // Reset layer
    dl.setLayer(DrawList::Layer::Default);
}

//=============================================================================
// Source API Implementation
//=============================================================================

bool BeginDragDropSource(DragDropFlags flags) {
    auto* ctx = Context::current();
    if (!ctx) return false;
    
    s_inSourceBlock = true;
    
    const auto& input = ctx->input();
    
    // Start new drag on mouse down + drag threshold
    // BeginDragDropSource must be called AFTER the widget it applies to
    WidgetId lastWidgetId = ctx->getLastWidgetId();
    if (lastWidgetId == INVALID_WIDGET_ID) {
        // Fallback to hovered if last widget not set (but this is risky)
        lastWidgetId = ctx->getHoveredWidget();
        if (lastWidgetId == INVALID_WIDGET_ID) {
            s_inSourceBlock = false;
            return false;
        }
    }
    
    // Check if THIS widget is the active source
    if (s_dragDropState.active) {
        if (s_dragDropState.payload.sourceWidget == lastWidgetId) {
            s_currentSourceWidget = s_dragDropState.payload.sourceWidget;
            return true;
        }
        return false; // Another widget is dragging
    }
    
    // Static state for potential drag - must track which widget initiated it
    static Vec2 s_mousePressPos;
    static bool s_potentialDrag = false;
    static WidgetId s_potentialDragSource = INVALID_WIDGET_ID;
    
    if (input.isMouseDown(MouseButton::Left)) {
        // On mouse press, check if this widget's bounds contain the press position
        // Only allow a widget to start a potential drag if mouse was pressed on it
        if (input.isMousePressed(MouseButton::Left)) {
            // Get bounds for this widget from the last widget bounds (set by the widget before BeginDragDropSource)
            Rect widgetBounds = ctx->getLastWidgetBounds();
            if (widgetBounds.contains(input.mousePos())) {
                s_mousePressPos = input.mousePos();
                s_potentialDrag = true;
                s_potentialDragSource = lastWidgetId;
            }
        }
        
        // Only allow drag if THIS widget started the potential drag
        if (s_potentialDrag && s_potentialDragSource == lastWidgetId) {
            float dragDistSq = (input.mousePos() - s_mousePressPos).lengthSquared();
            if (dragDistSq > 25.0f) { // 5 pixel threshold
                // Start drag
                s_dragDropState.active = true;
                s_dragDropState.startPos = s_mousePressPos;
                s_dragDropState.currentPos = input.mousePos();
                s_dragDropState.payload.sourceWidget = lastWidgetId;
                s_currentSourceWidget = lastWidgetId;
                s_potentialDrag = false;
                s_potentialDragSource = INVALID_WIDGET_ID;
                return true;
            }
        }
    } else {
        // Mouse released, cancel potential drag
        s_potentialDrag = false;
        s_potentialDragSource = INVALID_WIDGET_ID;
        s_inSourceBlock = false;
        return false;
    }
    
    s_inSourceBlock = false;
    return false;
}

bool SetDragDropPayload(const std::string& type, const void* data, size_t size) {
    if (!s_inSourceBlock || !s_dragDropState.active) return false;
    
    s_dragDropState.payload.type = type;
    s_dragDropState.payload.data.resize(size);
    if (size > 0 && data) {
        memcpy(s_dragDropState.payload.data.data(), data, size);
    }
    
    return true;
}

void SetDragDropDisplayText(const std::string& text) {
    if (!s_inSourceBlock) return;
    s_dragDropState.payload.displayText = text;
}

void EndDragDropSource() {
    if (!s_inSourceBlock) return;
    
    auto* ctx = Context::current();
    if (ctx) {
        const auto& input = ctx->input();
        s_dragDropState.currentPos = input.mousePos();
        
        // Render preview
        if (s_dragDropState.active) {
            renderDragPreview();
        }
        
        // Check for drop (mouse released)
        if (!input.isMouseDown(MouseButton::Left) && s_dragDropState.active) {
            // Do NOT clear here immediately, as targets might be rendered later in the frame.
            // Mark for clearing at end of frame.
            s_dragDropState.pendingClear = true;
        }
    }
    
    s_inSourceBlock = false;
}

//=============================================================================
// Target API Implementation
//=============================================================================

bool BeginDragDropTarget() {
    if (!s_dragDropState.active) return false;
    
    auto* ctx = Context::current();
    if (!ctx) return false;
    
    s_inTargetBlock = true;
    
    // Get current widget bounds (from layout)
    s_currentTargetRect = ctx->layout().currentBounds();
    
    const auto& input = ctx->input();
    
    // Check if mouse is over this target
    if (s_currentTargetRect.contains(input.mousePos())) {
        s_dragDropState.hoveredDropTarget = ctx->currentId();
        return true;
    }
    
    s_inTargetBlock = false;
    return false;
}

const DragPayload* AcceptDragDropPayload(const std::string& type, DragDropFlags flags) {
    if (!s_inTargetBlock || !s_dragDropState.active) return nullptr;
    
    // Check type match
    if (s_dragDropState.payload.type != type) {
        return nullptr;
    }
    
    auto* ctx = Context::current();
    if (!ctx) return nullptr;
    
    const auto& input = ctx->input();
    
    s_dragDropState.isOverValidTarget = true;
    
    // Highlight target
    if (!(flags & DragDropFlags_AcceptNoHighlight)) {
        auto& dl = ctx->drawList();
        const auto& theme = ctx->theme();
        dl.addRect(s_currentTargetRect, theme.colors.primary, 2.0f);
    }
    
    // Check for drop
    if (input.isMouseReleased(MouseButton::Left)) {
        s_dragDropState.payload.isDelivered = true;
        return &s_dragDropState.payload;
    }
    
    return nullptr;
}

void EndDragDropTarget() {
    if (!s_inTargetBlock) return;
    
    auto* ctx = Context::current();
    if (ctx) {
        const auto& input = ctx->input();
        
        // Clear state if drop occurred
        if (s_dragDropState.payload.isDelivered) {
            s_dragDropState.clear();
        }
        // Clear state if mouse released outside valid target
        else if (input.isMouseReleased(MouseButton::Left)) {
            if (!s_dragDropState.isOverValidTarget) {
                s_dragDropState.clear();
            }
        }
    }
    
    s_inTargetBlock = false;
    s_dragDropState.isOverValidTarget = false;
}

//=============================================================================
// Query API Implementation
//=============================================================================

bool IsDragDropActive() {
    return s_dragDropState.active;
}

const DragPayload* GetDragDropPayload() {
    if (!s_dragDropState.active) return nullptr;
    return &s_dragDropState.payload;
}

void CancelDragDrop() {
    s_dragDropState.clear();
    s_dragDropState.pendingClear = false;
}

void EndDragDropFrame() {
    // If pending clear was set (mouse released), and we reached end of frame,
    // we can safely clear the state now.
    if (s_dragDropState.pendingClear) {
        s_dragDropState.clear();
        s_dragDropState.pendingClear = false;
    }
}

} // namespace fst
