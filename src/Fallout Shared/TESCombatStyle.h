#pragma once

#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESForm.h"

// A combat style's CSSD data: cover, weapon range and targeting settings.
struct SimpleCombatStyleData {
    // Byte-swaps the fifteen floats read from a big-endian file.
    void Endian();

    float fCoverSearchRadius;
    float fCoverChance;
    float fCoverWaitTimerMin;
    float fCoverWaitTimerMax;
    float fCoverWaitToFireTimerMin;
    float fCoverWaitToFireTimerMax;
    float fCoverFireTimerMin;
    float fCoverFireTimerMax;
    float fWeaponMinRangeMult;
    float fClusterDistance;
    uint8_t cWeaponRestrictions;
    uint8_t padding29[3];
    float fWeaponMaxRangeMult;
    float fTargetingFOV;
    float fCombatRadius;
    float fSemiAutoFiringDelayMinMult;
    float fSemiAutoFiringDelayMaxMult;
};

// A combat style's CSAD data: the dodge, block, attack and power attack multipliers.
struct AdvancedCombatStyleData {
    // Byte-swaps the 21 floats read from a big-endian file.
    void Endian();

    float fDodgeFatigueMult;
    float fDodgeFatigueBase;
    float fDodgeSpeedBase;
    float fDodgeSpeedMult;
    float fDodgeDuringAttackMult;
    float fDodgeNoAttackMult;
    float fDodgeBackDuringAttackMult;
    float fDodgeBackNoAttackMult;
    float fDodgeForwardWhileAttackingMult;
    float fDodgeForwardNotAttackingMult;
    float fBlockSkillMult;
    float fBlockSkillBase;
    float fBlockDuringAttackMult;
    float fBlockNoAttackMult;
    float fAttackSkillMult;
    float fAttackSkillBase;
    float fAttackDuringAttackMult;
    float fAttackNoAttackMult;
    float fAttackDuringBlockMult;
    float fPowerAttackFatigueBase;
    float fPowerAttackFatigueMult;
};

// A combat style's CSTD data: dodge, idle, block, attack and power attack chances and timers, and the flags.
struct CombatStyleData {
    // Byte-swaps the float and flag dwords read from a big-endian file; the chance bytes are left alone.
    void Endian();

    uint8_t cDodgeChance;
    uint8_t cDodgeLeftRightChance;
    uint8_t padding02[2];
    float fDodgeLeftRightMinTime;
    float fDodgeLeftRightMaxTime;
    float fDodgeForwardMinTime;
    float fDodgeForwardMaxTime;
    float fDodgeBackwardMinTime;
    float fDodgeBackwardMaxTime;
    float fIdleMinTime;
    float fIdleMaxTime;
    uint8_t cBlockChance;
    uint8_t cAttackChance;
    uint8_t padding26[2];
    float fAttackDuringRecoilStaggerBonus;
    float fAttackDuringUnconsciousBonus;
    float fAttackHandBonus;
    uint8_t cPowerAttackChance;
    uint8_t padding35[3];
    float fPowerAttackRecoilStaggerBonus;
    float fPowerAttackUnconsciousBonus;
    uint8_t cPowerAttackNormalChance;
    uint8_t cPowerAttackForwardChance;
    uint8_t cPowerAttackBackwardChance;
    uint8_t cPowerAttackLeftChance;
    uint8_t cPowerAttackRightChance;
    uint8_t padding45[3];
    float fHoldMinTime;
    float fHoldMaxTime;
    uint32_t iFlags;
    uint8_t cAcrobaticDodgeChance;
    uint8_t cRushingAttackPercentChance;
    uint8_t padding56[2];
    float fRushingAttackDistanceMult;
};

// A combat style form (CSTY).
class TESCombatStyle : public TESForm {
public:
    // The combat style's flags (IsFlagSet), one bit each from 1, the CSTY flags.
    enum COMBAT_STYLE_FLAGS : int32_t {
        USE_PA_PERCENT_CHANCE = 0x1,
        MELEE_ALERT_ALLOWED = 0x2,
        PERSONAL_SURVIVAL_FLEE = 0x4,
        USE_CLUSTERING = 0x8,
        IGNORE_THREATS = 0x10,
        EXPLOSIVE_WEAPONS_IGNORE_SELF = 0x20,
        EXPLOSIVE_WEAPONS_IGNORE_GROUP = 0x40,
        EXPLOSIVE_WEAPONS_IGNORE_SPECTATORS = 0x80,
        CANNOT_USE_STEALTHBOY = 0x100,
    };

    TESCombatStyle();

    ~TESCombatStyle() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    float GetAttackSkillModBase();
    float GetDodgeDuringAttackMult();
    char GetDodgeChance();
    // The speed multiplier for an encumbered actor.
    float GetEncumSpeedModMult();
    float GetWeaponMinRangeMult();
    float GetWeaponMaxRangeMult();
    bool IsFlagSet(TESCombatStyle::COMBAT_STYLE_FLAGS aeFlag);

    // The combat style's three data blocks.
    SimpleCombatStyleData& GetSimpleData() {
        return SimpleData;
    }
    CombatStyleData& GetData() {
        return Data;
    }
    AdvancedCombatStyleData& GetAdvancedData() {
        return AdvancedData;
    }
    bool GetExplosiveWeaponsIgnoreSelf();
    bool GetExplosiveWeaponsIgnoreGroup();
    bool GetExplosiveWeaponsIgnoreSpectators();
    bool GetIgnoreThreats();
    char GetRushingAttackPercentChance();
    float GetSemiAutoFiringDelayMinMult();
    // Signed: callers sign-extend the value.
    int8_t GetPowerAttackF();
    // Signed: callers sign-extend the value.
    int8_t GetPowerAttackB();
    // The return value is signed: callers sign-extend it.
    int8_t QPowerAttackChance();
    char GetLRChance();
    // Signed: callers sign-extend the value.
    int8_t GetPowerAttackL();
    // Signed: callers sign-extend the result.
    int8_t GetPowerAttackN();
    bool IsFleeBasedOnPersonalSurvival();
    double GetRandomWaitTime();
    double GetRandomFireTimer();
    // A random wait before firing from cover, between the style's wait-to-fire minimum and maximum.
    double GetRandomCoverWaitTimer();
    float GetAttackNoAttackMult();
    float GetAttackHandBonus();
    float GetAttackDuringRecoilStaggerBonus();
    float GetAttackDuringUnconsciousBonus();
    float GetAttackDuringAttackMult();
    float GetAttackDuringBlockMult();
    char GetAttackChance();
    float GetAttackSkillMult();
    float GetBlockNoAttackMult();
    char GetBlockChance();
    float GetBlockSkillBase();
    float GetBlockSkillMult();
    float GetBlockDuringAttackMult();
    float GetDodgeFatigueBase();
    float GetDodgeFatigueMult();
    float GetDodgeSpeedBase();
    float GetDodgeNoAttackMult();
    uint8_t GetPowerAttackChance(ANIM_GROUP_ENUM aePowerAttack);

    // The default combat style form.
    static TESCombatStyle* GetDefaultCombatStyle();

    SimpleCombatStyleData SimpleData;
    CombatStyleData Data;
    AdvancedCombatStyleData AdvancedData;
};
