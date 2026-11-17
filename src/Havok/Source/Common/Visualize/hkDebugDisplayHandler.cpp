#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"

#include "BSCore/MemoryManager.h"

hkDebugDisplayHandler::~hkDebugDisplayHandler() {}

void hkDebugDisplayHandler::operator delete(void* apMemory) {
    ourFree(apMemory);
}
