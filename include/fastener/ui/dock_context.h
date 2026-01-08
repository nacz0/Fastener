#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/dock_node.h"
#include <memory>
#include <string>

namespace fst {

//=============================================================================
// DockContext - Application-level docking state management
//=============================================================================
class DockContext {
public:
    DockContext();
    ~DockContext();
    
    // Non-copyable
    DockContext(const DockContext&) = delete;
    DockContext& operator=(const DockContext&) = delete;
    
    //-------------------------------------------------------------------------
    // DockSpace Management
    //-------------------------------------------------------------------------
    
    /**
     * Creates or retrieves a dock space with the given identifier.
     * @param id Unique string identifier for the dock space
     * @param bounds Initial bounds for the dock space
     * @return ID of the root DockNode for this dock space
     */
    DockNode::Id createDockSpace(const std::string& id, const Rect& bounds);
    
    /**
     * Gets the root node of a dock space by its string ID.
     */
    DockNode* getDockSpace(const std::string& id);
    const DockNode* getDockSpace(const std::string& id) const;
    
    /**
     * Gets a dock node by its numeric ID.
     */
    DockNode* getDockNode(DockNode::Id nodeId);
    const DockNode* getDockNode(DockNode::Id nodeId) const;
    
    /**
     * Removes a dock space.
     */
    void removeDockSpace(const std::string& id);
    
    //-------------------------------------------------------------------------
    // Window Docking
    //-------------------------------------------------------------------------
    
    /**
     * Docks a window to a specific node.
     * @param windowId The widget ID of the window to dock
     * @param targetNode Target node ID to dock to
     * @param direction Where to dock relative to target (Center = tab)
     */
    void dockWindow(WidgetId windowId, DockNode::Id targetNode, 
                    DockDirection direction = DockDirection::Center);
    
    /**
     * Undocks a window from its current position.
     */
    void undockWindow(WidgetId windowId);
    
    /**
     * Checks if a window is currently docked.
     */
    bool isWindowDocked(WidgetId windowId) const;
    
    /**
     * Gets the dock node containing a specific window.
     */
    DockNode* getWindowDockNode(WidgetId windowId);
    const DockNode* getWindowDockNode(WidgetId windowId) const;
    
    // Window titles for UI
    void setWindowTitle(WidgetId windowId, const std::string& title);
    std::string getWindowTitle(WidgetId windowId) const;
    
    /**
     * Refreshes the window-to-node mapping for a subtree.
     */
    void refreshMappings(DockNode::Id nodeId = DockNode::INVALID_ID);
    
    //-------------------------------------------------------------------------
    // Drag State (for preview during dragging)
    //-------------------------------------------------------------------------
    
    struct DragState {
        bool active = false;
        WidgetId windowId = INVALID_WIDGET_ID;
        Vec2 mousePos;
        DockNode::Id hoveredNodeId = DockNode::INVALID_ID;
        DockDirection hoveredDirection = DockDirection::None;
    };
    
    DragState& dragState();
    const DragState& dragState() const;
    
    void beginDrag(WidgetId windowId, const Vec2& mousePos);
    void updateDrag(const Vec2& mousePos, DockNode* hoveredNode = nullptr, DockDirection direction = DockDirection::None);
    void endDrag(bool commit);
    
    //-------------------------------------------------------------------------
    // Frame Update
    //-------------------------------------------------------------------------
    
    void beginFrame();
    void endFrame();
    
    //-------------------------------------------------------------------------
    // Persistence
    //-------------------------------------------------------------------------
    
    /**
     * Serializes the current layout to a string.
     */
    std::string serializeLayout() const;
    
    /**
     * Deserializes a layout from a string.
     */
    bool deserializeLayout(const std::string& data);
    
    //-------------------------------------------------------------------------
    // ID Generation
    //-------------------------------------------------------------------------
    
    DockNode::Id generateNodeId();
    DockNode::Id getNodeIdFromString(const std::string& str) const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fst
