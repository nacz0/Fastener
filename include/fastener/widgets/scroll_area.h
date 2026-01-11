#pragma once

#include "fastener/core/types.h"
#include "fastener/ui/style.h"
#include <functional>
#include <string>

namespace fst {
class Context;


struct ScrollAreaOptions {
    Style style;
    bool showHorizontal = true;
    bool showVertical = true;
    bool autoHide = true;
    float scrollbarWidth = 10.0f;
    float minThumbSize = 20.0f;
};

class ScrollArea {
public:
    ScrollArea();
    ~ScrollArea();

    // Set logical size of content inside the scroll area
    void setContentSize(const Vec2& size) { m_contentSize = size; }
    Vec2 contentSize() const { return m_contentSize; }

    // Scroll control
    Vec2 scrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(const Vec2& offset);
    void scrollTo(const Rect& area);

    // Main render function
    // contentRenderer is called with the available viewport bounds and current scroll offset
    // It should render content using the provided scroll offset to adjust positions
    void render(Context& ctx, const std::string& id, const Rect& bounds, 
                std::function<void(const Rect& viewport)> contentRenderer,
                const ScrollAreaOptions& options = {});
    void render(const std::string& id, const Rect& bounds, 
                std::function<void(const Rect& viewport)> contentRenderer,
                const ScrollAreaOptions& options = {});


private:
    Vec2 m_contentSize = {0, 0};
    Vec2 m_scrollOffset = {0, 0};
    
    // Interaction state
    bool m_draggingH = false;
    bool m_draggingV = false;
    float m_dragStartPos = 0.0f;
    float m_dragStartOffset = 0.0f;

    void handleInteraction(Context& ctx, const std::string& id, const Rect& bounds, const Rect& viewport, const ScrollAreaOptions& options);
    void clampScroll(const Rect& viewport);

};

} // namespace fst
