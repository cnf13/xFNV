#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"

#include "BSCore/MemoryManager.h"

NiAllocator::~NiAllocator() {}

void NiAllocator::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
