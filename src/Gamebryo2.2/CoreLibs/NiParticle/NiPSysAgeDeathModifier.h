#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiPSysSpawnModifier;
class NiStream;

// kills particles at the end of their life, optionally spawning new ones.
class NiPSysAgeDeathModifier : public NiPSysModifier {
public:
    NiPSysAgeDeathModifier();
    ~NiPSysAgeDeathModifier() override;

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
    void ProcessClone(NiCloningProcess& kCloning) override;
    void Update(float fTime, NiPSysData* pkData) override;

    static NiObject* CreateObject();

    bool m_bSpawnOnDeath;
    NiPSysSpawnModifier* m_pkSpawnModifier;

    static NiRTTI ms_RTTI;
};
