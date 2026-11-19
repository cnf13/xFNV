#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/BSDegradePriority.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiRTTI NiPersistentSrcTextureRendererData::ms_RTTI("NiPersistentSrcTextureRendererData", &NiObject::ms_RTTI);

// The base's constructor sets no texture, a zero size, priorities 0xFF and state 0.
NiPersistentSrcTextureRendererData::NiPersistentSrcTextureRendererData()
  : NiTexture::RendererData(nullptr), m_pucPixels(nullptr), m_puiWidth(nullptr), m_puiHeight(nullptr),
    m_puiOffsetInBytes(nullptr), m_uiPadOffsetInBytes(0), m_uiMipmapLevels(0), m_uiPixelStride(0), m_uiFaces(0),
    m_eTargetPlatform(NI_ANY), m_pucPristinePixels(nullptr), m_uiPristineMaxOffsetInBytes(0),
    m_uiPristinePadOffsetInBytes(0), m_spPalette(nullptr) {}

NiPersistentSrcTextureRendererData::~NiPersistentSrcTextureRendererData() {
    m_spPalette = nullptr;
    if (m_pucPixels) {
        _NiFree(m_pucPixels);
        m_puiOffsetInBytes = nullptr;
        m_puiHeight = nullptr;
        m_puiWidth = nullptr;
        m_pucPixels = nullptr;
    }
}

// The mipmap tables are read into locals, the data allocated through the virtual AllocateData, the tables copied in and
// the pixels read. Files before 20.2.0.6 have no pad offset: it is the total offset. In tool mode a pristine copy of
// the pixels is kept. The width, height and offset tables and the platform are read into uninitialised locals, which a
// short read leaves unwritten before they are copied into the members, as the game does. The offset table has 17
// entries.
void NiPersistentSrcTextureRendererData::LoadBinary(NiStream& kStream) {
    uint32_t auiWidth[16];
    uint32_t auiHeight[16];
    uint32_t auiOffsetInBytes[17];
    NiObject::LoadBinary(kStream);
    m_kPixelFormat.LoadBinary(kStream);
    kStream.ReadLinkID();
    NiStreamLoadBinary(kStream, m_uiMipmapLevels);
    NiStreamLoadBinary(kStream, m_uiPixelStride);
    for (uint32_t i = 0; i < m_uiMipmapLevels; i++) {
        NiStreamLoadBinary(kStream, auiWidth[i]);
        NiStreamLoadBinary(kStream, auiHeight[i]);
        NiStreamLoadBinary(kStream, auiOffsetInBytes[i]);
    }
    NiStreamLoadBinary(kStream, auiOffsetInBytes[m_uiMipmapLevels]);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 2, 0, 6)) {
        m_uiPadOffsetInBytes = 0;
        NiStreamLoadBinary(kStream, m_uiPadOffsetInBytes);
    }
    else {
        m_uiPadOffsetInBytes = auiOffsetInBytes[m_uiMipmapLevels];
    }
    NiStreamLoadBinary(kStream, m_uiFaces);
    PlatformID ePlatform;
    uint32_t uiSize = 4;
    // EXE-INLINED none: NiStreamLoadEnum
    kStream.m_pkIstr->BinaryRead(&ePlatform, 4, &uiSize, 1);
    m_eTargetPlatform = ePlatform;
    AllocateData(m_uiMipmapLevels, m_uiFaces, auiOffsetInBytes[m_uiMipmapLevels]);
    uint32_t uiTableBytes = m_uiMipmapLevels * sizeof(uint32_t);
    rt::memcpy(m_puiWidth, auiWidth, uiTableBytes);
    rt::memcpy(m_puiHeight, auiHeight, uiTableBytes);
    rt::memcpy(m_puiOffsetInBytes, auiOffsetInBytes, (m_uiMipmapLevels + 1) * sizeof(uint32_t));
    // The pixels go as one-byte components.
    NiStreamLoadBinary(kStream, m_pucPixels, m_puiOffsetInBytes[m_uiMipmapLevels] * m_uiFaces);
    if (ms_bToolModeStreaming) {
        uint32_t uiMaxOffsetInBytes = m_puiOffsetInBytes[m_uiMipmapLevels];
        uint32_t uiTotalPixelMemory = uiMaxOffsetInBytes * m_uiFaces;
        m_uiPristineMaxOffsetInBytes = uiMaxOffsetInBytes;
        m_uiPristinePadOffsetInBytes = m_uiPadOffsetInBytes;
        m_pucPristinePixels = static_cast<uint8_t*>(_NiMalloc(uiTotalPixelMemory));
        rt::memcpy(m_pucPristinePixels, m_pucPixels, uiTotalPixelMemory);
    }
}

// The base call, NiObject::LinkObject, does nothing.
void NiPersistentSrcTextureRendererData::LinkObject(NiStream& kStream) {
    NiTexture::RendererData::LinkObject(kStream);
    m_spPalette = static_cast<NiPalette*>(kStream.GetObjectFromLinkID());
}

// Registers the object, then its palette.
bool NiPersistentSrcTextureRendererData::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spPalette) {
        m_spPalette->RegisterStreamables(kStream);
    }
    return true;
}

// Writes the base part (the shared empty SaveBinary), the pixel format, the palette link, the mipmap table and the
// pixels. With tool-mode streaming and a complete pristine copy, the pristine sizes and pixels are written in place of
// the live ones.
void NiPersistentSrcTextureRendererData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    bool bToolModeStreamingValid = ms_bToolModeStreaming && m_uiPristineMaxOffsetInBytes != 0 &&
        m_uiPristinePadOffsetInBytes != 0 && m_pucPristinePixels != nullptr;
    m_kPixelFormat.SaveBinary(kStream);
    kStream.SaveLinkID(m_spPalette.m_pObject);
    NiStreamSaveBinary(kStream, m_uiMipmapLevels);
    NiStreamSaveBinary(kStream, m_uiPixelStride);
    for (uint32_t i = 0; i < m_uiMipmapLevels; ++i) {
        NiStreamSaveBinary(kStream, m_puiWidth[i]);
        NiStreamSaveBinary(kStream, m_puiHeight[i]);
        NiStreamSaveBinary(kStream, m_puiOffsetInBytes[i]);
    }
    if (bToolModeStreamingValid) {
        NiStreamSaveBinary(kStream, m_uiPristineMaxOffsetInBytes);
        NiStreamSaveBinary(kStream, m_uiPristinePadOffsetInBytes);
    }
    else {
        NiStreamSaveBinary(kStream, m_puiOffsetInBytes[m_uiMipmapLevels]);
        NiStreamSaveBinary(kStream, m_uiPadOffsetInBytes);
    }
    NiStreamSaveBinary(kStream, m_uiFaces);
    uint32_t uiSize = 4;
    // EXE-INLINED none: NiStreamSaveEnum
    kStream.m_pkOstr->BinaryWrite(&m_eTargetPlatform, 4, &uiSize, 1);
    // The pixels go as one-byte components.
    if (bToolModeStreamingValid) {
        NiStreamSaveBinary(kStream, m_pucPristinePixels, m_uiPristineMaxOffsetInBytes * m_uiFaces);
    }
    else {
        NiStreamSaveBinary(kStream, m_pucPixels, m_puiOffsetInBytes[m_uiMipmapLevels] * m_uiFaces);
    }
}

// Equal when the other object is one of this class with an equal pixel format, an equal palette (when this has
// one), the same mipmap tables, sizes, face count and platform, and the same pixel bytes.
bool NiPersistentSrcTextureRendererData::IsEqual(NiObject* pkObject) {
    // EXE-INLINED none: NiIsKindOf
    if (!pkObject) {
        return false;
    }
    const NiRTTI* pkRTTI = pkObject->GetRTTI();
    if (!pkRTTI) {
        return false;
    }
    while (pkRTTI != &NiPersistentSrcTextureRendererData::ms_RTTI) {
        pkRTTI = pkRTTI->GetBaseRTTI();
        if (!pkRTTI) {
            return false;
        }
    }
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }
    NiPersistentSrcTextureRendererData* pkOther = static_cast<NiPersistentSrcTextureRendererData*>(pkObject);
    if (m_kPixelFormat != pkOther->m_kPixelFormat) {
        return false;
    }
    NiPalette* pkPalette = m_spPalette.m_pObject;
    if (pkPalette && !pkPalette->IsEqual(pkOther->m_spPalette.m_pObject)) {
        return false;
    }
    if (m_uiMipmapLevels != pkOther->m_uiMipmapLevels || m_uiPixelStride != pkOther->m_uiPixelStride) {
        return false;
    }
    for (uint32_t i = 0; i < m_uiMipmapLevels; i++) {
        if (m_puiWidth[i] != pkOther->m_puiWidth[i] || m_puiHeight[i] != pkOther->m_puiHeight[i] ||
            m_puiOffsetInBytes[i] != pkOther->m_puiOffsetInBytes[i]) {
            return false;
        }
    }
    if (m_puiOffsetInBytes[m_uiMipmapLevels] != pkOther->m_puiOffsetInBytes[m_uiMipmapLevels] ||
        m_uiPadOffsetInBytes != pkOther->m_uiPadOffsetInBytes || m_uiFaces != pkOther->m_uiFaces ||
        m_eTargetPlatform != pkOther->m_eTargetPlatform) {
        return false;
    }
    // Compares the pixel bytes up to the first difference.
    // EXE-INLINED none: memcmp
    uint32_t uiBytes = m_puiOffsetInBytes[m_uiMipmapLevels];
    const uint8_t* pucPixels = m_pucPixels;
    const uint8_t* pucOtherPixels = pkOther->m_pucPixels;
    for (uint32_t i = 0; i < uiBytes; i++) {
        if (pucPixels[i] != pucOtherPixels[i]) {
            return false;
        }
    }
    return true;
}

void NiPersistentSrcTextureRendererData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiPersistentSrcTextureRendererData::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* ViewerString_9 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, ViewerString_9);
    char* ViewerString_6 = NiGetViewerString("Width", *m_puiWidth);
    uint32_t v8 = pkStrings->m_usSize;
    uint32_t v9 = pkStrings->m_usMaxSize;
    ViewerString_9 = ViewerString_6;
    if (v8 >= v9) {
        pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v8, ViewerString_9);
    char* v10 = NiGetViewerString("Height", *m_puiHeight);
    uint32_t v11 = pkStrings->m_usSize;
    ViewerString_9 = v10;
    if (v11 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v11 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v11, ViewerString_9);
    char* v12 = NiGetViewerString("MipmapLevels", m_uiMipmapLevels);
    uint32_t v13 = pkStrings->m_usSize;
    uint32_t v14 = pkStrings->m_usMaxSize;
    ViewerString_9 = v12;
    if (v13 >= v14) {
        pkStrings->SetSize(v13 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v13, ViewerString_9);
    char* v15 = NiGetViewerString("Faces", m_uiFaces);
    uint32_t v16 = pkStrings->m_usSize;
    uint32_t v17 = pkStrings->m_usMaxSize;
    ViewerString_9 = v15;
    if (v16 >= v17) {
        pkStrings->SetSize(v16 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v16, ViewerString_9);
    char* v18 = NiGetViewerString("TotalSizeInBytes", m_uiFaces * m_puiOffsetInBytes[m_uiMipmapLevels]);
    uint32_t v19 = pkStrings->m_usSize;
    uint32_t v20 = pkStrings->m_usMaxSize;
    ViewerString_9 = v18;
    if (v19 >= v20) {
        pkStrings->SetSize(v19 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v19, ViewerString_9);
    char* v21 = NiGetViewerString("FaceSizeInBytes", m_puiOffsetInBytes[m_uiMipmapLevels]);
    uint32_t v22 = pkStrings->m_usSize;
    ViewerString_9 = v21;
    if (v22 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v22 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v22, ViewerString_9);
    switch (m_eTargetPlatform) {
        case NiPersistentSrcTextureRendererData::NI_ANY: {
            ViewerString_9 = NiGetViewerString("TargetPlatform", "NI_ANY - Generic (default)");
            pkStrings->Add(ViewerString_9);
            break;
        }
        case NiPersistentSrcTextureRendererData::NI_XENON: {
            ViewerString_9 = NiGetViewerString("TargetPlatform", "NI_XENON");
            pkStrings->Add(ViewerString_9);
            break;
        }
        case NiPersistentSrcTextureRendererData::NI_PS3: {
            ViewerString_9 = NiGetViewerString("TargetPlatform", "NI_PS3");
            pkStrings->Add(ViewerString_9);
            break;
        }
        case NiPersistentSrcTextureRendererData::NI_DX9: {
            ViewerString_9 = NiGetViewerString("TargetPlatform", "NI_DX9");
            pkStrings->Add(ViewerString_9);
            break;
        }
        default: {
            char* v23 = NiGetViewerString("TargetPlatform", "UNKNOWN!");
            uint32_t v24 = pkStrings->m_usSize;
            uint32_t v25 = pkStrings->m_usMaxSize;
            ViewerString_9 = v23;
            if (v24 >= v25) {
                pkStrings->SetSize(v24 + pkStrings->m_usGrowBy);
            }
            pkStrings->SetAt(v24, ViewerString_9);
            break;
        }
    }
}

// One untyped _NiMalloc block holds the pixels (each face's bytes rounded up to four) followed by the width, height
// and offset tables; the offset table has one entry more than there are levels.
void NiPersistentSrcTextureRendererData::AllocateData(uint32_t uiLevels, uint32_t uiFaces, uint32_t uiBytes) {
    uint32_t uiPixelBytes = ((uiBytes + 3) & ~3u) * uiFaces;
    m_pucPixels = static_cast<uint8_t*>(_NiMalloc(uiPixelBytes + 12 * uiLevels + 4));
    m_puiWidth = reinterpret_cast<uint32_t*>(m_pucPixels + uiPixelBytes);
    m_puiHeight = m_puiWidth + uiLevels;
    m_puiOffsetInBytes = m_puiHeight + uiLevels;
}

void NiPersistentSrcTextureRendererData::FreeData() {
    if (m_pucPixels) {
        _NiFree(m_pucPixels);
        m_puiOffsetInBytes = (uint32_t*)nullptr;
        m_puiHeight = (uint32_t*)nullptr;
        m_puiWidth = (uint32_t*)nullptr;
        m_pucPixels = (uint8_t*)nullptr;
    }
}

void NiPersistentSrcTextureRendererData::ResetStreamingFunctions() {
    NiStream::UnregisterLoader("NiPersistentSrcTextureRendererData");
    NiStream::RegisterLoader("NiPersistentSrcTextureRendererData", &NiPersistentSrcTextureRendererData::CreateObject);
}

NiObject* NiPersistentSrcTextureRendererData::CreateObject() {
    NiPersistentSrcTextureRendererData* v0 = static_cast<NiPersistentSrcTextureRendererData*>(
        NiMemObject::operator new(sizeof(NiPersistentSrcTextureRendererData))
    );
    if (v0) {
        return ::new (v0) NiPersistentSrcTextureRendererData;
    }

    return nullptr;
}
