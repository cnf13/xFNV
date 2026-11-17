#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9LightManager.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSpotLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

#include <cmath>

float NiDX9LightManager::ms_fMaxRange = static_cast<float>(std::sqrt(3.4028234663852886e+38));

NiDX9LightManager::LightEntry::LightEntry(const NiLight& kLight, int32_t iIndex)
	: m_uiRevID(0), m_iIndex(iIndex), m_bNeedsRemoval(false), m_bActive(false) {
	Update(kLight);
}

// Rebuilds the D3D light from kLight when its revision changed. The effect type says which light class kLight is,
// as the source's casts assume.
bool NiDX9LightManager::LightEntry::Update(const NiLight& kLight) {
	uint32_t uiRevID = kLight.m_uiRevID;
	bool bAttenuation = false;
	if (m_uiRevID != uiRevID) {
		m_uiRevID = uiRevID;
		std::memset(&m_kD3DLight, 0, sizeof(D3DLIGHT9));
		switch (kLight.m_ucEffectType) {
		case NiDynamicEffect::DIR_LIGHT: {
			m_kD3DLight.Type = D3DLIGHT_DIRECTIONAL;
			const NiPoint3& kDir = static_cast<const NiDirectionalLight&>(kLight).m_kWorldDir;
			m_kD3DLight.Direction.x = kDir.x;
			m_kD3DLight.Direction.y = kDir.y;
			m_kD3DLight.Direction.z = kDir.z;
			break;
		}
		case NiDynamicEffect::POINT_LIGHT: {
			m_kD3DLight.Type = D3DLIGHT_POINT;
			const NiPoint3& kPos = kLight.m_kWorld.m_Translate;
			m_kD3DLight.Position.x = kPos.x;
			m_kD3DLight.Position.y = kPos.y;
			m_kD3DLight.Position.z = kPos.z;
			bAttenuation = true;
			break;
		}
		case NiDynamicEffect::SPOT_LIGHT: {
			m_kD3DLight.Type = D3DLIGHT_SPOT;
			const NiSpotLight& kSpot = static_cast<const NiSpotLight&>(kLight);
			const NiPoint3& kPos = kSpot.m_kWorld.m_Translate;
			const NiPoint3& kDir = kSpot.m_kWorldDir;
			m_kD3DLight.Position.x = kPos.x;
			m_kD3DLight.Position.y = kPos.y;
			m_kD3DLight.Position.z = kPos.z;
			m_kD3DLight.Direction.x = kDir.x;
			m_kD3DLight.Direction.y = kDir.y;
			m_kD3DLight.Direction.z = kDir.z;
			m_kD3DLight.Theta = 0.0f;
			// The exe multiplies and divides by doubles (0x101FF40, 0x1066808): NI_PI widened, and 90.
			m_kD3DLight.Phi = kSpot.m_fOuterSpotAngle * 3.1415927410125732 / 90.0;
			m_kD3DLight.Falloff = kSpot.m_fSpotExponent;
			bAttenuation = true;
			break;
		}
		}

		if (bAttenuation) {
			const NiPointLight& kPoint = static_cast<const NiPointLight&>(kLight);
			m_kD3DLight.Attenuation0 = kPoint.m_fAtten0;
			m_kD3DLight.Attenuation1 = kPoint.m_fAtten1;
			m_kD3DLight.Attenuation2 = kPoint.m_fAtten2;
			// Gamebryo's LightEntry::ms_fRangeMax, sqrt(FLT_MAX).
			m_kD3DLight.Range = ms_fMaxRange;
		}

		float fDimmer = kLight.m_fDimmer;
		NiColor kAmbient = kLight.m_kAmb * fDimmer;
		NiColor kDiffuse = kLight.m_kDiff * fDimmer;
		NiColor kSpecular = kLight.m_kSpec * fDimmer;

		m_kD3DLight.Ambient.r = kAmbient.r;
		m_kD3DLight.Ambient.g = kAmbient.g;
		m_kD3DLight.Ambient.b = kAmbient.b;
		m_kD3DLight.Ambient.a = 1.0f;

		m_kD3DLight.Diffuse.r = kDiffuse.r;
		m_kD3DLight.Diffuse.g = kDiffuse.g;
		m_kD3DLight.Diffuse.b = kDiffuse.b;
		m_kD3DLight.Diffuse.a = 1.0f;

		m_kD3DLight.Specular.r = kSpecular.r;
		m_kD3DLight.Specular.g = kSpecular.g;
		m_kD3DLight.Specular.b = kSpecular.b;
		m_kD3DLight.Specular.a = 1.0f;
		return true;
	}
	return false;
}

// Gamebryo's sentinel for "no state set yet" is the all-ones pointer value.
NiDX9LightManager::NiDX9LightManager(NiDX9RenderState* pkRenderState, IDirect3DDevice9* pkD3DDevice9)
	: m_kLights(37), m_pkD3DDevice9(pkD3DDevice9), m_pkRenderState(pkRenderState),
	  m_pkInvalidState(reinterpret_cast<const NiDynamicEffectState*>(static_cast<intptr_t>(-1))),
	  m_uiNextAvailableIndex(0), m_auiValidIndices{} {
	pkD3DDevice9->AddRef();
	InitRenderModes();
}

// Frees every light entry (its light forgets it, its index becomes free) and device light node and releases the
// device; the map and the list go with their own destructors.
NiDX9LightManager::~NiDX9LightManager() {
	while (m_kLights.GetCount()) {
		NiLight* pkLight;
		LightEntry* pkLightEntry;
		NiTMapItem<NiLight*, LightEntry*>* kIter = m_kLights.GetFirstPos();
		m_kLights.GetNext(kIter, pkLight, pkLightEntry);
		m_kLights.RemoveAt(pkLight);
		pkLight->m_pvRendererData = nullptr;
		uint32_t uiIndex = static_cast<uint32_t>(pkLightEntry->m_iIndex);
		m_auiValidIndices[uiIndex >> 5] &= ~(1u << (uiIndex & 31));
		delete pkLightEntry;
	}
	m_kDeviceLights.RemoveAll();
	if (m_pkD3DDevice9) {
		m_pkD3DDevice9->Release();
		m_pkD3DDevice9 = nullptr;
	}
}

void NiDX9LightManager::SetState(const NiDynamicEffectState* pkState, const NiTexturingProperty* pkTex,
	const NiVertexColorProperty* pkColor) {
	uint32_t m_uFlags;
	bool v9;
	NiTexturingProperty::ApplyMode v5 = static_cast<NiTexturingProperty::ApplyMode>(pkTex->m_uFlags >> 1 & 7);
	m_eApply = v5;
	NiTexturingProperty::Map* v6 = *pkTex->m_kMaps.m_pBase;
	if (v6 && (v6->m_spTexture.m_pObject && v5 == NiTexturingProperty::APPLY_REPLACE)) {
		m_pkRenderState->SetRenderState(D3DRS_LIGHTING, 0, 0, 0);
		return;
	}
	NiVertexColorProperty::SourceVertexMode v7 = NiVertexColorProperty::SOURCE_IGNORE;
	if (!pkColor ||
		(m_uFlags = pkColor->m_uFlags,
			(v7 = static_cast<NiVertexColorProperty::SourceVertexMode>(m_uFlags >> 4 & 3), (m_uFlags >> 3 & 1) != 0))) {
		if (v7 == m_eSourceMode) {
		LABEL_20:;
			this->m_bAmbientDiffuse = 1;
			goto LABEL_21;
		}
		// The material source values: 0 is D3DMCS_MATERIAL, 1 D3DMCS_COLOR1.
		if (v7) {
			if (v7 != NiVertexColorProperty::SOURCE_EMISSIVE) {
				if (v7 == NiVertexColorProperty::SOURCE_AMB_DIFF) {
					m_pkRenderState->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0, 0, 0);
					m_pkRenderState->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, 1u, 0, 0);
					m_pkRenderState->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, 1u, 0, 0);
				}
				goto LABEL_19;
			}
			m_pkRenderState->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 1u, 0, 0);
		} else {
			m_pkRenderState->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0, 0, 0);
		}
		m_pkRenderState->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, 0, 0, 0);
		m_pkRenderState->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, 0, 0, 0);
	LABEL_19:;
		m_eSourceMode = v7;
		goto LABEL_20;
	}
	if (v7 && v7 != NiVertexColorProperty::SOURCE_AMB_DIFF) {
		m_pkRenderState->SetRenderState(D3DRS_LIGHTING, 0, 0, 0);
		return;
	}
	v9 = m_eSourceMode == NiVertexColorProperty::SOURCE_EMISSIVE;
	this->m_bAmbientDiffuse = 0;
	if (v9) {
		m_pkRenderState->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0, 0, 0);
		m_pkRenderState->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, 0, 0, 0);
		m_pkRenderState->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, 0, 0, 0);
		m_eSourceMode = NiVertexColorProperty::SOURCE_IGNORE;
	}
LABEL_21:;
	m_pkRenderState->SetRenderState(D3DRS_LIGHTING, 1u, 0, 0);
	const NiDynamicEffectState* m_pkLastState = this->m_pkLastState;
	if (m_pkLastState == m_pkInvalidState ||
		(pkState != m_pkLastState || this->m_bAmbientDiffuse != m_bLastAmbientDiffuse)) {
		UpdateLights(pkState);
		bool m_bAmbientDiffuse = this->m_bAmbientDiffuse;
		this->m_pkLastState = pkState;
		m_bLastAmbientDiffuse = m_bAmbientDiffuse;
	}
}

// Disables every device light enabled during the last frame and empties the device light list.
void NiDX9LightManager::NextFrame() {
	using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
	while (m_kDeviceLights.GetSize()) {
		// EXE-INLINED 0x7B5390
		NiTListItem<NiLight*>* pkNode = m_kDeviceLights.m_pkHead;
		m_kDeviceLights.m_pkHead = pkNode->m_pkNext;
		if (m_kDeviceLights.m_pkHead)
			m_kDeviceLights.m_pkHead->m_pkPrev = nullptr;
		else
			m_kDeviceLights.m_pkTail = nullptr;
		NiLight* pkActive = pkNode->m_element;
		// EXE-INLINED none: NiTPointerListBase<NiLight*>::DeleteItem
		pkNode->m_element = nullptr;
		PoolAllocator::Lock();
		// The freed node is raw pool memory again: the pool links it as an AllocNode, as the exe does.
		PoolAllocator::AllocNode* pkFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pkNode);
		pkFreed->m_pkData = nullptr;
		pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
		PoolAllocator::ms_pkFreeMem = pkFreed;
		PoolAllocator::Unlock();
		m_kDeviceLights.m_uiCount--;

		LightEntry* pkLightEntry = static_cast<LightEntry*>(pkActive->m_pvRendererData);
		m_pkD3DDevice9->LightEnable(pkLightEntry->m_iIndex, 0);
		pkLightEntry->m_bActive = false;
	}

	m_pkLastState = m_pkInvalidState;
}

void NiDX9LightManager::RemoveLight(NiLight* apLight) {
	LightEntry* pkEntry = static_cast<LightEntry*>(apLight->m_pvRendererData);
	// GetAt fills its own copy of the entry pointer (the exe's argument slot); only the second branch reads it.
	LightEntry* pkFound = pkEntry;
	if (pkEntry) {
		m_kLights.GetAt(apLight, pkFound);
		m_pkD3DDevice9->LightEnable(static_cast<uint32_t>(pkEntry->m_iIndex), 0);
		pkEntry->m_bActive = false;
		m_kLights.RemoveAt(apLight);
		m_kDeviceLights.Remove(apLight);
		apLight->m_pvRendererData = nullptr;
		m_auiValidIndices[static_cast<uint32_t>(pkEntry->m_iIndex) >> 5] &= ~(1u << (pkEntry->m_iIndex & 31));
		NiMemObject::operator delete(pkEntry, sizeof(LightEntry));
	} else if (m_kLights.GetAt(apLight, pkFound)) {
		m_kLights.RemoveAt(apLight);
		m_kDeviceLights.Remove(apLight);
		apLight->m_pvRendererData = nullptr;
		m_auiValidIndices[static_cast<uint32_t>(pkFound->m_iIndex) >> 5] &= ~(1u << (pkFound->m_iIndex & 31));
	}
}

// Disables and frees every light entry of the light map and takes its light off the device light list.
void NiDX9LightManager::ReleaseLights() {
	using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
	// EXE-INLINED 0xEE8AC0
	NiTMapItem<NiLight*, LightEntry*>* kPos = nullptr;
	for (uint32_t i = 0; i < m_kLights.m_uiHashSize; ++i) {
		if (m_kLights.m_ppkHashTable[i]) {
			kPos = m_kLights.m_ppkHashTable[i];
			break;
		}
	}
	while (kPos) {
		NiLight* pkLight;
		LightEntry* pkLightEntry = nullptr;
		m_kLights.GetNext(kPos, pkLight, pkLightEntry);

		if (!pkLightEntry)
			continue;

		m_pkD3DDevice9->LightEnable(pkLightEntry->m_iIndex, 0);
		pkLightEntry->m_bActive = false;

		m_kLights.RemoveAt(pkLight);

		// m_kDeviceLights.Remove(pkLight), inlined with its FindPos; only RemoveHead and RemoveTail stay calls.
		// EXE-INLINED 0x57C730
		NiTListItem<NiLight*>* pkHead = m_kDeviceLights.m_pkHead;
		// EXE-INLINED 0x49C680
		NiTListItem<NiLight*>* pkNode = nullptr;
		for (NiTListItem<NiLight*>* pkItem = pkHead; pkItem; pkItem = pkItem->m_pkNext) {
			if (pkLight == pkItem->m_element) {
				pkNode = pkItem;
				break;
			}
		}
		if (pkNode) {
			if (pkNode == pkHead) {
				m_kDeviceLights.RemoveHead();
			} else if (pkNode == m_kDeviceLights.m_pkTail) {
				m_kDeviceLights.RemoveTail();
			} else {
				NiTListItem<NiLight*>* pkPrev = pkNode->m_pkPrev;
				NiTListItem<NiLight*>* pkNext = pkNode->m_pkNext;
				if (pkPrev)
					pkPrev->m_pkNext = pkNext;
				if (pkNext)
					pkNext->m_pkPrev = pkPrev;
				// EXE-INLINED none: NiTPointerListBase<NiLight*>::DeleteItem
				pkNode->m_element = nullptr;
				PoolAllocator::Lock();
				// The freed node is raw pool memory again: the pool links it as an AllocNode, as the exe does.
				PoolAllocator::AllocNode* pkFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pkNode);
				pkFreed->m_pkData = nullptr;
				pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
				PoolAllocator::ms_pkFreeMem = pkFreed;
				PoolAllocator::Unlock();
				m_kDeviceLights.m_uiCount--;
			}
		}

		pkLight->m_pvRendererData = nullptr;

		NiMemObject::operator delete(pkLightEntry, sizeof(LightEntry));
	}
}

void NiDX9LightManager::InitRenderModes() {
	this->m_pkRenderState->SetRenderState(139u, 0, 0, 0);
	this->m_pkRenderState->SetRenderState(137u, 0, 0, 0);
	NiDX9RenderState* m_pkRenderState = this->m_pkRenderState;
	m_eApply = NiTexturingProperty::APPLY_REPLACE;
	m_bAmbientDiffuse = 0;
	m_eSourceMode = NiVertexColorProperty::SOURCE_IGNORE;
	m_pkRenderState->SetRenderState(148u, 0, 0, 0);
	this->m_pkRenderState->SetRenderState(147u, 0, 0, 0);
	this->m_pkRenderState->SetRenderState(145u, 0, 0, 0);
	const NiDynamicEffectState* m_pkInvalidState = this->m_pkInvalidState;
	m_bLastAmbientDiffuse = 0;
	m_pkLastState = m_pkInvalidState;
}

uint32_t NiDX9LightManager::GetNextAvailableIndex() {
	uint32_t result = m_uiNextAvailableIndex;
	while ((1 << (result & 31) & m_auiValidIndices[result >> 5]) != 0) {
		if (++result == 0x1000) {
			result = 0;
		}
		if (result == m_uiNextAvailableIndex) {
			return -1;
		}
	}
	m_auiValidIndices[result >> 5] |= 1 << (result & 31);
	m_uiNextAvailableIndex = static_cast<uint16_t>(result) + 1 & 4095;
	return result;
}

// Gamebryo 2.2 does this inline in SetState; the exe has it out of line (thiscall, ret 4). Marks every device light
// for removal, sets up to eight of the state's lights (building and setting a device light for each new or changed
// one) and sums the ambient lights, disables the device lights no light claimed and enables the new ones, then sets
// the ambient render state.
void NiDX9LightManager::UpdateLights(const NiDynamicEffectState* pkState) {
	float fAmbientR = 0.0f;
	float fAmbientG = 0.0f;
	float fAmbientB = 0.0f;
	NiTPointerList<NiLight*>* pkDeviceLights = &m_kDeviceLights;

	NiTListItem<NiLight*>* kPos = pkDeviceLights->m_pkHead;
	while (kPos) {
		NiLight* pkLight = kPos->m_element;
		kPos = kPos->m_pkNext;
		static_cast<LightEntry*>(pkLight->m_pvRendererData)->m_bNeedsRemoval = true;
	}

	if (m_bAmbientDiffuse) {
		NiDynamicEffectState::ListItem* pkIter = nullptr;
		if (pkState) {
			pkIter = pkState->m_pLightList;
		}
		uint32_t uiCount = 0;
		while (pkIter && uiCount < 8) {
			NiLight* pkLight = static_cast<NiLight*>(pkIter->m_pItem);
			pkIter = pkIter->m_pNext;
			if (!pkLight->m_bOn || pkLight->m_fDimmer < 0.01f) {
				continue;
			}
			if (pkLight->m_ucEffectType == NiDynamicEffect::AMBIENT_LIGHT) {
				float fDimmer = pkLight->m_fDimmer;
				fAmbientR = pkLight->m_kAmb.r * fDimmer + fAmbientR;
				fAmbientG = pkLight->m_kAmb.g * fDimmer + fAmbientG;
				fAmbientB = fDimmer * pkLight->m_kAmb.b + fAmbientB;
				continue;
			}
			LightEntry* pkEntry = static_cast<LightEntry*>(pkLight->m_pvRendererData);
			bool bChanged;
			if (pkEntry) {
				bChanged = pkEntry->Update(*pkLight);
			} else {
				uint32_t uiIndex = GetNextAvailableIndex();
				if (uiIndex == 0xFFFFFFFFu) {
					continue;
				}
				void* pvMemory = NiMemObject::operator new(sizeof(LightEntry));
				if (pvMemory) {
					pkEntry = ::new (pvMemory) LightEntry(*pkLight, static_cast<int32_t>(uiIndex));
				} else {
					pkEntry = nullptr;
				}
				m_kLights.SetAt(pkLight, pkEntry);
				pkLight->m_pvRendererData = pkEntry;
				bChanged = true;
			}
			if (bChanged) {
				m_pkD3DDevice9->SetLight(static_cast<uint32_t>(pkEntry->m_iIndex), &pkEntry->m_kD3DLight);
			}
			if (!pkEntry->m_bActive) {
				pkDeviceLights->AddHead(pkLight);
			}
			pkEntry->m_bNeedsRemoval = false;
			++uiCount;
		}
	}

	kPos = pkDeviceLights->m_pkHead;
	while (kPos) {
		NiTListItem<NiLight*>* pkNode = kPos;
		LightEntry* pkEntry = static_cast<LightEntry*>(pkNode->m_element->m_pvRendererData);
		kPos = pkNode->m_pkNext;
		if (!pkEntry->m_bNeedsRemoval) {
			continue;
		}
		m_pkD3DDevice9->LightEnable(static_cast<uint32_t>(pkEntry->m_iIndex), 0);
		pkEntry->m_bActive = false;
		// RemovePos, inline in the engine but for its RemoveTail call, with RemoveHead, DeleteItem and the pool's
		// Deallocate inlined: the node's memory is the allocator's node.
		// EXE-INLINED 0x49F590
		NiTListItem<NiLight*>* pkHead = pkDeviceLights->m_pkHead;
		if (pkNode == pkHead) {
			NiTListItem<NiLight*>* pkNext = pkHead->m_pkNext;
			pkDeviceLights->m_pkHead = pkNext;
			if (pkNext) {
				pkNext->m_pkPrev = nullptr;
			} else {
				pkDeviceLights->m_pkTail = nullptr;
			}
		} else if (pkNode == pkDeviceLights->m_pkTail) {
			pkDeviceLights->RemoveTail();
			continue;
		} else {
			NiTListItem<NiLight*>* pkPrev = pkNode->m_pkPrev;
			NiTListItem<NiLight*>* pkNext = pkNode->m_pkNext;
			if (pkPrev) {
				pkPrev->m_pkNext = pkNext;
			}
			if (pkNext) {
				pkNext->m_pkPrev = pkPrev;
			}
		}
		pkNode->m_element = nullptr;
		// EXE-INLINED 0x45CEE0
		NiTAbstractPoolAllocator<uint32_t>::AllocNode* pkFree =
			static_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(static_cast<void*>(pkNode));
		NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
		pkFree->m_pkData = nullptr;
		pkFree->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
		bool bUnlocked = NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount-- == 1;
		NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pkFree;
		if (bUnlocked) {
			NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
		}
		--pkDeviceLights->m_uiCount;
	}

	kPos = pkDeviceLights->m_pkHead;
	while (kPos) {
		LightEntry* pkEntry = static_cast<LightEntry*>(kPos->m_element->m_pvRendererData);
		kPos = kPos->m_pkNext;
		if (!pkEntry->m_bNeedsRemoval && !pkEntry->m_bActive) {
			m_pkD3DDevice9->LightEnable(static_cast<uint32_t>(pkEntry->m_iIndex), 1);
			pkEntry->m_bActive = true;
		}
	}

	// Each channel is truncated to a 64-bit integer (fistp qword) and its low dword clamped unsigned to 255.
	uint32_t uiR = static_cast<uint32_t>(static_cast<int64_t>(fAmbientR * 255.0));
	if (uiR > 255) {
		uiR = 255;
	}
	uint32_t uiG = static_cast<uint32_t>(static_cast<int64_t>(fAmbientG * 255.0));
	if (uiG > 255) {
		uiG = 255;
	}
	uint32_t uiB = static_cast<uint32_t>(static_cast<int64_t>(255.0 * fAmbientB));
	if (uiB > 255) {
		uiB = 255;
	}
	m_pkRenderState->SetRenderState(D3DRS_AMBIENT, 0xFF000000u | uiR << 16 | uiG << 8 | uiB, 0, false);
}
