#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderStencilCopy : public ImageSpaceShader {
public:
	ImageSpaceShaderStencilCopy();
	~ImageSpaceShaderStencilCopy() override;

	void ReloadShaders() override;
	void SetState() override;

	// The stencil reference value the copy compares against.
	static inline uint32_t uiStencilRef = 128;
};

inline constexpr fnvd::Vtable ImageSpaceShaderStencilCopy_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B2470; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderStencilCopy_vftable_ImageSpaceEffect{};
