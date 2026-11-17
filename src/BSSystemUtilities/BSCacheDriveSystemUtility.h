#pragma once

#include <stdint.h>

enum CACHEDRIVE_STATUS : uint32_t {
	CACHEDRIVE_STATUS_NOT_READY = 0,
	CACHEDRIVE_STATUS_OK = 1,
	CACHEDRIVE_STATUS_ERROR = 2,
	CACHEDRIVE_STATUS_FULL = 3,
};

// The platform's cache drive service. PC has no cache drive: Initialize and Destroy do nothing.
class BSCacheDriveSystemUtility {
public:
	BSCacheDriveSystemUtility();
	virtual ~BSCacheDriveSystemUtility();

	virtual void Initialize();
	virtual void Destroy();

	static void operator delete(void* apMemory);
	static BSCacheDriveSystemUtility* GetSingleton();

	bool bIsActive;
	CACHEDRIVE_STATUS eCacheDriveStatus_;
	char cCacheDriveDir[1055];
};
