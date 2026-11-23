#include "Fallout/Magic/DetectLifeEffect.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"

// Whether the detect-life archetype took Create; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::DETECT_LIFE,
    &DetectLifeEffect::Create
);

DetectLifeEffect::DetectLifeEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect) {}

DetectLifeEffect::~DetectLifeEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it.
ActiveEffect* DetectLifeEffect::Copy() {
    DetectLifeEffect* pCopy = new DetectLifeEffect(pCaster, pSpell, pEffect);
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

// Starts the value modifier; on the player it also clears the detect-life flag.
void DetectLifeEffect::Start() {
    ValueModifierEffect::Start();
    if (GetTarget()->GetTargetStatsObject() == pPlayer) {
        CheckPlayerRunningDetectLife();
    }
}

void DetectLifeEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
}

void DetectLifeEffect::Finish() {
    ValueModifierEffect::Finish();
    if (pTarget->GetTargetStatsObject() == pPlayer) {
        DetectLifeEffect::CheckPlayerRunningDetectLife();
    }
}

void DetectLifeEffect::CheckPlayerRunningDetectLife() {
    DetectLifeEffect::bPlayerDetectingLife = false;
}

char DetectLifeEffect::GetPlayerDetectingLife() {
    return DetectLifeEffect::bPlayerDetectingLife;
}

ActiveEffect* DetectLifeEffect::Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new DetectLifeEffect(apCaster, apSpell, apEffect);
}

bool DetectLifeEffect::CheckActorDetected(Actor* apTarget, bool abCurrentlyDetected) {
    TESEffectShader* pEffectData = MagicSystem::GetDetectLifeShaderEffect();
    if (!pEffectData || !apTarget || apTarget == pPlayer || !apTarget->GetCurrentAIProcess()) {
        return false;
    }
    // Scales the range by a magnitude of 0.0f, rounding the product and its square to floats.
    float fmagnitude = 0.0f;
    float fDetectRange = fmagnitude * fMagicUnitsPerFoot.Float();
    fDetectRange = fDetectRange * fDetectRange;
    NiPoint3 kLocation = pPlayer->GetLocationOnReference();
    // Detected only where the squared range is above the squared distance (not on NaN).
    if (fDetectRange > (kLocation - apTarget->GetLocationOnReference()).SqrLength() &&
        !apTarget->IsDead(false) &&
        apTarget->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH &&
        !apTarget->GetDestroyed()) {
        if (!abCurrentlyDetected) {
            MagicHitEffect* peffect = new MagicShaderHitEffect(apTarget, pEffectData, -1.0f);
            if (peffect->Init()) {
                ReferenceProcessLists.AddTempEffect(peffect);
            }
            else if (peffect) {
                delete peffect;
            }
        }
        return true;
    }
    if (abCurrentlyDetected) {
        ReferenceProcessLists.FinishMagicShaderHitEffect(apTarget, pEffectData);
    }
    return false;
}
