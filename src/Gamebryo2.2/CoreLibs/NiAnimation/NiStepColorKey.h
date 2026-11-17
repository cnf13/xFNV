#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "SDK/MSVC/ehvec.h"

class NiStream;

// A color key held until the next one: the time and color of NiColorKey, nothing more. The class is only the set of
// static functions NiAnimationKey's tables hold for this key type.
class NiStepColorKey : public NiColorKey {
public:
	// NiNew NiStepColorKey[uiNumKeys].
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// The step color (or rotation) value: pkKey0's four dwords at +4 while fTime < 1, else pkKey1's, into pvResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pvResult);
	// A new array of uiNumKeys step color keys, each read by NiColorKey::LoadBinary.
	static NiAnimationKey* CreateFromStream(NiStream& kStream, uint32_t uiNumKeys);
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Once (the guard byte), fills COLORKEY/STEPKEY's slot of each function table NiAnimationKey keeps and its key
	// size; 1 the first time, else 0.
	static int32_t RegisterLoader();
};

// A static of every file that includes this header, as the game does.
static int32_t NiStepColorKeyregisterloaderreturn = NiStepColorKey::RegisterLoader();
