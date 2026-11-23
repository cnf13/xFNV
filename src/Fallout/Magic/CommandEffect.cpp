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
ActiveEffect* CommandEffect::Copy() {
    ActiveEffect* pCopy;
    pCopy = new CommandEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void CommandEffect::Update(float afSecondsElapsed) {
    Actor* pTargetActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pMagicTarget = ActiveEffect::GetTarget();
        pTargetActor = pMagicTarget->GetTargetAsActor();
    }
    PlayerCharacter* pCasterActor = nullptr;
    if (ActiveEffect::GetCaster()) {
        MagicCaster* pMagicCaster = ActiveEffect::GetCaster();
        pCasterActor = static_cast<PlayerCharacter*>(pMagicCaster->GetCasterAsActor());
    }
    if (pTargetActor && pCasterActor) {
        if (pTargetActor->IsDead(false) || pTargetActor->IsDying()) {
            ActiveEffect::Dispel(false);
        }
        else {
            CommandEffect::UpdateCommandedActor(pTargetActor, pCasterActor);
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
    Actor* pTargetActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pTarget = ActiveEffect::GetTarget();
        pTargetActor = pTarget->GetTargetAsActor();
    }
    Actor* pCasterActor = nullptr;
    if (ActiveEffect::GetCaster()) {
        MagicCaster* pCaster = ActiveEffect::GetCaster();
        pCasterActor = pCaster->GetCasterAsActor();
    }
    if (pTargetActor && pCasterActor) {
        CommandEffect::InitCommandedActor(pTargetActor, pCasterActor);
    }
}

void CommandEffect::Finish() {
    Actor* pTargetActor = nullptr;
    if (GetTarget()) {
        MagicTarget* pMagicTarget = GetTarget();
        pTargetActor = pMagicTarget->GetTargetAsActor();
    }
    Actor* pCasterActor = nullptr;
    if (GetCaster()) {
        MagicCaster* pMagicCaster = GetCaster();
        pCasterActor = pMagicCaster->GetCasterAsActor();
    }
    if (pTargetActor && pCasterActor) {
        CommandEffect::CleanupCommandedActor(pTargetActor, pCasterActor, true);
    }
}

void CommandEffect::InitCommandedActor(Actor* apTarget, Actor* apCaster) {
    apTarget->ModifyDispositionTowardActor(apCaster, 100.0f);
    BaseProcess* pTargetProc = apTarget->MobileObject::GetCurrentAIProcess();
    pTargetProc->SetCommandingActor(apCaster);
    apTarget->TempModActorValue(ActorValue::CONFIDENCE, 100, nullptr);
    TESPackage* pCommanderPackage = nullptr;
    if (static_cast<const void*>(apCaster) != static_cast<const void*>(pPlayer)) {
        BaseProcess* pCasterProc = apCaster->MobileObject::GetCurrentAIProcess();
        pCommanderPackage = pCasterProc->GetPackageThatIsRunning();
    }
    if (pCommanderPackage) {
        pCommanderPackage->GetPackType();
    }
}

void CommandEffect::UpdateCommandedActor(Actor* apTarget, Actor* apCaster) {
    if (apTarget->pCurrentProcess) {
        if (apTarget->pCurrentProcess->GetCommandingActor() != apCaster) {
            apTarget->pCurrentProcess->SetCommandingActor(apCaster);
        }
        if (!apTarget->IsInCombat()) {
            TESPackage* pCommanderPackage = nullptr;
            if (apCaster != pPlayer) {
                pCommanderPackage = apCaster->pCurrentProcess->GetPackageThatIsRunning();
            }
            TESPackage* pTargetPackage = apTarget->pCurrentProcess->GetPackageThatIsRunning();
            if ((!pCommanderPackage || pCommanderPackage->GetPackType() == PACKAGE_TYPE_COMBAT) &&
                (!pTargetPackage || !apTarget->IsRunningRunOnce())) {
                // Actor::InitiateSummonCreatureDefendPackage(apCaster) belongs here, but it is an empty function in the
                // game, so nothing is called.
            }
        }
    }
}

void CommandEffect::CleanupCommandedActor(Actor* apTarget, Actor* apCaster, bool abClearCommandingActor) {
    if (apTarget->MobileObject::GetCurrentAIProcess() && abClearCommandingActor) {
        BaseProcess* pTargetProc = apTarget->MobileObject::GetCurrentAIProcess();
        pTargetProc->SetCommandingActor(nullptr);
    }

    apTarget->ModifyDispositionTowardActor(apCaster, -100.0f);
    apTarget->TempModActorValue(ActorValue::CONFIDENCE, -100, nullptr);
}
