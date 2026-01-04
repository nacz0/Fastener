#include "fastener/ui/layout.h"
#include <cmath>

namespace fst {

LayoutContext::LayoutContext() = default;
LayoutContext::~LayoutContext() = default;

void LayoutContext::beginContainer(const Rect& bounds, LayoutDirection direction) {
    ContainerState state;
    state.bounds = bounds;
    state.cursor = bounds.pos;
    state.direction = direction;
    state.remainingSize = (direction == LayoutDirection::Horizontal) 
        ? bounds.width() : bounds.height();
    m_stack.push_back(state);
}

void LayoutContext::endContainer() {
    if (!m_stack.empty()) {
        m_stack.pop_back();
    }
}

Rect LayoutContext::allocate(float width, float height, float flexGrow) {
    if (m_stack.empty()) {
        return Rect(0, 0, width, height);
    }
    
    auto& state = current();
    Rect result;
    
    // Apply padding to cursor on first item
    if (state.cursor == state.bounds.pos) {
        state.cursor.x += state.padding.left();
        state.cursor.y += state.padding.top();
    }
    
    // Apply scroll offset
    Vec2 pos = state.cursor - state.scrollOffset;
    
    if (state.direction == LayoutDirection::Horizontal) {
        result = Rect(pos.x, pos.y, width, height);
        state.cursor.x += width + state.spacing;
        state.remainingSize -= width + state.spacing;
    } else {
        result = Rect(pos.x, pos.y, width, height);
        state.cursor.y += height + state.spacing;
        state.remainingSize -= height + state.spacing;
    }
    
    state.totalFlex += flexGrow;
    
    return result;
}

Rect LayoutContext::allocateRemaining() {
    if (m_stack.empty()) {
        return Rect();
    }
    
    auto& state = current();
    Rect result;
    
    Vec2 pos = state.cursor - state.scrollOffset;
    
    float innerWidth = state.bounds.width() - state.padding.left() - state.padding.right();
    float innerHeight = state.bounds.height() - state.padding.top() - state.padding.bottom();
    
    if (state.direction == LayoutDirection::Horizontal) {
        float width = std::max(0.0f, state.remainingSize);
        result = Rect(pos.x, pos.y, width, innerHeight);
    } else {
        float height = std::max(0.0f, state.remainingSize);
        result = Rect(pos.x, pos.y, innerWidth, height);
    }
    
    return result;
}

void LayoutContext::setSpacing(float spacing) {
    if (!m_stack.empty()) {
        current().spacing = spacing;
    }
}

void LayoutContext::setPadding(float top, float right, float bottom, float left) {
    if (!m_stack.empty()) {
        current().padding = Vec4(top, right, bottom, left);
    }
}

void LayoutContext::setAlignment(Alignment mainAxis, Alignment crossAxis) {
    if (!m_stack.empty()) {
        current().mainAlign = mainAxis;
        current().crossAlign = crossAxis;
    }
}

Rect LayoutContext::currentBounds() const {
    if (m_stack.empty()) {
        return Rect();
    }
    return current().bounds;
}

Vec2 LayoutContext::currentPosition() const {
    if (m_stack.empty()) {
        return Vec2::zero();
    }
    return current().cursor;
}

float LayoutContext::remainingSpace() const {
    if (m_stack.empty()) {
        return 0.0f;
    }
    return current().remainingSize;
}

LayoutDirection LayoutContext::currentDirection() const {
    if (m_stack.empty()) {
        return LayoutDirection::Vertical;
    }
    return current().direction;
}

void LayoutContext::setScroll(float scrollX, float scrollY) {
    if (!m_stack.empty()) {
        current().scrollOffset = {scrollX, scrollY};
    }
}

Vec2 LayoutContext::scroll() const {
    if (m_stack.empty()) {
        return Vec2::zero();
    }
    return current().scrollOffset;
}

LayoutContext::ContainerState& LayoutContext::current() {
    return m_stack.back();
}

const LayoutContext::ContainerState& LayoutContext::current() const {
    return m_stack.back();
}

} // namespace fst
