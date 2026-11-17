#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"

class NiStream;

// A rotation key held until the next one: the time and quaternion of NiRotKey, nothing more. The class is only the
// set of static functions NiAnimationKey's tables hold for this key type.
class NiStepRotKey : public NiRotKey {
public:
    // Inserts a key at fTime with the rotation held there, in a new array one key longer, then refills the derived
    // values.
    static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
    // Once (the guard byte), fills ROTKEY/STEPKEY's slot of each function table NiAnimationKey keeps and its key size;
    // 1 the first time, else 0.
    static int32_t RegisterLoader();
};

// NiRegisterAnimationStream(NiStepRotKey): a static of every file that includes this header, as the game does.
static int32_t NiStepRotKeyregisterloaderreturn = NiStepRotKey::RegisterLoader();
