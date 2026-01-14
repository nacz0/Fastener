/**
 * @file x11_cursor_utils.cpp
 * @brief X11/Linux implementation of cursor utilities.
 */

#if defined(__linux__) && !defined(__ANDROID__)

#include "fastener/platform/cursor_utils.h"
#include "fastener/platform/platform_interface.h"
#include <X11/Xlib.h>

namespace fst {

// Thread-local display connection for cursor queries
static thread_local Display* s_display = nullptr;

static Display* getDisplay() {
    if (!s_display) {
        s_display = XOpenDisplay(nullptr);
    }
    return s_display;
}

Vec2 GetGlobalCursorPos() {
    Display* display = getDisplay();
    if (!display) return Vec2::zero();
    
    ::Window root = DefaultRootWindow(display);
    ::Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    
    if (XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask)) {
        return Vec2(static_cast<float>(root_x), static_cast<float>(root_y));
    }
    return Vec2::zero();
}

Vec2 ScreenToWindowLocal(const IPlatformWindow& window, Vec2 screenPos) {
    Display* display = getDisplay();
    ::Window xwin = reinterpret_cast<::Window>(window.nativeHandle());
    if (!display || !xwin) {
        return screenPos;
    }
    
    ::Window root = DefaultRootWindow(display);
    int destX, destY;
    ::Window child;
    
    if (XTranslateCoordinates(display, root, xwin, 
                              static_cast<int>(screenPos.x), static_cast<int>(screenPos.y),
                              &destX, &destY, &child)) {
        return Vec2(static_cast<float>(destX), static_cast<float>(destY));
    }
    return screenPos;
}

Vec2 WindowLocalToScreen(const IPlatformWindow& window, Vec2 localPos) {
    Display* display = getDisplay();
    ::Window xwin = reinterpret_cast<::Window>(window.nativeHandle());
    if (!display || !xwin) {
        return localPos;
    }
    
    ::Window root = DefaultRootWindow(display);
    int destX, destY;
    ::Window child;
    
    if (XTranslateCoordinates(display, xwin, root,
                              static_cast<int>(localPos.x), static_cast<int>(localPos.y),
                              &destX, &destY, &child)) {
        return Vec2(static_cast<float>(destX), static_cast<float>(destY));
    }
    return localPos;
}

Vec2 GetCursorPosInWindow(const IPlatformWindow& window) {
    // If no native handle (e.g., test stub), fallback to window's input state
    ::Window xwin = reinterpret_cast<::Window>(window.nativeHandle());
    if (!xwin) {
        return window.input().mousePos();
    }
    
    Vec2 globalPos = GetGlobalCursorPos();
    return ScreenToWindowLocal(window, globalPos);
}

} // namespace fst

#endif // __linux__
