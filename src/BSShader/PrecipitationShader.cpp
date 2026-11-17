#include "BSShader/PrecipitationShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

NiRTTI PrecipitationShader::ms_RTTI("PrecipitationShader", &NiD3DDefaultShader::ms_RTTI);

PrecipitationShader::PixelShaderArray PrecipitationShader::spPixelShader;
PrecipitationShader::VertexShaderArray PrecipitationShader::spVertexShader;
PrecipitationShader::PassArray PrecipitationShader::spPass;

PrecipitationShader::PrecipitationShader()
	: fParams(0.0f, 0.0f, 0.0f), spShaderDecl(nullptr), AmbientColor(NiColor_WHITE) {
	iShaderType = 18;
	m_bUsesNiRenderState = true;
}

PrecipitationShader::~PrecipitationShader() {
	for (int32_t i = 0; i < PRECIP_VSH_COUNT; ++i) {
		spVertexShader[i] = nullptr;
	}
	for (int32_t i = 0; i < PRECIP_PSH_COUNT; ++i) {
		spPixelShader[i] = nullptr;
	}
	spPass[PRECIP_PASS_RAIN] = nullptr;
}

const NiRTTI* PrecipitationShader::GetRTTI() const {
	return &PrecipitationShader::ms_RTTI;
}

bool PrecipitationShader::Initialize() {
	bool retval = BSShader::Initialize();
	for (int32_t i = 0; i < PRECIP_VSH_COUNT; ++i)
		PrecipitationShader::spVertexShader[i] = nullptr;
	for (int32_t i = 0; i < PRECIP_PSH_COUNT; ++i)
		PrecipitationShader::spPixelShader[i] = nullptr;
	NiD3DPass* pPass = PrecipitationShader::spPass[PRECIP_PASS_RAIN].m_pObject;
	if (pPass) {
		if (pPass->m_uiRefCount-- == 1) {
			pPass->ReturnPassToPool();
		}
		PrecipitationShader::spPass[PRECIP_PASS_RAIN].m_pObject = nullptr;
	}
	return retval;
}

void PrecipitationShader::CreateConstantMaps() {
	if (!m_spPixelConstantMap) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap = nullptr;
		if (pMemory)
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
		m_spPixelConstantMap = pMap;
		m_spPixelConstantMap->AddEntry("ambient color", 0x10000006, 0, 0, 1, "", 12, 4, &AmbientColor, false);
	}
	if (!m_spVertexConstantMap) {
		void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		NiD3DShaderConstantMap* pMap;
		if (pMemory)
			pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
		else
			pMap = nullptr;
		m_spVertexConstantMap = pMap;
		m_spVertexConstantMap->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false);
		m_spVertexConstantMap->AddEntry("eye pos", 0x10000006, 0, 8, 1, "", 12, 4, &pEyePosition, false);
		m_spVertexConstantMap->AddEntry("velocity", 0x10000006, 0, 9, 1, "", 12, 4, &kVelocity, false);
		m_spVertexConstantMap->AddEntry("min", 0x10000006, 0, 10, 1, "", 12, 4, &kMin, false);
		m_spVertexConstantMap->AddEntry("max", 0x10000006, 0, 11, 1, "", 12, 4, &kMax, false);
		m_spVertexConstantMap->AddEntry("params", 0x10000006, 0, 12, 1, "", 12, 4, &fParams, false);
		m_spVertexConstantMap->AddEntry("cam up", 0x10000006, 0, 13, 1, "", 12, 4, &pCameraUp, false);
	}
}

void PrecipitationShader::ReloadShaders() {
	LoadShaders();
	LoadStagesAndPasses();
}

[[clang::disable_tail_calls]] void PrecipitationShader::ClearPassStages() {
	ClearPass(spPass[PRECIP_PASS_RAIN].m_pObject);
}

void PrecipitationShader::LoadShaders() {
	LoadVertexShaders();
	LoadPixelShaders();
}

// The four macro sets are one array the loop walks; each set's tail is cleared from the first Definition after the
// named macros.
void PrecipitationShader::LoadVertexShaders() {
	BSShaderMacro aMacros[PRECIP_VSH_COUNT];
	aMacros[0].strPath = "precipitation\\precipitation.v.hlsl";
	aMacros[0].kMacros[0].Name = "BILLBOARD_UP";
	aMacros[0].kMacros[0].Definition = nullptr;
	aMacros[0].kMacros[1].Name = nullptr;
	std::memset(&aMacros[0].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	aMacros[1].strPath = "precipitation\\precipitation.v.hlsl";
	aMacros[1].kMacros[0].Name = "BILLBOARD_FACE";
	aMacros[1].kMacros[0].Definition = nullptr;
	aMacros[1].kMacros[1].Name = nullptr;
	std::memset(&aMacros[1].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	aMacros[2].kMacros[0].Name = "BILLBOARD_UP";
	aMacros[2].strPath = "precipitation\\precipitation.v.hlsl";
	aMacros[2].kMacros[0].Definition = nullptr;
	aMacros[2].kMacros[1].Name = "SNOW";
	aMacros[2].kMacros[1].Definition = nullptr;
	aMacros[2].kMacros[2].Name = nullptr;
	std::memset(&aMacros[2].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	aMacros[3].strPath = "precipitation\\precipitation.v.hlsl";
	aMacros[3].kMacros[0].Name = "BILLBOARD_FACE";
	aMacros[3].kMacros[0].Definition = nullptr;
	aMacros[3].kMacros[1].Name = "SNOW";
	aMacros[3].kMacros[1].Definition = nullptr;
	aMacros[3].kMacros[2].Name = nullptr;
	std::memset(&aMacros[3].kMacros[2].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition));
	int32_t i = 0;
	do {
		char compiledShaderName[260];
		char Str1[260];
		BSShader::MakePath(aMacros[i].strPath, Str1);
		BSsprintf(compiledShaderName, 260, "PRECIP%03i.vso", i);
		const char* VSTarget = BSShaderManager::GetVSTarget();
		NiD3DVertexShader* pShader = CompileVertexShaderHLSL(Str1, aMacros[i].kMacros, VSTarget, compiledShaderName);
		PrecipitationShader::spVertexShader[i] = pShader;
		++i;
	} while (i < PRECIP_VSH_COUNT);
}

void PrecipitationShader::LoadPixelShaders() {
	BSShaderMacro aMacros[PRECIP_PSH_COUNT];
	aMacros[0].strPath = "precipitation\\precipitation.p.hlsl";
	aMacros[0].kMacros[0].Name = nullptr;
	std::memset(&aMacros[0].kMacros[0].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
	aMacros[1].strPath = "precipitation\\precipitation.p.hlsl";
	aMacros[1].kMacros[0].Name = "SNOW";
	aMacros[1].kMacros[0].Definition = nullptr;
	aMacros[1].kMacros[1].Name = nullptr;
	std::memset(&aMacros[1].kMacros[1].Definition, 0,
		sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
	int32_t i = 0;
	do {
		char PSOFile[260];
		char HLSLFile[260];
		BSShader::MakePath(aMacros[i].strPath, HLSLFile);
		BSsprintf(PSOFile, 260, "PRECIP%03i.pso", i);
		const char* PSTarget = BSShaderManager::GetPSTarget(0);
		NiD3DPixelShader* pShader = CompilePixelShaderHLSL(HLSLFile, aMacros[i].kMacros, PSTarget, PSOFile);
		PrecipitationShader::spPixelShader[i] = pShader;
		++i;
	} while (i < PRECIP_PSH_COUNT);
}

bool PrecipitationShader::LoadStagesAndPasses() {
	NiPointer<NiD3DTextureStage> spStage;
	NiD3DPass::CreateNewPass(spPass[PRECIP_PASS_RAIN]);
	NiD3DTextureStage::CreateNewStage(spStage);
	BSShader::SetupDefaultStage(spStage, 0, 3, 2, true);
	spStage->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
	spPass[PRECIP_PASS_RAIN]->SetStage(spPass[PRECIP_PASS_RAIN]->m_uiCurrentStage, spStage);
	// EXE-INLINED none: NiD3DPass::SetVertexShader
	spPass[PRECIP_PASS_RAIN]->m_spVertexShader = spVertexShader[PRECIP_VSH_BILLBOARDUP].m_pObject;
	// EXE-INLINED none: NiD3DPass::SetPixelShader
	spPass[PRECIP_PASS_RAIN]->m_spPixelShader = spPixelShader[PRECIP_PSH_RAIN].m_pObject;
	spPass[PRECIP_PASS_RAIN]->SetRenderState(D3DRS_ZENABLE, 1);
	spPass[PRECIP_PASS_RAIN]->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	spPass[PRECIP_PASS_RAIN]->SetRenderState(D3DRS_ZFUNC, 4);
	spPass[PRECIP_PASS_RAIN]->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	spPass[PRECIP_PASS_RAIN]->SetSrcDestBlendRenderState(5, 6);
	spPass[PRECIP_PASS_RAIN]->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
	spPass[PRECIP_PASS_RAIN]->SetRenderState(D3DRS_COLORWRITEENABLE, 7);
	return true;
}

BSShader* PrecipitationShader::CreateShader() {
	PrecipitationShader* pshader = 0;
	NiDX9ShaderDeclaration* pshaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 2u);
	pshaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
	pshaderDecl->SetEntry(1u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
	pshaderDecl->SetEntry(1u, 0, 6u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK,
		NiShaderDeclaration::SPTYPE_FLOAT3, NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u,
		NiD3DShaderDeclaration::SPTESS_DEFAULT);
	pshaderDecl->GetD3DDeclaration();
	PrecipitationShader* v2 = static_cast<PrecipitationShader*>(NiMemObject::operator new(sizeof(PrecipitationShader)));
	if (v2) {
		pshader = ::new (v2) PrecipitationShader;
	}
	pshader->Initialize();
	pshader->LoadShaders();
	pshader->LoadStagesAndPasses();
	pshader->CreateConstantMaps();
	pshader->SetShaderDecl(pshaderDecl);
	return pshader;
}
