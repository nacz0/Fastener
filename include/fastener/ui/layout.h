#pragma once

#include "fastener/core/types.h"
#include <vector>
#include <functional>

namespace fst {

class Context;


//=============================================================================
// Layout Direction
//=============================================================================
enum class LayoutDirection {
    Horizontal,
    Vertical
};

//=============================================================================
// Layout Item - Result of layout calculation
//=============================================================================
struct LayoutItem {
    Rect bounds;
    WidgetId id = INVALID_WIDGET_ID;
    bool visible = true;
};

//=============================================================================
// Layout Constraints
//=============================================================================
struct LayoutConstraints {
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = 10000.0f;
    float maxHeight = 10000.0f;
    
    constexpr LayoutConstraints() = default;
    constexpr LayoutConstraints(float maxW, float maxH) 
        : maxWidth(maxW), maxHeight(maxH) {}
    constexpr LayoutConstraints(float minW, float minH, float maxW, float maxH)
        : minWidth(minW), minHeight(minH), maxWidth(maxW), maxHeight(maxH) {}
    
    constexpr LayoutConstraints tight(float w, float h) const {
        return {w, h, w, h};
    }
    
    constexpr LayoutConstraints loosen() const {
        return {0, 0, maxWidth, maxHeight};
    }
};

//=============================================================================
// Layout Context - Current layout state
//=============================================================================
class LayoutContext {
public:
    LayoutContext();
    ~LayoutContext();
    
    // Begin a new layout container
    void beginContainer(const Rect& bounds, LayoutDirection direction = LayoutDirection::Vertical);
    void endContainer();
    
    // Allocate space for a widget
    Rect allocate(float width, float height, float flexGrow = 0.0f);
    Rect allocateRemaining();
    
    // Container properties
    void setSpacing(float spacing);
    void setPadding(float top, float right, float bottom, float left);
    void setAlignment(Alignment mainAxis, Alignment crossAxis);
    
    // Current state
    Rect currentBounds() const;
    Vec2 currentPosition() const;
    float remainingSpace() const;
    LayoutDirection currentDirection() const;
    
    // Scrolling
    void setScroll(float scrollX, float scrollY);
    Vec2 scroll() const;
    
private:
    struct ContainerState {
        Rect bounds;
        Vec2 cursor;
        LayoutDirection direction;
        float spacing = 0.0f;
        Vec4 padding;
        Alignment mainAlign = Alignment::Start;
        Alignment crossAlign = Alignment::Start;
        
        Vec2 scrollOffset;
        
        // For flex layout and nesting
        float totalFlex = 0.0f;
        float remainingSize = 0.0f;
        float maxInnerWidth = 0.0f;
        float maxInnerHeight = 0.0f;
    };
    
    std::vector<ContainerState> m_stack;
    
    ContainerState& current();
    const ContainerState& current() const;
};

//=============================================================================
// Global Layout Helpers
//=============================================================================
void BeginHorizontal(Context& ctx, float spacing = -1.0f);
void EndHorizontal(Context& ctx);

void BeginVertical(Context& ctx, float spacing = -1.0f);
void EndVertical(Context& ctx);


void Spacing(Context& ctx, float size);
void Padding(Context& ctx, float top, float right, float bottom, float left);
void Padding(Context& ctx, float uniform);

Rect Allocate(Context& ctx, float width, float height, float flexGrow = 0.0f);
Rect AllocateRemainingSpace(Context& ctx);





} // namespace fst
