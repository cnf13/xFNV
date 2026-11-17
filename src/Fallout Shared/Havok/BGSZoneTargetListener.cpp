#include "Fallout Shared/Havok/BGSZoneTargetListener.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkTTNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESTrapListener.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

#include <stdint.h>

void ourMemalignFree(void* apPtr) {
	MemoryManager* singleton;
	int8_t* pRealPtr;
	if (apPtr) {
		pRealPtr = static_cast<int8_t*>(apPtr) - ((uint8_t)(*(static_cast<int8_t*>(apPtr) - 1)));
		singleton = &MemoryManager::Instance();
		singleton->Deallocate(pRealPtr);
	}
}

ZoneEntry::ZoneEntry(TESObjectREFR* apRefr, const hkpCollidable* apCollidable)
	: bhkTTNode<TESObjectREFR>(apRefr, apCollidable), pTargets(nullptr) {}

ZoneEntry::~ZoneEntry() {
	EmptyList();
}

// Sets the flags on the new entry without testing the allocation, as the game does.
TargetEntry* ZoneEntry::AddTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags) {
	TargetEntry* pa;
	if (this->pTargets) {
		pa = GetTargetEntry(apTarget);
	} else {
		pa = 0;
	}
	if (!pa) {
		void* apMemory = bhkRagdollController::operator new(sizeof(TargetEntry));
		if (apMemory) {
			pa = ::new (apMemory) TargetEntry(apTarget, apCol);
		} else {
			pa = 0;
		}
		pa->SetFlags(aiFlags, 1);
		// The node base sits at +0x10: a null head stays null.
		pa->pNext = static_cast<bhkTTNode<TESObjectREFR>*>(this->pTargets);
		this->pTargets = pa;
	}
	return pa;
}

ZoneEntry* ZoneEntry::GetZoneEntry(const hkpCollidable* apCollidable) {
	bhkTTNode<TESObjectREFR>* Entry = GetEntry(apCollidable);
	if (Entry) {
		return static_cast<ZoneEntry*>(Entry);
	} else {
		return 0;
	}
}

void ZoneEntry::RemoveTargetEntry(TargetEntry* apTarget) {
	if (apTarget) {
		// RemoveFromList is passed the head's node (null for no head), and the new head is stepped back from its node
		// to its entry (null stays null).
		pTargets = static_cast<TargetEntry*>(apTarget->RemoveFromList(pTargets));
		TargetEntry::operator delete(apTarget);
	}
}

// Sets the flags on the new entry without testing the allocation, as the game does.
TargetEntry* ZoneEntry::AddTrapTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags) {
	TargetEntry* pa;
	if (this->pTargets) {
		pa = GetTargetEntry(apTarget);
	} else {
		pa = 0;
	}
	if (!pa) {
		void* apMemory = bhkRagdollController::operator new(sizeof(TrapTargetEntry));
		if (apMemory) {
			pa = ::new (apMemory) TrapTargetEntry(apTarget, apCol);
		} else {
			pa = 0;
		}
		pa->SetFlags(aiFlags, 1);
		// The node base sits at +0x10: a null head stays null.
		pa->pNext = static_cast<bhkTTNode<TESObjectREFR>*>(this->pTargets);
		this->pTargets = pa;
	}
	return pa;
}

ZoneEntry* ZoneEntry::GetZoneEntry(TESObjectREFR* apZone) {
	bhkTTNode<TESObjectREFR>* Entry = GetEntry(apZone);
	if (Entry) {
		return static_cast<ZoneEntry*>(Entry);
	} else {
		return 0;
	}
}

TargetEntry* ZoneEntry::GetTargetEntry(TESObjectREFR* apTarget) {
	return pTargets->GetTargetEntry(apTarget);
}

ZoneEntry* ZoneEntry::QNextEntry() {
	bhkTTNode<TESObjectREFR>* v3 = static_cast<bhkTTNode<TESObjectREFR>*>(this)->pNext;
	if (v3) {
		return static_cast<ZoneEntry*>(v3);
	} else {
		return nullptr;
	}
}

TargetEntry* ZoneEntry::GetTargets() {
	return pTargets;
}

void ZoneEntry::EmptyList() {
	WaterTargetEntry* NextEntry;
	for (WaterTargetEntry* i = static_cast<WaterTargetEntry*>(pTargets); i; i = NextEntry) {
		NextEntry = i->GetNextEntry();
		TargetEntry::operator delete(i);
	}
	pTargets = 0;
}

void ZoneEntry::operator delete(void* apMemory) {
	ourFree(apMemory);
}

TargetEntry::TargetEntry(TESObjectREFR* apTarget, const hkpCollidable* apCollidable)
	: bhkTTNode<TESObjectREFR>(apTarget, apCollidable), eMaterialAtHit(BHK_MATERIAL_MAX) {
	PositionAtHit.setZero4();
	VelocityAtHit.setZero4();
}

void TargetEntry::SetPositionAtHit(const hkVector4& arPos) {
	PositionAtHit = arPos;
}

void TargetEntry::SetVelocityAtHit(const hkVector4& arVel) {
	VelocityAtHit = arVel;
}

int32_t TargetEntry::GetHit() {
	return GetFlags(1) != 0;
}

void TargetEntry::operator delete(void* apMemory) {
	ourMemalignFree(apMemory);
}

BGSZoneTargetListener::BGSZoneTargetListener() : pZones(nullptr) {}

BGSZoneTargetListener::~BGSZoneTargetListener() {
	EmptyList();
}

ZoneEntry* BGSZoneTargetListener::CreateZoneEntry(TESObjectREFR* apZone, const hkpCollidable* apCol) {
	void* apMemory = ourMalloc(sizeof(ZoneEntry));
	if (apMemory) {
		return ::new (apMemory) ZoneEntry(apZone, apCol);
	} else {
		return nullptr;
	}
}

TargetEntry* BGSZoneTargetListener::CreateTargetEntry(TESObjectREFR* apZone, const hkpCollidable* apCol) {
	void* alignedTo0x10 = bhkRagdollController::operator new(sizeof(TargetEntry));
	if (alignedTo0x10) {
		return ::new (alignedTo0x10) TargetEntry(apZone, apCol);
	} else {
		return nullptr;
	}
}

// Each zone's next entry is read before apTarget is removed from it.
void BGSZoneTargetListener::RemoveTarget(TESObjectREFR* apTarget) {
	if (!apTarget)
		return;
	ZoneEntry* pEntry = QZoneEntryList();
	while (pEntry) {
		ZoneEntry* pNext = pEntry->QNextEntry();
		RemoveTargetFromZone(pEntry, apTarget);
		pEntry = pNext;
	}
}

void BGSZoneTargetListener::RemoveZoneEntry(ZoneEntry* apZone) {
	if (apZone) {
		pZones = static_cast<ZoneEntry*>(apZone->RemoveFromList(pZones));
		delete apZone;
	}
}

void BGSZoneTargetListener::RemoveTargetFromZone(ZoneEntry* apZone, TESObjectREFR* apTarget) {
	if (apTarget) {
		if (apZone->GetTargets()) {
			apZone->RemoveTarget(apTarget);
		}
		if (!apZone->GetTargets()) {
			RemoveZoneEntry(apZone);
		}
	}
}

ZoneEntry* BGSZoneTargetListener::GetZoneEntry(TESObjectREFR* apZone) {
	if (pZones) {
		return pZones->GetZoneEntry(apZone);
	} else {
		return nullptr;
	}
}

ZoneEntry* BGSZoneTargetListener::QZoneEntryList() {
	return pZones;
}

void BGSZoneTargetListener::EmptyList() {
	ZoneEntry* v2;
	for (ZoneEntry* i = pZones; i; i = v2) {
		v2 = i->QNextEntry();
		delete i;
	}
	pZones = 0;
}

ZoneEntry* BGSZoneTargetListener::AddZone(TESObjectREFR* apZone, const hkpCollidable* apCol) {
	ZoneEntry* pt;
	if (pZones) {
		pt = pZones->GetZoneEntry(apCol);
	} else {
		pt = 0;
	}
	ZoneEntry* pt_2 = pt;
	if (!pt) {
		pt_2 = CreateZoneEntry(apZone, apCol);
		ZoneEntry* pHead = pZones;
		if (pHead) {
			pt_2->pNext = static_cast<bhkTTNode<TESObjectREFR>*>(pHead);
		} else {
			pt_2->pNext = 0;
		}
		pZones = pt_2;
	}
	return pt_2;
}

// The zone is whichever of the point's two root collidables belongs to apZone, the target's collidable whichever
// belongs to apTarget. The new target entry records the hit's position, the velocity of the zone entry's rigid body
// (zero without one) and the material at the hit: the hit sub-shape's for a shape collection, else the shape's.
ZoneEntry* BGSZoneTargetListener::AddZoneTargetPair(const hkpRootCdPoint& arPoint, TESObjectREFR* apZone,
	TESObjectREFR* apTarget, uint32_t aiFlags, bool abTrap) {
	NiAVObject* pObjectA = GetAVObjectForCollidable(const_cast<hkpCollidable*>(arPoint.m_rootCollidableA));
	NiAVObject* pObjectB = GetAVObjectForCollidable(const_cast<hkpCollidable*>(arPoint.m_rootCollidableB));
	TESObjectREFR* pRefA = TESObjectREFR::FindReferenceFor3D(pObjectA);
	TESObjectREFR* pRefB = TESObjectREFR::FindReferenceFor3D(pObjectB);
	const hkpCollidable* pZoneCol;
	if (pRefA == apZone)
		pZoneCol = arPoint.m_rootCollidableA;
	else
		pZoneCol = pRefB == apZone ? arPoint.m_rootCollidableB : nullptr;
	const hkpCollidable* pTargetCol;
	if (pRefA == apTarget)
		pTargetCol = arPoint.m_rootCollidableA;
	else
		pTargetCol = pRefB == apTarget ? arPoint.m_rootCollidableB : nullptr;
	ZoneEntry* pZone = AddZone(apZone, pZoneCol);
	if (!pZone)
		return pZone;
	TargetEntry* pTarget = nullptr;
	if (abTrap)
		pTarget = pZone->AddTrapTarget(apTarget, pTargetCol, aiFlags);
	else
		pTarget = pZone->AddTarget(apTarget, pTargetCol, aiFlags);
	if (!pTarget)
		return pZone;
	arPoint.m_contact.m_position.getQuad();
	pTarget->SetPositionAtHit(arPoint.m_contact.m_position);
	hkpRigidBody* pBody = hkpGetRigidBody(static_cast<bhkTTNode<TESObjectREFR>*>(pZone)->QCollidable());
	alignas(16) hkVector4 kVelocity;
	kVelocity.getQuad();
	if (pBody) {
		kVelocity = pBody->getLinearVelocity();
		bhkEntity* pEntity = bhkEntity::Getbhk(pBody);
		bhkShape* pShape = pEntity ? pEntity->GetShape() : nullptr;
		bhkShapeCollection* pCollection = pShape ? pShape->FindShapeCollection() : nullptr;
		if (pCollection && arPoint.m_shapeKeyB != 0xFFFFFFFF)
			pTarget->SetMaterialAtHit(pCollection->QSubMaterial(arPoint.m_shapeKeyB));
		else if (pShape)
			pTarget->SetMaterialAtHit(static_cast<HavokMaterialType>(pShape->GetMaterial()));
	} else {
		kVelocity.setZero4();
	}
	pTarget->SetVelocityAtHit(kVelocity);
	return pZone;
}

ZoneEntry* BGSZoneTargetListener::RemoveTargetFromZone_00620680(const hkpCollidable* apZone, TESObjectREFR* apTarget) {
	if (!pZones) {
		return 0;
	}
	ZoneEntry* pZone = pZones->GetZoneEntry(apZone);
	if (!pZone) {
		return 0;
	}
	RemoveTargetFromZone(pZone, apTarget);
	return pZone;
}

void BGSZoneTargetListener::operator delete(void* apMemory) {
	ourFree(apMemory);
}
