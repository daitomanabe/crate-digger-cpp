#pragma once
/**
 * @file rekordbox_pdb.hpp
 * @brief Rekordbox PDB binary parser (C++17 port of Kaitai Struct definition)
 *
 * Parses export.pdb and exportExt.pdb files from rekordbox.
 * Based on reverse engineering by @henrybetts, @flesniak, and Deep Symmetry.
 */

#include "types.hpp"
#include <fstream>
#include <memory>
#include <cstdint>
#include <vector>

namespace cratedigger {

// ============================================================================
// Page Types (from rekordbox_pdb.ksy enums)
// ============================================================================

enum class PageType : uint32_t {
    Tracks = 0,
    Genres = 1,
    Artists = 2,
    Albums = 3,
    Labels = 4,
    Keys = 5,
    Colors = 6,
    PlaylistTree = 7,
    PlaylistEntries = 8,
    Unknown9 = 9,
    Unknown10 = 10,
    HistoryPlaylists = 11,
    HistoryEntries = 12,
    Artwork = 13,
    Unknown14 = 14,
    Unknown15 = 15,
    Columns = 16,
    Unknown17 = 17,
    Unknown18 = 18,
    History = 19
};

enum class PageTypeExt : uint32_t {
    Unknown0 = 0,
    Unknown1 = 1,
    Unknown2 = 2,
    Tags = 3,
    TagTracks = 4,
    Unknown5 = 5,
    Unknown6 = 6,
    Unknown7 = 7,
    Unknown8 = 8
};

// ============================================================================
// Forward Declarations
// ============================================================================

class RekordboxPdb;
struct PdbPage;
struct PdbTable;
struct RowGroup;

// ============================================================================
// Raw Row Data Structures (direct from binary)
// ============================================================================

/// Raw track row data
struct RawTrackRow {
    uint16_t subtype;
    uint16_t index_shift;
    uint32_t bitmask;
    uint32_t sample_rate;
    uint32_t composer_id;
    uint32_t file_size;
    uint32_t unknown1;
    uint16_t unknown2;
    uint16_t unknown3;
    uint32_t artwork_id;
    uint32_t key_id;
    uint32_t original_artist_id;
    uint32_t label_id;
    uint32_t remixer_id;
    uint32_t bitrate;
    uint32_t track_number;
    uint32_t tempo;  // BPM * 100
    uint32_t genre_id;
    uint32_t album_id;
    uint32_t artist_id;
    uint32_t id;
    uint16_t disc_number;
    uint16_t play_count;
    uint16_t year;
    uint16_t sample_depth;
    uint16_t duration;
    uint16_t unknown4;
    uint8_t color_id;
    uint8_t rating;
    uint16_t unknown5;
    uint16_t unknown6;
    uint16_t ofs_strings[21];  // Offsets to variable-length strings
};

/// Raw artist row data
struct RawArtistRow {
    uint16_t subtype;
    uint16_t index_shift;
    uint32_t id;
    uint8_t unknown;
    uint8_t ofs_name_near;
};

/// Raw album row data
struct RawAlbumRow {
    uint16_t subtype;
    uint16_t index_shift;
    uint32_t unknown1;
    uint32_t artist_id;
    uint32_t id;
    uint32_t unknown2;
    uint8_t unknown3;
    uint8_t ofs_name_near;
};

/// Raw genre row data
struct RawGenreRow {
    uint32_t id;
    // Followed by device_sql_string for name
};

/// Raw label row data
struct RawLabelRow {
    uint32_t id;
    // Followed by device_sql_string for name
};

/// Raw key row data
struct RawKeyRow {
    uint32_t id;
    uint32_t id2;
    // Followed by device_sql_string for name
};

/// Raw color row data
struct RawColorRow {
    uint8_t padding[5];
    uint16_t id;
    uint8_t unknown;
    // Followed by device_sql_string for name
};

/// Raw artwork row data
struct RawArtworkRow {
    uint32_t id;
    // Followed by device_sql_string for path
};

/// Raw playlist tree row data
struct RawPlaylistTreeRow {
    uint32_t parent_id;
    uint32_t unknown;
    uint32_t sort_order;
    uint32_t id;
    uint32_t raw_is_folder;
    // Followed by device_sql_string for name
};

/// Raw playlist entry row data
struct RawPlaylistEntryRow {
    uint32_t entry_index;
    uint32_t track_id;
    uint32_t playlist_id;
};

/// Raw history playlist row data
struct RawHistoryPlaylistRow {
    uint32_t id;
    // Followed by device_sql_string for name
};

/// Raw history entry row data
struct RawHistoryEntryRow {
    uint32_t track_id;
    uint32_t playlist_id;
    uint32_t entry_index;
};

/// Raw tag row data (exportExt.pdb)
struct RawTagRow {
    uint16_t subtype;            // Usually 0x0680, or 0x0684 for long name offsets
    uint16_t tag_index;          // Increasing index in multiples of 0x20
    uint8_t reserved1[8];        // Always zero
    uint32_t category;           // ID of parent tag category (0 if this IS a category)
    uint32_t category_pos;       // Position within category (display order)
    uint32_t id;                 // Unique ID of this tag or category
    uint32_t raw_is_category;    // Non-zero if this row represents a tag category
    uint8_t reserved2;           // Always 0x03
    uint8_t ofs_name_near;       // Offset to tag/category name string
    uint8_t ofs_unknown_near;    // Offset to empty string
    // Followed by device_sql_string for name
};

/// Raw tag-track association row data (exportExt.pdb)
struct RawTagTrackRow {
    uint32_t tag_id;
    uint32_t track_id;
};

// ============================================================================
// PDB Parser Class
// ============================================================================

/**
 * @brief Rekordbox PDB file parser
 *
 * Reads and parses export.pdb and exportExt.pdb files.
 * Uses memory-mapped I/O for efficient access.
 */
class RekordboxPdb {
public:
    /// Open a PDB file
    [[nodiscard]] static Result<RekordboxPdb> open(
        const std::filesystem::path& path,
        bool is_ext = false
    );

    /// Move constructor
    RekordboxPdb(RekordboxPdb&& other) noexcept;

    /// Move assignment
    RekordboxPdb& operator=(RekordboxPdb&& other) noexcept;

    /// Destructor
    ~RekordboxPdb();

    /// Not copyable
    RekordboxPdb(const RekordboxPdb&) = delete;
    RekordboxPdb& operator=(const RekordboxPdb&) = delete;

    /// Get page size
    [[nodiscard]] uint32_t page_size() const { return page_size_; }

    /// Get number of tables
    [[nodiscard]] uint32_t table_count() const { return table_count_; }

    /// Is this an exportExt.pdb file?
    [[nodiscard]] bool is_ext() const { return is_ext_; }

    /// Get table info
    [[nodiscard]] const std::vector<PdbTable>& tables() const;

    /// Read a page at given index
    [[nodiscard]] Result<PdbPage> read_page(uint32_t page_index) const;

    /// Parse a device SQL string from the file
    [[nodiscard]] std::string read_string(size_t offset) const;

    /// Get raw data at offset (returns pointer and size, empty pair if out of bounds)
    [[nodiscard]] std::pair<const uint8_t*, size_t> data_at(size_t offset, size_t size) const;

private:
    RekordboxPdb() = default;

    std::vector<uint8_t> file_data_;
    std::vector<PdbTable> tables_;
    uint32_t page_size_{0};
    uint32_t table_count_{0};
    bool is_ext_{false};
};

// ============================================================================
// Table Structure
// ============================================================================

struct PdbTable {
    PageType type{PageType::Tracks};
    PageTypeExt type_ext{PageTypeExt::Unknown0};
    uint32_t empty_candidate{0};
    uint32_t first_page_index{0};
    uint32_t last_page_index{0};
};

// ============================================================================
// Page Structure
// ============================================================================

struct PdbPage {
    uint32_t page_index{0};
    PageType type{PageType::Tracks};
    PageTypeExt type_ext{PageTypeExt::Unknown0};
    uint32_t next_page_index{0};
    uint16_t num_row_offsets{0};
    uint16_t num_rows{0};
    uint8_t page_flags{0};
    uint16_t free_size{0};
    uint16_t used_size{0};
    bool is_data_page{false};

    /// Row groups for this page
    std::vector<RowGroup> row_groups;
};

// ============================================================================
// Row Group Structure
// ============================================================================

struct RowGroup {
    uint16_t row_present_flags{0};
    std::vector<uint16_t> row_offsets;  // Up to 16 row offsets
    size_t heap_pos{0};  // Position of heap in file
};

} // namespace cratedigger
