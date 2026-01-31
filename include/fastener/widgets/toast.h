#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <string>
#include <string_view>

/**
 * @file toast.h
 * @brief Toast/Notification widget for displaying temporary messages.
 * 
 * @ai_hint Toasts are non-blocking notifications that appear in a corner of the screen.
 *          They automatically dismiss after a configurable duration or can be manually closed.
 *          Use ShowToast() to queue notifications, and call RenderToasts() once per frame.
 * 
 * @example
 *   // Show different types of toasts
 *   if (fst::Button(ctx, "Save")) {
 *       fst::ShowToast(ctx, "File saved successfully!", {.type = fst::ToastType::Success});
 *   }
 *   if (error) {
 *       fst::ShowToast(ctx, "Error", "Failed to connect to server", 
 *                      {.type = fst::ToastType::Error, .duration = 5.0f});
 *   }
 *   
 *   // Call once at end of frame to render all toasts
 *   fst::RenderToasts(ctx);
 */

namespace fst {

class Context;

//=============================================================================
// Toast Type - Semantic styling for toast notifications
//=============================================================================
enum class ToastType {
    Info,       ///< Blue - general information
    Success,    ///< Green - operation succeeded
    Warning,    ///< Orange/Yellow - caution
    Error       ///< Red - something went wrong
};

//=============================================================================
// Toast Options - Configuration for individual toasts
//=============================================================================
struct ToastOptions {
    ToastType type = ToastType::Info;   ///< Semantic type for color styling
    float duration = 3.0f;              ///< Seconds before auto-dismiss (0 = manual only)
    bool dismissible = true;            ///< Show close button
    float width = 300.0f;               ///< Toast width in pixels
    
    // Convenience constructor for quick type specification
    ToastOptions() = default;
    explicit ToastOptions(ToastType t) : type(t) {}
};

//=============================================================================
// Toast Position - Where toasts appear on screen
//=============================================================================
enum class ToastPosition {
    TopRight,       ///< Default - top right corner
    TopLeft,
    BottomRight,
    BottomLeft,
    TopCenter,
    BottomCenter
};

//=============================================================================
// Toast Container Options - Global toast rendering settings
//=============================================================================
struct ToastContainerOptions {
    ToastPosition position = ToastPosition::TopRight;   ///< Screen position for toasts
    float margin = 16.0f;       ///< Distance from screen edge
    float spacing = 8.0f;       ///< Gap between stacked toasts
    int maxVisible = 5;         ///< Maximum toasts shown at once (older ones queue)
};

//=============================================================================
// Toast API Functions
//=============================================================================

/**
 * @brief Display a simple toast notification.
 * @param ctx The context.
 * @param message The message to display.
 * @param options Toast configuration options.
 * @return Toast ID that can be used with DismissToast().
 */
int ShowToast(Context& ctx, std::string_view message, const ToastOptions& options = {});

/**
 * @brief Display a toast with title and message body.
 * @param ctx The context.
 * @param title Bold title text.
 * @param message Body message text.
 * @param options Toast configuration options.
 * @return Toast ID that can be used with DismissToast().
 */
int ShowToast(Context& ctx, std::string_view title, std::string_view message, const ToastOptions& options = {});

/**
 * @brief Render all active toasts. Call once per frame, typically at end.
 * @param ctx The context.
 * @param options Container/positioning options.
 */
void RenderToasts(Context& ctx, const ToastContainerOptions& options = {});

/**
 * @brief Dismiss a specific toast by ID.
 * @param toastId The ID returned by ShowToast().
 */
void DismissToast(int toastId);

/**
 * @brief Dismiss all active toasts immediately.
 */
void DismissAllToasts();

//=============================================================================
// Internal State (for testing)
//=============================================================================
namespace internal {

/// @brief Get current number of toasts in queue (for testing).
int getToastCount();

} // namespace internal

} // namespace fst
