#include "cratedigger/rekordbox_anlz.hpp"
#include "cratedigger/logging.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <format>

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
            std::format("Cannot open ANLZ file: {}", path.string())
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
        return make_error(
            ErrorCode::InvalidFileFormat,
            std::format("Invalid ANLZ magic number: {:08X}", magic)
        );
    }

    anlz.parse_sections();
    anlz.is_valid_ = true;

    LOG_INFO("Parsed ANLZ file: {} cue points", anlz.cue_points_.size());

    return anlz;
}

RekordboxAnlz::RekordboxAnlz(RekordboxAnlz&& other) noexcept
    : file_data_(std::move(other.file_data_))
    , cue_points_(std::move(other.cue_points_))
    , track_path_(std::move(other.track_path_))
    , is_valid_(other.is_valid_)
{
    other.is_valid_ = false;
}

RekordboxAnlz& RekordboxAnlz::operator=(RekordboxAnlz&& other) noexcept {
    if (this != &other) {
        file_data_ = std::move(other.file_data_);
        cue_points_ = std::move(other.cue_points_);
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

            case AnlzSectionType::Path:
                track_path_ = parse_path_section(section_data, section_data_len);
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

std::string RekordboxAnlz::parse_path_section(const uint8_t* data, size_t len) {
    if (len < 4) return "";

    // Path length is first 4 bytes
    uint32_t path_len = read_u32_be(data);
    if (path_len == 0 || 4 + path_len > len) return "";

    // Path is UTF-16BE encoded
    return parse_utf16be_string(data + 4, path_len);
}

// ============================================================================
// CuePointManager Implementation
// ============================================================================

void CuePointManager::scan_directory(const std::filesystem::path& anlz_dir) {
    if (!std::filesystem::exists(anlz_dir)) {
        LOG_WARN("ANLZ directory does not exist: {}", anlz_dir.string());
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

    LOG_INFO("Loaded {} ANLZ files, {} tracks with cue points", loaded, cue_point_index_.size());
}

void CuePointManager::load_anlz_file(const std::filesystem::path& path) {
    auto result = RekordboxAnlz::open(path);
    if (!result) {
        // Skip files that fail to parse (e.g., corrupted or incompatible format)
        return;
    }

    auto& anlz = *result;
    if (anlz.cue_points().empty()) {
        return;
    }

    std::string track_path = anlz.track_path();
    if (track_path.empty()) {
        // Use filename as key if no path in ANLZ
        track_path = path.stem().string();
    }

    // Merge cue points (newer file overwrites)
    auto& existing = cue_point_index_[track_path];
    if (existing.empty()) {
        existing = anlz.cue_points();
    } else {
        // Extended format (.EXT) has priority over standard format (.DAT)
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".ext") {
            existing = anlz.cue_points();
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

} // namespace cratedigger
