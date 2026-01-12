#pragma once

#include "fastener/platform/window.h"
#include <vector>
#include <memory>
#include <functional>

namespace fst {

class Context;

/**
 * @brief Manages multiple windows with shared OpenGL context.
 * 
 * WindowManager provides centralized management for multi-window applications.
 * All windows share the same OpenGL context (created by the first window),
 * allowing textures and fonts to be used across windows.
 * 
 * Usage:
 * @code
 * WindowManager wm;
 * Window* main = wm.createWindow({.title = "Main", .width = 1280, .height = 720});
 * Window* tools = wm.createWindow({.title = "Tools", .width = 400, .height = 600});
 * 
 * Context ctx;
 * ctx.loadFont("arial.ttf", 14.0f);  // Loaded once, used everywhere
 * 
 * while (wm.anyWindowOpen()) {
 *     wm.pollAllEvents();
 *     
 *     for (auto* window : wm.windows()) {
 *         if (!window->isOpen()) continue;
 *         window->makeContextCurrent();
 *         ctx.beginFrame(*window);
 *         // ... render UI
 *         ctx.endFrame();
 *         window->swapBuffers();
 *     }
 * }
 * @endcode
 */
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    // Non-copyable
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    
    /**
     * @brief Create a new window with shared OpenGL context.
     * @param config Window configuration
     * @return Pointer to created window (owned by WindowManager)
     */
    Window* createWindow(const WindowConfig& config);
    
    /**
     * @brief Create a child window (e.g., for undocked panels).
     * @param config Window configuration
     * @param parent Parent window for positioning hints
     * @return Pointer to created window
     */
    Window* createChildWindow(const WindowConfig& config, Window* parent = nullptr);
    
    /**
     * @brief Destroy a window and remove it from management.
     * @param window Window to destroy (must be owned by this manager)
     */
    void destroyWindow(Window* window);
    
    /**
     * @brief Get all managed windows.
     */
    const std::vector<Window*>& windows() const;
    
    /**
     * @brief Get the main (first created) window.
     */
    Window* mainWindow() const;
    
    /**
     * @brief Get currently focused window, or nullptr.
     */
    Window* focusedWindow() const;
    
    /**
     * @brief Get window at screen position (for cross-window drag & drop).
     * @param screenPos Position in screen coordinates
     * @return Window under position, or nullptr
     */
    Window* windowAtPosition(Vec2 screenPos) const;
    
    /**
     * @brief Check if any window is still open.
     */
    bool anyWindowOpen() const;
    
    /**
     * @brief Poll events for all windows.
     */
    void pollAllEvents();
    
    /**
     * @brief Get count of open windows.
     */
    size_t windowCount() const;
    
    // Cross-window drag & drop support
    
    /**
     * @brief Notify that a drag operation started in a window.
     */
    void beginCrossWindowDrag(Window* sourceWindow);
    
    /**
     * @brief Notify that a drag operation ended.
     */
    void endCrossWindowDrag();
    
    /**
     * @brief Check if a cross-window drag is active.
     */
    bool isCrossWindowDragActive() const;
    
    /**
     * @brief Get the source window of current drag operation.
     */
    Window* dragSourceWindow() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fst
