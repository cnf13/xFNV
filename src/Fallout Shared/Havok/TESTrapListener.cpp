#include "Fallout Shared/Havok/TESTrapListener.h"

#include "BSCore/BSList.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkTrapListener.h"
#include "BSHavok/bhkTTNode.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/TrapEntry.h"
#include "Fallout Shared/Havok/TriggerEntry.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

TrapTargetEntry::TrapTargetEntry(TESObjectREFR* apRefr, const hkpCollidable* apCollidable)
	: TargetEntry(apRefr, apCollidable) {}

void TrapTargetEntry::Run(ZoneEntry* apTrap) {
	TESObjectREFR* CastingType = pRef;
	CastingType->DoTrap(static_cast<TrapEntry*>(apTrap), this);
}

TESTrapListener::TESTrapListener() : pTriggers(nullptr) {}

TESTrapListener::~TESTrapListener() {
	EmptyTriggerList();
}

ZoneEntry* TESTrapListener::CreateZoneEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) {
	TrapEntry* v5 = static_cast<TrapEntry*>(ourMalloc(sizeof(TrapEntry)));
	if (v5) {
		return ::new (v5) TrapEntry(apRef, apCol);
	} else {
		return 0;
	}
}

void TESTrapListener::Update(float afDelta) {
	for (TriggerEntry* ptriggers = pTriggers; ptriggers; ptriggers = ptriggers->QNextEntry()) {
		ptriggers->Update(afDelta);
	}
	for (ZoneEntry* ptrap = pZones; ptrap; ptrap = ptrap->QNextEntry()) {
		ptrap->Run();
	}
}

void TESTrapListener::entityRemovedCallback(hkpEntity* apEntity) {
	CFilter ifilt;
	const hkpCollidable* pcol = apEntity->getCollidable();
	ifilt.iFilter = pcol->getCollisionFilterInfo();
	COL_LAYER elayer = ifilt.GetLayer();
	if (elayer == L_TRAP || (elayer == L_CLOUDTRAP || elayer == L_CHARCONTROLLER)) {
		TESObjectREFR* pref = TESHavokUtilities::FindCollidableRef(pcol);
		if (pref) {
			if (elayer == L_TRAP || elayer == L_CLOUDTRAP) {
				RemoveZone(pref);
			} else {
				RemoveTarget(pref);
			}
		}
	}
}

void TESTrapListener::phantomRemovedCallback(hkpPhantom* apPhantom) {
	CFilter iflt;
	const hkpCollidable* pcol = apPhantom->getCollidable();
	iflt.iFilter = pcol->getCollisionFilterInfo();
	if (pTriggers) {
		COL_LAYER elayer = iflt.GetLayer();
		if (elayer == L_TRIGGER || (elayer == L_ACTORZONE || elayer == L_GASTRAP)) {
			TriggerEntry* ptrigger = pTriggers->GetTriggerEntry(pcol);
			if (ptrigger) {
				pTriggers = static_cast<TriggerEntry*>(ptrigger->RemoveFromList(pTriggers));
				delete ptrigger;
			}
		}
	}
}

void TESTrapListener::phantomAddedCallback(hkpPhantom* apPhantom) {
	CFilter iflt;
	const hkpCollidable* pcol = apPhantom->getCollidable();
	iflt.iFilter = pcol->getCollisionFilterInfo();
	COL_LAYER elayer = iflt.GetLayer();
	if (elayer == L_TRIGGER || (elayer == L_ACTORZONE || elayer == L_GASTRAP)) {
		TESObjectREFR* pref = TESHavokUtilities::FindCollidableRef(pcol);
		if (pref) {
			TriggerEntry* pentry;
			TriggerEntry* apMemory = static_cast<TriggerEntry*>(ourMalloc(sizeof(TriggerEntry)));
			if (apMemory) {
				pentry = ::new (apMemory) TriggerEntry(pref, pcol);
			} else {
				pentry = 0;
			}
			pentry->pNext = pTriggers;
			pTriggers = pentry;
		}
	}
}

bool TESTrapListener::contactPointAddedCallback(hkpRootCdPoint& arPoint) {
	bool bres = 0;
	TESObjectREFR* ptrapRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableB);
	TESObjectREFR* pTargetRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableA);
	if (ptrapRef) {
		if (pTargetRef) {
			if (pTargetRef->IsActor()) {
				return AddZoneTargetPair(arPoint, ptrapRef, pTargetRef, 0, 1) != 0;
			}
		}
	}
	return bres;
}

bool TESTrapListener::contactPointRemovedCallback(hkpRootCdPoint& arPoint) {
	bool bres = 0;
	TESObjectREFR* ptrapRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableB);
	TESObjectREFR* pTargetRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableA);
	if (ptrapRef) {
		if (pTargetRef) {
			if (pTargetRef->IsActor()) {
				RemoveTargetFromZone_00620680(arPoint.m_rootCollidableB, pTargetRef);
				return 1;
			}
		}
	}
	return bres;
}

void TESTrapListener::ClearCurrentRefs(TESObjectREFR* apRef) {
	TriggerEntry* i;
	for (i = pTriggers; i; i = i->QNextEntry()) {
		if (static_cast<const void*>(i->GetRef()) == static_cast<const void*>(apRef)) {
			i->ClearCurrentRefs();
			return;
		}
	}
}

// Finds the trigger's entry; a trigger the listener does not know holds nothing.
bool TESTrapListener::IsRefInTrigger(TESObjectREFR* apRef, TESObjectREFR* apTrigger) {
	TriggerEntry* pEntry = pTriggers->GetZoneEntry(apTrigger);
	if (!pEntry)
		return false;
	return pEntry->IsInTrigger(apRef);
}

bool TESTrapListener::IsCurrentTrigger(TESObjectREFR* apRef) {
	for (TriggerEntry* i = pTriggers; i; i = i->QNextEntry()) {
		if (i->pRef == apRef) {
			return 1;
		}
	}
	return 0;
}

// Reads each entry's next before deleting it, and null-tests the entry again before the delete.
void TESTrapListener::EmptyTriggerList() {
	TriggerEntry* pEntry = pTriggers;
	while (pEntry) {
		TriggerEntry* pNext = pEntry->QNextEntry();
		if (pEntry)
			delete pEntry;
		pEntry = pNext;
	}
	pTriggers = nullptr;
}

bhkTrapListener::bhkTrapListener() : bhkEntityListener(0) {}

bhkTrapListener::~bhkTrapListener() {}

TrapEntry::TrapEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) : ZoneEntry(apRef, apCol) {}

// Runs every target entry of the trap. The node base sits at +0x10: each next node steps back to its entry.
void TrapEntry::Run() {
	for (TargetEntry* pEntry = pTargets; pEntry; pEntry = static_cast<TargetEntry*>(pEntry->pNext)) {
		pEntry->Run(this);
	}
}

TriggerEntry::RefList::RefList() : iArraySize(0), iArrayMaxSize(20) {
	pRefArray = static_cast<RefStatus*>(ourMalloc(sizeof(RefStatus) * iArrayMaxSize));
}

bool TriggerEntry::RefList::Add(TESObjectREFR* apRef) {
	uint32_t newID = apRef->iFormID;
	for (int32_t irefIdx = 0; irefIdx < iArraySize; ++irefIdx) {
		if (newID == pRefArray[irefIdx].RefID) {
			if (pRefArray[irefIdx].bInsideTrigger) {
				return 0;
			}
			pRefArray[irefIdx].bInsideTrigger = 1;
			return 1;
		}
	}
	if (iArraySize == iArrayMaxSize) {
		int32_t inewArraySize = 2 * iArrayMaxSize;
		// The allocation size for `new RefStatus[inewArraySize]`: the count times the entry, all ones when that
		// overflows.
		uint32_t uicount = static_cast<uint32_t>(inewArraySize);
		TriggerEntry::RefList::RefStatus* pnewArray = static_cast<TriggerEntry::RefList::RefStatus*>(ourMalloc(uicount >
					SIZE_MAX / sizeof(TriggerEntry::RefList::RefStatus)
				? SIZE_MAX
				: sizeof(TriggerEntry::RefList::RefStatus) * uicount));
		for (int32_t icopyIdx = 0; icopyIdx < iArrayMaxSize; ++icopyIdx) {
			pnewArray[icopyIdx].RefID = pRefArray[icopyIdx].RefID;
			pnewArray[icopyIdx].bInsideTrigger = pRefArray[icopyIdx].bInsideTrigger;
		}
		iArrayMaxSize = inewArraySize;
		ourFree(pRefArray);
		pRefArray = pnewArray;
	}
	pRefArray[iArraySize].RefID = newID;
	pRefArray[iArraySize++].bInsideTrigger = 1;
	TriggerEntry::TriggerEvent* pnewEvent =
		static_cast<TriggerEntry::TriggerEvent*>(ourMalloc(sizeof(TriggerEntry::TriggerEvent)));
	pnewEvent->eStatus = TriggerEntry::TRIGGER_STATUS_ENTER;
	pnewEvent->RefID = newID;
	EventList.AddTail(pnewEvent);
	return 0;
}

TriggerEntry::TriggerEvent* TriggerEntry::RefList::GetNewEvent() {
	if (EventList.IsEmpty()) {
		return 0;
	} else {
		return EventList.RemoveHead();
	}
}

void TriggerEntry::RefList::UpdateExitList() {
	int32_t irefIdx = 0;
	while (irefIdx < iArraySize) {
		if (pRefArray[irefIdx].bInsideTrigger) {
			pRefArray[irefIdx++].bInsideTrigger = 0;
		} else {
			TriggerEntry::TriggerEvent* pnewEvent =
				static_cast<TriggerEntry::TriggerEvent*>(ourMalloc(sizeof(TriggerEntry::TriggerEvent)));
			pnewEvent->eStatus = TriggerEntry::TRIGGER_STATUS_EXIT;
			pnewEvent->RefID = pRefArray[irefIdx].RefID;
			EventList.AddTail(pnewEvent);
			for (int32_t ishrinkIdx = irefIdx; ishrinkIdx < iArraySize - 1; ++ishrinkIdx) {
				pRefArray[ishrinkIdx].RefID = pRefArray[ishrinkIdx + 1].RefID;
				pRefArray[ishrinkIdx].bInsideTrigger = pRefArray[ishrinkIdx + 1].bInsideTrigger;
			}
			--iArraySize;
		}
	}
}

TriggerEntry::TriggerEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) : bhkTTNode<TESObjectREFR>(apRef, apCol) {}

TriggerEntry::~TriggerEntry() {}

void TriggerEntry::ClearCurrentRefs() {
	if (!pSaveLoadGame->GetSaveGameLoading()) {
		TESObjectREFR* CastingType = pRef;
		CurrentRefs.ForceLeaveTrigger(CastingType);
	}
	CurrentRefs.Reset();
}

TriggerEntry* TriggerEntry::QNextEntry() {
	return static_cast<TriggerEntry*>(pNext);
}

TriggerEntry* TriggerEntry::GetZoneEntry(TESObjectREFR* apRef) {
	return static_cast<TriggerEntry*>(GetEntry(apRef));
}

TESObjectREFR* TriggerEntry::GetRef() {
	return pRef;
}

// The world's read locks around the phantom queries are empty in this build, so nothing is written for them. The action
// flags passed to Script::SetActionFlag are 0x10000000 inside, 0x20000000 entering and 0x40000000 leaving; they have no
// names.
bool TriggerEntry::Update(float afDelta) {
	bool btriggered = false;
	TESObjectREFR* ptriggerRef = pRef;
	if (!ptriggerRef || ptriggerRef->GetDisabled() || ptriggerRef->GetDelete() || !ptriggerRef->Get3D()) {
		return btriggered;
	}
	hkpPhantom* pp = static_cast<hkpPhantom*>(pCollidable->getOwner());
	if (pp->getType() == HK_PHANTOM_SIMPLE_SHAPE) {
		hkpSimpleShapePhantom* psp = static_cast<hkpSimpleShapePhantom*>(pp);
		if (!psp->getCollisionDetails().isEmpty()) {
			hkpAllCdBodyPairCollector kCollector;
			kCollector.hkpAllCdBodyPairCollector::reset();
			psp->getPenetrations(kCollector, nullptr);
			const hkArray<hkpRootCdBodyPair>& rcols = kCollector.getHits();
			if (!rcols.isEmpty()) {
				int32_t isize = rcols.getSize();
				for (int32_t i = 0; i < isize; ++i) {
					const hkpCollidable* pcol = rcols.operator[]<hkpRootCdBodyPair>(i).m_rootCollidableB;
					if (pcol) {
						TESObjectREFR* ptriggeredRef = TESHavokUtilities::FindCollidableRef(pcol);
						if (ptriggeredRef) {
							bool bactorLimb = ptriggeredRef->IsActor() && hkpGetRigidBody(pcol);
							bool bactorRagdollin = ptriggeredRef->IsDead(0) || ptriggeredRef->IsKnockedOut();
							if ((!bactorLimb || bactorRagdollin) && CurrentRefs.Add(ptriggeredRef)) {
								ExtraDataList* pextraList = pRef->GetExtra();
								bool bwas = btriggered;
								btriggered =
									Script::SetActionFlag(ptriggeredRef, pextraList, SCRIPT_ACTION_ONTRIGGER) || bwas;
							}
						}
					}
				}
			}
		}
	} else if (pp->getType() == HK_PHANTOM_AABB) {
		hkpAabbPhantom* pap = static_cast<hkpAabbPhantom*>(pp);
		hkArray<hkpCollidable*>& rcols = pap->m_overlappingCollidables;
		if (!rcols.isEmpty()) {
			int32_t isize = rcols.getSize();
			for (int32_t i = 0; i < isize; ++i) {
				hkpCollidable* pcol = rcols.operator[]<hkpCollidable*>(i);
				if (pcol) {
					TESObjectREFR* ptriggeredRef = TESHavokUtilities::FindCollidableRef(pcol);
					if (ptriggeredRef) {
						bool bactorLimb = ptriggeredRef->IsActor() && hkpGetRigidBody(pcol);
						bool bactorRagdollin = ptriggeredRef->IsDead(0) || ptriggeredRef->IsKnockedOut();
						if ((!bactorLimb || bactorRagdollin) && CurrentRefs.Add(ptriggeredRef)) {
							ExtraDataList* pextraList = pRef->GetExtra();
							bool bwas = btriggered;
							btriggered =
								Script::SetActionFlag(ptriggeredRef, pextraList, SCRIPT_ACTION_ONTRIGGER) || bwas;
						}
					}
				}
			}
		}
	}
	CurrentRefs.UpdateExitList();
	TriggerEntry::TriggerEvent* pcurrentEvent = CurrentRefs.GetNewEvent();
	if (pcurrentEvent) {
		uint32_t uflag;
		if (pcurrentEvent->eStatus == TriggerEntry::TRIGGER_STATUS_ENTER) {
			uflag = SCRIPT_ACTION_ONTRIGGERENTER;
		} else {
			uflag = SCRIPT_ACTION_ONTRIGGERLEAVE;
		}
		TESObjectREFR* peventRef = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(pcurrentEvent->RefID));
		ourFree(pcurrentEvent);
		if (peventRef) {
			ExtraDataList* pextraList = pRef->GetExtra();
			bool bwas = btriggered;
			btriggered = Script::SetActionFlag(peventRef, pextraList, uflag) || bwas;
			pRef->SetActionRef(peventRef);
		}
	}
	return btriggered;
}

void TriggerEntry::operator delete(void* apMemory) {
	ourFree(apMemory);
}

hkpCdBodyPairCollector::hkpCdBodyPairCollector() : m_earlyOut(false) {}

hkpCdBodyPairCollector::~hkpCdBodyPairCollector() {}

void hkpCdBodyPairCollector::operator delete(void* apMemory) {
	hkpCdPointCollector::operator delete(apMemory);
}

hkpAllCdBodyPairCollector::hkpAllCdBodyPairCollector() : m_hits(0) {
	hkpAllCdBodyPairCollector::reset();
}

// Clears the hits, then the base's early-out flag.
void hkpAllCdBodyPairCollector::reset() {
	m_hits.clear();
	hkpCdBodyPairCollector::reset();
}

hkpAllCdBodyPairCollector::~hkpAllCdBodyPairCollector() {}

const hkArray<hkpRootCdBodyPair, hkContainerHeapAllocator>& hkpAllCdBodyPairCollector::getHits() const {
	return m_hits;
}

void hkpAllCdBodyPairCollector::operator delete(void* p) {
	if (!p) {
		return;
	}
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	hkMemoryAllocator& kAllocator = kRouter.heap();
	kAllocator.blockFree(p, sizeof(hkpAllCdBodyPairCollector));
}

hkpPhantomListener::hkpPhantomListener() {}

hkpPhantomListener::~hkpPhantomListener() {}

void hkpPhantomListener::phantomShapeSetCallback(hkpPhantom* phantom) {
	if (phantom->hkpWorldObject::getWorld()) {
		phantomRemovedCallback(phantom);
		phantomAddedCallback(phantom);
	}
}

void hkpPhantomListener::operator delete(void* apMemory) {
	ourFree(apMemory);
}
