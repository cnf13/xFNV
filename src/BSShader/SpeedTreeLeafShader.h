#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class SpeedTreeLeafShaderProperty;

// The shader that draws a SpeedTree model's leaves.
class SpeedTreeLeafShader : public BSShader {
public:
	// The vertex shader constants shared by every leaf draw: the light, the billboard axes and the rock and rustle
	// animation parameters (the PDB has only a pointer to them, pGroupedVertexConstants). The names are IDA's;
	// not in the PDB.
	struct GroupedConstants {
		NiVector4 LightVector;
		NiVector4 LightPos;
		NiVector4 BillboardRight;
		NiVector4 BillboardUp;
		NiVector4 RockParams;
		NiVector4 RustleParams;
	};

	SpeedTreeLeafShader();
	~SpeedTreeLeafShader() override;

	const NiRTTI* GetRTTI() const override;
	bool SetupGeometry(NiGeometry* apGeometry) override { return false; }
	uint32_t UpdatePipeline(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
		NiGeometryData::RendererData* apBuffData, const NiPropertyState* apState, const NiDynamicEffectState* apEffects,
		const NiTransform& aWorld, const NiBound& aWorldBound) override;
	uint32_t SetupTransformations(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
		const NiSkinPartition::Partition* apPartition, NiGeometryData::RendererData* apBuffData,
		const NiPropertyState* apState, const NiDynamicEffectState* apEffects, const NiTransform& aWorld,
		const NiBound& aWorldBound) override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	// Makes the pass for the leaf render pass current.
	void InitPass(uint32_t auiCurrentPass) override;
	// Loads the fog distances, power and color into the shared fog vertex constants.
	void SetupPassConstants(uint32_t auiCurrentPass) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;

protected:
	virtual void LoadShaders();

public:
	void LoadStagesAndPasses();
	void UpdatePointLights(SpeedTreeLeafShaderProperty* apSTLSProp, _D3DMATRIX& d3dInvWorld, float afWorldScale);
	// UpdatePipeline passes its own arguments on.
	void UpdatePipelineForInstance(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
		NiGeometryData::RendererData* apBuffData, const NiPropertyState* apState, const NiDynamicEffectState* apEffects,
		const NiTransform& aWorld, const NiBound& aWorldBound);

	static void UpdateScaleAdjustment();
	static BSShader* CreateShader();
	// Inferred type: rockTimeScale: float: used as one at 0xbb13aa; rustleTimeScale: float: used as one at 0xbb13ba
	static void UpdateWindTimers(float a1, float a2, float rockTimeScale, float rustleTimeScale);

	NiPoint4 pLeafMaps[48];
	NiPointer<NiD3DVertexShader> pVertexShader[4];
	NiPointer<NiD3DPixelShader> pPixelShader[2];
	// The passes of BSSM_LEAVES_AD (62) and BSSM_LEAVES_ADPt (63).
	NiPointer<NiD3DPass> sp1StagePass[2];

	// IDA names (SpeedTreeLeafShader::Const::GroupedConstants); the PDB's static is the pointer
	// pGroupedVertexConstants.
	static GroupedConstants kGroupedConstants;
	static inline float fRockTime;
	static inline float fRustleTime;
	static NiRTTI ms_RTTI;
};

// The vtable the exe's constructor stores at offset 0.
inline constexpr fnvd::Vtable SpeedTreeLeafShader_vftable{};
