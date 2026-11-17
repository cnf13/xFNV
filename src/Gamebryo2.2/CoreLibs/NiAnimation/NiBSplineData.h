#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiBSplineBasisData;

// The control points of B-spline interpolators: full floats, and 16-bit values for the compressed interpolators.
class NiBSplineData : public NiObject {
public:
    // Offset of a channel's first control point in the control point arrays.
    using Handle = uint32_t;

    enum {
        INVALID_HANDLE = 0xFFFF,
    };

    NiBSplineData()
      : m_pafControlPoints(nullptr), m_pasCompactControlPoints(nullptr), m_uiControlPointCount(0),
        m_uiCompactControlPointCount(0) {}

    ~NiBSplineData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    // LinkObject and RegisterStreamables only call the NiObject implementations.
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

    // Evaluates the uiDimension floats of the curve whose control points start at kControlPointHandle, at fTime in
    // [0, 1], into afPos.
    void GetValueDegree3(
        float fTime,
        float* afPos,
        uint32_t uiDimension,
        NiBSplineBasisData* pkBasisData,
        Handle kControlPointHandle
    ) const;
    // The same over the 16-bit control points, which map [-32767, 32767] onto fOffset -/+ fHalfRange.
    void GetCompactedValueDegree3(
        float fTime,
        float* afPos,
        uint32_t uiDimension,
        NiBSplineBasisData* pkBasisData,
        Handle kControlPointHandle,
        float fOffset,
        float fHalfRange
    ) const;

    // The stream's factory for the class: no control points.
    static NiObject* CreateObject();

    float* m_pafControlPoints;
    int16_t* m_pasCompactControlPoints;
    uint32_t m_uiControlPointCount;
    uint32_t m_uiCompactControlPointCount;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiBSplineData);
