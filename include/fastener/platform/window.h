#pragma once

#include "fastener/core/types.h"
#include "fastener/core/input.h"
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
class Window {
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
    bool isOpen() const;
    void close();
    
    // Event handling
    void pollEvents();
    void waitEvents();
    
    // Rendering
    void swapBuffers();
    void makeContextCurrent();
    
    // Properties
    Vec2 size() const;
    Vec2 framebufferSize() const;  // May differ from size on high-DPI
    float dpiScale() const;
    
    int width() const;
    int height() const;
    
    void setTitle(const std::string& title);
    void setSize(int width, int height);
    void setMinSize(int minWidth, int minHeight);
    void setMaxSize(int maxWidth, int maxHeight);
    void setPosition(int x, int y);
    
    void minimize();
    void maximize();
    void restore();
    void focus();
    
    bool isMinimized() const;
    bool isMaximized() const;
    bool isFocused() const;
    
    // Cursor
    void setCursor(Cursor cursor);
    void hideCursor();
    void showCursor();
    
    // Clipboard
    std::string getClipboardText() const;
    void setClipboardText(const std::string& text);
    
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
    InputState& input();
    const InputState& input() const;
    
    // Platform handle (use with caution)
    void* nativeHandle() const;
    
    // Implementation (public for MSVC compatibility)
    struct Impl;
    
private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace fst
