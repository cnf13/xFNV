#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

void NiStepPosKey::DeleteArray(NiAnimationKey* pkKeyArray) {
    NiAnimationKey::FreeKeyArray(pkKeyArray);
}

// pkKey0 is taken only when 1 is above fTime; an unordered compare takes pkKey1.
void NiStepPosKey::Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult) {
    const NiPosKey* pkPos = static_cast<const NiPosKey*>(pkKey0);
    if (!(fTime < 1.0f)) {
        pkPos = static_cast<const NiPosKey*>(pkKey1);
    }
    NiPoint3* pkResult = static_cast<NiPoint3*>(pResult);
    pkResult->x = pkPos->m_Pos.x;
    pkResult->y = pkPos->m_Pos.y;
    pkResult->z = pkPos->m_Pos.z;
}

void NiStepPosKey::InterpolateD1(float, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult) {
    const NiPosKey* pkPos0 = static_cast<const NiPosKey*>(pkKey0);
    const NiPosKey* pkPos1 = static_cast<const NiPosKey*>(pkKey1);
    NiPoint3* pkResult = static_cast<NiPoint3*>(pResult);
    float v6 = pkPos0->m_Pos.x - pkPos1->m_Pos.x;
    float v7 = pkPos0->m_Pos.y - pkPos1->m_Pos.y;
    double v4 = pkPos0->m_Pos.z - pkPos1->m_Pos.z;
    pkResult->x = v6;
    pkResult->y = v7;
    float v8 = static_cast<float>(v4);
    pkResult->z = v8;
}

bool NiStepPosKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiStepPosKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiStepPosKey* pkNewKeys = AllocateKeyArray<NiStepPosKey>(uiNewNumKeys);

    NiStepPosKey* pkOld = static_cast<NiStepPosKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiStepPosKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiStepPosKey));
    }
    NiPoint3 kPos = NiPosKey::GenInterpDefault(fTime, pkOld, NiAnimationKey::STEPKEY, uiNumKeys, sizeof(NiStepPosKey));
    NiStepPosKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_Pos = kPos;
    uiNumKeys++;

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    return true;
}

void NiStepPosKey::RegisterSupportedFunctions(NiAnimationKey::KeyContent eContent, NiAnimationKey::KeyType eType) {
    uint32_t uiIndex = static_cast<uint32_t>(eContent) * NiAnimationKey::NUMKEYTYPES + eType;
    NiAnimationKey::ms_curves[uiIndex] = &NiBoolKey::ComputeMaxCurvature;
    NiAnimationKey::ms_interps[uiIndex] = &NiStepPosKey::Interpolate;
    NiAnimationKey::ms_interpd1s[uiIndex] = &NiStepPosKey::InterpolateD1;
    NiAnimationKey::ms_interpd2s[uiIndex] = &NiPosKey_Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiLinPosKey::Equal;
    NiAnimationKey::ms_fillderived[uiIndex] = &NiPosKey::FillDerivedVals;
    NiAnimationKey::ms_inserts[uiIndex] = &NiStepPosKey::Insert;
}

int32_t NiStepPosKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(NiAnimationKey::POSKEY) * NiAnimationKey::NUMKEYTYPES +
        NiAnimationKey::STEPKEY;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiLinPosKey::CreateFromStream;
    NiAnimationKey::ms_saves[uiIndex] = &NiLinPosKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiLinPosKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiBezFloatKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiStepPosKey);
    NiStepPosKey::RegisterSupportedFunctions(NiAnimationKey::POSKEY, NiAnimationKey::STEPKEY);
    return 1;
}
