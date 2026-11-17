#include "BSShader/TileShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSShader/TileShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

NiRTTI TileShader::ms_RTTI("TileShader", &NiD3DDefaultShader::ms_RTTI);

TileShader::TileShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apShaderDeclVC)
	: spShaderDecl(apShaderDecl), spShaderDeclVC(apShaderDeclVC) {
	bFirstPass = 1;
	iShaderType = 20;
	if (apShaderDecl) {
		NiIncRefCount(apShaderDecl);
	}
	if (apShaderDeclVC) {
		NiIncRefCount(apShaderDeclVC);
	}
	TileShader::TintColor.x = 1.0f;
	TileShader::TintColor.y = 1.0f;
	TileShader::TintColor.z = 1.0f;
	TileShader::TintColor.w = 1.0f;
	TileShader::Gamma.x = 1.0f;
	TileShader::Gamma.y = 1.0f;
	TileShader::Gamma.z = 1.0f;
	TileShader::Gamma.w = 1.0f;
}

// The declarations are raw pointers the shader holds a reference on: they are released here, the rest by the
// members. The exe's last release of spShaderDecl, after it was cleared, does nothing and is left out.
TileShader::~TileShader() {
	bFirstPass = 0;
	if (spShaderDecl) {
		NiDecRefCount(spShaderDecl);
		spShaderDecl = nullptr;
	}
	ePass = BSShaderManager::BSSM_EMPTY;
	if (spShaderDeclVC) {
		NiDecRefCount(spShaderDeclVC);
	}
}

const NiRTTI* TileShader::GetRTTI() const {
	return &TileShader::ms_RTTI;
}

bool TileShader::Initialize() {
	BSShader::Initialize();
	NiPointer<NiD3DVertexShader>* spTileVertexShader = this->spTileVertexShader;
	int32_t v3 = 3;
	do {
		*spTileVertexShader = nullptr;
		++spTileVertexShader;
		--v3;
	} while (v3);
	NiPointer<NiD3DPixelShader>* spTilePixelShader = this->spTilePixelShader;
	int32_t v6 = 3;
	do {
		*spTilePixelShader = nullptr;
		++spTilePixelShader;
		--v6;
	} while (v6);
	NiD3DPass* v8 = spTileShaderPass.m_pObject;
	if (v8) {
		if (v8->m_uiRefCount-- == 1) {
			v8->ReturnPassToPool();
		}
		spTileShaderPass.m_pObject = 0;
	}
	return 1;
}

// Selects the vertex and pixel programs from the tile property's flags, applies them to the current pass and the
// render state, then sets the tile and alpha-map stages. The exe's C++ EH (unwind) frame only covers the two local
// stage pointers and is left out.
void TileShader::SetupGeometryTextures(const NiPropertyState* apState) {
	TileShaderProperty* pProp = static_cast<TileShaderProperty*>(apState->m_aspProps[3].m_pObject);
	uint8_t bUseTexScroll = pProp->bUseTexScroll;
	bool bHasAlpha = pProp->spAlphaMap.m_pObject != nullptr;
	uint8_t bUseVertexColors = pProp->bUseVertexColors;
	NiD3DRenderState* pRenderState = NiD3DShader::m_spCurrentPass->GetD3DRenderState();
	NiD3DShaderDeclaration* pDecl;
	if (bUseVertexColors) {
		NiD3DShader::m_spCurrentPass->SetVertexShader(spTileVertexShader[2].m_pObject);
		NiD3DShader::m_spCurrentPass->SetPixelShader(spTilePixelShader[2].m_pObject);
		pRenderState->SetVertexShader(NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->GetShaderHandle(),
			false);
		pRenderState->SetPixelShader(NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject->GetShaderHandle(), false);
		pDecl = spShaderDeclVC;
	} else {
		NiD3DShader::m_spCurrentPass->SetVertexShader(bUseTexScroll ? spTileVertexShader[1].m_pObject
																	: spTileVertexShader[0].m_pObject);
		pRenderState->SetVertexShader(NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->GetShaderHandle(),
			false);
		NiD3DShader::m_spCurrentPass->SetPixelShader(bHasAlpha ? spTilePixelShader[1].m_pObject
															   : spTilePixelShader[0].m_pObject);
		pRenderState->SetPixelShader(NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject->GetShaderHandle(), false);
		pDecl = spShaderDecl;
	}
	SetShaderDecl(pDecl);
	uint32_t uiClampMode = bUseTexScroll ? NiTexturingProperty::WRAP_S_WRAP_T : pProp->TexClampMode;
	NiPointer<NiD3DTextureStage> spStage0 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[0];
	NiPointer<NiD3DTextureStage> spStage1 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1];
	spStage0.m_pObject->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(uiClampMode));
	if (!pProp->bUseTexturingProperty) {
		NiTexture* pTileTexture = pProp->spTileTexture.m_pObject;
		if (pTileTexture) {
			spStage0.m_pObject->m_spTexture = pTileTexture;
			spStage0.m_pObject->ConfigureStage(true);
		}
	}
	NiTexture* pAlphaMap = pProp->spAlphaMap.m_pObject;
	if (pAlphaMap) {
		spStage1.m_pObject->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
		spStage1.m_pObject->m_spTexture = pAlphaMap;
		spStage1.m_pObject->ConfigureStage(false);
	} else if (spStage1.m_pObject) {
		spStage1.m_pObject->m_spTexture = nullptr;
	}
}

// Sets the model transform, the tile property's scroll, tint and alpha constants and the shaders' constant maps,
// then the property's scissor rectangle (scaled into the letterbox viewport when the default target is current)
// and the stencil state.
void TileShader::SetupGeometryConstants(const NiPropertyState* apState) {
	if (BSShaderManager::pCurrentRenderPass) {
		NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
		BSNiBound* m_kWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
		if (!m_kWorldBound) {
			m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
		}
		NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
		TileShaderProperty* pProp = static_cast<TileShaderProperty*>(apState->m_aspProps[3].m_pObject);
		const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
		m_pkD3DRenderer->SetModelTransform(pgeometry->m_kWorld, 0);
		if (pProp->bUseTexScroll) {
			TileShader::TexScroll.x = pProp->fTexScrollX;
			TileShader::TexScroll.y = pProp->fTexScrollY;
			TileShader::TexScroll.z = pProp->fTexScaleX;
			TileShader::TexScroll.w = pProp->fTexScaleY;
		}
		TileShader::TintColor.x = pProp->TintColor.r;
		TileShader::TintColor.y = pProp->TintColor.g;
		TileShader::TintColor.z = pProp->TintColor.b;
		TileShader::TintColor.w = pProp->TintColor.a;
		float fMaterialAlpha = 1.0f;
		NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(apState->m_aspProps[2].m_pObject);
		if (pMaterial) {
			fMaterialAlpha = pMaterial->m_fAlpha;
		}
		TileShader::TintColor.w = pProp->fAlpha * fMaterialAlpha;
		NiD3DShaderConstantMap* v6 = m_spPixelConstantMap.m_pObject;
		if (v6) {
			NiD3DPixelShader* v19 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
			if (v19) {
				v6->SetShaderConstants(v19, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld,
					*m_kWorldBound, 0, 1);
			}
		}
		NiD3DShaderConstantMap* v7 = m_spVertexConstantMap.m_pObject;
		if (v7) {
			NiD3DVertexShader* v21 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
			if (v21) {
				v7->SetShaderConstants(v21, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld,
					*m_kWorldBound, 0, 1);
			}
		}
		if (pProp->bUseScissor) {
			int32_t iTop = pProp->ScissorRect[1];
			int32_t iRight = pProp->ScissorRect[2];
			int32_t iLeft = pProp->ScissorRect[0];
			int32_t iBottom = pProp->ScissorRect[3];
			if (BSShaderManager::bLetterbox) {
				NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
				if (pRenderer->GetCurrentRenderTargetGroup() == pRenderer->GetDefaultRenderTargetGroup()) {
					NiRect<float> kViewport = BSShaderManager::kFramebufferViewport;
					uint32_t uiHeight =
						static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->GetDefaultBackBuffer()->m_uiHeight;
					Ni2DBuffer* pBackBuffer =
						static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->GetDefaultBackBuffer();
					double fRange = kViewport.m_top - kViewport.m_bottom;
					iTop = static_cast<int32_t>(static_cast<double>(uiHeight) * kViewport.m_bottom +
						static_cast<double>(iTop) * fRange);
					iBottom = static_cast<int32_t>(static_cast<double>(pBackBuffer->m_uiHeight) * kViewport.m_bottom +
						fRange * static_cast<double>(iBottom));
				}
			}
			BSRenderState::SetScissorEnable(1, BSRS_NOLOCK);
			BSRenderState::SetScissorRect(iLeft, iTop, iRight, iBottom, BSRS_NOLOCK);
		}
		NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
		if (pStencil) {
			if (pStencil->m_uFlags & NiStencilProperty::ENABLE_MASK) {
				BSRenderState::SetStencilEnable(1, BSRS_NOLOCK);
				BSRenderState::SetStencilAction(static_cast<NiStencilProperty::Action>((pStencil->m_uFlags &
																						   NiStencilProperty::
																							   FAILACTION_MASK) >>
													NiStencilProperty::FAILACTION_POS),
					static_cast<NiStencilProperty::Action>((pStencil->m_uFlags & NiStencilProperty::ZFAILACTION_MASK) >>
						NiStencilProperty::ZFAILACTION_POS),
					static_cast<NiStencilProperty::Action>((pStencil->m_uFlags & NiStencilProperty::PASSACTION_MASK) >>
						NiStencilProperty::PASSACTION_POS),
					BSRS_NOLOCK);
				BSRenderState::SetStencilFunc(static_cast<NiStencilProperty::TestFunc>(pStencil->m_uFlags >>
												  NiStencilProperty::TESTFUNC_POS),
					pStencil->m_uiRef, pStencil->m_uiMask, BSRS_NOLOCK);
			}
		}
	}
}

void TileShader::PostGeometry(const NiPropertyState* apState) {
	if (static_cast<TileShaderProperty*>(apState->m_aspProps[3].m_pObject)->bUseScissor) {
		BSRenderState::SetScissorEnable(0, BSRS_NOLOCK);
	}
	NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
	if (pStencil) {
		if ((pStencil->m_uFlags & 1) != 0) {
			BSRenderState::SetStencilEnable(0, BSRS_NOLOCK);
		}
	}
}

void TileShader::RecreateRendererData() {
	if (m_spShaderDecl.m_pObject) {
		NiPointer<NiD3DVertexShader>* spTileVertexShader = this->spTileVertexShader;
		int32_t v3 = 3;
		do {
			if (spTileVertexShader->m_pObject) {
				spTileVertexShader->m_pObject->RecreateRendererData();
			}
			++spTileVertexShader;
			--v3;
		} while (v3);
	}
	NiPointer<NiD3DPixelShader>* spTilePixelShader = this->spTilePixelShader;
	int32_t v5 = 3;
	do {
		if (spTilePixelShader->m_pObject) {
			spTilePixelShader->m_pObject->RecreateRendererData();
		}
		++spTilePixelShader;
		--v5;
	} while (v5);
	BSShader::RecreateRendererData();
}

void TileShader::InitPass(uint32_t auiCurrentPass) {
	NiD3DShader::m_spCurrentPass = spTileShaderPass.m_pObject;
}

void TileShader::SetupPassConstants(uint32_t auiCurrentPass) {
	TileShader::Gamma.x = 1.0f / BSShaderManager::fGamma;
}

// Creates the pixel map (tint colour) and the vertex map (transform and texture scroll) when they are missing. The
// variable names are empty strings with the exe's contents, not its 0x1011584 copy: AddEntry does not compare the
// address.
void TileShader::CreateConstantMaps() {
	if (!m_spPixelConstantMap.m_pObject) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap = nullptr;
		if (pMemory)
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
		m_spPixelConstantMap = pMap;
		m_spPixelConstantMap.m_pObject->AddEntry("tintcolor", 0x10000007, 0, 0, 1, "", 0x10, 4, &TintColor, false);
	}
	if (!m_spVertexConstantMap.m_pObject) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap = nullptr;
		if (pMemory)
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
		m_spVertexConstantMap = pMap;
		m_spVertexConstantMap.m_pObject->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr,
			false);
		m_spVertexConstantMap.m_pObject->AddEntry("TexScroll", 0x10000007, 0, 4, 1, "", 0x10, 4, &TexScroll, false);
	}
}

void TileShader::ReloadShaders() {
	LoadShaders();
}

void TileShader::LoadShaders() {
	LoadVertexShaders();
	LoadPixelShaders();
}

void TileShader::LoadVertexShaders() {
	BSShaderMacro v10[3];
	int32_t i = 0;
	v10[0].strPath = "tile\\tile.v.hlsl";
	v10[0].kMacros[0].Name = 0;
	std::memset(&v10[0].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	v10[1].strPath = "tile\\tile.v.hlsl";
	v10[1].kMacros[0].Name = "TEXSCROLL";
	v10[1].kMacros[0].Definition = "";
	v10[1].kMacros[1].Name = 0;
	std::memset(&v10[1].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	v10[2].strPath = "tile\\tile.v.hlsl";
	v10[2].kMacros[0].Name = "VC";
	v10[2].kMacros[0].Definition = "";
	v10[2].kMacros[1].Name = 0;
	std::memset(&v10[2].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	BSShaderMacro* v3 = v10;
	BSShaderMacro* v8 = v10;
	NiPointer<NiD3DVertexShader>* spTileVertexShader = this->spTileVertexShader;
	do {
		if (v3->strPath) {
			char compiledShaderName[260];
			char Str1[260];
			BSShader::MakePath(v3->strPath, Str1);
			BSsprintf(compiledShaderName, 260, "TILE1%03i.vso", i);
			NiD3DVertexShader* v5 = CompileVertexShaderHLSL(Str1, v3->kMacros, ShaderTarget_vs_1_1, compiledShaderName);
			NiD3DVertexShader* m_pObject = spTileVertexShader->m_pObject;
			NiD3DVertexShader* v7 = v5;
			if (spTileVertexShader->m_pObject != v5) {
				if (m_pObject) {
					NiDecRefCount(m_pObject);
				}
				spTileVertexShader->m_pObject = v7;
				if (v7) {
					NiIncRefCount(v7);
				}
			}
		}
		++i;
		v3 = v8 + 1;
		++spTileVertexShader;
		++v8;
	} while (i < 3);
}

void TileShader::LoadPixelShaders() {
	BSShaderMacro v11[3];
	int32_t i = 0;
	v11[0].strPath = "tile\\tile.p.hlsl";
	v11[0].kMacros[0].Name = 0;
	std::memset(&v11[0].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	v11[1].strPath = "tile\\tile.p.hlsl";
	v11[1].kMacros[0].Name = "ALPHAMAP";
	v11[1].kMacros[0].Definition = "";
	v11[1].kMacros[1].Name = 0;
	std::memset(&v11[1].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	v11[2].strPath = "tile\\tile.p.hlsl";
	v11[2].kMacros[0].Name = "VC";
	v11[2].kMacros[0].Definition = "";
	v11[2].kMacros[1].Name = 0;
	std::memset(&v11[2].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	BSShaderMacro* v3 = v11;
	const char* ptarget = BSShaderManager::GetPSTarget(1);
	BSShaderMacro* v8 = v11;
	NiPointer<NiD3DPixelShader>* spTilePixelShader = this->spTilePixelShader;
	do {
		if (v3->strPath) {
			char apDest[260];
			char apPath[260];
			BSShader::MakePath(v3->strPath, apPath);
			BSsprintf(apDest, 260, "TILE1%03i.pso", i);
			NiD3DPixelShader* v5 = CompilePixelShaderHLSL(apPath, v3->kMacros, ptarget, apDest);
			NiD3DPixelShader* m_pObject = spTilePixelShader->m_pObject;
			NiD3DPixelShader* v7 = v5;
			if (spTilePixelShader->m_pObject != v5) {
				if (m_pObject) {
					NiDecRefCount(m_pObject);
				}
				spTilePixelShader->m_pObject = v7;
				if (v7) {
					NiIncRefCount(v7);
				}
			}
		}
		++i;
		v3 = v8 + 1;
		++spTilePixelShader;
		++v8;
	} while (i < 3);
}

// The exe's cleanup-only C++ EH frame is left out.
bool TileShader::LoadStagesAndPasses() {
	NiPointer<NiD3DPass>* p_spTileShaderPass = &spTileShaderPass;
	NiD3DPass::CreateNewPass(spTileShaderPass);
	NiPointer<NiD3DTextureStage> aspD3DTextureStage;
	NiD3DTextureStage::CreateNewStage(aspD3DTextureStage);
	BSShader::SetupDefaultStage(aspD3DTextureStage.m_pObject, 0, 1u, 2u, 1);
	p_spTileShaderPass->m_pObject->SetStage(p_spTileShaderPass->m_pObject->m_uiCurrentStage,
		aspD3DTextureStage.m_pObject);
	aspD3DTextureStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
	NiD3DTextureStage::CreateNewStage(aspD3DTextureStage);
	BSShader::SetupDefaultStage(aspD3DTextureStage.m_pObject, 1u, 1u, 2u, 1);
	p_spTileShaderPass->m_pObject->SetStage(p_spTileShaderPass->m_pObject->m_uiCurrentStage,
		aspD3DTextureStage.m_pObject);
	NiD3DPass* m_pObject = p_spTileShaderPass->m_pObject;
	if (!p_spTileShaderPass->m_pObject->m_pkRenderStateGroup) {
		m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	}
	m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 0, 1);
	NiD3DPass* v3 = p_spTileShaderPass->m_pObject;
	if (!p_spTileShaderPass->m_pObject->m_pkRenderStateGroup) {
		v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	}
	v3->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
	NiD3DPass* v4 = p_spTileShaderPass->m_pObject;
	if (!v4->m_pkRenderStateGroup) {
		v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	}
	v4->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
	return 1;
}

// The tile shader has no textures of its own to load.
bool TileShader::LoadTextures() {
	return true;
}

// The exe's cleanup-only C++ EH frame is left out.
BSShader* TileShader::CreateShader() {
	TileShader* pshader;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
	NiDX9ShaderDeclaration* spShaderDeclVC =
		NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 0, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
	spShaderDecl->GetD3DDeclaration();
	spShaderDeclVC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDeclVC->SetEntry(1u, 0, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDeclVC->SetEntry(2u, 0, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
	spShaderDeclVC->GetD3DDeclaration();
	TileShader* v2 = static_cast<TileShader*>(NiMemObject::operator new(sizeof(TileShader)));
	if (v2) {
		pshader = ::new (v2) TileShader(spShaderDecl, spShaderDeclVC);
	} else {
		pshader = 0;
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->LoadTextures();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(spShaderDecl);
	return pshader;
}
