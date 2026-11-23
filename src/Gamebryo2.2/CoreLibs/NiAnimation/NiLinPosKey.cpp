#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

void NiLinPosKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
    NiLinPosKey* pkNewPos = static_cast<NiLinPosKey*>(pkNewKey);
    const NiLinPosKey* pkOrigPos = static_cast<const NiLinPosKey*>(pkOrigKey);
    pkNewPos->m_fTime = pkOrigPos->m_fTime;
    pkNewPos->m_Pos = pkOrigPos->m_Pos;
}

void NiLinPosKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    const NiPosKey* pPosKey0 = static_cast<const NiPosKey*>(pKey0);
    const NiPosKey* pPosKey1 = static_cast<const NiPosKey*>(pKey1);
    // 1 - fTime, each scaled point and each sum are stored as floats.
    float fOneMinusTime = 1.0f - fTime;
    *static_cast<NiPoint3*>(pResult) = fOneMinusTime * pPosKey0->m_Pos + fTime * pPosKey1->m_Pos;
}

NiAnimationKey* NiLinPosKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
    NiLinPosKey* pkLinPosKeys = AllocateKeyArray<NiLinPosKey>(uiNumKeys);
    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        pkLinPosKeys[ui].LoadBinary(stream);
    }
    return pkLinPosKeys;
}

bool NiLinPosKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiLinPosKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiLinPosKey* pkNewKeys = AllocateKeyArray<NiLinPosKey>(uiNewNumKeys);

    NiLinPosKey* pkOld = static_cast<NiLinPosKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiLinPosKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiLinPosKey));
    }
    NiPoint3 kPos = NiPosKey::GenInterpDefault(fTime, pkOld, LINKEY, uiNumKeys, sizeof(NiLinPosKey));
    NiLinPosKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_Pos = kPos;
    uiNumKeys++;

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    return true;
}

void NiLinPosKey::RegisterSupportedFunctions(KeyContent eContent, KeyType eType) {
    uint32_t uiIndex = static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType;
    ms_curves[uiIndex] = &NiBoolKey::ComputeMaxCurvature;
    ms_interps[uiIndex] = &NiLinPosKey::Interpolate;
    ms_interpd1s[uiIndex] = &NiLinPosKey::InterpolateD1;
    ms_interpd2s[uiIndex] = &NiPosKey_Interpolate;
    ms_equals[uiIndex] = &NiLinPosKey::Equal;
    ms_fillderived[uiIndex] = &NiPosKey::FillDerivedVals;
    ms_inserts[uiIndex] = &NiLinPosKey::Insert;
}

int32_t NiLinPosKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    bRegistered = 1;
    ms_loaders[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + LINKEY] = &NiLinPosKey::CreateFromStream;
    ms_saves[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + LINKEY] = &NiLinPosKey::SaveToStream;
    ms_copies[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + LINKEY] = &NiLinPosKey::Copy;
    ms_arrays[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + LINKEY] = &NiBezFloatKey::CreateArray;
    ms_deletes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + LINKEY] = &NiStepPosKey::DeleteArray;
    ms_keysizes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + LINKEY] = sizeof(NiLinPosKey);
    RegisterSupportedFunctions(POSKEY, LINKEY);
    return 1;
}

void NiLinPosKey::InterpolateD1(float, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult) {
    const NiPosKey* pkPos0 = static_cast<const NiPosKey*>(pkKey0);
    const NiPosKey* pkPos1 = static_cast<const NiPosKey*>(pkKey1);
    NiPoint3* pkResult = static_cast<NiPoint3*>(pResult);
    float v6 = pkPos1->m_Pos.x - pkPos0->m_Pos.x;
    float v7 = pkPos1->m_Pos.y - pkPos0->m_Pos.y;
    double v4 = pkPos1->m_Pos.z - pkPos0->m_Pos.z;
    pkResult->x = v6;
    pkResult->y = v7;
    float v8 = v4;
    pkResult->z = v8;
}

bool NiLinPosKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    return NiPosKey_Equal(key0, key1);
}

void NiLinPosKey::SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiPosKey* pkPosKey = static_cast<NiPosKey*>(pkKeys);
    for (uint32_t i = uiNumKeys; i; --i) {
        NiPosKey::SaveBinary(kStream, pkPosKey);
        ++pkPosKey;
    }
}
