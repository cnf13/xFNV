#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

hkKdTreeCastCollector::~hkKdTreeCastCollector() {}

void hkKdTreeCastCollector::operator delete(void* apMemory) {
    ourFree(apMemory);
}
