#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A cloning pass that also scales the clones by Scale.
class BSCloningProcess : public NiCloningProcess {
public:
    // Scale 1: no scaling.
    BSCloningProcess() : BSCloningProcess(1.0f) {}
    BSCloningProcess(float afScale);
    ~BSCloningProcess();

    void SetScale(float afScale);
    float QScaleU() {
        return Scale.x;
    }

    static float QScaleU(NiCloningProcess& arCloning) {
        return static_cast<BSCloningProcess&>(arCloning).QScaleU();
    }

    NiPoint3 Scale;
};
