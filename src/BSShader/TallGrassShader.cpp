#include "BSShader/TallGrassShader.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSShader/TallGrassGroup.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>

NiRTTI TallGrassShader::ms_RTTI("TallGrassShader", &NiD3DDefaultShader::ms_RTTI);

TallGrassShader::ConstantGroup TallGrassShader::pGroupedVertexConstants;

TallGrassShader::TallGrassShader(NiDX9ShaderDeclaration* apShaderDecl)
	: fWindTimer(0.0f), pGrassInstanceDataEntry(nullptr) {
	pfPointLightColor.x = 0.0f;
	pfPointLightColor.y = 0.0f;
	pfPointLightColor.z = 0.0f;
	pfPointLightColor.w = 0.0f;
	pfPointLightData.x = 0.0f;
	pfPointLightData.y = 0.0f;
	pfPointLightData.z = 0.0f;
	pfPointLightData.w = 0.0f;
	pfAlphaTestRef.x = 0.0f;
	pfAlphaTestRef.y = 0.0f;
	pfAlphaTestRef.z = 0.0f;
	pfAlphaTestRef.w = 0.0f;
	iShaderType = 6;
	m_spShaderDecl = apShaderDecl;
	// The ambient colour constant starts as NiColorA::BLACK.
	TallGrassShader::pGroupedVertexConstants.kAmbientColor.x = NiColorA_BLACK.r;
	TallGrassShader::pGroupedVertexConstants.kAmbientColor.y = NiColorA_BLACK.g;
	TallGrassShader::pGroupedVertexConstants.kAmbientColor.z = NiColorA_BLACK.b;
	TallGrassShader::pGroupedVertexConstants.kAmbientColor.w = NiColorA_BLACK.a;
	iInstanceDataCount = BSShaderManager::eShaderLevel < 2 ? 80 : 228;
	pGrassInstanceData = static_cast<TallGrassGroup::GrassInstanceData*>(MemoryManager_Singleton
			.Allocate(sizeof(TallGrassGroup::GrassInstanceData) * iInstanceDataCount));
}

// The instance buffer is freed; the passes and shaders release themselves.
TallGrassShader::~TallGrassShader() {
	pGrassInstanceDataEntry = nullptr;
	MemoryManager_Singleton.Deallocate(pGrassInstanceData);
}

const NiRTTI* TallGrassShader::GetRTTI() const {
	return &TallGrassShader::ms_RTTI;
}

void TallGrassShader::SetupPassConstants(uint32_t auiCurrentPass) {
	// Registers 28 and 29 of the shared constants: the fog distances and power, then the fog color.
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
	} else {
		BSShaderLightingProperty::pShaderLightConstants[28].x = 1000000.0f;
		BSShaderLightingProperty::pShaderLightConstants[28].y = 0.0f;
		BSShaderLightingProperty::pShaderLightConstants[28].z = 0.0f;
		BSShaderLightingProperty::pShaderLightConstants[28].w = 0.0f;
		BSShaderLightingProperty::pShaderLightConstants[29].x = NiColorA_BLACK.r;
		BSShaderLightingProperty::pShaderLightConstants[29].y = NiColorA_BLACK.g;
		BSShaderLightingProperty::pShaderLightConstants[29].z = NiColorA_BLACK.b;
		BSShaderLightingProperty::pShaderLightConstants[29].w = NiColorA_BLACK.a;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool TallGrassShader::Initialize() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_GRASS;
	bool result = NiD3DShader::Initialize();
	if (result) {
		CreateShaders();
		CreateTextures();
		CreateStagesAndPasses();
		result = 1;
	}
	v2->eMemContext = eMemContext;
	return result;
}

// Applies the geometry's stencil property; the rest of the pipeline set-up is the base's.
uint32_t TallGrassShader::PreProcessPipeline(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
	NiGeometryData::RendererData* apBuffData, const NiPropertyState* apState, const NiDynamicEffectState* apEffects,
	const NiTransform& aWorld, const NiBound& aWorldBound) {
	m_pkD3DRenderState
		->ApplyStencil(static_cast<const NiStencilProperty*>(apState->m_aspProps[NiProperty::STENCIL].m_pObject));
	return 0;
}

// NiD3DShader::UpdatePipeline's body.
uint32_t TallGrassShader::UpdatePipeline(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
	NiGeometryData::RendererData* apBuffData, const NiPropertyState* apState, const NiDynamicEffectState* apEffects,
	const NiTransform& aWorld, const NiBound& aWorldBound) {
	return 0;
}

// The grass sway is the world's up axis in model space; tall grass properties load the sun's direction and
// colors, then the shadow or point lights. The exe keeps the current pass from before the D3DX calls.
uint32_t TallGrassShader::SetupTransformations(NiGeometry* apGeometry, const NiSkinInstance* apSkin,
	const NiSkinPartition::Partition* apPartition, NiGeometryData::RendererData* apBuffData,
	const NiPropertyState* apState, const NiDynamicEffectState* apEffects, const NiTransform& aWorld,
	const NiBound& aWorldBound) {
	m_pkD3DRenderer->SetModelTransform(aWorld, false);
	D3DXMATRIX kWorld;
	kWorld._11 = aWorld.m_fScale * aWorld.m_Rotate.m_pEntry[0][0];
	uint32_t uiPass = BSShaderManager::eCurrentPass;
	kWorld._12 = aWorld.m_Rotate.m_pEntry[1][0] * aWorld.m_fScale;
	kWorld._13 = aWorld.m_Rotate.m_pEntry[2][0] * aWorld.m_fScale;
	kWorld._21 = aWorld.m_Rotate.m_pEntry[0][1] * aWorld.m_fScale;
	kWorld._22 = aWorld.m_Rotate.m_pEntry[1][1] * aWorld.m_fScale;
	kWorld._23 = aWorld.m_Rotate.m_pEntry[2][1] * aWorld.m_fScale;
	kWorld._31 = aWorld.m_Rotate.m_pEntry[0][2] * aWorld.m_fScale;
	kWorld._32 = aWorld.m_Rotate.m_pEntry[1][2] * aWorld.m_fScale;
	kWorld._33 = aWorld.m_Rotate.m_pEntry[2][2] * aWorld.m_fScale;
	kWorld._41 = aWorld.m_Translate.x;
	kWorld._42 = aWorld.m_Translate.y;
	kWorld._43 = aWorld.m_Translate.z;
	kWorld._14 = 0.0f;
	kWorld._24 = 0.0f;
	kWorld._34 = 0.0f;
	kWorld._44 = 1.0f;
	D3DXMATRIX kInvWorld;
	D3DXMatrixInverse_Thunk(&kInvWorld, nullptr, &kWorld);
	D3DXVECTOR3 kUp;
	kUp.x = 0.0f;
	kUp.y = 1.0f;
	kUp.z = 0.0f;
	D3DXVECTOR3 kModelUp;
	D3DXVec3TransformNormal_Thunk(&kModelUp, &kUp, &kInvWorld);
	D3DXVec3Normalize_Thunk(&kUp, &kModelUp);
	pGroupedVertexConstants.kGrassSway.x = kUp.x;
	pGroupedVertexConstants.kGrassSway.y = kUp.y;
	NiProperty* pShade = apState->m_aspProps[NiProperty::SHADE].m_pObject;
	TallGrassShaderProperty* pTGSProp = nullptr;
	// Shader property type 4: the tall grass property.
	if (pShade && static_cast<NiShadeProperty*>(pShade)->iShaderPropertyType == 4) {
		pTGSProp = static_cast<TallGrassShaderProperty*>(pShade);
	}
	if (pTGSProp) {
		NiDirectionalLight* pSun = static_cast<NiDirectionalLight*>(BSShaderManager::GetAccumulator()
				->pActiveShadowSceneNode->pSunLight->spLight.m_pObject);
		if (pSun) {
			NiPoint3 kSunLightDir;
			kSunLightDir.x = pSun->m_kWorldDir.x;
			kSunLightDir.y = pSun->m_kWorldDir.y;
			kSunLightDir.z = pSun->m_kWorldDir.z;
			kSunLightDir.Unitize();
			pGroupedVertexConstants.kAmbientColor.x = pSun->m_kAmb.r;
			pGroupedVertexConstants.kAmbientColor.y = pSun->m_kAmb.g;
			pGroupedVertexConstants.kAmbientColor.z = pSun->m_kAmb.b;
			pGroupedVertexConstants.kAmbientColor.w = 1.0f;
			pGroupedVertexConstants.kDiffuseColor.r = pSun->m_kDiff.r;
			pGroupedVertexConstants.kDiffuseColor.g = pSun->m_kDiff.g;
			pGroupedVertexConstants.kDiffuseColor.b = pSun->m_kDiff.b;
			pGroupedVertexConstants.kDiffuseColor.a = 1.0f;
			pGroupedVertexConstants.kModelViewProj.x = -kSunLightDir.x;
			pGroupedVertexConstants.kModelViewProj.y = -kSunLightDir.y;
			pGroupedVertexConstants.kModelViewProj.z = -kSunLightDir.z;
			pGroupedVertexConstants.kModelViewProj.w = 1.0f;
		}
		if (BSShaderAccumulator::IsShadowPass(uiPass)) {
			UpdateShadowLights(apSkin, aWorld);
		} else if (TallGrassShaderProperty::bPointLighting) {
			// The exe tests only the count's low 16 bits (test ax, ax).
			if (static_cast<uint16_t>(pTGSProp->GetNumberofActiveNonShadowLights())) {
				UpdatePointLights(pTGSProp, kInvWorld, aWorld.m_fScale,
					static_cast<BSShaderManager::eBSSMPassEnum>(uiPass));
			}
		}
	}
	return NiD3DShader::SetupTransformations(apGeometry, apSkin, apPartition, apBuffData, apState, apEffects, aWorld,
		aWorldBound);
}

// Binds the shade property's base texture (its virtual GetBaseTexture, slot 45) to stage 0 and, in the grass shadow
// passes, the first scene light's dynamic shadow map to stage 1.
void TallGrassShader::SetupGeometryTextures(const NiPropertyState* apState) {
	BSShaderManager::eBSSMPassEnum ePass = BSShaderManager::eCurrentPass;
	NiD3DTextureStage* pStage0 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[0].m_pObject;
	BSShaderProperty* pShadeProp = static_cast<BSShaderProperty*>(apState->m_aspProps[NiProperty::SHADE].m_pObject);
	pStage0->m_spTexture = pShadeProp->GetBaseTexture();
	if (static_cast<uint32_t>(ePass) - static_cast<uint32_t>(BSShaderManager::BSSM_GRASS_SHADOW_L) <= 3) {
		NiD3DTextureStage* pStage1 = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject;
		pStage1->m_spTexture =
			(*BSShaderManager::pCurrentRenderPass->ppSceneLights)->spDynamicShadowMap.m_pObject->GetTexture(0);
	}
	ApplyTextureStages();
}

// Sets up the transformations through the vtable, then the shared grass constants: fades, alpha test reference,
// wind timer (wrapped at 2 pi by the CRT's fmod), scale mask, instances, dimmer and canopy shadow, then the pass's
// shader constants. The exe reads bHDR and eShaderLevel once, after SetupConstants.
void TallGrassShader::SetupGeometryConstants(const NiPropertyState* apState) {
	NiGeometry* pGeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
	const NiBound* pBound = pGeometry->m_kWorldBound;
	if (!pBound) {
		pBound = &NiAVObject::NullBoundS;
	}
	NiGeometryBufferData* pBuffData = pGeometry->m_spModelData.m_pObject->m_pkBuffData;
	TallGrassShaderProperty* pTGSProp =
		static_cast<TallGrassShaderProperty*>(apState->m_aspProps[NiProperty::SHADE].m_pObject);
	const NiSkinInstance* pSkin = pGeometry->m_spSkinInstance.m_pObject;
	NiD3DShader* pShader = this;
	pShader->SetupTransformations(pGeometry, pSkin, nullptr, pBuffData, apState, nullptr, pGeometry->m_kWorld, *pBound);
	if (BSShaderManager::bLiteBrite) {
		BSShaderLightingProperty::pShaderLightConstants[0].x = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].y = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].z = 1.0f;
		BSShaderLightingProperty::pShaderLightConstants[0].w = 1.0f;
	}
	pGroupedVertexConstants.kDrawDistances.x = TallGrassShaderProperty::kFadeParams.x;
	pGroupedVertexConstants.kDrawDistances.y = TallGrassShaderProperty::kFadeParams.y;
	pGroupedVertexConstants.kDrawDistances.z = TallGrassShaderProperty::kFadeParams.z;
	pGroupedVertexConstants.kDrawDistances.w = TallGrassShaderProperty::kFadeParams.w;
	NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[NiProperty::ALPHA].m_pObject);
	if (pAlpha) {
		// The divisor is a double constant (fdiv qword).
		pfAlphaTestRef.x = static_cast<float>(pAlpha->m_ucAlphaTestRef / 255.0);
		pfAlphaTestRef.y = 0.0f;
		pfAlphaTestRef.z = 0.0f;
		pfAlphaTestRef.w = 0.0f;
	} else {
		pfAlphaTestRef.x = NiColorA_BLACK.r;
		pfAlphaTestRef.y = NiColorA_BLACK.g;
		pfAlphaTestRef.z = NiColorA_BLACK.b;
		pfAlphaTestRef.w = NiColorA_BLACK.a;
	}
	// The constants are doubles (fdiv/fld qword).
	float fWind =
		BSShaderManager::fTimerValues[0] / 3600.0 * 6.283180236816406 * pTGSProp->pGrassGroup->kGrassParam.fWavePeriod;
	fWindTimer = fWind;
	if (fWind >= 6.283180236816406) {
		float fWrapped = static_cast<float>(std::fmod(fWind, 6.283180236816406));
		fWindTimer = fWrapped;
	}
	pGroupedVertexConstants.kGrassWindMagnitude.y = fWindTimer;
	if (pTGSProp->ulFlags[1] & 0x800) {
		pGroupedVertexConstants.kScaleMask.x = 1.0f;
		pGroupedVertexConstants.kScaleMask.y = 1.0f;
	} else {
		pGroupedVertexConstants.kScaleMask.x = 0.0f;
		pGroupedVertexConstants.kScaleMask.y = 0.0f;
	}
	pGroupedVertexConstants.kScaleMask.z = 1.0f;
	SetupConstants(pGeometry, pTGSProp);
	bool bHDR = BSShaderManager::bHDR;
	if (bHDR) {
		pGroupedVertexConstants.kDimmer.r = BSShaderManager::fGrassDimmer;
	}
	int32_t iShaderLevel = BSShaderManager::eShaderLevel;
	bool bSetAlphaTest = iShaderLevel < 2 || !bHDR;
	if (!BSShaderManager::bInterior && BSShaderManager::pProjectedShadowTexture &&
		(BSShaderManager::iEnabledPasses & 0x20) != 0) {
		if (iShaderLevel < 2) {
			bSetAlphaTest = true;
		} else if (static_cast<int32_t>(static_cast<int64_t>(ShadowLightShader::kShadowProjTransform.w)) != 0) {
			// The exe truncates with fistp qword and tests the low dword.
			float fProjX = ShadowLightShader::kShadowProjData.x;
			float fProjY = ShadowLightShader::kShadowProjData.y;
			pGroupedVertexConstants.kShadowProjData.x = fProjX;
			pGroupedVertexConstants.kShadowProjData.y = fProjY;
			pGroupedVertexConstants.kShadowProjData.z = fProjShadowMult;
			pGroupedVertexConstants.kShadowProjData.w = ShadowLightShader::kShadowProjData.w;
		}
	}
	if (bSetAlphaTest) {
		NiAlphaProperty* pAlphaTest = static_cast<NiAlphaProperty*>(apState->m_aspProps[NiProperty::ALPHA].m_pObject);
		BSRenderState::SetAlphaTestFuncRef(static_cast<NiAlphaProperty::TestFunction>((pAlphaTest->m_uFlags >> 10) & 7),
			pAlphaTest->m_ucAlphaTestRef, BSRS_NOLOCK);
	}
	NiD3DShaderConstantMap* pPixelMap = m_spPixelConstantMap.m_pObject;
	if (pPixelMap) {
		NiD3DPixelShader* pPixelShader = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
		if (pPixelShader) {
			pPixelMap->SetShaderConstants(pPixelShader, pGeometry, pSkin, nullptr, pBuffData, apState, nullptr,
				pGeometry->m_kWorld, *pBound, 0, true);
		}
	}
	NiD3DShaderConstantMap* pVertexMap = m_spVertexConstantMap.m_pObject;
	if (pVertexMap) {
		NiD3DVertexShader* pVertexShader = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
		if (pVertexShader) {
			pVertexMap->SetShaderConstants(pVertexShader, pGeometry, pSkin, nullptr, pBuffData, apState, nullptr,
				pGeometry->m_kWorld, *pBound, 0, true);
		}
	}
}

// Passes 0x16-0x3D and 0x25C-0x287 each map onto the pass table from its start; other passes change nothing. The exe
// indexes spPass (0x6C) with auiCurrentPass - 0x16 and auiCurrentPass - 0x25C and stores the pass into
// NiD3DShader::m_spCurrentPass (0x126F74C).
void TallGrassShader::InitPass(uint32_t auiCurrentPass) {
	if (auiCurrentPass - 0x16 <= 0x27)
		NiD3DShader::m_spCurrentPass = spPass[auiCurrentPass - 0x16].m_pObject;
	else if (auiCurrentPass - 0x25C <= 0x2B)
		NiD3DShader::m_spCurrentPass = spPass[auiCurrentPass - 0x25C].m_pObject;
}

void TallGrassShader::SetupPassTextures(uint32_t auiCurrentPass) {
	NiD3DPass* v2 = NiD3DShader::m_spCurrentPass;
	if (((auiCurrentPass >= 28) && (auiCurrentPass <= 37)) || ((auiCurrentPass - 610) <= 9)) {
		NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[1].m_pObject->m_spTexture =
			BSShaderManager::pProjectedShadowTexture.m_pObject;
		v2->m_kStages.m_pBase[2].m_pObject->m_spTexture =
			static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
	}
	if (((auiCurrentPass >= 48) && (auiCurrentPass <= 57)) || ((auiCurrentPass - 630) <= 9)) {
		v2->m_kStages.m_pBase[2].m_pObject->m_spTexture = BSShaderManager::pProjectedShadowTexture.m_pObject;
		v2->m_kStages.m_pBase[3].m_pObject->m_spTexture =
			static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
	}
}

// Builds the constant maps under the grass heap context. The exe passes its empty string at 0x1011584
// as each entry's variable name. The exe's cleanup-only EH frame around the two `new`s is left out.
void TallGrassShader::CreateConstantMaps() {
	int32_t& riContext = GetThreadMemContext();
	int32_t iOldContext = riContext;
	riContext = MEM_CONTEXT_GRASS;
	if (!m_spVertexConstantMap.m_pObject) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap = nullptr;
		if (pMemory)
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
		// EXE-INLINED none: NiPointer::operator=
		NiD3DShaderConstantMap* pOld = m_spVertexConstantMap.m_pObject;
		if (pOld != pMap) {
			if (pOld && InterlockedDecrementUInt32(&pOld->m_uiRefCount) == 0)
				static_cast<NiRefObject*>(pOld)->DeleteThis();
			m_spVertexConstantMap.m_pObject = pMap;
			if (pMap)
				pMap->IncRefCount();
		}
		m_spVertexConstantMap.m_pObject->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 9, 4, nullptr, 0, 0, nullptr,
			false);
		m_spVertexConstantMap.m_pObject->AddEntry("ShadowProjTransform", 0x10000007, 0, 13, 1, "", 16, 4,
			&ShadowLightShader::kShadowProjTransform, false);
		m_spVertexConstantMap.m_pObject->AddEntry("fogparam", 0x10000007, 0, 15, 1, "", 16, 4,
			&BSShaderLightingProperty::pShaderLightConstants[28], false);
		m_spVertexConstantMap.m_pObject->AddEntry("fogcolor", 0x10000007, 0, 14, 1, "", 16, 4,
			&BSShaderLightingProperty::pShaderLightConstants[29], false);
		m_spVertexConstantMap.m_pObject->AddEntry("grouped constants", 0x10000009, 0, 0, 9, "", 0x90, 4,
			&pGroupedVertexConstants, false);
		uint32_t uiCount = iInstanceDataCount;
		m_spVertexConstantMap.m_pObject->AddEntry("instance data", 0x10000009, 0, 20, uiCount, "", uiCount << 4, 4,
			pGrassInstanceData, false);
		pGrassInstanceDataEntry = m_spVertexConstantMap.m_pObject->GetEntry("instance data");
	}
	if (m_spPixelConstantMap.m_pObject) {
		riContext = iOldContext;
		return;
	}
	void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
	NiD3DShaderConstantMap* pMap;
	if (pMemory)
		pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
	else
		pMap = nullptr;
	// EXE-INLINED none: NiPointer::operator=
	NiD3DShaderConstantMap* pOld = m_spPixelConstantMap.m_pObject;
	if (pOld != pMap) {
		if (pOld && InterlockedDecrementUInt32(&pOld->m_uiRefCount) == 0)
			static_cast<NiRefObject*>(pOld)->DeleteThis();
		m_spPixelConstantMap.m_pObject = pMap;
		if (pMap)
			pMap->IncRefCount();
	}
	m_spPixelConstantMap.m_pObject->AddEntry("point light color", 0x10000007, 0, 2, 1, "", 16, 4, &pfPointLightColor,
		false);
	m_spPixelConstantMap.m_pObject->AddEntry("point light data", 0x10000007, 0, 4, 1, "", 16, 4, &pfPointLightData,
		false);
	m_spPixelConstantMap.m_pObject->AddEntry("alpha test ref", 0x10000007, 0, 3, 1, "", 16, 4, &pfAlphaTestRef, false);
	riContext = iOldContext;
}

void TallGrassShader::ReloadShaders() {
	LoadShaders();
	PresetStages();
}

void TallGrassShader::ClearPassStages() {
	for (uint32_t i = 0; i < 44; ++i)
		ClearPass(spPass[i].m_pObject);
}

void TallGrassShader::SetCurrentPass(uint32_t aPass, bool aFirstPass) {
	TallGrassShader::ePass = static_cast<BSShaderManager::eBSSMPassEnum>(aPass);
	TallGrassShader::bFirstPass = aFirstPass;
}

// Compiles the 24 vertex shaders (both shader models where b30TreeShaders and 3.0 shaders are on) and the 7 pixel
// shaders, the last three (SHADOWMAP) only at shader level 2.0a or better. Each table entry's macro list ends at its
// first null name; the exe clears the rest of an entry with memset (0xEC61C0) or with stores, as written here. The
// pixel shader passes append TRANSPARENCYMS at the end of an entry's list, once per pass that uses it.
// The exe's cleanup-only C++ EH frame is left out.
void TallGrassShader::LoadShaders() {
	BSShaderMacro kPS[7];
	BSShaderMacro kVS[24];
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_GRASS;
	kVS[0].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[0].kMacros[0].Name = "VS_2_0";
	kVS[0].kMacros[0].Definition = nullptr;
	kVS[0].kMacros[1].Name = nullptr;
	std::memset(&kVS[0].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kVS[1].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[1].kMacros[0].Name = "VS_2_0";
	kVS[1].kMacros[0].Definition = nullptr;
	kVS[1].kMacros[1].Name = "VERTLIT";
	kVS[1].kMacros[1].Definition = "";
	kVS[1].kMacros[2].Name = nullptr;
	std::memset(&kVS[1].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kVS[2].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[2].kMacros[0].Name = "VS_2_0";
	kVS[2].kMacros[0].Definition = nullptr;
	kVS[2].kMacros[1].Name = "FIT_TO_SLOPE";
	kVS[2].kMacros[1].Definition = "";
	kVS[2].kMacros[2].Name = nullptr;
	std::memset(&kVS[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kVS[3].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[3].kMacros[0].Name = "VS_2_0";
	kVS[3].kMacros[0].Definition = nullptr;
	kVS[3].kMacros[1].Name = "VERTLIT";
	kVS[3].kMacros[1].Definition = "";
	kVS[3].kMacros[2].Name = "FIT_TO_SLOPE";
	kVS[3].kMacros[2].Definition = "";
	kVS[3].kMacros[3].Name = nullptr;
	std::memset(&kVS[3].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[4].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[4].kMacros[0].Name = "VS_2_0";
	kVS[4].kMacros[0].Definition = nullptr;
	kVS[4].kMacros[1].Name = "PTLIGHT";
	kVS[4].kMacros[1].Definition = "";
	kVS[4].kMacros[2].Name = nullptr;
	std::memset(&kVS[4].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kVS[5].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[5].kMacros[0].Name = "VS_2_0";
	kVS[5].kMacros[0].Definition = nullptr;
	kVS[5].kMacros[1].Name = "PTLIGHT";
	kVS[5].kMacros[1].Definition = "";
	kVS[5].kMacros[2].Name = "VERTLIT";
	kVS[5].kMacros[2].Definition = "";
	kVS[5].kMacros[3].Name = nullptr;
	std::memset(&kVS[5].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[6].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[6].kMacros[0].Name = "VS_2_0";
	kVS[6].kMacros[0].Definition = nullptr;
	kVS[6].kMacros[1].Name = "PTLIGHT";
	kVS[6].kMacros[1].Definition = "";
	kVS[6].kMacros[2].Name = "FIT_TO_SLOPE";
	kVS[6].kMacros[2].Definition = "";
	kVS[6].kMacros[3].Name = nullptr;
	std::memset(&kVS[6].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[7].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[7].kMacros[0].Name = "VS_2_0";
	kVS[7].kMacros[0].Definition = nullptr;
	kVS[7].kMacros[1].Name = "PTLIGHT";
	kVS[7].kMacros[1].Definition = "";
	kVS[7].kMacros[2].Name = "VERTLIT";
	kVS[7].kMacros[2].Definition = "";
	kVS[7].kMacros[3].Name = "FIT_TO_SLOPE";
	kVS[7].kMacros[3].Definition = "";
	kVS[7].kMacros[4].Name = nullptr;
	kVS[7].kMacros[4].Definition = nullptr;
	kVS[7].kMacros[5].Name = nullptr;
	kVS[7].kMacros[5].Definition = nullptr;
	kVS[7].kMacros[6].Name = nullptr;
	kVS[7].kMacros[6].Definition = nullptr;
	kVS[7].kMacros[7].Name = nullptr;
	kVS[7].kMacros[7].Definition = nullptr;
	kVS[7].kMacros[8].Name = nullptr;
	kVS[7].kMacros[8].Definition = nullptr;
	kVS[8].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[8].kMacros[0].Name = "VS_2_0";
	kVS[8].kMacros[0].Definition = nullptr;
	kVS[8].kMacros[1].Name = "PROJ_SHADOW";
	kVS[8].kMacros[1].Definition = "";
	kVS[8].kMacros[2].Name = nullptr;
	std::memset(&kVS[8].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kVS[9].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[9].kMacros[0].Name = "VS_2_0";
	kVS[9].kMacros[0].Definition = nullptr;
	kVS[9].kMacros[1].Name = "VERTLIT";
	kVS[9].kMacros[1].Definition = "";
	kVS[9].kMacros[2].Name = "PROJ_SHADOW";
	kVS[9].kMacros[2].Definition = "";
	kVS[9].kMacros[3].Name = nullptr;
	std::memset(&kVS[9].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[10].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[10].kMacros[0].Name = "VS_2_0";
	kVS[10].kMacros[0].Definition = nullptr;
	kVS[10].kMacros[1].Name = "FIT_TO_SLOPE";
	kVS[10].kMacros[1].Definition = "";
	kVS[10].kMacros[2].Name = "PROJ_SHADOW";
	kVS[10].kMacros[2].Definition = "";
	kVS[10].kMacros[3].Name = nullptr;
	std::memset(&kVS[10].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[11].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[11].kMacros[0].Name = "VS_2_0";
	kVS[11].kMacros[0].Definition = nullptr;
	kVS[11].kMacros[1].Name = "VERTLIT";
	kVS[11].kMacros[1].Definition = "";
	kVS[11].kMacros[2].Name = "FIT_TO_SLOPE";
	kVS[11].kMacros[2].Definition = "";
	kVS[11].kMacros[3].Name = "PROJ_SHADOW";
	kVS[11].kMacros[3].Definition = "";
	kVS[11].kMacros[4].Name = nullptr;
	kVS[11].kMacros[4].Definition = nullptr;
	kVS[11].kMacros[5].Name = nullptr;
	kVS[11].kMacros[5].Definition = nullptr;
	kVS[11].kMacros[6].Name = nullptr;
	kVS[11].kMacros[6].Definition = nullptr;
	kVS[11].kMacros[7].Name = nullptr;
	kVS[11].kMacros[7].Definition = nullptr;
	kVS[11].kMacros[8].Name = nullptr;
	kVS[11].kMacros[8].Definition = nullptr;
	kVS[12].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[12].kMacros[0].Name = "VS_2_0";
	kVS[12].kMacros[0].Definition = nullptr;
	kVS[12].kMacros[1].Name = "PTLIGHT";
	kVS[12].kMacros[1].Definition = "";
	kVS[12].kMacros[2].Name = "PROJ_SHADOW";
	kVS[12].kMacros[2].Definition = "";
	kVS[12].kMacros[3].Name = nullptr;
	std::memset(&kVS[12].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[13].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[13].kMacros[0].Name = "VS_2_0";
	kVS[13].kMacros[0].Definition = nullptr;
	kVS[13].kMacros[1].Name = "PTLIGHT";
	kVS[13].kMacros[1].Definition = "";
	kVS[13].kMacros[2].Name = "VERTLIT";
	kVS[13].kMacros[2].Definition = "";
	kVS[13].kMacros[3].Name = "PROJ_SHADOW";
	kVS[13].kMacros[3].Definition = "";
	kVS[13].kMacros[4].Name = nullptr;
	kVS[13].kMacros[4].Definition = nullptr;
	kVS[13].kMacros[5].Name = nullptr;
	kVS[13].kMacros[5].Definition = nullptr;
	kVS[13].kMacros[6].Name = nullptr;
	kVS[13].kMacros[6].Definition = nullptr;
	kVS[13].kMacros[7].Name = nullptr;
	kVS[13].kMacros[7].Definition = nullptr;
	kVS[13].kMacros[8].Name = nullptr;
	kVS[13].kMacros[8].Definition = nullptr;
	kVS[14].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[14].kMacros[0].Name = "VS_2_0";
	kVS[14].kMacros[0].Definition = nullptr;
	kVS[14].kMacros[1].Name = "PTLIGHT";
	kVS[14].kMacros[1].Definition = "";
	kVS[14].kMacros[2].Name = "FIT_TO_SLOPE";
	kVS[14].kMacros[2].Definition = "";
	kVS[14].kMacros[3].Name = "PROJ_SHADOW";
	kVS[14].kMacros[3].Definition = "";
	kVS[14].kMacros[4].Name = nullptr;
	kVS[14].kMacros[4].Definition = nullptr;
	kVS[14].kMacros[5].Name = nullptr;
	kVS[14].kMacros[5].Definition = nullptr;
	kVS[14].kMacros[6].Name = nullptr;
	kVS[14].kMacros[6].Definition = nullptr;
	kVS[14].kMacros[7].Name = nullptr;
	kVS[14].kMacros[7].Definition = nullptr;
	kVS[14].kMacros[8].Name = nullptr;
	kVS[14].kMacros[8].Definition = nullptr;
	kVS[15].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[15].kMacros[0].Name = "VS_2_0";
	kVS[15].kMacros[0].Definition = nullptr;
	kVS[15].kMacros[1].Name = "PTLIGHT";
	kVS[15].kMacros[1].Definition = "";
	kVS[15].kMacros[2].Name = "VERTLIT";
	kVS[15].kMacros[2].Definition = "";
	kVS[15].kMacros[3].Name = "FIT_TO_SLOPE";
	kVS[15].kMacros[3].Definition = "";
	kVS[15].kMacros[4].Name = "PROJ_SHADOW";
	kVS[15].kMacros[4].Definition = "";
	kVS[15].kMacros[5].Name = nullptr;
	kVS[15].kMacros[5].Definition = nullptr;
	kVS[15].kMacros[6].Name = nullptr;
	kVS[15].kMacros[6].Definition = nullptr;
	kVS[15].kMacros[7].Name = nullptr;
	kVS[15].kMacros[7].Definition = nullptr;
	kVS[15].kMacros[8].Name = nullptr;
	kVS[15].kMacros[8].Definition = nullptr;
	kVS[16].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[16].kMacros[0].Name = "VS_2_0";
	kVS[16].kMacros[0].Definition = nullptr;
	kVS[16].kMacros[1].Name = "BILLBOARD";
	kVS[16].kMacros[1].Definition = "";
	kVS[16].kMacros[2].Name = nullptr;
	std::memset(&kVS[16].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kVS[17].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[17].kMacros[0].Name = "VS_2_0";
	kVS[17].kMacros[0].Definition = nullptr;
	kVS[17].kMacros[1].Name = "BILLBOARD";
	kVS[17].kMacros[1].Definition = "";
	kVS[17].kMacros[2].Name = "FIT_TO_SLOPE";
	kVS[17].kMacros[2].Definition = "";
	kVS[17].kMacros[3].Name = nullptr;
	std::memset(&kVS[17].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[18].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[18].kMacros[0].Name = "VS_2_0";
	kVS[18].kMacros[0].Definition = nullptr;
	kVS[18].kMacros[1].Name = "BILLBOARD";
	kVS[18].kMacros[1].Definition = "";
	kVS[18].kMacros[2].Name = "PTLIGHT";
	kVS[18].kMacros[2].Definition = "";
	kVS[18].kMacros[3].Name = nullptr;
	std::memset(&kVS[18].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[19].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[19].kMacros[0].Name = "VS_2_0";
	kVS[19].kMacros[0].Definition = nullptr;
	kVS[19].kMacros[1].Name = "BILLBOARD";
	kVS[19].kMacros[1].Definition = "";
	kVS[19].kMacros[2].Name = "PTLIGHT";
	kVS[19].kMacros[2].Definition = "";
	kVS[19].kMacros[3].Name = "FIT_TO_SLOPE";
	kVS[19].kMacros[3].Definition = "";
	kVS[19].kMacros[4].Name = nullptr;
	kVS[19].kMacros[4].Definition = nullptr;
	kVS[19].kMacros[5].Name = nullptr;
	kVS[19].kMacros[5].Definition = nullptr;
	kVS[19].kMacros[6].Name = nullptr;
	kVS[19].kMacros[6].Definition = nullptr;
	kVS[19].kMacros[7].Name = nullptr;
	kVS[19].kMacros[7].Definition = nullptr;
	kVS[19].kMacros[8].Name = nullptr;
	kVS[19].kMacros[8].Definition = nullptr;
	kVS[20].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[20].kMacros[0].Name = "VS_2_0";
	kVS[20].kMacros[0].Definition = nullptr;
	kVS[20].kMacros[1].Name = "SHADOWMAP";
	kVS[20].kMacros[1].Definition = "";
	kVS[20].kMacros[2].Name = nullptr;
	std::memset(&kVS[20].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kVS[21].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[21].kMacros[0].Name = "VS_2_0";
	kVS[21].kMacros[0].Definition = nullptr;
	kVS[21].kMacros[1].Name = "SHADOWMAP";
	kVS[21].kMacros[1].Definition = "";
	kVS[21].kMacros[2].Name = "FIT_TO_SLOPE";
	kVS[21].kMacros[2].Definition = "";
	kVS[21].kMacros[3].Name = nullptr;
	std::memset(&kVS[21].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[22].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[22].kMacros[0].Name = "VS_2_0";
	kVS[22].kMacros[0].Definition = nullptr;
	kVS[22].kMacros[1].Name = "SHADOWMAP";
	kVS[22].kMacros[1].Definition = "";
	kVS[22].kMacros[2].Name = "BILLBOARD";
	kVS[22].kMacros[2].Definition = "";
	kVS[22].kMacros[3].Name = nullptr;
	std::memset(&kVS[22].kMacros[3].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
	kVS[23].strPath = "tallgrass\\1x\\v\\lowDetail.v.hlsl";
	kVS[23].kMacros[0].Name = "VS_2_0";
	kVS[23].kMacros[0].Definition = nullptr;
	kVS[23].kMacros[1].Name = "SHADOWMAP";
	kVS[23].kMacros[1].Definition = "";
	kVS[23].kMacros[2].Name = "BILLBOARD";
	kVS[23].kMacros[2].Definition = "";
	kVS[23].kMacros[3].Name = "FIT_TO_SLOPE";
	kVS[23].kMacros[3].Definition = "";
	kVS[23].kMacros[4].Name = nullptr;
	kVS[23].kMacros[4].Definition = nullptr;
	kVS[23].kMacros[5].Name = nullptr;
	kVS[23].kMacros[5].Definition = nullptr;
	kVS[23].kMacros[6].Name = nullptr;
	kVS[23].kMacros[6].Definition = nullptr;
	kVS[23].kMacros[7].Name = nullptr;
	kVS[23].kMacros[7].Definition = nullptr;
	kVS[23].kMacros[8].Name = nullptr;
	kVS[23].kMacros[8].Definition = nullptr;
	kPS[0].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
	kPS[0].kMacros[0].Name = "PS_2_0";
	kPS[0].kMacros[0].Definition = nullptr;
	kPS[0].kMacros[1].Name = nullptr;
	std::memset(&kPS[0].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kPS[1].strPath = "tallgrass\\1x\\p\\highDetail_1pt.p.hlsl";
	kPS[1].kMacros[0].Name = "PS_2_0";
	kPS[1].kMacros[0].Definition = nullptr;
	kPS[1].kMacros[1].Name = nullptr;
	std::memset(&kPS[1].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	kPS[2].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
	kPS[2].kMacros[0].Name = "PS_2_0";
	kPS[2].kMacros[0].Definition = nullptr;
	kPS[2].kMacros[1].Name = "PROJ_SHADOW";
	kPS[2].kMacros[1].Definition = "";
	kPS[2].kMacros[2].Name = nullptr;
	std::memset(&kPS[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kPS[3].strPath = "tallgrass\\1x\\p\\highDetail_1pt.p.hlsl";
	kPS[3].kMacros[0].Name = "PS_2_0";
	kPS[3].kMacros[0].Definition = nullptr;
	kPS[3].kMacros[1].Name = "PROJ_SHADOW";
	kPS[3].kMacros[1].Definition = "";
	kPS[3].kMacros[2].Name = nullptr;
	std::memset(&kPS[3].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	kPS[4].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
	kPS[4].kMacros[0].Name = "PS_2_0";
	kPS[4].kMacros[0].Definition = nullptr;
	kPS[4].kMacros[1].Name = "SHADOWMAP";
	kPS[4].kMacros[1].Definition = "";
	kPS[4].kMacros[2].Name = "DEPTHBIAS";
	kPS[4].kMacros[2].Definition = "-0.7";
	kPS[4].kMacros[3].Name = "SAMPLE";
	kPS[4].kMacros[3].Definition = "1";
	kPS[4].kMacros[4].Name = "PASSES";
	kPS[4].kMacros[4].Definition = "1";
	kPS[4].kMacros[5].Name = nullptr;
	kPS[4].kMacros[5].Definition = nullptr;
	kPS[4].kMacros[6].Name = nullptr;
	kPS[4].kMacros[6].Definition = nullptr;
	kPS[4].kMacros[7].Name = nullptr;
	kPS[4].kMacros[7].Definition = nullptr;
	kPS[4].kMacros[8].Name = nullptr;
	kPS[4].kMacros[8].Definition = nullptr;
	kPS[5].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
	kPS[5].kMacros[0].Name = "PS_2_0";
	kPS[5].kMacros[0].Definition = nullptr;
	kPS[5].kMacros[1].Name = "SHADOWMAP";
	kPS[5].kMacros[1].Definition = "";
	kPS[5].kMacros[2].Name = "DEPTHBIAS";
	kPS[5].kMacros[2].Definition = "-0.7";
	kPS[5].kMacros[3].Name = "SAMPLE";
	kPS[5].kMacros[3].Definition = "4";
	kPS[5].kMacros[4].Name = "PASSES";
	kPS[5].kMacros[4].Definition = "1";
	kPS[5].kMacros[5].Name = nullptr;
	kPS[5].kMacros[5].Definition = nullptr;
	kPS[5].kMacros[6].Name = nullptr;
	kPS[5].kMacros[6].Definition = nullptr;
	kPS[5].kMacros[7].Name = nullptr;
	kPS[5].kMacros[7].Definition = nullptr;
	kPS[5].kMacros[8].Name = nullptr;
	kPS[5].kMacros[8].Definition = nullptr;
	kPS[6].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
	kPS[6].kMacros[0].Name = "PS_2_0";
	kPS[6].kMacros[0].Definition = nullptr;
	kPS[6].kMacros[1].Name = "SHADOWMAP";
	kPS[6].kMacros[1].Definition = "";
	kPS[6].kMacros[2].Name = "DEPTHBIAS";
	kPS[6].kMacros[2].Definition = "-0.7";
	kPS[6].kMacros[3].Name = "SAMPLE";
	kPS[6].kMacros[3].Definition = "4";
	kPS[6].kMacros[4].Name = "PASSES";
	kPS[6].kMacros[4].Definition = "2";
	kPS[6].kMacros[5].Name = nullptr;
	kPS[6].kMacros[5].Definition = nullptr;
	kPS[6].kMacros[6].Name = nullptr;
	kPS[6].kMacros[6].Definition = nullptr;
	kPS[6].kMacros[7].Name = nullptr;
	kPS[6].kMacros[7].Definition = nullptr;
	kPS[6].kMacros[8].Name = nullptr;
	kPS[6].kMacros[8].Definition = nullptr;
	const char* pcPSTarget = BSShaderManager::GetPSTarget(true);
	if (BSShaderManager::eShaderLevel == 1) {
		pTLSData->eMemContext = eMemContext;
		return;
	}
	char acPath[260];
	char acCacheName[260];
	if (BSShaderManager::b30TreeShaders && BSShaderManager::b30Shaders) {
		// Both vertex shader models: b30GrassVS is flipped before each pass over the table.
		int32_t iModels = 2;
		do {
			BSShaderManager::b30GrassVS = !BSShaderManager::b30GrassVS;
			for (int32_t i = 0; i < 24; ++i) {
				BSShader::MakePath(kVS[i].strPath, acPath);
				BSsprintf(acCacheName, 260, "GRASS2%03i.vso", i);
				const char* pcVSTarget = ShaderTarget_vs_2_0;
				if (BSShaderManager::b30Shaders && BSShaderManager::b30GrassVS) {
					BSsprintf(acCacheName, 260, "GRASS23x%03i.vso", i);
					pcVSTarget = "vs_3_0";
				}
				spVertexShaders[i] = CompileVertexShaderHLSL(acPath, kVS[i].kMacros, pcVSTarget, acCacheName);
			}
		} while (--iModels);
	} else {
		for (int32_t i = 0; i < 24; ++i) {
			BSShader::MakePath(kVS[i].strPath, acPath);
			BSsprintf(acCacheName, 260, "GRASS2%03i.vso", i);
			const char* pcVSTarget = ShaderTarget_vs_2_0;
			if (BSShaderManager::b30Shaders && BSShaderManager::b30GrassVS) {
				BSsprintf(acCacheName, 260, "GRASS23x%03i.vso", i);
				pcVSTarget = "vs_3_0";
			}
			spVertexShaders[i] = CompileVertexShaderHLSL(acPath, kVS[i].kMacros, pcVSTarget, acCacheName);
		}
	}
	for (int32_t i = 0; i < 7; ++i) {
		if (i < 4 || BSShaderManager::GetShaderLevel(false) >= BSShaderManager::BSSM_SV_2_A) {
			if (BSShaderManager::b30TreeShaders) {
				// Four passes: with and without transparency multisampling, each for both shader models.
				BSShaderManager::bTransparencyMultisampling = true;
				for (int32_t j = 0; j < 4; ++j) {
					if (j % 2 == 0) {
						BSShaderManager::bTransparencyMultisampling = !BSShaderManager::bTransparencyMultisampling;
					}
					BSShaderManager::b30GrassVS = !BSShaderManager::b30GrassVS;
					if (BSShaderManager::bTransparencyMultisampling) {
						D3DXMACRO* pMacro = kPS[i].kMacros;
						while (pMacro->Name) {
							++pMacro;
						}
						pMacro->Name = "TRANSPARENCYMS";
						pMacro->Definition = "";
						BSsprintf(acCacheName, 260, "GRASS2%03iTMS.pso", i);
					} else {
						BSsprintf(acCacheName, 260, "GRASS2%03i.pso", i);
					}
					BSShader::MakePath(kPS[i].strPath, acPath);
					if (BSShaderManager::b30Shaders && BSShaderManager::b30GrassVS) {
						if (BSShaderManager::bTransparencyMultisampling) {
							BSsprintf(acCacheName, 260, "GRASS23x%03iTMS.pso", i);
						} else {
							BSsprintf(acCacheName, 260, "GRASS23x%03i.pso", i);
						}
						pcPSTarget = "ps_3_0";
					} else {
						pcPSTarget = BSShaderManager::GetPSTarget(true);
					}
					spPixelShaders[i] = CompilePixelShaderHLSL(acPath, kPS[i].kMacros, pcPSTarget, acCacheName);
				}
			} else {
				if (BSShaderManager::bTransparencyMultisampling) {
					D3DXMACRO* pMacro = kPS[i].kMacros;
					while (pMacro->Name) {
						++pMacro;
					}
					pMacro->Name = "TRANSPARENCYMS";
					pMacro->Definition = "";
					if (BSShaderManager::b30Shaders && BSShaderManager::b30GrassVS) {
						BSsprintf(acCacheName, 260, "GRASS23x%03iTMS.pso", i);
					} else {
						BSsprintf(acCacheName, 260, "GRASS2%03iTMS.pso", i);
					}
				} else if (BSShaderManager::b30Shaders && BSShaderManager::b30GrassVS) {
					BSsprintf(acCacheName, 260, "GRASS23x%03i.pso", i);
				} else {
					BSsprintf(acCacheName, 260, "GRASS2%03i.pso", i);
				}
				if (BSShaderManager::b30Shaders && BSShaderManager::b30GrassVS) {
					pcPSTarget = "ps_3_0";
				}
				BSShader::MakePath(kPS[i].strPath, acPath);
				spPixelShaders[i] = CompilePixelShaderHLSL(acPath, kPS[i].kMacros, pcPSTarget, acCacheName);
			}
		}
	}
	pTLSData->eMemContext = eMemContext;
}

// Always true (the exe's `return 1`).
// The exe's cleanup-only C++ EH frame is left out.
bool TallGrassShader::LoadStagesAndPasses() {
	TLSData* v2 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_GRASS;
	NiPointer<NiD3DPass>* spPass = this->TallGrassShader::spPass;
	int32_t v4 = 44;
	do {
		NiD3DPass::CreateNewPass(*spPass++);
		--v4;
	} while (v4);
	PresetStages();
	v2->eMemContext = eMemContext;
	return 1;
}

// Gives each pass its stages, render states and shaders under the grass heap context: passes 0-19 (stage 0, and
// stages 1-2 with pixel shader 2 for passes 8-15), passes 20-39 (stage 0 and the attenuation stage 1, stages 2-3
// with pixel shader 3 for passes 28-35; vertex shaders 0-19 again) and the four shadow passes 40-43 (vertex shaders
// 20-23, pixel shader 4 + iShadowFilter). The temporary stage pointer is released before the context is restored.
// The exe's cleanup-only C++ EH frame is left out.
void TallGrassShader::PresetStages() {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_GRASS;
	{
		NiPointer<NiD3DTextureStage> spStage;
		for (int32_t i = 0; i <= 19; ++i) {
			int32_t iPixelShader;
			NiD3DPass* pPass = spPass[i].m_pObject;
			if (!pPass->m_uiStageCount) {
				NiD3DTextureStage::CreateNewStage(spStage);
				NiD3DTextureStage* pStage = spStage.m_pObject;
				BSShader::SetupDefaultStage(pStage, 0, 3u, 2u, 0);
				pPass->SetStage(pPass->m_uiCurrentStage, pStage);
			}
			if (i < 8 || i > 15) {
				iPixelShader = 0;
			} else {
				if (pPass->m_uiStageCount < 3) {
					NiD3DTextureStage::CreateNewStage(spStage);
					NiD3DTextureStage* pStage = spStage.m_pObject;
					BSShader::SetupDefaultStage(pStage, 1u, 1u, 2u, 0);
					pPass->SetStage(pPass->m_uiCurrentStage, pStage);
					NiD3DTextureStage::CreateNewStage(spStage);
					pStage = spStage.m_pObject;
					BSShader::SetupDefaultStage(pStage, 2u, 1u, 2u, 0);
					pPass->SetStage(pPass->m_uiCurrentStage, pStage);
				}
				iPixelShader = 2;
			}
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
			if (BSShaderManager::GetShaderLevel(0) >= BSShaderManager::BSSM_SV_2_0) {
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(24u, 10u, 1);
				if (BSShaderManager::bHDR && !BSShaderManager::bTransparencyMultisampling) {
					if (!pPass->m_pkRenderStateGroup) {
						pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
					}
					pPass->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
					if (!pPass->m_pkRenderStateGroup) {
						pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
					}
					pPass->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
					if (!pPass->m_pkRenderStateGroup) {
						pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
					}
					pPass->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
				}
			}
			pPass->m_spVertexShader = spVertexShaders[i].m_pObject;
			pPass->m_spPixelShader = spPixelShaders[iPixelShader].m_pObject;
		}
		for (int32_t i = 20; i <= 39; ++i) {
			int32_t iPixelShader;
			NiD3DPass* pPass = spPass[i].m_pObject;
			if (pPass->m_uiStageCount < 2) {
				NiD3DTextureStage::CreateNewStage(spStage);
				NiD3DTextureStage* pStage = spStage.m_pObject;
				BSShader::SetupDefaultStage(pStage, 0, 3u, 2u, 0);
				pPass->SetStage(pPass->m_uiCurrentStage, pStage);
				NiD3DTextureStage::CreateNewStage(spStage);
				pStage = spStage.m_pObject;
				BSShader::SetupDefaultStage(pStage, 1u, 3u, 2u, 0);
				pStage->m_spTexture = BSShaderManager::pAttenuationXY;
				pStage->m_uiTextureFlags = 1;
				pPass->SetStage(pPass->m_uiCurrentStage, pStage);
			}
			if (i < 28 || i > 35) {
				iPixelShader = 1;
			} else {
				if (pPass->m_uiStageCount < 4) {
					NiD3DTextureStage::CreateNewStage(spStage);
					NiD3DTextureStage* pStage = spStage.m_pObject;
					BSShader::SetupDefaultStage(pStage, 2u, 1u, 2u, 0);
					pPass->SetStage(pPass->m_uiCurrentStage, pStage);
					NiD3DTextureStage::CreateNewStage(spStage);
					pStage = spStage.m_pObject;
					BSShader::SetupDefaultStage(pStage, 3u, 1u, 2u, 0);
					pPass->SetStage(pPass->m_uiCurrentStage, pStage);
				}
				iPixelShader = 3;
			}
			if (BSShaderManager::GetShaderLevel(0) < BSShaderManager::BSSM_SV_2_0) {
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
			} else {
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
				if (!pPass->m_pkRenderStateGroup) {
					pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
				}
				pPass->m_pkRenderStateGroup->SetRenderState(24u, 10u, 1);
				if (BSShaderManager::bHDR && !BSShaderManager::bTransparencyMultisampling) {
					if (!pPass->m_pkRenderStateGroup) {
						pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
					}
					pPass->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
					if (!pPass->m_pkRenderStateGroup) {
						pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
					}
					pPass->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
					if (!pPass->m_pkRenderStateGroup) {
						pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
					}
					pPass->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
				}
			}
			pPass->m_spVertexShader = spVertexShaders[i - 20].m_pObject;
			pPass->m_spPixelShader = spPixelShaders[iPixelShader].m_pObject;
		}
		for (int32_t i = 40; i <= 43; ++i) {
			NiD3DPass* pPass = spPass[i].m_pObject;
			if (pPass->m_uiStageCount < 2) {
				NiD3DTextureStage::CreateNewStage(spStage);
				NiD3DTextureStage* pStage = spStage.m_pObject;
				BSShader::SetupDefaultStage(pStage, 0, 3u, 2u, 0);
				pPass->SetStage(pPass->m_uiCurrentStage, pStage);
				NiD3DTextureStage::CreateNewStage(spStage);
				pStage = spStage.m_pObject;
				BSShader::SetupDefaultStage(pStage, 1u, 3u, 2u, 0);
				pPass->SetStage(pPass->m_uiCurrentStage, pStage);
			}
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(19u, 9u, 1);
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(20u, 1u, 1);
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
			// The depth bias is passed as the float's bits; the exe reads it before the group's null test.
			uint32_t uiDepthBias = std::bit_cast<uint32_t>(BSShaderProperty::fDepthBiasUnit);
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(195u, uiDepthBias, 1);
			if (!pPass->m_pkRenderStateGroup) {
				pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
			}
			pPass->m_pkRenderStateGroup->SetRenderState(152u, 63u, 1);
			pPass->m_spVertexShader = spVertexShaders[i - 20].m_pObject;
			pPass->m_spPixelShader = spPixelShaders[BSShaderManager::iShadowFilter + 4].m_pObject;
		}
	}
	pTLS->eMemContext = eMemContext;
}

void TallGrassShader::UpdatePointLights(TallGrassShaderProperty* apTGSProp, D3DXMATRIX& d3dInvWorld, float afWorldScale,
	BSShaderManager::eBSSMPassEnum apPass) {
	_D3DVECTOR vLightPos;
	_D3DVECTOR vLightObjectPos;
	void* kiter = 0;
	NiLight* m_pObject = apTGSProp->GetFirstActiveNonShadowLight(kiter)->spLight.m_pObject;
	float g = m_pObject->m_kDiff.g;
	float b = m_pObject->m_kDiff.b;
	float red = m_pObject->m_kDiff.r;
	float v18 = red;
	float v14 = g;
	float v15 = b;
	pfPointLightColor.x = red;
	float alpha = 1.0f;
	pfPointLightColor.y = g;
	pfPointLightColor.z = b;
	pfPointLightColor.w = alpha;
	float y = m_pObject->m_kWorld.m_Translate.y;
	float z = m_pObject->m_kWorld.m_Translate.z;
	red = m_pObject->m_kWorld.m_Translate.x;
	vLightPos.x = red;
	v14 = y;
	vLightPos.y = y;
	v15 = z;
	vLightPos.z = z;
	D3DXVec3TransformCoord_Thunk(&vLightObjectPos, &vLightPos, static_cast<_D3DMATRIX*>(&d3dInvWorld));
	float radius = m_pObject->m_kSpec.r;
	if (!BSShaderAccumulator::IsShadowPass(apPass)) {
		radius = radius / afWorldScale;
	}
	TallGrassShader::pGroupedVertexConstants.kPointLighting.x = vLightObjectPos.x;
	TallGrassShader::pGroupedVertexConstants.kPointLighting.y = vLightObjectPos.y;
	TallGrassShader::pGroupedVertexConstants.kPointLighting.z = vLightObjectPos.z;
	TallGrassShader::pGroupedVertexConstants.kPointLighting.w = radius;
}

// The exe copies the matrix entries and the light's color and position through x87 float loads and stores
// (fld/fstp dword); src copies the bits, which differs only for a signalling NaN. The exe's IDA name is
// SetupShadowData.
void TallGrassShader::UpdateShadowLights(const NiSkinInstance* apSkin, const NiTransform& aWorld) {
	alignas(16) D3DXMATRIX kD3D;
	kD3D._12 = 0.0f;
	kD3D._13 = 0.0f;
	kD3D._14 = 0.0f;
	kD3D._21 = 0.0f;
	kD3D._23 = 0.0f;
	kD3D._24 = 0.0f;
	kD3D._31 = 0.0f;
	kD3D._32 = 0.0f;
	kD3D._34 = 0.0f;
	kD3D._41 = 0.0f;
	kD3D._42 = 0.0f;
	kD3D._43 = 0.0f;
	kD3D._11 = 1.0f;
	kD3D._22 = 1.0f;
	kD3D._33 = 1.0f;
	kD3D._44 = 1.0f;
	if (!apSkin) {
		NiD3DUtility::GetD3DFromNi(kD3D, aWorld.m_Rotate, aWorld.m_Translate, aWorld.m_fScale);
	}
	kD3D._41 = NiRenderer::PosAdjust.x + kD3D._41;
	kD3D._42 = NiRenderer::PosAdjust.y + kD3D._42;
	kD3D._43 = NiRenderer::PosAdjust.z + kD3D._43;
	ShadowSceneLight* pLight = *BSShaderManager::pCurrentRenderPass->ppSceneLights;
	alignas(16) D3DXMATRIX kViewProj = pLight->kViewProjection;
	alignas(16) D3DXMATRIX kProduct;
	D3DXMatrixMultiply_Thunk(&kProduct, &kD3D, &kViewProj);
	alignas(16) D3DXMATRIX kShadow = kProduct;
	D3DXMatrixTranspose_Thunk(&kShadow, &kShadow);
	pGroupedVertexConstants.kModelViewProj.x = kShadow._11;
	pGroupedVertexConstants.kModelViewProj.y = kShadow._12;
	pGroupedVertexConstants.kModelViewProj.z = kShadow._13;
	pGroupedVertexConstants.kModelViewProj.w = kShadow._14;
	pGroupedVertexConstants.kDiffuseColor.r = kShadow._21;
	pGroupedVertexConstants.kDiffuseColor.g = kShadow._22;
	pGroupedVertexConstants.kDiffuseColor.b = kShadow._23;
	pGroupedVertexConstants.kDiffuseColor.a = kShadow._24;
	pGroupedVertexConstants.kPointLighting.x = kShadow._31;
	pGroupedVertexConstants.kPointLighting.y = kShadow._32;
	pGroupedVertexConstants.kPointLighting.z = kShadow._33;
	pGroupedVertexConstants.kPointLighting.w = kShadow._34;
	pGroupedVertexConstants.kAmbientColor.x = kShadow._41;
	pGroupedVertexConstants.kAmbientColor.y = kShadow._42;
	pGroupedVertexConstants.kAmbientColor.z = kShadow._43;
	pGroupedVertexConstants.kAmbientColor.w = kShadow._44;
	// The exe reloads the scene light from the render pass.
	NiLight* pNiLight = (*BSShaderManager::pCurrentRenderPass->ppSceneLights)->spLight.m_pObject;
	pfPointLightColor.x = pNiLight->m_kDiff.r;
	pfPointLightColor.y = pNiLight->m_kDiff.g;
	pfPointLightColor.z = pNiLight->m_kDiff.b;
	pfPointLightColor.w = 1.0f;
	pfPointLightData.x = pNiLight->m_kWorld.m_Translate.x;
	pfPointLightData.y = pNiLight->m_kWorld.m_Translate.y;
	pfPointLightData.z = pNiLight->m_kWorld.m_Translate.z;
	pfPointLightData.w = pNiLight->m_kSpec.r;
}

void TallGrassShader::SetupConstants(NiGeometry* apGeometry, TallGrassShaderProperty* apTgsProp) {
	uint16_t usnewActiveTris;
	TallGrassGroup* pGrassGroup = apTgsProp->pGrassGroup;
	int32_t iMaxInstanceCount = pGrassGroup->iMaxInstanceCount;
	rt::memcpy(pGrassInstanceData, pGrassGroup->pInstanceData,
		sizeof(TallGrassGroup::GrassInstanceData) * iMaxInstanceCount);
	uint16_t iInstanceCount = pGrassGroup->iInstanceCount;
	NiTriBasedGeomData* pgeomData =
		static_cast<NiTriBasedGeomData*>(pGrassGroup->spGrassGeometry.m_pObject->m_spModelData.m_pObject);
	if (iInstanceCount == iMaxInstanceCount) {
		usnewActiveTris = pgeomData->m_usTriangles;
	} else {
		usnewActiveTris = iInstanceCount * pGrassGroup->spCachedGeometry.m_pObject->usInstanceTriCount;
	}
	pgeomData->SetActiveTriangleCount(usnewActiveTris);
	NiShaderConstantMapEntry* pGrassInstanceDataEntry = this->pGrassInstanceDataEntry;
	if (pGrassInstanceDataEntry) {
		pGrassInstanceDataEntry->m_uiRegisterCount = pGrassGroup->iInstanceCount;
	}
}

void TallGrassShader::SetWindRanges(float afWindMagMin, float afWindMagMax) {
	TallGrassShader::fWindMagMin = afWindMagMin;
	TallGrassShader::fWindMagMax = afWindMagMax;
}

// The exe's cleanup-only C++ EH frame is left out.
BSShader* TallGrassShader::CreateShader() {
	TLSData* v0 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v0->eMemContext;
	v0->eMemContext = MEM_CONTEXT_GRASS;
	TallGrassShader* pshader = 0;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
	spShaderDecl->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDecl->SetEntry(0, 4u, 6u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
		NiShaderDeclaration::SPTYPE_FLOAT1, NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u,
		NiD3DShaderDeclaration::SPTESS_DEFAULT);
	spShaderDecl->GetD3DDeclaration();
	TallGrassShader* v4 = static_cast<TallGrassShader*>(NiMemObject::operator new(sizeof(TallGrassShader)));
	if (v4) {
		pshader = ::new (v4) TallGrassShader(spShaderDecl);
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->CreateConstantMaps();
	pshader->LoadStagesAndPasses();
	pshader->SetShaderDecl(spShaderDecl);
	v0->eMemContext = eMemContext;
	return pshader;
}
