#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderStencilCopyNoTextureNoDepth : public ImageSpaceShader {
public:
	ImageSpaceShaderStencilCopyNoTextureNoDepth();
	~ImageSpaceShaderStencilCopyNoTextureNoDepth() override;

	void ReloadShaders() override;
	void SetState() override;

	// The stencil reference the shader tests against (TESWaterSystem::UpdatePlaceableWater sets it per water group).
	static inline uint32_t uiStencilRef;
};

inline constexpr fnvd::Vtable ImageSpaceShaderStencilCopyNoTextureNoDepth_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B25F0; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderStencilCopyNoTextureNoDepth_vftable_ImageSpaceEffect{};
