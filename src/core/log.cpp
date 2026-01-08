/**
 * @file log.cpp
 * @brief Logging system implementation.
 */

#include "fastener/core/log.h"
#include <cstdio>
#include <cstring>

namespace fst {

//=============================================================================
// Static State
//=============================================================================

static LogHandler g_logHandler = nullptr;
static LogLevel g_minLogLevel = LogLevel::Warning;

//=============================================================================
// Helper Functions
//=============================================================================

static const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "?";
    }
}

static const char* getFileName(const char* path) {
    // Extract just the filename from full path
    const char* lastSlash = std::strrchr(path, '/');
    const char* lastBackslash = std::strrchr(path, '\\');
    const char* last = lastSlash > lastBackslash ? lastSlash : lastBackslash;
    return last ? last + 1 : path;
}

static void defaultLogHandler(LogLevel level, const char* file, int line, const char* message) {
    const char* levelStr = levelToString(level);
    const char* fileName = getFileName(file);
    std::fprintf(stderr, "[FST %s] %s:%d: %s\n", levelStr, fileName, line, message);
}

//=============================================================================
// Public API
//=============================================================================

void setLogHandler(LogHandler handler) {
    g_logHandler = handler;
}

LogLevel getMinLogLevel() {
    return g_minLogLevel;
}

void setMinLogLevel(LogLevel level) {
    g_minLogLevel = level;
}

void logMessage(LogLevel level, const char* file, int line, const char* message) {
    // Filter by minimum level
    if (static_cast<int>(level) < static_cast<int>(g_minLogLevel)) {
        return;
    }
    
    if (g_logHandler) {
        g_logHandler(level, file, line, message);
    } else {
        defaultLogHandler(level, file, line, message);
    }
}

} // namespace fst
