#pragma once

// What a scene graph update pass carries down the tree.
struct NiUpdateData {
	// The three-parameter constructor stores the time and the two flags and clears the three "found" flags.
	NiUpdateData() = default;
	NiUpdateData(float afTime, bool abUpdateControllers, bool abParallelUpdate);

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
