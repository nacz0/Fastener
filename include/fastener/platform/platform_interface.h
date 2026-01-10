#pragma once

#include "fastener/core/types.h"
#include "fastener/core/input.h"
#include <string>
#include <vector>
#include <functional>

namespace fst {

struct WindowConfig;
struct WindowResizeEvent;
struct WindowCloseEvent;
struct WindowFocusEvent;

/**
 * @brief Interface for platform-specific window implementations.
 */
class IPlatformWindow {
public:
    virtual ~IPlatformWindow() = default;

    virtual bool isOpen() const = 0;
    virtual void close() = 0;
    
    virtual void pollEvents() = 0;
    virtual void waitEvents() = 0;
    
    virtual void swapBuffers() = 0;
    virtual void makeContextCurrent() = 0;
    
    virtual Vec2 size() const = 0;
    virtual Vec2 framebufferSize() const = 0;
    virtual float dpiScale() const = 0;
    
    virtual int width() const = 0;
    virtual int height() const = 0;
    
    virtual void setTitle(const std::string& title) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void setPosition(int x, int y) = 0;
    
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void focus() = 0;
    
    virtual bool isMinimized() const = 0;
    virtual bool isMaximized() const = 0;
    virtual bool isFocused() const = 0;
    
    virtual void setCursor(Cursor cursor) = 0;
    virtual void hideCursor() = 0;
    virtual void showCursor() = 0;
    
    virtual std::string getClipboardText() const = 0;
    virtual void setClipboardText(const std::string& text) = 0;
    
    virtual InputState& input() = 0;
    virtual const InputState& input() const = 0;
    
    virtual void* nativeHandle() const = 0;
};

} // namespace fst
