#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

// Interpolates a color from an NiColorData key array.
class NiColorInterpolator : public NiKeyBasedInterpolator {
public:
    // The constructor with the constant color, taken by value.
    NiColorInterpolator(NiColorA kPoseValue);
    // Constructs with the invalid colour and takes a reference to the colour data.
    NiColorInterpolator(NiColorData* pkColorData);
    ~NiColorInterpolator() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    bool Update(float fTime, NiObjectNET* pkInterpTarget, NiColorA& kValue) override;
    bool IsColorAValueSupported() const override {
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
    NiAnimationKey::KeyContent GetKeyContent(uint16_t usChannel) const override;
    NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
    uint8_t GetKeyStride(uint16_t usChannel) const override;
    bool GetChannelPosed(uint16_t usChannel) const override;
    using NiInterpolator::Update;

    // Drops the keys and holds kPoseValue, taken by value.
    void SetPoseValue(NiColorA kPoseValue);
    // Hands the keys to the color data, creating it on first use.
    void ReplaceKeys(NiColorKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    // Assigns m_spColorData.
    void SetColorData(NiColorData* pkColorData);
    // The base members, then the color, the color data (with a reference) and the last index into pkDest.
    void CopyMembers(NiColorInterpolator* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    NiColorA m_kColorValue;
    NiPointer<NiColorData> m_spColorData;
    uint32_t m_uiLastIdx;

    static NiRTTI ms_RTTI;
};

// Hands the keys to the color data, creating it on first use.
inline void NiColorInterpolator::ReplaceKeys(NiColorKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (uiNumKeys) {
        if (!m_spColorData) {
            NiColorData* pkColorData = new NiColorData;
            m_spColorData = pkColorData;
        }
        m_spColorData->ReplaceAnim(pkKeys, uiNumKeys, eType);
        m_uiLastIdx = 0;
    }
    else if (m_spColorData) {
        m_spColorData->ReplaceAnim(nullptr, 0, NiAnimationKey::NOINTERP);
    }
}
