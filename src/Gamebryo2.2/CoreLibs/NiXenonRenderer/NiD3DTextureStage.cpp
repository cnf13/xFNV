#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalConstantEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "SDK/MSVC/string.h"

// The exe's constructor leaves m_pkTexture as the memory holds it, and passes such as ShadowLightShader's BSSM_AD3
// never set some of their stages' textures, which ApplyTextureStages applies when not null: the exe works only where
// that pool memory is zero. The native heap's history differs, so the stage starts with no texture.
NiD3DTextureStage::NiD3DTextureStage()
	: m_uiStage(0), m_spTexture(nullptr), m_uiTextureFlags(0), m_uiTextureTransformFlags(0), m_pkGlobalEntry(nullptr),
	  m_usObjTextureFlags(0), m_bTextureTransform(false), m_bRendererOwned(false), m_uiRefCount(0), bApply(true) {
	m_pkTextureStageGroup = NiD3DTextureStageGroup::GetFreeTextureStageGroup();
}

// Releases the global constant the stage uses, then gives its stage group back.
NiD3DTextureStage::~NiD3DTextureStage() {
	if (m_pkGlobalEntry) {
		NiShaderFactory::ReleaseGlobalShaderConstant(m_pkGlobalEntry->m_kKey);
	}
	NiD3DTextureStageGroup::ReleaseTextureStageGroup(m_pkTextureStageGroup);
}

// Sets the stage's texture coordinate index, texture and sampler states. The exe also caps the anisotropy here,
// which the reference source leaves to ModifyFilterMode. The mip filter comes from the texture manager's S0 mipmap
// flag. The three flags PrepareTextureForRendering fills are read only on the path where it returned a texture.
void NiD3DTextureStage::ApplyTexture() {
	if (m_uiStage < NiD3DPass::ms_uiMaxTextureBlendStages) {
		uint32_t uiValue = 0;
		bool bSave = false;
		if (!m_pkTextureStageGroup->GetStageState(D3DTSS_TEXCOORDINDEX, uiValue, bSave)) {
			uiValue = m_uiStage;
		}
		ms_pkD3DRenderState->SetTextureStageState(m_uiStage, D3DTSS_TEXCOORDINDEX, uiValue, false);
	}
	bool bS0Mipmap;
	bool bNonPow2;
	IDirect3DBaseTexture9* pkD3DTexture = nullptr;
	if (m_spTexture) {
		bool bChanged;
		pkD3DTexture = ms_pkD3DRenderer->m_pkTextureManager->PrepareTextureForRendering(m_spTexture, bChanged,
			bS0Mipmap, bNonPow2);
	}
	ms_pkD3DRenderState->SetTexture(m_uiStage, pkD3DTexture);
	if (pkD3DTexture) {
		if (!bS0Mipmap) {
			ms_pkD3DRenderState->SetSamplerState(m_uiStage, D3DSAMP_MIPFILTER, 0, false);
		}
		ModifyClampMode(bNonPow2);
		ApplyTextureTransform();
	}
	if (m_eFilter == NiTexturingProperty::FILTER_ANISO) {
		ms_pkD3DDevice->SetSamplerState(m_uiStage, D3DSAMP_MAXANISOTROPY, (uint32_t)NiRenderer::iMaxAnisotropy);
	}
}

void NiD3DTextureStage::ApplyTextureTransform() {
	uint32_t m_uiStage = this->m_uiStage;
	if (m_uiStage < NiD3DPass::ms_uiMaxTextureBlendStages) {
		_D3DMATRIX* p_m_kTextureTransformation;
		if (!m_bTextureTransform) {
			NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(m_uiStage, 24u, 0, 0);
			return;
		}
		uint32_t m_uiTextureTransformFlags = this->m_uiTextureTransformFlags;
		uint32_t v4 = this->m_uiTextureTransformFlags & NiD3DTextureStage::TSTTF_SOURCE_MASK;
		if (v4) {
			int32_t v5;
			_D3DMATRIX* m_pvDataSource;
			if (v4 == NiD3DTextureStage::TSTTF_GLOBAL) {
				v5 = m_uiTextureTransformFlags & 0xFFFFFFF;
				if (v5) {
					m_pvDataSource = static_cast<_D3DMATRIX*>(m_pkGlobalEntry->m_pvDataSource);
				LABEL_12:;
					if (NiD3DTextureStage::CalculateTransformationMatrix(v5, m_pvDataSource)) {
						NiD3DTextureStage::ms_pkD3DDevice->SetTransform(this->m_uiStage + 16,
							&NiD3DTextureStage::ms_kCalculatedTextureTransformation);
					}
					return;
				}
				p_m_kTextureTransformation = static_cast<_D3DMATRIX*>(m_pkGlobalEntry->m_pvDataSource);
			LABEL_15:;
				NiD3DTextureStage::ms_pkD3DDevice->SetTransform(m_uiStage + 16, p_m_kTextureTransformation);
				return;
			}
			if (v4 != NiD3DTextureStage::TSTTF_CONSTANT) {
				return;
			}
			v5 = m_uiTextureTransformFlags & 0xFFFFFFF;
			if (v5) {
				m_pvDataSource = &m_kTextureTransformation;
				goto LABEL_12;
			}
		}
		p_m_kTextureTransformation = &m_kTextureTransformation;
		goto LABEL_15;
	}
}

void NiD3DTextureStage::ModifyClampMode(bool bNonPow2) {
	if (bNonPow2 && (NiD3DTextureStage::ms_pkD3DRenderer->GetFlags() & 8) == 0 &&
		(NiD3DTextureStage::ms_pkD3DRenderer->GetFlags() & 4) != 0) {
		NiD3DTextureStage::ms_pkD3DRenderState->SetSamplerState(m_uiStage, 1u, 3u, 0);
		NiD3DTextureStage::ms_pkD3DRenderState->SetSamplerState(m_uiStage, 2u, 3u, 0);
		uint32_t v3 = 198;
		int32_t v4 = 8;
		do {
			NiD3DTextureStage::ms_pkD3DRenderState->SetRenderState(v3 - 70, 0, 0, 0);
			NiD3DTextureStage::ms_pkD3DRenderState->SetRenderState(v3++, 0, 0, 0);
			--v4;
		} while (v4);
	}
}

bool NiD3DTextureStage::ConfigureStage(bool abForceUpdateSamplerStates) {
	if (!abForceUpdateSamplerStates && m_spTexture == pShadowedTextures[m_uiStage]) {
		return true;
	}
	pShadowedTextures[m_uiStage] = m_spTexture;
	if (m_pkTextureStageGroup->SetTextureStage(m_uiStage)) {
		return false;
	}
	ApplyTexture();
	return true;
}

// The exe reloads the mapping entry after each call.
void NiD3DTextureStage::SetFilterMode(NiTexturingProperty::FilterMode eFilter) {
	m_pkTextureStageGroup->SetSamplerState(D3DSAMP_MAGFILTER, NiDX9Renderer::ms_akD3DFilterMapping[eFilter].m_eMag,
		false, false);
	m_pkTextureStageGroup->SetSamplerState(D3DSAMP_MINFILTER, NiDX9Renderer::ms_akD3DFilterMapping[eFilter].m_eMin,
		false, false);
	m_pkTextureStageGroup->SetSamplerState(D3DSAMP_MIPFILTER, NiDX9Renderer::ms_akD3DFilterMapping[eFilter].m_eMip,
		false, false);
	m_eFilter = eFilter;
}

// The exe reloads the mapping entry after the first call.
void NiD3DTextureStage::SetClampMode(NiTexturingProperty::ClampMode eClamp) {
	m_pkTextureStageGroup->SetSamplerState(D3DSAMP_ADDRESSU, NiDX9Renderer::ms_akD3DAddressMapping[eClamp].m_eU, false,
		false);
	m_pkTextureStageGroup->SetSamplerState(D3DSAMP_ADDRESSV, NiDX9Renderer::ms_akD3DAddressMapping[eClamp].m_eV, false,
		false);
}

uint32_t NiD3DTextureStage::GetTexCoordIndex() {
	uint32_t uiValue = 0;
	bool bSave = false;
	if (!m_pkTextureStageGroup->GetStageState(D3DTSS_TEXCOORDINDEX, uiValue, bSave)) {
		return m_uiStage;
	}
	return uiValue;
}

bool NiD3DTextureStage::GetSaveTexCoordIndex() {
	NiD3DTextureStageGroup* m_pkTextureStageGroup = this->m_pkTextureStageGroup;
	uint32_t uiValue = 0;
	bool bSave = 0;
	bool StageState = m_pkTextureStageGroup->GetStageState(11u, uiValue, bSave);
	return StageState && bSave;
}

// A renderer-owned stage is cleared (texture, stage and sampler states, global constant, flags) and goes back to the
// pool; any other is deleted.
void NiD3DTextureStage::ReturnStageToPool() {
	if (m_bRendererOwned) {
		m_spTexture = nullptr;
		m_pkTextureStageGroup->RemoveAllStageStates();
		m_pkTextureStageGroup->RemoveAllSamplerStates();
		NiGlobalConstantEntry* pkGlobalEntry = m_pkGlobalEntry;
		if (pkGlobalEntry) {
			FNVD_INLINED_LOCAL(NiFixedString, kKey, std::nullopt, std::nullopt);
			NiGlobalStringTable::AcquireHandle(pkGlobalEntry->m_kKey.GetHandle());
			kKey.m_kHandle = pkGlobalEntry->m_kKey.GetHandle();
			NiShaderFactory::ReleaseGlobalShaderConstant(kKey);
			// The exe's inlined destructor of kKey.
			// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
			NiGlobalStringTable::ReleaseHandle(kKey.GetHandle());
		}
		m_pkGlobalEntry = nullptr;
		m_uiStage = 0;
		m_uiTextureFlags = 0;
		m_usObjTextureFlags = 0;
		m_uiTextureTransformFlags = 0;
		m_bTextureTransform = false;
		ms_pkTextureStagePool->ReleaseObject(this);
	} else {
		this->~NiD3DTextureStage();
		NiMemObject::operator delete(this, sizeof(NiD3DTextureStage));
	}
}

void NiD3DTextureStage::CreateNewStage(NiPointer<NiD3DTextureStage>& aspD3DTextureStage) {
	bool v7;
	uint32_t* p_m_uiUsed = &NiD3DTextureStage::ms_pkTextureStagePool->m_kFreeObjects.m_uiUsed;
	NiTObjectPool<NiD3DTextureStage>* v2 = NiD3DTextureStage::ms_pkTextureStagePool;
	if (!NiD3DTextureStage::ms_pkTextureStagePool->m_kFreeObjects.m_uiUsed) {
		uint32_t* p_m_uiCurrentSize = &NiD3DTextureStage::ms_pkTextureStagePool->m_uiCurrentSize;
		NiD3DTextureStage::ms_pkTextureStagePool
			->CreateNewObjects(NiD3DTextureStage::ms_pkTextureStagePool->m_uiCurrentSize);
		*p_m_uiCurrentSize *= 2;
	}
	NiD3DTextureStage** m_pBase = v2->m_kFreeObjects.m_pBase;
	NiD3DTextureStage* v5 = *v2->m_kFreeObjects.m_pBase;
	*m_pBase = m_pBase[--*p_m_uiUsed];
	if (v5) {
		NiIncRefCount(v5);
	}
	v5->m_bRendererOwned = 1;
	NiD3DTextureStage* m_pObject = aspD3DTextureStage.m_pObject;
	if (aspD3DTextureStage.m_pObject != v5) {
		if (m_pObject) {
			v7 = m_pObject->m_uiRefCount-- == 1;
			if (v7) {
				m_pObject->ReturnStageToPool();
			}
		}
		aspD3DTextureStage.m_pObject = v5;
		NiIncRefCount(v5);
	}
	v7 = v5->m_uiRefCount-- == 1;
	if (v7) {
		v5->ReturnStageToPool();
	}
}

void NiD3DTextureStage::DisableStage(uint32_t uiStage) {
	uint32_t i;
	for (i = uiStage; i < NiD3DPass::ms_uiMaxTextureBlendStages; ++i) {
		NiD3DTextureStage::ms_pkD3DRenderState->SetTexture(i, 0);
		NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(i, 1u, 1u, 0);
		NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(i, 4u, 1u, 0);
		NiD3DTextureStage::ms_pkD3DRenderState->SetTextureStageState(i, 24u, 0, 0);
	}
	for (; i < NiD3DPass::ms_uiMaxSamplers; ++i) {
		NiD3DTextureStage::ms_pkD3DRenderState->SetTexture(i, 0);
	}
}

// NiNew NiTObjectPool<NiD3DTextureStage>, the pool's constructor inlined: an empty free set, no containers and an
// initial size of 8.
void NiD3DTextureStage::InitializePools() {
	NiTObjectPool<NiD3DTextureStage>* pkPool = static_cast<NiTObjectPool<NiD3DTextureStage>*>(MemoryManager_Singleton
			.Allocate(sizeof(NiTObjectPool<NiD3DTextureStage>)));
	if (pkPool) {
		pkPool->m_kFreeObjects.m_pBase = nullptr;
		pkPool->m_kFreeObjects.m_uiAlloced = 0;
		pkPool->m_kFreeObjects.m_uiUsed = 0;
		pkPool->m_pkContainers = nullptr;
		pkPool->m_uiCurrentSize = 8;
		pkPool->m_uiInitialSize = 8;
	}
	ms_pkTextureStagePool = pkPool;
}

void NiD3DTextureStage::ShutdownPools() {
	NiTObjectPool<NiD3DTextureStage>* v0 = NiD3DTextureStage::ms_pkTextureStagePool;
	if (NiD3DTextureStage::ms_pkTextureStagePool) {
		NiTPoolContainer<NiD3DTextureStage, NiTNewInterface<NiD3DTextureStage>>* m_pkContainers =
			NiD3DTextureStage::ms_pkTextureStagePool->m_pkContainers;
		NiD3DTextureStage::ms_pkTextureStagePool->m_kFreeObjects.m_uiUsed = 0;
		if (m_pkContainers) {
			if (m_pkContainers->m_pkObjectArray) {
				delete[] m_pkContainers->m_pkObjectArray;
			}
			NiTPoolContainer<NiD3DTextureStage, NiTNewInterface<NiD3DTextureStage>>* m_pkNext =
				m_pkContainers->m_pkNext;
			if (m_pkNext) {
				delete m_pkNext;
			}
			MemoryManager_Singleton.Deallocate(m_pkContainers);
		}
		_NiFree(v0->m_kFreeObjects.m_pBase);
		MemoryManager_Singleton.Deallocate(v0);
	}
	NiD3DTextureStage::ms_pkTextureStagePool = 0;
}

// Holds the renderer and a reference on its device, and caches the renderer's render state; null drops both.
void NiD3DTextureStage::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
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

char NiD3DTextureStage::CalculateTransformationMatrix(int32_t a1, _D3DMATRIX* a2) {
	NiDX9Renderer* v2;
	_D3DMATRIX* v3;
	double v4;
	double v5;
	double _43;
	char result;
	_D3DMATRIX* v8;
	NiDX9Renderer* v9;
	double v10;
	double v11;
	switch (a1) {
	case 1: {
		v2 = NiD3DTextureStage::ms_pkD3DRenderer;
		v3 = a2;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_21 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_22 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_32;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._32 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_12 +
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_13 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 =
			a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._33 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_13 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * a2->_21 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * a2->_31 + a2->_41;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * a2->_22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * a2->_32 + a2->_42;
		v4 = a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41;
		v5 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * a2->_23;
		goto LABEL_3;
	}
	case 2: {
		v3 = a2;
		v2 = NiD3DTextureStage::ms_pkD3DRenderer;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_21 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 =
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_12 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_32;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._32 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_12 +
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_13 +
			a2->_23 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 =
			a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._33 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_13 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 * a2->_31 + a2->_41;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * a2->_12 +
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._43 + a2->_42;
		v4 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._42 * a2->_23;
		v5 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._41 * a2->_13;
	LABEL_3:;
		_43 = v4 + v5 + v2->m_kInvView._43 * v3->_33 + v3->_43;
		goto LABEL_4;
	}
	case 3: {
		v8 = a2;
		v9 = NiD3DTextureStage::ms_pkD3DRenderer;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * a2->_21 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_12 +
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_32;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_12 +
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._32 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * a2->_22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_32;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_13 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_13 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_33;
		v10 = a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31;
		v11 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * a2->_23;
		goto LABEL_7;
	}
	case 4: {
		std::memset(&NiD3DTextureStage::ms_kCalculatedTextureTransformation, 0,
			sizeof(NiD3DTextureStage::ms_kCalculatedTextureTransformation));
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 = 0.5f;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 = 0.5f;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = 0.5f;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 = 0.5f;
		return 1;
	}
	case 5: {
		v9 = NiD3DTextureStage::ms_pkD3DRenderer;
		v8 = a2;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_21 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_22 +
			a2->_12 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 =
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_12 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._32 =
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_32;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 =
			a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_13 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_33;
		v10 = a2->_23 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32;
		v11 = a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31;
		goto LABEL_7;
	}
	case 6: {
		v8 = a2;
		v9 = NiD3DTextureStage::ms_pkD3DRenderer;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._11 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._21 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_21 +
			a2->_11 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._31 =
			a2->_21 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_11 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33 * a2->_31;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._12 =
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 * a2->_12 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._22 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_22 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 * a2->_12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_32;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._32 =
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_12 +
			a2->_22 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 +
			a2->_32 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._13 =
			a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._11 +
			a2->_23 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._12 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._13 * a2->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._23 =
			a2->_13 * NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._21 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._22 * a2->_23 +
			NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._23 * a2->_33;
		v10 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._32 * a2->_23;
		v11 = NiD3DTextureStage::ms_pkD3DRenderer->m_kInvView._31 * a2->_13;
	LABEL_7:;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._33 = v10 + v11 + v9->m_kInvView._33 * v8->_33;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._41 = v8->_41;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._42 = v8->_42;
		_43 = v8->_43;
	LABEL_4:;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._43 = _43;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._14 = 0.0f;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._24 = 0.0f;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._34 = 0.0f;
		NiD3DTextureStage::ms_kCalculatedTextureTransformation._44 = 0.0f;
		result = 1;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return result;
}
