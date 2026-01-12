#include "fastener/platform/window_manager.h"
#include <algorithm>

namespace fst {

struct WindowManager::Impl {
    std::vector<std::unique_ptr<Window>> windows;
    Window* mainWindow = nullptr;
    
    // Cross-window drag state
    bool crossWindowDragActive = false;
    Window* dragSourceWindow = nullptr;
};

WindowManager::WindowManager() : m_impl(std::make_unique<Impl>()) {}

WindowManager::~WindowManager() = default;

Window* WindowManager::createWindow(const WindowConfig& config) {
    auto window = std::make_unique<Window>();
    
    if (m_impl->windows.empty()) {
        // First window - create with new GL context
        if (!window->create(config)) {
            return nullptr;
        }
        m_impl->mainWindow = window.get();
    } else {
        // Subsequent windows - share GL context with main window
        if (!window->createWithSharedContext(config, m_impl->mainWindow)) {
            return nullptr;
        }
    }
    
    Window* ptr = window.get();
    m_impl->windows.push_back(std::move(window));
    return ptr;
}

Window* WindowManager::createChildWindow(const WindowConfig& config, Window* parent) {
    auto window = std::make_unique<Window>();
    
    Window* shareWith = parent ? parent : m_impl->mainWindow;
    if (!shareWith) {
        // No parent and no main window - create as first window
        if (!window->create(config)) {
            return nullptr;
        }
        m_impl->mainWindow = window.get();
    } else {
        if (!window->createWithSharedContext(config, shareWith)) {
            return nullptr;
        }
    }
    
    Window* ptr = window.get();
    m_impl->windows.push_back(std::move(window));
    return ptr;
}

void WindowManager::destroyWindow(Window* window) {
    if (!window) return;
    
    auto it = std::find_if(m_impl->windows.begin(), m_impl->windows.end(),
        [window](const std::unique_ptr<Window>& w) { return w.get() == window; });
    
    if (it != m_impl->windows.end()) {
        if (m_impl->mainWindow == window) {
            // Main window destroyed - pick new main if available
            m_impl->mainWindow = (m_impl->windows.size() > 1) 
                ? (it == m_impl->windows.begin() ? m_impl->windows[1].get() : m_impl->windows[0].get())
                : nullptr;
        }
        m_impl->windows.erase(it);
    }
}

const std::vector<Window*>& WindowManager::windows() const {
    // Note: This returns pointers, but we need to build the vector
    // For now we store and return a cached version
    static thread_local std::vector<Window*> windowPtrs;
    windowPtrs.clear();
    windowPtrs.reserve(m_impl->windows.size());
    for (const auto& w : m_impl->windows) {
        windowPtrs.push_back(w.get());
    }
    return windowPtrs;
}

Window* WindowManager::mainWindow() const {
    return m_impl->mainWindow;
}

Window* WindowManager::focusedWindow() const {
    for (const auto& w : m_impl->windows) {
        if (w->isFocused()) {
            return w.get();
        }
    }
    return nullptr;
}

Window* WindowManager::windowAtPosition(Vec2 screenPos) const {
    // Check windows in reverse order (topmost first - assuming later created are on top)
    for (auto it = m_impl->windows.rbegin(); it != m_impl->windows.rend(); ++it) {
        const auto& w = *it;
        if (!w->isOpen() || w->isMinimized()) continue;
        
        Vec2 pos = w->screenPosition();
        Vec2 size = w->size();
        Rect windowRect(pos.x, pos.y, size.x, size.y);
        
        if (windowRect.contains(screenPos)) {
            return w.get();
        }
    }
    return nullptr;
}

bool WindowManager::anyWindowOpen() const {
    for (const auto& w : m_impl->windows) {
        if (w->isOpen()) return true;
    }
    return false;
}

void WindowManager::pollAllEvents() {
    for (const auto& w : m_impl->windows) {
        if (w->isOpen()) {
            w->pollEvents();
        }
    }
}

size_t WindowManager::windowCount() const {
    return m_impl->windows.size();
}

void WindowManager::beginCrossWindowDrag(Window* sourceWindow) {
    m_impl->crossWindowDragActive = true;
    m_impl->dragSourceWindow = sourceWindow;
}

void WindowManager::endCrossWindowDrag() {
    m_impl->crossWindowDragActive = false;
    m_impl->dragSourceWindow = nullptr;
}

bool WindowManager::isCrossWindowDragActive() const {
    return m_impl->crossWindowDragActive;
}

Window* WindowManager::dragSourceWindow() const {
    return m_impl->dragSourceWindow;
}

} // namespace fst
