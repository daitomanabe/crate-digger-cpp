/**
 * @file bindings.cpp
 * @brief Python bindings for Crate Digger using nanobind
 *
 * Per INTRODUCTION_JAVA_TO_CPP.md Section 2.2:
 * - MUST: nanobind for Python/NumPy interop
 * - MUST: std::span<float> APIs for zero-copy NumPy arrays
 */

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/filesystem.h>

#include "cratedigger/cratedigger.hpp"

namespace nb = nanobind;
using namespace cratedigger;

NB_MODULE(crate_digger, m) {
    m.doc() = "Crate Digger - Rekordbox database parser for Python/AI integration";

    // ========================================================================
    // Version and API Schema
    // ========================================================================

    m.attr("__version__") = VERSION;
    m.attr("__name__") = NAME;

    m.def("describe_api", []() {
        return to_json(describe_api());
    }, "Get JSON schema of all available commands (for AI agents)");

    // ========================================================================
    // Safety Curtain Functions
    // ========================================================================

    m.def("validate_bpm", &validate_bpm, nb::arg("bpm"),
          "Clamp BPM to valid range (20-300)");
    m.def("validate_duration", &validate_duration, nb::arg("duration_seconds"),
          "Clamp duration to valid range (max 24 hours)");
    m.def("validate_rating", &validate_rating, nb::arg("rating"),
          "Clamp rating to valid range (0-5)");
    m.def("is_valid_bpm", &is_valid_bpm, nb::arg("bpm"),
          "Check if BPM is within valid range (20-300)");
    m.def("is_valid_duration", &is_valid_duration, nb::arg("duration_seconds"),
          "Check if duration is within valid range");
    m.def("is_valid_rating", &is_valid_rating, nb::arg("rating"),
          "Check if rating is within valid range (0-5)");

    // Safety limits as constants
    m.attr("MIN_BPM") = SafetyLimits::MIN_BPM;
    m.attr("MAX_BPM") = SafetyLimits::MAX_BPM;
    m.attr("MAX_DURATION_SECONDS") = SafetyLimits::MAX_DURATION_SECONDS;
    m.attr("MIN_RATING") = SafetyLimits::MIN_RATING;
    m.attr("MAX_RATING") = SafetyLimits::MAX_RATING;

    // ========================================================================
    // ID Types (exposed as integers for simplicity)
    // ========================================================================

    nb::class_<TrackId>(m, "TrackId")
        .def(nb::init<int64_t>())
        .def_rw("value", &TrackId::value)
        .def("__int__", [](const TrackId& id) { return id.value; })
        .def("__repr__", [](const TrackId& id) {
            return "TrackId(" + std::to_string(id.value) + ")";
        });

    nb::class_<ArtistId>(m, "ArtistId")
        .def(nb::init<int64_t>())
        .def_rw("value", &ArtistId::value)
        .def("__int__", [](const ArtistId& id) { return id.value; })
        .def("__repr__", [](const ArtistId& id) {
            return "ArtistId(" + std::to_string(id.value) + ")";
        });

    nb::class_<AlbumId>(m, "AlbumId")
        .def(nb::init<int64_t>())
        .def_rw("value", &AlbumId::value)
        .def("__int__", [](const AlbumId& id) { return id.value; })
        .def("__repr__", [](const AlbumId& id) {
            return "AlbumId(" + std::to_string(id.value) + ")";
        });

    nb::class_<GenreId>(m, "GenreId")
        .def(nb::init<int64_t>())
        .def_rw("value", &GenreId::value)
        .def("__int__", [](const GenreId& id) { return id.value; })
        .def("__repr__", [](const GenreId& id) {
            return "GenreId(" + std::to_string(id.value) + ")";
        });

    nb::class_<PlaylistId>(m, "PlaylistId")
        .def(nb::init<int64_t>())
        .def_rw("value", &PlaylistId::value)
        .def("__int__", [](const PlaylistId& id) { return id.value; })
        .def("__repr__", [](const PlaylistId& id) {
            return "PlaylistId(" + std::to_string(id.value) + ")";
        });

    nb::class_<LabelId>(m, "LabelId")
        .def(nb::init<int64_t>())
        .def_rw("value", &LabelId::value)
        .def("__int__", [](const LabelId& id) { return id.value; })
        .def("__repr__", [](const LabelId& id) {
            return "LabelId(" + std::to_string(id.value) + ")";
        });

    nb::class_<ColorId>(m, "ColorId")
        .def(nb::init<int64_t>())
        .def_rw("value", &ColorId::value)
        .def("__int__", [](const ColorId& id) { return id.value; })
        .def("__repr__", [](const ColorId& id) {
            return "ColorId(" + std::to_string(id.value) + ")";
        });

    nb::class_<KeyId>(m, "KeyId")
        .def(nb::init<int64_t>())
        .def_rw("value", &KeyId::value)
        .def("__int__", [](const KeyId& id) { return id.value; })
        .def("__repr__", [](const KeyId& id) {
            return "KeyId(" + std::to_string(id.value) + ")";
        });

    nb::class_<ArtworkId>(m, "ArtworkId")
        .def(nb::init<int64_t>())
        .def_rw("value", &ArtworkId::value)
        .def("__int__", [](const ArtworkId& id) { return id.value; })
        .def("__repr__", [](const ArtworkId& id) {
            return "ArtworkId(" + std::to_string(id.value) + ")";
        });

    nb::class_<TagId>(m, "TagId")
        .def(nb::init<int64_t>())
        .def_rw("value", &TagId::value)
        .def("__int__", [](const TagId& id) { return id.value; })
        .def("__repr__", [](const TagId& id) {
            return "TagId(" + std::to_string(id.value) + ")";
        });

    // ========================================================================
    // Row Types
    // ========================================================================

    nb::class_<TrackRow>(m, "TrackRow")
        .def_ro("id", &TrackRow::id)
        .def_ro("title", &TrackRow::title)
        .def_ro("artist_id", &TrackRow::artist_id)
        .def_ro("composer_id", &TrackRow::composer_id)
        .def_ro("original_artist_id", &TrackRow::original_artist_id)
        .def_ro("remixer_id", &TrackRow::remixer_id)
        .def_ro("album_id", &TrackRow::album_id)
        .def_ro("genre_id", &TrackRow::genre_id)
        .def_ro("label_id", &TrackRow::label_id)
        .def_ro("key_id", &TrackRow::key_id)
        .def_ro("color_id", &TrackRow::color_id)
        .def_ro("artwork_id", &TrackRow::artwork_id)
        .def_ro("duration_seconds", &TrackRow::duration_seconds)
        .def_ro("bpm_100x", &TrackRow::bpm_100x)
        .def_ro("rating", &TrackRow::rating)
        .def_ro("year", &TrackRow::year)
        .def_ro("file_path", &TrackRow::file_path)
        .def_ro("comment", &TrackRow::comment)
        .def_ro("bitrate", &TrackRow::bitrate)
        .def_ro("sample_rate", &TrackRow::sample_rate)
        // Additional numeric fields
        .def_ro("file_size", &TrackRow::file_size)
        .def_ro("track_number", &TrackRow::track_number)
        .def_ro("disc_number", &TrackRow::disc_number)
        .def_ro("play_count", &TrackRow::play_count)
        .def_ro("sample_depth", &TrackRow::sample_depth)
        // Additional string fields
        .def_ro("isrc", &TrackRow::isrc)
        .def_ro("texter", &TrackRow::texter)
        .def_ro("message", &TrackRow::message)
        .def_ro("kuvo_public", &TrackRow::kuvo_public)
        .def_ro("autoload_hot_cues", &TrackRow::autoload_hot_cues)
        .def_ro("date_added", &TrackRow::date_added)
        .def_ro("release_date", &TrackRow::release_date)
        .def_ro("mix_name", &TrackRow::mix_name)
        .def_ro("analyze_path", &TrackRow::analyze_path)
        .def_ro("analyze_date", &TrackRow::analyze_date)
        .def_ro("filename", &TrackRow::filename)
        // Computed property
        .def_prop_ro("bpm", &TrackRow::bpm)
        .def("__repr__", [](const TrackRow& t) {
            return "TrackRow(id=" + std::to_string(t.id.value) +
                   ", title=\"" + t.title + "\")";
        });

    nb::class_<ArtistRow>(m, "ArtistRow")
        .def_ro("id", &ArtistRow::id)
        .def_ro("name", &ArtistRow::name)
        .def("__repr__", [](const ArtistRow& a) {
            return "ArtistRow(id=" + std::to_string(a.id.value) +
                   ", name=\"" + a.name + "\")";
        });

    nb::class_<AlbumRow>(m, "AlbumRow")
        .def_ro("id", &AlbumRow::id)
        .def_ro("name", &AlbumRow::name)
        .def_ro("artist_id", &AlbumRow::artist_id)
        .def("__repr__", [](const AlbumRow& a) {
            return "AlbumRow(id=" + std::to_string(a.id.value) +
                   ", name=\"" + a.name + "\")";
        });

    nb::class_<GenreRow>(m, "GenreRow")
        .def_ro("id", &GenreRow::id)
        .def_ro("name", &GenreRow::name)
        .def("__repr__", [](const GenreRow& g) {
            return "GenreRow(id=" + std::to_string(g.id.value) +
                   ", name=\"" + g.name + "\")";
        });

    nb::class_<PlaylistFolderEntry>(m, "PlaylistFolderEntry")
        .def_ro("name", &PlaylistFolderEntry::name)
        .def_ro("is_folder", &PlaylistFolderEntry::is_folder)
        .def_ro("id", &PlaylistFolderEntry::id)
        .def("__repr__", [](const PlaylistFolderEntry& e) {
            return "PlaylistFolderEntry(name=\"" + e.name +
                   "\", is_folder=" + (e.is_folder ? "True" : "False") + ")";
        });

    nb::class_<TagRow>(m, "TagRow")
        .def_ro("id", &TagRow::id)
        .def_ro("name", &TagRow::name)
        .def_ro("category_id", &TagRow::category_id)
        .def_ro("category_pos", &TagRow::category_pos)
        .def_ro("is_category", &TagRow::is_category)
        .def("__repr__", [](const TagRow& t) {
            std::string result = t.is_category ? "Category" : "TagRow";
            result += "(id=" + std::to_string(t.id.value) +
                   ", name=\"" + t.name + "\"";
            if (!t.is_category && t.category_id.value != 0) {
                result += ", category=" + std::to_string(t.category_id.value);
            }
            result += ")";
            return result;
        });

    // ========================================================================
    // Cue Point Types
    // ========================================================================

    nb::enum_<CuePointType>(m, "CuePointType")
        .value("Cue", CuePointType::Cue)
        .value("FadeIn", CuePointType::FadeIn)
        .value("FadeOut", CuePointType::FadeOut)
        .value("Load", CuePointType::Load)
        .value("Loop", CuePointType::Loop);

    nb::class_<CuePoint>(m, "CuePoint")
        .def_ro("type", &CuePoint::type)
        .def_ro("time_ms", &CuePoint::time_ms)
        .def_ro("loop_time_ms", &CuePoint::loop_time_ms)
        .def_ro("hot_cue_number", &CuePoint::hot_cue_number)
        .def_ro("color_id", &CuePoint::color_id)
        .def_ro("comment", &CuePoint::comment)
        .def_prop_ro("time_seconds", &CuePoint::time_seconds)
        .def_prop_ro("is_hot_cue", &CuePoint::is_hot_cue)
        .def_prop_ro("is_loop", &CuePoint::is_loop)
        .def_prop_ro("loop_duration_ms", &CuePoint::loop_duration_ms)
        .def("__repr__", [](const CuePoint& c) {
            std::string type_str(cue_point_type_to_string(c.type));
            std::string result = "CuePoint(type=" + type_str +
                   ", time_ms=" + std::to_string(c.time_ms);
            if (c.hot_cue_number > 0) {
                result += ", hot_cue=" + std::to_string(c.hot_cue_number);
            }
            if (c.is_loop()) {
                result += ", loop_end_ms=" + std::to_string(c.loop_time_ms);
            }
            result += ")";
            return result;
        });

    // ========================================================================
    // Beat Grid Types
    // ========================================================================

    nb::class_<BeatEntry>(m, "BeatEntry")
        .def_ro("beat_number", &BeatEntry::beat_number)
        .def_ro("tempo_100x", &BeatEntry::tempo_100x)
        .def_ro("time_ms", &BeatEntry::time_ms)
        .def_prop_ro("bpm", &BeatEntry::bpm)
        .def_prop_ro("time_seconds", &BeatEntry::time_seconds)
        .def("__repr__", [](const BeatEntry& b) {
            return "BeatEntry(beat=" + std::to_string(b.beat_number) +
                   ", bpm=" + std::to_string(b.bpm()) +
                   ", time_ms=" + std::to_string(b.time_ms) + ")";
        });

    nb::class_<BeatGrid>(m, "BeatGrid")
        .def_ro("beats", &BeatGrid::beats)
        .def("empty", &BeatGrid::empty)
        .def("__len__", &BeatGrid::size)
        .def("__getitem__", [](const BeatGrid& bg, size_t idx) -> const BeatEntry& {
            if (idx >= bg.size()) {
                throw nb::index_error("BeatGrid index out of range");
            }
            return bg[idx];
        })
        .def("find_beat_at", &BeatGrid::find_beat_at, nb::arg("time_ms"),
             "Find beat nearest to given time (binary search)")
        .def("get_beats_in_range", &BeatGrid::get_beats_in_range,
             nb::arg("start_ms"), nb::arg("end_ms"),
             "Get beats within time range")
        .def("average_bpm", &BeatGrid::average_bpm,
             "Get average BPM of all beats")
        .def("__repr__", [](const BeatGrid& bg) {
            return "BeatGrid(beats=" + std::to_string(bg.size()) +
                   ", avg_bpm=" + std::to_string(bg.average_bpm()) + ")";
        });

    // ========================================================================
    // Waveform Types
    // ========================================================================

    nb::enum_<WaveformStyle>(m, "WaveformStyle")
        .value("Blue", WaveformStyle::Blue)
        .value("RGB", WaveformStyle::RGB)
        .value("ThreeBand", WaveformStyle::ThreeBand);

    nb::class_<WaveformData>(m, "WaveformData")
        .def_ro("style", &WaveformData::style)
        .def_ro("entry_count", &WaveformData::entry_count)
        .def_ro("bytes_per_entry", &WaveformData::bytes_per_entry)
        .def("empty", &WaveformData::empty)
        .def("__len__", &WaveformData::size)
        .def("raw_size", &WaveformData::raw_size)
        .def("height_at", &WaveformData::height_at, nb::arg("idx"),
             "Get waveform height at position (0-31)")
        .def("color_at", &WaveformData::color_at, nb::arg("idx"),
             "Get color at position for RGB waveforms (returns 0xRRGGBB)")
        .def("bands_at", &WaveformData::bands_at, nb::arg("idx"),
             "Get 3-band values (low, mid, high) for ThreeBand waveforms")
        .def("raw_bytes", [](const WaveformData& w) {
            return nb::bytes(reinterpret_cast<const char*>(w.raw_data()), w.raw_size());
        }, "Get raw waveform data as bytes")
        .def("__repr__", [](const WaveformData& w) {
            std::string style_str(waveform_style_to_string(w.style));
            return "WaveformData(style=" + style_str +
                   ", entries=" + std::to_string(w.entry_count) + ")";
        });

    nb::class_<TrackWaveforms>(m, "TrackWaveforms")
        .def_ro("preview", &TrackWaveforms::preview)
        .def_ro("detail", &TrackWaveforms::detail)
        .def_ro("color_preview", &TrackWaveforms::color_preview)
        .def("has_any", &TrackWaveforms::has_any)
        .def("__repr__", [](const TrackWaveforms& w) {
            std::string parts;
            if (w.preview) parts += "preview";
            if (w.detail) {
                if (!parts.empty()) parts += ",";
                parts += "detail";
            }
            if (w.color_preview) {
                if (!parts.empty()) parts += ",";
                parts += "color_preview";
            }
            if (parts.empty()) parts = "none";
            return "TrackWaveforms(" + parts + ")";
        });

    // ========================================================================
    // Song Structure Types
    // ========================================================================

    nb::enum_<TrackMood>(m, "TrackMood")
        .value("High", TrackMood::High)
        .value("Mid", TrackMood::Mid)
        .value("Low", TrackMood::Low);

    nb::enum_<TrackBank>(m, "TrackBank")
        .value("Default", TrackBank::Default)
        .value("Cool", TrackBank::Cool)
        .value("Natural", TrackBank::Natural)
        .value("Hot", TrackBank::Hot)
        .value("Subtle", TrackBank::Subtle)
        .value("Warm", TrackBank::Warm)
        .value("Vivid", TrackBank::Vivid)
        .value("Club1", TrackBank::Club1)
        .value("Club2", TrackBank::Club2);

    nb::class_<PhraseEntry>(m, "PhraseEntry")
        .def_ro("index", &PhraseEntry::index)
        .def_ro("beat", &PhraseEntry::beat)
        .def_ro("kind", &PhraseEntry::kind)
        .def_ro("end_beat", &PhraseEntry::end_beat)
        .def_ro("k1", &PhraseEntry::k1)
        .def_ro("k2", &PhraseEntry::k2)
        .def_ro("k3", &PhraseEntry::k3)
        .def_ro("has_fill", &PhraseEntry::has_fill)
        .def_ro("fill_beat", &PhraseEntry::fill_beat)
        .def("phrase_name", &PhraseEntry::phrase_name, nb::arg("mood"),
             "Get human-readable phrase name based on mood")
        .def("__repr__", [](const PhraseEntry& p) {
            return "PhraseEntry(index=" + std::to_string(p.index) +
                   ", beat=" + std::to_string(p.beat) +
                   ", kind=" + std::to_string(p.kind) + ")";
        });

    nb::class_<SongStructure>(m, "SongStructure")
        .def_ro("mood", &SongStructure::mood)
        .def_ro("bank", &SongStructure::bank)
        .def_ro("end_beat", &SongStructure::end_beat)
        .def_ro("phrases", &SongStructure::phrases)
        .def("empty", &SongStructure::empty)
        .def("__len__", &SongStructure::size)
        .def("__getitem__", [](const SongStructure& s, size_t idx) -> const PhraseEntry& {
            if (idx >= s.size()) {
                throw nb::index_error("SongStructure index out of range");
            }
            return s[idx];
        })
        .def("find_phrase_at_beat", &SongStructure::find_phrase_at_beat, nb::arg("beat"),
             "Find phrase index at given beat")
        .def("__repr__", [](const SongStructure& s) {
            std::string mood_str(track_mood_to_string(s.mood));
            return "SongStructure(mood=" + mood_str +
                   ", phrases=" + std::to_string(s.size()) + ")";
        });

    // ========================================================================
    // Database Class
    // ========================================================================

    nb::class_<Database>(m, "Database")
        .def_static("open", [](const std::filesystem::path& path) {
            auto result = Database::open(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return std::move(*result);
        }, nb::arg("path"),
           "Open a rekordbox export.pdb database file")

        .def_static("open_ext", [](const std::filesystem::path& path) {
            auto result = Database::open_ext(path);
            if (!result) {
                throw std::runtime_error(result.error().message);
            }
            return std::move(*result);
        }, nb::arg("path"),
           "Open a rekordbox exportExt.pdb database file")

        // Primary index access
        .def("get_track", &Database::get_track, nb::arg("track_id"))
        .def("get_artist", &Database::get_artist, nb::arg("artist_id"))
        .def("get_album", &Database::get_album, nb::arg("album_id"))
        .def("get_genre", &Database::get_genre, nb::arg("genre_id"))

        // Secondary index access
        .def("find_tracks_by_title", &Database::find_tracks_by_title, nb::arg("title"))
        .def("find_tracks_by_artist", &Database::find_tracks_by_artist, nb::arg("artist_id"))
        .def("find_tracks_by_album", &Database::find_tracks_by_album, nb::arg("album_id"))
        .def("find_tracks_by_genre", &Database::find_tracks_by_genre, nb::arg("genre_id"))
        .def("find_artists_by_name", &Database::find_artists_by_name, nb::arg("name"))
        .def("find_albums_by_name", &Database::find_albums_by_name, nb::arg("name"))
        .def("find_genres_by_name", &Database::find_genres_by_name, nb::arg("name"))

        // Range search
        .def("find_tracks_by_bpm_range", &Database::find_tracks_by_bpm_range,
             nb::arg("min_bpm"), nb::arg("max_bpm"),
             "Find tracks within BPM range (inclusive)")
        .def("find_tracks_by_duration_range", &Database::find_tracks_by_duration_range,
             nb::arg("min_seconds"), nb::arg("max_seconds"),
             "Find tracks within duration range in seconds (inclusive)")
        .def("find_tracks_by_year_range", &Database::find_tracks_by_year_range,
             nb::arg("min_year"), nb::arg("max_year"),
             "Find tracks within year range (inclusive)")
        .def("find_tracks_by_rating_range", &Database::find_tracks_by_rating_range,
             nb::arg("min_rating"), nb::arg("max_rating"),
             "Find tracks within rating range (inclusive, 0-5)")
        .def("find_tracks_by_year", &Database::find_tracks_by_year, nb::arg("year"),
             "Find tracks by specific year")
        .def("find_tracks_by_rating", &Database::find_tracks_by_rating, nb::arg("rating"),
             "Find tracks by specific rating (0-5)")

        // Playlist access
        .def("get_playlist", &Database::get_playlist, nb::arg("playlist_id"))
        .def("get_playlist_folder", &Database::get_playlist_folder, nb::arg("folder_id"))
        .def("get_history_playlist", &Database::get_history_playlist, nb::arg("playlist_id"))
        .def("find_history_playlist_by_name", &Database::find_history_playlist_by_name, nb::arg("name"))

        // Tag access (exportExt.pdb)
        .def("get_tag", &Database::get_tag, nb::arg("tag_id"))
        .def("find_tags_by_name", &Database::find_tags_by_name, nb::arg("name"))
        .def("find_tracks_by_tag", &Database::find_tracks_by_tag, nb::arg("tag_id"))
        .def("find_tags_by_track", &Database::find_tags_by_track, nb::arg("track_id"))
        .def("all_tag_ids", &Database::all_tag_ids)
        .def_prop_ro("tag_count", &Database::tag_count)

        // Tag category access (exportExt.pdb)
        .def("get_category", &Database::get_category, nb::arg("category_id"),
             "Get category by ID")
        .def("find_categories_by_name", &Database::find_categories_by_name, nb::arg("name"),
             "Find categories by name")
        .def("category_order", &Database::category_order, nb::rv_policy::reference,
             "Get all category IDs in display order")
        .def("get_tags_in_category", &Database::get_tags_in_category, nb::arg("category_id"),
             "Get tags in a category, in display order")
        .def("all_category_ids", &Database::all_category_ids)
        .def_prop_ro("category_count", &Database::category_count)

        // Cue point access (ANLZ files)
        .def("load_cue_points", &Database::load_cue_points, nb::arg("anlz_dir"),
             "Load cue points from an ANLZ directory")
        .def("load_anlz_file", &Database::load_anlz_file, nb::arg("path"),
             "Load cue points from a single ANLZ file")
        .def("get_cue_points", &Database::get_cue_points, nb::arg("track_path"),
             "Get cue points for a track by its file path")
        .def("get_cue_points_for_track", &Database::get_cue_points_for_track, nb::arg("track_id"),
             "Get cue points for a track by ID")
        .def("find_cue_points_by_filename", &Database::find_cue_points_by_filename, nb::arg("filename"),
             "Find cue points by filename pattern")
        .def_prop_ro("cue_point_track_count", &Database::cue_point_track_count)

        // Beat grid access (ANLZ files)
        .def("get_beat_grid", &Database::get_beat_grid, nb::arg("track_path"),
             nb::rv_policy::reference,
             "Get beat grid for a track by its file path")
        .def("get_beat_grid_for_track", &Database::get_beat_grid_for_track, nb::arg("track_id"),
             nb::rv_policy::reference,
             "Get beat grid for a track by ID")
        .def("find_beat_grid_by_filename", &Database::find_beat_grid_by_filename, nb::arg("filename"),
             nb::rv_policy::reference,
             "Find beat grid by filename pattern")
        .def_prop_ro("beat_grid_track_count", &Database::beat_grid_track_count)

        // Waveform access (ANLZ files)
        .def("get_waveforms", &Database::get_waveforms, nb::arg("track_path"),
             nb::rv_policy::reference,
             "Get waveforms for a track by its file path")
        .def("get_waveforms_for_track", &Database::get_waveforms_for_track, nb::arg("track_id"),
             nb::rv_policy::reference,
             "Get waveforms for a track by ID")
        .def("find_waveforms_by_filename", &Database::find_waveforms_by_filename, nb::arg("filename"),
             nb::rv_policy::reference,
             "Find waveforms by filename pattern")
        .def_prop_ro("waveform_track_count", &Database::waveform_track_count)

        // Song structure access (ANLZ files)
        .def("get_song_structure", &Database::get_song_structure, nb::arg("track_path"),
             nb::rv_policy::reference,
             "Get song structure for a track by its file path")
        .def("get_song_structure_for_track", &Database::get_song_structure_for_track, nb::arg("track_id"),
             nb::rv_policy::reference,
             "Get song structure for a track by ID")
        .def("find_song_structure_by_filename", &Database::find_song_structure_by_filename, nb::arg("filename"),
             nb::rv_policy::reference,
             "Find song structure by filename pattern")
        .def_prop_ro("song_structure_track_count", &Database::song_structure_track_count)

        // Bulk access (for NumPy/AI)
        .def("all_track_ids", &Database::all_track_ids)
        .def("all_artist_ids", &Database::all_artist_ids)
        .def("all_album_ids", &Database::all_album_ids)
        .def("all_genre_ids", &Database::all_genre_ids)
        .def("all_playlist_ids", &Database::all_playlist_ids)

        // Statistics
        .def_prop_ro("track_count", &Database::track_count)
        .def_prop_ro("artist_count", &Database::artist_count)
        .def_prop_ro("album_count", &Database::album_count)
        .def_prop_ro("genre_count", &Database::genre_count)
        .def_prop_ro("playlist_count", &Database::playlist_count)
        .def_prop_ro("source_file", &Database::source_file)

        // Bulk data extraction for NumPy (returns lists that can be converted)
        .def("get_all_bpms", &Database::get_all_bpms,
             "Get all track BPMs as a list (for numpy.array)")
        .def("get_all_durations", &Database::get_all_durations,
             "Get all track durations in seconds as a list (for numpy.array)")
        .def("get_all_years", &Database::get_all_years,
             "Get all track years as a list (for numpy.array)")
        .def("get_all_ratings", &Database::get_all_ratings,
             "Get all track ratings as a list (for numpy.array)")
        .def("get_all_bitrates", &Database::get_all_bitrates,
             "Get all track bitrates as a list (for numpy.array)")
        .def("get_all_sample_rates", &Database::get_all_sample_rates,
             "Get all track sample rates as a list (for numpy.array)")

        .def("__repr__", [](const Database& db) {
            return "Database(tracks=" + std::to_string(db.track_count()) +
                   ", artists=" + std::to_string(db.artist_count()) +
                   ", albums=" + std::to_string(db.album_count()) + ")";
        });
}
