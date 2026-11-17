#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTCBFloatKey.h"

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
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

void NiTCBFloatKey::LoadBinary(NiStream& stream) {
    uint32_t v10;
    NiStream* v2 = &stream;
    NiFloatKey::LoadBinary(stream);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v10 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_fTension, 4, &v10, 1);
    NiBinaryStream* v6 = v2->Istr();
    uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
    v10 = 4;
    v6->m_uiAbsoluteCurrentPos += v7(v6, &m_fContinuity, 4, &v10, 1);
    NiBinaryStream* v8 = v2->Istr();
    uint32_t (*v9)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v8->m_pfnRead;
    v10 = 4;
    v8->m_uiAbsoluteCurrentPos += v9(v8, &m_fBias, 4, &v10, 1);
}

// The Kochanek-Bartels tangents: each side's weights from the tension, continuity and bias, scaled by the interval
// lengths so keys spaced unevenly in time join smoothly.
void NiTCBFloatKey::CalculateDVals(
    NiTCBFloatKey& result,
    float fValSub1,
    float fValPlus1,
    float fPreLen,
    float fNextLen
) {
    float fDiff1 = result.m_fValue - fValSub1;
    float fDiff2 = fValPlus1 - result.m_fValue;
    float fOpBias = result.m_fBias + 1.0f;
    float fOmBias = 1.0f - result.m_fBias;
    float fHomT = (1.0f - result.m_fTension) * 0.5;
    float fOpCon = result.m_fContinuity + 1.0f;
    float fTmp1 = fOpCon * fHomT;
    float fOmCon = 1.0f - result.m_fContinuity;
    float fTmp2 = fHomT * fOmCon;
    float fInvSum = 2.0 / (fPreLen + fNextLen);
    float fOut = fTmp1 * fOmBias * fDiff2 + fTmp2 * fOpBias * fDiff1;
    result.m_fDS = fPreLen * fInvSum * fOut;
    float fIn = fTmp1 * fOpBias * fDiff1 + fDiff2 * (fOmBias * fTmp2);
    result.m_fDD = fIn * (fNextLen * fInvSum);
}

// The engine calls NiInterpScalar::TCB here; this calls NiInterpScalar::Bezier with the same arguments, as the game
// does.
void NiTCBFloatKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
    const NiTCBFloatKey* pTcb0 = static_cast<const NiTCBFloatKey*>(pKey0);
    const NiTCBFloatKey* pTcb1 = static_cast<const NiTCBFloatKey*>(pKey1);

    *static_cast<float*>(pResult) =
        NiInterpScalar::Bezier(fTime, pTcb0->GetValue(), pTcb0->m_fDD, pTcb1->GetValue(), pTcb1->m_fDS);
}

// The mirrored end values are computed as value + value - neighbour, each rounded to a float argument.
void NiTCBFloatKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
    NiTCBFloatKey* pkTCBFloatKeys = static_cast<NiTCBFloatKey*>(pkKeys);

    if (uiNumKeys >= 2) {
        CalculateDVals(
            pkTCBFloatKeys[0], pkTCBFloatKeys[0].m_fValue * 2.0f - pkTCBFloatKeys[1].m_fValue,
            pkTCBFloatKeys[1].m_fValue, 1.0f, 1.0f
        );

        uint32_t uiNumKeysM1 = uiNumKeys - 1;
        for (uint32_t uiI = 1; uiI < uiNumKeysM1; uiI++) {
            CalculateDVals(
                pkTCBFloatKeys[uiI], pkTCBFloatKeys[uiI - 1].m_fValue, pkTCBFloatKeys[uiI + 1].m_fValue,
                pkTCBFloatKeys[uiI].m_fTime - pkTCBFloatKeys[uiI - 1].m_fTime,
                pkTCBFloatKeys[uiI + 1].m_fTime - pkTCBFloatKeys[uiI].m_fTime
            );
        }

        uint32_t uiNumKeysM2 = uiNumKeys - 2;
        CalculateDVals(
            pkTCBFloatKeys[uiNumKeysM1], pkTCBFloatKeys[uiNumKeysM2].m_fValue,
            2.0f * pkTCBFloatKeys[uiNumKeysM1].m_fValue - pkTCBFloatKeys[uiNumKeysM2].m_fValue, 1.0f, 1.0f
        );
    }
}

void NiTCBFloatKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
    NiTCBFloatKey* pkTCBFloatKeys = static_cast<NiTCBFloatKey*>(pkKeys);

    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        SaveBinary(stream, &pkTCBFloatKeys[ui]);
    }
}

void NiTCBFloatKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
    uint32_t v9;
    NiTCBFloatKey* pkTCBKey = static_cast<NiTCBFloatKey*>(pkKey);
    NiFloatKey::SaveBinary(&stream, pkKey);
    NiBinaryStream* m_pkOstr = stream.Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v9 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pkTCBKey->m_fTension, 4, &v9, 1);
    NiBinaryStream* v5 = stream.Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v6 = v5->m_pfnWrite;
    v9 = 4;
    v5->m_uiAbsoluteCurrentPos += v6(v5, &pkTCBKey->m_fContinuity, 4, &v9, 1);
    NiBinaryStream* v7 = stream.Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    v9 = 4;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &pkTCBKey->m_fBias, 4, &v9, 1);
}

bool NiTCBFloatKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
    NiAnimationKey* pkOldKeys = pkKeys;
    uint32_t uiInsertAt;
    if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiTCBFloatKey))) {
        return false;
    }

    // EXE-INLINED none: NiNew NiTCBFloatKey[uiNewNumKeys] (the overflow-clamped size, the count cookie,
    // MemoryManager::Allocate; no constructor runs). The block is raw memory to the exe: a uint32_t count cookie,
    // then the keys; the casts below and before Deallocate step over that cookie.
    uint32_t uiNewNumKeys = uiNumKeys + 1;
    uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiTCBFloatKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiTCBFloatKey* pkNewKeys;
    if (puiCookie) {
        *puiCookie = uiNewNumKeys;
        pkNewKeys = reinterpret_cast<NiTCBFloatKey*>(puiCookie + 1);
    }
    else {
        pkNewKeys = nullptr;
    }

    NiTCBFloatKey* pkOld = static_cast<NiTCBFloatKey*>(pkOldKeys);
    rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiTCBFloatKey));
    if (uiNumKeys > uiInsertAt) {
        rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiTCBFloatKey));
    }
    float fValue = GenInterpDefault(fTime, pkOld, TCBKEY, uiNumKeys, sizeof(NiTCBFloatKey));
    NiTCBFloatKey* pkNewKey = pkNewKeys + uiInsertAt;
    pkNewKey->m_fTime = fTime;
    pkNewKey->m_fValue = fValue;
    pkNewKey->m_fTension = 0.0f;
    pkNewKey->m_fContinuity = 0.0f;
    pkNewKey->m_fBias = 0.0f;
    uiNumKeys++;

    // EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
    if (pkOldKeys) {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
    }
    pkKeys = pkNewKeys;
    FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiTCBFloatKey));
    return true;
}

NiAnimationKey* NiTCBFloatKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
    // EXE-INLINED none: NiNew NiTCBFloatKey[uiNumKeys] (the overflow-clamped size, the count cookie,
    // MemoryManager::Allocate; no constructor runs). The block is raw memory to the exe: a uint32_t count cookie,
    // then the keys; the cast below steps over that cookie.
    uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiTCBFloatKey);
    uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
    uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    NiTCBFloatKey* pkKeys;
    if (puiCookie) {
        *puiCookie = uiNumKeys;
        pkKeys = reinterpret_cast<NiTCBFloatKey*>(puiCookie + 1);
    }
    else {
        pkKeys = nullptr;
    }
    for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
        pkKeys[ui].LoadBinary(stream);
    }
    return pkKeys;
}

// Allocates as NiNew NiTCBFloatKey[uiNumKeys] does: the element count first, then the keys (no constructors). The block
// is untyped memory filled by hand, hence the reinterpret_cast to the keys past the count.
NiAnimationKey* NiTCBFloatKey::CreateArray(uint32_t uiNumKeys) {
    size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiTCBFloatKey) ? sizeof(NiTCBFloatKey) * uiNumKeys : SIZE_MAX;
    size_t* v2 = static_cast<size_t*>(MemoryManager_Singleton.Allocate(
        (static_cast<uint32_t>(static_cast<uint32_t>(v1) + static_cast<uint32_t>(sizeof(size_t))) <
         static_cast<uint32_t>(v1))
            ? -1
            : v1 + sizeof(size_t)
    ));
    if (!v2) {
        return 0;
    }
    *v2 = uiNumKeys;
    return reinterpret_cast<NiAnimationKey*>(v2 + 1);
}

// FLOATKEY/TCBKEY: clears the slot's curvature and derivative entries before it sets the guard, then stores its own
// functions' addresses.
int32_t NiTCBFloatKey::RegisterSupportedFunctions() {
    if (NiTCBFloatKey_RegisterLoader_bRegistered) {
        return 0;
    }
    const uint32_t uiIndex =
        static_cast<uint32_t>(NiAnimationKey::FLOATKEY) * NiAnimationKey::NUMKEYTYPES + NiAnimationKey::TCBKEY;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    NiTCBFloatKey_RegisterLoader_bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiTCBFloatKey::CreateFromStream;
    NiAnimationKey::ms_saves[uiIndex] = &NiTCBFloatKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiTCBFloatKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiTCBFloatKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = 28;
    NiAnimationKey::ms_interps[uiIndex] = &NiTCBFloatKey::Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiTCBFloatKey::Equal;
    NiAnimationKey::ms_fillderived[uiIndex] = &NiTCBFloatKey::FillDerivedVals;
    NiAnimationKey::ms_inserts[uiIndex] = &NiTCBFloatKey::Insert;
    return 1;
}

bool NiTCBFloatKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    if (!NiFloatKey::Equal(key0, key1)) {
        return false;
    }
    const NiTCBFloatKey& tcb0 = static_cast<const NiTCBFloatKey&>(key0);
    const NiTCBFloatKey& tcb1 = static_cast<const NiTCBFloatKey&>(key1);
    return tcb0.m_fTension == tcb1.m_fTension && tcb0.m_fContinuity == tcb1.m_fContinuity &&
        tcb0.m_fBias == tcb1.m_fBias && tcb0.m_fDS == tcb1.m_fDS && tcb0.m_fDD == tcb1.m_fDD;
}

void NiTCBFloatKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
    NiTCBFloatKey* pkNew = static_cast<NiTCBFloatKey*>(pkNewKey);
    const NiTCBFloatKey* pkOrig = static_cast<const NiTCBFloatKey*>(pkOrigKey);
    pkNew->m_fTime = pkOrig->m_fTime;
    pkNew->m_fValue = pkOrig->m_fValue;
    pkNew->m_fTension = pkOrig->m_fTension;
    pkNew->m_fContinuity = pkOrig->m_fContinuity;
    pkNew->m_fBias = pkOrig->m_fBias;
    pkNew->m_fDS = pkOrig->m_fDS;
    pkNew->m_fDD = pkOrig->m_fDD;
}
