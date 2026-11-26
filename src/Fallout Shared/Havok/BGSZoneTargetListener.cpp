#include "Fallout Shared/Havok/BGSZoneTargetListener.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkEntity.h"
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

#include <cstdint>

ZoneEntry::ZoneEntry(TESObjectREFR* apRefr, const hkpCollidable* apCollidable)
  : bhkTTNode<TESObjectREFR>(apRefr, apCollidable), pTargets(nullptr) {}

ZoneEntry::~ZoneEntry() {
    EmptyList();
}

// Sets the flags on the new entry without testing the allocation.
TargetEntry* ZoneEntry::AddTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags) {
    TargetEntry* pEntry = this->pTargets ? GetTargetEntry(apTarget) : nullptr;
    if (!pEntry) {
        pEntry = new TargetEntry(apTarget, apCol);
        pEntry->SetFlags(aiFlags, true);
        // The node base sits at +0x10: a null head stays null.
        pEntry->pNext = static_cast<bhkTTNode<TESObjectREFR>*>(this->pTargets);
        this->pTargets = pEntry;
    }
    return pEntry;
}

ZoneEntry* ZoneEntry::GetZoneEntry(const hkpCollidable* apCollidable) {
    bhkTTNode<TESObjectREFR>* pEntry = GetEntry(apCollidable);
    return static_cast<ZoneEntry*>(pEntry);
}

void ZoneEntry::RemoveTargetEntry(TargetEntry* apTarget) {
    if (apTarget) {
        // RemoveFromList is passed the head's node (null for no head), and the new head is stepped back from its node
        // to its entry (null stays null).
        pTargets = static_cast<TargetEntry*>(apTarget->RemoveFromList(pTargets));
        TargetEntry::operator delete(apTarget);
    }
}

// Sets the flags on the new entry without testing the allocation.
TargetEntry* ZoneEntry::AddTrapTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags) {
    TargetEntry* pEntry = this->pTargets ? GetTargetEntry(apTarget) : nullptr;
    if (!pEntry) {
        pEntry = new TrapTargetEntry(apTarget, apCol);
        pEntry->SetFlags(aiFlags, true);
        // The node base sits at +0x10: a null head stays null.
        pEntry->pNext = static_cast<bhkTTNode<TESObjectREFR>*>(this->pTargets);
        this->pTargets = pEntry;
    }
    return pEntry;
}

ZoneEntry* ZoneEntry::GetZoneEntry(TESObjectREFR* apZone) {
    bhkTTNode<TESObjectREFR>* pEntry = GetEntry(apZone);
    return static_cast<ZoneEntry*>(pEntry);
}

TargetEntry* ZoneEntry::GetTargetEntry(TESObjectREFR* apTarget) {
    return pTargets->GetTargetEntry(apTarget);
}

ZoneEntry* ZoneEntry::QNextEntry() {
    bhkTTNode<TESObjectREFR>* pNext = static_cast<bhkTTNode<TESObjectREFR>*>(this)->pNext;
    return static_cast<ZoneEntry*>(pNext);
}

TargetEntry* ZoneEntry::GetTargets() {
    return pTargets;
}

void ZoneEntry::EmptyList() {
    WaterTargetEntry* pEntry = static_cast<WaterTargetEntry*>(pTargets);
    while (pEntry) {
        WaterTargetEntry* pNextEntry = pEntry->GetNextEntry();
        TargetEntry::operator delete(pEntry);
        pEntry = pNextEntry;
    }
    pTargets = nullptr;
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
    return GetFlags(TGE_HIT) != 0;
}

void* TargetEntry::operator new(size_t aSize) {
    return MemoryManager::Instance().Allocate(aSize);
}

void TargetEntry::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

BGSZoneTargetListener::BGSZoneTargetListener() : pZones(nullptr) {}

BGSZoneTargetListener::~BGSZoneTargetListener() {
    EmptyList();
}

ZoneEntry* BGSZoneTargetListener::CreateZoneEntry(TESObjectREFR* apZone, const hkpCollidable* apCol) {
    return new ZoneEntry(apZone, apCol);
}

TargetEntry* BGSZoneTargetListener::CreateTargetEntry(TESObjectREFR* apZone, const hkpCollidable* apCol) {
    return new TargetEntry(apZone, apCol);
}

// Each zone's next entry is read before apTarget is removed from it.
void BGSZoneTargetListener::RemoveTarget(TESObjectREFR* apTarget) {
    if (!apTarget) {
        return;
    }
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
    }

    return nullptr;
}

ZoneEntry* BGSZoneTargetListener::QZoneEntryList() {
    return pZones;
}

void BGSZoneTargetListener::EmptyList() {
    ZoneEntry* pZone = pZones;
    while (pZone) {
        ZoneEntry* pNext = pZone->QNextEntry();
        delete pZone;
        pZone = pNext;
    }
    pZones = nullptr;
}

ZoneEntry* BGSZoneTargetListener::AddZone(TESObjectREFR* apZone, const hkpCollidable* apCol) {
    ZoneEntry* pEntry = pZones ? pZones->GetZoneEntry(apCol) : nullptr;
    if (!pEntry) {
        pEntry = CreateZoneEntry(apZone, apCol);
        pEntry->pNext = static_cast<bhkTTNode<TESObjectREFR>*>(pZones);
        pZones = pEntry;
    }
    return pEntry;
}

// The zone is whichever of the point's two root collidables belongs to apZone, the target's collidable whichever
// belongs to apTarget. The new target entry records the hit's position, the velocity of the zone entry's rigid body
// (zero without one) and the material at the hit: the hit sub-shape's for a shape collection, else the shape's.
ZoneEntry* BGSZoneTargetListener::AddZoneTargetPair(
    const hkpRootCdPoint& arPoint,
    TESObjectREFR* apZone,
    TESObjectREFR* apTarget,
    uint32_t aiFlags,
    bool abTrap
) {
    NiAVObject* pObjectA = GetAVObjectForCollidable(const_cast<hkpCollidable*>(arPoint.m_rootCollidableA));
    NiAVObject* pObjectB = GetAVObjectForCollidable(const_cast<hkpCollidable*>(arPoint.m_rootCollidableB));
    TESObjectREFR* pRefA = TESObjectREFR::FindReferenceFor3D(pObjectA);
    TESObjectREFR* pRefB = TESObjectREFR::FindReferenceFor3D(pObjectB);
    const hkpCollidable* pZoneCol;
    if (pRefA == apZone) {
        pZoneCol = arPoint.m_rootCollidableA;
    }
    else {
        pZoneCol = pRefB == apZone ? arPoint.m_rootCollidableB : nullptr;
    }
    const hkpCollidable* pTargetCol;
    if (pRefA == apTarget) {
        pTargetCol = arPoint.m_rootCollidableA;
    }
    else {
        pTargetCol = pRefB == apTarget ? arPoint.m_rootCollidableB : nullptr;
    }
    ZoneEntry* pZone = AddZone(apZone, pZoneCol);
    if (!pZone) {
        return pZone;
    }
    TargetEntry* pTarget = nullptr;
    if (abTrap) {
        pTarget = pZone->AddTrapTarget(apTarget, pTargetCol, aiFlags);
    }
    else {
        pTarget = pZone->AddTarget(apTarget, pTargetCol, aiFlags);
    }
    if (!pTarget) {
        return pZone;
    }
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
        if (pCollection && arPoint.m_shapeKeyB != 0xFFFFFFFF) {
            pTarget->SetMaterialAtHit(pCollection->QSubMaterial(arPoint.m_shapeKeyB));
        }
        else if (pShape) {
            pTarget->SetMaterialAtHit(pShape->GetMaterial());
        }
    }
    else {
        kVelocity.setZero4();
    }
    pTarget->SetVelocityAtHit(kVelocity);
    return pZone;
}

ZoneEntry* BGSZoneTargetListener::RemoveTargetFromZone(const hkpCollidable* apZone, TESObjectREFR* apTarget) {
    if (!pZones) {
        return nullptr;
    }
    ZoneEntry* pZone = pZones->GetZoneEntry(apZone);
    if (!pZone) {
        return nullptr;
    }
    RemoveTargetFromZone(pZone, apTarget);
    return pZone;
}

void BGSZoneTargetListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
