#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

void NiLinFloatKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    *static_cast<float*>(pResult) = NiInterpScalar::Linear(fTime, pKey0[1].m_fTime, pKey1[1].m_fTime);
}

bool NiLinFloatKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiLinFloatKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiLinFloatKey* pkNewKeys = AllocateKeyArray<NiLinFloatKey>(uiNewNumKeys);

    NiLinFloatKey* pkOld = static_cast<NiLinFloatKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiLinFloatKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiLinFloatKey));
    }
    float fValue = GenInterpDefault(fTime, pkOld, LINKEY, uiNumKeys, sizeof(NiLinFloatKey));
    NiLinFloatKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_fValue = fValue;
    uiNumKeys++;

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    return true;
}

// Clears the slot's curvature and derivative entries before it sets the guard, then stores its own functions'
// addresses.
int32_t NiLinFloatKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(FLOATKEY) * NUMKEYTYPES + LINKEY;
    ms_curves[uiIndex] = nullptr;
    ms_interpd1s[uiIndex] = nullptr;
    ms_interpd2s[uiIndex] = nullptr;
    bRegistered = 1;
    ms_loaders[uiIndex] = &NiLinFloatKey::CreateFromStream;
    ms_saves[uiIndex] = &NiAnimationKey::SaveToStream;
    ms_copies[uiIndex] = &NiLinFloatKey::Copy;
    ms_arrays[uiIndex] = &NiLinFloatKey::CreateArray;
    ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    ms_keysizes[uiIndex] = sizeof(NiLinFloatKey);
    ms_interps[uiIndex] = &NiLinFloatKey::Interpolate;
    ms_equals[uiIndex] = &NiLinFloatKey::Equal;
    ms_fillderived[uiIndex] = &NiLinFloatKey::FillDerivedVals;
    ms_inserts[uiIndex] = &NiLinFloatKey::Insert;
    return 1;
}

// This deliberately saves the key the way NiBoolKey::SaveBinary does, not as a NiFloatKey: after the key's time it
// writes the bool as a single byte, 0 or 1.
void NiLinFloatKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
    NiAnimationKey::SaveBinary(stream, pkKey);
    NiBoolKey* pkBool = static_cast<NiBoolKey*>(pkKey);
    // Compares the whole byte with 0; a bool read would copy it unchanged.
    uint8_t ucBool = static_cast<uint8_t>(pkBool->m_bBool) != 0;
    NiStreamSaveBinary(stream, ucBool);
}

void NiLinFloatKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiLinFloatKey* pkLinFloatKeys = static_cast<NiLinFloatKey*>(pkKeys);

    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        SaveBinary(stream, &pkLinFloatKeys[ui]);
    }
}

NiAnimationKey* NiLinFloatKey::CreateFromStream(NiStream& arStream, uint32_t auiCount) {
    NiAnimationKey* v5;
    uint32_t v2 = auiCount;
    size_t v3 = auiCount <= SIZE_MAX / sizeof(NiFloatKey) ? sizeof(NiFloatKey) * auiCount : SIZE_MAX;
    size_t* v4 = static_cast<size_t*>(MemoryManager::Instance().Allocate(
        ((static_cast<uint32_t>(v3) + static_cast<uint32_t>(sizeof(size_t))) < static_cast<uint32_t>(v3))
            ? -1
            : v3 + sizeof(size_t)
    ));
    if (v4) {
        // new[]'s block, treated as untyped raw memory: the count cookie, then the keys.
        *v4 = auiCount;
        v5 = reinterpret_cast<NiAnimationKey*>(v4 + 1);
    }
    else {
        v5 = nullptr;
    }
    if (auiCount) {
        NiFloatKey* v6 = static_cast<NiFloatKey*>(v5);
        do {
            v6++->LoadBinary(arStream);
            --v2;
        }
        while (v2);
    }
    return v5;
}

// Equivalent to NiNew NiLinFloatKey[uiNumKeys]: the count cookie, then the keys (no constructors). The block is untyped
// memory filled by hand, hence the reinterpret_cast to the keys past the cookie.
NiAnimationKey* NiLinFloatKey::CreateArray(uint32_t uiNumKeys) {
    size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiLinFloatKey) ? sizeof(NiLinFloatKey) * uiNumKeys : SIZE_MAX;
    size_t* v2 = static_cast<size_t*>(MemoryManager::Instance().Allocate(
        ((static_cast<uint32_t>(v1) + static_cast<uint32_t>(sizeof(size_t))) < static_cast<uint32_t>(v1))
            ? -1
            : v1 + sizeof(size_t)
    ));
    if (!v2) {
        return nullptr;
    }
    *v2 = uiNumKeys;
    return reinterpret_cast<NiAnimationKey*>(v2 + 1);
}

bool NiLinFloatKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    return NiFloatKey::Equal(key0, key1);
}

void NiLinFloatKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
    NiLinFloatKey* pkNew = static_cast<NiLinFloatKey*>(pkNewKey);
    const NiLinFloatKey* pkOrig = static_cast<const NiLinFloatKey*>(pkOrigKey);
    pkNew->m_fTime = pkOrig->m_fTime;
    pkNew->m_fValue = pkOrig->m_fValue;
}
