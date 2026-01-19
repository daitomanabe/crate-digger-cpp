#pragma once
/**
 * @file rekordbox_anlz.hpp
 * @brief Rekordbox ANLZ (analysis) file parser
 *
 * Parses ANLZ0000.DAT and ANLZ0000.EXT files from rekordbox.
 * These files contain cue points, beat grids, and waveform data.
 *
 * Based on reverse engineering by @flesniak and Deep Symmetry.
 * Reference: https://github.com/Deep-Symmetry/crate-digger/blob/main/src/main/kaitai/rekordbox_anlz.ksy
 */

#include "types.hpp"
#include <filesystem>
#include <vector>
#include <cstdint>

namespace cratedigger {

// ============================================================================
// ANLZ Section Types
// ============================================================================

/// ANLZ file section types (4-byte tags)
enum class AnlzSectionType : uint32_t {
    FileHeader = 0x50434F42,  // "PCOБ" - File header
    BeatGrid = 0x50424954,    // "PBIT" - Beat grid
    CuePointList = 0x50435545, // "PCUE" - Cue point list (older format)
    CuePointList2 = 0x50435532, // "PCU2" - Cue point list (newer format)
    ExtCuePointList = 0x50435832, // "PCX2" - Extended cue points (with colors)
    Path = 0x50505448,        // "PPTH" - File path
    VBR = 0x50564252,         // "PVBR" - VBR info
    WaveformPreview = 0x50574156, // "PWAV" - Waveform preview
    WaveformTiny = 0x5057563,  // "PWV3" - Tiny waveform
    WaveformDetail = 0x50575632, // "PWV2" - Detailed waveform
    WaveformColor = 0x50575634, // "PWV4" - Colored waveform preview
    WaveformColorDetail = 0x50575635, // "PWV5" - Colored detailed waveform
    SongStructure = 0x50534932, // "PSI2" - Song structure/phrases
    Unknown = 0
};

// ============================================================================
// Raw ANLZ Structures
// ============================================================================

/// ANLZ file header
struct RawAnlzHeader {
    uint32_t magic;           // "PMAI" = 0x504D4149
    uint32_t len_header;      // Header length
    uint32_t len_file;        // Total file length
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
};

/// ANLZ section header
struct RawAnlzSectionHeader {
    uint32_t type;            // Section type (AnlzSectionType)
    uint32_t len_header;      // Section header length
    uint32_t len_section;     // Total section length including header
};

/// Raw cue point entry (PCU2/PCUE format)
struct RawCuePointEntry {
    uint32_t magic;           // "PCP2" for PCU2, "PCPT" for PCUE
    uint32_t len_header;
    uint32_t len_entry;
    uint32_t hot_cue;         // 0 = memory cue, 1-8 = hot cue number
    uint32_t status;          // Active status
    uint32_t unknown1;
    uint32_t order_first;     // Sorting order (cue list)
    uint32_t order_last;      // Sorting order (hot cue bank)
    uint8_t type;             // Cue type
    uint8_t unknown2[3];
    uint32_t time_ms;         // Position in milliseconds
    uint32_t loop_time_ms;    // Loop end position (if loop)
    uint8_t unknown3[16];
};

/// Raw extended cue point entry (PCX2 format, with colors)
struct RawExtCuePointEntry {
    uint32_t magic;           // "PCP2"
    uint32_t len_header;
    uint32_t len_entry;
    uint32_t hot_cue;         // 0 = memory cue, 1-8 = hot cue number
    uint32_t status;          // Active status
    uint32_t unknown1;
    uint32_t order_first;
    uint32_t order_last;
    uint8_t type;             // Cue type
    uint8_t unknown2[3];
    uint32_t time_ms;         // Position in milliseconds
    uint32_t loop_time_ms;    // Loop end position
    uint8_t color_id;         // Color ID (0-8)
    uint8_t unknown3[7];
    uint32_t loop_numerator;  // Loop length numerator
    uint32_t loop_denominator; // Loop length denominator
    uint32_t len_comment;     // Comment length
    // Followed by UTF-16LE comment string
};

// ============================================================================
// Parsed Structures
// ============================================================================

/// Parsed cue point
struct CuePointData {
    uint32_t hot_cue_number{0};  // 0 = memory cue, 1-8 = hot cue
    CuePointType type{CuePointType::Cue};
    uint32_t time_ms{0};         // Position in milliseconds
    uint32_t loop_time_ms{0};    // Loop end position (0 if not a loop)
    uint8_t color_id{0};         // Color (0-8, from extended format)
    std::string comment;         // Comment (from extended format)
    bool is_active{true};
};

// ============================================================================
// ANLZ Parser Class
// ============================================================================

/**
 * @brief Rekordbox ANLZ file parser
 *
 * Parses ANLZ0000.DAT and ANLZ0000.EXT files to extract cue points,
 * beat grids, and other analysis data.
 */
class RekordboxAnlz {
public:
    /// Parse an ANLZ file
    [[nodiscard]] static Result<RekordboxAnlz> open(const std::filesystem::path& path);

    /// Move constructor
    RekordboxAnlz(RekordboxAnlz&& other) noexcept;

    /// Move assignment
    RekordboxAnlz& operator=(RekordboxAnlz&& other) noexcept;

    /// Destructor
    ~RekordboxAnlz();

    /// Not copyable
    RekordboxAnlz(const RekordboxAnlz&) = delete;
    RekordboxAnlz& operator=(const RekordboxAnlz&) = delete;

    /// Get all cue points from this file
    [[nodiscard]] const std::vector<CuePointData>& cue_points() const { return cue_points_; }

    /// Get file path stored in ANLZ
    [[nodiscard]] const std::string& track_path() const { return track_path_; }

    /// Check if file was parsed successfully
    [[nodiscard]] bool is_valid() const { return is_valid_; }

private:
    RekordboxAnlz() = default;

    void parse_sections();
    void parse_cue_list(const uint8_t* data, size_t len, bool is_extended);
    std::string parse_path_section(const uint8_t* data, size_t len);

    std::vector<uint8_t> file_data_;
    std::vector<CuePointData> cue_points_;
    std::string track_path_;
    bool is_valid_{false};
};

// ============================================================================
// Cue Point Manager
// ============================================================================

/**
 * @brief Manages cue points from ANLZ files
 *
 * Scans a directory for ANLZ files and builds an index of cue points
 * associated with track file paths.
 */
class CuePointManager {
public:
    /// Create a cue point manager
    CuePointManager() = default;

    /// Scan a directory for ANLZ files
    void scan_directory(const std::filesystem::path& anlz_dir);

    /// Load a single ANLZ file
    void load_anlz_file(const std::filesystem::path& path);

    /// Get cue points for a track by its file path
    [[nodiscard]] std::vector<CuePointData> get_cue_points(const std::string& track_path) const;

    /// Get cue points for a track by partial path match
    [[nodiscard]] std::vector<CuePointData> find_cue_points_by_filename(const std::string& filename) const;

    /// Get number of tracks with cue points
    [[nodiscard]] size_t track_count() const { return cue_point_index_.size(); }

    /// Clear all loaded cue points
    void clear() { cue_point_index_.clear(); }

private:
    // Map from track path to cue points
    std::map<std::string, std::vector<CuePointData>> cue_point_index_;
};

} // namespace cratedigger
