#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepBoolKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

void NiStepBoolKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
    pkNewKey->m_fTime = pkOrigKey->m_fTime;
    static_cast<NiBoolKey*>(pkNewKey)->m_bBool = static_cast<const NiBoolKey*>(pkOrigKey)->m_bBool;
}

bool NiStepBoolKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    return NiBoolKey::Equal(key0, key1);
}

// Takes pkKey0 only when 1 is above fTime; an unordered compare (NaN fTime) takes pkKey1.
void NiStepBoolKey::Interpolate(
    float fTime,
    const NiAnimationKey* pkKey0,
    const NiAnimationKey* pkKey1,
    void* pvResult
) {
    const NiAnimationKey* pkKey = pkKey0;
    if (!(fTime < 1.0f)) {
        pkKey = pkKey1;
    }
    *static_cast<bool*>(pvResult) = static_cast<const NiBoolKey*>(pkKey)->m_bBool;
}

bool NiStepBoolKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiBoolKey))) {
        return false;
    }

    // EXE-INLINED none: NiNew NiStepBoolKey[uiNewNumKeys] (the overflow-clamped size, the count cookie and
    // MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys; the casts below and before Deallocate step over that cookie.
    uint32_t uiNewNumKeys = uiNumKeys + 1;
    uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiBoolKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiBoolKey* pkNewKeys;
    if (puiCookie) {
        *puiCookie = uiNewNumKeys;
        pkNewKeys = reinterpret_cast<NiBoolKey*>(puiCookie + 1);
    }
    else {
        pkNewKeys = nullptr;
    }

    NiBoolKey* pkOld = static_cast<NiBoolKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiBoolKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiBoolKey));
    }
    uint8_t ucBool = NiBoolKey::GenInterpDefault(fTime, pkOld, NiAnimationKey::STEPKEY, uiNumKeys, sizeof(NiBoolKey));
    NiBoolKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    // Stores GenInterpDefault's raw byte into the flag: the field is treated as an untyped byte here, not a normalised
    // bool.
    reinterpret_cast<uint8_t&>(pkNewKey->m_bBool) = ucBool;
    uiNumKeys++;

    // EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
    if (pkOldKeys) {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
    }
    pkKeys = pkNewKeys;
    return true;
}

NiAnimationKey* NiStepBoolKey::CreateFromStream(NiStream& kStream, uint32_t uiNumKeys) {
    // EXE-INLINED none: NiNew NiStepBoolKey[uiNumKeys] (the overflow-clamped size, the count cookie and
    // MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys.
    uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiBoolKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiBoolKey* pkKeys;
    if (puiCookie) {
        *puiCookie = uiNumKeys;
        pkKeys = reinterpret_cast<NiBoolKey*>(puiCookie + 1);
    }
    else {
        pkKeys = nullptr;
    }
    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        pkKeys[ui].LoadBinary(kStream);
    }
    return pkKeys;
}

// Clears the slot's curvature and derivative entries before it sets the guard, then stores its own functions'
// addresses. The save function and the array allocator are the linear float key's (the same 8-byte key); the
// fill-derived entry is the shared empty NiBoolKey_FillDerivedVals.
int32_t NiStepBoolKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex =
        static_cast<uint32_t>(NiAnimationKey::BOOLKEY) * NiAnimationKey::NUMKEYTYPES + NiAnimationKey::STEPKEY;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiStepBoolKey::CreateFromStream;
    NiAnimationKey::ms_saves[uiIndex] = &NiLinFloatKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiStepBoolKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiLinFloatKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiBoolKey);
    NiAnimationKey::ms_interps[uiIndex] = &NiStepBoolKey::Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiStepBoolKey::Equal;
    NiAnimationKey::ms_fillderived[uiIndex] = &NiBoolKey::FillDerivedVals;
    NiAnimationKey::ms_inserts[uiIndex] = &NiStepBoolKey::Insert;
    return 1;
}
