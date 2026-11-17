#pragma once

#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Reconstruction.hpp"

// An image space modifier instance that expires fDuration after it started.
class ImageSpaceModifierInstanceTemp : public ImageSpaceModifierInstance {
public:
	ImageSpaceModifierInstanceTemp(float afStartTime, float afStrength, NiAVObject* apTarget, float afDuration);
	~ImageSpaceModifierInstanceTemp() override;

	bool IsExpired() override;
	void PrintInfo(char* apBuffer) override;

	float fDuration;
};
inline constexpr fnvd::Vtable ImageSpaceModifierInstanceTemp_vftable{};
