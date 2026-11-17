#pragma once

#include "Reconstruction.hpp"

#include <cstdint>

// The Steam SDK's utilities interface (isteamutils.h), the slots up to GetAppID. The exe calls GetAppID through slot 9
// (`call [edx+0x24]`, 0x6FF75F). Names and parameters are the SDK's; the PDB has no Steam types.
class ISteamUtils {
public:
	virtual uint32_t GetSecondsSinceAppActive() = 0;
	virtual uint32_t GetSecondsSinceComputerActive() = 0;
	// SDK: EUniverse.
	virtual int32_t GetConnectedUniverse() = 0;
	virtual uint32_t GetServerRealTime() = 0;
	virtual const char* GetIPCountry() = 0;
	virtual bool GetImageSize(int32_t iImage, uint32_t* pnWidth, uint32_t* pnHeight) = 0;
	virtual bool GetImageRGBA(int32_t iImage, uint8_t* pubDest, int32_t nDestBufferSize) = 0;
	virtual bool GetCSERIPPort(uint32_t* unIP, uint16_t* usPort) = 0;
	virtual uint8_t GetCurrentBatteryPower() = 0;
	// SDK: AppId_t.
	virtual uint32_t GetAppID() = 0;
};

// steam_api.dll's SteamUtils (cdecl), through the exe's import slot: the utilities interface, or null when Steam is
// not running.
using SteamUtils_t = ISteamUtils*(*)();
FNVD_IMPORT(SteamUtils_t, SteamUtils, 0xFDF49C);
