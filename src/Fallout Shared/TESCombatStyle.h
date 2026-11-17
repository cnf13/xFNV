#pragma once

#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

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
	// The combat style's flags (IsFlagSet). Names from the PDB, in its order; one bit each from 1, the CSTY flags.
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

	// The attack skill modifier base (thiscall, ret 0, ST0 float, 0x4E4600).
	// Inferred type: return: float: used as one at 0x4e4157
	float GetAttackSkillModBase();
	float GetDodgeDuringAttackMult();
	char GetDodgeChance();
	// The speed multiplier for an encumbered actor.
	// Inferred type: return: float from 0x6d62e0 (passed to NavMeshTriangle::GetCoverLevel 0)
	float GetEncumSpeedModMult();
	// The weapon minimum range multiplier; a shared one-float getter (+0x38, thiscall, ret 0,
	// st0 result, 0x9A9350).
	float GetWeaponMinRangeMult();
	// The weapon maximum range multiplier (+0x44, thiscall, ret 0, st0 result, 0x508050).
	float GetWeaponMaxRangeMult();
	bool IsFlagSet(TESCombatStyle::COMBAT_STYLE_FLAGS aeFlag);

	// The combat style's three data blocks. The exe's one-field getters are out of line; their folded bodies are
	// attributed to NiAVObject::GetTranslate (0x43C490), BGSProjectile::GetMuzzleFlashModel (0x6AB360) and
	// BGSListForm::GetFormList (0x500940) (thiscall, ret 0).
	SimpleCombatStyleData& GetSimpleData() { return SimpleData; }
	CombatStyleData& GetData() { return Data; }
	AdvancedCombatStyleData& GetAdvancedData() { return AdvancedData; }
	bool GetExplosiveWeaponsIgnoreSelf();
	bool GetExplosiveWeaponsIgnoreGroup();
	bool GetExplosiveWeaponsIgnoreSpectators();
	bool GetIgnoreThreats();
	char GetRushingAttackPercentChance();
	float GetSemiAutoFiringDelayMinMult();
	// IDA name; not in the PDB. Signed: the exe's callers sign-extend it (movsx at 0x9CF4E0).
	int8_t GetPowerAttackF();
	// IDA name; not in the PDB. Signed: the exe's callers sign-extend it (movsx at 0x9CF501).
	int8_t GetPowerAttackB();
	// IDA name; the PDB's GetPowerAttackChance() returns char: the exe's callers sign-extend it (movsx at 0x64482D).
	int8_t QPowerAttackChance();
	char GetLRChance();
	// IDA name; not in the PDB. Signed: the exe's callers sign-extend it (movsx at 0x9CF522).
	int8_t GetPowerAttackL();
	// IDA name; not in the PDB. Signed: the exe's callers sign-extend it (movsx at 0x9CF4BC).
	int8_t GetPowerAttackN();
	bool IsFleeBasedOnPersonalSurvival();
	double GetRandomWaitTime();
	double GetRandomFireTimer();
	// A random wait before firing from cover, between the style's wait-to-fire minimum and maximum.
	double GetRandomCoverWaitTimer();
	// Name not in the PDB (the field at +0xF8); folded accessor, IDA: TESObjectWEAP::GetAnimMult.
	float GetAttackNoAttackMult();
	// Name not in the PDB (the field at +0x88); folded accessor, IDA: ::GetUnk88.
	float GetAttackHandBonus();
	// Name not in the PDB (the field at +0x80); folded accessor, IDA: BGSProjectile::GetAltTimer.
	float GetAttackDuringRecoilStaggerBonus();
	// Name not in the PDB (the field at +0x84); folded accessor, IDA: TESObjectTREE::GetRockSpeed.
	float GetAttackDuringUnconsciousBonus();
	// Name not in the PDB (the field at +0xF4); folded accessor, IDA: Sky::GetWeatherPercent.
	float GetAttackDuringAttackMult();
	// Name not in the PDB (the field at +0xFC); folded accessor, IDA: NiCamera::GetMaxFarNearRatio.
	float GetAttackDuringBlockMult();
	// Name not in the PDB (the field at +0x7D); folded accessor, IDA: BGSNote::GetHasBeenRead.
	char GetAttackChance();
	// Name not in the PDB (the field at +0xEC); folded accessor, IDA: Sky::GetCurrentGameHour.
	float GetAttackSkillMult();
	// Name not in the PDB (the field at +0xE8); folded accessor, IDA: Sky::GetCellFogPower.
	float GetBlockNoAttackMult();
	// Name not in the PDB (the field at +0x7C); folded accessor, IDA: InterfaceManager::IsDebugTextVisible.
	char GetBlockChance();
	// Name not in the PDB (the field at +0xE0); folded accessor, IDA: Projectile::GetAlpha.
	float GetBlockSkillBase();
	// Name not in the PDB (the field at +0xDC); folded accessor, IDA: CombatController::GetRunSpeedMult.
	float GetBlockSkillMult();
	// Name not in the PDB (the field at +0xE4); folded accessor, IDA: Projectile::GetExplosionTimer.
	float GetBlockDuringAttackMult();
	// Name not in the PDB (the field at +0xB8); folded accessor, IDA: TESAmmo::GetWeight.
	float GetDodgeFatigueBase();
	// Name not in the PDB (the field at +0xB4); folded accessor, IDA: BSFadeNode::GetLastFadePercent.
	float GetDodgeFatigueMult();
	// Name not in the PDB (the field at +0xBC); folded accessor, IDA: BSSceneGraph::GetCameraFOV.
	float GetDodgeSpeedBase();
	// Name not in the PDB (the field at +0xC8); folded accessor, IDA: TESWorldSpace::GetDefaultLandHeight.
	float GetDodgeNoAttackMult();
	uint8_t GetPowerAttackChance(ANIM_GROUP_ENUM aePowerAttack);

	// The default combat style form.
	static TESCombatStyle* GetDefaultCombatStyle();

	SimpleCombatStyleData SimpleData;
	CombatStyleData Data;
	AdvancedCombatStyleData AdvancedData;
};
inline constexpr fnvd::Vtable TESCombatStyle_vftable{};
