#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepFloatKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

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

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiFloatKey* pkNewKeys = AllocateKeyArray<NiFloatKey>(uiNewNumKeys);

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

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    return true;
}

int32_t NiStepFloatKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(NiAnimationKey::FLOATKEY) * NiAnimationKey::NUMKEYTYPES +
        NiAnimationKey::STEPKEY;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiLinFloatKey::CreateFromStream;
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
