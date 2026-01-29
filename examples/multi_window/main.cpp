/**
 * @file multi_window_demo.cpp
 * @brief Demo aplikacji z wieloma oknami w Fastener
 * 
 * Pokazuje jak:
 * - Używać WindowManager do zarządzania wieloma oknami
 * - Współdzielić Context (font, theme) między oknami
 * - Renderować UI w wielu oknach jednocześnie
 */

#include "fastener/fastener.h"
#include <iostream>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace fst;

namespace {

std::filesystem::path findAssetPath(const std::filesystem::path& relativePath) {
    namespace fs = std::filesystem;
    fs::path base = fs::current_path();
#ifdef _WIN32
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
        base = fs::path(exePath).parent_path();
    }
#endif

    for (int i = 0; i < 6; ++i) {
        fs::path candidate = base / relativePath;
        if (fs::exists(candidate)) {
            return candidate;
        }
        if (!base.has_parent_path()) break;
        base = base.parent_path();
    }

    return {};
}

bool loadDemoFont(Context& ctx) {
    namespace fs = std::filesystem;
    fs::path assetFont = findAssetPath(fs::path("assets") / "arial.ttf");
    if (!assetFont.empty() && ctx.loadFont(assetFont.string(), 14.0f)) {
        return true;
    }

#ifdef _WIN32
    fs::path systemArial = "C:/Windows/Fonts/arial.ttf";
    if (fs::exists(systemArial)) {
        return ctx.loadFont(systemArial.string(), 14.0f);
    }
#else
    fs::path liberation = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
    if (fs::exists(liberation) && ctx.loadFont(liberation.string(), 14.0f)) {
        return true;
    }
    fs::path dejavu = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    if (fs::exists(dejavu)) {
        return ctx.loadFont(dejavu.string(), 14.0f);
    }
#endif

    return false;
}

} // namespace

int main() {
    WindowManager wm;
    
    // Główne okno
    WindowConfig mainConfig;
    mainConfig.title = "Fastener - Main Window";
    mainConfig.width = 600;
    mainConfig.height = 400;
    mainConfig.vsync = true;
    
    Window* mainWindow = wm.createWindow(mainConfig);
    
    if (!mainWindow) {
        std::cerr << "Failed to create main window!" << std::endl;
        return 1;
    }
    
    // Okno narzędzi
    WindowConfig toolsConfig;
    toolsConfig.title = "Fastener - Tools";
    toolsConfig.width = 350;
    toolsConfig.height = 300;
    toolsConfig.vsync = true;
    
    Window* toolsWindow = wm.createWindow(toolsConfig);
    
    // WAŻNE: Aktywny GL context przed utworzeniem Context
    mainWindow->makeContextCurrent();
    
    Context ctx;
    ctx.setTheme(Theme::dark());
    
    if (!loadDemoFont(ctx)) {
        std::cerr << "Failed to load a font." << std::endl;
    }
    
    // Stan współdzielony między oknami
    float sharedValue = 50.0f;
    bool featureEnabled = false;
    int counter = 0;
    std::vector<std::string> items = {"Apple", "Banana", "Cherry", "Dog", "Elephant"};
    
    std::cout << "Multi-window demo running. Close main window to exit." << std::endl;
    
    while (mainWindow->isOpen()) {
        wm.pollAllEvents();
        
        // === GŁÓWNE OKNO ===
        if (!mainWindow->isMinimized()) {
            mainWindow->makeContextCurrent();
            ctx.beginFrame(*mainWindow);
            
            DrawList& dl = ctx.drawList();
            const Theme& theme = ctx.theme();
            
            // Tło
            dl.addRectFilled(
                Rect(0, 0, (float)mainWindow->width(), (float)mainWindow->height()),
                theme.colors.windowBackground
            );
            
            PanelOptions panelOpts;
            panelOpts.style = Style().withPos(15, 15).withSize(
                (float)mainWindow->width() - 30, (float)mainWindow->height() - 30
            );
            
            Panel(ctx, "MainPanel", panelOpts) {
                LabelHeading(ctx, "Multi-Window Demo");
                Spacing(ctx, 10);
                
                Label(ctx, "Współdzielone kontrolki:");
                Spacing(ctx, 5);
                
                SliderOptions sliderOpts;
                sliderOpts.style = Style().withWidth(250);
                (void)Slider(ctx, "Shared Value", sharedValue, 0.0f, 100.0f, sliderOpts);
                
                (void)Checkbox(ctx, "Enable Feature", featureEnabled);
                
                Spacing(ctx, 10);
                
                BeginHorizontal(ctx, 10);
                    if (Button(ctx, "Counter++")) counter++;
                    if (Button(ctx, "Counter--")) counter--;
                    Label(ctx, "= " + std::to_string(counter));
                EndHorizontal(ctx);
                
                Spacing(ctx, 15);
                
                // Lista z Drag & Drop (cross-window!)
                Label(ctx, "Drag & Drop (przeciągaj do Tools):");
                Spacing(ctx, 5);
                
                for (size_t i = 0; i < items.size(); ++i) {
                    ctx.pushId(static_cast<int>(i));
                    bool sel = false;
                    if (Selectable(ctx, items[i], sel)) {}
                    
                    if (BeginDragDropSource(ctx)) {
                        int idx = static_cast<int>(i);
                        SetDragDropPayload("ITEM_IDX", &idx, sizeof(int));
                        SetDragDropDisplayText(items[i]);
                        EndDragDropSource();
                    }
                    
                    if (BeginDragDropTarget(ctx)) {
                        if (const auto* payload = AcceptDragDropPayload(ctx, "ITEM_IDX")) {
                            int srcIdx = payload->getData<int>();
                            int dstIdx = static_cast<int>(i);
                            if (srcIdx != dstIdx && srcIdx >= 0 && srcIdx < (int)items.size()) {
                                std::string tmp = items[srcIdx];
                                items.erase(items.begin() + srcIdx);
                                if (dstIdx > srcIdx) dstIdx--;
                                items.insert(items.begin() + dstIdx, tmp);
                            }
                        }
                        EndDragDropTarget();
                    }
                    ctx.popId();
                }
            }
            
            ctx.endFrame();
            mainWindow->swapBuffers();
        }
        
        // === OKNO TOOLS ===
        if (toolsWindow && toolsWindow->isOpen() && !toolsWindow->isMinimized()) {
            ctx.beginFrame(*toolsWindow);
            
            DrawList& dl = ctx.drawList();
            const Theme& theme = ctx.theme();
            
            dl.addRectFilled(
                Rect(0, 0, (float)toolsWindow->width(), (float)toolsWindow->height()),
                theme.colors.windowBackground.darker(0.1f)
            );
            
            PanelOptions toolsOpts;
            toolsOpts.style = Style().withPos(10, 10).withSize(
                (float)toolsWindow->width() - 20, (float)toolsWindow->height() - 20
            );
            
            Panel(ctx, "ToolsPanel", toolsOpts) {
                LabelOptions titleOpts;
                titleOpts.color = theme.colors.primary;
                Label(ctx, "TOOLS - Podgląd Stanu", titleOpts);
                Spacing(ctx, 10);
                
                Separator(ctx);
                Spacing(ctx, 10);
                
                // Wyświetl współdzielone wartości
                Label(ctx, "Slider: " + std::to_string((int)sharedValue));
                ProgressBarOptions pbOpts;
                pbOpts.style = Style().withWidth(180);
                ProgressBar(ctx, "v", sharedValue / 100.0f, pbOpts);
                
                Spacing(ctx, 8);
                
                Label(ctx, featureEnabled ? "Feature: ON" : "Feature: OFF");
                Label(ctx, "Counter: " + std::to_string(counter));
                
                Spacing(ctx, 10);
                Separator(ctx);
                Spacing(ctx, 10);
                
                // Cross-window drop zone
                Label(ctx, "Przeciągnij elementy tutaj:");
                Spacing(ctx, 5);
                
                // Drop zone area
                static std::vector<std::string> droppedItems;
                
                Rect dropZone = ctx.layout().allocate(180, 80);
                DrawList& dlTools = ctx.drawList();
                
                // Background for drop zone
                Color dropColor = theme.colors.panelBackground;
                if (IsDragDropActive()) {
                    dropColor = theme.colors.primary.withAlpha(0.3f);
                }
                dlTools.addRectFilled(dropZone, dropColor, 4.0f);
                dlTools.addRect(dropZone, theme.colors.border, 4.0f);
                
                // Show dropped items
                float textY = dropZone.y() + 5;
                for (const auto& item : droppedItems) {
                    dlTools.addText(ctx.font(), Vec2(dropZone.x() + 8, textY), item, theme.colors.text);
                    textY += 16;
                    if (textY > dropZone.y() + dropZone.height() - 16) break;
                }
                
                if (droppedItems.empty()) {
                    dlTools.addText(ctx.font(), 
                        Vec2(dropZone.x() + 15, dropZone.y() + 30), 
                        "(drop zone)", 
                        theme.colors.textSecondary);
                }
                
                // Accept cross-window drops!
                if (BeginDragDropTarget(ctx, dropZone)) {
                    if (const auto* payload = AcceptDragDropPayload(ctx, "ITEM_IDX")) {
                        int srcIdx = payload->getData<int>();
                        if (srcIdx >= 0 && srcIdx < (int)items.size()) {
                            droppedItems.push_back(items[srcIdx]);
                            items.erase(items.begin() + srcIdx);
                        }
                    }
                    EndDragDropTarget();
                }
                
                // Clear button
                Spacing(ctx, 5);
                if (Button(ctx, "Clear")) {
                    // Move items back
                    for (const auto& item : droppedItems) {
                        items.push_back(item);
                    }
                    droppedItems.clear();
                }
                
                Spacing(ctx, 15);
                
                ButtonOptions closeOpts;
                closeOpts.style = Style().withWidth(120);
                if (Button(ctx, "Close", closeOpts)) {
                    toolsWindow->close();
                }
            }
            
            ctx.endFrame();
            toolsWindow->swapBuffers();
        }
    }
    
    std::cout << "Demo closed." << std::endl;
    return 0;
}
