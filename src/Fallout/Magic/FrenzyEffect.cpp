#include "Fallout/Magic/FrenzyEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "SDK/MSVC/rtti.h"

// Not named in the PDB or IDA; name chosen here. Whether the frenzy archetype took Instantiate: the exe stores
// RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::FRENZY, &FrenzyEffect::Instantiate);

FrenzyEffect::FrenzyEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
	: ValueModifierEffect(apCaster, apItem, apEffect), bStartedInCombat(false) {}

FrenzyEffect::~FrenzyEffect() {}

// The exe copies the flag byte onto the new effect, then copies the base data through the vtable (slot 17, ecx = this).
ActiveEffect* FrenzyEffect::Copy() {
	FrenzyEffect* pCopy;
	void* pMemory = ourMalloc(sizeof(FrenzyEffect));
	if (pMemory) {
		pCopy = ::new (pMemory) FrenzyEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	pCopy->bStartedInCombat = bStartedInCombat;
	CopyData(pCopy);
	return pCopy;
}

uint16_t FrenzyEffect::GetSaveSize(Actor* apActor) {
	return (uint16_t)(((int16_t)ValueModifierEffect::GetSaveSize(apActor)) + 1);
}

void FrenzyEffect::SaveGame(Actor* apActor) {
	ValueModifierEffect::SaveGame(apActor);
	pSaveLoadGameOLD->SaveGameDataOLD(&bStartedInCombat, 1);
}

void FrenzyEffect::LoadGame(Actor* apActor) {
	ValueModifierEffect::LoadGame(apActor);
	pSaveLoadGameOLD->LoadGameDataOLD(&bStartedInCombat, 1);
}

bool FrenzyEffect::CheckDeletedCaster(MagicCaster* apCaster) {
	bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
	if (bDeleted) {
		Dispel(true);
	}
	return bDeleted;
}

bool FrenzyEffect::CheckTarget(MagicTarget* apTarget) {
	uint16_t level;
	float afMagnitude;
	Character* pCharacter = dynamic_cast<Character*>(apTarget);
	uint8_t result = 0;
	if (pCharacter) {
		if ((!pCharacter->IsDead((bool)0)) && (!pCharacter->IsDying())) {
			afMagnitude = fMagnitude;
			level = pCharacter->GetLevel();
			if (MagicFormulas::CheckTargetLevelMagnitude(level, afMagnitude)) {
				return (bool)1;
			}
		}
	}
	return (bool)result;
}

void FrenzyEffect::Start() {
	Actor* CasterAsActor;
	Actor* TargetAsActor;
	ValueModifierEffect::Start();
	if (GetTarget()) {
		MagicTarget* Unk24 = GetTarget();
		TargetAsActor = Unk24->GetTargetAsActor();
	} else {
		TargetAsActor = 0;
	}
	if (GetCaster()) {
		MagicCaster* ProcessLevel = GetCaster();
		CasterAsActor = ProcessLevel->GetCasterAsActor();
	} else {
		CasterAsActor = 0;
	}
	if (TargetAsActor) {
		if (CasterAsActor) {
			if (TargetAsActor->IsInCombat()) {
				bStartedInCombat = 1;
			}
		}
	}
}

void FrenzyEffect::Finish() {
	ValueModifierEffect::Finish();
	if (!bStartedInCombat) {
		Actor* TargetAsActor;
		if (pTarget) {
			TargetAsActor = pTarget->GetTargetAsActor();
		} else {
			TargetAsActor = 0;
		}
		if (TargetAsActor) {
			if (TargetAsActor->IsInCombat()) {
				TargetAsActor->StopCombat(0);
				ProcessArray* ProcessArray_ = &ReferenceProcessLists.MobProcessArray;
				uint32_t uiIndex = 0;
				for (uint32_t i = ProcessArray_->GetProcessLevelTail(PROCESS_TYPE_HIGH); uiIndex < i;
					i = ProcessArray_->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
					Actor* Item = static_cast<Actor*>(ProcessArray_->GetItem(uiIndex));
					if (Item != TargetAsActor) {
						Actor* v2;
						if (Item && Item->IsActor()) {
							v2 = Item;
						} else {
							v2 = 0;
						}
						if (v2) {
							v2->RemoveFleeTarget(TargetAsActor);
							if (v2->IsInCombat()) {
								v2->StopCombat(TargetAsActor);
							}
						}
					}
					++uiIndex;
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
ActiveEffect* FrenzyEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	FrenzyEffect* v5 = static_cast<FrenzyEffect*>(ourMalloc(sizeof(FrenzyEffect)));
	if (v5) {
		return ::new (v5) FrenzyEffect(apCaster, apSpell, apEffect);
	} else {
		return 0;
	}
}
