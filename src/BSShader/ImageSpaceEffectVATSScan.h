#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiSourceTexture;

// The VATS scan-line image-space effect.
class ImageSpaceEffectVATSScan : public ImageSpaceEffect {
public:
	// Values from the Xbox PDB.
	enum EffectListEnum : int32_t {
		EFFECT_VATS_SCAN = 0x0,
		EFFECT_COUNT = 0x1,
	};

	// Values from the Xbox PDB.
	enum TextureListEnum : int32_t {
		TEXTURE_DEST = 0x0,
		TEXTURE_SRC_COLORS = 0x1,
		TEXTURE_SRC_MASK = 0x2,
		TEXTURE_SCANLINES = 0x3,
		TEXTURE_SCANMASK = 0x4,
		TEXTURE_COUNT = 0x5,
	};

	ImageSpaceEffectVATSScan();
	~ImageSpaceEffectVATSScan() override;

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	void ReturnTextures() override;
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;

	// The scan-line parameters Setup/UpdateParams hand to the shader.
	static inline float fScanVerticalScale = 2.0f;
	static inline float fScanlineFrequency = 150.0f;
	static inline float fScanVerticalPosition;
	static inline NiColorA kTint{};
	// FOVATSEffectManager::PrepareScanEffect (0x801AD0) copies the manager's scan-line texture in through NiPointer's
	// assignment (0x6E5CC0) and assigns the mask's NiRenderedTexture through 0x66B0D0.
	static NiPointer<NiTexture> spScanlinesTexture;
	static NiPointer<NiTexture> spMaskTexture;
	// PDB spDistortTexture; an NiPointer: the exe registers its release (0xFDD1F0) at exit.
	static NiPointer<NiSourceTexture> PipboyDistortEffectMap;
};
inline constexpr fnvd::Vtable ImageSpaceEffectVATSScan_vftable{};
