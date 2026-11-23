#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

#include <cmath>
#include <cstdint>

// The division is done in double precision and rounded to float only on the store.
float NiPosKey::ms_fSampleDelta = 1.0 / NiPosKey::ms_iSampleSize;
NiPoint3 NiPosKey::ms_kDefault(NiPoint3::ZERO);

void NiPosKey_Interpolate(float, const NiAnimationKey*, const NiAnimationKey*, void* pResult) {
    *static_cast<NiPoint3*>(pResult) = NiPoint3::ZERO;
}

bool NiPosKey_Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    if (!static_cast<uint8_t>(NiAnimationKey::Equal(&key0.m_fTime, &key1.m_fTime))) {
        return false;
    }
    const NiPosKey& pos0 = static_cast<const NiPosKey&>(key0);
    const NiPosKey& pos1 = static_cast<const NiPosKey&>(key1);
    return pos0.m_Pos.x == pos1.m_Pos.x && pos0.m_Pos.y == pos1.m_Pos.y && pos0.m_Pos.z == pos1.m_Pos.z;
}

void NiPosKey::LoadBinary(NiStream& stream) {
    NiAnimationKey::LoadBinary(stream);
    m_Pos.LoadBinary(stream);
}

// The position at fTime; uiLastIdx caches the key the previous call ended on.
NiPoint3 NiPosKey::GenInterp(
    float fTime,
    NiPosKey* pkKeys,
    NiAnimationKey::KeyType eType,
    uint32_t uiNumKeys,
    uint32_t& uiLastIdx,
    uint8_t ucSize
) {
    // Also returns the first key's position for fTime == -FLT_MAX. It copies x, then loads y and z
    // before storing them, so a result overlapping the key reads the words in that order.
    if (uiNumKeys == 1 || fTime == -3.4028234663852886e+38) {
        NiPoint3 kFirst;
        kFirst.x = pkKeys->m_Pos.x;
        float fY = pkKeys->m_Pos.y;
        float fZ = pkKeys->m_Pos.z;
        kFirst.y = fY;
        kFirst.z = fZ;
        return kFirst;
    }

    uint32_t uiNumKeysM1 = uiNumKeys - 1;
    uint32_t uiLast = uiLastIdx;
    float fLastTime = pkKeys->GetKeyAt(uiLast, ucSize)->GetTime();
    if (fTime < fLastTime) {
        uiLast = 0;
        fLastTime = pkKeys->GetTime();
    }

    // fNextTime is zeroed before the search.
    float fNextTime = 0.0f;
    uint32_t uiNextIdx;
    for (uiNextIdx = uiLast + 1; uiNextIdx <= uiNumKeysM1; uiNextIdx++) {
        fNextTime = pkKeys->GetKeyAt(uiNextIdx, ucSize)->GetTime();
        // The search stops only when fNextTime >= fTime; an unordered compare goes on searching.
        if (fNextTime >= fTime) {
            break;
        }
        uiLast++;
        fLastTime = fNextTime;
    }

    float fNormTime = (fTime - fLastTime) / (fNextTime - fLastTime);
    InterpFunction interp = GetInterpFunction(eType);
    NiPoint3 kResult;
    interp(fNormTime, pkKeys->GetKeyAt(uiLast, ucSize), pkKeys->GetKeyAt(uiNextIdx, ucSize), &kResult);
    uiLastIdx = uiLast;
    return static_cast<const NiPoint3&>(kResult);
}

// The chosen position is copied into the result field by field in order, so a result overlapping the key reads
// already-copied fields; a struct copy moves the bytes in another order. Each path's local is scoped to it so that it
// is built in the result itself.
NiPoint3 NiPosKey::GenInterpDefault(float fTime, NiPosKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize) {
    if (!uiNumKeys) {
        NiPoint3 kResult;
        kResult.x = ms_kDefault.x;
        kResult.y = ms_kDefault.y;
        kResult.z = ms_kDefault.z;
        return kResult;
    }
    if (pkKeys->m_fTime > fTime) {
        NiPoint3 kResult;
        kResult.x = pkKeys->m_Pos.x;
        kResult.y = pkKeys->m_Pos.y;
        kResult.z = pkKeys->m_Pos.z;
        return kResult;
    }
    NiPosKey* pkLast = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize);
    if (pkLast->m_fTime < fTime) {
        NiPoint3 kResult;
        kResult.x = pkLast->m_Pos.x;
        kResult.y = pkLast->m_Pos.y;
        kResult.z = pkLast->m_Pos.z;
        return kResult;
    }
    // uiNumKeys is reused as the zeroed last index that GenInterp updates.
    uint32_t uiCount = uiNumKeys;
    uiNumKeys = 0;
    return GenInterp(fTime, pkKeys, eType, uiCount, uiNumKeys, ucSize);
}

void NiPosKey::CoordinateFrame(
    float fTime,
    NiPosKey* pKey0,
    NiPosKey* pKey1,
    NiAnimationKey::KeyType eType,
    NiPoint3& tangent,
    NiPoint3& normal,
    NiPoint3& binormal,
    float& curvature
) {
    InterpFunction d1Func = GetInterpD1Function(eType);
    InterpFunction d2Func = GetInterpD2Function(eType);

    NiPoint3 d1;
    NiPoint3 d2;
    d1Func(fTime, pKey0, pKey1, &d1);
    d2Func(fTime, pKey0, pKey1, &d2);

    float fLengthSqr = d1.SqrLength();
    float fLength = std::sqrt(fLengthSqr);
    float fInvLength = 1.0f / fLength;
    tangent = fInvLength * d1;

    NiPoint3 kCross = d1.Cross(d2);
    float fCrossLength = kCross.Length();
    float fCurvature = fCrossLength * fInvLength * fInvLength;
    curvature = fCurvature;

    const float epsilon = 1e-06f;
    if (std::fabs(fCurvature) > epsilon) {
        float fDot = d1.Dot(d2);
        normal = fLengthSqr * d2 - fDot * d1;
    }
    else {
        curvature = 0.0f;
        if (std::fabs(tangent.x) > epsilon || std::fabs(tangent.y) > epsilon) {
            normal.x = tangent.y;
            normal.y = -tangent.x;
            normal.z = 0.0f;
        }
        else {
            normal.z = tangent.y;
            normal.y = -tangent.z;
            normal.x = 0.0f;
        }
    }
    normal.Unitize();

    binormal = tangent.Cross(normal);
}

void NiPosKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
    NiAnimationKey::SaveBinary(stream, pkKey);
    NiPosKey* pkPos = static_cast<NiPosKey*>(pkKey);
    pkPos->m_Pos.SaveBinary(stream);
}

void NiPosKey::RegisterSupportedFunctions(KeyContent eContent, KeyType eType) {
    uint32_t uiIndex = static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType;
    ms_curves[uiIndex] = &NiBoolKey::ComputeMaxCurvature;
    ms_interps[uiIndex] = &NiPosKey_Interpolate;
    ms_interpd1s[uiIndex] = &NiPosKey_Interpolate;
    ms_interpd2s[uiIndex] = &NiPosKey_Interpolate;
    ms_equals[uiIndex] = &NiPosKey_Equal;
    ms_fillderived[uiIndex] = &NiPosKey::FillDerivedVals;
    ms_inserts[uiIndex] = nullptr;
}

int32_t NiPosKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    bRegistered = 1;
    ms_loaders[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + NOINTERP] = &NiPosKey::CreateFromStream;
    ms_saves[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + NOINTERP] = &NiPosKey::SaveToStream;
    ms_copies[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + NOINTERP] = &NiPosKey::Copy;
    ms_arrays[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + NOINTERP] = &NiPosKey::CreateArray;
    ms_deletes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + NOINTERP] = &NiPosKey::DeleteArray;
    ms_keysizes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + NOINTERP] = sizeof(NiPosKey);
    RegisterSupportedFunctions(POSKEY, NOINTERP);
    return 1;
}
