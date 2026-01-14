#include "fastener/widgets/profiler_widget.h"
#include "fastener/core/context.h"
#include "fastener/core/profiler.h"
#include "fastener/widgets/panel.h"
#include "fastener/widgets/label.h"
#include "fastener/widgets/progress_bar.h"
#include "fastener/widgets/table.h"
#include "fastener/widgets/separator.h"
#include "fastener/widgets/dockable_window.h"
#include "fastener/graphics/draw_list.h"
#include <cstdio>
#include <algorithm>

namespace fst {

void ShowProfilerOverlay(Context& ctx, bool* open) {
    if (open && !*open) return;

    const float width = 180.0f;
    const float height = 80.0f;
    const float margin = 10.0f;
    
    PanelOptions opt;
    opt.style.withPos(ctx.window().width() - width - margin, margin).withSize(width, height);
    opt.title = "Profiler Overlay";

    // Using a simple panel for overlay
    if (BeginPanel(ctx, "Profiler Overlay", opt)) {
        float avgTime = ctx.profiler().getAverageFrameTime();
        float fps = avgTime > 0.0f ? 1000.0f / avgTime : 0.0f;

        char buf[128];
        snprintf(buf, sizeof(buf), "FPS: %.1f", fps);
        Label(ctx, buf);

        snprintf(buf, sizeof(buf), "Frame: %.2f ms", avgTime);
        Label(ctx, buf);

        // Simple sparkline using frame history
        float history[128];
        ctx.profiler().getFrameHistory(history, 128);
        
        float maxTime = 0.0f;
        for(float t : history) if (t > maxTime) maxTime = t;
        if (maxTime < 16.6f) maxTime = 16.6f; // Min scale 60fps

        // Draw basic lines for history
        DrawList& dl = ctx.drawList();
        Rect graphRect = ctx.layout().allocate(width - 20, 30);

        dl.addRectFilled(graphRect, Color(40, 40, 40, 200));
        
        const int count = 128;
        for (int i = 0; i < count - 1; ++i) {
            float h1 = history[i] / maxTime;
            float h2 = history[i + 1] / maxTime;
            h1 = std::clamp(h1, 0.0f, 1.0f);
            h2 = std::clamp(h2, 0.0f, 1.0f);

            Vec2 p1(graphRect.x() + (i / (float)count) * graphRect.width(), graphRect.y() + graphRect.height() * (1.0f - h1));
            Vec2 p2(graphRect.x() + ((i + 1) / (float)count) * graphRect.width(), graphRect.y() + graphRect.height() * (1.0f - h2));
            dl.addLine(p1, p2, Color(0, 255, 0), 1.0f);
        }

        EndPanel(ctx);
    }
}

void ShowProfilerWindow(Context& ctx, const char* title, bool* open) {
    if (open && !*open) return;

    DockableWindowOptions opt;
    opt.title = title;
    opt.open = open;
    opt.style.withSize(450, 500).withPos(50, 50);

    if (BeginDockableWindow(ctx, "Performance Profiler", opt)) {
        float avgTime = ctx.profiler().getAverageFrameTime();
        char buf[128];
        snprintf(buf, sizeof(buf), "Average Frame Time: %.2f ms (%.1f FPS)", avgTime, avgTime > 0 ? 1000.0f / avgTime : 0);
        Label(ctx, buf);
        
        Separator(ctx);
        
        std::vector<TableColumn> columns = {
            {"section", "Section", 200},
            {"time", "Time (ms)", 80},
            {"graph", "Graph", 150}
        };

        if (BeginTable(ctx, "ProfilerTable", columns)) {
            TableHeader(ctx);

            const auto& entries = ctx.profiler().getLastFrameEntries();
            // Sort entries by depth then by start time
            auto sortedEntries = entries;
            std::sort(sortedEntries.begin(), sortedEntries.end(), [](const ProfileEntry& a, const ProfileEntry& b) {
                if (a.depth != b.depth) return a.depth < b.depth;
                return a.startTime < b.startTime;
            });

            for (const auto& entry : sortedEntries) {
                // Indent based on depth
                std::string name = entry.name;
                for (int i = 0; i < entry.depth; ++i) name = "  " + name;
                
                char timeBuf[32];
                snprintf(timeBuf, sizeof(timeBuf), "%.3f", entry.duration);

                if (TableRow(ctx, {name, timeBuf, ""})) {
                    // Clicked row?
                }
            }
            EndTable(ctx);
        }

        EndDockableWindow(ctx);
    }
}

} // namespace fst
