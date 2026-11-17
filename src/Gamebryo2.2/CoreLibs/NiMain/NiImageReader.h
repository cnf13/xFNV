#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Reconstruction.hpp"

class NiFile;
class NiPixelData;
class NiPixelFormat;

// Base of the image file readers: checks a file, reads its header and reads it into NiPixelData under a lock.
class NiImageReader : public NiMemObject {
public:
	// The constructor the exe inlines into its derived readers' (NiNIFImageReader's, 0xA8AE50).
	NiImageReader();
	virtual ~NiImageReader();

	// Slots 1-3 are _purecall in the exe's NiImageReader vtable (0x109F3D0).
	virtual bool CanReadFile(const char*) const = 0;
	virtual NiPixelData* ReadFile(NiFile&, NiPixelData*) = 0;
	virtual bool ReadHeader(NiFile&, uint32_t&, uint32_t&, NiPixelFormat&, bool&, uint32_t&) = 0;

	// The deleting destructor frees through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);

	NiCriticalSection m_kReadCriticalSection;
};
inline constexpr fnvd::Vtable NiImageReader_vftable{};
