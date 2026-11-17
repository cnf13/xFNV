#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

// NiTArray<NiPointer<NiD3DTextureStage>>::SetAt for this element type (the stages count by hand), with its own guarded
// static zero element, compared by its pointer only.
void NiD3DTextureStageArray_SetAt(NiTObjectArray<NiPointer<NiD3DTextureStage>>* apArray, uint32_t auiIndex,
	const NiPointer<NiD3DTextureStage>& akElement) {
	// SetAt's zero element, constructed on the first call and released at exit.
	static NiPointer<NiD3DTextureStage> tcmp = static_cast<NiD3DTextureStage*>(nullptr);
	if (auiIndex < apArray->m_usSize) {
		NiPointer<NiD3DTextureStage>* pBase = apArray->m_pBase;
		if (akElement.m_pObject == tcmp.m_pObject) {
			if (pBase[auiIndex].m_pObject != tcmp.m_pObject) {
				--apArray->m_usESize;
			}
		} else {
			if (pBase[auiIndex].m_pObject == tcmp.m_pObject) {
				++apArray->m_usESize;
			}
		}
	} else {
		apArray->m_usSize = auiIndex + 1;
		if (akElement.m_pObject != tcmp.m_pObject) {
			++apArray->m_usESize;
		}
	}
	NiPointer<NiD3DTextureStage>* pBase = apArray->m_pBase;
	NiD3DTextureStage* pkOld = pBase[auiIndex].m_pObject;
	NiPointer<NiD3DTextureStage>* pSlot = &pBase[auiIndex];
	if (pkOld != akElement.m_pObject) {
		if (pkOld) {
			if (pkOld->m_uiRefCount-- == 1) {
				pkOld->ReturnStageToPool();
			}
		}
		NiD3DTextureStage* pkNew = akElement.m_pObject;
		pSlot->m_pObject = akElement.m_pObject;
		if (pkNew) {
			NiIncRefCount(pkNew);
		}
	}
}

// NiTArray<NiPointer<NiD3DTextureStage>>::SetSize for this element type: slots past the new size are cleared, the kept
// ones are copied into new storage from the shared NiPointer new[] and the rest of it is cleared, then the old storage
// goes through the stage arrays' delete[].
void NiD3DTextureStageArray_SetSize(NiTObjectArray<NiPointer<NiD3DTextureStage>>* apArray, uint32_t auiMaxSize) {
	if (auiMaxSize == apArray->m_usMaxSize) {
		return;
	}
	if (auiMaxSize < apArray->m_usSize) {
		for (uint16_t i = static_cast<uint16_t>(auiMaxSize); i < apArray->m_usSize; ++i) {
			NiPointer<NiD3DTextureStage>& spStage = apArray->m_pBase[i];
			if (spStage.m_pObject) {
				spStage.m_pObject->DecRefCount();
				spStage.m_pObject = nullptr;
				--apArray->m_usESize;
			}
		}
		apArray->m_usSize = static_cast<uint16_t>(auiMaxSize);
	}
	NiPointer<NiD3DTextureStage>* pSaveBase = apArray->m_pBase;
	apArray->m_usMaxSize = static_cast<uint16_t>(auiMaxSize);
	if (auiMaxSize) {
		apArray->m_pBase = NiTNewInterface<NiPointer<NiD3DTextureStage>>::Allocate(static_cast<uint16_t>(auiMaxSize));
		for (uint16_t i = 0; i < apArray->m_usSize; ++i) {
			apArray->m_pBase[i] = pSaveBase[i];
		}
		for (uint16_t i = apArray->m_usSize; i < apArray->m_usMaxSize; ++i) {
			NiPointer<NiD3DTextureStage>& spStage = apArray->m_pBase[i];
			spStage = nullptr;
		}
	} else {
		apArray->m_pBase = nullptr;
	}
	NiTNewInterface<NiPointer<NiD3DTextureStage>>::Deallocate(pSaveBase);
}

NiD3DPass::NiD3DPass()
	: m_uiCurrentStage(0), m_uiStageCount(0), m_uiTexturesPerPass(0),
	  m_kStages(NiD3DPass::ms_uiMaxTextureBlendStages, 4u), m_pkRenderStateGroup(nullptr),
	  m_spPixelConstantMap(nullptr), m_pcPixelShaderProgramFile(nullptr), m_pcPixelShaderEntryPoint(nullptr),
	  m_pcPixelShaderTarget(nullptr), m_spPixelShader(nullptr), m_spVertexConstantMap(nullptr), cEnabledStages(0),
	  m_pcVertexShaderProgramFile(nullptr), m_pcVertexShaderEntryPoint(nullptr), m_pcVertexShaderTarget(nullptr),
	  m_spVertexShader(nullptr), m_bSoftwareVP(false), m_bRendererOwned(false), m_uiRefCount(0) {
	m_szName[0] = 0;
}

NiD3DPass::~NiD3DPass() {
	NiD3DRenderStateGroup::ReleaseRenderStateGroup(m_pkRenderStateGroup);
	m_spPixelConstantMap = nullptr;
	_NiFree(m_pcPixelShaderProgramFile);
	_NiFree(m_pcPixelShaderEntryPoint);
	_NiFree(m_pcPixelShaderTarget);
	m_spPixelShader = nullptr;
	m_spVertexConstantMap = nullptr;
	_NiFree(m_pcVertexShaderProgramFile);
	_NiFree(m_pcVertexShaderEntryPoint);
	_NiFree(m_pcVertexShaderTarget);
	m_spVertexShader = nullptr;
	m_kStages.RemoveAll();
	m_uiCurrentStage = 0;
	m_uiStageCount = 0;
	m_uiTexturesPerPass = 0;
}

// Sets the pixel and vertex shaders with their constants. With a vertex shader each blend stage gets its own texture
// coordinates and keeps only a projected texture transform; without one the geometry's FVF (or its declaration) is set
// instead. The stage loop's uiValue and bSave are locals that GetStageState reaches only through their pointers.
uint32_t NiD3DPass::SetupShaderPrograms(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	const NiSkinPartition::Partition* pkPartition, NiGeometryBufferData* pkBuffData, const NiPropertyState* pkState,
	const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound, uint32_t uiPass) {
	if (m_spPixelShader) {
		ms_pkD3DRenderState->SetPixelShader(m_spPixelShader->GetShaderHandle(), false);
		if (m_spPixelConstantMap) {
			m_spPixelConstantMap->SetShaderConstants(m_spPixelShader, pkGeometry, pkSkin, pkPartition, pkBuffData,
				pkState, pkEffects, kWorld, kWorldBound, uiPass, false);
		}
	} else {
		ms_pkD3DRenderState->SetPixelShader(nullptr, false);
	}

	if (!m_spVertexShader) {
		ms_pkD3DRenderState->SetVertexShader(nullptr, false);
		if (!pkBuffData) {
			if (pkPartition) {
				pkBuffData = pkPartition->m_pkBuffData;
			} else {
				pkBuffData = pkGeometry->m_spModelData->m_pkBuffData;
			}
		}
		const uint32_t uiFVF = pkBuffData->m_uiFVF;
		if (uiFVF) {
			ms_pkD3DRenderState->SetFVF(uiFVF, false);
		} else {
			ms_pkD3DRenderState->SetDeclaration(pkBuffData->m_hDeclaration, false);
		}
		return 0;
	}

	ms_pkD3DRenderState->SetVertexShader(m_spVertexShader->GetShaderHandle(), false);
	if (m_spVertexConstantMap) {
		m_spVertexConstantMap->SetShaderConstants(m_spVertexShader, pkGeometry, pkSkin, pkPartition, pkBuffData,
			pkState, pkEffects, kWorld, kWorldBound, uiPass, false);
	}

	for (uint32_t i = 0; i < ms_uiMaxTextureBlendStages; i++) {
		ms_pkD3DRenderState->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i, false);

		uint32_t uiFlags = D3DTTFF_DISABLE;
		if (i < m_uiStageCount) {
			NiD3DTextureStage* pkStage = m_kStages[i];
			if (pkStage && pkStage->m_pkTextureStageGroup) {
				uint32_t uiValue = 0;
				bool bSave = false;
				if (pkStage->m_pkTextureStageGroup->GetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, uiValue, bSave) &&
					(uiValue & D3DTTFF_PROJECTED)) {
					uiFlags = D3DTTFF_PROJECTED;
				}
			}
		}
		ms_pkD3DRenderState->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, uiFlags, false);
	}
	return 0;
}

// Sets the vertex processing mode and the pass's render states, configures the stages up to the first empty slot
// (returning the last configured stage's result) and disables the stage after them.
uint32_t NiD3DPass::SetupRenderingPass(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryBufferData* pkBuffData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound, uint32_t uiPass) {
	uint32_t uiReturn = 0;
	if (m_spVertexShader) {
		ms_pkD3DRenderState->SetSoftwareVertexProcessing(m_spVertexShader->GetSoftwareVertexProcessing());
	} else {
		ms_pkD3DRenderState->SetSoftwareVertexProcessing(m_bSoftwareVP);
	}

	if (m_pkRenderStateGroup) {
		m_pkRenderStateGroup->SetRenderStates();
	}

	const uint32_t uiSize = m_kStages.GetSize();
	uint32_t i = 0;
	for (; i < uiSize; i++) {
		NiD3DTextureStage* pkStage = m_kStages[i];
		if (!pkStage) {
			break;
		}
		if (pkStage->bApply) {
			uiReturn = pkStage->ConfigureStage(false);
		}
	}

	if (i < ms_uiMaxSamplers) {
		NiD3DTextureStage::DisableStage(i);
	}
	return uiReturn;
}

// Restores the render states the pass changed.
uint32_t NiD3DPass::PostProcessRenderingPass(uint32_t uiPass) {
	if (m_pkRenderStateGroup) {
		m_pkRenderStateGroup->RestoreRenderStates();
	}
	return 0;
}

// Return the render state shared by D3D passes.
NiDX9RenderState* NiD3DPass::GetD3DRenderState() {
	return static_cast<NiDX9RenderState*>(ms_pkD3DRenderState);
}

void NiD3DPass::SetPixelConstantMap(NiD3DShaderConstantMap* pkSCMPixel) {
	m_spPixelConstantMap = pkSCMPixel;
}

void NiD3DPass::SetPixelShader(NiD3DPixelShader* pkPixelShader) {
	m_spPixelShader = pkPixelShader;
}

void NiD3DPass::SetVertexConstantMap(NiD3DShaderConstantMap* pkSCMVertex) {
	m_spVertexConstantMap = pkSCMVertex;
}

void NiD3DPass::SetVertexShader(NiD3DVertexShader* pkVertexShader) {
	m_spVertexShader = pkVertexShader;
}

// A blend stage takes the stage directly; a stage past the blend stages grows the array first. A cleared slot that was
// the last one pulls the current stage back, but the slot before it is read only once: when that one is filled it spins
// forever, as the game does.
uint32_t NiD3DPass::SetStage(uint32_t uiStageNum, NiD3DTextureStage* pkStage) {
	if (uiStageNum >= ms_uiMaxSamplers) {
		return 1;
	}

	{
		NiPointer<NiD3DTextureStage> spStage(pkStage);
		if (uiStageNum >= ms_uiMaxTextureBlendStages) {
			// NiTArray::SetAtGrow.
			if (uiStageNum >= m_kStages.GetAllocatedSize()) {
				NiD3DTextureStageArray_SetSize(&m_kStages, uiStageNum + m_kStages.GetGrowBy());
			}
			NiD3DTextureStageArray_SetAt(&m_kStages, uiStageNum, spStage);
		} else {
			NiD3DTextureStageArray_SetAt(&m_kStages, uiStageNum, spStage);
		}
	}

	if (pkStage) {
		pkStage->m_uiStage = uiStageNum;
		if (m_uiCurrentStage == uiStageNum) {
			m_uiCurrentStage = uiStageNum + 1;
		}
		m_uiStageCount++;
		if (pkStage->m_spTexture) {
			m_uiTexturesPerPass++;
		}
		return 0;
	}

	const uint32_t uiCurrentStage = m_uiCurrentStage;
	if (uiCurrentStage == uiStageNum + 1 && uiCurrentStage != 0) {
		if (m_kStages[uiCurrentStage - 1]) {
			for (;;) {
			}
		}
		m_uiCurrentStage = uiCurrentStage - 1;
	}
	return 0;
}

// The last test deliberately adds uiTextures to the current stage, not to the texture count, as the game does.
bool NiD3DPass::CheckFreeStages(uint32_t uiStages, uint32_t uiTextures) {
	if (ms_uiMaxTextureBlendStages < m_uiCurrentStage + uiStages) {
		return false;
	}
	if (ms_uiMaxSimultaneousTextures < m_uiTexturesPerPass + uiTextures) {
		return false;
	}
	return ms_uiMaxSimultaneousTextures >= m_uiCurrentStage + uiTextures;
}

// A renderer-owned pass drops its shaders, constant maps, render states, program names and stages and goes back to
// the pool's free list; any other pass is deleted.
void NiD3DPass::ReturnPassToPool() {
	if (!m_bRendererOwned) {
		delete this;
		return;
	}

	m_spPixelConstantMap = nullptr;
	m_spPixelShader = nullptr;
	m_spVertexConstantMap = nullptr;
	m_spVertexShader = nullptr;
	NiD3DRenderStateGroup::ReleaseRenderStateGroup(m_pkRenderStateGroup);
	m_pkRenderStateGroup = nullptr;
	_NiFree(m_pcPixelShaderProgramFile);
	m_pcPixelShaderProgramFile = nullptr;
	_NiFree(m_pcPixelShaderEntryPoint);
	m_pcPixelShaderEntryPoint = nullptr;
	_NiFree(m_pcPixelShaderTarget);
	m_pcPixelShaderTarget = nullptr;
	_NiFree(m_pcVertexShaderProgramFile);
	m_pcVertexShaderProgramFile = nullptr;
	_NiFree(m_pcVertexShaderEntryPoint);
	m_pcVertexShaderEntryPoint = nullptr;
	_NiFree(m_pcVertexShaderTarget);
	m_pcVertexShaderTarget = nullptr;

	m_kStages.RemoveAll();
	m_uiCurrentStage = 0;
	m_uiStageCount = 0;
	m_uiTexturesPerPass = 0;

	m_bSoftwareVP = false;
	// The pass goes on the pool's free list once.
	NiD3DPass* pkPass = this;
	ms_pkPassPool->ReleaseObject(pkPass);
}

// Store the device's supported blend-stage count.
void NiD3DPass::SetMaxTextureBlendStages(uint32_t uiMaxTextureBlendStages) {
	ms_uiMaxTextureBlendStages = uiMaxTextureBlendStages;
}

// Store the device's supported simultaneous texture count.
void NiD3DPass::SetMaxSimultaneousTextures(uint32_t uiMaxSimultaneousTextures) {
	ms_uiMaxSimultaneousTextures = uiMaxSimultaneousTextures;
}

// Store the device's supported sampler count.
void NiD3DPass::SetMaxSamplers(uint32_t uiMaxSamplers) {
	ms_uiMaxSamplers = uiMaxSamplers;
}

// An empty pool of eight from the engine heap; null when the allocation fails.
void NiD3DPass::InitializePools() {
	NiTObjectPool<NiD3DPass>* pkPool =
		static_cast<NiTObjectPool<NiD3DPass>*>(MemoryManager_Singleton.Allocate(sizeof(NiTObjectPool<NiD3DPass>)));
	if (pkPool) {
		pkPool->m_kFreeObjects.m_pBase = nullptr;
		pkPool->m_kFreeObjects.m_uiAlloced = 0;
		pkPool->m_kFreeObjects.m_uiUsed = 0;
		pkPool->m_pkContainers = nullptr;
		pkPool->m_uiCurrentSize = 8;
		pkPool->m_uiInitialSize = 8;
	}
	ms_pkPassPool = pkPool;
}

// The pool is destroyed and its memory returned to the engine heap.
void NiD3DPass::ShutdownPools() {
	NiTObjectPool<NiD3DPass>* pkPool = ms_pkPassPool;
	if (pkPool) {
		pkPool->~NiTObjectPool<NiD3DPass>();
		MemoryManager_Singleton.Deallocate(pkPool);
	}
	ms_pkPassPool = nullptr;
}

// Takes the renderer's device (holding a reference) and render state.
void NiD3DPass::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
	ms_pkD3DRenderer = pkD3DRenderer;
	if (pkD3DRenderer) {
		IDirect3DDevice9* pkDevice = pkD3DRenderer->m_pkD3DDevice9;
		if (ms_pkD3DDevice) {
			ms_pkD3DDevice->Release();
		}
		ms_pkD3DDevice = pkDevice;
		if (pkDevice) {
			pkDevice->AddRef();
		}
		ms_pkD3DRenderState = ms_pkD3DRenderer->m_pkRenderState;
	} else {
		if (ms_pkD3DDevice) {
			ms_pkD3DDevice->Release();
		}
		ms_pkD3DDevice = nullptr;
		ms_pkD3DRenderState = nullptr;
	}
}

void NiD3DPass::CreateNewPass(NiPointer<NiD3DPass>& aspD3DPass) {
	// An empty pool first makes m_uiCurrentSize passes and doubles that size.
	NiTObjectPool<NiD3DPass>* pkPool = ms_pkPassPool;
	if (pkPool->m_kFreeObjects.m_uiUsed == 0) {
		pkPool->CreateNewObjects(pkPool->m_uiCurrentSize);
		pkPool->m_uiCurrentSize *= 2;
	}
	NiD3DPass** ppkFree = pkPool->m_kFreeObjects.m_pBase;
	NiD3DPass* pkFree = ppkFree[0];
	ppkFree[0] = ppkFree[--pkPool->m_kFreeObjects.m_uiUsed];

	// The local reference is taken behind a null test, but it is dropped (and the caller's added) without one, since
	// the pass has already been dereferenced, as the game does.
	if (pkFree) {
		pkFree->IncRefCount();
	}
	if (!pkFree->m_bRendererOwned) {
		pkFree->m_bRendererOwned = true;
	}

	NiD3DRenderStateGroup* pkGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
	NiD3DRenderStateGroup* pkOldGroup = pkFree->m_pkRenderStateGroup;
	if (pkOldGroup != pkGroup) {
		NiD3DRenderStateGroup::ReleaseRenderStateGroup(pkOldGroup);
	}
	pkFree->m_pkRenderStateGroup = pkGroup;

	NiD3DPass* pkOld = aspD3DPass.m_pObject;
	if (pkOld != pkFree) {
		if (pkOld) {
			pkOld->DecRefCount();
		}
		aspD3DPass.m_pObject = pkFree;
		pkFree->IncRefCount();
	}
	pkFree->DecRefCount();
}
