#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

// Interpolates a bool from an NiBoolData key array; the value byte holds 2 while it is invalid.
class NiBoolInterpolator : public NiKeyBasedInterpolator {
public:
    NiBoolInterpolator(bool abPoseValue);
    NiBoolInterpolator(NiBoolData* pkBoolData);
    ~NiBoolInterpolator() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    bool Update(float fTime, NiObjectNET* pkInterpTarget, bool& bValue) override;
    bool IsBoolValueSupported() const override {
        return true;
    }
    void Collapse() override;
    void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
    NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime) override;
    NiBoolInterpolator* IsNiBoolInterpolator() override {
        return this;
    }
    uint16_t GetKeyChannelCount() const override {
        return 1;
    }
    uint32_t GetKeyCount(uint16_t usChannel) const override;
    NiAnimationKey::KeyType GetKeyType(uint16_t usChannel) const override;
    NiAnimationKey::KeyContent GetKeyContent(uint16_t usChannel) const override;
    NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
    uint8_t GetKeyStride(uint16_t usChannel) const override;
    bool GetChannelPosed(uint16_t usChannel) const override;
    using NiInterpolator::Update;

    // Drops the keys and holds bPoseValue.
    void SetPoseValue(bool bPoseValue);
    // Hands pkKeys to the bool data (created when missing). Unlike the float version, the key type comes before
    // the count.
    void ReplaceKeys(NiBoolKey* pkKeys, NiAnimationKey::KeyType eType, uint32_t uiNumKeys);
    // The data's keys, count, type and stride; null and zeros without data.
    NiBoolKey* GetKeys(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
    // Copies the base part, then the value, the data and the last index; NiBoolInterpolator's and
    // NiBoolTimelineInterpolator's CreateClone call it on this with the new object.
    void CopyMembers(NiBoolInterpolator* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    uint8_t m_bBoolValue;
    NiPointer<NiBoolData> m_spBoolData;
    uint32_t m_uiLastIdx;

    static NiRTTI ms_RTTI;
};
