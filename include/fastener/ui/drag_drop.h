#pragma once

#include "fastener/core/types.h"
#include <string>
#include <vector>
#include <functional>

namespace fst {

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
};
using DragDropFlags = int;

//=============================================================================
// Drag Payload
//=============================================================================

/**
 * @brief Data transferred during drag and drop operation
 */
struct DragPayload {
    std::string type;              ///< Payload type identifier (e.g., "FILE", "TREE_NODE")
    std::vector<uint8_t> data;     ///< Serialized payload data
    std::string displayText;       ///< Text to display during drag preview
    WidgetId sourceWidget = INVALID_WIDGET_ID;  ///< Source widget ID
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
 */
struct DragDropState {
    bool active = false;                         ///< Is drag operation in progress
    DragPayload payload;                         ///< Current payload being dragged
    Vec2 startPos;                               ///< Mouse position when drag started
    Vec2 currentPos;                             ///< Current mouse position
    WidgetId hoveredDropTarget = INVALID_WIDGET_ID;  ///< Currently hovered drop target
    bool isOverValidTarget = false;              ///< Is hovering over valid target
    float holdTimer = 0.0f;                      ///< Timer for hold-to-open
    
    void clear() {
        active = false;
        payload = DragPayload{};
        startPos = Vec2(0, 0);
        currentPos = Vec2(0, 0);
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
bool BeginDragDropTarget();

/**
 * @brief Accept a payload of specific type
 * @param type Payload type to accept (must match SetDragDropPayload type)
 * @param flags Optional flags
 * @return Pointer to payload if accepted, nullptr otherwise
 */
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

} // namespace fst
