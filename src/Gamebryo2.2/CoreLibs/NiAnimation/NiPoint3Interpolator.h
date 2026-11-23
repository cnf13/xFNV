#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// Interpolates a point from an NiPosData key array.
class NiPoint3Interpolator : public NiKeyBasedInterpolator {
public:
    // Runs the constructor on apMemory and returns it; the interpolator holds
    // kPoseValue and no data.
    NiPoint3Interpolator(NiPoint3 kPoseValue);
    NiPoint3Interpolator(NiPosData* pkPoint3Data);
    ~NiPoint3Interpolator() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    bool Update(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) override;
    bool IsPoint3ValueSupported() const override {
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
        return NiAnimationKey::POSKEY;
    }
    NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
    uint8_t GetKeyStride(uint16_t usChannel) const override;
    bool GetChannelPosed(uint16_t usChannel) const override;
    using NiInterpolator::Update;

    // Drops the keys and holds kPoseValue, taken by value.
    void SetPoseValue(NiPoint3 kPoseValue);
    void CopyMembers(NiPoint3Interpolator* pkDest, NiCloningProcess& kCloning);
    void SetKeys(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

    static NiObject* CreateObject();

    NiPoint3 m_kPoint3Value;
    NiPointer<NiPosData> m_spPoint3Data;
    uint32_t m_uiLastIdx;

    static NiRTTI ms_RTTI;
};
