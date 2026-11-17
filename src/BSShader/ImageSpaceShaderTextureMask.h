#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"

// The texture-mask shader. InitEffects allocates 0xD4 bytes; its constructor retains the window texture.
class ImageSpaceShaderTextureMask : public ImageSpaceShader {
public:
	ImageSpaceShaderTextureMask();
	~ImageSpaceShaderTextureMask() override;

	void ReloadShaders() override;

	NiPointer<NiSourceTexture> pWindowTexture;
};
// The table of its ImageSpaceEffect subobject (at 0x6C).
inline constexpr fnvd::Vtable ImageSpaceShaderTextureMask_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceShaderTextureMask_vftable{};
