# API Reference

This document summarizes the core public API of Fastener.

## Core Types (include/fastener/core/types.h)

- `Vec2`: 2D float vector with math helpers (`length`, `normalized`, `dot`).
- `Vec4`: 4D vector used for padding/margins.
- `Rect`: position + size with hit tests and geometry helpers.
- `Color`: RGBA color with helpers (`fromHex`, `fromHSL`, `fromHSV`).
- `WidgetId`: `uint64_t` widget identifiers.
- Enums: `Alignment`, `Direction`, `Cursor`.

## Context and Lifecycle (include/fastener/core/context.h)

`fst::Context` manages frame lifecycle, state, and rendering:

- Frame: `beginFrame(IPlatformWindow&)`, `endFrame()`
- Theme: `setTheme(Theme)`, `theme()`
- Fonts: `loadFont(path, size)`, `font()`, `defaultFont()`
- Input: `input()`
- Drawing: `drawList()`, `renderer()`, `layout()`, `docking()`
- Time: `deltaTime()`, `time()`
- Focus/hover/active: `getFocusedWidget()`, `setFocusedWidget()`, etc.
- ID stack: `pushId(...)`, `popId()`, `currentId()`
- Deferred rendering: `deferRender(lambda)`

Context stack helpers:

- `Context::pushContext(ctx)`, `Context::popContext()`
- `Context::current()` is deprecated for new code.
- `WidgetScope` (include/fastener/ui/widget_scope.h) is a RAII helper for push/pop.

## Window and Input (include/fastener/platform/window.h)

`fst::Window` is the platform window implementation:

- Lifecycle: `create`, `destroy`, `isOpen`, `close`
- Events: `pollEvents`, `waitEvents`
- Rendering: `swapBuffers`, `makeContextCurrent`
- Size: `size`, `framebufferSize`, `dpiScale`, `setSize`
- Clipboard: `getClipboardText`, `setClipboardText`
- Cursor: `setCursor`, `hideCursor`, `showCursor`
- Callbacks: resize, focus, close, file drop

Input is accessible via `Context::input()` or `Window::input()`.

## Layout (include/fastener/ui/layout.h)

Low-level layout helpers:

- `BeginHorizontal(ctx, spacing)` / `EndHorizontal(ctx)`
- `BeginVertical(ctx, spacing)` / `EndVertical(ctx)`
- `Spacing(ctx, pixels)`
- `Padding(ctx, top, right, bottom, left)`
- `Allocate(ctx, width, height, flexGrow)`
- `AllocateRemainingSpace(ctx)`

## Flex Layout (include/fastener/ui/flex_layout.h)

Flex-style containers with RAII macros:

- `HStack(ctx, options) { ... }`
- `VStack(ctx, options) { ... }`
- `Grid(ctx, options) { ... }`
- Helpers: `Spacer(ctx)`, `FixedSpacer(ctx, size)`, `Divider(ctx, options)`

## Drawing (include/fastener/graphics/draw_list.h)

`DrawList` provides immediate drawing primitives such as:

- `addRect`, `addRectFilled`, `addLine`, `addCircleFilled`, `addText`
- Use `ctx.drawList()` to access the list each frame.

## Drag and Drop (include/fastener/ui/drag_drop.h)

Drag and drop uses explicit Context overloads. Deprecated overloads without Context still exist for legacy code.

Core flow:

- Source: `BeginDragDropSource(ctx)`, `SetDragDropPayload(...)`, `EndDragDropSource(ctx)`
- Target: `BeginDragDropTarget(ctx)`, `AcceptDragDropPayload(ctx, type)`, `EndDragDropTarget(ctx)`

Utilities:

- `IsDragDropActive()`, `GetDragDropPayload()`, `CancelDragDrop()`

## Theming and Style (include/fastener/ui/theme.h, style.h)

- `Theme`, `ThemeColors`, `ThemeMetrics`
- Built-in themes: `Theme::dark()`, `Theme::light()`
- Per-widget overrides: `Style` (size, padding, background, border, flex, etc.)

## Localization (include/fastener/core/i18n.h)

- `I18n::instance()` singleton
- `loadFromFile`, `setLocale`, `translate`, `translatePlural`
- Helpers: `i18n(key)`, `i18n(key, args)`, `i18n_plural(...)`

## Profiling (include/fastener/core/profiler.h, widgets/profiler_widget.h)

- `Profiler`: `beginFrame`, `endFrame`, `beginSection`, `endSection`
- `ProfileScope` RAII helper
- Widgets: `ShowProfilerOverlay(ctx)`, `ShowProfilerWindow(ctx, title)`

---
Next: WIDGETS.md
