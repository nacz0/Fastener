/**
 * @file test_i18n.cpp
 * @brief Unit tests for internationalization (i18n) system.
 */

#include <gtest/gtest.h>
#include "fastener/core/i18n.h"

using namespace fst;

class I18nTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing translations and reset to defaults
        I18n::instance().clear();
        I18n::instance().setLocale("en");
        I18n::instance().setFallbackLocale("en");
        I18n::instance().setReturnKeyIfMissing(true);
    }
    
    void TearDown() override {
        I18n::instance().clear();
    }
};

//=============================================================================
// Basic Translation Tests
//=============================================================================

TEST_F(I18nTest, AddAndRetrieveTranslation) {
    I18n::instance().addTranslation("en", "hello", "Hello World");
    
    EXPECT_EQ(i18n("hello"), "Hello World");
}

TEST_F(I18nTest, MissingTranslationReturnsKey) {
    EXPECT_EQ(i18n("missing.key"), "missing.key");
}

TEST_F(I18nTest, MissingTranslationReturnsEmptyWhenConfigured) {
    I18n::instance().setReturnKeyIfMissing(false);
    EXPECT_EQ(i18n("missing.key"), "");
}

TEST_F(I18nTest, HasTranslation) {
    I18n::instance().addTranslation("en", "exists", "Value");
    
    EXPECT_TRUE(I18n::instance().hasTranslation("exists"));
    EXPECT_FALSE(I18n::instance().hasTranslation("does.not.exist"));
}

//=============================================================================
// Locale Management Tests
//=============================================================================

TEST_F(I18nTest, SwitchLocale) {
    I18n::instance().addTranslation("en", "greeting", "Hello");
    I18n::instance().addTranslation("pl", "greeting", "Cześć");
    
    I18n::instance().setLocale("en");
    EXPECT_EQ(i18n("greeting"), "Hello");
    
    I18n::instance().setLocale("pl");
    EXPECT_EQ(i18n("greeting"), "Cześć");
}

TEST_F(I18nTest, GetAvailableLocales) {
    I18n::instance().addTranslation("en", "key", "English");
    I18n::instance().addTranslation("pl", "key", "Polish");
    I18n::instance().addTranslation("de", "key", "German");
    
    auto locales = I18n::instance().getAvailableLocales();
    EXPECT_EQ(locales.size(), 3);
    
    // Check all locales are present (order not guaranteed)
    EXPECT_TRUE(std::find(locales.begin(), locales.end(), "en") != locales.end());
    EXPECT_TRUE(std::find(locales.begin(), locales.end(), "pl") != locales.end());
    EXPECT_TRUE(std::find(locales.begin(), locales.end(), "de") != locales.end());
}

TEST_F(I18nTest, FallbackLocale) {
    I18n::instance().addTranslation("en", "common", "English Common");
    I18n::instance().addTranslation("pl", "local", "Polish Only");
    
    I18n::instance().setLocale("pl");
    I18n::instance().setFallbackLocale("en");
    
    // Should find Polish translation
    EXPECT_EQ(i18n("local"), "Polish Only");
    
    // Should fallback to English
    EXPECT_EQ(i18n("common"), "English Common");
}

//=============================================================================
// Placeholder Substitution Tests
//=============================================================================

TEST_F(I18nTest, SinglePlaceholder) {
    I18n::instance().addTranslation("en", "greeting", "Hello {0}!");
    
    EXPECT_EQ(i18n("greeting", {"World"}), "Hello World!");
}

TEST_F(I18nTest, MultiplePlaceholders) {
    I18n::instance().addTranslation("en", "info", 
        "Name: {0}, Age: {1}, City: {2}");
    
    EXPECT_EQ(i18n("info", {"Alice", "30", "Warsaw"}), 
              "Name: Alice, Age: 30, City: Warsaw");
}

TEST_F(I18nTest, RepeatedPlaceholder) {
    I18n::instance().addTranslation("en", "repeat", 
        "{0} and {0} again");
    
    EXPECT_EQ(i18n("repeat", {"test"}), "test and test again");
}

TEST_F(I18nTest, NoPlaceholdersWithArgs) {
    I18n::instance().addTranslation("en", "static", "No placeholders here");
    
    // Should return text unchanged
    EXPECT_EQ(i18n("static", {"unused"}), "No placeholders here");
}

TEST_F(I18nTest, InitializerListArgs) {
    I18n::instance().addTranslation("en", "multi", "{0} + {1} = {2}");
    
    EXPECT_EQ(i18n("multi", {"2", "3", "5"}), "2 + 3 = 5");
}

//=============================================================================
// Plural Forms Tests
//=============================================================================

TEST_F(I18nTest, PluralFormSingular) {
    I18n::instance().addTranslation("en", "item.one", "{0} item");
    I18n::instance().addTranslation("en", "item.many", "{0} items");
    
    EXPECT_EQ(i18n_plural("item.one", "item.many", 1), "1 item");
}

TEST_F(I18nTest, PluralFormPlural) {
    I18n::instance().addTranslation("en", "item.one", "{0} item");
    I18n::instance().addTranslation("en", "item.many", "{0} items");
    
    EXPECT_EQ(i18n_plural("item.one", "item.many", 5), "5 items");
    EXPECT_EQ(i18n_plural("item.one", "item.many", 0), "0 items");
}

TEST_F(I18nTest, PluralFormWithCountPlaceholder) {
    I18n::instance().addTranslation("en", "msg.one", "You have {count} message");
    I18n::instance().addTranslation("en", "msg.many", "You have {count} messages");
    
    EXPECT_EQ(i18n_plural("msg.one", "msg.many", 1), "You have 1 message");
    EXPECT_EQ(i18n_plural("msg.one", "msg.many", 42), "You have 42 messages");
}

//=============================================================================
// JSON Loading Tests
//=============================================================================

TEST_F(I18nTest, LoadFromString) {
    const std::string json = R"({
        "en": {
            "title": "My App",
            "button.save": "Save"
        },
        "de": {
            "title": "Meine App",
            "button.save": "Speichern"
        }
    })";
    
    EXPECT_TRUE(I18n::instance().loadFromString(json));
    
    I18n::instance().setLocale("en");
    EXPECT_EQ(i18n("title"), "My App");
    EXPECT_EQ(i18n("button.save"), "Save");
    
    I18n::instance().setLocale("de");
    EXPECT_EQ(i18n("title"), "Meine App");
    EXPECT_EQ(i18n("button.save"), "Speichern");
}

TEST_F(I18nTest, LoadFromStringWithEscapes) {
    const std::string json = R"({
        "en": {
            "quote": "Say \"Hello\"",
            "newline": "Line1\nLine2",
            "tab": "Col1\tCol2"
        }
    })";
    
    EXPECT_TRUE(I18n::instance().loadFromString(json));
    
    EXPECT_EQ(i18n("quote"), "Say \"Hello\"");
    EXPECT_EQ(i18n("newline"), "Line1\nLine2");
    EXPECT_EQ(i18n("tab"), "Col1\tCol2");
}

TEST_F(I18nTest, MergeTranslations) {
    // Load first batch
    const std::string json1 = R"({
        "en": {
            "first": "First Value"
        }
    })";
    EXPECT_TRUE(I18n::instance().loadFromString(json1));
    
    // Load second batch (should merge, not replace)
    const std::string json2 = R"({
        "en": {
            "second": "Second Value"
        }
    })";
    EXPECT_TRUE(I18n::instance().loadFromString(json2));
    
    EXPECT_EQ(i18n("first"), "First Value");
    EXPECT_EQ(i18n("second"), "Second Value");
}

TEST_F(I18nTest, InvalidJsonReturnsError) {
    EXPECT_FALSE(I18n::instance().loadFromString("not valid json"));
    EXPECT_FALSE(I18n::instance().loadFromString("{incomplete"));
}

//=============================================================================
// Clear Tests
//=============================================================================

TEST_F(I18nTest, ClearRemovesAllTranslations) {
    I18n::instance().addTranslation("en", "key", "value");
    EXPECT_EQ(i18n("key"), "value");
    
    I18n::instance().clear();
    
    EXPECT_EQ(i18n("key"), "key");  // Returns key since missing
    EXPECT_EQ(I18n::instance().getAvailableLocales().size(), 0);
}
