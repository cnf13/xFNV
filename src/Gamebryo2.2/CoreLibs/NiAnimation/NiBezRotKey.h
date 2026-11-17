#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiStream;

// A rotation key with a second quaternion, m_IntQuat, after the key's own rotation.
class NiBezRotKey : public NiRotKey {
public:
    // Zeroes m_quat through NiQuaternion::SetValues; new[]'s vector constructor iterator runs it.
    NiBezRotKey();

    static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
    // Puts a key at fTime into the array, which it reallocates one key larger; false when OkayToInsert refuses.
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // Fills the derived values (m_IntQuat) of uiNumKeys keys of ucSize bytes each.
    static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
    static int32_t RegisterLoader();
    static void Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
    static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
    static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
    // Allocates uiNumKeys keys (count cookie first) and loads each through NiRotKey::LoadBinary.
    static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
    static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);

    NiQuaternion m_IntQuat;
};

// NiRegisterAnimationStream(NiBezRotKey): deliberately a static of every file that includes this header, as the game
// does.
static int32_t NiBezRotKeyregisterloaderreturn = NiBezRotKey::RegisterLoader();
