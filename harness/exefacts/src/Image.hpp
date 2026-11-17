// The exe's memory image: headers and sections at their virtual addresses, as pefile's get_memory_mapped_image()
// builds it (the Python tools' view of the bytes).
#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace exefacts {

// Where the Python facts raise (an out-of-range table read, a function without code): the fact has no answer.
struct Raised : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Image {
public:
    explicit Image(const std::string& path);

    uint32_t base() const { return base_; }
    uint32_t textStart() const { return textStart_; }
    // .text's raw data end (fn.py's function bounds)
    uint32_t textRawEnd() const { return textRawEnd_; }

    // draft.Exe.valid: va lies in the image
    bool valid(uint32_t va) const;
    // The bytes from va to the image's end (empty outside it)
    std::span<const uint8_t> from(uint32_t va) const;
    // draft.Exe.u8 / u32 at an address computed without wrapping: Python indexing, so an offset below the base
    // counts from the image's end; Raised outside
    uint8_t u8(int64_t va) const;
    uint32_t u32(int64_t va) const;

private:
    int64_t pythonOffset(int64_t va, uint32_t size) const;

    std::vector<uint8_t> bytes_;
    uint32_t base_ = 0;
    uint32_t textStart_ = 0;
    uint32_t textRawEnd_ = 0;
};

}  // namespace exefacts
