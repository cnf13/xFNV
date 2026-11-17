#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

// The read critical section initialises and deletes its section in its own constructor and destructor (inlined by
// the exe).
NiImageReader::NiImageReader() {}

NiImageReader::~NiImageReader() {}

void NiImageReader::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
