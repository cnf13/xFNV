#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

hkClass hkBufferedStreamWriterClass(
    "hkBufferedStreamWriter",
    nullptr,
    28,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);

hkBufferedStreamWriter::hkBufferedStreamWriter(void* apBuffer, int32_t aiBufferSize, hkBool aMemoryIsString)
  : m_stream(nullptr), m_buf(static_cast<char*>(apBuffer)), m_bufSize(0),
    m_bufCapacity(aMemoryIsString.m_bool ? aiBufferSize - 1 : aiBufferSize), m_ownBuffer(false) {
    // A C string keeps its last byte for the terminator; the block starts zeroed.
    if (aMemoryIsString.m_bool) {
        hkString::memSet(apBuffer, 0, aiBufferSize);
    }
}

hkBufferedStreamWriter::hkBufferedStreamWriter(hkStreamWriter* apStream, int32_t aiBufSize)
  : m_stream(apStream),
    m_buf(static_cast<char*>(hkMemoryRouter::alignedAlloc(*hkMemoryRouter::getInstance().m_heap, aiBufSize, 64))),
    m_bufSize(0), m_bufCapacity(aiBufSize), m_ownBuffer(true) {
    if (m_stream) {
        m_stream->addReference();
    }
}

hkBufferedStreamWriter::~hkBufferedStreamWriter() {
    flush();
    if (m_stream) {
        m_stream->removeReference();
    }
    if (m_ownBuffer.m_bool) {
        hkMemoryRouter::alignedFree(*hkMemoryRouter::getInstance().m_heap, m_buf);
    }
}

// Whether the stream is ok, or with no stream whether the buffer is not full.
hkBool* hkBufferedStreamWriter::isOk(hkBool* arResult) const {
    hkBool kResult;
    if (m_stream) {
        arResult->m_bool = m_stream->isOk(&kResult)->m_bool;
        return arResult;
    }
    kResult.m_bool = m_bufSize != m_bufCapacity;
    arResult->m_bool = kResult.m_bool;
    return arResult;
}

// Copies the bytes into the buffer, flushing it each time it fills; returns how many bytes were taken, short when a
// flush writes less than the buffer held.
int32_t hkBufferedStreamWriter::write(const void* mem, int32_t memSize) {
    int32_t iLeft = memSize;
    int32_t iSpace = m_bufCapacity - m_bufSize;
    while (iLeft > iSpace) {
        hkString::memCpy(&m_buf[m_bufSize], mem, iSpace);
        m_bufSize += iSpace;
        int32_t iBuffered = m_bufSize;
        mem = static_cast<const char*>(mem) + iSpace;
        iLeft -= iSpace;
        if (flushBuffer() != iBuffered) {
            return memSize - iLeft;
        }
        iSpace = m_bufCapacity - m_bufSize;
    }
    hkString::memCpy(&m_buf[m_bufSize], mem, iLeft);
    m_bufSize += iLeft;
    return memSize;
}

// Writes the buffered bytes out, then flushes the stream they went to.
void hkBufferedStreamWriter::flush() {
    flushBuffer();
    if (m_stream) {
        m_stream->flush();
    }
}

hkBool* hkBufferedStreamWriter::seekTellSupported(hkBool* arResult) const {
    hkBool v4;
    if (m_stream) {
        char m_bool = m_stream->seekTellSupported(&v4)->m_bool;
        arResult->m_bool = m_bool;
        return arResult;
    }
    else {
        arResult->m_bool = 1;
        return arResult;
    }
}

// Moves the write position.
hkResult hkBufferedStreamWriter::seek(int32_t relOffset, hkStreamWriter::SeekWhence whence) {
    hkResult result;
    if (m_stream) {
        flushBuffer();
        return m_stream->seek(relOffset, whence);
    }
    else {
        int32_t m_bufCapacity = -1;
        if (whence) {
            if (whence == hkStreamWriter::STREAM_CUR) {
                m_bufCapacity = relOffset + m_bufSize;
            }
            else {
                if (whence == hkStreamWriter::STREAM_END) {
                    m_bufCapacity = m_bufSize - relOffset;
                }
            }
        }
        else {
            m_bufCapacity = relOffset;
        }
        result = HK_SUCCESS;
        if (m_bufCapacity >= 0) {
            if (m_bufCapacity > this->m_bufCapacity) {
                m_bufCapacity = this->m_bufCapacity;
                result = HK_FAILURE;
            }
            m_bufSize = m_bufCapacity;
        }
        else {
            m_bufSize = 0;
            return HK_FAILURE;
        }
    }
    return result;
}

int32_t hkBufferedStreamWriter::tell() const {
    if (!m_stream) {
        return (int32_t)((uint32_t)m_bufSize);
    }
    int32_t v2 = m_stream->tell();
    if (v2 < 0) {
        return (-1);
    }
    else {
        return (int32_t)(v2 + ((uint32_t)m_bufSize));
    }
}

void hkBufferedStreamWriter::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->addChunk(hkStatisticsCollector::MEM_NORMAL, "Buffer", m_buf, m_bufCapacity, 0);
    collector->addReferencedObject("stream", m_stream, 0);
}

const hkClass* hkBufferedStreamWriter::getClassType() const {
    return &hkBufferedStreamWriterClass;
}

// Writes the buffered bytes to the stream; returns how many.
int32_t hkBufferedStreamWriter::flushBuffer() {
    if (!m_stream) {
        return 0;
    }
    int32_t m_bufSize = this->m_bufSize;
    int32_t v3 = 0;
    if (m_bufSize <= 0) {
    LABEL_5:;
        this->m_bufSize = 0;
    }
    else {
        while (1) {
            int32_t v4 = m_stream->write(&m_buf[v3], m_bufSize - v3);
            v3 += v4;
            if (!v4) {
                break;
            }
            if (v3 >= m_bufSize) {
                goto LABEL_5;
            }
        }
    }
    return v3;
}
