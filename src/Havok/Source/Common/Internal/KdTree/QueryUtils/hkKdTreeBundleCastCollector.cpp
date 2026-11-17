#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"

#include "BSCore/MemoryManager.h"

hkKdTreeBundleCastCollector::~hkKdTreeBundleCastCollector() {}

void hkKdTreeBundleCastCollector::operator delete(void* apMemory) {
	ourFree(apMemory);
}
