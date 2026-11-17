#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// A water image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class WaterSmoothHeightmap : public ImageSpaceShader {
public:
	WaterSmoothHeightmap();
	~WaterSmoothHeightmap() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable WaterSmoothHeightmap_vftable{};
// The ImageSpaceEffect subobject's vtable, stored at +0x6C.
inline constexpr fnvd::Vtable WaterSmoothHeightmap_vftable_ImageSpaceEffect{};
