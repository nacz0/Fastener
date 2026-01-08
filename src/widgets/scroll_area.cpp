/**
 * @file scroll_area.cpp
 * @brief Scrollable container widget implementation.
 */

#include "fastener/widgets/scroll_area.h"
#include "fastener/widgets/menu.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/theme.h"
#include <algorithm>

namespace fst {

//=============================================================================
// ScrollArea Implementation
//=============================================================================

ScrollArea::ScrollArea() = default;
ScrollArea::~ScrollArea() = default;

/**
 * @brief Set the scroll offset directly.
 * @param offset New scroll offset (x, y)
 */
void ScrollArea::setScrollOffset(const Vec2& offset) {
    m_scrollOffset = offset;
}

void ScrollArea::scrollTo(const Rect& area) {
    // TODO: Implement logic to make a specific sub-rect visible
}

void ScrollArea::render(const std::string& id, const Rect& bounds, 
                        std::function<void(const Rect& viewport)> contentRenderer,
                        const ScrollAreaOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return;

    ctx->pushId(id.c_str());

    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();

    // Background & Border
    dl.addRectFilled(bounds, theme.colors.windowBackground, options.style.borderRadius);
    dl.addRect(bounds, theme.colors.border, options.style.borderRadius);

    float sbSize = options.scrollbarWidth;
    bool showV = options.showVertical && (!options.autoHide || m_contentSize.y > bounds.height());
    bool showH = options.showHorizontal && (!options.autoHide || m_contentSize.x > bounds.width() - (showV ? sbSize : 0));
    
    // Re-check V if H added a scrollbar
    if (showH && !showV && options.showVertical && options.autoHide && m_contentSize.y > bounds.height() - sbSize) {
        showV = true;
    }

    Rect viewport = bounds;
    if (showV) viewport.size.x -= sbSize;
    if (showH) viewport.size.y -= sbSize;

    clampScroll(viewport);
    handleInteraction(id, bounds, viewport, options);

    // Content Rendering
    dl.pushClipRect(viewport);
    contentRenderer(viewport);
    dl.popClipRect();

    // Scrollbar Tracks and Thumbs
    if (showV) {
        Rect track(bounds.right() - sbSize, bounds.y(), sbSize, viewport.height());
        Color trackColor = theme.colors.scrollbarTrack;
        dl.addRectFilled(track, trackColor); // Track background

        float thumbHeight = std::max(options.minThumbSize, (viewport.height() / std::max(1.0f, m_contentSize.y)) * viewport.height());
        float maxScroll = std::max(0.1f, m_contentSize.y - viewport.height());
        float thumbY = track.y() + (m_scrollOffset.y / maxScroll) * (track.height() - thumbHeight);
        
        Rect thumb(track.x() + 2, thumbY, track.width() - 4, thumbHeight);
        Color thumbColor;
        if (m_draggingV) thumbColor = theme.colors.scrollbarThumbHover; // or primary
        else if (track.contains(ctx->input().mousePos())) thumbColor = theme.colors.scrollbarThumbHover;
        else thumbColor = theme.colors.scrollbarThumb;
        
        dl.addRectFilled(thumb, thumbColor, (sbSize - 4) / 2);
    }

    if (showH) {
        Rect track(bounds.x(), bounds.bottom() - sbSize, viewport.width(), sbSize);
        Color trackColor = theme.colors.scrollbarTrack;
        dl.addRectFilled(track, trackColor);

        float thumbWidth = std::max(options.minThumbSize, (viewport.width() / std::max(1.0f, m_contentSize.x)) * viewport.width());
        float maxScroll = std::max(0.1f, m_contentSize.x - viewport.width());
        float thumbX = track.x() + (m_scrollOffset.x / maxScroll) * (track.width() - thumbWidth);

        Rect thumb(thumbX, track.y() + 2, thumbWidth, track.height() - 4);
        Color thumbColor;
        if (m_draggingH) thumbColor = theme.colors.scrollbarThumbHover;
        else if (track.contains(ctx->input().mousePos())) thumbColor = theme.colors.scrollbarThumbHover;
        else thumbColor = theme.colors.scrollbarThumb;

        dl.addRectFilled(thumb, thumbColor, (sbSize - 4) / 2);
    }
    
    if (showV && showH) {
        dl.addRectFilled(Rect(bounds.right() - sbSize, bounds.bottom() - sbSize, sbSize, sbSize), Color(0, 0, 0, 15));
    }

    ctx->popId();
}

void ScrollArea::handleInteraction(const std::string& id, const Rect& bounds, const Rect& viewport, const ScrollAreaOptions& options) {
    Context* ctx = Context::current();
    InputState& input = ctx->input();
    Vec2 mp = input.mousePos();
    float sbSize = options.scrollbarWidth;

    bool showV = options.showVertical && (!options.autoHide || m_contentSize.y > bounds.height());
    bool showH = options.showHorizontal && (!options.autoHide || m_contentSize.x > bounds.width() - (showV ? sbSize : 0));

    // Wheel Scrolling
    if (bounds.contains(mp) && !ctx->isOccluded(mp)) {
        Vec2 delta = input.scrollDelta();
        if (input.modifiers().shift || !showV) {
            m_scrollOffset.x -= delta.y * 30.0f; 
        } else {
            m_scrollOffset.y -= delta.y * 30.0f; 
        }
        m_scrollOffset.x -= delta.x * 30.0f; 
    }

    // Vertical Dragging
    if (showV) {
        Rect track(bounds.right() - sbSize, bounds.y(), sbSize, viewport.height());
        float thumbHeight = std::max(options.minThumbSize, (viewport.height() / std::max(1.0f, m_contentSize.y)) * viewport.height());
        float maxScroll = std::max(0.1f, m_contentSize.y - viewport.height());
        
        if (input.isMousePressed(MouseButton::Left) && track.contains(mp) && !ctx->isOccluded(mp)) {
            m_draggingV = true;
            m_dragStartPos = mp.y;
            m_dragStartOffset = m_scrollOffset.y;
        }

        if (m_draggingV) {
            if (input.isMouseDown(MouseButton::Left)) {
                float dy = mp.y - m_dragStartPos;
                float ratio = maxScroll / std::max(0.1f, track.height() - thumbHeight);
                m_scrollOffset.y = m_dragStartOffset + dy * ratio;
            } else {
                m_draggingV = false;
            }
        }
    }

    // Horizontal Dragging
    if (showH) {
        Rect track(bounds.x(), bounds.bottom() - sbSize, viewport.width(), sbSize);
        float thumbWidth = std::max(options.minThumbSize, (viewport.width() / std::max(1.0f, m_contentSize.x)) * viewport.width());
        float maxScroll = std::max(0.1f, m_contentSize.x - viewport.width());

        if (input.isMousePressed(MouseButton::Left) && track.contains(mp) && !ctx->isOccluded(mp)) {
            m_draggingH = true;
            m_dragStartPos = mp.x;
            m_dragStartOffset = m_scrollOffset.x;
        }

        if (m_draggingH) {
            if (input.isMouseDown(MouseButton::Left)) {
                float dx = mp.x - m_dragStartPos;
                float ratio = maxScroll / std::max(0.1f, track.width() - thumbWidth);
                m_scrollOffset.x = m_dragStartOffset + dx * ratio;
            } else {
                m_draggingH = false;
            }
        }
    }

    clampScroll(viewport);
}

void ScrollArea::clampScroll(const Rect& viewport) {
    m_scrollOffset.y = std::clamp(m_scrollOffset.y, 0.0f, std::max(0.0f, m_contentSize.y - viewport.height()));
    m_scrollOffset.x = std::clamp(m_scrollOffset.x, 0.0f, std::max(0.0f, m_contentSize.x - viewport.width()));
}

} // namespace fst
