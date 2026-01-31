#include <gtest/gtest.h>
#include <fastener/widgets/rich_text_preview.h>

using namespace fst;
using namespace fst::rich_text::internal;

namespace {

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
