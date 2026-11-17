#include "BSMain/RendererSettingCollection.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/TES.h"

RendererSettingCollection::RendererSettingCollection() {}

// The exe's destructor runs only SettingCollectionList's (through sub_45D240); the bases' destructors do it here.
RendererSettingCollection::~RendererSettingCollection() {}

// The exe's deleting destructor (0x4DE550) frees through ourFree.
void RendererSettingCollection::operator delete(void* apMemory) {
	ourFree(apMemory);
}
