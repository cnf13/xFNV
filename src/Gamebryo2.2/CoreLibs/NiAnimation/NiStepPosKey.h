#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"

// A position key held until the next one: the time and position of NiPosKey, nothing more. The class is only the
// set of static functions NiAnimationKey's tables hold for this key type.
class NiStepPosKey : public NiPosKey {
public:
    // Frees a key array allocated with a 4-byte header in front of it.
    static void DeleteArray(NiAnimationKey* pkKeyArray);
    // Interpolate copies pkKey0's position to pResult, or pkKey1's from fTime 1 on.
    static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
    // Writes pkKey0's position minus pkKey1's to pResult.
    static void InterpolateD1(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    static void RegisterSupportedFunctions(NiAnimationKey::KeyContent eContent, NiAnimationKey::KeyType eType);
    static int32_t RegisterLoader();
};

// NiRegisterAnimationStream(NiStepPosKey): a static of every file that includes this header, as the game does.
static int32_t NiStepPosKeyregisterloaderreturn = NiStepPosKey::RegisterLoader();
