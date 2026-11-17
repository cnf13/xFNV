#pragma once

#include "BSShader/ImageSpaceShader.h"

// The noise normal-map post-processing shader. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderNoiseNormalmap : public ImageSpaceShader {
public:
	ImageSpaceShaderNoiseNormalmap();
	~ImageSpaceShaderNoiseNormalmap() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderNoiseNormalmap_vftable{};
// The secondary vtable the destructor stores at the object's +0x6C ImageSpaceEffect subobject (IDA rdata_10B5620).
inline constexpr fnvd::Vtable ImageSpaceShaderNoiseNormalmap_vftable_ImageSpaceEffect{};
