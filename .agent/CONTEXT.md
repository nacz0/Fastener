# Fastener Quick Context (for AI)

**Type:** Immediate-Mode GUI | **Lang:** C++17 | **NS:** `fst`

## Core Pattern
```cpp
fst::Window window("Title", w, h);
fst::Context ctx;
ctx.loadFont("path.ttf", 14); ctx.setTheme(fst::Theme::dark());
while(window.isOpen()) {
    window.pollEvents(); ctx.beginFrame(window);
    // Widgets here - functions returning bool (interactions) or void
    ctx.endFrame(); window.swapBuffers();
}
```

## Key Rules
1. **Widgets = functions**, not objects. State via refs (`bool&`, `float&`).
2. **ID uniqueness**: Use `ctx.pushId(i)/popId()` in loops.
3. **Order matters**: `Tooltip()` AFTER widget it attaches to.
4. **Drag-drop**: `BeginDragDropSource` → `SetDragDropPayload` → `EndDragDropSource`
5. **C++17 Only**: Do NOT use designated initializers `{.field = value}` - requires C++20. Use explicit struct construction instead.

## Key Files
| Purpose | Path |
|---------|------|
| Full API | `docs/AI_CONTEXT.md` |
| Signatures | `docs/SIGNATURES.json` |
| Main include | `include/fastener/fastener.h` |
| Tests | `tests/` (run `ctest` in build/) |

## Common Widgets
`Button`, `Checkbox`, `Slider`, `TextInput`, `Panel`, `ComboBox`, `TreeView`, `TabControl`

## Common Pitfalls
1. **Missing `widget.h`**: For `WidgetInteraction`, `WidgetState`, `handleWidgetInteraction()`, `getWidgetState()` - include `fastener/ui/widget.h`
2. **Layout API**: Use `ctx.layout().beginContainer()` / `endContainer()`, NOT `pushLayout()`
3. **Window size**: Use `ctx.window().width()` / `.height()`, NOT `ctx.windowSize()`
4. **Mouse clicks**: Use `input.isMousePressed()`, NOT `mouseClicked()`
5. **New source files**: After adding `.cpp` files, run `cmake ..` to reconfigure (GLOB_RECURSE needs refresh)
