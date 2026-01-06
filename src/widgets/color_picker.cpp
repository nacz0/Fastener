/**
 * @file color_picker.cpp
 * @brief Color picker widget implementation with HSV color selection.
 */

#include "fastener/widgets/color_picker.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include "fastener/widgets/label.h"
#include "fastener/widgets/text_input.h"
#include <cstdio>
#include <algorithm>

namespace fst {

//=============================================================================
// ColorPicker Implementation
//=============================================================================

/**
 * @brief Renders an HSV color picker widget.
 * 
 * @param label Label displayed above the color picker
 * @param color Reference to the color being edited
 * @param options ColorPicker styling options
 * @return true if the color was changed this frame
 */
bool ColorPicker(const char* label, Color& color, const ColorPickerOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;

    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    // Generate base ID for the widget group
    ctx->pushId(label);
    WidgetId baseId = ctx->currentId();

    // Prepare HSV values for interaction
    float h, s, v;
    color.toHSV(h, s, v);

    // Layout
    float width = options.style.width > 0 ? options.style.width : 250.0f;
    float svSize = width - 30.0f; // Square size for Saturation-Value
    float hueBarWidth = 20.0f;
    float rowHeight = svSize;
    
    // Allocate wrapper bounds to reserve space in layout
    Rect bounds;
    if (options.style.x < 0.0f && options.style.y < 0.0f) {
        // Calculate total height: Label + SV Square + Hue Bar + (Hex Input)
        float totalHeight = svSize;
        if (label && label[0] != '\0') totalHeight += ctx->font()->lineHeight() + theme.metrics.paddingSmall;
        if (options.showHex) totalHeight += theme.metrics.inputHeight + theme.metrics.paddingSmall;
        // Padding for internal layout
        totalHeight += 20; 

        bounds = ctx->layout().allocate(width, totalHeight, options.style.flexGrow);
        
        // Start explicit vertical layout within these bounds
        ctx->layout().beginContainer(bounds, LayoutDirection::Vertical);
        ctx->layout().setSpacing(10);
    } else {
        // Absolute positioning fall-back
        BeginVertical(10);
    }
    
    if (label && label[0] != '\0') {
        Label(label);
    }

    bool changed = false;
    
    // Grid: [ SV Square ] [ Hue Bar ]
    BeginHorizontal(10);
    
    // 1. SV Square
    Rect svRect = Allocate(svSize, svSize);
    WidgetId svId = ctx->makeId("sv_square");
    WidgetInteraction svInteract = handleWidgetInteraction(svId, svRect);
    
    if (svInteract.dragging || (svInteract.hovered && ctx->input().isMousePressed(MouseButton::Left))) {
        Vec2 mousePos = ctx->input().mousePos();
        s = std::clamp((mousePos.x - svRect.x()) / svRect.width(), 0.0f, 1.0f);
        v = 1.0f - std::clamp((mousePos.y - svRect.y()) / svRect.height(), 0.0f, 1.0f);
        changed = true;
    }

    // Draw SV Square
    Color pureHue = Color::fromHSV(h, 1.0f, 1.0f);
    dl.addRectFilledMultiColor(svRect, Color::white(), pureHue, pureHue, Color::white());
    
    // Draw Value gradient (black overlay)
    // Note: We need a second pass or a more complex gradient for SV.
    // SV square is usually: (White -> Hue) horizontally, then overlayed with (Transparent -> Black) vertically.
    // fst::DrawList currently only supports one multi-color rect, so let's use two.
    dl.addRectFilledMultiColor(svRect, Color(0,0,0,0), Color(0,0,0,0), Color::black(), Color::black());

    // Draw SV cursor
    Vec2 svCursor(
        svRect.x() + s * svRect.width(),
        svRect.y() + (1.0f - v) * svRect.height()
    );
    // Draw SV cursor (Ring style)
    dl.addCircle(svCursor, 6.0f, Color::black(), 16);
    dl.addCircle(svCursor, 4.0f, Color::white(), 16);

    // 2. Hue Bar
    Rect hueRect = Allocate(hueBarWidth, svSize);
    WidgetId hueId = ctx->makeId("hue_bar");
    WidgetInteraction hueInteract = handleWidgetInteraction(hueId, hueRect);

    if (hueInteract.dragging || (hueInteract.hovered && ctx->input().isMousePressed(MouseButton::Left))) {
        Vec2 mousePos = ctx->input().mousePos();
        h = std::clamp((mousePos.y - hueRect.y()) / hueRect.height(), 0.0f, 1.0f);
        changed = true;
    }

    // Draw Hue Bar (Spectrum)
    // We draw it in segments since it's a multi-point gradient
    const int segments = 6;
    float segHeight = hueRect.height() / segments;
    for (int i = 0; i < segments; ++i) {
        Rect segRect(hueRect.x(), hueRect.y() + i * segHeight, hueRect.width(), segHeight);
        dl.addRectFilledMultiColor(segRect, 
            Color::fromHSV(i / (float)segments, 1, 1), 
            Color::fromHSV(i / (float)segments, 1, 1),
            Color::fromHSV((i+1) / (float)segments, 1, 1),
            Color::fromHSV((i+1) / (float)segments, 1, 1)
        );
    }
    
    // Draw Hue cursor
    // Draw Hue cursor
    float hueY = hueRect.y() + h * hueRect.height();
    dl.addRectFilled(Rect(hueRect.x() - 1, hueY - 2, hueRect.width() + 2, 4), Color::white());
    dl.addRect(Rect(hueRect.x() - 1, hueY - 2, hueRect.width() + 2, 4), Color::black());

    EndHorizontal();

    // 3. Optional hex/numeric display
    if (options.showHex) {
        char hex[10];
        std::sprintf(hex, "#%02X%02X%02X", color.r, color.g, color.b);
        std::string hexStr = hex;
        fst::TextInputOptions hexOpts;
        hexOpts.style = Style().withWidth(100);
        if (TextInput("hex", hexStr, hexOpts)) {
            // Hex parsing (simple)
            if (hexStr.length() >= 7 && hexStr[0] == '#') {
                uint32_t val;
                if (std::sscanf(hexStr.c_str() + 1, "%X", &val) == 1) {
                    color = Color::fromHex(val);
                    changed = true;
                }
            }
        }
    }

    if (options.style.x < 0.0f && options.style.y < 0.0f) {
        ctx->layout().endContainer(); 
    } else {
        EndVertical();
    }
    ctx->popId();

    if (changed) {
        color = Color::fromHSV(h, s, v, color.af());
    }

    return changed;
}

bool ColorPicker(const std::string& label, Color& color, const ColorPickerOptions& options) {
    return ColorPicker(label.c_str(), color, options);
}

} // namespace fst
