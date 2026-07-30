/**
 * @file log.cpp
 * @brief Logging system implementation.
 */

#include "fastener/core/log.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <atomic>
#include <mutex>
#include <utility>

namespace fst {

//=============================================================================
// Static State
//=============================================================================

static std::mutex g_logHandlerMutex;
static LogHandler g_logHandler;
static std::atomic<LogLevel> g_minLogLevel{LogLevel::Warning};

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
    if (!path) {
        return "";
    }
    // Extract just the filename from full path
    const char* lastSlash = std::strrchr(path, '/');
    const char* lastBackslash = std::strrchr(path, '\\');
    const char* last = lastSlash;
    if (!last || (lastBackslash && lastBackslash > last)) {
        last = lastBackslash;
    }
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
    std::lock_guard<std::mutex> lock(g_logHandlerMutex);
    g_logHandler = std::move(handler);
}

LogLevel getMinLogLevel() {
    return g_minLogLevel.load(std::memory_order_relaxed);
}

void setMinLogLevel(LogLevel level) {
    g_minLogLevel.store(level, std::memory_order_relaxed);
}

void logMessage(LogLevel level, const char* file, int line, const char* message) {
    // Filter by minimum level
    if (static_cast<int>(level) <
        static_cast<int>(g_minLogLevel.load(std::memory_order_relaxed))) {
        return;
    }

    LogHandler handler;
    {
        std::lock_guard<std::mutex> lock(g_logHandlerMutex);
        handler = g_logHandler;
    }
    if (handler) {
        handler(level, file, line, message);
    } else {
        defaultLogHandler(level, file, line, message);
    }
}

void logMessageF(LogLevel level, const char* file, int line, const char* fmt, ...) {
    // Filter by minimum level
    if (static_cast<int>(level) <
        static_cast<int>(g_minLogLevel.load(std::memory_order_relaxed))) {
        return;
    }
    
    // Format the message
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    LogHandler handler;
    {
        std::lock_guard<std::mutex> lock(g_logHandlerMutex);
        handler = g_logHandler;
    }
    if (handler) {
        handler(level, file, line, buffer);
    } else {
        defaultLogHandler(level, file, line, buffer);
    }
}

} // namespace fst
