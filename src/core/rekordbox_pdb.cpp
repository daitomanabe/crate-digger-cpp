#include "cratedigger/rekordbox_pdb.hpp"
#include "cratedigger/logging.hpp"
#include <cstring>
#include <algorithm>
#include <fmt/format.h>

namespace cratedigger {

namespace {

/// Read little-endian uint16
inline uint16_t read_u16_le(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
}

/// Read little-endian uint32
inline uint32_t read_u32_le(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

/// Parse device SQL string at given position
std::string parse_device_sql_string(const uint8_t* data, size_t max_len) {
    if (max_len == 0) return "";

    uint8_t length_and_kind = data[0];

    // Check for special encoding markers
    if (length_and_kind == 0x40) {
        // Long ASCII string
        if (max_len < 4) return "";
        uint16_t length = read_u16_le(data + 1);
        if (length < 4 || static_cast<size_t>(length - 4) > max_len - 4) return "";
        return std::string(reinterpret_cast<const char*>(data + 4), length - 4);
    }
    else if (length_and_kind == 0x90) {
        // Long UTF-16LE string
        if (max_len < 4) return "";
        uint16_t length = read_u16_le(data + 1);
        if (length < 4) return "";

        size_t char_count = (length - 4) / 2;
        std::string result;
        result.reserve(char_count);

        const uint8_t* utf16_data = data + 4;
        for (size_t i = 0; i < char_count && (i * 2 + 1) < max_len - 4; ++i) {
            uint16_t ch = read_u16_le(utf16_data + i * 2);
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
    else {
        // Short ASCII string
        size_t length = length_and_kind >> 1;
        if (length == 0 || length > max_len) return "";
        return std::string(reinterpret_cast<const char*>(data + 1), length - 1);
    }
}

} // anonymous namespace

// ============================================================================
// RekordboxPdb Implementation
// ============================================================================

Result<RekordboxPdb> RekordboxPdb::open(const std::filesystem::path& path, bool is_ext) {
    RekordboxPdb pdb;
    pdb.is_ext_ = is_ext;

    // Read entire file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return make_error(
            ErrorCode::FileNotFound,
            fmt::format("Cannot open file: {}", path.string())
        );
    }

    auto file_size = file.tellg();
    if (file_size < 28) {
        return make_error(
            ErrorCode::InvalidFileFormat,
            "File too small to be a valid PDB file"
        );
    }

    pdb.file_data_.resize(static_cast<size_t>(file_size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(pdb.file_data_.data()), file_size);

    if (!file) {
        return make_error(
            ErrorCode::IoError,
            "Failed to read file contents"
        );
    }

    const uint8_t* data = pdb.file_data_.data();

    // Parse header
    pdb.page_size_ = read_u32_le(data + 4);
    pdb.table_count_ = read_u32_le(data + 8);

    if (pdb.page_size_ == 0 || pdb.page_size_ > 65536) {
        return make_error(
            ErrorCode::InvalidFileFormat,
            fmt::format("Invalid page size: {}", pdb.page_size_)
        );
    }

    // Parse table definitions (starting at offset 28)
    size_t offset = 28;
    for (uint32_t i = 0; i < pdb.table_count_; ++i) {
        if (offset + 16 > pdb.file_data_.size()) {
            return make_error(
                ErrorCode::CorruptedData,
                "Table definition extends past end of file"
            );
        }

        PdbTable table;
        uint32_t type_raw = read_u32_le(data + offset);
        if (is_ext) {
            table.type_ext = static_cast<PageTypeExt>(type_raw);
        } else {
            table.type = static_cast<PageType>(type_raw);
        }
        table.empty_candidate = read_u32_le(data + offset + 4);
        table.first_page_index = read_u32_le(data + offset + 8);
        table.last_page_index = read_u32_le(data + offset + 12);

        pdb.tables_.push_back(table);
        offset += 16;
    }

    LOG_INFO("Opened PDB file: {} tables, page size: {}", pdb.table_count_, pdb.page_size_);

    return pdb;
}

RekordboxPdb::RekordboxPdb(RekordboxPdb&& other) noexcept
    : file_data_(std::move(other.file_data_))
    , tables_(std::move(other.tables_))
    , page_size_(other.page_size_)
    , table_count_(other.table_count_)
    , is_ext_(other.is_ext_)
{
    other.page_size_ = 0;
    other.table_count_ = 0;
}

RekordboxPdb& RekordboxPdb::operator=(RekordboxPdb&& other) noexcept {
    if (this != &other) {
        file_data_ = std::move(other.file_data_);
        tables_ = std::move(other.tables_);
        page_size_ = other.page_size_;
        table_count_ = other.table_count_;
        is_ext_ = other.is_ext_;
        other.page_size_ = 0;
        other.table_count_ = 0;
    }
    return *this;
}

RekordboxPdb::~RekordboxPdb() = default;

gsl::span<const PdbTable> RekordboxPdb::tables() const {
    return tables_;
}

Result<PdbPage> RekordboxPdb::read_page(uint32_t page_index) const {
    size_t page_offset = static_cast<size_t>(page_size_) * page_index;

    if (page_offset + page_size_ > file_data_.size()) {
        return make_error(
            ErrorCode::CorruptedData,
            fmt::format("Page {} extends past end of file", page_index)
        );
    }

    const uint8_t* page_data = file_data_.data() + page_offset;

    PdbPage page;
    // Skip gap (4 bytes of zeros)
    page.page_index = read_u32_le(page_data + 4);

    uint32_t type_raw = read_u32_le(page_data + 8);
    if (is_ext_) {
        page.type_ext = static_cast<PageTypeExt>(type_raw);
    } else {
        page.type = static_cast<PageType>(type_raw);
    }

    page.next_page_index = read_u32_le(page_data + 12);

    // Parse bit fields at offset 20
    uint32_t row_info = read_u32_le(page_data + 20);
    page.num_row_offsets = static_cast<uint16_t>(row_info & 0x1FFF);
    page.num_rows = static_cast<uint16_t>((row_info >> 13) & 0x7FF);
    page.page_flags = static_cast<uint8_t>((row_info >> 24) & 0xFF);

    page.free_size = read_u16_le(page_data + 24);
    page.used_size = read_u16_le(page_data + 26);

    page.is_data_page = (page.page_flags & 0x40) == 0;

    // Parse row groups if this is a data page
    if (page.is_data_page && page.num_row_offsets > 0) {
        uint16_t num_groups = (page.num_row_offsets - 1) / 16 + 1;
        size_t heap_pos = 40;

        for (uint16_t group_idx = 0; group_idx < num_groups; ++group_idx) {
            RowGroup group;
            group.heap_pos = page_offset + heap_pos;

            size_t base = page_size_ - (group_idx * 0x24);

            if (base >= 4 && base <= page_size_) {
                group.row_present_flags = read_u16_le(page_data + base - 4);
            }

            for (int row_idx = 0; row_idx < 16; ++row_idx) {
                size_t ofs_pos = base - (6 + 2 * row_idx);
                if (ofs_pos >= 2 && ofs_pos < page_size_) {
                    uint16_t ofs = read_u16_le(page_data + ofs_pos);
                    group.row_offsets.push_back(ofs);
                }
            }

            page.row_groups.push_back(std::move(group));
        }
    }

    return page;
}

std::string RekordboxPdb::read_string(size_t offset) const {
    if (offset >= file_data_.size()) {
        return "";
    }
    return parse_device_sql_string(
        file_data_.data() + offset,
        file_data_.size() - offset
    );
}

gsl::span<const uint8_t> RekordboxPdb::data_at(size_t offset, size_t size) const {
    if (offset + size > file_data_.size()) {
        return {};
    }
    return gsl::span<const uint8_t>(file_data_.data() + offset, size);
}

} // namespace cratedigger
