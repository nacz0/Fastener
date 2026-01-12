#pragma once

#include "fastener/core/types.h"
#include "fastener/core/input.h"
#include "fastener/platform/platform_interface.h"
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace fst {

// Forward declarations
class Renderer;
class Font;
class Theme;

//=============================================================================
// Window Configuration
//=============================================================================
struct WindowConfig {
    std::string title = "Fastener Application";
    int width = 1280;
    int height = 720;
    bool resizable = true;
    bool decorated = true;       // Has title bar and borders
    bool maximized = false;
    bool vsync = true;
    int msaaSamples = 4;         // Anti-aliasing samples (0 to disable)
    bool highDPI = true;         // Enable high-DPI support
};

//=============================================================================
// Window Events
//=============================================================================
struct WindowResizeEvent {
    int width;
    int height;
};

struct WindowCloseEvent {};

struct WindowFocusEvent {
    bool focused;
};

struct WindowMoveEvent {
    int x;
    int y;
};

//=============================================================================
// Window Class
//=============================================================================
class Window : public IPlatformWindow {
public:
    Window();
    explicit Window(const std::string& title, int width = 1280, int height = 720);
    explicit Window(const WindowConfig& config);
    ~Window();
    
    // Non-copyable, movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    
    // Lifecycle
    bool create(const WindowConfig& config);
    void destroy();
    bool isOpen() const override;
    void close() override;
    
    // Event handling
    void pollEvents() override;
    void waitEvents() override;
    
    // Rendering
    void swapBuffers() override;
    void makeContextCurrent() override;
    
    // Properties
    Vec2 size() const override;
    Vec2 framebufferSize() const override;  // May differ from size on high-DPI
    float dpiScale() const override;
    
    int width() const override;
    int height() const override;
    
    void setTitle(const std::string& title) override;
    void setSize(int width, int height) override;
    void setMinSize(int minWidth, int minHeight);
    void setMaxSize(int maxWidth, int maxHeight);
    void setPosition(int x, int y) override;
    
    void minimize() override;
    void maximize() override;
    void restore() override;
    void focus() override;
    
    bool isMinimized() const override;
    bool isMaximized() const override;
    bool isFocused() const override;
    
    // Cursor
    void setCursor(Cursor cursor) override;
    void hideCursor() override;
    void showCursor() override;
    
    // Clipboard
    std::string getClipboardText() const override;
    void setClipboardText(const std::string& text) override;
    
    // Callbacks
    using ResizeCallback = std::function<void(const WindowResizeEvent&)>;
    using CloseCallback = std::function<void(const WindowCloseEvent&)>;
    using FocusCallback = std::function<void(const WindowFocusEvent&)>;
    using RefreshCallback = std::function<void()>;
    using FileDropCallback = std::function<void(const std::vector<std::string>& paths)>;
    
    void setResizeCallback(ResizeCallback callback);
    void setCloseCallback(CloseCallback callback);
    void setFocusCallback(FocusCallback callback);
    void setRefreshCallback(RefreshCallback callback);
    void setFileDropCallback(FileDropCallback callback);
    
    // File drop query (alternative to callback)
    const std::vector<std::string>& droppedFiles() const;
    void clearDroppedFiles();
    
    // Input access
    InputState& input() override;
    const InputState& input() const override;
    
    // Screen position (for cross-window drag & drop)
    Vec2 screenPosition() const;
    Vec2 position() const;
    
    // Shared OpenGL context (for multi-window)
    /**
     * @brief Create window with shared OpenGL context.
     * @param config Window configuration
     * @param shareWindow Window to share GL context with (nullptr for new context)
     * @return true if window was created successfully
     */
    bool createWithSharedContext(const WindowConfig& config, Window* shareWindow);
    
    /** @brief Get the OpenGL context handle (for sharing). */
    void* glContext() const;
    
    // Platform handle (use with caution)
    void* nativeHandle() const override;
    
    // Implementation (public for MSVC compatibility)
    struct Impl;
    
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace fst
