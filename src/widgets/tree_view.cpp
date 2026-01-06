/**
 * @file tree_view.cpp
 * @brief Tree view widget implementation for hierarchical data display.
 */

#include "fastener/widgets/tree_view.h"
#include "fastener/widgets/menu.h"
#include "fastener/core/context.h"
#include "fastener/graphics/draw_list.h"
#include "fastener/graphics/font.h"
#include "fastener/ui/widget.h"
#include "fastener/ui/theme.h"
#include <cmath>
#include <algorithm>

namespace fst {

//=============================================================================
// TreeView Implementation
//=============================================================================

TreeView::TreeView() {
    m_root = std::make_shared<TreeNode>("root", "Root");
}

TreeView::~TreeView() = default;

void TreeView::setRoot(std::shared_ptr<TreeNode> root) {
    m_root = root;
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    m_scrollY = 0.0f;
}

void TreeView::selectNode(TreeNode* node) {
    if (m_selectedNode) {
        m_selectedNode->isSelected = false;
    }
    m_selectedNode = node;
    if (m_selectedNode) {
        m_selectedNode->isSelected = true;
    }
}

void TreeView::clearSelection() {
    if (m_selectedNode) {
        m_selectedNode->isSelected = false;
        m_selectedNode = nullptr;
    }
}

void TreeView::expandAll() {
    std::function<void(TreeNode*)> expand = [&](TreeNode* node) {
        if (!node->isLeaf) {
            node->isExpanded = true;
        }
        for (auto& child : node->children) {
            expand(child.get());
        }
    };
    if (m_root) expand(m_root.get());
}

void TreeView::collapseAll() {
    std::function<void(TreeNode*)> collapse = [&](TreeNode* node) {
        node->isExpanded = false;
        for (auto& child : node->children) {
            collapse(child.get());
        }
    };
    if (m_root) collapse(m_root.get());
}

void TreeView::expandTo(TreeNode* node) {
    if (!node) return;
    
    TreeNode* parent = node->parent;
    while (parent) {
        parent->isExpanded = true;
        parent = parent->parent;
    }
}

void TreeView::scrollToNode(TreeNode* node) {
    // TODO: Calculate y position of node and scroll to it
    expandTo(node);
}

void TreeView::render(const std::string& id, const Rect& bounds,
                      const TreeViewOptions& options,
                      const TreeViewEvents& events) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    
    // Generate ID
    ctx->pushId(id.c_str());
    WidgetId widgetId = ctx->currentId();
    
    // Draw background
    dl.addRectFilled(bounds, theme.colors.panelBackground, theme.metrics.borderRadius);
    dl.addRect(bounds, theme.colors.border, theme.metrics.borderRadius);
    
    // Clip content
    Rect contentBounds = bounds.shrunk(2);
    dl.pushClipRect(contentBounds);
    
    // Handle scrolling
    if (bounds.contains(ctx->input().mousePos())) {
        Vec2 scroll = ctx->input().scrollDelta();
        m_scrollY -= scroll.y * options.rowHeight * 3;
        m_scrollY = std::max(0.0f, std::min(m_scrollY, std::max(0.0f, m_contentHeight - contentBounds.height())));
    }
    
    // Reset hovered
    m_hoveredNode = nullptr;
    
    // Render tree starting from root's children (or root itself if showing root)
    float y = contentBounds.y() - m_scrollY;
    
    if (m_root) {
        // Render children of root (root itself is hidden)
        for (auto& child : m_root->children) {
            y = renderNode(child.get(), contentBounds, y, options, events);
        }
    }
    
    m_contentHeight = (y + m_scrollY) - contentBounds.y();
    
    dl.popClipRect();
    ctx->popId();
}

float TreeView::renderNode(TreeNode* node, const Rect& bounds, float y,
                           const TreeViewOptions& options,
                           const TreeViewEvents& events) {
    Context* ctx = Context::current();
    if (!ctx) return y;
    
    DrawList& dl = ctx->drawList();
    const Theme& theme = ctx->theme();
    Font* font = ctx->font();
    
    // Skip if completely above or below visible area
    if (y + options.rowHeight < bounds.y() || y > bounds.bottom()) {
        // Still need to recurse for content height calculation
        y += options.rowHeight;
        if (node->isExpanded) {
            for (auto& child : node->children) {
                y = renderNode(child.get(), bounds, y, options, events);
            }
        }
        return y;
    }
    
    int depth = node->depth() - 1;  // -1 because root is hidden
    float indent = depth * options.indentWidth;
    
    Rect rowBounds(bounds.x(), y, bounds.width(), options.rowHeight);
    Rect actualRow(bounds.x() + indent, y, bounds.width() - indent, options.rowHeight);
    
    // Hit testing
    bool isHovered = rowBounds.contains(ctx->input().mousePos()) && !fst::IsMouseOverAnyMenu();
    if (isHovered) {
        m_hoveredNode = node;
    }
    
    // Handle interaction
    WidgetId nodeId = ctx->makeId(node->id.c_str());
    
    if (isHovered && ctx->input().isMousePressed(MouseButton::Left)) {
        // Check if clicked on expand arrow
        float arrowX = bounds.x() + indent;
        float arrowWidth = 16.0f;
        Vec2 mousePos = ctx->input().mousePos();
        
        if (!node->isLeaf && node->hasChildren() && 
            mousePos.x >= arrowX && mousePos.x < arrowX + arrowWidth) {
            // Toggle expand
            node->isExpanded = !node->isExpanded;
            if (node->isExpanded && events.onExpand) {
                events.onExpand(node);
            } else if (!node->isExpanded && events.onCollapse) {
                events.onCollapse(node);
            }
        } else {
            // Select node
            selectNode(node);
            if (events.onSelect) {
                events.onSelect(node);
            }
        }
    }
    
    if (isHovered && ctx->input().isMouseDoubleClicked(MouseButton::Left)) {
        if (events.onDoubleClick) {
            events.onDoubleClick(node);
        }
        // Toggle expand on double click for folders
        if (!node->isLeaf) {
            node->isExpanded = !node->isExpanded;
        }
    }
    
    if (isHovered && ctx->input().isMousePressed(MouseButton::Right)) {
        if (events.onContextMenu) {
            events.onContextMenu(node);
        }
    }
    
    // Draw selection/hover background
    if (node->isSelected) {
        dl.addRectFilled(rowBounds, theme.colors.selection);
    } else if (isHovered) {
        dl.addRectFilled(rowBounds, theme.colors.buttonHover);
    }
    
    float x = bounds.x() + indent;
    
    // Draw expand arrow
    if (!node->isLeaf && node->hasChildren()) {
        drawExpandArrow(Vec2(x + 8, y + options.rowHeight / 2), node->isExpanded,
                        node->isSelected ? theme.colors.selectionText : theme.colors.text);
    }
    x += 16.0f;
    
    // Draw icon
    if (options.showIcons) {
        drawIcon(Vec2(x + 8, y + options.rowHeight / 2), !node->isLeaf, node->isExpanded,
                 node->isSelected ? theme.colors.selectionText : theme.colors.textSecondary);
        x += 20.0f;
    }
    
    // Draw label
    if (font) {
        Color textColor = node->isSelected ? theme.colors.selectionText : theme.colors.text;
        Vec2 textPos(x + 4, y + (options.rowHeight - font->lineHeight()) / 2);
        dl.addText(font, textPos, node->label, textColor);
    }
    
    y += options.rowHeight;
    
    // Render children if expanded
    if (node->isExpanded) {
        for (auto& child : node->children) {
            y = renderNode(child.get(), bounds, y, options, events);
        }
    }
    
    return y;
}

void TreeView::drawExpandArrow(const Vec2& pos, bool expanded, Color color) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    DrawList& dl = ctx->drawList();
    float size = 4.0f;
    
    if (expanded) {
        // Down arrow (triangle pointing down)
        Vec2 p1(pos.x - size, pos.y - size / 2);
        Vec2 p2(pos.x + size, pos.y - size / 2);
        Vec2 p3(pos.x, pos.y + size / 2);
        dl.addTriangleFilled(p1, p2, p3, color);
    } else {
        // Right arrow (triangle pointing right)
        Vec2 p1(pos.x - size / 2, pos.y - size);
        Vec2 p2(pos.x + size / 2, pos.y);
        Vec2 p3(pos.x - size / 2, pos.y + size);
        dl.addTriangleFilled(p1, p2, p3, color);
    }
}

void TreeView::drawIcon(const Vec2& pos, bool isFolder, bool isExpanded, Color color) {
    Context* ctx = Context::current();
    if (!ctx) return;
    
    DrawList& dl = ctx->drawList();
    
    if (isFolder) {
        // Draw folder icon
        float w = 12.0f;
        float h = 9.0f;
        float tabW = 5.0f;
        float tabH = 2.0f;
        
        // Folder tab
        dl.addRectFilled(Rect(pos.x - w/2, pos.y - h/2 - tabH, tabW, tabH), color);
        // Folder body
        dl.addRectFilled(Rect(pos.x - w/2, pos.y - h/2, w, h), color, 2.0f);
    } else {
        // Draw file icon
        float w = 10.0f;
        float h = 12.0f;
        float corner = 3.0f;
        
        // File body
        dl.addRectFilled(Rect(pos.x - w/2, pos.y - h/2, w, h), color, 1.0f);
        // Corner fold
        dl.addTriangleFilled(
            Vec2(pos.x + w/2 - corner, pos.y - h/2),
            Vec2(pos.x + w/2, pos.y - h/2 + corner),
            Vec2(pos.x + w/2 - corner, pos.y - h/2 + corner),
            color.darker(0.2f)
        );
    }
}

void TreeViewSimple(const std::string& id, TreeNode* root, const Rect& bounds,
                    std::function<void(TreeNode*)> onSelect,
                    const TreeViewOptions& options) {
    static std::unordered_map<std::string, TreeView> treeViews;
    
    auto& tv = treeViews[id];
    if (tv.root() != root) {
        auto sharedRoot = std::shared_ptr<TreeNode>(root, [](TreeNode*){});
        tv.setRoot(sharedRoot);
    }
    
    TreeViewEvents events;
    events.onSelect = onSelect;
    
    tv.render(id, bounds, options, events);
}

} // namespace fst
