#include "Havok/Source/Common/Base/System/Io/Writer/Crc32/hkCrc32StreamWriter.h"

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
static uint32_t calcPermute(uint32_t p);

static uint32_t calcPermute(uint32_t p) {
    uint32_t v1;
    uint32_t v2;
    uint32_t v3;
    uint32_t v4;
    uint32_t v5;
    uint32_t v6;
    uint32_t v7;
    if ((p & 1) != 0) {
        v1 = p >> 1 ^ 0xEDB88320u;
    }
    else {
        v1 = p >> 1;
    }
    if ((v1 & 1) != 0) {
        v2 = v1 >> 1 ^ 0xEDB88320u;
    }
    else {
        v2 = v1 >> 1;
    }
    if ((v2 & 1) != 0) {
        v3 = v2 >> 1 ^ 0xEDB88320u;
    }
    else {
        v3 = v2 >> 1;
    }
    if ((v3 & 1) != 0) {
        v4 = v3 >> 1 ^ 0xEDB88320u;
    }
    else {
        v4 = v3 >> 1;
    }
    if ((v4 & 1) != 0) {
        v5 = v4 >> 1 ^ 0xEDB88320u;
    }
    else {
        v5 = v4 >> 1;
    }
    if ((v5 & 1) != 0) {
        v6 = v5 >> 1 ^ 0xEDB88320u;
    }
    else {
        v6 = v5 >> 1;
    }
    if ((v6 & 1) != 0) {
        v7 = v6 >> 1 ^ 0xEDB88320u;
    }
    else {
        v7 = v6 >> 1;
    }
    if ((v7 & 1) != 0) {
        return v7 >> 1 ^ 0xEDB88320u;
    }

    return v7 >> 1;
}

hkCrc32StreamWriter::hkCrc32StreamWriter(uint32_t startCrc) : m_crc32(~startCrc) {}

hkBool hkCrc32StreamWriter::isOk() const {
    return 1;
}

// Each byte's table entry comes from calcPermute (the CRC-32 step), not from a table.
int32_t hkCrc32StreamWriter::write(const void* ptr, int32_t nbytes) {
    const uint8_t* pBuffer = static_cast<const uint8_t*>(ptr);
    uint32_t uiCrc = m_crc32;
    for (int32_t i = 0; i < nbytes; ++i) {
        uiCrc = calcPermute((uiCrc ^ pBuffer[i]) & 0xFF) ^ (uiCrc >> 8);
    }
    m_crc32 = uiCrc;
    return nbytes;
}

uint32_t hkCrc32StreamWriter::getCrc32() const {
    return ~m_crc32;
}
