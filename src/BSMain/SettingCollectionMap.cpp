#include "BSMain/SettingCollectionMap.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

// The map is a BSTCaseInsensitiveStringMap<Setting*>, as the exe builds it; not a copy of the strings.
SettingCollectionMap::SettingCollectionMap(uint32_t aiMapSize) : Settings(aiMapSize, false) {}

// The map is emptied by its own destructor.
SettingCollectionMap::~SettingCollectionMap() {}
