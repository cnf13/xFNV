#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepRotKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

bool NiStepRotKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiRotKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiRotKey* pkNewKeys = AllocateKeyArray<NiRotKey>(uiNewNumKeys);

    NiRotKey* pkOld = static_cast<NiRotKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiRotKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiRotKey));
    }
    NiQuaternion kQuat = NiRotKey::GenInterpDefault(fTime, pkOld, NiAnimationKey::STEPKEY, uiNumKeys, sizeof(NiRotKey));
    NiRotKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_quat = kQuat;
    uiNumKeys++;

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    NiRotKey::FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiRotKey));
    return true;
}

int32_t NiStepRotKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(NiAnimationKey::ROTKEY) * NiAnimationKey::NUMKEYTYPES +
        NiAnimationKey::STEPKEY;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiLinRotKey::CreateFromStream;
    NiAnimationKey::ms_saves[uiIndex] = &NiLinRotKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiLinColorKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiLinRotKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiRotKey);
    NiAnimationKey::ms_interps[uiIndex] = &NiStepColorKey::Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiLinRotKey::Equal;
    NiAnimationKey::ms_fillderived[uiIndex] = &NiLinRotKey::FillDerivedVals;
    NiAnimationKey::ms_inserts[uiIndex] = &NiStepRotKey::Insert;
    return 1;
}
