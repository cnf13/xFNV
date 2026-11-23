#include "Fallout Shared/ActorValueOwner.h"

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/TESLeveledList.h"

#include <cmath>

int32_t ActorValueOwner::GetPermanentActorValue(ActorValue::Index aiWhich) {
    return static_cast<int32_t>(std::floorf(GetPermanentActorFloatValue(aiWhich)));
}

float ActorValueOwner::GetPermanentActorFloatValue(ActorValue::Index aiWhich) {
    return GetBaseActorFloatValue(aiWhich) + GetPermanentModifier(aiWhich);
}

int32_t ActorValueOwner::GetClampedActorValue(ActorValue::Index aiWhich) {
    return static_cast<int32_t>(std::floorf(GetClampedActorFloatValue(aiWhich)));
}

float ActorValueOwner::GetClampedActorFloatValue(ActorValue::Index aiWhich) {
    return ActorValue::ClampActorValue(aiWhich, GetActorFloatValue(aiWhich));
}

float ActorValueOwner::CalculateFlySpeed(ActorValueOwner* apOwner, float afFlyInventoryWeight) {
    float fSpeedMult = apOwner->GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
    float fMinSpeed = fMoveMinFlySpeed.Float();
    float fSpeed = (fMoveMaxFlySpeed.Float() - fMoveMinFlySpeed.Float()) * fSpeedMult + fMinSpeed;
    if (fSpeed < 0.0) {
        return 0.0;
    }

    return fSpeed;
}

char ActorValueOwner::ShouldDamageKnockdownTarget(ActorValueOwner* apTarget, int32_t aiDamage) {
    float fHealth = apTarget->GetActorFloatValue(ActorValue::HEALTH);
    if (fHealth > 0.0) {
        float fHealthLeft = fHealth - static_cast<double>(aiDamage);
        if (fHealthLeft > 0.0) {
            float fBaseHealthRatio = static_cast<double>(aiDamage) /
                apTarget->GetBaseActorFloatValue(ActorValue::HEALTH);
            float fBaseHealthPercent = fBaseHealthRatio * 100.0;
            if (fKnockdownBaseHealthThreshold.Float() < static_cast<double>(fBaseHealthPercent)) {
                return 1;
            }
            float fCurrentHealthRatio = static_cast<double>(aiDamage) / fHealth;
            float fCurrentHealthPercent = fCurrentHealthRatio * 100.0;
            if (fKnockdownCurrentHealthThreshold.Float() < static_cast<double>(fCurrentHealthPercent)) {
                float fAgility = apTarget->GetClampedActorFloatValue(ActorValue::AGILITY) * 10.0;
                float fAgilityScore = fKnockdownAgilMult.Float() * fAgility + fKnockdownAgilBase.Float();
                float fDamageScore = static_cast<double>(aiDamage) * fKnockdownDamageMult.Float() +
                    fKnockdownDamageBase.Float();
                float fChance = fDamageScore / fAgilityScore;
                if (fKnockdownChance.Float() <= static_cast<double>(fChance)) {
                    fChance = fKnockdownChance.Float();
                }
                float fScaledChance = fChance * 1000.0;
                // Divides unsigned, then compares the remainder signed.
                if (static_cast<int32_t>(Random() % 1000) < static_cast<int32_t>(fScaledChance)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}
