#pragma once

/**
 * @file log.h
 * @brief Lightweight logging system for Fastener library.
 */

#include <functional>
#include <string>

namespace fst {

//=============================================================================
// Log Levels
//=============================================================================

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

//=============================================================================
// Log Handler Type
//=============================================================================

/**
 * @brief Custom log handler function type.
 * @param level Log severity level
 * @param file Source file name
 * @param line Source line number
 * @param message Log message
 */
using LogHandler = std::function<void(LogLevel level, const char* file, int line, const char* message)>;

//=============================================================================
// Log API
//=============================================================================

/**
 * @brief Set a custom log handler. Pass nullptr to restore default.
 * Default handler prints to stderr.
 */
void setLogHandler(LogHandler handler);

/**
 * @brief Get the current minimum log level. Messages below this level are ignored.
 */
LogLevel getMinLogLevel();

/**
 * @brief Set the minimum log level.
 */
void setMinLogLevel(LogLevel level);

/**
 * @brief Internal logging function. Use macros instead.
 */
void logMessage(LogLevel level, const char* file, int line, const char* message);

/**
 * @brief Internal formatted logging function. Use FST_LOGF_* macros instead.
 * @param level Log severity level
 * @param file Source file name
 * @param line Source line number
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void logMessageF(LogLevel level, const char* file, int line, const char* fmt, ...);

//=============================================================================
// Logging Macros
//=============================================================================

#define FST_LOG_DEBUG(msg) ::fst::logMessage(::fst::LogLevel::Debug, __FILE__, __LINE__, msg)
#define FST_LOG_INFO(msg)  ::fst::logMessage(::fst::LogLevel::Info, __FILE__, __LINE__, msg)
#define FST_LOG_WARN(msg)  ::fst::logMessage(::fst::LogLevel::Warning, __FILE__, __LINE__, msg)
#define FST_LOG_ERROR(msg) ::fst::logMessage(::fst::LogLevel::Error, __FILE__, __LINE__, msg)

//=============================================================================
// Formatted Logging Macros (printf-style)
//=============================================================================

#define FST_LOGF_DEBUG(fmt, ...) ::fst::logMessageF(::fst::LogLevel::Debug, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define FST_LOGF_INFO(fmt, ...)  ::fst::logMessageF(::fst::LogLevel::Info, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define FST_LOGF_WARN(fmt, ...)  ::fst::logMessageF(::fst::LogLevel::Warning, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define FST_LOGF_ERROR(fmt, ...) ::fst::logMessageF(::fst::LogLevel::Error, __FILE__, __LINE__, fmt, __VA_ARGS__)

} // namespace fst
