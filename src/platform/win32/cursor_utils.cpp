/**
 * @file cursor_utils.cpp
 * @brief Win32 implementation of cursor utilities.
 */

#ifdef _WIN32

#include "fastener/platform/cursor_utils.h"
#include "fastener/platform/platform_interface.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace fst {

Vec2 GetGlobalCursorPos() {
    POINT pt;
    if (::GetCursorPos(&pt)) {
        return Vec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }
    return Vec2::zero();
}

Vec2 ScreenToWindowLocal(const IPlatformWindow& window, Vec2 screenPos) {
    HWND hwnd = static_cast<HWND>(window.nativeHandle());
    if (!hwnd) {
        return screenPos;
    }
    
    POINT pt;
    pt.x = static_cast<LONG>(screenPos.x);
    pt.y = static_cast<LONG>(screenPos.y);
    
    if (::ScreenToClient(hwnd, &pt)) {
        return Vec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }
    return screenPos;
}

Vec2 WindowLocalToScreen(const IPlatformWindow& window, Vec2 localPos) {
    HWND hwnd = static_cast<HWND>(window.nativeHandle());
    if (!hwnd) {
        return localPos;
    }
    
    POINT pt;
    pt.x = static_cast<LONG>(localPos.x);
    pt.y = static_cast<LONG>(localPos.y);
    
    if (::ClientToScreen(hwnd, &pt)) {
        return Vec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }
    return localPos;
}

Vec2 GetCursorPosInWindow(const IPlatformWindow& window) {
    // If no native handle (e.g., test stub), fallback to window's input state
    HWND hwnd = static_cast<HWND>(window.nativeHandle());
    if (!hwnd) {
        return window.input().mousePos();
    }
    
    Vec2 globalPos = GetGlobalCursorPos();
    return ScreenToWindowLocal(window, globalPos);
}

} // namespace fst

#endif // _WIN32
