#include "cratedigger/logging.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <format>

namespace cratedigger {

namespace {

std::string_view level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warning: return "warning";
        case LogLevel::Error: return "error";
    }
    return "unknown";
}

std::string escape_json_string(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c >= 0 && c < 32) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

} // anonymous namespace

void Logger::log(LogLevel level, const SourceLocation& loc, const std::string& message) {
    if (level < min_level_) {
        return;
    }

    // Get current timestamp in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream timestamp_stream;
    timestamp_stream << std::put_time(std::gmtime(&time), "%FT%TZ");

    // Build JSON Lines output using std::format (C++20)
    std::string json_line = std::format(
        R"({{"timestamp":"{}","level":"{}","message":"{}","source":"{}:{}"}})",
        timestamp_stream.str(),
        level_to_string(level),
        escape_json_string(message),
        loc.file_name(),
        loc.line()
    );

    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_) {
        callback_(level, json_line);
    } else {
        // Default: write to stderr for machine parsing
        std::cerr << json_line << '\n';
    }
}

} // namespace cratedigger
