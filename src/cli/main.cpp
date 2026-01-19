/**
 * @file main.cpp
 * @brief Crate Digger CLI Tool (JSONL I/O with --schema support)
 *
 * Per INTRODUCTION_JAVA_TO_CPP.md:
 * - MUST: CLI (JSONL for Agents/Pipe)
 * - MUST: --schema support for describe_api()
 */

#include "cratedigger/cratedigger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

namespace {

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [OPTIONS] [FILE]\n"
              << "\n"
              << "Crate Digger - Rekordbox database parser\n"
              << "\n"
              << "Options:\n"
              << "  --schema          Output API schema as JSON (for AI agents)\n"
              << "  --help            Show this help message\n"
              << "  --version         Show version information\n"
              << "\n"
              << "Interactive mode:\n"
              << "  When FILE is provided, opens the database and accepts JSONL commands on stdin.\n"
              << "\n"
              << "JSONL Commands:\n"
              << "  {\"cmd\": \"describe_api\"}              Get API schema\n"
              << "  {\"cmd\": \"get_track\", \"id\": 123}      Get track by ID\n"
              << "  {\"cmd\": \"find_tracks_by_title\", \"title\": \"...\"}\n"
              << "  {\"cmd\": \"all_track_ids\"}             Get all track IDs\n"
              << "  {\"cmd\": \"track_count\"}               Get track count\n"
              << "  {\"cmd\": \"exit\"}                      Exit the program\n"
              << "\n"
              << "Example:\n"
              << "  " << program_name << " --schema\n"
              << "  " << program_name << " export.pdb\n"
              << "  echo '{\"cmd\":\"track_count\"}' | " << program_name << " export.pdb\n";
}

void print_version() {
    std::cout << R"({"name":")" << cratedigger::NAME
              << R"(","version":")" << cratedigger::VERSION << R"("})" << '\n';
}

void print_schema() {
    auto schema = cratedigger::describe_api();
    std::cout << cratedigger::to_json(schema) << '\n';
}

/// Escape a string for JSON output
std::string json_escape(std::string_view str) {
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

/// Parse a simple JSON string value
std::string parse_json_string(std::string_view json, std::string_view key) {
    auto key_pos = json.find(std::string("\"") + std::string(key) + "\"");
    if (key_pos == std::string_view::npos) return "";

    auto colon_pos = json.find(':', key_pos);
    if (colon_pos == std::string_view::npos) return "";

    auto quote_start = json.find('"', colon_pos + 1);
    if (quote_start == std::string_view::npos) return "";

    auto quote_end = json.find('"', quote_start + 1);
    if (quote_end == std::string_view::npos) return "";

    return std::string(json.substr(quote_start + 1, quote_end - quote_start - 1));
}

/// Parse a simple JSON integer value
int64_t parse_json_int(std::string_view json, std::string_view key) {
    auto key_pos = json.find(std::string("\"") + std::string(key) + "\"");
    if (key_pos == std::string_view::npos) return 0;

    auto colon_pos = json.find(':', key_pos);
    if (colon_pos == std::string_view::npos) return 0;

    // Skip whitespace
    size_t num_start = colon_pos + 1;
    while (num_start < json.size() && (json[num_start] == ' ' || json[num_start] == '\t')) {
        ++num_start;
    }

    // Find end of number
    size_t num_end = num_start;
    while (num_end < json.size() && (std::isdigit(json[num_end]) || json[num_end] == '-')) {
        ++num_end;
    }

    if (num_end == num_start) return 0;

    return std::stoll(std::string(json.substr(num_start, num_end - num_start)));
}

/// Output track as JSON
void output_track(const cratedigger::TrackRow& track) {
    std::cout << "{"
              << "\"id\":" << track.id.value
              << ",\"title\":\"" << json_escape(track.title) << "\""
              << ",\"artist_id\":" << track.artist_id.value
              << ",\"album_id\":" << track.album_id.value
              << ",\"genre_id\":" << track.genre_id.value
              << ",\"bpm\":" << (track.bpm_100x / 100.0)
              << ",\"duration\":" << track.duration_seconds
              << ",\"rating\":" << track.rating
              << ",\"year\":" << track.year
              << ",\"file_path\":\"" << json_escape(track.file_path) << "\""
              << "}\n";
}

/// Output error as JSON
void output_error(std::string_view message) {
    std::cout << "{\"error\":\"" << json_escape(message) << "\"}\n";
}

/// Process a single JSONL command
bool process_command(const cratedigger::Database& db, std::string_view line) {
    auto cmd = parse_json_string(line, "cmd");

    if (cmd == "exit" || cmd == "quit") {
        return false;
    }
    else if (cmd == "describe_api") {
        print_schema();
    }
    else if (cmd == "get_track") {
        int64_t id = parse_json_int(line, "id");
        auto track = db.get_track(cratedigger::TrackId{id});
        if (track) {
            output_track(*track);
        } else {
            output_error("Track not found");
        }
    }
    else if (cmd == "find_tracks_by_title") {
        auto title = parse_json_string(line, "title");
        auto ids = db.find_tracks_by_title(title);
        std::cout << "{\"track_ids\":[";
        bool first = true;
        for (const auto& id : ids) {
            if (!first) std::cout << ",";
            first = false;
            std::cout << id.value;
        }
        std::cout << "]}\n";
    }
    else if (cmd == "all_track_ids") {
        auto ids = db.all_track_ids();
        std::cout << "{\"track_ids\":[";
        bool first = true;
        for (const auto& id : ids) {
            if (!first) std::cout << ",";
            first = false;
            std::cout << id.value;
        }
        std::cout << "]}\n";
    }
    else if (cmd == "track_count") {
        std::cout << "{\"count\":" << db.track_count() << "}\n";
    }
    else if (cmd == "artist_count") {
        std::cout << "{\"count\":" << db.artist_count() << "}\n";
    }
    else if (cmd == "album_count") {
        std::cout << "{\"count\":" << db.album_count() << "}\n";
    }
    else if (cmd == "genre_count") {
        std::cout << "{\"count\":" << db.genre_count() << "}\n";
    }
    else if (cmd == "playlist_count") {
        std::cout << "{\"count\":" << db.playlist_count() << "}\n";
    }
    else if (cmd.empty()) {
        // Ignore empty commands
    }
    else {
        output_error("Unknown command: " + std::string(cmd));
    }

    return true;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string db_path;
    bool show_schema = false;
    bool show_help = false;
    bool show_version = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--schema") == 0) {
            show_schema = true;
        }
        else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            show_help = true;
        }
        else if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            show_version = true;
        }
        else if (argv[i][0] != '-') {
            db_path = argv[i];
        }
        else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (show_version) {
        print_version();
        return 0;
    }

    if (show_schema) {
        print_schema();
        return 0;
    }

    if (db_path.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    // Open database
    auto db_result = cratedigger::Database::open(db_path);
    if (!db_result) {
        output_error(db_result.error().message);
        return 1;
    }

    auto& db = *db_result;

    // Output database info
    std::cout << "{\"status\":\"opened\""
              << ",\"tracks\":" << db.track_count()
              << ",\"artists\":" << db.artist_count()
              << ",\"albums\":" << db.album_count()
              << ",\"genres\":" << db.genre_count()
              << ",\"playlists\":" << db.playlist_count()
              << "}\n";

    // Interactive mode: read JSONL commands from stdin
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!process_command(db, line)) {
            break;
        }
    }

    return 0;
}
