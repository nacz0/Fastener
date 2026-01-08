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
    tabs.addTab("input_demo", "Input Demo", true);
    tabs.addTab("new_widgets_demo", "New Widgets", true);
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
    
    // Slider state
    float sliderValue1 = 50.0f;
    float sliderValue2 = 0.75f;
    int sliderValueInt = 5;
    
    // TextInput state
    std::string textInputValue = "Hello, Fastener!";
    std::string passwordValue = "secret123";
    std::string searchValue = "";

    // New Priority 2 Widgets state
    int listboxSelection = 0;
    std::vector<int> listboxMultiSelection;
    std::vector<std::string> listboxItems = {"Option Alpha", "Option Beta", "Option Gamma", "Option Delta", "Option Epsilon", "Option Zeta", "Option Eta", "Option Theta"};
    bool selectable1 = false, selectable2 = true, selectable3 = false;
    std::string textAreaContent = "Welcome to TextArea!\n\nThis is a multi-line text input widget.\nYou can type here, navigate with arrows,\nand scroll through long content.\n\n- Supports keyboard navigation\n- Auto-scrolling\n- Line numbers (optional)";

    // New Priority 3 Widgets state
    int radioSelection = 0;
    int inputNumberValue = 5;
    float inputNumberFloat = 2.5f;
    bool collapsingOpen1 = true;
    bool collapsingOpen2 = false;

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
    
    // Initialize default dock layout
    static bool layoutInitialized = false;
    auto initDockLayout = [&]() {
        DockNode::Id mainDockId = DockBuilder::GetDockSpaceId("##MainDockSpace");
        DockBuilder::Begin(mainDockId);
        DockBuilder::ClearDockSpace(mainDockId);
        
        // Split Root into Left and Right
        DockNode::Id leftNode = DockBuilder::SplitNode(mainDockId, DockDirection::Left, 0.2f);
        DockNode::Id rightNode = DockBuilder::GetNode(mainDockId, DockDirection::Right);
        
        // Split Right into Center and Bottom
        DockNode::Id bottomNode = DockBuilder::SplitNode(rightNode, DockDirection::Bottom, 0.25f);
        DockNode::Id centralNode = DockBuilder::GetNode(rightNode, DockDirection::Top);
        
        DockBuilder::DockWindow("Explorer", leftNode);
        DockBuilder::DockWindow("Terminal", bottomNode);
        DockBuilder::DockWindow("Editor", centralNode);
        DockBuilder::DockWindow("Settings", centralNode);
        DockBuilder::DockWindow("Input Demo", centralNode);
        DockBuilder::DockWindow("New Widgets", centralNode);
        
        DockBuilder::Finish();
        layoutInitialized = true;
    };
    
    auto renderFrame = [&]() {
        ctx.beginFrame(window);
        
        if (!layoutInitialized) {
            initDockLayout();
        }

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

        menuBar.render(Rect(0, 0, windowW, menuBarHeight));
        
        // Define the main docking area between menu bar and status bar
        Rect mainDockArea(0, menuBarHeight, windowW, windowH - menuBarHeight - statusBarHeight);
        DockSpace("##MainDockSpace", mainDockArea);

        // --- Dockable Windows ---

        // Explorer Window
        DockableWindow("Explorer") {
            if (ctx.font()) {
                dl.addText(ctx.font(), Vec2(ctx.layout().currentBounds().x() + 10, ctx.layout().currentBounds().y() + 8), "EXPLORER", theme.colors.textSecondary);
            }
            
            Rect treeRect = ctx.layout().currentBounds();
            treeRect = Rect(treeRect.x(), treeRect.y() + 30, treeRect.width(), treeRect.height() - 30);
            
            TreeViewOptions treeOpts;
            treeOpts.rowHeight = 24.0f;
            
            TreeViewEvents treeEvents;
            treeEvents.onDoubleClick = [&](TreeNode* node) {
                if (node->isLeaf) {
                    // Logic to activate editor for this file would go here
                    statusText = "Opened: " + node->label;
                }
            };
            
            fileTree.render("explorer_tree", treeRect, treeOpts, treeEvents);
        }

        // Terminal Window
        DockableWindow("Terminal") {
            Rect terminalArea = ctx.layout().currentBounds();
            dl.addRectFilled(terminalArea, theme.colors.windowBackground.darker(0.1f));
            if (ctx.font()) {
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 10), "TERMINAL", theme.colors.textSecondary);
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 40), "> cmake --build build", theme.colors.text);
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 60), "[100%] Built target fastener_demo", theme.colors.success);
            }
        }

        // Settings Window
        DockableWindow("Settings") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions settingsPanelOpts;
            settingsPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
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
                EndVertical();
                
                Spacing(20);
                
                BeginVertical(10);
                    Label("Color Selection:", sectionOpts);
                    ColorPicker("Accent Color", pickerColor);
                EndVertical();
            }
            
            ctx.layout().endContainer();
        }

        // Editor Placeholder
        DockableWindow("Editor") {
            Rect contentRect = ctx.layout().currentBounds();
            dl.addRectFilled(contentRect, theme.colors.panelBackground);
            
            TabControlOptions tabOpts;
            tabOpts.tabHeight = 32.0f;
            tabOpts.showCloseButtons = true;
            
            Rect editorTabsRect = tabs.render("editor_tabs", contentRect, tabOpts);
            
            if (tabs.selectedTab()) {
                const std::string& tabId = tabs.selectedTab()->id;
                TextEditor& editor = getOrCreateEditor(tabId);
                editor.render(editorTabsRect);
            } else {
                if (ctx.font()) {
                    dl.addText(ctx.font(), Vec2(editorTabsRect.x() + 20, editorTabsRect.y() + 20), "No file open", theme.colors.textSecondary);
                }
            }
        }

        // Input Demo Window
        DockableWindow("Input Demo") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions inputPanelOpts;
            inputPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel("InputDemoPanel", inputPanelOpts) {
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;
                
                BeginVertical(10);
                    Label("Sliders:", sectionOpts);
                    SliderOptions sliderOpts;
                    sliderOpts.style = Style().withWidth(250);
                    Slider("Volume", sliderValue1, 0.0f, 100.0f, sliderOpts);
                    
                    Spacing(20);
                    
                    Label("Text Inputs:", sectionOpts);
                    TextInputOptions tiOpts;
                    tiOpts.style = Style().withWidth(250);
                    TextInput("Username", textInputValue, tiOpts);
                EndVertical();
            }
            
            ctx.layout().endContainer();
        }

        // New Widgets Window
        DockableWindow("New Widgets") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions p2PanelOpts;
            p2PanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel("NewWidgetsPanel", p2PanelOpts) {
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;

                BeginHorizontal(30);
                    BeginVertical(10);
                        Label("Listbox:", sectionOpts);
                        ListboxOptions lbOpts;
                        lbOpts.height = 120;
                        lbOpts.style = Style().withWidth(200);
                        Listbox("demo_listbox", listboxSelection, listboxItems, lbOpts);
                    EndVertical();
                    
                    BeginVertical(10);
                        Label("Spinner:", sectionOpts);
                        SpinnerWithLabel("loading", "Processing...");
                        
                        Spacing(20);
                        
                        Label("Selectables:", sectionOpts);
                        Selectable("Option 1", selectable1);
                        Selectable("Option 2", selectable2);
                    EndVertical();
                EndHorizontal();
            }
            
            ctx.layout().endContainer();
        }

        // Status Bar
        Rect statusRect(0, windowH - statusBarHeight, windowW, statusBarHeight);
        dl.addRectFilled(statusRect, theme.colors.primary);
        if (ctx.font()) {
            dl.addText(ctx.font(), Vec2(10, statusRect.y() + 4), statusText, theme.colors.primaryText);
        }
        
        menuBar.renderPopups();
        RenderContextMenu();
        
        // Render docking preview overlay at the end
        RenderDockPreview();
        
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
