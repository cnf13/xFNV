#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// An instance of the PDB's NiStreamLoadBinary<T> template for a 4-byte T (which T the exe's code does not show),
// with BinaryRead inlined.
void NiStreamLoadBinary(NiFile* a1, void* a2);
// Named here after its address; it is the PDB's protected static
// NiDDSReader::Read24Bit(NiFile&, NiPixelData*, NiPixelFormat, NiPixelFormat, mip, face, bEndianConverted): ReadFile
// (0xA8D0D0) pushes seven arguments. Returns void (the PDB's `?Read24Bit@NiDDSReader@@KAX...`); no caller reads eax.
void Read24Bit(NiFile& kIst, NiPixelData* pkDest, NiPixelFormat kSrcFmt, NiPixelFormat kDestFmt, uint32_t uiMipMapIdx,
	uint32_t uiFaceIdx, bool bEndianConverted);
// Named here after its address; it is the PDB's protected static NiDDSReader::Read32Bit, with Read24Bit's seven
// arguments and void return.
void Read32Bit(NiFile& kIst, NiPixelData* pkDest, NiPixelFormat kSrcFmt, NiPixelFormat kDestFmt, uint32_t uiMipMapIdx,
	uint32_t uiFaceIdx, bool bEndianConverted);

// The image reader for DirectDraw Surface (.dds) files.
class NiDDSReader : public NiImageReader {
public:
	// Values from the Xbox PDB.
	enum : int32_t {
		NI_ALPHAPIXELS = 0x1,
		NI_FOURCC = 0x4,
		NI_RGB = 0x40,
	};

	// The FourCC compression codes (Gamebryo's NiDDSReader.h).
	enum : uint32_t {
		NI_DXTC1 = 0x31545844,
		NI_DXTC3 = 0x33545844,
		NI_DXTC5 = 0x35545844,
		NI_R16 = 0x6F,
		NI_RG32 = 0x70,
		NI_RGBA64 = 0x71,
		NI_R32 = 0x72,
		NI_RG64 = 0x73,
		NI_RGBA128 = 0x74,
	};

	// The file's magic number, "DDS ".
	enum : uint32_t {
		NI_DDS = 0x20534444,
	};

	// The header's flags.
	enum : uint32_t {
		NI_DDSD_CAPS = 0x1,
		NI_DDSD_HEIGHT = 0x2,
		NI_DDSD_WIDTH = 0x4,
		NI_DDSD_PIXELFORMAT = 0x1000,
		NI_DDSD_LINEARSIZE = 0x80000,
		NI_DDSD_DEPTH = 0x800000,
	};

	// The first caps word.
	enum : uint32_t {
		NI_DDSCAPS_COMPLEX = 0x8,
		NI_DDSCAPS_TEXTURE = 0x1000,
		NI_DDSCAPS_MIPMAP = 0x400000,
	};

	// The second caps word.
	enum : uint32_t {
		NI_DDSCAPS2_CUBEMAP = 0x200,
		NI_DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
		NI_DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
		NI_DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
		NI_DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
		NI_DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
		NI_DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
		NI_DDSCAPS2_VOLUME = 0x200000,
	};

	NiDDSReader();

	// Slot 0 is NiImageReader's vector deleting destructor (0xA8E480), which the exe's NiDDSReader vtable shares;
	// hdrgen's vtable check therefore sees it as the base's, while C++ gives every class its own destructor slot.
	bool CanReadFile(const char*) const override;
	NiPixelData* ReadFile(NiFile&, NiPixelData*) override;
	bool ReadHeader(NiFile&, uint32_t&, uint32_t&, NiPixelFormat&, bool&, uint32_t&) override;

protected:
	// thiscall, ret 0x14: only the bit depth decides (16, 24 or 32).
	bool ValidateRGBABitmasks(uint32_t uiRedMask, uint32_t uiGreenMask, uint32_t uiBlueMask, uint32_t uiAlphaMask,
		uint32_t uiBitsPerPixel);
	// thiscall, ret 0x48: the NiPixelFormat result through the hidden pointer, then kSrcFormat by value.
	NiPixelFormat* ComputeFinalFormat(NiPixelFormat* apResult, NiPixelFormat kSrcFormat);

public:
	// Named here after its address; it is the PDB's protected static NiDDSReader::Read16Bit, with Read24Bit's seven
	// arguments and void return.
	static void Read16Bit(NiFile& kIst, NiPixelData* pkDest, NiPixelFormat kSrcFmt, NiPixelFormat kDestFmt,
		uint32_t uiMipMapIdx, uint32_t uiFaceIdx, bool bEndianConverted);

	uint32_t m_uiHeight;
	uint32_t m_uiWidth;
	uint32_t m_uiMipmapLevels;
	bool m_bIsEndianConverted;
	NiPixelFormat m_kOriginalFormat;
};
inline constexpr fnvd::Vtable NiDDSReader_vftable{};
