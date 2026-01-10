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

## Key Files
| Purpose | Path |
|---------|------|
| Full API | `docs/AI_CONTEXT.md` |
| Signatures | `docs/SIGNATURES.json` |
| Main include | `include/fastener/fastener.h` |
| Tests | `tests/` (run `ctest` in build/) |

## Common Widgets
`Button`, `Checkbox`, `Slider`, `TextInput`, `Panel`, `ComboBox`, `TreeView`, `TabControl`
