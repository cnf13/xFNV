#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiAVObject::ms_RTTI("NiAVObject", &NiObjectNET::ms_RTTI);
BSSpinLock NiAVObject::kPropertyStateLock;

NiAVObject::NiAVObject()
	: m_pkParent(nullptr), m_spCollisionObject(nullptr), m_kWorldBound(nullptr),
	  m_uFlags(SELECTIVE_UPDATE | SELECTIVE_UPDATE_TRANSFORMS | SELECTIVE_UPDATE_CONTROLLER | HAS_ORPHANED_BOUND) {
	m_kLocal.MakeIdentity();
	m_kWorld.MakeIdentity();
}

// The world bound comes from Bethesda's heap (SetBound); the property list's and the collision object's own
// destructors do the rest.
NiAVObject::~NiAVObject() {
	if (m_kWorldBound) {
		MemoryManager_Singleton.Deallocate(m_kWorldBound);
	}
	m_uFlags &= ~HAS_ORPHANED_BOUND;
	m_kWorldBound = nullptr;
	m_kPropertyList.RemoveAll();
	m_spCollisionObject = nullptr;
}

// The flags (a word before Bethesda header version 0x1C), remapped for old files, the local transform, then the links.
// A pre-5.0.0.19 file carries a velocity, a bound-volume flag and propagation bits instead of the collision object's
// link; they make a collision object here. "NiCollisionSwitch" is compared with the stream's m_acLastErrorMessage
// rather than the last loaded RTTI name, and the final flag update clears 0x10 and sets 0x8000E.
void NiAVObject::LoadBinary(NiStream& kStream) {
	NiObjectNET::LoadBinary(kStream);
	uint32_t uiSize;
	if (kStream.m_BSStreamHeader.uiVersion >= 0x1C) {
		uiSize = 4;
		kStream.m_pkIstr->BinaryRead(&m_uFlags, 4, &uiSize, 1);
	} else {
		uint16_t usFlags;
		uiSize = 2;
		kStream.m_pkIstr->BinaryRead(&usFlags, 2, &uiSize, 1);
		m_uFlags = static_cast<NiFlags>(usFlags);
	}
	if (kStream.m_uiNifFileVersion < NiStream::GetVersion(4, 1, 0, 11)) {
		uint32_t uiFlags = m_uFlags;
		m_uFlags = static_cast<NiFlags>(((uiFlags & 0xFFFFFFF8) * 2) | (static_cast<uint16_t>(uiFlags) & 7));
	}
	if (kStream.m_uiNifFileVersion < NiStream::GetVersion(4, 1, 0, 12)) {
		uint32_t usFlags = static_cast<uint16_t>(m_uFlags);
		uint32_t uiFlags = (static_cast<uint32_t>(m_uFlags) << 4) & 0xFFFFFF00;
		m_uFlags = static_cast<NiFlags>((usFlags & 0xF) | (uiFlags & 0xFFFFFF7F) | 0x70);
	}
	if (kStream.m_uiNifFileVersion < NiStream::GetVersion(5, 0, 0, 1)) {
		uint32_t uiFlags = m_uFlags;
		m_uFlags = static_cast<NiFlags>(((uiFlags & 0xFFFFFF00) * 2) | static_cast<uint8_t>(uiFlags));
	}
	m_kLocal.m_Translate.LoadBinary(kStream);
	m_kLocal.m_Rotate.LoadBinary(kStream);
	uiSize = 4;
	kStream.m_pkIstr->BinaryRead(&m_kLocal.m_fScale, 4, &uiSize, 1);
	if (kStream.m_uiNifFileVersion < NiStream::GetVersion(5, 0, 0, 19)) {
		uint32_t uiFlags = m_uFlags;
		NiPoint3 kVel;
		kVel.x = 0.0f;
		kVel.y = 0.0f;
		kVel.z = 0.0f;
		uint32_t uiPropagationBits = (uiFlags >> 1) & 7;
		m_uFlags = static_cast<NiFlags>(((uiFlags >> 3) & 0xFFFE) | (static_cast<uint16_t>(uiFlags) & 1));
		kVel.LoadBinary(kStream);
		kStream.ReadMultipleLinkIDs();
		uint8_t ucABV;
		if (kStream.m_uiNifFileVersion < NiStream::GetVersion(4, 1, 0, 0)) {
			uint32_t uiBV;
			uiSize = 4;
			kStream.m_pkIstr->BinaryRead(&uiBV, 4, &uiSize, 1);
			ucABV = uiBV != 0;
		} else {
			uiSize = 1;
			kStream.m_pkIstr->BinaryRead(&ucABV, 1, &uiSize, 1);
		}
		if (!strcmp(kStream.m_acLastErrorMessage, "NiCollisionSwitch") && !((m_uFlags >> 3) & 1)) {
			uiPropagationBits = 4;
		}
		if (ucABV || kVel != NiPoint3_ZERO || static_cast<uint16_t>(uiPropagationBits) != 2) {
			NiObject* pkObject = NiStream::CreateObjectByRTTI("NiCollisionData");
			if (pkObject) {
				m_spCollisionObject = static_cast<bhkNiCollisionObject*>(pkObject);
				NiCollisionObject::OldNifInit kInitData;
				kInitData.m_usProgagationBits = static_cast<uint16_t>(uiPropagationBits);
				kInitData.m_bABV = ucABV != 0;
				kInitData.m_pkAVObject = this;
				kInitData.m_pkStream = &kStream;
				NiCollisionObject* pkCollision = m_spCollisionObject.m_pObject;
				pkCollision->Initialize(&kInitData);
			}
		}
	} else {
		kStream.ReadMultipleLinkIDs();
		kStream.ReadLinkID();
	}
	if (kStream.m_uiNifFileVersion < NiStream::GetVersion(10, 0, 1, 2)) {
		kStream.m_usNiAVObjectFlags = static_cast<uint16_t>(m_uFlags);
		m_uFlags = static_cast<NiFlags>(m_uFlags & 0x3F);
	}
	uint32_t uiVersion = kStream.m_uiNifFileVersion;
	if (uiVersion < NiStream::GetVersion(10, 0, 1, 6)) {
		NiCollisionObject* pkCollision = m_spCollisionObject.m_pObject;
		if (pkCollision) {
			pkCollision->Convert(uiVersion, nullptr);
		}
	}
	if (kStream.m_uiNifFileVersion < NiStream::GetVersion(20, 0, 0, 4)) {
		m_uFlags = static_cast<NiFlags>(m_uFlags & ~DISABLE_SORTING);
	}
	m_uFlags = static_cast<NiFlags>((m_uFlags & ~SELECTIVE_UPDATE_RIGID) | 0x8000E);
}

// KEEP: niread: before Bethesda header version 0x1C the exe reads the 2-byte flags into the stream argument's own
// slot, so a short read (malformed data) leaves the low word of the stream's address as m_uFlags; from 4.1.0.0 to
// 5.0.0.19 the 1-byte bound-volume flag is read into the low byte of that same slot. src cannot express either;
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiAVObject::LinkObject(NiStream& kStream) {
	NiObjectNET::LinkObject(kStream);
	uint32_t uiLinks = kStream.GetNumberOfLinkIDs();
	if (kStream.m_uiNifFileVersion < 0x04010008) {
		while (uiLinks) {
			--uiLinks;
			NiProperty* pkProperty = static_cast<NiProperty*>(kStream.GetObjectFromLinkID());
			NiPointer<NiProperty> spProperty;
			spProperty.m_pObject = pkProperty;
			if (pkProperty) {
				NiIncRefCount(pkProperty);
			}
			m_kPropertyList.AddTail(spProperty);
		}
		return;
	}
	while (uiLinks) {
		--uiLinks;
		NiProperty* pkProperty = static_cast<NiProperty*>(kStream.GetObjectFromLinkID());
		NiPointer<NiProperty> spProperty;
		spProperty.m_pObject = pkProperty;
		if (pkProperty) {
			NiIncRefCount(pkProperty);
		}
		m_kPropertyList.AddHead(spProperty);
	}
	if (kStream.m_uiNifFileVersion >= 0x05000013) {
		m_spCollisionObject = static_cast<bhkNiCollisionObject*>(kStream.GetObjectFromLinkID());
		NiCollisionObject* pkCollisionObject = m_spCollisionObject.m_pObject;
		if (pkCollisionObject) {
			pkCollisionObject->Convert(kStream.m_uiNifFileVersion, nullptr);
		}
	}
}

// Registers the base, each property (the next item is read before the call), then the collision object.
bool NiAVObject::RegisterStreamables(NiStream& kStream) {
	if (!NiObjectNET::RegisterStreamables(kStream)) {
		return false;
	}
	const NiTListItem<NiPointer<NiProperty>>* pkItem = m_kPropertyList.m_pkHead;
	while (pkItem) {
		NiProperty* pkProperty = pkItem->m_element.m_pObject;
		pkItem = pkItem->m_pkNext;
		if (pkProperty) {
			pkProperty->RegisterStreamables(kStream);
		}
	}
	if (m_spCollisionObject.m_pObject) {
		m_spCollisionObject.m_pObject->RegisterStreamables(kStream);
	}
	return true;
}

// The flags, the local transform, then the property links last to first (copied into a scratch array) and the
// collision object's link. The property count is written from the stream argument's slot as a signed int.
void NiAVObject::SaveBinary(NiStream& kStream) {
	NiObjectNET::SaveBinary(kStream);
	uint32_t uiSize = 4;
	kStream.m_pkOstr->BinaryWrite(&m_uFlags, 4, &uiSize, 1);
	m_kLocal.m_Translate.SaveBinary(kStream);
	m_kLocal.m_Rotate.SaveBinary(kStream);
	uiSize = 4;
	kStream.m_pkOstr->BinaryWrite(&m_kLocal.m_fScale, 4, &uiSize, 1);
	int32_t iCount = static_cast<int32_t>(m_kPropertyList.m_uiCount);
	uiSize = 4;
	kStream.m_pkOstr->BinaryWrite(&iCount, 4, &uiSize, 1);
	if (iCount > 0) {
		NiProperty** ppkProperties = static_cast<NiProperty**>(_NiMalloc(sizeof(NiProperty*) * iCount));
		NiProperty** ppkNext = ppkProperties;
		for (NiTListItem<NiPointer<NiProperty>>* pkItem = m_kPropertyList.m_pkHead; pkItem;) {
			NiProperty* pkProperty = pkItem->m_element.m_pObject;
			pkItem = pkItem->m_pkNext;
			*ppkNext++ = pkProperty;
		}
		for (int32_t i = iCount - 1; i >= 0; --i) {
			kStream.SaveLinkID(ppkProperties[i]);
		}
		_NiFree(ppkProperties);
	}
	kStream.SaveLinkID(m_spCollisionObject.m_pObject);
}

// The base, the app-culled flag, the local transform, the properties pairwise in list order, then the collision
// objects.
bool NiAVObject::IsEqual(NiObject* pkObject) {
	if (!NiObjectNET::IsEqual(pkObject)) {
		return false;
	}
	NiAVObject* pkAVObject = static_cast<NiAVObject*>(pkObject);
	if ((static_cast<uint8_t>(m_uFlags) ^ static_cast<uint8_t>(pkAVObject->m_uFlags)) & APP_CULLED) {
		return false;
	}
	if (m_kLocal != pkAVObject->m_kLocal) {
		return false;
	}
	uint32_t uiCount = m_kPropertyList.m_uiCount;
	if (uiCount != pkAVObject->m_kPropertyList.m_uiCount) {
		return false;
	}
	if (uiCount) {
		NiTListItem<NiPointer<NiProperty>>* pkItem0 = m_kPropertyList.m_pkHead;
		NiTListItem<NiPointer<NiProperty>>* pkItem1 = pkAVObject->m_kPropertyList.m_pkHead;
		while (pkItem0) {
			NiProperty* pkProperty0 = pkItem0->m_element.m_pObject;
			pkItem0 = pkItem0->m_pkNext;
			NiProperty* pkProperty1 = pkItem1->m_element.m_pObject;
			pkItem1 = pkItem1->m_pkNext;
			if (pkProperty0) {
				if (!pkProperty1 || !pkProperty0->IsEqual(pkProperty1)) {
					return false;
				}
			} else if (pkProperty1) {
				return false;
			}
		}
	}
	NiCollisionObject* pkCO0 = m_spCollisionObject.m_pObject;
	NiCollisionObject* pkCO1 = pkAVObject->m_spCollisionObject.m_pObject;
	if (pkCO0 && pkCO1) {
		if (!pkCO0->IsEqual(pkCO1)) {
			return false;
		}
	} else if (pkCO0 != pkCO1) {
		return false;
	}
	return true;
}

void NiAVObject::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObjectNET::GetViewerStrings(pkStrings);
	char* pcString = NiGetViewerString(NiAVObject::ms_RTTI.GetName());
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("m_bAppCulled", (m_uFlags & APP_CULLED) != 0);
	pkStrings->Add(pcString);
	if ((m_uFlags & ALWAYS_DRAW) != 0) {
		pcString = NiGetViewerString("AlwaysDraw", true);
		pkStrings->Add(pcString);
	}
	pcString = m_kLocal.m_Translate.GetViewerString("m_localTranslate");
	pkStrings->Add(pcString);
	pcString = m_kLocal.m_Rotate.GetViewerString("m_localRotate");
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("m_fLocalScale", m_kLocal.m_fScale);
	pkStrings->Add(pcString);
	pcString = m_kWorld.m_Translate.GetViewerString("m_worldTranslate");
	pkStrings->Add(pcString);
	pcString = m_kWorld.m_Rotate.GetViewerString("m_worldRotate");
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("m_worldScale", m_kWorld.m_fScale);
	pkStrings->Add(pcString);
	NiBound* pkWorldBound = m_kWorldBound;
	if (!pkWorldBound) {
		pkWorldBound = &NiAVObject::NullBoundS;
	}
	pcString = pkWorldBound->GetViewerString("WorldBound");
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("SelUpdate", (m_uFlags & SELECTIVE_UPDATE) != 0);
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("SelUpdateTransforms", (m_uFlags & SELECTIVE_UPDATE_TRANSFORMS) != 0);
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("SelUpdateRigid", (m_uFlags & SELECTIVE_UPDATE_RIGID) != 0);
	pkStrings->Add(pcString);
	pcString = NiGetViewerString("SelUpdatePropControllers", (m_uFlags & SELECTIVE_UPDATE_CONTROLLER) != 0);
	pkStrings->Add(pcString);
	if (m_kPropertyList.m_uiCount) {
		NiTListItem<NiPointer<NiProperty>>* pkItem = m_kPropertyList.m_pkHead;
		while (pkItem) {
			NiProperty* pkProperty = pkItem->m_element.m_pObject;
			pkItem = pkItem->m_pkNext;
			const NiRTTI* pkRTTI = pkProperty->GetRTTI();
			char* pcProperty = NiGetViewerString(pkRTTI->GetName(), static_cast<const void*>(pkProperty));
			uint32_t uiPropIndex = pkStrings->m_usSize;
			if (uiPropIndex >= pkStrings->m_usMaxSize) {
				pkStrings->SetSize(uiPropIndex + pkStrings->m_usGrowBy);
			}
			if (uiPropIndex < pkStrings->m_usSize) {
				if (pcProperty) {
					if (!pkStrings->m_pBase[uiPropIndex]) {
						++pkStrings->m_usESize;
					}
				} else {
					if (pkStrings->m_pBase[uiPropIndex]) {
						--pkStrings->m_usESize;
					}
				}
			} else {
				pkStrings->m_usSize = static_cast<uint16_t>(uiPropIndex + 1);
				if (pcProperty) {
					++pkStrings->m_usESize;
				}
			}
			pkStrings->m_pBase[uiPropIndex] = pcProperty;
		}
	}
	if (m_spCollisionObject.m_pObject) {
		pcString = NiGetViewerString("m_spCollisionObject", static_cast<const void*>(m_spCollisionObject.m_pObject));
		pkStrings->Add(pcString);
	}
	if ((m_uFlags & NO_DECALS) != 0) {
		pcString = NiGetViewerString("No Decals", true);
		pkStrings->Add(pcString);
	}
	if ((m_uFlags & FIXED_BOUND) != 0) {
		pcString = NiGetViewerString("Fixed Bound", true);
		pkStrings->Add(pcString);
	}
}

void NiAVObject::ProcessClone(NiCloningProcess& kCloning) {
	NiObjectNET::ProcessClone(kCloning);
	if (m_kPropertyList.m_uiCount) {
		ProcessPropertyClones(&m_kPropertyList, kCloning);
	}
	NiCollisionObject* pkCollisionObject = m_spCollisionObject.m_pObject;
	if (pkCollisionObject) {
		pkCollisionObject->ProcessClone(kCloning);
	}
}

// Frees through the global memory manager without calling Instance, and sets or clears HAS_ORPHANED_BOUND directly
// rather than calling SetBit.
void NiAVObject::SetBound(BSNiBound* apBound) {
	if (m_kWorldBound) {
		MemoryManager_Singleton.Deallocate(m_kWorldBound);
	}
	m_kWorldBound = apBound;
	if (apBound) {
		m_uFlags = NiFlags(m_uFlags | HAS_ORPHANED_BOUND);
	} else {
		m_uFlags = NiFlags(m_uFlags & ~HAS_ORPHANED_BOUND);
	}
}

void NiAVObject::UpdateControllers(NiUpdateData& UpdateData) {
	UpdateObjectControllers(UpdateData, true);
}

NiAVObject* NiAVObject::GetObjectByName(const NiFixedString& kName) {
	if (kName.GetHandle() == NiGlobalStringTable::NULL_STRING) {
		return nullptr;
	}
	return m_kName.GetHandle() != kName.GetHandle() ? nullptr : this;
}

void NiAVObject::SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& abRigid) {
	NiTimeController* pkController = m_spControllers;
	while (pkController) {
		if (pkController->IsTransformController()) {
			break;
		}
		pkController = pkController->m_spNext.m_pObject;
	}
	const bool bRigid = !pkController && !(m_uFlags & SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE);
	abRigid = bRigid;
	// The flag is tested at its low byte only.
	const bool bUpdateTransforms = static_cast<uint8_t>(bSelectiveUpdateTransforms) ? true : !bRigid;
	bool bPropertyController = false;
	auto* pkItem = m_kPropertyList.m_pkHead;
	while (pkItem) {
		NiProperty* pkProperty = pkItem->m_element.m_pObject;
		pkItem = pkItem->m_pkNext;
		if (pkProperty && pkProperty->m_spControllers) {
			bPropertyController = true;
			break;
		}
	}
	if (!bPropertyController && !bUpdateTransforms && !m_spControllers) {
		bSelectiveUpdate = false;
		m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE);
		m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE_TRANSFORMS);
	} else {
		bSelectiveUpdate = true;
		m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE);
		if (bUpdateTransforms) {
			m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_TRANSFORMS);
		} else {
			m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE_TRANSFORMS);
		}
	}
	if (bPropertyController) {
		m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_CONTROLLER | SELECTIVE_UPDATE_RIGID);
	} else {
		m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE_CONTROLLER);
		m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_RIGID);
	}
}

void NiAVObject::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t) {
	if (UpdateData.bUpdateControllers) {
		UpdateObjectControllers(UpdateData, true);
	}
	UpdateWorldData(UpdateData);
	UpdateWorldBound();
}

void NiAVObject::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t) {
	bool bUpdateControllers = UpdateData.bUpdateControllers;
	UpdateData.bUpdateControllers = (m_uFlags & SELECTIVE_UPDATE_CONTROLLER) != 0;
	UpdateObjectControllers(UpdateData, true);
	UpdateData.bUpdateControllers = bUpdateControllers;
	if ((m_uFlags & SELECTIVE_UPDATE_TRANSFORMS) && !UpdateData.bParallelUpdate) {
		UpdateWorldData(UpdateData);
		UpdateWorldBound();
	}
}

void NiAVObject::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t) {
	bool bUpdateControllers = UpdateData.bUpdateControllers;
	UpdateData.bUpdateControllers = (m_uFlags & SELECTIVE_UPDATE_CONTROLLER) != 0;
	UpdateObjectControllers(UpdateData, true);
	UpdateData.bUpdateControllers = bUpdateControllers;
	if ((m_uFlags & SELECTIVE_UPDATE_TRANSFORMS) && !UpdateData.bParallelUpdate) {
		UpdateWorldData(UpdateData);
		UpdateWorldBound();
	}
}

// SetBit is applied here as a 32-bit or/and on m_uFlags, which rewrites all four bytes.
void NiAVObject::UpdatePropertiesDownward(NiPropertyState*) {
	uint32_t& ruFlags = m_uFlags;
	for (const NiTListItem<NiPointer<NiProperty>>* pItem = m_kPropertyList.m_pkHead; pItem; pItem = pItem->m_pkNext) {
		const NiProperty* pProperty = pItem->m_element.m_pObject;
		if (pProperty && pProperty->m_spControllers) {
			ruFlags = NiFlags(ruFlags | HAS_PROPERTY_CONTROLLER);
			return;
		}
	}
	ruFlags = NiFlags(ruFlags & ~HAS_PROPERTY_CONTROLLER);
}

void NiAVObject::UpdateWorldData(NiUpdateData& UpdateData) {
	if (m_spCollisionObject.m_pObject) {
		m_spCollisionObject.m_pObject->UpdateWorldData(UpdateData);
	} else {
		UpdateWorldLoc();
	}
}

void NiAVObject::UpdateTransformAndBounds(NiUpdateData& UpdateData) {
	if (m_uFlags & SELECTIVE_UPDATE_TRANSFORMS) {
		UpdateWorldData(UpdateData);
		UpdateWorldBound();
	}
}

// Updates the object as if it already hung under apEventualParent.
void NiAVObject::PreAttachUpdate(NiNode* pEventualParent, NiUpdateData& UpdateData) {
	m_pkParent = pEventualParent;
	UpdateDownwardPass(UpdateData, 0);
	m_pkParent = nullptr;
}

void NiAVObject::PreAttachUpdateProperties(NiNode* pEventualParent) {
	m_pkParent = pEventualParent;
	UpdateProperties();
	m_pkParent = nullptr;
}

void NiAVObject::PreAttachUpdateEffects(NiNode* pEventualParent) {
	m_pkParent = nullptr;
}

void NiAVObject::PostAttachUpdate() {
	m_pkParent->UpdateUpwardPass();
}

void NiAVObject::PurgeRendererData(NiRenderer* apkRenderer) {
	NiProperty* property = GetProperty(2);
	if (property) {
		static_cast<NiDX9Renderer*>(apkRenderer)->PurgeMaterial(static_cast<NiMaterialProperty*>(property));
	}
}

NiNode* NiAVObject::GetParent() const {
	return m_pkParent;
}

const NiPoint3& NiAVObject::GetTranslate() {
	return m_kLocal.m_Translate;
}

const NiPoint3& NiAVObject::GetWorldTranslate() const {
	return m_kWorld.m_Translate;
}

const NiTransform& NiAVObject::GetLocalTransform() {
	return m_kLocal;
}

void NiAVObject::SetTranslate(const NiPoint3& akTrn) {
	m_kLocal.m_Translate = akTrn;
}

void NiAVObject::SetTranslate(float afX, float afY, float afZ) {
	m_kLocal.m_Translate = NiPoint3(afX, afY, afZ);
}

void NiAVObject::SetWorldTranslate(const NiPoint3& kTrn) {
	m_kWorld.m_Translate = kTrn;
}

void NiAVObject::SetRotate(const NiMatrix3& akRot) {
	m_kLocal.m_Rotate = akRot;
}

void NiAVObject::SetRotate(float fAngle, float x, float y, float z) {
	(&m_kLocal.m_Rotate)->MakeRotation(fAngle, x, y, z);
}

void NiAVObject::SetWorldRotate(const NiMatrix3& kRot) {
	m_kWorld.m_Rotate = kRot;
}

void NiAVObject::SetScale(float fScale) {
	m_kLocal.m_fScale = NiAbs(fScale);
}

uint32_t NiAVObject::GetFlag(NiFlags auiMask) const {
	return (m_uFlags & auiMask) != 0;
}

void NiAVObject::SetBit(bool bVal, uint32_t uMask) {
	if (bVal) {
		m_uFlags = NiFlags(m_uFlags | uMask);
	} else {
		m_uFlags = NiFlags(m_uFlags & ~uMask);
	}
}

bool NiAVObject::GetAppCulled() const {
	return GetFlag(APP_CULLED);
}

void NiAVObject::SetAppCulled(bool bAppCulled) {
	SetBit(bAppCulled, APP_CULLED);
}

uint32_t NiAVObject::GetSelectiveUpdate() const {
	return GetFlag(SELECTIVE_UPDATE);
}

void NiAVObject::SetSelectiveUpdate(bool bUpdate) {
	SetBit(bUpdate, SELECTIVE_UPDATE);
}

void NiAVObject::SetSelectiveUpdateTransforms(bool bUpdate) {
	SetBit(bUpdate, NiAVObject::SELECTIVE_UPDATE_TRANSFORMS);
}

void NiAVObject::SetSelectiveUpdateRigid(bool bRigid) {
	SetBit(bRigid, NiAVObject::SELECTIVE_UPDATE_RIGID);
}

uint32_t NiAVObject::GetWorldBoundChange() const {
	return GetFlag(NiAVObject::WORLD_BOUND_CHANGE);
}

void NiAVObject::SetWorldBoundChange(bool abBit) {
	SetBit(abBit, NiAVObject::WORLD_BOUND_CHANGE);
}

uint32_t NiAVObject::QHasPropController() const {
	return GetFlag(HAS_PROPERTY_CONTROLLER);
}

uint32_t NiAVObject::GetNotVisible() const {
	return GetFlag(NOT_VISIBLE);
}

uint32_t NiAVObject::GetHighDetail() const {
	return GetFlag(HIGH_DETAIL);
}

void NiAVObject::SetDisplayObject(bool bDisplay) {
	SetBit(bDisplay, DISPLAY_OBJECT);
}

void NiAVObject::SetFadedIn(bool abFaded) {
	SetBit(abFaded, FADED_IN);
}

void NiAVObject::SetFixedBound(bool abTF) {
	SetBit(abTF, FIXED_BOUND);
}

void NiAVObject::SetPickOff(bool abVal) {
	SetBit(abVal, IGNORES_PICKING);
}

void NiAVObject::SetPlayerBone(bool abVal) {
	SetBit(abVal, PLAYER_BONE);
}

void NiAVObject::SetShadowReceiver(bool abShadow) {
	SetBit(!abShadow, NO_SHADOWS);
}

bool NiAVObject::HasPropertyController() const {
	for (const NiTListItem<NiPointer<NiProperty>>* pItem = m_kPropertyList.m_pkHead; pItem; pItem = pItem->m_pkNext) {
		const NiProperty* pProperty = pItem->m_element.m_pObject;
		if (pProperty && pProperty->m_spControllers) {
			return true;
		}
	}
	return false;
}

bool NiAVObject::HasTransformController() {
	NiTimeController* pkController = m_spControllers;
	while (pkController) {
		if (pkController->IsTransformController()) {
			return true;
		}
		pkController = pkController->m_spNext.m_pObject;
	}
	return false;
}

NiProperty* NiAVObject::GetProperty(int32_t iType) const {
	if (iType >= 7) {
		return nullptr;
	}
	for (const NiTListItem<NiPointer<NiProperty>>* pItem = m_kPropertyList.m_pkHead; pItem;) {
		NiProperty* pProperty = pItem->m_element.m_pObject;
		pItem = pItem->m_pkNext;
		if (pProperty && pProperty->Type() == uint32_t(iType)) {
			return pProperty;
		}
	}
	return nullptr;
}

int32_t NiAVObject::IsVisualObject() {
	return (int32_t)((uint8_t)(m_kWorldBound && (m_kWorldBound->m_fRadius != 0.0)));
}

NiTPointerList<NiPointer<NiProperty>>& NiAVObject::GetPropertyList() {
	return m_kPropertyList;
}

void NiAVObject::RemoveProperty(int32_t aiType) {
	NiPointer<NiProperty> spProperty;
	RemoveProperty(aiType, spProperty);
}

void NiAVObject::RemoveProperty(int32_t aiType, NiPointer<NiProperty>& aspProperty) {
	if (aiType < static_cast<int32_t>(NiProperty::MAX_TYPES)) {
		auto* pkItem = m_kPropertyList.m_pkHead;
		while (pkItem) {
			NiProperty* pkProperty = pkItem->m_element.m_pObject;
			pkItem = pkItem->m_pkNext;
			NiPointer<NiProperty> spProperty;
			spProperty.m_pObject = pkProperty;
			if (!pkProperty) {
				continue;
			}
			NiIncRefCount(pkProperty);
			if (pkProperty->Type() == static_cast<uint32_t>(aiType)) {
				{
					NiPointer<NiProperty> spRemoved = m_kPropertyList.Remove(spProperty);
				}
				aspProperty = spProperty;
				return;
			}
		}
	}
	aspProperty = nullptr;
}

void NiAVObject::UpdateProperties() {
	NiPropertyState* pkState = nullptr;
	kPropertyStateLock.Lock(nullptr);
	NiNode* pkParent = m_pkParent;
	if (pkParent) {
		pkParent->UpdatePropertiesUpward(pkState);
	} else {
		void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiPropertyState));
		pkState = pvMemory ? ::new (pvMemory) NiPropertyState : nullptr;
	}
	NiAVObject* pkObject = this;
	pkObject->UpdatePropertiesDownward(pkState);
	NiPropertyState* pkOldState = pkState;
	if (pkState) {
		pkState->~NiPropertyState();
		MemoryManager_Singleton.Deallocate(pkOldState);
	}
	if (!--kPropertyStateLock.uiLockCount) {
		kPropertyStateLock.uiOwningThread = 0;
	}
}

const NiBound& NiAVObject::GetWorldBound() {
	if (m_kWorldBound)
		return *m_kWorldBound;
	return NullBoundS;
}

NiCollisionObject* NiAVObject::GetCollisionObject() const {
	return m_spCollisionObject.m_pObject;
}

void NiAVObject::Cull(NiCullingProcess& kCuller) {
	if (!(m_uFlags & APP_CULLED)) {
		kCuller.Process(this);
	}
}

void NiAVObject::AttachParent(NiNode* pkParent) {
	if (m_pkParent) {
		m_pkParent->DetachChild(this);
	}
	m_pkParent = pkParent;
}

void NiAVObject::DetachParent() {
	m_pkParent = nullptr;
}

void NiAVObject::Update(NiUpdateData& UpdateData) {
	UpdateDownwardPass(UpdateData, 0);
	if (m_pkParent) {
		m_pkParent->UpdateUpwardPass();
	}
}

void NiAVObject::UpdateSelected(NiUpdateData& UpdateData) {
	UpdateSelectedDownwardPass(UpdateData, 0);
	if (m_pkParent) {
		m_pkParent->UpdateUpwardPass();
	}
}

void NiAVObject::SetAlwaysDraw(bool abVal) {
	SetBit(abVal, ALWAYS_DRAW);
	CreateBoundIfNone();
	if (abVal) {
		m_kWorldBound->SetRadius(1.0f);
	}
}

void NiAVObject::CopyMembers(NiAVObject* pkDest, NiCloningProcess& kCloning) {
	NiObjectNET::CopyMembers(pkDest, kCloning);
	pkDest->m_uFlags = NiFlags(m_uFlags | HAS_ORPHANED_BOUND);
	pkDest->m_kLocal = m_kLocal;
	if (m_kPropertyList.m_uiCount) {
		CloneProperties(pkDest, &m_kPropertyList, kCloning);
	}
	NiCollisionObject* pkCollisionObject = m_spCollisionObject.m_pObject;
	if (pkCollisionObject) {
		NiObject* pkClone = pkCollisionObject->CreateClone(kCloning);
		pkDest->SetCollisionObject(static_cast<NiCollisionObject*>(pkClone));
	}
}

void NiAVObject::ProcessPropertyClones(NiTPointerList<NiPointer<NiProperty>>* pkList, NiCloningProcess& kCloning) {
	auto* pkItem = pkList->m_pkTail;
	while (pkItem) {
		NiProperty* pkProperty = pkItem->m_element.m_pObject;
		pkItem = pkItem->m_pkPrev;
		bool bProcessed;
		if (!kCloning.m_pkProcessMap->GetAt(pkProperty, bProcessed)) {
			pkProperty->ProcessClone(kCloning);
		}
	}
}

void NiAVObject::PushLocalProperties(NiPropertyState* pkParentState, bool bCopyOnChange,
	NiPropertyState*& aspPropertyState) {
	if (!m_kPropertyList.m_uiCount) {
		aspPropertyState = pkParentState;
		return;
	}
	kPropertyStateLock.Lock(nullptr);
	NiPropertyState* pkState = pkParentState;
	if (bCopyOnChange) {
		void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiPropertyState));
		pkState = pvMemory ? ::new (pvMemory) NiPropertyState(*pkParentState) : nullptr;
	}
	auto* pkItem = m_kPropertyList.m_pkHead;
	while (pkItem) {
		NiProperty* pkProperty = pkItem->m_element.m_pObject;
		pkItem = pkItem->m_pkNext;
		if (pkProperty && static_cast<int32_t>(pkProperty->Type()) <= static_cast<int32_t>(NiProperty::MAX_TYPES)) {
			NiProperty** ppkSlot = &pkState->m_aspProps[pkProperty->Type()].m_pObject;
			NiProperty* pkOld = *ppkSlot;
			if (pkOld != pkProperty) {
				if (pkOld) {
					NiDecRefCount(pkOld);
				}
				*ppkSlot = pkProperty;
				NiIncRefCount(pkProperty);
			}
		}
	}
	if (!--kPropertyStateLock.uiLockCount) {
		kPropertyStateLock.uiOwningThread = 0;
	}
	aspPropertyState = pkState;
}

uint32_t NiAVObject::QHasMovingSound() {
	return GetFlag(NiAVObject::HAS_MOVING_SOUND);
}

uint32_t NiAVObject::GetIgnoreFade() {
	return GetFlag(NiAVObject::IGNORE_FADE);
}

uint32_t NiAVObject::QFadedIn() {
	return GetFlag(NiAVObject::FADED_IN);
}

uint32_t NiAVObject::GetAlwaysDraw() {
	return GetFlag(NiAVObject::ALWAYS_DRAW);
}

void NiAVObject::SetWorldScale(float fScale) {
	m_kWorld.m_fScale = fScale;
}

void NiAVObject::GetRotate(NiQuaternion& kQuat) const {
	(&kQuat)->FromRotation(m_kLocal.m_Rotate);
}

void NiAVObject::SetNotVisible(bool abTF) {
	SetBit(abTF, NiAVObject::NOT_VISIBLE);
}

void NiAVObject::CloneProperties(NiAVObject* pkDest, NiTPointerList<NiPointer<NiProperty>>* pkList,
	NiCloningProcess& kCloning) {
	kPropertyStateLock.Lock(nullptr);
	auto* pkItem = pkList->m_pkTail;
	while (pkItem) {
		NiProperty* pkProperty = pkItem->m_element.m_pObject;
		pkItem = pkItem->m_pkPrev;
		if (pkProperty && static_cast<int32_t>(pkProperty->Type()) < static_cast<int32_t>(NiProperty::MAX_TYPES)) {
			NiProperty* pkClone = static_cast<NiProperty*>(pkProperty->CreateSharedClone(kCloning));
			if (static_cast<int32_t>(pkClone->Type()) < static_cast<int32_t>(NiProperty::MAX_TYPES)) {
				NiPointer<NiProperty> spClone;
				spClone.m_pObject = pkClone;
				NiIncRefCount(pkClone);
				pkDest->m_kPropertyList.AddHead(spClone);
			}
		}
	}
	if (!--kPropertyStateLock.uiLockCount) {
		kPropertyStateLock.uiOwningThread = 0;
	}
}
