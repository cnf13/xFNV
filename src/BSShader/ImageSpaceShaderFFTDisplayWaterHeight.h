#pragma once

#include "BSShader/ImageSpaceShader.h"

// The PDB has no source path; placed with the image-space shaders. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderFFTDisplayWaterHeight : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTDisplayWaterHeight();
	~ImageSpaceShaderFFTDisplayWaterHeight() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderFFTDisplayWaterHeight_vftable{};
// The secondary vtable the destructor stores at the object's +0x6C ImageSpaceEffect subobject (IDA rdata_10B7BCC).
inline constexpr fnvd::Vtable ImageSpaceShaderFFTDisplayWaterHeight_vftable_ImageSpaceEffect{};
