#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNIFImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class NiFile;

// Reads image files and converts pixel data between formats; one converter is installed for the engine. Only part of
// the class is declared. ReadImageFileInfo, ReadImageFile and ConvertPixelData are declared only to complete the
// vtable.
class NiImageConverter : public NiRefObject {
public:
    NiImageConverter();
    ~NiImageConverter() override;

    virtual bool ReadImageFileInfo(
        const char* pcFilename,
        NiPixelFormat& kFmt,
        bool& bMipmap,
        uint32_t& uiWidth,
        uint32_t& uiHeight,
        uint32_t& uiNumFaces
    );
    virtual NiPixelData* ReadImageFile(const char* pcFilename, NiPixelData* pkOptDest, NiFile* apFile);
    virtual bool CanConvertPixelData(const NiPixelFormat& kSrcFmt, const NiPixelFormat& kDestFmt) const;
    virtual NiPixelData*
    ConvertPixelData(const NiPixelData& kSrc, const NiPixelFormat& kDestFmt, NiPixelData* pkOptDest, bool bMipmap);

    // The installed converter.
    static NiImageConverter* GetImageConverter();
    // Installs pkConverter in place of the current converter.
    static void SetImageConverter(NiImageConverter* pkConverter);

    // A fresh 260-byte block holding pcPath with ms_pcPlatformSubDir inserted into it.
    static char* ConvertFilenameToPlatformSpecific(const char* pcPath);
    // kPlatformPath becomes kPath with ms_pcPlatformSubDir inserted.
    static void ConvertFilenameToPlatformSpecific(const NiFixedString& kPath, NiFixedString& kPlatformPath);
    // Releases the installed converter.
    static void _SDMShutdown();
    // Installs a fresh converter.
    static void _SDMInit();

    // The reader for images stored in NIF files; it has 128-byte alignment.
    NiNIFImageReader m_kReader;

    // The converter installed for the engine.
    static NiPointer<NiImageConverter> ms_spConverter;
    // The directory platform-specific paths are placed in; null when a path is used unchanged.
    static inline char* ms_pcPlatformSubDir;
};
