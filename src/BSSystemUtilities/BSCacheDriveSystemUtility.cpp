#include "BSSystemUtilities/BSCacheDriveSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"

BSCacheDriveSystemUtility::BSCacheDriveSystemUtility()
	: bIsActive(false), eCacheDriveStatus_(CACHEDRIVE_STATUS_NOT_READY) {}

BSCacheDriveSystemUtility::~BSCacheDriveSystemUtility() {}

void BSCacheDriveSystemUtility::Initialize() {
	Destroy();
}

void BSCacheDriveSystemUtility::Destroy() {}

void BSCacheDriveSystemUtility::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out. The instance's atexit thunk is the exe's `dynamic atexit
// destructor for 'stCacheDriveSystemUtility'' (0xFDC320: the destructor 0xAF3900 on the instance).
BSCacheDriveSystemUtility* BSCacheDriveSystemUtility::GetSingleton() {
	static BSCacheDriveSystemUtility stCacheDriveSystemUtility;
	return &stCacheDriveSystemUtility;
}
