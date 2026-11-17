#include "Image.hpp"

#include <cstring>
#include <fstream>
#include <iterator>

namespace exefacts {

namespace {

uint32_t le32(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 4 > data.size())
        throw std::runtime_error("truncated PE header");
    uint32_t value;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

uint16_t le16(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 2 > data.size())
        throw std::runtime_error("truncated PE header");
    uint16_t value;
    std::memcpy(&value, data.data() + offset, 2);
    return value;
}

}  // namespace

Image::Image(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open " + path);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    const size_t pe = le32(file, 0x3C);
    if (le32(file, pe) != 0x4550)
        throw std::runtime_error(path + ": not a PE image");
    const size_t coff = pe + 4;
    const uint16_t sectionCount = le16(file, coff + 2);
    const size_t optional = coff + 20;
    const size_t sections = optional + le16(file, coff + 16);
    base_ = le32(file, optional + 28);
    const uint32_t headerSize = le32(file, optional + 60);

    // pefile: the headers, then each section's raw data at its virtual address (zero gaps, an overlap cut)
    bytes_.assign(file.begin(), file.begin() + std::min<size_t>(headerSize, file.size()));
    for (uint16_t i = 0; i < sectionCount; ++i) {
        const size_t header = sections + 40 * size_t(i);
        char name[9] = {};
        std::memcpy(name, file.data() + header, 8);
        const uint32_t virtualSize = le32(file, header + 8);
        const uint32_t virtualAddress = le32(file, header + 12);
        const uint32_t rawSize = le32(file, header + 16);
        const uint32_t rawOffset = le32(file, header + 20);
        if (std::strcmp(name, ".text") == 0) {
            textStart_ = base_ + virtualAddress;
            textRawEnd_ = textStart_ + rawSize;
        }
        if ((virtualSize == 0 && rawSize == 0) || uint64_t(rawOffset) + rawSize > file.size())
            continue;
        bytes_.resize(virtualAddress);
        bytes_.insert(bytes_.end(), file.begin() + rawOffset, file.begin() + rawOffset + rawSize);
    }
    if (textStart_ == 0)
        throw std::runtime_error(path + ": no .text section");
}

bool Image::valid(uint32_t va) const {
    return !from(va).empty();
}

std::span<const uint8_t> Image::from(uint32_t va) const {
    if (va < base_ || va - base_ >= bytes_.size())
        return {};
    return std::span(bytes_).subspan(va - base_);
}

int64_t Image::pythonOffset(int64_t va, uint32_t size) const {
    int64_t offset = va - base_;
    if (offset < 0)
        offset += int64_t(bytes_.size());
    if (offset < 0 || offset + size > int64_t(bytes_.size()))
        throw Raised("image read out of range");
    return offset;
}

uint8_t Image::u8(int64_t va) const {
    return bytes_[pythonOffset(va, 1)];
}

uint32_t Image::u32(int64_t va) const {
    uint32_t value;
    std::memcpy(&value, bytes_.data() + pythonOffset(va, 4), 4);
    return value;
}

}  // namespace exefacts
