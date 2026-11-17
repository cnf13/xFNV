#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// A water image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class WaterDisplacementNormals : public ImageSpaceShader {
public:
	WaterDisplacementNormals();
	~WaterDisplacementNormals() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable WaterDisplacementNormals_vftable{};
// The ImageSpaceEffect subobject's vtable, stored at +0x6C.
inline constexpr fnvd::Vtable WaterDisplacementNormals_vftable_ImageSpaceEffect{};
