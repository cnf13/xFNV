#include "BSShader/SpeedTreeLeafShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeBranchShader.h"
#include "BSShader/SpeedTreeLeafShaderProperty.h"
#include "BSShader/SpeedTreeShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI SpeedTreeLeafShader::ms_RTTI("SpeedTreeLeafShader", &NiD3DDefaultShader::ms_RTTI);

SpeedTreeLeafShader::GroupedConstants SpeedTreeLeafShader::kGroupedConstants;

SpeedTreeLeafShader::SpeedTreeLeafShader() {
	iShaderType = 9;
	std::memset(pLeafMaps, 0, sizeof(pLeafMaps));
	GroupedConstants& rConstants = kGroupedConstants;
	rConstants.BillboardRight.x = 1.0f;
	rConstants.BillboardRight.y = 1.0f;
	rConstants.BillboardRight.z = 1.0f;
	rConstants.BillboardRight.w = 1.0f;
	rConstants.BillboardUp.x = 1.0f;
	rConstants.BillboardUp.y = 1.0f;
	rConstants.BillboardUp.z = 1.0f;
	rConstants.BillboardUp.w = 1.0f;
	rConstants.RockParams.x = 0.0f;
	rConstants.RockParams.y = 0.0f;
	rConstants.RockParams.z = 0.0f;
	rConstants.RockParams.w = 0.0f;
	rConstants.RustleParams.x = 0.0f;
	rConstants.RustleParams.y = 0.0f;
	rConstants.RustleParams.z = 0.0f;
	rConstants.RustleParams.w = 0.0f;
}

SpeedTreeLeafShader::~SpeedTreeLeafShader() {
	for (NiPointer<NiD3DVertexShader>& spVertexShader : pVertexShader) {
		spVertexShader = nullptr;
	}
	for (NiPointer<NiD3DPixelShader>& spPixelShader : pPixelShader) {
		spPixelShader = nullptr;
	}
}

const NiRTTI* SpeedTreeLeafShader::GetRTTI() const {
	return &SpeedTreeLeafShader::ms_RTTI;
}

uint32_t SpeedTreeLeafShader::UpdatePipeline(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
	NiGeometryData::RendererData* apBuffData, const NiPropertyState* apState, const NiDynamicEffectState* apEffects,
	const NiTransform& aWorld, const NiBound& aWorldBound) {
	ResetPasses();
	UpdatePipelineForInstance(apGeometry, apSkin, apBuffData, apState, apEffects, aWorld, aWorldBound);
	NiD3DShader::m_spCurrentPass = sp1StagePass[0].m_pObject;
	return 0;
}

uint32_t SpeedTreeLeafShader::SetupTransformations(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
	const NiSkinPartition::Partition* apPartition, NiGeometryData::RendererData* apBuffData,
	const NiPropertyState* apState, const NiDynamicEffectState* apEffects, const NiTransform& aWorld,
	const NiBound& aWorldBound) {
	D3DXMATRIX model;
	D3DXMATRIX d3dInvWorld;
	m_pkD3DRenderer->SetModelTransform(aWorld, 0);
	model._11 = aWorld.m_fScale * aWorld.m_Rotate.m_pEntry[0][0];
	model._12 = aWorld.m_Rotate.m_pEntry[1][0] * aWorld.m_fScale;
	model._13 = aWorld.m_Rotate.m_pEntry[2][0] * aWorld.m_fScale;
	model._21 = aWorld.m_Rotate.m_pEntry[0][1] * aWorld.m_fScale;
	model._22 = aWorld.m_Rotate.m_pEntry[1][1] * aWorld.m_fScale;
	model._23 = aWorld.m_Rotate.m_pEntry[2][1] * aWorld.m_fScale;
	model._31 = aWorld.m_Rotate.m_pEntry[0][2] * aWorld.m_fScale;
	model._32 = aWorld.m_Rotate.m_pEntry[1][2] * aWorld.m_fScale;
	model._33 = aWorld.m_Rotate.m_pEntry[2][2] * aWorld.m_fScale;
	model._41 = aWorld.m_Translate.x;
	model._42 = aWorld.m_Translate.y;
	model._43 = aWorld.m_Translate.z;
	model._14 = 0.0f;
	model._24 = 0.0f;
	model._34 = 0.0f;
	model._44 = 1.0f;
	D3DXMatrixInverse_Thunk(&d3dInvWorld, nullptr, &model);
	SpeedTreeLeafShaderProperty* m_pObject =
		static_cast<SpeedTreeLeafShaderProperty*>(apState->m_aspProps[3].m_pObject);
	int32_t v12 = 0;
	if (m_pObject) {
		v12 = static_cast<int32_t>((static_cast<uint32_t>(v12) & 0xFFFFFF00u) |
			(static_cast<uint32_t>(static_cast<uint8_t>((m_pObject->iShaderPropertyType == 6)))));
	}
	UpdatePointLights(v12 != 0 ? m_pObject : 0, d3dInvWorld, aWorld.m_fScale);
	SpeedTreeLeafShader::UpdateScaleAdjustment();
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
void SpeedTreeLeafShader::SetupGeometryTextures(const NiPropertyState* apState) {
	SpeedTreeLeafShaderProperty* pstlsprop =
		static_cast<SpeedTreeLeafShaderProperty*>(apState->m_aspProps[3].m_pObject);
	NiD3DTextureStage* m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
	if (m_pObject) {
		NiIncRefCount(m_pObject);
	}
	m_pObject->m_spTexture = pstlsprop->GetBaseTexture();
	ApplyTextureStages();
	if (m_pObject->m_uiRefCount-- == 1) {
		m_pObject->ReturnStageToPool();
	}
}

void SpeedTreeLeafShader::SetupGeometryConstants(const NiPropertyState* apState) {
	SpeedTreeLeafShader* pThis = this;
	BSShaderProperty::RenderPass* v2 = BSShaderManager::pCurrentRenderPass;
	NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
	SpeedTreeLeafShader* v13 = pThis;
	BSNiBound* m_kWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
	if (!m_kWorldBound) {
		m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
	}
	NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
	SpeedTreeLeafShaderProperty* pstlsprop =
		static_cast<SpeedTreeLeafShaderProperty*>(apState->m_aspProps[3].m_pObject);
	uint8_t i = 0;
	float fdarkness = pstlsprop->fForcedDarkness;
	const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
	if (BSShaderManager::pCurrentRenderPass->ucNumLights) {
		do {
			BSShaderLightingProperty::SetLight(i, v2->ppSceneLights[i], fdarkness);
			++i;
		} while (i < v2->ucNumLights);
		pThis = v13;
	}
	pThis->SetupTransformations(pgeometry, 0, 0, pbufferData, apState, 0, pgeometry->m_kWorld, *m_kWorldBound);
	if (BSShaderManager::bLiteBrite) {
		float v16;
		float v17;
		BSShaderLightingProperty::pShaderLightConstants[0].x = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].y = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].z = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].w = 1.0f;
		if (BSShaderManager::eRenderMode == 9) {
			BSShaderLightingProperty::pShaderLightConstants[1].x = 0.0f;
			v16 = 0.0f;
			v17 = 0.0f;
			BSShaderLightingProperty::pShaderLightConstants[1].y = 0.4f;
		} else {
			v16 = 0.0f;
			v17 = 0.0f;
			BSShaderLightingProperty::pShaderLightConstants[1].x = 0.0f;
			BSShaderLightingProperty::pShaderLightConstants[1].y = 0.0f;
		}
		BSShaderLightingProperty::pShaderLightConstants[1].z = v16;
		BSShaderLightingProperty::pShaderLightConstants[1].w = v17;
	}
	float* v6 = pstlsprop->GetLeafMaps();
	rt::memcpy(v13->pLeafMaps, v6, sizeof(v13->pLeafMaps));
	SpeedTreeLeafShader::kGroupedConstants.RockParams.y =
		pstlsprop->GetLeafData()->fRockSpeed * SpeedTreeLeafShader::fRockTime;
	SpeedTreeLeafShader::kGroupedConstants.RustleParams.y =
		pstlsprop->GetLeafData()->fRustleSpeed * SpeedTreeLeafShader::fRustleTime;
	SpeedTreeLeafShader::kGroupedConstants.RockParams.z = pstlsprop->GetLeafData()->fRustle;
	SpeedTreeLeafShader::kGroupedConstants.RustleParams.z = pstlsprop->GetLeafData()->fRock;
	SpeedTreeShader_pShaderData[0] = 0.0f;
	SpeedTreeShader_pShaderData[1] = 0.0f;
	SpeedTreeShader_pShaderData[2] = 0.0f;
	SpeedTreeShader_pShaderData[3] = 0.0f;
	SpeedTreeShader_pShaderData[1] = pstlsprop->GetLeafData()->fCurveScalar;
	NiD3DShaderConstantMap* m_pObject = v13->m_spVertexConstantMap.m_pObject;
	if (m_pObject) {
		NiD3DVertexShader* v8 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
		if (v8) {
			m_pObject->SetShaderConstants(v8, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld,
				*m_kWorldBound, 0, 1);
		}
	}
	NiD3DShaderConstantMap* v9 = v13->m_spPixelConstantMap.m_pObject;
	if (v9) {
		NiD3DPixelShader* v10 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
		if (v10) {
			v9->SetShaderConstants(v10, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld,
				*m_kWorldBound, 0, 1);
		}
	}
}

void SpeedTreeLeafShader::InitPass(uint32_t auiCurrentPass) {
	// 62 is BSSM_LEAVES_AD and 63 BSSM_LEAVES_ADPt (src has no RenderPassType enum).
	if (auiCurrentPass == 62) {
		NiD3DShader::m_spCurrentPass = sp1StagePass[0].m_pObject;
	} else if (auiCurrentPass == 63) {
		NiD3DShader::m_spCurrentPass = sp1StagePass[1].m_pObject;
	}
}

void SpeedTreeLeafShader::SetupPassConstants(uint32_t auiCurrentPass) {
	// Registers 28 and 29 of the shared constants: the fog distances and power, then the fog color. Without fog
	// the registers keep their values.
	BSFogProperty* pFog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
	if (pFog) {
		float fEndDistance = pFog->fEndDistance;
		NiColor kColor = pFog->m_kColor;
		float fRange = fEndDistance - pFog->fStartDistance;
		float fPower = pFog->fPower;
		BSShaderLightingProperty::pShaderLightConstants[28].x = fEndDistance;
		BSShaderLightingProperty::pShaderLightConstants[28].y = fRange;
		BSShaderLightingProperty::pShaderLightConstants[28].z = fPower;
		BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
		BSShaderLightingProperty::pShaderLightConstants[29].x = kColor.r;
		BSShaderLightingProperty::pShaderLightConstants[29].y = kColor.g;
		BSShaderLightingProperty::pShaderLightConstants[29].z = kColor.b;
		BSShaderLightingProperty::pShaderLightConstants[29].w = 0.0f;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void SpeedTreeLeafShader::CreateConstantMaps() {
	if (!m_spVertexConstantMap.m_pObject) {
		NiD3DShaderConstantMap* v3;
		NiD3DShaderConstantMap* v2 =
			static_cast<NiD3DShaderConstantMap*>(NiMemObject::operator new(sizeof(NiD3DShaderConstantMap)));
		if (v2) {
			v3 = ::new (v2) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
		} else {
			v3 = 0;
		}
		m_spVertexConstantMap = v3;
		m_spVertexConstantMap.m_pObject->AddEntry("WorldViewProjTranspose", 0x20000009u, 0, 0, 4u, 0, 0, 0, 0, 0);
		m_spVertexConstantMap.m_pObject->AddEntry("Ambient Color", 0x10000007u, 0, 5u, 1u, "", 16u, 4u,
			BSShaderLightingProperty::pShaderLightConstants, 0);
		m_spVertexConstantMap.m_pObject->AddEntry("Diff Color 0", 0x10000007u, 0, 6u, 1u, "", 16u, 4u,
			&BSShaderLightingProperty::pShaderLightConstants[1], 0);
		m_spVertexConstantMap.m_pObject->AddEntry("Diff Color 1", 0x10000007u, 0, 7u, 1u, "", 16u, 4u,
			&BSShaderLightingProperty::pShaderLightConstants[2], 0);
		m_spVertexConstantMap.m_pObject->AddEntry("FogParam | ShadowVolumeFatness", 0x10000007u, 0, 8u, 1u, "", 16u, 4u,
			&BSShaderLightingProperty::pShaderLightConstants[28], 0);
		m_spVertexConstantMap.m_pObject->AddEntry("FogColor | ShadowVolumeExtrudeDistance", 0x10000007u, 0, 9u, 1u, "",
			16u, 4u, &BSShaderLightingProperty::pShaderLightConstants[29], 0);
		if (BSShaderManager::bHDR) {
			m_spVertexConstantMap.m_pObject->AddEntry("Tree Dimmer", 0x10000004u, 0, 10u, 1u, "", 4u, 4u,
				&BSShaderManager::fTreeDimmer, 0);
		}
		m_spVertexConstantMap.m_pObject->AddEntry("grouped constants", 0x10000009u, 0, 11u, 6u, "", 96u, 4u,
			&SpeedTreeLeafShader::kGroupedConstants, 0);
		sub_BB2F50(m_spVertexConstantMap.m_pObject, 17u);
		m_spVertexConstantMap.m_pObject->AddEntry("leaf data", 0x10000009u, 0, 34u, 48u, "", 768u, 4u, pLeafMaps, 0);
	}
}

void SpeedTreeLeafShader::ReloadShaders() {
	LoadShaders();
	LoadStagesAndPasses();
}

// The exe calls ClearPass (slot 0x128) and then returns; it does not tail-jump.
[[clang::disable_tail_calls]] void SpeedTreeLeafShader::ClearPassStages() {
	ClearPass(sp1StagePass[0].m_pObject);
}

// The exe builds four vertex and two pixel shader descriptions on the stack. Each one's last dword
// (kMacros[8].Definition) is left unwritten, as the exe's memsets leave it; nothing reads it.
void SpeedTreeLeafShader::LoadShaders() {
	BSShaderMacro kPixelMacros[2];
	BSShaderMacro kVertexMacros[4];
	char acShaderName[260];
	char acPath[260];
	kVertexMacros[0].strPath = "speedtree\\leaf.v.hlsl";
	kVertexMacros[0].kMacros[0].Name = 0;
	kVertexMacros[0].kMacros[0].Definition = 0;
	std::memset(&kVertexMacros[0].kMacros[0].Definition, 0,
		offsetof(BSShaderMacro, kMacros[8].Definition) - offsetof(BSShaderMacro, kMacros[0].Definition));
	kVertexMacros[1].strPath = "speedtree\\leaf.v.hlsl";
	kVertexMacros[1].kMacros[0].Name = "FOG";
	kVertexMacros[1].kMacros[0].Definition = "";
	kVertexMacros[1].kMacros[1].Name = 0;
	kVertexMacros[1].kMacros[1].Definition = 0;
	std::memset(&kVertexMacros[1].kMacros[1].Definition, 0,
		offsetof(BSShaderMacro, kMacros[8].Definition) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kVertexMacros[2].strPath = "speedtree\\leaf.v.hlsl";
	kVertexMacros[2].kMacros[0].Name = "PT";
	kVertexMacros[2].kMacros[0].Definition = "";
	kVertexMacros[2].kMacros[1].Name = 0;
	kVertexMacros[2].kMacros[1].Definition = 0;
	std::memset(&kVertexMacros[2].kMacros[1].Definition, 0,
		offsetof(BSShaderMacro, kMacros[8].Definition) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kVertexMacros[3].strPath = "speedtree\\leaf.v.hlsl";
	kVertexMacros[3].kMacros[0].Name = "PT";
	kVertexMacros[3].kMacros[0].Definition = "";
	kVertexMacros[3].kMacros[1].Name = "FOG";
	kVertexMacros[3].kMacros[1].Definition = "";
	kVertexMacros[3].kMacros[2].Name = 0;
	kVertexMacros[3].kMacros[2].Definition = 0;
	std::memset(&kVertexMacros[3].kMacros[2].Definition, 0,
		offsetof(BSShaderMacro, kMacros[8].Definition) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kPixelMacros[0].strPath = "speedtree\\leaf.p.hlsl";
	kPixelMacros[0].kMacros[0].Name = 0;
	kPixelMacros[0].kMacros[0].Definition = 0;
	std::memset(&kPixelMacros[0].kMacros[0].Definition, 0,
		offsetof(BSShaderMacro, kMacros[8].Definition) - offsetof(BSShaderMacro, kMacros[0].Definition));
	kPixelMacros[1].strPath = "speedtree\\leaf.p.hlsl";
	kPixelMacros[1].kMacros[0].Name = "FOG";
	kPixelMacros[1].kMacros[0].Definition = "";
	kPixelMacros[1].kMacros[1].Name = 0;
	kPixelMacros[1].kMacros[1].Definition = 0;
	std::memset(&kPixelMacros[1].kMacros[1].Definition, 0,
		offsetof(BSShaderMacro, kMacros[8].Definition) - offsetof(BSShaderMacro, kMacros[1].Definition));
	for (int32_t i = 0; i < 4; ++i) {
		BSShaderMacro& rMacro = kVertexMacros[i];
		if (rMacro.strPath) {
			BSShader::MakePath(rMacro.strPath, acPath);
			BSsprintf(acShaderName, 260, "STLEAF%03i.vso", i);
			NiD3DVertexShader* pShader =
				CompileVertexShaderHLSL(acPath, rMacro.kMacros, ShaderTarget_vs_2_0, acShaderName);
			// The array holds NiPointers; their assignment is inlined.
			pVertexShader[i] = pShader;
		} else {
			pVertexShader[i] = nullptr;
		}
	}
	const char* ptarget = BSShaderManager::GetPSTarget(1);
	if (BSShaderManager::eShaderLevel < 2) {
		for (int32_t i = 0; i < 2; ++i) {
			BSShaderMacro& rMacro = kPixelMacros[i];
			if (rMacro.strPath) {
				BSShader::MakePath(rMacro.strPath, acPath);
				BSsprintf(acShaderName, 260, "STLEAF%03i.pso", i);
				NiD3DPixelShader* pShader = CompilePixelShaderHLSL(acPath, rMacro.kMacros, ptarget, acShaderName);
				pPixelShader[i] = pShader;
			} else {
				pPixelShader[i] = nullptr;
			}
		}
		return;
	}
	for (int32_t i = 0; i < 2; ++i) {
		BSShaderMacro& rMacro = kPixelMacros[i];
		if (!rMacro.strPath) {
			pPixelShader[i] = nullptr;
			continue;
		}
		// With b30TreeShaders set (the exe reads it once per shader), the exe compiles the shader twice, flipping
		// bTransparencyMultisampling before each (first off, then on, the TRANSPARENCYMS macro appended); otherwise
		// once, as the flag stands.
		bool bTwice = BSShaderManager::b30TreeShaders;
		int32_t iCount = bTwice ? 2 : 1;
		bool bTMS = true;
		do {
			if (bTwice) {
				bTMS = !bTMS;
				BSShaderManager::bTransparencyMultisampling = bTMS;
			} else {
				bTMS = BSShaderManager::bTransparencyMultisampling;
			}
			if (bTMS) {
				D3DXMACRO* pMacro = rMacro.kMacros;
				while (pMacro->Name) {
					++pMacro;
				}
				pMacro->Name = "TRANSPARENCYMS";
				pMacro->Definition = "";
				BSsprintf(acShaderName, 260, "STLEAF2%03iTMS.pso", i);
			} else {
				BSsprintf(acShaderName, 260, "STLEAF2%03i.pso", i);
			}
			BSShader::MakePath(rMacro.strPath, acPath);
			NiD3DPixelShader* pShader = CompilePixelShaderHLSL(acPath, rMacro.kMacros, ptarget, acShaderName);
			pPixelShader[i] = pShader;
			if (bTwice) {
				bTMS = BSShaderManager::bTransparencyMultisampling;
			}
		} while (--iCount);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void SpeedTreeLeafShader::LoadStagesAndPasses() {
	NiPointer<NiD3DPass>* sp1StagePass = this->sp1StagePass;
	NiD3DPass::CreateNewPass(*this->sp1StagePass);
	NiPointer<NiD3DTextureStage> spStage;
	NiD3DTextureStage::CreateNewStage(spStage);
	BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, 1);
	sp1StagePass->m_pObject->SetStage(sp1StagePass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
	NiD3DPass* m_pObject = sp1StagePass->m_pObject;
	NiD3DRenderStateGroup* FreeRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	if (m_pObject->m_pkRenderStateGroup != FreeRenderStateGroup) {
		NiD3DRenderStateGroup::ReleaseRenderStateGroup(m_pObject->m_pkRenderStateGroup);
	}
	m_pObject->m_pkRenderStateGroup = FreeRenderStateGroup;
	NiD3DVertexShader* v5 = pVertexShader[1].m_pObject;
	NiD3DVertexShader* v6 = sp1StagePass->m_pObject->m_spVertexShader.m_pObject;
	NiD3DPass* v20 = sp1StagePass->m_pObject;
	if (v6 != v5) {
		if (v6) {
			NiDecRefCount(v6);
		}
		v20->m_spVertexShader.m_pObject = v5;
		if (v5) {
			NiIncRefCount(v5);
		}
	}
	NiD3DPass* v7 = sp1StagePass->m_pObject;
	NiD3DPixelShader* v8 = pPixelShader[1].m_pObject;
	v7->m_spPixelShader = v8;
	NiPointer<NiD3DPass>* v10 = &this->sp1StagePass[1];
	NiD3DPass::CreateNewPass(this->sp1StagePass[1]);
	NiD3DTextureStage::CreateNewStage(spStage);
	BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, 1);
	v10->m_pObject->SetStage(v10->m_pObject->m_uiCurrentStage, spStage.m_pObject);
	NiD3DPass* v11 = this->sp1StagePass[1].m_pObject;
	NiD3DRenderStateGroup* v12 = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	if (v11->m_pkRenderStateGroup != v12) {
		NiD3DRenderStateGroup::ReleaseRenderStateGroup(v11->m_pkRenderStateGroup);
	}
	v11->m_pkRenderStateGroup = v12;
	NiD3DVertexShader* v13 = pVertexShader[3].m_pObject;
	NiD3DVertexShader* v14 = v10->m_pObject->m_spVertexShader.m_pObject;
	v20 = v10->m_pObject;
	if (v14 != v13) {
		if (v14) {
			NiDecRefCount(v14);
		}
		v20->m_spVertexShader.m_pObject = v13;
		if (v13) {
			NiIncRefCount(v13);
		}
	}
	NiD3DPass* v15 = v10->m_pObject;
	NiD3DPixelShader* v16 = pPixelShader[1].m_pObject;
	v15->m_spPixelShader = v16;
}

void SpeedTreeLeafShader::UpdatePointLights(SpeedTreeLeafShaderProperty* apSTLSProp, _D3DMATRIX& d3dInvWorld,
	float afWorldScale) {
	_D3DVECTOR v4;
	_D3DVECTOR pV;
	_D3DVECTOR pOut;
	_D3DVECTOR v8;
	v4.x = -BSShaderLightingProperty::pShaderLightConstants[19].X();
	v4.y = -BSShaderLightingProperty::pShaderLightConstants[19].Y();
	v4.z = -BSShaderLightingProperty::pShaderLightConstants[19].Z();
	D3DXVec3TransformNormal_Thunk(&v8, &v4, &d3dInvWorld);
	D3DXVec3Normalize_Thunk(&v4, &v8);
	pOut.x = v4.x;
	SpeedTreeLeafShader::kGroupedConstants.LightVector.x = v4.x;
	pOut.y = v4.y;
	SpeedTreeLeafShader::kGroupedConstants.LightVector.y = v4.y;
	pOut.z = v4.z;
	SpeedTreeLeafShader::kGroupedConstants.LightVector.z = v4.z;
	float v7 = 0.0;
	SpeedTreeLeafShader::kGroupedConstants.LightVector.w = 0.0;
	if (apSTLSProp->GetNumberofActiveLights()) {
		pV.x = BSShaderLightingProperty::pShaderLightConstants[12].X();
		pV.y = BSShaderLightingProperty::pShaderLightConstants[12].Y();
		pV.z = BSShaderLightingProperty::pShaderLightConstants[12].Z();
		D3DXVec3TransformCoord_Thunk(&pOut, &pV, static_cast<_D3DMATRIX*>(&d3dInvWorld));
		SpeedTreeLeafShader::kGroupedConstants.LightPos.x = pOut.x;
		SpeedTreeLeafShader::kGroupedConstants.LightPos.y = pOut.y;
		SpeedTreeLeafShader::kGroupedConstants.LightPos.z = pOut.z;
		SpeedTreeLeafShader::kGroupedConstants.LightPos.w =
			BSShaderLightingProperty::pShaderLightConstants[20].X() / afWorldScale;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void SpeedTreeLeafShader::UpdatePipelineForInstance(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
	NiGeometryData::RendererData* apBuffData, const NiPropertyState* apState, const NiDynamicEffectState* apEffects,
	const NiTransform& aWorld, const NiBound& aWorldBound) {
	int32_t v16;
	NiD3DPass* v20;
	NiD3DVertexShader* v21;
	NiD3DPass* v23;
	NiD3DPixelShader* v24;
	m_pkD3DRenderState->ApplyAlpha(static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject));
	char v10 = 0;
	BSFogProperty* pfog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
	double v12 = 0.0;
	if (pfog) {
		double v14;
		// The exe copies both distances before it measures the geometry's distance from the eye.
		float fStartDistance = pfog->fStartDistance;
		float fEndDistance = pfog->fEndDistance;
		float a5c = apGeometry->m_kWorld.m_Translate.x - BSShaderLightingProperty::pShaderLightConstants[30].X();
		float a2a = apGeometry->m_kWorld.m_Translate.y - BSShaderLightingProperty::pShaderLightConstants[30].Y();
		float v37 = apGeometry->m_kWorld.m_Translate.z - BSShaderLightingProperty::pShaderLightConstants[30].Z();
		float a5d = a5c * a5c + a2a * a2a + v37 * v37;
		float a5a = std::sqrt((double)a5d);
		if (fStartDistance < static_cast<double>(a5a)) {
			v14 = fStartDistance;
		} else {
			v14 = fStartDistance;
			if (!(fStartDistance < a5a + aWorldBound.m_fRadius)) {
				v12 = 0.0;
				goto LABEL_5;
			}
		}
		NiColor m_kColor = pfog->m_kColor;
		float a5e = fEndDistance - v14;
		v10 = 1;
		BSShaderLightingProperty::pShaderLightConstants[28].x = fEndDistance;
		v12 = 0.0;
		BSShaderLightingProperty::pShaderLightConstants[28].y = a5e;
		BSShaderLightingProperty::pShaderLightConstants[28].z = 0.0f;
		BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
		BSShaderLightingProperty::pShaderLightConstants[29].x = m_kColor.r;
		BSShaderLightingProperty::pShaderLightConstants[29].y = m_kColor.g;
		BSShaderLightingProperty::pShaderLightConstants[29].z = m_kColor.b;
		BSShaderLightingProperty::pShaderLightConstants[29].w = 0.0f;
	}
LABEL_5:;
	if (BSShaderManager::bLiteBrite) {
		float v40;
		float v41;
		BSShaderLightingProperty::pShaderLightConstants[0].x = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].y = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].z = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].w = 1.0f;
		if (BSShaderManager::eRenderMode == 9) {
			BSShaderLightingProperty::pShaderLightConstants[1].x = v12;
			v40 = v12;
			v41 = v12;
			BSShaderLightingProperty::pShaderLightConstants[1].y = 0.4f;
		} else {
			float v39 = v12;
			v40 = v12;
			v41 = v12;
			BSShaderLightingProperty::pShaderLightConstants[1].x = v12;
			BSShaderLightingProperty::pShaderLightConstants[1].y = v39;
		}
		BSShaderLightingProperty::pShaderLightConstants[1].z = v40;
		BSShaderLightingProperty::pShaderLightConstants[1].w = v41;
	}
	NiProperty* m_pObject = apState->m_aspProps[3].m_pObject;
	if (m_pObject) {
		v16 = static_cast<NiShadeProperty*>(m_pObject)->iShaderPropertyType ==
			BSShaderManager::BSSM_PROPERTYTYPE_SPEEDTREELEAF;
	} else {
		v16 = 0;
	}
	SpeedTreeLeafShaderProperty* pstlsprop = v16 ? static_cast<SpeedTreeLeafShaderProperty*>(m_pObject) : 0;
	float* v18 = pstlsprop->GetLeafMaps();
	rt::memcpy(pLeafMaps, v18, sizeof(pLeafMaps));
	SpeedTreeLeafShader::kGroupedConstants.RockParams.y =
		pstlsprop->GetLeafData()->fRockSpeed * SpeedTreeLeafShader::fRockTime;
	SpeedTreeLeafShader::kGroupedConstants.RustleParams.y =
		pstlsprop->GetLeafData()->fRustleSpeed * SpeedTreeLeafShader::fRustleTime;
	SpeedTreeLeafShader::kGroupedConstants.RockParams.z = pstlsprop->GetLeafData()->fRustle;
	SpeedTreeLeafShader::kGroupedConstants.RustleParams.z = pstlsprop->GetLeafData()->fRock;
	SpeedTreeShader_pShaderData[0] = 0.0f;
	SpeedTreeShader_pShaderData[1] = 0.0f;
	SpeedTreeShader_pShaderData[2] = 0.0f;
	SpeedTreeShader_pShaderData[3] = 0.0f;
	SpeedTreeShader_pShaderData[1] = pstlsprop->GetLeafData()->fCurveScalar;
	NiD3DTextureStage* v19 = sp1StagePass[0].m_pObject->m_kStages.m_pBase->m_pObject;
	NiD3DTextureStage* a5b = v19;
	if (v19) {
		NiIncRefCount(v19);
	}
	v19->m_spTexture = pstlsprop->GetBaseTexture();
	if (!BSShaderManager::bLiteBrite) {
		NiD3DVertexShader* v33;
		uint16_t NumberofActiveLights = pstlsprop->GetNumberofActiveLights();
		NiD3DPass* v32 = sp1StagePass[0].m_pObject;
		if (NumberofActiveLights) {
			if (!v10) {
				v32->SetVertexShader(pVertexShader[2].m_pObject);
				sp1StagePass[0].m_pObject->SetPixelShader(pPixelShader[0].m_pObject);
				goto LABEL_31;
			}
			v33 = pVertexShader[3].m_pObject;
		} else {
			if (!v10) {
				v32->SetVertexShader(pVertexShader[0].m_pObject);
				sp1StagePass[0].m_pObject->SetPixelShader(pPixelShader[0].m_pObject);
				goto LABEL_31;
			}
			v33 = pVertexShader[1].m_pObject;
		}
		v32->SetVertexShader(v33);
		sp1StagePass[0].m_pObject->SetPixelShader(pPixelShader[1].m_pObject);
		goto LABEL_31;
	}
	v20 = sp1StagePass[0].m_pObject;
	v21 = pVertexShader[0].m_pObject;
	v20->m_spVertexShader = v21;
	v23 = sp1StagePass[0].m_pObject;
	v24 = pPixelShader[0].m_pObject;
	v23->m_spPixelShader = v24;
	v19 = a5b;
LABEL_31:;
	NiD3DPass* v26 = sp1StagePass[0].m_pObject;
	if (BSShaderManager::eRenderMode == 6) {
		if (!v26->m_pkRenderStateGroup) {
			v26->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v26->m_pkRenderStateGroup->SetRenderState(52u, 1u, 1);
		NiD3DPass* v27 = sp1StagePass[0].m_pObject;
		if (!v27->m_pkRenderStateGroup) {
			v27->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v27->m_pkRenderStateGroup->SetRenderState(56u, 8u, 1);
		NiD3DPass* v28 = sp1StagePass[0].m_pObject;
		if (!v28->m_pkRenderStateGroup) {
			v28->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v28->m_pkRenderStateGroup->SetRenderState(55u, 7u, 1);
		NiD3DPass* v29 = sp1StagePass[0].m_pObject;
		if (!v29->m_pkRenderStateGroup) {
			v29->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v29->m_pkRenderStateGroup->SetRenderState(53u, 1u, 1);
		NiD3DPass* v30 = sp1StagePass[0].m_pObject;
		if (!v30->m_pkRenderStateGroup) {
			v30->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v30->m_pkRenderStateGroup->SetRenderState(54u, 1u, 1);
	} else {
		if (!v26->m_pkRenderStateGroup) {
			v26->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v26->m_pkRenderStateGroup->SetRenderState(52u, 0, 1);
	}
	NiD3DPass* v34 = sp1StagePass[0].m_pObject;
	if ((BSShaderManager::usRenderFlags & 1) != 0) {
		if (!v34->m_pkRenderStateGroup) {
			v34->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v34->m_pkRenderStateGroup->SetRenderState(168u, 0, 1);
	} else {
		if (!v34->m_pkRenderStateGroup) {
			v34->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v34->m_pkRenderStateGroup->SetRenderState(168u, 7u, 1);
	}
	NiD3DPass* v35 = sp1StagePass[0].m_pObject;
	if (!v35->m_pkRenderStateGroup) {
		v35->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	}
	v35->m_pkRenderStateGroup->SetRenderState(28u, 0, 1);
	if (v19->m_uiRefCount-- == 1) {
		v19->ReturnStageToPool();
	}
}

void SpeedTreeLeafShader::UpdateScaleAdjustment() {
	float v0 = BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
	float v1 = BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
	float v2 = BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
	float v3 = BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
	float v4 = BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
	SpeedTreeLeafShader::kGroupedConstants.BillboardUp.x = BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
	SpeedTreeLeafShader::kGroupedConstants.BillboardUp.y = v0;
	SpeedTreeLeafShader::kGroupedConstants.BillboardUp.z = v1;
	SpeedTreeLeafShader::kGroupedConstants.BillboardUp.w = 0.0;
	SpeedTreeLeafShader::kGroupedConstants.BillboardRight.x = v2;
	SpeedTreeLeafShader::kGroupedConstants.BillboardRight.y = v3;
	SpeedTreeLeafShader::kGroupedConstants.BillboardRight.z = v4;
	SpeedTreeLeafShader::kGroupedConstants.BillboardRight.w = 0.0;
}

// The exe's cleanup-only C++ EH frame is left out.
BSShader* SpeedTreeLeafShader::CreateShader() {
	SpeedTreeLeafShader* pshader = 0;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDecl->SetEntry(0, 3u, 8u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
		NiShaderDeclaration::SPTYPE_FLOAT4, NiShaderDeclaration::SPUSAGE_BLENDINDICES, 0,
		NiD3DShaderDeclaration::SPTESS_DEFAULT);
	spShaderDecl->GetD3DDeclaration();
	SpeedTreeLeafShader* v2 = static_cast<SpeedTreeLeafShader*>(NiMemObject::operator new(sizeof(SpeedTreeLeafShader)));
	if (v2) {
		pshader = ::new (v2) SpeedTreeLeafShader;
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(spShaderDecl);
	return pshader;
}

void SpeedTreeLeafShader::UpdateWindTimers(float a1, float a2, float rockTimeScale, float rustleTimeScale) {
	SpeedTreeLeafShader::kGroupedConstants.RockParams.x = a1;
	SpeedTreeLeafShader::kGroupedConstants.RustleParams.x = a2;
	SpeedTreeLeafShader::fRockTime = (float)(SpeedTreeLeafShader::fRockTime + rockTimeScale);
	SpeedTreeLeafShader::fRustleTime = (float)(SpeedTreeLeafShader::fRustleTime + rustleTimeScale);
}
