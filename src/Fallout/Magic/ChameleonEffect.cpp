#include "Fallout/Magic/ChameleonEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"

static char bChameleonEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::CHAMELEON,
    &ChameleonEffect::Instantiate
);

ChameleonEffect::ChameleonEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect) {}

ChameleonEffect::~ChameleonEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* ChameleonEffect::Copy() {
    ActiveEffect* pCopy;
    pCopy = new ChameleonEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void ChameleonEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
    if (BSShaderManager::IsRefractionEnabled()) {
        apActor->SetAlpha(1.0f);
        float fScaled = apActor->GetActorFloatValue(eWhichActorValue) / 100.0;
        float fClamped = BSMAX(0.0f, fScaled);
        float fChameleon = BSMIN(1.0f, fClamped);
        float fVisibility = 1.0 - fChameleon;
        float fMaxRefraction = MagicSystem::GetChameleonMaxRefraction();
        float fMinRefraction = MagicSystem::GetChameleonMinRefraction();
        float fRefractionPower = FLerp(fMinRefraction, fMaxRefraction, 0.0f, 1.0f, fVisibility);
        apActor->SetRefraction(true, fRefractionPower);
    }
    else {
        apActor->UpdateAlpha();
    }
}

void ChameleonEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    if (BSShaderManager::IsRefractionEnabled()) {
        apActor->SetRefraction(true, 0.0f);
    }
    else {
        apActor->UpdateAlpha();
    }
}

void ChameleonEffect::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = apLoadGameBuffer->GetActor();
    // The base ActiveEffect::Revert body is empty.
    ActiveEffect::Revert(apLoadGameBuffer);
    if (BSShaderManager::IsRefractionEnabled()) {
        pActor->SetRefraction(true, 0.0f);
    }
    else {
        pActor->UpdateAlpha();
    }
}

void ChameleonEffect::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = apLoadGameBuffer->GetActor();
    ActiveEffect::FinishLoadGame(apLoadGameBuffer);
    if (bActive) {
        if (BSShaderManager::IsRefractionEnabled()) {
            pActor->SetAlpha(1.0f);
            float fScaled = pActor->GetActorFloatValue(eWhichActorValue) / 100.0;
            float fClamped = BSMAX(0.0f, fScaled);
            float fChameleon = BSMIN(1.0f, fClamped);
            float fVisibility = 1.0 - fChameleon;
            float fMaxRefraction = MagicSystem::GetChameleonMaxRefraction();
            float fMinRefraction = MagicSystem::GetChameleonMinRefraction();
            float fRefractionPower = FLerp(fMinRefraction, fMaxRefraction, 0.0f, 1.0f, fVisibility);
            pActor->SetRefraction(true, fRefractionPower);
        }
        else {
            pActor->UpdateAlpha();
        }
    }
}

void ChameleonEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

bool ChameleonEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pTargetActor != nullptr;
}

void ChameleonEffect::Start() {
    Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    ValueModifierEffect::Start();
    if (pTargetActor) {
        if (BSShaderManager::IsRefractionEnabled()) {
            pTargetActor->SetAlpha(1.0f);
            float fScaled = pTargetActor->GetActorFloatValue(eWhichActorValue) / 100.0;
            float fClamped = BSMAX(0.0f, fScaled);
            float fChameleon = BSMIN(1.0f, fClamped);
            float fVisibility = 1.0 - fChameleon;
            float fMaxRefraction = MagicSystem::GetChameleonMaxRefraction();
            float fMinRefraction = MagicSystem::GetChameleonMinRefraction();
            float fRefractionPower = FLerp(fMinRefraction, fMaxRefraction, 0.0f, 1.0f, fVisibility);
            pTargetActor->SetRefraction(true, fRefractionPower);
        }
        else {
            pTargetActor->UpdateAlpha();
        }
    }
}

void ChameleonEffect::Finish() {
    ValueModifierEffect::Finish();
    Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pTargetActor) {
        if (BSShaderManager::IsRefractionEnabled()) {
            float fScaled = pTargetActor->GetActorFloatValue(eWhichActorValue) / 100.0;
            float fClamped = BSMAX(0.0, fScaled);
            float fChameleon = BSMIN(1.0f, fClamped);
            if (!(fChameleon > 0.0)) {
                pTargetActor->SetRefraction(true, 0.0f);
                pTargetActor->UpdateAlpha();
            }
            else {
                float fVisibility = 1.0 - fChameleon / 100.0;
                float fMaxRefraction = MagicSystem::GetChameleonMaxRefraction();
                float fMinRefraction = MagicSystem::GetChameleonMinRefraction();
                float fRefractionPower = FLerp(fMinRefraction, fMaxRefraction, 0.0f, 1.0f, fVisibility);
                pTargetActor->SetRefraction(true, fRefractionPower);
            }
        }
        else {
            pTargetActor->UpdateAlpha();
        }
    }
}

// The detect life, invisibility and night eye effects share this entry.
void ChameleonEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
}

ActiveEffect* ChameleonEffect::Instantiate(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    return new ChameleonEffect(apCaster, apItem, apEffect);
}
