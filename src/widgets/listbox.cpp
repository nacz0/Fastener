/**
 * @file listbox.cpp
 * @brief Scrollable list selection widget implementation.
 */

#include "fastener/widgets/listbox.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <algorithm>
#include <unordered_map>

namespace fst {

//=============================================================================
// Listbox State
//=============================================================================

/** @brief Per-listbox persistent state for scroll management. */
struct ListboxState {
    float scrollOffset = 0.0f;
    int hoveredIndex = -1;
};

static std::unordered_map<WidgetId, ListboxState> s_listboxStates;

//=============================================================================
// Listbox Implementation
//=============================================================================

bool Listbox(Context& ctx, std::string_view label, int& selectedIndex, 
             const std::vector<std::string>& items,
             const ListboxOptions& options) {
    auto wc = WidgetContext::make(ctx);

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = ctx.input();

    WidgetId id = ctx.makeId(label);
    ListboxState& state = s_listboxStates[id];

    // Calculate dimensions
    float width = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : options.height;
    float itemHeight = options.itemHeight > 0 
        ? options.itemHeight 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);
    float labelWidth = 0;

    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }

    // Allocate bounds
    Rect bounds = allocateWidgetBounds(ctx, options.style, labelWidth + width, height);
    Rect boxBounds(bounds.x() + labelWidth, bounds.y(), width, height);

    // Draw label
    if (font && !label.empty()) {
        Vec2 labelPos(bounds.x(), bounds.y() + theme.metrics.paddingSmall);
        Color labelColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, labelPos, label, labelColor);
    }

    // Draw listbox background
    float radius = theme.metrics.borderRadiusSmall;
    Color bgColor = options.disabled 
        ? theme.colors.inputBackground.withAlpha(0.5f) 
        : theme.colors.inputBackground;
    dl.addRectFilled(boxBounds, bgColor, radius);
    dl.addRect(boxBounds, theme.colors.inputBorder, radius);

    // Calculate scrollbar visibility
    float totalContentHeight = itemHeight * (float)items.size();
    bool needsScrollbar = totalContentHeight > boxBounds.height();
    float scrollbarWidth = needsScrollbar ? 10.0f : 0.0f;
    Rect contentArea(boxBounds.x(), boxBounds.y(), boxBounds.width() - scrollbarWidth, boxBounds.height());

    float maxScroll = std::max(0.0f, totalContentHeight - boxBounds.height());

    // Handle scrollbar interaction
    if (needsScrollbar && !options.disabled) {
        Rect track(boxBounds.right() - scrollbarWidth, boxBounds.y(), scrollbarWidth, boxBounds.height());
        std::string scrollerLabel = std::string(label) + "_scroller";
        WidgetId scrollbarId = ctx.makeId(scrollerLabel);
        
        // Handle scrollbar dragging
        WidgetInteraction scrollInteraction = handleWidgetInteraction(ctx, scrollbarId, track, true);
        
        if (scrollInteraction.dragging || (scrollInteraction.clicked && track.contains(input.mousePos()))) {
             float thumbHeight = std::max(20.0f, (boxBounds.height() / totalContentHeight) * boxBounds.height());
             float effectiveTrackHeight = track.height() - thumbHeight;
             
             if (effectiveTrackHeight > 0) {
                 float mouseRelY = input.mousePos().y - track.y() - thumbHeight * 0.5f;
                 float t = std::clamp(mouseRelY / effectiveTrackHeight, 0.0f, 1.0f);
                 state.scrollOffset = t * maxScroll;
             }
        }
    }

    // Handle mouse wheel scroll
    if (boxBounds.contains(input.mousePos()) && !options.disabled && !ctx.isOccluded(input.mousePos())) {
        state.scrollOffset -= input.scrollDelta().y * itemHeight;
        state.scrollOffset = std::clamp(state.scrollOffset, 0.0f, maxScroll);
    }

    bool changed = false;

    // Draw items
    dl.pushClipRect(contentArea);
    
    for (int i = 0; i < (int)items.size(); ++i) {
        float y = boxBounds.y() + i * itemHeight - state.scrollOffset;
        
        // Skip items outside visible area
        if (y + itemHeight < boxBounds.y() || y > boxBounds.bottom()) continue;

        Rect itemRect(contentArea.x(), y, contentArea.width(), itemHeight);
        
        // Handle hover
        bool isHovered = itemRect.contains(input.mousePos()) && 
                         contentArea.contains(input.mousePos()) && 
                         !options.disabled && 
                         !ctx.isOccluded(input.mousePos());
        if (isHovered) {
            state.hoveredIndex = i;
        }

        // Handle click
        if (isHovered && input.isMousePressed(MouseButton::Left)) {
            if (selectedIndex != i) {
                selectedIndex = i;
                changed = true;
            }
        }

        // Draw item background
        if (i == selectedIndex) {
            dl.addRectFilled(itemRect, theme.colors.selection);
        } else if (isHovered) {
            dl.addRectFilled(itemRect, theme.colors.selection.withAlpha((uint8_t)100));
        }

        // Draw item text
        if (font) {
            Vec2 textPos(
                itemRect.x() + theme.metrics.paddingSmall,
                itemRect.y() + (itemHeight - font->lineHeight()) * 0.5f
            );
            Color textColor = (i == selectedIndex) 
                ? theme.colors.selectionText 
                : (options.disabled ? theme.colors.textDisabled : theme.colors.text);
            dl.addText(font, textPos, items[i], textColor);
        }
    }

    dl.popClipRect();

    // Draw scrollbar
    if (needsScrollbar) {
        Rect track(boxBounds.right() - scrollbarWidth, boxBounds.y(), scrollbarWidth, boxBounds.height());
        dl.addRectFilled(track, theme.colors.scrollbarTrack);

        float thumbHeight = std::max(20.0f, (boxBounds.height() / totalContentHeight) * boxBounds.height());
        float maxScroll = std::max(0.0f, totalContentHeight - boxBounds.height());
        
        float thumbY = track.y();
        if (maxScroll > 0.001f) {
            float t = std::clamp(state.scrollOffset / maxScroll, 0.0f, 1.0f);
            thumbY += t * (track.height() - thumbHeight);
        }

        Rect thumb(track.x() + 2, thumbY, track.width() - 4, thumbHeight);
        
        std::string scrollerLabel = std::string(label) + "_scroller";
        WidgetId scrollbarId = ctx.makeId(scrollerLabel);
        WidgetState scrollState = getWidgetState(ctx, scrollbarId);
        
        Color thumbColor = (scrollState.hovered || scrollState.active)
            ? theme.colors.scrollbarThumbHover 
            : theme.colors.scrollbarThumb;
        dl.addRectFilled(thumb, thumbColor, (scrollbarWidth - 4) / 2);
    }

    // Handle keyboard navigation
    if (boxBounds.contains(input.mousePos()) && !options.disabled) {
        if (input.isKeyPressed(Key::Down) && selectedIndex < (int)items.size() - 1) {
            selectedIndex++;
            changed = true;
            // Scroll to keep selection visible
            float selY = selectedIndex * itemHeight;
            if (selY + itemHeight > state.scrollOffset + boxBounds.height()) {
                state.scrollOffset = selY + itemHeight - boxBounds.height();
            }
        }
        if (input.isKeyPressed(Key::Up) && selectedIndex > 0) {
            selectedIndex--;
            changed = true;
            // Scroll to keep selection visible
            float selY = selectedIndex * itemHeight;
            if (selY < state.scrollOffset) {
                state.scrollOffset = selY;
            }
        }
    }

    return changed;
}

bool ListboxMulti(Context& ctx, std::string_view label, std::vector<int>& selectedIndices,
                  const std::vector<std::string>& items,
                  const ListboxOptions& options) {
    auto wc = WidgetContext::make(ctx);

    const Theme& theme = *wc.theme;
    IDrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = ctx.input();

    WidgetId id = ctx.makeId(label);
    ListboxState& state = s_listboxStates[id];

    float width = options.style.width > 0 ? options.style.width : 200.0f;
    float height = options.style.height > 0 ? options.style.height : options.height;
    float itemHeight = options.itemHeight > 0 
        ? options.itemHeight 
        : (font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f);
    float labelWidth = 0;

    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }

    Rect bounds = allocateWidgetBounds(ctx, options.style, labelWidth + width, height);
    Rect boxBounds(bounds.x() + labelWidth, bounds.y(), width, height);

    // Draw label
    if (font && !label.empty()) {
        Vec2 labelPos(bounds.x(), bounds.y() + theme.metrics.paddingSmall);
        Color labelColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, labelPos, label, labelColor);
    }

    float radius = theme.metrics.borderRadiusSmall;
    Color bgColor = options.disabled 
        ? theme.colors.inputBackground.withAlpha(0.5f) 
        : theme.colors.inputBackground;
    dl.addRectFilled(boxBounds, bgColor, radius);
    dl.addRect(boxBounds, theme.colors.inputBorder, radius);

    float totalContentHeight = itemHeight * (float)items.size();
    bool needsScrollbar = totalContentHeight > boxBounds.height();
    float scrollbarWidth = needsScrollbar ? 10.0f : 0.0f;
    Rect contentArea(boxBounds.x(), boxBounds.y(), boxBounds.width() - scrollbarWidth, boxBounds.height());

    float maxScroll = std::max(0.0f, totalContentHeight - boxBounds.height());

    // Handle scrollbar interaction
    if (needsScrollbar && !options.disabled) {
        Rect track(boxBounds.right() - scrollbarWidth, boxBounds.y(), scrollbarWidth, boxBounds.height());
        std::string scrollerLabel = std::string(label) + "_scroller";
        WidgetId scrollbarId = ctx.makeId(scrollerLabel);
        
        WidgetInteraction scrollInteraction = handleWidgetInteraction(ctx, scrollbarId, track, true);
        
        if (scrollInteraction.dragging || (scrollInteraction.clicked && track.contains(input.mousePos()))) {
             float thumbHeight = std::max(20.0f, (boxBounds.height() / totalContentHeight) * boxBounds.height());
             float effectiveTrackHeight = track.height() - thumbHeight;
             
             if (effectiveTrackHeight > 0) {
                 float mouseRelY = input.mousePos().y - track.y() - thumbHeight * 0.5f;
                 float t = std::clamp(mouseRelY / effectiveTrackHeight, 0.0f, 1.0f);
                 state.scrollOffset = t * maxScroll;
             }
        }
    }

    if (boxBounds.contains(input.mousePos()) && !options.disabled && !ctx.isOccluded(input.mousePos())) {
        state.scrollOffset -= input.scrollDelta().y * itemHeight;
        state.scrollOffset = std::clamp(state.scrollOffset, 0.0f, maxScroll);
    }

    bool changed = false;

    dl.pushClipRect(contentArea);

    auto isSelected = [&](int idx) {
        return std::find(selectedIndices.begin(), selectedIndices.end(), idx) != selectedIndices.end();
    };

    for (int i = 0; i < (int)items.size(); ++i) {
        float y = boxBounds.y() + i * itemHeight - state.scrollOffset;
        if (y + itemHeight < boxBounds.y() || y > boxBounds.bottom()) continue;

        Rect itemRect(contentArea.x(), y, contentArea.width(), itemHeight);
        
        bool isHovered = itemRect.contains(input.mousePos()) && 
                         contentArea.contains(input.mousePos()) && 
                         !options.disabled && 
                         !ctx.isOccluded(input.mousePos());
        bool selected = isSelected(i);

        if (isHovered && input.isMousePressed(MouseButton::Left)) {
            if (input.modifiers().ctrl) {
                // Toggle selection with Ctrl
                if (selected) {
                    selectedIndices.erase(
                        std::remove(selectedIndices.begin(), selectedIndices.end(), i),
                        selectedIndices.end()
                    );
                } else {
                    selectedIndices.push_back(i);
                }
            } else {
                // Single selection without Ctrl
                selectedIndices.clear();
                selectedIndices.push_back(i);
            }
            changed = true;
        }

        if (selected) {
            dl.addRectFilled(itemRect, theme.colors.selection);
        } else if (isHovered) {
            dl.addRectFilled(itemRect, theme.colors.selection.withAlpha((uint8_t)100));
        }

        if (font) {
            Vec2 textPos(
                itemRect.x() + theme.metrics.paddingSmall,
                itemRect.y() + (itemHeight - font->lineHeight()) * 0.5f
            );
            Color textColor = selected 
                ? theme.colors.selectionText 
                : (options.disabled ? theme.colors.textDisabled : theme.colors.text);
            dl.addText(font, textPos, items[i], textColor);
        }
    }

    dl.popClipRect();

    if (needsScrollbar) {
        Rect track(boxBounds.right() - scrollbarWidth, boxBounds.y(), scrollbarWidth, boxBounds.height());
        dl.addRectFilled(track, theme.colors.scrollbarTrack);

        float thumbHeight = std::max(20.0f, (boxBounds.height() / totalContentHeight) * boxBounds.height());
        float maxScroll = std::max(0.0f, totalContentHeight - boxBounds.height());
        
        float thumbY = track.y();
        if (maxScroll > 0.001f) {
            float t = std::clamp(state.scrollOffset / maxScroll, 0.0f, 1.0f);
            thumbY += t * (track.height() - thumbHeight);
        }

        Rect thumb(track.x() + 2, thumbY, track.width() - 4, thumbHeight);
        
        std::string scrollerLabel = std::string(label) + "_scroller";
        WidgetId scrollbarId = ctx.makeId(scrollerLabel);
        WidgetState scrollState = getWidgetState(ctx, scrollbarId);

        Color thumbColor = (scrollState.hovered || scrollState.active)
            ? theme.colors.scrollbarThumbHover 
            : theme.colors.scrollbarThumb;
        dl.addRectFilled(thumb, thumbColor, (scrollbarWidth - 4) / 2);
    }

    return changed;
}



} // namespace fst
