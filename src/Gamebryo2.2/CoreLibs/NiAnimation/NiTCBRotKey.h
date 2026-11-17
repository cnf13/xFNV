#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

class NiStream;

// A tension/continuity/bias rotation key: the key's own rotation, the three TCB parameters and the two derived tangent
// quaternions (m_A, m_B) CalculateDVals fills.
class NiTCBRotKey : public NiRotKey {
public:
    void CalculateDVals(const NiTCBRotKey* pPrev, const NiTCBRotKey* pNext);
    void LoadBinary(NiStream& stream);

    static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
    // NiRotKey::Equal, then the TCB parameters, m_A and m_B.
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
    // Writes one key: the NiRotKey part, then the tension, continuity and bias.
    static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
    // Writes uiNumKeys keys, one SaveBinary each.
    static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    // Copies the time, the rotation, the TCB parameters, m_A and m_B.
    static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
    // Fills the ROTKEY/TCBKEY slot of every function table and the key-size table once; 1 on the first call, else 0.
    static int32_t RegisterLoader();
    // The table's ArrayFunction: NiNew NiTCBRotKey[uiNumKeys].
    static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
    // The table's InsertFunction: inserts a key at fTime, then recomputes the derived values of every key; returns
    // false when OkayToInsert refuses.
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // The table's CreateFunction: reads uiNumKeys keys into a new[] block, each read by LoadBinary.
    static NiAnimationKey* CreateFromStream(NiStream& kStream, uint32_t uiNumKeys);
    // The key array's fill function: NiRotKey's, then each key's CalculateDVals from its neighbours (the first and last
    // key their own neighbour).
    static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);

    float m_fTension;
    float m_fContinuity;
    float m_fBias;
    NiQuaternion m_A;
    NiQuaternion m_B;
};

// NiRegisterAnimationStream(NiTCBRotKey): a static of every file that includes this header, as the game does.
static int32_t NiTCBRotKeyregisterloaderreturn = NiTCBRotKey::RegisterLoader();
