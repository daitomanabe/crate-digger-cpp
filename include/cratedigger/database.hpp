#pragma once
/**
 * @file database.hpp
 * @brief Rekordbox Database Parser (C++17 Port)
 *
 * Design Rules (per INTRODUCTION_JAVA_TO_CPP.md):
 * - MUST: Headless (no GUI/Window dependencies)
 * - MUST: gsl::span for array access
 * - MUST: Handle Pattern (integer IDs)
 * - MUST: No Exceptions in Process Loop
 * - MUST: No Allocation in Process Loop
 */

#include "types.hpp"
#include "logging.hpp"
#include <filesystem>
#include <memory>
#include <gsl/gsl-lite.hpp>
#include <functional>

namespace cratedigger {

// Forward declarations
class DatabaseImpl;
class RekordboxPdb;

/**
 * @brief Row handler callback for table scanning
 *
 * This mirrors Java's DatabaseUtil.RowHandler interface.
 * Used for processing rows during index building.
 */
template<typename RowType>
using RowHandler = std::function<void(const RowType&)>;

/**
 * @brief Main database class for parsing rekordbox export.pdb files
 *
 * Provides access to tracks, artists, albums, genres, labels, colors,
 * musical keys, artwork, and playlists from rekordbox database exports.
 *
 * Usage:
 *   auto result = Database::open("/path/to/export.pdb");
 *   if (result) {
 *       auto& db = *result;
 *       auto track = db.get_track(TrackId{1});
 *   }
 */
class Database {
public:
    /// Open a database file
    [[nodiscard]] static Result<Database> open(const std::filesystem::path& path);

    /// Open an exportExt.pdb file
    [[nodiscard]] static Result<Database> open_ext(const std::filesystem::path& path);

    /// Move constructor
    Database(Database&& other) noexcept;

    /// Move assignment
    Database& operator=(Database&& other) noexcept;

    /// Destructor (closes file)
    ~Database();

    /// Not copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // ========================================================================
    // Primary Index Access (ID -> Row)
    // ========================================================================

    /// Get track by ID
    [[nodiscard]] std::optional<TrackRow> get_track(TrackId id) const;

    /// Get artist by ID
    [[nodiscard]] std::optional<ArtistRow> get_artist(ArtistId id) const;

    /// Get album by ID
    [[nodiscard]] std::optional<AlbumRow> get_album(AlbumId id) const;

    /// Get genre by ID
    [[nodiscard]] std::optional<GenreRow> get_genre(GenreId id) const;

    /// Get label by ID
    [[nodiscard]] std::optional<LabelRow> get_label(LabelId id) const;

    /// Get color by ID
    [[nodiscard]] std::optional<ColorRow> get_color(ColorId id) const;

    /// Get musical key by ID
    [[nodiscard]] std::optional<KeyRow> get_key(KeyId id) const;

    /// Get artwork by ID
    [[nodiscard]] std::optional<ArtworkRow> get_artwork(ArtworkId id) const;

    // ========================================================================
    // Secondary Index Access (Name/Key -> IDs)
    // ========================================================================

    /// Find tracks by title (case-insensitive)
    [[nodiscard]] std::vector<TrackId> find_tracks_by_title(std::string_view title) const;

    /// Find tracks by artist ID
    [[nodiscard]] std::vector<TrackId> find_tracks_by_artist(ArtistId artist_id) const;

    /// Find tracks by album ID
    [[nodiscard]] std::vector<TrackId> find_tracks_by_album(AlbumId album_id) const;

    /// Find tracks by genre ID
    [[nodiscard]] std::vector<TrackId> find_tracks_by_genre(GenreId genre_id) const;

    // ========================================================================
    // Range Search
    // ========================================================================

    /// Find tracks by BPM range (inclusive)
    [[nodiscard]] std::vector<TrackId> find_tracks_by_bpm_range(float min_bpm, float max_bpm) const;

    /// Find tracks by duration range in seconds (inclusive)
    [[nodiscard]] std::vector<TrackId> find_tracks_by_duration_range(uint32_t min_seconds, uint32_t max_seconds) const;

    /// Find tracks by year range (inclusive)
    [[nodiscard]] std::vector<TrackId> find_tracks_by_year_range(uint16_t min_year, uint16_t max_year) const;

    /// Find tracks by rating range (inclusive, 0-5)
    [[nodiscard]] std::vector<TrackId> find_tracks_by_rating_range(uint16_t min_rating, uint16_t max_rating) const;

    /// Find tracks by year
    [[nodiscard]] std::vector<TrackId> find_tracks_by_year(uint16_t year) const;

    /// Find tracks by rating
    [[nodiscard]] std::vector<TrackId> find_tracks_by_rating(uint16_t rating) const;

    /// Find artists by name (case-insensitive)
    [[nodiscard]] std::vector<ArtistId> find_artists_by_name(std::string_view name) const;

    /// Find albums by name (case-insensitive)
    [[nodiscard]] std::vector<AlbumId> find_albums_by_name(std::string_view name) const;

    /// Find albums by artist ID
    [[nodiscard]] std::vector<AlbumId> find_albums_by_artist(ArtistId artist_id) const;

    /// Find genres by name (case-insensitive)
    [[nodiscard]] std::vector<GenreId> find_genres_by_name(std::string_view name) const;

    /// Find labels by name (case-insensitive)
    [[nodiscard]] std::vector<LabelId> find_labels_by_name(std::string_view name) const;

    /// Find colors by name (case-insensitive)
    [[nodiscard]] std::vector<ColorId> find_colors_by_name(std::string_view name) const;

    /// Find musical keys by name (case-insensitive)
    [[nodiscard]] std::vector<KeyId> find_keys_by_name(std::string_view name) const;

    // ========================================================================
    // Playlist Access
    // ========================================================================

    /// Get playlist track IDs
    [[nodiscard]] std::optional<std::vector<TrackId>> get_playlist(PlaylistId id) const;

    /// Get playlist folder entries
    [[nodiscard]] std::optional<std::vector<PlaylistFolderEntry>> get_playlist_folder(PlaylistId folder_id) const;

    /// Get history playlist track IDs
    [[nodiscard]] std::optional<std::vector<TrackId>> get_history_playlist(PlaylistId id) const;

    /// Find history playlist by name
    [[nodiscard]] std::optional<PlaylistId> find_history_playlist_by_name(std::string_view name) const;

    // ========================================================================
    // Tag Access (exportExt.pdb)
    // ========================================================================

    /// Get tag by ID
    [[nodiscard]] std::optional<TagRow> get_tag(TagId id) const;

    /// Find tags by name (case-insensitive)
    [[nodiscard]] std::vector<TagId> find_tags_by_name(std::string_view name) const;

    /// Get all track IDs associated with a tag
    [[nodiscard]] std::vector<TrackId> find_tracks_by_tag(TagId tag_id) const;

    /// Get all tag IDs associated with a track
    [[nodiscard]] std::vector<TagId> find_tags_by_track(TrackId track_id) const;

    /// Get all tag IDs
    [[nodiscard]] std::vector<TagId> all_tag_ids() const;

    /// Get tag count
    [[nodiscard]] size_t tag_count() const;

    // ========================================================================
    // Cue Point Access (requires loading ANLZ files)
    // ========================================================================

    /// Load cue points from an ANLZ directory
    void load_cue_points(const std::filesystem::path& anlz_dir);

    /// Load cue points from a single ANLZ file
    void load_anlz_file(const std::filesystem::path& path);

    /// Get cue points for a track by its file path
    [[nodiscard]] std::vector<CuePoint> get_cue_points(const std::string& track_path) const;

    /// Get cue points for a track by ID (uses track's file_path)
    [[nodiscard]] std::vector<CuePoint> get_cue_points_for_track(TrackId id) const;

    /// Get cue points by matching filename
    [[nodiscard]] std::vector<CuePoint> find_cue_points_by_filename(const std::string& filename) const;

    /// Get number of tracks with loaded cue points
    [[nodiscard]] size_t cue_point_track_count() const;

    // ========================================================================
    // Bulk Access (for Python/NumPy interop via std::span)
    // ========================================================================

    /// Get all track IDs (for iteration)
    [[nodiscard]] std::vector<TrackId> all_track_ids() const;

    /// Get all artist IDs
    [[nodiscard]] std::vector<ArtistId> all_artist_ids() const;

    /// Get all album IDs
    [[nodiscard]] std::vector<AlbumId> all_album_ids() const;

    /// Get all genre IDs
    [[nodiscard]] std::vector<GenreId> all_genre_ids() const;

    /// Get all playlist IDs
    [[nodiscard]] std::vector<PlaylistId> all_playlist_ids() const;

    // ========================================================================
    // Bulk Data Extraction (for NumPy/AI integration)
    // ========================================================================

    /// Get all track BPMs (for numpy.array)
    [[nodiscard]] std::vector<float> get_all_bpms() const;

    /// Get all track durations in seconds (for numpy.array)
    [[nodiscard]] std::vector<int32_t> get_all_durations() const;

    /// Get all track years (for numpy.array)
    [[nodiscard]] std::vector<int32_t> get_all_years() const;

    /// Get all track ratings (for numpy.array)
    [[nodiscard]] std::vector<int32_t> get_all_ratings() const;

    /// Get all track bitrates (for numpy.array)
    [[nodiscard]] std::vector<int32_t> get_all_bitrates() const;

    /// Get all track sample rates (for numpy.array)
    [[nodiscard]] std::vector<int32_t> get_all_sample_rates() const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /// Get track count
    [[nodiscard]] size_t track_count() const;

    /// Get artist count
    [[nodiscard]] size_t artist_count() const;

    /// Get album count
    [[nodiscard]] size_t album_count() const;

    /// Get genre count
    [[nodiscard]] size_t genre_count() const;

    /// Get playlist count
    [[nodiscard]] size_t playlist_count() const;

    // ========================================================================
    // Source File
    // ========================================================================

    /// Get the source file path
    [[nodiscard]] const std::filesystem::path& source_file() const;

private:
    /// Private constructor (use open/open_ext factory methods)
    explicit Database(std::unique_ptr<DatabaseImpl> impl);

    std::unique_ptr<DatabaseImpl> impl_;
};

} // namespace cratedigger
