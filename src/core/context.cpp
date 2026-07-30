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
#include "../ui/drag_drop_internal.h"
#include "widget_state_registry.h"
#include "fastener/core/profiler.h"
#include "fastener/core/i18n.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <exception>

namespace fst {

// Thread-local frame stack validates nested multi-context frame ordering
// without exposing an implicit context-routing API.
namespace {

thread_local std::vector<Context*> s_frameStack;

} // namespace

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
    std::unique_ptr<IRenderer> renderer;
    DrawList drawList;
    IDrawList* drawListOverride = nullptr;
    LayoutContext layout;
    DockContext dockContext;
    Profiler profiler;
    I18n translations;
    detail::DragDropContextState dragDrop;
    detail::WidgetStateRegistry widgetStates;
    
    // Theme
    Theme theme = Theme::dark();
    
    // Fonts
    std::unique_ptr<Font> defaultFont;
    Font* currentFont = nullptr;
    
    // Input
    InputState* inputState = nullptr;
    IPlatformWindow* currentWindow = nullptr;
    std::vector<IPlatformWindow*> resourceWindows;
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
    
    explicit Impl(std::unique_ptr<IRenderer> rendererOverride = {})
        : renderer(
              rendererOverride
                  ? std::move(rendererOverride)
                  : std::make_unique<Renderer>()) {
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
    bool frameEnding = false;
};

Context::Context(bool initializeRenderer) : m_impl(std::make_unique<Impl>()) {
    m_impl->rendererEnabled = initializeRenderer;
}

Context::Context(
    std::unique_ptr<IRenderer> renderer,
    bool initializeRenderer)
    : m_impl(std::make_unique<Impl>(std::move(renderer))) {
    m_impl->rendererEnabled = initializeRenderer;
}

Context::~Context() {
    releaseTrackedWindowResources();

    s_frameStack.erase(
        std::remove_if(
            s_frameStack.begin(), s_frameStack.end(),
            [this](const Context* context) {
                return context == this;
            }),
        s_frameStack.end());
}

namespace detail {

DragDropContextState& dragDropState(Context& ctx) {
    return ctx.m_impl->dragDrop;
}

const DragDropContextState& dragDropState(const Context& ctx) {
    return ctx.m_impl->dragDrop;
}

WidgetStateRegistry& widgetStates(Context& ctx) {
    return ctx.m_impl->widgetStates;
}

} // namespace detail

void Context::beginFrame(IPlatformWindow& window) {
    if (m_impl->frameActive) {
        FST_LOG_ERROR("Context::beginFrame called while a frame is already active");
        return;
    }

    s_frameStack.push_back(this);
    m_impl->frameActive = true;
    m_impl->currentWindow = &window;
    m_impl->inputState = &window.input();
    m_impl->idStack.resize(1);
    m_impl->idStack.front() = combineIds(
        0,
        static_cast<WidgetId>(reinterpret_cast<std::uintptr_t>(&window)));
    m_impl->inputState->onResize(static_cast<float>(window.width()), static_cast<float>(window.height()));

    if (m_impl->rendererEnabled &&
        std::find(
            m_impl->resourceWindows.begin(),
            m_impl->resourceWindows.end(),
            &window) == m_impl->resourceWindows.end() &&
        window.addResourceListener(*this)) {
        m_impl->resourceWindows.push_back(&window);
    }

    
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
            if (m_impl->renderer->init()) {
                m_impl->rendererInitialized = true;
            } else {
                FST_LOG_ERROR("Context::beginFrame failed to initialize renderer");
                m_impl->rendererEnabled = false;
            }
        }
    }
    if (m_impl->rendererInitialized) {
        Vec2 fbSize = window.framebufferSize();
        m_impl->renderer->beginFrame(
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
    
    // Reset frame-local interaction state.
    window.setCursor(Cursor::Arrow);
    m_impl->hoveredWidget = INVALID_WIDGET_ID;
    m_impl->lastWidgetId = INVALID_WIDGET_ID;
    m_impl->lastWidgetBounds = Rect{};
    
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
    if (m_impl->frameEnding) {
        FST_LOG_ERROR("Context::endFrame called recursively");
        return;
    }
    if (!m_impl->frameActive) {
        FST_LOG_ERROR("Context::endFrame called without an active frame");
        return;
    }
    if (s_frameStack.empty() || s_frameStack.back() != this) {
        FST_LOG_ERROR(
            "Context::endFrame must close the top-most active frame");
        return;
    }

    m_impl->frameEnding = true;
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
    // Deferred overlays must start from a root scope even if user code missed
    // an End... call during the main UI pass.
    m_impl->layout.reset();
    m_impl->idStack.resize(1);
    
    // Pop clip rect
    m_impl->drawList.popClipRect();
    
    // Execute a stable snapshot. Commands deferred by these callbacks belong
    // to the next frame and remain in postRenderCommands.
    std::deque<std::function<void()>> commands;
    commands.swap(m_impl->postRenderCommands);
    std::exception_ptr deferredException;
    for (const auto& cmd : commands) {
        if (cmd) {
            try {
                cmd();
            } catch (...) {
                if (!deferredException) {
                    deferredException = std::current_exception();
                }
            }
        }
    }

    // Cleanup drag and drop state if needed (and render preview)
    EndDragDropFrame(*this);
    
    // Render
    m_impl->drawList.mergeLayers();
    if (m_impl->rendererInitialized) {
        m_impl->renderer->render(m_impl->drawList);
        m_impl->renderer->endFrame();
    }
    
    m_impl->profiler.endSection(); // Internal
    m_impl->profiler.endSection(); // Frame
    m_impl->profiler.endFrame();

    m_impl->inputState = nullptr;
    m_impl->currentWindow = nullptr;
    m_impl->frameActive = false;
    m_impl->frameEnding = false;
    // Also recover any scopes left unbalanced by deferred rendering.
    m_impl->layout.reset();
    m_impl->idStack.resize(1);
    m_impl->idStack.front() = 0;

    s_frameStack.pop_back();

    if (deferredException) {
        std::rethrow_exception(deferredException);
    }
}

bool Context::isFrameActive() const {
    return m_impl->frameActive;
}

bool Context::releaseWindowResources(IPlatformWindow& window) {
    if (m_impl->frameActive) {
        FST_LOG_ERROR(
            "Context::releaseWindowResources called during an active frame");
        return false;
    }

    window.makeContextCurrent();
    return m_impl->renderer->releaseCurrentContextResources();
}

bool Context::shutdown(IPlatformWindow& window) {
    if (m_impl->frameActive) {
        FST_LOG_ERROR("Context::shutdown called during an active frame");
        return false;
    }

    window.makeContextCurrent();

    m_impl->currentFont = nullptr;
    m_impl->defaultFont.reset();
    (void)m_impl->renderer->releaseCurrentContextResources();
    m_impl->renderer->shutdown();
    m_impl->rendererInitialized = false;
    return true;
}

void Context::beforeWindowDestroyed(IPlatformWindow& window) {
    auto it = std::find(
        m_impl->resourceWindows.begin(),
        m_impl->resourceWindows.end(),
        &window);
    if (it == m_impl->resourceWindows.end()) {
        return;
    }

    if (m_impl->frameActive) {
        FST_LOG_ERROR(
            "A platform window was destroyed during an active Context frame");
    } else if (m_impl->resourceWindows.size() == 1) {
        (void)shutdown(window);
    } else {
        (void)releaseWindowResources(window);
    }

    m_impl->resourceWindows.erase(it);
}

void Context::releaseTrackedWindowResources() {
    if (m_impl->resourceWindows.empty()) {
        return;
    }
    if (m_impl->frameActive) {
        FST_LOG_ERROR(
            "Context destroyed during an active frame; GPU cleanup cannot be guaranteed");
        for (IPlatformWindow* window : m_impl->resourceWindows) {
            if (window) {
                window->removeResourceListener(*this);
            }
        }
        m_impl->resourceWindows.clear();
        return;
    }

    while (m_impl->resourceWindows.size() > 1) {
        IPlatformWindow* window = m_impl->resourceWindows.back();
        m_impl->resourceWindows.pop_back();
        if (window) {
            (void)releaseWindowResources(*window);
            window->removeResourceListener(*this);
        }
    }

    IPlatformWindow* finalWindow = m_impl->resourceWindows.back();
    m_impl->resourceWindows.clear();
    if (finalWindow) {
        (void)shutdown(*finalWindow);
        finalWindow->removeResourceListener(*this);
    }
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
    auto candidate = std::make_unique<Font>();
    if (!candidate->loadFromFile(path, size)) {
        FST_LOGF_ERROR("Context::loadFont failed - path: %s, size: %.1f", path.c_str(), size);
        return false;
    }

    m_impl->defaultFont = std::move(candidate);
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

[[nodiscard]] IRenderer& Context::renderer() {
    return *m_impl->renderer;
}

DrawList& Context::drawList() {
    return m_impl->drawList;
}

IDrawList* Context::activeDrawList() {
    return m_impl->drawListOverride
        ? m_impl->drawListOverride
        : &m_impl->drawList;
}

const IDrawList* Context::activeDrawList() const {
    return m_impl->drawListOverride
        ? m_impl->drawListOverride
        : &m_impl->drawList;
}

void Context::setDrawListOverride(IDrawList* drawList) {
    m_impl->drawListOverride = drawList;
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

I18n& Context::i18n() {
    return m_impl->translations;
}

const I18n& Context::i18n() const {
    return m_impl->translations;
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
    const IDrawList* dl = activeDrawList();
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

void Context::deferRender(std::function<void()> cmd) {
    m_impl->postRenderCommands.push_back(std::move(cmd));
}

Context::MenuState& Context::menuState() {
    return m_impl->menuState;
}

} // namespace fst
