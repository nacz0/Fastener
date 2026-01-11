#include "fastener/core/context.h"
#include "fastener/core/log.h"
#include "fastener/platform/platform_interface.h"
#include "fastener/platform/window.h"
#include "fastener/graphics/renderer.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/theme.h"
#include "fastener/ui/layout.h"
#include "fastener/ui/dock_context.h"
#include "fastener/ui/drag_drop.h"
#include <vector>
#include <chrono>
#include <algorithm>

namespace fst {

// Thread-local context stack for multi-window/DI support
static thread_local std::vector<Context*> s_contextStack;
static IDrawList* s_testDrawList = nullptr;

struct Context::Impl {
    // Components
    Renderer renderer;
    DrawList drawList;
    LayoutContext layout;
    DockContext dockContext;
    
    // Theme
    Theme theme = Theme::dark();
    
    // Fonts
    std::unique_ptr<Font> defaultFont;
    Font* currentFont = nullptr;
    
    // Input
    InputState* inputState = nullptr;
    IPlatformWindow* currentWindow = nullptr;
    
    // Time
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    
    // Widget state
    WidgetId focusedWidget = INVALID_WIDGET_ID;
    WidgetId hoveredWidget = INVALID_WIDGET_ID;
    WidgetId activeWidget = INVALID_WIDGET_ID;
    WidgetId lastWidgetId = INVALID_WIDGET_ID;
    Rect lastWidgetBounds;
    
    // ID stack
    std::vector<WidgetId> idStack;
    
    // Deferred rendering
    std::deque<std::function<void()>> postRenderCommands;
    
    // Menu state (moved from global variables in menu.cpp)
    Context::MenuState menuState;
    
    Impl() {
        startTime = std::chrono::steady_clock::now();
        lastFrameTime = startTime;
        idStack.push_back(0);
    }
    
    // Floating window occlusion
    std::vector<Rect> currentFloatingRects;
    std::vector<Rect> prevFloatingRects;
    bool rendererInitialized = false;
};

Context::Context(bool initializeRenderer) : m_impl(std::make_unique<Impl>()) {
    if (initializeRenderer) {
        m_impl->renderer.init();
        m_impl->rendererInitialized = true;
    }
}

Context::~Context() {
    // Remove this context from the stack if it's there
    auto it = std::find(s_contextStack.begin(), s_contextStack.end(), this);
    if (it != s_contextStack.end()) {
        s_contextStack.erase(it);
    }
}

void Context::beginFrame(IPlatformWindow& window) {
    pushContext(this);
    m_impl->currentWindow = &window;
    m_impl->inputState = &window.input();
    m_impl->inputState->beginFrame();
    m_impl->inputState->onResize(static_cast<float>(window.width()), static_cast<float>(window.height()));
    
    // Calculate delta time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_impl->lastFrameTime);
    m_impl->deltaTime = elapsed.count() / 1000000.0f;
    m_impl->lastFrameTime = now;
    
    auto total = std::chrono::duration_cast<std::chrono::microseconds>(now - m_impl->startTime);
    m_impl->totalTime = total.count() / 1000000.0f;
    
    // Clear draw list
    m_impl->inputState->setFrameTime(m_impl->totalTime);
    m_impl->drawList.clear();
    
    // Begin rendering
    if (m_impl->rendererInitialized) {
        Vec2 fbSize = window.framebufferSize();
        m_impl->renderer.beginFrame(
            static_cast<int>(fbSize.x), 
            static_cast<int>(fbSize.y), 
            window.dpiScale()
        );
    }
    
    // Push fullscreen clip rect
    m_impl->drawList.pushClipRectFullScreen(window.size());
    
    // Begin root layout container
    m_impl->layout.beginContainer(
        Rect(0.0f, 0.0f, static_cast<float>(window.width()), static_cast<float>(window.height())),
        LayoutDirection::Vertical
    );
    
    // Reset cursor and hovered widget
    window.setCursor(Cursor::Arrow);
    m_impl->hoveredWidget = INVALID_WIDGET_ID;
    
    // Swap floating rects for occlusion testing
    m_impl->prevFloatingRects = m_impl->currentFloatingRects;
    m_impl->currentFloatingRects.clear();
    
    // Begin docking frame
    m_impl->dockContext.beginFrame();
}

void Context::endFrame() {
    // End docking frame
    m_impl->dockContext.endFrame();
    
    // End layout
    m_impl->layout.endContainer();
    
    // Pop clip rect
    m_impl->drawList.popClipRect();
    
    // Execute deferred commands (popups, etc.)
    for (const auto& cmd : m_impl->postRenderCommands) {
        cmd();
    }
    m_impl->postRenderCommands.clear();

    // Cleanup drag and drop state if needed (and render preview)
    EndDragDropFrame();
    
    // Render
    m_impl->drawList.mergeLayers();
    if (m_impl->rendererInitialized) {
        m_impl->renderer.render(m_impl->drawList);
        m_impl->renderer.endFrame();
    }
    
    popContext();
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
        FST_LOGF_ERROR("Context::loadFont failed - path: %s, size: %.1f", path.c_str(), size);
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

[[nodiscard]] InputState& Context::input() {
    return *m_impl->inputState;
}

const InputState& Context::input() const {
    return *m_impl->inputState;
}

[[nodiscard]] Renderer& Context::renderer() {
    return m_impl->renderer;
}

DrawList& Context::drawList() {
    return m_impl->drawList;
}

IDrawList* Context::activeDrawList() {
    return s_testDrawList ? s_testDrawList : &m_impl->drawList;
}


LayoutContext& Context::layout() {
    return m_impl->layout;
}

[[nodiscard]] IPlatformWindow& Context::window() const {
    return *m_impl->currentWindow;
}

DockContext& Context::docking() {
    return m_impl->dockContext;
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

WidgetId Context::getLastWidgetId() const {
    return m_impl->lastWidgetId;
}

void Context::setLastWidgetId(WidgetId id) {
    m_impl->lastWidgetId = id;
}

Rect Context::getLastWidgetBounds() const {
    return m_impl->lastWidgetBounds;
}

void Context::setLastWidgetBounds(const Rect& bounds) {
    m_impl->lastWidgetBounds = bounds;
}

bool Context::isCapturedBy(WidgetId id) const {
    return m_impl->activeWidget == id;
}

bool Context::isInputCaptured() const {
    return m_impl->activeWidget != INVALID_WIDGET_ID || m_impl->dockContext.dragState().active;
}

bool Context::isPointClipped(const Vec2& pos) const {
    return !m_impl->drawList.currentClipRect().contains(pos);
}

void Context::addFloatingWindowRect(const Rect& rect) {
    m_impl->currentFloatingRects.push_back(rect);
}

bool Context::isOccluded(const Vec2& pos) const {
    // Only block if we are in the default layer (background)
    IDrawList* dl = s_testDrawList ? s_testDrawList : &m_impl->drawList;
    if (dl->currentLayer() != DrawLayer::Default) {
        return false;
    }
    
    for (const auto& r : m_impl->prevFloatingRects) {
        if (r.contains(pos)) {
            return true;
        }
    }
    return false;
}

const std::vector<Rect>& Context::currentFloatingRects() const { return m_impl->currentFloatingRects; }
const std::vector<Rect>& Context::prevFloatingRects() const { return m_impl->prevFloatingRects; }

void Context::pushId(WidgetId id) {
    WidgetId combined = combineIds(m_impl->idStack.back(), id);
    m_impl->idStack.push_back(combined);
}

void Context::pushId(std::string_view str) {
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

WidgetId Context::makeId(std::string_view str) const {
    return combineIds(m_impl->idStack.back(), hashString(str));
}

WidgetId Context::makeId(int idx) const {
    return combineIds(m_impl->idStack.back(), static_cast<WidgetId>(idx));
}

void Context::pushContext(Context* ctx) {
    s_contextStack.push_back(ctx);
}

void Context::popContext() {
    if (!s_contextStack.empty()) {
        s_contextStack.pop_back();
    }
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)  // Disable deprecated warning for our own usage
#endif

Context* Context::current() {
    return s_contextStack.empty() ? nullptr : s_contextStack.back();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void Context::deferRender(std::function<void()> cmd) {
    m_impl->postRenderCommands.push_back(std::move(cmd));
}

Context::MenuState& Context::menuState() {
    return m_impl->menuState;
}

void Context::setTestDrawList(IDrawList* testDl) {
    s_testDrawList = testDl;
}

IDrawList* Context::testDrawList() {
    return s_testDrawList;
}

} // namespace fst
