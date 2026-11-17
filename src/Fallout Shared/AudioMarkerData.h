#pragma once

// AudioMarkerInfo (the AudioMarker member's type) is defined there.
#include "Fallout/AI/PlayerCharacter.h"
#include "Reconstruction.hpp"

// The audio marker data a buoy marker reference carries. Only the members the reconstructed code calls are
// declared.
class AudioBuoyMarkerData {
public:
	AudioBuoyMarkerData();
	~AudioBuoyMarkerData();

	// Whether the data differs from apCompare's; the exe's body is the shared "return true"
	// (thiscall, ret 4, 0x401290).
	bool Compare(AudioBuoyMarkerData* apCompare);
	// Whether apFile's current chunk is MMRK; reads nothing.
	bool Load(TESFile* apFile);
	// The exe's Copy is that empty body (thiscall, ret 4), kept out of line because the exe calls it.
	void Copy(AudioBuoyMarkerData*) {}

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// The marker's data (the PDB has it private; public here for the layout pin). The exe allocates 8 bytes and its
	// constructor (0x680890) clears both dwords.
	AudioMarkerInfo AudioMarker;
};
