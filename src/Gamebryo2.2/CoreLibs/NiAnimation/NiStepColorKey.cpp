#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepColorKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

NiAnimationKey* NiStepColorKey::CreateArray(uint32_t uiNumKeys) {
    // EXE-INLINED none: NiNew NiStepColorKey[uiNumKeys] (the overflow-clamped size, the count cookie,
    // MemoryManager::Allocate and the vector constructor iterator). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys.
    uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiColorKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    if (!puiCookie) {
        return nullptr;
    }
    *puiCookie = uiNumKeys;
    NiColorKey* pkKeys = ::new (static_cast<void*>(puiCookie + 1)) NiColorKey[uiNumKeys];
    return pkKeys;
}

// pkKey0 is taken only when 1 is above fTime; an unordered compare takes pkKey1.
void NiStepColorKey::Interpolate(
    float fTime,
    const NiAnimationKey* pkKey0,
    const NiAnimationKey* pkKey1,
    void* pvResult
) {
    const NiAnimationKey* pkKey = pkKey0;
    if (!(fTime < 1.0f)) {
        pkKey = pkKey1;
    }
    *static_cast<NiColorA*>(pvResult) = static_cast<const NiColorKey*>(pkKey)->m_Color;
}

NiAnimationKey* NiStepColorKey::CreateFromStream(NiStream& kStream, uint32_t uiNumKeys) {
    // EXE-INLINED none: NiNew NiStepColorKey[uiNumKeys], as in CreateArray (raw memory to the exe: the count cookie,
    // then the keys).
    uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiColorKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiColorKey* pkKeys;
    if (puiCookie) {
        *puiCookie = uiNumKeys;
        pkKeys = ::new (static_cast<void*>(puiCookie + 1)) NiColorKey[uiNumKeys];
    }
    else {
        pkKeys = nullptr;
    }
    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        pkKeys[ui].LoadBinary(kStream);
    }
    return pkKeys;
}

bool NiStepColorKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiColorKey))) {
        return false;
    }

    // EXE-INLINED none: NiNew NiStepColorKey[uiNewNumKeys] (the overflow-clamped size, the count cookie,
    // MemoryManager::Allocate and the vector constructor iterator). The block is raw memory to the exe: a uint32_t
    // count cookie, then the keys; the casts below and before Deallocate step over that cookie.
    uint32_t uiNewNumKeys = uiNumKeys + 1;
    uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiColorKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiColorKey* pkNewKeys;
    if (puiCookie) {
        *puiCookie = uiNewNumKeys;
        pkNewKeys = ::new (static_cast<void*>(puiCookie + 1)) NiColorKey[uiNewNumKeys];
    }
    else {
        pkNewKeys = nullptr;
    }

    NiColorKey* pkOld = static_cast<NiColorKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiColorKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiColorKey));
    }
    NiColorA kColor =
        NiColorKey::GenInterpDefault(fTime, pkOld, NiAnimationKey::STEPKEY, uiNumKeys, sizeof(NiColorKey));
    NiColorKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_Color = kColor;
    uiNumKeys++;

    // EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
    if (pkOldKeys) {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
    }
    pkKeys = pkNewKeys;
    return true;
}

// COLORKEY/STEPKEY: the slot's curvature, derivative and fill-derived entries are cleared before the guard is set, then
// this class's own functions are stored. The save, copy and equality functions are the linear color key's (the same key
// layout).
int32_t NiStepColorKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex =
        static_cast<uint32_t>(NiAnimationKey::COLORKEY) * NiAnimationKey::NUMKEYTYPES + NiAnimationKey::STEPKEY;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    NiAnimationKey::ms_fillderived[uiIndex] = nullptr;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiStepColorKey::CreateFromStream;
    NiAnimationKey::ms_saves[uiIndex] = &NiLinColorKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiLinColorKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiStepColorKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiColorKey);
    NiAnimationKey::ms_interps[uiIndex] = &NiStepColorKey::Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiLinColorKey::Equal;
    NiAnimationKey::ms_inserts[uiIndex] = &NiStepColorKey::Insert;
    return 1;
}
