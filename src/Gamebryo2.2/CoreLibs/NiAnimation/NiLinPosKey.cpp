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
    // 1 - fTime, each scaled point and each sum are stored as floats, as the game does.
    float fOneMinusTime = 1.0f - fTime;
    // EXE-INLINED 0x4A3760
    float fX0 = pPosKey0->m_Pos.x * fOneMinusTime;
    float fY0 = pPosKey0->m_Pos.y * fOneMinusTime;
    float fZ0 = fOneMinusTime * pPosKey0->m_Pos.z;
    // EXE-INLINED 0x4A3760
    float fX1 = pPosKey1->m_Pos.x * fTime;
    float fY1 = pPosKey1->m_Pos.y * fTime;
    float fZ1 = fTime * pPosKey1->m_Pos.z;
    // EXE-INLINED 0x439E90
    NiPoint3* pkResult = static_cast<NiPoint3*>(pResult);
    pkResult->x = fX1 + fX0;
    pkResult->y = fY0 + fY1;
    pkResult->z = fZ1 + fZ0;
}

NiAnimationKey* NiLinPosKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
    // EXE-INLINED none: NiNew NiLinPosKey[uiNumKeys] (the overflow-clamped size, the count cookie and
    // MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys; the cast below steps over that cookie.
    uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiLinPosKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiLinPosKey* pkLinPosKeys;
    if (puiCookie) {
        *puiCookie = uiNumKeys;
        pkLinPosKeys = reinterpret_cast<NiLinPosKey*>(puiCookie + 1);
    }
    else {
        pkLinPosKeys = nullptr;
    }
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

    // EXE-INLINED none: NiNew NiLinPosKey[uiNewNumKeys] (the overflow-clamped size, the count cookie and
    // MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys; the casts below and before Deallocate step over that cookie.
    uint32_t uiNewNumKeys = uiNumKeys + 1;
    uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiLinPosKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiLinPosKey* pkNewKeys;
    if (puiCookie) {
        *puiCookie = uiNewNumKeys;
        pkNewKeys = reinterpret_cast<NiLinPosKey*>(puiCookie + 1);
    }
    else {
        pkNewKeys = nullptr;
    }

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

    // EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
    if (pkOldKeys) {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
    }
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
    return ::NiPosKey_Equal(key0, key1);
}

void NiLinPosKey::SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiPosKey* pkPosKey = static_cast<NiPosKey*>(pkKeys);
    for (uint32_t i = uiNumKeys; i; --i) {
        NiPosKey::SaveBinary(kStream, pkPosKey);
        ++pkPosKey;
    }
}
