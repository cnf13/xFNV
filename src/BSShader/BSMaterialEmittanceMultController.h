#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"

// Animates the emittance multiplier of its target material.
class BSMaterialEmittanceMultController : public NiFloatInterpController {
public:
    BSMaterialEmittanceMultController();
    ~BSMaterialEmittanceMultController() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void Update(NiUpdateData& UpdateData) override;

protected:
    bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
    void GetTargetFloatValue(float& fValue) override;

public:
    // Stream factory for this class: returns a new controller.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
