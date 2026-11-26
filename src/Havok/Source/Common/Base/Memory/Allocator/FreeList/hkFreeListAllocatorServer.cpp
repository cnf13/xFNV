#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocatorServer.h"

#include "BSCore/NewOverloads.h"

hkFreeListAllocatorServer::~hkFreeListAllocatorServer() {}

void hkFreeListAllocatorServer::operator delete(void* apMemory) {
    ::ourFree(apMemory);
}
