#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// The fifteen-tap blur shader. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderBlur15 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur15();
	~ImageSpaceShaderBlur15() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderBlur15_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; IDA calls the data rdata_10B14A0.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur15_vftable_ImageSpaceEffect{};
