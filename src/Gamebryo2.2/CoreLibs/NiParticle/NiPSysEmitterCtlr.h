#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"

class NiBlendInterpolator;
class NiBoolInterpolator;
class NiCloningProcess;
class NiFloatInterpolator;
class NiInterpolator;
class NiObjectNET;
class NiStream;
struct NiUpdateData;

// animates the birth rate and the active flag of a particle emitter.
class NiPSysEmitterCtlr : public NiPSysModifierCtlr {
public:
    NiPSysEmitterCtlr();
    NiPSysEmitterCtlr(const char* apcModifierName);
    ~NiPSysEmitterCtlr() override;

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
    void Stop() override;
    void Update(NiUpdateData& UpdateData) override;
    uint16_t GetInterpolatorCount() const override {
        return 2;
    }
    const char* GetInterpolatorID(uint16_t usIndex) override;
    uint16_t GetInterpolatorIndex(const char* pcID) const override;
    NiInterpolator* GetInterpolator(uint16_t usIndex) const override;
    void SetInterpolator(NiInterpolator* pkInterpolator, uint16_t usIndex) override;
    void ResetTimeExtrema() override;
    NiInterpolator* CreatePoseInterpolator(uint16_t usIndex) override;
    void SynchronizePoseInterpolator(NiInterpolator*, uint16_t) override {}
    NiBlendInterpolator* CreateBlendInterpolator(
        uint16_t usIndex,
        int32_t bManagerControlled,
        bool bAccumulateAnimations,
        float fWeightThreshold,
        int32_t ucArraySize
    ) const override;
    void GuaranteeTimeRange(float fStartTime, float fEndTime) override;

protected:
    bool TargetIsRequiredType() const override;
    bool InterpolatorIsCorrectType(NiInterpolator* pkInterp, uint16_t usIndex) const override;
    bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
    virtual void SetTargetData() {}

public:
    // The birth rate interpolator, or the highest weighted one of a blend; null when there is none.
    NiFloatInterpolator* GetBirthRateInterpolator();
    // The active interpolator, or the highest weighted one of a blend; null when there is none.
    NiBoolInterpolator* GetEmitterActiveInterpolator();

protected:
    void CopyMembers(NiPSysEmitterCtlr* pkDest, NiCloningProcess& kCloning);
    // Emits the particles born between fLastTime and fCurTime of the emit window [fEmitStart, fEmitStop] at
    // fBirthRate per second, at most sLimitParticlesS, with their ages at fTime.
    void
    EmitParticles(float fTime, float fCurTime, float fLastTime, float fEmitStart, float fEmitStop, float fBirthRate);

public:
    static void SetParticlesLimit(uint16_t asLimit);
    static NiObject* CreateObject();

    NiPointer<NiInterpolator> m_spEmitterActiveInterpolator;
    NiInterpolator* m_pkLastBirthRateInterpolator;
    float m_fLastScaledTime;
    bool m_bLastActive;
    NiTPrimitiveSet<float> m_kParticleAges;

    static NiRTTI ms_RTTI;
    // The IDs of the two interpolators: "BirthRate" (index 0) and "EmitterActive" (index 1).
    static inline const char* ms_pcBirthRate = "BirthRate";
    static inline const char* ms_pcEmitterActive = "EmitterActive";
    // The most particles one EmitParticles call emits.
    static inline uint16_t sLimitParticlesS = 15;
};
