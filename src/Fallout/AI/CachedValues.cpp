#include "Fallout/AI/CachedValues.h"

// fCachedSurvivalEffectivenessMult is left unset, as the exe's constructor leaves it.
CachedValues::CachedValues()
	: fCachedRadius(-1.0f), fCachedWidth(-1.0f), fCachedLength(-1.0f), fCachedForwardLength(-1.0f),
	  fCachedWeaponDPS(-1.0f), fCachedMedicineEffectivenessMult(-1.0f), fCachedParalysis(-1.0f),
	  fCachedHealingRate(-1.0f), fCachedEndurance(-1.0f), fCachedPerceptionCondition(-1.0f), fCachedEyeLevel(-1.0f),
	  eCachedActorAggression(-1), eCachedActorAssistance(-1), fCachedWalkSpeed(0.0f), fCachedRunSpeed(0.0f),
	  bConditionPreventsRun(false), iFlags(0) {}

void CachedValues::SetDirty(CachedValues::Flags aiFlag) {
	iFlags &= ~aiFlag;
}

// not reconstructed: kept as calls to the original engine functions
void CachedValues::SetUpToDate(CachedValues::Flags aiFlag) {
	iFlags |= aiFlag;
}

void CachedValues::SetFlag(CachedValues::Flags aiFlag, bool abSet) {
	if (abSet)
		iFlags |= aiFlag;
	else
		iFlags &= ~aiFlag;
}

// not reconstructed: kept as calls to the original engine functions
bool CachedValues::IsDirty(CachedValues::Flags aiFlag) {
	return ((aiFlag & iFlags) == 0);
}

void CachedValues::Copy(CachedValues* apSource) {
	if (apSource) {
		fCachedRadius = apSource->fCachedRadius;
		fCachedWidth = apSource->fCachedWidth;
		fCachedLength = apSource->fCachedLength;
		fCachedForwardLength = apSource->fCachedForwardLength;
		fCachedWeaponDPS = apSource->fCachedWeaponDPS;
		fCachedMedicineEffectivenessMult = apSource->fCachedMedicineEffectivenessMult;
		fCachedParalysis = apSource->fCachedParalysis;
		fCachedHealingRate = apSource->fCachedHealingRate;
		fCachedEndurance = apSource->fCachedEndurance;
		fCachedPerceptionCondition = apSource->fCachedPerceptionCondition;
		fCachedEyeLevel = apSource->fCachedEyeLevel;
		eCachedActorAggression = apSource->eCachedActorAggression;
		eCachedActorAssistance = apSource->eCachedActorAssistance;
		fCachedWalkSpeed = apSource->fCachedWalkSpeed;
		fCachedRunSpeed = apSource->fCachedRunSpeed;
		bConditionPreventsRun = (bool)((uint8_t)apSource->bConditionPreventsRun);
		iFlags = apSource->iFlags;
	}
}
