#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "BSShader/ImageSpaceShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

// The image-space shader that unpacks the menu's packed regions. The PDB declares no instance members (only the
// static PackedRegions, bChangedFlag, fOffsetX and fOffsetY), so it adds none to ImageSpaceShader.
class ImageSpaceShaderMenuUnpack : public ImageSpaceShader {
public:
	using kPackedRegions_t = NiTArray<void*, NiTMallocInterface<void*>>;

	ImageSpaceShaderMenuUnpack();

	// Loads the unpack programs: imagespace\1x\v\base.v.hlsl and imagespace\1x\p\copy.p.hlsl as COPYUNPACK, then
	// the stages and passes (0xB91AB0; vtable slot 72, ret 0).
	void ReloadShaders() override;
	// The ImageSpaceEffect subobject's Render (0xBB7530; vtable 0x10B511C slot 1, ret 0x10): draws the menu unpack
	// geometry through the effect's programs and output texture. IDA names it ImageSpaceEffectMenuUnpack::RenderShader.
	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;

	// Sets the unpack offset and marks it changed.
	static void SetOffset(float afX, float afY);

	static inline bool bChangedFlag = true;
	static inline float fOffsetX;
	static inline float fOffsetY;
	static kPackedRegions_t kPackedRegions;
};
inline constexpr fnvd::Vtable ImageSpaceShaderMenuUnpack_vftable{};
// The class's secondary ImageSpaceEffect vtable, stored at offset 0x6C by the constructor. IDA names it
// ImageSpaceEffectMenuUnpack's vtable.
inline constexpr fnvd::Vtable ImageSpaceShaderMenuUnpack_vftable_ImageSpaceEffect{};
