#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Reconstruction.hpp"

// Bethesda's multi-target transform-controller interface; the exe operates on the base's storage unchanged.
class BSTransCon : public NiMultiTargetTransformController {
public:
	// Puts the controlled objects back to the sequence's last key.
	void UpdateAllButTarget(float afTime, bool abSkipSelectiveUpdateTest);
};
