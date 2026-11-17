#include "BSShader/ParticleShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <stdint.h>

NiPointer<NiTriShape> ParticleShader::spBaseGeometry;
NiRTTI ParticleShader::ms_RTTI("ParticleShader", &NiD3DDefaultShader::ms_RTTI);

ParticleShader::ParticleShader(NiDX9ShaderDeclaration* apShaderDecl)
	: ePass(BSShaderManager::BSSM_PARTICLE), pParticleData(nullptr), Velocity(0.0f, 0.0f, 0.0f),
	  Acceleration(0.0f, 0.0f, 0.0f), Color1(0.0f, 0.0f, 0.0f, 0.0f), Color2(0.0f, 0.0f, 0.0f, 0.0f),
	  Color3(0.0f, 0.0f, 0.0f, 0.0f) {
	iShaderType = 14;
	m_bUsesNiRenderState = 1;
	m_spShaderDecl = apShaderDecl;
	Wind.x = 0.0f;
	Wind.y = 0.0f;
	Wind.z = 0.0f;
	Wind.w = 0.0f;
	uint32_t uiInstances = GetMaxInstanceSize();
	pParticleData = static_cast<ParticleData*>(MemoryManager_Singleton.Allocate(sizeof(ParticleData) * uiInstances));
	std::memset(pParticleData, 0, sizeof(ParticleData) * uiInstances);
	std::memset(fVars, 0, sizeof(fVars));
}

ParticleShader::~ParticleShader() {
	spBaseGeometry = nullptr;
	MemoryManager_Singleton.Deallocate(pParticleData);
	pParticleData = nullptr;
	spParticlePass = nullptr;
	spParticlePrePass = nullptr;
	spParticleVertexShader = nullptr;
	spParticlePixelShader = nullptr;
}

const NiRTTI* ParticleShader::GetRTTI() const {
	return &ParticleShader::ms_RTTI;
}

bool ParticleShader::Initialize() {
	return (bool)((uint8_t)BSShader::Initialize());
}

void ParticleShader::SetupGeometryTextures(const NiPropertyState* apState) {
	ParticleShaderProperty* pProperty = static_cast<ParticleShaderProperty*>(apState->m_aspProps[3].m_pObject);
	if (pProperty && pProperty->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_PARTICLE) {
		NiD3DTextureStage* pstage = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
		pstage->m_spTexture = pProperty->spBaseTexture.m_pObject;
		pstage->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
		pstage->ConfigureStage(0);
	}
}

void ParticleShader::SetupGeometryConstants(const NiPropertyState* apState) {
	int32_t v5;
	NiPoint3 windPoint_2;
	NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
	m_pkD3DRenderer->SetModelTransform(BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld, 0);
	NiProperty* m_pObject = apState->m_aspProps[3].m_pObject;
	if (m_pObject) {
		v5 = static_cast<NiShadeProperty*>(m_pObject)->iShaderPropertyType ==
			BSShaderManager::BSSM_PROPERTYTYPE_PARTICLE;
	} else {
		v5 = 0;
	}
	ParticleShaderProperty* v6 = v5 ? static_cast<ParticleShaderProperty*>(m_pObject) : 0;
	if (v6) {
		double v14;
		NiPoint3 windPoint;
		fVars[0].x = v6->fTimer;
		fVars[0].y = v6->fLifetime;
		fVars[0].z = v6->fNormalSpeed;
		fVars[0].w = v6->fNormalAcceleration;
		Velocity.x = v6->Velocity.x;
		Velocity.y = v6->Velocity.y;
		Velocity.z = v6->Velocity.z;
		Acceleration.x = v6->Acceleration.x;
		Acceleration.y = v6->Acceleration.y;
		Acceleration.z = v6->Acceleration.z;
		bool v7 = !v6->bWorldspace;
		float y = v6->WindPoint.y;
		float z = v6->WindPoint.z;
		windPoint.x = v6->WindPoint.x;
		windPoint.y = y;
		windPoint.z = z;
		if (v7) {
			NiTransform geometryWorldInvert;
			pgeometry->m_kWorld.Invert(geometryWorldInvert);
			NiPoint3* v10 = ::new (&windPoint_2) NiPoint3(geometryWorldInvert * windPoint);
			float x = v10->x;
			float v12 = v10->y;
			float v13 = v10->z;
			windPoint.x = x;
			windPoint.y = v12;
			windPoint.z = v13;
		}
		Wind.x = windPoint.x;
		Wind.y = windPoint.y;
		Wind.z = windPoint.z;
		Wind.w = v6->fWindSpeed;
		Color1.r = v6->Color1.r;
		Color1.g = v6->Color1.g;
		Color1.b = v6->Color1.b;
		Color1.a = v6->Color1.a;
		Color2.r = v6->Color2.r;
		Color2.g = v6->Color2.g;
		Color2.b = v6->Color2.b;
		Color2.a = v6->Color2.a;
		Color3.r = v6->Color3.r;
		Color3.g = v6->Color3.g;
		Color3.b = v6->Color3.b;
		Color3.a = v6->Color3.a;
		fVars[1].x = v6->ColorTime1;
		fVars[1].y = v6->ColorTime2;
		fVars[1].z = v6->ColorTime3;
		fVars[2].x = v6->Scale1;
		fVars[2].y = v6->Scale2;
		fVars[2].z = v6->ScaleTime1;
		fVars[2].w = v6->ScaleTime2;
		if (v6->bWorldspace) {
			fVars[3].x = pgeometry->m_kWorld.m_Translate.x;
			fVars[3].y = pgeometry->m_kWorld.m_Translate.y;
			v14 = pgeometry->m_kWorld.m_Translate.z;
		} else {
			v14 = 0.0;
			fVars[3].x = 0.0f;
			fVars[3].y = 0.0f;
		}
		fVars[3].z = v14;
		fVars[3].w = v6->fFade;
		fVars[4].x = 1.0 / v6->fTextureCountX;
		fVars[4].y = 1.0 / v6->fTextureCountY;
		int32_t v15 = ParticleShader::uiMaxInstanceSize;
		if (!ParticleShader::uiMaxInstanceSize) {
			v15 = BSShaderManager::eShaderLevel < 2 ? 26 : 78;
			ParticleShader::uiMaxInstanceSize = v15;
		}
		rt::memcpy(pParticleData, v6->pInstanceData, sizeof(ParticleShader::ParticleData) * v15);
	}
	BSNiBound* m_kWorldBound = pgeometry->m_kWorldBound;
	if (!m_kWorldBound) {
		m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
	}
	NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
	NiD3DShaderConstantMap* v17 = m_spPixelConstantMap.m_pObject;
	const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
	if (v17) {
		NiD3DPixelShader* v22 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
		if (v22) {
			v17->SetShaderConstants(v22, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld,
				*m_kWorldBound, 0, 1);
		}
	}
	NiD3DShaderConstantMap* v19 = m_spVertexConstantMap.m_pObject;
	if (v19) {
		NiD3DVertexShader* v20 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
		if (v20) {
			v19->SetShaderConstants(v20, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld,
				*m_kWorldBound, 0, 1);
		}
	}
}

void ParticleShader::SetupGeometryAlphaBlending(const NiPropertyState* apState) {
	NiShadeProperty* pShadeProp = static_cast<NiShadeProperty*>(apState->m_aspProps[3].m_pObject);
	ParticleShaderProperty* pProp =
		pShadeProp && pShadeProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_PARTICLE
		? static_cast<ParticleShaderProperty*>(pShadeProp)
		: nullptr;
	BSRenderState::SetAlphaBlendEnable(1, BSRS_NOLOCK);
	if (pProp) {
		NiAlphaProperty::AlphaFunction eDest = NiAlphaProperty::GetNiAlphaFunctionFromD3D(pProp->eDestBlendMode);
		NiAlphaProperty::AlphaFunction eSrc = NiAlphaProperty::GetNiAlphaFunctionFromD3D(pProp->eSrcBlendMode);
		BSRenderState::SetAlphaBlendFunc(eSrc, eDest, BSRS_NOLOCK);
	} else {
		BSRenderState::SetAlphaBlendFunc(NiAlphaProperty::ALPHA_SRCALPHA, NiAlphaProperty::ALPHA_INVSRCALPHA,
			BSRS_NOLOCK);
	}
}

// The exe reads the z test of the particle property without a null test, through a stack table indexed by the
// D3DCMPFUNC value less one.
void ParticleShader::SetupGeometryRenderStates(const NiPropertyState* apState, bool abFirstPass) {
	NiShadeProperty* pShadeProp = static_cast<NiShadeProperty*>(apState->m_aspProps[3].m_pObject);
	ParticleShaderProperty* pProp =
		pShadeProp && pShadeProp->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_PARTICLE
		? static_cast<ParticleShaderProperty*>(pShadeProp)
		: nullptr;
	NiZBufferProperty::TestFunction aeTestFunction[8];
	aeTestFunction[0] = NiZBufferProperty::TEST_NEVER;
	aeTestFunction[1] = NiZBufferProperty::TEST_LESS;
	aeTestFunction[2] = NiZBufferProperty::TEST_EQUAL;
	aeTestFunction[3] = NiZBufferProperty::TEST_LESSEQUAL;
	aeTestFunction[4] = NiZBufferProperty::TEST_GREATER;
	aeTestFunction[5] = NiZBufferProperty::TEST_NOTEQUAL;
	aeTestFunction[6] = NiZBufferProperty::TEST_GREATEREQUAL;
	aeTestFunction[7] = NiZBufferProperty::TEST_ALWAYS;
	BSRenderState::SetZFunc(aeTestFunction[pProp->eZTestFunction - 1], BSRS_NOLOCK);
}

void ParticleShader::RecreateRendererData() {
	if (m_spShaderDecl.m_pObject) {
		spParticleVertexShader.m_pObject->RecreateRendererData();
		spParticlePixelShader.m_pObject->RecreateRendererData();
	}
	NiD3DShader::RecreateRendererData();
}

// Pass type 0x41 renders with the pre-pass.
void ParticleShader::InitPass(uint32_t auiCurrentPass) {
	if (auiCurrentPass == 0x41)
		NiD3DShader::m_spCurrentPass = spParticlePrePass.m_pObject;
	else
		NiD3DShader::m_spCurrentPass = spParticlePass.m_pObject;
}

void ParticleShader::SetupPassConstants(uint32_t auiCurrentPass) {
	UpdateFov();
}

// The exe's cleanup-only EH frame around the two `new`s is left out.
void ParticleShader::CreateConstantMaps() {
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
		// The exe passes its own empty string 0x1011584 as each entry's variable name.
		m_spVertexConstantMap.m_pObject->AddPredefinedEntry("WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr,
			false);
		m_spVertexConstantMap.m_pObject->AddEntry("Wind", 0x10000007, 0, 15, 1, "", 16, 4, &Wind, false);
		m_spVertexConstantMap.m_pObject->AddEntry("Color1", 0x10000007, 0, 9, 1, "", 16, 4, &Color1, false);
		m_spVertexConstantMap.m_pObject->AddEntry("Color2", 0x10000007, 0, 10, 1, "", 16, 4, &Color2, false);
		m_spVertexConstantMap.m_pObject->AddEntry("Color3", 0x10000007, 0, 11, 1, "", 16, 4, &Color3, false);
		m_spVertexConstantMap.m_pObject->AddEntry("Velocity", 0x10000007, 0, 12, 1, "", 12, 4, &Velocity, false);
		m_spVertexConstantMap.m_pObject->AddEntry("Acceleration", 0x10000007, 0, 13, 1, "", 12, 4, &Acceleration,
			false);
		m_spVertexConstantMap.m_pObject->AddEntry("Scale Adjustment", 0x10000007, 0, 14, 1, "", 16, 4, &ScaleAdjust,
			false);
		m_spVertexConstantMap.m_pObject->AddEntry("fVars", 0x10000009, 0, 4, 5, "", 0x50, 4, fVars, false);
		// EXE-INLINED 0xBA52D0
		uint32_t uiSize = uiMaxInstanceSize;
		if (!uiSize) {
			uiSize = BSShaderManager::eShaderLevel < 2 ? 26 : 78;
			uiMaxInstanceSize = uiSize;
		}
		m_spVertexConstantMap.m_pObject->AddEntry("particle data", 0x10000009, 0, 18, uiSize * 3, "", uiSize * 48, 4,
			pParticleData, false);
	}
	if (!m_spPixelConstantMap.m_pObject) {
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
	}
}

void ParticleShader::ReloadShaders() {
	LoadShaders();
	LoadStagesAndPasses();
}

// The exe calls ClearPass (slot 0x128) and then returns; it does not tail-jump.
[[clang::disable_tail_calls]] void ParticleShader::ClearPassStages() {
	ClearPass(spParticlePass.m_pObject);
	ClearPass(spParticlePrePass.m_pObject);
}

void ParticleShader::SetCurrentPass(uint32_t aPass, bool aFirstPass) {
	ePass = static_cast<BSShaderManager::eBSSMPassEnum>(aPass);
	bFirstPass = (bool)((uint8_t)aFirstPass);
}

void ParticleShader::LoadShaders() {
	_D3DXMACRO v9;
	char v10[64];
	_D3DXMACRO Size;
	char v13[64];
	char PSOFile[260];
	char Str1[260];
	const char* apFile = "particle\\v\\particle.v.hlsl";
	Size.Name = 0;
	Size.Definition = 0;
	std::memset(v13, 0, sizeof(v13));
	BSShader::MakePath("particle\\v\\particle.v.hlsl", Str1);
	BSsprintf(PSOFile, 260, "PARTICLE.vso");
	NiD3DVertexShader* v2 = CompileVertexShaderHLSL(Str1, &Size, ShaderTarget_vs_2_0, PSOFile);
	spParticleVertexShader = v2;
	const char* ptarget = BSShaderManager::GetPSTarget(1);
	v9.Name = 0;
	v9.Definition = 0;
	std::memset(v10, 0, sizeof(v10));
	BSShader::MakePath("particle\\p\\particle.p.hlsl", Str1);
	BSsprintf(PSOFile, 260, "PARTICLE.pso");
	NiD3DPixelShader* v6 = CompilePixelShaderHLSL(Str1, &v9, ptarget, PSOFile);
	spParticlePixelShader = v6;
}

bool ParticleShader::LoadStagesAndPasses() {
	if (!spParticlePass.m_pObject)
		NiD3DPass::CreateNewPass(spParticlePass);
	if (!spParticlePrePass.m_pObject)
		NiD3DPass::CreateNewPass(spParticlePrePass);
	PresetStages();
	return true;
}

// The exe's cleanup-only C++ EH frame is left out.
void ParticleShader::PresetStages() {
	NiD3DPass* v2 = 0;
	NiD3DPass* v15 = 0;
	{
		NiPointer<NiD3DTextureStage> a1;
		NiD3DPass* m_pObject = spParticlePass.m_pObject;
		if (m_pObject) {
			v2 = m_pObject;
			NiIncRefCount(m_pObject);
			v15 = m_pObject;
		}
		if (!v2->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v2->SetStage(v2->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v4 = spParticleVertexShader.m_pObject;
		v2->m_spVertexShader = v4;
		NiD3DPixelShader* v6 = spParticlePixelShader.m_pObject;
		v2->m_spPixelShader = v6;
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(52u, 0, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
		if (v2 != spParticlePrePass.m_pObject) {
			v2->DecRefCount();
			v2 = spParticlePrePass.m_pObject;
			v15 = v2;
			if (v2) {
				NiIncRefCount(v2);
			}
		}
		if (!v2->m_uiStageCount) {
			NiD3DTextureStage::CreateNewStage(a1);
			BSShader::SetupDefaultStage(a1.m_pObject, 0, 1u, 2u, 1);
			v2->SetStage(v2->m_uiCurrentStage, a1.m_pObject);
		}
		NiD3DVertexShader* v9 = spParticleVertexShader.m_pObject;
		v2->m_spVertexShader = v9;
		NiD3DPixelShader* v11 = spParticlePixelShader.m_pObject;
		v2->m_spPixelShader = v11;
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(20u, 6u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(52u, 0, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
		if (!v2->m_pkRenderStateGroup) {
			v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v2->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
	}
	v2->DecRefCount();
}

uint32_t ParticleShader::GetMaxInstanceSize() {
	uint32_t result = ParticleShader::uiMaxInstanceSize;
	if (!ParticleShader::uiMaxInstanceSize) {
		result = BSShaderManager::eShaderLevel < 2 ? 26 : 78;
		ParticleShader::uiMaxInstanceSize = result;
	}
	return result;
}

ParticleShaderProperty* ParticleShader::Start(NiAVObject* apRoot, NiAVObject* apObject) {
	NiUpdateData UpdateData;
	if (!apRoot) {
		return 0;
	}
	NiNode* pnode = apRoot->IsNode();
	if (!pnode) {
		return 0;
	}
	NiGeometry* pparticleGeometry = ParticleShader::GetParticleGeometry();
	NiGeometry* pclone = static_cast<NiGeometry*>(pparticleGeometry->Clone());
	NiGeometryData* m_pObject = pclone->m_spModelData.m_pObject;
	float x = m_pObject->m_kBound.m_kCenter.x;
	float y = m_pObject->m_kBound.m_kCenter.y;
	float z = m_pObject->m_kBound.m_kCenter.z;
	float m_fRadius = m_pObject->m_kBound.m_fRadius;
	BSNiBound* m_kWorldBound = apObject->m_kWorldBound;
	if (!m_kWorldBound) {
		m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
	}
	float apRoota = static_cast<NiBound&>(*m_kWorldBound).m_fRadius + static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
	double v9 = m_fRadius;
	if (!(apRoota < static_cast<double>(m_fRadius))) {
		v9 = apRoota;
	}
	m_pObject->m_kBound.m_kCenter.x = x;
	float apRootb = v9;
	m_pObject->m_kBound.m_kCenter.y = y;
	m_pObject->m_kBound.m_fRadius = apRootb;
	m_pObject->m_kBound.m_kCenter.z = z;
	pnode->AttachChild(pclone, 1);
	UpdateData.fTime = 0.0f;
	UpdateData.bUpdateControllers = 0;
	UpdateData.bParallelUpdate = 0;
	UpdateData.bFoundParticles = 0;
	UpdateData.bFoundMorphController = 0;
	UpdateData.bSceneGraphChange = 0;
	pnode->Update(UpdateData);
	BSShaderManager::PrepareObject(pclone, 0, 0);
	NiShadeProperty* pprop = static_cast<NiShadeProperty*>(pclone->GetProperty(3));
	int32_t v12 = 0;
	if (pprop) {
		v12 = static_cast<int32_t>((static_cast<uint32_t>(v12) & 0xFFFFFF00u) |
			(static_cast<uint32_t>(static_cast<uint8_t>((pprop->iShaderPropertyType == 17)))));
	}
	ParticleShaderProperty* ppsProp = v12 != 0 ? static_cast<ParticleShaderProperty*>(pprop) : 0;
	if (ppsProp) {
		ppsProp->SetupGeometry(pclone);
		ParticleShader::GetParticleGeometry();
		++ParticleShader::iNumParticles;
	}
	return ppsProp;
}

void ParticleShader::Stop(NiAVObject* apObject, ParticleShaderProperty* apProp) {
	NiNode* v2;
	uint32_t i;
	NiAVObject* m_pObject;
	NiProperty* property;
	int32_t v6;
	ParticleShaderProperty* v7;
	NiTriShape* v8;
	if (apObject) {
		v2 = apObject->IsNode();
		if (v2) {
			if (apProp) {
				for (i = 0; i < ((uint32_t)((int32_t)v2->m_kChildren.m_usSize)); ++i) {
					m_pObject = v2->m_kChildren.m_pBase[i].m_pObject;
					if (m_pObject) {
						property = m_pObject->GetProperty(3);
						if (property) {
							v6 = (int32_t)(property[1].m_uiRefCount == 17);
						} else {
							v6 = 0;
						}
					} else {
						property = nullptr;
						v6 = 0;
					}
					v7 = v6 ? static_cast<ParticleShaderProperty*>(property) : (nullptr);
					if (v7) {
						if (v7 == apProp) {
							v2->DetachChildAt(i);
						}
					}
				}
				if ((--ParticleShader::iNumParticles) <= 0) {
					if (ParticleShader::spBaseGeometry.m_pObject) {
						v8 = ParticleShader::spBaseGeometry.m_pObject;
						if (!InterlockedDecrementUInt32(&ParticleShader::spBaseGeometry.m_pObject->m_uiRefCount)) {
							v8->DeleteThis();
						}
						ParticleShader::spBaseGeometry.m_pObject = nullptr;
					}
					ParticleShader::iNumParticles = 0;
				}
			}
		}
	}
}

// The exe inlines the pointer release: it calls InterlockedDecrement and clears the static only when it was set.
void ParticleShader::Free() {
	NiTriShape* pGeometry = spBaseGeometry.m_pObject;
	if (pGeometry) {
		NiRefObject* pObject = pGeometry;
		pObject->DecRefCount();
		spBaseGeometry.m_pObject = nullptr;
	}
}

// The width goes through a float; the height is divided in at full precision. ScaleAdjust.y uses the unrounded
// reciprocal the exe keeps in ST0 after storing ScaleAdjust.x.
void ParticleShader::UpdateFov() {
	float fWidth = static_cast<float>(NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup()->GetWidth(0));
	float fAspect =
		fWidth / static_cast<double>(NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup()->GetHeight(0));
	double dInvFov = 1.0f / ParticleShader::fFOVAdjust;
	ParticleShader::ScaleAdjust.x = dInvFov;
	ParticleShader::ScaleAdjust.y = dInvFov * fAspect;
	ParticleShader::ScaleAdjust.z = 1.0f;
	ParticleShader::ScaleAdjust.w = 1.0f;
}

// The exe's cleanup-only C++ EH frame is left out.
NiGeometry* ParticleShader::GetParticleGeometry() {
	if (!BSShaderManager::eShaderLevel) {
		return 0;
	}
	NiGeometry* result = ParticleShader::spBaseGeometry.m_pObject;
	if (!ParticleShader::spBaseGeometry.m_pObject) {
		NiAdditionalGeometryData* pdata;
		NiTriShape* pnewGeometry;
		ParticleShaderProperty* pprop;
		int16_t v1 = ParticleShader::uiMaxInstanceSize;
		if (!ParticleShader::uiMaxInstanceSize) {
			v1 = BSShaderManager::eShaderLevel < 2 ? 26 : 78;
			ParticleShader::uiMaxInstanceSize = BSShaderManager::eShaderLevel < 2 ? 26 : 78;
		}
		uint16_t v2 = v1;
		uint16_t usTriangles = 2 * v1;
		uint16_t inumVertices_2 = 4 * v1;
		uint16_t inumVertices = 4 * v1;
		uint16_t* pusTriList =
			static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(3 * static_cast<uint32_t>(usTriangles) <=
						SIZE_MAX / sizeof(uint16_t)
					? sizeof(uint16_t) * (3 * static_cast<uint32_t>(usTriangles))
					: SIZE_MAX));
		NiPoint3* pvertex =
			static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(inumVertices_2 <= SIZE_MAX / sizeof(NiPoint3)
					? sizeof(NiPoint3) * inumVertices_2
					: SIZE_MAX));
		NiPoint2* ptex =
			static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(inumVertices_2 <= SIZE_MAX / sizeof(NiPoint2)
					? sizeof(NiPoint2) * inumVertices_2
					: SIZE_MAX));
		if (v2) {
			int32_t v6 = 0;
			uint16_t* v7 = pusTriList + 2;
			int32_t v25 = v2;
			do {
				int32_t v8 = static_cast<uint16_t>(v6);
				pvertex[v8].x = -1.0f;
				pvertex[v8].y = -1.0f;
				pvertex[v8].z = 0.0f;
				pvertex[v8 + 1].x = 1.0f;
				pvertex[v8 + 1].y = -1.0f;
				pvertex[static_cast<uint16_t>(v6) + 1].z = 0.0f;
				v8 += 2;
				pvertex[v8].x = 1.0f;
				pvertex[v8].y = 1.0f;
				pvertex[v8].z = 0.0f;
				int32_t v9 = static_cast<uint16_t>(v6) + 3;
				pvertex[v9].x = -1.0f;
				pvertex[v9].y = 1.0f;
				pvertex[v9].z = 0.0f;
				ptex[static_cast<uint16_t>(v6)].x = 0.0f;
				ptex[static_cast<uint16_t>(v6)].y = 0.0f;
				ptex[static_cast<uint16_t>(v6) + 1].x = 1.0f;
				ptex[static_cast<uint16_t>(v6) + 1].y = 0.0f;
				ptex[static_cast<uint16_t>(v6) + 2].x = 1.0f;
				ptex[static_cast<uint16_t>(v6) + 2].y = 1.0f;
				ptex[static_cast<uint16_t>(v6) + 3].x = 0.0f;
				ptex[static_cast<uint16_t>(v6) + 3].y = 1.0f;
				*(v7 - 2) = v6;
				v7[2] = v6;
				*(v7 - 1) = v6 + 1;
				*v7 = v6 + 2;
				v7[1] = v6 + 3;
				v7[3] = v6 + 2;
				v6 += 4;
				v7 += 6;
				--v25;
			} while (v25);
			inumVertices_2 = inumVertices;
		}
		NiTriShapeData* v10 = static_cast<NiTriShapeData*>(NiMemObject::operator new(sizeof(NiTriShapeData)));
		NiTriShapeData* pnewShapeData = 0;
		if (v10) {
			pnewShapeData = ::new (v10) NiTriShapeData(inumVertices_2, pvertex, nullptr, nullptr, ptex, 1,
				NiGeometryData::NBT_METHOD_NONE, usTriangles, pusTriList);
		}
		NiAdditionalGeometryData* v12 =
			static_cast<NiAdditionalGeometryData*>(NiMemObject::operator new(sizeof(NiAdditionalGeometryData)));
		if (v12) {
			pdata = ::new (v12) NiAdditionalGeometryData(inumVertices_2, 1u);
		} else {
			pdata = 0;
		}
		pdata->SetDataBlockCount(1u);
		uint8_t* v14 = ParticleShaderProperty::GetInstanceIndex();
		pdata->SetDataBlock(0, v14, 4 * inumVertices_2, 1);
		pdata->SetDataStream(0, 0, 0, 1u, inumVertices_2, 4u, 4u);
		pnewShapeData->SetAdditionalGeometryData(pdata);
		NiTriShape* v15 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
		if (v15) {
			pnewGeometry = ::new (v15) NiTriShape(pnewShapeData);
		} else {
			pnewGeometry = 0;
		}
		NiGeometryData* m_pObject = pnewGeometry->m_spModelData.m_pObject;
		float x = m_pObject->m_kBound.m_kCenter.x;
		int32_t m_iRadiusAsInt = m_pObject->m_kBound.m_iRadiusAsInt;
		float y = m_pObject->m_kBound.m_kCenter.y;
		float z = m_pObject->m_kBound.m_kCenter.z;
		m_pObject->m_kBound.m_fRadius = 512.0f;
		m_pObject->m_kBound.m_kCenter.x = x;
		m_pObject->m_kBound.m_kCenter.y = y;
		m_pObject->m_kBound.m_kCenter.z = z;
		ParticleShaderProperty* v22 =
			static_cast<ParticleShaderProperty*>(NiMemObject::operator new(sizeof(ParticleShaderProperty)));
		if (v22) {
			pprop = ::new (v22) ParticleShaderProperty;
		} else {
			pprop = 0;
		}
		pnewGeometry->AttachProperty(pprop);
		result = ParticleShader::spBaseGeometry.m_pObject;
		if (ParticleShader::spBaseGeometry.m_pObject != pnewGeometry) {
			if (ParticleShader::spBaseGeometry.m_pObject) {
				NiDecRefCount(ParticleShader::spBaseGeometry.m_pObject);
			}
			ParticleShader::spBaseGeometry.m_pObject = pnewGeometry;
			NiIncRefCount(pnewGeometry);
			return ParticleShader::spBaseGeometry.m_pObject;
		}
	}
	return result;
}

// The exe's cleanup-only C++ EH frame is left out.
BSShader* ParticleShader::CreateShader() {
	ParticleShader* pshader = 0;
	NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
	spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
	spShaderDecl->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2,
		0);
	spShaderDecl->SetEntry(0, 4u, 6u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
		NiShaderDeclaration::SPTYPE_FLOAT1, NiShaderDeclaration::SPUSAGE_COLOR, 1u,
		NiD3DShaderDeclaration::SPTESS_DEFAULT);
	spShaderDecl->GetD3DDeclaration();
	ParticleShader* v2 = static_cast<ParticleShader*>(NiMemObject::operator new(sizeof(ParticleShader)));
	if (v2) {
		pshader = ::new (v2) ParticleShader(spShaderDecl);
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(spShaderDecl);
	return pshader;
}

// The exe builds the table on the stack, indexed by the D3DBLEND value less one.
// KEEP: for 0 the exe reads the uninitialised stack slot below its table.
NiAlphaProperty::AlphaFunction NiAlphaProperty::GetNiAlphaFunctionFromD3D(uint32_t auiD3D) {
	NiAlphaProperty::AlphaFunction aeResult[11];
	aeResult[0] = NiAlphaProperty::ALPHA_ZERO;
	aeResult[1] = NiAlphaProperty::ALPHA_ONE;
	aeResult[2] = NiAlphaProperty::ALPHA_SRCCOLOR;
	aeResult[3] = NiAlphaProperty::ALPHA_INVSRCCOLOR;
	aeResult[4] = NiAlphaProperty::ALPHA_SRCALPHA;
	aeResult[5] = NiAlphaProperty::ALPHA_INVSRCALPHA;
	aeResult[6] = NiAlphaProperty::ALPHA_DESTALPHA;
	aeResult[7] = NiAlphaProperty::ALPHA_INVDESTALPHA;
	aeResult[8] = NiAlphaProperty::ALPHA_DESTCOLOR;
	aeResult[9] = NiAlphaProperty::ALPHA_INVDESTCOLOR;
	aeResult[10] = NiAlphaProperty::ALPHA_SRCALPHASAT;
	return aeResult[auiD3D - 1];
}
