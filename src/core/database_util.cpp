#include "database_impl.hpp"
#include <algorithm>
#include <cctype>

namespace cratedigger {

// ============================================================================
// Case-Insensitive String Comparison
// ============================================================================

bool CaseInsensitiveCompare::operator()(const std::string& a, const std::string& b) const {
    return std::lexicographical_compare(
        a.begin(), a.end(),
        b.begin(), b.end(),
        [](unsigned char ca, unsigned char cb) {
            return std::tolower(ca) < std::tolower(cb);
        }
    );
}

// ============================================================================
// Table Scanning
// ============================================================================

template<typename RowHandler>
void DatabaseImpl::scan_table(PageType type, RowHandler handler) {
    // Find the table with matching type
    for (const auto& table : pdb_.tables()) {
        if (table.type != type) continue;

        uint32_t current_page_idx = table.first_page_index;
        uint32_t last_page_idx = table.last_page_index;
        bool more_pages = true;

        while (more_pages) {
            auto page_result = pdb_.read_page(current_page_idx);
            if (!page_result) {
                LOG_ERROR("Failed to read page " + std::to_string(current_page_idx));
                break;
            }

            const auto& page = *page_result;

            if (page.is_data_page) {
                for (const auto& row_group : page.row_groups) {
                    for (size_t row_idx = 0; row_idx < row_group.row_offsets.size(); ++row_idx) {
                        // Check if row is present
                        bool present = ((row_group.row_present_flags >> row_idx) & 1) != 0;
                        if (!present) continue;

                        uint16_t row_offset = row_group.row_offsets[row_idx];
                        size_t row_base = row_group.heap_pos + row_offset;

                        handler(row_base);
                    }
                }
            }

            if (current_page_idx == last_page_idx) {
                more_pages = false;
            } else {
                current_page_idx = page.next_page_index;
            }
        }

        return;  // Found and processed the table
    }

    LOG_WARN("Table type " + std::to_string(static_cast<int>(type)) + " not found");
}

template<typename RowHandler>
void scan_table_ext(RekordboxPdb& pdb, PageTypeExt type, RowHandler handler) {
    for (const auto& table : pdb.tables()) {
        if (table.type_ext != type) continue;

        uint32_t current_page_idx = table.first_page_index;
        uint32_t last_page_idx = table.last_page_index;
        bool more_pages = true;

        while (more_pages) {
            auto page_result = pdb.read_page(current_page_idx);
            if (!page_result) {
                LOG_ERROR("Failed to read page " + std::to_string(current_page_idx));
                break;
            }

            const auto& page = *page_result;

            if (page.is_data_page) {
                for (const auto& row_group : page.row_groups) {
                    for (size_t row_idx = 0; row_idx < row_group.row_offsets.size(); ++row_idx) {
                        bool present = ((row_group.row_present_flags >> row_idx) & 1) != 0;
                        if (!present) continue;

                        uint16_t row_offset = row_group.row_offsets[row_idx];
                        size_t row_base = row_group.heap_pos + row_offset;

                        handler(row_base);
                    }
                }
            }

            if (current_page_idx == last_page_idx) {
                more_pages = false;
            } else {
                current_page_idx = page.next_page_index;
            }
        }

        return;
    }

    LOG_WARN("Table type " + std::to_string(static_cast<int>(type)) + " not found");
}

std::string DatabaseImpl::read_string_at_row(size_t row_base, uint16_t offset) const {
    return pdb_.read_string(row_base + offset);
}

// ============================================================================
// Index Building
// ============================================================================

void DatabaseImpl::build_indices() {
    if (pdb_.is_ext()) {
        // exportExt.pdb tables
        index_tags();
        index_tag_tracks();
    } else {
        // export.pdb tables
        index_tracks();
        index_artists();
        index_albums();
        index_genres();
        index_labels();
        index_colors();
        index_keys();
        index_artwork();
        index_playlists();
        index_playlist_folders();
        index_history_playlists();
        index_history_entries();
    }
}

void DatabaseImpl::index_tracks() {
    scan_table(PageType::Tracks, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawTrackRow));
        if (data.second < sizeof(RawTrackRow)) return;

        const auto* raw = reinterpret_cast<const RawTrackRow*>(data.first);

        TrackRow row;
        row.id = TrackId{static_cast<int64_t>(raw->id)};
        row.artist_id = ArtistId{static_cast<int64_t>(raw->artist_id)};
        row.composer_id = ArtistId{static_cast<int64_t>(raw->composer_id)};
        row.original_artist_id = ArtistId{static_cast<int64_t>(raw->original_artist_id)};
        row.remixer_id = ArtistId{static_cast<int64_t>(raw->remixer_id)};
        row.album_id = AlbumId{static_cast<int64_t>(raw->album_id)};
        row.genre_id = GenreId{static_cast<int64_t>(raw->genre_id)};
        row.label_id = LabelId{static_cast<int64_t>(raw->label_id)};
        row.key_id = KeyId{static_cast<int64_t>(raw->key_id)};
        row.color_id = ColorId{static_cast<int64_t>(raw->color_id)};
        row.artwork_id = ArtworkId{static_cast<int64_t>(raw->artwork_id)};
        row.duration_seconds = raw->duration;
        row.bpm_100x = raw->tempo;
        row.rating = raw->rating;
        row.bitrate = raw->bitrate;
        row.sample_rate = raw->sample_rate;
        row.year = raw->year;
        // Additional numeric fields
        row.file_size = raw->file_size;
        row.track_number = raw->track_number;
        row.disc_number = raw->disc_number;
        row.play_count = raw->play_count;
        row.sample_depth = raw->sample_depth;

        // Read strings (per IR_SCHEMA.md string offset indices)
        row.isrc = read_string_at_row(row_base, raw->ofs_strings[0]);
        row.texter = read_string_at_row(row_base, raw->ofs_strings[1]);
        row.message = read_string_at_row(row_base, raw->ofs_strings[5]);
        row.kuvo_public = read_string_at_row(row_base, raw->ofs_strings[6]);
        row.autoload_hot_cues = read_string_at_row(row_base, raw->ofs_strings[7]);
        row.date_added = read_string_at_row(row_base, raw->ofs_strings[10]);
        row.release_date = read_string_at_row(row_base, raw->ofs_strings[11]);
        row.mix_name = read_string_at_row(row_base, raw->ofs_strings[12]);
        row.analyze_path = read_string_at_row(row_base, raw->ofs_strings[14]);
        row.analyze_date = read_string_at_row(row_base, raw->ofs_strings[15]);
        row.comment = read_string_at_row(row_base, raw->ofs_strings[16]);
        row.title = read_string_at_row(row_base, raw->ofs_strings[17]);
        row.filename = read_string_at_row(row_base, raw->ofs_strings[19]);
        row.file_path = read_string_at_row(row_base, raw->ofs_strings[20]);

        // Add to primary index
        track_index[row.id] = row;

        // Add to secondary indices
        if (!row.title.empty()) {
            track_title_index[row.title].insert(row.id);
        }
        if (row.artist_id.value > 0) {
            track_artist_index[row.artist_id].insert(row.id);
        }
        if (row.composer_id.value > 0) {
            track_artist_index[row.composer_id].insert(row.id);
        }
        if (row.original_artist_id.value > 0) {
            track_artist_index[row.original_artist_id].insert(row.id);
        }
        if (row.remixer_id.value > 0) {
            track_artist_index[row.remixer_id].insert(row.id);
        }
        if (row.album_id.value > 0) {
            track_album_index[row.album_id].insert(row.id);
        }
        if (row.genre_id.value > 0) {
            track_genre_index[row.genre_id].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(track_index.size()) + " tracks");
}

void DatabaseImpl::index_artists() {
    scan_table(PageType::Artists, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawArtistRow));
        if (data.second < sizeof(RawArtistRow)) return;

        const auto* raw = reinterpret_cast<const RawArtistRow*>(data.first);

        ArtistRow row;
        row.id = ArtistId{static_cast<int64_t>(raw->id)};

        // Determine name offset (near or far)
        uint16_t name_offset = raw->ofs_name_near;
        if ((raw->subtype & 0x04) == 0x04) {
            auto far_data = pdb_.data_at(row_base + 0x0a, 2);
            if (far_data.second >= 2) {
                name_offset = static_cast<uint16_t>(far_data.first[0]) |
                              (static_cast<uint16_t>(far_data.first[1]) << 8);
            }
        }

        row.name = read_string_at_row(row_base, name_offset);

        artist_index[row.id] = row;

        if (!row.name.empty()) {
            artist_name_index[row.name].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(artist_index.size()) + " artists");
}

void DatabaseImpl::index_albums() {
    scan_table(PageType::Albums, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawAlbumRow));
        if (data.second < sizeof(RawAlbumRow)) return;

        const auto* raw = reinterpret_cast<const RawAlbumRow*>(data.first);

        AlbumRow row;
        row.id = AlbumId{static_cast<int64_t>(raw->id)};
        row.artist_id = ArtistId{static_cast<int64_t>(raw->artist_id)};

        // Determine name offset
        uint16_t name_offset = raw->ofs_name_near;
        if ((raw->subtype & 0x04) == 0x04) {
            auto far_data = pdb_.data_at(row_base + 0x16, 2);
            if (far_data.second >= 2) {
                name_offset = static_cast<uint16_t>(far_data.first[0]) |
                              (static_cast<uint16_t>(far_data.first[1]) << 8);
            }
        }

        row.name = read_string_at_row(row_base, name_offset);

        album_index[row.id] = row;

        if (!row.name.empty()) {
            album_name_index[row.name].insert(row.id);
        }
        if (row.artist_id.value > 0) {
            album_artist_index[row.artist_id].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(album_index.size()) + " albums");
}

void DatabaseImpl::index_genres() {
    scan_table(PageType::Genres, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawGenreRow));
        if (data.second < sizeof(RawGenreRow)) return;

        const auto* raw = reinterpret_cast<const RawGenreRow*>(data.first);

        GenreRow row;
        row.id = GenreId{static_cast<int64_t>(raw->id)};
        row.name = pdb_.read_string(row_base + sizeof(RawGenreRow));

        genre_index[row.id] = row;

        if (!row.name.empty()) {
            genre_name_index[row.name].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(genre_index.size()) + " genres");
}

void DatabaseImpl::index_labels() {
    scan_table(PageType::Labels, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawLabelRow));
        if (data.second < sizeof(RawLabelRow)) return;

        const auto* raw = reinterpret_cast<const RawLabelRow*>(data.first);

        LabelRow row;
        row.id = LabelId{static_cast<int64_t>(raw->id)};
        row.name = pdb_.read_string(row_base + sizeof(RawLabelRow));

        label_index[row.id] = row;

        if (!row.name.empty()) {
            label_name_index[row.name].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(label_index.size()) + " labels");
}

void DatabaseImpl::index_colors() {
    scan_table(PageType::Colors, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawColorRow));
        if (data.second < sizeof(RawColorRow)) return;

        const auto* raw = reinterpret_cast<const RawColorRow*>(data.first);

        ColorRow row;
        row.id = ColorId{static_cast<int64_t>(raw->id)};
        row.name = pdb_.read_string(row_base + sizeof(RawColorRow));

        color_index[row.id] = row;

        if (!row.name.empty()) {
            color_name_index[row.name].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(color_index.size()) + " colors");
}

void DatabaseImpl::index_keys() {
    scan_table(PageType::Keys, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawKeyRow));
        if (data.second < sizeof(RawKeyRow)) return;

        const auto* raw = reinterpret_cast<const RawKeyRow*>(data.first);

        KeyRow row;
        row.id = KeyId{static_cast<int64_t>(raw->id)};
        row.name = pdb_.read_string(row_base + sizeof(RawKeyRow));

        key_index[row.id] = row;

        if (!row.name.empty()) {
            key_name_index[row.name].insert(row.id);
        }
    });

    LOG_INFO("Indexed " + std::to_string(key_index.size()) + " musical keys");
}

void DatabaseImpl::index_artwork() {
    scan_table(PageType::Artwork, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawArtworkRow));
        if (data.second < sizeof(RawArtworkRow)) return;

        const auto* raw = reinterpret_cast<const RawArtworkRow*>(data.first);

        ArtworkRow row;
        row.id = ArtworkId{static_cast<int64_t>(raw->id)};
        row.path = pdb_.read_string(row_base + sizeof(RawArtworkRow));

        artwork_index[row.id] = row;
    });

    LOG_INFO("Indexed " + std::to_string(artwork_index.size()) + " artwork paths");
}

void DatabaseImpl::index_playlists() {
    scan_table(PageType::PlaylistEntries, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawPlaylistEntryRow));
        if (data.second < sizeof(RawPlaylistEntryRow)) return;

        const auto* raw = reinterpret_cast<const RawPlaylistEntryRow*>(data.first);

        PlaylistId playlist_id{static_cast<int64_t>(raw->playlist_id)};
        TrackId track_id{static_cast<int64_t>(raw->track_id)};
        uint32_t entry_index = raw->entry_index;

        auto& playlist = playlist_index[playlist_id];
        if (playlist.size() <= entry_index) {
            playlist.resize(entry_index + 1);
        }
        playlist[entry_index] = track_id;
    });

    LOG_INFO("Indexed " + std::to_string(playlist_index.size()) + " playlists");
}

void DatabaseImpl::index_playlist_folders() {
    scan_table(PageType::PlaylistTree, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawPlaylistTreeRow));
        if (data.second < sizeof(RawPlaylistTreeRow)) return;

        const auto* raw = reinterpret_cast<const RawPlaylistTreeRow*>(data.first);

        PlaylistFolderEntry entry;
        entry.id = PlaylistId{static_cast<int64_t>(raw->id)};
        entry.is_folder = raw->raw_is_folder != 0;
        entry.name = pdb_.read_string(row_base + sizeof(RawPlaylistTreeRow));

        PlaylistId parent_id{static_cast<int64_t>(raw->parent_id)};
        uint32_t sort_order = raw->sort_order;

        auto& folder = playlist_folder_index[parent_id];
        if (folder.size() <= sort_order) {
            folder.resize(sort_order + 1);
        }
        folder[sort_order] = entry;
    });

    LOG_INFO("Indexed " + std::to_string(playlist_folder_index.size()) + " playlist folders");
}

void DatabaseImpl::index_history_playlists() {
    scan_table(PageType::HistoryPlaylists, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawHistoryPlaylistRow));
        if (data.second < sizeof(RawHistoryPlaylistRow)) return;

        const auto* raw = reinterpret_cast<const RawHistoryPlaylistRow*>(data.first);

        PlaylistId id{static_cast<int64_t>(raw->id)};
        std::string name = pdb_.read_string(row_base + sizeof(RawHistoryPlaylistRow));

        history_playlist_name_index[name] = id;
    });

    LOG_INFO("Indexed " + std::to_string(history_playlist_name_index.size()) + " history playlist names");
}

void DatabaseImpl::index_history_entries() {
    scan_table(PageType::HistoryEntries, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawHistoryEntryRow));
        if (data.second < sizeof(RawHistoryEntryRow)) return;

        const auto* raw = reinterpret_cast<const RawHistoryEntryRow*>(data.first);

        PlaylistId playlist_id{static_cast<int64_t>(raw->playlist_id)};
        TrackId track_id{static_cast<int64_t>(raw->track_id)};
        uint32_t entry_index = raw->entry_index;

        auto& playlist = history_playlist_index[playlist_id];
        if (playlist.size() <= entry_index) {
            playlist.resize(entry_index + 1);
        }
        playlist[entry_index] = track_id;
    });

    LOG_INFO("Indexed " + std::to_string(history_playlist_index.size()) + " history playlists");
}

void DatabaseImpl::index_tags() {
    // Temporary storage for ordering
    std::vector<std::pair<uint32_t, TagId>> category_positions;  // (pos, id)
    std::map<TagId, std::vector<std::pair<uint32_t, TagId>>> tag_positions;  // category -> [(pos, tag)]

    scan_table_ext(pdb_, PageTypeExt::Tags, [this, &category_positions, &tag_positions](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawTagRow));
        if (data.second < sizeof(RawTagRow)) return;

        const auto* raw = reinterpret_cast<const RawTagRow*>(data.first);

        TagRow row;
        row.id = TagId{static_cast<int64_t>(raw->id)};
        row.category_id = TagId{static_cast<int64_t>(raw->category)};
        row.category_pos = raw->category_pos;
        row.is_category = raw->raw_is_category != 0;

        // Read name string using offset
        // Name is at row_base + ofs_name_near for near offsets, or further for far
        size_t name_offset = row_base + raw->ofs_name_near;
        if (raw->subtype == 0x0684) {
            // Long name offset - read additional offset
            auto far_offset_data = pdb_.data_at(row_base + raw->ofs_name_near, 4);
            if (far_offset_data.second >= 4) {
                uint32_t far_offset = *reinterpret_cast<const uint32_t*>(far_offset_data.first);
                name_offset = row_base + far_offset;
            }
        }
        row.name = pdb_.read_string(name_offset);

        if (row.is_category) {
            // This is a category
            category_index[row.id] = row;
            if (!row.name.empty()) {
                category_name_index[row.name].insert(row.id);
            }
            category_positions.emplace_back(row.category_pos, row.id);
        } else {
            // This is a tag
            tag_index[row.id] = row;
            if (!row.name.empty()) {
                tag_name_index[row.name].insert(row.id);
            }
            // Group by category for ordering
            tag_positions[row.category_id].emplace_back(row.category_pos, row.id);
        }
    });

    // Sort categories by position and build category_order
    std::sort(category_positions.begin(), category_positions.end());
    for (const auto& [pos, id] : category_positions) {
        category_order.push_back(id);
    }

    // Sort tags within each category and build category_tags
    for (auto& [cat_id, tags] : tag_positions) {
        std::sort(tags.begin(), tags.end());
        for (const auto& [pos, tag_id] : tags) {
            category_tags[cat_id].push_back(tag_id);
        }
    }

    LOG_INFO("Indexed " + std::to_string(tag_index.size()) + " tags, " + std::to_string(category_index.size()) + " categories");
}

void DatabaseImpl::index_tag_tracks() {
    scan_table_ext(pdb_, PageTypeExt::TagTracks, [this](size_t row_base) {
        auto data = pdb_.data_at(row_base, sizeof(RawTagTrackRow));
        if (data.second < sizeof(RawTagTrackRow)) return;

        const auto* raw = reinterpret_cast<const RawTagTrackRow*>(data.first);

        TagId tag_id{static_cast<int64_t>(raw->tag_id)};
        TrackId track_id{static_cast<int64_t>(raw->track_id)};

        tag_track_index[tag_id].insert(track_id);
        track_tag_index[track_id].insert(tag_id);
    });

    LOG_INFO("Indexed tag-track associations");
}

} // namespace cratedigger
