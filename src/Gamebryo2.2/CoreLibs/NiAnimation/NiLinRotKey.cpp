#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

NiAnimationKey* NiLinRotKey::CreateArray(uint32_t uiNumKeys) {
    return AllocateKeyArray<NiLinRotKey>(uiNumKeys);
}

void NiLinRotKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    const NiRotKey* pRotKey0 = static_cast<const NiRotKey*>(pKey0);
    const NiRotKey* pRotKey1 = static_cast<const NiRotKey*>(pKey1);
    static_cast<NiQuaternion*>(pResult)->Slerp(fTime, pRotKey0->m_quat, pRotKey1->m_quat);
}

NiAnimationKey* NiLinRotKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
    NiLinRotKey* pkLinRotKeys = AllocateKeyArray<NiLinRotKey>(uiNumKeys);
    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        pkLinRotKeys[ui].LoadBinary(stream);
    }
    return pkLinRotKeys;
}

bool NiLinRotKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiLinRotKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiLinRotKey* pkNewKeys = AllocateKeyArray<NiLinRotKey>(uiNewNumKeys);

    NiLinRotKey* pkOld = static_cast<NiLinRotKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiLinRotKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiLinRotKey));
    }
    NiQuaternion kQuat = NiRotKey::GenInterpDefault(fTime, pkOld, LINKEY, uiNumKeys, sizeof(NiLinRotKey));
    NiLinRotKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_quat = kQuat;
    uiNumKeys++;

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    NiRotKey::FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiLinRotKey));
    return true;
}

// Clears the slot's curvature and derivative entries before it sets the guard, then stores its own functions'
// addresses.
int32_t NiLinRotKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + LINKEY;
    ms_curves[uiIndex] = nullptr;
    ms_interpd1s[uiIndex] = nullptr;
    ms_interpd2s[uiIndex] = nullptr;
    bRegistered = 1;
    ms_loaders[uiIndex] = &NiLinRotKey::CreateFromStream;
    ms_saves[uiIndex] = &NiLinRotKey::SaveToStream;
    ms_copies[uiIndex] = &NiLinColorKey::Copy;
    ms_arrays[uiIndex] = &NiLinRotKey::CreateArray;
    ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    ms_keysizes[uiIndex] = sizeof(NiLinRotKey);
    ms_interps[uiIndex] = &NiLinRotKey::Interpolate;
    ms_equals[uiIndex] = &NiLinRotKey::Equal;
    ms_fillderived[uiIndex] = &NiLinRotKey::FillDerivedVals;
    ms_inserts[uiIndex] = &NiLinRotKey::Insert;
    return 1;
}

void NiLinRotKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
    NiRotKey::FillDerivedVals(pkKeys, uiNumKeys, ucSize);
}

bool NiLinRotKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    return NiRotKey::Equal(key0, key1);
}

void NiLinRotKey::SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiRotKey* pkRotKeys = static_cast<NiRotKey*>(pkKeys);
    for (uint32_t i = uiNumKeys; i; --i) {
        NiRotKey::SaveBinary(kStream, pkRotKeys);
        pkRotKeys++;
    }
}
