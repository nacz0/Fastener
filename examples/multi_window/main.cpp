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

#ifdef _WIN32
#include <windows.h>
#endif

using namespace fst;

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
    ctx.loadFont("C:/Windows/Fonts/arial.ttf", 14.0f);
    
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
                
                // Lista z Drag & Drop (w ramach tego okna)
                Label(ctx, "Drag & Drop (wewnątrz okna):");
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
            #ifdef _WIN32
            HDC toolsDC = GetDC((HWND)toolsWindow->nativeHandle());
            wglMakeCurrent(toolsDC, (HGLRC)mainWindow->glContext());
            #endif
            
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
                
                // Lista items (podgląd)
                Label(ctx, "Items (kolejność):");
                for (const auto& item : items) {
                    LabelOptions lo;
                    lo.color = theme.colors.textSecondary;
                    Label(ctx, "  - " + item, lo);
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
