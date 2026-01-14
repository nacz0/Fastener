#include "fastener/ui/drag_drop.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/core/input.h"
#include "fastener/ui/layout.h"
#include "fastener/platform/cursor_utils.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#endif

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
static Vec2 s_mousePressPos;
static Vec2 s_globalMousePressPos;  // Global (screen) coordinates at mouse press
static bool s_potentialDrag = false;
static WidgetId s_potentialDragSource = INVALID_WIDGET_ID;

//=============================================================================
// Internal Helpers
//=============================================================================

// Check GLOBAL mouse button state (works across all windows)
static bool IsGlobalMouseButtonDown(Context* ctx, MouseButton button) {
#ifdef _WIN32
    int vk = 0;
    switch (button) {
        case MouseButton::Left: vk = VK_LBUTTON; break;
        case MouseButton::Right: vk = VK_RBUTTON; break;
        case MouseButton::Middle: vk = VK_MBUTTON; break;
        default: return false;
    }
    // GetAsyncKeyState returns global state regardless of which window has focus
    return (::GetAsyncKeyState(vk) & 0x8000) != 0;
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        // Fallback to local state if can't open display
        if (!ctx) return false;
        return ctx->input().isMouseDown(button);
    }
    
    ::Window root = DefaultRootWindow(display);
    ::Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);
    XCloseDisplay(display);
    
    switch (button) {
        case MouseButton::Left: return (mask & Button1Mask) != 0;
        case MouseButton::Right: return (mask & Button3Mask) != 0;
        case MouseButton::Middle: return (mask & Button2Mask) != 0;
        default: return false;
    }
#else
    // Fallback for other platforms - use local state
    if (!ctx) return false;
    return ctx->input().isMouseDown(button);
#endif
}

static void renderDragPreview(Context& ctx) {
    if (!s_dragDropState.active) return;
    
    IDrawList& dl = *ctx.activeDrawList();
    const auto& theme = ctx.theme();
    
    // Switch to overlay layer for preview
    DrawLayer prevLayer = dl.currentLayer();
    dl.setLayer(DrawLayer::Overlay);
    
    // For cross-window D&D: use global cursor converted to this window's local coordinates
    // This ensures preview appears at cursor even if drag started in different window
    Vec2 pos = GetCursorPosInWindow(ctx.window()) + Vec2(15, 15);
    
    std::string text = s_dragDropState.payload.displayText;
    if (text.empty()) {
        text = "[" + s_dragDropState.payload.type + "]";
    }
    
    Font* font = ctx.font();
    float padding = 8.0f;
    Vec2 textSize = font ? font->measureText(text) : Vec2(80, 14);
    
    Rect bgRect(pos.x, pos.y, textSize.x + padding * 2, textSize.y + padding * 2);
    
    // Background
    dl.addRectFilled(bgRect, theme.colors.panelBackground.withAlpha(0.9f), 4.0f);
    dl.addRect(bgRect, s_dragDropState.isOverValidTarget ? 
               theme.colors.primary : theme.colors.border, 4.0f);
    
    // Text
    if (font) {
        dl.addText(font, pos + Vec2(padding, padding), text, theme.colors.text);
    }
    
    // Reset layer
    dl.setLayer(prevLayer);
}

//=============================================================================
// Source API Implementation
//=============================================================================

bool BeginDragDropSource(Context& ctx, DragDropFlags flags) {
    s_inSourceBlock = true;
    
    const auto& input = ctx.input();
    
    // Start new drag on mouse down + drag threshold
    // BeginDragDropSource must be called AFTER the widget it applies to
    WidgetId lastWidgetId = ctx.getLastWidgetId();
    if (lastWidgetId == INVALID_WIDGET_ID) {
        // Fallback to hovered if last widget not set (but this is risky)
        lastWidgetId = ctx.getHoveredWidget();
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
    
    // Drag initiation logic using global potential drag state
    // NOTE: Use per-window input state here to stay consistent with other widgets
    // Global state is only used in AcceptDragDropPayload for cross-window drop detection
    if (input.isMouseDown(MouseButton::Left)) {
        // On mouse press, check if this widget's bounds contain the press position
        // Only allow a widget to start a potential drag if mouse was pressed on it
        if (input.isMousePressed(MouseButton::Left)) {
            // Get bounds for this widget from the last widget bounds (set by the widget before BeginDragDropSource)
            Rect widgetBounds = ctx.getLastWidgetBounds();
            if (widgetBounds.contains(input.mousePos())) {
                s_mousePressPos = input.mousePos();
                s_globalMousePressPos = GetGlobalCursorPos();  // Capture global position
                s_potentialDrag = true;
                s_potentialDragSource = lastWidgetId;
            }
        }
        
        // Only allow drag if THIS widget started the potential drag
        if (s_potentialDrag && s_potentialDragSource == lastWidgetId) {
            // Calculate drag distance
            // For cross-window D&D with real windows, use global coordinates
            // For test stubs (no native handle), use local coords since GetGlobalCursorPos
            // returns the real desktop cursor which doesn't reflect simulated mouse input
            float dragDistSq;
            bool hasNativeHandle = (ctx.window().nativeHandle() != nullptr);
            
            if (hasNativeHandle) {
                Vec2 globalCurrent = GetGlobalCursorPos();
                dragDistSq = (globalCurrent - s_globalMousePressPos).lengthSquared();
            } else {
                // Fallback to local coordinates (test environment)
                dragDistSq = (input.mousePos() - s_mousePressPos).lengthSquared();
            }
            
            if (dragDistSq > 25.0f) { // 5 pixel threshold
                // Start drag - store both local and global coordinates
                s_dragDropState.active = true;
                s_dragDropState.pendingClear = false;  // Reset any pending clear from previous drop
                s_dragDropState.startPos = s_mousePressPos;
                s_dragDropState.currentPos = input.mousePos();
                s_dragDropState.globalStartPos = s_globalMousePressPos;
                s_dragDropState.globalCurrentPos = hasNativeHandle ? GetGlobalCursorPos() : input.mousePos();
                s_dragDropState.payload.sourceWidget = lastWidgetId;
                s_dragDropState.payload.sourceWindow = &ctx.window();  // Track source window
                s_dragDropState.payload.isDelivered = false;  // Reset from any previous drop
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

bool BeginDragDropSource(DragDropFlags flags) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    return fst::BeginDragDropSource(*ctx, flags);
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

void EndDragDropSource(Context& ctx) {
    if (!s_inSourceBlock) return;
    
    const auto& input = ctx.input();
    s_dragDropState.currentPos = input.mousePos();
    // Always update global position - works even if cursor left this window
    s_dragDropState.globalCurrentPos = GetGlobalCursorPos();
    
    // Check for drop (mouse released)
    // Use global state only if we have a native handle (real window)
    // For test stubs, use local input state since GetAsyncKeyState checks real mouse
    bool hasNativeHandle = (ctx.window().nativeHandle() != nullptr);
    bool mouseDown = hasNativeHandle 
        ? IsGlobalMouseButtonDown(&ctx, MouseButton::Left)
        : input.isMouseDown(MouseButton::Left);
    
    if (!mouseDown && s_dragDropState.active) {
        // Do NOT clear here immediately, as targets might be rendered later in the frame.
        // Mark for clearing at end of frame.
        s_dragDropState.pendingClear = true;
    }
    
    s_inSourceBlock = false;
}

void EndDragDropSource() {
    Context* ctx = Context::current();
    if (ctx) fst::EndDragDropSource(*ctx);
    else s_inSourceBlock = false;
}

//=============================================================================
// Target API Implementation
//=============================================================================

bool BeginDragDropTarget(Context& ctx) {
    if (!s_dragDropState.active) return false;
    
    s_inTargetBlock = true;
    
    // Get current widget bounds (from layout)
    s_currentTargetRect = ctx.layout().currentBounds();
    
    // For cross-window D&D, use global cursor converted to this window's local coords
    Vec2 mousePos = GetCursorPosInWindow(ctx.window());
    
    // Check if mouse is over this target
    if (s_currentTargetRect.contains(mousePos) && !ctx.isOccluded(mousePos)) {
        s_dragDropState.hoveredDropTarget = ctx.currentId();
        s_dragDropState.targetWindow = &ctx.window();
        return true;
    }
    
    s_inTargetBlock = false;
    return false;
}

bool BeginDragDropTarget() {
    Context* ctx = Context::current();
    if (!ctx) return false;
    return fst::BeginDragDropTarget(*ctx);
}

bool BeginDragDropTarget(Context& ctx, const Rect& targetRect) {
    if (!s_dragDropState.active) return false;
    
    s_inTargetBlock = true;
    
    s_currentTargetRect = targetRect;
    
    // For cross-window D&D, use global cursor converted to this window's local coords
    Vec2 mousePos = GetCursorPosInWindow(ctx.window());
    
    // Check if mouse is over this target
    if (s_currentTargetRect.contains(mousePos) && !ctx.isOccluded(mousePos)) {
        s_dragDropState.hoveredDropTarget = ctx.currentId();
        s_dragDropState.targetWindow = &ctx.window();
        return true;
    }
    
    s_inTargetBlock = false;
    return false;
}

bool BeginDragDropTarget(const Rect& targetRect) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    return fst::BeginDragDropTarget(*ctx, targetRect);
}

const DragPayload* AcceptDragDropPayload(Context& ctx, const std::string& type, DragDropFlags flags) {
    if (!s_inTargetBlock || !s_dragDropState.active) return nullptr;
    
    // Check type match
    if (s_dragDropState.payload.type != type) {
        return nullptr;
    }

    // Prevent handling if already delivered to another target
    if (s_dragDropState.payload.isDelivered) {
        return nullptr;
    }
    
    s_dragDropState.isOverValidTarget = true;
    
    // Highlight target
    if (!(flags & DragDropFlags_AcceptNoHighlight)) {
        IDrawList& dl = *ctx.activeDrawList();
        const auto& theme = ctx.theme();
        dl.addRect(s_currentTargetRect, theme.colors.primary, 2.0f);
    }
    
    // Check for drop using GLOBAL mouse state (works for cross-window D&D)
    // GetAsyncKeyState checks actual physical button state regardless of window focus
    if (!IsGlobalMouseButtonDown(&ctx, MouseButton::Left)) {
        s_dragDropState.payload.isDelivered = true;
        // IMPORTANT: Clear activeWidget because after D&D reorder, the source widget's
        // ID may have changed (e.g., pushId(index) where index changes). If we don't
        // clear it, isInputCaptured() will return true forever, blocking all other widgets.
        ctx.clearActiveWidget();
        return &s_dragDropState.payload;
    }
    
    return nullptr;
}

const DragPayload* AcceptDragDropPayload(const std::string& type, DragDropFlags flags) {
    Context* ctx = Context::current();
    if (!ctx) return nullptr;
    return fst::AcceptDragDropPayload(*ctx, type, flags);
}

void EndDragDropTarget(Context& ctx) {
    if (!s_inTargetBlock) return;
    
    const auto& input = ctx.input();
    
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
    
    s_inTargetBlock = false;
}

void EndDragDropTarget() {
    Context* ctx = Context::current();
    if (ctx) fst::EndDragDropTarget(*ctx);
    else s_inTargetBlock = false;
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
    s_potentialDrag = false;
    s_potentialDragSource = INVALID_WIDGET_ID;
}

void EndDragDropFrame(Context& ctx) {
    // Render preview at the end of the frame when all potential targets have updated isOverValidTarget
    if (s_dragDropState.active) {
        renderDragPreview(ctx);
    }

    // If pending clear was set (mouse released), and we reached end of frame,
    // we can safely clear the state now.
    if (s_dragDropState.pendingClear) {
        s_dragDropState.clear();
        s_dragDropState.pendingClear = false;
    }
    
    // Reset frame-cumulative state
    s_dragDropState.isOverValidTarget = false;
}

void EndDragDropFrame() {
    Context* ctx = Context::current();
    if (ctx) fst::EndDragDropFrame(*ctx);
}

} // namespace fst
