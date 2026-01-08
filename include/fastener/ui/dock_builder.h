#pragma once

#include "fastener/ui/dock_node.h"
#include <string>

namespace fst {

//=============================================================================
// DockBuilder - Programmatic API for building dock layouts
//=============================================================================
/**
 * DockBuilder provides a programmatic API for creating dock layouts.
 * Primarily used to set up the initial application layout.
 * 
 * Example:
 * @code
 * auto dockspaceId = fst::DockBuilder::GetDockSpaceId("MainDockSpace");
 * 
 * fst::DockBuilder::Begin(dockspaceId);
 * auto left = fst::DockBuilder::SplitNode(dockspaceId, DockDirection::Left, 0.25f);
 * auto bottom = fst::DockBuilder::SplitNode(dockspaceId, DockDirection::Bottom, 0.3f);
 * 
 * fst::DockBuilder::DockWindow("Hierarchy", left);
 * fst::DockBuilder::DockWindow("Console", bottom);
 * fst::DockBuilder::DockWindow("Scene", dockspaceId);  // Central
 * fst::DockBuilder::Finish();
 * @endcode
 */
class DockBuilder {
public:
    /**
     * Gets the numeric ID for a dock space by name.
     * If the dock space doesn't exist, creates a new ID.
     */
    static DockNode::Id GetDockSpaceId(const std::string& name);
    
    /**
     * Begins building a dock layout.
     * Must be called before any other DockBuilder operations.
     */
    static void Begin(DockNode::Id dockspaceId);
    
    /**
     * Finishes building the dock layout.
     * Commits all pending changes.
     */
    static void Finish();
    
    /**
     * Checks if currently in build mode.
     */
    static bool IsBuilding();
    
    /**
     * Splits a node and returns the ID of the new node.
     * @param nodeId The node to split
     * @param direction Direction of the split (Left/Right/Top/Bottom)
     * @param sizeRatio Size ratio for the new node (0.0-1.0)
     * @return ID of the newly created node
     */
    static DockNode::Id SplitNode(DockNode::Id nodeId, 
                                   DockDirection direction, 
                                   float sizeRatio);
    
    /**
     * Docks a window to a specific node.
     */
    static void DockWindow(const std::string& windowId, DockNode::Id nodeId);
    
    /**
     * Sets flags for a specific node.
     */
    static void SetNodeFlags(DockNode::Id nodeId, DockNodeFlags flags);
    
    /**
     * Gets the node at a specific direction from an existing node.
     * Useful for getting nodes created by SplitNode.
     */
    static DockNode::Id GetNode(DockNode::Id parentId, DockDirection direction);
    
    /**
     * Removes all windows and resets a dock space to empty.
     */
    static void ClearDockSpace(DockNode::Id dockspaceId);
};

} // namespace fst
