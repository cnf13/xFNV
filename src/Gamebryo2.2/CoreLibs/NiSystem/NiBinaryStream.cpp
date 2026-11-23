#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStreamMacro.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiEndian.h"

NiBinaryStream::NiBinaryStream() : m_uiAbsoluteCurrentPos(0), m_pfnRead(nullptr), m_pfnWrite(nullptr) {}

NiBinaryStream::~NiBinaryStream() {}

uint32_t NiBinaryStream::Read(void* pvBuffer, uint32_t uiBytes) {
    uint32_t uiSize = 1;
    return BinaryRead(pvBuffer, uiBytes, &uiSize, 1);
}

uint32_t NiBinaryStream::GetLine(char* pBuffer, uint32_t uiMaxBytes) {
    uint32_t uiNext = 1;
    uint32_t uiBytesRead = 0;
    uint32_t uiLength = 0;
    if (uiMaxBytes > 1) {
        for (;;) {
            char c;
            uint32_t uiComponentSize = 1;
            uint32_t uiRead = m_pfnRead(this, &c, 1, &uiComponentSize, 1);
            m_uiAbsoluteCurrentPos += uiRead;
            uiBytesRead += uiRead;
            if (uiRead != 1 || c == '\n') {
                break;
            }
            if (c != '\r') {
                pBuffer[uiNext - 1] = c;
                ++uiLength;
                ++uiNext;
            }
            if (uiNext >= uiMaxBytes) {
                break;
            }
        }
    }
    pBuffer[uiLength] = '\0';
    return uiBytesRead;
}

// Returns the number of successful one-byte writes.
uint32_t NiBinaryStream::PutS(const char* pBuffer) {
    uint32_t uiLength = 0;
    while (*pBuffer) {
        uint32_t uiComponentSize = 1;
        uint32_t uiWritten = m_pfnWrite(this, pBuffer, 1, &uiComponentSize, 1);
        m_uiAbsoluteCurrentPos += uiWritten;
        ++pBuffer;
        if (uiWritten != 1) {
            break;
        }
        ++uiLength;
    }
    return uiLength;
}

bool NiBinaryStream::GetEndianMatchHint() {
    return ms_bEndianMatchHint;
}

// Swaps the byte order of each component of the buffer in place: the components' sizes repeat until uiBytes are done.
void NiBinaryStream::DoByteSwap(
    void* pvData,
    uint32_t uiTotalBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    char* pcData = static_cast<char*>(pvData);
    uint32_t uiSize = 0;
    while (uiSize < uiTotalBytes) {
        for (uint32_t j = 0; j < uiNumComponents; ++j) {
            uint32_t uiCompSize = puiComponentSizes[j];
            switch (uiCompSize) {
                case 2u: {
                    NiEndian::Swap16(pcData, 1);
                    break;
                }
                case 4u: {
                    NiEndian::Swap32(pcData, 1);
                    break;
                }
                case 8u: {
                    NiEndian::Swap64(pcData, 1);
                    break;
                }
                case 16u:
                case 64u: {
                    NiEndian::Swap32(pcData, uiCompSize >> 2);
                    break;
                }
                default: {
                    break;
                }
            }
            uiSize += uiCompSize;
            pcData += uiCompSize;
        }
    }
}

void NiBinaryStream::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
