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

BSCacheDriveSystemUtility* BSCacheDriveSystemUtility::GetSingleton() {
	static BSCacheDriveSystemUtility stCacheDriveSystemUtility;
	return &stCacheDriveSystemUtility;
}
