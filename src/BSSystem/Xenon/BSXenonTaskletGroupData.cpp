#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTaskletGroupData.h"

BSTaskletGroupData::~BSTaskletGroupData() {}

void BSTaskletGroupData::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
