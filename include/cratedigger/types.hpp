#pragma once
/**
 * @file types.hpp
 * @brief Core types for Crate Digger C++ (C++20)
 *
 * Design Rules (per INTRODUCTION_JAVA_TO_CPP.md):
 * - MUST: std::span for array access
 * - MUST: Handle Pattern (integer IDs, not raw pointers)
 * - MUST: No Magic Numbers
 * - MUST: Deterministic (Time Injection, Random Injection)
 */

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <optional>
#include <variant>
#include <vector>
#include <map>
#include <set>

namespace cratedigger {

// ============================================================================
// Handle Types (MUST: Integer IDs for object management)
// ============================================================================

/// Macro to define comparison operators for Handle types (C++17 compatible)
#define CRATEDIGGER_DEFINE_HANDLE_OPS(TypeName) \
    bool operator==(const TypeName& other) const { return value == other.value; } \
    bool operator!=(const TypeName& other) const { return value != other.value; } \
    bool operator<(const TypeName& other) const { return value < other.value; } \
    bool operator>(const TypeName& other) const { return value > other.value; } \
    bool operator<=(const TypeName& other) const { return value <= other.value; } \
    bool operator>=(const TypeName& other) const { return value >= other.value; }

/// Strong type for Track ID
struct TrackId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(TrackId)
};

/// Strong type for Artist ID
struct ArtistId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(ArtistId)
};

/// Strong type for Album ID
struct AlbumId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(AlbumId)
};

/// Strong type for Genre ID
struct GenreId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(GenreId)
};

/// Strong type for Label ID
struct LabelId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(LabelId)
};

/// Strong type for Color ID
struct ColorId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(ColorId)
};

/// Strong type for Musical Key ID
struct KeyId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(KeyId)
};

/// Strong type for Artwork ID
struct ArtworkId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(ArtworkId)
};

/// Strong type for Playlist ID
struct PlaylistId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(PlaylistId)
};

/// Strong type for Tag ID (exportExt.pdb)
struct TagId {
    int64_t value{0};
    CRATEDIGGER_DEFINE_HANDLE_OPS(TagId)
};

// ============================================================================
// Error Handling (MUST: No Exceptions in Process Loop)
// ============================================================================

/// Error codes for database operations
enum class ErrorCode {
    Success = 0,
    FileNotFound,
    InvalidFileFormat,
    CorruptedData,
    TableNotFound,
    RowNotFound,
    OutOfMemory,
    IoError,
    InvalidParameter,
    UnknownError
};

/// Error information with source location (for AI debugging)
struct Error {
    ErrorCode code{ErrorCode::UnknownError};
    std::string message;
    std::string source_file;
    int source_line{0};
};

/// Result type for operations that may fail (C++20 compatible alternative to std::expected)
template<typename T>
class Result {
public:
    Result(T value) : storage_(std::move(value)) {}
    Result(Error err) : storage_(std::move(err)) {}

    bool has_value() const { return std::holds_alternative<T>(storage_); }
    explicit operator bool() const { return has_value(); }

    T& value() { return std::get<T>(storage_); }
    const T& value() const { return std::get<T>(storage_); }
    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }

    Error& error() { return std::get<Error>(storage_); }
    const Error& error() const { return std::get<Error>(storage_); }

private:
    std::variant<T, Error> storage_;
};

// ============================================================================
// Database Row Types (Mapped from Kaitai Struct)
// ============================================================================

/// Track information row
struct TrackRow {
    TrackId id;
    std::string title;
    ArtistId artist_id;
    ArtistId composer_id;
    ArtistId original_artist_id;
    ArtistId remixer_id;
    AlbumId album_id;
    GenreId genre_id;
    LabelId label_id;
    KeyId key_id;
    ColorId color_id;
    ArtworkId artwork_id;
    uint32_t duration_seconds{0};
    uint32_t bpm_100x{0};  // BPM * 100
    uint16_t rating{0};
    std::string file_path;
    std::string comment;
    uint32_t bitrate{0};
    uint32_t sample_rate{0};
    uint16_t year{0};
    // Additional fields (string offsets from rekordbox PDB)
    std::string isrc;           // Index 0: ISRC code
    std::string message;        // Index 5: Message
    std::string date_added;     // Index 10: Date added
    std::string release_date;   // Index 11: Release date
    std::string mix_name;       // Index 12: Mix name
    std::string analyze_path;   // Index 14: Analyze file path
    std::string analyze_date;   // Index 15: Analyze date
    std::string filename;       // Index 19: Filename only
};

/// Artist information row
struct ArtistRow {
    ArtistId id;
    std::string name;
};

/// Album information row
struct AlbumRow {
    AlbumId id;
    std::string name;
    ArtistId artist_id;
};

/// Genre information row
struct GenreRow {
    GenreId id;
    std::string name;
};

/// Label information row
struct LabelRow {
    LabelId id;
    std::string name;
};

/// Color information row
struct ColorRow {
    ColorId id;
    std::string name;
};

/// Musical key information row
struct KeyRow {
    KeyId id;
    std::string name;
};

/// Artwork information row
struct ArtworkRow {
    ArtworkId id;
    std::string path;
};

/// Playlist folder entry
struct PlaylistFolderEntry {
    std::string name;
    bool is_folder{false};
    PlaylistId id;
};

/// Tag information row (exportExt.pdb)
struct TagRow {
    TagId id;
    std::string name;
};

// ============================================================================
// Cue Points (ANLZ file support)
// ============================================================================

/// Cue point type enumeration
enum class CuePointType : uint8_t {
    Cue = 0,        // Standard cue point
    FadeIn = 1,     // Fade in point
    FadeOut = 2,    // Fade out point
    Load = 3,       // Auto-load point
    Loop = 4        // Loop point
};

/// Convert CuePointType to string
inline std::string_view cue_point_type_to_string(CuePointType type) {
    switch (type) {
        case CuePointType::Cue: return "cue";
        case CuePointType::FadeIn: return "fade_in";
        case CuePointType::FadeOut: return "fade_out";
        case CuePointType::Load: return "load";
        case CuePointType::Loop: return "loop";
    }
    return "unknown";
}

/// Cue point information
struct CuePoint {
    CuePointType type{CuePointType::Cue};
    uint32_t time_ms{0};       // Position in milliseconds
    uint32_t loop_time_ms{0};  // Loop end position (0 if not a loop)
    uint8_t hot_cue_number{0}; // 0 = memory cue, 1-8 = hot cue
    uint8_t color_id{0};       // Color for hot cues (0-8)
    std::string comment;       // Optional comment/name

    /// Get position in seconds (float)
    float time_seconds() const { return time_ms / 1000.0f; }

    /// Check if this is a hot cue
    bool is_hot_cue() const { return hot_cue_number > 0 && hot_cue_number <= 8; }

    /// Check if this is a loop
    bool is_loop() const { return type == CuePointType::Loop && loop_time_ms > 0; }

    /// Get loop duration in milliseconds (0 if not a loop)
    uint32_t loop_duration_ms() const {
        if (is_loop() && loop_time_ms > time_ms) {
            return loop_time_ms - time_ms;
        }
        return 0;
    }
};

// ============================================================================
// Index Types (Case-insensitive string comparison)
// ============================================================================

/// Case-insensitive string comparator
struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const;
};

/// Primary index: ID -> Row
template<typename IdType, typename RowType>
using PrimaryIndex = std::map<IdType, RowType>;

/// Secondary index: Key -> Set of IDs
template<typename KeyType, typename IdType>
using SecondaryIndex = std::map<KeyType, std::set<IdType>>;

/// Name-based secondary index (case-insensitive)
template<typename IdType>
using NameIndex = std::map<std::string, std::set<IdType>, CaseInsensitiveCompare>;

/// Playlist index: Playlist ID -> List of Track IDs
using PlaylistEntryList = std::vector<TrackId>;
using PlaylistIndex = std::map<PlaylistId, PlaylistEntryList>;

/// Playlist folder index
using PlaylistFolderIndex = std::map<PlaylistId, std::vector<PlaylistFolderEntry>>;

// ============================================================================
// Configuration (No Magic Numbers)
// ============================================================================

/// Database configuration constants
struct DatabaseConfig {
    static constexpr size_t MAX_STRING_LENGTH = 4096;
    static constexpr size_t MAX_ROWS_PER_TABLE = 1000000;
    static constexpr uint32_t EXPECTED_PAGE_SIZE = 4096;
};

// ============================================================================
// Safety Curtain (Hardware Control Limits)
// ============================================================================

/// Safety limits for hardware control applications
struct SafetyLimits {
    static constexpr float MIN_BPM = 20.0f;
    static constexpr float MAX_BPM = 300.0f;
    static constexpr uint32_t MAX_DURATION_SECONDS = 86400;  // 24 hours
    static constexpr uint16_t MIN_RATING = 0;
    static constexpr uint16_t MAX_RATING = 5;
};

/// Validate BPM value (returns clamped value)
inline float validate_bpm(float bpm) {
    if (bpm < SafetyLimits::MIN_BPM) return SafetyLimits::MIN_BPM;
    if (bpm > SafetyLimits::MAX_BPM) return SafetyLimits::MAX_BPM;
    return bpm;
}

/// Validate duration value (returns clamped value)
inline uint32_t validate_duration(uint32_t duration_seconds) {
    if (duration_seconds > SafetyLimits::MAX_DURATION_SECONDS) {
        return SafetyLimits::MAX_DURATION_SECONDS;
    }
    return duration_seconds;
}

/// Validate rating value (returns clamped value)
inline uint16_t validate_rating(uint16_t rating) {
    if (rating > SafetyLimits::MAX_RATING) return SafetyLimits::MAX_RATING;
    return rating;
}

/// Check if BPM is within valid range
inline bool is_valid_bpm(float bpm) {
    return bpm >= SafetyLimits::MIN_BPM && bpm <= SafetyLimits::MAX_BPM;
}

/// Check if duration is within valid range
inline bool is_valid_duration(uint32_t duration_seconds) {
    return duration_seconds <= SafetyLimits::MAX_DURATION_SECONDS;
}

/// Check if rating is within valid range
inline bool is_valid_rating(uint16_t rating) {
    return rating <= SafetyLimits::MAX_RATING;
}

} // namespace cratedigger
