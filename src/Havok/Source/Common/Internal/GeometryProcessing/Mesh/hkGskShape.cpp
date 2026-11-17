#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"

#include "BSCore/MemoryManager.h"

hkGskShape::~hkGskShape() {}

void hkGskShape::operator delete(void* apMemory) {
    ourFree(apMemory);
}
