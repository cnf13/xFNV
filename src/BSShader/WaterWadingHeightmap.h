#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// A water image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class WaterWadingHeightmap : public ImageSpaceShader {
public:
	WaterWadingHeightmap();
	~WaterWadingHeightmap() override;

	void ReloadShaders() override;
	void SetState() override;
};

inline constexpr fnvd::Vtable WaterWadingHeightmap_vftable{};
// The ImageSpaceEffect subobject's vtable, stored at +0x6C.
inline constexpr fnvd::Vtable WaterWadingHeightmap_vftable_ImageSpaceEffect{};
