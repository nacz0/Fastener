#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/dock_node.h"
#include <string>

namespace fst {

class Context;


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
 */
DockNode::Id DockSpace(Context& ctx, const std::string& id, const Rect& bounds, 
                       const DockSpaceOptions& options = {});

DockNode::Id DockSpace(const std::string& id, const Rect& bounds, 
                       const DockSpaceOptions& options = {});

/**
 * Helper: DockSpace covering the entire viewport.
 */
DockNode::Id DockSpaceOverViewport(Context& ctx, const DockSpaceOptions& options = {});
DockNode::Id DockSpaceOverViewport(const DockSpaceOptions& options = {});


/**
 * Renders the splitters between docked panels in a dock space.
 * Called internally during DockSpace rendering.
 */
void RenderDockSplitters(Context& ctx, DockNode* rootNode);
void RenderDockSplitters(DockNode* rootNode);

/**
 * Handles splitter interaction (drag to resize).
 * Called internally during DockSpace rendering.
 */
bool HandleDockSplitter(Context& ctx, DockNode* node, const Rect& splitterRect, bool isVertical);
bool HandleDockSplitter(DockNode* node, const Rect& splitterRect, bool isVertical);

/**
 * Renders the tab bar for a dock node with multiple windows.
 * Called internally during DockSpace rendering.
 */
void RenderDockTabBar(Context& ctx, DockNode* node);
void RenderDockTabBar(DockNode* node);


} // namespace fst
