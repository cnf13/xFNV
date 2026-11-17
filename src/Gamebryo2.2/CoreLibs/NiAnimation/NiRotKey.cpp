#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

#include <cstdint>

NiQuaternion NiRotKey::ms_kDefault(NiQuaternion_IDENTITY);

void NiRotKey::LoadBinary(NiStream& stream) {
    NiAnimationKey::LoadBinary(stream);
    m_quat.LoadBinary(stream);
}

// Flips each key's quaternion to the hemisphere of the one before it, then clamps every key's w to [-1, 1].
void NiRotKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
    uint32_t i;
    for (i = 0; i < uiNumKeys - 1; i++) {
        NiRotKey* pkKey0 = static_cast<NiRotKey*>(pkKeys->GetKeyAt(i, ucSize));
        NiRotKey* pkKey1 = static_cast<NiRotKey*>(pkKeys->GetKeyAt(i + 1, ucSize));
        // NiQuaternion::Dot, written out with its operands in this order; the sum is rounded to float before the
        // compare.
        float fDot = pkKey1->m_quat.m_fX * pkKey0->m_quat.m_fX + pkKey0->m_quat.m_fW * pkKey1->m_quat.m_fW +
            pkKey1->m_quat.m_fY * pkKey0->m_quat.m_fY + pkKey1->m_quat.m_fZ * pkKey0->m_quat.m_fZ;
        if (fDot < 0.0f) {
            pkKey1->m_quat = -pkKey1->m_quat;
        }
    }

    for (i = 0; i < uiNumKeys; i++) {
        NiRotKey* pkKey = static_cast<NiRotKey*>(pkKeys->GetKeyAt(i, ucSize));
        float fW = pkKey->m_quat.m_fW;
        if (fW < -1.0f) {
            // EXE-INLINED 0x52CE20
            pkKey->m_quat.m_fW = -1.0f;
        }
        else if (fW > 1.0f) {
            // EXE-INLINED 0x52CE20
            pkKey->m_quat.m_fW = 1.0f;
        }
    }
}

// The value at fTime over uiNumKeys keys of ucSize bytes; uiLastIdx caches the key the previous call ended on.
NiQuaternion NiRotKey::GenInterp(
    float fTime,
    NiRotKey* pkKeys,
    KeyType eType,
    uint32_t uiNumKeys,
    uint32_t& uiLastIdx,
    uint8_t ucSize
) {
    // The asm also takes the single-key path when fTime is -NI_INFINITY (compared as the double -FLT_MAX), which the
    // reference source does not.
    if (uiNumKeys == 1 || fTime == -3.4028234663852886e+38) {
        if (eType != EULERKEY) {
            return pkKeys->m_quat;
        }
    }
    if (eType == EULERKEY) {
        InterpFunction interp = ms_interps[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + eType];
        NiQuaternion kQuat;
        interp(fTime, pkKeys, nullptr, &kQuat);
        NiQuaternion kResult;
        kResult.m_fW = kQuat.m_fW;
        kResult.m_fX = kQuat.m_fX;
        kResult.m_fY = kQuat.m_fY;
        kResult.m_fZ = kQuat.m_fZ;
        return kResult;
    }

    uint32_t uiNumKeysM1 = uiNumKeys - 1;
    uint32_t uiLast = uiLastIdx;
    float fLastTime = pkKeys->GetKeyAt(uiLast, ucSize)->m_fTime;
    if (fTime < fLastTime) {
        uiLast = 0;
        fLastTime = pkKeys->m_fTime;
    }
    uint32_t uiNextIdx;
    // fNextTime starts at 0.0f, so when no key follows uiLast the code divides by -fLastTime, as the game does.
    float fNextTime = 0.0f;
    for (uiNextIdx = uiLast + 1; uiNextIdx <= uiNumKeysM1; uiNextIdx++) {
        fNextTime = pkKeys->GetKeyAt(uiNextIdx, ucSize)->m_fTime;
        if (fTime <= fNextTime) {
            break;
        }
        uiLast++;
        fLastTime = fNextTime;
    }
    float fNormTime = (fTime - fLastTime) / (fNextTime - fLastTime);
    InterpFunction interp = ms_interps[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + eType];
    NiQuaternion kQuat;
    interp(fNormTime, pkKeys->GetKeyAt(uiLast, ucSize), pkKeys->GetKeyAt(uiNextIdx, ucSize), &kQuat);
    uiLastIdx = uiLast;
    NiQuaternion kResult;
    kResult.m_fW = kQuat.m_fW;
    kResult.m_fX = kQuat.m_fX;
    kResult.m_fY = kQuat.m_fY;
    kResult.m_fZ = kQuat.m_fZ;
    return kResult;
}

// Returns the first key's value before it and the last key's value after it. A NaN fails both compares and falls
// through to GenInterp. Otherwise GenInterp runs from key 0 with uiLastIdx set to 0.
NiQuaternion
NiRotKey::GenInterpDefault(float fTime, NiRotKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize) {
    const NiQuaternion* pkSource;
    if (!uiNumKeys) {
        pkSource = &ms_kDefault;
    }
    else if (pkKeys->m_fTime > fTime) {
        pkSource = &pkKeys->m_quat;
    }
    else {
        NiRotKey* pkLast = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize);
        if (!(pkLast->m_fTime < fTime)) {
            uint32_t uiLastIdx = 0;
            return GenInterp(fTime, pkKeys, eType, uiNumKeys, uiLastIdx, ucSize);
        }
        pkSource = &pkLast->m_quat;
    }
    NiQuaternion kResult;
    kResult.m_fW = pkSource->m_fW;
    kResult.m_fX = pkSource->m_fX;
    kResult.m_fY = pkSource->m_fY;
    kResult.m_fZ = pkSource->m_fZ;
    return kResult;
}

void NiRotKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
    NiAnimationKey::SaveBinary(stream, pkKey);
    NiRotKey* pkRot = static_cast<NiRotKey*>(pkKey);
    pkRot->m_quat.SaveBinary(stream);
}

// NiAnimationKey::Equal (the times), then the quaternions.
bool NiRotKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    // Compares the times with NiAnimationKey::Equal and tests only its low byte.
    if (!static_cast<uint8_t>(NiAnimationKey::Equal(&key0.m_fTime, &key1.m_fTime))) {
        return false;
    }
    const NiRotKey& rot0 = static_cast<const NiRotKey&>(key0);
    const NiRotKey& rot1 = static_cast<const NiRotKey&>(key1);
    return rot0.m_quat == rot1.m_quat;
}

// NiRotKey's NOINTERP slot has no curvature, derivative or insert functions.
int32_t NiRotKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + NOINTERP;
    ms_curves[uiIndex] = nullptr;
    ms_interpd1s[uiIndex] = nullptr;
    ms_interpd2s[uiIndex] = nullptr;
    ms_inserts[uiIndex] = nullptr;
    bRegistered = 1;
    ms_loaders[uiIndex] = &NiRotKey::CreateFromStream;
    ms_saves[uiIndex] = &NiRotKey::SaveToStream;
    ms_copies[uiIndex] = &NiRotKey::Copy;
    ms_arrays[uiIndex] = &NiRotKey::CreateArray;
    ms_deletes[uiIndex] = &NiRotKey::DeleteArray;
    ms_keysizes[uiIndex] = sizeof(NiRotKey);
    ms_interps[uiIndex] = &NiRotKey::Interpolate;
    ms_equals[uiIndex] = &NiRotKey::Equal;
    ms_fillderived[uiIndex] = &NiRotKey::FillDerivedVals;
    return 1;
}

void NiRotKey::Interpolate(float, const NiAnimationKey*, const NiAnimationKey*, void* pResult) {
    NiQuaternion v4;
    *static_cast<NiQuaternion*>(pResult) = v4.SetValues(1.0, 0.0, 0.0, 0.0);
}
