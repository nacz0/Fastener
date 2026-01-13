#pragma once

/**
 * @file i18n.h
 * @brief Internationalization (i18n) support for Fastener library.
 * 
 * Provides translation string management with locale support, placeholder
 * substitution, and plural forms handling.
 * 
 * @example
 * // Load translations and set locale
 * fst::I18n::instance().loadFromFile("translations.json");
 * fst::I18n::instance().setLocale("pl");
 * 
 * // Use translations in UI
 * fst::Button(ctx, fst::i18n("button.save"));
 * fst::Label(ctx, fst::i18n("items.count", {std::to_string(count)}));
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace fst {

//=============================================================================
// I18n - Internationalization Manager
//=============================================================================

/**
 * @brief Manages translation strings for internationalization.
 * 
 * Singleton class that stores translations organized by locale and key.
 * Supports JSON format for translation files, placeholder substitution,
 * and fallback chains for missing translations.
 */
class I18n {
public:
    /**
     * @brief Get the singleton instance.
     */
    static I18n& instance();
    
    // Non-copyable
    I18n(const I18n&) = delete;
    I18n& operator=(const I18n&) = delete;
    
    //=========================================================================
    // Locale Management
    //=========================================================================
    
    /**
     * @brief Set the active locale.
     * @param locale Locale code (e.g., "en", "pl", "de-DE")
     */
    void setLocale(const std::string& locale);
    
    /**
     * @brief Get the current active locale.
     */
    std::string getLocale() const;
    
    /**
     * @brief Get list of all loaded locales.
     */
    std::vector<std::string> getAvailableLocales() const;
    
    /**
     * @brief Set the fallback locale for missing translations.
     * @param locale Fallback locale code (default: "en")
     */
    void setFallbackLocale(const std::string& locale);
    
    /**
     * @brief Get the fallback locale.
     */
    std::string getFallbackLocale() const;
    
    //=========================================================================
    // Translation Loading
    //=========================================================================
    
    /**
     * @brief Load translations from a JSON file.
     * 
     * Expected format:
     * @code
     * {
     *   "en": {
     *     "key1": "English text",
     *     "key2": "Text with {0} placeholder"
     *   },
     *   "pl": {
     *     "key1": "Polski tekst",
     *     "key2": "Tekst z {0} parametrem"
     *   }
     * }
     * @endcode
     * 
     * @param path Path to JSON file
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::string& path);
    
    /**
     * @brief Load translations from a JSON string.
     * @param json JSON string content
     * @return true if parsed successfully
     */
    bool loadFromString(const std::string& json);
    
    /**
     * @brief Add a single translation entry.
     * @param locale Locale code
     * @param key Translation key
     * @param value Translated text
     */
    void addTranslation(const std::string& locale, 
                        const std::string& key, 
                        const std::string& value);
    
    /**
     * @brief Clear all loaded translations.
     */
    void clear();
    
    //=========================================================================
    // Translation Lookup
    //=========================================================================
    
    /**
     * @brief Translate a key using the current locale.
     * 
     * Lookup order:
     * 1. Current locale
     * 2. Fallback locale
     * 3. Returns key itself (if returnKeyIfMissing is true)
     * 
     * @param key Translation key
     * @return Translated text or key if not found
     */
    std::string translate(const std::string& key) const;
    
    /**
     * @brief Translate a key with placeholder substitution.
     * 
     * Placeholders use {N} format where N is the argument index.
     * Example: "Hello {0}, you have {1} messages"
     * 
     * @param key Translation key
     * @param args Vector of arguments to substitute
     * @return Translated text with placeholders replaced
     */
    std::string translate(const std::string& key, 
                          const std::vector<std::string>& args) const;
    
    /**
     * @brief Translate with plural form selection.
     * 
     * Selects singular or plural key based on count.
     * 
     * @param keySingular Key for singular form (count == 1)
     * @param keyPlural Key for plural form (count != 1)
     * @param count Number to determine form
     * @return Translated text
     */
    std::string translatePlural(const std::string& keySingular,
                                const std::string& keyPlural,
                                int count) const;
    
    //=========================================================================
    // Configuration
    //=========================================================================
    
    /**
     * @brief Set whether to return the key when translation is missing.
     * @param enabled If true, returns key; if false, returns empty string
     */
    void setReturnKeyIfMissing(bool enabled);
    
    /**
     * @brief Check if a translation exists for the given key.
     * @param key Translation key
     * @param locale Optional locale to check (uses current if empty)
     */
    bool hasTranslation(const std::string& key, 
                        const std::string& locale = "") const;

private:
    I18n();
    ~I18n();
    
    std::string lookupTranslation(const std::string& locale, 
                                   const std::string& key) const;
    std::string replacePlaceholders(const std::string& text, 
                                     const std::vector<std::string>& args) const;
    
    // Locale -> (Key -> Translation)
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_translations;
    
    std::string m_currentLocale;
    std::string m_fallbackLocale;
    bool m_returnKeyIfMissing;
    
    mutable std::mutex m_mutex;  // Thread safety for multi-window
};

//=============================================================================
// Convenience Functions
//=============================================================================

/**
 * @brief Translate a key using the global I18n instance.
 * @param key Translation key
 * @return Translated text
 */
inline std::string i18n(const std::string& key) {
    return I18n::instance().translate(key);
}

/**
 * @brief Translate a key with placeholder arguments.
 * @param key Translation key
 * @param args Arguments for placeholder substitution
 * @return Translated text
 */
inline std::string i18n(const std::string& key, const std::vector<std::string>& args) {
    return I18n::instance().translate(key, args);
}

/**
 * @brief Translate a key with initializer list of arguments.
 * @param key Translation key
 * @param args Arguments for placeholder substitution
 * @return Translated text
 * 
 * @example
 * i18n("greeting", {"World", "42"})  // "Hello World, code 42"
 */
inline std::string i18n(const std::string& key, std::initializer_list<std::string> args) {
    return I18n::instance().translate(key, std::vector<std::string>(args));
}

/**
 * @brief Translate with plural form selection.
 * @param keySingular Key for singular form
 * @param keyPlural Key for plural form
 * @param count Number to determine form
 * @return Translated text
 */
inline std::string i18n_plural(const std::string& keySingular,
                                const std::string& keyPlural,
                                int count) {
    return I18n::instance().translatePlural(keySingular, keyPlural, count);
}

} // namespace fst
