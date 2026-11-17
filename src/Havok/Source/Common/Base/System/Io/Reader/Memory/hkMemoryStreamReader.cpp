#include "Havok/Source/Common/Base/System/Io/Reader/Memory/hkMemoryStreamReader.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

// MEMORY_COPY copies the bytes into a buffer of its own; the other kinds read mem itself.
hkMemoryStreamReader::hkMemoryStreamReader(const void* mem, int32_t memSize, hkMemoryStreamReader::MemoryType t)
  : m_bufCurrent(0), m_bufSize(memSize), m_markPos(-1), m_memType(t) {
    if (t != MEMORY_COPY) {
        m_buf = static_cast<char*>(const_cast<void*>(mem));
    }
    else {
        m_buf = static_cast<char*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, memSize));
        hkString::memCpy(m_buf, mem, memSize);
    }
}

hkBool* hkMemoryStreamReader::isOk(hkBool* arResult) const {
    arResult->m_bool = m_bufCurrent != m_bufSize + 1;
    return arResult;
}

int32_t hkMemoryStreamReader::read(void* buf, int32_t nbytes) {
    hkBool v7;
    if (!isOk(&v7)->m_bool) {
        return 0;
    }
    int32_t m_bufCurrent = this->m_bufCurrent;
    int32_t v5 = nbytes;
    if (nbytes >= m_bufSize - m_bufCurrent) {
        v5 = m_bufSize - m_bufCurrent;
    }
    hkString::memCpy(buf, &m_buf[m_bufCurrent], v5);
    this->m_bufCurrent += v5;
    if ((!v5) && (nbytes)) {
        this->m_bufCurrent = m_bufSize + 1;
    }

    return v5;
}

int32_t hkMemoryStreamReader::skip(int32_t nbytes) {
    hkBool v6;
    if (!isOk(&v6)->m_bool) {
        return 0;
    }
    int32_t m_bufSize = this->m_bufSize;
    int32_t m_bufCurrent = this->m_bufCurrent;
    int32_t result = m_bufSize - m_bufCurrent;
    if (nbytes < m_bufSize - m_bufCurrent) {
        result = nbytes;
    }
    this->m_bufCurrent = result + m_bufCurrent;
    if ((!result) && (nbytes)) {
        this->m_bufCurrent = m_bufSize + 1;
    }

    return result;
}

hkBool* hkMemoryStreamReader::markSupported(hkBool* arResult) const {
    arResult->m_bool = m_bufSize != 0;
    return arResult;
}

hkResult hkMemoryStreamReader::setMark(int32_t markLimit) {
    m_markPos = m_bufCurrent;
    return HK_SUCCESS;
}

hkResult hkMemoryStreamReader::rewindToMark() {
    int32_t iMarkPos = m_markPos;
    if (iMarkPos < 0) {
        return HK_FAILURE;
    }
    m_bufCurrent = iMarkPos;
    return HK_SUCCESS;
}

hkBool* hkMemoryStreamReader::seekTellSupported(hkBool* arResult) const {
    arResult->m_bool = 1;
    return arResult;
}

hkResult hkMemoryStreamReader::seek(int32_t relOffset, SeekWhence whence) {
    int32_t v5;
    int32_t v3 = (-1);
    if (whence) {
        if (whence == 1) {
            v3 = relOffset + m_bufCurrent;
        }
        else {
            if (whence == 2) {
                v3 = m_bufSize - relOffset;
            }
        }
    }
    else {
        v3 = relOffset;
    }
    hkResult result = HK_SUCCESS;
    if (v3 >= 0) {
        v5 = m_bufSize;
        if (v3 <= v5) {
            m_bufCurrent = v3;
        }
        else {
            m_bufCurrent = v5;
            return HK_FAILURE;
        }
    }
    else {
        m_bufCurrent = 0;
        return HK_FAILURE;
    }
    return result;
}

int32_t hkMemoryStreamReader::tell() const {
    return m_bufCurrent;
}

hkMemoryStreamReader::~hkMemoryStreamReader() {
    if (m_memType == MEMORY_COPY || m_memType == MEMORY_TAKE) {
        hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, m_buf);
    }
}
