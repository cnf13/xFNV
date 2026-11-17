#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// An interpolator that evaluates B-splines over the control points of an NiBSplineData; one spline per channel.
class NiBSplineInterpolator : public NiInterpolator {
public:
    NiBSplineInterpolator(NiBSplineData* a1, NiBSplineBasisData* a2);
    ~NiBSplineInterpolator() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    // LinkObject only runs the NiKeyBasedInterpolator body.
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void GetActiveTimeRange(float& fBeginTime, float& fEndTime) const override;
    NiInterpolator* GetSequenceInterpolator(float, float) override {
        return nullptr;
    }
    virtual uint16_t GetChannelCount() const = 0;
    virtual uint32_t GetControlPointCount(int32_t usChannel) const;
    virtual uint32_t GetDimension(int32_t aiChannel) const = 0;
    virtual uint32_t GetDegree(uint16_t ausChannel) const = 0;
    virtual bool GetChannelPosed(int32_t aiChannel) const = 0;
    virtual bool UsesCompressedControlPoints() const {
        return false;
    }
    virtual uint32_t GetAllocatedSize(int32_t usChannel) const;
    virtual NiBSplineData::Handle GetControlHandle(int32_t aiChannel) const = 0;
    virtual void SetControlHandle(NiBSplineData::Handle akHandle, uint16_t ausChannel) = 0;

    // The times and data into a clone; the basis is cloned shared, so clones of one pass keep one basis.
    void CopyMembers(NiBSplineInterpolator* pkDest, NiCloningProcess& kCloning);

    float m_fStartTime;
    float m_fEndTime;
    NiPointer<NiBSplineData> m_spData;
    NiPointer<NiBSplineBasisData> m_spBasisData;

    static NiRTTI ms_RTTI;
};
