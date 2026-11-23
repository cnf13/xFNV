#include "Fallout/Magic/ConcussionEffect.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "SDK/MSVC/rtti.h"

#include <cstdint>

static char bConcussionEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::CONCUSSION,
    &ConcussionEffect::Instantiate
);

TESImageSpaceModifier* GetConcussionIMOD() {
    if (!g_bEnableHitAndConcussionIMODs) {
        return nullptr;
    }
    if (!g_ConcussionIMOD) {
        TESForm* pForm = TESForm::GetFormByNumericID(0x164u);
        g_ConcussionIMOD = dynamic_cast<TESImageSpaceModifier*>(pForm);
    }
    return g_ConcussionIMOD;
}

ConcussionEffect::ConcussionEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect), fTimer(0.0f) {}

ConcussionEffect::~ConcussionEffect() {}

ActiveEffect* ConcussionEffect::Copy() {
    ConcussionEffect* pCopy = new ConcussionEffect(pCaster, pSpell, pEffect);
    pCopy->fTimer = fTimer;
    CopyData(pCopy);
    return pCopy;
}

// The base ActiveEffect::Update(afSecondsElapsed) does nothing, so it is not called here.
void ConcussionEffect::Update(float afSecondsElapsed) {
    fTimer = fTimer + afSecondsElapsed;
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pActor) {
        CheckApplyConcussion(pActor);
    }
}

bool ConcussionEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

bool ConcussionEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pActor && !pActor->IsDead(false) && !pActor->IsDying();
}

void ConcussionEffect::Start() {
    ActiveEffect::Start();
}

void ConcussionEffect::Finish() {
    ActiveEffect::Finish();
}

void ConcussionEffect::CheckApplyConcussion(Actor* apActor) {
    if (fConcussionTimer.Float() < static_cast<double>(fTimer)) {
        fTimer = 0.0;
        if (apActor->GetPerceptionCondition() == 0.0 && !apActor->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
            bCheckingConcussions = 1;
            if (apActor->IsPc() && !pPlayer->IsSleepingorResting()) {
                float fStrength = fGetHitPainMult.Float();
                ImageSpaceModifierInstanceForm::Trigger(GetConcussionIMOD(), fStrength, nullptr);
                BSAudio::QInstance().GetSoundHandleByName("FXCrippleHead", 257u).Play(false);
            }
            apActor->pCurrentProcess->SetupSpecialIdle(apActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
            bCheckingConcussions = 0;
        }
    }
}

ActiveEffect* ConcussionEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new ConcussionEffect(apCaster, apSpell, apEffect);
}
