#pragma once

#include "fastener/core/types.h"
#include "fastener/core/input.h"
#include "fastener/platform/platform_interface.h"
#include <vector>
#include <deque>
#include <functional>
#include <memory>
#include <string_view>

namespace fst {

// Forward declarations
class Context;
class IRenderer;
class DrawList;
class IDrawList;
class Theme;
class Font;
class LayoutContext;
class DockContext;
class Profiler;
class I18n;

namespace detail {
struct DragDropContextState;
class WidgetStateRegistry;
DragDropContextState& dragDropState(Context& ctx);
const DragDropContextState& dragDropState(const Context& ctx);
WidgetStateRegistry& widgetStates(Context& ctx);
}

//=============================================================================
// Context - Main application context
//=============================================================================
class Context : private IWindowResourceListener {
public:
    Context(bool initializeRenderer = true);
    explicit Context(
        std::unique_ptr<IRenderer> renderer,
        bool initializeRenderer = true);
    ~Context();
    
    // Non-copyable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    // Frame management
    void beginFrame(IPlatformWindow& window);
    void endFrame();
    [[nodiscard]] bool isFrameActive() const;

    /**
     * Release renderer objects local to one platform window's graphics
     * context. Fastener Window performs this automatically; use this for a
     * foreign IPlatformWindow that does not support resource listeners.
     */
    [[nodiscard]] bool releaseWindowResources(IPlatformWindow& window);

    /**
     * Release fonts and shared renderer objects with the supplied window's
     * graphics context current. Fastener Window performs this automatically
     * for the final registered window. The Context may be initialized again
     * by beginFrame.
     */
    [[nodiscard]] bool shutdown(IPlatformWindow& window);
    
    // Profiling
    Profiler& profiler();

    // Theme
    void setTheme(const Theme& theme);
    Theme& theme();
    const Theme& theme() const;
    
    // Font
    /**
     * Load and select the default font. A failed load leaves the current font
     * unchanged.
     */
    bool loadFont(const std::string& path, float size);
    Font* font() const;
    Font* defaultFont() const;
    
    // Input
    InputState& input();
    const InputState& input() const;
    
    // Drawing
    DrawList& drawList();
    IDrawList* activeDrawList();
    const IDrawList* activeDrawList() const;
    /**
     * Override drawing for this context, primarily for tests and tooling.
     * The caller retains ownership and must keep the object alive until the
     * override is cleared or this Context is destroyed.
     */
    void setDrawListOverride(IDrawList* drawList);
    IRenderer& renderer();
    LayoutContext& layout();
    IPlatformWindow& window() const;
    DockContext& docking();
    I18n& i18n();
    const I18n& i18n() const;
    
    // Time
    float deltaTime() const;
    float time() const;
    
    // Focus management
    WidgetId getFocusedWidget() const;
    void setFocusedWidget(WidgetId id);
    void clearFocus();
    
    // Hover tracking
    WidgetId getHoveredWidget() const;
    void setHoveredWidget(WidgetId id);
    
    // Active (pressed) widget
    WidgetId getActiveWidget() const;
    void setActiveWidget(WidgetId id);
    void clearActiveWidget();

    // Last widget submitted in the current frame (for Drag & Drop etc.)
    WidgetId getLastWidgetId() const;
    void setLastWidgetId(WidgetId id);
    Rect getLastWidgetBounds() const;
    void setLastWidgetBounds(const Rect& bounds);
    
    /** @brief Check if a specific widget has captured the mouse. */
    bool isCapturedBy(WidgetId id) const;

    /** @brief Check if any widget has captured the mouse (is active). */
    bool isInputCaptured() const;

    /** @brief Check if a point is outside the current clip rectangle. */
    bool isPointClipped(const Vec2& pos) const;

    // Occlusion handling
    void addFloatingWindowRect(const Rect& rect);
    void addGlobalOcclusionRect(const Rect& rect);  // Blocks input on all layers
    bool isOccluded(const Vec2& pos) const;
    const std::vector<Rect>& currentFloatingRects() const;
    const std::vector<Rect>& prevFloatingRects() const;
    
    // ID stack (for hierarchical widgets)
    void pushId(WidgetId id);
    void pushId(std::string_view str);
    void pushId(int idx);
    void popId();
    WidgetId currentId() const;
    WidgetId makeId(std::string_view str) const;
    WidgetId makeId(int idx) const;
    
    // Deferred rendering (for popups/tooltips). Commands queued while deferred
    // commands are executing run during the following frame.
    void deferRender(std::function<void()> cmd);
    
    // Menu state management (for internal use)
    struct MenuState {
        Rect contextMenuRect;
        Rect menuBarDropdownRect;
        bool menuBarOpen = false;
        bool contextMenuActive = false;
    };
    MenuState& menuState();
    
private:
    void beforeWindowDestroyed(IPlatformWindow& window) override;
    void releaseTrackedWindowResources();

    friend detail::DragDropContextState& detail::dragDropState(Context& ctx);
    friend const detail::DragDropContextState& detail::dragDropState(const Context& ctx);
    friend detail::WidgetStateRegistry& detail::widgetStates(Context& ctx);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
};

} // namespace fst
