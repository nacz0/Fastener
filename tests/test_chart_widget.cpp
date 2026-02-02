#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include "TestContext.h"
#include <fastener/widgets/chart.h>

using namespace fst;
using namespace fst::testing;
using ::testing::_;
using ::testing::AtLeast;

namespace {

std::string testFontPath() {
    namespace fs = std::filesystem;
    fs::path root = fs::path(__FILE__).parent_path().parent_path();
    return (root / "assets" / "arial.ttf").string();
}

} // namespace

TEST(ChartWidgetTest, DrawsLineChart) {
    TestContext tc;
    tc.beginFrame();

    std::vector<float> values = {1.0f, 2.0f, 3.0f};
    ChartOptions options;
    options.type = ChartType::Line;
    options.showGrid = false;
    options.showAxes = false;
    options.showBackground = false;
    options.showPoints = false;
    options.style = Style().withPos(0, 0).withSize(120, 60);

    EXPECT_CALL(tc.mockDrawList(), addLine(_, _, _, _)).Times(AtLeast(2));

    Chart(tc.context(), "line_chart", values, options);

    tc.endFrame();
}

TEST(ChartWidgetTest, DrawsBarChart) {
    TestContext tc;
    tc.beginFrame();

    std::vector<float> values = {3.0f, 1.5f, 2.5f};
    ChartOptions options;
    options.type = ChartType::Bar;
    options.showGrid = false;
    options.showAxes = false;
    options.showBackground = false;
    options.style = Style().withPos(0, 0).withSize(120, 60);

    EXPECT_CALL(tc.mockDrawList(), addRectFilled(_, _, _)).Times(AtLeast(3));

    Chart(tc.context(), "bar_chart", values, options);

    tc.endFrame();
}

TEST(ChartWidgetTest, DrawsPieChart) {
    TestContext tc;
    tc.beginFrame();

    std::vector<float> values = {2.0f, 3.0f, 5.0f};
    ChartOptions options;
    options.type = ChartType::Pie;
    options.showBackground = false;
    options.style = Style().withPos(0, 0).withSize(120, 120);

    EXPECT_CALL(tc.mockDrawList(), addTriangleFilled(_, _, _, _)).Times(AtLeast(3));

    Chart(tc.context(), "pie_chart", values, options);

    tc.endFrame();
}

TEST(ChartWidgetTest, DrawsLegendAndLabelsWhenFontLoaded) {
    TestContext tc;
    ASSERT_TRUE(tc.context().loadFont(testFontPath(), 16.0f));
    tc.beginFrame();

    std::vector<float> values = {2.0f, 4.0f, 3.0f};
    ChartOptions options;
    options.type = ChartType::Bar;
    options.showLegend = true;
    options.showLabels = true;
    options.labels = {"Alpha", "Beta", "Gamma"};
    options.showGrid = false;
    options.showAxes = false;
    options.showBackground = false;
    options.style = Style().withPos(0, 0).withSize(180, 100);

    EXPECT_CALL(tc.mockDrawList(), addText(_, _, _, _)).Times(AtLeast(1));

    Chart(tc.context(), "legend_chart", values, options);

    tc.endFrame();
}

TEST(ChartWidgetTest, ShowsTooltipOnHover) {
    TestContext tc;
    ASSERT_TRUE(tc.context().loadFont(testFontPath(), 16.0f));

    tc.window().input().beginFrame();
    tc.beginFrame();

    std::vector<float> values = {0.5f, 0.8f, 0.2f};
    ChartOptions options;
    options.type = ChartType::Line;
    options.showTooltips = true;
    options.showGrid = false;
    options.showAxes = false;
    options.showBackground = false;
    options.showPoints = true;
    options.style = Style().withPos(0, 0).withSize(120, 60);

    Rect bounds(0.0f, 0.0f, 120.0f, 60.0f);
    Rect plot = bounds.shrunk(options.plotPadding);
    float rangeMin = 0.0f;
    float rangeMax = 0.8f;
    float normalized = (values[0] - rangeMin) / (rangeMax - rangeMin);
    float hoverX = plot.x();
    float hoverY = plot.bottom() - normalized * plot.height();
    tc.window().input().onMouseMove(hoverX, hoverY);

    EXPECT_CALL(tc.mockDrawList(), addText(_, _, _, _)).Times(AtLeast(1));

    Chart(tc.context(), "hover_chart", values, options);

    tc.endFrame();
}
