#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemStream.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiMemStream::NiMemStream()
  : m_pBuffer(static_cast<char*>(_NiMalloc(1024u))), m_uiPos(0), m_uiEnd(0), m_uiAllocSize(1024), m_bUserMemory(false),
    m_bFreeze(false) {
    m_pfnRead = &NiMemStream::ReadNoSwap;
    m_pfnWrite = &NiMemStream::WriteNoSwap;
}

NiMemStream::NiMemStream(void* apBuffer, uint32_t auiSize)
  : m_pBuffer(static_cast<char*>(apBuffer)), m_uiPos(0), m_uiEnd(auiSize), m_uiAllocSize(auiSize), m_bUserMemory(true),
    m_bFreeze(false) {
    m_pfnRead = &NiMemStream::ReadNoSwap;
    m_pfnWrite = &NiMemStream::WriteNoSwap;
}

NiMemStream::~NiMemStream() {
    if (!m_bUserMemory && !m_bFreeze) {
        _NiFree(m_pBuffer);
    }
}

// A memory stream is always good.
NiMemStream::operator bool() const {
    return true;
}

void NiMemStream::Seek(int32_t iNumBytes) {
    int32_t v2 = iNumBytes + ((int32_t)m_uiPos);
    if ((v2 >= 0) && (((uint32_t)v2) < ((uint32_t)((int32_t)m_uiAllocSize)))) {
        m_uiPos = (uint32_t)v2;
        m_uiAbsoluteCurrentPos = (uint32_t)v2;
    }
}

void NiMemStream::SetEndianSwap(bool bDoSwap) {
    if (bDoSwap) {
        m_pfnRead = &NiMemStream::ReadAndSwap;
        m_pfnWrite = &NiMemStream::WriteAndSwap;
    }
    else {
        m_pfnRead = &NiMemStream::ReadNoSwap;
        m_pfnWrite = &NiMemStream::WriteNoSwap;
    }
}

void* NiMemStream::Str() {
    char* result = m_pBuffer;
    m_bFreeze = true;
    return result;
}

uint32_t NiMemStream::MemWrite(const void* pBuffer, uint32_t uiBytes) {
    if (m_bUserMemory) {
        return 0;
    }
    uint32_t m_uiAllocSize = this->m_uiAllocSize;
    uint32_t v6 = m_uiAllocSize - this->m_uiPos;
    if (uiBytes > v6) {
        uint32_t v7;
        if (uiBytes > m_uiAllocSize + v6) {
            v7 = uiBytes + (m_uiAllocSize - v6);
        }
        else {
            v7 = 2 * m_uiAllocSize;
        }
        void* v8 = _NiMalloc(v7);
        rt::memcpy(v8, m_pBuffer, m_uiEnd);
        _NiFree(m_pBuffer);
        m_pBuffer = static_cast<char*>(v8);
        this->m_uiAllocSize = v7;
    }
    rt::memcpy(&m_pBuffer[this->m_uiPos], pBuffer, uiBytes);
    this->m_uiPos += uiBytes;
    uint32_t m_uiPos = this->m_uiPos;
    if (m_uiPos > m_uiEnd) {
        m_uiEnd = m_uiPos;
    }
    return uiBytes;
}

// Copies up to uiBytes from the current position, clamped to the end; returns the bytes read.
uint32_t NiMemStream::ReadNoSwap(NiBinaryStream* pkThis, void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t) {
    NiMemStream* pkStream = static_cast<NiMemStream*>(pkThis);
    uint32_t uiAvailable = pkStream->m_uiEnd - pkStream->m_uiPos;
    if (uiBytes > uiAvailable) {
        uiBytes = uiAvailable;
    }
    rt::memcpy(pvBuffer, pkStream->m_pBuffer + pkStream->m_uiPos, uiBytes);
    pkStream->m_uiPos += uiBytes;
    return uiBytes;
}

uint32_t NiMemStream::WriteNoSwap(NiBinaryStream* pkThis, const void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t) {
    return static_cast<NiMemStream*>(pkThis)->MemWrite(pvBuffer, uiBytes);
}

// Reads like ReadNoSwap, then swaps the whole requested size in place (uiBytes, not the bytes read).
uint32_t NiMemStream::ReadAndSwap(
    NiBinaryStream* pkThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    if (uiBytes == 0) {
        return 0;
    }
    NiMemStream* pkStream = static_cast<NiMemStream*>(pkThis);
    uint32_t uiRead = pkStream->m_uiEnd - pkStream->m_uiPos;
    if (uiBytes <= uiRead) {
        uiRead = uiBytes;
    }
    rt::memcpy(pvBuffer, pkStream->m_pBuffer + pkStream->m_uiPos, uiRead);
    pkStream->m_uiPos += uiRead;
    DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
    return uiRead;
}

// Swaps a temporary copy of pvBuffer and writes that.
uint32_t NiMemStream::WriteAndSwap(
    NiBinaryStream* pkThis,
    const void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    if (uiBytes == 0) {
        return 0;
    }
    void* pvSwapped = _NiMalloc(uiBytes);
    rt::memcpy(pvSwapped, pvBuffer, uiBytes);
    DoByteSwap(pvSwapped, uiBytes, puiComponentSizes, uiNumComponents);
    uint32_t uiWritten = static_cast<NiMemStream*>(pkThis)->MemWrite(pvSwapped, uiBytes);
    _NiFree(pvSwapped);
    return uiWritten;
}
