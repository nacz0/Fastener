#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/dock_node.h"

namespace fst {

class Context;


//=============================================================================
// DockPreviewState - State for dock preview overlay
//=============================================================================
struct DockPreviewState {
    bool visible = false;
    Rect targetRect;
    DockDirection direction = DockDirection::None;
    Color overlayColor;
};

//=============================================================================
// Dock Preview Functions
//=============================================================================

/**
 * Renders the dock preview overlay during window dragging.
 * Called automatically by DockContext::endFrame().
 */
void RenderDockPreview(Context& ctx);


/**
 * Calculates the dock preview state for a target node.
 * @param targetNode The node being hovered over
 * @param mousePos Current mouse position
 * @return Preview state with target rect and direction
 */
DockPreviewState CalculateDockPreview(Context& ctx, const DockNode* targetNode, 
                                       const Vec2& mousePos);

/**
 * Renders dock target indicators (the zones where you can drop a window).
 * Shows directional indicators (left/right/top/bottom/center).
 */
void RenderDockTargetIndicators(Context& ctx, DockNode* targetNode, const Vec2& mousePos);


/**
 * Gets the dock direction based on mouse position over a node.
 * @param node The dock node to test
 * @param mousePos Current mouse position
 * @return The dock direction, or None if not over a valid dock zone
 */
DockDirection GetDockDirectionFromMouse(const DockNode* node, const Vec2& mousePos);

/**
 * Gets the preview rect for a specific dock direction.
 * @param node The target node
 * @param direction The dock direction
 * @return The rect that would be occupied by the docked window
 */
Rect GetDockPreviewRect(const DockNode* node, DockDirection direction);

} // namespace fst
