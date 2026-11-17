#include "Fallout/Misc/MemoryManagerFile.h"

#include "BSCore/IMemoryManagerFile.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSSystemFile.h"

MemoryManagerFile::MemoryManagerFile(const char* apFileName) : File(apFileName, 1u, 2u, 0) {}

MemoryManagerFile::~MemoryManagerFile() {}

// Reads aSize bytes into apData; returns the count read.
int32_t MemoryManagerFile::Read(void* apData, uint32_t aSize) {
	uint64_t uiRead;
	File.Read(apData, aSize, uiRead);
	return static_cast<int32_t>(uiRead);
}

int32_t MemoryManagerFile::Write(const void* apData, uint32_t aSize) {
	uint64_t uiwritten;
	File.Write(apData, aSize, uiwritten);
	return static_cast<int32_t>(uiwritten);
}

int32_t MemoryManagerFile::Size() {
	uint64_t v2;
	(&File)->GetSize(v2);
	return (int32_t)v2;
}

// Seeks to aiPos from the start of the file.
void MemoryManagerFile::Seek(int32_t aiPos) {
	uint64_t uiPos;
	File.Seek(aiPos, BSSystemFile::SM_SET, uiPos);
}

void MemoryManagerFile::operator delete(void* apMemory) {
	ourFree(apMemory);
}

IMemoryManagerFile::IMemoryManagerFile() {}

IMemoryManagerFile::~IMemoryManagerFile() {}

// The PDB places BSSystemFile::Seek in this file.
BSSystemFile::ErrorCode BSSystemFile::Seek(uint64_t auiOffset, SeekMode aeWhence, uint64_t& auiPos) {
	eLastError = DoSeek(auiOffset, aeWhence, auiPos);
	return eLastError;
}
