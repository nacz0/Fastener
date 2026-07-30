#include "fastener/ui/drag_drop.h"
#include "drag_drop_internal.h"
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
#include <cstring>

namespace fst {

// Legacy no-context queries need a routing hint after Context::endFrame() pops
// the current context. Drag state itself remains owned by Context.
static thread_local Context* s_legacyDragContext = nullptr;

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

static void renderDragPreview(Context& ctx, const detail::DragDropContextState& state) {
    if (!state.active) return;
    
    IDrawList& dl = *ctx.activeDrawList();
    const auto& theme = ctx.theme();
    
    // Switch to overlay layer for preview
    DrawLayer prevLayer = dl.currentLayer();
    dl.setLayer(DrawLayer::Overlay);
    
    // For cross-window D&D: use global cursor converted to this window's local coordinates
    // This ensures preview appears at cursor even if drag started in different window
    Vec2 pos = GetCursorPosInWindow(ctx.window()) + Vec2(15, 15);
    
    std::string text = state.payload.displayText;
    if (text.empty()) {
        text = "[" + state.payload.type + "]";
    }
    
    Font* font = ctx.font();
    float padding = 8.0f;
    Vec2 textSize = font ? font->measureText(text) : Vec2(80, 14);
    
    Rect bgRect(pos.x, pos.y, textSize.x + padding * 2, textSize.y + padding * 2);
    
    // Background
    dl.addRectFilled(bgRect, theme.colors.panelBackground.withAlpha(0.9f), 4.0f);
    dl.addRect(bgRect, state.isOverValidTarget ?
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
    auto& state = detail::dragDropState(ctx);
    state.inSourceBlock = true;
    
    const auto& input = ctx.input();
    
    // Start new drag on mouse down + drag threshold
    // BeginDragDropSource must be called AFTER the widget it applies to
    WidgetId lastWidgetId = ctx.getLastWidgetId();
    if (lastWidgetId == INVALID_WIDGET_ID) {
        // Fallback to hovered if last widget not set (but this is risky)
        lastWidgetId = ctx.getHoveredWidget();
        if (lastWidgetId == INVALID_WIDGET_ID) {
            state.inSourceBlock = false;
            return false;
        }
    }
    
    // Check if THIS widget is the active source
    if (state.active) {
        if (state.payload.sourceWidget == lastWidgetId) {
            state.currentSourceWidget = state.payload.sourceWidget;
            return true;
        }
        state.inSourceBlock = false;
        return false; // Another widget is dragging
    }
    
    // Drag initiation logic using potential drag state owned by this Context
    // NOTE: Use per-window input state here to stay consistent with other widgets
    // OS-global input is only used for cross-window drop detection.
    if (input.isMouseDown(MouseButton::Left)) {
        // On mouse press, check if this widget's bounds contain the press position
        // Only allow a widget to start a potential drag if mouse was pressed on it
        if (input.isMousePressed(MouseButton::Left)) {
            // Get bounds for this widget from the last widget bounds (set by the widget before BeginDragDropSource)
            Rect widgetBounds = ctx.getLastWidgetBounds();
            if (widgetBounds.contains(input.mousePos())) {
                state.mousePressPos = input.mousePos();
                state.globalMousePressPos = GetGlobalCursorPos();  // Capture global position
                state.potentialDrag = true;
                state.potentialDragSource = lastWidgetId;
                s_legacyDragContext = &ctx;
            }
        }
        
        // Only allow drag if THIS widget started the potential drag
        if (state.potentialDrag && state.potentialDragSource == lastWidgetId) {
            // Calculate drag distance
            // For cross-window D&D with real windows, use global coordinates
            // For test stubs (no native handle), use local coords since GetGlobalCursorPos
            // returns the real desktop cursor which doesn't reflect simulated mouse input
            float dragDistSq;
            bool hasNativeHandle = (ctx.window().nativeHandle() != nullptr);
            
            if (hasNativeHandle) {
                Vec2 globalCurrent = GetGlobalCursorPos();
                dragDistSq = (globalCurrent - state.globalMousePressPos).lengthSquared();
            } else {
                // Fallback to local coordinates (test environment)
                dragDistSq = (input.mousePos() - state.mousePressPos).lengthSquared();
            }
            
            if (dragDistSq > 25.0f) { // 5 pixel threshold
                // Start drag - store both local and global coordinates
                state.active = true;
                state.pendingClear = false;  // Reset any pending clear from previous drop
                state.startPos = state.mousePressPos;
                state.currentPos = input.mousePos();
                state.globalStartPos = state.globalMousePressPos;
                state.globalCurrentPos = hasNativeHandle ? GetGlobalCursorPos() : input.mousePos();
                state.payload.sourceWidget = lastWidgetId;
                state.payload.sourceWindow = &ctx.window();  // Track source window
                state.payload.isDelivered = false;  // Reset from any previous drop
                state.currentSourceWidget = lastWidgetId;
                state.potentialDrag = false;
                state.potentialDragSource = INVALID_WIDGET_ID;
                return true;
            }
        }
    } else {
        // Mouse released, cancel potential drag
        state.potentialDrag = false;
        state.potentialDragSource = INVALID_WIDGET_ID;
        state.inSourceBlock = false;
        return false;
    }
    
    state.inSourceBlock = false;
    return false;
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) 
#endif

bool BeginDragDropSource(DragDropFlags flags) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    return fst::BeginDragDropSource(*ctx, flags);
}

bool SetDragDropPayload(Context& ctx, const std::string& type, const void* data, size_t size) {
    auto& state = detail::dragDropState(ctx);
    if (!state.inSourceBlock || !state.active) return false;
    
    state.payload.type = type;
    state.payload.data.resize(size);
    if (size > 0 && data) {
        memcpy(state.payload.data.data(), data, size);
    }
    
    return true;
}

bool SetDragDropPayload(const std::string& type, const void* data, size_t size) {
    Context* ctx = Context::current();
    return ctx ? fst::SetDragDropPayload(*ctx, type, data, size) : false;
}

void SetDragDropDisplayText(Context& ctx, const std::string& text) {
    auto& state = detail::dragDropState(ctx);
    if (!state.inSourceBlock) return;
    state.payload.displayText = text;
}

void SetDragDropDisplayText(const std::string& text) {
    Context* ctx = Context::current();
    if (ctx) fst::SetDragDropDisplayText(*ctx, text);
}

void EndDragDropSource(Context& ctx) {
    auto& state = detail::dragDropState(ctx);
    if (!state.inSourceBlock) return;
    
    const auto& input = ctx.input();
    state.currentPos = input.mousePos();
    // Always update global position - works even if cursor left this window
    state.globalCurrentPos = GetGlobalCursorPos();
    
    // Check for drop (mouse released)
    // Use global state only if we have a native handle (real window)
    // For test stubs, use local input state since GetAsyncKeyState checks real mouse
    bool hasNativeHandle = (ctx.window().nativeHandle() != nullptr);
    bool mouseDown = hasNativeHandle 
        ? IsGlobalMouseButtonDown(&ctx, MouseButton::Left)
        : input.isMouseDown(MouseButton::Left);
    
    if (!mouseDown && state.active) {
        // Do NOT clear here immediately, as targets might be rendered later in the frame.
        // Mark for clearing at end of frame.
        state.pendingClear = true;
    }
    
    state.inSourceBlock = false;
}

void EndDragDropSource() {
    Context* ctx = Context::current();
    if (ctx) fst::EndDragDropSource(*ctx);
}

//=============================================================================
// Target API Implementation
//=============================================================================

bool BeginDragDropTarget(Context& ctx) {
    auto& state = detail::dragDropState(ctx);
    if (!state.active) return false;
    
    state.inTargetBlock = true;
    
    // Get current widget bounds (from layout)
    state.currentTargetRect = ctx.layout().currentBounds();
    
    // For cross-window D&D, use global cursor converted to this window's local coords
    Vec2 mousePos = GetCursorPosInWindow(ctx.window());
    
    // Check if mouse is over this target
    if (state.currentTargetRect.contains(mousePos) && !ctx.isOccluded(mousePos)) {
        state.hoveredDropTarget = ctx.currentId();
        state.targetWindow = &ctx.window();
        return true;
    }
    
    state.inTargetBlock = false;
    return false;
}

bool BeginDragDropTarget() {
    Context* ctx = Context::current();
    if (!ctx) return false;
    return fst::BeginDragDropTarget(*ctx);
}

bool BeginDragDropTarget(Context& ctx, const Rect& targetRect) {
    auto& state = detail::dragDropState(ctx);
    if (!state.active) return false;
    
    state.inTargetBlock = true;
    
    state.currentTargetRect = targetRect;
    
    // For cross-window D&D, use global cursor converted to this window's local coords
    Vec2 mousePos = GetCursorPosInWindow(ctx.window());
    
    // Check if mouse is over this target
    if (state.currentTargetRect.contains(mousePos) && !ctx.isOccluded(mousePos)) {
        state.hoveredDropTarget = ctx.currentId();
        state.targetWindow = &ctx.window();
        return true;
    }
    
    state.inTargetBlock = false;
    return false;
}

bool BeginDragDropTarget(const Rect& targetRect) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    return fst::BeginDragDropTarget(*ctx, targetRect);
}

const DragPayload* AcceptDragDropPayload(Context& ctx, const std::string& type, DragDropFlags flags) {
    auto& state = detail::dragDropState(ctx);
    if (!state.inTargetBlock || !state.active) return nullptr;
    
    // Check type match
    if (state.payload.type != type) {
        return nullptr;
    }

    // Prevent handling if already delivered to another target
    if (state.payload.isDelivered) {
        return nullptr;
    }
    
    state.isOverValidTarget = true;
    
    // Highlight target
    if (!(flags & DragDropFlags_AcceptNoHighlight)) {
        IDrawList& dl = *ctx.activeDrawList();
        const auto& theme = ctx.theme();
        dl.addRect(state.currentTargetRect, theme.colors.primary, 2.0f);
    }
    
    // Real windows need OS-global state for cross-window drops. Injected and
    // headless windows must use their own input state; consulting the physical
    // desktop mouse would make behavior nondeterministic and untestable.
    const bool hasNativeHandle = ctx.window().nativeHandle() != nullptr;
    const bool mouseDown = hasNativeHandle
        ? IsGlobalMouseButtonDown(&ctx, MouseButton::Left)
        : ctx.input().isMouseDown(MouseButton::Left);
    if (!mouseDown) {
        state.payload.isDelivered = true;
        // IMPORTANT: Clear activeWidget because after D&D reorder, the source widget's
        // ID may have changed (e.g., pushId(index) where index changes). If we don't
        // clear it, isInputCaptured() will return true forever, blocking all other widgets.
        ctx.clearActiveWidget();
        return &state.payload;
    }
    
    return nullptr;
}

const DragPayload* AcceptDragDropPayload(const std::string& type, DragDropFlags flags) {
    Context* ctx = Context::current();
    if (!ctx) return nullptr;
    return fst::AcceptDragDropPayload(*ctx, type, flags);
}

void EndDragDropTarget(Context& ctx) {
    auto& state = detail::dragDropState(ctx);
    if (!state.inTargetBlock) return;
    
    const auto& input = ctx.input();
    
    // Clear state if drop occurred
    if (state.payload.isDelivered) {
        state.clear();
    }
    // Clear state if mouse released outside valid target
    else if (input.isMouseReleased(MouseButton::Left)) {
        if (!state.isOverValidTarget) {
            state.clear();
        }
    }
    
    state.inTargetBlock = false;
}

void EndDragDropTarget() {
    Context* ctx = Context::current();
    if (ctx) fst::EndDragDropTarget(*ctx);
}

//=============================================================================
// Query API Implementation
//=============================================================================

bool IsDragDropActive(const Context& ctx) {
    return detail::dragDropState(ctx).active;
}

bool IsDragDropActive() {
    Context* ctx = Context::current();
    if (!ctx) ctx = s_legacyDragContext;
    return ctx ? fst::IsDragDropActive(*ctx) : false;
}

const DragPayload* GetDragDropPayload(const Context& ctx) {
    const auto& state = detail::dragDropState(ctx);
    return state.active ? &state.payload : nullptr;
}

const DragPayload* GetDragDropPayload() {
    Context* ctx = Context::current();
    if (!ctx) ctx = s_legacyDragContext;
    return ctx ? fst::GetDragDropPayload(*ctx) : nullptr;
}

void CancelDragDrop(Context& ctx) {
    detail::dragDropState(ctx).clear();
    if (s_legacyDragContext == &ctx) {
        s_legacyDragContext = nullptr;
    }
}

void CancelDragDrop() {
    Context* ctx = Context::current();
    if (!ctx) ctx = s_legacyDragContext;
    if (ctx) fst::CancelDragDrop(*ctx);
}

void EndDragDropFrame(Context& ctx) {
    auto& state = detail::dragDropState(ctx);

    // Render preview at the end of the frame when all potential targets have updated isOverValidTarget
    if (state.active) {
        renderDragPreview(ctx, state);
    }

    // If pending clear was set (mouse released), and we reached end of frame,
    // we can safely clear the state now.
    if (state.pendingClear) {
        state.clear();
        if (s_legacyDragContext == &ctx) {
            s_legacyDragContext = nullptr;
        }
    }
    
    // Reset frame-cumulative state
    state.isOverValidTarget = false;
}

void EndDragDropFrame() {
    Context* ctx = Context::current();
    if (ctx) fst::EndDragDropFrame(*ctx);
}

namespace detail {

void CancelDragDropForContext(Context& ctx) {
    if (s_legacyDragContext == &ctx) {
        s_legacyDragContext = nullptr;
    }
}

} // namespace detail

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace fst
