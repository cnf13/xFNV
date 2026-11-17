#include "BSMain/RendererSettingCollection.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/TES.h"

RendererSettingCollection::RendererSettingCollection() {}

RendererSettingCollection::~RendererSettingCollection() {}

void RendererSettingCollection::operator delete(void* apMemory) {
	ourFree(apMemory);
}
