#include "fastener/core/context.h"
#include "fastener/platform/window.h"
#include "fastener/graphics/renderer.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include <vector>
#include <chrono>

namespace fst {

Context* Context::s_current = nullptr;

struct Context::Impl {
    // Components
    Renderer renderer;
    DrawList drawList;
    LayoutContext layout;
    
    // Theme
    Theme theme = Theme::dark();
    
    // Fonts
    std::unique_ptr<Font> defaultFont;
    Font* currentFont = nullptr;
    
    // Input
    InputState* inputState = nullptr;
    Window* currentWindow = nullptr;
    
    // Time
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    
    // Widget state
    WidgetId focusedWidget = INVALID_WIDGET_ID;
    WidgetId hoveredWidget = INVALID_WIDGET_ID;
    WidgetId activeWidget = INVALID_WIDGET_ID;
    
    // ID stack
    std::vector<WidgetId> idStack;
    
    Impl() {
        startTime = std::chrono::steady_clock::now();
        lastFrameTime = startTime;
        idStack.push_back(0);
    }
};

Context::Context() : m_impl(std::make_unique<Impl>()) {
    m_impl->renderer.init();
}

Context::~Context() {
    if (s_current == this) {
        s_current = nullptr;
    }
}

void Context::beginFrame(Window& window) {
    s_current = this;
    m_impl->currentWindow = &window;
    m_impl->inputState = &window.input();
    m_impl->inputState->onResize(static_cast<float>(window.width()), static_cast<float>(window.height()));
    
    // Calculate delta time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_impl->lastFrameTime);
    m_impl->deltaTime = elapsed.count() / 1000000.0f;
    m_impl->lastFrameTime = now;
    
    auto total = std::chrono::duration_cast<std::chrono::microseconds>(now - m_impl->startTime);
    m_impl->totalTime = total.count() / 1000000.0f;
    
    // Clear draw list
    m_impl->drawList.clear();
    
    // Begin rendering
    Vec2 fbSize = window.framebufferSize();
    m_impl->renderer.beginFrame(
        static_cast<int>(fbSize.x), 
        static_cast<int>(fbSize.y), 
        window.dpiScale()
    );
    
    // Push fullscreen clip rect
    m_impl->drawList.pushClipRectFullScreen(window.size());
    
    // Begin root layout container
    m_impl->layout.beginContainer(
        Rect(0, 0, window.width(), window.height()),
        LayoutDirection::Vertical
    );
    
    // Reset hovered widget
    m_impl->hoveredWidget = INVALID_WIDGET_ID;
}

void Context::endFrame() {
    // End layout
    m_impl->layout.endContainer();
    
    // Pop clip rect
    m_impl->drawList.popClipRect();
    
    // Render
    m_impl->renderer.render(m_impl->drawList);
    m_impl->renderer.endFrame();
    
    s_current = nullptr;
}

void Context::setTheme(const Theme& theme) {
    m_impl->theme = theme;
}

Theme& Context::theme() {
    return m_impl->theme;
}

const Theme& Context::theme() const {
    return m_impl->theme;
}

bool Context::loadFont(const std::string& path, float size) {
    m_impl->defaultFont = std::make_unique<Font>();
    if (!m_impl->defaultFont->loadFromFile(path, size)) {
        m_impl->defaultFont.reset();
        return false;
    }
    m_impl->currentFont = m_impl->defaultFont.get();
    return true;
}

Font* Context::font() const {
    return m_impl->currentFont;
}

Font* Context::defaultFont() const {
    return m_impl->defaultFont.get();
}

InputState& Context::input() {
    return *m_impl->inputState;
}

const InputState& Context::input() const {
    return *m_impl->inputState;
}

DrawList& Context::drawList() {
    return m_impl->drawList;
}

Renderer& Context::renderer() {
    return m_impl->renderer;
}

Window& Context::window() {
    return *m_impl->currentWindow;
}

float Context::deltaTime() const {
    return m_impl->deltaTime;
}

float Context::time() const {
    return m_impl->totalTime;
}

WidgetId Context::getFocusedWidget() const {
    return m_impl->focusedWidget;
}

void Context::setFocusedWidget(WidgetId id) {
    m_impl->focusedWidget = id;
}

void Context::clearFocus() {
    m_impl->focusedWidget = INVALID_WIDGET_ID;
}

WidgetId Context::getHoveredWidget() const {
    return m_impl->hoveredWidget;
}

void Context::setHoveredWidget(WidgetId id) {
    m_impl->hoveredWidget = id;
}

WidgetId Context::getActiveWidget() const {
    return m_impl->activeWidget;
}

void Context::setActiveWidget(WidgetId id) {
    m_impl->activeWidget = id;
}

void Context::clearActiveWidget() {
    m_impl->activeWidget = INVALID_WIDGET_ID;
}

void Context::pushId(WidgetId id) {
    WidgetId combined = combineIds(m_impl->idStack.back(), id);
    m_impl->idStack.push_back(combined);
}

void Context::pushId(const char* str) {
    pushId(hashString(str));
}

void Context::pushId(int idx) {
    pushId(static_cast<WidgetId>(idx));
}

void Context::popId() {
    if (m_impl->idStack.size() > 1) {
        m_impl->idStack.pop_back();
    }
}

WidgetId Context::currentId() const {
    return m_impl->idStack.back();
}

WidgetId Context::makeId(const char* str) const {
    return combineIds(m_impl->idStack.back(), hashString(str));
}

WidgetId Context::makeId(int idx) const {
    return combineIds(m_impl->idStack.back(), static_cast<WidgetId>(idx));
}

Context* Context::current() {
    return s_current;
}

} // namespace fst
