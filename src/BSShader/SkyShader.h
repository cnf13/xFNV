#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Reconstruction.hpp"

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiTriShape;

// The sky's shader, with the PC pass and program counts.
class SkyShader : public BSShader {
public:
	using LUMRampTable = float[2];
	// The three blend colours and the eye position the vertex shader gets (0x11FF910 holds the three colours); the
	// PDB's names, which IDA labels SkyShader::Const::BlendColor and Const::EyePosition.
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
	// Inferred type: auID: uint16_t: unsigned uses from 0x6348b3; afOffset: float: used as one at 0x6348c7
	static void SetCloudTexCoordOffset(uint16_t auID, float afOffset);
	static BSShader* CreateShader();

	// PC has eleven passes, eleven vertex shaders and eight pixel shaders, unlike Xbox's eight, eight and five.
	NiPointer<NiD3DPass> spPass[11];
	NiD3DVertexShader* spVertexShader[11];
	NiD3DPixelShader* spPixelShader[8];
	// PC has a float and a two-component vector here, not the Xbox pVSParam and pPSParam pointers.
	// The cloud layer's texture coordinate offset.
	float TexCoordYOff;
	// The blend value and the luminance ramp the vertex shader gets.
	NiPoint2 Params;

	static inline BlendColorArray pBlendColor;
	// The PDB's NiColorA (static member type 0xCD61); the exe stores NiColorA::BLACK into it (0xB8A1F0).
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
inline constexpr fnvd::Vtable SkyShader_vftable{};
