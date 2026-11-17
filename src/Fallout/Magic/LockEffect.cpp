#include "Fallout/Magic/LockEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "SDK/MSVC/rtti.h"

// Whether the lock archetype took LockEffect::Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::LOCK, &LockEffect::Instantiate);

LockEffect::LockEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ActiveEffect(apCaster, apSpell, apEffect) {}

LockEffect::~LockEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* LockEffect::Copy() {
	ActiveEffect* v2;
	void* v4 = ourMalloc(sizeof(LockEffect));
	if (v4) {
		v2 = ::new (v4) LockEffect(pCaster, pSpell, pEffect);
	} else {
		v2 = nullptr;
	}
	ActiveEffect* pThis = this;
	pThis->CopyData(v2);
	return v2;
}

bool LockEffect::CheckTarget(MagicTarget* apTarget) {
	NonActorMagicTarget* pnaTarget = dynamic_cast<NonActorMagicTarget*>(apTarget);
	if (!pnaTarget || !pnaTarget->GetTargetStatsObject()) {
		return 0;
	}
	TESObjectREFR* v3 = pnaTarget->GetTargetStatsObject();
	TESObjectDOOR* pdoor = dynamic_cast<TESObjectDOOR*>(v3->GetObjectReference());
	TESObjectREFR* v4 = pnaTarget->GetTargetStatsObject();
	TESObjectCONT* pcontainer = dynamic_cast<TESObjectCONT*>(v4->GetObjectReference());
	return pdoor || pcontainer;
}

void LockEffect::Start() {
	NonActorMagicTarget* pnaTarget = dynamic_cast<NonActorMagicTarget*>(pTarget);
	if (pnaTarget) {
		if (pnaTarget->GetTargetStatsObject()) {
			TESObjectREFR* v1 = pnaTarget->GetTargetStatsObject();
			LockData* plock = v1->GetLock();
			if (plock && ((plock->cFlags & 2) == 0 || (plock->cFlags & 1) != 0)) {
				Dispel(0);
			} else {
				TESObjectREFR* v2 = pnaTarget->GetTargetStatsObject();
				LockData* plock_2 = v2->AddLock();
				plock_2->cLockLevel = static_cast<int32_t>(fMagnitude);
				plock_2->pKey = 0;
				plock_2->cFlags = 2;
				TESObjectREFR* v3 = pnaTarget->GetTargetStatsObject();
				v3->AddLockChange();
				TESObjectREFR* v4 = pnaTarget->GetTargetStatsObject();
				v4->Lock();
			}
		}
	}
}

ActiveEffect* LockEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	void* v5 = ourMalloc(sizeof(LockEffect));
	if (v5) {
		return ::new (v5) LockEffect(a1, a2, a3);
	} else {
		return nullptr;
	}
}
