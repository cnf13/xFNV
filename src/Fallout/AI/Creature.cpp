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

// The recursion depth of FindRagDoll's search.
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
    return Actor::GetActorValue(MapActorValue(aiWhich));
}

float Creature::GetActorFloatValue(ActorValue::Index aiWhich) {
    return Actor::GetActorFloatValue(MapActorValue(aiWhich));
}

float Creature::GetTurningSpeed() {
    float fTurnSpeed = 0.0f;
    TESActorBase* pActorBase = Actor::GetActorBase();
    if (pActorBase) {
        fTurnSpeed = static_cast<TESCreature*>(pActorBase)->fTurnSpeed;
    }
    if (fTurnSpeed == 0.0) {
        return fCreatureDefaultTurningSpeed.Float();
    }
    return fTurnSpeed;
}

float Creature::GetReach() {
    float fReach;
    TESActorBase* pActorBase = GetActorBase();
    TESCreature* pCreature = dynamic_cast<TESCreature*>(pActorBase);
    if (pCreature) {
        fReach = static_cast<float>(pCreature->GetHandReach());
        if (pCreature->GetCreatureType() == 7) {
            return fReach * fCombatGiantCreatureReachMult.Float();
        }
    }
    else {
        return Actor::GetReach();
    }
    return fReach;
}

void Creature::SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
    ActorValue::Index eWhich = MapActorValue(aiWhich);
    Actor::SetActorValue(eWhich, aiValue);
}

void Creature::SetActorValue(ActorValue::Index aiWhich, float afValue) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::SetActorValue(static_cast<ActorValue::Index>(iWhich), afValue);
}

void Creature::TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::TempModActorValue(static_cast<ActorValue::Index>(iWhich), aiValue, apAttacker);
}

void Creature::TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::TempModActorValue(static_cast<ActorValue::Index>(iWhich), afValue, apAttacker);
}

void Creature::PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::PermanentModActorValue(static_cast<ActorValue::Index>(iWhich), aiValue, apAttacker);
}

void Creature::PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::PermanentModActorValue(static_cast<ActorValue::Index>(iWhich), afValue, apAttacker);
}

void Creature::DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::DamageModActorValue(static_cast<ActorValue::Index>(iWhich), aiValue, apAttacker);
}

void Creature::DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::DamageModActorValue(static_cast<ActorValue::Index>(iWhich), afValue, apAttacker);
}

void Creature::ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::ModActorBaseValue(static_cast<ActorValue::Index>(iWhich), aiValue);
}

void Creature::ModActorBaseValue(ActorValue::Index aiWhich, float afValue) {
    int32_t iWhich = MapActorValue(aiWhich);
    Actor::ModActorBaseValue(static_cast<ActorValue::Index>(iWhich), afValue);
}

// Forgets the cached armor rating and threshold so they are recalculated.
void Creature::ResetArmorRating() {
    fArmorRating = -1.0f;
    fArmorThreshold = -1.0f;
}

float Creature::GetArmorRating() {
    if (!(fArmorRating >= 0.0)) {
        ActorValue::Index edamageResist = ActorValue::DAMAGE_RESISTANCE;
        // Calls Actor::GetEquippedWeight directly rather than through virtual dispatch.
        float fInternalValue = Actor::GetEquippedWeight();
        bool bFound = false;
        float fOverride = GetBaseValueOverride(ActorValue::DAMAGE_RESISTANCE, bFound);
        if (bFound) {
            fInternalValue = fOverride;
        }
        double dPermanent = GetPermanentModifier(edamageResist);
        double dWithPermanent = dPermanent + fInternalValue;
        double dWithTemporary = GetTemporaryModifier(edamageResist) + dWithPermanent;
        fArmorRating = GetDamageModifier(edamageResist) + dWithTemporary;
        return fArmorRating;
    }

    return fArmorRating;
}

float Creature::GetArmorThreshold() {
    if (!(fArmorThreshold >= 0.0)) {
        ActorValue::Index edamageThreshold = ActorValue::DAMAGE_THRESHOLD;
        // Calls Actor::GetEquippedWeight directly rather than through virtual dispatch.
        float fBaseValue = Actor::GetEquippedWeight();
        bool bExists = false;
        float fInternalValue = GetBaseValueOverride(edamageThreshold, bExists);
        if (bExists) {
            fBaseValue = fInternalValue;
        }
        double dPermanent = GetPermanentModifier(edamageThreshold);
        double dWithPermanent = dPermanent + fBaseValue;
        fArmorThreshold = GetDamageModifier(edamageThreshold) + dWithPermanent;
        return fArmorThreshold;
    }

    return fArmorThreshold;
}

int32_t Creature::GetAttackDamage() {
    TESForm* pActorBase = GetActorBase();
    int32_t iAttackDamage = TESAttackDamageForm::GetAttackDamage_0(pActorBase);
    float fFatiguePercent = GetFatiguePercentage();
    return static_cast<int32_t>(CombatFormulas::CalcCreatureDamage(fFatiguePercent, iAttackDamage));
}

float Creature::CalculateWalkSpeed() {
    float fresult = 0.0f;
    if (!IsOverEncumbered()) {
        double dSpeedMult = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
        return static_cast<float>(GetMovementAnimationSpeed(ANIM_GROUP_MOVE_FORWARD) * dSpeedMult);
    }
    return fresult;
}

float Creature::CalculateRunSpeed() {
    float fresult = 0.0f;
    if (!IsOverEncumbered()) {
        double dSpeedMult = GetActorFloatValue(ActorValue::SPEED_MULT) * 0.01;
        return GetMovementAnimationSpeed(ANIM_GROUP_FAST_FORWARD) * dSpeedMult;
    }
    return fresult;
}

float Creature::GetWeaponSkill() {
    TESActorBase* pActorBase = Actor::GetActorBase();
    if (dynamic_cast<TESCreature*>(pActorBase)) {
        return static_cast<float>(static_cast<ActorValueOwner*>(this)->GetActorValue(ActorValue::UNARMED));
    }

    return 0.0f;
}

ActorValue::Index Creature::MapActorValue(ActorValue::Index aiWhich) {
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::COMBAT_SKILL)) {
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
        if (panim && panim->GetControllerManager()) {
            NiControllerManager* pControllerManager = panim->GetControllerManager();
            NiAVObjectPalette* ppalette = pControllerManager->GetObjectPalette();
            if (pcrea->GetNoLeftArm()) {
                NiFixedString* pName = FixedStrings::QBip01LUpperArm();
                NiAVObject* pobj = ppalette->GetAVObject(*pName);
                TESHavokUtilities::RemoveHavokFromSceneGraph(pobj);
            }
            if (pcrea->GetNoRightArm()) {
                NiFixedString* pName = FixedStrings::QBip01RUpperArm();
                NiAVObject* pobj = ppalette->GetAVObject(*pName);
                TESHavokUtilities::RemoveHavokFromSceneGraph(pobj);
            }
            if (pcrea->GetNoHead()) {
                NiFixedString* pName = FixedStrings::GetBip01_Head();
                NiAVObject* pobj = ppalette->GetAVObject(*pName);
                TESHavokUtilities::RemoveHavokFromSceneGraph(pobj);
            }
        }
    }
}

float Creature::GetMovementAnimationSpeed(ANIM_GROUP_ENUM aeMoveMode) {
    if (!GetAnimation()) {
        return 0x1p+5f;
    }
    ItemChange* pWeapon = pCurrentProcess->GetCurrentWeapon();
    uint16_t animGroup = Actor::GetAnimGroup(aeMoveMode, pWeapon, true, nullptr);
    Animation* pAnimation = GetAnimation();
    return static_cast<double>(pAnimation->GetGroupSpeed(animGroup));
}
