#include "cratedigger/api_schema.hpp"
#include <sstream>

namespace cratedigger {

std::string_view to_string(ParamType type) {
    switch (type) {
        case ParamType::Int: return "int";
        case ParamType::Float: return "float";
        case ParamType::String: return "string";
        case ParamType::Bool: return "bool";
        case ParamType::IntArray: return "int[]";
        case ParamType::FloatArray: return "float[]";
        case ParamType::StringArray: return "string[]";
    }
    return "unknown";
}

namespace {

// Helper to create ParamSchema
ParamSchema make_param(
    const std::string& name,
    ParamType type,
    const std::string& description,
    bool required,
    std::optional<double> min_val = std::nullopt)
{
    ParamSchema p;
    p.name = name;
    p.type = type;
    p.description = description;
    p.required = required;
    p.min_value = min_val;
    return p;
}

// Helper to create CommandSchema
CommandSchema make_command(
    const std::string& name,
    const std::string& description,
    std::vector<ParamSchema> params,
    const std::string& returns)
{
    CommandSchema cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.params = std::move(params);
    cmd.returns = returns;
    return cmd;
}

// Helper to create TensorShape
TensorShape make_tensor(
    const std::string& name,
    std::vector<int> dims,
    const std::string& dtype,
    const std::string& description)
{
    TensorShape t;
    t.name = name;
    t.dims = std::move(dims);
    t.dtype = dtype;
    t.description = description;
    return t;
}

} // anonymous namespace

ApiSchema describe_api() {
    ApiSchema schema;
    schema.name = "crate_digger";
    schema.version = "1.0.0";
    schema.description = "Rekordbox database parser for AI/Python integration";

    // Open database command
    schema.commands.push_back(make_command(
        "open",
        "Open a rekordbox export.pdb database file",
        { make_param("path", ParamType::String, "Path to export.pdb file", true) },
        "Database handle or error"
    ));

    // Get track command
    schema.commands.push_back(make_command(
        "get_track",
        "Get track information by ID",
        { make_param("track_id", ParamType::Int, "Track ID", true, 1) },
        "TrackRow object or null"
    ));

    // Find tracks by title
    schema.commands.push_back(make_command(
        "find_tracks_by_title",
        "Find tracks by title (case-insensitive)",
        { make_param("title", ParamType::String, "Track title to search for", true) },
        "Array of track IDs"
    ));

    // Find tracks by artist
    schema.commands.push_back(make_command(
        "find_tracks_by_artist",
        "Find tracks by artist ID",
        { make_param("artist_id", ParamType::Int, "Artist ID", true, 1) },
        "Array of track IDs"
    ));

    // Get artist command
    schema.commands.push_back(make_command(
        "get_artist",
        "Get artist information by ID",
        { make_param("artist_id", ParamType::Int, "Artist ID", true, 1) },
        "ArtistRow object or null"
    ));

    // Get album command
    schema.commands.push_back(make_command(
        "get_album",
        "Get album information by ID",
        { make_param("album_id", ParamType::Int, "Album ID", true, 1) },
        "AlbumRow object or null"
    ));

    // Get playlist command
    schema.commands.push_back(make_command(
        "get_playlist",
        "Get playlist track IDs",
        { make_param("playlist_id", ParamType::Int, "Playlist ID", true, 0) },
        "Array of track IDs"
    ));

    // All track IDs
    schema.commands.push_back(make_command(
        "all_track_ids",
        "Get all track IDs in the database",
        {},
        "Array of track IDs"
    ));

    // Range search commands
    schema.commands.push_back(make_command(
        "find_tracks_by_bpm_range",
        "Find tracks within BPM range (inclusive)",
        {
            make_param("min_bpm", ParamType::Float, "Minimum BPM", true, 0),
            make_param("max_bpm", ParamType::Float, "Maximum BPM", true, 0)
        },
        "Array of track IDs"
    ));

    schema.commands.push_back(make_command(
        "find_tracks_by_duration_range",
        "Find tracks within duration range in seconds (inclusive)",
        {
            make_param("min_seconds", ParamType::Int, "Minimum duration in seconds", true, 0),
            make_param("max_seconds", ParamType::Int, "Maximum duration in seconds", true, 0)
        },
        "Array of track IDs"
    ));

    schema.commands.push_back(make_command(
        "find_tracks_by_year_range",
        "Find tracks within year range (inclusive)",
        {
            make_param("min_year", ParamType::Int, "Minimum year", true, 0),
            make_param("max_year", ParamType::Int, "Maximum year", true, 0)
        },
        "Array of track IDs"
    ));

    schema.commands.push_back(make_command(
        "find_tracks_by_rating",
        "Find tracks by rating (0-5 stars)",
        { make_param("rating", ParamType::Int, "Rating (0-5)", true, 0) },
        "Array of track IDs"
    ));

    // Bulk data extraction for NumPy/AI
    schema.commands.push_back(make_command(
        "get_all_bpms",
        "Get all track BPMs as float array (for numpy.array)",
        {},
        "Array of float BPM values"
    ));

    schema.commands.push_back(make_command(
        "get_all_durations",
        "Get all track durations in seconds as int array (for numpy.array)",
        {},
        "Array of int32 duration values"
    ));

    schema.commands.push_back(make_command(
        "get_all_years",
        "Get all track years as int array (for numpy.array)",
        {},
        "Array of int32 year values"
    ));

    schema.commands.push_back(make_command(
        "get_all_ratings",
        "Get all track ratings as int array (for numpy.array)",
        {},
        "Array of int32 rating values (0-5)"
    ));

    schema.commands.push_back(make_command(
        "get_all_bitrates",
        "Get all track bitrates as int array (for numpy.array)",
        {},
        "Array of int32 bitrate values"
    ));

    schema.commands.push_back(make_command(
        "get_all_sample_rates",
        "Get all track sample rates as int array (for numpy.array)",
        {},
        "Array of int32 sample rate values"
    ));

    // Statistics
    schema.commands.push_back(make_command(
        "track_count",
        "Get total number of tracks",
        {},
        "Integer count"
    ));

    // Describe API (self-referential)
    schema.commands.push_back(make_command(
        "describe_api",
        "Get JSON schema of all available commands (this command)",
        {},
        "API schema JSON"
    ));

    // Input/Output tensor definitions for Python/NumPy interop
    schema.inputs.push_back(make_tensor(
        "track_ids",
        {-1},  // Variable length
        "int64",
        "Array of track IDs for batch operations"
    ));

    schema.outputs.push_back(make_tensor(
        "bpm_values",
        {-1},
        "float32",
        "Array of BPM values"
    ));

    schema.outputs.push_back(make_tensor(
        "duration_values",
        {-1},
        "int32",
        "Array of track durations in seconds"
    ));

    schema.outputs.push_back(make_tensor(
        "year_values",
        {-1},
        "int32",
        "Array of track years"
    ));

    schema.outputs.push_back(make_tensor(
        "rating_values",
        {-1},
        "int32",
        "Array of track ratings (0-5)"
    ));

    schema.outputs.push_back(make_tensor(
        "bitrate_values",
        {-1},
        "int32",
        "Array of track bitrates"
    ));

    schema.outputs.push_back(make_tensor(
        "sample_rate_values",
        {-1},
        "int32",
        "Array of track sample rates"
    ));

    return schema;
}

namespace {

std::string escape_json(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

} // anonymous namespace

std::string to_json(const ParamSchema& param) {
    std::ostringstream ss;
    ss << "{";
    ss << "\"name\":\"" << escape_json(param.name) << "\"";
    ss << ",\"type\":\"" << to_string(param.type) << "\"";
    ss << ",\"description\":\"" << escape_json(param.description) << "\"";
    ss << ",\"required\":" << (param.required ? "true" : "false");

    if (param.min_value) {
        ss << ",\"min\":" << *param.min_value;
    }
    if (param.max_value) {
        ss << ",\"max\":" << *param.max_value;
    }
    if (param.unit) {
        ss << ",\"unit\":\"" << escape_json(*param.unit) << "\"";
    }
    if (param.default_value) {
        ss << ",\"default\":\"" << escape_json(*param.default_value) << "\"";
    }

    ss << "}";
    return ss.str();
}

std::string to_json(const CommandSchema& cmd) {
    std::ostringstream ss;
    ss << "{";
    ss << "\"name\":\"" << escape_json(cmd.name) << "\"";
    ss << ",\"description\":\"" << escape_json(cmd.description) << "\"";
    ss << ",\"params\":[";

    bool first = true;
    for (const auto& param : cmd.params) {
        if (!first) ss << ",";
        first = false;
        ss << to_json(param);
    }

    ss << "]";
    ss << ",\"returns\":\"" << escape_json(cmd.returns) << "\"";
    ss << "}";
    return ss.str();
}

std::string to_json(const TensorShape& tensor) {
    std::ostringstream ss;
    ss << "{";
    ss << "\"name\":\"" << escape_json(tensor.name) << "\"";
    ss << ",\"dims\":[";

    bool first = true;
    for (int dim : tensor.dims) {
        if (!first) ss << ",";
        first = false;
        ss << dim;
    }

    ss << "]";
    ss << ",\"dtype\":\"" << escape_json(tensor.dtype) << "\"";
    ss << ",\"description\":\"" << escape_json(tensor.description) << "\"";
    ss << "}";
    return ss.str();
}

std::string to_json(const ApiSchema& schema) {
    std::ostringstream ss;
    ss << "{";
    ss << "\"name\":\"" << escape_json(schema.name) << "\"";
    ss << ",\"version\":\"" << escape_json(schema.version) << "\"";
    ss << ",\"description\":\"" << escape_json(schema.description) << "\"";

    // Commands
    ss << ",\"commands\":[";
    bool first = true;
    for (const auto& cmd : schema.commands) {
        if (!first) ss << ",";
        first = false;
        ss << to_json(cmd);
    }
    ss << "]";

    // Inputs
    ss << ",\"inputs\":[";
    first = true;
    for (const auto& input : schema.inputs) {
        if (!first) ss << ",";
        first = false;
        ss << to_json(input);
    }
    ss << "]";

    // Outputs
    ss << ",\"outputs\":[";
    first = true;
    for (const auto& output : schema.outputs) {
        if (!first) ss << ",";
        first = false;
        ss << to_json(output);
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

} // namespace cratedigger
