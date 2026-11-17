#pragma once

#include "Fallout Shared/ImageSpaceModifierInstanceTemp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Reconstruction.hpp"

// A radial blur: ramps up, holds and ramps down around a screen position.
class ImageSpaceModifierInstanceRB : public ImageSpaceModifierInstanceTemp {
public:
	ImageSpaceModifierInstanceRB(float afStrength, float afAge, NiAVObject* apTarget, float afDuration, float afRampup,
		float afStart, float afRampdown, float afDownStart, const NiPoint2& aCenter);
	~ImageSpaceModifierInstanceRB() override;

	void Apply() override;
	void PrintInfo(char* apBuffer) override;

	// Makes the effect and starts it (cdecl). IDA names the center's type FloatPair.
	static ImageSpaceModifierInstanceRB* Trigger(float afStrength, float afRampup, float afStart, float afRampdown,
		float afDownStart, NiPoint2* aCenter, float afDuration);
	// Inferred type: afStrength: float from 0x52a4b6 (passed to ImageSpaceModifierInstanceRB::Construct 1); afRampup:
	// float from 0x52a49c (passed to ImageSpaceModifierInstanceRB::Construct 5); afStart: float from 0x52a495 (passed
	// to ImageSpaceModifierInstanceRB::Construct 6); afRampdown: float from 0x52a48e (passed to
	// ImageSpaceModifierInstanceRB::Construct 7); afDownStart: float from 0x52a487 (passed to
	// ImageSpaceModifierInstanceRB::Construct 8); afDuration: float from 0x52a4a3 (passed to
	// ImageSpaceModifierInstanceRB::Construct 4)
	static ImageSpaceModifierInstanceRB* Trigger_0052A440(float afStrength, float afRampup, float afStart,
		float afRampdown, float afDownStart, const NiPoint2& aCenter, float afDuration);

	float fRampup;
	float fStart;
	float fRampdown;
	float fDownStart;
	NiPoint2 Center;
};
inline constexpr fnvd::Vtable ImageSpaceModifierInstanceRB_vftable{};
