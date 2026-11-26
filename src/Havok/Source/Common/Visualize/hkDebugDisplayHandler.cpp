#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"

#include "BSCore/NewOverloads.h"

hkDebugDisplayHandler::~hkDebugDisplayHandler() {}

void hkDebugDisplayHandler::operator delete(void* apMemory) {
    ourFree(apMemory);
}
