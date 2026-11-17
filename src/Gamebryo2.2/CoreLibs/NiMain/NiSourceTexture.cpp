#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"

NiTexture::FormatPrefs NiSourceTexture::ms_kDefaultPrefs{
    NiTexture::FormatPrefs::PIX_DEFAULT, NiTexture::FormatPrefs::ALPHA_DEFAULT, NiTexture::FormatPrefs::MIP_DEFAULT
};
NiRTTI NiSourceTexture::ms_RTTI("NiSourceTexture", &NiTexture::ms_RTTI);

NiSourceTexture::NiSourceTexture()
  : m_spPersistentSrcRendererData(nullptr), m_spSrcPixelData(nullptr), m_bStatic(true),
    m_bLoadDirectToRendererHint(true), m_bSrcRendererDataIsPersistent(false), cLooked(0), m_pFile(nullptr) {}

NiSourceTexture::~NiSourceTexture() {
    m_spSrcPixelData = nullptr;
    m_spPersistentSrcRendererData = nullptr;
}

// KEEP: niread: the exe reads the pre-10.0.1.4 name and pixel-data flags, the three format enums and the three
// trailing flags into stack slots nothing initialised and branches on them or stores them into fields, so a short
// read (malformed data) leaves stack garbage there, which src cannot express; porting.md's unwritten-slot exception
// covers GetChunkData only. The base call is a direct call to NiTexture's LoadBinary slot body (0xA47350, a jump to
// NiObjectNET's, registered as NiSequenceStreamHelper::LoadBinary).
void NiSourceTexture::LoadBinary(NiStream& kStream) {
    NiTexture::LoadBinary(kStream);
    NiIncRefCount(this);
    uint8_t bExternalTexture = 0;
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 4)) {
        uint8_t bSaveName;
        NiStreamLoadBinary(kStream, bSaveName);
        if (bSaveName) {
            kStream.LoadCStringAsFixedString(m_kFilename);
            m_spSrcPixelData = nullptr;
            bExternalTexture = 1;
            kStream.SetNumberOfLinkIDs(0);
        }
        else {
            m_kFilename = static_cast<const char*>(nullptr);
            uint8_t bSavePixelData;
            NiStreamLoadBinary(kStream, bSavePixelData);
            if (bSavePixelData) {
                kStream.SetNumberOfLinkIDs(1);
                kStream.ReadLinkID();
            }
            else {
                kStream.SetNumberOfLinkIDs(0);
            }
        }
        m_spPersistentSrcRendererData = nullptr;
    }
    else {
        NiStreamLoadBinary(kStream, bExternalTexture);
        if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
            kStream.LoadCStringAsFixedString(m_kFilename);
        }
        else {
            kStream.LoadFixedString(m_kFilename);
        }
        kStream.SetNumberOfLinkIDs(1);
        kStream.ReadLinkID();
    }
    if (m_kFilename.GetHandle()) {
        NiImageConverter::ConvertFilenameToPlatformSpecific(m_kFilename, m_kPlatformFilename);
    }
    if (bExternalTexture) {
        // The search path's SetFilePath and Reset (bUseDefaultPath) are the stream's here.
        char acFile[260];
        kStream.SetFilePath(m_kPlatformFilename.GetHandle());
        NiStream::bUseDefaultPath = true;
        NiPointer<NiTexture> spShared;
        while (!spShared.m_pObject && kStream.GetNextSearchPath(acFile, 260u)) {
            kStream.GetTexture(acFile, spShared);
        }
        if (spShared.m_pObject) {
            m_kPlatformFilename = acFile;
            kStream.ChangeObject(spShared.m_pObject);
        }
        else {
            NiStream::bUseDefaultPath = true;
            while (kStream.GetNextSearchPath(acFile, 260u)) {
                if (NiFile::Access(acFile, NiFile::READ_ONLY)) {
                    m_kPlatformFilename.Set(acFile);
                    kStream.SetTexture(this);
                    break;
                }
            }
        }
    }
    else {
        const char* pcPlatformFilename = m_kPlatformFilename.GetHandle();
        if (pcPlatformFilename != NiGlobalStringTable::NULL_STRING) {
            NiPointer<NiTexture> spShared;
            kStream.GetTexture(pcPlatformFilename, spShared);
            if (!spShared.m_pObject) {
                kStream.SetTexture(this);
            }
            else {
                kStream.ChangeObject(spShared.m_pObject);
            }
        }
    }
    uint32_t uiPixelLayout;
    NiStreamLoadBinary(kStream, uiPixelLayout);
    m_kFormatPrefs.m_ePixelLayout = static_cast<NiTexture::FormatPrefs::PixelLayout>(uiPixelLayout);
    uint32_t uiMipMapped;
    NiStreamLoadBinary(kStream, uiMipMapped);
    m_kFormatPrefs.m_eMipMapped = static_cast<NiTexture::FormatPrefs::MipFlag>(uiMipMapped);
    uint32_t uiAlphaFmt;
    NiStreamLoadBinary(kStream, uiAlphaFmt);
    m_kFormatPrefs.m_eAlphaFmt = static_cast<NiTexture::FormatPrefs::AlphaFormat>(uiAlphaFmt);
    uint32_t uiVersion = kStream.GetFileVersion();
    if (uiVersion < NiStream::GetVersion(3, 3, 0, 12)) {
        if (m_kFormatPrefs.m_ePixelLayout == NiTexture::FormatPrefs::BUMPMAP) {
            m_kFormatPrefs.m_ePixelLayout = NiTexture::FormatPrefs::PIX_DEFAULT;
        }
    }
    else if (
        (uiVersion < NiStream::GetVersion(5, 0, 0, 1)) &&
        (m_kFormatPrefs.m_ePixelLayout == NiTexture::FormatPrefs::PALETTIZED_4)
    ) {
        m_kFormatPrefs.m_ePixelLayout = NiTexture::FormatPrefs::PIX_DEFAULT;
    }

    uint8_t bStatic;
    NiStreamLoadBinary(kStream, bStatic);
    m_bStatic = bStatic != 0;
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 103)) {
        uint8_t bLoadDirectToRendererHint;
        NiStreamLoadBinary(kStream, bLoadDirectToRendererHint);
        m_bLoadDirectToRendererHint = bLoadDirectToRendererHint != 0;
    }
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 2, 0, 4)) {
        uint8_t bSrcRendererDataIsPersistent;
        NiStreamLoadBinary(kStream, bSrcRendererDataIsPersistent);
        m_bSrcRendererDataIsPersistent = bSrcRendererDataIsPersistent != 0;
    }
    NiDecRefCount(this);
}

// Only 20.2.0.4 and later files with the persistent flag link the renderer data; every other file links the source
// pixel data.
void NiSourceTexture::LinkObject(NiStream& kStream) {
    NiTexture::LinkObject(kStream);
    if (!kStream.GetNumberOfLinkIDs()) {
        return;
    }
    if (kStream.m_uiNifFileVersion >= NiStream::GetVersion(20, 2, 0, 4) && m_bSrcRendererDataIsPersistent) {
        m_spPersistentSrcRendererData.SetNew(
            static_cast<NiPersistentSrcTextureRendererData*>(kStream.GetObjectFromLinkID())
        );
        NiPixelData* pkSrcPixelData = m_spSrcPixelData.m_pObject;
        if (pkSrcPixelData) {
            if (!InterlockedDecrementUInt32(&pkSrcPixelData->m_uiRefCount)) {
                NiRefObject* pRefObject = static_cast<NiRefObject*>(pkSrcPixelData);
                pRefObject->DeleteThis();
            }
            m_spSrcPixelData.m_pObject = nullptr;
        }
        return;
    }
    m_spSrcPixelData.SetNew(static_cast<NiPixelData*>(kStream.GetObjectFromLinkID()));
    NiPersistentSrcTextureRendererData* pkRendererData = m_spPersistentSrcRendererData.m_pObject;
    if (pkRendererData) {
        if (!InterlockedDecrementUInt32(&pkRendererData->m_uiRefCount)) {
            NiRefObject* pRefObject = static_cast<NiRefObject*>(pkRendererData);
            pRefObject->DeleteThis();
        }
        m_spPersistentSrcRendererData.m_pObject = nullptr;
    }
}

// Registers the file name, then the persistent renderer data if any, else the source pixel data, and returns that
// call's result.
bool NiSourceTexture::RegisterStreamables(NiStream& kStream) {
    if (!NiTexture::RegisterStreamables(kStream)) {
        return false;
    }
    kStream.RegisterFixedString(m_kFilename);
    if (m_spPersistentSrcRendererData.m_pObject) {
        return m_spPersistentSrcRendererData.m_pObject->RegisterStreamables(kStream);
    }
    if (m_spSrcPixelData.m_pObject) {
        return m_spSrcPixelData.m_pObject->RegisterStreamables(kStream);
    }
    return true;
}

void NiSourceTexture::SaveBinary(NiStream& kStream) {
    NiTexture::SaveBinary(kStream);
    uint8_t bExternalImage = !m_spPersistentSrcRendererData.m_pObject && !m_spSrcPixelData.m_pObject;
    NiStreamSaveBinary(kStream, bExternalImage);
    kStream.SaveFixedString(m_kFilename);
    if (m_bSrcRendererDataIsPersistent) {
        kStream.SaveLinkID(m_spPersistentSrcRendererData.m_pObject);
    }
    else {
        kStream.SaveLinkID(m_spSrcPixelData.m_pObject);
    }
    NiStreamSaveBinary(kStream, m_kFormatPrefs.m_ePixelLayout);
    NiStreamSaveBinary(kStream, m_kFormatPrefs.m_eMipMapped);
    NiStreamSaveBinary(kStream, m_kFormatPrefs.m_eAlphaFmt);
    uint8_t bStatic = m_bStatic;
    NiStreamSaveBinary(kStream, bStatic);
    uint8_t bLoadDirectToRendererHint = m_bLoadDirectToRendererHint;
    NiStreamSaveBinary(kStream, bLoadDirectToRendererHint);
    uint8_t bSrcRendererDataIsPersistent = m_bSrcRendererDataIsPersistent;
    NiStreamSaveBinary(kStream, bSrcRendererDataIsPersistent);
}

// The file names compare by handle; the two data pointers must both be present and equal; then the format preferences
// and the three flags compare.
bool NiSourceTexture::IsEqual(NiObject* pkObject) {
    if (!NiObjectNET::IsEqual(pkObject)) {
        return false;
    }
    NiSourceTexture* pkTexture = static_cast<NiSourceTexture*>(pkObject);
    if (!(m_kFilename == pkTexture->m_kFilename)) {
        return false;
    }
    if (m_spSrcPixelData.m_pObject) {
        if (!pkTexture->m_spSrcPixelData.m_pObject) {
            return false;
        }
        if (!m_spSrcPixelData->IsEqual(pkTexture->m_spSrcPixelData.m_pObject)) {
            return false;
        }
    }
    else if (pkTexture->m_spSrcPixelData.m_pObject) {
        return false;
    }
    if (m_spPersistentSrcRendererData.m_pObject) {
        if (!pkTexture->m_spPersistentSrcRendererData.m_pObject) {
            return false;
        }
        if (!m_spPersistentSrcRendererData->IsEqual(pkTexture->m_spPersistentSrcRendererData.m_pObject)) {
            return false;
        }
    }
    else if (pkTexture->m_spPersistentSrcRendererData.m_pObject) {
        return false;
    }
    if (m_kFormatPrefs.m_ePixelLayout != pkTexture->m_kFormatPrefs.m_ePixelLayout) {
        return false;
    }
    if (m_kFormatPrefs.m_eMipMapped != pkTexture->m_kFormatPrefs.m_eMipMapped) {
        return false;
    }
    if (m_kFormatPrefs.m_eAlphaFmt != pkTexture->m_kFormatPrefs.m_eAlphaFmt) {
        return false;
    }
    if (m_bStatic != pkTexture->m_bStatic) {
        return false;
    }
    if (m_bLoadDirectToRendererHint != pkTexture->m_bLoadDirectToRendererHint) {
        return false;
    }
    if (m_bSrcRendererDataIsPersistent != pkTexture->m_bSrcRendererDataIsPersistent) {
        return false;
    }
    return true;
}

void NiSourceTexture::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTexture::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiSourceTexture::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v10 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v10);
    char* ViewerString_9 = NiGetViewerString("File:", m_kPlatformFilename.GetHandle());
    uint32_t v8 = pkStrings->m_usSize;
    uint32_t v9 = pkStrings->m_usMaxSize;
    v10 = ViewerString_9;
    if (v8 >= v9) {
        pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v8, v10);
}

void NiSourceTexture::PostLinkObject(NiStream&) {
    if (NiSourceTexture::ms_bPreload) {
        NiStream::ms_kCleanupCriticalSection.Lock();
        NiSourceTexture* pTexture = this;
        pTexture->CreateRendererData();
        NiStream::ms_kCleanupCriticalSection.Unlock();
    }
}

// Reopens the file of a degrading or degraded texture and has the renderer reload it from there.
void NiSourceTexture::Upgrade() {
    RendererData* pkRendererData = m_pkRendererData;
    if (!pkRendererData) {
        return;
    }
    uint32_t uiState = pkRendererData->DegradePriorityRange.cState;
    if (uiState != PS_UPGRADING && uiState != PS_DEGRADED) {
        return;
    }
    const char* pcFilename = GetFilename();
    m_pFile = NiFile::GetFile(pcFilename, NiFile::READ_ONLY, 0x8000);
    if (!m_pFile) {
        return;
    }
    m_pkRendererData->Upgrade(this);
    if (m_pFile) {
        delete m_pFile;
    }
    m_pFile = nullptr;
}

// Degrades the renderer's copy only once it has finished upgrading.
void NiSourceTexture::Degrade() {
    RendererData* pkRendererData = m_pkRendererData;
    if (pkRendererData && pkRendererData->DegradePriorityRange.cState == PS_UPGRADED) {
        pkRendererData->Degrade(this);
    }
}

uint32_t NiSourceTexture::GetWidth() const {
    NiTexture::RendererData* m_pkRendererData;
    NiPersistentSrcTextureRendererData* m_spPersistentSrcRendererData;
    NiPixelData* m_spSrcPixelData;
    m_pkRendererData = this->m_pkRendererData;
    if (m_pkRendererData) {
        return m_pkRendererData->m_uiWidth;
    }
    m_spPersistentSrcRendererData = this->m_spPersistentSrcRendererData.m_pObject;
    if (m_spPersistentSrcRendererData) {
        return *m_spPersistentSrcRendererData->m_puiWidth;
    }
    m_spSrcPixelData = this->m_spSrcPixelData.m_pObject;
    if (m_spSrcPixelData) {
        return *m_spSrcPixelData->m_puiWidth;
    }

    return (uint32_t)0;
}

uint32_t NiSourceTexture::GetHeight() const {
    NiTexture::RendererData* m_pkRendererData;
    NiPersistentSrcTextureRendererData* m_pObject;
    m_pkRendererData = this->m_pkRendererData;
    if (m_pkRendererData) {
        return m_pkRendererData->m_uiHeight;
    }
    m_pObject = m_spPersistentSrcRendererData.m_pObject;
    if (m_pObject) {
        return *m_pObject->m_puiHeight;
    }
    NiPixelData* v4 = m_spSrcPixelData.m_pObject;
    if (v4) {
        return *v4->m_puiHeight;
    }

    return (uint32_t)0;
}

// Reads the platform file's pixel data when the texture has a file name but neither pixel data nor persistent renderer
// data.
void NiSourceTexture::LoadPixelDataFromFile() {
    if ((m_kFilename.GetHandle() != NiGlobalStringTable::NULL_STRING) &&
        (!m_spSrcPixelData.m_pObject && !m_spPersistentSrcRendererData.m_pObject)) {
        NiImageConverter* pConverter = NiImageConverter::GetImageConverter();
        NiPointer<NiPixelData> spFileData =
            pConverter->ReadImageFile(m_kPlatformFilename.GetHandle(), m_spSrcPixelData.m_pObject, nullptr);
        if (!m_spSrcPixelData.m_pObject || spFileData.m_pObject != m_spSrcPixelData.m_pObject) {
            m_spSrcPixelData.operator=(spFileData);
        }
    }
}

void NiSourceTexture::DestroyAppPixelData() {
    NiPixelData* pPixelData = this->m_spSrcPixelData.m_pObject;
    if (pPixelData) {
        if (!InterlockedDecrementUInt32(&pPixelData->m_uiRefCount)) {
            NiRefObject* pRefObject = static_cast<NiRefObject*>(pPixelData);
            pRefObject->DeleteThis();
        }
        this->m_spSrcPixelData.m_pObject = nullptr;
    }
}

// Has the renderer create the texture's renderer data (once), then drops the app-side pixel data of a static texture
// when ms_bDestroyAppData is set. Without a renderer it fails. The file the texture was opened from is closed (deleted
// through its vtable) on the paths that already have or cannot get renderer data and dropped unclosed after a creation.
bool NiSourceTexture::CreateRendererData() {
    NiRenderer* pkRenderer = NiRenderer::ms_pkRenderer;
    if (!pkRenderer) {
        if (m_pFile) {
            delete m_pFile;
            m_pFile = nullptr;
        }
        return false;
    }
    if (m_pkRendererData) {
        if (m_pFile) {
            delete m_pFile;
            m_pFile = nullptr;
        }
        return true;
    }
    if (!static_cast<NiDX9Renderer*>(pkRenderer)->CreateSourceTextureRendererData(this)) {
        return false;
    }
    if (ms_bDestroyAppData && m_pkRendererData && m_bStatic) {
        DestroyAppPixelData();
    }
    m_pFile = nullptr;
    return true;
}

// Takes kFilename, standardizes it and derives the platform-specific name.
void NiSourceTexture::SetFilename(const NiFixedString& kFilename) {
    NiGlobalStringTable::GlobalStringHandle kHandle = kFilename.GetHandle();
    // EXE-INLINED 0x43BA10
    if (m_kFilename.GetHandle() != kHandle) {
        if (kHandle != NiGlobalStringTable::NULL_STRING) {
            InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
        }
        NiGlobalStringTable::GlobalStringHandle kOld = m_kFilename.GetHandle();
        if (kOld != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kOld) - 8));
        }
        m_kFilename.SetHandle(kHandle);
    }
    NiStandardizeFilePath(m_kFilename);
    NiImageConverter::ConvertFilenameToPlatformSpecific(m_kFilename, m_kPlatformFilename);
}

void* NiSourceTexture::GetSrcPixelData() {
    return (&m_spSrcPixelData)->operator->();
}

// A texture of apRawData named apcName (no name when null); as the file-name overload otherwise.
NiSourceTexture* NiSourceTexture::Create(NiPixelData* apRawData, const char* apcName, NiTexture::FormatPrefs& arPrefs) {
    void* pMemory = NiMemObject::operator new(sizeof(NiSourceTexture));
    NiSourceTexture* pkTexture = pMemory ? ::new (pMemory) NiSourceTexture : nullptr;
    pkTexture->m_kFormatPrefs = arPrefs;
    pkTexture->m_spSrcPixelData = apRawData;
    if (apcName) {
        NiGlobalStringTable::GlobalStringHandle kHandle = NiGlobalStringTable::AddString(apcName);
        NiFixedString kName;
        kName.SetHandle(kHandle);
        pkTexture->SetFilename(kName);
        // EXE-INLINED 0x4381B0
        if (kHandle != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
        }
    }
    if (ms_bPreload && !pkTexture->CreateRendererData()) {
        delete pkTexture;
        return nullptr;
    }
    return pkTexture;
}

// A texture of the file kFilename. With preloading on, a texture whose renderer data cannot be made is deleted (through
// its vtable) and null is returned.
NiSourceTexture* NiSourceTexture::Create(
    const NiFixedString& kFilename,
    NiTexture::FormatPrefs& kPrefs,
    bool bLoadDirectToRendererHint,
    bool bSrcRendererDataIsPersistent
) {
    void* pMemory = NiMemObject::operator new(sizeof(NiSourceTexture));
    NiSourceTexture* pkTexture = pMemory ? ::new (pMemory) NiSourceTexture : nullptr;
    pkTexture->m_kFormatPrefs = kPrefs;
    pkTexture->m_bLoadDirectToRendererHint = bLoadDirectToRendererHint;
    pkTexture->m_bSrcRendererDataIsPersistent = bSrcRendererDataIsPersistent;
    // EXE-INLINED 0xA5FBA0
    // EXE-INLINED 0x43BA10
    NiGlobalStringTable::GlobalStringHandle kHandle = kFilename.GetHandle();
    if (pkTexture->m_kFilename.GetHandle() != kHandle) {
        if (kHandle != NiGlobalStringTable::NULL_STRING) {
            InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kHandle) - 8));
        }
        NiGlobalStringTable::GlobalStringHandle kOld = pkTexture->m_kFilename.GetHandle();
        if (kOld != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kOld) - 8));
        }
        pkTexture->m_kFilename.SetHandle(kHandle);
    }
    NiStandardizeFilePath(pkTexture->m_kFilename);
    NiImageConverter::ConvertFilenameToPlatformSpecific(pkTexture->m_kFilename, pkTexture->m_kPlatformFilename);
    if (ms_bPreload && !pkTexture->CreateRendererData()) {
        delete pkTexture;
        return nullptr;
    }
    return pkTexture;
}

// A texture read from the open file pFile, named apFileName (no name when null); as the file-name overload otherwise.
NiSourceTexture* NiSourceTexture::Create(
    NiFile* pFile,
    const char* apFileName,
    NiTexture::FormatPrefs& kPrefs,
    bool bLoadDirectToRendererHint
) {
    void* pMemory = NiMemObject::operator new(sizeof(NiSourceTexture));
    NiSourceTexture* pkTexture = pMemory ? ::new (pMemory) NiSourceTexture : nullptr;
    pkTexture->m_kFormatPrefs = kPrefs;
    pkTexture->m_bLoadDirectToRendererHint = bLoadDirectToRendererHint;
    pkTexture->m_pFile = pFile;
    if (apFileName) {
        NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString(apFileName);
        // EXE-INLINED 0xA5FBA0
        // EXE-INLINED 0x43BA10
        if (pkTexture->m_kFilename.GetHandle() != kName) {
            if (kName != NiGlobalStringTable::NULL_STRING) {
                InterlockedIncrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
            }
            NiGlobalStringTable::GlobalStringHandle kOld = pkTexture->m_kFilename.GetHandle();
            if (kOld != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kOld) - 8));
            }
            pkTexture->m_kFilename.SetHandle(kName);
        }
        NiStandardizeFilePath(pkTexture->m_kFilename);
        NiImageConverter::ConvertFilenameToPlatformSpecific(pkTexture->m_kFilename, pkTexture->m_kPlatformFilename);
        // EXE-INLINED 0x4381B0
        if (kName != NiGlobalStringTable::NULL_STRING) {
            InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
        }
    }
    if (ms_bPreload && !pkTexture->CreateRendererData()) {
        delete pkTexture;
        return nullptr;
    }
    return pkTexture;
}

NiObject* NiSourceTexture::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiSourceTexture));
    if (v0) {
        return ::new (v0) NiSourceTexture;
    }

    return nullptr;
}
