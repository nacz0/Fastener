/**
 * @file tooltip.cpp
 * @brief Tooltip and help marker widget implementations.
 */

#include "fastener/widgets/tooltip.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/widget_utils.h"
#include "fastener/platform/window.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace fst {

namespace {

std::vector<std::string> wrapText(const Font* font, const std::string& text, float maxWidth) {
    std::vector<std::string> lines;
    std::string currentLine;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        if (font->measureText(testLine).x > maxWidth) {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine = word;
            } else {
                // Word itself is wider than maxWidth, forced break
                lines.push_back(word);
                currentLine = "";
            }
        } else {
            currentLine = testLine;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    return lines;
}

} // namespace

namespace internal {

static TooltipState s_tooltipState;

TooltipState& getTooltipState() {
    return s_tooltipState;
}

void registerHoveredWidget(WidgetId id, const Rect& bounds) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    TooltipState& state = s_tooltipState;
    
    if (state.hoveredWidget != id) {
        // New widget being hovered
        state.hoveredWidget = id;
        state.hoveredBounds = bounds;
        state.hoverStartTime = ctx->time();
        state.isShowing = false;
        state.showAlpha = 0.0f;
    }
}

void resetTooltipState() {
    // Reset at frame start - will be re-registered if still hovered
    s_tooltipState.hoveredWidget = INVALID_WIDGET_ID;
}

void renderActiveTooltip() {
    // This is called via deferRender, actual rendering happens in Tooltip()
}

} // namespace internal

//=============================================================================
// Tooltip Implementation
//=============================================================================

void Tooltip(const char* text, const TooltipOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    // Check if last widget was hovered
    WidgetId hoveredId = ctx->getHoveredWidget();
    if (hoveredId == INVALID_WIDGET_ID) return;
    
    internal::TooltipState& state = internal::s_tooltipState;
    
    // Track this hover
    if (state.hoveredWidget != hoveredId) {
        state.hoveredWidget = hoveredId;
        state.hoverStartTime = ctx->time();
        state.isShowing = false;
        state.showAlpha = 0.0f;
    }
    
    // Check delay
    float hoverDuration = ctx->time() - state.hoverStartTime;
    if (hoverDuration < options.delay) return;
    
    // Calculate fade alpha
    float fadeProgress = (hoverDuration - options.delay) / options.fadeInDuration;
    state.showAlpha = std::min(1.0f, fadeProgress);
    state.isShowing = true;
    
    // Calculate position (near mouse, but within window bounds)
    Vec2 mousePos = ctx->input().mousePos();
    
    // Defer rendering so tooltip appears above everything
    std::string textStr = text;
    ctx->deferRender([textStr, mousePos, options, alpha = state.showAlpha]() {
        Context* ctx = Context::current();
        if (!ctx) return;
        
        const Theme& theme = ctx->theme();
        DrawList& dl = ctx->drawList();
        Font* font = ctx->font();
        if (!font) return;
        
        // Wrap text
        std::vector<std::string> lines = wrapText(font, textStr, options.maxWidth);
        if (lines.empty()) return;

        // Measure multi-line dimensions
        float maxLineWidth = 0.0f;
        for (const auto& line : lines) {
            maxLineWidth = std::max(maxLineWidth, font->measureText(line).x);
        }
        
        float padding = theme.metrics.paddingMedium;
        float lineHeight = font->lineHeight();
        float tooltipW = maxLineWidth + padding * 2;
        float tooltipH = (lines.size() * lineHeight) + padding * 2;
        
        // Position: prefer below and right of cursor
        float offsetX = 12.0f;
        float offsetY = 16.0f;
        Vec2 tooltipPos(mousePos.x + offsetX, mousePos.y + offsetY);
        
        // Keep within window bounds
        Window& window = ctx->window();
        float windowW = static_cast<float>(window.width());
        float windowH = static_cast<float>(window.height());
        
        if (tooltipPos.x + tooltipW > windowW - 5) {
            tooltipPos.x = mousePos.x - tooltipW - 5;
        }
        if (tooltipPos.y + tooltipH > windowH - 5) {
            tooltipPos.y = mousePos.y - tooltipH - 5;
        }
        if (tooltipPos.x < 5) tooltipPos.x = 5;
        if (tooltipPos.y < 5) tooltipPos.y = 5;
        
        Rect bounds(tooltipPos.x, tooltipPos.y, tooltipW, tooltipH);
        
        // Colors with alpha for fade
        Color bgColor = theme.colors.tooltipBackground.withAlpha(
            static_cast<uint8_t>(alpha * theme.colors.tooltipBackground.a));
        Color textColor = theme.colors.tooltipText.withAlpha(
            static_cast<uint8_t>(alpha * 255));
        Color borderColor = theme.colors.tooltipBorder.withAlpha(
            static_cast<uint8_t>(alpha * theme.colors.tooltipBorder.a));
        
        // Draw shadow
        Color shadowColor = theme.colors.shadow.withAlpha(
            static_cast<uint8_t>(alpha * theme.colors.shadow.a));
        Rect shadowRect(bounds.x() + 2, bounds.y() + 2, bounds.width(), bounds.height());
        dl.addRectFilled(shadowRect, shadowColor, theme.metrics.borderRadiusSmall);
        
        // Draw background
        dl.addRectFilled(bounds, bgColor, theme.metrics.borderRadiusSmall);
        
        // Draw border
        dl.addRect(bounds, borderColor, theme.metrics.borderRadiusSmall);
        
        // Draw text lines
        for (size_t i = 0; i < lines.size(); ++i) {
            Vec2 textPos(bounds.x() + padding, bounds.y() + padding + i * lineHeight);
            dl.addText(font, textPos, lines[i], textColor);
        }
    });
}

void Tooltip(const std::string& text, const TooltipOptions& options) {
    Tooltip(text.c_str(), options);
}

void ShowTooltip(const char* text, Vec2 position, const TooltipOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    // Directly show tooltip at position (no delay)
    std::string textStr = text;
    ctx->deferRender([textStr, position, options]() {
        Context* ctx = Context::current();
        if (!ctx) return;
        
        const Theme& theme = ctx->theme();
        DrawList& dl = ctx->drawList();
        Font* font = ctx->font();
        if (!font) return;
        
        // Wrap text
        std::vector<std::string> lines = wrapText(font, textStr, options.maxWidth);
        if (lines.empty()) return;

        float maxLineWidth = 0.0f;
        for (const auto& line : lines) {
            maxLineWidth = std::max(maxLineWidth, font->measureText(line).x);
        }
        
        float padding = theme.metrics.paddingMedium;
        float lineHeight = font->lineHeight();
        float tooltipW = maxLineWidth + padding * 2;
        float tooltipH = (lines.size() * lineHeight) + padding * 2;
        
        Rect bounds(position.x, position.y, tooltipW, tooltipH);
        
        // Draw shadow
        Rect shadowRect(bounds.x() + 2, bounds.y() + 2, bounds.width(), bounds.height());
        dl.addRectFilled(shadowRect, theme.colors.shadow, theme.metrics.borderRadiusSmall);
        
        // Draw background and border
        dl.addRectFilled(bounds, theme.colors.tooltipBackground, theme.metrics.borderRadiusSmall);
        dl.addRect(bounds, theme.colors.tooltipBorder, theme.metrics.borderRadiusSmall);
        
        // Draw text
        for (size_t i = 0; i < lines.size(); ++i) {
            Vec2 textPos(bounds.x() + padding, bounds.y() + padding + i * lineHeight);
            dl.addText(font, textPos, lines[i], theme.colors.tooltipText);
        }
    });
}

void ShowTooltip(const std::string& text, Vec2 position, const TooltipOptions& options) {
    ShowTooltip(text.c_str(), position, options);
}

//=============================================================================
// HelpMarker Implementation
//=============================================================================

bool HelpMarker(const char* text, const HelpMarkerOptions& options) {
    Context* ctx = Context::current();
    if (!ctx) return false;
    
    const Theme& theme = ctx->theme();
    DrawList& dl = ctx->drawList();
    Font* font = ctx->font();
    
    // Size of help marker circle
    float size = theme.metrics.fontSizeSmall + theme.metrics.paddingSmall * 2;
    float radius = size * 0.5f;
    
    // Position from style or layout (simple placeholder layout)
    float x = options.style.x;
    float y = options.style.y;
    
    // Bounds for interaction
    Rect bounds(x, y, size, size);
    
    // Generate unique ID
    WidgetId id = ctx->makeId(text);
    
    // Handle interaction
    WidgetInteraction interaction = handleWidgetInteraction(id, bounds, true);
    WidgetState state = getWidgetState(id);
    
    // Colors based on state
    Color circleColor = state.hovered ? theme.colors.textSecondary : theme.colors.textDisabled;
    Color textColor = theme.colors.primaryText;
    
    if (state.active) {
        circleColor = theme.colors.primary;
    }
    
    // Draw circle with "?"
    Vec2 center(bounds.x() + radius, bounds.y() + radius);
    dl.addCircleFilled(center, radius, circleColor);
    
    if (font) {
        Vec2 qSize = font->measureText("?");
        Vec2 qPos(center.x - qSize.x * 0.5f, center.y - qSize.y * 0.5f);
        dl.addText(font, qPos, "?", nullptr, textColor);
    }
    
    // Show tooltip if hovered
    if (state.hovered) {
        Tooltip(text, options.tooltipOptions);
    }
    
    return interaction.clicked;
}

bool HelpMarker(const std::string& text, const HelpMarkerOptions& options) {
    return HelpMarker(text.c_str(), options);
}

} // namespace fst
