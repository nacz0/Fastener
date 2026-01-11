#include "fastener/widgets/dock_preview.h"
#include "fastener/core/context.h"
#include "fastener/ui/dock_context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/widget_utils.h"


namespace fst {

//=============================================================================
// Dock Preview Rendering
//=============================================================================

void RenderDockPreview(Context& ctx) {
    auto& docking = ctx.docking();
    auto& dragState = docking.dragState();

    
    if (!dragState.active || dragState.hoveredNodeId == DockNode::INVALID_ID || 
        dragState.hoveredDirection == DockDirection::None) {
        return;
    }
    
    DockNode* hoveredNode = docking.getDockNode(dragState.hoveredNodeId);
    if (!hoveredNode) return;
    
    auto& dl = ctx.drawList();
    const auto& theme = ctx.theme();

    
    dl.setLayer(DrawLayer::Overlay);
    
    // Calculate preview rect
    DockPreviewState preview = CalculateDockPreview(hoveredNode, 
                                                     dragState.mousePos);
    
    if (preview.visible) {
        // Draw semi-transparent overlay
        Color overlayColor = theme.colors.primary.withAlpha(static_cast<uint8_t>(80));
        dl.addRectFilled(preview.targetRect, overlayColor, 4.0f);
        
        // Draw border
        dl.addRect(preview.targetRect, theme.colors.primary, 4.0f);
    }

    // Draw target indicators (the 5-way cross)
    RenderDockTargetIndicators(ctx, hoveredNode, dragState.mousePos);
    
    dl.setLayer(DrawLayer::Default);
}

void RenderDockPreview() {
    auto wc = getWidgetContext();
    if (wc.valid()) RenderDockPreview(*wc.ctx);
}


//=============================================================================
// Preview Calculation
//=============================================================================

DockPreviewState CalculateDockPreview(const DockNode* targetNode, 
                                       const Vec2& mousePos) {
    DockPreviewState state;
    
    if (!targetNode) {
        return state;
    }
    
    auto wc = getWidgetContext();
    if (!wc.valid()) return state;
    
    const auto& theme = *wc.theme;


    
    DockDirection direction = GetDockDirectionFromMouse(targetNode, mousePos);
    
    if (direction == DockDirection::None) {
        return state;
    }
    
    state.visible = true;
    state.direction = direction;
    state.targetRect = GetDockPreviewRect(targetNode, direction);
    state.overlayColor = theme.colors.primary.withAlpha(static_cast<uint8_t>(80));
    
    return state;
}

//=============================================================================
// Dock Target Indicators
//=============================================================================

void RenderDockTargetIndicators(Context& ctx, DockNode* targetNode, const Vec2& mousePos) {
    if (!targetNode) return;
    
    auto& dl = ctx.drawList();
    const auto& theme = ctx.theme();

    
    const Rect& bounds = targetNode->bounds;
    const float indicatorSize = 40.0f;
    const float indicatorMargin = 10.0f;
    
    // Calculate indicator positions
    Vec2 center = bounds.center();
    
    struct Indicator {
        DockDirection direction;
        Rect rect;
    };
    
    Indicator indicators[] = {
        { DockDirection::Left, Rect(bounds.x() + indicatorMargin, 
                                    center.y - indicatorSize * 0.5f,
                                    indicatorSize, indicatorSize) },
        { DockDirection::Right, Rect(bounds.right() - indicatorMargin - indicatorSize,
                                     center.y - indicatorSize * 0.5f,
                                     indicatorSize, indicatorSize) },
        { DockDirection::Top, Rect(center.x - indicatorSize * 0.5f,
                                   bounds.y() + indicatorMargin,
                                   indicatorSize, indicatorSize) },
        { DockDirection::Bottom, Rect(center.x - indicatorSize * 0.5f,
                                      bounds.bottom() - indicatorMargin - indicatorSize,
                                      indicatorSize, indicatorSize) },
        { DockDirection::Center, Rect(center.x - indicatorSize * 0.5f,
                                      center.y - indicatorSize * 0.5f,
                                      indicatorSize, indicatorSize) }
    };
    
    for (const auto& ind : indicators) {
        bool hovered = ind.rect.contains(mousePos);
        
        Color bgColor = hovered ? theme.colors.primary : 
                                  theme.colors.panelBackground.withAlpha(static_cast<uint8_t>(200));
        Color borderColor = hovered ? theme.colors.primary : theme.colors.border;
        
        // Draw indicator background
        dl.addRectFilled(ind.rect, bgColor, 4.0f);
        dl.addRect(ind.rect, borderColor, 4.0f);
        
        // Draw directional arrow/icon
        Vec2 iconCenter = ind.rect.center();
        const float arrowSize = 12.0f;
        Color iconColor = hovered ? theme.colors.primaryText : theme.colors.text;
        
        switch (ind.direction) {
            case DockDirection::Left:
                dl.addTriangleFilled(
                    Vec2(iconCenter.x - arrowSize * 0.5f, iconCenter.y),
                    Vec2(iconCenter.x + arrowSize * 0.5f, iconCenter.y - arrowSize * 0.5f),
                    Vec2(iconCenter.x + arrowSize * 0.5f, iconCenter.y + arrowSize * 0.5f),
                    iconColor
                );
                break;
            case DockDirection::Right:
                dl.addTriangleFilled(
                    Vec2(iconCenter.x + arrowSize * 0.5f, iconCenter.y),
                    Vec2(iconCenter.x - arrowSize * 0.5f, iconCenter.y - arrowSize * 0.5f),
                    Vec2(iconCenter.x - arrowSize * 0.5f, iconCenter.y + arrowSize * 0.5f),
                    iconColor
                );
                break;
            case DockDirection::Top:
                dl.addTriangleFilled(
                    Vec2(iconCenter.x, iconCenter.y - arrowSize * 0.5f),
                    Vec2(iconCenter.x - arrowSize * 0.5f, iconCenter.y + arrowSize * 0.5f),
                    Vec2(iconCenter.x + arrowSize * 0.5f, iconCenter.y + arrowSize * 0.5f),
                    iconColor
                );
                break;
            case DockDirection::Bottom:
                dl.addTriangleFilled(
                    Vec2(iconCenter.x, iconCenter.y + arrowSize * 0.5f),
                    Vec2(iconCenter.x - arrowSize * 0.5f, iconCenter.y - arrowSize * 0.5f),
                    Vec2(iconCenter.x + arrowSize * 0.5f, iconCenter.y - arrowSize * 0.5f),
                    iconColor
                );
                break;
            case DockDirection::Center:
                // Draw a small square for center/tab docking
                dl.addRectFilled(
                    Rect(iconCenter.x - arrowSize * 0.3f, iconCenter.y - arrowSize * 0.3f,
                         arrowSize * 0.6f, arrowSize * 0.6f),
                    iconColor
                );
                break;
            default:
                break;
        }
    }
}

void RenderDockTargetIndicators(DockNode* targetNode, const Vec2& mousePos) {
    auto wc = getWidgetContext();
    if (wc.valid()) RenderDockTargetIndicators(*wc.ctx, targetNode, mousePos);
}


//=============================================================================
// Direction Detection
//=============================================================================

DockDirection GetDockDirectionFromMouse(const DockNode* node, const Vec2& mousePos) {
    if (!node || !node->bounds.contains(mousePos)) {
        return DockDirection::None;
    }
    
    const Rect& bounds = node->bounds;
    const float edgeThreshold = 0.25f;  // 25% of each edge
    
    float relX = (mousePos.x - bounds.x()) / bounds.width();
    float relY = (mousePos.y - bounds.y()) / bounds.height();
    
    // Check edges
    if (relX < edgeThreshold) {
        return DockDirection::Left;
    } else if (relX > 1.0f - edgeThreshold) {
        return DockDirection::Right;
    } else if (relY < edgeThreshold) {
        return DockDirection::Top;
    } else if (relY > 1.0f - edgeThreshold) {
        return DockDirection::Bottom;
    } else {
        return DockDirection::Center;
    }
}

//=============================================================================
// Preview Rect Calculation
//=============================================================================

Rect GetDockPreviewRect(const DockNode* node, DockDirection direction) {
    if (!node) {
        return Rect();
    }
    
    const Rect& bounds = node->bounds;
    const float ratio = 0.5f;  // New window gets 50% of space
    
    switch (direction) {
        case DockDirection::Left:
            return Rect(bounds.x(), bounds.y(), 
                       bounds.width() * ratio, bounds.height());
        case DockDirection::Right:
            return Rect(bounds.x() + bounds.width() * (1.0f - ratio), bounds.y(),
                       bounds.width() * ratio, bounds.height());
        case DockDirection::Top:
            return Rect(bounds.x(), bounds.y(),
                       bounds.width(), bounds.height() * ratio);
        case DockDirection::Bottom:
            return Rect(bounds.x(), bounds.y() + bounds.height() * (1.0f - ratio),
                       bounds.width(), bounds.height() * ratio);
        case DockDirection::Center:
            return bounds;  // Full bounds for tab docking
        default:
            return Rect();
    }
}

} // namespace fst
