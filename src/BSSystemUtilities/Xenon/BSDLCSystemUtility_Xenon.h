#pragma once

#include "Reconstruction.hpp"

class BSDLCSystemUtility;

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
void nullsub(BSDLCSystemUtility* apThis, [[maybe_unused]] void* apEdx);
char return_1_1();

// Not named in the PDB or IDA; names chosen here. One entry of the PC's DLC table LoadDLC (0xAF2940) walks: the
// Steam app id it asks BIsDlcInstalled about and the wide display name it gives the package (8 bytes per entry:
// `[ecx*8+0x11ACCB8]`, `[eax*8+0x11ACCBC]`).
struct DLCAppEntry {
	// SDK: AppId_t; -1 ends the table.
	int32_t iAppID;
	const char16_t* pDisplayName;
};

// Not named in the PDB or IDA; name chosen here. The DLC table, ended by an app id of -1; in 1.4.0.525 it holds only
// that terminator (0x11ACCB8: -1, 0).
using DLCAppTable_t = DLCAppEntry[1];
inline DLCAppTable_t DLCAppTable{{-1}};
