#pragma once

#include "fastener/ui/drag_drop.h"

namespace fst::detail {

struct DragDropContextState : DragDropState {
    bool pendingClear = false;
    WidgetId currentSourceWidget = INVALID_WIDGET_ID;
    Rect currentTargetRect;
    bool inSourceBlock = false;
    bool inTargetBlock = false;
    Vec2 mousePressPos;
    Vec2 globalMousePressPos;
    bool potentialDrag = false;
    WidgetId potentialDragSource = INVALID_WIDGET_ID;
    DragDropFlags sourceFlags = DragDropFlags_None;

    void clear() {
        DragDropState::clear();
        pendingClear = false;
        currentSourceWidget = INVALID_WIDGET_ID;
        currentTargetRect = Rect{};
        inSourceBlock = false;
        inTargetBlock = false;
        mousePressPos = Vec2{};
        globalMousePressPos = Vec2{};
        potentialDrag = false;
        potentialDragSource = INVALID_WIDGET_ID;
        sourceFlags = DragDropFlags_None;
    }
};

} // namespace fst::detail
