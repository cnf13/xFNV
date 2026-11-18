#include "Havok/Source/Common/Base/System/Io/Reader/Stdio/hkStdioStreamReader.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/Reader/Seekable/hkSeekableStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/MSVC/stdio.h"

// The mark position starts at -1 in hkSeekableStreamReader's constructor.
hkStdioStreamReader::hkStdioStreamReader(const char* nameIn) : m_handle(rt::fopen(nameIn, "rb")) {
    m_isOk.m_bool = m_handle != nullptr;
}

hkStdioStreamReader::~hkStdioStreamReader() {
    if (m_handle) {
        std::fclose(m_handle);
    }
}

// Reads aiNumBytes single bytes; a read of nothing marks the reader bad. The compare is signed.
int32_t hkStdioStreamReader::read(void* buf, int32_t nbytes) {
    int32_t result = static_cast<int32_t>(std::fread(buf, 1, static_cast<size_t>(nbytes), m_handle));
    if (result <= 0) {
        m_isOk.m_bool = 0;
    }
    return result;
}

hkBool hkStdioStreamReader::isOk() const {
    return (int8_t)((uint8_t)m_isOk.m_bool);
}

hkBool hkStdioStreamReader::markSupported() const {
    return 1;
}

hkBool hkStdioStreamReader::seekTellSupported() const {
    return 1;
}

hkResult hkStdioStreamReader::seek(int32_t offset, hkStreamReader::SeekWhence whence) {
    return static_cast<hkResult>((uint8_t)(std::fseek(m_handle, offset, whence) != 0));
}

int32_t hkStdioStreamReader::tell() const {
    return std::ftell(m_handle);
}

hkBool hkSeekableStreamReader::markSupported() const {
    return 1;
}

hkResult hkSeekableStreamReader::setMark(int32_t markLimit) {
    int32_t v3 = tell();
    m_markPos = v3;
    return static_cast<hkResult>(v3 == -1);
}

hkResult hkSeekableStreamReader::rewindToMark() {
    return seek(m_markPos, hkStreamReader::STREAM_SET);
}

hkBool hkSeekableStreamReader::seekTellSupported() const {
    return 1;
}
