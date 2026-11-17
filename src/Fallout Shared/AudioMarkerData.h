#pragma once

// AudioMarkerInfo (the AudioMarker member's type) is defined there.
#include "Fallout/AI/PlayerCharacter.h"

// The audio marker data a buoy marker reference carries. Only some of its members are declared.
class AudioBuoyMarkerData {
public:
	AudioBuoyMarkerData();
	~AudioBuoyMarkerData();

	// Whether the data differs from apCompare's.
	bool Compare(AudioBuoyMarkerData* apCompare);
	// Whether apFile's current chunk is MMRK; reads nothing.
	bool Load(TESFile* apFile);
	// Copy deliberately has an empty body, as the game does.
	void Copy(AudioBuoyMarkerData*) {}

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	AudioMarkerInfo AudioMarker;
};
