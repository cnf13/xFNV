#include "Havok/Source/Common/Base/System/Io/Reader/Buffered/hkBufferedStreamReader.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

hkBufferedStreamReader::Buffer::Buffer(int32_t aiCapacity)
  : begin(static_cast<char*>(hkMemoryRouter::alignedAlloc(*hkMemoryRouter::getInstance().m_heap, aiCapacity, 64))),
    current(0), size(0), capacity(aiCapacity), markPos(-1), markLimit(-1) {}

hkBufferedStreamReader::Buffer::~Buffer() {
    hkMemoryRouter::alignedFree(*hkMemoryRouter::getInstance().m_heap, begin);
}

hkBufferedStreamReader::hkBufferedStreamReader(hkStreamReader* s, int32_t bufSize) : m_stream(s), m_buf(bufSize) {
    m_stream->addReference();
}

// The buffer is freed by its own destructor.
hkBufferedStreamReader::~hkBufferedStreamReader() {
    m_stream->removeReference();
}

int32_t hkBufferedStreamReader::read(void* buf, int32_t nbytes) {
    int32_t v3 = nbytes;
    int32_t current = m_buf.current;
    int32_t v7 = m_buf.size - current;
    while (v3 > v7) {
        hkString::memCpy(buf, &m_buf.begin[current], v7);
        m_buf.current += v7;
        buf = static_cast<char*>(buf) + v7;
        v3 -= v7;
        if (this->refillBuffer()) {
            return nbytes - v3;
        }
        current = m_buf.current;
        v7 = m_buf.size - current;
    }
    hkString::memCpy(buf, &m_buf.begin[m_buf.current], v3);
    m_buf.current += v3;
    return nbytes;
}

int32_t hkBufferedStreamReader::skip(int32_t nbytes) {
    int32_t v3 = m_buf.size - m_buf.current;
    int32_t v4 = nbytes;
    while (v4 > v3) {
        v4 -= v3;
        if (refillBuffer()) {
            return nbytes - v4;
        }
        v3 = m_buf.size - m_buf.current;
    }
    m_buf.current += v4;
    return nbytes;
}

// Buffered bytes left mean the reader is still good; otherwise the underlying stream decides.
hkBool hkBufferedStreamReader::isOk() const {
    if (m_buf.current != m_buf.size || m_stream->isOk().m_bool) {
        return 1;
    }

    return 0;
}

hkBool hkBufferedStreamReader::markSupported() const {
    return m_buf.capacity != 0;
}

hkResult hkBufferedStreamReader::setMark(int32_t markLimit) {
    m_buf.markPos = m_buf.current;
    m_buf.markLimit = markLimit;
    return markLimit > m_buf.capacity ? HK_FAILURE : HK_SUCCESS;
}

hkResult hkBufferedStreamReader::rewindToMark() {
    int32_t v1 = m_buf.markPos;
    if (v1 < 0) {
        return HK_FAILURE;
    }
    m_buf.current = v1;
    return HK_SUCCESS;
}

hkBool hkBufferedStreamReader::seekTellSupported() const {
    return m_stream->seekTellSupported();
}

hkResult hkBufferedStreamReader::seek(int32_t offset, hkStreamReader::SeekWhence whence) {
    m_buf.markPos = -1;
    m_buf.markLimit = -1;
    m_buf.current = 0;
    m_buf.size = 0;
    return m_stream->seek(offset, whence);
}

int32_t hkBufferedStreamReader::tell() const {
    int32_t v2 = m_stream->tell();
    if (v2 < 0) {
        return -1;
    }

    return v2 + (m_buf.current - m_buf.size);
}

hkResult hkBufferedStreamReader::refillBuffer() {
    if (!m_stream->isOk().m_bool) {
        return HK_FAILURE;
    }
    prepareBufferForRefill();
    int32_t v3 = m_buf.capacity - m_buf.size;
    int32_t v4 = 0;
    if (v3 <= 0) {
        return HK_SUCCESS;
    }
    while (true) {
        int32_t v5 = m_stream->read(&m_buf.begin[m_buf.current], v3);
        m_buf.size += v5;
        v4 += v5;
        if (v5 != v3) {
            break;
        }
        if (v4 >= v3) {
            return HK_SUCCESS;
        }
    }
    return static_cast<hkResult>(v4 == 0);
}

void hkBufferedStreamReader::prepareBufferForRefill() {
    int32_t markPos = m_buf.markPos;
    int32_t v3 = 0;
    if (markPos >= 0) {
        int32_t v4 = m_buf.current - markPos;
        if (v4 <= m_buf.markLimit) {
            if (markPos > 0) {
                int32_t v5 = v4 % 512;
                if (v4 % 512) {
                    v3 = 512 - v5;
                }
                hkString::memMove(&m_buf.begin[v3], &m_buf.begin[markPos], v4);
                int32_t v6 = ((v5 != 0) + v4 / 512) << 9;
                m_buf.markPos = v3;
                m_buf.current = v6;
                m_buf.size = v6;
            }
        }
        else {
            m_buf.current = 0;
            m_buf.size = 0;
            m_buf.markPos = -1;
            m_buf.markLimit = -1;
        }
    }
    else {
        m_buf.current = 0;
        m_buf.size = 0;
    }
}
