#include <gtest/gtest.h>
#include <fastener/core/context.h>
#include <fastener/core/i18n.h>
#include <algorithm>

using namespace fst;

class I18nTest : public ::testing::Test {
protected:
    I18n translations;
};

TEST_F(I18nTest, AddAndRetrieveTranslation) {
    translations.addTranslation("en", "hello", "Hello World");
    EXPECT_EQ(translations.translate("hello"), "Hello World");
}

TEST_F(I18nTest, MissingTranslationPolicyIsConfigurable) {
    EXPECT_EQ(translations.translate("missing.key"), "missing.key");
    translations.setReturnKeyIfMissing(false);
    EXPECT_EQ(translations.translate("missing.key"), "");
}

TEST_F(I18nTest, HasTranslation) {
    translations.addTranslation("en", "exists", "Value");
    EXPECT_TRUE(translations.hasTranslation("exists"));
    EXPECT_FALSE(translations.hasTranslation("does.not.exist"));
}

TEST_F(I18nTest, SwitchLocale) {
    translations.addTranslation("en", "greeting", "Hello");
    translations.addTranslation("pl", "greeting", "Cześć");

    translations.setLocale("en");
    EXPECT_EQ(translations.translate("greeting"), "Hello");
    translations.setLocale("pl");
    EXPECT_EQ(translations.translate("greeting"), "Cześć");
}

TEST_F(I18nTest, GetAvailableLocales) {
    translations.addTranslation("en", "key", "English");
    translations.addTranslation("pl", "key", "Polish");
    translations.addTranslation("de", "key", "German");

    const auto locales = translations.getAvailableLocales();
    EXPECT_EQ(locales.size(), 3);
    EXPECT_NE(std::find(locales.begin(), locales.end(), "en"), locales.end());
    EXPECT_NE(std::find(locales.begin(), locales.end(), "pl"), locales.end());
    EXPECT_NE(std::find(locales.begin(), locales.end(), "de"), locales.end());
}

TEST_F(I18nTest, FallbackLocale) {
    translations.addTranslation("en", "common", "English Common");
    translations.addTranslation("pl", "local", "Polish Only");
    translations.setLocale("pl");
    translations.setFallbackLocale("en");

    EXPECT_EQ(translations.translate("local"), "Polish Only");
    EXPECT_EQ(translations.translate("common"), "English Common");
}

TEST_F(I18nTest, ReplacesIndexedPlaceholders) {
    translations.addTranslation(
        "en", "info", "Name: {0}, Age: {1}, {0} again");
    EXPECT_EQ(
        translations.translate("info", {"Alice", "30"}),
        "Name: Alice, Age: 30, Alice again");
}

TEST_F(I18nTest, LeavesTextWithoutPlaceholdersUnchanged) {
    translations.addTranslation("en", "static", "No placeholders here");
    EXPECT_EQ(
        translations.translate("static", {"unused"}),
        "No placeholders here");
}

TEST_F(I18nTest, SelectsPluralFormsAndReplacesCount) {
    translations.addTranslation("en", "item.one", "{0} item");
    translations.addTranslation("en", "item.many", "{0} items");
    translations.addTranslation("en", "msg.one", "You have {count} message");
    translations.addTranslation("en", "msg.many", "You have {count} messages");

    EXPECT_EQ(
        translations.translatePlural("item.one", "item.many", 1),
        "1 item");
    EXPECT_EQ(
        translations.translatePlural("item.one", "item.many", 5),
        "5 items");
    EXPECT_EQ(
        translations.translatePlural("msg.one", "msg.many", 42),
        "You have 42 messages");
}

TEST_F(I18nTest, LoadsAndMergesJsonTranslations) {
    const std::string firstBatch = R"json({
        "en": {"first": "First", "quote": "Say \"Hello\""},
        "de": {"first": "Erste"}
    })json";
    const std::string secondBatch = R"json({
        "en": {"second": "Second"}
    })json";
    EXPECT_TRUE(translations.loadFromString(firstBatch));
    EXPECT_TRUE(translations.loadFromString(secondBatch));

    EXPECT_EQ(translations.translate("first"), "First");
    EXPECT_EQ(translations.translate("second"), "Second");
    EXPECT_EQ(translations.translate("quote"), "Say \"Hello\"");
    translations.setLocale("de");
    EXPECT_EQ(translations.translate("first"), "Erste");
}

TEST_F(I18nTest, InvalidJsonReturnsErrorWithoutErasingExistingData) {
    translations.addTranslation("en", "key", "value");
    EXPECT_FALSE(translations.loadFromString("not valid json"));
    EXPECT_FALSE(translations.loadFromString("{incomplete"));
    EXPECT_EQ(translations.translate("key"), "value");
}

TEST_F(I18nTest, ClearRemovesAllTranslations) {
    translations.addTranslation("en", "key", "value");
    translations.clear();
    EXPECT_EQ(translations.translate("key"), "key");
    EXPECT_TRUE(translations.getAvailableLocales().empty());
}

TEST(I18nContextTest, ContextsOwnIndependentTranslationState) {
    Context first(false);
    Context second(false);
    first.i18n().addTranslation("en", "title", "First");
    second.i18n().addTranslation("en", "title", "Second");

    EXPECT_EQ(first.i18n().translate("title"), "First");
    EXPECT_EQ(second.i18n().translate("title"), "Second");
}
