#include "database_impl.hpp"

namespace cratedigger {

// ============================================================================
// Database Implementation
// ============================================================================

Database::Database(std::unique_ptr<DatabaseImpl> impl)
    : impl_(std::move(impl))
{}

Database::Database(Database&& other) noexcept = default;
Database& Database::operator=(Database&& other) noexcept = default;
Database::~Database() = default;

Result<Database> Database::open(const std::filesystem::path& path) {
    auto pdb_result = RekordboxPdb::open(path, false);
    if (!pdb_result) {
        return pdb_result.error();
    }

    auto impl = std::make_unique<DatabaseImpl>(std::move(*pdb_result), path);
    impl->build_indices();

    return Database(std::move(impl));
}

Result<Database> Database::open_ext(const std::filesystem::path& path) {
    auto pdb_result = RekordboxPdb::open(path, true);
    if (!pdb_result) {
        return pdb_result.error();
    }

    auto impl = std::make_unique<DatabaseImpl>(std::move(*pdb_result), path);
    impl->build_indices();

    return Database(std::move(impl));
}

// ============================================================================
// Primary Index Access
// ============================================================================

std::optional<TrackRow> Database::get_track(TrackId id) const {
    auto it = impl_->track_index.find(id);
    if (it != impl_->track_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ArtistRow> Database::get_artist(ArtistId id) const {
    auto it = impl_->artist_index.find(id);
    if (it != impl_->artist_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AlbumRow> Database::get_album(AlbumId id) const {
    auto it = impl_->album_index.find(id);
    if (it != impl_->album_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<GenreRow> Database::get_genre(GenreId id) const {
    auto it = impl_->genre_index.find(id);
    if (it != impl_->genre_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<LabelRow> Database::get_label(LabelId id) const {
    auto it = impl_->label_index.find(id);
    if (it != impl_->label_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ColorRow> Database::get_color(ColorId id) const {
    auto it = impl_->color_index.find(id);
    if (it != impl_->color_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<KeyRow> Database::get_key(KeyId id) const {
    auto it = impl_->key_index.find(id);
    if (it != impl_->key_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ArtworkRow> Database::get_artwork(ArtworkId id) const {
    auto it = impl_->artwork_index.find(id);
    if (it != impl_->artwork_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ============================================================================
// Secondary Index Access
// ============================================================================

std::vector<TrackId> Database::find_tracks_by_title(std::string_view title) const {
    std::vector<TrackId> result;
    auto it = impl_->track_title_index.find(std::string(title));
    if (it != impl_->track_title_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_artist(ArtistId artist_id) const {
    std::vector<TrackId> result;
    auto it = impl_->track_artist_index.find(artist_id);
    if (it != impl_->track_artist_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_album(AlbumId album_id) const {
    std::vector<TrackId> result;
    auto it = impl_->track_album_index.find(album_id);
    if (it != impl_->track_album_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_genre(GenreId genre_id) const {
    std::vector<TrackId> result;
    auto it = impl_->track_genre_index.find(genre_id);
    if (it != impl_->track_genre_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

// ============================================================================
// Range Search
// ============================================================================

std::vector<TrackId> Database::find_tracks_by_bpm_range(float min_bpm, float max_bpm) const {
    std::vector<TrackId> result;
    uint32_t min_bpm_100x = static_cast<uint32_t>(min_bpm * 100.0f);
    uint32_t max_bpm_100x = static_cast<uint32_t>(max_bpm * 100.0f);

    for (const auto& [id, track] : impl_->track_index) {
        if (track.bpm_100x >= min_bpm_100x && track.bpm_100x <= max_bpm_100x) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_duration_range(uint32_t min_seconds, uint32_t max_seconds) const {
    std::vector<TrackId> result;
    for (const auto& [id, track] : impl_->track_index) {
        if (track.duration_seconds >= min_seconds && track.duration_seconds <= max_seconds) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_year_range(uint16_t min_year, uint16_t max_year) const {
    std::vector<TrackId> result;
    for (const auto& [id, track] : impl_->track_index) {
        if (track.year >= min_year && track.year <= max_year) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_rating_range(uint16_t min_rating, uint16_t max_rating) const {
    std::vector<TrackId> result;
    for (const auto& [id, track] : impl_->track_index) {
        if (track.rating >= min_rating && track.rating <= max_rating) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_year(uint16_t year) const {
    return find_tracks_by_year_range(year, year);
}

std::vector<TrackId> Database::find_tracks_by_rating(uint16_t rating) const {
    return find_tracks_by_rating_range(rating, rating);
}

std::vector<ArtistId> Database::find_artists_by_name(std::string_view name) const {
    std::vector<ArtistId> result;
    auto it = impl_->artist_name_index.find(std::string(name));
    if (it != impl_->artist_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<AlbumId> Database::find_albums_by_name(std::string_view name) const {
    std::vector<AlbumId> result;
    auto it = impl_->album_name_index.find(std::string(name));
    if (it != impl_->album_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<AlbumId> Database::find_albums_by_artist(ArtistId artist_id) const {
    std::vector<AlbumId> result;
    auto it = impl_->album_artist_index.find(artist_id);
    if (it != impl_->album_artist_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<GenreId> Database::find_genres_by_name(std::string_view name) const {
    std::vector<GenreId> result;
    auto it = impl_->genre_name_index.find(std::string(name));
    if (it != impl_->genre_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<LabelId> Database::find_labels_by_name(std::string_view name) const {
    std::vector<LabelId> result;
    auto it = impl_->label_name_index.find(std::string(name));
    if (it != impl_->label_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<ColorId> Database::find_colors_by_name(std::string_view name) const {
    std::vector<ColorId> result;
    auto it = impl_->color_name_index.find(std::string(name));
    if (it != impl_->color_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<KeyId> Database::find_keys_by_name(std::string_view name) const {
    std::vector<KeyId> result;
    auto it = impl_->key_name_index.find(std::string(name));
    if (it != impl_->key_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

// ============================================================================
// Playlist Access
// ============================================================================

std::optional<std::vector<TrackId>> Database::get_playlist(PlaylistId id) const {
    auto it = impl_->playlist_index.find(id);
    if (it != impl_->playlist_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::vector<PlaylistFolderEntry>> Database::get_playlist_folder(PlaylistId folder_id) const {
    auto it = impl_->playlist_folder_index.find(folder_id);
    if (it != impl_->playlist_folder_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::vector<TrackId>> Database::get_history_playlist(PlaylistId id) const {
    auto it = impl_->history_playlist_index.find(id);
    if (it != impl_->history_playlist_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<PlaylistId> Database::find_history_playlist_by_name(std::string_view name) const {
    auto it = impl_->history_playlist_name_index.find(std::string(name));
    if (it != impl_->history_playlist_name_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ============================================================================
// Tag Access (exportExt.pdb)
// ============================================================================

std::optional<TagRow> Database::get_tag(TagId id) const {
    auto it = impl_->tag_index.find(id);
    if (it != impl_->tag_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<TagId> Database::find_tags_by_name(std::string_view name) const {
    std::vector<TagId> result;
    auto it = impl_->tag_name_index.find(std::string(name));
    if (it != impl_->tag_name_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<TrackId> Database::find_tracks_by_tag(TagId tag_id) const {
    std::vector<TrackId> result;
    auto it = impl_->tag_track_index.find(tag_id);
    if (it != impl_->tag_track_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<TagId> Database::find_tags_by_track(TrackId track_id) const {
    std::vector<TagId> result;
    auto it = impl_->track_tag_index.find(track_id);
    if (it != impl_->track_tag_index.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

std::vector<TagId> Database::all_tag_ids() const {
    std::vector<TagId> result;
    result.reserve(impl_->tag_index.size());
    for (const auto& [id, _] : impl_->tag_index) {
        result.push_back(id);
    }
    return result;
}

size_t Database::tag_count() const {
    return impl_->tag_index.size();
}

// ============================================================================
// Cue Point Access
// ============================================================================

void Database::load_cue_points(const std::filesystem::path& anlz_dir) {
    impl_->cue_point_manager_.scan_directory(anlz_dir);
}

void Database::load_anlz_file(const std::filesystem::path& path) {
    impl_->cue_point_manager_.load_anlz_file(path);
}

std::vector<CuePoint> Database::get_cue_points(const std::string& track_path) const {
    auto cue_data = impl_->cue_point_manager_.get_cue_points(track_path);
    std::vector<CuePoint> result;
    result.reserve(cue_data.size());

    for (const auto& data : cue_data) {
        CuePoint cue;
        cue.type = data.type;
        cue.time_ms = data.time_ms;
        cue.loop_time_ms = data.loop_time_ms;
        cue.hot_cue_number = static_cast<uint8_t>(data.hot_cue_number);
        cue.color_id = data.color_id;
        cue.comment = data.comment;
        result.push_back(std::move(cue));
    }
    return result;
}

std::vector<CuePoint> Database::get_cue_points_for_track(TrackId id) const {
    auto track = get_track(id);
    if (!track || track->file_path.empty()) {
        return {};
    }
    return get_cue_points(track->file_path);
}

std::vector<CuePoint> Database::find_cue_points_by_filename(const std::string& filename) const {
    auto cue_data = impl_->cue_point_manager_.find_cue_points_by_filename(filename);
    std::vector<CuePoint> result;
    result.reserve(cue_data.size());

    for (const auto& data : cue_data) {
        CuePoint cue;
        cue.type = data.type;
        cue.time_ms = data.time_ms;
        cue.loop_time_ms = data.loop_time_ms;
        cue.hot_cue_number = static_cast<uint8_t>(data.hot_cue_number);
        cue.color_id = data.color_id;
        cue.comment = data.comment;
        result.push_back(std::move(cue));
    }
    return result;
}

size_t Database::cue_point_track_count() const {
    return impl_->cue_point_manager_.track_count();
}

// ============================================================================
// Bulk Access
// ============================================================================

std::vector<TrackId> Database::all_track_ids() const {
    std::vector<TrackId> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, _] : impl_->track_index) {
        result.push_back(id);
    }
    return result;
}

std::vector<ArtistId> Database::all_artist_ids() const {
    std::vector<ArtistId> result;
    result.reserve(impl_->artist_index.size());
    for (const auto& [id, _] : impl_->artist_index) {
        result.push_back(id);
    }
    return result;
}

std::vector<AlbumId> Database::all_album_ids() const {
    std::vector<AlbumId> result;
    result.reserve(impl_->album_index.size());
    for (const auto& [id, _] : impl_->album_index) {
        result.push_back(id);
    }
    return result;
}

std::vector<GenreId> Database::all_genre_ids() const {
    std::vector<GenreId> result;
    result.reserve(impl_->genre_index.size());
    for (const auto& [id, _] : impl_->genre_index) {
        result.push_back(id);
    }
    return result;
}

std::vector<PlaylistId> Database::all_playlist_ids() const {
    std::vector<PlaylistId> result;
    result.reserve(impl_->playlist_index.size());
    for (const auto& [id, _] : impl_->playlist_index) {
        result.push_back(id);
    }
    return result;
}

// ============================================================================
// Bulk Data Extraction (for NumPy/AI)
// ============================================================================

std::vector<float> Database::get_all_bpms() const {
    std::vector<float> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, track] : impl_->track_index) {
        result.push_back(track.bpm_100x / 100.0f);
    }
    return result;
}

std::vector<int32_t> Database::get_all_durations() const {
    std::vector<int32_t> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, track] : impl_->track_index) {
        result.push_back(static_cast<int32_t>(track.duration_seconds));
    }
    return result;
}

std::vector<int32_t> Database::get_all_years() const {
    std::vector<int32_t> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, track] : impl_->track_index) {
        result.push_back(static_cast<int32_t>(track.year));
    }
    return result;
}

std::vector<int32_t> Database::get_all_ratings() const {
    std::vector<int32_t> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, track] : impl_->track_index) {
        result.push_back(static_cast<int32_t>(track.rating));
    }
    return result;
}

std::vector<int32_t> Database::get_all_bitrates() const {
    std::vector<int32_t> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, track] : impl_->track_index) {
        result.push_back(static_cast<int32_t>(track.bitrate));
    }
    return result;
}

std::vector<int32_t> Database::get_all_sample_rates() const {
    std::vector<int32_t> result;
    result.reserve(impl_->track_index.size());
    for (const auto& [id, track] : impl_->track_index) {
        result.push_back(static_cast<int32_t>(track.sample_rate));
    }
    return result;
}

// ============================================================================
// Statistics
// ============================================================================

size_t Database::track_count() const {
    return impl_->track_index.size();
}

size_t Database::artist_count() const {
    return impl_->artist_index.size();
}

size_t Database::album_count() const {
    return impl_->album_index.size();
}

size_t Database::genre_count() const {
    return impl_->genre_index.size();
}

size_t Database::playlist_count() const {
    return impl_->playlist_index.size();
}

const std::filesystem::path& Database::source_file() const {
    return impl_->source_file_;
}

} // namespace cratedigger
