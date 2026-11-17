#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

// The image reader for pixel data stored in NIF files, read through its own NiStream.
// sizeof: NiImageConverter::_SDMInit allocates 0x780 bytes with this reader at +0x80, so the 128-byte alignment
// pads it from 0x6C4 to 0x700.
class NiNIFImageReader : public NiImageReader {
public:
	NiNIFImageReader();
	~NiNIFImageReader() override;

	bool CanReadFile(const char*) const override;
	NiPixelData* ReadFile(NiFile&, NiPixelData*) override;
	bool ReadHeader(NiFile&, uint32_t&, uint32_t&, NiPixelFormat&, bool&, uint32_t&) override;

	NiStream m_kStream;
};
