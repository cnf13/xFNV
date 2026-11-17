#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"

class NiFloatKey : public NiAnimationKey {
public:
    NiFloatKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
        return static_cast<NiFloatKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucKeySize));
    }

    void SetValue(float fValue);
    float GetValue() const {
        return m_fValue;
    }
    // The base's LoadBinary, then m_fValue's 4 bytes.
    void LoadBinary(NiStream& stream);

    static uint8_t GetKeySize(KeyType eType) {
        return NiAnimationKey::GetKeySize(FLOATKEY, eType);
    }
    static InterpFunction GetInterpFunction(KeyType eType);
    // Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on.
    static float
    GenInterp(float fTime, NiFloatKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx, uint8_t ucSize);
    // The value at fTime over uiNumKeys keys without a cached index: ms_fDefault without keys, the first or last
    // key's value outside their times, else GenInterp from index 0. Protected in the engine; public here so the
    // replacement table can name it.
    static float GenInterpDefault(float fTime, NiFloatKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize);
    // The NOINTERP table functions, which assert(false) in Gamebryo: empty, or returning 0.
    static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) {
        return nullptr;
    }
    static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
    static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
    static NiAnimationKey* CreateArray(uint32_t) {
        return nullptr;
    }
    static void DeleteArray(NiAnimationKey*) {}
    static void Interpolate(float, const NiAnimationKey*, const NiAnimationKey*, void*) {}
    static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
    // Fills NiAnimationKey's tables for the FLOATKEY/NOINTERP slot once; returns 1 the first time, else 0.
    static int32_t RegisterLoader();
    // a2 is the key: writes the NiAnimationKey part of a2 with NiAnimationKey::SaveBinary, then m_fValue.
    static uint32_t SaveBinary(NiStream* a1, NiAnimationKey* a2);
    // The times (NiAnimationKey::Equal), then the values. The false after the times is NiAnimationKey::Equal's
    // int32_t; the callers test its low byte.
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);

    float m_fValue;

    static inline float ms_fDefault;
};

// NiRegisterAnimationStream(NiFloatKey): a static of every file that includes this header, as the game does.
static int32_t NiFloatKeyregisterloaderreturn = NiFloatKey::RegisterLoader();
