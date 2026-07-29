#include <gtest/gtest.h>

#include <fastener/core/context.h>
#include <fastener/platform/window.h>
#include <fastener/widgets/table.h>

using namespace fst;

namespace {

class TableStubWindow final : public IPlatformWindow {
public:
    bool isOpen() const override { return true; }
    void close() override {}
    void pollEvents() override {}
    void waitEvents() override {}
    void swapBuffers() override {}
    void makeContextCurrent() override {}
    Vec2 size() const override { return {800.0f, 600.0f}; }
    Vec2 framebufferSize() const override { return {800.0f, 600.0f}; }
    float dpiScale() const override { return 1.0f; }
    int width() const override { return 800; }
    int height() const override { return 600; }
    void setTitle(const std::string&) override {}
    void setSize(int, int) override {}
    void setPosition(int, int) override {}
    void minimize() override {}
    void maximize() override {}
    void restore() override {}
    void focus() override {}
    bool isMinimized() const override { return false; }
    bool isMaximized() const override { return false; }
    bool isFocused() const override { return true; }
    void setCursor(Cursor) override {}
    void hideCursor() override {}
    void showCursor() override {}
    std::string getClipboardText() const override { return {}; }
    void setClipboardText(const std::string&) override {}
    InputState& input() override { return inputState; }
    const InputState& input() const override { return inputState; }
    void* nativeHandle() const override { return nullptr; }

private:
    InputState inputState;
};

} // namespace

TEST(TableContextTest, SortStateIsIsolatedForMatchingIds) {
    Context firstContext(false);
    Context secondContext(false);
    TableStubWindow firstWindow;
    TableStubWindow secondWindow;
    const std::vector<TableColumn> columns = {
        {"name", "Name", 100.0f},
        {"size", "Size", 100.0f},
    };
    TableOptions options;
    options.showHeader = false;
    options.bordered = false;

    firstWindow.input().beginFrame();
    firstContext.beginFrame(firstWindow);
    ASSERT_TRUE(BeginTable(firstContext, "shared-id", columns, options));
    SetTableSort(firstContext, 1, false);
    EXPECT_EQ(GetTableSortColumn(firstContext), 1);
    EXPECT_FALSE(GetTableSortAscending(firstContext));
    EndTable(firstContext);
    firstContext.endFrame();

    secondWindow.input().beginFrame();
    secondContext.beginFrame(secondWindow);
    ASSERT_TRUE(BeginTable(secondContext, "shared-id", columns, options));
    EXPECT_EQ(GetTableSortColumn(secondContext), -1);
    EXPECT_TRUE(GetTableSortAscending(secondContext));
    SetTableSort(secondContext, 0, true);
    EndTable(secondContext);
    secondContext.endFrame();

    firstWindow.input().beginFrame();
    firstContext.beginFrame(firstWindow);
    ASSERT_TRUE(BeginTable(firstContext, "shared-id", columns, options));
    EXPECT_EQ(GetTableSortColumn(firstContext), 1);
    EXPECT_FALSE(GetTableSortAscending(firstContext));
    EndTable(firstContext);
    firstContext.endFrame();
}

TEST(TableContextTest, EndingNestedTableRestoresParentState) {
    Context ctx(false);
    TableStubWindow window;
    const std::vector<TableColumn> columns = {
        {"name", "Name", 100.0f},
        {"size", "Size", 100.0f},
    };
    TableOptions options;
    options.showHeader = false;
    options.bordered = false;

    window.input().beginFrame();
    ctx.beginFrame(window);

    ASSERT_TRUE(BeginTable(ctx, "parent", columns, options));
    SetTableSort(ctx, 1, false);

    ASSERT_TRUE(BeginTable(ctx, "child", columns, options));
    SetTableSort(ctx, 0, true);
    EXPECT_EQ(GetTableSortColumn(ctx), 0);
    EndTable(ctx);

    EXPECT_EQ(GetTableSortColumn(ctx), 1);
    EXPECT_FALSE(GetTableSortAscending(ctx));
    EndTable(ctx);

    ctx.endFrame();
}
