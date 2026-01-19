#pragma once
/**
 * @file database_impl.hpp
 * @brief Internal database implementation header
 *
 * This header is internal to the library and should not be exposed to users.
 */

#include "cratedigger/database.hpp"
#include "cratedigger/rekordbox_pdb.hpp"
#include "cratedigger/rekordbox_anlz.hpp"
#include "cratedigger/logging.hpp"

namespace cratedigger {

/**
 * @brief Internal implementation class for Database
 *
 * Contains all indices and parsing logic.
 */
class DatabaseImpl {
public:
    explicit DatabaseImpl(RekordboxPdb&& pdb, const std::filesystem::path& path)
        : pdb_(std::move(pdb))
        , source_file_(path)
    {}

    void build_indices();

    // Primary indices
    PrimaryIndex<TrackId, TrackRow> track_index;
    PrimaryIndex<ArtistId, ArtistRow> artist_index;
    PrimaryIndex<AlbumId, AlbumRow> album_index;
    PrimaryIndex<GenreId, GenreRow> genre_index;
    PrimaryIndex<LabelId, LabelRow> label_index;
    PrimaryIndex<ColorId, ColorRow> color_index;
    PrimaryIndex<KeyId, KeyRow> key_index;
    PrimaryIndex<ArtworkId, ArtworkRow> artwork_index;

    // Secondary indices
    NameIndex<TrackId> track_title_index;
    SecondaryIndex<ArtistId, TrackId> track_artist_index;
    SecondaryIndex<AlbumId, TrackId> track_album_index;
    SecondaryIndex<GenreId, TrackId> track_genre_index;

    NameIndex<ArtistId> artist_name_index;
    NameIndex<AlbumId> album_name_index;
    SecondaryIndex<ArtistId, AlbumId> album_artist_index;
    NameIndex<GenreId> genre_name_index;
    NameIndex<LabelId> label_name_index;
    NameIndex<ColorId> color_name_index;
    NameIndex<KeyId> key_name_index;

    // Playlist indices
    PlaylistIndex playlist_index;
    PlaylistFolderIndex playlist_folder_index;
    PlaylistIndex history_playlist_index;
    std::map<std::string, PlaylistId, CaseInsensitiveCompare> history_playlist_name_index;

    // Tag indices (exportExt.pdb)
    PrimaryIndex<TagId, TagRow> tag_index;
    NameIndex<TagId> tag_name_index;
    SecondaryIndex<TagId, TrackId> tag_track_index;      // Tag -> Tracks
    SecondaryIndex<TrackId, TagId> track_tag_index;      // Track -> Tags

    RekordboxPdb pdb_;
    std::filesystem::path source_file_;

    // Cue point manager (loaded separately from ANLZ files)
    CuePointManager cue_point_manager_;

private:
    void index_tracks();
    void index_artists();
    void index_albums();
    void index_genres();
    void index_labels();
    void index_colors();
    void index_keys();
    void index_artwork();
    void index_playlists();
    void index_playlist_folders();
    void index_history_playlists();
    void index_history_entries();
    void index_tags();
    void index_tag_tracks();

    template<typename RowHandler>
    void scan_table(PageType type, RowHandler handler);

    std::string read_string_at_row(size_t row_base, uint16_t offset) const;
};

} // namespace cratedigger
