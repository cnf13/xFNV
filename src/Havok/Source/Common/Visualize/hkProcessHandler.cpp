#include "Havok/Source/Common/Visualize/hkProcessHandler.h"

#include "BSCore/NewOverloads.h"

hkProcessHandler::~hkProcessHandler() {}

void hkProcessHandler::operator delete(void* apMemory) {
    ourFree(apMemory);
}
