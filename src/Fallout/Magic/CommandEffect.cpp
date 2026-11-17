#include "Fallout/Magic/CommandEffect.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"

CommandEffect::CommandEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ActiveEffect(apCaster, apSpell, apEffect) {}

CommandEffect::~CommandEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
ActiveEffect* CommandEffect::Copy() {
	ActiveEffect* v2;
	void* v4 = ourMalloc(sizeof(CommandEffect));
	if (v4) {
		v2 = ::new (v4) CommandEffect(pCaster, pSpell, pEffect);
	} else {
		v2 = nullptr;
	}
	// The exe calls the override through vtable slot 17 (+0x44), with this in ecx.
	ActiveEffect* pThis = this;
	pThis->CopyData(v2);
	return v2;
}

void CommandEffect::Update(float afSecondsElapsed) {
	MagicTarget* unk24;
	MagicCaster* processLevel;
	PlayerCharacter* casterAsActor;
	Actor* targetAsActor;
	if (ActiveEffect::GetTarget()) {
		unk24 = ActiveEffect::GetTarget();
		targetAsActor = unk24->GetTargetAsActor();
	} else {
		targetAsActor = nullptr;
	}
	if (ActiveEffect::GetCaster()) {
		processLevel = ActiveEffect::GetCaster();
		casterAsActor = static_cast<PlayerCharacter*>(processLevel->GetCasterAsActor());
	} else {
		casterAsActor = nullptr;
	}
	if (targetAsActor) {
		if (casterAsActor) {
			if (targetAsActor->IsDead((bool)0) || targetAsActor->IsDying()) {
				ActiveEffect::Dispel(0);
			} else {
				CommandEffect::UpdateCommandedActor(targetAsActor, casterAsActor);
			}
		}
	}
}

bool CommandEffect::CheckDeletedCaster(MagicCaster* apCaster) {
	bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
	if (bDeleted) {
		Dispel(true);
	}
	return bDeleted;
}

bool CommandEffect::CheckTarget(MagicTarget* apTarget) {
	return ActiveEffect::CheckTarget(apTarget);
}

void CommandEffect::Start() {
	MagicTarget* pTarget;
	MagicCaster* pCaster;
	Actor* pCasterActor;
	Actor* pTargetActor;
	if (ActiveEffect::GetTarget()) {
		pTarget = ActiveEffect::GetTarget();
		pTargetActor = pTarget->GetTargetAsActor();
	} else {
		pTargetActor = nullptr;
	}
	if (ActiveEffect::GetCaster()) {
		pCaster = ActiveEffect::GetCaster();
		pCasterActor = pCaster->GetCasterAsActor();
	} else {
		pCasterActor = nullptr;
	}
	if (pTargetActor) {
		if (pCasterActor) {
			CommandEffect::InitCommandedActor(pTargetActor, pCasterActor);
		}
	}
}

void CommandEffect::Finish() {
	Actor* CasterAsActor;
	Actor* TargetAsActor;
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
			CommandEffect::CleanupCommandedActor(TargetAsActor, CasterAsActor, 1);
		}
	}
}

void CommandEffect::InitCommandedActor(Actor* apTarget, Actor* apCaster) {
	BaseProcess* currentAIProcess;
	apTarget->ModifyDispositionTowardActor(apCaster, 100.0f);
	BaseProcess* pTargetProc = apTarget->MobileObject::GetCurrentAIProcess();
	pTargetProc->SetCommandingActor(apCaster);
	apTarget->TempModActorValue(ActorValue::CONFIDENCE, 100, nullptr);
	TESPackage* v4 = nullptr;
	if (static_cast<const void*>(apCaster) != static_cast<const void*>(pPlayer)) {
		currentAIProcess = apCaster->MobileObject::GetCurrentAIProcess();
		v4 = currentAIProcess->GetPackageThatIsRunning();
	}
	if (v4) {
		v4->GetPackType();
	}
}

void CommandEffect::UpdateCommandedActor(Actor* apTarget, Actor* apCaster) {
	if (apTarget->pCurrentProcess) {
		BaseProcess* CurrentAIProcess = apTarget->pCurrentProcess;
		if (CurrentAIProcess->GetCommandingActor() != apCaster) {
			BaseProcess* v4 = apTarget->pCurrentProcess;
			v4->SetCommandingActor(apCaster);
		}
		if (!apTarget->IsInCombat()) {
			TESPackage* pCommanderPackage = 0;
			if (apCaster != pPlayer) {
				BaseProcess* v3 = apCaster->pCurrentProcess;
				pCommanderPackage = v3->GetPackageThatIsRunning();
			}
			BaseProcess* v2 = apTarget->pCurrentProcess;
			TESPackage* v6 = v2->GetPackageThatIsRunning();
			if ((!pCommanderPackage || pCommanderPackage->GetPackType() == PACKAGE_TYPE_COMBAT) &&
				(!v6 || !apTarget->IsRunningRunOnce())) {
				// The debug build calls Actor::InitiateSummonCreatureDefendPackage(apCaster) here; this exe's call
				// is the shared empty function 0x4534F0 (thiscall, ret 4), so src writes nothing.
			}
		}
	}
}

void CommandEffect::CleanupCommandedActor(Actor* apTarget, Actor* apCaster, bool abClearCommandingActor) {
	BaseProcess* currentAIProcess;
	if (apTarget->MobileObject::GetCurrentAIProcess()) {
		if (abClearCommandingActor) {
			currentAIProcess = apTarget->MobileObject::GetCurrentAIProcess();
			currentAIProcess->SetCommandingActor(nullptr);
		}
	}
	apTarget->ModifyDispositionTowardActor(apCaster, (-100.0f));
	apTarget->TempModActorValue(ActorValue::CONFIDENCE, (-100), nullptr);
}
