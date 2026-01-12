#pragma once

#include "fastener/core/types.h"
#include <string>
#include <vector>
#include <functional>

/**
 * @file drag_drop.h  
 * @brief Drag and drop system for transferring data between widgets
 * 
 * @ai_hint 
 * SOURCE PATTERN (call after rendering the draggable widget):
 *   if (fst::BeginDragDropSource()) {
 *       int itemIndex = 42;
 *       fst::SetDragDropPayload("MY_TYPE", &itemIndex, sizeof(int));
 *       fst::SetDragDropDisplayText("Dragging item 42");
 *       fst::EndDragDropSource();
 *   }
 * 
 * TARGET PATTERN (call after rendering the drop-target widget):
 *   if (fst::BeginDragDropTarget()) {
 *       if (const auto* payload = fst::AcceptDragDropPayload("MY_TYPE")) {
 *           int droppedIndex = payload->getData<int>();
 *           handleDrop(droppedIndex);
 *       }
 *       fst::EndDragDropTarget();
 *   }
 * 
 * IMPORTANT: Type strings must match exactly between source and target.
 *            Payload is delivered on mouse release over valid target.
 */

namespace fst {
class Context;


//=============================================================================
// Drag and Drop Flags
//=============================================================================

enum DragDropFlags_ {
    DragDropFlags_None                  = 0,
    DragDropFlags_SourceNoPreviewTooltip = 1 << 0,  // Don't show preview tooltip
    DragDropFlags_SourceNoDisableHover   = 1 << 1,  // Keep source widget hover
    DragDropFlags_SourceNoHoldToOpenOthers = 1 << 2, // Don't open others on hold
    DragDropFlags_AcceptNoHighlight      = 1 << 3,  // Don't highlight target
    DragDropFlags_AcceptNoPreviewTooltip = 1 << 4,  // Don't show accept preview
    DragDropFlags_CrossWindow            = 1 << 5,  // Allow drag between windows
};
using DragDropFlags = int;

//=============================================================================
// Drag Payload
//=============================================================================

/**
 * @brief Data transferred during drag and drop operation
 */
class IPlatformWindow;  // Forward declaration

struct DragPayload {
    std::string type;              ///< Payload type identifier (e.g., "FILE", "TREE_NODE")
    std::vector<uint8_t> data;     ///< Serialized payload data
    std::string displayText;       ///< Text to display during drag preview
    WidgetId sourceWidget = INVALID_WIDGET_ID;  ///< Source widget ID
    IPlatformWindow* sourceWindow = nullptr;    ///< Window where drag started (for cross-window)
    bool isDelivered = false;      ///< True if payload was accepted by target
    
    /// Helper to set typed data
    template<typename T>
    void setData(const T& value) {
        data.resize(sizeof(T));
        memcpy(data.data(), &value, sizeof(T));
    }
    
    /// Helper to get typed data
    template<typename T>
    T getData() const {
        T result{};
        if (data.size() >= sizeof(T)) {
            memcpy(&result, data.data(), sizeof(T));
        }
        return result;
    }
};

//=============================================================================
// Drag Drop State
//=============================================================================

/**
 * @brief Global state for drag and drop operations
 * 
 * @ai_hint For cross-window D&D: globalStartPos/globalCurrentPos are screen coordinates
 * obtained via GetGlobalCursorPos(). targetWindow tracks which window the cursor is over.
 */
struct DragDropState {
    bool active = false;                         ///< Is drag operation in progress
    DragPayload payload;                         ///< Current payload being dragged
    Vec2 startPos;                               ///< Mouse position when drag started (window-local)
    Vec2 currentPos;                             ///< Current mouse position (window-local)
    Vec2 globalStartPos;                         ///< Drag start position in screen coordinates
    Vec2 globalCurrentPos;                       ///< Current position in screen coordinates
    IPlatformWindow* targetWindow = nullptr;     ///< Window currently under cursor (for cross-window)
    WidgetId hoveredDropTarget = INVALID_WIDGET_ID;  ///< Currently hovered drop target
    bool isOverValidTarget = false;              ///< Is hovering over valid target
    float holdTimer = 0.0f;                      ///< Timer for hold-to-open
    
    void clear() {
        active = false;
        payload = DragPayload{};
        startPos = Vec2(0, 0);
        currentPos = Vec2(0, 0);
        globalStartPos = Vec2(0, 0);
        globalCurrentPos = Vec2(0, 0);
        targetWindow = nullptr;
        hoveredDropTarget = INVALID_WIDGET_ID;
        isOverValidTarget = false;
        holdTimer = 0.0f;
    }
};

//=============================================================================
// System File Drop
//=============================================================================

/**
 * @brief Callback for system file drop events
 */
using FileDropCallback = std::function<void(const std::vector<std::string>& paths)>;

//=============================================================================
// Drag and Drop API - Source Functions
//=============================================================================

/**
 * @brief Begin a drag source. Call when widget can initiate drag.
 * @param flags Optional flags to customize behavior
 * @return true if drag is active from this source
 */
bool BeginDragDropSource(Context& ctx, DragDropFlags flags = DragDropFlags_None);
bool BeginDragDropSource(DragDropFlags flags = DragDropFlags_None);

/**
 * @brief Set the payload for current drag operation
 * @param type Payload type identifier for matching with targets
 * @param data Pointer to data to copy
 * @param size Size of data in bytes
 * @return true if payload was set
 */
bool SetDragDropPayload(const std::string& type, const void* data, size_t size);

/**
 * @brief Set display text for drag preview
 */
void SetDragDropDisplayText(const std::string& text);

/**
 * @brief End drag source block
 */
void EndDragDropSource();

//=============================================================================
// Drag and Drop API - Target Functions
//=============================================================================

/**
 * @brief Begin a drop target. Call when widget can accept drops.
 * @return true if something is being dragged over this target
 */
bool BeginDragDropTarget(Context& ctx);
bool BeginDragDropTarget();

/**
 * @brief Begin a drop target with explicit bounds.
 * @param targetRect The rectangle to check for mouse overlap
 * @return true if something is being dragged over this target
 */
bool BeginDragDropTarget(Context& ctx, const Rect& targetRect);
bool BeginDragDropTarget(const Rect& targetRect);

/**
 * @brief Accept a payload of specific type
 * @param type Payload type to accept (must match SetDragDropPayload type)
 * @param flags Optional flags
 * @return Pointer to payload if accepted, nullptr otherwise
 */
const DragPayload* AcceptDragDropPayload(Context& ctx, const std::string& type, 
                                          DragDropFlags flags = DragDropFlags_None);
const DragPayload* AcceptDragDropPayload(const std::string& type, 
                                          DragDropFlags flags = DragDropFlags_None);

/**
 * @brief End drop target block
 */
void EndDragDropTarget();

//=============================================================================
// Drag and Drop API - Query Functions
//=============================================================================

/**
 * @brief Check if a drag operation is in progress
 */
bool IsDragDropActive();

/**
 * @brief Get the current drag payload (if any)
 */
const DragPayload* GetDragDropPayload();

/**
 * @brief Cancel current drag operation
 */
void CancelDragDrop();

/**
 * @brief Ends the drag and drop frame, performing cleanup.
 *        Called internally by the Context.
 */
void EndDragDropFrame();

} // namespace fst
