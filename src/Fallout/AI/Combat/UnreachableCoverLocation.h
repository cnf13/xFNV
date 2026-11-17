#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"

#include <stdint.h>

class BGSSaveFormBuffer;

// A cover key and the time it was recorded as unreachable.
class UnreachableCoverLocation {
public:
	UnreachableCoverLocation();

	// Saves iCoverKey (4 bytes), then fTimeStamp.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Loads iCoverKey (4 bytes), then fTimeStamp.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	uint32_t iCoverKey;
	CombatTimeStamp fTimeStamp;
};
