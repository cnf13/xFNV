#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkGskShape.h"

#include "BSCore/NewOverloads.h"

hkGskShape::~hkGskShape() {}

void hkGskShape::operator delete(void* apMemory) {
    ourFree(apMemory);
}
