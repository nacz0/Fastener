#include "fastener/fastener.h"
#include <string>
#include <unordered_map>
#include "fastener/widgets/text_editor.h"
#include "fastener/widgets/splitter.h"

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
    
    // Create menu bar
    fst::MenuBar menuBar;
    std::string statusText = "Ready";
    
    std::unordered_map<std::string, fst::TextEditor> editors;
    auto getOrCreateEditor = [&](const std::string& id) -> fst::TextEditor& {
        auto it = editors.find(id);
        if (it == editors.end()) {
            it = editors.emplace(id, fst::TextEditor()).first;
            if (id == "main.cpp") {
                it->second.setText("// main.cpp\n\n#include <iostream>\n\nint main() {\n    std::cout << \"Hello, Fastener!\" << std::endl;\n    return 0;\n}\n");
            } else if (id == "types.cpp") {
                it->second.setText("// types.cpp\n#include \"fastener/core/types.h\"\n\nnamespace fst {\n    // Implementation here\n}\n");
            } else if (id == "context.cpp") {
                it->second.setText("// context.cpp\n#include \"fastener/core/context.h\"\n\nnamespace fst {\n    // Implementation here\n}\n");
            }

            // Example of external StyleProvider (syntax highlighting moved to application level)
            it->second.setStyleProvider([](int lineIndex, const std::string& text) {
                std::vector<fst::TextSegment> segments;
                static const std::vector<std::string> keywords = {
                    "int", "void", "float", "bool", "char", "if", "else", "for", "while", "return",
                    "namespace", "class", "struct", "public", "private", "protected", "static", "const", "using", "include"
                };

                std::string word;
                for (int i = 0; i < (int)text.length(); ++i) {
                    char ch = text[i];
                    if (isalnum(ch) || ch == '_') {
                        word += ch;
                    } else {
                        if (!word.empty()) {
                            if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
                                segments.push_back({i - (int)word.length(), i, fst::Color(86, 156, 214)});
                            } else if (isdigit(word[0])) {
                                segments.push_back({i - (int)word.length(), i, fst::Color(181, 206, 168)});
                            }
                            word.clear();
                        }
                        if (ch == '#' || ch == '<' || ch == '>' || ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']') {
                            segments.push_back({i, i + 1, fst::Color(212, 212, 212)});
                        }
                    }
                }
                if (!word.empty()) {
                    if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
                        segments.push_back({(int)text.length() - (int)word.length(), (int)text.length(), fst::Color(86, 156, 214)});
                    }
                }
                return segments;
            });
        }
        return it->second;
    };
    
    // File menu
    menuBar.addMenu("File", {
        fst::MenuItem("new", "New File", [&]() { 
            static int n = 1;
            std::string name = "untitled" + std::to_string(n++) + ".cpp";
            tabs.addTab(name, name);
            statusText = "Created: " + name;
        }).withShortcut("Ctrl+N"),
        fst::MenuItem("open", "Open File...").withShortcut("Ctrl+O").disabled(),
        fst::MenuItem("save", "Save", [&]() { statusText = "Saved!"; }).withShortcut("Ctrl+S"),
        fst::MenuItem("saveAs", "Save As...").withShortcut("Ctrl+Shift+S"),
        fst::MenuItem::separator(),
        fst::MenuItem("exit", "Exit", [&]() { window.close(); }).withShortcut("Alt+F4")
    });
    
    // Edit menu
    menuBar.addMenu("Edit", {
        fst::MenuItem("undo", "Undo").withShortcut("Ctrl+Z").disabled(),
        fst::MenuItem("redo", "Redo").withShortcut("Ctrl+Y").disabled(),
        fst::MenuItem::separator(),
        fst::MenuItem("cut", "Cut").withShortcut("Ctrl+X"),
        fst::MenuItem("copy", "Copy").withShortcut("Ctrl+C"),
        fst::MenuItem("paste", "Paste").withShortcut("Ctrl+V"),
        fst::MenuItem::separator(),
        fst::MenuItem("find", "Find...").withShortcut("Ctrl+F"),
        fst::MenuItem("replace", "Replace...").withShortcut("Ctrl+H")
    });
    
    // View menu
    fst::MenuItem appearance = fst::MenuItem::submenu("appearance", "Appearance");
    appearance.add(fst::MenuItem::checkbox("fullScreen", "Full Screen", false));
    appearance.add(fst::MenuItem::checkbox("distractionFree", "Distraction Free", false));
    appearance.add(fst::MenuItem::separator());
    appearance.add(fst::MenuItem("theme", "Color Theme..."));

    menuBar.addMenu("View", {
        fst::MenuItem::checkbox("explorer", "Explorer", true),
        fst::MenuItem::checkbox("terminal", "Terminal", false),
        fst::MenuItem::separator(),
        appearance,
        fst::MenuItem::separator(),
        fst::MenuItem("zoomIn", "Zoom In").withShortcut("Ctrl++"),
        fst::MenuItem("zoomOut", "Zoom Out").withShortcut("Ctrl+-")
    });
    
    // Help menu
    menuBar.addMenu("Help", {
        fst::MenuItem("about", "About Fastener", [&]() { 
            statusText = "Fastener v0.1.0 - High-performance C++ GUI"; 
        })
    });
    
    while (window.isOpen()) {
        window.pollEvents();
        
        if (window.input().isKeyPressed(fst::Key::Escape)) {
            if (menuBar.isOpen()) {
                menuBar.closeAll();
            } else if (fst::IsContextMenuOpen()) {
                fst::CloseContextMenu();
            } else {
                window.close();
            }
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
        float menuBarHeight = 28.0f;
        float windowW = static_cast<float>(window.width());
        float windowH = static_cast<float>(window.height());
        const float statusBarHeight = 24.0f;

        static float sidebarRatio = 250.0f;
        static float terminalHeight = 150.0f;
        static bool showTerminal = true;
        
        // === MENU BAR ===
        menuBar.render(fst::Rect(0, 0, windowW, menuBarHeight));
        
        // === MAIN AREA (Sidebar + Editor/Terminal) ===
        fst::Rect mainArea(0, menuBarHeight, windowW, windowH - menuBarHeight - statusBarHeight);
        
        // === SIDEBAR (File Explorer) ===
        fst::Rect sidebarRect(mainArea.x(), mainArea.y(), sidebarRatio, mainArea.height());
        dl.addRectFilled(sidebarRect, theme.colors.panelBackground.darker(0.03f));
        
        // Sidebar title
        if (ctx.font()) {
            dl.addText(ctx.font(), fst::Vec2(sidebarRect.x() + 10, sidebarRect.y() + 8), "EXPLORER", theme.colors.textSecondary);
        }
        
        // TreeView
        fst::Rect treeRect(sidebarRect.x(), sidebarRect.y() + 30, sidebarRect.width(), sidebarRect.height() - 30);
        
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
        treeEvents.onContextMenu = [&](fst::TreeNode* node) {
            fst::ShowContextMenu({
                fst::MenuItem("open", "Open"),
                fst::MenuItem("rename", "Rename"),
                fst::MenuItem::separator(),
                fst::MenuItem("delete", "Delete").withShortcut("Del")
            }, ctx.input().mousePos());
        };
        
        fileTree.render("explorer", treeRect, treeOpts, treeEvents);
        
        // Sidebar border is now handled by the splitter visual, but we can add a subtle one if needed
        // dl.addRectFilled(fst::Rect(sidebarRect.right() - 1, sidebarRect.y(), 1, sidebarRect.height()), theme.colors.border);
        
        // === RIGHT AREA (Editor + Terminal) ===
        fst::Rect rightArea(sidebarRect.right(), mainArea.y(), mainArea.width() - sidebarRect.width(), mainArea.height());
        
        float currentTerminalHeight = showTerminal ? terminalHeight : 0.0f;
        float editorHeight = rightArea.height() - currentTerminalHeight;
        
        fst::Rect editorArea(rightArea.x(), rightArea.y(), rightArea.width(), editorHeight);
        
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
        if (tabs.selectedTab()) {
            fst::TextEditor& editor = getOrCreateEditor(tabs.selectedTab()->id);
            editor.render(contentRect);
        } else {
            dl.addRectFilled(contentRect, theme.colors.panelBackground);
            if (ctx.font()) {
                dl.addText(ctx.font(), fst::Vec2(contentRect.x() + 20, contentRect.y() + 20),
                           "No file open", theme.colors.textSecondary);
            }
        }

        // === TERMINAL AREA ===
        if (showTerminal) {
            fst::Rect terminalArea(rightArea.x(), rightArea.y() + editorHeight, rightArea.width(), terminalHeight);
            dl.addRectFilled(terminalArea, theme.colors.windowBackground.darker(0.1f));
            
            // Terminal top border
            dl.addRectFilled(fst::Rect(terminalArea.x(), terminalArea.y(), terminalArea.width(), 1), theme.colors.border);
            
            if (ctx.font()) {
                dl.addText(ctx.font(), fst::Vec2(terminalArea.x() + 10, terminalArea.y() + 10), 
                           "TERMINAL", theme.colors.textSecondary);
                dl.addText(ctx.font(), fst::Vec2(terminalArea.x() + 10, terminalArea.y() + 35), 
                           "PS C:\\Users\\natma\\Documents\\GitHub\\Fastener> _", theme.colors.text);
            }
        }

        // === SPLITTERS (Rendered last to stay on top and capture input) ===
        
        // Sidebar Splitter
        fst::SplitterOptions sidebarSplitterOpts;
        sidebarSplitterOpts.direction = fst::Direction::Vertical;
        sidebarSplitterOpts.minSize1 = 100.0f;
        sidebarSplitterOpts.minSize2 = 200.0f;
        fst::Splitter("sidebar_splitter", sidebarRatio, mainArea, sidebarSplitterOpts);

        // Terminal Splitter
        if (showTerminal) {
            float splitY = rightArea.height() - terminalHeight;
            fst::SplitterOptions terminalSplitterOpts;
            terminalSplitterOpts.direction = fst::Direction::Horizontal;
            terminalSplitterOpts.minSize1 = 100.0f;
            terminalSplitterOpts.minSize2 = 50.0f;
            if (fst::Splitter("terminal_splitter", splitY, rightArea, terminalSplitterOpts)) {
                terminalHeight = rightArea.height() - splitY;
            }
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
        
        // Render popups on top of everything
        menuBar.renderPopups();
        fst::RenderContextMenu();
        
        ctx.endFrame();
        window.swapBuffers();
    }
    
    return 0;
}
