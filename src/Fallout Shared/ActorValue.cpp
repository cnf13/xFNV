#include "Fallout Shared/ActorValue.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/rtti.h"

#include <stdarg.h>

ActorValue ActorValue::Instance;

ActorValue::ActorValue() : pActorValueInfo{} {}

ActorValue::~ActorValue() {
    FreeActorValues();
}

ActorValueInfo* ActorValue::GetActorValueInfo(Index aiIndex) {
    if (aiIndex >= AGGRESSION && aiIndex < ACTOR_VALUE_COUNT) {
        return Instance.pActorValueInfo[aiIndex];
    }
    return nullptr;
}

const char* ActorValue::GetActorValueAbbreviation(Index aiIndex) {
    int8_t* v3 = nullptr;
    ActorValueInfo* actorValueInfo = ActorValue::GetActorValueInfo(aiIndex);
    if (actorValueInfo) {
        return (&actorValueInfo->sAbbreviation)->pString;
    }
    return static_cast<const char*>(static_cast<const void*>(v3));
}

ActorValue::Index* ActorValue::GetDependentActorValues(Index aiWhich, int32_t& aiCount) {
    Index* pDependents = nullptr;
    aiCount = 0;
    ActorValueInfo* pInfo = GetActorValueInfo(aiWhich);
    if (pInfo) {
        pDependents = reinterpret_cast<Index*>(pInfo->DependentActorValues);
        aiCount = static_cast<int32_t>(pInfo->iNumDependentActorValues);
    }
    return pDependents;
}

bool ActorValue::HasModifiedCallback(Index aiWhich) {
    ActorValueInfo* pInfo = GetActorValueInfo(aiWhich);
    return pInfo && pInfo->pModifiedCallback;
}

void ActorValue::CheckCallModifiedCallback(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    ActorValueInfo* pinfo = ActorValue::GetActorValueInfo(aiWhich);
    if (pinfo) {
        if (pinfo->pModifiedCallback) {
            pinfo->pModifiedCallback(apActor, aiWhich, afOriginalValue, afDelta, apActorSource);
        }
    }
}

bool ActorValue::IsSkill(Index aiIndex) {
    return aiIndex >= BARTER && aiIndex < INVENTORY_WEIGHT;
}

bool ActorValue::IsSPECIAL(Index aiIndex) {
    return aiIndex >= STRENGTH && aiIndex < ACTION_POINTS;
}

bool ActorValue::IsDerivedAttribute(Index aiIndex) {
    return aiIndex >= ACTION_POINTS && aiIndex < BARTER;
}

bool ActorValue::IsAIAttribute(Index aiIndex) {
    return aiIndex >= AGGRESSION && aiIndex < STRENGTH;
}

bool ActorValue::IsFlagSet(Index aiIndex, Flags aiFlag) {
    return (aiFlag & Instance.pActorValueInfo[aiIndex]->uFlags) != 0;
}

char ActorValue::ToArrayIndex(ActorValue::Section aiSection, Index aiIndex) {
    switch (aiSection) {
        case 0: return static_cast<int8_t>(aiIndex - 5);
        case 1: return static_cast<int8_t>(aiIndex - 12);
        case 2: return static_cast<int8_t>(aiIndex - 32);
        case 3: return static_cast<int8_t>(aiIndex);
        case 4: return static_cast<int8_t>(aiIndex - 25);
        case 5: return static_cast<int8_t>(aiIndex - 46);
        default: return 0;
    }
}

ActorValue::Index ActorValue::ToActorValue(ActorValue::Section aiSection, char aiIndex) {
    switch (aiSection) {
        case 0: return static_cast<Index>(aiIndex + 5);
        case 1: return static_cast<Index>(aiIndex + 12);
        case 2: return static_cast<Index>(aiIndex + 32);
        case 3: return static_cast<Index>(aiIndex);
        case 4: return static_cast<Index>(aiIndex + 25);
        case 5: return static_cast<Index>(aiIndex + 46);
        default: return static_cast<Index>(0);
    }
}

ActorValue::Section ActorValue::GetSection(Index aiWhich) {
    if (IsSPECIAL(aiWhich)) {
        return ActorValue::ATTRIBUTE;
    }
    if (IsDerivedAttribute(aiWhich)) {
        return ActorValue::DERIVED_ATTRIBUTE;
    }
    if (IsSkill(aiWhich)) {
        return ActorValue::SKILL;
    }
    if (IsAIAttribute(aiWhich)) {
        return ActorValue::AI_ATTRIBUTE;
    }
    return ActorValue::INT_VALUE;
}

// Passes the clamped value as BSMAX's second argument and tests MIN_ONE before calling BSMIN.
float ActorValue::ClampActorValue(Index aiWhich, float afValue) {
    if (IsFlagSet(aiWhich, MAX_ONE_HUNDRED)) {
        return BSMAX(0.0f, BSMIN(afValue, 100.0f));
    }
    if (IsFlagSet(aiWhich, MAX_TEN)) {
        float fMin = IsFlagSet(aiWhich, MIN_ONE) ? 1.0f : 0.0f;
        return BSMAX(fMin, BSMIN(afValue, 10.0f));
    }
    return afValue;
}

uint32_t ActorValue::GetSectionCount(ActorValue::Section aiSection) {
    uint32_t result;
    switch (aiSection) {
        case 0: {
            result = 7;
            break;
        }
        case 1: {
            result = 20;
            break;
        }
        case 2: {
            result = 14;
            break;
        }
        case 3: {
            result = 5;
            break;
        }
        case 4: {
            result = 7;
            break;
        }
        case 5: {
            result = 16;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

const char* ActorValue::GetActorValueName(ActorValue::Index aiIndex) {
    ActorValueInfo* pinfo = ActorValue::GetActorValueInfo(aiIndex);
    if (!pinfo) {
        return 0;
    }
    if (pinfo->GetFullName() && BSstrlen(pinfo->GetFullName())) {
        return pinfo->GetFullName();
    }
    return pinfo->sScriptName;
}

const char* ActorValue::GetActorValueScriptName(ActorValue::Index aiIndex) {
    int8_t* v3 = nullptr;
    ActorValueInfo* actorValueInfo = ActorValue::GetActorValueInfo(aiIndex);
    if (actorValueInfo) {
        return actorValueInfo->sScriptName;
    }
    return static_cast<char*>(static_cast<void*>(v3));
}

const char* ActorValue::GetActorValueDesc(ActorValue::Index aiIndex) {
    ActorValueInfo* pinfo = ActorValue::GetActorValueInfo(aiIndex);
    if (pinfo) {
        return pinfo->GetDescription(0, DESC_ID);
    }
    else {
        return 0;
    }
}

const char* ActorValue::GetActorValueIcon(ActorValue::Index aiIndex) {
    ActorValueInfo* ActorValueInfo_ = ActorValue::GetActorValueInfo(aiIndex);
    if (ActorValueInfo_) {
        return ActorValueInfo_->GetTextureName();
    }
    else {
        return 0;
    }
}

const char* ActorValue::GetItemWeaponTypeBadge(ActorValue::Index aiIndex) {
    const char* v2 = 0;
    switch (aiIndex) {
        case ActorValue::BIG_GUNS: {
            v2 = "Interface\\Icons\\TypeIcons\\weap_skill_icon_big_guns.dds";
            break;
        }
        case ActorValue::ENERGY_WEAPONS: {
            v2 = "Interface\\Icons\\TypeIcons\\weap_skill_icon_energy.dds";
            break;
        }
        case ActorValue::EXPLOSIVES: {
            v2 = "Interface\\Icons\\TypeIcons\\weap_skill_icon_explosives.dds";
            break;
        }
        case ActorValue::MELEE_WEAPONS: {
            v2 = "Interface\\Icons\\TypeIcons\\weap_skill_icon_melee.dds";
            break;
        }
        case ActorValue::SMALL_GUNS: {
            v2 = "Interface\\Icons\\TypeIcons\\weap_skill_icon_sm_arms.dds";
            break;
        }
        case ActorValue::UNARMED: {
            v2 = "Interface\\Icons\\TypeIcons\\weap_skill_icon_unarmed.dds";
            break;
        }
        default: {
            return v2;
        }
    }
    return v2;
}

void ActorValue::RegisterActorValues() {
    if (!bActorValuesRegistered) {
        ActorValue::RegisterActorValue(
            ActorValue::AGGRESSION, "Aggression", ActorValue::AI_ATTRIBUTE, 257u, 0, 0,
            &Actor::AggressionModifiedCallback, 0
        );
        ActorValueInfo* pinfo = ActorValue::GetActorValueInfo(ActorValue::AGGRESSION);
        if (pinfo) {
            pinfo->SetEnumerated(4, "Unaggressive", "Aggressive", "Very Aggressive", "Frenzied");
        }
        ActorValue::RegisterActorValue(
            ActorValue::ASSISTANCE, "Assistance", ActorValue::AI_ATTRIBUTE, 257u, 0, 0,
            &Actor::AssistanceModifiedCallback, 0
        );
        ActorValueInfo* pinfo_2 = ActorValue::GetActorValueInfo(ActorValue::ASSISTANCE);
        if (pinfo_2) {
            pinfo_2->SetEnumerated(3, "Helps Nobody", "Helps Allies", "Helps Friends and Allies");
        }
        ActorValue::RegisterActorValue(
            ActorValue::CONFIDENCE, "Confidence", ActorValue::AI_ATTRIBUTE, 257u, 0, 0, 0, 0
        );
        ActorValueInfo* pinfo_3 = ActorValue::GetActorValueInfo(ActorValue::CONFIDENCE);
        if (pinfo_3) {
            pinfo_3->SetEnumerated(5, "Cowardly", "Cautious", "Average", "Brave", "Foolhardy");
        }
        ActorValue::RegisterActorValue(ActorValue::ENERGY, "Energy", ActorValue::AI_ATTRIBUTE, 1u, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(
            ActorValue::RESPONSIBILITY, "Responsibility", ActorValue::AI_ATTRIBUTE, 1u, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(ActorValue::MOOD, "Mood", ActorValue::AI_ATTRIBUTE, 257u, 0, 0, 0, 0);
        ActorValueInfo* pinfo_4 = ActorValue::GetActorValueInfo(ActorValue::MOOD);
        if (pinfo_4) {
            pinfo_4->SetEnumerated(
                9, "Neutral", "Afraid", "Annoyed", "Cocky", "Drugged", "Pleasant", "Angry", "Sad", "UNDEFINED"
            );
        }
        ActorValue::RegisterActorValue(
            ActorValue::STRENGTH, "Strength", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0, 0, 3, ActorValue::CARRY_WEIGHT,
            ActorValue::MELEE_WEAPONS, ActorValue::MELEE_DAMAGE
        );
        ActorValue::RegisterActorValue(
            ActorValue::PERCEPTION, "Perception", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0, 0, 3,
            ActorValue::ENERGY_WEAPONS, ActorValue::LOCKPICK, ActorValue::EXPLOSIVES
        );
        ActorValue::RegisterActorValue(
            ActorValue::ENDURANCE, "Endurance", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0,
            &Actor::EnduranceModifiedCallback, 7, ActorValue::HEALTH, ActorValue::HEALING_RATE,
            ActorValue::POISON_RESISTANCE, ActorValue::RADIATION_RESISTANCE, ActorValue::BIG_GUNS, ActorValue::UNARMED,
            ActorValue::SURVIVAL
        );
        ActorValue::RegisterActorValue(
            ActorValue::CHARISMA, "Charisma", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0, 0, 2, ActorValue::BARTER,
            ActorValue::SPEECH
        );
        ActorValue::RegisterActorValue(
            ActorValue::INTELLIGENCE, "Intelligence", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0, 0, 3, ActorValue::SCIENCE,
            ActorValue::REPAIR, ActorValue::MEDICINE
        );
        ActorValue::RegisterActorValue(
            ActorValue::AGILITY, "Agility", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0, 0, 3,
            ActorValue::ACTION_POINTS, ActorValue::SMALL_GUNS, ActorValue::SNEAK
        );
        ActorValue::RegisterActorValue(
            ActorValue::LUCK, "Luck", ActorValue::ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::MAX_TEN | ActorValue::MIN_ONE, 0, 0, 0, 15,
            ActorValue::CRITICAL_CHANCE, ActorValue::BARTER, ActorValue::BIG_GUNS, ActorValue::ENERGY_WEAPONS,
            ActorValue::EXPLOSIVES, ActorValue::LOCKPICK, ActorValue::MEDICINE, ActorValue::MELEE_WEAPONS,
            ActorValue::REPAIR, ActorValue::SCIENCE, ActorValue::SMALL_GUNS, ActorValue::SNEAK, ActorValue::SPEECH,
            ActorValue::UNARMED, ActorValue::SURVIVAL
        );
        ActorValue::RegisterActorValue(
            ActorValue::ACTION_POINTS, "ActionPoints", ActorValue::DERIVED_ATTRIBUTE, 0,
            &AiFormulas::AVDeriveActionPoints, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::CARRY_WEIGHT, "CarryWeight", ActorValue::DERIVED_ATTRIBUTE, 2049u,
            &AiFormulas::AVDeriveCarryWeight, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::CRITICAL_CHANCE, "CritChance", ActorValue::DERIVED_ATTRIBUTE, 2048u,
            &AiFormulas::AVDeriveCriticalChance, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::HEALING_RATE, "HealRate", ActorValue::DERIVED_ATTRIBUTE, 2049u,
            &AiFormulas::AVDeriveHealingRate, 0, &Actor::HealingRateModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::HEALTH, "Health", ActorValue::DERIVED_ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::DERIVED_PLUS_BASE | ActorValue::DERIVED_IF_AUTOCALC |
                ActorValue::CREATURE_NOT_DERIVED,
            &AiFormulas::AVDeriveHealth, 0, &Actor::HealthFatigueModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::MELEE_DAMAGE, "MeleeDamage", ActorValue::DERIVED_ATTRIBUTE, 2048u,
            &AiFormulas::AVDeriveFromPrimary, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::DAMAGE_RESISTANCE, "DamageResist", ActorValue::RESISTANCE, 2048u,
            &Actor::GetBaseDamageResistance, 0, &Actor::ResetActorDTDR, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::POISON_RESISTANCE, "PoisonResist", ActorValue::RESISTANCE, 2048u,
            &AiFormulas::AVDeriveResistance, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::RADIATION_RESISTANCE, "RadResist", ActorValue::RESISTANCE, 2048u,
            &AiFormulas::AVDeriveResistance, 0, &Actor::HandleChange_008BA2B0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::FIRE_RESISTANCE, "FireResist", ActorValue::RESISTANCE, 0, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::ELECTRICAL_RESISTANCE, "ElectricResist", ActorValue::RESISTANCE, 0, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::FROST_RESISTANCE, "FrostResist", ActorValue::RESISTANCE, 0, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::ENERGY_RESISTANCE, "EnergyResist", ActorValue::RESISTANCE, 0, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(ActorValue::EMP_RESISTANCE, "EmpResist", ActorValue::RESISTANCE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(
            ActorValue::SPEED_MULT, "SpeedMult", ActorValue::DERIVED_ATTRIBUTE, 2049u, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::FATIGUE, "Fatigue", ActorValue::DERIVED_ATTRIBUTE,
            ActorValue::FAST_MODIFIER | ActorValue::DERIVED_PLUS_BASE | ActorValue::DERIVED_ALWAYS |
                ActorValue::CREATURE_NOT_DERIVED,
            &AiFormulas::AVDeriveFatigue, 0, &Actor::HealthFatigueModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::PERCEPTION_CONDITION, "PerceptionCondition", ActorValue::CONDITION, 2081u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::ConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::ENDURANCE_CONDITION, "EnduranceCondition", ActorValue::CONDITION, 2081u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::ConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::LEFT_ATTACK_CONDITION, "LeftAttackCondition", ActorValue::CONDITION, 2081u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::ConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::RIGHT_ATTACK_CONDITION, "RightAttackCondition", ActorValue::CONDITION, 2081u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::ConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::LEFT_MOBILITY_CONDITION, "LeftMobilityCondition", ActorValue::CONDITION, 2081u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::MobilityConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::RIGHT_MOBILITY_CONDITION, "RightMobilityCondition", ActorValue::CONDITION, 2081u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::MobilityConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::BRAIN_CONDITION, "BrainCondition", ActorValue::CONDITION, 2080u,
            &AiFormulas::AVDeriveBodyPartCondition, 0, &Actor::ConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::KARMA, "Karma", ActorValue::DERIVED_ATTRIBUTE, ActorValue::NO_SCRIPT_MODAV, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::EXPERIENCE_POINTS, "XP", ActorValue::DERIVED_ATTRIBUTE, ActorValue::NO_SCRIPT_MODAV, 0, 0,
            &ActorValue::HandleXPChange, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::UNARMED_DAMAGE, "UnarmedDamage", ActorValue::DERIVED_ATTRIBUTE, 2048u,
            &AiFormulas::AVDeriveUnarmedDamage, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::BARTER, "Barter", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::BIG_GUNS, "BigGuns", ActorValue::SKILL, ActorValue::DEPRECATED, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::ENERGY_WEAPONS, "EnergyWeapons", ActorValue::SKILL, 1040u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::EXPLOSIVES, "Explosives", ActorValue::SKILL, 1040u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::LOCKPICK, "Lockpick", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::MEDICINE, "Medicine", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0,
            &Actor::HandleChange_008B9AB0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::MELEE_WEAPONS, "MeleeWeapons", ActorValue::SKILL, 1040u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::REPAIR, "Repair", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::SCIENCE, "Science", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::SMALL_GUNS, "Guns", ActorValue::SKILL, 1040u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::SNEAK, "Sneak", ActorValue::SKILL, 17u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::SPEECH, "Speech", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::SURVIVAL, "Survival", ActorValue::SKILL, 16u, &AiFormulas::AVDeriveSkill_0, 0,
            &Actor::HandleChange_008B9B10, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::UNARMED, "Unarmed", ActorValue::SKILL, 1040u, &AiFormulas::AVDeriveSkill_0, 0, 0, 1,
            ActorValue::UNARMED_DAMAGE
        );
        ActorValue::RegisterActorValue(
            ActorValue::INVENTORY_WEIGHT, "InventoryWeight", ActorValue::DERIVED_ATTRIBUTE, 1u,
            &Actor::GetInventoryWeight, 0, &Actor::HandleChange_008BA2F0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::PARALYSIS, "Paralysis", ActorValue::INT_VALUE, 1u, 0, 0, &Actor::ParalysisModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(ActorValue::INVISIBILITY, "Invisibility", ActorValue::INT_VALUE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::CHAMELEON, "Chameleon", ActorValue::INT_VALUE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::NIGHT_EYE_BONUS, "NightEye", ActorValue::INT_VALUE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::TURBO, "Turbo", ActorValue::INT_VALUE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(
            ActorValue::WATER_BREATHING, "WaterBreathing", ActorValue::INT_VALUE, 1u, 0, 0, 0, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::RADIATION_RADS, "RadiationRads", ActorValue::INT_VALUE, 544u, 0, 0,
            &Actor::CheckRadiationLevelChange, 0
        );
        ActorValue::RegisterActorValue(ActorValue::BLOODY_MESS, "BloodyMess", ActorValue::INT_VALUE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_01, "Variable01", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_02, "Variable02", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_03, "Variable03", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_04, "Variable04", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_05, "Variable05", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_06, "Variable06", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_07, "Variable07", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_08, "Variable08", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_09, "Variable09", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(ActorValue::VARIABLE_10, "Variable10", ActorValue::VARIABLE, 0, 0, 0, 0, 0);
        ActorValue::RegisterActorValue(
            ActorValue::IGNORE_CRIPPLED_LIMBS, "IgnoreCrippledLimbs", ActorValue::VARIABLE, 1u, 0, 0,
            &Actor::ConditionModifiedCallback, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::DEHYDRATION, "Dehydration", ActorValue::INT_VALUE, 545u, 0, 0,
            &Actor::CheckDehydrationLevelChange, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::HUNGER, "Hunger", ActorValue::INT_VALUE, 545u, 0, 0, &Actor::CheckHungerLevelChange, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::SLEEPDEPREVATION, "SleepDeprevation", ActorValue::INT_VALUE, 545u, 0, 0,
            &Actor::CheckSleepDeprevationLevelChange, 0
        );
        ActorValue::RegisterActorValue(
            ActorValue::DAMAGE_THRESHOLD, "DamageThreshold", ActorValue::RESISTANCE, 2048u, &Actor::GetTotalArmorDT, 0,
            &Actor::ResetActorDTDR, 0
        );
        for (int32_t i = 0; i < 77; ++i) {
            ;
        }
        bActorValuesRegistered = 1;
    }
}

// The entry is dereferenced even when ourMalloc fails, as the game does.
void ActorValue::RegisterActorValue(
    Index aiWhich,
    const char* asScriptName,
    Section aeAVType,
    uint32_t auFlags,
    float (*apDerivationFunction)(ActorValueOwner*, Index),
    Index (*apMappingFunction)(ActorValueOwner*, Index),
    void (*apModifiedCallback)(ActorValueOwner*, Index, float, float, ActorValueOwner*),
    int32_t aiNumDependentActorValues,
    ...
) {
    if (Instance.pActorValueInfo[aiWhich]) {
        return;
    }
    void* pMemory = ourMalloc(sizeof(ActorValueInfo));
    ActorValueInfo* pInfo =
        pMemory ? ::new (pMemory) ActorValueInfo(aiWhich, const_cast<char*>(asScriptName)) : nullptr;
    pInfo->eAVType = aeAVType;
    pInfo->uFlags = auFlags;
    pInfo->pDerivationFunction = apDerivationFunction;
    pInfo->pMappingFunction = apMappingFunction;
    pInfo->pModifiedCallback = apModifiedCallback;
    pInfo->iNumDependentActorValues = BSMIN(aiNumDependentActorValues, 15);
    if (aiNumDependentActorValues > 0) {
        va_list vList;
        va_start(vList, aiNumDependentActorValues);
        // The count is compared signed.
        for (int32_t i = 0; i < static_cast<int32_t>(pInfo->iNumDependentActorValues); ++i) {
            pInfo->DependentActorValues[i] = va_arg(vList, int32_t);
        }
        va_end(vList);
    }
    Instance.pActorValueInfo[aiWhich] = pInfo;
}

bool ActorValue::GetDerivedActorValue(ActorValueOwner* apActor, ActorValue::Index aiWhich, float& afValue) {
    bool v5 = 0;
    afValue = 0.0f;
    ActorValueInfo* pValueInfo = ActorValue::GetActorValueInfo(aiWhich);
    if (pValueInfo) {
        if (pValueInfo->pDerivationFunction) {
            afValue = pValueInfo->pDerivationFunction(apActor, aiWhich);
            return 1;
        }
    }
    return v5;
}

ActorValue::Index ActorValue::GetActorValueIndex(char* acScriptName) {
    int32_t i;
    int32_t v3 = 77;
    for (i = 0; (i < 77) && (v3 == 77); ++i) {
        if (!BSstricmp(ActorValue::Instance.pActorValueInfo[i]->sScriptName, acScriptName)) {
            v3 = i;
        }
    }
    return (ActorValue::Index)v3;
}

bool ActorValue::IsResistance(ActorValue::Index aiIndex) {
    int8_t v2 = 0;
    switch ((int32_t)aiIndex) {
        case 18:
        case 19:
        case 20:
        case 52:
        case 58:
        case 59:
        case 60:
        case 61: {
            v2 = 1;
            break;
        }
        default: {
            return (bool)v2;
        }
    }
    return (bool)v2;
}

int32_t ActorValue::CompareForSorting(ListBoxItem<ActorValue::Index>* a1, ListBoxItem<ActorValue::Index>* a2) {
    const char* actorValueName = ActorValue::GetActorValueName(a1->data);
    const char* v3 = ActorValue::GetActorValueName(a2->data);
    return BSstrcmp(actorValueName, v3);
}

void ActorValue::FreeActorValues() {
    for (int32_t i = 0; i < 77; ++i) {
        if (ActorValue::Instance.pActorValueInfo[i]) {
            delete ActorValue::Instance.pActorValueInfo[i];
            ActorValue::Instance.pActorValueInfo[i] = 0;
        }
    }
    bActorValuesRegistered = 0;
}

void ActorValue::HandleXPChange(
    ActorValueOwner* apActor,
    Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    Actor* pActor = static_cast<Actor*>(apActor);
    if (pActor) {
        if (pActor->IsPc()) {
            CharacterProgression* pProgression = pPlayer->GetCharacterProgressionInfo();
            if (!pProgression->pData->bIgnoreCallback && aiWhich == EXPERIENCE_POINTS && afDelta > 0.0) {
                if (static_cast<int32_t>(afDelta) > 0) {
                    HUDMainMenu::QueueXPMessage(static_cast<int32_t>(afDelta));
                }
                uint16_t Level = pPlayer->GetLevel();
                if (Level < iMaxCharacterLevel.Int()) {
                    uint16_t v5 = pPlayer->GetLevel();
                    pProgression->pData->CheckCanLevelUp(v5 + 1);
                }
            }
        }
    }
}

ActorValueInfo::ActorValueInfo(ActorValue::Index aiWhich, char* apScriptName)
  : sScriptName(apScriptName), uFlags(0), pDerivationFunction(nullptr), pMappingFunction(nullptr),
    pModifiedCallback(nullptr), iNumDependentActorValues(0), iEnumCount(0), sEnumNames{} {
    cFormType = AVIF_ID;
    if (aiWhich != ActorValue::VALUE_NONE) {
        TESForm::SetFormID(GetActorValueFormID(aiWhich), 0);
    }
    if (apScriptName) {
        char acEditorID[108];
        BSsprintf(acEditorID, 100, "AV%s", apScriptName);
        TESForm::SetFormEditorID(acEditorID);
    }
    sAbbreviation = "";
    for (int32_t i = 0; i < 15; ++i) {
        DependentActorValues[i] = ActorValue::ACTOR_VALUE_COUNT;
    }
}

ActorValueInfo::~ActorValueInfo() {}

// Reads the value's name, description, icon, abbreviation and editor ID.
bool ActorValueInfo::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != AVIF_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk > FULL_ID) {
            if (uiChunk == ANAM_ID) {
                // a zero-size chunk clears the abbreviation
                if (apFile->GetChunkSize() != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                    apFile->GetChunkData(pBuffer, 0);
                    sAbbreviation = pBuffer;
                }
                else {
                    sAbbreviation = "";
                }
            }
            else if (uiChunk == ICON_ID) {
                TESTexture::LoadTextureChunk(this, apFile);
            }
        }
        else {
            switch (uiChunk) {
                case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
                case DESC_ID: TESDescription::LoadDescriptionData(this, apFile); break;
                case EDID_ID: {
                    // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200 bytes.
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                    apFile->GetChunkData(pBuffer, 0x200);
                    SetFormEditorID(pBuffer);
                    break;
                }
                case OBND_ID: LoadObjectBound(apFile); break;
                default: break;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void ActorValueInfo::Save() {
    char* v1;
    uint32_t v2;
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESDescription*>(this)->Save();
    static_cast<TESIcon*>(this)->TESTexture::Save(static_cast<CHUNK_ID>(1313817417));
    if ((&sAbbreviation)->GetLength()) {
        v2 = (&sAbbreviation)->GetLength() + 1;
        v1 = sAbbreviation.pString;
        TESForm::AddChunkArray(ANAM_ID, v1, v2);
    }
    TESForm::CloseForm();
}

void ActorValueInfo::InitItem() {
    if (!TESForm::GetInitialized()) {
        TESForm::SetInitialized(1);
    }
}

void ActorValueInfo::Copy(TESForm* apCopy) {
    ClearData();
    ActorValueInfo* pinfo = dynamic_cast<ActorValueInfo*>(apCopy);
    if (pinfo) {
        int32_t i;
        sScriptName = pinfo->sScriptName;
        sAbbreviation.Copy(pinfo->sAbbreviation);
        uFlags = pinfo->uFlags;
        pDerivationFunction = pinfo->pDerivationFunction;
        pMappingFunction = pinfo->pMappingFunction;
        pModifiedCallback = pinfo->pModifiedCallback;
        iNumDependentActorValues = pinfo->iNumDependentActorValues;
        for (i = 0; i < iNumDependentActorValues && i < 15; ++i) {
            DependentActorValues[i] = pinfo->DependentActorValues[i];
        }
        while (i < 15) {
            DependentActorValues[i++] = 77;
        }
        CopyAllComponents(apCopy);
    }
}

bool ActorValueInfo::Compare(TESForm* apCompare) {
    ActorValueInfo* pinfo = dynamic_cast<ActorValueInfo*>(apCompare);
    if (!pinfo) {
        return 1;
    }
    if (CompareAllComponents(pinfo)) {
        return 1;
    }
    return static_cast<uint8_t>(sAbbreviation.IsNotEqual(pinfo->sAbbreviation)) != 0;
}

const char* ActorValueInfo::GetDefaultPath() const {
    return "Textures\\";
}

// The count compare is signed.
void ActorValueInfo::SetEnumerated(int32_t aiEnumCount, ...) {
    iEnumCount = BSMIN(aiEnumCount, 10);
    va_list ap;
    va_start(ap, aiEnumCount);
    for (int32_t i = 0; i < static_cast<int32_t>(iEnumCount); ++i) {
        sEnumNames[i] = va_arg(ap, const char*);
    }
    va_end(ap);
}
