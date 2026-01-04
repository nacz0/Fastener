#include "fastener/fastener.h"
#include <string>

int main() {
    // Create window
    fst::WindowConfig config;
    config.title = "Fastener IDE Demo";
    config.width = 1280;
    config.height = 720;
    config.vsync = true;
    
    fst::Window window(config);
    if (!window.isOpen()) {
        return 1;
    }
    
    // Initialize context
    fst::Context ctx;
    ctx.setTheme(fst::Theme::dark());
    
    // Load font
    ctx.loadFont("C:/Windows/Fonts/arial.ttf", 14.0f);
    
    // Create file tree
    fst::TreeView fileTree;
    auto root = fileTree.root();
    
    auto src = root->addChild("src", "src");
    auto srcCore = src->addChild("src/core", "core");
    srcCore->addChild("src/core/types.cpp", "types.cpp", true);
    srcCore->addChild("src/core/context.cpp", "context.cpp", true);
    srcCore->addChild("src/core/input.cpp", "input.cpp", true);
    
    auto srcWidgets = src->addChild("src/widgets", "widgets");
    srcWidgets->addChild("src/widgets/button.cpp", "button.cpp", true);
    srcWidgets->addChild("src/widgets/tree_view.cpp", "tree_view.cpp", true);
    srcWidgets->addChild("src/widgets/tab_control.cpp", "tab_control.cpp", true);
    
    auto include = root->addChild("include", "include");
    include->addChild("include/fastener.h", "fastener.h", true);
    
    root->addChild("CMakeLists.txt", "CMakeLists.txt", true);
    root->addChild("README.md", "README.md", true);
    
    src->isExpanded = true;
    srcCore->isExpanded = true;
    
    // Create tab control for open files
    fst::TabControl tabs;
    tabs.addTab("main.cpp", "main.cpp", true);
    tabs.addTab("types.cpp", "types.cpp", true);
    tabs.addTab("context.cpp", "context.cpp", true);
    
    // Mark one as modified
    if (auto tab = tabs.getTab(1)) {
        tab->modified = true;
    }
    
    std::string statusText = "Ready";
    
    while (window.isOpen()) {
        window.pollEvents();
        
        if (window.input().isKeyPressed(fst::Key::Escape)) {
            window.close();
        }
        
        ctx.beginFrame(window);
        
        fst::DrawList& dl = ctx.drawList();
        const fst::Theme& theme = ctx.theme();
        
        // Background
        dl.addRectFilled(
            fst::Rect(0, 0, static_cast<float>(window.width()), static_cast<float>(window.height())),
            theme.colors.windowBackground
        );
        
        // Layout constants
        float sidebarWidth = 250.0f;
        float statusBarHeight = 24.0f;
        float windowW = static_cast<float>(window.width());
        float windowH = static_cast<float>(window.height());
        
        // === SIDEBAR (File Explorer) ===
        fst::Rect sidebarRect(0, 0, sidebarWidth, windowH - statusBarHeight);
        dl.addRectFilled(sidebarRect, theme.colors.panelBackground.darker(0.03f));
        
        // Sidebar title
        if (ctx.font()) {
            dl.addText(ctx.font(), fst::Vec2(10, 8), "EXPLORER", theme.colors.textSecondary);
        }
        
        // TreeView
        fst::Rect treeRect(0, 30, sidebarWidth, sidebarRect.height() - 30);
        
        fst::TreeViewOptions treeOpts;
        treeOpts.rowHeight = 24.0f;
        treeOpts.indentWidth = 16.0f;
        
        fst::TreeViewEvents treeEvents;
        treeEvents.onDoubleClick = [&](fst::TreeNode* node) {
            if (node->isLeaf) {
                // Open file in new tab
                if (tabs.findTabIndex(node->id) < 0) {
                    tabs.addTab(node->id, node->label, true);
                }
                tabs.selectTabById(node->id);
                statusText = "Opened: " + node->label;
            }
        };
        treeEvents.onSelect = [&](fst::TreeNode* node) {
            statusText = "Selected: " + node->label;
        };
        
        fileTree.render("explorer", treeRect, treeOpts, treeEvents);
        
        // Sidebar border
        dl.addRectFilled(fst::Rect(sidebarWidth - 1, 0, 1, sidebarRect.height()), theme.colors.border);
        
        // === EDITOR AREA ===
        fst::Rect editorArea(sidebarWidth, 0, windowW - sidebarWidth, windowH - statusBarHeight);
        
        // TabControl
        fst::TabControlOptions tabOpts;
        tabOpts.tabHeight = 32.0f;
        tabOpts.showCloseButtons = true;
        tabOpts.showAddButton = true;
        
        fst::TabControlEvents tabEvents;
        tabEvents.onSelect = [&](int index, const fst::TabItem& tab) {
            statusText = "Switched to: " + tab.label;
        };
        tabEvents.onClose = [&](int index, const fst::TabItem& tab) {
            statusText = "Closed: " + tab.label;
            tabs.removeTab(index);
        };
        tabEvents.onAdd = [&]() {
            static int newFileCount = 1;
            std::string name = "untitled" + std::to_string(newFileCount++) + ".cpp";
            tabs.addTab(name, name, true);
            tabs.selectTab(tabs.tabCount() - 1);
            statusText = "Created: " + name;
        };
        
        fst::Rect contentRect = tabs.render("editor_tabs", editorArea, tabOpts, tabEvents);
        
        // Editor content area
        dl.addRectFilled(contentRect, theme.colors.panelBackground);
        
        // Show file content placeholder
        if (ctx.font() && tabs.selectedTab()) {
            dl.addText(ctx.font(), fst::Vec2(contentRect.x() + 20, contentRect.y() + 20),
                       "// " + tabs.selectedTab()->label, theme.colors.textSecondary);
            dl.addText(ctx.font(), fst::Vec2(contentRect.x() + 20, contentRect.y() + 50),
                       "// File content would appear here", theme.colors.textSecondary);
            dl.addText(ctx.font(), fst::Vec2(contentRect.x() + 20, contentRect.y() + 80),
                       "// Double-click files in Explorer to open them", theme.colors.textSecondary);
        }
        
        // === STATUS BAR ===
        fst::Rect statusRect(0, windowH - statusBarHeight, windowW, statusBarHeight);
        dl.addRectFilled(statusRect, theme.colors.primary);
        
        if (ctx.font()) {
            dl.addText(ctx.font(), fst::Vec2(10, statusRect.y() + 4), statusText, 
                       theme.colors.primaryText);
            
            // Right side info
            std::string info = "Ln 1, Col 1 | UTF-8 | C++";
            float infoWidth = ctx.font()->measureText(info).x;
            dl.addText(ctx.font(), fst::Vec2(windowW - infoWidth - 10, statusRect.y() + 4),
                       info, theme.colors.primaryText);
        }
        
        ctx.endFrame();
        window.swapBuffers();
    }
    
    return 0;
}
