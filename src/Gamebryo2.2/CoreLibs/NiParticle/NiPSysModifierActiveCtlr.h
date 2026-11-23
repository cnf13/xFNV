#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierBoolCtlr.h"

class NiCloningProcess;
class NiObjectNET;
class NiStream;

// animates whether the particle modifier it targets is active.
class NiPSysModifierActiveCtlr : public NiPSysModifierBoolCtlr {
public:
    NiPSysModifierActiveCtlr();
    ~NiPSysModifierActiveCtlr() override;

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
    void GetTargetBoolValue(bool& bValue) override;
    void SetTargetBoolValue(bool bValue) override;

public:
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
