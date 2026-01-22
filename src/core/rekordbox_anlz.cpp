#include "cratedigger/rekordbox_anlz.hpp"
#include "cratedigger/logging.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace cratedigger {

namespace {

/// Read big-endian uint32 (ANLZ files use big-endian)
inline uint32_t read_u32_be(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

/// Read big-endian uint16
inline uint16_t read_u16_be(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) |
           static_cast<uint16_t>(data[1]);
}

/// Convert CuePointType from raw value
CuePointType parse_cue_type(uint8_t raw_type) {
    switch (raw_type) {
        case 0: return CuePointType::Cue;
        case 1: return CuePointType::FadeIn;
        case 2: return CuePointType::FadeOut;
        case 3: return CuePointType::Load;
        case 4: return CuePointType::Loop;
        default: return CuePointType::Cue;
    }
}

/// Parse UTF-16BE string to UTF-8
std::string parse_utf16be_string(const uint8_t* data, size_t byte_len) {
    std::string result;
    size_t char_count = byte_len / 2;
    result.reserve(char_count);

    for (size_t i = 0; i < char_count; ++i) {
        uint16_t ch = read_u16_be(data + i * 2);
        if (ch == 0) break;

        if (ch < 0x80) {
            result += static_cast<char>(ch);
        } else if (ch < 0x800) {
            result += static_cast<char>(0xC0 | (ch >> 6));
            result += static_cast<char>(0x80 | (ch & 0x3F));
        } else {
            result += static_cast<char>(0xE0 | (ch >> 12));
            result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (ch & 0x3F));
        }
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// RekordboxAnlz Implementation
// ============================================================================

Result<RekordboxAnlz> RekordboxAnlz::open(const std::filesystem::path& path) {
    RekordboxAnlz anlz;

    // Read entire file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return make_error(
            ErrorCode::FileNotFound,
            "Cannot open ANLZ file: " + path.string()
        );
    }

    auto file_size = file.tellg();
    if (file_size < static_cast<std::streamoff>(sizeof(RawAnlzHeader))) {
        return make_error(
            ErrorCode::InvalidFileFormat,
            "File too small to be a valid ANLZ file"
        );
    }

    anlz.file_data_.resize(static_cast<size_t>(file_size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(anlz.file_data_.data()), file_size);

    if (!file) {
        return make_error(
            ErrorCode::IoError,
            "Failed to read ANLZ file contents"
        );
    }

    // Verify magic number "PMAI"
    uint32_t magic = read_u32_be(anlz.file_data_.data());
    if (magic != 0x504D4149) {  // "PMAI"
        std::ostringstream oss;
        oss << "Invalid ANLZ magic number: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << magic;
        return make_error(
            ErrorCode::InvalidFileFormat,
            oss.str()
        );
    }

    anlz.parse_sections();
    anlz.is_valid_ = true;

    LOG_INFO("Parsed ANLZ file: " + std::to_string(anlz.cue_points_.size()) + " cue points, " + std::to_string(anlz.beat_grid_.beats.size()) + " beats");

    return anlz;
}

RekordboxAnlz::RekordboxAnlz(RekordboxAnlz&& other) noexcept
    : file_data_(std::move(other.file_data_))
    , cue_points_(std::move(other.cue_points_))
    , beat_grid_(std::move(other.beat_grid_))
    , waveforms_(std::move(other.waveforms_))
    , song_structure_(std::move(other.song_structure_))
    , track_path_(std::move(other.track_path_))
    , is_valid_(other.is_valid_)
{
    other.is_valid_ = false;
}

RekordboxAnlz& RekordboxAnlz::operator=(RekordboxAnlz&& other) noexcept {
    if (this != &other) {
        file_data_ = std::move(other.file_data_);
        cue_points_ = std::move(other.cue_points_);
        beat_grid_ = std::move(other.beat_grid_);
        waveforms_ = std::move(other.waveforms_);
        song_structure_ = std::move(other.song_structure_);
        track_path_ = std::move(other.track_path_);
        is_valid_ = other.is_valid_;
        other.is_valid_ = false;
    }
    return *this;
}

RekordboxAnlz::~RekordboxAnlz() = default;

void RekordboxAnlz::parse_sections() {
    if (file_data_.size() < sizeof(RawAnlzHeader)) {
        return;
    }

    const uint8_t* data = file_data_.data();
    uint32_t header_len = read_u32_be(data + 4);
    size_t offset = header_len;

    while (offset + sizeof(RawAnlzSectionHeader) <= file_data_.size()) {
        uint32_t section_type = read_u32_be(data + offset);
        uint32_t section_header_len = read_u32_be(data + offset + 4);
        uint32_t section_len = read_u32_be(data + offset + 8);

        if (section_len == 0 || offset + section_len > file_data_.size()) {
            break;
        }

        const uint8_t* section_data = data + offset + section_header_len;
        size_t section_data_len = section_len - section_header_len;

        switch (static_cast<AnlzSectionType>(section_type)) {
            case AnlzSectionType::CuePointList:
            case AnlzSectionType::CuePointList2:
                parse_cue_list(section_data, section_data_len, false);
                break;

            case AnlzSectionType::ExtCuePointList:
                parse_cue_list(section_data, section_data_len, true);
                break;

            case AnlzSectionType::BeatGrid:
                parse_beat_grid(section_data, section_data_len);
                break;

            case AnlzSectionType::Path:
                track_path_ = parse_path_section(section_data, section_data_len);
                break;

            case AnlzSectionType::WaveformPreview:
            case AnlzSectionType::WaveformTiny:
                parse_waveform_preview(section_data, section_data_len);
                break;

            case AnlzSectionType::WaveformScroll:
                parse_waveform_scroll(section_data, section_data_len, WaveformStyle::Blue);
                break;

            case AnlzSectionType::WaveformColorPreview:
                parse_waveform_color_preview(section_data, section_data_len);
                break;

            case AnlzSectionType::WaveformColorScroll:
                parse_waveform_color_scroll(section_data, section_data_len);
                break;

            case AnlzSectionType::Waveform3BandPreview:
                parse_waveform_3band(section_data, section_data_len, true);
                break;

            case AnlzSectionType::Waveform3BandScroll:
                parse_waveform_3band(section_data, section_data_len, false);
                break;

            case AnlzSectionType::SongStructure:
                parse_song_structure(section_data, section_data_len);
                break;

            default:
                // Skip unknown sections
                break;
        }

        offset += section_len;
    }
}

void RekordboxAnlz::parse_cue_list(const uint8_t* data, size_t len, bool is_extended) {
    // First 4 bytes are the cue count
    if (len < 4) return;

    uint32_t cue_count = read_u32_be(data);
    size_t offset = 4;

    for (uint32_t i = 0; i < cue_count && offset < len; ++i) {
        // Each cue entry starts with magic and lengths
        if (offset + 12 > len) break;

        uint32_t entry_magic = read_u32_be(data + offset);
        // uint32_t entry_header_len = read_u32_be(data + offset + 4);  // Not used
        uint32_t entry_len = read_u32_be(data + offset + 8);

        if (entry_len == 0 || offset + entry_len > len) break;

        // Check for valid cue entry magic ("PCPT" or "PCP2")
        if (entry_magic != 0x50435054 && entry_magic != 0x50435032) {
            offset += entry_len;
            continue;
        }

        CuePointData cue;

        if (is_extended && entry_len >= sizeof(RawExtCuePointEntry)) {
            // Extended format with color
            cue.hot_cue_number = read_u32_be(data + offset + 12);
            uint32_t status = read_u32_be(data + offset + 16);
            cue.is_active = (status != 0);
            cue.type = parse_cue_type(data[offset + 32]);
            cue.time_ms = read_u32_be(data + offset + 36);
            cue.loop_time_ms = read_u32_be(data + offset + 40);
            cue.color_id = data[offset + 44];

            // Parse comment if present
            if (entry_len > 60) {
                uint32_t comment_len = read_u32_be(data + offset + 56);
                if (comment_len > 0 && offset + 60 + comment_len <= len) {
                    cue.comment = parse_utf16be_string(data + offset + 60, comment_len);
                }
            }
        } else if (entry_len >= 44) {
            // Standard format
            cue.hot_cue_number = read_u32_be(data + offset + 12);
            uint32_t status = read_u32_be(data + offset + 16);
            cue.is_active = (status != 0);
            cue.type = parse_cue_type(data[offset + 32]);
            cue.time_ms = read_u32_be(data + offset + 36);
            cue.loop_time_ms = read_u32_be(data + offset + 40);
            cue.color_id = 0;
        }

        // Only add active cue points
        if (cue.is_active) {
            cue_points_.push_back(std::move(cue));
        }

        offset += entry_len;
    }

    // Sort by time
    std::sort(cue_points_.begin(), cue_points_.end(),
              [](const CuePointData& a, const CuePointData& b) {
                  return a.time_ms < b.time_ms;
              });
}

void RekordboxAnlz::parse_beat_grid(const uint8_t* data, size_t len) {
    // Beat grid section format:
    // 4 bytes: unknown/flags
    // 4 bytes: beat count
    // For each beat: 2 bytes beat_number, 2 bytes tempo_100x, 4 bytes time_ms

    if (len < 8) return;

    // Skip 4 bytes of unknown data, then read beat count
    uint32_t beat_count = read_u32_be(data + 4);

    // Each beat entry is 8 bytes
    constexpr size_t beat_entry_size = 8;
    size_t offset = 8;

    beat_grid_.beats.reserve(beat_count);

    for (uint32_t i = 0; i < beat_count && offset + beat_entry_size <= len; ++i) {
        BeatEntry beat;
        beat.beat_number = read_u16_be(data + offset);
        beat.tempo_100x = read_u16_be(data + offset + 2);
        beat.time_ms = read_u32_be(data + offset + 4);

        beat_grid_.beats.push_back(beat);
        offset += beat_entry_size;
    }
}

std::string RekordboxAnlz::parse_path_section(const uint8_t* data, size_t len) {
    if (len < 4) return "";

    // Path length is first 4 bytes
    uint32_t path_len = read_u32_be(data);
    if (path_len == 0 || 4 + path_len > len) return "";

    // Path is UTF-16BE encoded
    return parse_utf16be_string(data + 4, path_len);
}

void RekordboxAnlz::parse_waveform_preview(const uint8_t* data, size_t len) {
    // PWAV/PWV2 format:
    // 4 bytes: len_data (length of preview data)
    // 4 bytes: unknown (always 0x10000)
    // len_data bytes: waveform data (1 byte per entry)
    if (len < 8) return;

    uint32_t data_len = read_u32_be(data);
    if (data_len == 0 || 8 + data_len > len) return;

    WaveformData waveform;
    waveform.style = WaveformStyle::Blue;
    waveform.bytes_per_entry = 1;
    waveform.entry_count = data_len;
    waveform.data.assign(data + 8, data + 8 + data_len);

    waveforms_.preview = std::move(waveform);
}

void RekordboxAnlz::parse_waveform_scroll(const uint8_t* data, size_t len, WaveformStyle style) {
    // PWV3 format:
    // 4 bytes: len_entry_bytes (usually 1)
    // 4 bytes: len_entries (number of waveform data points)
    // 4 bytes: unknown (always 0x960000?)
    // len_entries * len_entry_bytes bytes: waveform data
    if (len < 12) return;

    uint32_t bytes_per_entry = read_u32_be(data);
    uint32_t entry_count = read_u32_be(data + 4);
    // Skip unknown at offset 8

    size_t data_size = static_cast<size_t>(entry_count) * bytes_per_entry;
    if (data_size == 0 || 12 + data_size > len) return;

    WaveformData waveform;
    waveform.style = style;
    waveform.bytes_per_entry = static_cast<uint8_t>(bytes_per_entry);
    waveform.entry_count = entry_count;
    waveform.data.assign(data + 12, data + 12 + data_size);

    waveforms_.detail = std::move(waveform);
}

void RekordboxAnlz::parse_waveform_color_preview(const uint8_t* data, size_t len) {
    // PWV4 format:
    // 4 bytes: len_entry_bytes (usually 6)
    // 4 bytes: len_entries (number of data points)
    // 4 bytes: unknown
    // len_entries * len_entry_bytes bytes: waveform data
    if (len < 12) return;

    uint32_t bytes_per_entry = read_u32_be(data);
    uint32_t entry_count = read_u32_be(data + 4);

    size_t data_size = static_cast<size_t>(entry_count) * bytes_per_entry;
    if (data_size == 0 || 12 + data_size > len) return;

    WaveformData waveform;
    waveform.style = WaveformStyle::RGB;
    waveform.bytes_per_entry = static_cast<uint8_t>(bytes_per_entry);
    waveform.entry_count = entry_count;
    waveform.data.assign(data + 12, data + 12 + data_size);

    waveforms_.color_preview = std::move(waveform);
}

void RekordboxAnlz::parse_waveform_color_scroll(const uint8_t* data, size_t len) {
    // PWV5 format:
    // 4 bytes: len_entry_bytes (usually 2)
    // 4 bytes: len_entries (number of columns)
    // 4 bytes: unknown
    // len_entries * len_entry_bytes bytes: waveform data (RGB format)
    if (len < 12) return;

    uint32_t bytes_per_entry = read_u32_be(data);
    uint32_t entry_count = read_u32_be(data + 4);

    size_t data_size = static_cast<size_t>(entry_count) * bytes_per_entry;
    if (data_size == 0 || 12 + data_size > len) return;

    WaveformData waveform;
    waveform.style = WaveformStyle::RGB;
    waveform.bytes_per_entry = static_cast<uint8_t>(bytes_per_entry);
    waveform.entry_count = entry_count;
    waveform.data.assign(data + 12, data + 12 + data_size);

    // Color scroll is the preferred detail waveform
    waveforms_.detail = std::move(waveform);
}

void RekordboxAnlz::parse_waveform_3band(const uint8_t* data, size_t len, bool is_preview) {
    // PWV6/PWV7 format (CDJ-3000 style):
    // 4 bytes: len_entry_bytes (usually 3)
    // 4 bytes: len_entries
    // len_entries * len_entry_bytes bytes: waveform data (low/mid/high)
    if (len < 8) return;

    uint32_t bytes_per_entry = read_u32_be(data);
    uint32_t entry_count = read_u32_be(data + 4);

    size_t data_size = static_cast<size_t>(entry_count) * bytes_per_entry;
    if (data_size == 0 || 8 + data_size > len) return;

    WaveformData waveform;
    waveform.style = WaveformStyle::ThreeBand;
    waveform.bytes_per_entry = static_cast<uint8_t>(bytes_per_entry);
    waveform.entry_count = entry_count;
    waveform.data.assign(data + 8, data + 8 + data_size);

    if (is_preview) {
        waveforms_.color_preview = std::move(waveform);
    } else {
        waveforms_.detail = std::move(waveform);
    }
}

void RekordboxAnlz::parse_song_structure(const uint8_t* data, size_t len) {
    // PSI2/PSSI format:
    // 4 bytes: len_entry_bytes (always 24)
    // 2 bytes: len_entries (number of phrases)
    // Rest: body (may be XOR masked)
    //
    // Body (after unmasking):
    // 2 bytes: mood (1=high, 2=mid, 3=low)
    // 6 bytes: padding
    // 2 bytes: end_beat
    // 2 bytes: padding
    // 1 byte: bank
    // 1 byte: padding
    // phrases[]: 24 bytes each

    if (len < 6) return;

    uint32_t entry_bytes = read_u32_be(data);
    uint16_t entry_count = read_u16_be(data + 4);

    if (entry_bytes != 24 || entry_count == 0) return;

    // Calculate body offset and size
    size_t body_offset = 6;
    size_t body_len = len - body_offset;

    if (body_len < 14 + entry_count * 24) return;

    // Copy body data for potential unmasking
    std::vector<uint8_t> body(data + body_offset, data + len);

    // Check if data is masked (raw_mood > 20 indicates masking)
    uint16_t raw_mood = read_u16_be(body.data());
    if (raw_mood > 20) {
        // Unmask the data using XOR
        // Mask: repeating 19-byte sequence based on entry_count
        uint8_t c = static_cast<uint8_t>(entry_count);
        uint8_t mask[19] = {
            static_cast<uint8_t>(0xCB + c), static_cast<uint8_t>(0xE1 + c),
            static_cast<uint8_t>(0xEE + c), static_cast<uint8_t>(0xFA + c),
            static_cast<uint8_t>(0xE5 + c), static_cast<uint8_t>(0xEE + c),
            static_cast<uint8_t>(0xAD + c), static_cast<uint8_t>(0xEE + c),
            static_cast<uint8_t>(0xE9 + c), static_cast<uint8_t>(0xD2 + c),
            static_cast<uint8_t>(0xE9 + c), static_cast<uint8_t>(0xEB + c),
            static_cast<uint8_t>(0xE1 + c), static_cast<uint8_t>(0xE9 + c),
            static_cast<uint8_t>(0xF3 + c), static_cast<uint8_t>(0xE8 + c),
            static_cast<uint8_t>(0xE9 + c), static_cast<uint8_t>(0xF4 + c),
            static_cast<uint8_t>(0xE1 + c)
        };

        for (size_t i = 0; i < body.size(); ++i) {
            body[i] ^= mask[i % 19];
        }
    }

    // Parse the unmasked body
    uint16_t mood = read_u16_be(body.data());
    if (mood < 1 || mood > 3) return;  // Invalid mood

    song_structure_.mood = static_cast<TrackMood>(mood);
    song_structure_.end_beat = read_u16_be(body.data() + 8);
    song_structure_.bank = static_cast<TrackBank>(body[12]);

    // Parse phrase entries
    size_t entry_offset = 14;
    song_structure_.phrases.reserve(entry_count);

    for (uint16_t i = 0; i < entry_count && entry_offset + 24 <= body.size(); ++i) {
        const uint8_t* e = body.data() + entry_offset;

        PhraseEntry phrase;
        phrase.index = read_u16_be(e);
        phrase.beat = read_u16_be(e + 2);
        phrase.kind = read_u16_be(e + 4);
        phrase.k1 = e[7];
        phrase.k2 = e[9];
        // e[10] is 'b' flag for extra beats, e[11-16] are beat2/beat3/beat4
        phrase.k3 = e[19];
        phrase.has_fill = e[21] != 0;
        phrase.fill_beat = read_u16_be(e + 22);

        // Calculate end_beat from next phrase or track end
        if (i + 1 < entry_count && entry_offset + 48 <= body.size()) {
            phrase.end_beat = read_u16_be(body.data() + entry_offset + 26);  // next phrase's beat
        } else {
            phrase.end_beat = song_structure_.end_beat;
        }

        song_structure_.phrases.push_back(std::move(phrase));
        entry_offset += 24;
    }
}

// ============================================================================
// CuePointManager Implementation
// ============================================================================

void CuePointManager::scan_directory(const std::filesystem::path& anlz_dir) {
    if (!std::filesystem::exists(anlz_dir)) {
        LOG_WARN("ANLZ directory does not exist: " + anlz_dir.string());
        return;
    }

    size_t loaded = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(anlz_dir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".dat" || ext == ".ext") {
                load_anlz_file(entry.path());
                ++loaded;
            }
        }
    }

    LOG_INFO("Loaded " + std::to_string(loaded) + " ANLZ files: " +
             std::to_string(cue_point_index_.size()) + " cues, " +
             std::to_string(beat_grid_index_.size()) + " beats, " +
             std::to_string(waveform_index_.size()) + " waves, " +
             std::to_string(song_structure_index_.size()) + " structures");
}

void CuePointManager::load_anlz_file(const std::filesystem::path& path) {
    auto result = RekordboxAnlz::open(path);
    if (!result) {
        // Skip files that fail to parse (e.g., corrupted or incompatible format)
        return;
    }

    auto& anlz = *result;

    std::string track_path = anlz.track_path();
    if (track_path.empty()) {
        // Use filename as key if no path in ANLZ
        track_path = path.stem().string();
    }

    // Merge cue points (newer file overwrites)
    if (!anlz.cue_points().empty()) {
        auto& existing_cues = cue_point_index_[track_path];
        if (existing_cues.empty()) {
            existing_cues = anlz.cue_points();
        } else {
            // Extended format (.EXT) has priority over standard format (.DAT)
            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".ext") {
                existing_cues = anlz.cue_points();
            }
        }
    }

    // Store beat grid if present
    if (anlz.has_beat_grid()) {
        auto& existing_grid = beat_grid_index_[track_path];
        if (existing_grid.empty()) {
            existing_grid = anlz.beat_grid();
        }
    }

    // Store waveforms if present
    if (anlz.has_waveforms()) {
        auto& existing_waveforms = waveform_index_[track_path];
        const auto& new_waveforms = anlz.waveforms();

        // Merge waveforms - prefer higher quality versions
        if (new_waveforms.preview && !existing_waveforms.preview) {
            existing_waveforms.preview = new_waveforms.preview;
        }
        if (new_waveforms.detail) {
            // Prefer colored/3-band detail over blue
            if (!existing_waveforms.detail ||
                (existing_waveforms.detail->style == WaveformStyle::Blue &&
                 new_waveforms.detail->style != WaveformStyle::Blue)) {
                existing_waveforms.detail = new_waveforms.detail;
            }
        }
        if (new_waveforms.color_preview) {
            // Prefer 3-band over RGB
            if (!existing_waveforms.color_preview ||
                (existing_waveforms.color_preview->style == WaveformStyle::RGB &&
                 new_waveforms.color_preview->style == WaveformStyle::ThreeBand)) {
                existing_waveforms.color_preview = new_waveforms.color_preview;
            }
        }
    }

    // Store song structure if present
    if (anlz.has_song_structure()) {
        auto& existing = song_structure_index_[track_path];
        if (existing.empty()) {
            existing = anlz.song_structure();
        }
    }
}

std::vector<CuePointData> CuePointManager::get_cue_points(const std::string& track_path) const {
    auto it = cue_point_index_.find(track_path);
    if (it != cue_point_index_.end()) {
        return it->second;
    }
    return {};
}

std::vector<CuePointData> CuePointManager::find_cue_points_by_filename(const std::string& filename) const {
    // Search for partial match
    for (const auto& [path, cues] : cue_point_index_) {
        if (path.find(filename) != std::string::npos) {
            return cues;
        }
    }
    return {};
}

const BeatGrid* CuePointManager::get_beat_grid(const std::string& track_path) const {
    auto it = beat_grid_index_.find(track_path);
    if (it != beat_grid_index_.end()) {
        return &it->second;
    }
    return nullptr;
}

const BeatGrid* CuePointManager::find_beat_grid_by_filename(const std::string& filename) const {
    // Search for partial match
    for (const auto& [path, grid] : beat_grid_index_) {
        if (path.find(filename) != std::string::npos) {
            return &grid;
        }
    }
    return nullptr;
}

const TrackWaveforms* CuePointManager::get_waveforms(const std::string& track_path) const {
    auto it = waveform_index_.find(track_path);
    if (it != waveform_index_.end()) {
        return &it->second;
    }
    return nullptr;
}

const TrackWaveforms* CuePointManager::find_waveforms_by_filename(const std::string& filename) const {
    // Search for partial match
    for (const auto& [path, waveforms] : waveform_index_) {
        if (path.find(filename) != std::string::npos) {
            return &waveforms;
        }
    }
    return nullptr;
}

const SongStructure* CuePointManager::get_song_structure(const std::string& track_path) const {
    auto it = song_structure_index_.find(track_path);
    if (it != song_structure_index_.end()) {
        return &it->second;
    }
    return nullptr;
}

const SongStructure* CuePointManager::find_song_structure_by_filename(const std::string& filename) const {
    // Search for partial match
    for (const auto& [path, structure] : song_structure_index_) {
        if (path.find(filename) != std::string::npos) {
            return &structure;
        }
    }
    return nullptr;
}

// ============================================================================
// BeatGrid Implementation
// ============================================================================

size_t BeatGrid::find_beat_at(uint32_t time_ms) const {
    if (beats.empty()) return 0;

    // Binary search for the beat nearest to the given time
    auto it = std::lower_bound(beats.begin(), beats.end(), time_ms,
        [](const BeatEntry& beat, uint32_t t) {
            return beat.time_ms < t;
        });

    if (it == beats.end()) {
        return beats.size() - 1;
    }
    if (it == beats.begin()) {
        return 0;
    }

    // Check if previous beat is closer
    auto prev = it - 1;
    if ((time_ms - prev->time_ms) <= (it->time_ms - time_ms)) {
        return static_cast<size_t>(prev - beats.begin());
    }
    return static_cast<size_t>(it - beats.begin());
}

std::vector<BeatEntry> BeatGrid::get_beats_in_range(uint32_t start_ms, uint32_t end_ms) const {
    std::vector<BeatEntry> result;
    if (beats.empty() || start_ms > end_ms) return result;

    auto start_it = std::lower_bound(beats.begin(), beats.end(), start_ms,
        [](const BeatEntry& beat, uint32_t t) {
            return beat.time_ms < t;
        });

    auto end_it = std::upper_bound(beats.begin(), beats.end(), end_ms,
        [](uint32_t t, const BeatEntry& beat) {
            return t < beat.time_ms;
        });

    result.reserve(static_cast<size_t>(end_it - start_it));
    for (auto it = start_it; it != end_it; ++it) {
        result.push_back(*it);
    }
    return result;
}

float BeatGrid::average_bpm() const {
    if (beats.empty()) return 0.0f;

    float sum = 0.0f;
    for (const auto& beat : beats) {
        sum += beat.bpm();
    }
    return sum / static_cast<float>(beats.size());
}

// ============================================================================
// SongStructure Implementation
// ============================================================================

size_t SongStructure::find_phrase_at_beat(uint16_t beat) const {
    if (phrases.empty()) return 0;

    // Binary search for the phrase containing the given beat
    for (size_t i = 0; i < phrases.size(); ++i) {
        if (beat >= phrases[i].beat && beat < phrases[i].end_beat) {
            return i;
        }
    }

    // If beat is past all phrases, return last phrase
    if (beat >= phrases.back().beat) {
        return phrases.size() - 1;
    }

    return 0;
}

} // namespace cratedigger
