#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// A cloning pass that also scales the clones by Scale.
class BSCloningProcess : public NiCloningProcess {
public:
	// The local the exe constructs in place (a plain local or FNVD_INLINED_LOCAL, reconstructed later): the
	// reconstruct runs the destructor first, which frees the base's maps, so the default process has them built, at
	// scale 1 (no scaling) until the reconstruct sets the exe's scale.
	BSCloningProcess() : BSCloningProcess(1.0f) {}
	BSCloningProcess(float afScale);
	~BSCloningProcess();

	void SetScale(float afScale);

	NiPoint3 Scale;
};
