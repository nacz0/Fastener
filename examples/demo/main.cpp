#include "fastener/fastener.h"
#include <string>

int main() {
    // Create window
    fst::WindowConfig config;
    config.title = "Fastener Demo";
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
    
    // Demo state
    bool checkboxValue = false;
    float sliderValue = 0.5f;
    int sliderIntValue = 50;
    std::string textValue = "";
    int clickCount = 0;
    bool darkTheme = true;
    
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
        
        // Demo UI would go here
        // Note: Without a font loaded, text won't render
        // In a full implementation, we'd use the layout system here
        
        // Simple test: draw some shapes
        dl.addRectFilled(fst::Rect(50, 50, 200, 40), ctx.theme().colors.primary, 8.0f);
        dl.addRectFilled(fst::Rect(50, 110, 200, 40), ctx.theme().colors.buttonBackground, 8.0f);
        dl.addRectFilled(fst::Rect(50, 170, 200, 40), ctx.theme().colors.success, 8.0f);
        
        // Draw a panel-like shape with shadow
        dl.addShadow(fst::Rect(300, 50, 300, 200), ctx.theme().colors.shadow, 10.0f, 8.0f);
        dl.addRectFilled(fst::Rect(300, 50, 300, 200), ctx.theme().colors.panelBackground, 8.0f);
        
        // Draw some circles
        dl.addCircleFilled(fst::Vec2(700, 150), 50, ctx.theme().colors.primary);
        dl.addCircle(fst::Vec2(700, 150), 60, ctx.theme().colors.border);
        
        // Draw a slider-like shape
        dl.addRectFilled(fst::Rect(50, 250, 200, 6), ctx.theme().colors.secondary, 3.0f);
        dl.addRectFilled(fst::Rect(50, 250, 100, 6), ctx.theme().colors.primary, 3.0f);
        dl.addCircleFilled(fst::Vec2(150, 253), 10, ctx.theme().colors.buttonBackground);
        dl.addCircle(fst::Vec2(150, 253), 10, ctx.theme().colors.border);
        
        // Draw checkbox-like shapes
        dl.addRectFilled(fst::Rect(50, 300, 20, 20), ctx.theme().colors.primary, 4.0f);
        dl.addLine(fst::Vec2(54, 310), fst::Vec2(58, 316), fst::Color::white(), 2.0f);
        dl.addLine(fst::Vec2(58, 316), fst::Vec2(66, 304), fst::Color::white(), 2.0f);
        
        dl.addRectFilled(fst::Rect(90, 300, 20, 20), ctx.theme().colors.inputBackground, 4.0f);
        dl.addRect(fst::Rect(90, 300, 20, 20), ctx.theme().colors.inputBorder, 4.0f);
        
        // Draw text labels if font is loaded
        if (ctx.font()) {
            dl.addText(ctx.font(), fst::Vec2(260, 60), "Primary Button", ctx.theme().colors.primaryText);
            dl.addText(ctx.font(), fst::Vec2(260, 120), "Secondary Button", ctx.theme().colors.text);
            dl.addText(ctx.font(), fst::Vec2(260, 180), "Success Button", ctx.theme().colors.text);
            
            dl.addText(ctx.font(), fst::Vec2(320, 70), "Fastener Demo", ctx.theme().colors.text);
            dl.addText(ctx.font(), fst::Vec2(320, 100), "A high-performance C++ GUI library", ctx.theme().colors.textSecondary);
            
            dl.addText(ctx.font(), fst::Vec2(50, 340), "Checkbox Example", ctx.theme().colors.text);
            dl.addText(ctx.font(), fst::Vec2(50, 280), "Slider Example", ctx.theme().colors.text);
        }
        
        // End frame
        ctx.endFrame();
        window.swapBuffers();
    }
    
    return 0;
}
