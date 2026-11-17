#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiRTTI NiMaterial::ms_RTTI("NiMaterial", nullptr);

// The name takes a reference on its handle, then the material is entered in the map under it.
NiMaterial::NiMaterial(const NiFixedString& kName) {
	m_kMaterialName = kName;
	ms_pkMaterials->SetAt(m_kMaterialName, this);
}

// The name's reference is released by its NiFixedString member.
NiMaterial::~NiMaterial() {
	ms_pkMaterials->RemoveAt(m_kMaterialName);
}

const NiRTTI* NiMaterial::GetRTTI() const {
	return &NiMaterial::ms_RTTI;
}

// The type queries: a material is none of these Gamebryo or Bethesda classes.
NiNode* NiMaterial::IsNode() {
	return nullptr;
}

BSFadeNode* NiMaterial::IsFadeNode() {
	return nullptr;
}

BSMultiBoundNode* NiMaterial::IsMultiBoundNode() {
	return nullptr;
}

NiGeometry* NiMaterial::IsGeometry() {
	return nullptr;
}

NiTriBasedGeom* NiMaterial::IsTriBasedGeom() {
	return nullptr;
}

NiTriStrips* NiMaterial::IsTriStrips() {
	return nullptr;
}

NiTriShape* NiMaterial::IsTriShape() {
	return nullptr;
}

BSSegmentedTriShape* NiMaterial::IsSegmentedTriShape() {
	return nullptr;
}

BSResizableTriShape* NiMaterial::IsResizableTriShape() {
	return nullptr;
}

NiParticles* NiMaterial::IsParticlesGeom() {
	return nullptr;
}

NiLines* NiMaterial::IsLinesGeom() {
	return nullptr;
}

bhkNiCollisionObject* NiMaterial::IsBhkNiCollisionObject() {
	return nullptr;
}

bhkBlendCollisionObject* NiMaterial::IsBhkBlendCollisionObject() {
	return nullptr;
}

bhkRigidBody* NiMaterial::IsBhkRigidBody() const {
	return nullptr;
}

bhkLimitedHingeConstraint* NiMaterial::IsBhkLimitedHingeConstraint() {
	return nullptr;
}

const char* NiMaterial::GetDefaultWorkingDirectory() {
	return ms_acDefaultWorkingDirectory[0] ? ms_acDefaultWorkingDirectory : nullptr;
}

NiMaterial* NiMaterial::GetMaterial(const NiFixedString& kName) {
	const char* pcHandle = kName.GetHandle();
	// kName.GetLength() == 0: the null handle, or an interned string of length 0.
	if (pcHandle == NiGlobalStringTable::NULL_STRING ||
		NiGlobalStringTable::GetStringHeader(pcHandle)->m_uiLength == 0) {
		return nullptr;
	}
	NiMaterial* pkMaterial = nullptr;
	// EXE-INLINED none: NiTFixedStringMapBase::GetAt (the handle's address modulo the bucket count, then the chain)
	NiTMapItem<NiFixedString, NiMaterial*>* pkItem =
		ms_pkMaterials->m_ppkHashTable[reinterpret_cast<uintptr_t>(pcHandle) % ms_pkMaterials->m_uiHashSize];
	while (pkItem) {
		if (pkItem->m_key.GetHandle() == pcHandle) {
			pkMaterial = pkItem->m_val;
			break;
		}
		pkItem = pkItem->m_pkNext;
	}
	return pkMaterial;
}

// Allocates the material map with 37 buckets, then calls SetDefaultWorkingDirectory(NULL).
void NiMaterial::_SDMInit() {
	NiTFixedStringMap<NiMaterial*>* pkMaterials = static_cast<NiTFixedStringMap<NiMaterial*>*>(MemoryManager_Singleton
			.Allocate(sizeof(NiTFixedStringMap<NiMaterial*>)));
	if (pkMaterials != nullptr) {
		::new (pkMaterials) NiTFixedStringMap<NiMaterial*>(37);
		ms_pkMaterials = pkMaterials;
	} else {
		ms_pkMaterials = nullptr;
	}
	ms_acDefaultWorkingDirectory[0] = 0;
}

// The map itself is re-read for every GetNext, and the string's reference is only dropped when the walk ran.
void NiMaterial::UnloadShadersForAllMaterials() {
	NiTFixedStringMap<NiMaterial*>* pkMaterials = ms_pkMaterials;
	uint32_t uiHashSize = pkMaterials->m_uiHashSize;
	NiTMapItem<NiFixedString, NiMaterial*>* pkItem = nullptr;
	if (uiHashSize != 0) {
		uint32_t uiIndex = 0;
		while (pkMaterials->m_ppkHashTable[uiIndex] == nullptr) {
			++uiIndex;
			if (uiIndex >= uiHashSize)
				break;
		}
		if (uiIndex < uiHashSize)
			pkItem = pkMaterials->m_ppkHashTable[uiIndex];
	}

	if (pkItem != nullptr) {
		FNVD_INLINED_LOCAL(NiFixedString, kKey, std::nullopt, std::nullopt);
		// EXE-INLINED none: NiFixedString::NiFixedString
		kKey.SetHandle(NiGlobalStringTable::NULL_STRING);
		NiMaterial* pkMaterial = nullptr;
		do {
			ms_pkMaterials->GetNext(pkItem, kKey, pkMaterial);
			if (pkMaterial != nullptr)
				pkMaterial->UnloadShaders();
		} while (pkItem != nullptr);
		// Drops the key's reference, as the string's destructor would.
		// EXE-INLINED 0x4381B0
		NiGlobalStringTable::ReleaseHandle(kKey.GetHandle());
	}
}

void NiMaterial::_SDMShutdown() {
	delete ms_pkMaterials;
	ms_pkMaterials = nullptr;
}
