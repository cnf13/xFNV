#include "Fallout/AI/Creature.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/rtti.h"

// FindRagDoll's function-local static in the PDB (int idepth, Creature.obj): the recursion depth of the search.
static int32_t idepth;

Creature::Creature() : Actor(true), bRagDoll(false), fArmorRating(-1.0f), fArmorThreshold(-1.0f) {
	SetFormType(60u);
	iMajorCrimes = 0;
	iMinorCrimes = 0;
}

Creature::Creature(bool abadd) : Actor(abadd), bRagDoll(false), fArmorRating(-1.0f), fArmorThreshold(-1.0f) {
	SetFormType(0x3Cu);
	iMajorCrimes = 0;
	iMinorCrimes = 0;
}

Creature::~Creature() {
	if (!GetTemporary()) {
		Creature::Set3D(nullptr, false);
	}
}

// Loads the actor's state, then clears the major crime count.
void Creature::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor::LoadGame(apLoadGameBuffer);
	iMajorCrimes = 0;
}

void Creature::Set3D(NiAVObject* apObject, bool abTestProcessLevel) {
	TESObjectREFR::Set3D(apObject, abTestProcessLevel);
}

int32_t Creature::GetActorValue(ActorValue::Index aiWhich) {
	return Actor::GetActorValue(static_cast<ActorValue::Index>(MapActorValue(aiWhich)));
}

float Creature::GetActorFloatValue(ActorValue::Index aiWhich) {
	return Actor::GetActorFloatValue(static_cast<ActorValue::Index>(MapActorValue(aiWhich)));
}

float Creature::GetTurningSpeed() {
	float v3 = (float)0.0;
	TESActorBase* actorBase = Actor::GetActorBase();
	if (actorBase) {
		v3 = static_cast<TESCreature*>(actorBase)->fTurnSpeed;
	}
	if (v3 == 0.0) {
		return (&fCreatureDefaultTurningSpeed)->Float();
	}
	return v3;
}

float Creature::GetReach() {
	float DisableLightUpdate;
	TESActorBase* ActorBase = GetActorBase();
	TESCreature* v6 = dynamic_cast<TESCreature*>(ActorBase);
	if (v6) {
		DisableLightUpdate = static_cast<float>(v6->GetHandReach());
		if (v6->GetCreatureType() == 7) {
			return DisableLightUpdate * fCombatGiantCreatureReachMult.Float();
		}
	} else {
		return Actor::GetReach();
	}
	return DisableLightUpdate;
}

void Creature::SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
	ActorValue::Index v3 = MapActorValue(aiWhich);
	Actor::SetActorValue(v3, aiValue);
}

void Creature::SetActorValue(ActorValue::Index aiWhich, float afValue) {
	int32_t actorFloatValue = MapActorValue(aiWhich);
	Actor::SetActorValue(static_cast<ActorValue::Index>(actorFloatValue), afValue);
}

void Creature::TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
	int32_t v4 = MapActorValue(aiWhich);
	Actor::TempModActorValue(static_cast<ActorValue::Index>(v4), aiValue, apAttacker);
}

void Creature::TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
	int32_t v4 = MapActorValue(aiWhich);
	Actor::TempModActorValue(static_cast<ActorValue::Index>(v4), afValue, apAttacker);
}

void Creature::PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
	int32_t v4 = MapActorValue(aiWhich);
	Actor::PermanentModActorValue(static_cast<ActorValue::Index>(v4), aiValue, apAttacker);
}

void Creature::PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
	int32_t v4 = MapActorValue(aiWhich);
	Actor::PermanentModActorValue(static_cast<ActorValue::Index>(v4), afValue, apAttacker);
}

void Creature::DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
	int32_t v4 = MapActorValue(aiWhich);
	Actor::DamageModActorValue(static_cast<ActorValue::Index>(v4), aiValue, apAttacker);
}

void Creature::DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
	int32_t v4 = MapActorValue(aiWhich);
	Actor::DamageModActorValue(static_cast<ActorValue::Index>(v4), afValue, apAttacker);
}

void Creature::ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) {
	int32_t v3 = MapActorValue(aiWhich);
	Actor::ModActorBaseValue(static_cast<ActorValue::Index>(v3), aiValue);
}

void Creature::ModActorBaseValue(ActorValue::Index aiWhich, float afValue) {
	int32_t v3 = MapActorValue(aiWhich);
	Actor::ModActorBaseValue(static_cast<ActorValue::Index>(v3), afValue);
}

// Forgets the cached armor rating and threshold so they are recalculated.
void Creature::ResetArmorRating() {
	fArmorRating = -1.0f;
	fArmorThreshold = -1.0f;
}

float Creature::GetArmorRating() {
	if (!(fArmorRating >= 0.0)) {
		ActorValue::Index edamageResist = ActorValue::DAMAGE_RESISTANCE;
		// The exe calls Actor's slot 196 (0x87D880) directly.
		float fInternalValue = Actor::GetEquippedWeight();
		bool bFound = 0;
		float v7 = GetBaseValueOverride(ActorValue::DAMAGE_RESISTANCE, bFound);
		if (bFound) {
			fInternalValue = v7;
		}
		double v2 = GetPermanentModifier(edamageResist);
		double v5 = v2 + fInternalValue;
		double v4 = GetTemporaryModifier(edamageResist) + v5;
		fArmorRating = GetDamageModifier(edamageResist) + v4;
		return fArmorRating;
	} else {
		return fArmorRating;
	}
}

float Creature::GetArmorThreshold() {
	if (!(fArmorThreshold >= 0.0)) {
		ActorValue::Index edamageThreshold = ActorValue::DAMAGE_THRESHOLD;
		// The exe calls Actor's slot 196 (0x87D880) directly.
		float fBaseValue = Actor::GetEquippedWeight();
		bool bExists = 0;
		float fInternalValue = GetBaseValueOverride(edamageThreshold, bExists);
		if (bExists) {
			fBaseValue = fInternalValue;
		}
		double v3 = GetPermanentModifier(edamageThreshold);
		double v5 = v3 + fBaseValue;
		fArmorThreshold = GetDamageModifier(edamageThreshold) + v5;
		return fArmorThreshold;
	} else {
		return fArmorThreshold;
	}
}

int32_t Creature::GetAttackDamage() {
	TESForm* ActorBase = GetActorBase();
	int32_t AttackDamage_0 = TESAttackDamageForm::GetAttackDamage_0(ActorBase);
	float afFatiguePercent = GetFatiguePercentage();
	return static_cast<int32_t>(CombatFormulas::CalcCreatureDamage(afFatiguePercent, AttackDamage_0));
}

float Creature::CalculateWalkSpeed() {
	double v2;
	float v4 = (float)0.0;
	if (!IsOverEncumbered()) {
		v2 = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
		return (float)(GetMovementAnimationSpeed(static_cast<ANIM_GROUP_ENUM>(3)) * v2);
	}
	return v4;
}

float Creature::CalculateRunSpeed() {
	float fresult = 0.0f;
	if (!IsOverEncumbered()) {
		double fSpeedMult = GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
		return GetMovementAnimationSpeed(ANIM_GROUP_FAST_FORWARD) * fSpeedMult;
	}
	return fresult;
}

float Creature::GetWeaponSkill() {
	TESActorBase* actorBase = Actor::GetActorBase();
	if (dynamic_cast<TESCreature*>(actorBase)) {
		return (float)static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::UNARMED);
	} else {
		return (float)0;
	}
}

ActorValue::Index Creature::MapActorValue(ActorValue::Index aiWhich) {
	if (ActorValue::IsFlagSet(static_cast<ActorValue::Index>(aiWhich), ActorValue::COMBAT_SKILL)) {
		return ActorValue::UNARMED;
	}
	return aiWhich;
}

bool Creature::FindRagDoll(NiAVObject* apObject) {
	if (!apObject) {
		return false;
	}

	bhkNiCollisionObject* pCollisionObject = bhkNiCollisionObject::GetbhkNiCollisionObject(apObject);
	++idepth;

	bool bHasRagDoll = pCollisionObject != nullptr;
	if (!pCollisionObject && idepth < 4) {
		NiNode* pNode = apObject->IsNode();
		if (pNode) {
			bHasRagDoll = FindRagDoll(pNode->GetAtChecked(0));
		}
	}

	--idepth;
	return bHasRagDoll;
}

void Creature::CheckMissingLimbs() {
	TESCreature* pcrea = dynamic_cast<TESCreature*>(GetActorBase());
	if (pcrea) {
		Animation* panim = GetAnimation();
		if (panim) {
			if (panim->GetControllerManager()) {
				NiControllerManager* ControllerManager = panim->GetControllerManager();
				NiAVObjectPalette* ppalette = ControllerManager->GetObjectPalette();
				if (pcrea->GetNoLeftArm()) {
					NiFixedString* Bip01_L_UpperArm = FixedStrings::QBip01LUpperArm();
					NiAVObject* pobj = ppalette->GetAVObject(*Bip01_L_UpperArm);
					TESHavokUtilities::RemoveHavokFromSceneGraph(pobj);
				}
				if (pcrea->GetNoRightArm()) {
					NiFixedString* Bip01_R_UpperArm = FixedStrings::QBip01RUpperArm();
					NiAVObject* pobj_2 = ppalette->GetAVObject(*Bip01_R_UpperArm);
					TESHavokUtilities::RemoveHavokFromSceneGraph(pobj_2);
				}
				if (pcrea->GetNoHead()) {
					NiFixedString* Bip01_Head = FixedStrings::GetBip01_Head();
					NiAVObject* pobj_3 = ppalette->GetAVObject(*Bip01_Head);
					TESHavokUtilities::RemoveHavokFromSceneGraph(pobj_3);
				}
			}
		}
	}
}

float Creature::GetMovementAnimationSpeed(ANIM_GROUP_ENUM aeMoveMode) {
	if (!GetAnimation()) {
		return 0x1p+5f;
	}
	ItemChange* pWeapon = pCurrentProcess->GetCurrentWeapon();
	uint16_t animGroup = Actor::GetAnimGroup(aeMoveMode, pWeapon, 1, nullptr);
	Animation* pAnimation = GetAnimation();
	return (double)((int16_t)pAnimation->GetGroupSpeed(animGroup));
}
