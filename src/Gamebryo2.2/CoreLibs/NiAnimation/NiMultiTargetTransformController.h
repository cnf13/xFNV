#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "SDK/MSVC/ehvec.h"

// Drives the transforms of many targets from one blend interpolator per target.
class NiMultiTargetTransformController : public NiInterpController {
public:
    NiMultiTargetTransformController();
    NiMultiTargetTransformController(uint16_t usCount);
    ~NiMultiTargetTransformController() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void Update(NiUpdateData& UpdateData) override;
    uint16_t GetInterpolatorCount() const override {
        return m_usNumInterps;
    }
    const char* GetInterpolatorID(uint16_t usIndex) override;
    uint16_t GetInterpolatorIndex(const char* pcID) const override;
    uint16_t GetInterpolatorIndexFx(const NiFixedString& arName) const override;
    NiInterpolator* GetInterpolator(uint16_t usIndex) const override;
    void SetInterpolator(NiInterpolator*, uint16_t) override {}
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
    bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t usIndex) const override;

public:
    // Sets the target of interpolator usIndex.
    void SetInterpolatorTarget(uint16_t usIndex, NiAVObject* pkTarget);
    // Removes apNode and every node below it from the targets.
    void RemoveNodeRecurse(NiAVObject* apNode);
    NiBlendTransformInterpolator* GetBlendInterpolator(uint16_t usIndex);
    NiAVObject* GetInterpolatorTarget(uint16_t usIndex);
    uint16_t GetInterpolatorTargetIndex(NiAVObject* pkObj);
    // Puts pkTarget in the first empty target slot and returns its index, or INVALID_INDEX when none is empty.
    uint16_t AddInterpolatorTarget(NiAVObject* pkTarget) {
        for (uint16_t usIndex = 0; usIndex < m_usNumInterps; usIndex++) {
            if (!m_ppkTargets[usIndex]) {
                m_ppkTargets[usIndex] = pkTarget;
                pkTarget->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE;
                return usIndex;
            }
        }
        return INVALID_INDEX;
    }
    void ReallocateStorage();

protected:
    // Sets the interpolator count and, for a non-zero count, builds that many blend interpolators (a reference held
    // on each) and a zeroed target array. The old arrays are not freed.
    void AllocateStorage(uint16_t usCount);
    // The base's part, then fresh blend interpolators copied from these; the targets stay null until ProcessClone.
    void CopyMembers(NiMultiTargetTransformController* pkDest, NiCloningProcess& kCloning);

public:
    static NiObject* CreateObject();

    NiBlendTransformInterpolator* m_pkBlendInterps;
    NiAVObject** m_ppkTargets;
    uint16_t m_usNumInterps;

    static NiRTTI ms_RTTI;
};
