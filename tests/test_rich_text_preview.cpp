#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fastener/widgets/rich_text_preview.h>
#include "TestContext.h"
#include <algorithm>
#include <filesystem>

using namespace fst;
using namespace fst::rich_text::internal;

namespace {

std::string testFontPath() {
    namespace fs = std::filesystem;
    fs::path root = fs::path(__FILE__).parent_path().parent_path();
    return (root / "assets" / "arial.ttf").string();
}

const RichTextSpan* findSpan(const RichTextLine& line, std::string_view text) {
    for (const auto& span : line.spans) {
        if (span.text == text) {
            return &span;
        }
    }
    return nullptr;
}

} // namespace

TEST(RichTextMarkdownTest, ParsesHeading) {
    auto lines = parseMarkdown("# Title");
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_TRUE(lines[0].isHeading);
    EXPECT_EQ(lines[0].headingLevel, 1);
    ASSERT_EQ(lines[0].spans.size(), 1u);
    EXPECT_EQ(lines[0].spans[0].text, "Title");
    EXPECT_TRUE(lines[0].spans[0].style & SpanStyleHeading);
}

TEST(RichTextMarkdownTest, ParsesInlineStylesAndLinks) {
    auto lines = parseMarkdown("This is **bold** and *italic* with `code` and a [Link](https://example.com).");
    ASSERT_EQ(lines.size(), 1u);

    const RichTextSpan* bold = findSpan(lines[0], "bold");
    const RichTextSpan* italic = findSpan(lines[0], "italic");
    const RichTextSpan* code = findSpan(lines[0], "code");
    const RichTextSpan* link = findSpan(lines[0], "Link");

    ASSERT_NE(bold, nullptr);
    EXPECT_TRUE(bold->style & SpanStyleBold);

    ASSERT_NE(italic, nullptr);
    EXPECT_TRUE(italic->style & SpanStyleItalic);

    ASSERT_NE(code, nullptr);
    EXPECT_TRUE(code->style & SpanStyleCode);

    ASSERT_NE(link, nullptr);
    EXPECT_TRUE(link->style & SpanStyleLink);
    EXPECT_EQ(link->link, "https://example.com");
}

TEST(RichTextHtmlTest, ParsesBoldInParagraph) {
    auto lines = parseHtml("<p>Hello <strong>World</strong></p>");
    ASSERT_EQ(lines.size(), 1u);
    ASSERT_GE(lines[0].spans.size(), 2u);

    const RichTextSpan* world = findSpan(lines[0], "World");
    ASSERT_NE(world, nullptr);
    EXPECT_TRUE(world->style & SpanStyleBold);
}

TEST(RichTextHtmlTest, ParsesListItems) {
    auto lines = parseHtml("<ul><li>One</li><li>Two</li></ul>");
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0].prefix, "-");
    EXPECT_EQ(lines[1].prefix, "-");
    EXPECT_EQ(lines[0].spans[0].text, "One");
    EXPECT_EQ(lines[1].spans[0].text, "Two");
}

TEST(RichTextAutoTest, ParsesBasicRtf) {
    auto lines = parseRichText("{\\rtf1 Hello\\par Bold}", RichTextFormat::Auto);
    ASSERT_GE(lines.size(), 2u);
    EXPECT_EQ(lines[0].spans[0].text, "Hello");
    EXPECT_EQ(lines[1].spans[0].text, "Bold");
}

TEST(RichTextContextTest, ScrollPositionDoesNotLeakToMatchingIdInNewContext) {
    constexpr std::string_view document =
        "Line0\nLine1\nLine2\nLine3\nLine4\nLine5\nLine6\nLine7";
    RichTextPreviewOptions options;
    options.width = 200.0f;
    options.height = 45.0f;
    options.showBackground = false;
    options.showBorder = false;
    options.showScrollbar = false;

    {
        fst::testing::TestContext first;
        ASSERT_TRUE(first.context().loadFont(testFontPath(), 16.0f));

        auto& input = first.window().input();
        input.beginFrame();
        input.onMouseMove(10.0f, 10.0f);
        input.onMouseScroll(0.0f, -10.0f);

        first.beginFrame();
        RichTextPreview(first.context(), "SharedPreview", document, options);
        first.endFrame();
    }

    {
        fst::testing::TestContext second;
        ASSERT_TRUE(second.context().loadFont(testFontPath(), 16.0f));

        std::vector<std::string> renderedText;
        EXPECT_CALL(second.mockDrawList(),
                    addText(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(::testing::AtLeast(1))
            .WillRepeatedly([&renderedText](Font*, const Vec2&, std::string_view text, Color) {
                renderedText.emplace_back(text);
            });

        second.window().input().beginFrame();
        second.beginFrame();
        RichTextPreview(second.context(), "SharedPreview", document, options);
        second.endFrame();

        EXPECT_NE(std::find(renderedText.begin(), renderedText.end(), "Line0"),
                  renderedText.end());
    }
}
