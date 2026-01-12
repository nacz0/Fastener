#pragma once

/**
 * @file cursor_utils.h
 * @brief Platform-abstracted cursor utilities for cross-window operations.
 * 
 * This module provides global cursor position tracking independent of per-window
 * input state. Essential for cross-window drag and drop operations where the
 * cursor may leave one window and enter another.
 * 
 * @ai_hint
 * Platform-specific implementations:
 * - Win32: Uses GetCursorPos/ScreenToClient (see win32/cursor_utils.cpp)
 * - macOS: Would use CGEventGetLocation/convertPoint (future)
 * - X11: Would use XQueryPointer (future)
 * 
 * To add a new platform:
 * 1. Create src/platform/<platform>/cursor_utils.cpp
 * 2. Implement GetGlobalCursorPos() and ScreenToWindowLocal()
 * 3. Update CMakeLists.txt to include the new source
 */

#include "fastener/core/types.h"

namespace fst {

// Forward declarations
class Window;
class IPlatformWindow;

//=============================================================================
// Global Cursor Position API
//=============================================================================

/**
 * @brief Get the cursor position in screen (global) coordinates.
 * 
 * This function queries the OS for the current cursor position regardless
 * of which window (if any) has focus or contains the cursor.
 * 
 * @return Cursor position in screen coordinates (pixels from top-left of primary monitor)
 */
Vec2 GetGlobalCursorPos();

/**
 * @brief Convert screen coordinates to window-local coordinates.
 * 
 * Transforms a position from global screen space to the local coordinate
 * system of the specified window's client area.
 * 
 * @param window The target window
 * @param screenPos Position in screen coordinates
 * @return Position relative to the window's client area top-left
 */
Vec2 ScreenToWindowLocal(const IPlatformWindow& window, Vec2 screenPos);

/**
 * @brief Convert window-local coordinates to screen coordinates.
 * 
 * @param window The source window
 * @param localPos Position in window-local coordinates
 * @return Position in screen coordinates
 */
Vec2 WindowLocalToScreen(const IPlatformWindow& window, Vec2 localPos);

/**
 * @brief Get cursor position relative to a specific window.
 * 
 * Convenience function that combines GetGlobalCursorPos() and ScreenToWindowLocal().
 * 
 * @param window The target window
 * @return Cursor position in window-local coordinates
 */
Vec2 GetCursorPosInWindow(const IPlatformWindow& window);

} // namespace fst
