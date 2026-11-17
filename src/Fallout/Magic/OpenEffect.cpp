#include "Fallout/Magic/OpenEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

// Not named in the PDB or IDA; name chosen here. Whether the open archetype took OpenEffect::Instantiate: the exe stores
// RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::OPEN, &OpenEffect::Instantiate);

OpenEffect::OpenEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ActiveEffect(apCaster, apSpell, apEffect) {}

OpenEffect::~OpenEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
ActiveEffect* OpenEffect::Copy() {
	ActiveEffect* v2;
	void* v4 = ourMalloc(sizeof(OpenEffect));
	if (v4) {
		v2 = ::new (v4) OpenEffect(pCaster, pSpell, pEffect);
	} else {
		v2 = nullptr;
	}
	// The exe calls the override through vtable slot 17 (+0x44), thiscall with this in ecx and ret 4.
	ActiveEffect* pThis = this;
	pThis->CopyData(v2);
	return v2;
}

// A non-actor target whose reference is a door or a container.
bool OpenEffect::CheckTarget(MagicTarget* apTarget) {
	NonActorMagicTarget* pTarget = dynamic_cast<NonActorMagicTarget*>(apTarget);
	if (!pTarget || !pTarget->GetTargetStatsObject()) {
		return false;
	}
	TESObjectDOOR* pDoor = dynamic_cast<TESObjectDOOR*>(pTarget->GetTargetStatsObject()->GetObjectReference());
	TESObjectCONT* pContainer = dynamic_cast<TESObjectCONT*>(pTarget->GetTargetStatsObject()->GetObjectReference());
	return pDoor || pContainer;
}

void OpenEffect::Start() {
	NonActorMagicTarget* pnaTarget = dynamic_cast<NonActorMagicTarget*>(pTarget);
	if (pnaTarget) {
		if (pnaTarget->GetTargetStatsObject()) {
			TESObjectREFR* v1 = pnaTarget->GetTargetStatsObject();
			LockData* plock = v1->GetLock();
			double SequenceWeight = fMagnitude;
			LOCK_LEVEL espellLevel = static_cast<LOCK_LEVEL>(GetLockPickLevel(static_cast<int32_t>(SequenceWeight)));
			TESObjectREFR* v3 = pnaTarget->GetTargetStatsObject();
			int32_t LockLevel = v3->GetLockLevel();
			LOCK_LEVEL elockLevel = static_cast<LOCK_LEVEL>(GetLockPickLevel(LockLevel));
			if (plock) {
				if (espellLevel >= elockLevel) {
					TESObjectREFR* v5 = pnaTarget->GetTargetStatsObject();
					v5->UnLock();
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
ActiveEffect* OpenEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	OpenEffect* v5 = static_cast<OpenEffect*>(ourMalloc(sizeof(OpenEffect)));
	if (v5) {
		return ::new (v5) OpenEffect(a1, a2, a3);
	} else {
		return 0;
	}
}
