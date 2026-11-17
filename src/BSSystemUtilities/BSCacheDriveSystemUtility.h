#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

// The cache drive's state (PDB enum; the constructor stores it as a dword, 0xAF38EA).
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
	// IDA name; the PDB's is QInstance.
	static BSCacheDriveSystemUtility* GetSingleton();

	bool bIsActive;
	CACHEDRIVE_STATUS eCacheDriveStatus_;
	// The PDB's char[1055]; the static instance (0x11F72F8) ends at its guard (0x11F7724), 0x42C bytes on.
	char cCacheDriveDir[1055];
};

inline constexpr fnvd::Vtable BSCacheDriveSystemUtility_vftable{};
