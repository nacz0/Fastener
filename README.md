# Fastener

A high-performance, minimal-dependency C++ GUI library for building modern desktop applications.

## Features

- **Minimal Dependencies**: Only uses stb_truetype and stb_image (bundled), OpenGL, and native platform APIs
- **High Performance**: Batched rendering, efficient layout caching, optimized draw calls
- **Modern Design**: Flat UI style with full theming support (light/dark modes)
- **End-User Ready**: Unlike debug-focused libraries, Fastener is designed for polished end-user applications

## Target Applications

- IDEs and code editors
- File explorers
- Desktop productivity software
- Any application requiring a native-feel, high-performance UI

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Quick Start

```cpp
#include <fastener/fastener.h>

int main() {
    fst::Window window("My App", 1280, 720);
    fst::Context ctx;
    ctx.setTheme(fst::Theme::Dark);
    
    while (window.isOpen()) {
        window.pollEvents();
        ctx.beginFrame(window);
        
        fst::Panel("Main") {
            fst::Label("Hello, Fastener!");
            if (fst::Button("Click Me")) {
                // Handle click
            }
        }
        
        ctx.endFrame();
        window.swapBuffers();
    }
    
    return 0;
}
```

## License

MIT License - see [LICENSE](LICENSE) for details.
