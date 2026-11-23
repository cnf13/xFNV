#include "Gamebryo2.2/CoreLibs/NiMain/NiNIFImageReader.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiNIFImageReader::NiNIFImageReader() {}

// The stream is destroyed by the compiler, the critical section by NiImageReader.
NiNIFImageReader::~NiNIFImageReader() {}

bool NiNIFImageReader::CanReadFile(const char* pcFileExtension) const {
    return NiStricmp(pcFileExtension, ".nif") == 0;
}

// Loads the NIF through the reader's own stream and returns its single top object when that is pixel data. pkOptDest is
// never read. The stream is a member object, so Load goes through a base pointer to keep the call indirect.
NiPixelData* NiNIFImageReader::ReadFile(NiFile& kIst, NiPixelData* pkOptDest) {
    m_kReadCriticalSection.Lock();
    m_kStream.RemoveAllObjects();
    NiStream* pkStream = &m_kStream;
    pkStream->Load(&kIst);
    if (m_kStream.m_kTopObjects.m_uiSize == 1) {
        NiObject* pkObject = m_kStream.m_kTopObjects.m_pBase[0].m_pObject;
        if (pkObject) {
            for (const NiRTTI* pkRTTI = pkObject->GetRTTI(); pkRTTI; pkRTTI = pkRTTI->m_pkBaseRTTI) {
                if (pkRTTI == &NiPixelData::ms_RTTI) {
                    m_kReadCriticalSection.Unlock();
                    return static_cast<NiPixelData*>(pkObject);
                }
            }
        }
    }
    m_kReadCriticalSection.Unlock();
    return nullptr;
}

bool NiNIFImageReader::ReadHeader(
    NiFile& kIst,
    uint32_t& uiWidth,
    uint32_t& uiHeight,
    NiPixelFormat& kFormat,
    bool& bMipmap,
    uint32_t& uiFaces
) {
    NiPixelData* v7 = ReadFile(kIst, nullptr);
    if (!v7) {
        return false;
    }
    NiIncRefCount(v7);
    uiWidth = *v7->m_puiWidth;
    uiHeight = *v7->m_puiHeight;
    kFormat = v7->m_kPixelFormat;
    bMipmap = v7->m_uiMipmapLevels > 1;
    uiFaces = v7->m_uiFaces;
    NiDecRefCount(v7);
    return true;
}
