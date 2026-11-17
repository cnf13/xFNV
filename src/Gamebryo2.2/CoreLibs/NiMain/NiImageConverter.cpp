#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNIFImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFilename.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

NiPointer<NiImageConverter> NiImageConverter::ms_spConverter;

NiImageConverter::NiImageConverter() {}

// The platform directory is freed and cleared; the reader is destroyed by the compiler.
NiImageConverter::~NiImageConverter() {
    if (ms_pcPlatformSubDir) {
        _NiFree(ms_pcPlatformSubDir);
    }
    ms_pcPlatformSubDir = nullptr;
}

// The reader's virtuals are called through a base pointer. The file is deleted on every path that opened it.
bool NiImageConverter::ReadImageFileInfo(
    const char* pcFilename,
    NiPixelFormat& kFmt,
    bool& bMipmap,
    uint32_t& uiWidth,
    uint32_t& uiHeight,
    uint32_t& uiNumFaces
) {
    NiFilename kFileName(pcFilename);
    NiImageReader* pkReader = &m_kReader;
    if (!pkReader->CanReadFile(kFileName.m_acExt)) {
        return false;
    }
    NiFile* pkIst = NiFile::GetFile(pcFilename, NiFile::READ_ONLY, 0x8000);
    if (pkIst == nullptr) {
        return false;
    }
    if (!*pkIst) {
        delete pkIst;
        return false;
    }
    bool bRead = pkReader->ReadHeader(*pkIst, uiWidth, uiHeight, kFmt, bMipmap, uiNumFaces);
    delete pkIst;
    return bRead;
}

// A file the caller passes is deleted here too, also when the reader does not take the extension.
NiPixelData* NiImageConverter::ReadImageFile(const char* pcFilename, NiPixelData* pkOptDest, NiFile* apFile) {
    NiFilename kFileName(pcFilename);
    NiImageReader* pkReader = &m_kReader;
    NiFile* pkIst = apFile;
    if (!pkReader->CanReadFile(kFileName.m_acExt)) {
        if (pkIst != nullptr) {
            delete pkIst;
        }
        return nullptr;
    }
    if (pkIst == nullptr) {
        pkIst = NiFile::GetFile(pcFilename, NiFile::READ_ONLY, 0x8000);
        if (pkIst == nullptr) {
            return nullptr;
        }
    }
    if (!*pkIst) {
        delete pkIst;
        return nullptr;
    }
    NiPixelData* pkDest = pkReader->ReadFile(*pkIst, pkOptDest);
    delete pkIst;
    return pkDest;
}

bool NiImageConverter::CanConvertPixelData(const NiPixelFormat& kSrcFmt, const NiPixelFormat& kDestFmt) const {
    return (bool)((uint8_t)(&kSrcFmt)->operator==(kDestFmt));
}

NiPixelData* NiImageConverter::ConvertPixelData(
    const NiPixelData& kSrc,
    const NiPixelFormat& kDestFmt,
    NiPixelData* pkOptDest,
    bool bMipmap
) {
    if (kSrc.m_kPixelFormat.operator!=(kDestFmt)) {
        return nullptr;
    }
    if (pkOptDest) {
        if (pkOptDest == &kSrc) {
            return pkOptDest;
        }
        if (pkOptDest->m_kPixelFormat.operator==(kDestFmt) && *pkOptDest->m_puiWidth == *kSrc.m_puiWidth &&
            *pkOptDest->m_puiHeight == *kSrc.m_puiHeight) {
            uint32_t m_uiMipmapLevels = pkOptDest->m_uiMipmapLevels;
            if (m_uiMipmapLevels <= kSrc.m_uiMipmapLevels) {
                for (uint32_t i = 0; i < m_uiMipmapLevels; ++i) {
                    rt::memcpy(
                        &pkOptDest->m_pucPixels[pkOptDest->m_puiOffsetInBytes[i]],
                        &kSrc.m_pucPixels[kSrc.m_puiOffsetInBytes[i]],
                        kSrc.m_puiOffsetInBytes[i + 1] - kSrc.m_puiOffsetInBytes[i]
                    );
                }
                if (kSrc.m_spPalette.m_pObject) {
                    NiPalette* m_pObject = pkOptDest->m_spPalette.m_pObject;
                    if (!m_pObject) {
                        NiPalette* v9 = static_cast<NiPalette*>(NiMemObject::operator new(sizeof(NiPalette)));
                        if (v9) {
                            NiPalette* v10 = ::new (v9) NiPalette(*kSrc.m_spPalette.m_pObject);
                            pkOptDest->SetPalette(v10);
                        }
                        else {
                            pkOptDest->SetPalette(nullptr);
                        }
                        return pkOptDest;
                    }
                    m_pObject->operator=(*kSrc.m_spPalette.m_pObject);
                }
                return pkOptDest;
            }
        }
    }
    NiPixelData* v11 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
    if (v11) {
        return ::new (v11) NiPixelData(kSrc);
    }
    else {
        return nullptr;
    }
}

NiImageConverter* NiImageConverter::GetImageConverter() {
    return NiImageConverter::ms_spConverter;
}

void NiImageConverter::SetImageConverter(NiImageConverter* pkConverter) {
    NiImageConverter* pkOld = ms_spConverter.m_pObject;
    if (pkOld == pkConverter) {
        return;
    }
    if (pkOld) {
        pkOld->DecRefCount();
    }
    ms_spConverter.m_pObject = pkConverter;
    if (pkConverter) {
        pkConverter->IncRefCount();
    }
}

char* NiImageConverter::ConvertFilenameToPlatformSpecific(const char* pcPath) {
    char* pcPlatformPath = static_cast<char*>(_NiMalloc(0x104));
    NiFilename kPlatform;
    if (ms_pcPlatformSubDir) {
        kPlatform.~NiFilename();
        ::new (&kPlatform) NiFilename(pcPath);
        strcpy_s(kPlatform.m_acSubDir, 0x100, ms_pcPlatformSubDir);
        kPlatform.GetFullPath(pcPlatformPath, 0x104);
    }
    else {
        strncpy_s(pcPlatformPath, 0x104, pcPath, std::strlen(pcPath));
    }
    return pcPlatformPath;
}

void NiImageConverter::ConvertFilenameToPlatformSpecific(const NiFixedString& kPath, NiFixedString& kPlatformPath) {
    if (NiImageConverter::ms_pcPlatformSubDir) {
        char pcString[260];
        NiFilename kPlatform((&kPath)->GetHandle());
        strcpy_s(kPlatform.m_acSubDir, 256, NiImageConverter::ms_pcPlatformSubDir);
        kPlatform.GetFullPath(pcString, 260u);
        const char* m_kHandle = (&kPlatformPath)->GetHandle();
        if ((&kPlatformPath)->GetHandle() != pcString) {
            kPlatformPath.m_kHandle = NiGlobalStringTable::AddString(pcString);
            NiGlobalStringTable::ReleaseHandle(m_kHandle);
        }
    }
    else {
        const char* v3 = (&kPath)->GetHandle();
        if ((&kPlatformPath)->GetHandle() != (&kPath)->GetHandle()) {
            NiGlobalStringTable::AcquireHandle(v3);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle((&kPlatformPath)->GetHandle());
            kPlatformPath.m_kHandle = v3;
        }
    }
}

void NiImageConverter::_SDMShutdown() {
    ms_spConverter = nullptr;
}

void NiImageConverter::_SDMInit() {
    NiImageConverter* pkConverter = static_cast<NiImageConverter*>(NiMemObject::operator new(sizeof(NiImageConverter)));
    if (pkConverter != nullptr) {
        pkConverter = ::new (pkConverter) NiImageConverter;
    }
    NiImageConverter* pkOld = ms_spConverter.m_pObject;
    if (pkOld == pkConverter) {
        return;
    }
    if (pkOld) {
        pkOld->DecRefCount();
    }
    ms_spConverter.m_pObject = pkConverter;
    if (pkConverter) {
        pkConverter->IncRefCount();
    }
}

void NiPixelData::SetPalette(NiPalette* pkPal) {
    NiPalette* pkOld = m_spPalette.m_pObject;
    if (pkOld == pkPal) {
        return;
    }
    if (pkOld) {
        pkOld->DecRefCount();
    }
    m_spPalette.m_pObject = pkPal;
    if (pkPal) {
        pkPal->IncRefCount();
    }
}
