#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"

// Evaluates position, rotation and scale B-splines into a transform.
class NiBSplineTransformInterpolator : public NiBSplineInterpolator {
public:
    // The spline channels.
    enum Channel {
        POSITION,
        ROTATION,
        SCALE,
    };

    NiBSplineTransformInterpolator(
        NiBSplineData* pkData,
        uint32_t kTransCPHandle,
        uint32_t kRotCPHandle,
        uint32_t kScaleCPHandle,
        NiBSplineBasisData* pkBasisData
    );
    ~NiBSplineTransformInterpolator() override;

    const NiRTTI* GetRTTI() const override;
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
    uint16_t GetChannelCount() const override {
        return 3;
    }
    uint32_t GetDimension(int32_t usChannel) const override;
    uint32_t GetDegree(uint16_t usChannel) const override;
    bool GetChannelPosed(int32_t usChannel) const override;
    NiBSplineData::Handle GetControlHandle(int32_t usChannel) const override;
    void SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) override;
    using NiInterpolator::Update;

    void CopyMembers(NiBSplineTransformInterpolator* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    NiQuatTransform m_kTransformValue;
    NiBSplineData::Handle m_kTransCPHandle;
    NiBSplineData::Handle m_kRotCPHandle;
    NiBSplineData::Handle m_kScaleCPHandle;

    static NiRTTI ms_RTTI;
};
