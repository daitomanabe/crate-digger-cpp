# Crate Digger C++ (C++17)

A C++17 library for parsing rekordbox database exports (export.pdb and exportExt.pdb files) and analysis files (ANLZ).

This is a C++ port of the original [Crate Digger](https://github.com/Deep-Symmetry/crate-digger) Java library by Deep Symmetry.

## Features

- Parse rekordbox `export.pdb` database files
- Parse rekordbox `exportExt.pdb` files (Tags support)
- Parse ANLZ files for Cue Points and Hot Cues
- Access tracks, artists, albums, genres, colors, labels, keys, tags, and playlists
- Primary and secondary index lookups
- Range search (BPM, duration, year, rating)
- Bulk data retrieval for NumPy integration
- Handle Pattern design (strongly-typed integer IDs)
- Safety validation functions for data integrity
- Self-describing API via `describe_api()` for AI agent integration
- Python bindings via nanobind
- JSONL structured logging

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+
- fmt library (for string formatting)
- gsl-lite (for span support)
- nlohmann_json (for JSON handling)
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

// Load Cue Points from ANLZ directory
db.load_cue_points("path/to/PIONEER/USBANLZ");
auto cues = db.get_cue_points_for_track(TrackId{123});
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

# Cue Points
db.load_cue_points("path/to/PIONEER/USBANLZ")
cues = db.get_cue_points_for_track(track_id)
for cue in cues:
    print(f"Cue at {cue.time_seconds()}s - {cue.comment}")

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
- `TagRow` - Tag name (exportExt.pdb)
- `CuePoint` - Cue point/hot cue information

### Safety Limits

```cpp
SafetyLimits::MIN_BPM = 20.0f
SafetyLimits::MAX_BPM = 300.0f
SafetyLimits::MAX_DURATION_SECONDS = 86400
SafetyLimits::MIN_RATING = 0
SafetyLimits::MAX_RATING = 5
```

## Architecture

The library uses C++17 with compatibility libraries:

- **Handle Pattern**: All entity references use strongly-typed integer IDs
- **Result<T>**: Error handling without exceptions using `std::variant`
- **gsl::span**: Non-owning views for binary data (via gsl-lite)
- **fmt::format**: Modern string formatting (via fmt library)
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

## License

Eclipse Public License - v 2.0

Copyright (c) 2025 Daito Manabe / Rhizomatiks

This is a C++ port of the original Crate Digger Java library.
Original Java implementation Copyright (c) 2018-2024 Deep Symmetry, LLC.

See [LICENSE](LICENSE) for details.

## Credits

- [Deep Symmetry, LLC](https://github.com/Deep-Symmetry) - Original Java implementation
- [@henrybetts](https://github.com/henrybetts), [@flesniak](https://github.com/flesniak) - Reverse engineering of rekordbox format
- [Daito Manabe](https://github.com/daitomanabe) / [Rhizomatiks](https://rhizomatiks.com) - C++ port
