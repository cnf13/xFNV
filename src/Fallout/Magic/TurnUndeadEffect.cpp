#include "Fallout/Magic/TurnUndeadEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "SDK/MSVC/rtti.h"

// Whether the turn-undead archetype took Create; stores RegisterActiveEffect's whole return byte.
static char bRegistered =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::TURN_UNDEAD, &TurnUndeadEffect::Create);

TurnUndeadEffect::TurnUndeadEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
	: ActiveEffect(apCaster, apItem, apEffect), bTurned(false) {}

TurnUndeadEffect::~TurnUndeadEffect() {}

// Copies the flag byte onto the new effect, then copies the base data through the virtual copy function.
ActiveEffect* TurnUndeadEffect::Copy() {
	TurnUndeadEffect* pCopy;
	void* pMemory = ourMalloc(sizeof(TurnUndeadEffect));
	if (pMemory) {
		pCopy = ::new (pMemory) TurnUndeadEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	pCopy->bTurned = bTurned;
	CopyData(pCopy);
	return pCopy;
}

// The base's size plus the one-byte bTurned.
uint16_t TurnUndeadEffect::GetSaveSize(Actor* apActor) {
	return (uint16_t)(((int16_t)ActiveEffect::GetSaveSize(apActor)) + 1);
}

// The old-format save: the base's data, then bTurned.
void TurnUndeadEffect::SaveGame(Actor* apActor) {
	ActiveEffect::SaveGame(apActor);
	pSaveLoadGameOLD->SaveGameDataOLD(&bTurned, 1);
}

// The old-format load: the base's data, then bTurned.
void TurnUndeadEffect::LoadGame(Actor* apActor) {
	ActiveEffect::LoadGame(apActor);
	pSaveLoadGameOLD->LoadGameDataOLD(&bTurned, 1);
}

// A deleted caster dispels the effect at once.
bool TurnUndeadEffect::CheckDeletedCaster(MagicCaster* apCaster) {
	bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
	if (bDeleted) {
		Dispel(true);
	}
	return bDeleted;
}

bool TurnUndeadEffect::CheckTarget(MagicTarget* apTarget) {
	Actor* TargetAsActor;
	if (apTarget) {
		TargetAsActor = apTarget->GetTargetAsActor();
	} else {
		TargetAsActor = 0;
	}
	bool result = false;
	if (TargetAsActor) {
		if (!TargetAsActor->IsDead(0) && !TargetAsActor->IsDying()) {
			float afMagnitude = GetMagnitude();
			uint16_t Level = TargetAsActor->GetLevel();
			if (MagicFormulas::CheckTargetLevelMagnitude(Level, afMagnitude) &&
				(!dynamic_cast<Creature*>(apTarget) && TargetAsActor->IsReanimated())) {
				return true;
			}
		}
	}
	return result;
}

void TurnUndeadEffect::Start() {
	Actor* pcasterActor;
	Actor* ptargetActor;
	if (pTarget) {
		ptargetActor = pTarget->GetTargetAsActor();
	} else {
		ptargetActor = 0;
	}
	if (pCaster) {
		pcasterActor = pCaster->GetCasterAsActor();
	} else {
		pcasterActor = 0;
	}
	if (ptargetActor) {
		if (pcasterActor) {
			if (ptargetActor->GetCombatController()) {
				ptargetActor->GetCombatController()->ForceFlee(pcasterActor, 0);
				bTurned = 1;
			}
		}
	}
}

void TurnUndeadEffect::Finish() {
	MagicTarget* unk24;
	MagicCaster* processLevel;
	BaseProcess* currentAIProcess;
	Actor* casterAsActor;
	Actor* targetAsActor;
	if (bTurned) {
		if (ActiveEffect::GetTarget()) {
			unk24 = ActiveEffect::GetTarget();
			targetAsActor = unk24->GetTargetAsActor();
		} else {
			targetAsActor = nullptr;
		}
		if (ActiveEffect::GetCaster()) {
			processLevel = ActiveEffect::GetCaster();
			casterAsActor = processLevel->GetCasterAsActor();
		} else {
			casterAsActor = nullptr;
		}
		if (targetAsActor) {
			if (targetAsActor->IsInCombat()) {
				targetAsActor->StopCombat(casterAsActor);
				if (casterAsActor) {
					if (targetAsActor->MobileObject::GetCurrentAIProcess()) {
						currentAIProcess = targetAsActor->MobileObject::GetCurrentAIProcess();
						currentAIProcess->EnterCombat(targetAsActor, casterAsActor, (bool)1, (bool)0, nullptr, (bool)0,
							(bool)0, (bool)0, (bool)0, (bool)0, (bool)0, (bool)1, nullptr);
					}
				}
			}
		}
	}
}

ActiveEffect* TurnUndeadEffect::Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	void* v5 = ourMalloc(sizeof(TurnUndeadEffect));
	if (v5) {
		return ::new (v5) TurnUndeadEffect(a1, a2, a3);
	} else {
		return nullptr;
	}
}
