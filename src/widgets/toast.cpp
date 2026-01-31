/**
 * @file toast.cpp
 * @brief Toast/Notification widget implementation.
 */

#include "fastener/widgets/toast.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/platform/window.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace fst {

//=============================================================================
// Internal Toast State
//=============================================================================

namespace {

/// Animation phase for toast lifecycle
enum class ToastPhase {
    FadeIn,     ///< Animating in
    Visible,    ///< Fully visible, counting down
    FadeOut     ///< Animating out before removal
};

/// Internal toast entry with all state
struct ToastEntry {
    int id;                         ///< Unique identifier
    std::string title;              ///< Optional title (empty if none)
    std::string message;            ///< Message body
    ToastOptions options;           ///< Configuration
    ToastPhase phase = ToastPhase::FadeIn;
    float timeRemaining;            ///< Time until auto-dismiss
    float animationProgress = 0.0f; ///< 0-1 for fade animations
    bool markedForRemoval = false;  ///< Will be removed next frame
    
    ToastEntry(int _id, std::string _title, std::string _msg, const ToastOptions& _opts)
        : id(_id)
        , title(std::move(_title))
        , message(std::move(_msg))
        , options(_opts)
        , timeRemaining(_opts.duration)
    {}
};

/// Thread-local toast queue
thread_local std::vector<ToastEntry> s_toasts;
thread_local int s_nextToastId = 1;

/// Animation constants
constexpr float FADE_DURATION = 0.2f;       // Seconds for fade in/out
constexpr float TOAST_PADDING = 12.0f;
constexpr float CLOSE_BUTTON_SIZE = 16.0f;
constexpr float ICON_SIZE = 20.0f;
constexpr float TITLE_MARGIN = 4.0f;

/// Get color for toast type from theme
Color getToastColor(const Theme& theme, ToastType type) {
    switch (type) {
        case ToastType::Success: return theme.colors.success;
        case ToastType::Warning: return theme.colors.warning;
        case ToastType::Error:   return theme.colors.error;
        case ToastType::Info:
        default:                 return theme.colors.info;
    }
}

/// Get icon character for toast type
const char* getToastIcon(ToastType type) {
    switch (type) {
        case ToastType::Success: return "✓";
        case ToastType::Warning: return "⚠";
        case ToastType::Error:   return "✕";
        case ToastType::Info:
        default:                 return "ℹ";
    }
}

/// Calculate toast position based on container options and index
Vec2 calculateToastPosition(const ToastContainerOptions& containerOpts,
                            float windowW, float windowH,
                            float toastW, float toastH,
                            int visibleIndex, float animProgress) {
    float margin = containerOpts.margin;
    float spacing = containerOpts.spacing;
    float stackOffset = static_cast<float>(visibleIndex) * (toastH + spacing);
    
    // Slide animation offset (slides in from edge)
    float slideOffset = (1.0f - animProgress) * (toastW + margin);
    
    float x = 0, y = 0;
    
    switch (containerOpts.position) {
        case ToastPosition::TopRight:
            x = windowW - margin - toastW + slideOffset;
            y = margin + stackOffset;
            break;
        case ToastPosition::TopLeft:
            x = margin - slideOffset;
            y = margin + stackOffset;
            break;
        case ToastPosition::BottomRight:
            x = windowW - margin - toastW + slideOffset;
            y = windowH - margin - toastH - stackOffset;
            break;
        case ToastPosition::BottomLeft:
            x = margin - slideOffset;
            y = windowH - margin - toastH - stackOffset;
            break;
        case ToastPosition::TopCenter:
            x = (windowW - toastW) * 0.5f;
            y = margin + stackOffset - slideOffset + toastW;  // Slide down from top
            break;
        case ToastPosition::BottomCenter:
            x = (windowW - toastW) * 0.5f;
            y = windowH - margin - toastH - stackOffset + slideOffset;
            break;
    }
    
    return Vec2(x, y);
}

/// Render a single toast
void renderToast(Context& ctx, ToastEntry& toast, const Vec2& position, float alpha) {
    auto wc = WidgetContext::make(ctx);
    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    
    if (!font) return;
    
    // Calculate dimensions
    float toastW = toast.options.width;
    float lineHeight = font->lineHeight();
    float contentHeight = lineHeight;  // Message
    if (!toast.title.empty()) {
        contentHeight += lineHeight + TITLE_MARGIN;  // Title + gap
    }
    float toastH = contentHeight + TOAST_PADDING * 2;
    
    Rect bounds(position.x, position.y, toastW, toastH);
    
    // Register toast as a global occlusion rect (blocks all layers)
    // This prevents click-through even for overlay widgets.
    ctx.addGlobalOcclusionRect(bounds);
    
    // Get type color
    Color typeColor = getToastColor(theme, toast.options.type);
    
    // Apply alpha for fade animation
    auto applyAlpha = [alpha](Color c) {
        return c.withAlpha(static_cast<uint8_t>(c.a * alpha));
    };
    
    // Switch to overlay layer
    dl.setLayer(DrawLayer::Overlay);
    
    // Draw shadow
    Rect shadowRect(bounds.x() + 2, bounds.y() + 2, bounds.width(), bounds.height());
    dl.addRectFilled(shadowRect, applyAlpha(theme.colors.shadow), theme.metrics.borderRadius);
    
    // Draw background
    dl.addRectFilled(bounds, applyAlpha(theme.colors.popupBackground), theme.metrics.borderRadius);
    
    // Draw left accent bar
    float accentWidth = 4.0f;
    Rect accentRect(bounds.x(), bounds.y(), accentWidth, bounds.height());
    dl.addRectFilled(accentRect, applyAlpha(typeColor), theme.metrics.borderRadius);
    
    // Draw border
    dl.addRect(bounds, applyAlpha(theme.colors.border), theme.metrics.borderRadius);
    
    // Content start position
    float contentX = bounds.x() + TOAST_PADDING + accentWidth;
    float contentY = bounds.y() + TOAST_PADDING;
    float availableWidth = toastW - TOAST_PADDING * 2 - accentWidth;
    
    // Draw icon
    Vec2 iconPos(contentX, contentY + (lineHeight - ICON_SIZE) * 0.5f);
    dl.addText(font, iconPos, getToastIcon(toast.options.type), applyAlpha(typeColor));
    
    float textX = contentX + ICON_SIZE + 8.0f;
    float textWidth = availableWidth - ICON_SIZE - 8.0f;
    
    // Reserve space for close button
    if (toast.options.dismissible) {
        textWidth -= CLOSE_BUTTON_SIZE + 8.0f;
    }
    
    // Draw title (if present)
    float messageY = contentY;
    if (!toast.title.empty()) {
        dl.addText(font, Vec2(textX, contentY), toast.title, applyAlpha(theme.colors.text));
        messageY = contentY + lineHeight + TITLE_MARGIN;
    }
    
    // Draw message
    dl.addText(font, Vec2(textX, messageY), toast.message, applyAlpha(theme.colors.textSecondary));
    
    // Draw close button if dismissible
    if (toast.options.dismissible) {
        float closeX = bounds.right() - TOAST_PADDING - CLOSE_BUTTON_SIZE;
        float closeY = bounds.y() + (bounds.height() - CLOSE_BUTTON_SIZE) * 0.5f;
        Rect closeRect(closeX, closeY, CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE);
        
        // Handle close button interaction first so it isn't blocked by the toast body
        WidgetId closeId = ctx.makeId(toast.id);
        WidgetInteraction interaction = handleWidgetInteraction(ctx, closeId, closeRect, true, true, true);
        WidgetState state = getWidgetState(ctx, closeId);
        
        if (interaction.clicked) {
            toast.phase = ToastPhase::FadeOut;
            toast.animationProgress = 1.0f;
            // Consume the mouse to prevent click-through to widgets behind
            ctx.input().consumeMouse();
        }
        
        // Draw X
        Color xColor = state.hovered 
            ? applyAlpha(theme.colors.text)
            : applyAlpha(theme.colors.textSecondary);
        
        float xPad = 4.0f;
        dl.addLine(
            Vec2(closeRect.x() + xPad, closeRect.y() + xPad),
            Vec2(closeRect.right() - xPad, closeRect.bottom() - xPad),
            xColor, 1.5f
        );
        dl.addLine(
            Vec2(closeRect.right() - xPad, closeRect.y() + xPad),
            Vec2(closeRect.x() + xPad, closeRect.bottom() - xPad),
            xColor, 1.5f
        );
    }

    // Consume mouse when hovering the toast so widgets rendered later can't steal the click.
    if (bounds.contains(ctx.input().mousePos())) {
        ctx.input().consumeMouse();
    }

    // Block input on the entire toast area to prevent click-through
    // This ensures widgets behind the toast don't receive clicks
    WidgetId toastId = ctx.makeId(toast.id * 1000);  // Unique ID for toast body
    handleWidgetInteraction(ctx, toastId, bounds, true, true, true);  // ignoreOcclusion=true for overlay
    
    // Restore layer
    dl.setLayer(DrawLayer::Default);
}

} // anonymous namespace

//=============================================================================
// Public API Implementation
//=============================================================================

int ShowToast(Context& ctx, std::string_view message, const ToastOptions& options) {
    (void)ctx;  // Not currently used but kept for API consistency
    int id = s_nextToastId++;
    s_toasts.emplace_back(id, "", std::string(message), options);
    return id;
}

int ShowToast(Context& ctx, std::string_view title, std::string_view message, const ToastOptions& options) {
    (void)ctx;
    int id = s_nextToastId++;
    s_toasts.emplace_back(id, std::string(title), std::string(message), options);
    return id;
}

void RenderToasts(Context& ctx, const ToastContainerOptions& options) {
    if (s_toasts.empty()) return;
    
    Font* font = ctx.font();
    
    if (!font) return;
    
    float lineHeight = font->lineHeight();
    float windowW = static_cast<float>(ctx.window().width());
    float windowH = static_cast<float>(ctx.window().height());
    
    // EARLY CONSUMPTION: Consume mouse if it's over ANY toast BEFORE processing
    // This prevents widgets rendered earlier in the frame from getting clicks
    Vec2 mousePos = ctx.input().mousePos();
    int tempVisibleCount = 0;
    for (const auto& toast : s_toasts) {
        if (tempVisibleCount >= options.maxVisible) break;
        
        float contentHeight = lineHeight;
        if (!toast.title.empty()) {
            contentHeight += lineHeight + TITLE_MARGIN;
        }
        float toastH = contentHeight + TOAST_PADDING * 2;
        
        Vec2 pos = calculateToastPosition(
            options, windowW, windowH,
            toast.options.width, toastH,
            tempVisibleCount, toast.animationProgress
        );
        
        Rect toastBounds(pos.x, pos.y, toast.options.width, toastH);
        if (toastBounds.contains(mousePos)) {
            ctx.input().consumeMouse();
            break;
        }
        tempVisibleCount++;
    }
    
    float deltaTime = ctx.deltaTime();

    // Process toast animations and timers
    for (auto& toast : s_toasts) {
        switch (toast.phase) {
            case ToastPhase::FadeIn:
                toast.animationProgress += deltaTime / FADE_DURATION;
                if (toast.animationProgress >= 1.0f) {
                    toast.animationProgress = 1.0f;
                    toast.phase = ToastPhase::Visible;
                }
                break;
                
            case ToastPhase::Visible:
                // Count down timer (if not infinite)
                if (toast.options.duration > 0.0f) {
                    toast.timeRemaining -= deltaTime;
                    if (toast.timeRemaining <= 0.0f) {
                        toast.phase = ToastPhase::FadeOut;
                        toast.animationProgress = 1.0f;
                    }
                }
                break;
                
            case ToastPhase::FadeOut:
                toast.animationProgress -= deltaTime / FADE_DURATION;
                if (toast.animationProgress <= 0.0f) {
                    toast.markedForRemoval = true;
                }
                break;
        }
    }
    
    // Remove dismissed toasts
    s_toasts.erase(
        std::remove_if(s_toasts.begin(), s_toasts.end(),
                       [](const ToastEntry& t) { return t.markedForRemoval; }),
        s_toasts.end()
    );
    
    // Render visible toasts (limited by maxVisible)
    int visibleCount = 0;

    for (auto& toast : s_toasts) {
        if (visibleCount >= options.maxVisible) break;
        
        // Calculate height for positioning
        float contentHeight = lineHeight;
        if (!toast.title.empty()) {
            contentHeight += lineHeight + TITLE_MARGIN;
        }
        float toastH = contentHeight + TOAST_PADDING * 2;
        
        // Calculate position
        Vec2 pos = calculateToastPosition(
            options, windowW, windowH,
            toast.options.width, toastH,
            visibleCount, toast.animationProgress
        );
        
        // Calculate alpha
        float alpha = toast.animationProgress;
        
        // Render
        renderToast(ctx, toast, pos, alpha);
        
        visibleCount++;
    }
}

void DismissToast(int toastId) {
    for (auto& toast : s_toasts) {
        if (toast.id == toastId && toast.phase != ToastPhase::FadeOut) {
            toast.phase = ToastPhase::FadeOut;
            toast.animationProgress = 1.0f;
            break;
        }
    }
}

void DismissAllToasts() {
    s_toasts.clear();
}

//=============================================================================
// Internal API (for testing)
//=============================================================================

namespace internal {

int getToastCount() {
    return static_cast<int>(s_toasts.size());
}

} // namespace internal

} // namespace fst
