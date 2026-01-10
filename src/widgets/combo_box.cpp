/**
 * @file combo_box.cpp
 * @brief ComboBox dropdown widget implementation.
 */

#include "fastener/widgets/combo_box.h"
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
// ComboBox State
//=============================================================================

/** @brief Per-combobox persistent state for dropdown management. */
struct ComboBoxState {
    bool isOpen = false;
    int hoveredIndex = -1;
    float scrollOffset = 0.0f;
};

static std::unordered_map<WidgetId, ComboBoxState> s_comboStates;

//=============================================================================
// ComboBox Implementation
//=============================================================================

/**
 * @brief Renders a dropdown combobox for item selection.
 * 
 * @param label Label displayed before the combobox
 * @param selectedIndex Reference to the selected item index
 * @param items Vector of item strings to display
 * @param options ComboBox styling and behavior options
 * @return true if the selection was changed this frame
 */
bool ComboBox(std::string_view label, int& selectedIndex, 
              const std::vector<std::string>& items,
              const ComboBoxOptions& options) {
    // Get widget context
    auto wc = getWidgetContext();
    if (!wc.valid()) return false;

    const Theme& theme = *wc.theme;
    DrawList& dl = *wc.dl;
    Font* font = wc.font;
    InputState& input = wc.ctx->input();

    WidgetId id = wc.ctx->makeId(label);
    ComboBoxState& state = s_comboStates[id];

    float width = options.style.width > 0 ? options.style.width : 150.0f;
    float height = options.style.height > 0 ? options.style.height : 28.0f;
    float labelWidth = 0;

    if (font && !label.empty()) {
        labelWidth = font->measureText(label).x + theme.metrics.paddingMedium;
    }

    // Allocate bounds
    Rect bounds = allocateWidgetBounds(options.style, labelWidth + width, height);

    Rect boxBounds(bounds.x() + labelWidth, bounds.y(), width, height);

    // Handle main button interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, boxBounds, true);
    WidgetState widgetState = getWidgetState(id);
    widgetState.disabled = options.disabled;

    bool changed = false;

    if (interaction.clicked && !options.disabled) {
        state.isOpen = !state.isOpen;
        state.hoveredIndex = selectedIndex;
        state.scrollOffset = 0.0f;
    }

    // Close on click outside
    if (state.isOpen && input.isMousePressed(MouseButton::Left) && !boxBounds.contains(input.mousePos())) {
        // Check if clicking in dropdown
        float itemHeight = font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f;
        float dropdownHeight = std::min(options.dropdownMaxHeight, itemHeight * (float)items.size());
        Rect dropdownBounds(boxBounds.x(), boxBounds.bottom(), boxBounds.width(), dropdownHeight);
        
        if (!dropdownBounds.contains(input.mousePos())) {
            state.isOpen = false;
        }
    }

    // Consume mouse if hovering open dropdown
    if (state.isOpen) {
        float itemHeight = font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f;
        float dropdownHeight = std::min(options.dropdownMaxHeight, itemHeight * (float)items.size());
        // Use a slightly expanded rect to cover the gap as well
        Rect dropdownBounds(boxBounds.x(), boxBounds.bottom(), boxBounds.width(), dropdownHeight + 2);
        
        if (dropdownBounds.contains(input.mousePos())) {
            input.consumeMouse();
        }
    }

    // Draw label
    if (font && !label.empty()) {
        Vec2 labelPos(bounds.x(), bounds.y() + (height - font->lineHeight()) * 0.5f);
        Color labelColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        dl.addText(font, labelPos, label, labelColor);
    }

    // Draw combo box button
    Color bgColor;
    if (options.disabled) {
        bgColor = theme.colors.inputBackground.withAlpha(0.5f);
    } else if (state.isOpen || widgetState.active) {
        bgColor = theme.colors.inputBackground.lighter(0.1f);
    } else if (widgetState.hovered) {
        bgColor = theme.colors.inputBackground.lighter(0.05f);
    } else {
        bgColor = theme.colors.inputBackground;
    }

    float radius = theme.metrics.borderRadiusSmall;
    dl.addRectFilled(boxBounds, bgColor, radius);
    
    Color borderColor = state.isOpen ? theme.colors.borderFocused : theme.colors.inputBorder;
    dl.addRect(boxBounds, borderColor, radius);

    // Draw selected item text
    if (font && selectedIndex >= 0 && selectedIndex < (int)items.size()) {
        Vec2 textPos(
            boxBounds.x() + theme.metrics.paddingSmall,
            boxBounds.y() + (height - font->lineHeight()) * 0.5f
        );
        Color textColor = options.disabled ? theme.colors.textDisabled : theme.colors.text;
        
        // Clip text if too long
        dl.pushClipRect(Rect(boxBounds.x(), boxBounds.y(), boxBounds.width() - 20, boxBounds.height()));
        dl.addText(font, textPos, items[selectedIndex], textColor);
        dl.popClipRect();
    }

    // Draw dropdown arrow
    {
        float arrowSize = 6.0f;
        Vec2 arrowCenter(boxBounds.right() - 12, boxBounds.center().y);
        Color arrowColor = options.disabled ? theme.colors.textDisabled : theme.colors.textSecondary;
        
        if (state.isOpen) {
            // Up arrow
            dl.addTriangleFilled(
                Vec2(arrowCenter.x - arrowSize, arrowCenter.y + arrowSize * 0.5f),
                Vec2(arrowCenter.x + arrowSize, arrowCenter.y + arrowSize * 0.5f),
                Vec2(arrowCenter.x, arrowCenter.y - arrowSize * 0.5f),
                arrowColor
            );
        } else {
            // Down arrow
            dl.addTriangleFilled(
                Vec2(arrowCenter.x - arrowSize, arrowCenter.y - arrowSize * 0.5f),
                Vec2(arrowCenter.x + arrowSize, arrowCenter.y - arrowSize * 0.5f),
                Vec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.5f),
                arrowColor
            );
        }
    }

    // Draw dropdown popup safely at the end of frame
    if (state.isOpen && !items.empty()) {
        // Capture data needed for deferred rendering
        std::vector<std::string> safeItems = items;
        int selectedIndexCopy = selectedIndex;
        
        wc.ctx->deferRender([=, &state]() mutable {
            // Re-acquire context helpers since we are in a callback
            Context* ctx = Context::current();
            if (!ctx) return;
            DrawList& dl = ctx->drawList();
            Font* font = ctx->font();
            InputState& input = ctx->input();
            const Theme& theme = ctx->theme();

            float itemHeight = font ? font->lineHeight() + theme.metrics.paddingSmall * 2 : 24.0f;
            float dropdownHeight = std::min(options.dropdownMaxHeight, itemHeight * (float)safeItems.size());
            Rect dropdownBounds(boxBounds.x(), boxBounds.bottom() + 2, boxBounds.width(), dropdownHeight);
            ctx->addFloatingWindowRect(Rect(boxBounds.x(), boxBounds.bottom(), boxBounds.width(), dropdownHeight + 2));

            // Dropdown background with shadow
            dl.addShadow(dropdownBounds, theme.colors.shadow, 8.0f, radius);
            dl.addRectFilled(dropdownBounds, theme.colors.panelBackground, radius);
            dl.addRect(dropdownBounds, theme.colors.border, radius);

            dl.pushClipRect(dropdownBounds);

            // Handle scroll
            if (dropdownBounds.contains(input.mousePos())) {
                state.scrollOffset -= input.scrollDelta().y * itemHeight;
                float maxScroll = std::max(0.0f, itemHeight * (float)safeItems.size() - dropdownHeight);
                state.scrollOffset = std::clamp(state.scrollOffset, 0.0f, maxScroll);
            }

            // Draw items
            for (int i = 0; i < (int)safeItems.size(); ++i) {
                float y = dropdownBounds.y() + i * itemHeight - state.scrollOffset;
                if (y + itemHeight < dropdownBounds.y() || y > dropdownBounds.bottom()) continue;

                Rect itemRect(dropdownBounds.x(), y, dropdownBounds.width(), itemHeight);
                
                // Draw hover/selected background
                if (i == state.hoveredIndex) {
                    dl.addRectFilled(itemRect, theme.colors.selection);
                } else if (i == selectedIndexCopy) {
                    dl.addRectFilled(itemRect, theme.colors.selection.withAlpha((uint8_t)100));
                }

                if (font) {
                    Vec2 textPos(
                        itemRect.x() + theme.metrics.paddingSmall,
                        itemRect.y() + (itemHeight - font->lineHeight()) * 0.5f
                    );
                    Color textColor = (i == state.hoveredIndex) ? theme.colors.selectionText : theme.colors.text;
                    dl.addText(font, textPos, safeItems[i], textColor);
                }
            }

            dl.popClipRect();
        });
    }

    return changed;
}

} // namespace fst
