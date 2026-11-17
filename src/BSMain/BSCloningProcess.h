#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A cloning pass that also scales the clones by Scale.
class BSCloningProcess : public NiCloningProcess {
public:
    // The default process has the base's maps built, at scale 1 (no scaling), because a local constructed in place and
    // reconstructed later runs the destructor first, which frees the base's maps; the reconstruct then sets the scale.
    BSCloningProcess() : BSCloningProcess(1.0f) {}
    BSCloningProcess(float afScale);
    ~BSCloningProcess();

    void SetScale(float afScale);

    NiPoint3 Scale;
};
