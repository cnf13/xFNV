#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezPosKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <cstdint>

void NiBezPosKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
    NiBezPosKey* pkNewBez = static_cast<NiBezPosKey*>(pkNewKey);
    const NiBezPosKey* pkOrigBez = static_cast<const NiBezPosKey*>(pkOrigKey);

    pkNewBez->m_fTime = pkOrigBez->m_fTime;
    pkNewBez->m_Pos = pkOrigBez->m_Pos;
    pkNewBez->m_InTan = pkOrigBez->m_InTan;
    pkNewBez->m_OutTan = pkOrigBez->m_OutTan;
    pkNewBez->m_A = pkOrigBez->m_A;
    pkNewBez->m_B = pkOrigBez->m_B;
}

// Compares as NiPosKey::Equal, then the tangents m_InTan and m_OutTan.
bool NiBezPosKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    if (!NiPosKey_Equal(key0, key1)) {
        return false;
    }
    const NiBezPosKey& bez0 = static_cast<const NiBezPosKey&>(key0);
    const NiBezPosKey& bez1 = static_cast<const NiBezPosKey&>(key1);
    if (!(bez0.m_InTan == bez1.m_InTan)) {
        return false;
    }
    return bez0.m_OutTan == bez1.m_OutTan;
}

// Computes m_A = 3*(Pos1 - Pos0) - (InTan1 + 2*OutTan0) and m_B = OutTan0 + InTan1 - 2*(Pos1 - Pos0) component by
// component, storing each intermediate as a float, with 2.0 and 3.0 as doubles. Pos1 - Pos0 is computed twice and
// ucSize is not read.
void NiBezPosKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
    NiBezPosKey* pkBezPosKeys = static_cast<NiBezPosKey*>(pkKeys);

    for (uint32_t uiI = 0; uiI < uiNumKeys - 1; uiI++) {
        NiBezPosKey* pBez0 = &pkBezPosKeys[uiI];
        NiBezPosKey* pBez1 = &pkBezPosKeys[uiI + 1];

        float fOut2X = pBez0->m_OutTan.x * 2.0;
        float fOut2Y = pBez0->m_OutTan.y * 2.0;
        float fOut2Z = pBez0->m_OutTan.z * 2.0;
        float fInOut2X = pBez1->m_InTan.x + fOut2X;
        float fInOut2Y = pBez1->m_InTan.y + fOut2Y;
        float fInOut2Z = pBez1->m_InTan.z + fOut2Z;
        float fDeltaX = pBez1->m_Pos.x - pBez0->m_Pos.x;
        float fDeltaY = pBez1->m_Pos.y - pBez0->m_Pos.y;
        float fDeltaZ = pBez1->m_Pos.z - pBez0->m_Pos.z;
        float fDelta3X = fDeltaX * 3.0;
        float fDelta3Y = fDeltaY * 3.0;
        float fDelta3Z = fDeltaZ * 3.0;
        pBez0->m_A.x = fDelta3X - fInOut2X;
        pBez0->m_A.y = fDelta3Y - fInOut2Y;
        pBez0->m_A.z = fDelta3Z - fInOut2Z;

        float fDelta2X = (pBez1->m_Pos.x - pBez0->m_Pos.x) * 2.0;
        float fDelta2Y = (pBez1->m_Pos.y - pBez0->m_Pos.y) * 2.0;
        float fDelta2Z = (pBez1->m_Pos.z - pBez0->m_Pos.z) * 2.0;
        float fInOutX = pBez1->m_InTan.x + pBez0->m_OutTan.x;
        float fInOutY = pBez1->m_InTan.y + pBez0->m_OutTan.y;
        float fInOutZ = pBez1->m_InTan.z + pBez0->m_OutTan.z;
        pBez0->m_B.x = fInOutX - fDelta2X;
        pBez0->m_B.y = fInOutY - fDelta2Y;
        pBez0->m_B.z = fInOutZ - fDelta2Z;
    }
}

NiAnimationKey* NiBezPosKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
    NiBezPosKey* pkBezPosKeys = AllocateKeyArray<NiBezPosKey>(uiNumKeys);

    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        pkBezPosKeys[ui].NiPosKey::LoadBinary(stream);
        pkBezPosKeys[ui].m_InTan.LoadBinary(stream);
        pkBezPosKeys[ui].m_OutTan.LoadBinary(stream);
    }
    return pkBezPosKeys;
}

void NiBezPosKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiBezPosKey* pkBezPosKeys = static_cast<NiBezPosKey*>(pkKeys);

    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        NiPosKey::SaveBinary(stream, &pkBezPosKeys[ui]);
        pkBezPosKeys[ui].m_InTan.SaveBinary(stream);
        pkBezPosKeys[ui].m_OutTan.SaveBinary(stream);
    }
}

bool NiBezPosKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiBezPosKey* pkBezPosKeys = static_cast<NiBezPosKey*>(pkKeys);

    uint32_t uiInsertAt;
    if (!OkayToInsert(fTime, pkBezPosKeys, uiNumKeys, uiInsertAt, sizeof(NiBezPosKey))) {
        return false;
    }

    uint32_t uiNewNumKeys = uiNumKeys + 1;
    NiBezPosKey* pkNewKeys = AllocateKeyArray<NiBezPosKey>(uiNewNumKeys);

    rt::memcpy(pkNewKeys, pkBezPosKeys, uiInsertAt * sizeof(NiBezPosKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(
            &pkNewKeys[uiInsertAt + 1],
            &pkBezPosKeys[uiInsertAt],
            (uiNumKeys - uiInsertAt) * sizeof(NiBezPosKey)
        );
    }

    NiPoint3 kPt = NiPosKey::GenInterpDefault(fTime, pkBezPosKeys, BEZKEY, uiNumKeys, sizeof(NiBezPosKey));

    NiBezPosKey* pkNewKey = &pkNewKeys[uiInsertAt];
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_Pos = kPt;
    pkNewKey->m_InTan = NiPoint3::ZERO;
    pkNewKey->m_OutTan = NiPoint3::ZERO;

    // Between two keys, fit the new key's tangents and the neighbours' facing tangents (NiInterpScalar::AdjustBezier,
    // per component).
    if (uiInsertAt != 0 && uiInsertAt != uiNumKeys) {
        NiBezPosKey* pkLastKey = &pkNewKeys[uiInsertAt - 1];
        NiPoint3 kLastValue = pkLastKey->m_Pos;
        float fLastTime = pkLastKey->m_fTime;
        NiPoint3 kLastOut = pkLastKey->m_OutTan;

        NiPoint3 kCurrentValue = pkNewKey->m_Pos;
        NiPoint3 kCurrentIn;
        NiPoint3 kCurrentOut;

        NiBezPosKey* pkNextKey = &pkNewKeys[uiInsertAt + 1];
        NiPoint3 kNextValue = pkNextKey->m_Pos;
        float fNextTime = pkNextKey->m_fTime;
        NiPoint3 kNextIn = pkNextKey->m_InTan;

        // Passes fTime (the new key's time) as the current time.
        NiInterpScalar::AdjustBezier(
            kLastValue.x,
            fLastTime,
            &kLastOut.x,
            kNextValue.x,
            fNextTime,
            &kNextIn.x,
            fTime,
            kCurrentValue.x,
            &kCurrentIn.x,
            &kCurrentOut.x
        );
        NiInterpScalar::AdjustBezier(
            kLastValue.y,
            fLastTime,
            &kLastOut.y,
            kNextValue.y,
            fNextTime,
            &kNextIn.y,
            fTime,
            kCurrentValue.y,
            &kCurrentIn.y,
            &kCurrentOut.y
        );
        NiInterpScalar::AdjustBezier(
            kLastValue.z,
            fLastTime,
            &kLastOut.z,
            kNextValue.z,
            fNextTime,
            &kNextIn.z,
            fTime,
            kCurrentValue.z,
            &kCurrentIn.z,
            &kCurrentOut.z
        );

        pkLastKey->m_OutTan = kLastOut;
        pkNewKey->m_InTan = kCurrentIn;
        pkNewKey->m_OutTan = kCurrentOut;
        pkNextKey->m_InTan = kNextIn;
    }

    uiNumKeys++;
    FreeKeyArray(pkBezPosKeys);
    pkKeys = pkNewKeys;
    FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiBezPosKey));
    return true;
}

void NiBezPosKey::RegisterSupportedFunctions(KeyContent eContent, KeyType eType) {
    uint32_t uiIndex = static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType;
    ms_curves[uiIndex] = &NiBezPosKey::ComputeMaxCurvature;
    ms_interps[uiIndex] = &NiBezPosKey::Interpolate;
    ms_interpd1s[uiIndex] = &NiBezPosKey::InterpolateD1;
    ms_interpd2s[uiIndex] = &NiBezPosKey::InterpolateD2;
    ms_equals[uiIndex] = &NiBezPosKey::Equal;
    ms_fillderived[uiIndex] = &NiBezPosKey::FillDerivedVals;
    ms_inserts[uiIndex] = &NiBezPosKey::Insert;
}

int32_t NiBezPosKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    bRegistered = 1;
    ms_loaders[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + BEZKEY] = &NiBezPosKey::CreateFromStream;
    ms_saves[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + BEZKEY] = &NiBezPosKey::SaveToStream;
    ms_copies[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + BEZKEY] = &NiBezPosKey::Copy;
    ms_arrays[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + BEZKEY] = &NiBezPosKey::CreateArray;
    ms_deletes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + BEZKEY] = &NiStepPosKey::DeleteArray;
    ms_keysizes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + BEZKEY] = sizeof(NiBezPosKey);
    RegisterSupportedFunctions(POSKEY, BEZKEY);
    return 1;
}

// Same as NiTCBPosKey::ComputeMaxCurvature except for the key stride and the two derivative callees. The cross
// product's components, its squared length, the square root and the quotient of d1.Cross(d2).Length() and
// d1.SqrLength() are each stored as floats.
float NiBezPosKey::ComputeMaxCurvature(NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiBezPosKey* pkBezPosKeys = static_cast<NiBezPosKey*>(pkKeys);

    float fMaxCurvature = 0.0f;

    for (uint32_t uiI = 0; uiI < uiNumKeys - 1; uiI++) {
        NiBezPosKey* pBez0 = &pkBezPosKeys[uiI];
        NiBezPosKey* pBez1 = &pkBezPosKeys[uiI + 1];

        for (int32_t iJ = 0; iJ <= ms_iSampleSize; iJ++) {
            float fTime = iJ * GetMaxCurvatureSampleDelta();
            NiPoint3 d1;
            alignas(8) NiPoint3 d2;
            InterpolateD1(fTime, pBez0, pBez1, &d1);
            InterpolateD2(fTime, pBez0, pBez1, &d2);
            NiPoint3 kCross = d1.Cross(d2);
            float fCrossLength = kCross.Length();
            float fSqrLength = d1.SqrLength();
            float fCurvature = fCrossLength / fSqrLength;
            if (fCurvature > fMaxCurvature) {
                fMaxCurvature = fCurvature;
            }
        }
    }

    return fMaxCurvature;
}

void NiBezPosKey::InterpolateD1(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    double v4 = fTime;
    float fTimea = 3.0 * fTime;
    float v9 = pKey0[13].GetTime() * fTimea;
    float v11 = pKey0[14].GetTime() * fTimea;
    float v13 = fTimea * pKey0[15].GetTime();
    float v6 = pKey0[10].GetTime() * 2.0;
    float v7 = pKey0[11].GetTime() * 2.0;
    float v8 = 2.0 * pKey0[12].GetTime();
    float v15 = v6 + v9;
    float v17 = v7 + v11;
    float v19 = v8 + v13;
    float v10 = v15 * v4;
    float v12 = v17 * v4;
    float v14 = v4 * v19;
    float v16 = pKey0[7].GetTime() + v10;
    float v18 = pKey0[8].GetTime() + v12;
    double v5 = pKey0[9].GetTime() + v14;
    *static_cast<float*>(pResult) = v16;
    *(static_cast<float*>(pResult) + 1) = v18;
    float v20 = v5;
    *(static_cast<float*>(pResult) + 2) = v20;
}

void NiBezPosKey::InterpolateD2(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    float fTimea = fTime * 6.0;
    float v7 = pKey0[13].GetTime() * fTimea;
    float v8 = pKey0[14].GetTime() * fTimea;
    float v9 = fTimea * pKey0[15].GetTime();
    float v4 = pKey0[10].GetTime() * 2.0;
    float v5 = pKey0[11].GetTime() * 2.0;
    float v6 = 2.0 * pKey0[12].GetTime();
    float v10 = v4 + v7;
    *static_cast<float*>(pResult) = v10;
    float v11 = v8 + v5;
    *(static_cast<float*>(pResult) + 1) = v11;
    float v12 = v6 + v9;
    *(static_cast<float*>(pResult) + 2) = v12;
}

// Allocates like NiNew NiBezPosKey[uiNumKeys]: a count cookie, then the keys (no constructors). The block is untyped
// memory filled by hand, hence the reinterpret_cast to the keys past the cookie.
NiAnimationKey* NiBezPosKey::CreateArray(uint32_t uiNumKeys) {
    size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiBezPosKey) ? sizeof(NiBezPosKey) * uiNumKeys : SIZE_MAX;
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

// The cubic ((m_B * t + m_A) * t + m_OutTan) * t + m_Pos of pkKey0 at fTime; pkKey1 is unused.
void NiBezPosKey::Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey*, void* pResult) {
    const NiBezPosKey* pkBez = static_cast<const NiBezPosKey*>(pkKey0);
    NiPoint3* pkResult = static_cast<NiPoint3*>(pResult);
    float v6 = pkBez->m_B.x * fTime;
    float v9 = pkBez->m_B.y * fTime;
    float v12 = pkBez->m_B.z * fTime;
    float v15 = pkBez->m_A.x + v6;
    float v18 = pkBez->m_A.y + v9;
    float v21 = pkBez->m_A.z + v12;
    float v7 = v15 * fTime;
    float v10 = v18 * fTime;
    float v13 = v21 * fTime;
    float v16 = pkBez->m_OutTan.x + v7;
    float v19 = pkBez->m_OutTan.y + v10;
    float v22 = pkBez->m_OutTan.z + v13;
    float v8 = v16 * fTime;
    float v11 = v19 * fTime;
    float v14 = fTime * v22;
    float v17 = pkBez->m_Pos.x + v8;
    float v20 = pkBez->m_Pos.y + v11;
    double v5 = pkBez->m_Pos.z + v14;
    pkResult->x = v17;
    pkResult->y = v20;
    float v23 = static_cast<float>(v5);
    pkResult->z = v23;
}
