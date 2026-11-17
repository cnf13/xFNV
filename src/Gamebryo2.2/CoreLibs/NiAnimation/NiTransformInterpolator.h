#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// Interpolates a transform from the position, rotation and scale key arrays of an NiTransformData.
class NiTransformInterpolator : public NiKeyBasedInterpolator {
public:
    // The key channels.
    enum Channel {
        POSITION,
        ROTATION,
        SCALE,
    };

    NiTransformInterpolator(NiTransformData* a1);
    NiTransformInterpolator(NiQuatTransform kPoseValue);
    ~NiTransformInterpolator() override;

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
    void Collapse() override;
    void GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const override;
    void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
    NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime) override;
    uint16_t GetKeyChannelCount() const override {
        return 3;
    }
    uint32_t GetKeyCount(uint16_t usChannel) const override;
    NiAnimationKey::KeyType GetKeyType(uint16_t usChannel) const override;
    NiAnimationKey::KeyContent GetKeyContent(uint16_t usChannel) const override;
    NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
    uint8_t GetKeyStride(uint16_t usChannel) const override;
    bool GetChannelPosed(uint16_t usChannel) const override;
    using NiInterpolator::Update;

    // Takes the transform by value. Each valid part replaces the current value and drops that part's keys.
    void SetPoseValue(NiQuatTransform kPoseValue);
    // Sets the data and restarts the three key searches.
    void SetTransformData(NiTransformData* pkData);
    // A channel's keys, count, type and key size; null and zeros without data.
    NiPosKey* GetPosData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
    NiRotKey* GetRotData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
    NiFloatKey* GetScaleData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;

protected:
    // Copies everything but bPose; the clone shares the data.
    void CopyMembers(NiTransformInterpolator* pkDest, NiCloningProcess& kCloning);

public:
    static NiObject* CreateObject();

    NiQuatTransform m_kTransformValue;
    NiPointer<NiTransformData> m_spData;
    uint16_t m_usLastTransIdx;
    uint16_t m_usLastRotIdx;
    uint16_t m_usLastScaleIdx;
    NiPoint3 m_origTranslate;
    bool bPose;

    static NiRTTI ms_RTTI;
};
