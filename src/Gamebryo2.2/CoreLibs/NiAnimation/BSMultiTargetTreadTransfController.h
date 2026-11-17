#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSBlendTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"

class NiAVObject;
class NiNode;

// Drives tank-tread links: each link lerps between two transforms by its blend interpolator.
class BSMultiTargetTreadTransfController : public NiInterpController {
public:
    struct LinkAVObj {
        uint16_t usBlendIdx;
        // Padding; AllocateStorage zeroes it together with usBlendIdx.
        uint16_t unk02;
        NiAVObject* pTarget;
        BSTreadTransfInterpolator::LinkInfo* pLink;
    };

    BSMultiTargetTreadTransfController();
    BSMultiTargetTreadTransfController(uint16_t ausCount, uint16_t ausLinkCount);
    ~BSMultiTargetTreadTransfController() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void Update(NiUpdateData& UpdateData) override;
    uint16_t GetInterpolatorCount() const override {
        return usNumInterps;
    }
    const char* GetInterpolatorID(uint16_t ausIndex) override;
    uint16_t GetInterpolatorIndex(const char* pcID) const override;
    uint16_t GetInterpolatorIndexFx(const NiFixedString& arName) const override;
    NiInterpolator* GetInterpolator(uint16_t ausIndex) const override;
    void SetInterpolator(NiInterpolator*, uint16_t) override {}
    NiInterpolator* CreatePoseInterpolator(uint16_t) override {
        return nullptr;
    }
    void SynchronizePoseInterpolator(NiInterpolator*, uint16_t) override {}
    NiBlendInterpolator* CreateBlendInterpolator(
        uint16_t ausIndex,
        int32_t abManagerControlled,
        bool abAccumulateAnimations,
        float afWeightThreshold,
        int32_t aucArraySize
    ) const override;
    void GuaranteeTimeRange(float afStartTime, float afEndTime) override;

protected:
    bool TargetIsRequiredType() const override;
    bool InterpolatorIsCorrectType(NiInterpolator* apInterpolator, uint16_t ausIndex) const override;

public:
    // Finds the interpolator target for apTarget.
    uint16_t GetInterpolatorTargetIndex(NiAVObject* apTarget);
    // Removes apObject and its descendant targets.
    void RemoveNodeRecurse(NiNode* apObject);
    // Sets the target at ausIndex.
    void A35FC0(uint16_t ausIndex, NiNode* apTarget);
    void AllocateStorage(uint16_t usCount, uint16_t ausLinkCount);
    void SetInterpolatorTarget(
        uint16_t ausIndex,
        NiAVObject* apTarget,
        uint16_t ausBlendIdx,
        BSTreadTransfInterpolator::LinkInfo* apLink
    );
    uint16_t
    AddInterpolatorTarget(NiAVObject* apTarget, uint16_t ausBlendIdx, BSTreadTransfInterpolator::LinkInfo* apLink);

    static NiObject* CreateObject();

    BSBlendTreadTransfInterpolator* BlendInterpsA;
    uint16_t usNumInterps;
    LinkAVObj** pLinksA;
    uint16_t usNumLinks;

    static NiRTTI ms_RTTI;
};
