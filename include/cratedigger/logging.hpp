#pragma once
/**
 * @file logging.hpp
 * @brief Structural Logging (MUST per INTRODUCTION_JAVA_TO_CPP.md Section 2.3)
 *
 * Machine-readable logging in JSON Lines format.
 * Error format: {"error": "...", "source": "file:line"}
 *
 * C++17 implementation
 */

#include "types.hpp"
#include <string>
#include <string_view>
#include <functional>
#include <mutex>
#include <iostream>
#include <sstream>

namespace cratedigger {

/// Log severity levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/// Source location struct (C++17 compatible)
struct SourceLocation {
    const char* file{"unknown"};
    int line{0};
    const char* function{"unknown"};

    const char* file_name() const { return file; }
};

/// Macro to capture current source location
#define CRATEDIGGER_CURRENT_LOCATION() \
    cratedigger::SourceLocation{__FILE__, __LINE__, __func__}

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
    void debug(const std::string& msg, SourceLocation loc = SourceLocation{}) {
        log(LogLevel::Debug, loc, msg);
    }

    /// Info log
    void info(const std::string& msg, SourceLocation loc = SourceLocation{}) {
        log(LogLevel::Info, loc, msg);
    }

    /// Warning log
    void warn(const std::string& msg, SourceLocation loc = SourceLocation{}) {
        log(LogLevel::Warning, loc, msg);
    }

    /// Error log
    void error(const std::string& msg, SourceLocation loc = SourceLocation{}) {
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

/// Convenience macros for logging (C++17)
#define LOG_DEBUG(msg) cratedigger::Logger::instance().debug(msg, CRATEDIGGER_CURRENT_LOCATION())
#define LOG_INFO(msg) cratedigger::Logger::instance().info(msg, CRATEDIGGER_CURRENT_LOCATION())
#define LOG_WARN(msg) cratedigger::Logger::instance().warn(msg, CRATEDIGGER_CURRENT_LOCATION())
#define LOG_ERROR(msg) cratedigger::Logger::instance().error(msg, CRATEDIGGER_CURRENT_LOCATION())

/// Create Error with source location (C++17)
#define CRATEDIGGER_MAKE_ERROR(code, message) \
    cratedigger::make_error_impl(code, message, __FILE__, __LINE__)

[[nodiscard]] inline Error make_error_impl(
    ErrorCode code,
    std::string message,
    const char* file,
    int line
) {
    Error err;
    err.code = code;
    err.message = std::move(message);
    err.source_file = file;
    err.source_line = line;
    return err;
}

/// Helper for simple error creation without location
[[nodiscard]] inline Error make_error(
    ErrorCode code,
    std::string message
) {
    Error err;
    err.code = code;
    err.message = std::move(message);
    return err;
}

} // namespace cratedigger
