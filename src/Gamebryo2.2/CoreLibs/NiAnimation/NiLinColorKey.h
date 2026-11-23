#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "SDK/MSVC/ehvec.h"

// A color key interpolated linearly to the next one: the time and color of NiColorKey, nothing more.
class NiLinColorKey : public NiColorKey {
public:
    NiLinColorKey();

    // The table's equality test for linear color keys: NiColorKey::Equal.
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
    // Copies the time and the four color floats.
    static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
    // NiNew NiLinColorKey[uiNumKeys]; the ms_arrays entry.
    static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
    // A new array of uiNumKeys keys, each read by LoadBinary.
    static NiAnimationKey* CreateFromStream(NiStream& kStream, uint32_t uiNumKeys);
    // Inserts a key at fTime with the interpolated color, in a new array one key longer (ms_inserts entry).
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // Once (the guard byte), fills the COLORKEY/LINKEY slot of each function table NiAnimationKey keeps and its key
    // size; returns 1 the first time, else 0.
    static int32_t RegisterLoader();
    // Writes pkKey0's color times (1 - fTime) plus pkKey1's times fTime to pResult.
    static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
    // One NiColorKey::SaveBinary per 20-byte key; the step color keys' table holds it too.
    static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
};

// NiRegisterAnimationStream(NiLinColorKey): a static of every file that includes this header.
static int32_t NiLinColorKeyregisterloaderreturn = NiLinColorKey::RegisterLoader();
