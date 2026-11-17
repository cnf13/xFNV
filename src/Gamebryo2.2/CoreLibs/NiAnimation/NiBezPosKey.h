#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"

class NiStream;

// A Bezier position key: the time and position of NiPosKey, the incoming and outgoing tangents and the cubic
// coefficients (m_A, m_B) FillDerivedVals computes. The static functions are protected in the engine; public here so
// the replacement table can name them.
class NiBezPosKey : public NiPosKey {
public:
    // Copies the time, position, both tangents and both coefficients (ms_copies entry).
    static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
    // NiPosKey::Equal, then both tangents.
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
    // Computes m_A and m_B of each key but the last from its own and the next key's position and tangents. ucSize is
    // not read.
    static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
    // Allocates uiNumKeys keys (count cookie first) and loads each: the NiPosKey part, then both tangents.
    static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
    // Writes uiNumKeys keys: the NiPosKey part, then both tangents of each.
    static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    // Puts a key at fTime into the array, which it reallocates one key larger, and fits the tangents of the new key
    // and its neighbours; false when OkayToInsert refuses.
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // Fills this key type's slot of the interpolation, equality, derived-value and insert tables.
    static void RegisterSupportedFunctions(KeyContent eContent, KeyType eType);
    // Once (the guard byte), fills the POSKEY/BEZKEY slot's stream and array functions and key size, then registers its
    // supported functions; returns 1 the first time, else 0.
    static int32_t RegisterLoader();
    // The largest |d1 x d2| / |d1|^2 over ms_iSampleSize + 1 samples of each key interval.
    static float ComputeMaxCurvature(NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    // The first derivative at fTime (0..1) of the interval starting at pKey0, written to
    // pResult as an NiPoint3.
    static void InterpolateD1(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
    // The second derivative, as InterpolateD1.
    static void InterpolateD2(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
    // CreateArray allocates uiNumKeys keys after a count cookie, from MemoryManager.
    static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
    // Evaluates pkKey0's cubic at fTime and writes the result to pResult.
    static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);

    NiPoint3 m_InTan;
    NiPoint3 m_OutTan;
    NiPoint3 m_A;
    NiPoint3 m_B;
};

// NiRegisterAnimationStream(NiBezPosKey): a static of every file that includes this header, as in the original.
static int32_t NiBezPosKeyregisterloaderreturn = NiBezPosKey::RegisterLoader();
