#pragma once

#include "BSCore/IMemoryManagerFile.h"
#include "BSSystem/BSSystemFile.h"
#include "Reconstruction.hpp"

// The memory manager's debug output file, over an OS file.
class MemoryManagerFile : public IMemoryManagerFile {
public:
	// The hybrid's implicit constructor, for a local constructed later in place (File starts closed).
	MemoryManagerFile() = default;
	MemoryManagerFile(const char* apFileName);
	~MemoryManagerFile() override;

	int32_t Read(void* apData, uint32_t aSize) override;
	int32_t Write(const void* apData, uint32_t aSize) override;
	int32_t Size() override;
	void Seek(int32_t aiPos) override;

	static void operator delete(void* apMemory);

	BSSystemFile File;
};
inline constexpr fnvd::Vtable MemoryManagerFile_vftable{};
