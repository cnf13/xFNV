#include "BSCore/MemoryManager.h"
#include "BSSystem/BSTaskletGroupData.h"
#include "Reconstruction.hpp"

BSTaskletGroupData::~BSTaskletGroupData() {}

void BSTaskletGroupData::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
