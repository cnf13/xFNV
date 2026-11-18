#include "Havok/Source/Common/Base/System/Io/Writer/OffsetOnly/hkOffsetOnlyStreamWriter.h"

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

hkOffsetOnlyStreamWriter::hkOffsetOnlyStreamWriter() : m_offset(0), m_eofOffset(0) {}

hkOffsetOnlyStreamWriter::~hkOffsetOnlyStreamWriter() {}

hkBool hkOffsetOnlyStreamWriter::isOk() const {
    return 1;
}

int32_t hkOffsetOnlyStreamWriter::write(const void* apBuffer, int32_t n) {
    int32_t result = n;
    this->m_offset += n;
    int32_t m_offset = this->m_offset;
    int32_t m_eofOffset = this->m_eofOffset;
    if (m_offset <= m_eofOffset) {
        this->m_eofOffset = m_eofOffset;
    }
    else {
        this->m_eofOffset = m_offset;
    }
    return result;
}

hkBool hkOffsetOnlyStreamWriter::seekTellSupported() const {
    return 1;
}

// Moves the write offset.
hkResult hkOffsetOnlyStreamWriter::seek(int32_t offset, hkStreamWriter::SeekWhence whence) {
    int32_t v3;
    if (whence) {
        if (whence == hkStreamWriter::STREAM_CUR) {
            this->m_offset += offset;
            goto LABEL_8;
        }
        if (whence != hkStreamWriter::STREAM_END) {
            goto LABEL_8;
        }
        v3 = this->m_eofOffset - offset;
    }
    else {
        v3 = offset;
    }
    this->m_offset = v3;
LABEL_8:;
    int32_t m_offset = this->m_offset;
    int32_t m_eofOffset = this->m_eofOffset;
    if (m_offset <= m_eofOffset) {
        this->m_eofOffset = m_eofOffset;
    }
    else {
        this->m_eofOffset = m_offset;
    }
    return HK_SUCCESS;
}

int32_t hkOffsetOnlyStreamWriter::tell() const {
    return m_offset;
}
