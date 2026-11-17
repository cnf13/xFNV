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

bool NiNIFImageReader::ReadHeader(NiFile& a1, uint32_t& a2, uint32_t& a3, NiPixelFormat& a4, bool& a5, uint32_t& a6) {
    NiPixelData* v7 = ReadFile(a1, nullptr);
    if (!v7) {
        return false;
    }
    NiIncRefCount(v7);
    a2 = *v7->m_puiWidth;
    a3 = *v7->m_puiHeight;
    a4 = v7->m_kPixelFormat;
    a5 = v7->m_uiMipmapLevels > 1;
    a6 = v7->m_uiFaces;
    NiDecRefCount(v7);
    return true;
}
