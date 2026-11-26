#include "BSMain/RendererSettingCollection.h"

#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TES.h"

RendererSettingCollection::RendererSettingCollection() {}

RendererSettingCollection::~RendererSettingCollection() {}

void RendererSettingCollection::operator delete(void* apMemory) {
    ourFree(apMemory);
}
