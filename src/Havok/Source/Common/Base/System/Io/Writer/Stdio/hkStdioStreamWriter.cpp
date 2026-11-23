#include "Havok/Source/Common/Base/System/Io/Writer/Stdio/hkStdioStreamWriter.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/stdio.h"

hkStdioStreamWriter::hkStdioStreamWriter(hkFileSystem::StdStream s)
  : m_handle(s == hkFileSystem::STDERR ? stderr : stdout), m_owned(false) {}

hkStdioStreamWriter::hkStdioStreamWriter(const char* nameIn) : m_handle(nullptr), m_owned(true) {
    if (nameIn) {
        open(nameIn, "wb");
    }
}

hkStdioStreamWriter::~hkStdioStreamWriter() {
    close();
}

hkBool hkStdioStreamWriter::isOk() const {
    return m_handle != nullptr;
}

int32_t hkStdioStreamWriter::write(const void* buf, int32_t nbytes) {
    std::FILE* m_handle = this->m_handle;
    if (!m_handle) {
        return 0;
    }
    int32_t v5 = std::fwrite(buf, 1u, nbytes, m_handle);
    if (v5 <= 0) {
        close();
    }
    return v5;
}

// Flushes the stream when the writer has one.
void hkStdioStreamWriter::flush() {
    if (m_handle) {
        std::fflush(m_handle);
    }
}

hkBool hkStdioStreamWriter::seekTellSupported() const {
    return 1;
}

hkResult hkStdioStreamWriter::seek(int32_t offset, hkStreamWriter::SeekWhence whence) {
    return static_cast<hkResult>(std::fseek(m_handle, offset, whence) != 0);
}

int32_t hkStdioStreamWriter::tell() const {
    return std::ftell(m_handle);
}

hkBool hkStdioStreamWriter::markSupported() const {
    return 1;
}

void hkStdioStreamWriter::close() {
    if (m_handle && m_owned.m_bool) {
        std::fclose(m_handle);
    }

    m_handle = nullptr;
}

void hkStdioStreamWriter::open(const char* nameIn, const char* mode) {
    m_handle = rt::fopen(nameIn, mode);
}
