#pragma once
/**
 * @file logging.hpp
 * @brief Structural Logging (MUST per INTRODUCTION_JAVA_TO_CPP.md Section 2.3)
 *
 * Machine-readable logging in JSON Lines format.
 * Error format: {"error": "...", "source": "file:line"}
 *
 * C++17 compatible (using fmt library instead of std::format)
 */

#include "types.hpp"
#include <string>
#include <string_view>
#include <functional>
#include <mutex>
#include <iostream>
#include <fmt/format.h>

namespace cratedigger {

/// Log severity levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/// Source location information (C++17 replacement for std::source_location)
struct SourceLocation {
    const char* file_name;
    int line;
    const char* function_name;

    constexpr SourceLocation(
        const char* file = __builtin_FILE(),
        int ln = __builtin_LINE(),
        const char* func = __builtin_FUNCTION()
    ) noexcept : file_name(file), line(ln), function_name(func) {}
};

/// Log output callback type
using LogCallback = std::function<void(LogLevel, std::string_view)>;

/**
 * @brief Logger class for structured JSON Lines output
 */
class Logger {
public:
    /// Get the singleton logger instance
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    /// Set custom log callback (default: stderr)
    void set_callback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
    }

    /// Set minimum log level
    void set_level(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
    }

    /// Log with source location
    void log(LogLevel level, const SourceLocation& loc, const std::string& message);

    /// Debug log
    void debug(const std::string& msg, SourceLocation loc = SourceLocation()) {
        log(LogLevel::Debug, loc, msg);
    }

    /// Info log
    void info(const std::string& msg, SourceLocation loc = SourceLocation()) {
        log(LogLevel::Info, loc, msg);
    }

    /// Warning log
    void warn(const std::string& msg, SourceLocation loc = SourceLocation()) {
        log(LogLevel::Warning, loc, msg);
    }

    /// Error log
    void error(const std::string& msg, SourceLocation loc = SourceLocation()) {
        log(LogLevel::Error, loc, msg);
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogCallback callback_;
    LogLevel min_level_{LogLevel::Info};
    std::mutex mutex_;
};

/// Convenience macros for logging with format (C++17 compatible)
#define LOG_DEBUG(msg) cratedigger::Logger::instance().debug(msg)
#define LOG_INFO(...) cratedigger::Logger::instance().info(fmt::format(__VA_ARGS__))
#define LOG_WARN(...) cratedigger::Logger::instance().warn(fmt::format(__VA_ARGS__))
#define LOG_ERROR(...) cratedigger::Logger::instance().error(fmt::format(__VA_ARGS__))

/// Create Error with source location (C++17 compatible)
[[nodiscard]] inline Error make_error(
    ErrorCode code,
    std::string message,
    SourceLocation loc = SourceLocation()
) {
    Error err;
    err.code = code;
    err.message = std::move(message);
    err.source_file = loc.file_name;
    err.source_line = loc.line;
    return err;
}

} // namespace cratedigger
