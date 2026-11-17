#pragma once

#include "Reconstruction.hpp"

// What a scene graph update pass carries down the tree.
struct NiUpdateData {
	// The constructor, thiscall, ret 0xC, 0x43D410: stores the time and the two flags and
	// clears the three "found" flags.
	// The hybrid's implicit default constructor (no zero-parameter Construct).
	NiUpdateData() = default;
	NiUpdateData(float afTime, bool abUpdateControllers, bool abParallelUpdate);

	// The hybrid's aggregate initialisation of all six members (`NiUpdateData kData = {fTime, true, false, ...}`).
	NiUpdateData(float afTime, bool abUpdateControllers, bool abParallelUpdate, bool abFoundParticles,
		bool abFoundMorphController, bool abSceneGraphChange)
		: fTime(afTime), bUpdateControllers(abUpdateControllers), bParallelUpdate(abParallelUpdate),
		  bFoundParticles(abFoundParticles), bFoundMorphController(abFoundMorphController),
		  bSceneGraphChange(abSceneGraphChange) {}

	float fTime;
	bool bUpdateControllers;
	bool bParallelUpdate;
	bool bFoundParticles;
	bool bFoundMorphController;
	bool bSceneGraphChange;
};
