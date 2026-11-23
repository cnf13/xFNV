#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// The rotation, position and scale key arrays an NiTransformInterpolator samples.
class NiTransformData : public NiObject {
public:
    NiTransformData();
    ~NiTransformData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& stream) override;
    // LinkObject and RegisterStreamables do only what the NiObject versions do.
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

    // Adds keys at afStartTime and afEndTime to every channel that lacks them.
    void GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime);
    // A new data object holding only the keys between afStartTime and afEndTime.
    void GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiTransformData>& aspTransformData);

    // Each frees the channel's current keys and takes over pkKeys.
    void ReplaceRotAnim(NiRotKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    void ReplacePosAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    void ReplaceScaleAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    void SetPosAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    void SetScaleAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    NiRotKey* GetRotAnim(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize);

    static NiObject* CreateObject();

    uint16_t m_usNumRotKeys;
    uint16_t m_usNumPosKeys;
    uint16_t m_usNumScaleKeys;
    NiRotKey::KeyType m_eRotType;
    NiPosKey::KeyType m_ePosType;
    NiFloatKey::KeyType m_eScaleType;
    uint8_t m_ucRotSize;
    uint8_t m_ucPosSize;
    uint8_t m_ucScaleSize;
    NiRotKey* m_pkRotKeys;
    NiPosKey* m_pkPosKeys;
    NiFloatKey* m_pkScaleKeys;

    static NiRTTI ms_RTTI;
};
