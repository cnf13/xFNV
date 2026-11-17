#include "Havok/Source/Common/Visualize/hkProcess.h"

#include "BSCore/MemoryManager.h"

hkProcess::hkProcess(hkBool selectable) : m_selectable(selectable) {}

hkProcess::~hkProcess() {}

void hkProcess::operator delete(void* apMemory) {
	ourFree(apMemory);
}
