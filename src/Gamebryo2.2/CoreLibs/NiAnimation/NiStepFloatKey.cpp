#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepFloatKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

void NiStepFloatKey::Interpolate(
    float fTime,
    const NiAnimationKey* pkKey0,
    const NiAnimationKey* pkKey1,
    void* pvResult
) {
    if (!(fTime < 1.0f)) {
        *static_cast<float*>(pvResult) = static_cast<const NiStepFloatKey*>(pkKey1)->m_fValue;
    }
    else {
        *static_cast<float*>(pvResult) = static_cast<const NiStepFloatKey*>(pkKey0)->m_fValue;
    }
}

bool NiStepFloatKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiFloatKey))) {
        return false;
    }

    // EXE-INLINED none: NiNew NiStepFloatKey[uiNewNumKeys] (the overflow-clamped size, the count cookie and
    // MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys; the casts below and before Deallocate step over that cookie.
    uint32_t uiNewNumKeys = uiNumKeys + 1;
    uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiFloatKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiFloatKey* pkNewKeys;
    if (puiCookie) {
        *puiCookie = uiNewNumKeys;
        pkNewKeys = reinterpret_cast<NiFloatKey*>(puiCookie + 1);
    }
    else {
        pkNewKeys = nullptr;
    }

    NiFloatKey* pkOld = static_cast<NiFloatKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiFloatKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiFloatKey));
    }
    float fValue = NiFloatKey::GenInterpDefault(fTime, pkOld, NiAnimationKey::STEPKEY, uiNumKeys, sizeof(NiFloatKey));
    NiFloatKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_fValue = fValue;
    uiNumKeys++;

    // EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
    if (pkOldKeys) {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
    }
    pkKeys = pkNewKeys;
    return true;
}

int32_t NiStepFloatKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex =
        static_cast<uint32_t>(NiAnimationKey::FLOATKEY) * NiAnimationKey::NUMKEYTYPES + NiAnimationKey::STEPKEY;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiAnimationKey::CreateFunction_00A2A5A0;
    NiAnimationKey::ms_saves[uiIndex] = &NiAnimationKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiLinFloatKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiLinFloatKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiStepFloatKey);
    NiAnimationKey::ms_interps[uiIndex] = &NiStepFloatKey::Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiLinFloatKey::Equal;
    NiAnimationKey::ms_fillderived[uiIndex] = &NiLinFloatKey::FillDerivedVals;
    NiAnimationKey::ms_inserts[uiIndex] = &NiStepFloatKey::Insert;
    return 1;
}
