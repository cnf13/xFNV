#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9LightManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSCMExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

// The shader map name's literal: the exe compares the old handle with its address before interning it.
using NDLShaderMapNameLiteral_t = const char[14];
static NDLShaderMapNameLiteral_t NDLShaderMapNameLiteral{"__NDL_SCMData"};
NiFixedString NiD3DShader::ms_kNDLShaderMapName(EmptyFS);
NiRTTI NiD3DShader::ms_RTTI("NiD3DShader", &NiD3DShaderInterface::ms_RTTI);

// Takes the shader out of the factory, under its implementation and, when it is the best one, under
// DEFAULT_IMPLEMENTATION, if the factory holds it there; then releases its renderer data and render state group.
// The smart pointer members and the NiD3DShaderInterface part are destroyed by the compiler.
NiD3DShader::~NiD3DShader() {
	const char* pcName = m_kName.GetHandle();
	if (pcName != NiGlobalStringTable::NULL_STRING && NiGlobalStringTable::GetStringHeader(pcName)->m_uiLength) {
		NiD3DShaderFactory* pkShaderFactory = NiD3DShaderFactory::GetD3DShaderFactory();
		if (this == pkShaderFactory->FindShader(m_kName.GetHandle(), GetImplementation())) {
			pkShaderFactory->RemoveShader(m_kName.GetHandle(), GetImplementation());
		}
		if (m_bIsBestImplementation &&
			this == pkShaderFactory->FindShader(m_kName.GetHandle(), DEFAULT_IMPLEMENTATION)) {
			pkShaderFactory->RemoveShader(m_kName.GetHandle(), DEFAULT_IMPLEMENTATION);
		}
	}
	if (m_pkD3DRenderer) {
		m_pkD3DRenderer->ReleaseD3DShader(this);
	}
	m_bInitialized = false;
	NiD3DRenderStateGroup::ReleaseRenderStateGroup(m_pkRenderStateGroup);
	m_uiCurrentPass = 0;
	NiD3DShader::m_spCurrentPass = nullptr;
}

// Initialization is implemented by the shader interface base.
bool NiD3DShader::Initialize() {
	return NiD3DShaderInterface::Initialize();
}

// Sets up the shader-constant-map extra data and reports success.
bool NiD3DShader::SetupGeometry(NiGeometry* pkGeometry) {
	NiD3DShader::SetupSCMExtraData(this, pkGeometry);
	return true;
}

// Fails (0xFFFFFFFF) when not initialized. Otherwise applies the Ni render
// state and the fixed-function lights where the shader uses them, then its
// render state group. 0 on success.
uint32_t NiD3DShader::PreProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	if (!m_bInitialized)
		return 0xFFFFFFFF;
	if (m_bUsesNiRenderState)
		m_pkD3DRenderState->UpdateRenderState(pkState);
	if (m_bUsesNiLightState && m_pkD3DRenderer->m_pkLightManager) {
		const NiTexturingProperty* pkTex =
			static_cast<const NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject);
		m_pkD3DRenderer->m_pkLightManager->SetState(pkEffects, pkTex, nullptr);
	}
	if (m_pkRenderStateGroup)
		m_pkRenderStateGroup->SetRenderStates();
	return 0;
}

// The current pass sets up its rendering pass (as pass 0).
uint32_t NiD3DShader::SetupRenderingPass(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	// The pass still takes the D3D9 buffer type; the exe forwards the renderer data pointer unchanged.
	return m_spCurrentPass->SetupRenderingPass(pkGeometry, pkSkin, static_cast<NiGeometryBufferData*>(pkRendererData),
		pkState, pkEffects, kWorld, kWorldBound, 0);
}

// A skinned partition gets its bone matrices (computed on the first pass), its
// bone count and the skinned model transforms. Unskinned geometry gets no bones
// and the model transform, on the first pass or while batching. Always 0.
uint32_t NiD3DShader::SetupTransformations(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
	const NiBound& kWorldBound) {
	if (pkPartition) {
		if (m_uiCurrentPass == 0)
			m_pkD3DRenderer->CalculateBoneMatrices(pkSkin, kWorld, false, 4, false);
		m_pkD3DRenderState->SetBoneCount(pkPartition->m_usBonesPerVertex);
		m_pkD3DRenderer->SetSkinnedModelTransforms(pkSkin, pkPartition, kWorld);
		return 0;
	}
	if (m_uiCurrentPass == 0 || m_pkD3DRenderer->m_bBatchRendering) {
		m_pkD3DRenderState->SetBoneCount(0);
		m_pkD3DRenderer->SetModelTransform(kWorld, true);
	}
	return 0;
}

// Rewinds the geometry's constant map extra data, sets up the current pass's
// programs, then the shader's own pixel and vertex constants (as global
// constants) on the pass's programs, and on the first pass the shader's vertex
// declaration. Returns the pass's result.
uint32_t NiD3DShader::SetupShaderPrograms(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
	const NiBound& kWorldBound) {
	// The pass and the constant maps still take the D3D9 buffer type; the exe forwards the pointer unchanged.
	NiGeometryBufferData* pkBuffData = static_cast<NiGeometryBufferData*>(pkRendererData);
	if (pkGeometry) {
		NiSCMExtraData* pkSCMExtraData = static_cast<NiSCMExtraData*>(pkGeometry->GetExtraData(ms_kNDLShaderMapName));
		if (pkSCMExtraData) {
			pkSCMExtraData->m_auiCurrentEntry[NiGPUProgram::PROGRAM_VERTEX] = 0;
			pkSCMExtraData->m_auiCurrentEntry[NiGPUProgram::PROGRAM_PIXEL] = 0;
		}
	}

	uint32_t uiRet = m_spCurrentPass->SetupShaderPrograms(pkGeometry, pkSkin, pkPartition, pkBuffData, pkState,
		pkEffects, kWorld, kWorldBound, m_uiCurrentPass);

	if (m_spPixelConstantMap) {
		NiD3DPixelShader* pkPixelShader = m_spCurrentPass->m_spPixelShader;
		if (pkPixelShader) {
			m_spPixelConstantMap->SetShaderConstants(pkPixelShader, pkGeometry, pkSkin, pkPartition, pkBuffData,
				pkState, pkEffects, kWorld, kWorldBound, m_uiCurrentPass, true);
		}
	}
	if (m_spVertexConstantMap) {
		NiD3DVertexShader* pkVertexShader = m_spCurrentPass->m_spVertexShader;
		if (pkVertexShader) {
			m_spVertexConstantMap->SetShaderConstants(pkVertexShader, pkGeometry, pkSkin, pkPartition, pkBuffData,
				pkState, pkEffects, kWorld, kWorldBound, m_uiCurrentPass, true);
		}
	}
	if (m_uiCurrentPass == 0 && m_spShaderDecl)
		m_pkD3DRenderState->SetDeclaration(m_spShaderDecl->GetD3DDeclaration(), false);
	return uiRet;
}

// On the first pass: sets the vertex declaration (the packed additional data's,
// else the shader's) and packs the geometry's (or the partition's) vertex
// buffers. Then binds the buffer data's vertex streams and index buffer.
// Returns the buffer data used.
NiGeometryData::RendererData* NiD3DShader::PrepareGeometryForRendering(NiGeometry* pkGeometry,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState) {
	// The PDB's parameter is the renderer data base; the D3D9 buffers the exe passes and returns are its
	// NiGeometryBufferData.
	NiGeometryBufferData* pkBuffData = static_cast<NiGeometryBufferData*>(pkRendererData);
	if (pkGeometry && m_uiCurrentPass == 0) {
		NiGeometryData* pkData = pkGeometry->m_spModelData;
		NiSkinInstance* pkSkin = pkGeometry->m_spSkinInstance;
		NiAdditionalGeometryData* pkAddData = pkData ? pkData->m_spAdditionalGeomData.m_pObject : nullptr;
		if (pkAddData && pkAddData->GetVBReady()) {
			uint32_t uiDeclIndex = static_cast<BSPackedAdditionalGeometryData*>(pkAddData)->m_uiShaderDeclarationIndex;
			m_pkD3DRenderState->SetDeclaration(m_pkD3DRenderer->GetD3DDeclarationFromIndex(uiDeclIndex), false);
		} else if (m_spShaderDecl) {
			m_pkD3DRenderState->SetDeclaration(m_spShaderDecl->GetD3DDeclaration(), false);
		}

		if (pkPartition) {
			pkBuffData = pkPartition->m_pkBuffData;
			m_pkD3DRenderer->PackSkinnedGeometryBuffer(pkBuffData, pkData, pkSkin,
				const_cast<NiSkinPartition::Partition*>(pkPartition), m_spShaderDecl, false);
		} else {
			pkBuffData = pkData->m_pkBuffData;
			m_pkD3DRenderer->PackGeometryBuffer(pkBuffData, pkData, pkSkin, m_spShaderDecl, false);
		}
	}

	if (pkBuffData) {
		for (uint32_t i = 0; i < pkBuffData->m_uiStreamCount; i++) {
			uint32_t uiStride = i < pkBuffData->m_uiStreamCount ? pkBuffData->m_puiVertexStride[i] : 0;
			NiVBChip* pkChip = i < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[i] : nullptr;
			m_pkD3DDevice->SetStreamSource(i, pkChip->m_pkVB, 0, uiStride);
		}
		m_pkD3DDevice->SetIndices(pkBuffData->m_pkIB);
	}
	return pkBuffData;
}

// Restores the render states the shader's render state group changed; always 0.
uint32_t NiD3DShader::PostProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	if (m_pkRenderStateGroup)
		m_pkRenderStateGroup->RestoreRenderStates();
	return 0;
}

// Whether a pass is current.
uint32_t NiD3DShader::FirstPass() {
	return m_spCurrentPass != nullptr;
}

// Lets the current pass finish its rendering pass; always 0.
uint32_t NiD3DShader::NextPass() {
	NiD3DPass* pkPass = m_spCurrentPass;
	if (pkPass)
		pkPass->PostProcessRenderingPass(m_uiCurrentPass);
	return 0;
}

// Holds a reference on the new declaration and releases the old one.
void NiD3DShader::SetShaderDecl(NiD3DShaderDeclaration* pkShaderDecl) {
	m_spShaderDecl = pkShaderDecl;
}

// Drops the current pass's program device objects and marks the shader
// uninitialized.
void NiD3DShader::DestroyRendererData() {
	NiD3DPass* pkPass = m_spCurrentPass;
	if (pkPass) {
		if (pkPass->m_spPixelShader)
			pkPass->m_spPixelShader->DestroyRendererData();
		if (pkPass->m_spVertexShader)
			pkPass->m_spVertexShader->DestroyRendererData();
	}
	m_bInitialized = false;
}

// With a renderer: recreates the current pass's program device objects and
// marks the shader initialized.
void NiD3DShader::RecreateRendererData() {
	if (!m_pkD3DRenderer)
		return;
	NiD3DPass* pkPass = m_spCurrentPass;
	if (pkPass) {
		if (pkPass->m_spPixelShader)
			pkPass->m_spPixelShader->RecreateRendererData();
		if (pkPass->m_spVertexShader)
			pkPass->m_spVertexShader->RecreateRendererData();
	}
	m_bInitialized = true;
}

// Whether the current pass has a vertex shader.
bool NiD3DShader::GetVSPresentAllPasses() {
	return m_spCurrentPass->m_spVertexShader != nullptr;
}

bool NiD3DShader::GetVSPresentAnyPass() {
	return m_spCurrentPass->m_spVertexShader != nullptr;
}

// Whether the current pass has a pixel shader.
bool NiD3DShader::GetPSPresentAllPasses() {
	return m_spCurrentPass->m_spPixelShader != nullptr;
}

bool NiD3DShader::GetPSPresentAnyPass() {
	return m_spCurrentPass->m_spPixelShader != nullptr;
}

// Clears the current pass and pass index.
void NiD3DShader::ResetPasses() {
	m_spCurrentPass = nullptr;
	m_uiCurrentPass = 0;
}

// Creates the pass and texture stage pools.
void NiD3DShader::InitializePools() {
	NiD3DPass::InitializePools();
	NiD3DTextureStage::InitializePools();
}

// Destroys the pass and texture stage pools.
void NiD3DShader::ShutdownPools() {
	NiD3DPass::ShutdownPools();
	NiD3DTextureStage::ShutdownPools();
}

// Names the shader map, then sets one packer per NiTextureEffect::CoordGenType, in the exe's order
// (NiDX9RendererSDM::Init tail-jumps here). The
// exe inlines NiFixedString's assignment: skipped when the handle already is the literal's address, else the
// literal is interned and the old handle released through the import slot unless it is the table's empty one (a
// string's block is untyped memory whose first word, 8 bytes before the characters, is the reference count).
void NiD3DShader::_SDMInit() {
	const char* pcOld = ms_kNDLShaderMapName.GetHandle();
	if (pcOld != NDLShaderMapNameLiteral) {
		ms_kNDLShaderMapName.SetHandle(NiGlobalStringTable::AddString(NDLShaderMapNameLiteral));
		if (pcOld != NiGlobalStringTable::NULL_STRING)
			InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
	}
	ms_apfnDynEffectPackers[NiTextureEffect::WORLD_PARALLEL] =
		&fnvd::callFunction<&NiD3DShader::PackWorldParallelEffect>;
	ms_apfnDynEffectPackers[NiTextureEffect::WORLD_PERSPECTIVE] =
		&fnvd::callFunction<&NiD3DShader::PackWorldPerspectiveEffect>;
	ms_apfnDynEffectPackers[NiTextureEffect::SPHERE_MAP] = &fnvd::callFunction<&NiD3DShader::PackWorldSphereEffect>;
	ms_apfnDynEffectPackers[NiTextureEffect::SPECULAR_CUBE_MAP] =
		&fnvd::callFunction<&NiD3DShader::PackSpecularCubeEffect>;
	ms_apfnDynEffectPackers[NiTextureEffect::DIFFUSE_CUBE_MAP] =
		&fnvd::callFunction<&NiD3DShader::PackDiffuseCubeEffect>;
}

// Interns nothing and drops the shader map name's old handle. The exe inlines NiFixedString's null assignment: it
// skips everything for a null handle, interns null, and releases the old handle through the import slot unless it
// is the table's empty one (NULL_STRING), never through GetRealBufferStart. A string's block is untyped memory whose
// first word, 8 bytes before the characters, is the reference count.
void NiD3DShader::_SDMShutdown() {
	const char* pcOld = ms_kNDLShaderMapName.GetHandle();
	if (pcOld) {
		ms_kNDLShaderMapName.SetHandle(NiGlobalStringTable::AddString(nullptr));
		if (pcOld != NiGlobalStringTable::NULL_STRING)
			InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(pcOld) - 8));
	}
}

// Replaces the geometry's constant map extra data. Counts the attribute entries
// of the shader's and the current pass's vertex and pixel constant maps; when
// there are any, attaches a new NiSCMExtraData caching, for each attribute
// entry, the geometry's extra data of that name. NiSCMExtraData::AddEntry is
// inlined: the shader's entries are keyed as global (pass 0xFF), the pass's as
// pass 0.
void NiD3DShader::SetupSCMExtraData(NiD3DShader* pkD3DShader, NiGeometry* pkGeometry) {
	auto IsAttribute = [](const NiShaderConstantMapEntry* pkEntry) {
		return (pkEntry->m_uiFlags & NiShaderConstantMapEntry::SCME_MAP_MASK) ==
			NiShaderConstantMapEntry::SCME_MAP_ATTRIBUTE;
	};

	pkGeometry->RemoveExtraData(ms_kNDLShaderMapName);

	uint32_t uiNumVertexEntries = 0;
	uint32_t uiNumPixelEntries = 0;
	if (pkD3DShader->m_spVertexConstantMap) {
		uint32_t uiCount = pkD3DShader->m_spVertexConstantMap->m_aspEntries.GetEffectiveSize();
		for (uint32_t i = 0; i < uiCount; i++) {
			NiShaderConstantMapEntry* pkEntry = pkD3DShader->m_spVertexConstantMap->GetEntryAtIndex(i);
			if (pkEntry && IsAttribute(pkEntry))
				uiNumVertexEntries++;
		}
	}
	if (pkD3DShader->m_spPixelConstantMap) {
		uint32_t uiCount = pkD3DShader->m_spPixelConstantMap->m_aspEntries.GetEffectiveSize();
		for (uint32_t i = 0; i < uiCount; i++) {
			NiShaderConstantMapEntry* pkEntry = pkD3DShader->m_spPixelConstantMap->GetEntryAtIndex(i);
			if (pkEntry && IsAttribute(pkEntry))
				uiNumPixelEntries++;
		}
	}
	NiD3DPass* pkPass = m_spCurrentPass;
	if (pkPass) {
		NiD3DShaderConstantMap* pkMap = pkPass->m_spPixelConstantMap;
		if (pkMap) {
			uint32_t uiCount = pkMap->m_aspEntries.GetEffectiveSize();
			for (uint32_t i = 0; i < uiCount; i++) {
				NiShaderConstantMapEntry* pkEntry = pkMap->GetEntryAtIndex(i);
				if (pkEntry && IsAttribute(pkEntry))
					uiNumPixelEntries++;
			}
		}
		pkMap = pkPass->m_spVertexConstantMap;
		if (pkMap) {
			uint32_t uiCount = pkMap->m_aspEntries.GetEffectiveSize();
			for (uint32_t i = 0; i < uiCount; i++) {
				NiShaderConstantMapEntry* pkEntry = pkMap->GetEntryAtIndex(i);
				if (pkEntry && IsAttribute(pkEntry))
					uiNumVertexEntries++;
			}
		}
	}

	if (uiNumVertexEntries == 0 && uiNumPixelEntries == 0)
		return;

	void* pvMemory = NiMemObject::operator new(sizeof(NiSCMExtraData));
	NiSCMExtraData* pkSCMExtraData = pvMemory
		? ::new (pvMemory) NiSCMExtraData(ms_kNDLShaderMapName, uiNumVertexEntries, uiNumPixelEntries)
		: nullptr;

	NiD3DPass* pkCurrentPass = m_spCurrentPass;
	if (pkCurrentPass) {
		const uint32_t VS = NiGPUProgram::PROGRAM_VERTEX;
		const uint32_t PS = NiGPUProgram::PROGRAM_PIXEL;

		uint32_t uiCount =
			pkD3DShader->m_spPixelConstantMap ? pkD3DShader->m_spPixelConstantMap->m_aspEntries.GetEffectiveSize() : 0;
		for (uint32_t i = 0; i < uiCount; i++) {
			NiShaderConstantMapEntry* pkEntry = pkD3DShader->m_spPixelConstantMap->GetEntryAtIndex(i);
			if (!pkEntry || !IsAttribute(pkEntry))
				continue;
			NiExtraData* pkExtraData = pkGeometry->GetExtraData(pkEntry->m_kKey);
			if (!pkExtraData)
				continue;
			pkSCMExtraData->m_apkEntries[PS][pkSCMExtraData->m_auiCurrentEntry[PS]].m_uiIndex =
				(0xFFu << NiSCMExtraData::m_uiPassShift) | pkEntry->m_uiShaderRegister;
			pkSCMExtraData->m_apkEntries[PS][pkSCMExtraData->m_auiCurrentEntry[PS]].m_pkExtraData = pkExtraData;
			pkSCMExtraData->m_auiCurrentEntry[PS]++;
		}

		uiCount = pkD3DShader->m_spVertexConstantMap
			? pkD3DShader->m_spVertexConstantMap->m_aspEntries.GetEffectiveSize()
			: 0;
		for (uint32_t i = 0; i < uiCount; i++) {
			NiShaderConstantMapEntry* pkEntry = pkD3DShader->m_spVertexConstantMap->GetEntryAtIndex(i);
			if (!pkEntry || !IsAttribute(pkEntry))
				continue;
			NiExtraData* pkExtraData = pkGeometry->GetExtraData(pkEntry->m_kKey);
			if (!pkExtraData)
				continue;
			pkSCMExtraData->m_apkEntries[VS][pkSCMExtraData->m_auiCurrentEntry[VS]].m_uiIndex =
				(0xFFu << NiSCMExtraData::m_uiPassShift) | pkEntry->m_uiShaderRegister;
			pkSCMExtraData->m_apkEntries[VS][pkSCMExtraData->m_auiCurrentEntry[VS]].m_pkExtraData = pkExtraData;
			pkSCMExtraData->m_auiCurrentEntry[VS]++;
		}

		NiD3DShaderConstantMap* pkMap = pkCurrentPass->m_spPixelConstantMap;
		if (pkMap) {
			uiCount = pkMap->m_aspEntries.GetEffectiveSize();
			for (uint32_t i = 0; i < uiCount; i++) {
				NiShaderConstantMapEntry* pkEntry = pkMap->GetEntryAtIndex(i);
				if (!pkEntry || !IsAttribute(pkEntry))
					continue;
				NiExtraData* pkExtraData = pkGeometry->GetExtraData(pkEntry->m_kKey);
				if (!pkExtraData)
					continue;
				pkSCMExtraData->m_apkEntries[PS][pkSCMExtraData->m_auiCurrentEntry[PS]].m_uiIndex =
					(0u << NiSCMExtraData::m_uiPassShift) | pkEntry->m_uiShaderRegister;
				pkSCMExtraData->m_apkEntries[PS][pkSCMExtraData->m_auiCurrentEntry[PS]].m_pkExtraData = pkExtraData;
				pkSCMExtraData->m_auiCurrentEntry[PS]++;
			}
		}

		pkMap = pkCurrentPass->m_spVertexConstantMap;
		if (pkMap) {
			uiCount = pkMap->m_aspEntries.GetEffectiveSize();
			for (uint32_t i = 0; i < uiCount; i++) {
				NiShaderConstantMapEntry* pkEntry = pkMap->GetEntryAtIndex(i);
				if (!pkEntry || !IsAttribute(pkEntry))
					continue;
				NiExtraData* pkExtraData = pkGeometry->GetExtraData(pkEntry->m_kKey);
				if (!pkExtraData)
					continue;
				pkSCMExtraData->m_apkEntries[VS][pkSCMExtraData->m_auiCurrentEntry[VS]].m_uiIndex =
					(0u << NiSCMExtraData::m_uiPassShift) | pkEntry->m_uiShaderRegister;
				pkSCMExtraData->m_apkEntries[VS][pkSCMExtraData->m_auiCurrentEntry[VS]].m_pkExtraData = pkExtraData;
				pkSCMExtraData->m_auiCurrentEntry[VS]++;
			}
		}
	}
	pkGeometry->AddExtraData(pkSCMExtraData);
}

// The packer for the effect's coordinate mode, given the effect's world
// projection.
void NiD3DShader::PackDynamicEffect(const NiTextureEffect* apTextureEffect, NiD3DTextureStage* apStage, int32_t abSave,
	NiDX9Renderer* apD3DRenderer) {
	ms_apfnDynEffectPackers[apTextureEffect->m_eCoordMode](apTextureEffect->m_kWorldProjMat,
		apTextureEffect->m_kWorldProjTrans, apStage, abSave, apD3DRenderer);
}

// Parallel projection: texture coordinates from the camera-space position,
// transformed by the world rotation and translation taken into camera space
// (the inverse view), as two-component coordinates.
void NiD3DShader::PackWorldParallelEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
	NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer) {
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION, bSave);
	pkStage->m_bTextureTransform = true;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2, bSave);

	const float (*W)[3] = kWorldMat.m_pEntry;
	const D3DMATRIX& kV = pkD3DRenderer->m_kInvView;
	D3DMATRIX& kT = pkStage->m_kTextureTransformation;
	kT._11 = W[0][0] * kV._11 + W[0][1] * kV._12 + W[0][2] * kV._13;
	kT._21 = W[0][0] * kV._21 + W[0][1] * kV._22 + W[0][2] * kV._23;
	kT._31 = W[0][0] * kV._31 + W[0][1] * kV._32 + W[0][2] * kV._33;
	kT._12 = W[1][0] * kV._11 + W[1][1] * kV._12 + W[1][2] * kV._13;
	kT._22 = W[1][0] * kV._21 + W[1][1] * kV._22 + W[1][2] * kV._23;
	kT._32 = W[1][0] * kV._31 + W[1][1] * kV._32 + W[1][2] * kV._33;
	kT._13 = W[2][0] * kV._11 + W[2][1] * kV._12 + W[2][2] * kV._13;
	kT._23 = W[2][0] * kV._21 + W[2][1] * kV._22 + W[2][2] * kV._23;
	kT._33 = W[2][0] * kV._31 + W[2][1] * kV._32 + W[2][2] * kV._33;
	kT._41 = W[0][0] * kV._41 + W[0][1] * kV._42 + W[0][2] * kV._43 + kWorldTrans.x;
	kT._42 = W[1][0] * kV._41 + W[1][1] * kV._42 + W[1][2] * kV._43 + kWorldTrans.y;
	kT._43 = W[2][0] * kV._41 + W[2][1] * kV._42 + W[2][2] * kV._43 + kWorldTrans.z;
	kT._44 = 0.0f;
	kT._34 = 0.0f;
	kT._24 = 0.0f;
	kT._14 = 0.0f;
}

// Perspective projection: as the parallel one, with the renderer's projected
// texture flags (a transform is used unless they are 0 or D3DTTFF_PROJECTED
// alone).
void NiD3DShader::PackWorldPerspectiveEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
	NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer) {
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION, bSave);
	uint32_t uiFlags = NiDX9Renderer::ms_uiProjectedTextureFlags;
	pkStage->m_bTextureTransform = uiFlags != D3DTTFF_DISABLE && uiFlags != D3DTTFF_PROJECTED;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, uiFlags, bSave);

	const float (*W)[3] = kWorldMat.m_pEntry;
	const D3DMATRIX& kV = pkD3DRenderer->m_kInvView;
	D3DMATRIX& kT = pkStage->m_kTextureTransformation;
	kT._11 = W[0][0] * kV._11 + W[0][1] * kV._12 + W[0][2] * kV._13;
	kT._21 = W[0][0] * kV._21 + W[0][1] * kV._22 + W[0][2] * kV._23;
	kT._31 = W[0][0] * kV._31 + W[0][1] * kV._32 + W[0][2] * kV._33;
	kT._12 = W[1][0] * kV._11 + W[1][1] * kV._12 + W[1][2] * kV._13;
	kT._22 = W[1][0] * kV._21 + W[1][1] * kV._22 + W[1][2] * kV._23;
	kT._32 = W[1][0] * kV._31 + W[1][1] * kV._32 + W[1][2] * kV._33;
	kT._13 = W[2][0] * kV._11 + W[2][1] * kV._12 + W[2][2] * kV._13;
	kT._23 = W[2][0] * kV._21 + W[2][1] * kV._22 + W[2][2] * kV._23;
	kT._33 = W[2][0] * kV._31 + W[2][1] * kV._32 + W[2][2] * kV._33;
	kT._41 = W[0][0] * kV._41 + W[0][1] * kV._42 + W[0][2] * kV._43 + kWorldTrans.x;
	kT._42 = W[1][0] * kV._41 + W[1][1] * kV._42 + W[1][2] * kV._43 + kWorldTrans.y;
	kT._43 = W[2][0] * kV._41 + W[2][1] * kV._42 + W[2][2] * kV._43 + kWorldTrans.z;
	kT._44 = 0.0f;
	kT._34 = 0.0f;
	kT._24 = 0.0f;
	kT._14 = 0.0f;
}

// Sphere map: texture coordinates from the camera-space reflection vector,
// rotated by the world rotation taken into camera space, translated by the
// world translation, as two-component coordinates.
void NiD3DShader::PackWorldSphereEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
	NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer) {
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, bSave);
	pkStage->m_bTextureTransform = true;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2, bSave);

	const float (*W)[3] = kWorldMat.m_pEntry;
	const D3DMATRIX& kV = pkD3DRenderer->m_kInvView;
	D3DMATRIX& kT = pkStage->m_kTextureTransformation;
	kT._11 = W[0][0] * kV._11 + W[0][1] * kV._12 + W[0][2] * kV._13;
	kT._21 = W[0][0] * kV._21 + W[0][1] * kV._22 + W[0][2] * kV._23;
	kT._31 = W[0][0] * kV._31 + W[0][1] * kV._32 + W[0][2] * kV._33;
	kT._12 = W[1][0] * kV._11 + W[1][1] * kV._12 + W[1][2] * kV._13;
	kT._22 = W[1][0] * kV._21 + W[1][1] * kV._22 + W[1][2] * kV._23;
	kT._32 = W[1][0] * kV._31 + W[1][1] * kV._32 + W[1][2] * kV._33;
	kT._13 = W[2][0] * kV._11 + W[2][1] * kV._12 + W[2][2] * kV._13;
	kT._23 = W[2][0] * kV._21 + W[2][1] * kV._22 + W[2][2] * kV._23;
	kT._33 = W[2][0] * kV._31 + W[2][1] * kV._32 + W[2][2] * kV._33;
	kT._41 = kWorldTrans.x;
	kT._42 = kWorldTrans.y;
	kT._43 = kWorldTrans.z;
	kT._44 = 0.0f;
	kT._34 = 0.0f;
	kT._24 = 0.0f;
	kT._14 = 0.0f;
}

// Specular cube map: as the sphere map, as three-component coordinates.
void NiD3DShader::PackSpecularCubeEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
	NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer) {
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, bSave);
	pkStage->m_bTextureTransform = true;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3, bSave);

	const float (*W)[3] = kWorldMat.m_pEntry;
	const D3DMATRIX& kV = pkD3DRenderer->m_kInvView;
	D3DMATRIX& kT = pkStage->m_kTextureTransformation;
	kT._11 = W[0][0] * kV._11 + W[0][1] * kV._12 + W[0][2] * kV._13;
	kT._21 = W[0][0] * kV._21 + W[0][1] * kV._22 + W[0][2] * kV._23;
	kT._31 = W[0][0] * kV._31 + W[0][1] * kV._32 + W[0][2] * kV._33;
	kT._12 = W[1][0] * kV._11 + W[1][1] * kV._12 + W[1][2] * kV._13;
	kT._22 = W[1][0] * kV._21 + W[1][1] * kV._22 + W[1][2] * kV._23;
	kT._32 = W[1][0] * kV._31 + W[1][1] * kV._32 + W[1][2] * kV._33;
	kT._13 = W[2][0] * kV._11 + W[2][1] * kV._12 + W[2][2] * kV._13;
	kT._23 = W[2][0] * kV._21 + W[2][1] * kV._22 + W[2][2] * kV._23;
	kT._33 = W[2][0] * kV._31 + W[2][1] * kV._32 + W[2][2] * kV._33;
	kT._41 = kWorldTrans.x;
	kT._42 = kWorldTrans.y;
	kT._43 = kWorldTrans.z;
	kT._44 = 0.0f;
	kT._34 = 0.0f;
	kT._24 = 0.0f;
	kT._14 = 0.0f;
}

// Diffuse cube map: as the specular one, from the camera-space normal.
void NiD3DShader::PackDiffuseCubeEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
	NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer) {
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL, bSave);
	pkStage->m_bTextureTransform = true;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3, bSave);

	const float (*W)[3] = kWorldMat.m_pEntry;
	const D3DMATRIX& kV = pkD3DRenderer->m_kInvView;
	D3DMATRIX& kT = pkStage->m_kTextureTransformation;
	kT._11 = W[0][0] * kV._11 + W[0][1] * kV._12 + W[0][2] * kV._13;
	kT._21 = W[0][0] * kV._21 + W[0][1] * kV._22 + W[0][2] * kV._23;
	kT._31 = W[0][0] * kV._31 + W[0][1] * kV._32 + W[0][2] * kV._33;
	kT._12 = W[1][0] * kV._11 + W[1][1] * kV._12 + W[1][2] * kV._13;
	kT._22 = W[1][0] * kV._21 + W[1][1] * kV._22 + W[1][2] * kV._23;
	kT._32 = W[1][0] * kV._31 + W[1][1] * kV._32 + W[1][2] * kV._33;
	kT._13 = W[2][0] * kV._11 + W[2][1] * kV._12 + W[2][2] * kV._13;
	kT._23 = W[2][0] * kV._21 + W[2][1] * kV._22 + W[2][2] * kV._23;
	kT._33 = W[2][0] * kV._31 + W[2][1] * kV._32 + W[2][2] * kV._33;
	kT._41 = kWorldTrans.x;
	kT._42 = kWorldTrans.y;
	kT._43 = kWorldTrans.z;
	kT._44 = 0.0f;
	kT._34 = 0.0f;
	kT._24 = 0.0f;
	kT._14 = 0.0f;
}
