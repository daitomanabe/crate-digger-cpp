# Crate Digger C++ (C++17)

A C++17 library for parsing rekordbox database exports (export.pdb and exportExt.pdb files) and analysis files (ANLZ).

This is a **complete C++17 port** of the original [Crate Digger](https://github.com/Deep-Symmetry/crate-digger) Java library by [Deep Symmetry](https://github.com/Deep-Symmetry), developed by [Daito Manabe](https://github.com/daitomanabe) 

## Features

### Database Parsing (export.pdb / exportExt.pdb)
- Parse rekordbox `export.pdb` database files
- Parse rekordbox `exportExt.pdb` files (Tags and Categories support)
- Access tracks, artists, albums, genres, colors, labels, keys, artwork, and playlists
- Tag hierarchy with categories (rekordbox 6.x+)
- Primary and secondary index lookups
- Range search (BPM, duration, year, rating)

### ANLZ File Parsing
- **Cue Points**: Memory cues and Hot Cues with colors and comments
- **Beat Grid**: Beat positions with tempo information for sync
- **Waveforms**: Preview, scroll, color preview, color scroll, and 3-band waveforms
- **Song Structure**: Phrase analysis with mood and bank information

### Integration
- Bulk data retrieval for NumPy integration
- Handle Pattern design (strongly-typed integer IDs)
- Safety validation functions for data integrity
- Self-describing API via `describe_api()` for AI agent integration
- Python bindings via nanobind
- JSONL structured logging

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+
- Python 3.8+ (optional, for Python bindings)
- nanobind (optional, for Python bindings)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

### Build Options

- `BUILD_PYTHON_BINDINGS=ON` - Build Python module (requires nanobind)
- `BUILD_TESTS=ON` - Build unit tests (default: ON)

## Usage

### C++ API

```cpp
#include <cratedigger/database.hpp>

// Open a rekordbox database
auto result = cratedigger::Database::open("path/to/export.pdb");
if (!result.has_value()) {
    std::cerr << "Error: " << result.error().message << std::endl;
    return 1;
}

auto& db = result.value();

// Look up a track by ID
if (auto track = db.get_track(TrackId{123})) {
    std::cout << "Title: " << track->title << std::endl;
    std::cout << "Artist ID: " << track->artist_id.value << std::endl;
}

// Look up artist by track's artist_id
if (auto artist = db.get_artist(track->artist_id)) {
    std::cout << "Artist: " << artist->name << std::endl;
}

// Iterate all tracks
for (const auto& id : db.all_track_ids()) {
    if (auto track = db.get_track(id)) {
        std::cout << track->title << std::endl;
    }
}

// Range search
auto fast_tracks = db.find_tracks_by_bpm_range(140.0f, 180.0f);

// Bulk data for NumPy
auto all_bpms = db.get_all_bpms();  // Returns vector of {id, bpm}

// Load ANLZ data (cue points, beat grids, waveforms, song structure)
db.load_cue_points("path/to/PIONEER/USBANLZ");

// Cue Points
auto cues = db.get_cue_points_for_track(TrackId{123});

// Beat Grid
if (auto* grid = db.get_beat_grid_for_track(TrackId{123})) {
    for (const auto& beat : grid->beats) {
        std::cout << "Beat at " << beat.time_ms << "ms, BPM: " << beat.tempo / 100.0f << std::endl;
    }
}

// Waveforms
if (auto* waveforms = db.get_waveforms_for_track(TrackId{123})) {
    if (waveforms->has_color_scroll()) {
        // Access color scroll waveform data
    }
}

// Song Structure (phrase analysis)
if (auto* structure = db.get_song_structure_for_track(TrackId{123})) {
    for (const auto& phrase : structure->entries) {
        std::cout << "Phrase at beat " << phrase.beat << std::endl;
    }
}
```

### CLI Tool

```bash
# Show API schema (JSON)
./crate-digger --schema

# Process JSONL commands from stdin
echo '{"command":"open","path":"export.pdb"}' | ./crate-digger
echo '{"command":"get_track","id":123}' | ./crate-digger
```

### Python API

```python
import cratedigger

# Open database
db = cratedigger.Database.open("path/to/export.pdb")

# Get track by ID
track = db.get_track(123)
print(f"Title: {track.title}")

# Get all tracks
for track_id in db.all_track_ids():
    track = db.get_track(track_id)
    print(track.title)

# Tags (from exportExt.pdb)
db_ext = cratedigger.Database.open_ext("path/to/exportExt.pdb")
for tag_id in db_ext.all_tag_ids():
    tag = db_ext.get_tag(tag_id)
    print(f"Tag: {tag.name}")

# Load ANLZ data
db.load_cue_points("path/to/PIONEER/USBANLZ")

# Cue Points
cues = db.get_cue_points_for_track(track_id)
for cue in cues:
    print(f"Cue at {cue.time_seconds()}s - {cue.comment}")

# Beat Grid
grid = db.get_beat_grid_for_track(track_id)
if grid:
    for beat in grid.beats:
        print(f"Beat at {beat.time_ms}ms, BPM: {beat.tempo / 100}")

# Waveforms
waveforms = db.get_waveforms_for_track(track_id)
if waveforms and waveforms.has_color_scroll():
    data = waveforms.color_scroll
    print(f"Waveform with {len(data.data)} entries")

# Song Structure
structure = db.get_song_structure_for_track(track_id)
if structure:
    for phrase in structure.entries:
        print(f"Phrase: {phrase.mood.name} at beat {phrase.beat}")

# Tag Categories (exportExt.pdb)
for cat_id in db_ext.all_category_ids():
    cat = db_ext.get_category(cat_id)
    print(f"Category: {cat.name}")
    for tag_id in db_ext.get_tags_in_category(cat_id):
        tag = db_ext.get_tag(tag_id)
        print(f"  Tag: {tag.name}")

# Safety validation
validated_bpm = cratedigger.validate_bpm(999.0)  # Returns 300.0 (MAX_BPM)
```

## API Schema

The library provides a self-describing API for AI agent integration:

```cpp
#include <cratedigger/api_schema.hpp>

std::string schema = cratedigger::describe_api();
// Returns JSON schema describing all types and functions
```

## Data Types

### Handle Types (Strongly-Typed Integer IDs)

- `TrackId` - Track identifier
- `ArtistId` - Artist identifier
- `AlbumId` - Album identifier
- `GenreId` - Genre identifier
- `ColorId` - Color identifier
- `LabelId` - Label identifier
- `KeyId` - Musical key identifier
- `ArtworkId` - Artwork identifier
- `PlaylistId` - Playlist identifier
- `TagId` - Tag identifier (exportExt.pdb)

### Row Types

- `TrackRow` - Track metadata (title, artist_id, album_id, duration, tempo, etc.)
- `ArtistRow` - Artist name
- `AlbumRow` - Album name and artist
- `GenreRow` - Genre name
- `ColorRow` - Color name
- `LabelRow` - Label name
- `KeyRow` - Musical key name
- `ArtworkRow` - Artwork path
- `TagRow` - Tag name with category hierarchy (exportExt.pdb)
- `CuePoint` - Cue point/hot cue information

### ANLZ Types

- `BeatEntry` - Single beat position with tempo
- `BeatGrid` - Collection of beat entries for a track
- `WaveformData` - Waveform samples with style information
- `TrackWaveforms` - All waveform types for a track (preview, scroll, color, 3-band)
- `PhraseEntry` - Song structure phrase with mood and bank
- `SongStructure` - Phrase analysis for a track

### Safety Limits

```cpp
SafetyLimits::MIN_BPM = 20.0f
SafetyLimits::MAX_BPM = 300.0f
SafetyLimits::MAX_DURATION_SECONDS = 86400
SafetyLimits::MIN_RATING = 0
SafetyLimits::MAX_RATING = 5
```

## Architecture

The library uses modern C++17 features:

- **Handle Pattern**: All entity references use strongly-typed integer IDs
- **Result<T>**: Error handling without exceptions using `std::variant`
- **std::optional**: Safe nullable value handling
- **std::string_view**: Non-owning string references
- **std::filesystem**: Cross-platform file system operations
- **RAII**: Resource management with smart pointers

## Testing

```bash
cd build
ctest --output-on-failure
```

Or run individual tests:

```bash
./test_database      # 13 unit tests
./test_api_schema    # 9 schema tests
python3 ../tests/golden_test.py
```

## Credits

**C++17 Port**
- [Daito Manabe](https://github.com/daitomanabe)

**Original Implementation**
- [Deep Symmetry, LLC](https://github.com/Deep-Symmetry) - [Crate Digger](https://github.com/Deep-Symmetry/crate-digger) Java library

**Reverse Engineering**
- [@henrybetts](https://github.com/henrybetts), [@flesniak](https://github.com/flesniak) - Rekordbox format analysis

## License

Eclipse Public License - v 2.0

Copyright (c) 2025 Daito Manabe

This is a C++17 port of the original Crate Digger Java library.
Original Java implementation Copyright (c) 2018-2024 Deep Symmetry, LLC.

See [LICENSE](LICENSE) for details.
