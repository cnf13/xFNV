#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"

// A tension/continuity/bias float key: the key's time and value, the three TCB parameters and the two derived tangents
// (m_fDS, m_fDD) CalculateDVals fills.
class NiTCBFloatKey : public NiFloatKey {
public:
    // Reads one key: the NiFloatKey part, then tension, continuity and bias;
    // the TCB float keys' loader runs it on each 28-byte key.
    void LoadBinary(NiStream& stream);

    // Fills result's incoming and outgoing tangents from the neighbouring values fValSub1 and fValPlus1 and the
    // lengths of the intervals before (fPreLen) and after (fNextLen) it.
    static void CalculateDVals(NiTCBFloatKey& result, float fValSub1, float fValPlus1, float fPreLen, float fNextLen);
    // The value at fTime (0..1) of the interval from pKey0 to pKey1: the Hermite curve through both values with
    // pKey0's outgoing and pKey1's incoming tangent, written to pResult as a float.
    static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
    // Fills every key's tangents; the end keys get mirrored neighbours and unit interval lengths. ucSize is unused.
    // Protected in the engine, like SaveToStream and SaveBinary; public here so the replacement table can name them.
    static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
    // Writes uiNumKeys keys to kStream, one SaveBinary each.
    static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    // Writes one key: the NiFloatKey part, then tension, continuity and bias.
    static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
    // Puts a key at fTime into the array, which it reallocates one key larger; false when OkayToInsert refuses.
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // Allocates uiNumKeys keys (with the array's count cookie) and reads each with LoadBinary.
    static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
    // CreateArray allocates uiNumKeys keys after a count cookie, from MemoryManager.
    static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
    // Once (the guard byte), fills FLOATKEY/TCBKEY's slot of each
    // function table NiAnimationKey keeps and its key size; 1 the first time, else 0.
    static int32_t RegisterSupportedFunctions();
    // NiFloatKey::Equal, then tension, continuity, bias and both derived tangents.
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
    // The key's seven floats.
    static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);

    float m_fTension;
    float m_fContinuity;
    float m_fBias;
    float m_fDS;
    float m_fDD;
};

inline char NiTCBFloatKey_RegisterLoader_bRegistered;

// NiRegisterAnimationStream(NiTCBFloatKey): a static of every file that includes this header, as the game does.
static int32_t NiTCBFloatKeyregisterloaderreturn = NiTCBFloatKey::RegisterSupportedFunctions();
