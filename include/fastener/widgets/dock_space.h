#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/dock_node.h"
#include <string>

namespace fst {

//=============================================================================
// DockSpaceOptions - Configuration for dock space widgets
//=============================================================================
struct DockSpaceOptions {
    DockNodeFlags nodeFlags;
    bool passthruCentralNode = true;  // For IDE-like main area
};

//=============================================================================
// DockSpace Widget Functions
//=============================================================================

/**
 * Creates and renders a DockSpace in the given area.
 * Windows can be dragged and docked to this area.
 * 
 * @param id Unique identifier for the dock space
 * @param bounds Area for the dock space (usually entire viewport)
 * @param options Configuration
 * @return ID of the created/existing root DockNode
 */
DockNode::Id DockSpace(const std::string& id, const Rect& bounds, 
                       const DockSpaceOptions& options = {});

/**
 * Helper: DockSpace covering the entire viewport.
 */
DockNode::Id DockSpaceOverViewport(const DockSpaceOptions& options = {});

/**
 * Renders the splitters between docked panels in a dock space.
 * Called internally during DockSpace rendering.
 */
void RenderDockSplitters(DockNode* rootNode);

/**
 * Handles splitter interaction (drag to resize).
 * Called internally during DockSpace rendering.
 */
bool HandleDockSplitter(DockNode* node, const Rect& splitterRect, bool isVertical);

/**
 * Renders the tab bar for a dock node with multiple windows.
 * Called internally during DockSpace rendering.
 */
void RenderDockTabBar(DockNode* node);

} // namespace fst
