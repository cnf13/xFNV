#pragma once

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"

#include <stdint.h>

// A stream writer that keeps the CRC-32 of the bytes written to it and stores none.
class hkCrc32StreamWriter : public hkStreamWriter {
public:
    hkCrc32StreamWriter() = default;
    hkCrc32StreamWriter(uint32_t startCrc);

    // Says yes.
    hkBool isOk() const override;
    // Folds the bytes into the CRC; returns nbytes.
    int32_t write(const void* ptr, int32_t nbytes) override;

    // The finished CRC: the running value inverted.
    uint32_t getCrc32() const;

    uint32_t m_crc32;
};
