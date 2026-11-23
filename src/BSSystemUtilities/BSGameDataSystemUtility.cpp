#include "BSSystemUtilities/BSGameDataSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"

BSGameDataSystemUtility::BSGameDataSystemUtility()
  : bIsActive(false), bNewGameData(false), eGameDataStatus_(GAMEDATA_STATUS_NOT_READY), iFreeSpaceKB_(0) {}

BSGameDataSystemUtility::~BSGameDataSystemUtility() {}

void BSGameDataSystemUtility::Initialize() {
    Destroy();
}

void BSGameDataSystemUtility::Destroy() {
    ;
}

void BSGameDataSystemUtility::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

BSGameDataSystemUtility* BSGameDataSystemUtility::GetSingleton() {
    static BSGameDataSystemUtility stGameDataSystemUtility;
    return &stGameDataSystemUtility;
}
