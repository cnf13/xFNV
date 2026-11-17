#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"

class NiObjectNET;

// Blends transforms and accumulates the blended motion of its accumulation root over time.
class NiBlendAccumTransformInterpolator : public NiBlendInterpolator {
public:
    // The accumulation state of one blended interpolator.
    struct AccumArrayItem {
        // Every transform invalid, the reference frame the identity.
        AccumArrayItem();

        // Forgets the last sample: time, last value and delta invalid, reference frame back to the identity.
        void ClearValues() {
            m_fLastTime = -FLT_MAX;
            m_kLastValue.m_kTranslate.x = -FLT_MAX;
            m_kLastValue.m_kRotate.m_fX = -FLT_MAX;
            m_kLastValue.m_fScale = -FLT_MAX;
            m_kDeltaValue.m_kTranslate.x = -FLT_MAX;
            m_kDeltaValue.m_kRotate.m_fX = -FLT_MAX;
            m_kDeltaValue.m_fScale = -FLT_MAX;
            m_kRefFrame = NiMatrix3_IDENTITY;
        }

        float m_fLastTime;
        NiQuatTransform m_kLastValue;
        NiQuatTransform m_kDeltaValue;
        NiMatrix3 m_kRefFrame;
    };

    NiBlendAccumTransformInterpolator();
    // abManagerControlled is an int32_t rather than a bool so the caller's whole value is passed on; a bool would be
    // zero-extended.
    NiBlendAccumTransformInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
    ~NiBlendAccumTransformInterpolator() override;

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
    bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) override;
    bool IsTransformValueSupported() const override {
        return true;
    }
    uint8_t AddInterpInfo(NiInterpolator* pkInterpolator, float fWeight, char cPriority, float fEaseSpinner) override;
    void RemoveInterpInfo(uint8_t ucIndex) override;
    bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
    bool ReallocateArrays() override;
    // Returns the accumulated transform; false while it is invalid.
    virtual bool GetValue(NiQuatTransform& kValue) const;
    using NiInterpolator::Update;

    void ClearAccumulatedTransform();

protected:
    // Takes the accumulated motion of the only interpolator since its last sample.
    bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue);
    // Weighs the motion deltas of all interpolators into the accumulated transform.
    bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue);
    // Samples one interpolator and stores its motion delta since its last sample.
    void CalculateValue(uint8_t ucIndex, float fTime, NiObjectNET* pkInterpTarget);
    void CopyMembers(NiBlendAccumTransformInterpolator* pkDest, NiCloningProcess& kCloning);

public:
    void CalculateSingleValue(float fTime, NiObjectNET* pkInterpTarget);

    static NiObject* CreateObject();

    NiQuatTransform m_kAccumulatedTransformValue;
    AccumArrayItem* m_pkAccumArray;
    bool m_bReset;

    static NiRTTI ms_RTTI;
};
