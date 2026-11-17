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
	MemoryManager_Singleton.Deallocate(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out. The instance's atexit thunk is the exe's `dynamic atexit
// destructor for 'stGameDataSystemUtility'' (0xFDC330: the destructor 0xAF39C0 on the instance).
BSGameDataSystemUtility* BSGameDataSystemUtility::GetSingleton() {
	static BSGameDataSystemUtility stGameDataSystemUtility;
	return &stGameDataSystemUtility;
}
