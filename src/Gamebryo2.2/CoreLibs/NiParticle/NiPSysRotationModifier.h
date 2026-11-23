#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// spins particles.
class NiPSysRotationModifier : public NiPSysModifier {
public:
    NiPSysRotationModifier();
    ~NiPSysRotationModifier() override;

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
    void PostLinkObject(NiStream& kStream) override;
    void Update(float fTime, NiPSysData* pkData) override;
    void Initialize(NiPSysData* pkData, uint16_t usNewParticle) override;

    // Copies the base's members, then this modifier's into pkDest, in field order. Hides NiPSysModifier::CopyMembers,
    // as in the engine.
    void CopyMembers(NiPSysRotationModifier* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    float m_fInitialRotSpeed;
    float m_fInitialRotSpeedVar;
    float m_fInitialRotAngle;
    float m_fInitialRotAngleVar;
    NiPoint3 m_kInitialAxis;
    bool m_bRandomInitialAxis;
    bool m_bRandomRotSpeedSign;

    static NiRTTI ms_RTTI;
};
