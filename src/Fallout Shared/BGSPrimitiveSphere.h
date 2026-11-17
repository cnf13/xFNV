#pragma once

#include "Fallout Shared/BGSPrimitive.h"
#include "Reconstruction.hpp"

// A sphere primitive; it adds no fields to BGSPrimitive. Its radius is the x component of Radii.
class BGSPrimitiveSphere : public BGSPrimitive {
public:
	BGSPrimitiveSphere();
	~BGSPrimitiveSphere() override;

	void Build() override;
	void SetColor(NiColorA& aColor) override;
	void SetRadii(NiPoint3& aRadii) override;
	bool IsInside(NiPoint3& aPoint) override;
	BSMultiBoundShape* MakeMultiBoundShape(NiPoint3& akAngle) override;

	// Sets all three radii and rebuilds the sphere's geometry.
	void SetRadius(float afRadius);
};
inline constexpr fnvd::Vtable BGSPrimitiveSphere_vftable{};
