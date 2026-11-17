#include "Fallout/Magic/DemoralizeEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESObjectREFR.h"
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

// Whether the demoralize archetype took Create: stores RegisterActiveEffect's whole return byte.
static char bRegistered =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::DEMORALIZE, &DemoralizeEffect::Create);

DemoralizeEffect::DemoralizeEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
	: ActiveEffect(apCaster, apItem, apEffect), bTurned(false) {}

DemoralizeEffect::~DemoralizeEffect() {}

// Copies the flag byte onto the new effect, then copies the base data.
ActiveEffect* DemoralizeEffect::Copy() {
	DemoralizeEffect* pCopy;
	void* pMemory = ourMalloc(sizeof(DemoralizeEffect));
	if (pMemory) {
		pCopy = ::new (pMemory) DemoralizeEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	pCopy->bTurned = bTurned;
	CopyData(pCopy);
	return pCopy;
}

uint16_t DemoralizeEffect::GetSaveSize(Actor* apActor) {
	return (uint16_t)(((int16_t)ActiveEffect::GetSaveSize(apActor)) + 1);
}

void DemoralizeEffect::SaveGame(Actor* a2) {
	ActiveEffect::SaveGame(a2);
	pSaveLoadGameOLD->SaveGameDataOLD(&bTurned, 1);
}

void DemoralizeEffect::LoadGame(Actor* a2) {
	ActiveEffect::LoadGame(a2);
	pSaveLoadGameOLD->LoadGameDataOLD(&bTurned, 1);
}

bool DemoralizeEffect::CheckDeletedCaster(MagicCaster* apCaster) {
	bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
	if (bDeleted) {
		Dispel(true);
	}
	return bDeleted;
}

bool DemoralizeEffect::CheckTarget(MagicTarget* apTarget) {
	Actor* targetAsActor;
	if (apTarget) {
		targetAsActor = apTarget->GetTargetAsActor();
	} else {
		targetAsActor = nullptr;
	}
	if (!targetAsActor) {
		return (bool)0;
	}
	if (targetAsActor->IsDead((bool)0)) {
		return (bool)0;
	}
	if (targetAsActor->IsDying()) {
		return (bool)0;
	}
	float afMagnitude = fMagnitude;
	uint16_t level = targetAsActor->GetLevel();
	if (!MagicFormulas::CheckTargetLevelMagnitude(level, afMagnitude)) {
		return (bool)0;
	}
	return (bool)((uint8_t)(dynamic_cast<Creature*>(apTarget) || (!targetAsActor->IsReanimated())));
}

void DemoralizeEffect::Start() {
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
				ptargetActor->GetCombatController()->ForceFlee(pcasterActor, false);
				bTurned = 1;
			} else {
				if (!ptargetActor->IsFleeing(0)) {
					ptargetActor->EndInterruptPackage(0);
					ptargetActor->InitiateFlee(pcasterActor, 1, 1, 1, 0, 0, -1.0f, -1.0f);
					bTurned = 1;
				}
			}
		}
	}
}

void DemoralizeEffect::Finish() {
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
			} else {
				if (targetAsActor->IsFleeing(0)) {
					targetAsActor->RemoveFleeTarget(casterAsActor);
				}
			}
		}
	}
}

ActiveEffect* DemoralizeEffect::Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	DemoralizeEffect* apMemory = static_cast<DemoralizeEffect*>(ourMalloc(sizeof(DemoralizeEffect)));
	if (apMemory) {
		return ::new (apMemory) DemoralizeEffect(a1, a2, a3);
	} else {
		return 0;
	}
}
