#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiAVObject;
class NiCloningProcess;
class NiStream;

// base of the modifiers that apply a force field around an object.
class NiPSysFieldModifier : public NiPSysModifier {
public:
    NiPSysFieldModifier();
    ~NiPSysFieldModifier() override;

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

    // NiPSysModifier's CopyMembers, then the field object, magnitude, attenuation and maximum
    // distance into pkDest.
    void CopyMembers(NiPSysFieldModifier* pkDest, NiCloningProcess& kCloning);

    NiAVObject* m_pkFieldObj;
    float m_fMagnitude;
    float m_fAttenuation;
    bool m_bUseMaxDistance;
    float m_fMaxDistance;
    float m_fMaxDistanceSqr;

    static NiRTTI ms_RTTI;
};
