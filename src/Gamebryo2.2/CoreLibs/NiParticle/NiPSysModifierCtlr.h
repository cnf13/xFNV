#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"

class NiCloningProcess;
class NiObjectNET;
class NiPSysModifier;
class NiStream;

// base of the controllers that animate a particle modifier, found by name.
class NiPSysModifierCtlr : public NiSingleInterpController {
public:
    NiPSysModifierCtlr();
    NiPSysModifierCtlr(const NiFixedString& akModifierName);
    ~NiPSysModifierCtlr() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void PostLinkObject(NiStream& kStream) override;
    void SetTarget(NiObjectNET* pkTarget) override;
    const char* GetCtlrID() override {
        return m_kModifierName;
    }
    virtual void HandleReset() {}

protected:
    bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;

    void CopyMembers(NiPSysModifierCtlr* pkDest, NiCloningProcess& kCloning);
    // Points m_pkModifier at the target particle system's modifier named m_kModifierName, or null.
    void GetModifierPointerFromName();

public:
    NiFixedString m_kModifierName;
    NiPSysModifier* m_pkModifier;

    static NiRTTI ms_RTTI;
};
