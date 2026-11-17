#pragma once

#include "Fallout Shared/ImageSpaceModifierInstanceTemp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Reconstruction.hpp"

// A timed depth radial blur: the blur around Center (or around the target's bound) and the focus distance and range.
// The PDB module is ImageSpaceModifierInstance.cpp. IDA names fRampdown and fDownStart flt28 and flt2C, Center
// kCenter and Mode eMode. The mode is the PDB's DepthOfFieldMode, whose values this class's code does not show.
class ImageSpaceModifierInstanceDRB : public ImageSpaceModifierInstanceTemp {
public:
	// The PDB's mode names; the values follow their order.
	enum DepthOfFieldMode : int32_t {
		DOF_FRONT_BACK = 0x0,
		DOF_FRONT = 0x1,
		DOF_BACK = 0x2,
		DOF_NONE = 0x3,
	};

	ImageSpaceModifierInstanceDRB(float afArg2, float afArg3, NiAVObject* apArg4, float afArg5, DepthOfFieldMode aeArg6,
		float afArg7, float afArg8, float afArg9, float afArg10, float afArg11, float afArg12, const NiPoint2& arArg13);
	~ImageSpaceModifierInstanceDRB() override;

	void Apply() override;
	void PrintInfo(char* apBuffer) override;

	// Sets fDistance (the exe's shared setter 0x965580, also
	// FORenderedMenu::SetScreenLightBaseIntensity; thiscall, ret 4).
	void SetDistance(float afDistance);

	// Starts a depth radial blur instance around aCenter; null when none
	// can be made.
	static ImageSpaceModifierInstanceDRB* Trigger(float afDistance, float afRange, DepthOfFieldMode aMode,
		float afStrength, float afRampup, float afStart, float afRampdown, float afDownStart, const NiPoint2& aCenter,
		float afDuration);

	float fRampup;
	float fStart;
	float fRampdown;
	float fDownStart;
	NiPoint2 Center;
	float fDistance;
	float fRange;
	DepthOfFieldMode Mode;
};
inline constexpr fnvd::Vtable ImageSpaceModifierInstanceDRB_vftable{};
