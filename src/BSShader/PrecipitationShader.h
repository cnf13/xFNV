#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;

// The rain and snow shader: the precipitation's velocity, bounds, parameters and ambient color, with its own vertex
// declaration.
class PrecipitationShader : public BSShader {
public:
	// Values from the Xbox PDB.
	enum etPasses : int32_t {
		PRECIP_PASS_RAIN = 0x0,
		PRECIP_PASS_COUNT = 0x1,
	};

	// Values from the Xbox PDB.
	enum eVertexShaders : int32_t {
		PRECIP_VSH_BILLBOARDUP = 0x0,
		PRECIP_VSH_BILLBOARDFACE = 0x1,
		PRECIP_VSH_SNOW_BILLBOARDUP = 0x2,
		PRECIP_VSH_SNOW_BILLBOARDFACE = 0x3,
		PRECIP_VSH_COUNT = 0x4,
	};

	// Values from the Xbox PDB.
	enum ePixelShaders : int32_t {
		PRECIP_PSH_RAIN = 0x0,
		PRECIP_PSH_SNOW = 0x1,
		PRECIP_PSH_COUNT = 0x2,
	};

	// The shader's passes, one per etPasses value (static in the PDB).
	using PassArray = NiPointer<NiD3DPass>[PRECIP_PASS_COUNT];
	using PixelShaderArray = NiPointer<NiD3DPixelShader>[PRECIP_PSH_COUNT];
	using VertexShaderArray = NiPointer<NiD3DVertexShader>[PRECIP_VSH_COUNT];

	PrecipitationShader();
	~PrecipitationShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;
	virtual void LoadShaders();
	virtual void LoadVertexShaders();
	virtual void LoadPixelShaders();
	virtual bool LoadStagesAndPasses();

	static BSShader* CreateShader();

	NiPoint3 kVelocity;
	NiPoint3 kMin;
	NiPoint3 kMax;
	NiPoint3 fParams;
	// The Xbox PDB types it NiPointer<NiXenonShaderDeclaration>; the PC's declaration is NiDX9's.
	NiPointer<NiDX9ShaderDeclaration> spShaderDecl;
	NiColor AmbientColor;

	static NiRTTI ms_RTTI;
	static PassArray spPass;
	static PixelShaderArray spPixelShader;
	static VertexShaderArray spVertexShader;
	// The eye position and camera up vector the "eye pos" and "cam up" vertex constants read.
	static inline NiPoint3 pEyePosition{};
	static inline NiPoint3 pCameraUp{};
	// The time the precipitation's vertex shader animates by; UpdateTime adds each frame's seconds.
	static inline float fTime = 10.0f;
};
inline constexpr fnvd::Vtable PrecipitationShader_vftable{};
