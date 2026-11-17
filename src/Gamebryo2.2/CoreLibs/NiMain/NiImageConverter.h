#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNIFImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

class NiFile;

// Reads image files and converts pixel data between formats; one converter is installed for the engine.
// Only the members called so far; the rest is not reconstructed yet. ReadImageFileInfo, ReadImageFile and
// ConvertPixelData are declared only to complete the vtable.
class NiImageConverter : public NiRefObject {
public:
	NiImageConverter();
	~NiImageConverter() override;

	// slot 2, 0xA766D0
	virtual bool ReadImageFileInfo(const char* pcFilename, NiPixelFormat& kFmt, bool& bMipmap, uint32_t& uiWidth,
		uint32_t& uiHeight, uint32_t& uiNumFaces);
	// slot 3, 0xA767C0
	virtual NiPixelData* ReadImageFile(const char* pcFilename, NiPixelData* pkOptDest, NiFile* apFile);
	// slot 4, 0xA76890. Whether pixel data in akSrcFmt can be converted to akDestFmt.
	virtual bool CanConvertPixelData(const NiPixelFormat& kSrcFmt, const NiPixelFormat& kDestFmt) const;
	// slot 5, 0xA76C10
	virtual NiPixelData* ConvertPixelData(const NiPixelData& kSrc, const NiPixelFormat& kDestFmt,
		NiPixelData* pkOptDest, bool bMipmap);

	// cdecl, ret 0, 0xA76640. The installed converter.
	static NiImageConverter* GetImageConverter();
	// cdecl, ret 0. Installs pkConverter in place of the current converter.
	static void SetImageConverter(NiImageConverter* pkConverter);

	// cdecl, ret 0, 0xA768B0. A fresh 260-byte block holding pcPath with ms_pcPlatformSubDir inserted into it.
	static char* ConvertFilenameToPlatformSpecific(const char* pcPath);
	// kPlatformPath becomes kPath with ms_pcPlatformSubDir inserted.
	static void ConvertFilenameToPlatformSpecific(const NiFixedString& kPath, NiFixedString& kPlatformPath);
	// cdecl, ret 0. Releases the installed converter.
	static void _SDMShutdown();
	// Installs a fresh converter.
	static void _SDMInit();

	// The reader for images stored in NIF files; its 128-byte alignment puts it at 0x80 (the constructor 0xA76650
	// builds it at this + 0x80).
	NiNIFImageReader m_kReader;

	// The converter installed for the engine.
	static NiPointer<NiImageConverter> ms_spConverter;
	// The directory platform-specific paths are placed in; null when a path is used unchanged.
	static inline char* ms_pcPlatformSubDir;
};
inline constexpr fnvd::Vtable NiImageConverter_vftable{};
