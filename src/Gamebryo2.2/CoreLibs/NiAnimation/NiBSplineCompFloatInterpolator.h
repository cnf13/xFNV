#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineFloatInterpolator.h"

// A float B-spline with compressed control points.
class NiBSplineCompFloatInterpolator : public NiBSplineFloatInterpolator {
public:
    // Indices into m_afCompScalars: the offset and half range that decompress each channel's 16-bit control points.
    enum {
        FLOAT_OFFSET,
        FLOAT_RANGE,
        NUM_SCALARS,
    };

    NiBSplineCompFloatInterpolator(NiBSplineData* pkData, uint32_t kFloatCPHandle, NiBSplineBasisData* pkBasisData);
    ~NiBSplineCompFloatInterpolator() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    bool Update(float fTime, NiObjectNET* pkInterpTarget, float& fValue) override;
    bool UsesCompressedControlPoints() const override {
        return true;
    }
    using NiInterpolator::Update;

    static NiObject* CreateObject();

    float m_afCompScalars[NUM_SCALARS];

    static NiRTTI ms_RTTI;
};
