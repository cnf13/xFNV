#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <bit>

// NiRotKey's constructor zeroes the key's quaternion.
NiBezRotKey::NiBezRotKey() {}

void NiBezRotKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
    pkNewKey->m_fTime = pkOrigKey->m_fTime;
    pkNewKey[1].m_fTime = pkOrigKey[1].m_fTime;
    pkNewKey[2].m_fTime = pkOrigKey[2].m_fTime;
    pkNewKey[3].m_fTime = pkOrigKey[3].m_fTime;
    pkNewKey[4].m_fTime = pkOrigKey[4].m_fTime;
    pkNewKey[5].m_fTime = pkOrigKey[5].m_fTime;
    pkNewKey[6].m_fTime = pkOrigKey[6].m_fTime;
    pkNewKey[7].m_fTime = pkOrigKey[7].m_fTime;
    pkNewKey[8].m_fTime = pkOrigKey[8].m_fTime;
}

bool NiBezRotKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiBezRotKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiBezRotKey* pkNewKeys = AllocateKeyArray<NiBezRotKey>(uiNewNumKeys);

    NiBezRotKey* pkOld = static_cast<NiBezRotKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiBezRotKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiBezRotKey));
    }
    NiQuaternion kQuat = GenInterpDefault(fTime, pkOld, BEZKEY, uiNumKeys, sizeof(NiBezRotKey));
    NiBezRotKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_quat = kQuat;
    uiNumKeys++;

    FreeKeyArray(pkOldKeys);
    pkKeys = pkNewKeys;
    FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiBezRotKey));
    return true;
}

void NiBezRotKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
    if (uiNumKeys < 2) {
        return;
    }

    NiRotKey::FillDerivedVals(pkKeys, uiNumKeys, ucSize);

    NiBezRotKey* pkBezKeys = static_cast<NiBezRotKey*>(pkKeys);
    NiQuaternion kResult;
    uint32_t uiNumKeysM1 = uiNumKeys - 1;
    pkBezKeys[0].m_IntQuat = *NiQuaternion::Intermediate(
        &kResult,
        pkBezKeys[0].m_quat,
        pkBezKeys[0].m_quat,
        pkBezKeys[1].m_quat
    );
    for (uint32_t i = 1; i < uiNumKeysM1; i++) {
        pkBezKeys[i].m_IntQuat = *NiQuaternion::Intermediate(
            &kResult,
            pkBezKeys[i - 1].m_quat,
            pkBezKeys[i].m_quat,
            pkBezKeys[i + 1].m_quat
        );
    }
    pkBezKeys[uiNumKeysM1].m_IntQuat = *NiQuaternion::Intermediate(
        &kResult,
        pkBezKeys[uiNumKeysM1 - 1].m_quat,
        pkBezKeys[uiNumKeysM1].m_quat,
        pkBezKeys[uiNumKeysM1].m_quat
    );
}

int32_t NiBezRotKey::RegisterLoader() {
    static char bRegistered;
    int32_t result = 0;
    if (!bRegistered) {
        NiAnimationKey::ms_curves[14] = nullptr;
        NiAnimationKey::ms_interpd1s[14] = nullptr;
        NiAnimationKey::ms_interpd2s[14] = nullptr;
        bRegistered = 1;
        NiAnimationKey::ms_loaders[14] = &NiBezRotKey::CreateFromStream;
        NiAnimationKey::ms_saves[14] = &NiBezRotKey::SaveToStream;
        NiAnimationKey::ms_copies[14] = &NiBezRotKey::Copy;
        NiAnimationKey::ms_arrays[14] = &NiBezRotKey::CreateArray;
        NiAnimationKey::ms_deletes[14] = &NiStepPosKey::DeleteArray;
        NiAnimationKey::ms_keysizes[14] = 36;
        NiAnimationKey::ms_interps[14] = &NiBezRotKey::Interpolate;
        NiAnimationKey::ms_equals[14] = &NiBezRotKey::Equal;
        NiAnimationKey::ms_fillderived[14] = &NiBezRotKey::FillDerivedVals;
        NiAnimationKey::ms_inserts[14] = &NiBezRotKey::Insert;
        return 1;
    }
    return result;
}

void NiBezRotKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    const NiBezRotKey* pkBez0 = static_cast<const NiBezRotKey*>(pKey0);
    const NiBezRotKey* pkBez1 = static_cast<const NiBezRotKey*>(pKey1);
    static_cast<NiQuaternion*>(pResult)
        ->Squad(fTime, pkBez0->m_quat, pkBez0->m_IntQuat, pkBez1->m_IntQuat, pkBez1->m_quat);
}

// NiRotKey::Equal, then the interpolation quaternions.
bool NiBezRotKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    if (!NiRotKey::Equal(key0, key1)) {
        return false;
    }
    const NiBezRotKey& bez0 = static_cast<const NiBezRotKey&>(key0);
    const NiBezRotKey& bez1 = static_cast<const NiBezRotKey&>(key1);
    return bez0.m_IntQuat == bez1.m_IntQuat;
}

NiAnimationKey* NiBezRotKey::CreateArray(uint32_t uiNumKeys) {
    size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiBezRotKey) ? sizeof(NiBezRotKey) * uiNumKeys : SIZE_MAX;
    size_t* v2 = static_cast<size_t*>(MemoryManager::Instance().Allocate(
        ((static_cast<uint32_t>(v1) + static_cast<uint32_t>(sizeof(size_t))) < static_cast<uint32_t>(v1))
            ? -1
            : v1 + sizeof(size_t)
    ));
    if (!v2) {
        return nullptr;
    }
    // new[]'s block, treated as untyped raw memory: the count cookie, then the keys.
    *v2 = uiNumKeys;
    NiAnimationKey* v3 = ::new (static_cast<void*>(v2 + 1)) NiBezRotKey[static_cast<int32_t>(uiNumKeys)];
    return v3;
}

NiAnimationKey* NiBezRotKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
    NiBezRotKey* pkBezRotKeys = AllocateKeyArray<NiBezRotKey>(uiNumKeys);

    for (uint32_t i = 0; i < uiNumKeys; i++) {
        pkBezRotKeys[i].LoadBinary(stream);
    }
    return pkBezRotKeys;
}

void NiBezRotKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    for (uint32_t i = uiNumKeys; i; --i) {
        NiRotKey::SaveBinary(stream, pkKeys);
        pkKeys += 9;
    }
}
