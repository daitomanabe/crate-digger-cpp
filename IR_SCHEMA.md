# Crate Digger IR Schema

Internal Representation Schema for Crate Digger C++20 Port.

Per INTRODUCTION_JAVA_TO_CPP.md Section 1.2:
- MUST: No Magic Numbers - All constants defined here

## Constants

### Page Size
```
EXPECTED_PAGE_SIZE = 4096  // bytes
```

### String Limits
```
MAX_STRING_LENGTH = 4096   // characters
MAX_ROWS_PER_TABLE = 1000000
```

### BPM Encoding
```
BPM stored as: uint32_t tempo = BPM * 100
Example: 128.50 BPM = 12850
```

### Duration
```
Duration stored as: uint16_t duration (seconds)
```

### Rating
```
Rating stored as: uint8_t rating (0-5 stars)
```

## ID Types

All IDs are 64-bit signed integers wrapped in strong types:

| Type | Range | Description |
|------|-------|-------------|
| TrackId | 1+ | Unique track identifier |
| ArtistId | 1+ | Unique artist identifier |
| AlbumId | 1+ | Unique album identifier |
| GenreId | 1+ | Unique genre identifier |
| LabelId | 1+ | Unique label identifier |
| ColorId | 0-8 | Track color tag |
| KeyId | 1+ | Musical key identifier |
| ArtworkId | 1+ | Artwork path identifier |
| PlaylistId | 0+ | Playlist/folder identifier |

## Page Types (export.pdb)

| Value | Name | Description |
|-------|------|-------------|
| 0 | Tracks | Track metadata |
| 1 | Genres | Genre names |
| 2 | Artists | Artist names |
| 3 | Albums | Album names |
| 4 | Labels | Record label names |
| 5 | Keys | Musical key names |
| 6 | Colors | Color tag names |
| 7 | PlaylistTree | Playlist folder hierarchy |
| 8 | PlaylistEntries | Playlist track entries |
| 11 | HistoryPlaylists | History playlist names |
| 12 | HistoryEntries | History playlist entries |
| 13 | Artwork | Artwork file paths |

## Page Types (exportExt.pdb)

| Value | Name | Description |
|-------|------|-------------|
| 3 | Tags | Tag names and categories |
| 4 | TagTracks | Tag-track associations |

## String Encoding

DeviceSqlString format:
- `0x01-0x7F odd`: Short ASCII (length = value >> 1)
- `0x40`: Long ASCII (2-byte length follows)
- `0x90`: Long UTF-16LE (2-byte length follows)

## Row Structure Offsets

### TrackRow String Offsets
| Index | Field |
|-------|-------|
| 0 | ISRC |
| 5 | Message |
| 10 | Date Added |
| 11 | Release Date |
| 12 | Mix Name |
| 14 | Analyze Path |
| 15 | Analyze Date |
| 16 | Comment |
| 17 | Title |
| 19 | Filename |
| 20 | File Path |

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | Success | Operation completed successfully |
| 1 | FileNotFound | Database file not found |
| 2 | InvalidFileFormat | Not a valid PDB file |
| 3 | CorruptedData | Data corruption detected |
| 4 | TableNotFound | Requested table not in database |
| 5 | RowNotFound | Requested row not found |
| 6 | OutOfMemory | Memory allocation failed |
| 7 | IoError | File I/O error |
| 8 | InvalidParameter | Invalid parameter value |
| 9 | UnknownError | Unspecified error |

## Protocol Endpoints

### CLI Commands (JSONL)
```json
{"cmd": "describe_api"}
{"cmd": "open", "path": "/path/to/export.pdb"}
{"cmd": "get_track", "id": 123}
{"cmd": "find_tracks_by_title", "title": "..."}
{"cmd": "all_track_ids"}
{"cmd": "track_count"}
```

### CLI Options
```
--schema    Output API schema JSON
--help      Show help
--version   Show version
```

## Python API

```python
import crate_digger

# Get API schema
schema = crate_digger.describe_api()

# Open database
db = crate_digger.Database.open("/path/to/export.pdb")

# Query data
track = db.get_track(crate_digger.TrackId(123))
tracks = db.find_tracks_by_title("Song Title")
bpms = db.get_all_bpms()  # For numpy.array()
```

## NumPy Integration

Data extraction for AI/ML:

```python
import numpy as np
import crate_digger

db = crate_digger.Database.open("export.pdb")

# Zero-copy-friendly extraction
bpms = np.array(db.get_all_bpms(), dtype=np.float32)
durations = np.array(db.get_all_durations(), dtype=np.int32)
```

## Safety Curtain (Future)

For hardware control applications:

```
MAX_BPM = 300.0
MIN_BPM = 20.0
MAX_DURATION = 86400  // 24 hours
```

Any values outside these ranges should be clamped or rejected.
