#include "fastener/fastener.h"
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

using namespace fst;

int main() {
    // Create window
    WindowConfig config;
    config.title = "Fastener IDE Demo";
    config.width = 1280;
    config.height = 720;
    config.vsync = true;
    
    Window window(config);
    if (!window.isOpen()) {
        return 1;
    }
    
    // Initialize context
    Context ctx;
    ctx.setTheme(Theme::dark());
    
    // Load font
    ctx.loadFont("C:/Windows/Fonts/arial.ttf", 14.0f);
    
    // Create file tree
    TreeView fileTree;
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
    TabControl tabs;
    tabs.addTab("main.cpp", "main.cpp", true);
    tabs.addTab("scroll_demo", "Scroll Demo", true);
    tabs.addTab("settings_demo", "Settings", true);
    tabs.addTab("types.cpp", "types.cpp", true);
    tabs.addTab("context.cpp", "context.cpp", true);
    
    // Mark one as modified
    if (auto tab = tabs.getTab(1)) {
        tab->modified = true;
    }
    
    // Create menu bar
    MenuBar menuBar;
    std::string statusText = "Ready";
    
    std::unordered_map<std::string, TextEditor> editors;
    std::unordered_map<std::string, ScrollArea> scrollAreas;
    
    // Settings state
    bool checkValue1 = true;
    bool checkValue2 = false;
    int selectedCombo = 1;
    std::vector<std::string> comboOptions = {"Disabled", "Fast", "Balanced", "Quality", "Ultra"};
    float progressVal = 0.45f;
    float indeterminateProgress = 0.0f;
    Color pickerColor = Color::fromHex(0x3498DB);

    auto getOrCreateEditor = [&](const std::string& id) -> TextEditor& {
        auto it = editors.find(id);
        if (it == editors.end()) {
            it = editors.emplace(id, TextEditor()).first;
            if (id == "main.cpp") {
                it->second.setText("// main.cpp\n\n#include <iostream>\n\nint main() {\n    std::cout << \"Hello, Fastener!\" << std::endl;\n    return 0;\n}\n");
            } else if (id == "types.cpp") {
                it->second.setText("// types.cpp\n#include \"fastener/core/types.h\"\n\nnamespace fst {\n    // Implementation here\n}\n");
            } else if (id == "context.cpp") {
                it->second.setText("// context.cpp\n#include \"fastener/core/context.h\"\n\nnamespace fst {\n    // Implementation here\n}\n");
            }

            it->second.setStyleProvider([](int lineIndex, const std::string& text) {
                std::vector<TextSegment> segments;
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
                                segments.push_back({i - (int)word.length(), i, Color(86, 156, 214)});
                            } else if (isdigit(word[0])) {
                                segments.push_back({i - (int)word.length(), i, Color(181, 206, 168)});
                            }
                            word.clear();
                        }
                        if (ch == '#' || ch == '<' || ch == '>' || ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']') {
                            segments.push_back({i, i + 1, Color(212, 212, 212)});
                        }
                    }
                }
                if (!word.empty()) {
                    if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
                        segments.push_back({(int)text.length() - (int)word.length(), (int)text.length(), Color(86, 156, 214)});
                    }
                }
                return segments;
            });
        }
        return it->second;
    };
    
    // Menus
    menuBar.addMenu("File", {
        MenuItem("new", "New File", [&]() { 
            static int n = 1;
            std::string name = "untitled" + std::to_string(n++) + ".cpp";
            tabs.addTab(name, name);
            statusText = "Created: " + name;
        }).withShortcut("Ctrl+N"),
        MenuItem("open", "Open File...").withShortcut("Ctrl+O").disabled(),
        MenuItem("save", "Save", [&]() { statusText = "Saved!"; }).withShortcut("Ctrl+S"),
        MenuItem("saveAs", "Save As...").withShortcut("Ctrl+Shift+S"),
        MenuItem::separator(),
        MenuItem("exit", "Exit", [&]() { window.close(); }).withShortcut("Alt+F4")
    });
    
    menuBar.addMenu("Edit", {
        MenuItem("undo", "Undo").withShortcut("Ctrl+Z").disabled(),
        MenuItem("redo", "Redo").withShortcut("Ctrl+Y").disabled(),
        MenuItem::separator(),
        MenuItem("cut", "Cut").withShortcut("Ctrl+X"),
        MenuItem("copy", "Copy").withShortcut("Ctrl+C"),
        MenuItem("paste", "Paste").withShortcut("Ctrl+V")
    });
    
    MenuItem appearance = MenuItem::submenu("appearance", "Appearance");
    appearance.add(MenuItem::checkbox("fullScreen", "Full Screen", false));
    appearance.add(MenuItem("theme", "Color Theme..."));

    menuBar.addMenu("View", {
        MenuItem::checkbox("explorer", "Explorer", true),
        MenuItem::checkbox("terminal", "Terminal", false),
        MenuItem::separator(),
        appearance
    });
    
    menuBar.addMenu("Help", {
        MenuItem("about", "About Fastener", [&]() { 
            statusText = "Fastener v0.1.0 - High-performance C++ GUI"; 
        })
    });
    
    auto renderFrame = [&]() {
        ctx.beginFrame(window);
        
        DrawList& dl = ctx.drawList();
        const Theme& theme = ctx.theme();
        
        dl.addRectFilled(
            Rect(0, 0, static_cast<float>(window.width()), static_cast<float>(window.height())),
            theme.colors.windowBackground
        );
        
        float menuBarHeight = 28.0f;
        float windowW = static_cast<float>(window.width());
        float windowH = static_cast<float>(window.height());
        const float statusBarHeight = 24.0f;

        static float sidebarRatio = 250.0f;
        static float terminalHeight = 150.0f;
        static bool showTerminal = true;
        
        menuBar.render(Rect(0, 0, windowW, menuBarHeight));
        
        Rect mainArea(0, menuBarHeight, windowW, windowH - menuBarHeight - statusBarHeight);
        Rect sidebarRect(mainArea.x(), mainArea.y(), sidebarRatio, mainArea.height());
        dl.addRectFilled(sidebarRect, theme.colors.panelBackground.darker(0.03f));
        
        if (ctx.font()) {
            dl.addText(ctx.font(), Vec2(sidebarRect.x() + 10, sidebarRect.y() + 8), "EXPLORER", theme.colors.textSecondary);
        }
        
        Rect treeRect(sidebarRect.x(), sidebarRect.y() + 30, sidebarRect.width(), sidebarRect.height() - 30);
        TreeViewOptions treeOpts;
        treeOpts.rowHeight = 24.0f;
        
        TreeViewEvents treeEvents;
        treeEvents.onDoubleClick = [&](TreeNode* node) {
            if (node->isLeaf) {
                if (tabs.findTabIndex(node->id) < 0) {
                    tabs.addTab(node->id, node->label, true);
                }
                tabs.selectTabById(node->id);
                statusText = "Opened: " + node->label;
            }
        };
        
        fileTree.render("explorer", treeRect, treeOpts, treeEvents);
        
        Rect rightArea(sidebarRect.right(), mainArea.y(), mainArea.width() - sidebarRect.width(), mainArea.height());
        float currentTerminalHeight = showTerminal ? terminalHeight : 0.0f;
        float editorHeight = rightArea.height() - currentTerminalHeight;
        Rect editorArea(rightArea.x(), rightArea.y(), rightArea.width(), editorHeight);
        
        TabControlOptions tabOpts;
        tabOpts.tabHeight = 32.0f;
        tabOpts.showCloseButtons = true;
        
        TabControlEvents tabEvents;
        tabEvents.onSelect = [&](int index, const TabItem& tab) {
            statusText = "Switched to: " + tab.label;
        };
        tabEvents.onClose = [&](int index, const TabItem& tab) {
            tabs.removeTab(index);
        };
        
        Rect contentRect = tabs.render("editor_tabs", editorArea, tabOpts, tabEvents);
        
        if (tabs.selectedTab()) {
            const std::string& tabId = tabs.selectedTab()->id;
            if (tabId == "scroll_demo") {
                auto& sa = scrollAreas["demo"];
                sa.setContentSize(Vec2(2000, 2000));
                sa.render("scroll_demo_widget", contentRect, [&](const Rect& viewport) {
                    for (int i = 0; i < 20; ++i) {
                        Rect itemRect(viewport.x() + 20, viewport.y() + 20 + i * 40 - sa.scrollOffset().y, 100, 30);
                        dl.addRectFilled(itemRect, Color(60, 60, 80), 4.0f);
                    }
                });
            } else if (tabId == "settings_demo") {
                // Bridge manual layout -> automatic layout
                ctx.layout().beginContainer(contentRect);
                
                PanelOptions settingsPanelOpts;
                // Make panel fill the tab content area
                settingsPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
                // Set explicit coordinates since we are the root of this layout container, 
                // but actually 'allocate' will do the right thing if we are inside a container.
                // However, Panel defaults to -1, -1. allocate() inside beginContainer will return contentRect.pos.
                // So this is perfect.
                
                Panel("SettingsPanel", settingsPanelOpts) {
                    LabelOptions titleOpts;
                    titleOpts.color = theme.colors.primary;
                    Label("WIDGET DEMO", titleOpts);
                    Spacing(10);
                    
                    LabelOptions sectionOpts;
                    sectionOpts.color = theme.colors.textSecondary;

                    BeginVertical(10);
                        Label("Checkboxes:", sectionOpts);
                        Checkbox("Show Line Numbers", checkValue1);
                        Checkbox("Word Wrap", checkValue2);
                    EndVertical();
                    
                    Spacing(20);
                    
                    BeginVertical(10);
                        Label("Select Optimization:", sectionOpts);
                        ComboBoxOptions comboOpts;
                        comboOpts.style = Style().withWidth(300);
                        ComboBox("Performance", selectedCombo, comboOptions, comboOpts);
                    EndVertical();
                    
                    Spacing(20);
                    
                    BeginVertical(10);
                        Label("Progress Indicators:", sectionOpts);
                        progressVal = std::fmod(ctx.time() * 0.1f, 1.0f);
                        ProgressBarOptions pb1; pb1.style = Style().withWidth(400);
                        ProgressBar("Task Progress", progressVal, pb1);
                        
                        ProgressBarOptions pb2; 
                        pb2.style = Style().withWidth(400);
                        pb2.indeterminate = true;
                        ProgressBar("Background Sync", 0.0f, pb2);
                    EndVertical();
                    
                    Spacing(20);
                    
                    BeginVertical(10);
                        Label("Tooltips & Buttons:", sectionOpts);
                        BeginHorizontal(10);
                            if (Button("Save")) { statusText = "Save clicked!"; }
                            Tooltip("Save current document (Ctrl+S)");
                            if (Button("Build")) { statusText = "Build clicked!"; }
                            Tooltip("Compile and link the project (F5)");
                        EndHorizontal();
                    EndVertical();
                    
                    Spacing(20);

                    BeginVertical(10);
                        Label("Color Selection:", sectionOpts);
                        ColorPicker("Accent Color", pickerColor);
                    EndVertical();

                    Spacing(20);
                    
                    BeginHorizontal(10);
                        Label("Help Marker:", sectionOpts);
                        HelpMarker("This is a help marker. It uses automatic layout!");
                    EndHorizontal();
                }
                
                ctx.layout().endContainer();
            } else {
                TextEditor& editor = getOrCreateEditor(tabId);
                editor.render(contentRect);
            }
        } else {
            dl.addRectFilled(contentRect, theme.colors.panelBackground);
            if (ctx.font()) {
                dl.addText(ctx.font(), Vec2(contentRect.x() + 20, contentRect.y() + 20), "No file open", theme.colors.textSecondary);
            }
        }

        if (showTerminal) {
            Rect terminalArea(rightArea.x(), rightArea.y() + editorHeight, rightArea.width(), terminalHeight);
            dl.addRectFilled(terminalArea, theme.colors.windowBackground.darker(0.1f));
            dl.addRectFilled(Rect(terminalArea.x(), terminalArea.y(), terminalArea.width(), 1), theme.colors.border);
            if (ctx.font()) {
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 10), "TERMINAL", theme.colors.textSecondary);
            }
        }

        SplitterOptions sidebarSplitterOpts;
        sidebarSplitterOpts.direction = Direction::Vertical;
        Splitter("sidebar_splitter", sidebarRatio, mainArea, sidebarSplitterOpts);

        if (showTerminal) {
            float splitY = rightArea.height() - terminalHeight;
            SplitterOptions terminalSplitterOpts;
            terminalSplitterOpts.direction = Direction::Horizontal;
            if (Splitter("terminal_splitter", splitY, rightArea, terminalSplitterOpts)) {
                terminalHeight = rightArea.height() - splitY;
            }
        }

        Rect statusRect(0, windowH - statusBarHeight, windowW, statusBarHeight);
        dl.addRectFilled(statusRect, theme.colors.primary);
        if (ctx.font()) {
            dl.addText(ctx.font(), Vec2(10, statusRect.y() + 4), statusText, theme.colors.primaryText);
        }
        
        menuBar.renderPopups();
        RenderContextMenu();
        
        ctx.endFrame();
        window.swapBuffers();
    };

    window.setRefreshCallback(renderFrame);
    
    while (window.isOpen()) {
        window.pollEvents();
        if (window.input().isKeyPressed(Key::Escape)) {
            if (menuBar.isOpen()) menuBar.closeAll();
            else if (IsContextMenuOpen()) CloseContextMenu();
            else window.close();
        }
        renderFrame();
    }
    
    return 0;
}
