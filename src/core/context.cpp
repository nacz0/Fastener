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
#include "fastener/core/profiler.h"
#include <vector>
#include <chrono>
#include <algorithm>

namespace fst {

// Thread-local context stack for multi-window/DI support
static thread_local std::vector<Context*> s_contextStack;
static IDrawList* s_testDrawList = nullptr;

namespace {

class NullWindow final : public IPlatformWindow {
public:
    explicit NullWindow(InputState& input) : m_input(input) {}

    bool isOpen() const override { return false; }
    void close() override {}

    void pollEvents() override {}
    void waitEvents() override {}

    void swapBuffers() override {}
    void makeContextCurrent() override {}

    Vec2 size() const override { return {0.0f, 0.0f}; }
    Vec2 framebufferSize() const override { return {0.0f, 0.0f}; }
    float dpiScale() const override { return 1.0f; }

    int width() const override { return 0; }
    int height() const override { return 0; }

    void setTitle(const std::string&) override {}
    void setSize(int, int) override {}
    void setPosition(int, int) override {}

    void minimize() override {}
    void maximize() override {}
    void restore() override {}
    void focus() override {}

    bool isMinimized() const override { return false; }
    bool isMaximized() const override { return false; }
    bool isFocused() const override { return false; }

    void setCursor(Cursor) override {}
    void hideCursor() override {}
    void showCursor() override {}

    std::string getClipboardText() const override { return {}; }
    void setClipboardText(const std::string&) override {}

    InputState& input() override { return m_input; }
    const InputState& input() const override { return m_input; }

    void* nativeHandle() const override { return nullptr; }

private:
    InputState& m_input;
};

} // namespace

struct Context::Impl {
    // Components
    Renderer renderer;
    DrawList drawList;
    LayoutContext layout;
    DockContext dockContext;
    Profiler profiler;
    
    // Theme
    Theme theme = Theme::dark();
    
    // Fonts
    std::unique_ptr<Font> defaultFont;
    Font* currentFont = nullptr;
    
    // Input
    InputState* inputState = nullptr;
    IPlatformWindow* currentWindow = nullptr;
    InputState nullInput;
    NullWindow nullWindow{nullInput};

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
    std::vector<Rect> currentGlobalOcclusionRects;
    std::vector<Rect> prevGlobalOcclusionRects;
    bool rendererInitialized = false;
    bool rendererEnabled = false;
    bool frameActive = false;
};

Context::Context(bool initializeRenderer) : m_impl(std::make_unique<Impl>()) {
    m_impl->rendererEnabled = initializeRenderer;
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
    m_impl->frameActive = true;
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
    
    // Profiler
    m_impl->profiler.beginFrame();
    m_impl->profiler.beginSection("Frame");

    // Clear draw list
    m_impl->inputState->setFrameTime(m_impl->totalTime);
    m_impl->drawList.clear();
    
    // Begin rendering
    if (m_impl->rendererEnabled) {
        window.makeContextCurrent();
        if (!m_impl->rendererInitialized) {
            if (m_impl->renderer.init()) {
                m_impl->rendererInitialized = true;
            } else {
                FST_LOG_ERROR("Context::beginFrame failed to initialize renderer");
                m_impl->rendererEnabled = false;
            }
        }
    }
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
    m_impl->prevGlobalOcclusionRects = m_impl->currentGlobalOcclusionRects;
    m_impl->currentGlobalOcclusionRects.clear();

    // If the mouse is over a previous-frame occluder, consume input early so
    // widgets rendered before overlays don't steal the click.
    if (m_impl->inputState) {
        Vec2 mousePos = m_impl->inputState->mousePos();
        for (const auto& r : m_impl->prevGlobalOcclusionRects) {
            if (r.contains(mousePos)) {
                m_impl->inputState->consumeMouse();
                break;
            }
        }
    }
    
    // Begin docking frame
    m_impl->dockContext.beginFrame(*this);

    m_impl->profiler.beginSection("UI");
}

void Context::endFrame() {
    m_impl->profiler.endSection(); // UI

    // Safety net: If mouse was released but activeWidget wasn't cleared by any widget,
    // clear it here to prevent stuck input capture.
    if (m_impl->activeWidget != INVALID_WIDGET_ID && 
        m_impl->inputState->isMouseReleased(MouseButton::Left)) {
        m_impl->activeWidget = INVALID_WIDGET_ID;
    }

    m_impl->profiler.beginSection("Internal");
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
    EndDragDropFrame(*this);
    
    // Render
    m_impl->drawList.mergeLayers();
    if (m_impl->rendererInitialized) {
        m_impl->renderer.render(m_impl->drawList);
        m_impl->renderer.endFrame();
    }
    
    m_impl->profiler.endSection(); // Internal
    m_impl->profiler.endSection(); // Frame
    m_impl->profiler.endFrame();

    m_impl->inputState = nullptr;
    m_impl->currentWindow = nullptr;
    m_impl->frameActive = false;

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
    if (!m_impl->inputState) {
        FST_LOG_ERROR("Context::input called outside of beginFrame/endFrame");
        return m_impl->nullInput;
    }
    return *m_impl->inputState;
}

const InputState& Context::input() const {
    if (!m_impl->inputState) {
        FST_LOG_ERROR("Context::input called outside of beginFrame/endFrame");
        return m_impl->nullInput;
    }
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
    if (!m_impl->currentWindow) {
        FST_LOG_ERROR("Context::window called outside of beginFrame/endFrame");
        return m_impl->nullWindow;
    }
    return *m_impl->currentWindow;
}

DockContext& Context::docking() {
    return m_impl->dockContext;
}

Profiler& Context::profiler() {
    return m_impl->profiler;
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
    FST_LOGF_DEBUG("setActiveWidget: %llu", id);
    m_impl->activeWidget = id;
}

void Context::clearActiveWidget() {
    FST_LOGF_DEBUG("clearActiveWidget (was: %llu)", m_impl->activeWidget);
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

void Context::addGlobalOcclusionRect(const Rect& rect) {
    m_impl->currentGlobalOcclusionRects.push_back(rect);
}

bool Context::isOccluded(const Vec2& pos) const {
    float windowW = static_cast<float>(m_impl->currentWindow ? m_impl->currentWindow->width() : 1);
    float windowH = static_cast<float>(m_impl->currentWindow ? m_impl->currentWindow->height() : 1);
    
    // First check for fullscreen modal backdrop - these block ALL layers
    auto isFullscreen = [windowW, windowH](const Rect& r) {
        return r.x() <= 1 && r.y() <= 1 && 
               r.width() >= windowW - 2 && r.height() >= windowH - 2;
    };
    
    for (const auto& r : m_impl->currentFloatingRects) {
        if (isFullscreen(r) && r.contains(pos)) {
            return true;  // Fullscreen modal blocks everything
        }
    }
    for (const auto& r : m_impl->prevFloatingRects) {
        if (isFullscreen(r) && r.contains(pos)) {
            return true;  // Fullscreen modal blocks everything
        }
    }

    // Global occlusion rects block all layers
    for (const auto& r : m_impl->currentGlobalOcclusionRects) {
        if (r.contains(pos)) {
            return true;
        }
    }
    for (const auto& r : m_impl->prevGlobalOcclusionRects) {
        if (r.contains(pos)) {
            return true;
        }
    }
    
    // For non-fullscreen floating windows, only block default layer
    IDrawList* dl = s_testDrawList ? s_testDrawList : &m_impl->drawList;
    if (dl->currentLayer() != DrawLayer::Default) {
        return false;
    }
    
    // Check smaller floating windows (only for default layer)
    for (const auto& r : m_impl->currentFloatingRects) {
        if (!isFullscreen(r) && r.contains(pos)) {
            return true;
        }
    }
    for (const auto& r : m_impl->prevFloatingRects) {
        if (!isFullscreen(r) && r.contains(pos)) {
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
