#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiStream;

// Writes NiPosKey's default position at pResult.
void NiPosKey_Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
// NiPosKey's Equal: compares the times (NiAnimationKey::Equal), then the positions.
bool NiPosKey_Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);

class NiPosKey : public NiAnimationKey {
public:
    NiPosKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
        return static_cast<NiPosKey*>(NiAnimationKey::GetKeyAt(uiIndex, ucKeySize));
    }

    void LoadBinary(NiStream& stream);

    static uint8_t GetKeySize(KeyType eType) {
        return NiAnimationKey::GetKeySize(POSKEY, eType);
    }
    // Value at fTime over uiNumKeys keys; uiLastIdx caches the key the previous call ended on.
    static NiPoint3
    GenInterp(float fTime, NiPosKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint32_t& uiLastIdx, uint8_t ucSize);
    // The position at fTime over uiNumKeys keys without a cached index: ms_kDefault without keys, the first or last
    // key's position outside their times, else GenInterp from index 0.
    static NiPoint3 GenInterpDefault(float fTime, NiPosKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize);
    static float GetMaxCurvatureSampleDelta() {
        return ms_fSampleDelta;
    }

    static CurvatureFunction GetCurvatureFunction(KeyType eType) {
        return NiAnimationKey::GetCurvatureFunction(POSKEY, eType);
    }

    static InterpFunction GetInterpFunction(KeyType eType) {
        return NiAnimationKey::GetInterpFunction(POSKEY, eType);
    }

    static InterpFunction GetInterpD1Function(KeyType eType) {
        return NiAnimationKey::GetInterpD1Function(POSKEY, eType);
    }

    static InterpFunction GetInterpD2Function(KeyType eType) {
        return NiAnimationKey::GetInterpD2Function(POSKEY, eType);
    }

    // The tangent, normal, binormal and curvature of the path between pKey0 and pKey1 at fTime.
    static void CoordinateFrame(
        float fTime,
        NiPosKey* pKey0,
        NiPosKey* pKey1,
        NiAnimationKey::KeyType eType,
        NiPoint3& tangent,
        NiPoint3& normal,
        NiPoint3& binormal,
        float& curvature
    );
    // Writes one key: the NiAnimationKey part, then the position.
    static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
    // Fills this key type's slot of each function table NiAnimationKey keeps.
    static void RegisterSupportedFunctions(KeyContent eContent, KeyType eType);
    // Once (the guard byte), fills the POSKEY/NOINTERP slot's stream and array functions and key size, then registers
    // its supported functions; returns 1 the first time, else 0.
    static int32_t RegisterLoader();

    // The NOINTERP table functions: empty, or returning 0.
    static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
    static void SaveToStream(NiStream&, NiAnimationKey*, uint32_t) {}
    static void Copy(NiAnimationKey*, const NiAnimationKey*) {}
    static void DeleteArray(NiAnimationKey*) {}
    static NiAnimationKey* CreateFromStream(NiStream&, uint32_t) {
        return nullptr;
    }
    static NiAnimationKey* CreateArray(uint32_t) {
        return nullptr;
    }

    NiPoint3 m_Pos;

    static NiPoint3 ms_kDefault;
    // The number of steps ComputeMaxCurvature samples per key interval, and the time step between samples (1/20).
    static inline int32_t ms_iSampleSize = 20;
    static float ms_fSampleDelta;
};

// NiRegisterAnimationStream(NiPosKey): a static of every file that includes this header, as in the original.
static int32_t NiPosKeyregisterloaderreturn = NiPosKey::RegisterLoader();
