#include "fastener/fastener.h"
#include <string>

int main() {
    // Create window
    fst::WindowConfig config;
    config.title = "Fastener Demo - TreeView";
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
    
    // Load font from Windows Fonts directory
    ctx.loadFont("C:/Windows/Fonts/arial.ttf", 14.0f);
    
    // Create sample tree structure (file explorer style)
    fst::TreeView fileTree;
    auto root = fileTree.root();
    
    // Add folders and files
    auto src = root->addChild("src", "src");
    auto srcCore = src->addChild("src/core", "core");
    srcCore->addChild("src/core/types.cpp", "types.cpp", true);
    srcCore->addChild("src/core/context.cpp", "context.cpp", true);
    srcCore->addChild("src/core/input.cpp", "input.cpp", true);
    
    auto srcGraphics = src->addChild("src/graphics", "graphics");
    srcGraphics->addChild("src/graphics/renderer.cpp", "renderer.cpp", true);
    srcGraphics->addChild("src/graphics/draw_list.cpp", "draw_list.cpp", true);
    srcGraphics->addChild("src/graphics/font.cpp", "font.cpp", true);
    srcGraphics->addChild("src/graphics/texture.cpp", "texture.cpp", true);
    
    auto srcWidgets = src->addChild("src/widgets", "widgets");
    srcWidgets->addChild("src/widgets/button.cpp", "button.cpp", true);
    srcWidgets->addChild("src/widgets/label.cpp", "label.cpp", true);
    srcWidgets->addChild("src/widgets/tree_view.cpp", "tree_view.cpp", true);
    
    auto include = root->addChild("include", "include");
    auto incFastener = include->addChild("include/fastener", "fastener");
    incFastener->addChild("include/fastener/fastener.h", "fastener.h", true);
    
    auto examples = root->addChild("examples", "examples");
    auto demo = examples->addChild("examples/demo", "demo");
    demo->addChild("examples/demo/main.cpp", "main.cpp", true);
    
    root->addChild("CMakeLists.txt", "CMakeLists.txt", true);
    root->addChild("README.md", "README.md", true);
    root->addChild("LICENSE", "LICENSE", true);
    
    // Expand some folders by default
    src->isExpanded = true;
    srcCore->isExpanded = true;
    include->isExpanded = true;
    
    // Selected node info
    std::string selectedPath = "No selection";
    
    while (window.isOpen()) {
        window.pollEvents();
        
        // Handle escape to close
        if (window.input().isKeyPressed(fst::Key::Escape)) {
            window.close();
        }
        
        // Begin frame
        ctx.beginFrame(window);
        
        // Draw background
        fst::DrawList& dl = ctx.drawList();
        dl.addRectFilled(
            fst::Rect(0, 0, static_cast<float>(window.width()), static_cast<float>(window.height())),
            ctx.theme().colors.windowBackground
        );
        
        // Title
        if (ctx.font()) {
            dl.addText(ctx.font(), fst::Vec2(20, 20), "Fastener - TreeView Demo", 
                       ctx.theme().colors.text);
            dl.addText(ctx.font(), fst::Vec2(20, 45), "Click folders to expand/collapse, files to select",
                       ctx.theme().colors.textSecondary);
        }
        
        // TreeView panel
        fst::Rect treeRect(20, 80, 350, 600);
        
        fst::TreeViewOptions options;
        options.rowHeight = 26.0f;
        options.indentWidth = 20.0f;
        options.showIcons = true;
        
        fst::TreeViewEvents events;
        events.onSelect = [&](fst::TreeNode* node) {
            selectedPath = "Selected: " + node->id;
        };
        events.onDoubleClick = [&](fst::TreeNode* node) {
            if (node->isLeaf) {
                selectedPath = "Opened: " + node->id;
            }
        };
        
        fileTree.render("file_tree", treeRect, options, events);
        
        // Info panel
        fst::Rect infoRect(400, 80, 400, 200);
        dl.addRectFilled(infoRect, ctx.theme().colors.panelBackground, 8.0f);
        dl.addRect(infoRect, ctx.theme().colors.border, 8.0f);
        
        if (ctx.font()) {
            dl.addText(ctx.font(), fst::Vec2(420, 100), "File Info", ctx.theme().colors.text);
            dl.addText(ctx.font(), fst::Vec2(420, 130), selectedPath, ctx.theme().colors.textSecondary);
            
            if (fileTree.selectedNode()) {
                std::string nodeType = fileTree.selectedNode()->isLeaf ? "File" : "Folder";
                dl.addText(ctx.font(), fst::Vec2(420, 160), "Type: " + nodeType, 
                           ctx.theme().colors.textSecondary);
            }
        }
        
        // End frame
        ctx.endFrame();
        window.swapBuffers();
    }
    
    return 0;
}
