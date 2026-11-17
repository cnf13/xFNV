#include "Havok/Source/Common/Visualize/hkProcessHandler.h"

#include "BSCore/MemoryManager.h"

hkProcessHandler::~hkProcessHandler() {}

void hkProcessHandler::operator delete(void* apMemory) {
	ourFree(apMemory);
}
