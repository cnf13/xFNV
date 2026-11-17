#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTCBRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

#include <cfloat>
#include <stdint.h>

void NiFloatKey::SetValue(float fValue) {
    m_fValue = fValue;
}

void NiFloatKey::LoadBinary(NiStream& stream) {
    uint32_t v6;
    NiStream* v2 = &stream;
    NiAnimationKey::LoadBinary(stream);
    NiBinaryStream* m_pkIstr = v2->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v6 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_fValue, 4, &v6, 1);
}

// Unlike the stock Gamebryo source, fTime is tested against the invalid time (-FLT_MAX), which returns the first key's
// value as for one key, as the game does. The last index lives in a local and is written back to uiLastIdx only on the
// returns after the search.
float NiFloatKey::GenInterp(
    float fTime,
    NiFloatKey* pkKeys,
    NiAnimationKey::KeyType eType,
    uint32_t uiNumKeys,
    uint32_t& uiLastIdx,
    uint8_t ucSize
) {
    if (uiNumKeys == 1 || fTime == -FLT_MAX) {
        return pkKeys->GetValue();
    }

    uint32_t uiNumKeysM1 = uiNumKeys - 1;
    uint32_t uiLast = uiLastIdx;
    float fLastTime = pkKeys->GetKeyAt(uiLast, ucSize)->GetTime();
    if (fTime < fLastTime) {
        uiLast = 0;
        fLastTime = pkKeys->GetKeyAt(0, ucSize)->GetTime();
    }

    uint32_t uiNextIdx;
    float fNextTime = 0.0f;
    for (uiNextIdx = uiLast + 1; uiNextIdx <= uiNumKeysM1; uiNextIdx++) {
        fNextTime = pkKeys->GetKeyAt(uiNextIdx, ucSize)->GetTime();
        if (fTime <= fNextTime) {
            break;
        }
        uiLast++;
        fLastTime = fNextTime;
    }

    if (uiNextIdx < uiNumKeys) {
        float fNormTime = (fTime - fLastTime) / (fNextTime - fLastTime);
        InterpFunction interp = ms_interps[static_cast<uint32_t>(FLOATKEY) * NUMKEYTYPES + eType];
        float fResult;
        interp(fNormTime, pkKeys->GetKeyAt(uiLast, ucSize), pkKeys->GetKeyAt(uiNextIdx, ucSize), &fResult);
        uiLastIdx = uiLast;
        return fResult;
    }

    uiLastIdx = uiLast;
    return pkKeys->GetKeyAt(uiLast, ucSize)->GetValue();
}

float NiFloatKey::GenInterpDefault(float fTime, NiFloatKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize) {
    if (!uiNumKeys) {
        return ms_fDefault;
    }
    if (pkKeys->m_fTime > fTime) {
        return pkKeys->m_fValue;
    }
    NiFloatKey* pkLast = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize);
    if (pkLast->m_fTime < fTime) {
        return pkLast->m_fValue;
    }
    // uiNumKeys is reused as the zeroed last index that GenInterp updates.
    uint32_t uiCount = uiNumKeys;
    uiNumKeys = 0;
    return GenInterp(fTime, pkKeys, eType, uiCount, uiNumKeys, ucSize);
}

// Clears the slot's curvature, derivative and insert entries before it sets the guard, then stores the shared stubs'
// and Equal's addresses.
int32_t NiFloatKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex =
        static_cast<uint32_t>(NiAnimationKey::FLOATKEY) * NiAnimationKey::NUMKEYTYPES + NiAnimationKey::NOINTERP;
    NiAnimationKey::ms_curves[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
    NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
    NiAnimationKey::ms_inserts[uiIndex] = nullptr;
    bRegistered = 1;
    NiAnimationKey::ms_loaders[uiIndex] = &NiFloatKey::CreateFromStream;
    NiAnimationKey::ms_saves[uiIndex] = &NiFloatKey::SaveToStream;
    NiAnimationKey::ms_copies[uiIndex] = &NiFloatKey::Copy;
    NiAnimationKey::ms_arrays[uiIndex] = &NiFloatKey::CreateArray;
    NiAnimationKey::ms_deletes[uiIndex] = &NiFloatKey::DeleteArray;
    NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiFloatKey);
    NiAnimationKey::ms_interps[uiIndex] = &NiFloatKey::Interpolate;
    NiAnimationKey::ms_equals[uiIndex] = &NiFloatKey::Equal;
    NiAnimationKey::ms_fillderived[uiIndex] = &NiFloatKey::FillDerivedVals;
    return 1;
}

// Returns the write's result.
uint32_t NiFloatKey::SaveBinary(NiStream* a1, NiAnimationKey* a2) {
    NiFloatKey* pkFloatKey = static_cast<NiFloatKey*>(a2);
    NiAnimationKey::SaveBinary(*a1, a2);
    NiBinaryStream* pkOstr = a1->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN pfnWrite = pkOstr->m_pfnWrite;
    uint32_t uiSize = 4;
    uint32_t uiWritten = pfnWrite(pkOstr, &pkFloatKey->m_fValue, 4, &uiSize, 1);
    pkOstr->m_uiAbsoluteCurrentPos += uiWritten;
    return uiWritten;
}

bool NiFloatKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    if (!static_cast<uint8_t>(::NiAnimationKey::Equal(&key0.m_fTime, &key1.m_fTime))) {
        return false;
    }
    return static_cast<const NiFloatKey&>(key0).m_fValue == static_cast<const NiFloatKey&>(key1).m_fValue;
}
