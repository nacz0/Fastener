#include "fastener/ui/layout.h"
#include "fastener/core/context.h"
#include "fastener/ui/theme.h"
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
    if (m_stack.size() < 2) {
        if (!m_stack.empty()) m_stack.pop_back();
        return;
    }
    
    // Get size of container that just finished
    ContainerState finished = m_stack.back();
    m_stack.pop_back();
    
    // Report size to parent and advance its cursor
    ContainerState& parent = m_stack.back();
    
    float usedW = finished.maxInnerWidth + finished.padding.left() + finished.padding.right();
    float usedH = finished.maxInnerHeight + finished.padding.top() + finished.padding.bottom();
    
    // Position reporting logic (similar to allocate)
    if (parent.direction == LayoutDirection::Horizontal) {
        parent.cursor.x += usedW + parent.spacing;
        parent.remainingSize -= usedW + parent.spacing;
        parent.maxInnerWidth = std::max(parent.maxInnerWidth, parent.cursor.x - (parent.bounds.x() + parent.padding.left()));
        parent.maxInnerHeight = std::max(parent.maxInnerHeight, usedH);
    } else {
        parent.cursor.y += usedH + parent.spacing;
        parent.remainingSize -= usedH + parent.spacing;
        parent.maxInnerWidth = std::max(parent.maxInnerWidth, usedW);
        parent.maxInnerHeight = std::max(parent.maxInnerHeight, parent.cursor.y - (parent.bounds.y() + parent.padding.top()));
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
        
        state.maxInnerWidth = std::max(state.maxInnerWidth, state.cursor.x - (state.bounds.x() + state.padding.left()));
        state.maxInnerHeight = std::max(state.maxInnerHeight, height);
    } else {
        result = Rect(pos.x, pos.y, width, height);
        state.cursor.y += height + state.spacing;
        state.remainingSize -= height + state.spacing;
        
        state.maxInnerWidth = std::max(state.maxInnerWidth, width);
        state.maxInnerHeight = std::max(state.maxInnerHeight, state.cursor.y - (state.bounds.y() + state.padding.top()));
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

//=============================================================================
// Global Layout Helpers
//=============================================================================

void BeginHorizontal(Context& ctx, float spacing) {
    LayoutContext& lc = ctx.layout();
    lc.beginContainer(lc.allocateRemaining(), LayoutDirection::Horizontal);

    
    if (spacing >= 0) {
        lc.setSpacing(spacing);
    } else {
        lc.setSpacing(ctx.theme().metrics.paddingSmall);
    }
}



void EndHorizontal(Context& ctx) {
    ctx.layout().endContainer();
}



void BeginVertical(Context& ctx, float spacing) {
    LayoutContext& lc = ctx.layout();
    lc.beginContainer(lc.allocateRemaining(), LayoutDirection::Vertical);

    
    if (spacing >= 0) {
        lc.setSpacing(spacing);
    } else {
        lc.setSpacing(ctx.theme().metrics.paddingSmall);
    }
}



void EndVertical(Context& ctx) {
    ctx.layout().endContainer();
}




void Spacing(Context& ctx, float size) {
    LayoutContext& lc = ctx.layout();
    if (lc.currentDirection() == LayoutDirection::Horizontal) {
        lc.allocate(size, 0);
    } else {
        lc.allocate(0, size);
    }
}



void Padding(Context& ctx, float top, float right, float bottom, float left) {
    ctx.layout().setPadding(top, right, bottom, left);
}



void Padding(Context& ctx, float uniform) {
    Padding(ctx, uniform, uniform, uniform, uniform);
}



Rect Allocate(Context& ctx, float width, float height, float flexGrow) {
    return ctx.layout().allocate(width, height, flexGrow);
}



Rect AllocateRemainingSpace(Context& ctx) {
    return ctx.layout().allocateRemaining();
}







} // namespace fst
