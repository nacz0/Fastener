#pragma once

#include "fastener/core/types.h"
#include "fastener/core/input.h"
#include <vector>
#include <deque>
#include <functional>
#include <memory>

namespace fst {

// Forward declarations
class Window;
class Renderer;
class DrawList;
class Theme;
class Font;
class LayoutContext;
class DockContext;

//=============================================================================
// Context - Main application context
//=============================================================================
class Context {
public:
    Context();
    ~Context();
    
    // Non-copyable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    // Frame management
    void beginFrame(Window& window);
    void endFrame();
    
    // Theme
    void setTheme(const Theme& theme);
    Theme& theme();
    const Theme& theme() const;
    
    // Font
    bool loadFont(const std::string& path, float size);
    Font* font() const;
    Font* defaultFont() const;
    
    // Input
    InputState& input();
    const InputState& input() const;
    
    // Drawing
    DrawList& drawList();
    Renderer& renderer();
    LayoutContext& layout();
    Window& window();
    DockContext& docking();
    
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

    // Last used widget (for Drag & Drop etc.)
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
    bool isOccluded(const Vec2& pos) const;
    
    // ID stack (for hierarchical widgets)
    void pushId(WidgetId id);
    void pushId(const char* str);
    void pushId(int idx);
    void popId();
    WidgetId currentId() const;
    WidgetId makeId(const char* str) const;
    WidgetId makeId(int idx) const;
    
    // Global access (use sparingly)
    static Context* current();

    // Deferred rendering (for popups/tooltips)
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
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    static Context* s_current;
};

} // namespace fst
