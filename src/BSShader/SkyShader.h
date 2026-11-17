#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiTriShape;

// The sky's shader, with the PC pass and program counts.
class SkyShader : public BSShader {
public:
	using LUMRampTable = float[2];
	// The three blend colours and the eye position the vertex shader gets.
	using BlendColorArray = NiColorA[3];
	using CloudTexCoordYOffsetArray = float[4];

	SkyShader();
	~SkyShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	void PostGeometry(const NiPropertyState* apState) override;
	void InitPass(uint32_t auiCurrentPass) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;

	bool LoadStagesAndPasses();
	void LoadShaders();

	// Sets g_fSunBrightness_BlurShader.
	static void SetSunBrightness(float value);
	// Sets fSkyBrightness.
	static void SetSkyBrightness(float value);
	// Builds the sky quad: a black four-vertex, two-triangle shape just inside the far plane, bound radius 1000,
	// with sky shader properties, into aspTriShape.
	static void CreateSkyQuad(NiPointer<NiTriShape>& aspTriShape);
	static void SetCloudTexCoordOffset(uint16_t auID, float afOffset);
	static BSShader* CreateShader();

	NiPointer<NiD3DPass> spPass[11];
	NiD3DVertexShader* spVertexShader[11];
	NiD3DPixelShader* spPixelShader[8];
	// The cloud layer's texture coordinate offset.
	float TexCoordYOff;
	// The blend value and the luminance ramp the vertex shader gets.
	NiPoint2 Params;

	static inline BlendColorArray pBlendColor;
	static inline NiColorA kEyePosition{};
	static inline CloudTexCoordYOffsetArray pCloudTexCoordYOffset{};
	static NiRTTI ms_RTTI;
	static inline float fSkyBrightness;
	static inline float g_fSunBrightness_BlurShader;
	// The luminance ramp's two ends, taken from the image space's HDR data
	// (ImageSpaceManager::ApplyCurrentParameterData).
	static inline LUMRampTable pLUMRamp{1.0f, 1.0f};
	// Set while the camera is under water.
	static inline bool bUnderWater;
	// The camera's height while it is under water.
	static inline float fPlayerDepth;
};
