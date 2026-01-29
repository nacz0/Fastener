/**
 * @file i18n.cpp
 * @brief Internationalization implementation.
 */

#include "fastener/core/i18n.h"
#include "fastener/core/log.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace fst {

//=============================================================================
// JSON Parser (Minimal Implementation)
//=============================================================================

namespace {

void appendUtf8(std::string& out, uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0x10FFFF) {
        out += static_cast<char>(0xF0 | (codepoint >> 18));
        out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        appendUtf8(out, 0xFFFD);
    }
}

bool parseHex4(const std::string& json, size_t& pos, uint16_t& out) {
    if (pos + 4 > json.size()) return false;
    uint16_t value = 0;
    for (int i = 0; i < 4; ++i) {
        char c = json[pos + i];
        value <<= 4;
        if (c >= '0' && c <= '9') value |= static_cast<uint16_t>(c - '0');
        else if (c >= 'a' && c <= 'f') value |= static_cast<uint16_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') value |= static_cast<uint16_t>(c - 'A' + 10);
        else return false;
    }
    pos += 4;
    out = value;
    return true;
}

// Skip whitespace
void skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
}

// Parse a JSON string (assumes pos is at opening quote)
std::string parseString(const std::string& json, size_t& pos) {
    if (pos >= json.size() || json[pos] != '"') {
        return "";
    }
    ++pos;  // Skip opening quote
    
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            ++pos;
            switch (json[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'u': {
                    ++pos;
                    uint16_t codeUnit = 0;
                    if (!parseHex4(json, pos, codeUnit)) {
                        appendUtf8(result, 0xFFFD);
                        --pos;
                        break;
                    }
                    
                    uint32_t codepoint = codeUnit;
                    
                    // Handle surrogate pairs
                    if (codeUnit >= 0xD800 && codeUnit <= 0xDBFF) {
                        if (pos + 2 < json.size() && json[pos] == '\\' && json[pos + 1] == 'u') {
                            pos += 2;
                            uint16_t low = 0;
                            if (parseHex4(json, pos, low) && low >= 0xDC00 && low <= 0xDFFF) {
                                codepoint = 0x10000 + (((codeUnit - 0xD800) << 10) | (low - 0xDC00));
                            } else {
                                appendUtf8(result, 0xFFFD);
                                --pos;
                                break;
                            }
                        }
                    }
                    
                    appendUtf8(result, codepoint);
                    --pos;
                    break;
                }
                default: result += json[pos]; break;
            }
        } else {
            result += json[pos];
        }
        ++pos;
    }
    
    if (pos < json.size()) {
        ++pos;  // Skip closing quote
    }
    
    return result;
}

// Parse translations from JSON into the map
bool parseTranslations(const std::string& json, 
                       std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& out) {
    size_t pos = 0;
    skipWhitespace(json, pos);
    
    // Expect opening brace for root object
    if (pos >= json.size() || json[pos] != '{') {
        FST_LOG_ERROR("i18n: Expected '{' at start of JSON");
        return false;
    }
    ++pos;
    
    while (pos < json.size()) {
        skipWhitespace(json, pos);
        
        if (json[pos] == '}') {
            break;  // End of root object
        }
        
        if (json[pos] == ',') {
            ++pos;
            continue;
        }
        
        // Parse locale key
        std::string locale = parseString(json, pos);
        if (locale.empty()) {
            FST_LOG_ERROR("i18n: Expected locale string");
            return false;
        }
        
        skipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            FST_LOG_ERROR("i18n: Expected ':' after locale");
            return false;
        }
        ++pos;
        skipWhitespace(json, pos);
        
        // Parse translations object for this locale
        if (pos >= json.size() || json[pos] != '{') {
            FST_LOG_ERROR("i18n: Expected '{' for locale translations");
            return false;
        }
        ++pos;
        
        auto& localeMap = out[locale];
        
        while (pos < json.size()) {
            skipWhitespace(json, pos);
            
            if (json[pos] == '}') {
                ++pos;
                break;
            }
            
            if (json[pos] == ',') {
                ++pos;
                continue;
            }
            
            // Parse key
            std::string key = parseString(json, pos);
            if (key.empty()) {
                FST_LOG_ERROR("i18n: Expected translation key");
                return false;
            }
            
            skipWhitespace(json, pos);
            if (pos >= json.size() || json[pos] != ':') {
                FST_LOG_ERROR("i18n: Expected ':' after key");
                return false;
            }
            ++pos;
            skipWhitespace(json, pos);
            
            // Parse value
            std::string value = parseString(json, pos);
            localeMap[key] = value;
        }
    }
    
    return true;
}

}  // anonymous namespace

//=============================================================================
// I18n Implementation
//=============================================================================

I18n::I18n()
    : m_currentLocale("en")
    , m_fallbackLocale("en")
    , m_returnKeyIfMissing(true)
{
}

I18n::~I18n() = default;

I18n& I18n::instance() {
    static I18n instance;
    return instance;
}

//=============================================================================
// Locale Management
//=============================================================================

void I18n::setLocale(const std::string& locale) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentLocale = locale;
}

std::string I18n::getLocale() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentLocale;
}

std::vector<std::string> I18n::getAvailableLocales() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> locales;
    locales.reserve(m_translations.size());
    for (const auto& pair : m_translations) {
        locales.push_back(pair.first);
    }
    return locales;
}

void I18n::setFallbackLocale(const std::string& locale) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fallbackLocale = locale;
}

std::string I18n::getFallbackLocale() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_fallbackLocale;
}

//=============================================================================
// Translation Loading
//=============================================================================

bool I18n::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        FST_LOG_ERROR("i18n: Failed to open translation file");
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

bool I18n::loadFromString(const std::string& json) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> parsed;
    if (!parseTranslations(json, parsed)) {
        return false;
    }
    
    // Merge parsed translations into existing
    for (auto& localePair : parsed) {
        auto& localeMap = m_translations[localePair.first];
        for (auto& kvPair : localePair.second) {
            localeMap[kvPair.first] = std::move(kvPair.second);
        }
    }
    
    return true;
}

void I18n::addTranslation(const std::string& locale, 
                          const std::string& key, 
                          const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_translations[locale][key] = value;
}

void I18n::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_translations.clear();
}

//=============================================================================
// Translation Lookup
//=============================================================================

std::string I18n::lookupTranslation(const std::string& locale, 
                                     const std::string& key) const {
    auto localeIt = m_translations.find(locale);
    if (localeIt != m_translations.end()) {
        auto keyIt = localeIt->second.find(key);
        if (keyIt != localeIt->second.end()) {
            return keyIt->second;
        }
    }
    return "";
}

std::string I18n::replacePlaceholders(const std::string& text, 
                                       const std::vector<std::string>& args) const {
    std::string result = text;
    
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    
    return result;
}

std::string I18n::translate(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Try current locale
    std::string result = lookupTranslation(m_currentLocale, key);
    if (!result.empty()) {
        return result;
    }
    
    // Try fallback locale
    if (m_currentLocale != m_fallbackLocale) {
        result = lookupTranslation(m_fallbackLocale, key);
        if (!result.empty()) {
            return result;
        }
    }
    
    // Return key or empty string
    return m_returnKeyIfMissing ? key : "";
}

std::string I18n::translate(const std::string& key, 
                            const std::vector<std::string>& args) const {
    std::string translated = translate(key);
    if (args.empty()) {
        return translated;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    return replacePlaceholders(translated, args);
}

std::string I18n::translatePlural(const std::string& keySingular,
                                  const std::string& keyPlural,
                                  int count) const {
    // Simple plural rule: 1 = singular, otherwise plural
    // More complex rules (like Polish) would need additional logic
    const std::string& key = (count == 1) ? keySingular : keyPlural;
    std::string result = translate(key);
    
    // Replace {count} or {0} with the actual count
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string countStr = std::to_string(count);
    
    // Replace {count}
    size_t pos = result.find("{count}");
    if (pos != std::string::npos) {
        result.replace(pos, 7, countStr);
    }
    
    // Replace {0}
    pos = result.find("{0}");
    if (pos != std::string::npos) {
        result.replace(pos, 3, countStr);
    }
    
    return result;
}

//=============================================================================
// Configuration
//=============================================================================

void I18n::setReturnKeyIfMissing(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_returnKeyIfMissing = enabled;
}

bool I18n::hasTranslation(const std::string& key, 
                          const std::string& locale) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    const std::string& checkLocale = locale.empty() ? m_currentLocale : locale;
    return !lookupTranslation(checkLocale, key).empty();
}

} // namespace fst
