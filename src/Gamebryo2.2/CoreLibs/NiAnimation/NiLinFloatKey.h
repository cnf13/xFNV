#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"

// A float key interpolated linearly to the next one: the time and value of NiFloatKey, nothing more.
class NiLinFloatKey : public NiFloatKey {
public:
    static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
    // Inserts a key at fTime with the interpolated value, in a new array one key longer (ms_inserts entry).
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // Once (guarded by a flag), fills the FLOATKEY/LINKEY slot of each function table NiAnimationKey keeps and its key
    // size; returns 1 the first time, else 0.
    static int32_t RegisterLoader();
    // Writes one key the way NiBoolKey does: the time, then the byte after it as 0 or 1.
    static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
    // Writes uiNumKeys keys to kStream, one SaveBinary each.
    static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
    // Linear float keys have nothing to derive.
    static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
    // Reads auiCount keys into a new array after a count cookie, each by NiFloatKey::LoadBinary; the step float keys'
    // table holds it too.
    static NiAnimationKey* CreateFromStream(NiStream& arStream, uint32_t auiCount);
    // The table's ArrayFunction: allocates uiNumKeys 8-byte keys after a count cookie, from MemoryManager.
    static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
    // NiFloatKey::Equal; the step float keys' table holds it too.
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
    // The time and value; the step float keys' table holds it too.
    static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
};

// NiRegisterAnimationStream(NiLinFloatKey): a static of every file that includes this header.
static int32_t NiLinFloatKeyregisterloaderreturn = NiLinFloatKey::RegisterLoader();
