#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// Interpolates a float from an NiFloatData key array.
class NiFloatInterpolator : public NiKeyBasedInterpolator {
public:
    // No data.
    NiFloatInterpolator();
    // The interpolator holds pkFloatData and an invalid value.
    NiFloatInterpolator(NiFloatData* pkFloatData);
    // Runs the constructor on apMemory and returns it; the interpolator holds
    // fPoseValue and no data.
    NiFloatInterpolator(float fPoseValue);
    ~NiFloatInterpolator() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    bool Update(float fTime, NiObjectNET* pkInterpTarget, float& fValue) override;
    bool IsFloatValueSupported() const override {
        return true;
    }
    void Collapse() override;
    void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
    NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime) override;
    uint16_t GetKeyChannelCount() const override {
        return 1;
    }
    uint32_t GetKeyCount(uint16_t usChannel) const override;
    NiAnimationKey::KeyType GetKeyType(uint16_t usChannel) const override;
    NiAnimationKey::KeyContent GetKeyContent(uint16_t) const override {
        return NiAnimationKey::FLOATKEY;
    }
    NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
    uint8_t GetKeyStride(uint16_t usChannel) const override;
    bool GetChannelPosed(uint16_t usChannel) const override;
    using NiInterpolator::Update;

    // Drops the keys and holds fPoseValue.
    void SetPoseValue(float fPoseValue);
    void SetFloatData(NiFloatData* pkFloatData);
    // Hands pkKeys to the float data (created when missing); no keys empty existing data.
    void ReplaceKeys(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    // The data's keys, count, type and stride; null and zeros without data.
    NiFloatKey* GetKeys(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
    void CopyMembers(NiFloatInterpolator* pkDest, NiCloningProcess& kCloning);
    void SetKeys(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

    static NiObject* CreateObject();

    float m_fFloatValue;
    NiPointer<NiFloatData> m_spFloatData;
    uint32_t m_uiLastIdx;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiFloatInterpolator);
