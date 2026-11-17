#include "BSShader/SpeedTreeBranchShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SpeedTreeBranchShaderProperty.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSShader/SpeedTreeShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI SpeedTreeBranchShader::ms_RTTI("SpeedTreeBranchShader", &ShadowLightShader::ms_RTTI);

SpeedTreeBranchShader::m_kPasses_t SpeedTreeBranchShader::m_kPasses;

void sub_BB2F50(NiD3DShaderConstantMap* apConstantMap, uint32_t auiCount) {
	apConstantMap->AddEntry("TreeData", 0x10000007u, 0, auiCount, 1u, "", 16u, 4u, SpeedTreeShader_pShaderData, 0);
	apConstantMap->AddEntry("WindMatrices", 0x10000009u, 0, auiCount + 1, 16u, "", 256u, 4u,
		SpeedTreeShader_pWindMatrices, 0);
}

SpeedTreeBranchShader::SpeedTreeBranchShader(NiDX9ShaderDeclaration* apShaderDecl)
	: ShadowLightShader(apShaderDecl, nullptr, nullptr, nullptr) {
	iShaderType = 5;
}

SpeedTreeBranchShader::~SpeedTreeBranchShader() {
	for (NiPointer<NiD3DVertexShader>& spVertexShader : pVertexShader) {
		spVertexShader = nullptr;
	}
	for (NiPointer<NiD3DPixelShader>& spPixelShader : pPixelShader) {
		spPixelShader = nullptr;
	}
	for (NiPointer<NiD3DPass>& spPass : m_kPasses) {
		spPass = nullptr;
	}
}

const NiRTTI* SpeedTreeBranchShader::GetRTTI() const {
	return &SpeedTreeBranchShader::ms_RTTI;
}

bool SpeedTreeBranchShader::Initialize() {
	BSShader::Initialize();
	NiPointer<NiD3DVertexShader>* pVertexShader = this->pVertexShader;
	int32_t v3 = 30;
	do {
		*pVertexShader = nullptr;
		++pVertexShader;
		--v3;
	} while (v3);
	NiPointer<NiD3DPixelShader>* pPixelShader = this->pPixelShader;
	int32_t v6 = 10;
	do {
		*pPixelShader = nullptr;
		++pPixelShader;
		--v6;
	} while (v6);
	for (NiPointer<NiD3DPass>& spPass : SpeedTreeBranchShader::m_kPasses) {
		NiD3DPass* pPass = spPass.m_pObject;
		if (pPass) {
			if (pPass->m_uiRefCount-- == 1) {
				pPass->ReturnPassToPool();
			}
			spPass.m_pObject = 0;
		}
	}
	return 1;
}

// Calls through the property without a null test: a geometry whose shade property is not a branch property faults
// there, as the game does.
bool SpeedTreeBranchShader::SetupGeometry(NiGeometry* apGeometry) {
	NiShadeProperty* pShadeProp = static_cast<NiShadeProperty*>(apGeometry->GetProperty(NiProperty::SHADE));
	SpeedTreeBranchShaderProperty* pProperty =
		(pShadeProp && pShadeProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_SPEEDTREEBRANCH)
		? static_cast<SpeedTreeBranchShaderProperty*>(pShadeProp)
		: nullptr;
	pProperty->iShader = 4;
	static_cast<BSShaderProperty*>(pProperty)->SetupGeometry(apGeometry);
	return true;
}

void SpeedTreeBranchShader::SetupGeometryTextures(const NiPropertyState* apState) {
	NiD3DTextureStage* v8;
	NiProperty* m_pObject = apState->m_aspProps[3].m_pObject;
	BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(m_pObject);
	NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
	BSShaderManager::eBSSMPassEnum v4 = BSShaderManager::eCurrentPass;
	if (BSShaderManager::eCurrentPass <= BSShaderManager::BSSM_2x_SIMPLESHADOW_Sb) {
		if (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_2x_SIMPLESHADOW_Sb) {
		LABEL_6:;
			NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->m_spTexture =
				pProperty->spTextures[0][0].m_pObject;
		} else {
			switch (BSShaderManager::eCurrentPass) {
			case BSShaderManager::BSSM_AMBDIFFTEX_Sb:
			case BSShaderManager::BSSM_AMBDIFFTEX_SbF:
			case BSShaderManager::BSSM_ADTS_Sb:
			case BSShaderManager::BSSM_ADTS_SbShp:
			case BSShaderManager::BSSM_ADTS2_Sb:
			case BSShaderManager::BSSM_ADTS2_SbShp:
			case BSShaderManager::BSSM_AD2_Sb:
			case BSShaderManager::BSSM_AD2_SbShp:
			case BSShaderManager::BSSM_AD3_Sb:
			case BSShaderManager::BSSM_AD3_SbShp:
			case BSShaderManager::BSSM_ADT_Sb:
			case BSShaderManager::BSSM_ADT_SbShp:
			case BSShaderManager::BSSM_ADT2_Sb:
			case BSShaderManager::BSSM_ADT2_SbShp: {
				NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->m_spTexture =
					pProperty->spTextures[0][0].m_pObject;
				NiD3DTextureStage* v7 = v3->m_kStages.m_pBase[1].m_pObject;
				v7->m_spTexture = BSShaderPPLightingProperty::GetNormalMap(pProperty, 0);
				break;
			}
			case BSShaderManager::BSSM_AMBDIFFDIRANDPT_Sb:
			case BSShaderManager::BSSM_DIFFUSEDIR_Sb:
			case BSShaderManager::BSSM_DIFFUSEPT_Sb:
			case BSShaderManager::BSSM_DIFFUSEPT2_Sb:
			case BSShaderManager::BSSM_DIFFUSEPT3_Sb:
			case BSShaderManager::BSSM_UNK_344:
			case BSShaderManager::BSSM_UNK_345:
			case BSShaderManager::BSSM_2x_SPECULARDIR_Sb:
			case BSShaderManager::BSSM_2x_SPECULARDIR_SbShp:
			case BSShaderManager::BSSM_2x_SPECULARPT_Sb:
			case BSShaderManager::BSSM_2x_SPECULARPT2_Sb:
			case BSShaderManager::BSSM_2x_SPECULARPT3_Sb: {
				NiD3DTextureStage* v6 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
				v6->m_spTexture = BSShaderPPLightingProperty::GetNormalMap(pProperty, 0);
				break;
			}
			case BSShaderManager::BSSM_TEXTURE_Sb: {
				goto LABEL_6;
			}
			default: {
				break;
			}
			}
		}
	}
	switch (v4) {
	case BSShaderManager::BSSM_ADTS_SbShp:
	case BSShaderManager::BSSM_ADTS2_SbShp:
	case BSShaderManager::BSSM_ADT_SbShp:
	case BSShaderManager::BSSM_ADT2_SbShp: {
		v3->m_kStages.m_pBase[6].m_pObject->m_spTexture = BSShaderManager::pProjectedShadowTexture.m_pObject;
		v8 = v3->m_kStages.m_pBase[7].m_pObject;
		goto LABEL_11;
	}
	case BSShaderManager::BSSM_AD2_SbShp:
	case BSShaderManager::BSSM_AD3_SbShp: {
		v3->m_kStages.m_pBase[5].m_pObject->m_spTexture = BSShaderManager::pProjectedShadowTexture.m_pObject;
		v8 = v3->m_kStages.m_pBase[6].m_pObject;
		goto LABEL_11;
	}
	case BSShaderManager::BSSM_2x_SPECULARDIR_SbShp: {
		v3->m_kStages.m_pBase[1].m_pObject->m_spTexture = BSShaderManager::pProjectedShadowTexture.m_pObject;
		v8 = v3->m_kStages.m_pBase[2].m_pObject;
	LABEL_11:;
		v8->m_spTexture = static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
		break;
	}
	default: {
		break;
	}
	}
	if (v4 == BSShaderManager::BSSM_2x_SIMPLESHADOW_Sb) {
		NiD3DTextureStage* v9 = v3->m_kStages.m_pBase[6].m_pObject;
		v9->m_spTexture = (NiTexture*)(*BSShaderManager::pCurrentRenderPass->ppSceneLights)
							  ->spDynamicShadowMap.m_pObject->GetTexture(0);
	}
	ApplyTextureStages();
}

void SpeedTreeBranchShader::SetupGeometryConstants(const NiPropertyState* apState) {
	NiTriStrips* pGeometry = static_cast<NiTriStrips*>(BSShaderManager::pCurrentRenderPass->pGeometry);
	NiBound* worldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
	if (!worldBound) {
		worldBound = &NiAVObject::NullBoundS;
	}
	NiGeometryBufferData* m_pkBuffData = pGeometry->m_spModelData.m_pObject->m_pkBuffData;
	NiSkinInstance* m_pObject = pGeometry->m_spSkinInstance.m_pObject;
	NiMaterialProperty* a1b = static_cast<NiMaterialProperty*>(apState->m_aspProps[2].m_pObject);
	ShadowLightShader::SetupGeometryConstants(apState);
	BSFogProperty* fogProperty = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
	UpdateTogglesConstant(pGeometry, fogProperty, a1b);
	BSShader* a1c = BSShaderManager::GetShader(1);
	a1c->m_spPixelConstantMap.m_pObject->SetShaderConstants(NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject,
		pGeometry, m_pObject, 0, m_pkBuffData, apState, 0, pGeometry->m_kWorld, *worldBound, 0, 1);
	a1c->m_spVertexConstantMap.m_pObject->SetShaderConstants(NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject,
		pGeometry, m_pObject, 0, m_pkBuffData, apState, 0, pGeometry->m_kWorld, *worldBound, 0, 1);
	NiD3DShaderConstantMap* v6 = m_spPixelConstantMap.m_pObject;
	if (v6) {
		NiD3DPixelShader* a1 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
		if (a1) {
			v6->SetShaderConstants(a1, pGeometry, m_pObject, 0, m_pkBuffData, apState, 0, pGeometry->m_kWorld,
				*worldBound, 0, 1);
		}
	}
	NiD3DShaderConstantMap* v7 = m_spVertexConstantMap.m_pObject;
	if (v7) {
		NiD3DVertexShader* a1a = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
		if (a1a) {
			v7->SetShaderConstants(a1a, pGeometry, m_pObject, 0, m_pkBuffData, apState, 0, pGeometry->m_kWorld,
				*worldBound, 0, 1);
		}
	}
}

void SpeedTreeBranchShader::InitPass(uint32_t auiCurrentPass) {
	NiD3DPass* v3;
	if (auiCurrentPass > 351) {
		switch (auiCurrentPass) {
		case 357u: {
			NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[25];
			break;
		}
		case 363u: {
			v3 = SpeedTreeBranchShader::m_kPasses[26];
			goto LABEL_35;
		}
		case 369u: {
			NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[27];
			break;
		}
		case 375u: {
			NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[28];
			break;
		}
		case 564u: {
			v3 = SpeedTreeBranchShader::m_kPasses[29];
		LABEL_35:;
			NiD3DShader::m_spCurrentPass = v3;
			break;
		}
		case 598u: {
			NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[9];
			break;
		}
		default: {
			break;
		}
		}
	} else {
		if (auiCurrentPass == 351) {
			NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[24];
		} else {
			switch (auiCurrentPass) {
			case 96u: {
				v3 = SpeedTreeBranchShader::m_kPasses[0];
				goto LABEL_35;
			}
			case 122u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[1];
				break;
			}
			case 123u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[2];
				break;
			}
			case 126u: {
				v3 = SpeedTreeBranchShader::m_kPasses[3];
				goto LABEL_35;
			}
			case 159u: {
				v3 = SpeedTreeBranchShader::m_kPasses[18];
				goto LABEL_35;
			}
			case 170u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[19];
				break;
			}
			case 181u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[20];
				break;
			}
			case 192u: {
				v3 = SpeedTreeBranchShader::m_kPasses[21];
				goto LABEL_35;
			}
			case 223u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[10];
				break;
			}
			case 232u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[11];
				break;
			}
			case 241u: {
				v3 = SpeedTreeBranchShader::m_kPasses[12];
				goto LABEL_35;
			}
			case 250u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[13];
				break;
			}
			case 264u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[14];
				break;
			}
			case 276u: {
				v3 = SpeedTreeBranchShader::m_kPasses[15];
				goto LABEL_35;
			}
			case 287u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[16];
				break;
			}
			case 298u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[17];
				break;
			}
			case 304u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[4];
				break;
			}
			case 305u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[5];
				break;
			}
			case 311u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[22];
				break;
			}
			case 323u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[23];
				break;
			}
			case 337u: {
				v3 = SpeedTreeBranchShader::m_kPasses[6];
				goto LABEL_35;
			}
			case 344u: {
				v3 = SpeedTreeBranchShader::m_kPasses[7];
				goto LABEL_35;
			}
			case 345u: {
				NiD3DShader::m_spCurrentPass = SpeedTreeBranchShader::m_kPasses[8];
				break;
			}
			default: {
				break;
			}
			}
		}
	}
	NiDX9ShaderDeclaration* m_pObject = spShaderDecl.m_pObject;
	m_spShaderDecl = m_pObject;
}

void SpeedTreeBranchShader::SetupPassConstants(uint32_t auiCurrentPass) {
	ShadowLightShader::SetupPassConstants(auiCurrentPass);
}

void SpeedTreeBranchShader::CreateConstantMaps() {
	if (!m_spVertexConstantMap.m_pObject) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap = nullptr;
		if (pMemory) {
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
		}
		m_spVertexConstantMap = pMap;
	}
	if (!m_spPixelConstantMap.m_pObject) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap;
		if (pMemory) {
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
		} else {
			pMap = nullptr;
		}
		m_spPixelConstantMap = pMap;
	}
	spDefaultPixelConstantMap = m_spPixelConstantMap;
	spDefaultVertexConstantMap = m_spVertexConstantMap;
	sub_BB2F50(m_spVertexConstantMap.m_pObject, 0x2A);
}

void SpeedTreeBranchShader::ReloadShaders() {
	LoadShaders();
	PresetPasses_1x();
	if (BSShaderManager::eShaderLevel >= 2) {
		PresetPasses_2x();
	}
}

void SpeedTreeBranchShader::ClearPassStages() {
	for (NiPointer<NiD3DPass>& spPass : SpeedTreeBranchShader::m_kPasses) {
		ClearPass(spPass.m_pObject);
	}
}

// Builds two macro tables: ten shader model 1 entries and twenty shader model 2 entries.
void SpeedTreeBranchShader::LoadVertexShaders() {
	BSShaderMacro kMacroArray1x[10];
	BSShaderMacro kMacroArray2x[20];
	char acShaderName[260];
	char acPath[260];
	kMacroArray1x[0].strPath = "lighting\\1x\\v\\base.v.hlsl";
	kMacroArray1x[0].kMacros[0].Name = "TREE";
	kMacroArray1x[0].kMacros[0].Definition = "";
	kMacroArray1x[0].kMacros[1].Name = 0;
	std::memset(&kMacroArray1x[0].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray1x[1].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
	kMacroArray1x[1].kMacros[0].Name = "TREE";
	kMacroArray1x[1].kMacros[0].Definition = "";
	kMacroArray1x[1].kMacros[1].Name = "VC";
	kMacroArray1x[1].kMacros[1].Definition = "";
	kMacroArray1x[1].kMacros[2].Name = 0;
	std::memset(&kMacroArray1x[1].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray1x[2].strPath = "lighting\\1x\\v\\ambDiffuseDirTexture.v.hlsl";
	kMacroArray1x[2].kMacros[0].Name = "TREE";
	kMacroArray1x[2].kMacros[0].Definition = "";
	kMacroArray1x[2].kMacros[1].Name = "VC";
	kMacroArray1x[2].kMacros[1].Definition = "";
	kMacroArray1x[2].kMacros[2].Name = 0;
	std::memset(&kMacroArray1x[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray1x[3].strPath = "lighting\\1x\\v\\ambDiffuseDirAndPt.v.hlsl";
	kMacroArray1x[3].kMacros[0].Name = "TREE";
	kMacroArray1x[3].kMacros[0].Definition = "";
	kMacroArray1x[3].kMacros[1].Name = 0;
	std::memset(&kMacroArray1x[3].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray1x[4].strPath = "lighting\\1x\\v\\diffuseDir.v.hlsl";
	kMacroArray1x[4].kMacros[0].Name = "TREE";
	kMacroArray1x[4].kMacros[0].Definition = "";
	kMacroArray1x[4].kMacros[1].Name = 0;
	std::memset(&kMacroArray1x[4].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray1x[5].strPath = "lighting\\1x\\v\\diffusePt.v.hlsl";
	kMacroArray1x[5].kMacros[0].Name = "TREE";
	kMacroArray1x[5].kMacros[0].Definition = "";
	kMacroArray1x[5].kMacros[1].Name = 0;
	std::memset(&kMacroArray1x[5].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray1x[6].strPath = "lighting\\1x\\v\\base.v.hlsl";
	kMacroArray1x[6].kMacros[0].Name = "TREE";
	kMacroArray1x[6].kMacros[0].Definition = "";
	kMacroArray1x[6].kMacros[1].Name = "VC";
	kMacroArray1x[6].kMacros[1].Definition = "";
	kMacroArray1x[6].kMacros[2].Name = 0;
	std::memset(&kMacroArray1x[6].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray1x[7].strPath = "lighting\\1x\\v\\specularDir.v.hlsl";
	kMacroArray1x[7].kMacros[0].Name = "TREE";
	kMacroArray1x[7].kMacros[0].Definition = "";
	kMacroArray1x[7].kMacros[1].Name = 0;
	std::memset(&kMacroArray1x[7].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray1x[8].strPath = "lighting\\1x\\v\\specularPt.v.hlsl";
	kMacroArray1x[8].kMacros[0].Name = "TREE";
	kMacroArray1x[8].kMacros[0].Definition = "";
	kMacroArray1x[8].kMacros[1].Name = 0;
	std::memset(&kMacroArray1x[8].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray1x[9].strPath = "lighting\\1x\\v\\base.v.hlsl";
	kMacroArray1x[9].kMacros[0].Name = "TREE";
	kMacroArray1x[9].kMacros[0].Definition = "";
	kMacroArray1x[9].kMacros[1].Name = "FOG";
	kMacroArray1x[9].kMacros[1].Definition = "";
	kMacroArray1x[9].kMacros[2].Name = 0;
	std::memset(&kMacroArray1x[9].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[0].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray2x[0].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[0].kMacros[0].Definition = "2";
	kMacroArray2x[0].kMacros[1].Name = "TREE";
	kMacroArray2x[0].kMacros[1].Definition = "";
	kMacroArray2x[0].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[0].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[1].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray2x[1].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[1].kMacros[0].Definition = "2";
	kMacroArray2x[1].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray2x[1].kMacros[1].Definition = "";
	kMacroArray2x[1].kMacros[2].Name = "TREE";
	kMacroArray2x[1].kMacros[2].Definition = "";
	kMacroArray2x[1].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[1].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[2].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray2x[2].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[2].kMacros[0].Definition = "3";
	kMacroArray2x[2].kMacros[1].Name = "TREE";
	kMacroArray2x[2].kMacros[1].Definition = "";
	kMacroArray2x[2].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[3].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray2x[3].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[3].kMacros[0].Definition = "3";
	kMacroArray2x[3].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray2x[3].kMacros[1].Definition = "";
	kMacroArray2x[3].kMacros[2].Name = "TREE";
	kMacroArray2x[3].kMacros[2].Definition = "";
	kMacroArray2x[3].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[3].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[4].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[4].kMacros[0].Name = "TREE";
	kMacroArray2x[4].kMacros[0].Definition = "";
	kMacroArray2x[4].kMacros[1].Name = 0;
	std::memset(&kMacroArray2x[4].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray2x[5].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[5].kMacros[0].Name = "PROJ_SHADOW";
	kMacroArray2x[5].kMacros[0].Definition = "";
	kMacroArray2x[5].kMacros[1].Name = "TREE";
	kMacroArray2x[5].kMacros[1].Definition = "";
	kMacroArray2x[5].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[5].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[6].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[6].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[6].kMacros[0].Definition = "2";
	kMacroArray2x[6].kMacros[1].Name = "TREE";
	kMacroArray2x[6].kMacros[1].Definition = "";
	kMacroArray2x[6].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[6].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[7].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[7].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[7].kMacros[0].Definition = "2";
	kMacroArray2x[7].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray2x[7].kMacros[1].Definition = "";
	kMacroArray2x[7].kMacros[2].Name = "TREE";
	kMacroArray2x[7].kMacros[2].Definition = "";
	kMacroArray2x[7].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[7].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[8].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[8].kMacros[0].Name = "SPECULAR";
	kMacroArray2x[8].kMacros[0].Definition = "";
	kMacroArray2x[8].kMacros[1].Name = "TREE";
	kMacroArray2x[8].kMacros[1].Definition = "";
	kMacroArray2x[8].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[8].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[9].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[9].kMacros[0].Name = "SPECULAR";
	kMacroArray2x[9].kMacros[0].Definition = "";
	kMacroArray2x[9].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray2x[9].kMacros[1].Definition = "";
	kMacroArray2x[9].kMacros[2].Name = "TREE";
	kMacroArray2x[9].kMacros[2].Definition = "";
	kMacroArray2x[9].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[9].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[10].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[10].kMacros[0].Name = "SPECULAR";
	kMacroArray2x[10].kMacros[0].Definition = "";
	kMacroArray2x[10].kMacros[1].Name = "LIGHTS";
	kMacroArray2x[10].kMacros[1].Definition = "2";
	kMacroArray2x[10].kMacros[2].Name = "TREE";
	kMacroArray2x[10].kMacros[2].Definition = "";
	kMacroArray2x[10].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[10].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[11].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray2x[11].kMacros[0].Name = "SPECULAR";
	kMacroArray2x[11].kMacros[0].Definition = "";
	kMacroArray2x[11].kMacros[1].Name = "LIGHTS";
	kMacroArray2x[11].kMacros[1].Definition = "2";
	kMacroArray2x[11].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray2x[11].kMacros[2].Definition = "";
	kMacroArray2x[11].kMacros[3].Name = "TREE";
	kMacroArray2x[11].kMacros[3].Definition = "";
	kMacroArray2x[11].kMacros[4].Name = 0;
	// EXE-INLINED none: memset
	kMacroArray2x[11].kMacros[4].Definition = 0;
	for (int32_t i = 5; i < 9; ++i) {
		kMacroArray2x[11].kMacros[i].Name = 0;
		kMacroArray2x[11].kMacros[i].Definition = 0;
	}
	kMacroArray2x[12].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
	kMacroArray2x[12].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[12].kMacros[0].Definition = "2";
	kMacroArray2x[12].kMacros[1].Name = "TREE";
	kMacroArray2x[12].kMacros[1].Definition = "";
	kMacroArray2x[12].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[12].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[13].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
	kMacroArray2x[13].kMacros[0].Name = "LIGHTS";
	kMacroArray2x[13].kMacros[0].Definition = "3";
	kMacroArray2x[13].kMacros[1].Name = "TREE";
	kMacroArray2x[13].kMacros[1].Definition = "";
	kMacroArray2x[13].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[13].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[14].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray2x[14].kMacros[0].Name = "TREE";
	kMacroArray2x[14].kMacros[0].Definition = "";
	kMacroArray2x[14].kMacros[1].Name = 0;
	std::memset(&kMacroArray2x[14].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray2x[15].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray2x[15].kMacros[0].Name = "PROJ_SHADOW";
	kMacroArray2x[15].kMacros[0].Definition = "";
	kMacroArray2x[15].kMacros[1].Name = "TREE";
	kMacroArray2x[15].kMacros[1].Definition = "";
	kMacroArray2x[15].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[15].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[16].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray2x[16].kMacros[0].Name = "POINT";
	kMacroArray2x[16].kMacros[0].Definition = "";
	kMacroArray2x[16].kMacros[1].Name = "TREE";
	kMacroArray2x[16].kMacros[1].Definition = "";
	kMacroArray2x[16].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[16].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray2x[17].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray2x[17].kMacros[0].Name = "NUM_PT_LIGHTS";
	kMacroArray2x[17].kMacros[0].Definition = "2";
	kMacroArray2x[17].kMacros[1].Name = "POINT";
	kMacroArray2x[17].kMacros[1].Definition = "";
	kMacroArray2x[17].kMacros[2].Name = "TREE";
	kMacroArray2x[17].kMacros[2].Definition = "";
	kMacroArray2x[17].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[17].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[18].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray2x[18].kMacros[0].Name = "NUM_PT_LIGHTS";
	kMacroArray2x[18].kMacros[0].Definition = "3";
	kMacroArray2x[18].kMacros[1].Name = "POINT";
	kMacroArray2x[18].kMacros[1].Definition = "";
	kMacroArray2x[18].kMacros[2].Name = "TREE";
	kMacroArray2x[18].kMacros[2].Definition = "";
	kMacroArray2x[18].kMacros[3].Name = 0;
	std::memset(&kMacroArray2x[18].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray2x[19].strPath = "lighting\\2x\\v\\SimpleShadow.v.hlsl";
	kMacroArray2x[19].kMacros[0].Name = "SHADOWMAP";
	kMacroArray2x[19].kMacros[0].Definition = "";
	kMacroArray2x[19].kMacros[1].Name = "TREE";
	kMacroArray2x[19].kMacros[1].Definition = "";
	kMacroArray2x[19].kMacros[2].Name = 0;
	std::memset(&kMacroArray2x[19].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	for (int32_t i = 0; i < 10; ++i) {
		BSShaderMacro& rMacro = kMacroArray1x[i];
		NiPointer<NiD3DVertexShader>& spShader = pVertexShader[i];
		if (rMacro.strPath) {
			BSShader::MakePath(rMacro.strPath, acPath);
			BSsprintf(acShaderName, 260, "STB1%03i.vso", i);
			NiD3DVertexShader* pShader =
				CompileVertexShaderHLSL(acPath, rMacro.kMacros, ShaderTarget_vs_1_1, acShaderName);
			spShader = pShader;
		} else {
			spShader = nullptr;
		}
	}
	if (BSShaderManager::eShaderLevel >= 2) {
		for (int32_t i = 0; i < 20; ++i) {
			BSShaderMacro& rMacro = kMacroArray2x[i];
			NiPointer<NiD3DVertexShader>& spShader = pVertexShader[10 + i];
			BSShader::MakePath(rMacro.strPath, acPath);
			BSsprintf(acShaderName, 260, "STB2%03i.vso", i);
			NiD3DVertexShader* pShader =
				CompileVertexShaderHLSL(acPath, rMacro.kMacros, ShaderTarget_vs_2_0, acShaderName);
			spShader = pShader;
		}
	}
}

void SpeedTreeBranchShader::LoadPixelShaders() {
	char acSpecMult[32];
	BSShaderMacro kMacroArray[10];
	_gcvt(ImageSpaceEffectBlur::fSpecularMult, 12, acSpecMult);
	kMacroArray[0].strPath = "lighting\\1x\\p\\base.p.hlsl";
	kMacroArray[0].kMacros[0].Name = 0;
	std::memset(&kMacroArray[0].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	kMacroArray[1].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
	kMacroArray[1].kMacros[0].Name = "VC";
	kMacroArray[1].kMacros[0].Definition = "";
	kMacroArray[1].kMacros[1].Name = 0;
	std::memset(&kMacroArray[1].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[2].strPath = "lighting\\1x\\p\\ambDiffDirTexture.p.hlsl";
	kMacroArray[2].kMacros[0].Name = "VC";
	kMacroArray[2].kMacros[0].Definition = "";
	kMacroArray[2].kMacros[1].Name = 0;
	std::memset(&kMacroArray[2].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[3].strPath = "lighting\\1x\\p\\ambDiffDirAndPt.p.hlsl";
	kMacroArray[3].kMacros[0].Name = 0;
	std::memset(&kMacroArray[3].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	kMacroArray[4].strPath = "lighting\\1x\\p\\diffuseDir.p.hlsl";
	kMacroArray[4].kMacros[0].Name = 0;
	std::memset(&kMacroArray[4].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	kMacroArray[5].strPath = "lighting\\1x\\p\\diffusePt.p.hlsl";
	kMacroArray[5].kMacros[0].Name = 0;
	std::memset(&kMacroArray[5].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	kMacroArray[6].strPath = "lighting\\1x\\p\\base.p.hlsl";
	kMacroArray[6].kMacros[0].Name = "TEX";
	kMacroArray[6].kMacros[0].Definition = "";
	kMacroArray[6].kMacros[1].Name = "VC";
	kMacroArray[6].kMacros[1].Definition = "";
	kMacroArray[6].kMacros[2].Name = 0;
	std::memset(&kMacroArray[6].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[7].strPath = "lighting\\1x\\p\\specularDir.p.hlsl";
	kMacroArray[7].kMacros[0].Name = "SPECMULT";
	kMacroArray[7].kMacros[0].Definition = acSpecMult;
	kMacroArray[7].kMacros[1].Name = 0;
	std::memset(&kMacroArray[7].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[8].strPath = "lighting\\1x\\p\\specularPt.p.hlsl";
	kMacroArray[8].kMacros[0].Name = "SPECMULT";
	kMacroArray[8].kMacros[0].Definition = acSpecMult;
	kMacroArray[8].kMacros[1].Name = 0;
	std::memset(&kMacroArray[8].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[9].strPath = "lighting\\1x\\p\\base.p.hlsl";
	kMacroArray[9].kMacros[0].Name = "FOG";
	kMacroArray[9].kMacros[0].Definition = "";
	kMacroArray[9].kMacros[1].Name = 0;
	std::memset(&kMacroArray[9].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	const char* pcTarget = BSShaderManager::GetPSTarget(true);
	for (int32_t i = 0; i < 10; ++i) {
		BSShaderMacro& rMacro = kMacroArray[i];
		NiPointer<NiD3DPixelShader>& spShader = pPixelShader[i];
		if (rMacro.strPath) {
			char acPath[260];
			char acShaderName[260];
			BSShader::MakePath(rMacro.strPath, acPath);
			BSsprintf(acShaderName, 260, "STB1%03i.pso", i);
			NiD3DPixelShader* pShader = CompilePixelShaderHLSL(acPath, rMacro.kMacros, pcTarget, acShaderName);
			spShader = pShader;
		} else {
			spShader = nullptr;
		}
	}
}

bool SpeedTreeBranchShader::LoadStagesAndPasses() {
	NiPointer<NiD3DPass>* v2 = SpeedTreeBranchShader::m_kPasses;
	int32_t v3 = 30;
	do {
		NiD3DPass::CreateNewPass(*v2++);
		--v3;
	} while (v3);
	PresetPasses_1x();
	if (BSShaderManager::eShaderLevel >= 2) {
		PresetPasses_2x();
	}
	return 1;
}

void SpeedTreeBranchShader::PresetPasses_1x() {
	NiD3DPass* v1 = 0;
	NiD3DPass* v46 = 0;
	{
		NiPointer<NiD3DTextureStage> a1;
		if (SpeedTreeBranchShader::m_kPasses[0]) {
			v1 = SpeedTreeBranchShader::m_kPasses[0];
			v46 = SpeedTreeBranchShader::m_kPasses[0];
			++SpeedTreeBranchShader::m_kPasses[0]->m_uiRefCount;
		}
		if (!v1->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* m_pObject = this->pVertexShader[0].m_pObject;
		v1->m_spVertexShader = m_pObject;
		NiD3DPixelShader* v4 = this->pPixelShader[0].m_pObject;
		v1->m_spPixelShader = v4;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[96] = ShadowLightShader::ulVertexConstantFlags[88];
		ShadowLightShader::ulPixelConstantFlags[96] = ShadowLightShader::ulPixelConstantFlags[88];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[96] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[88];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[96] = ShadowLightShader::uiPixelNonVolatileConstantFlags[88];
		if (v1 != SpeedTreeBranchShader::m_kPasses[1]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[1];
			v46 = SpeedTreeBranchShader::m_kPasses[1];
			if (SpeedTreeBranchShader::m_kPasses[1]) {
				++SpeedTreeBranchShader::m_kPasses[1]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 2) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v7 = this->pVertexShader[1].m_pObject;
		v1->m_spVertexShader = v7;
		NiD3DPixelShader* v9 = this->pPixelShader[1].m_pObject;
		v1->m_spPixelShader = v9;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[122] = ShadowLightShader::ulVertexConstantFlags[100];
		ShadowLightShader::ulPixelConstantFlags[122] = ShadowLightShader::ulPixelConstantFlags[100];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[122] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[100];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[122] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[100];
		if (v1 != SpeedTreeBranchShader::m_kPasses[2]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[2];
			v46 = SpeedTreeBranchShader::m_kPasses[2];
			if (SpeedTreeBranchShader::m_kPasses[2]) {
				++SpeedTreeBranchShader::m_kPasses[2]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 2) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v11 = this->pVertexShader[2].m_pObject;
		v1->m_spVertexShader = v11;
		NiD3DPixelShader* v13 = this->pPixelShader[2].m_pObject;
		v1->m_spPixelShader = v13;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[123] = ShadowLightShader::ulVertexConstantFlags[127];
		ShadowLightShader::ulPixelConstantFlags[123] = ShadowLightShader::ulPixelConstantFlags[127];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[123] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[127];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[123] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[127];
		if (v1 != SpeedTreeBranchShader::m_kPasses[3]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[3];
			v46 = SpeedTreeBranchShader::m_kPasses[3];
			if (SpeedTreeBranchShader::m_kPasses[3]) {
				++SpeedTreeBranchShader::m_kPasses[3]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 4) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v15 = this->pVertexShader[3].m_pObject;
		v1->m_spVertexShader = v15;
		NiD3DPixelShader* v17 = this->pPixelShader[3].m_pObject;
		v1->m_spPixelShader = v17;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[126] = ShadowLightShader::ulVertexConstantFlags[124];
		ShadowLightShader::ulPixelConstantFlags[126] = ShadowLightShader::ulPixelConstantFlags[124];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[126] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[124];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[126] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[124];
		if (v1 != SpeedTreeBranchShader::m_kPasses[4]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[4];
			v46 = SpeedTreeBranchShader::m_kPasses[4];
			if (SpeedTreeBranchShader::m_kPasses[4]) {
				++SpeedTreeBranchShader::m_kPasses[4u]->m_uiRefCount;
			}
		}
		if (!v1->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v19 = this->pVertexShader[4].m_pObject;
		v1->m_spVertexShader = v19;
		NiD3DPixelShader* v21 = this->pPixelShader[4].m_pObject;
		v1->m_spPixelShader = v21;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
		ShadowLightShader::ulVertexConstantFlags[304] = ShadowLightShader::ulVertexConstantFlags[300];
		ShadowLightShader::ulPixelConstantFlags[304] = ShadowLightShader::ulPixelConstantFlags[300];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[304] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[300];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[304] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[300];
		if (v1 != SpeedTreeBranchShader::m_kPasses[5]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[5];
			v46 = SpeedTreeBranchShader::m_kPasses[5];
			if (SpeedTreeBranchShader::m_kPasses[5]) {
				++SpeedTreeBranchShader::m_kPasses[5]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 4) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v23 = this->pVertexShader[5].m_pObject;
		v1->m_spVertexShader = v23;
		NiD3DPixelShader* v25 = this->pPixelShader[5].m_pObject;
		v1->m_spPixelShader = v25;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
		ShadowLightShader::ulVertexConstantFlags[305] = ShadowLightShader::ulVertexConstantFlags[301];
		ShadowLightShader::ulPixelConstantFlags[305] = ShadowLightShader::ulPixelConstantFlags[301];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[305] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[301];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[305] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[301];
		if (v1 != SpeedTreeBranchShader::m_kPasses[7]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[7];
			v46 = SpeedTreeBranchShader::m_kPasses[7];
			if (SpeedTreeBranchShader::m_kPasses[7]) {
				++SpeedTreeBranchShader::m_kPasses[7]->m_uiRefCount;
			}
		}
		if (!v1->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v27 = this->pVertexShader[7].m_pObject;
		v1->m_spVertexShader = v27;
		NiD3DPixelShader* v29 = this->pPixelShader[7].m_pObject;
		v1->m_spPixelShader = v29;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
		ShadowLightShader::ulVertexConstantFlags[344] = ShadowLightShader::ulVertexConstantFlags[340];
		ShadowLightShader::ulPixelConstantFlags[344] = ShadowLightShader::ulPixelConstantFlags[340];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[344] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[340];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[344] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[340];
		if (v1 != SpeedTreeBranchShader::m_kPasses[8]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[8];
			v46 = SpeedTreeBranchShader::m_kPasses[8];
			if (SpeedTreeBranchShader::m_kPasses[8]) {
				++SpeedTreeBranchShader::m_kPasses[8]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 4) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 1u, 3u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 2u, 3u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::pAttenuationZ;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 3u, 1u, 2u, 1);
			a1.m_pObject->m_spTexture = BSShaderManager::spCubeMapTexture.m_pObject;
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v31 = this->pVertexShader[8].m_pObject;
		v1->m_spVertexShader = v31;
		NiD3DPixelShader* v33 = this->pPixelShader[8].m_pObject;
		v1->m_spPixelShader = v33;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
		ShadowLightShader::ulVertexConstantFlags[345] = ShadowLightShader::ulVertexConstantFlags[341];
		ShadowLightShader::ulPixelConstantFlags[345] = ShadowLightShader::ulPixelConstantFlags[341];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[345] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[341];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[345] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[341];
		if (v1 != SpeedTreeBranchShader::m_kPasses[6]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[6];
			v46 = SpeedTreeBranchShader::m_kPasses[6];
			if (SpeedTreeBranchShader::m_kPasses[6]) {
				++SpeedTreeBranchShader::m_kPasses[6]->m_uiRefCount;
			}
		}
		if (!v1->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v35 = this->pVertexShader[6].m_pObject;
		v1->m_spVertexShader = v35;
		NiD3DPixelShader* v37 = this->pPixelShader[6].m_pObject;
		v1->m_spPixelShader = v37;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(19u, 9u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(20u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[337] = ShadowLightShader::ulVertexConstantFlags[330];
		ShadowLightShader::ulPixelConstantFlags[337] = ShadowLightShader::ulPixelConstantFlags[330];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[337] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[330];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[337] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[330];
		if (v1 != SpeedTreeBranchShader::m_kPasses[9]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[9];
			v46 = SpeedTreeBranchShader::m_kPasses[9];
			if (SpeedTreeBranchShader::m_kPasses[9]) {
				++SpeedTreeBranchShader::m_kPasses[9]->m_uiRefCount;
			}
		}
		if (!v1->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v39 = this->pVertexShader[9].m_pObject;
		v1->m_spVertexShader = v39;
		NiD3DPixelShader* v41 = this->pPixelShader[9].m_pObject;
		v1->m_spPixelShader = v41;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 3u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
		ShadowLightShader::ulVertexConstantFlags[598] = ShadowLightShader::ulVertexConstantFlags[594];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[598] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[594];
		ShadowLightShader::ulPixelConstantFlags[598] = ShadowLightShader::ulPixelConstantFlags[594];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[598] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[594];
	}
	v1->DecRefCount();
}

void SpeedTreeBranchShader::PresetPasses_2x() {
	NiD3DPass* v1 = 0;
	NiD3DPass* v60 = 0;
	{
		NiPointer<NiD3DTextureStage> spstage;
		if (SpeedTreeBranchShader::m_kPasses[10]) {
			v1 = SpeedTreeBranchShader::m_kPasses[10];
			v60 = SpeedTreeBranchShader::m_kPasses[10];
			++SpeedTreeBranchShader::m_kPasses[10]->m_uiRefCount;
		}
		if (v1->m_uiStageCount < 7) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
		}
		NiD3DVertexShader* m_pObject = this->pVertexShader[10].m_pObject;
		v1->m_spVertexShader = m_pObject;
		NiD3DPixelShader* v4 = ShadowLightShader::spPixelShaders[85];
		v1->m_spPixelShader = v4;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[223] = ShadowLightShader::ulVertexConstantFlags[215];
		ShadowLightShader::ulPixelConstantFlags[223] = ShadowLightShader::ulPixelConstantFlags[215];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[223] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[215];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[223] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[215];
		if (v1 != SpeedTreeBranchShader::m_kPasses[11]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[11];
			v60 = SpeedTreeBranchShader::m_kPasses[11];
			if (SpeedTreeBranchShader::m_kPasses[11]) {
				++SpeedTreeBranchShader::m_kPasses[11]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 7) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
		}
		NiD3DVertexShader* v7 = this->pVertexShader[11].m_pObject;
		v1->m_spVertexShader = v7;
		NiD3DPixelShader* v9 = ShadowLightShader::spPixelShaders[87];
		v1->m_spPixelShader = v9;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[232] = ShadowLightShader::ulVertexConstantFlags[224];
		ShadowLightShader::ulPixelConstantFlags[232] = ShadowLightShader::ulPixelConstantFlags[224];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[232] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[224];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[232] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[224];
		if (v1 != SpeedTreeBranchShader::m_kPasses[12]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[12];
			v60 = SpeedTreeBranchShader::m_kPasses[12];
			if (SpeedTreeBranchShader::m_kPasses[12]) {
				++SpeedTreeBranchShader::m_kPasses[12]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 7) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
		}
		NiD3DVertexShader* v11 = this->pVertexShader[12].m_pObject;
		v1->m_spVertexShader = v11;
		NiD3DPixelShader* v13 = ShadowLightShader::spPixelShaders[89];
		v1->m_spPixelShader = v13;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[241] = ShadowLightShader::ulVertexConstantFlags[233];
		ShadowLightShader::ulPixelConstantFlags[241] = ShadowLightShader::ulPixelConstantFlags[233];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[241] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[233];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[241] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[233];
		if (v1 != SpeedTreeBranchShader::m_kPasses[13]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[13];
			v60 = SpeedTreeBranchShader::m_kPasses[13];
			if (SpeedTreeBranchShader::m_kPasses[13]) {
				++SpeedTreeBranchShader::m_kPasses[13]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 7) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
		}
		NiD3DVertexShader* v15 = this->pVertexShader[13].m_pObject;
		v1->m_spVertexShader = v15;
		NiD3DPixelShader* v17 = ShadowLightShader::spPixelShaders[91];
		v1->m_spPixelShader = v17;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[250] = ShadowLightShader::ulVertexConstantFlags[242];
		ShadowLightShader::ulPixelConstantFlags[250] = ShadowLightShader::ulPixelConstantFlags[242];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[250] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[242];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[250] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[242];
		if (v1 != SpeedTreeBranchShader::m_kPasses[14]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[14];
			v60 = SpeedTreeBranchShader::m_kPasses[14];
			if (SpeedTreeBranchShader::m_kPasses[14]) {
				++SpeedTreeBranchShader::m_kPasses[14]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v19 = this->pVertexShader[14].m_pObject;
		v1->m_spVertexShader = v19;
		NiD3DPixelShader* v21 = ShadowLightShader::spPixelShaders[48];
		v1->m_spPixelShader = v21;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[264] = ShadowLightShader::ulVertexConstantFlags[251];
		ShadowLightShader::ulPixelConstantFlags[264] = ShadowLightShader::ulPixelConstantFlags[251];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[264] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[251];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[264] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[251];
		if (v1 != SpeedTreeBranchShader::m_kPasses[15]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[15];
			v60 = SpeedTreeBranchShader::m_kPasses[15];
			if (SpeedTreeBranchShader::m_kPasses[15]) {
				++SpeedTreeBranchShader::m_kPasses[15]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v23 = this->pVertexShader[15].m_pObject;
		v1->m_spVertexShader = v23;
		NiD3DPixelShader* v25 = ShadowLightShader::spPixelShaders[53];
		v1->m_spPixelShader = v25;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[276] = ShadowLightShader::ulVertexConstantFlags[266];
		ShadowLightShader::ulPixelConstantFlags[276] = ShadowLightShader::ulPixelConstantFlags[266];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[276] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[266];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[276] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[266];
		if (v1 != SpeedTreeBranchShader::m_kPasses[16]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[16];
			v60 = SpeedTreeBranchShader::m_kPasses[16];
			if (SpeedTreeBranchShader::m_kPasses[16]) {
				++SpeedTreeBranchShader::m_kPasses[16]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v27 = this->pVertexShader[16].m_pObject;
		v1->m_spVertexShader = v27;
		NiD3DPixelShader* v29 = ShadowLightShader::spPixelShaders[59];
		v1->m_spPixelShader = v29;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[287] = ShadowLightShader::ulVertexConstantFlags[278];
		ShadowLightShader::ulPixelConstantFlags[287] = ShadowLightShader::ulPixelConstantFlags[278];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[287] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[278];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[287] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[278];
		if (v1 != SpeedTreeBranchShader::m_kPasses[17]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[17];
			v60 = SpeedTreeBranchShader::m_kPasses[17];
			if (SpeedTreeBranchShader::m_kPasses[17]) {
				++SpeedTreeBranchShader::m_kPasses[17]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v31 = this->pVertexShader[17].m_pObject;
		v1->m_spVertexShader = v31;
		NiD3DPixelShader* v33 = ShadowLightShader::spPixelShaders[62];
		v1->m_spPixelShader = v33;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[298] = ShadowLightShader::ulVertexConstantFlags[289];
		ShadowLightShader::ulPixelConstantFlags[298] = ShadowLightShader::ulPixelConstantFlags[289];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[298] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[289];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[298] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[289];
		if (v1 != SpeedTreeBranchShader::m_kPasses[18]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[18];
			v60 = SpeedTreeBranchShader::m_kPasses[18];
			if (SpeedTreeBranchShader::m_kPasses[18]) {
				++SpeedTreeBranchShader::m_kPasses[18]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v35 = this->pVertexShader[18].m_pObject;
		v1->m_spVertexShader = v35;
		NiD3DPixelShader* v37 = ShadowLightShader::spPixelShaders[65];
		v1->m_spPixelShader = v37;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[159] = ShadowLightShader::ulVertexConstantFlags[149];
		ShadowLightShader::ulPixelConstantFlags[159] = ShadowLightShader::ulPixelConstantFlags[149];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[159] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[149];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[159] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[149];
		if (v1 != SpeedTreeBranchShader::m_kPasses[19]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[19];
			v60 = SpeedTreeBranchShader::m_kPasses[19];
			if (SpeedTreeBranchShader::m_kPasses[19]) {
				++SpeedTreeBranchShader::m_kPasses[19]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v39 = this->pVertexShader[19].m_pObject;
		v1->m_spVertexShader = v39;
		NiD3DPixelShader* v41 = ShadowLightShader::spPixelShaders[68];
		v1->m_spPixelShader = v41;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[170] = ShadowLightShader::ulVertexConstantFlags[160];
		ShadowLightShader::ulPixelConstantFlags[170] = ShadowLightShader::ulPixelConstantFlags[160];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[170] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[160];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[170] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[160];
		if (v1 != SpeedTreeBranchShader::m_kPasses[20]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[20];
			v60 = SpeedTreeBranchShader::m_kPasses[20];
			if (SpeedTreeBranchShader::m_kPasses[20]) {
				++SpeedTreeBranchShader::m_kPasses[20u]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v43 = this->pVertexShader[20].m_pObject;
		v1->m_spVertexShader = v43;
		NiD3DPixelShader* v45 = ShadowLightShader::spPixelShaders[71];
		v1->m_spPixelShader = v45;
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[181] = ShadowLightShader::ulVertexConstantFlags[171];
		ShadowLightShader::ulPixelConstantFlags[181] = ShadowLightShader::ulPixelConstantFlags[171];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[181] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[171];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[181] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[171];
		if (v1 != SpeedTreeBranchShader::m_kPasses[21]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[21];
			v60 = SpeedTreeBranchShader::m_kPasses[21];
			if (SpeedTreeBranchShader::m_kPasses[21]) {
				++SpeedTreeBranchShader::m_kPasses[21]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 2u, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 2u, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		NiD3DVertexShader* v47 = this->pVertexShader[21].m_pObject;
		v1->m_spVertexShader = v47;
		v1->SetPixelShader(ShadowLightShader::spPixelShaders[74]);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
		ShadowLightShader::ulVertexConstantFlags[192] = ShadowLightShader::ulVertexConstantFlags[182];
		ShadowLightShader::ulPixelConstantFlags[192] = ShadowLightShader::ulPixelConstantFlags[182];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[192] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[182];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[192] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[182];
		if (v1 != SpeedTreeBranchShader::m_kPasses[22]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[22];
			v60 = SpeedTreeBranchShader::m_kPasses[22];
			if (SpeedTreeBranchShader::m_kPasses[22]) {
				++SpeedTreeBranchShader::m_kPasses[22]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 4) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[22].m_pObject);
		NiD3DPixelShader* v50 = ShadowLightShader::GetPixelShader(93);
		v1->SetPixelShader(v50);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[311] = ShadowLightShader::ulVertexConstantFlags[306];
		ShadowLightShader::ulPixelConstantFlags[311] = ShadowLightShader::ulPixelConstantFlags[306];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[311] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[306];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[311] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[306];
		if (v1 != SpeedTreeBranchShader::m_kPasses[23]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[23];
			v60 = SpeedTreeBranchShader::m_kPasses[23];
			if (SpeedTreeBranchShader::m_kPasses[23]) {
				++SpeedTreeBranchShader::m_kPasses[23]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 4) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[23].m_pObject);
		NiD3DPixelShader* v51 = ShadowLightShader::GetPixelShader(94);
		v1->SetPixelShader(v51);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[323] = ShadowLightShader::ulVertexConstantFlags[318];
		ShadowLightShader::ulPixelConstantFlags[323] = ShadowLightShader::ulPixelConstantFlags[318];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[323] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[318];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[323] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[318];
		if (v1 != SpeedTreeBranchShader::m_kPasses[24]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[24];
			v60 = SpeedTreeBranchShader::m_kPasses[24];
			if (SpeedTreeBranchShader::m_kPasses[24]) {
				++SpeedTreeBranchShader::m_kPasses[24u]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 6) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[24].m_pObject);
		NiD3DPixelShader* v52 = ShadowLightShader::GetPixelShader(95);
		v1->SetPixelShader(v52);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[351] = ShadowLightShader::ulVertexConstantFlags[346];
		ShadowLightShader::ulPixelConstantFlags[351] = ShadowLightShader::ulPixelConstantFlags[346];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[351] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[346];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[351] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[346];
		if (v1 != SpeedTreeBranchShader::m_kPasses[25]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[25];
			v60 = SpeedTreeBranchShader::m_kPasses[25];
			if (SpeedTreeBranchShader::m_kPasses[25]) {
				++SpeedTreeBranchShader::m_kPasses[25]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 6) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[25].m_pObject);
		NiD3DPixelShader* v53 = ShadowLightShader::GetPixelShader(97);
		v1->SetPixelShader(v53);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[357] = ShadowLightShader::ulVertexConstantFlags[352];
		ShadowLightShader::ulPixelConstantFlags[357] = ShadowLightShader::ulPixelConstantFlags[352];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[357] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[352];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[357] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[352];
		if (v1 != SpeedTreeBranchShader::m_kPasses[26]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[26];
			v60 = SpeedTreeBranchShader::m_kPasses[26];
			if (SpeedTreeBranchShader::m_kPasses[26]) {
				++SpeedTreeBranchShader::m_kPasses[26]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 6) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[26].m_pObject);
		NiD3DPixelShader* v54 = ShadowLightShader::GetPixelShader(99);
		v1->SetPixelShader(v54);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[363] = ShadowLightShader::ulVertexConstantFlags[358];
		ShadowLightShader::ulPixelConstantFlags[363] = ShadowLightShader::ulPixelConstantFlags[358];
		if (v1 != SpeedTreeBranchShader::m_kPasses[27]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[27];
			v60 = SpeedTreeBranchShader::m_kPasses[27];
			if (SpeedTreeBranchShader::m_kPasses[27]) {
				++SpeedTreeBranchShader::m_kPasses[27]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 6) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[27].m_pObject);
		NiD3DPixelShader* v55 = ShadowLightShader::GetPixelShader(101);
		v1->SetPixelShader(v55);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[369] = ShadowLightShader::ulVertexConstantFlags[364];
		ShadowLightShader::ulPixelConstantFlags[369] = ShadowLightShader::ulPixelConstantFlags[364];
		if (v1 != SpeedTreeBranchShader::m_kPasses[28]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[28];
			v60 = SpeedTreeBranchShader::m_kPasses[28];
			if (SpeedTreeBranchShader::m_kPasses[28]) {
				++SpeedTreeBranchShader::m_kPasses[28u]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 6) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(v1->m_uiCurrentStage, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
			v1->SetStage(5u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[28].m_pObject);
		NiD3DPixelShader* v56 = ShadowLightShader::GetPixelShader(103);
		v1->SetPixelShader(v56);
		v1->SetSrcDestBlendRenderState(2u, 2u, 0);
		ShadowLightShader::ulVertexConstantFlags[375] = ShadowLightShader::ulVertexConstantFlags[370];
		ShadowLightShader::ulPixelConstantFlags[375] = ShadowLightShader::ulPixelConstantFlags[370];
		if (v1 != SpeedTreeBranchShader::m_kPasses[29]) {
			v1->DecRefCount();
			v1 = SpeedTreeBranchShader::m_kPasses[29];
			v60 = SpeedTreeBranchShader::m_kPasses[29];
			if (SpeedTreeBranchShader::m_kPasses[29]) {
				++SpeedTreeBranchShader::m_kPasses[29]->m_uiRefCount;
			}
		}
		if (v1->m_uiStageCount < 8) {
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
			v1->SetStage(0, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
			v1->SetStage(1u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
			v1->SetStage(2u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
			v1->SetStage(3u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
			v1->SetStage(4u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
			spstage.m_pObject->m_spTexture = (NiTexture*)BSShaderManager::pAttenuationXY;
			spstage.m_pObject->m_uiTextureFlags = 1;
			v1->SetStage(5u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
			v1->SetStage(6u, spstage.m_pObject);
			NiD3DTextureStage::CreateNewStage(spstage);
			BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
			v1->SetStage(7u, spstage.m_pObject);
		}
		v1->SetVertexShader(this->pVertexShader[29].m_pObject);
		NiD3DPixelShader* v57 = ShadowLightShader::GetPixelShader(200);
		v1->SetPixelShader(v57);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
		v1->SetSrcDestBlendRenderState(9u, 1u, 0);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v1->m_pkRenderStateGroup) {
			v1->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v1->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
		ShadowLightShader::ulVertexConstantFlags[564] = ShadowLightShader::ulVertexConstantFlags[561];
		ShadowLightShader::uiPixelNonVolatileConstantFlags[564] =
			ShadowLightShader::uiPixelNonVolatileConstantFlags[561];
		ShadowLightShader::ulPixelConstantFlags[564] = ShadowLightShader::ulPixelConstantFlags[561];
		ShadowLightShader::uiVertexNonVolatileConstantFlags[564] =
			ShadowLightShader::uiVertexNonVolatileConstantFlags[561];
	}
	v1->DecRefCount();
}

BSShader* SpeedTreeBranchShader::CreateShader() {
	SpeedTreeBranchShader* pshader = 0;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDecl->SetEntry(3u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(4u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3,
		0);
	spShaderDecl->SetEntry(0, 5u, 10u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
		NiShaderDeclaration::SPTYPE_FLOAT4, NiShaderDeclaration::SPUSAGE_BLENDINDICES, 0,
		NiD3DShaderDeclaration::SPTESS_DEFAULT);
	spShaderDecl->GetD3DDeclaration();
	SpeedTreeBranchShader* v2 =
		static_cast<SpeedTreeBranchShader*>(NiMemObject::operator new(sizeof(SpeedTreeBranchShader)));
	if (v2) {
		pshader = ::new (v2) SpeedTreeBranchShader(spShaderDecl);
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(spShaderDecl);
	return pshader;
}
