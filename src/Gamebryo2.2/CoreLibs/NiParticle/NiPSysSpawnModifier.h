#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiParticleSystem;
class NiPSysData;
class NiStream;

// spawns new particles from old ones.
class NiPSysSpawnModifier : public NiPSysModifier {
public:
    NiPSysSpawnModifier();
    ~NiPSysSpawnModifier() override;

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
    void Update(float, NiPSysData*) override {}
    virtual void
    SpawnParticles(float fCurrentTime, float fSpawnTime, int32_t usOldIndex, NiParticleSystem* pkOldPSystem);
    virtual void
    SpawnParticle(float fCurrentTime, float fSpawnTime, int32_t aiOldIndex, NiParticleSystem* pkOldPSystem);

    // Stores m_fLifeSpan.
    void SetLifeSpan(float fLifeSpan);

    static NiObject* CreateObject();

    uint16_t m_usNumSpawnGenerations;
    float m_fPercentageSpawned;
    uint16_t m_usMinNumToSpawn;
    uint16_t m_usMaxNumToSpawn;
    float m_fSpawnSpeedChaos;
    float m_fSpawnDirChaos;
    float m_fLifeSpan;
    float m_fLifeSpanVar;

    static NiRTTI ms_RTTI;
};
