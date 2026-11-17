#pragma once

#include "BSShader/ImageSpaceShader.h"

// The PDB has no source path; placed with the image-space shaders. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderLightAdapt : public ImageSpaceShader {
public:
	ImageSpaceShaderLightAdapt();
	~ImageSpaceShaderLightAdapt() override;

	// IDA calls the entry LoadShaders; it occupies ImageSpaceShader's parameterless ReloadShaders slot.
	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderLightAdapt_vftable{};
// The secondary vtable the destructor stores at the object's +0x6C ImageSpaceEffect subobject (IDA rdata_10B42A4).
inline constexpr fnvd::Vtable ImageSpaceShaderLightAdapt_vftable_ImageSpaceEffect{};
