#include "BSShader/ParallaxShader.h"

#include "BSCore/BSString.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

ParallaxShaderPassArray ParallaxShader::spPass;

NiRTTI ParallaxShader::ms_RTTI("ParallaxShader", &ShadowLightShader::ms_RTTI);

ParallaxShader::ParallaxShader(NiDX9ShaderDeclaration* apShaderDecl)
	: ShadowLightShader(apShaderDecl, nullptr, nullptr, nullptr), pVertexShader{}, pPixelShader{} {
	iShaderType = 3;
	bFirstPass = 0;
	spShaderDecl = apShaderDecl;
	spSkinnedShaderDecl = nullptr;
	ePass = BSShaderManager::BSSM_EMPTY;
}

// Releases the class's shaders and the shared passes; a pass whose last reference goes returns to the pass pool.
ParallaxShader::~ParallaxShader() {
	for (int32_t i = 0; i < 20; ++i) {
		if (pVertexShader[i]) {
			NiDecRefCount(pVertexShader[i]);
			pVertexShader[i] = nullptr;
		}
	}
	for (int32_t i = 0; i < 33; ++i) {
		if (pPixelShader[i]) {
			NiDecRefCount(pPixelShader[i]);
			pPixelShader[i] = nullptr;
		}
	}
	for (NiPointer<NiD3DPass>& rspPass : spPass) {
		NiD3DPass* pPass = rspPass.m_pObject;
		if (pPass) {
			if (pPass->m_uiRefCount-- == 1) {
				pPass->ReturnPassToPool();
			}
			rspPass.m_pObject = nullptr;
		}
	}
}

const NiRTTI* ParallaxShader::GetRTTI() const {
	return &ParallaxShader::ms_RTTI;
}

// Releases every vertex and pixel shader and the shared pass table, after the base's Initialize.
bool ParallaxShader::Initialize() {
	BSShader::Initialize();
	for (int32_t i = 0; i < 20; ++i) {
		NiD3DVertexShader* pShader = pVertexShader[i];
		if (pShader) {
			NiDecRefCount(pShader);
			pVertexShader[i] = nullptr;
		}
	}
	for (int32_t i = 0; i < 33; ++i) {
		NiD3DPixelShader* pShader = pPixelShader[i];
		if (pShader) {
			NiDecRefCount(pShader);
			pPixelShader[i] = nullptr;
		}
	}
	for (NiPointer<NiD3DPass>& rspPass : ParallaxShader::spPass) {
		NiD3DPass* pPass = rspPass.m_pObject;
		if (pPass) {
			if (pPass->m_uiRefCount-- == 1) {
				pPass->ReturnPassToPool();
			}
			rspPass.m_pObject = nullptr;
		}
	}
	return true;
}

void ParallaxShader::SetupGeometryTextures(const NiPropertyState* apState) {
	BSShaderPPLightingProperty* pprop = static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
	if (BSShaderManager::eCurrentPass > BSShaderManager::BSSM_2x_SIMPLESHADOW_LAND) {
	LABEL_15:;
		ShadowLightShader::SetupGeometryTextures(apState);
	} else {
		if (BSShaderManager::eCurrentPass >= BSShaderManager::BSSM_2x_SIMPLESHADOW) {
			SetDiffuseMap(pprop, 0, 0);
			SetShadowMap(6u);
			ApplyTextureStages();
		} else {
			switch (BSShaderManager::eCurrentPass) {
			case BSShaderManager::BSSM_ADTS_Px:
			case BSShaderManager::BSSM_ADTS_PxShp:
			case BSShaderManager::BSSM_ADTS2_Px:
			case BSShaderManager::BSSM_ADTS2_PxShp: {
				goto LABEL_10;
			}
			case BSShaderManager::BSSM_ADTS_GPx:
			case BSShaderManager::BSSM_ADTS_GPxShp:
			case BSShaderManager::BSSM_ADTS2_GPx:
			case BSShaderManager::BSSM_ADTS2_GPxShp: {
				SetGlowMap(pprop, 4u, 0, 1);
			LABEL_10:;
				SetDiffuseMap(pprop, 0, 0);
				SetNormalMap(pprop, 1u, 0);
				SetParallaxTexture(pprop, 3, 0, 1);
				ApplyTextureStages();
				return;
			}
			case BSShaderManager::BSSM_AD2_Px:
			case BSShaderManager::BSSM_AD2_PxShp:
			case BSShaderManager::BSSM_AD3_Px:
			case BSShaderManager::BSSM_AD3_PxShp: {
				goto LABEL_5;
			}
			case BSShaderManager::BSSM_AD2_GPx:
			case BSShaderManager::BSSM_AD2_GPxShp:
			case BSShaderManager::BSSM_AD3_GPx:
			case BSShaderManager::BSSM_AD3_GPxShp: {
				SetGlowMap(pprop, 3u, 0, 1);
			LABEL_5:;
				SetDiffuseMap(pprop, 0, 0);
				SetNormalMap(pprop, 1u, 0);
				goto LABEL_6;
			}
			case BSShaderManager::BSSM_ADT_Px:
			case BSShaderManager::BSSM_ADT_PxShp:
			case BSShaderManager::BSSM_ADT2_Px:
			case BSShaderManager::BSSM_ADT2_PxShp: {
				goto LABEL_8;
			}
			case BSShaderManager::BSSM_ADT_GPx:
			case BSShaderManager::BSSM_ADT_GPxShp:
			case BSShaderManager::BSSM_ADT2_GPx:
			case BSShaderManager::BSSM_ADT2_GPxShp: {
				SetGlowMap(pprop, 4u, 0, 1);
			LABEL_8:;
				SetDiffuseMap(pprop, 0, 0);
				SetNormalMap(pprop, 1u, 0);
				SetParallaxTexture(pprop, 3, 0, 1);
				ApplyTextureStages();
				return;
			}
			case BSShaderManager::BSSM_DIFFUSEPT2_Px:
			case BSShaderManager::BSSM_DIFFUSEPT3_Px: {
				SetDiffuseMap(pprop, 1u, 0);
				SetNormalMap(pprop, 0, 0);
				goto LABEL_6;
			}
			case BSShaderManager::BSSM_TEXTURE_Px: {
				SetDiffuseMap(pprop, 0, 0);
			LABEL_6:;
				SetParallaxTexture(pprop, 2, 0, 1);
				ApplyTextureStages();
				break;
			}
			case BSShaderManager::BSSM_2x_SPECULARDIR_Px:
			case BSShaderManager::BSSM_2x_SPECULARDIR_PxShp:
			case BSShaderManager::BSSM_2x_SPECULARPT_Px:
			case BSShaderManager::BSSM_2x_SPECULARPT2_Px:
			case BSShaderManager::BSSM_2x_SPECULARPT3_Px: {
				SetDiffuseMap(pprop, 1u, 0);
				SetNormalMap(pprop, 0, 0);
				SetParallaxTexture(pprop, 2, 0, 1);
				ApplyTextureStages();
				break;
			}
			default: {
				goto LABEL_15;
			}
			}
		}
	}
}

void ParallaxShader::RecreateRendererData() {
	NiD3DVertexShader** pVertexShader = this->pVertexShader;
	int32_t v3 = 20;
	do {
		if (*pVertexShader) {
			(*pVertexShader)->RecreateRendererData();
		}
		++pVertexShader;
		--v3;
	} while (v3);
	NiD3DPixelShader** pPixelShader = this->pPixelShader;
	int32_t v5 = 33;
	do {
		if (*pPixelShader) {
			(*pPixelShader)->RecreateRendererData();
		}
		++pPixelShader;
		--v5;
	} while (v5);
	ShadowLightShader::RecreateRendererData();
}

void ParallaxShader::InitPass(uint32_t auiCurrentPass) {
	NiDX9ShaderDeclaration* m_pObject = spShaderDecl.m_pObject;
	NiD3DShaderDeclaration* v4 = m_spShaderDecl.m_pObject;
	NiPointer<NiD3DShaderDeclaration>* p_m_spShaderDecl = &m_spShaderDecl;
	if (v4 != m_pObject) {
		if (v4) {
			NiDecRefCount(v4);
		}
		p_m_spShaderDecl->m_pObject = m_pObject;
		if (m_pObject) {
			NiIncRefCount(m_pObject);
		}
	}
	if (auiCurrentPass > 257) {
		switch (auiCurrentPass) {
		case 258u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[9].m_pObject;
			break;
		}
		case 259u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[10].m_pObject;
			break;
		}
		case 270u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[11].m_pObject;
			break;
		}
		case 271u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[12].m_pObject;
			break;
		}
		case 281u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[13].m_pObject;
			break;
		}
		case 282u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[14].m_pObject;
			break;
		}
		case 292u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[15].m_pObject;
			break;
		}
		case 293u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[16].m_pObject;
			break;
		}
		case 308u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[25].m_pObject;
			break;
		}
		case 320u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[26].m_pObject;
			break;
		}
		case 339u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[27].m_pObject;
			break;
		}
		case 348u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[28].m_pObject;
			break;
		}
		case 354u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[29].m_pObject;
			break;
		}
		case 360u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[30].m_pObject;
			break;
		}
		case 366u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[31].m_pObject;
			break;
		}
		case 372u: {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[32].m_pObject;
			break;
		}
		default: {
		LABEL_45:;
			ShadowLightShader::InitPass(auiCurrentPass);
			BSShader* Shader = BSShaderManager::GetShader(1);
			Shader->InitPass(auiCurrentPass);
			break;
		}
		}
	} else {
		if (auiCurrentPass == 257) {
			NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[8].m_pObject;
		} else {
			switch (auiCurrentPass) {
			case 1u:
			case 2u: {
				ShadowLightShader::InitPass(auiCurrentPass);
				BSShader* v6 = BSShaderManager::GetShader(1);
				v6->InitPass(auiCurrentPass);
				p_m_spShaderDecl->SetNew(spShaderDecl.m_pObject);
				break;
			}
			case 153u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[17].m_pObject;
				break;
			}
			case 154u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[18].m_pObject;
				break;
			}
			case 164u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[19].m_pObject;
				break;
			}
			case 165u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[20].m_pObject;
				break;
			}
			case 175u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[21].m_pObject;
				break;
			}
			case 176u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[22].m_pObject;
				break;
			}
			case 186u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[23].m_pObject;
				break;
			}
			case 187u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[24].m_pObject;
				break;
			}
			case 217u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[0].m_pObject;
				break;
			}
			case 218u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[1].m_pObject;
				break;
			}
			case 226u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[2].m_pObject;
				break;
			}
			case 227u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[3].m_pObject;
				break;
			}
			case 235u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[4].m_pObject;
				break;
			}
			case 236u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[5].m_pObject;
				break;
			}
			case 244u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[6].m_pObject;
				break;
			}
			case 245u: {
				NiD3DShader::m_spCurrentPass = ParallaxShader::spPass[7].m_pObject;
				break;
			}
			default: {
				goto LABEL_45;
			}
			}
		}
	}
}

void ParallaxShader::SetupPassTextures(uint32_t auiCurrentPass) {
	switch (auiCurrentPass) {
	case 153u:
	case 154u:
	case 175u:
	case 176u:
	case 218u:
	case 236u:
	case 257u:
	case 259u:
	case 281u:
	case 282u:
	case 308u:
	case 320u:
	case 339u:
	case 348u:
	case 360u:
	case 366u:
	case 372u: {
		return;
	}
	case 164u:
	case 165u:
	case 186u:
	case 187u:
	case 270u:
	case 271u:
	case 292u:
	case 293u: {
		SetProjectedShadowMap(6u);
		SetProjectedShadowMaskMap(7u);
		break;
	}
	case 226u:
	case 227u:
	case 244u:
	case 245u: {
		SetProjectedShadowMap(5u);
		SetProjectedShadowMaskMap(6u);
		break;
	}
	case 354u: {
		SetProjectedShadowMap(4u);
		SetProjectedShadowMaskMap(5u);
		break;
	}
	default: {
		ShadowLightShader::SetupPassTextures(auiCurrentPass);
		break;
	}
	}
}

NiD3DShaderDeclaration* ParallaxShader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
	return spShaderDecl.m_pObject;
}

// Shares the multi-pass shadow light shader's constant maps.
void ParallaxShader::CreateConstantMaps() {
	ShadowLightShader* pShader =
		static_cast<ShadowLightShader*>(BSShaderManager::GetShader(BSShaderManager::BSSM_SHADER_MP_SHADOWLIGHTSHADER));
	NiD3DShaderConstantMap* pPixelConstantMap = pShader->m_spPixelConstantMap.m_pObject;
	if (pPixelConstantMap) {
		NiIncRefCount(pPixelConstantMap);
	}
	NiD3DShaderConstantMap* pVertexConstantMap = pShader->m_spVertexConstantMap.m_pObject;
	if (pVertexConstantMap) {
		NiIncRefCount(pVertexConstantMap);
	}
	m_spVertexConstantMap = pVertexConstantMap;
	m_spPixelConstantMap = pPixelConstantMap;
	spTexEffectPConstantMap = pShader->spTexEffectPConstantMap.m_pObject;
	spTexEffectVConstantMap = pShader->spTexEffectVConstantMap.m_pObject;
	spDefaultPixelConstantMap = pShader->spDefaultPixelConstantMap.m_pObject;
	spDefaultVertexConstantMap = pShader->spDefaultVertexConstantMap.m_pObject;
	if (pVertexConstantMap) {
		NiDecRefCount(pVertexConstantMap);
	}
	if (pPixelConstantMap) {
		NiDecRefCount(pPixelConstantMap);
	}
}

// Reloads the shader programs, then rebuilds the passes.
void ParallaxShader::ReloadShaders() {
	LoadShaders();
	PresetPasses();
}

void ParallaxShader::ClearPassStages() {
	for (uint32_t i = 0; i < 33; ++i)
		ClearPass(spPass[i].m_pObject);
}

// Fills the 20 vertex shader macro sets, then on shader level 2 or higher compiles each into pVertexShader.
void ParallaxShader::LoadVertexShaders() {
	BSShaderMacro kMacroArray[20];
	kMacroArray[0].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[0].kMacros[0].Name = "PARALLAX";
	kMacroArray[0].kMacros[0].Definition = "";
	kMacroArray[0].kMacros[1].Name = nullptr;
	std::memset(&kMacroArray[0].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[1].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[1].kMacros[0].Name = "PARALLAX";
	kMacroArray[1].kMacros[0].Definition = "";
	kMacroArray[1].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray[1].kMacros[1].Definition = "";
	kMacroArray[1].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[1].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[2].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[2].kMacros[0].Name = "LIGHTS";
	kMacroArray[2].kMacros[0].Definition = "2";
	kMacroArray[2].kMacros[1].Name = "PARALLAX";
	kMacroArray[2].kMacros[1].Definition = "";
	kMacroArray[2].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[3].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[3].kMacros[0].Name = "LIGHTS";
	kMacroArray[3].kMacros[0].Definition = "2";
	kMacroArray[3].kMacros[1].Name = "PARALLAX";
	kMacroArray[3].kMacros[1].Definition = "";
	kMacroArray[3].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[3].kMacros[2].Definition = "";
	kMacroArray[3].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[3].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[4].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[4].kMacros[0].Name = "SPECULAR";
	kMacroArray[4].kMacros[0].Definition = "";
	kMacroArray[4].kMacros[1].Name = "PARALLAX";
	kMacroArray[4].kMacros[1].Definition = "";
	kMacroArray[4].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[4].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[5].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[5].kMacros[0].Name = "SPECULAR";
	kMacroArray[5].kMacros[0].Definition = "";
	kMacroArray[5].kMacros[1].Name = "PARALLAX";
	kMacroArray[5].kMacros[1].Definition = "";
	kMacroArray[5].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[5].kMacros[2].Definition = "";
	kMacroArray[5].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[5].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[6].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[6].kMacros[0].Name = "SPECULAR";
	kMacroArray[6].kMacros[0].Definition = "";
	kMacroArray[6].kMacros[1].Name = "LIGHTS";
	kMacroArray[6].kMacros[1].Definition = "2";
	kMacroArray[6].kMacros[2].Name = "PARALLAX";
	kMacroArray[6].kMacros[2].Definition = "";
	kMacroArray[6].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[6].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[7].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
	kMacroArray[7].kMacros[0].Name = "SPECULAR";
	kMacroArray[7].kMacros[0].Definition = "";
	kMacroArray[7].kMacros[1].Name = "LIGHTS";
	kMacroArray[7].kMacros[1].Definition = "2";
	kMacroArray[7].kMacros[2].Name = "PARALLAX";
	kMacroArray[7].kMacros[2].Definition = "";
	kMacroArray[7].kMacros[3].Name = "PROJ_SHADOW";
	kMacroArray[7].kMacros[3].Definition = "";
	kMacroArray[7].kMacros[4].Name = nullptr;
	kMacroArray[7].kMacros[4].Definition = nullptr;
	kMacroArray[7].kMacros[5].Name = nullptr;
	kMacroArray[7].kMacros[5].Definition = nullptr;
	kMacroArray[7].kMacros[6].Name = nullptr;
	kMacroArray[7].kMacros[6].Definition = nullptr;
	kMacroArray[7].kMacros[7].Name = nullptr;
	kMacroArray[7].kMacros[7].Definition = nullptr;
	kMacroArray[7].kMacros[8].Name = nullptr;
	kMacroArray[7].kMacros[8].Definition = nullptr;
	kMacroArray[8].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray[8].kMacros[0].Name = "LIGHTS";
	kMacroArray[8].kMacros[0].Definition = "2";
	kMacroArray[8].kMacros[1].Name = "PARALLAX";
	kMacroArray[8].kMacros[1].Definition = "";
	kMacroArray[8].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[8].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[9].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray[9].kMacros[0].Name = "LIGHTS";
	kMacroArray[9].kMacros[0].Definition = "2";
	kMacroArray[9].kMacros[1].Name = "PARALLAX";
	kMacroArray[9].kMacros[1].Definition = "";
	kMacroArray[9].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[9].kMacros[2].Definition = "";
	kMacroArray[9].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[9].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[10].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray[10].kMacros[0].Name = "LIGHTS";
	kMacroArray[10].kMacros[0].Definition = "3";
	kMacroArray[10].kMacros[1].Name = "PARALLAX";
	kMacroArray[10].kMacros[1].Definition = "";
	kMacroArray[10].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[10].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[11].strPath = "lighting\\2x\\v\\AD.v.hlsl";
	kMacroArray[11].kMacros[0].Name = "LIGHTS";
	kMacroArray[11].kMacros[0].Definition = "3";
	kMacroArray[11].kMacros[1].Name = "PARALLAX";
	kMacroArray[11].kMacros[1].Definition = "";
	kMacroArray[11].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[11].kMacros[2].Definition = "";
	kMacroArray[11].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[11].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[12].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
	kMacroArray[12].kMacros[0].Name = "LIGHTS";
	kMacroArray[12].kMacros[0].Definition = "2";
	kMacroArray[12].kMacros[1].Name = "PARALLAX";
	kMacroArray[12].kMacros[1].Definition = "";
	kMacroArray[12].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[12].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[13].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
	kMacroArray[13].kMacros[0].Name = "LIGHTS";
	kMacroArray[13].kMacros[0].Definition = "3";
	kMacroArray[13].kMacros[1].Name = "PARALLAX";
	kMacroArray[13].kMacros[1].Definition = "";
	kMacroArray[13].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[13].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[14].strPath = "lighting\\2x\\v\\Texture.v.hlsl";
	kMacroArray[14].kMacros[0].Name = "PARALLAX";
	kMacroArray[14].kMacros[0].Definition = "";
	kMacroArray[14].kMacros[1].Name = nullptr;
	std::memset(&kMacroArray[14].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[15].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray[15].kMacros[0].Name = "PARALLAX";
	kMacroArray[15].kMacros[0].Definition = "";
	kMacroArray[15].kMacros[1].Name = nullptr;
	std::memset(&kMacroArray[15].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[16].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray[16].kMacros[0].Name = "PARALLAX";
	kMacroArray[16].kMacros[0].Definition = "";
	kMacroArray[16].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray[16].kMacros[1].Definition = "";
	kMacroArray[16].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[16].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[17].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray[17].kMacros[0].Name = "POINT";
	kMacroArray[17].kMacros[0].Definition = "";
	kMacroArray[17].kMacros[1].Name = "PARALLAX";
	kMacroArray[17].kMacros[1].Definition = "";
	kMacroArray[17].kMacros[2].Name = nullptr;
	std::memset(&kMacroArray[17].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[18].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray[18].kMacros[0].Name = "NUM_PT_LIGHTS";
	kMacroArray[18].kMacros[0].Definition = "2";
	kMacroArray[18].kMacros[1].Name = "POINT";
	kMacroArray[18].kMacros[1].Definition = "";
	kMacroArray[18].kMacros[2].Name = "PARALLAX";
	kMacroArray[18].kMacros[2].Definition = "";
	kMacroArray[18].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[18].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[19].strPath = "lighting\\2x\\v\\Specular.v.hlsl";
	kMacroArray[19].kMacros[0].Name = "NUM_PT_LIGHTS";
	kMacroArray[19].kMacros[0].Definition = "3";
	kMacroArray[19].kMacros[1].Name = "POINT";
	kMacroArray[19].kMacros[1].Definition = "";
	kMacroArray[19].kMacros[2].Name = "PARALLAX";
	kMacroArray[19].kMacros[2].Definition = "";
	kMacroArray[19].kMacros[3].Name = nullptr;
	std::memset(&kMacroArray[19].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	if (BSShaderManager::eShaderLevel >= 2) {
		for (int32_t i = 0; i < 20; ++i) {
			char acPath[260];
			char acCompiledName[260];
			BSShader::MakePath(kMacroArray[i].strPath, acPath);
			BSsprintf(acCompiledName, 260, "PAR2%03i.vso", i);
			NiD3DVertexShader* pNew =
				CompileVertexShaderHLSL(acPath, kMacroArray[i].kMacros, ShaderTarget_vs_2_0, acCompiledName);
			NiD3DVertexShader* pOld = pVertexShader[i];
			if (pOld != pNew) {
				if (pOld) {
					NiDecRefCount(pOld);
				}
				pVertexShader[i] = pNew;
				if (pNew) {
					NiIncRefCount(pNew);
				}
			}
		}
	}
}

void ParallaxShader::LoadPixelShaders() {
	BSShaderMacro kMacroArray[33];
	kMacroArray[0].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[0].kMacros[0].Name = "PARALLAX";
	kMacroArray[0].kMacros[0].Definition = "";
	kMacroArray[0].kMacros[1].Name = 0;
	std::memset(&kMacroArray[0].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[1].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[1].kMacros[0].Name = "PARALLAX";
	kMacroArray[1].kMacros[0].Definition = "";
	kMacroArray[1].kMacros[1].Name = "OPT";
	kMacroArray[1].kMacros[1].Definition = "";
	kMacroArray[1].kMacros[2].Name = 0;
	std::memset(&kMacroArray[1].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[2].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[2].kMacros[0].Name = "PARALLAX";
	kMacroArray[2].kMacros[0].Definition = "";
	kMacroArray[2].kMacros[1].Name = "SI";
	kMacroArray[2].kMacros[1].Definition = "";
	kMacroArray[2].kMacros[2].Name = 0;
	std::memset(&kMacroArray[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[3].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[3].kMacros[0].Name = "PARALLAX";
	kMacroArray[3].kMacros[0].Definition = "";
	kMacroArray[3].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray[3].kMacros[1].Definition = "";
	kMacroArray[3].kMacros[2].Name = 0;
	std::memset(&kMacroArray[3].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[4].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[4].kMacros[0].Name = "PARALLAX";
	kMacroArray[4].kMacros[0].Definition = "";
	kMacroArray[4].kMacros[1].Name = "SI";
	kMacroArray[4].kMacros[1].Definition = "";
	kMacroArray[4].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[4].kMacros[2].Definition = "";
	kMacroArray[4].kMacros[3].Name = 0;
	std::memset(&kMacroArray[4].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[5].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[5].kMacros[0].Name = "LIGHTS";
	kMacroArray[5].kMacros[0].Definition = "2";
	kMacroArray[5].kMacros[1].Name = "PARALLAX";
	kMacroArray[5].kMacros[1].Definition = "";
	kMacroArray[5].kMacros[2].Name = 0;
	std::memset(&kMacroArray[5].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[6].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[6].kMacros[0].Name = "LIGHTS";
	kMacroArray[6].kMacros[0].Definition = "2";
	kMacroArray[6].kMacros[1].Name = "PARALLAX";
	kMacroArray[6].kMacros[1].Definition = "";
	kMacroArray[6].kMacros[2].Name = "SI";
	kMacroArray[6].kMacros[2].Definition = "";
	kMacroArray[6].kMacros[3].Name = 0;
	std::memset(&kMacroArray[6].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[7].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[7].kMacros[0].Name = "LIGHTS";
	kMacroArray[7].kMacros[0].Definition = "2";
	kMacroArray[7].kMacros[1].Name = "PARALLAX";
	kMacroArray[7].kMacros[1].Definition = "";
	kMacroArray[7].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[7].kMacros[2].Definition = "";
	kMacroArray[7].kMacros[3].Name = 0;
	std::memset(&kMacroArray[7].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[8].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[8].kMacros[0].Name = "LIGHTS";
	kMacroArray[8].kMacros[0].Definition = "2";
	kMacroArray[8].kMacros[1].Name = "PARALLAX";
	kMacroArray[8].kMacros[1].Definition = "";
	kMacroArray[8].kMacros[2].Name = "SI";
	kMacroArray[8].kMacros[2].Definition = "";
	kMacroArray[8].kMacros[3].Name = "PROJ_SHADOW";
	kMacroArray[8].kMacros[3].Definition = "";
	kMacroArray[8].kMacros[4].Name = 0;
	kMacroArray[8].kMacros[4].Definition = 0;
	kMacroArray[8].kMacros[5].Name = 0;
	kMacroArray[8].kMacros[5].Definition = 0;
	kMacroArray[8].kMacros[6].Name = 0;
	kMacroArray[8].kMacros[6].Definition = 0;
	kMacroArray[8].kMacros[7].Name = 0;
	kMacroArray[8].kMacros[7].Definition = 0;
	kMacroArray[8].kMacros[8].Name = 0;
	kMacroArray[8].kMacros[8].Definition = 0;
	kMacroArray[9].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[9].kMacros[0].Name = "SPECULAR";
	kMacroArray[9].kMacros[0].Definition = "";
	kMacroArray[9].kMacros[1].Name = "PARALLAX";
	kMacroArray[9].kMacros[1].Definition = "";
	kMacroArray[9].kMacros[2].Name = 0;
	std::memset(&kMacroArray[9].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[10].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[10].kMacros[0].Name = "SPECULAR";
	kMacroArray[10].kMacros[0].Definition = "";
	kMacroArray[10].kMacros[1].Name = "PARALLAX";
	kMacroArray[10].kMacros[1].Definition = "";
	kMacroArray[10].kMacros[2].Name = "SI";
	kMacroArray[10].kMacros[2].Definition = "";
	kMacroArray[10].kMacros[3].Name = 0;
	std::memset(&kMacroArray[10].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[11].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[11].kMacros[0].Name = "SPECULAR";
	kMacroArray[11].kMacros[0].Definition = "";
	kMacroArray[11].kMacros[1].Name = "PARALLAX";
	kMacroArray[11].kMacros[1].Definition = "";
	kMacroArray[11].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[11].kMacros[2].Definition = "";
	kMacroArray[11].kMacros[3].Name = 0;
	std::memset(&kMacroArray[11].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[12].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[12].kMacros[0].Name = "SPECULAR";
	kMacroArray[12].kMacros[0].Definition = "";
	kMacroArray[12].kMacros[1].Name = "PARALLAX";
	kMacroArray[12].kMacros[1].Definition = "";
	kMacroArray[12].kMacros[2].Name = "SI";
	kMacroArray[12].kMacros[2].Definition = "";
	kMacroArray[12].kMacros[3].Name = "PROJ_SHADOW";
	kMacroArray[12].kMacros[3].Definition = "";
	kMacroArray[12].kMacros[4].Name = 0;
	kMacroArray[12].kMacros[4].Definition = 0;
	kMacroArray[12].kMacros[5].Name = 0;
	kMacroArray[12].kMacros[5].Definition = 0;
	kMacroArray[12].kMacros[6].Name = 0;
	kMacroArray[12].kMacros[6].Definition = 0;
	kMacroArray[12].kMacros[7].Name = 0;
	kMacroArray[12].kMacros[7].Definition = 0;
	kMacroArray[12].kMacros[8].Name = 0;
	kMacroArray[12].kMacros[8].Definition = 0;
	kMacroArray[13].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[13].kMacros[0].Name = "LIGHTS";
	kMacroArray[13].kMacros[0].Definition = "2";
	kMacroArray[13].kMacros[1].Name = "PARALLAX";
	kMacroArray[13].kMacros[1].Definition = "";
	kMacroArray[13].kMacros[2].Name = 0;
	std::memset(&kMacroArray[13].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[14].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[14].kMacros[0].Name = "LIGHTS";
	kMacroArray[14].kMacros[0].Definition = "2";
	kMacroArray[14].kMacros[1].Name = "PARALLAX";
	kMacroArray[14].kMacros[1].Definition = "";
	kMacroArray[14].kMacros[2].Name = "SI";
	kMacroArray[14].kMacros[2].Definition = "";
	kMacroArray[14].kMacros[3].Name = 0;
	std::memset(&kMacroArray[14].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[15].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[15].kMacros[0].Name = "LIGHTS";
	kMacroArray[15].kMacros[0].Definition = "2";
	kMacroArray[15].kMacros[1].Name = "PARALLAX";
	kMacroArray[15].kMacros[1].Definition = "";
	kMacroArray[15].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[15].kMacros[2].Definition = "";
	kMacroArray[15].kMacros[3].Name = 0;
	std::memset(&kMacroArray[15].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[16].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[16].kMacros[0].Name = "LIGHTS";
	kMacroArray[16].kMacros[0].Definition = "2";
	kMacroArray[16].kMacros[1].Name = "PARALLAX";
	kMacroArray[16].kMacros[1].Definition = "";
	kMacroArray[16].kMacros[2].Name = "SI";
	kMacroArray[16].kMacros[2].Definition = "";
	kMacroArray[16].kMacros[3].Name = "PROJ_SHADOW";
	kMacroArray[16].kMacros[3].Definition = "";
	kMacroArray[16].kMacros[4].Name = 0;
	kMacroArray[16].kMacros[4].Definition = 0;
	kMacroArray[16].kMacros[5].Name = 0;
	kMacroArray[16].kMacros[5].Definition = 0;
	kMacroArray[16].kMacros[6].Name = 0;
	kMacroArray[16].kMacros[6].Definition = 0;
	kMacroArray[16].kMacros[7].Name = 0;
	kMacroArray[16].kMacros[7].Definition = 0;
	kMacroArray[16].kMacros[8].Name = 0;
	kMacroArray[16].kMacros[8].Definition = 0;
	kMacroArray[17].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[17].kMacros[0].Name = "LIGHTS";
	kMacroArray[17].kMacros[0].Definition = "3";
	kMacroArray[17].kMacros[1].Name = "PARALLAX";
	kMacroArray[17].kMacros[1].Definition = "";
	kMacroArray[17].kMacros[2].Name = 0;
	std::memset(&kMacroArray[17].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[18].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[18].kMacros[0].Name = "LIGHTS";
	kMacroArray[18].kMacros[0].Definition = "3";
	kMacroArray[18].kMacros[1].Name = "PARALLAX";
	kMacroArray[18].kMacros[1].Definition = "";
	kMacroArray[18].kMacros[2].Name = "SI";
	kMacroArray[18].kMacros[2].Definition = "";
	kMacroArray[18].kMacros[3].Name = 0;
	std::memset(&kMacroArray[18].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[19].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[19].kMacros[0].Name = "LIGHTS";
	kMacroArray[19].kMacros[0].Definition = "3";
	kMacroArray[19].kMacros[1].Name = "PARALLAX";
	kMacroArray[19].kMacros[1].Definition = "";
	kMacroArray[19].kMacros[2].Name = "PROJ_SHADOW";
	kMacroArray[19].kMacros[2].Definition = "";
	kMacroArray[19].kMacros[3].Name = 0;
	std::memset(&kMacroArray[19].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[20].strPath = "lighting\\2x\\p\\AD.p.hlsl";
	kMacroArray[20].kMacros[0].Name = "LIGHTS";
	kMacroArray[20].kMacros[0].Definition = "3";
	kMacroArray[20].kMacros[1].Name = "PARALLAX";
	kMacroArray[20].kMacros[1].Definition = "";
	kMacroArray[20].kMacros[2].Name = "SI";
	kMacroArray[20].kMacros[2].Definition = "";
	kMacroArray[20].kMacros[3].Name = "PROJ_SHADOW";
	kMacroArray[20].kMacros[3].Definition = "";
	kMacroArray[20].kMacros[4].Name = 0;
	kMacroArray[20].kMacros[4].Definition = 0;
	kMacroArray[20].kMacros[5].Name = 0;
	kMacroArray[20].kMacros[5].Definition = 0;
	kMacroArray[20].kMacros[6].Name = 0;
	kMacroArray[20].kMacros[6].Definition = 0;
	kMacroArray[20].kMacros[7].Name = 0;
	kMacroArray[20].kMacros[7].Definition = 0;
	kMacroArray[20].kMacros[8].Name = 0;
	kMacroArray[20].kMacros[8].Definition = 0;
	kMacroArray[21].strPath = "lighting\\2x\\p\\DiffusePt.p.hlsl";
	kMacroArray[21].kMacros[0].Name = "LIGHTS";
	kMacroArray[21].kMacros[0].Definition = "2";
	kMacroArray[21].kMacros[1].Name = "PARALLAX";
	kMacroArray[21].kMacros[1].Definition = "";
	kMacroArray[21].kMacros[2].Name = 0;
	std::memset(&kMacroArray[21].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[22].strPath = "lighting\\2x\\p\\DiffusePt.p.hlsl";
	kMacroArray[22].kMacros[0].Name = "LIGHTS";
	kMacroArray[22].kMacros[0].Definition = "3";
	kMacroArray[22].kMacros[1].Name = "PARALLAX";
	kMacroArray[22].kMacros[1].Definition = "";
	kMacroArray[22].kMacros[2].Name = 0;
	std::memset(&kMacroArray[22].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[23].strPath = "lighting\\2x\\p\\Texture.p.hlsl";
	kMacroArray[23].kMacros[0].Name = "PARALLAX";
	kMacroArray[23].kMacros[0].Definition = "";
	kMacroArray[23].kMacros[1].Name = 0;
	std::memset(&kMacroArray[23].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[24].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
	kMacroArray[24].kMacros[0].Name = "PARALLAX";
	kMacroArray[24].kMacros[0].Definition = "";
	kMacroArray[24].kMacros[1].Name = 0;
	std::memset(&kMacroArray[24].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kMacroArray[25].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
	kMacroArray[25].kMacros[0].Name = "PARALLAX";
	kMacroArray[25].kMacros[0].Definition = "";
	kMacroArray[25].kMacros[1].Name = "PROJ_SHADOW";
	kMacroArray[25].kMacros[1].Definition = "";
	kMacroArray[25].kMacros[2].Name = 0;
	std::memset(&kMacroArray[25].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[26].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
	kMacroArray[26].kMacros[0].Name = "POINT";
	kMacroArray[26].kMacros[0].Definition = "";
	kMacroArray[26].kMacros[1].Name = "PARALLAX";
	kMacroArray[26].kMacros[1].Definition = "";
	kMacroArray[26].kMacros[2].Name = 0;
	std::memset(&kMacroArray[26].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kMacroArray[27].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
	kMacroArray[27].kMacros[0].Name = "NUM_PT_LIGHTS";
	kMacroArray[27].kMacros[0].Definition = "2";
	kMacroArray[27].kMacros[1].Name = "POINT";
	kMacroArray[27].kMacros[1].Definition = "";
	kMacroArray[27].kMacros[2].Name = "PARALLAX";
	kMacroArray[27].kMacros[2].Definition = "";
	kMacroArray[27].kMacros[3].Name = 0;
	std::memset(&kMacroArray[27].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[28].strPath = "lighting\\2x\\p\\Specular.p.hlsl";
	kMacroArray[28].kMacros[0].Name = "NUM_PT_LIGHTS";
	kMacroArray[28].kMacros[0].Definition = "3";
	kMacroArray[28].kMacros[1].Name = "POINT";
	kMacroArray[28].kMacros[1].Definition = "";
	kMacroArray[28].kMacros[2].Name = "PARALLAX";
	kMacroArray[28].kMacros[2].Definition = "";
	kMacroArray[28].kMacros[3].Name = 0;
	std::memset(&kMacroArray[28].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[29].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[29].kMacros[0].Name = "SPECULAR";
	kMacroArray[29].kMacros[0].Definition = "";
	kMacroArray[29].kMacros[1].Name = "LIGHTS";
	kMacroArray[29].kMacros[1].Definition = "2";
	kMacroArray[29].kMacros[2].Name = "PARALLAX";
	kMacroArray[29].kMacros[2].Definition = "";
	kMacroArray[29].kMacros[3].Name = 0;
	std::memset(&kMacroArray[29].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kMacroArray[30].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[30].kMacros[0].Name = "SPECULAR";
	kMacroArray[30].kMacros[0].Definition = "";
	kMacroArray[30].kMacros[4].Definition = 0;
	kMacroArray[30].kMacros[5].Name = 0;
	kMacroArray[30].kMacros[5].Definition = 0;
	kMacroArray[30].kMacros[6].Name = 0;
	kMacroArray[30].kMacros[6].Definition = 0;
	kMacroArray[30].kMacros[7].Name = 0;
	kMacroArray[30].kMacros[7].Definition = 0;
	kMacroArray[30].kMacros[8].Name = 0;
	kMacroArray[30].kMacros[8].Definition = 0;
	kMacroArray[31].kMacros[4].Definition = 0;
	kMacroArray[31].kMacros[5].Name = 0;
	kMacroArray[31].kMacros[5].Definition = 0;
	kMacroArray[31].kMacros[6].Name = 0;
	kMacroArray[31].kMacros[6].Definition = 0;
	kMacroArray[31].kMacros[7].Name = 0;
	kMacroArray[31].kMacros[7].Definition = 0;
	kMacroArray[31].kMacros[8].Name = 0;
	kMacroArray[31].kMacros[8].Definition = 0;
	kMacroArray[30].kMacros[1].Name = "LIGHTS";
	kMacroArray[30].kMacros[1].Definition = "2";
	kMacroArray[30].kMacros[2].Name = "PARALLAX";
	kMacroArray[30].kMacros[2].Definition = "";
	kMacroArray[30].kMacros[3].Name = "SI";
	kMacroArray[30].kMacros[3].Definition = "";
	kMacroArray[30].kMacros[4].Name = 0;
	kMacroArray[31].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[31].kMacros[0].Name = "SPECULAR";
	kMacroArray[31].kMacros[0].Definition = "";
	kMacroArray[31].kMacros[1].Name = "LIGHTS";
	kMacroArray[31].kMacros[1].Definition = "2";
	kMacroArray[31].kMacros[2].Name = "PARALLAX";
	kMacroArray[31].kMacros[2].Definition = "";
	kMacroArray[31].kMacros[3].Name = "PROJ_SHADOW";
	kMacroArray[31].kMacros[3].Definition = "";
	kMacroArray[31].kMacros[4].Name = 0;
	kMacroArray[32].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
	kMacroArray[32].kMacros[0].Name = "SPECULAR";
	kMacroArray[32].kMacros[0].Definition = "";
	kMacroArray[32].kMacros[1].Name = "LIGHTS";
	kMacroArray[32].kMacros[1].Definition = "2";
	kMacroArray[32].kMacros[2].Name = "PARALLAX";
	kMacroArray[32].kMacros[2].Definition = "";
	kMacroArray[32].kMacros[3].Name = "SI";
	kMacroArray[32].kMacros[3].Definition = "";
	kMacroArray[32].kMacros[4].Name = "PROJ_SHADOW";
	kMacroArray[32].kMacros[4].Definition = "";
	kMacroArray[32].kMacros[5].Name = 0;
	kMacroArray[32].kMacros[5].Definition = 0;
	kMacroArray[32].kMacros[6].Name = 0;
	kMacroArray[32].kMacros[6].Definition = 0;
	kMacroArray[32].kMacros[7].Name = 0;
	kMacroArray[32].kMacros[7].Definition = 0;
	kMacroArray[32].kMacros[8].Name = 0;
	kMacroArray[32].kMacros[8].Definition = 0;
	// The shader level is read once: sets 29 to 32 need level 5 or higher.
	int32_t iShaderLevel = BSShaderManager::eShaderLevel;
	if (iShaderLevel >= 2) {
		char acPath[260];
		char acCompiledName[260];
		for (int32_t i = 0; i < 29; ++i) {
			BSShader::MakePath(kMacroArray[i].strPath, acPath);
			BSsprintf(acCompiledName, 260, "PAR2%03i.pso", i);
			NiD3DPixelShader* pNew = CompilePixelShaderHLSL(acPath, kMacroArray[i].kMacros, "ps_2_0", acCompiledName);
			NiD3DPixelShader* pOld = pPixelShader[i];
			if (pOld != pNew) {
				if (pOld) {
					NiDecRefCount(pOld);
				}
				pPixelShader[i] = pNew;
				if (pNew) {
					NiIncRefCount(pNew);
				}
			}
		}
		if (iShaderLevel >= 5) {
			for (int32_t i = 29; i < 33; ++i) {
				BSShader::MakePath(kMacroArray[i].strPath, acPath);
				BSsprintf(acCompiledName, 260, "PAR2%03i.pso", i);
				NiD3DPixelShader* pNew =
					CompilePixelShaderHLSL(acPath, kMacroArray[i].kMacros, "ps_2_0", acCompiledName);
				NiD3DPixelShader* pOld = pPixelShader[i];
				if (pOld != pNew) {
					if (pOld) {
						NiDecRefCount(pOld);
					}
					pPixelShader[i] = pNew;
					if (pNew) {
						NiIncRefCount(pNew);
					}
				}
			}
		}
	}
}

BSShader* ParallaxShader::CreateShader() {
	ParallaxShader* pshader = 0;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
	spShaderDecl->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDecl->SetEntry(4u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(5u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3,
		0);
	spShaderDecl->GetD3DDeclaration();
	ParallaxShader* v2 = static_cast<ParallaxShader*>(NiMemObject::operator new(sizeof(ParallaxShader)));
	if (v2) {
		pshader = ::new (v2) ParallaxShader(spShaderDecl);
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(spShaderDecl);
	return pshader;
}
