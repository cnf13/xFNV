#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

#include <cstdint>

NiColorA NiColorKey::ms_kDefault(NiColorA_BLACK);

NiColorKey::NiColorKey() : m_Color(0.0f, 0.0f, 0.0f, 0.0f) {}

// Copies the color one component at a time, so a source overlapping m_Color reads already-copied components.
void NiColorKey::SetColor(const NiColorA& color) {
    m_Color.r = color.r;
    m_Color.g = color.g;
    m_Color.b = color.b;
    m_Color.a = color.a;
}

void NiColorKey::LoadBinary(NiStream& stream) {
    NiAnimationKey::LoadBinary(stream);
    m_Color.LoadBinary(stream);
}

// Returns the first key's color for one key and at -FLT_MAX, keeps the last index in a local and writes it back after
// the interpolation call, and copies the interpolated color into the result only after that call. The color is copied
// into the result as four untyped 32-bit words in the order 0, 1, 2, 3 with loads of 2 and 3 before their stores, so a
// result overlapping the key reads already-copied words, as the game does; a struct copy would move them in another
// order and a float copy could quiet a signalling NaN.
NiColorA NiColorKey::GenInterp(
    float fTime,
    NiColorKey* pkKeys,
    NiAnimationKey::KeyType eType,
    uint32_t uiNumKeys,
    uint32_t& uiLastIdx,
    int32_t ucSize
) {
    NiColorA kInterp;
    const NiColorA* pkColor;
    if (uiNumKeys == 1 || fTime == -3.4028234663852886e+38) {
        pkColor = &pkKeys->m_Color;
    }
    else {
        uint8_t ucKeySize = static_cast<uint8_t>(ucSize);
        uint32_t uiNumKeysM1 = uiNumKeys - 1;
        uint32_t uiLast = uiLastIdx;
        float fLastTime = pkKeys->GetKeyAt(uiLast, ucKeySize)->m_fTime;
        if (fTime < fLastTime) {
            uiLast = 0;
            fLastTime = pkKeys->m_fTime;
        }

        // The next time starts at 0 when no key follows.
        uint32_t uiNextIdx;
        float fNextTime = 0.0f;
        for (uiNextIdx = uiLast + 1; uiNextIdx <= uiNumKeysM1; uiNextIdx++) {
            fNextTime = pkKeys->GetKeyAt(uiNextIdx, ucKeySize)->m_fTime;
            if (fTime <= fNextTime) {
                break;
            }
            uiLast++;
            fLastTime = fNextTime;
        }

        float fNormTime = (fTime - fLastTime) / (fNextTime - fLastTime);
        ms_interps[static_cast<uint32_t>(COLORKEY) * NUMKEYTYPES + eType](
            fNormTime, pkKeys->GetKeyAt(uiLast, ucKeySize), pkKeys->GetKeyAt(uiNextIdx, ucKeySize), &kInterp
        );
        uiLastIdx = uiLast;
        pkColor = &kInterp;
    }

    NiColorA kResult;
    const uint32_t* puiFrom = reinterpret_cast<const uint32_t*>(pkColor);
    uint32_t* puiTo = reinterpret_cast<uint32_t*>(&kResult);
    puiTo[0] = puiFrom[0];
    puiTo[1] = puiFrom[1];
    uint32_t uiWord2 = puiFrom[2];
    uint32_t uiWord3 = puiFrom[3];
    puiTo[2] = uiWord2;
    puiTo[3] = uiWord3;
    return kResult;
}

// The chosen color is copied into the result as four untyped 32-bit words in field order, each load right before its
// store, so a result overlapping the key reads already-copied words, as the game does. A struct copy would reorder
// them, and a float copy would quiet a signalling NaN.
NiColorA
NiColorKey::GenInterpDefault(float fTime, NiColorKey* pkKeys, KeyType eType, uint32_t uiNumKeys, int32_t ucSize) {
    const NiColorA* pkColor;
    if (!uiNumKeys) {
        pkColor = &ms_kDefault;
    }
    else if (pkKeys->m_fTime > fTime) {
        pkColor = &pkKeys->m_Color;
    }
    else {
        NiColorKey* pkLast = pkKeys->GetKeyAt(uiNumKeys - 1, static_cast<uint8_t>(ucSize));
        if (!(pkLast->m_fTime < fTime)) {
            // uiNumKeys becomes the zeroed last index GenInterp updates.
            uint32_t uiCount = uiNumKeys;
            uiNumKeys = 0;
            return GenInterp(fTime, pkKeys, eType, uiCount, uiNumKeys, ucSize);
        }
        pkColor = &pkLast->m_Color;
    }

    NiColorA kResult;
    const uint32_t* puiFrom = reinterpret_cast<const uint32_t*>(pkColor);
    uint32_t* puiTo = reinterpret_cast<uint32_t*>(&kResult);
    puiTo[0] = puiFrom[0];
    puiTo[1] = puiFrom[1];
    puiTo[2] = puiFrom[2];
    puiTo[3] = puiFrom[3];
    return kResult;
}

bool NiColorKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
    // NiAnimationKey::Equal compares the times; only its low byte is tested.
    if (!static_cast<uint8_t>(NiAnimationKey::Equal(&key0.m_fTime, &key1.m_fTime))) {
        return false;
    }
    const NiColorKey& color0 = static_cast<const NiColorKey&>(key0);
    const NiColorKey& color1 = static_cast<const NiColorKey&>(key1);
    return color0.m_Color == color1.m_Color;
}

void NiColorKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
    NiColorKey* pkColorKey = static_cast<NiColorKey*>(pkKey);
    NiAnimationKey::SaveBinary(stream, pkKey);
    pkColorKey->m_Color.NiColorA::SaveBinary(stream);
}

// NiColorKey's NOINTERP slot has no curvature, derivative, fill or insert functions.
int32_t NiColorKey::RegisterLoader() {
    static char bRegistered;
    if (bRegistered) {
        return 0;
    }
    const uint32_t uiIndex = static_cast<uint32_t>(COLORKEY) * NUMKEYTYPES + NOINTERP;
    ms_curves[uiIndex] = nullptr;
    ms_interpd1s[uiIndex] = nullptr;
    ms_interpd2s[uiIndex] = nullptr;
    ms_fillderived[uiIndex] = nullptr;
    ms_inserts[uiIndex] = nullptr;
    bRegistered = 1;
    ms_loaders[uiIndex] = &NiColorKey::CreateFromStream;
    ms_saves[uiIndex] = &NiColorKey::SaveToStream;
    ms_copies[uiIndex] = &NiColorKey::Copy;
    ms_arrays[uiIndex] = &NiColorKey::CreateArray;
    ms_deletes[uiIndex] = &NiColorKey::DeleteArray;
    ms_keysizes[uiIndex] = sizeof(NiColorKey);
    ms_interps[uiIndex] = &NiColorKey::Interpolate;
    ms_equals[uiIndex] = &NiColorKey::Equal;
    return 1;
}

void NiColorKey::Interpolate(float, const NiAnimationKey*, const NiAnimationKey*, void* pResult) {
    *static_cast<NiColorA*>(pResult) = NiColorA_BLACK;
}
