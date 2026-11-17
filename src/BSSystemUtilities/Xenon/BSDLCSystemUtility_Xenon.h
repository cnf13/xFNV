#pragma once

#include <cstdint>

class BSDLCSystemUtility;

void nullsub(BSDLCSystemUtility* apThis, [[maybe_unused]] void* apEdx);
char return_1_1();

// One entry of the PC's DLC table that LoadDLC walks: the Steam app id it asks BIsDlcInstalled about and the wide
// display name it gives the package.
struct DLCAppEntry {
	// SDK: AppId_t; -1 ends the table.
	int32_t iAppID;
	const char16_t* pDisplayName;
};

// The DLC table, ended by an app id of -1; in 1.4.0.525 it holds only that terminator.
using DLCAppTable_t = DLCAppEntry[1];
inline DLCAppTable_t DLCAppTable{{-1}};
