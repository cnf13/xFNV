#include "Fallout Shared/TEX_USER_DATA.h"

#include "BSCore/MemoryManager.h"

void TEX_USER_DATA::operator delete(void* apMemory) {
	ourFree(apMemory);
}
