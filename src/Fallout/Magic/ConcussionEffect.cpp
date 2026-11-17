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
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

#include <cstdint>

static char bConcussionEffectInitialized =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CONCUSSION, &ConcussionEffect::Instantiate);

TESImageSpaceModifier* GetConcussionIMOD() {
    TESForm* formByNumericID;
    if (!g_bEnableHitAndConcussionIMODs) {
        return nullptr;
    }
    if (!g_ConcussionIMOD) {
        formByNumericID = TESForm::GetFormByNumericID(0x164u);
        g_ConcussionIMOD = dynamic_cast<TESImageSpaceModifier*>(formByNumericID);
    }
    return g_ConcussionIMOD;
}

ConcussionEffect::ConcussionEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect), fTimer(0.0f) {}

ConcussionEffect::~ConcussionEffect() {}

ActiveEffect* ConcussionEffect::Copy() {
    ConcussionEffect* v2;
    void* v4 = ourMalloc(sizeof(ConcussionEffect));
    if (v4) {
        v2 = ::new (v4) ConcussionEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    v2->fTimer = fTimer;
    CopyData(v2);
    return v2;
}

// The base ActiveEffect::Update(afSecondsElapsed) does nothing, so it is not called here.
void ConcussionEffect::Update(float afSecondsElapsed) {
    Actor* ptarget;
    fTimer = fTimer + afSecondsElapsed;
    if (pTarget) {
        ptarget = pTarget->GetTargetAsActor();
    }
    else {
        ptarget = nullptr;
    }
    if (ptarget) {
        UpdateSound(ptarget);
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
    Actor* targetAsActor;
    if (apTarget) {
        targetAsActor = apTarget->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    return (bool)((uint8_t)(targetAsActor && ((!targetAsActor->IsDead(false)) && (!targetAsActor->IsDying()))));
}

void ConcussionEffect::Start() {
    ActiveEffect::Start();
}

void ConcussionEffect::Finish() {
    ActiveEffect::Finish();
}

void ConcussionEffect::UpdateSound(Actor* a2) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (fConcussionTimer.Float() < static_cast<double>(fTimer)) {
        fTimer = 0.0;
        if (a2->GetPerceptionCondition() == 0.0 && !a2->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS)) {
            bConcussion_11DBE50 = 1;
            if (a2->IsPc() && !pPlayer->IsSleepingorResting()) {
                float afStrength = fGetHitPainMult.Float();
                TESImageSpaceModifier* ConcussionIMOD = GetConcussionIMOD();
                ImageSpaceModifierInstanceForm::Trigger(ConcussionIMOD, afStrength, nullptr);
                BSAudio* v3 = &BSAudio::QInstance();
                BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "FXCrippleHead", 257u);
                SoundHandleByName->Play(false);
            }
            BaseProcess* CurrentAIProcess = a2->pCurrentProcess;
            CurrentAIProcess->SetupSpecialIdle(a2, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
            bConcussion_11DBE50 = 0;
        }
    }
}

ActiveEffect* ConcussionEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(ConcussionEffect));
    if (v5) {
        return ::new (v5) ConcussionEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
