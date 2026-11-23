#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"

// Animates the fire period of its target's refraction shader.
class BSRefractionFirePeriodController : public NiFloatInterpController {
public:
    BSRefractionFirePeriodController();
    ~BSRefractionFirePeriodController() override;

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
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
