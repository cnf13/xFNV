#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"

class NiCloningProcess;
class NiObjectNET;
class NiStream;

// animates the maximum distance of the particle field it targets.
class NiPSysFieldMaxDistanceCtlr : public NiPSysModifierFloatCtlr {
public:
    NiPSysFieldMaxDistanceCtlr();
    ~NiPSysFieldMaxDistanceCtlr() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

protected:
    bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
    void GetTargetFloatValue(float& fValue) override;
    void SetTargetFloatValue(float fValue) override;

public:
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
