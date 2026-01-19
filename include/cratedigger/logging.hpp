#pragma once
/**
 * @file logging.hpp
 * @brief Structural Logging (MUST per INTRODUCTION_JAVA_TO_CPP.md Section 2.3)
 *
 * Machine-readable logging in JSON Lines format.
 * Error format: {"error": "...", "source": "file:line"}
 *
 * C++20 implementation using std::format and std::source_location
 */

#include "types.hpp"
#include <string>
#include <string_view>
#include <functional>
#include <mutex>
#include <iostream>
#include <format>
#include <source_location>

namespace cratedigger {

/// Log severity levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/// Source location alias (C++20 std::source_location)
using SourceLocation = std::source_location;

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
    void debug(const std::string& msg, SourceLocation loc = SourceLocation::current()) {
        log(LogLevel::Debug, loc, msg);
    }

    /// Info log
    void info(const std::string& msg, SourceLocation loc = SourceLocation::current()) {
        log(LogLevel::Info, loc, msg);
    }

    /// Warning log
    void warn(const std::string& msg, SourceLocation loc = SourceLocation::current()) {
        log(LogLevel::Warning, loc, msg);
    }

    /// Error log
    void error(const std::string& msg, SourceLocation loc = SourceLocation::current()) {
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

/// Convenience macros for logging with format (C++20)
#define LOG_DEBUG(msg) cratedigger::Logger::instance().debug(msg)
#define LOG_INFO(...) cratedigger::Logger::instance().info(std::format(__VA_ARGS__))
#define LOG_WARN(...) cratedigger::Logger::instance().warn(std::format(__VA_ARGS__))
#define LOG_ERROR(...) cratedigger::Logger::instance().error(std::format(__VA_ARGS__))

/// Create Error with source location (C++20)
[[nodiscard]] inline Error make_error(
    ErrorCode code,
    std::string message,
    SourceLocation loc = SourceLocation::current()
) {
    Error err;
    err.code = code;
    err.message = std::move(message);
    err.source_file = loc.file_name();
    err.source_line = static_cast<int>(loc.line());
    return err;
}

} // namespace cratedigger
