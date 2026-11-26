#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"

#include "BSCore/NewOverloads.h"

hkKdTreeBundleCastCollector::~hkKdTreeBundleCastCollector() {}

void hkKdTreeBundleCastCollector::operator delete(void* apMemory) {
    ourFree(apMemory);
}
