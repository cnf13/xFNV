#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

NiSmartPointer(NiFloatData);

// The float keys an interpolator samples: one key array plus its interpolation type and key size.
class NiFloatData : public NiObject {
public:
    // Runs the default constructor on caller-owned storage.
    NiFloatData();
    ~NiFloatData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

    // The keys with their count, type and key size.
    NiFloatKey* GetAnim(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize);
    // Adds keys at afStartTime and afEndTime when the data lacks them.
    void GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime);
    // A new data object holding only the keys between afStartTime and afEndTime.
    void GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiFloatData>& aspFloatData);

    // Frees the current keys and takes over pkKeys.
    void ReplaceAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
    // Takes over pkKeys without freeing the current keys.
    void SetAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

    static NiObject* CreateObject();

    uint32_t m_uiNumKeys;
    NiFloatKey* m_pkKeys;
    NiFloatKey::KeyType m_eType;
    uint8_t m_ucKeySize;

    static NiRTTI ms_RTTI;
};
