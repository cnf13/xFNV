#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// An interpolator controller with exactly one interpolator.
class NiSingleInterpController : public NiInterpController {
public:
    NiSingleInterpController();
    ~NiSingleInterpController() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    uint16_t GetInterpolatorCount() const override {
        return 1;
    }
    const char* GetInterpolatorID(uint16_t) override {
        return nullptr;
    }
    uint16_t GetInterpolatorIndex(const char*) const override {
        return 0;
    }
    uint16_t GetInterpolatorIndexFx(const NiFixedString& arName) const override;
    NiInterpolator* GetInterpolator(uint16_t usIndex) const override;
    void SetInterpolator(NiInterpolator* pkInterpolator, uint16_t usIndex) override;
    void ResetTimeExtrema() override;
    void GuaranteeTimeRange(float fStartTime, float fEndTime) override;

protected:
    bool TargetIsRequiredType() const override;
    virtual bool InterpTargetIsCorrectType(NiObjectNET* apTarget) const = 0;

public:
    // The clone shares the interpolator's clone, made once per cloning process.
    void CopyMembers(NiSingleInterpController* pkDest, NiCloningProcess& kCloning);

    NiPointer<NiInterpolator> m_spInterpolator;

    static NiRTTI ms_RTTI;
};
