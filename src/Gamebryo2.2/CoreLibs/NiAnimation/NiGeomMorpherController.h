#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMorphData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "SDK/MSVC/ehvec.h"

class NiPoint3;

// Blends morph targets into its target geometry's vertices.
class NiGeomMorpherController : public NiInterpController {
public:
    enum MorpherFlags : uint16_t {
        UPDATE_NORMALS_MASK = 0x1,
    };

    // The weight interpolator of one morph target and the weight it last produced.
    class InterpItem : public NiMemObject {
    public:
        InterpItem();

        NiPointer<NiInterpolator> m_spInterpolator;
        float m_fWeight;
    };

    NiGeomMorpherController();
    ~NiGeomMorpherController() override;

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
    void SetTarget(NiObjectNET* pkTarget) override;
    bool IsVertexController() const override {
        return true;
    }
    void OnPreDisplay() override;
    uint16_t GetInterpolatorCount() const override;
    const char* GetInterpolatorID(uint16_t usIndex) override;
    uint16_t GetInterpolatorIndex(const char* pcID) const override;
    uint16_t GetInterpolatorIndexFx(const NiFixedString& arName) const override;
    NiInterpolator* GetInterpolator(uint16_t usIndex) const override;
    void SetInterpolator(NiInterpolator* pkInterpolator, uint16_t usIndex) override;
    void ResetTimeExtrema() override;
    NiInterpolator* CreatePoseInterpolator(uint16_t usIndex) override;
    void SynchronizePoseInterpolator(NiInterpolator* pkInterp, uint16_t usIndex) override;
    NiBlendInterpolator* CreateBlendInterpolator(
        uint16_t usIndex,
        int32_t bManagerControlled,
        bool bAccumulateAnimations,
        float fWeightThreshold,
        int32_t ucArraySize
    ) const override;
    void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
    // the target geometry's vertex array, created (uninitialized) when the data has none; null without a target
    virtual NiPoint3* GetVertexData();

protected:
    bool TargetIsRequiredType() const override;
    bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t usIndex) const override;
    // Sets the target data's bound to enclose every morph target.
    virtual void CalculateMorphBound();

public:
    // the morph data's target count, 0 without morph data
    uint32_t GetNumTargets() const;
    // Replaces the morph data and gives every target a fresh, empty interpolator slot.
    void SetMorphData(NiMorphData* pkData, bool bCalculateMorphBound);

    // the last weight of target auiIndex, 0 without interpolator slots
    float GetWeight(uint32_t uiIndex) const {
        return m_pkInterpItems ? m_pkInterpItems[uiIndex].m_fWeight : 0.0f;
    }

    void SetWeight(uint32_t uiIndex, float fWeight) {
        if (m_pkInterpItems) {
            m_pkInterpItems[uiIndex].m_fWeight = fWeight;
        }
    }

protected:
    void CopyMembers(NiGeomMorpherController* pkDest, NiCloningProcess& kCloning);
    // Updates the weights from the interpolators at the scaled time (afTime is not read) and blends the targets into
    // the vertex array.
    void GenMorphInterp(float fTime);
    // Frees the interpolator slots and allocates one per morph target.
    void AllocateInterpArray();

public:
    // Morph target uiIndex of m_spMorphData; it hides NiTimeController::GetTarget.
    NiMorphData::MorphTarget* GetTarget(uint32_t uiIndex);

    static NiObject* CreateObject();

    // bit 0: recompute the target's normals after a morph
    uint16_t m_uFlags;
    NiPointer<NiMorphData> m_spMorphData;
    // one per morph target (new[] array)
    InterpItem* m_pkInterpItems;
    bool m_bNeedPreDisplay;
    // the base target (target 0 of relative morph data) gets weight 0 instead of 1 and the vertices are not cleared
    bool m_bIgnoreBaseTarget;
    bool m_bAlwaysUpdate;
    bool m_bBoundCalculated;

    static NiRTTI ms_RTTI;
    static inline bool ms_bParallelUpdateEnabled;
    // when set, cloning turns on m_bAlwaysUpdate in both the original and the clone
    static inline bool ms_bAlwaysUpdateClone;
};
