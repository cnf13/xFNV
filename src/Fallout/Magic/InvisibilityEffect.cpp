#include "Fallout/Magic/InvisibilityEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "SDK/MSVC/rtti.h"

// Whether the invisibility archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::INVISIBILITY, &InvisibilityEffect::Instantiate);

InvisibilityEffect::InvisibilityEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect) {}

InvisibilityEffect::~InvisibilityEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it.
ActiveEffect* InvisibilityEffect::Copy() {
    InvisibilityEffect* pCopy = static_cast<InvisibilityEffect*>(ourMalloc(sizeof(InvisibilityEffect)));
    if (pCopy) {
        pCopy = ::new (pCopy) InvisibilityEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

void InvisibilityEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
}

void InvisibilityEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
    if (BSShaderManager::IsRefractionEnabled() &&
        (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer))) {
        apActor->SetAlpha((float)1.0);
        apActor->SetRefraction((bool)1, 0.05000000074505806f);
    }
    else {
        apActor->UpdateAlpha();
    }
}

void InvisibilityEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    if (BSShaderManager::IsRefractionEnabled() && apActor == pPlayer) {
        apActor->SetRefraction(1, 0.0f);
    }
    else {
        apActor->UpdateAlpha();
    }
}

void InvisibilityEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

bool InvisibilityEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor;
    if (apTarget) {
        pTargetActor = apTarget->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    return pTargetActor != nullptr;
}

void InvisibilityEffect::Start() {
    Actor* ptargetActor;
    if (pTarget) {
        ptargetActor = pTarget->GetTargetAsActor();
    }
    else {
        ptargetActor = 0;
    }
    ValueModifierEffect::Start();
    if (ptargetActor) {
        if (static_cast<double>(ptargetActor->GetActorValue(eWhichActorValue)) > 0.0) {
            if (BSShaderManager::IsRefractionEnabled() && ptargetActor == pPlayer) {
                ptargetActor->SetAlpha(1.0f);
                ptargetActor->SetRefraction(1, 0.05f);
            }
            else {
                HighProcess* pprocess = dynamic_cast<HighProcess*>(ptargetActor->pCurrentProcess);
                if (pprocess) {
                    if (pprocess->GetFadeState() != 4) {
                        pprocess->FadeOut(ptargetActor, 0, 0);
                    }
                }
            }
        }
    }
}

void InvisibilityEffect::Finish() {
    Actor* ptargetActor;
    ValueModifierEffect::Finish();
    if (pTarget) {
        ptargetActor = pTarget->GetTargetAsActor();
    }
    else {
        ptargetActor = 0;
    }
    if (ptargetActor) {
        if (static_cast<double>(ptargetActor->GetActorValue(eWhichActorValue)) == 0.0) {
            if (BSShaderManager::IsRefractionEnabled() && ptargetActor == pPlayer) {
                ptargetActor->SetRefraction(1, 0.0f);
            }
            else {
                HighProcess* pprocess = dynamic_cast<HighProcess*>(ptargetActor->pCurrentProcess);
                if (pprocess) {
                    if (pprocess->GetFadeState() != BaseProcess::FADE_TELEPORT_OUT) {
                        pprocess->FadeIn(ptargetActor, 0);
                    }
                }
            }
            if (static_cast<float>(0.0) > 0.0 && BSShaderManager::IsRefractionEnabled()) {
                float value = 1.0 - 0.0f;
                float x2 = MagicSystem::GetChameleonMaxRefraction();
                float x1 = MagicSystem::GetChameleonMinRefraction();
                float fRefractionPower = FLerp(x1, x2, 0.0f, 1.0f, value);
                ptargetActor->SetRefraction(1, fRefractionPower);
            }
            else {
                ptargetActor->UpdateAlpha();
            }
            MagicHitEffect* peffect = 0;
            if (GetEffectSetting()->GetModelLength()) {
                MagicHitEffect* peffect_2;
                MagicModelHitEffect* apMemory =
                    static_cast<MagicModelHitEffect*>(NiMemObject::operator new(sizeof(MagicModelHitEffect)));
                if (apMemory) {
                    TESModel* v5 = static_cast<TESModel*>(GetEffectSetting());
                    const char* v1 = v5->GetModel();
                    peffect_2 = (MagicHitEffect*)::new (apMemory) MagicModelHitEffect(ptargetActor, v1, -1.0f);
                }
                else {
                    peffect_2 = 0;
                }
                peffect = peffect_2;
            }
            if (peffect && peffect->Init()) {
                ReferenceProcessLists.AddTempEffect(peffect);
            }
            else {
                if (peffect) {
                    delete peffect;
                }
            }
        }
    }
}

ActiveEffect* InvisibilityEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(InvisibilityEffect));
    if (v5) {
        return ::new (v5) InvisibilityEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
