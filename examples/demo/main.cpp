#include "fastener/fastener.h"
#include <iostream>
#include <vector>
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
    config.msaaSamples = 16;
    
    Window window(config);
    if (!window.isOpen()) {
        return 1;
    }
    
    // Initialize context
    Context ctx;
    ctx.setTheme(Theme::dark());
    
    // Load font
#ifdef _WIN32
    if (!ctx.loadFont("assets/arial.ttf", 14.0f)) {
        ctx.loadFont("C:/Windows/Fonts/arial.ttf", 14.0f);
    }
#else
    if (!ctx.loadFont("assets/arial.ttf", 14.0f)) {
        if (!ctx.loadFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 14.0f)) {
            ctx.loadFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14.0f);
        }
    }
#endif
    
    // Initialize localization
    I18n::instance().loadFromString(R"({
        "en": {
            "app.title": "Fastener IDE Demo",
            "menu.file": "File",
            "menu.edit": "Edit",
            "menu.view": "View",
            "menu.help": "Help",
            "menu.file.new": "New File",
            "menu.file.open": "Open File...",
            "menu.file.save": "Save",
            "menu.file.exit": "Exit",
            "explorer.title": "EXPLORER",
            "terminal.title": "TERMINAL",
            "settings.title": "WIDGET DEMO",
            "settings.checkboxes": "Checkboxes:",
            "settings.lineNumbers": "Show Line Numbers",
            "settings.wordWrap": "Word Wrap",
            "settings.optimization": "Select Optimization:",
            "settings.progress": "Progress Indicators:",
            "settings.color": "Color Selection:",
            "button.save": "Save",
            "button.cancel": "Cancel",
            "button.clear": "Clear Files",
            "status.ready": "Ready",
            "status.created": "Created: {0}",
            "status.saved": "Saved!",
            "localization.title": "LOCALIZATION DEMO",
            "localization.current": "Current Language:",
            "localization.select": "Select Language:",
            "localization.greeting": "Hello, World!",
            "localization.items.one": "{0} item selected",
            "localization.items.other": "{0} items selected"
        },
        "pl": {
            "app.title": "Fastener IDE Demo",
            "menu.file": "Plik",
            "menu.edit": "Edycja",
            "menu.view": "Widok",
            "menu.help": "Pomoc",
            "menu.file.new": "Nowy plik",
            "menu.file.open": "Otwórz plik...",
            "menu.file.save": "Zapisz",
            "menu.file.exit": "Zakończ",
            "explorer.title": "EKSPLORATOR",
            "terminal.title": "TERMINAL",
            "settings.title": "DEMO WIDŻETÓW",
            "settings.checkboxes": "Pola wyboru:",
            "settings.lineNumbers": "Pokaż numery linii",
            "settings.wordWrap": "Zawijanie wierszy",
            "settings.optimization": "Wybierz optymalizację:",
            "settings.progress": "Wskaźniki postępu:",
            "settings.color": "Wybór koloru:",
            "button.save": "Zapisz",
            "button.cancel": "Anuluj",
            "button.clear": "Wyczyść pliki",
            "status.ready": "Gotowy",
            "status.created": "Utworzono: {0}",
            "status.saved": "Zapisano!",
            "localization.title": "DEMO LOKALIZACJI",
            "localization.current": "Aktualny język:",
            "localization.select": "Wybierz język:",
            "localization.greeting": "Witaj, Świecie!",
            "localization.items.one": "{0} element zaznaczony",
            "localization.items.other": "{0} elementów zaznaczonych"
        },
        "de": {
            "app.title": "Fastener IDE Demo",
            "menu.file": "Datei",
            "menu.edit": "Bearbeiten",
            "menu.view": "Ansicht",
            "menu.help": "Hilfe",
            "menu.file.new": "Neue Datei",
            "menu.file.open": "Datei öffnen...",
            "menu.file.save": "Speichern",
            "menu.file.exit": "Beenden",
            "explorer.title": "EXPLORER",
            "terminal.title": "TERMINAL",
            "settings.title": "WIDGET-DEMO",
            "settings.checkboxes": "Kontrollkästchen:",
            "settings.lineNumbers": "Zeilennummern anzeigen",
            "settings.wordWrap": "Zeilenumbruch",
            "settings.optimization": "Optimierung wählen:",
            "settings.progress": "Fortschrittsanzeigen:",
            "settings.color": "Farbauswahl:",
            "button.save": "Speichern",
            "button.cancel": "Abbrechen",
            "button.clear": "Dateien löschen",
            "status.ready": "Bereit",
            "status.created": "Erstellt: {0}",
            "status.saved": "Gespeichert!",
            "localization.title": "LOKALISIERUNGSDEMO",
            "localization.current": "Aktuelle Sprache:",
            "localization.select": "Sprache wählen:",
            "localization.greeting": "Hallo, Welt!",
            "localization.items.one": "{0} Element ausgewählt",
            "localization.items.other": "{0} Elemente ausgewählt"
        }
    })");
    I18n::instance().setLocale("en");
    
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
    tabs.addTab("table_demo", "Table Demo", true);
    tabs.addTab("blur_demo", "Blur Demo", true);
    tabs.addTab("types.cpp", "types.cpp", true);
    tabs.addTab("context.cpp", "context.cpp", true);
    
    // Mark one as modified
    if (auto tab = tabs.getTab(1)) {
        tab->modified = true;
    }
    
    // Create menu bar
    MenuBar menuBar;
    CommandPalette commandPalette;
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
    Date demoDate = {2026, 1, 31};
    TimeOfDay demoTime = {14, 30, 0};
    
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

    // Table Demo state
    std::vector<TableColumn> tableColumns = {
        {"name", "Name", 180, 80, 300, Alignment::Start, true},
        {"type", "Type", 100, 60, 150, Alignment::Center, true},
        {"size", "Size", 80, 50, 120, Alignment::End, true},
        {"modified", "Modified", 140, 80, 200, Alignment::Start, false}
    };
    struct FileEntry {
        std::string name, type, size, modified;
    };
    std::vector<FileEntry> tableData = {
        {"main.cpp", "C++ Source", "4.2 KB", "2026-01-08"},
        {"README.md", "Markdown", "1.1 KB", "2026-01-05"},
        {"CMakeLists.txt", "CMake", "2.8 KB", "2026-01-07"},
        {"fastener.h", "C++ Header", "3.5 KB", "2026-01-08"},
        {"button.cpp", "C++ Source", "2.1 KB", "2026-01-02"},
        {"context.cpp", "C++ Source", "5.6 KB", "2026-01-06"},
        {"table.cpp", "C++ Source", "23.5 KB", "2026-01-08"},
        {"types.h", "C++ Header", "4.8 KB", "2026-01-03"},
        {"window.cpp", "C++ Source", "8.2 KB", "2026-01-04"},
        {"font.ttf", "Font", "124 KB", "2025-12-15"}
    };
    int tableSelectedRow = -1;
    int tableSortColumn = 0;
    bool tableSortAsc = true;

    // Drag and Drop Demo state
    std::vector<std::string> droppedFilePaths;
    std::vector<std::string> dragDropList1 = {"Item A", "Item B", "Item C"};
    std::vector<std::string> dragDropList2 = {"Item X", "Item Y", "Item Z"};
    std::string selectedDragItem1;
    std::string selectedDragItem2;

    // Profiler state
    bool showProfilerOverlay = true;

    // Localization state
    int selectedLocale = 0;
    std::vector<std::string> localeOptions = {"English", "Polski", "Deutsch"};
    std::vector<std::string> localeCodes = {"en", "pl", "de"};
    float localizationItemCount = 3.0f;
    bool showProfilerWindow = false;

    // New Widget Demo state
    bool toggleSwitch1 = false;
    bool toggleSwitch2 = true;
    int badgeCount = 5;
    bool showModal = false;
    std::vector<std::string> breadcrumbPath = {"Home", "Documents", "Projects", "Fastener"};

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

    menuBar.addMenu("Profiler", {
        MenuItem::checkbox("showOverlay", "Show Overlay", &showProfilerOverlay),
        MenuItem::checkbox("showWindow", "Show Detailed Window", &showProfilerWindow)
    });

    commandPalette.setCommands({
        CommandPaletteCommand("file.new", "New File", [&]() {
            static int n = 1;
            std::string name = "untitled" + std::to_string(n++) + ".cpp";
            tabs.addTab(name, name);
            statusText = "Created: " + name;
        }).withShortcut("Ctrl+N").withDescription("Create a new file"),
        CommandPaletteCommand("file.save", "Save", [&]() {
            statusText = "Saved!";
        }).withShortcut("Ctrl+S"),
        CommandPaletteCommand("view.toggle_profiler_overlay", "Toggle Profiler Overlay", [&]() {
            showProfilerOverlay = !showProfilerOverlay;
        }),
        CommandPaletteCommand("view.toggle_profiler_window", "Toggle Profiler Window", [&]() {
            showProfilerWindow = !showProfilerWindow;
        }),
        CommandPaletteCommand("app.exit", "Exit", [&]() {
            window.close();
        }).withShortcut("Alt+F4")
    });
    
    // Initialize default dock layout
    static bool layoutInitialized = false;
    auto initDockLayout = [&]() {
        DockNode::Id mainDockId = DockBuilder::GetDockSpaceId(ctx, "##MainDockSpace");
        DockBuilder::Begin(mainDockId);
        DockBuilder::ClearDockSpace(ctx, mainDockId);
        
        // Split Root into Left and Right
        DockNode::Id leftNode = DockBuilder::SplitNode(ctx, mainDockId, DockDirection::Left, 0.2f);
        DockNode::Id rightNode = DockBuilder::GetNode(ctx, mainDockId, DockDirection::Right);
        
        // Split Right into Center and Bottom
        DockNode::Id bottomNode = DockBuilder::SplitNode(ctx, rightNode, DockDirection::Bottom, 0.25f);
        DockNode::Id centralNode = DockBuilder::GetNode(ctx, rightNode, DockDirection::Top);
        
        DockBuilder::DockWindow(ctx, "Explorer", leftNode);
        DockBuilder::DockWindow(ctx, "Terminal", bottomNode);
        DockBuilder::DockWindow(ctx, "Editor", centralNode);
        DockBuilder::DockWindow(ctx, "Settings", centralNode);
        DockBuilder::DockWindow(ctx, "Schedule", centralNode);
        DockBuilder::DockWindow(ctx, "Input Demo", centralNode);
        DockBuilder::DockWindow(ctx, "New Widgets", centralNode);
        DockBuilder::DockWindow(ctx, "Drag & Drop Demo", centralNode);
        DockBuilder::DockWindow(ctx, "Layout Demo", centralNode);
        DockBuilder::DockWindow(ctx, "Localization", centralNode);
        
        DockBuilder::Finish();
        layoutInitialized = true;
    };
    
    // Setup file drop callback
    window.setFileDropCallback([&droppedFilePaths](const std::vector<std::string>& paths) {
        for (const auto& path : paths) {
            droppedFilePaths.push_back(path);
        }
    });
    
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

        menuBar.render(ctx, Rect(0, 0, windowW, menuBarHeight));
        
        // Define the main docking area between menu bar and status bar
        Rect mainDockArea(0, menuBarHeight, windowW, windowH - menuBarHeight - statusBarHeight);
        DockSpace(ctx, "##MainDockSpace", mainDockArea);

        // --- Dockable Windows ---

        // Explorer Window
        DockableWindow(ctx, "Explorer") {
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
            
            fileTree.render(ctx, "explorer_tree", treeRect, treeOpts, treeEvents);
        }

        // Terminal Window
        DockableWindow(ctx, "Terminal") {
            Rect terminalArea = ctx.layout().currentBounds();
            dl.addRectFilled(terminalArea, theme.colors.windowBackground.darker(0.1f));
            if (ctx.font()) {
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 10), "TERMINAL", theme.colors.textSecondary);
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 40), "> cmake --build build", theme.colors.text);
                dl.addText(ctx.font(), Vec2(terminalArea.x() + 10, terminalArea.y() + 60), "[100%] Built target fastener_demo", theme.colors.success);
            }
        }

        // Settings Window
        DockableWindow(ctx, "Settings") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions settingsPanelOpts;
            settingsPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel(ctx, "SettingsPanel", settingsPanelOpts) {
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, "WIDGET DEMO", titleOpts);
                Spacing(ctx, 10);
                
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;

                BeginVertical(ctx, 10);
                    Label(ctx, "Checkboxes:", sectionOpts);
                    (void)Checkbox(ctx, "Show Line Numbers", checkValue1);
                    (void)Checkbox(ctx, "Word Wrap", checkValue2);
                EndVertical(ctx);
                
                Spacing(ctx, 20);
                
                BeginVertical(ctx, 10);
                    Label(ctx, "Select Optimization:", sectionOpts);
                    ComboBoxOptions comboOpts;
                    comboOpts.style = Style().withWidth(300);
                    (void)ComboBox(ctx, "Performance", selectedCombo, comboOptions, comboOpts);
                EndVertical(ctx);
                
                Spacing(ctx, 20);
                
                BeginVertical(ctx, 10);
                    Label(ctx, "Progress Indicators:", sectionOpts);
                    progressVal = std::fmod(ctx.time() * 0.1f, 1.0f);
                    ProgressBarOptions pb1; pb1.style = Style().withWidth(400);
                    ProgressBar(ctx, "Task Progress", progressVal, pb1);
                EndVertical(ctx);
                
                Spacing(ctx, 20);
                
                BeginVertical(ctx, 10);
                    Label(ctx, "Color Selection:", sectionOpts);
                    ColorPicker(ctx, "Accent Color", pickerColor);
                EndVertical(ctx);
            }
            
            ctx.layout().endContainer();
        }

        // Schedule Window
        DockableWindow(ctx, "Schedule") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);

            PanelOptions schedulePanelOpts;
            schedulePanelOpts.title = "Schedule";
            schedulePanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            Panel(ctx, "SchedulePanel", schedulePanelOpts) {
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;

                BeginVertical(ctx, 10);
                    Label(ctx, "Date & Time:", sectionOpts);
                    DatePickerOptions dateOpts;
                    dateOpts.format = DateFormat::ISO;
                    (void)DatePicker(ctx, "Start Date", demoDate, dateOpts);

                    TimePickerOptions timeOpts;
                    timeOpts.showSeconds = true;
                    timeOpts.use24Hour = true;
                    (void)TimePicker(ctx, "Start Time", demoTime, timeOpts);
                EndVertical(ctx);
            }

            ctx.layout().endContainer();
        }

        // Editor Placeholder
        DockableWindow(ctx, "Editor") {
            Rect contentRect = ctx.layout().currentBounds();
            dl.addRectFilled(contentRect, theme.colors.panelBackground);
            
            TabControlOptions tabOpts;
            tabOpts.tabHeight = 32.0f;
            tabOpts.showCloseButtons = true;
            
            Rect editorTabsRect = tabs.render(ctx, "editor_tabs", contentRect, tabOpts);
            
            if (tabs.selectedTab()) {
                const std::string& tabId = tabs.selectedTab()->id;
                if (tabId == "blur_demo") {
                    Rect demoRect = editorTabsRect;
                    dl.addRectFilledMultiColor(
                        demoRect,
                        Color(32, 90, 170),
                        Color(120, 60, 190),
                        Color(30, 160, 140),
                        Color(18, 30, 60)
                    );
                    dl.addCircleFilled(
                        Vec2(demoRect.x() + demoRect.width() * 0.25f, demoRect.y() + demoRect.height() * 0.35f),
                        120.0f,
                        Color(255, 120, 90, 120),
                        48
                    );
                    dl.addCircleFilled(
                        Vec2(demoRect.x() + demoRect.width() * 0.75f, demoRect.y() + demoRect.height() * 0.55f),
                        160.0f,
                        Color(80, 180, 255, 110),
                        48
                    );
                    
                    float panelPadding = 40.0f;
                    Rect blurPanelRect(
                        demoRect.x() + panelPadding,
                        demoRect.y() + panelPadding,
                        demoRect.width() - panelPadding * 2,
                        demoRect.height() - panelPadding * 2
                    );
                    
                    PanelOptions blurPanelOpts;
                    blurPanelOpts.title = "Blur / Frosted Glass";
                    blurPanelOpts.style = Style()
                        .withPos(blurPanelRect.x(), blurPanelRect.y())
                        .withSize(blurPanelRect.width(), blurPanelRect.height())
                        .withBlur(16.0f, Color(255, 255, 255, 120))
                        .withBorderRadius(16.0f)
                        .withBorder(1.0f, Color(255, 255, 255, 90))
                        .withShadow(14.0f, Color(0, 0, 0, 70));
                    
                    Panel(ctx, "BlurDemoPanel", blurPanelOpts) {
                        LabelOptions titleOpts;
                        titleOpts.color = Color(30, 60, 120);
                        Label(ctx, "Frosted Glass Panels", titleOpts);
                        Spacing(ctx, 8);
                        
                        LabelOptions infoOpts;
                        infoOpts.color = Color(80, 80, 90);
                        Label(ctx, "Blur radius and tint are controlled per-widget.", infoOpts);
                        Spacing(ctx, 16);
                        
                        BeginHorizontal(ctx, 16);
                            CardOptions softOpts;
                            softOpts.style = Style().withSize(180, 110)
                                .withBlur(6.0f, Color(255, 255, 255, 110))
                                .withBorderRadius(12.0f)
                                .withBorder(1.0f, Color(255, 255, 255, 80))
                                .withShadow(10.0f, Color(0, 0, 0, 45));
                            softOpts.title = "Soft";
                            Card(ctx, "BlurSoft", softOpts) {
                                LabelOptions cardText;
                                cardText.color = Color(60, 60, 70);
                                Label(ctx, "Radius: 6", cardText);
                                Label(ctx, "Tint: 110", cardText);
                            }
                            
                            CardOptions midOpts;
                            midOpts.style = Style().withSize(180, 110)
                                .withBlur(12.0f, Color(255, 255, 255, 125))
                                .withBorderRadius(12.0f)
                                .withBorder(1.0f, Color(255, 255, 255, 80))
                                .withShadow(10.0f, Color(0, 0, 0, 45));
                            midOpts.title = "Medium";
                            Card(ctx, "BlurMedium", midOpts) {
                                LabelOptions cardText;
                                cardText.color = Color(60, 60, 70);
                                Label(ctx, "Radius: 12", cardText);
                                Label(ctx, "Tint: 125", cardText);
                            }
                            
                            CardOptions heavyOpts;
                            heavyOpts.style = Style().withSize(180, 110)
                                .withBlur(18.0f, Color(255, 255, 255, 140))
                                .withBorderRadius(12.0f)
                                .withBorder(1.0f, Color(255, 255, 255, 80))
                                .withShadow(10.0f, Color(0, 0, 0, 45));
                            heavyOpts.title = "Heavy";
                            Card(ctx, "BlurHeavy", heavyOpts) {
                                LabelOptions cardText;
                                cardText.color = Color(60, 60, 70);
                                Label(ctx, "Radius: 18", cardText);
                                Label(ctx, "Tint: 140", cardText);
                            }
                        EndHorizontal(ctx);
                        
                        Spacing(ctx, 18);
                        Separator(ctx);
                        Spacing(ctx, 12);
                        
                        BeginHorizontal(ctx, 12);
                            ButtonOptions btnOpts;
                            btnOpts.style = Style().withSize(120, 32);
                            (void)Button(ctx, "Primary", btnOpts);
                            (void)Button(ctx, "Secondary", btnOpts);
                        EndHorizontal(ctx);
                    }
                } else {
                    TextEditor& editor = getOrCreateEditor(tabId);
                    editor.render(ctx, editorTabsRect);
                }
            } else {
                if (ctx.font()) {
                    dl.addText(ctx.font(), Vec2(editorTabsRect.x() + 20, editorTabsRect.y() + 20), "No file open", theme.colors.textSecondary);
                }
            }
        }

        // Input Demo Window
        DockableWindow(ctx, "Input Demo") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions inputPanelOpts;
            inputPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel(ctx, "InputDemoPanel", inputPanelOpts) {
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;
                
                BeginVertical(ctx, 10);
                    Label(ctx, "Sliders:", sectionOpts);
                    SliderOptions sliderOpts;
                    sliderOpts.style = Style().withWidth(250);
                    (void)Slider(ctx, "Volume", sliderValue1, 0.0f, 100.0f, sliderOpts);
                    
                    Spacing(ctx, 20);
                    
                    Label(ctx, "Text Inputs:", sectionOpts);
                    TextInputOptions tiOpts;
                    tiOpts.style = Style().withWidth(250);
                    (void)TextInput(ctx, "Username", textInputValue, tiOpts);
                EndVertical(ctx);
            }
            
            ctx.layout().endContainer();
        }

        // New Widgets Window
        DockableWindow(ctx, "New Widgets") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions p2PanelOpts;
            p2PanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            p2PanelOpts.scrollable = true;
            
            Panel(ctx, "NewWidgetsPanel", p2PanelOpts) {
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;

                BeginHorizontal(ctx, 30);
                    BeginVertical(ctx, 10);
                        Label(ctx, "Listbox:", sectionOpts);
                        ListboxOptions lbOpts;
                        lbOpts.height = 120;
                        lbOpts.style = Style().withWidth(200);
                        Listbox(ctx, "demo_listbox", listboxSelection, listboxItems, lbOpts);
                        
                        Spacing(ctx, 20);
                        
                        Label(ctx, "RadioButtons:", sectionOpts);
                        (void)RadioButton(ctx, "Option A", radioSelection, 0);
                        (void)RadioButton(ctx, "Option B", radioSelection, 1);
                        (void)RadioButton(ctx, "Option C", radioSelection, 2);
                    EndVertical(ctx);
                    
                    BeginVertical(ctx, 10);
                        Label(ctx, "Spinner:", sectionOpts);
                        SpinnerWithLabel(ctx, "loading", "Processing...");
                        
                        Spacing(ctx, 20);
                        
                        Label(ctx, "Selectables:", sectionOpts);
                        (void)Selectable(ctx, "Option 1", selectable1);
                        (void)Selectable(ctx, "Option 2", selectable2);
                        
                        Spacing(ctx, 20);
                        
                        Label(ctx, "InputNumber:", sectionOpts);
                        InputNumberOptions inOpts;
                        inOpts.style = Style().withWidth(150);
                        (void)InputNumberInt(ctx, "Amount", inputNumberValue, 0, 100, inOpts);
                    EndVertical(ctx);
                    
                    BeginVertical(ctx, 10);
                        Label(ctx, "CollapsingHeader:", sectionOpts);
                        if (CollapsingHeader(ctx, "Section 1", collapsingOpen1)) {
                            Label(ctx, "Content inside Section 1");
                            Label(ctx, "More content here...");
                        }
                        if (CollapsingHeader(ctx, "Section 2", collapsingOpen2)) {
                            Label(ctx, "Content inside Section 2");
                            (void)Checkbox(ctx, "Nested checkbox", checkValue2);
                        }
                        
                        Spacing(ctx, 20);
                        
                        Label(ctx, "Separator:", sectionOpts);
                        BeginVertical(ctx, 5);
                            Label(ctx, "Above separator");
                            Separator(ctx);
                            Label(ctx, "Below separator");
                            SeparatorWithLabel(ctx, "With Text");
                            Label(ctx, "After text separator");
                        EndVertical(ctx);
                    EndVertical(ctx);
                EndHorizontal(ctx);
                
                Separator(ctx);
                Spacing(ctx, 10);
                
                // New Widgets Row
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, "NEW WIDGETS (Toggle, Badge, Breadcrumb, Modal)", titleOpts);
                Spacing(ctx, 10);
                
                BeginHorizontal(ctx, 30);
                    BeginVertical(ctx, 10);
                        Label(ctx, "Toggle Switch:", sectionOpts);
                        (void)ToggleSwitch(ctx, "Dark Mode", toggleSwitch1);
                        (void)ToggleSwitch(ctx, "Notifications", toggleSwitch2);
                    EndVertical(ctx);
                    
                    BeginVertical(ctx, 10);
                        Label(ctx, "Badge:", sectionOpts);
                        BeginHorizontal(ctx, 10);
                            Badge(ctx, badgeCount);
                            BadgeOptions badgeOpts1; badgeOpts1.maxValue = 99;
                            Badge(ctx, 123, badgeOpts1);
                            BadgeOptions badgeOpts2; badgeOpts2.color = Color::fromHex(0x2ECC71);
                            Badge(ctx, "NEW", badgeOpts2);
                        EndHorizontal(ctx);
                        ButtonOptions addBadgeBtn; addBadgeBtn.style = Style().withSize(100, 24);
                        if (Button(ctx, "Add Badge", addBadgeBtn)) {
                            badgeCount++;
                        }
                    EndVertical(ctx);
                    
                    BeginVertical(ctx, 10);
                        Label(ctx, "Breadcrumb:", sectionOpts);
                        int clickedBreadcrumb = Breadcrumb(ctx, breadcrumbPath);
                        if (clickedBreadcrumb >= 0) {
                            statusText = "Navigated to: " + breadcrumbPath[clickedBreadcrumb];
                        }
                    EndVertical(ctx);
                    
                    BeginVertical(ctx, 10);
                        Label(ctx, "Modal Dialog:", sectionOpts);
                        ButtonOptions modalBtn; modalBtn.style = Style().withSize(100, 28);
                        if (Button(ctx, "Open Modal", modalBtn)) {
                            showModal = true;
                        }
                    EndVertical(ctx);
                EndHorizontal(ctx);
            }
            
            ctx.layout().endContainer();
        }
        
        // Modal Dialog (rendered outside the New Widgets panel)
        ModalOptions modalOpts; modalOpts.title = "Example Modal"; modalOpts.width = 350;
        if (BeginModal(ctx, "demo_modal", showModal, modalOpts)) {
            Label(ctx, "This is a modal dialog!");
            Spacing(ctx, 10);
            Label(ctx, "Click outside or press Close to dismiss.");
            Spacing(ctx, 20);
            
            BeginHorizontal(ctx, 10);
                if (ModalButton(ctx, "Cancel")) {
                    showModal = false;
                }
                if (ModalButton(ctx, "OK", true)) {
                    showModal = false;
                    statusText = "Modal confirmed!";
                }
            EndHorizontal(ctx);
        }
        EndModal(ctx);

        // Table Demo Window
        DockableWindow(ctx, "Table Demo") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions tablePanelOpts;
            tablePanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel(ctx, "TableDemoPanel", tablePanelOpts) {
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, "TABLE WIDGET DEMO", titleOpts);
                Spacing(ctx, 10);
                
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;
                
                Label(ctx, "Click column headers to sort. Resize columns by dragging dividers.", sectionOpts);
                Spacing(ctx, 10);
                
                // Table widget
                TableOptions tableOpts;
                tableOpts.style = Style().withSize(contentRect.width() - 40, 250);
                tableOpts.alternateRowColors = true;
                tableOpts.bordered = true;
                tableOpts.resizableColumns = true;
                
                if (BeginTable(ctx, "file_table", tableColumns, tableOpts)) {
                    TableHeader(ctx, tableSortColumn, tableSortAsc);
                    
                    for (int i = 0; i < (int)tableData.size(); ++i) {
                        const auto& file = tableData[i];
                        bool isSelected = (i == tableSelectedRow);
                        
                        if (TableRow(ctx, {file.name, file.type, file.size, file.modified}, isSelected)) {
                            tableSelectedRow = i;
                            statusText = "Selected: " + file.name;
                        }
                    }
                    
                    // Update sort state from table
                    tableSortColumn = GetTableSortColumn(ctx);
                    tableSortAsc = GetTableSortAscending(ctx);
                    
                    EndTable(ctx);
                }
                
                Spacing(ctx, 15);
                
                // Selected row info
                if (tableSelectedRow >= 0 && tableSelectedRow < (int)tableData.size()) {
                    const auto& file = tableData[tableSelectedRow];
                    Label(ctx, "Selected: " + file.name + " (" + file.size + ")", sectionOpts);
                } else {
                    Label(ctx, "No row selected. Click a row to select it.", sectionOpts);
                }
                
                Spacing(ctx, 10);
                Label(ctx, "Sort column: " + std::to_string(tableSortColumn) + 
                      (tableSortAsc ? " (ascending)" : " (descending)"), sectionOpts);
            }
            
            ctx.layout().endContainer();
        }

        // Drag & Drop Demo Window
        DockableWindow(ctx, "Drag & Drop Demo") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions ddPanelOpts;
            ddPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel(ctx, "DragDropDemoPanel", ddPanelOpts) {
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, "DRAG & DROP DEMO", titleOpts);
                Spacing(ctx, 10);
                
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;
                
                // Section 1: System File Drop
                Label(ctx, "System File Drop", titleOpts);
                Label(ctx, "Drag files from your desktop/explorer onto this window:", sectionOpts);
                Spacing(ctx, 5);
                
                // Display dropped files
                if (droppedFilePaths.empty()) {
                    Label(ctx, "No files dropped yet. Try dragging files here!", sectionOpts);
                } else {
                    for (size_t i = 0; i < droppedFilePaths.size() && i < 10; ++i) {
                        Label(ctx, std::to_string(i + 1) + ". " + droppedFilePaths[i], sectionOpts);
                    }
                    if (droppedFilePaths.size() > 10) {
                        Label(ctx, "... and " + std::to_string(droppedFilePaths.size() - 10) + " more", sectionOpts);
                    }
                    
                    Spacing(ctx, 5);
                    ButtonOptions clearBtnOpts;
                    clearBtnOpts.style = Style().withSize(120, 28);
                    if (Button(ctx, "Clear Files", clearBtnOpts)) {
                        droppedFilePaths.clear();
                    }
                }
                
                Spacing(ctx, 20);
                Separator(ctx);
                Spacing(ctx, 10);
                
                // Section 2: Internal Drag and Drop
                Label(ctx, "Internal Drag & Drop", titleOpts);
                Label(ctx, "Drag items between lists to move them:", sectionOpts);
                Spacing(ctx, 10);
                
                // Two lists side by side
                Rect listsRect = ctx.layout().currentBounds();
                float listWidth = (listsRect.width() - 20) / 2;
                
                // --- List 1 ---
                Label(ctx, "List 1 (Drop here)", sectionOpts);
                
                PanelOptions list1Opts;
                list1Opts.style = Style().withSize(listWidth, 180);
                Panel(ctx, "List1Panel", list1Opts) {
                    Rect list1Rect = ctx.layout().currentBounds();
                    bool itemTargetHit = false;
                    
                    struct DndAction {
                        std::string item;
                        int targetIndex = -1;
                        bool insertAfter = false;
                    } pendingDrop;

                    for (size_t i = 0; i < dragDropList1.size(); ++i) {
                        SelectableOptions selOpts;
                        selOpts.spanWidth = true;
                        bool isSelected = (selectedDragItem1 == dragDropList1[i]);
                        if (Selectable(ctx, dragDropList1[i], isSelected, selOpts)) {
                            selectedDragItem1 = dragDropList1[i];
                        }
                        
                        // Item Drop Target (Insert Before/After)
                        Rect itemRect = ctx.getLastWidgetBounds();
                        Rect targetRect = itemRect;
                        targetRect.pos.y -= (i == 0) ? 15.0f : 2.0f; 
                        targetRect.size.y += (i == 0) ? 17.0f : 4.0f;
                        
                        if (BeginDragDropTarget(targetRect)) {
                            itemTargetHit = true;
                            bool insertAfter = ctx.input().mousePos().y > itemRect.center().y;
                            
                            // Visual Feedback
                            if (IsDragDropActive() && GetDragDropPayload()->type == "DND_DEMO_ITEM") {
                                float halfSpacing = theme.metrics.paddingSmall / 2.0f;
                                // If first item and dropping above, shift line down slightly to avoid panel clipping
                                float lineY = insertAfter ? itemRect.bottom() + halfSpacing : itemRect.top() + (i == 0 ? 2.0f : -halfSpacing);
                                dl.addLine(Vec2(itemRect.left(), lineY), Vec2(itemRect.right(), lineY), theme.colors.primary, 2.0f);
                            }

                            if (const DragPayload* payload = AcceptDragDropPayload("DND_DEMO_ITEM", DragDropFlags_AcceptNoHighlight)) {
                                pendingDrop = {(const char*)payload->data.data(), (int)i, insertAfter};
                            }
                            EndDragDropTarget();
                        }
                        
                        if (BeginDragDropSource()) {
                            selectedDragItem1 = dragDropList1[i];
                            SetDragDropPayload("DND_DEMO_ITEM", dragDropList1[i].c_str(), dragDropList1[i].size() + 1);
                            SetDragDropDisplayText("Moving: " + dragDropList1[i]);
                            EndDragDropSource();
                        }
                    }

                    // Process deferred drop
                    if (pendingDrop.targetIndex != -1) {
                        std::string item = pendingDrop.item;
                        int i = pendingDrop.targetIndex;
                        bool insertAfter = pendingDrop.insertAfter;

                        auto it1 = std::find(dragDropList1.begin(), dragDropList1.end(), item);
                        if (it1 != dragDropList1.end()) {
                            int oldIndex = (int)std::distance(dragDropList1.begin(), it1);
                            int newIndex = insertAfter ? i + 1 : i;
                            if (newIndex > oldIndex) newIndex--;
                            dragDropList1.erase(it1);
                            dragDropList1.insert(dragDropList1.begin() + newIndex, item);
                            selectedDragItem1 = item;
                        } else {
                            auto it2 = std::find(dragDropList2.begin(), dragDropList2.end(), item);
                            if (it2 != dragDropList2.end()) {
                                dragDropList2.erase(it2);
                                int newIndex = insertAfter ? i + 1 : i;
                                dragDropList1.insert(dragDropList1.begin() + newIndex, item);
                                selectedDragItem1 = item;
                                selectedDragItem2 = ""; // Clear other selection
                            }
                        }
                    }

                    // Container Drop Target (Fallback: Append)
                    if (!itemTargetHit && BeginDragDropTarget(list1Rect)) {
                        if (const DragPayload* payload = AcceptDragDropPayload("DND_DEMO_ITEM")) {
                            std::string item = (const char*)payload->data.data();
                            auto it2 = std::find(dragDropList2.begin(), dragDropList2.end(), item);
                            if (it2 != dragDropList2.end()) {
                                dragDropList2.erase(it2);
                                dragDropList1.push_back(item);
                                selectedDragItem1 = item;
                                selectedDragItem2 = "";
                            } else {
                                auto itSelf = std::find(dragDropList1.begin(), dragDropList1.end(), item);
                                if (itSelf != dragDropList1.end()) {
                                    dragDropList1.erase(itSelf);
                                    dragDropList1.push_back(item);
                                    selectedDragItem1 = item;
                                }
                            }
                        }
                        EndDragDropTarget();
                    }
                }
                
                Spacing(ctx, 15);
                
                // --- List 2 ---
                Label(ctx, "List 2 (Drop here)", sectionOpts);
                
                PanelOptions list2Opts;
                list2Opts.style = Style().withSize(listWidth, 180);
                Panel(ctx, "List2Panel", list2Opts) {
                    Rect list2Rect = ctx.layout().currentBounds();
                    bool itemTargetHit = false;
                    
                    struct DndAction {
                        std::string item;
                        int targetIndex = -1;
                        bool insertAfter = false;
                    } pendingDrop;

                    for (size_t i = 0; i < dragDropList2.size(); ++i) {
                        SelectableOptions selOpts;
                        selOpts.spanWidth = true;
                        bool isSelected = (selectedDragItem2 == dragDropList2[i]);
                        if (Selectable(ctx, dragDropList2[i], isSelected, selOpts)) {
                            selectedDragItem2 = dragDropList2[i];
                        }
                        
                        // Item Drop Target (Insert Before/After)
                        Rect itemRect = ctx.getLastWidgetBounds();
                        Rect targetRect = itemRect;
                        targetRect.pos.y -= (i == 0) ? 15.0f : 2.0f; 
                        targetRect.size.y += (i == 0) ? 17.0f : 4.0f;
                        
                        if (BeginDragDropTarget(targetRect)) {
                            itemTargetHit = true;
                            bool insertAfter = ctx.input().mousePos().y > itemRect.center().y;
                            
                            // Visual Feedback
                            if (IsDragDropActive() && GetDragDropPayload()->type == "DND_DEMO_ITEM") {
                                float halfSpacing = theme.metrics.paddingSmall / 2.0f;
                                float lineY = insertAfter ? itemRect.bottom() + halfSpacing : itemRect.top() + (i == 0 ? 2.0f : -halfSpacing);
                                dl.addLine(Vec2(itemRect.left(), lineY), Vec2(itemRect.right(), lineY), theme.colors.primary, 2.0f);
                            }

                            if (const DragPayload* payload = AcceptDragDropPayload("DND_DEMO_ITEM", DragDropFlags_AcceptNoHighlight)) {
                                pendingDrop = {(const char*)payload->data.data(), (int)i, insertAfter};
                            }
                            EndDragDropTarget();
                        }
                        
                        if (BeginDragDropSource()) {
                            selectedDragItem2 = dragDropList2[i];
                            SetDragDropPayload("DND_DEMO_ITEM", dragDropList2[i].c_str(), dragDropList2[i].size() + 1);
                            SetDragDropDisplayText("Moving: " + dragDropList2[i]);
                            EndDragDropSource();
                        }
                    }

                    // Process deferred drop
                    if (pendingDrop.targetIndex != -1) {
                        std::string item = pendingDrop.item;
                        int i = pendingDrop.targetIndex;
                        bool insertAfter = pendingDrop.insertAfter;

                        auto it2 = std::find(dragDropList2.begin(), dragDropList2.end(), item);
                        if (it2 != dragDropList2.end()) {
                            int oldIndex = (int)std::distance(dragDropList2.begin(), it2);
                            int newIndex = insertAfter ? i + 1 : i;
                            if (newIndex > oldIndex) newIndex--;
                            dragDropList2.erase(it2);
                            dragDropList2.insert(dragDropList2.begin() + newIndex, item);
                            selectedDragItem2 = item;
                        } else {
                            auto it1 = std::find(dragDropList1.begin(), dragDropList1.end(), item);
                            if (it1 != dragDropList1.end()) {
                                dragDropList1.erase(it1);
                                int newIndex = insertAfter ? i + 1 : i;
                                dragDropList2.insert(dragDropList2.begin() + newIndex, item);
                                selectedDragItem2 = item;
                                selectedDragItem1 = ""; // Clear other selection
                            }
                        }
                    }

                    // Container Drop Target (Fallback: Append)
                    if (!itemTargetHit && BeginDragDropTarget(list2Rect)) {
                        if (const DragPayload* payload = AcceptDragDropPayload("DND_DEMO_ITEM")) {
                            std::string item = (const char*)payload->data.data();
                            auto it1 = std::find(dragDropList1.begin(), dragDropList1.end(), item);
                            if (it1 != dragDropList1.end()) {
                                dragDropList1.erase(it1);
                                dragDropList2.push_back(item);
                                selectedDragItem2 = item;
                                selectedDragItem1 = "";
                            } else {
                                auto itSelf = std::find(dragDropList2.begin(), dragDropList2.end(), item);
                                if (itSelf != dragDropList2.end()) {
                                    dragDropList2.erase(itSelf);
                                    dragDropList2.push_back(item);
                                    selectedDragItem2 = item;
                                }
                            }
                        }
                        EndDragDropTarget();
                    }
                }
            }
            
            ctx.layout().endContainer();
        }

        // Layout Demo Window
        DockableWindow(ctx, "Layout Demo") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions layoutPanelOpts;
            layoutPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel(ctx, "LayoutDemoPanel", layoutPanelOpts) {
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, "FLEX LAYOUT DEMO", titleOpts);
                Spacing(ctx, 10);
                
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;
                
                // HStack Example
                Label(ctx, "HStack - Horizontal container with gap:", sectionOpts);
                Spacing(ctx, 5);
                
                FlexOptions hstackOpts1;
                hstackOpts1.gap = 10;
                hstackOpts1.style = Style().withSize(400, 40).withBackground(theme.colors.inputBackground).withBorderRadius(4);
                HStack(ctx, hstackOpts1) {
                    (void)Button(ctx, "Button 1");
                    (void)Button(ctx, "Button 2");
                    (void)Button(ctx, "Button 3");
                }
                
                Spacing(ctx, 15);
                
                // HStack with Spacer
                Label(ctx, "HStack with Spacer (pushes buttons apart):", sectionOpts);
                Spacing(ctx, 5);
                
                FlexOptions hstackOpts2;
                hstackOpts2.gap = 10;
                hstackOpts2.style = Style().withSize(400, 40).withBackground(theme.colors.inputBackground).withBorderRadius(4);
                HStack(ctx, hstackOpts2) {
                    (void)Button(ctx, "Left");
                    Spacer(ctx);
                    (void)Button(ctx, "Right");
                }
                
                Spacing(ctx, 15);
                
                // VStack Example
                Label(ctx, "VStack - Vertical container:", sectionOpts);
                Spacing(ctx, 5);
                
                FlexOptions vstackOpts1;
                vstackOpts1.gap = 8;
                vstackOpts1.style = Style().withSize(200, 120).withBackground(theme.colors.inputBackground).withBorderRadius(4).withPadding(8);
                VStack(ctx, vstackOpts1) {
                    Label(ctx, "Item 1");
                    Label(ctx, "Item 2");
                    Label(ctx, "Item 3");
                }
                
                Spacing(ctx, 15);
                
                // Divider Example
                Label(ctx, "Divider in VStack:", sectionOpts);
                Spacing(ctx, 5);
                
                FlexOptions vstackOpts2;
                vstackOpts2.gap = 4;
                vstackOpts2.style = Style().withSize(250, 100).withBackground(theme.colors.inputBackground).withBorderRadius(4).withPadding(8);
                VStack(ctx, vstackOpts2) {
                    Label(ctx, "Above divider");
                    DividerOptions divOpts;
                    divOpts.margin = 4;
                    Divider(ctx, divOpts);
                    Label(ctx, "Below divider");
                }
                
                Spacing(ctx, 15);
                
                // Grid Example - using nested layout for proper row wrapping
                Label(ctx, "Grid - 3 column layout:", sectionOpts);
                Spacing(ctx, 5);
                
                // Manual grid using VStack of HStacks
                FlexOptions gridContainerOpts;
                gridContainerOpts.gap = 8;
                gridContainerOpts.style = Style().withSize(350, 120).withBackground(theme.colors.inputBackground).withBorderRadius(4).withPadding(8);
                VStack(ctx, gridContainerOpts) {
                    // Row 1: Cells 1-3
                    FlexOptions rowOpts;
                    rowOpts.gap = 8;
                    HStack(ctx, rowOpts) {
                        for (int gi = 0; gi < 3; ++gi) {
                            ctx.pushId(gi);
                            ButtonOptions btnOpts;
                            btnOpts.style = Style().withSize(100, 30);
                            (void)Button(ctx, "Cell " + std::to_string(gi + 1), btnOpts);
                            ctx.popId();
                        }
                    }
                    // Row 2: Cells 4-6
                    HStack(ctx, rowOpts) {
                        for (int gi = 3; gi < 6; ++gi) {
                            ctx.pushId(gi);
                            ButtonOptions btnOpts;
                            btnOpts.style = Style().withSize(100, 30);
                            (void)Button(ctx, "Cell " + std::to_string(gi + 1), btnOpts);
                            ctx.popId();
                        }
                    }
                }
                
                Spacing(ctx, 15);
                
                // Nested Layout Example
                Label(ctx, "Nested: VStack containing HStacks:", sectionOpts);
                Spacing(ctx, 5);
                
                FlexOptions vstackOpts3;
                vstackOpts3.gap = 10;
                vstackOpts3.style = Style().withSize(350, 150).withBackground(theme.colors.inputBackground).withBorderRadius(4).withPadding(10);
                VStack(ctx, vstackOpts3) {
                    FlexOptions rowOpts;
                    rowOpts.gap = 8;
                    HStack(ctx, rowOpts) {
                        Label(ctx, "Row 1:");
                        (void)Button(ctx, "A");
                        (void)Button(ctx, "B");
                    }
                    HStack(ctx, rowOpts) {
                        Label(ctx, "Row 2:");
                        (void)Button(ctx, "C");
                        (void)Button(ctx, "D");
                    }
                    Spacer(ctx);
                    FlexOptions footerOpts;
                    footerOpts.gap = 8;
                    footerOpts.mainAlign = Alignment::End;
                    HStack(ctx, footerOpts) {
                        (void)Button(ctx, "Cancel");
                        (void)Button(ctx, "OK");
                    }
                }
            }
            
            ctx.layout().endContainer();
        }

        // Localization Demo Window
        DockableWindow(ctx, "Localization") {
            Rect contentRect = ctx.layout().currentBounds();
            ctx.layout().beginContainer(contentRect);
            
            PanelOptions i18nPanelOpts;
            i18nPanelOpts.style = Style().withSize(contentRect.width(), contentRect.height());
            
            Panel(ctx, "LocalizationDemoPanel", i18nPanelOpts) {
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, i18n("localization.title"), titleOpts);
                Spacing(ctx, 10);
                
                LabelOptions sectionOpts;
                sectionOpts.color = theme.colors.textSecondary;
                
                // Language selector
                BeginVertical(ctx, 10);
                    Label(ctx, i18n("localization.select"), sectionOpts);
                    ComboBoxOptions comboOpts;
                    comboOpts.style = Style().withWidth(200);
                    if (ComboBox(ctx, "Language", selectedLocale, localeOptions, comboOpts)) {
                        I18n::instance().setLocale(localeCodes[selectedLocale]);
                    }
                EndVertical(ctx);
                
                Spacing(ctx, 20);
                Separator(ctx);
                Spacing(ctx, 15);
                
                // Show current locale info
                BeginVertical(ctx, 10);
                    Label(ctx, i18n("localization.current") + " " + I18n::instance().getLocale(), sectionOpts);
                    Spacing(ctx, 10);
                    
                    // Greeting example
                    LabelOptions greetingOpts;
                    greetingOpts.color = theme.colors.success;
                    Label(ctx, i18n("localization.greeting"), greetingOpts);
                EndVertical(ctx);
                
                Spacing(ctx, 20);
                Separator(ctx);
                Spacing(ctx, 15);
                
                // Plural forms demo
                Label(ctx, "Plural Forms Demo:", sectionOpts);
                Spacing(ctx, 5);
                
                BeginVertical(ctx, 10);
                    SliderOptions sliderOpts;
                    sliderOpts.style = Style().withWidth(200);
                    (void)Slider(ctx, "Item Count", localizationItemCount, 0.0f, 10.0f, sliderOpts);
                    
                    // Show plural translation
                    std::string pluralText = i18n_plural(
                        "localization.items.one", 
                        "localization.items.other", 
                        static_cast<int>(localizationItemCount));
                    Label(ctx, pluralText);
                EndVertical(ctx);
                
                Spacing(ctx, 20);
                Separator(ctx);
                Spacing(ctx, 15);
                
                // Show some translated UI elements
                Label(ctx, "Translated UI Elements:", sectionOpts);
                Spacing(ctx, 5);
                
                BeginHorizontal(ctx, 10);
                    ButtonOptions btnOpts;
                    btnOpts.style = Style().withSize(100, 30);
                    (void)Button(ctx, i18n("button.save"), btnOpts);
                    (void)Button(ctx, i18n("button.cancel"), btnOpts);
                    (void)Button(ctx, i18n("button.clear"), btnOpts);
                EndHorizontal(ctx);
            }
            
            ctx.layout().endContainer();
        }

        // Status Bar
        Rect statusBarRect(0, windowH - statusBarHeight, windowW, statusBarHeight);
        dl.addRectFilled(statusBarRect, theme.colors.primary);
        if (ctx.font()) {
            dl.addText(ctx.font(), Vec2(statusBarRect.x() + 10, statusBarRect.y() + 4), statusText, theme.colors.primaryText);
            
            // DEBUG: Show input state on-screen
            std::string debugInfo = "ActiveWidget: " + std::to_string(ctx.getActiveWidget()) +
                                   " InputCaptured: " + (ctx.isInputCaptured() ? "YES" : "no") +
                                   " DragActive: " + (ctx.docking().dragState().active ? "YES" : "no");
            dl.addText(ctx.font(), Vec2(windowW - 500, statusBarRect.y() + 4), debugInfo, Color::yellow());
        }

        // Render Profiler Widgets
        ShowProfilerOverlay(ctx, &showProfilerOverlay);
        ShowProfilerWindow(ctx, "Performance Profiler", &showProfilerWindow);
        
        menuBar.renderPopups(ctx);
        RenderContextMenu(ctx);
        if (const CommandPaletteCommand* executed = commandPalette.render(ctx)) {
            statusText = "Command: " + executed->label;
        }
        
        // Render docking preview overlay at the end
        RenderDockPreview(ctx);
        
        ctx.endFrame();
        window.swapBuffers();
    };

    window.setRefreshCallback(renderFrame);
    
    while (window.isOpen()) {
        window.pollEvents();
        if (window.input().isKeyPressed(Key::Escape)) {
            if (menuBar.isOpen()) menuBar.closeAll();
            else if (IsContextMenuOpen(ctx)) CloseContextMenu(ctx);
            else window.close();
        }
        renderFrame();
    }
    
    return 0;
}
