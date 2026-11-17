#pragma once

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

// The file the memory manager writes its debug output to: every slot is abstract.
class IMemoryManagerFile {
public:
	IMemoryManagerFile();
	virtual ~IMemoryManagerFile();

	virtual int32_t Read(void* apData, uint32_t aSize) = 0;
	virtual int32_t Write(const void* apData, uint32_t aSize) = 0;
	virtual int32_t Size() = 0;
	virtual void Seek(int32_t aiPos) = 0;

	static void operator delete(void* apMemory) { ourFree(apMemory); }
};
inline constexpr fnvd::Vtable IMemoryManagerFile_vftable{};
