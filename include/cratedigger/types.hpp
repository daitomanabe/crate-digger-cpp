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
    // Additional numeric fields
    uint32_t file_size{0};      // File size in bytes
    uint32_t track_number{0};   // Track number in album
    uint16_t disc_number{0};    // Disc number in multi-disc album
    uint16_t play_count{0};     // Play count
    uint16_t sample_depth{0};   // Bits per sample (e.g., 16, 24)
    // Additional fields (string offsets from rekordbox PDB)
    std::string isrc;           // Index 0: ISRC code
    std::string texter;         // Index 1: Unknown/Texter
    std::string message;        // Index 5: Message
    std::string kuvo_public;    // Index 6: Kuvo public flag
    std::string autoload_hot_cues; // Index 7: Autoload hot cues flag
    std::string date_added;     // Index 10: Date added
    std::string release_date;   // Index 11: Release date
    std::string mix_name;       // Index 12: Mix name
    std::string analyze_path;   // Index 14: Analyze file path
    std::string analyze_date;   // Index 15: Analyze date
    std::string filename;       // Index 19: Filename only

    /// Get BPM as float
    float bpm() const { return bpm_100x / 100.0f; }
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
    TagId category_id;           // Parent category ID (0 if this IS a category)
    uint32_t category_pos{0};    // Position within category (for ordering)
    bool is_category{false};     // True if this row represents a category, not a tag
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
// Beat Grid (ANLZ file support)
// ============================================================================

/// Beat grid entry
struct BeatEntry {
    uint16_t beat_number{0};   // Beat number within bar (1-4 typically)
    uint16_t tempo_100x{0};    // BPM * 100 (allows tempo changes)
    uint32_t time_ms{0};       // Position in milliseconds

    /// Get tempo as float BPM
    float bpm() const { return tempo_100x / 100.0f; }

    /// Get position in seconds
    float time_seconds() const { return time_ms / 1000.0f; }
};

/// Beat grid for a track
struct BeatGrid {
    std::vector<BeatEntry> beats;

    /// Check if beat grid is empty
    bool empty() const { return beats.empty(); }

    /// Get number of beats
    size_t size() const { return beats.size(); }

    /// Get beat at specific index
    const BeatEntry& operator[](size_t idx) const { return beats[idx]; }

    /// Find beat nearest to given time (binary search)
    size_t find_beat_at(uint32_t time_ms) const;

    /// Get beats within time range
    std::vector<BeatEntry> get_beats_in_range(uint32_t start_ms, uint32_t end_ms) const;

    /// Get average BPM
    float average_bpm() const;
};

// ============================================================================
// Waveform Data (ANLZ file support)
// ============================================================================

/// Waveform style/type
enum class WaveformStyle : uint8_t {
    Blue = 0,         // Monochrome blue waveform (1 byte per entry)
    RGB = 1,          // RGB colored waveform (2 bytes per entry)
    ThreeBand = 2     // 3-band waveform (3 bytes per entry, CDJ-3000 style)
};

/// Convert WaveformStyle to string
inline std::string_view waveform_style_to_string(WaveformStyle style) {
    switch (style) {
        case WaveformStyle::Blue: return "blue";
        case WaveformStyle::RGB: return "rgb";
        case WaveformStyle::ThreeBand: return "three_band";
    }
    return "unknown";
}

/// Waveform data container
struct WaveformData {
    WaveformStyle style{WaveformStyle::Blue};
    std::vector<uint8_t> data;       // Raw waveform data
    uint32_t entry_count{0};         // Number of waveform entries
    uint8_t bytes_per_entry{1};      // Bytes per entry (1, 2, 3, or 6)

    /// Check if waveform is empty
    bool empty() const { return data.empty(); }

    /// Get number of entries
    size_t size() const { return entry_count; }

    /// Get raw data pointer
    const uint8_t* raw_data() const { return data.data(); }

    /// Get raw data size in bytes
    size_t raw_size() const { return data.size(); }

    /// Get height at position (for Blue style, returns 0-31)
    uint8_t height_at(size_t idx) const {
        if (idx >= entry_count || data.empty()) return 0;
        if (style == WaveformStyle::Blue) {
            return data[idx] & 0x1F;  // Lower 5 bits are height
        } else if (style == WaveformStyle::RGB) {
            // RGB format: 2 bytes per entry
            size_t offset = idx * 2;
            if (offset + 1 < data.size()) {
                return data[offset + 1] & 0x1F;  // Height in second byte
            }
        } else if (style == WaveformStyle::ThreeBand) {
            // 3-band: max of low/mid/high
            size_t offset = idx * 3;
            if (offset + 2 < data.size()) {
                uint8_t low = data[offset] & 0x1F;
                uint8_t mid = data[offset + 1] & 0x1F;
                uint8_t high = data[offset + 2] & 0x1F;
                return std::max({low, mid, high});
            }
        }
        return 0;
    }

    /// Get color at position (for RGB style, returns 0xRRGGBB)
    uint32_t color_at(size_t idx) const {
        if (idx >= entry_count || style != WaveformStyle::RGB) return 0xFFFFFF;
        size_t offset = idx * 2;
        if (offset + 1 >= data.size()) return 0xFFFFFF;
        // RGB format: bits are packed as RGB565 or similar
        uint16_t packed = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
        uint8_t r = ((packed >> 11) & 0x1F) << 3;
        uint8_t g = ((packed >> 5) & 0x3F) << 2;
        uint8_t b = (packed & 0x1F) << 3;
        return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
    }

    /// Get 3-band values at position (low, mid, high)
    std::tuple<uint8_t, uint8_t, uint8_t> bands_at(size_t idx) const {
        if (idx >= entry_count || style != WaveformStyle::ThreeBand) {
            return {0, 0, 0};
        }
        size_t offset = idx * 3;
        if (offset + 2 >= data.size()) return {0, 0, 0};
        return {
            data[offset] & 0x1F,      // Low
            data[offset + 1] & 0x1F,  // Mid
            data[offset + 2] & 0x1F   // High
        };
    }
};

/// Waveform collection for a track (preview and detail)
struct TrackWaveforms {
    std::optional<WaveformData> preview;        // Low-res preview (PWAV)
    std::optional<WaveformData> detail;         // High-res scroll (PWV3/PWV5/PWV7)
    std::optional<WaveformData> color_preview;  // Color preview (PWV4/PWV6)

    /// Check if any waveform is available
    bool has_any() const {
        return preview.has_value() || detail.has_value() || color_preview.has_value();
    }
};

// ============================================================================
// Song Structure / Phrase Data (ANLZ file support)
// ============================================================================

/// Track mood/energy level
enum class TrackMood : uint8_t {
    High = 1,  // Up, Down, Chorus patterns
    Mid = 2,   // Multiple verse types
    Low = 3    // Verse variants with intro/outro
};

/// Track bank style
enum class TrackBank : uint8_t {
    Default = 0,
    Cool = 1,
    Natural = 2,
    Hot = 3,
    Subtle = 4,
    Warm = 5,
    Vivid = 6,
    Club1 = 7,
    Club2 = 8
};

/// Phrase kind for high mood tracks
enum class HighMoodPhrase : uint8_t {
    Intro = 1,
    Up = 2,
    Down = 3,
    Chorus = 5,
    Outro = 6
};

/// Phrase kind for mid mood tracks
enum class MidMoodPhrase : uint8_t {
    Intro = 1,
    Verse1 = 2,
    Verse2 = 3,
    Verse3 = 4,
    Verse4 = 5,
    Verse5 = 6,
    Verse6 = 7,
    Bridge = 8,
    Chorus = 9,
    Outro = 10
};

/// Phrase kind for low mood tracks
enum class LowMoodPhrase : uint8_t {
    Intro = 1,
    Verse1 = 2,
    Verse1b = 3,
    Verse1c = 4,
    Verse2 = 5,
    Verse2b = 6,
    Verse2c = 7,
    Bridge = 8,
    Chorus = 9,
    Outro = 10
};

/// Convert TrackMood to string
inline std::string_view track_mood_to_string(TrackMood mood) {
    switch (mood) {
        case TrackMood::High: return "high";
        case TrackMood::Mid: return "mid";
        case TrackMood::Low: return "low";
    }
    return "unknown";
}

/// Convert TrackBank to string
inline std::string_view track_bank_to_string(TrackBank bank) {
    switch (bank) {
        case TrackBank::Default: return "default";
        case TrackBank::Cool: return "cool";
        case TrackBank::Natural: return "natural";
        case TrackBank::Hot: return "hot";
        case TrackBank::Subtle: return "subtle";
        case TrackBank::Warm: return "warm";
        case TrackBank::Vivid: return "vivid";
        case TrackBank::Club1: return "club_1";
        case TrackBank::Club2: return "club_2";
    }
    return "unknown";
}

/// Song phrase/structure entry
struct PhraseEntry {
    uint16_t index{0};           // Phrase number (starts at 1)
    uint16_t beat{0};            // Beat where phrase starts
    uint16_t kind{0};            // Phrase kind (interpretation depends on mood)
    uint16_t end_beat{0};        // Beat where phrase ends
    uint8_t k1{0};               // Variant flag 1 (high mood)
    uint8_t k2{0};               // Variant flag 2 (high mood)
    uint8_t k3{0};               // Variant flag 3 (high mood)
    bool has_fill{false};        // Fill-in present at end
    uint16_t fill_beat{0};       // Beat where fill starts

    /// Get phrase name based on mood
    std::string phrase_name(TrackMood mood) const {
        switch (mood) {
            case TrackMood::High:
                switch (static_cast<HighMoodPhrase>(kind)) {
                    case HighMoodPhrase::Intro: return "Intro";
                    case HighMoodPhrase::Up: return "Up";
                    case HighMoodPhrase::Down: return "Down";
                    case HighMoodPhrase::Chorus: return "Chorus";
                    case HighMoodPhrase::Outro: return "Outro";
                    default: return "Unknown";
                }
            case TrackMood::Mid:
                switch (static_cast<MidMoodPhrase>(kind)) {
                    case MidMoodPhrase::Intro: return "Intro";
                    case MidMoodPhrase::Verse1: return "Verse 1";
                    case MidMoodPhrase::Verse2: return "Verse 2";
                    case MidMoodPhrase::Verse3: return "Verse 3";
                    case MidMoodPhrase::Verse4: return "Verse 4";
                    case MidMoodPhrase::Verse5: return "Verse 5";
                    case MidMoodPhrase::Verse6: return "Verse 6";
                    case MidMoodPhrase::Bridge: return "Bridge";
                    case MidMoodPhrase::Chorus: return "Chorus";
                    case MidMoodPhrase::Outro: return "Outro";
                    default: return "Unknown";
                }
            case TrackMood::Low:
                switch (static_cast<LowMoodPhrase>(kind)) {
                    case LowMoodPhrase::Intro: return "Intro";
                    case LowMoodPhrase::Verse1:
                    case LowMoodPhrase::Verse1b:
                    case LowMoodPhrase::Verse1c: return "Verse 1";
                    case LowMoodPhrase::Verse2:
                    case LowMoodPhrase::Verse2b:
                    case LowMoodPhrase::Verse2c: return "Verse 2";
                    case LowMoodPhrase::Bridge: return "Bridge";
                    case LowMoodPhrase::Chorus: return "Chorus";
                    case LowMoodPhrase::Outro: return "Outro";
                    default: return "Unknown";
                }
        }
        return "Unknown";
    }
};

/// Song structure data for a track
struct SongStructure {
    TrackMood mood{TrackMood::Mid};
    TrackBank bank{TrackBank::Default};
    uint16_t end_beat{0};           // Beat number where last phrase ends
    std::vector<PhraseEntry> phrases;

    /// Check if structure is empty
    bool empty() const { return phrases.empty(); }

    /// Get number of phrases
    size_t size() const { return phrases.size(); }

    /// Get phrase at index
    const PhraseEntry& operator[](size_t idx) const { return phrases[idx]; }

    /// Find phrase at given beat (binary search)
    size_t find_phrase_at_beat(uint16_t beat) const;
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
