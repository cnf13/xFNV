#include "BSShader/SpeedTreeFrondShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeBranchShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

NiRTTI SpeedTreeFrondShader::ms_RTTI("SpeedTreeFrondShader", &NiD3DDefaultShader::ms_RTTI);

SpeedTreeFrondShader::SpeedTreeFrondShader() : sp1StagePass(nullptr) {
	pLightPosition.x = 0.0f;
	pLightPosition.y = 0.0f;
	pLightPosition.z = 0.0f;
	pLightPosition.w = 0.0f;
	pLightDirection.x = 0.0f;
	pLightDirection.y = 0.0f;
	pLightDirection.z = 0.0f;
	pLightDirection.w = 0.0f;
	iShaderType = 8;
}

SpeedTreeFrondShader::~SpeedTreeFrondShader() {
	for (NiPointer<NiD3DVertexShader>& spVertexShader : pVertexShader) {
		spVertexShader = nullptr;
	}
	for (NiPointer<NiD3DPixelShader>& spPixelShader : pPixelShader) {
		spPixelShader = nullptr;
	}
}

const NiRTTI* SpeedTreeFrondShader::GetRTTI() const {
	return &SpeedTreeFrondShader::ms_RTTI;
}

// The fronds set nothing up per geometry.
bool SpeedTreeFrondShader::SetupGeometry(NiGeometry* apGeometry) {
	return false;
}

// The exe's cleanup-only C++ EH frame is left out.
void SpeedTreeFrondShader::CreateConstantMaps() {
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
		m_spVertexConstantMap.m_pObject->AddEntry("DirectronalLightDir", 0x10000007u, 0, 8u, 1u, "", 16u, 4u,
			&pLightDirection, 0);
		m_spVertexConstantMap.m_pObject->AddEntry("PointLightPos", 0x10000007u, 0, 9u, 1u, "", 16u, 4u, &pLightPosition,
			0);
		m_spVertexConstantMap.m_pObject->AddEntry("LightRadius", 0x10000007u, 0, 10u, 1u, "", 16u, 4u,
			&BSShaderLightingProperty::pShaderLightConstants[19], 0);
		m_spVertexConstantMap.m_pObject->AddEntry("FogParam | ShadowVolumeFatness", 0x10000007u, 0, 14u, 1u, "", 16u,
			4u, &BSShaderLightingProperty::pShaderLightConstants[28], 0);
		m_spVertexConstantMap.m_pObject->AddEntry("FogColor | ShadowVolumeExtrudeDistance", 0x10000007u, 0, 15u, 1u, "",
			16u, 4u, &BSShaderLightingProperty::pShaderLightConstants[29], 0);
		if (BSShaderManager::bHDR) {
			m_spVertexConstantMap.m_pObject->AddEntry("Tree Dimmer", 0x10000004u, 0, 16u, 1u, "", 4u, 4u,
				&BSShaderManager::fTreeDimmer, 0);
		}
		sub_BB2F50(m_spVertexConstantMap.m_pObject, 17u);
	}
}

// Reloads the shader programs.
void SpeedTreeFrondShader::ReloadShaders() {
	LoadShaders();
}

void SpeedTreeFrondShader::ClearPassStages() {
	ClearPass(sp1StagePass.m_pObject);
}

void SpeedTreeFrondShader::LoadShaders() {
	BSShaderMacro kMacroArray_VS[4];
	BSShaderMacro kMacroArray_PS[2];
	char Str1[260];
	char apDest[260];
	int32_t v1 = 0;
	kMacroArray_VS[0].strPath = "speedtree\\frond.v.hlsl";
	kMacroArray_VS[0].kMacros[0].Name = 0;
	kMacroArray_VS[0].kMacros[0].Definition = 0;
	std::memset(&kMacroArray_VS[0].kMacros[1], 0, sizeof(_D3DXMACRO) * 8);
	kMacroArray_VS[1].strPath = "speedtree\\frond.v.hlsl";
	kMacroArray_VS[1].kMacros[0].Name = "FOG";
	kMacroArray_VS[1].kMacros[0].Definition = "";
	kMacroArray_VS[1].kMacros[1].Name = 0;
	kMacroArray_VS[1].kMacros[1].Definition = 0;
	std::memset(&kMacroArray_VS[1].kMacros[2], 0, sizeof(_D3DXMACRO) * 7);
	kMacroArray_VS[2].strPath = "speedtree\\frond.v.hlsl";
	kMacroArray_VS[2].kMacros[0].Name = "PT";
	kMacroArray_VS[2].kMacros[0].Definition = "";
	kMacroArray_VS[2].kMacros[1].Name = 0;
	kMacroArray_VS[2].kMacros[1].Definition = 0;
	std::memset(&kMacroArray_VS[2].kMacros[2], 0, sizeof(_D3DXMACRO) * 7);
	kMacroArray_VS[3].strPath = "speedtree\\frond.v.hlsl";
	kMacroArray_VS[3].kMacros[0].Name = "PT";
	kMacroArray_VS[3].kMacros[0].Definition = "";
	kMacroArray_VS[3].kMacros[1].Name = "FOG";
	kMacroArray_VS[3].kMacros[1].Definition = "";
	kMacroArray_VS[3].kMacros[2].Name = 0;
	kMacroArray_VS[3].kMacros[2].Definition = 0;
	std::memset(&kMacroArray_VS[3].kMacros[3], 0, sizeof(_D3DXMACRO) * 6);
	kMacroArray_PS[0].strPath = "speedtree\\frond.p.hlsl";
	kMacroArray_PS[0].kMacros[0].Name = 0;
	kMacroArray_PS[0].kMacros[0].Definition = 0;
	std::memset(&kMacroArray_PS[0].kMacros[1], 0, sizeof(_D3DXMACRO) * 8);
	kMacroArray_PS[1].strPath = "speedtree\\frond.p.hlsl";
	kMacroArray_PS[1].kMacros[0].Name = "FOG";
	kMacroArray_PS[1].kMacros[0].Definition = "";
	kMacroArray_PS[1].kMacros[1].Name = 0;
	kMacroArray_PS[1].kMacros[1].Definition = 0;
	std::memset(&kMacroArray_PS[1].kMacros[2], 0, sizeof(_D3DXMACRO) * 7);
	BSShaderMacro* v3 = kMacroArray_VS;
	BSShaderMacro* v20 = kMacroArray_VS;
	NiPointer<NiD3DVertexShader>* pVertexShader = this->pVertexShader;
	do {
		if (v3->strPath) {
			BSShader::MakePath(v3->strPath, Str1);
			BSsprintf(apDest, 260, "STFROND%03i.vso", v1);
			NiD3DVertexShader* v5 = CompileVertexShaderHLSL(Str1, v3->kMacros, ShaderTarget_vs_1_1, apDest);
			NiD3DVertexShader* m_pObject = pVertexShader->m_pObject;
			NiD3DVertexShader* v7 = v5;
			if (pVertexShader->m_pObject != v5) {
				if (m_pObject) {
					NiDecRefCount(m_pObject);
				}
				pVertexShader->m_pObject = v7;
				if (v7) {
					NiIncRefCount(v7);
				}
			}
		} else {
			*pVertexShader = nullptr;
		}
		++v1;
		v3 = v20 + 1;
		++pVertexShader;
		++v20;
	} while (v1 < 4);
	int32_t v9 = 0;
	NiPointer<NiD3DPixelShader>* pPixelShader = this->pPixelShader;
	const char* ptarget = BSShaderManager::GetPSTarget(1);
	BSShaderMacro* v11 = kMacroArray_PS;
	if (BSShaderManager::eShaderLevel < 2) {
		BSShaderMacro* v22 = kMacroArray_PS;
		do {
			if (v11->strPath) {
				BSShader::MakePath(v11->strPath, Str1);
				BSsprintf(apDest, 260, "STFROND%03i.pso", v9);
				NiD3DPixelShader* v16 = CompilePixelShaderHLSL(Str1, v11->kMacros, ptarget, apDest);
				NiD3DPixelShader* v17 = pPixelShader->m_pObject;
				NiD3DPixelShader* v18 = v16;
				if (pPixelShader->m_pObject != v16) {
					if (v17) {
						NiDecRefCount(v17);
					}
					pPixelShader->m_pObject = v18;
					if (v18) {
						NiIncRefCount(v18);
					}
				}
			} else {
				*pPixelShader = nullptr;
			}
			++v9;
			v11 = v22 + 1;
			++pPixelShader;
			++v22;
		} while (v9 < 2);
	} else {
		BSShaderMacro* v21 = kMacroArray_PS;
		do {
			if (v11->strPath) {
				BSShader::MakePath(v11->strPath, Str1);
				BSsprintf(apDest, 260, "STFROND2%03i.pso", v9);
				NiD3DPixelShader* v12 = CompilePixelShaderHLSL(Str1, v11->kMacros, ptarget, apDest);
				NiD3DPixelShader* v13 = pPixelShader->m_pObject;
				NiD3DPixelShader* v14 = v12;
				if (pPixelShader->m_pObject != v12) {
					if (v13) {
						NiDecRefCount(v13);
					}
					pPixelShader->m_pObject = v14;
					if (v14) {
						NiIncRefCount(v14);
					}
				}
			} else {
				*pPixelShader = nullptr;
			}
			++v9;
			v11 = v21 + 1;
			++pPixelShader;
			++v21;
		} while (v9 < 2);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void SpeedTreeFrondShader::LoadStagesAndPasses() {
	NiPointer<NiD3DPass>* p_sp1StagePass = &sp1StagePass;
	NiD3DPass::CreateNewPass(sp1StagePass);
	NiPointer<NiD3DTextureStage> spStage;
	NiD3DTextureStage::CreateNewStage(spStage);
	BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, 1);
	p_sp1StagePass->m_pObject->SetStage(p_sp1StagePass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
	NiD3DVertexShader* m_pObject = pVertexShader[0].m_pObject;
	p_sp1StagePass->m_pObject->m_spVertexShader = m_pObject;
	NiD3DPass* v5 = p_sp1StagePass->m_pObject;
	NiD3DPixelShader* v6 = pPixelShader[0].m_pObject;
	v5->m_spPixelShader = v6;
}

// The exe's cleanup-only C++ EH frame is left out.
BSShader* SpeedTreeFrondShader::CreateShader() {
	TLSData* v0 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v0->eMemContext;
	v0->eMemContext = MEM_CONTEXT_TREESYSTEM;
	SpeedTreeFrondShader* pshader = 0;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
	spShaderDecl->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDecl->SetEntry(0, 4u, 8u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
		NiShaderDeclaration::SPTYPE_FLOAT4, NiShaderDeclaration::SPUSAGE_BLENDINDICES, 0,
		NiD3DShaderDeclaration::SPTESS_DEFAULT);
	spShaderDecl->GetD3DDeclaration();
	SpeedTreeFrondShader* v4 =
		static_cast<SpeedTreeFrondShader*>(NiMemObject::operator new(sizeof(SpeedTreeFrondShader)));
	if (v4) {
		pshader = ::new (v4) SpeedTreeFrondShader;
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(spShaderDecl);
	v0->eMemContext = eMemContext;
	return pshader;
}
