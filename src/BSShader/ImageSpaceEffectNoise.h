#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

// The film grain image space effect: a noise texture built from a permutation table.
class ImageSpaceEffectNoise : public ImageSpaceEffect {
public:
	ImageSpaceEffectNoise();
	~ImageSpaceEffectNoise() override;

	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	bool IsActive() override;
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;

	bool IsLoaded();
	// Fills both halves of the permutation table p from the exe's default table, rotated by a hash of the seed.
	void InitializeUtilityBuffers(uint32_t aiSeed);
	void ReshufflePermutationTable();
	void ShufflePermuationTable();

	// Xbox GetEffectName is not a PC virtual. Shutdown is declared in the PDB but the exe's slot is the base's.
	// The one noise effect the image space manager builds.
	static ImageSpaceEffectNoise* GetInstance();

	ImageSpaceTexture pNoiseMap;
	ImageSpaceTexture pTexBuffer;
	ImageSpaceTexture pPermutedTex2D;
	ImageSpaceTexture pPermutedGradientTex;
	uint8_t p[512];
	float fTexRatio0[4];
	float fTexRatio1[4];
	bool bIsLoaded;
	uint8_t padding2B9[3];

	static inline ImageSpaceEffectNoise* pInstance;
	static inline float fNoiseScale;
	static inline NiColorA fTexScroll0{};
	static inline NiColorA fTexScroll1{};
	static inline NiColorA fTexScroll2{};
	static inline NiColorA fTexScale{};
	static inline NiColorA fAmplitude{};
};
inline constexpr fnvd::Vtable ImageSpaceEffectNoise_vftable{};

// The noise permutation table.
using NoisePermutationTable = uint8_t[256];
inline NoisePermutationTable permutation{151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36,
	103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203,
	117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48,
	27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143,
	54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
	164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207,
	206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153,
	101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
	228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214,
	31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29,
	24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180};
