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
	float x = GetClampedActorFloatValue(aiWhich);
	return (int32_t)std::floorf(x);
}

float ActorValueOwner::GetClampedActorFloatValue(ActorValue::Index aiWhich) {
	float v3 = (float)GetActorFloatValue(aiWhich);
	return (float)ActorValue::ClampActorValue(static_cast<ActorValue::Index>(aiWhich), v3);
}

float ActorValueOwner::CalculateFlySpeed(ActorValueOwner* apOwner, float afFlyInventoryWeight) {
	float v5 = apOwner->GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
	float fMoveMinFlySpeeda = ::fMoveMinFlySpeed.Float();
	float* v2 = &fMoveMaxFlySpeed.Float();
	float fMoveMinFlySpeed = (*v2 - ::fMoveMinFlySpeed.Float()) * v5 + fMoveMinFlySpeeda;
	if (!(fMoveMinFlySpeed < 0.0)) {
		return fMoveMinFlySpeed;
	} else {
		return 0.0;
	}
}

char ActorValueOwner::ShouldDamageKnockdownTarget(ActorValueOwner* a1, int32_t a2) {
	char v17 = 0;
	float v16 = a1->GetActorFloatValue(ActorValue::HEALTH);
	if (v16 > 0.0) {
		float v15 = v16 - static_cast<double>(a2);
		if (v15 > 0.0) {
			float v13 = static_cast<double>(a2) / a1->GetBaseActorFloatValue(ActorValue::HEALTH);
			float v14 = v13 * 100.0;
			if (!(fKnockdownBaseHealthThreshold.Float() < static_cast<double>(v14))) {
				float v11 = static_cast<double>(a2) / v16;
				float v12 = v11 * 100.0;
				if (fKnockdownCurrentHealthThreshold.Float() < static_cast<double>(v12)) {
					float v6 = a1->GetClampedActorFloatValue(ActorValue::AGILITY) * 10.0;
					float* v3 = &fKnockdownAgilBase.Float();
					float v10 = fKnockdownAgilMult.Float() * v6 + *v3;
					float* v4 = &fKnockdownDamageBase.Float();
					float v7 = static_cast<double>(a2) * fKnockdownDamageMult.Float() + *v4;
					float v8 = v7 / v10;
					if (fKnockdownChance.Float() <= static_cast<double>(v8)) {
						v8 = fKnockdownChance.Float();
					}
					float v9 = v8 * 1000.0;
					// The exe divides unsigned, then compares the remainder signed (jge).
					if (static_cast<int32_t>(Random() % 1000) < static_cast<int32_t>(v9)) {
						return 1;
					}
				}
			} else {
				return 1;
			}
		}
	}
	return v17;
}
