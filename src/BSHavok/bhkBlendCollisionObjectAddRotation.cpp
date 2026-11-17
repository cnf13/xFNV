#include "BSHavok/bhkBlendCollisionObjectAddRotation.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI bhkBlendCollisionObjectAddRotation::ms_RTTI("bhkBlendCollisionObjectAddRotation",
	&bhkBlendCollisionObject::ms_RTTI);

bhkBlendCollisionObjectAddRotation::bhkBlendCollisionObjectAddRotation() : AdditionalRotation(NiMatrix3_IDENTITY) {}

bhkBlendCollisionObjectAddRotation::~bhkBlendCollisionObjectAddRotation() {}

bool bhkBlendCollisionObjectAddRotation::RegisterStreamables(NiStream& arStream) {
	return bhkBlendCollisionObject::RegisterStreamables(arStream);
}

// A new object charged to the Havok heap context, then the copied members and this object's additional rotation.
NiObject* bhkBlendCollisionObjectAddRotation::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	bhkBlendCollisionObjectAddRotation* pClone = static_cast<
		bhkBlendCollisionObjectAddRotation*>(NiMemObject::operator new(sizeof(bhkBlendCollisionObjectAddRotation)));
	if (pClone)
		::new (pClone) bhkBlendCollisionObjectAddRotation;
	CopyMembers(pClone, arCloneData);
	__builtin_memcpy(&pClone->AdditionalRotation, &AdditionalRotation, sizeof(pClone->AdditionalRotation));
	return pClone;
}

void bhkBlendCollisionObjectAddRotation::AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	TLSData* v3 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v3->eMemContext;
	v3->eMemContext = MEM_CONTEXT_HAVOK;
	bhkBlendCollisionObject::AddViewerStrings(apStrings);
	char* ViewerString = NiGetViewerString(bhkBlendCollisionObjectAddRotation::ms_RTTI.GetName());
	uint32_t m_usSize = apStrings->m_usSize;
	uint32_t m_usMaxSize = apStrings->m_usMaxSize;
	char* v12 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(m_usSize, v12);
	char* v8 = AdditionalRotation.GetViewerString("AdditionalRotation");
	uint32_t v9 = apStrings->m_usSize;
	uint32_t v10 = apStrings->m_usMaxSize;
	v12 = v8;
	if (v9 >= v10) {
		apStrings->SetSize(v9 + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(v9, v12);
	v3->eMemContext = eMemContext;
}

void bhkBlendCollisionObjectAddRotation::UpdateWorldData(NiUpdateData& UpdateData) {
	NiMatrix3 v6;
	NiMatrix3 result;
	if (spWorldObject) {
		bhkBlendCollisionObject::UpdateWorldData(UpdateData);
	} else {
		NiAVObject* m_pkSceneObject = this->m_pkSceneObject;
		if (m_pkSceneObject) {
			m_pkSceneObject->UpdateWorldLoc();
		}
	}
	NiAVObject* v4 = this->m_pkSceneObject;
	NiMatrix3* v5 = &(result = v4->m_pkParent->m_kWorld.m_Rotate * v4->m_kLocal.m_Rotate);
	v4->m_kWorld.m_Rotate = (v6 = *v5 * AdditionalRotation);
}

// Takes the world object over from apBlendObj (keeping it alive across the two SetWorldObject calls), then the blend
// settings, then points the scene graph object at this object.
void bhkBlendCollisionObjectAddRotation::CopyFrom(bhkBlendCollisionObject* apBlendObj) {
	bhkRigidBody* pWorldObject = apBlendObj->spWorldObject;
	if (pWorldObject) {
		NiIncRefCount(pWorldObject);
	}
	apBlendObj->SetWorldObject(nullptr);
	this->SetWorldObject(pWorldObject);
	this->fVelGain = apBlendObj->fVelGain;
	this->fHeirGain = apBlendObj->fHeirGain;
	this->pStoredWorld = apBlendObj->pStoredWorld;
	this->SetSceneGraphObject(apBlendObj->m_pkSceneObject);
	if (pWorldObject) {
		NiDecRefCount(pWorldObject);
	}
}

void bhkBlendCollisionObjectAddRotation::SetAdditionalRotation(const NiMatrix3& aAdditionalRotation) {
	__builtin_memcpy(&AdditionalRotation, &aAdditionalRotation, sizeof(AdditionalRotation));
}

// Copies the gains and the stored world itself after the base's copy.
void bhkBlendCollisionObjectAddRotation::CopyMembers(bhkBlendCollisionObjectAddRotation* apDest,
	NiCloningProcess& arCloneData) {
	bhkBlendCollisionObject::CopyMembers(apDest, arCloneData);
	apDest->fVelGain = fVelGain;
	apDest->fHeirGain = fHeirGain;
	apDest->pStoredWorld = pStoredWorld;
}
