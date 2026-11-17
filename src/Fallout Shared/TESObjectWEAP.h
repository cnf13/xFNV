#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESCreature.h"
#include "BSHavok/bhkShape.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Reconstruction.hpp"

enum ANIM_GROUP_ENUM : int32_t;
class Actor;
class BGSImpactData;
class BGSProjectile;
class SpellItem;
class TESAmmo;
class TESEffectShader;
class TESObjectIMOD;
class TESObjectSTAT;
class TESSound;
enum WEAPON_MOD_EFFECT : int32_t;

// What a weapon's hit does to the target's limbs (OBJ_WEAP::eHitBehavior; PDB names and values).
enum WEAPONHITBEHAVIOR : uint32_t {
	WHB_NORMAL = 0,
	WHB_DISMEMBER_ONLY = 1,
	WHB_EXPLODE_ONLY = 2,
	WHB_NO_DISMEMBER_OR_EXPLODE = 3,
};

// The controller rumble's wave form while a weapon fires (OBJ_WEAP::eRumblePattern; PDB names and values).
enum WEAPON_RUMBLE_PATTERN : uint32_t {
	RUMBLE_CONSTANT = 0,
	RUMBLE_PERIODIC_SQUARE = 1,
	RUMBLE_PERIODIC_TRIANGLE = 2,
	RUMBLE_PERIODIC_SAWTOOTH = 3,
	RUMBLE_PATTERN_COUNT = 4,
};

// The weapon record's main data (OBJ_WEAP).
struct OBJ_WEAP {
	// Swaps the multi-byte fields read from a big-endian file.
	void Endian();

	int8_t eType;
	uint8_t padding01[3];
	float fSpeed;
	float fReach;
	uint8_t cFlags;
	uint8_t cHandGripAnim;
	uint8_t cAmmoPerShot;
	uint8_t cReloadAnim;
	float fMinSpread;
	float fSpread;
	float fDrift;
	float fIronFOV;
	uint32_t cConditionLevel;
	BGSProjectile* pProjectile;
	uint8_t cVATSToHitChance;
	uint8_t cAttackAnim;
	uint8_t cNumProjectiles;
	uint8_t cEmbeddedConditionValue;
	float fMinRange;
	float fMaxRange;
	WEAPONHITBEHAVIOR eHitBehavior;
	uint32_t iFlagsEx;
	float fAttackMult;
	float fShotsPerSec;
	float fActionPoints;
	float fFiringRumbleLeftMotorStrength;
	float fFiringRumbleRightMotorStrength;
	float fFiringRumbleDuration;
	float fDamageToWeaponMult;
	float fAnimShotsPerSecond;
	float fAnimReloadTime;
	float fAnimJamTime;
	float fAimArc;
	ActorValue::Index eSkill;
	WEAPON_RUMBLE_PATTERN eRumblePattern;
	float fRumbleWavelength;
	float fLimbDamageMult;
	// The resistance the weapon's damage is reduced by; -1 for none.
	ActorValue::Index eResistance;
	float fIronSightUseMult;
	float fSemiAutomaticFireDelayMin;
	float fSemiAutomaticFireDelayMax;
	float fCookTimer;
	WEAPON_MOD_EFFECT eModActionOne;
	WEAPON_MOD_EFFECT eModActionTwo;
	WEAPON_MOD_EFFECT eModActionThree;
	float fModActionOneValue;
	float fModActionTwoValue;
	float fModActionThreeValue;
	uint32_t cPowerAttackOverrideAnim;
	uint32_t iStrengthRequirement;
	// the PC engine reads iModReloadClipAnimation as the weapon mods' reload animation
	uint8_t iModFireAnimation;
	uint8_t iModReloadClipAnimation;
	uint8_t padding0AE[2];
	float fAmmoRegenRate;
	float fKillImpulse;
	float fModActionOneValueTwo;
	float fModActionTwoValueTwo;
	float fModActionThreeValueTwo;
	float fKillImpulseDistance;
	uint32_t iSkillRequirement;
};

// The weapon record's critical hit data (OBJ_WEAP_CRITICAL).
struct OBJ_WEAP_CRITICAL {
	// Byte swaps the critical damage and the chance multiplier; the effect's form ID stays as read. The exe's shared
	// 2-byte-then-4-byte swap (thiscall, 0x535AA0, the PDB's OBJ_ARMO::Endian).
	void Endian();

	uint16_t sCriticalDamage;
	uint8_t padding02[2];
	float fCriticalChanceMult;
	uint8_t bEffectOnDeath;
	uint8_t padding09[3];
	SpellItem* pEffect;
};

// The weapon record's VATS special attack data (OBJ_WEAP_VATS_SPECIAL).
struct OBJ_WEAP_VATS_SPECIAL {
	// Byte swaps the three floats; the effect's form ID and the bytes stay as read. The exe's shared swap of the
	// dwords at +4, +8 and +0xC (thiscall, 0x67FF10, the PDB's PACK_TARG_FILE::Endian).
	void Endian();

	SpellItem* pVATSSpecialEffect;
	float fVATSSpecialAP;
	float fVATSSpecialMultiplier;
	float fVATSSkillRequired;
	uint8_t bSilent;
	uint8_t bModRequired;
	// The record's flags byte; Load reads the whole 20-byte struct raw.
	uint8_t cFlags;
	uint8_t padding13[1];
};

// The weapon types the engine distinguishes (OBJ_WEAP::eType).
enum WEAPON_TYPE : int32_t {
	WEAPON_HAND_TO_HAND = 0,
	WEAPON_ONE_HAND_MELEE = 1,
	WEAPON_TWO_HAND_MELEE = 2,
	WEAPON_ONE_HAND_PISTOL = 3,
	WEAPON_ONE_HAND_PISTOL_ENERGY = 4,
	WEAPON_TWO_HAND_RIFLE = 5,
	WEAPON_TWO_HAND_AUTOMATIC = 6,
	WEAPON_TWO_HAND_RIFLE_ENERGY = 7,
	WEAPON_TWO_HAND_HANDLE = 8,
	WEAPON_TWO_HAND_LAUNCHER = 9,
	WEAPON_ONE_HAND_GRENADE = 10,
	WEAPON_ONE_HAND_MINE = 11,
	WEAPON_ONE_HAND_LUNCHBOX_MINE = 12,
	WEAPON_ONE_HAND_THROWN = 13,
	WEAPON_TYPE_COUNT = 14,
	WEAPON_NONE = -1,
};

// The combat categories of weapons (TESObjectWEAP::GetCombatWeaponType; melee is 2 there). The count also
// stands for "any category" (InventoryChanges::GetBestWeapon).
enum COMBAT_WEAPON_TYPE : int32_t {
	COMBAT_WEAPON_RANGED_EXPLOSIVE = 0,
	COMBAT_WEAPON_RANGED = 1,
	COMBAT_WEAPON_MELEE = 2,
	COMBAT_WEAPON_GRENADE = 3,
	COMBAT_WEAPON_MINE = 4,
	COMBAT_WEAPON_THROWN = 5,
	COMBAT_WEAPON_TYPE_COUNT = 6,
	COMBAT_WEAPON_TYPE_NONE = 6,
	COMBAT_WEAPON_TYPE_FULL_COUNT = 7,
};

// The effects a weapon mod applies (TESObjectWEAP::eModActionOne..Three, GetModEffectValue).
enum WEAPON_MOD_EFFECT : int32_t {
	WEAPON_MOD_NONE = 0,
	WEAPON_MOD_INCREASE_WEAPON_DAMAGE = 1,
	WEAPON_MOD_INCREASE_CLIP_SIZE = 2,
	WEAPON_MOD_DECREASE_SPREAD = 3,
	WEAPON_MOD_DECREASE_WEIGHT = 4,
	WEAPON_MOD_AMMO_REGEN_SHOT = 5,
	WEAPON_MOD_AMMO_REGEN_SECONDS = 6,
	WEAPON_MOD_EQUIP_SPEED = 7,
	WEAPON_MOD_FIRE_SPEED = 8,
	WEAPON_MOD_PROJECTILE_SPEED = 9,
	WEAPON_MOD_MAX_HEALTH = 10,
	WEAPON_MOD_SILENCE = 11,
	WEAPON_MOD_SPLIT_BEAM = 12,
	WEAPON_MOD_VATS_BONUS = 13,
	WEAPON_MOD_IRON_SITES = 14,
	WEAPON_MOD_VATS_SPECIAL_ATTACK = 15,
	WEAPON_MOD_EFFECT_COUNT = 16,
};

// The base form of weapons.
class TESObjectWEAP : public TESBoundObject,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESIcon,
					  public TESScriptableForm,
					  public TESEnchantableForm,
					  public TESValueForm,
					  public TESWeightForm,
					  public TESHealthForm,
					  public TESAttackDamageForm,
					  public BGSAmmoForm,
					  public BGSClipRoundsForm,
					  public BGSDestructibleObjectForm,
					  public BGSRepairItemList,
					  public BGSEquipType,
					  public BGSPreloadable,
					  public BGSMessageIcon,
					  public BGSBipedModelList,
					  public BGSPickupPutdownSounds {
public:
	// The weapon type names.
	using WeaponNameArray = const char* [14];

	// The mod slot bits of the weapon mods (GetModEffect, SetItemMod).
	enum WEAPON_MOD_SLOT : int32_t {
		MOD_ONE = 1,
		MOD_TWO = 2,
		MOD_THREE = 4,
	};

	// OBJ_WEAP::cFlags
	enum WEAPON_FLAG : uint8_t {
		WEAPON_FLAG_AUTOMATIC = 0x2,
		WEAPON_FLAG_HIDE_BACKPACK = 0x10,
		WEAPON_FLAG_EMBEDDED = 0x20,
		WEAPON_FLAG_NOT_PLAYABLE = 0x80,
	};

	// OBJ_WEAP::iFlagsEx
	enum WEAPON_FLAG_EX : uint32_t {
		WEAPON_FLAG_EX_PLAYER_ONLY = 0x1,
		WEAPON_FLAG_EX_NPCS_USE_AMMO = 0x2,
		WEAPON_FLAG_EX_PREVENT_USE_IN_NORMAL_COMBAT = 0x40,
		WEAPON_FLAG_EX_BURST_SHOT = 0x200,
	};

	// GetFormHealth(bool) below is not virtual (the weapon's TESHealthForm table is inherited); keep the
	// component's GetFormHealth overloads visible
	using TESHealthForm::GetFormHealth;

	TESObjectWEAP();
	~TESObjectWEAP() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	// also override TESValueForm's slots of the same signatures
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	const char* GetObjectTypeName() override;

	// TESEnchantableForm slot 4 (+0x74): the shared stub 0x9B3120, which returns 2
	MagicSystem::CastingType GetCastingType() override { return static_cast<MagicSystem::CastingType>(2); }
	// BGSPreloadable slot 4 (0x525450, entered with this at the BGSPreloadable base)
	void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;
	using TESForm::Save;

	// Whether the burst is a long one.
	bool IsLongBurst();
	bool GetUse3PISAnims();
	bool GetUse1PISAnims();
	void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, bool bLoadScope);
	// Ejects a shell casing from apSource's ShellCasingNode as debris when the weapon has a casing model and the
	// node is in view and near the camera.
	void ExpelShellCasing(TESObjectREFR* apSource);
	// Fires the weapon from apSource, an actor or any other reference (0x523150 tests IsActor on it; the FireWeapon
	// script command passes any reference): picks up the spent ammo item for the player, works out the release point,
	// angles and spread, launches each projectile, then uses up the ammo (thiscall, ret 4).
	void Fire(TESObjectREFR* apSource);
	// Data.fFiringRumbleLeftMotorStrength. The exe calls it out of line (thiscall, ret 0, ST0 float).
	float GetFiringRumbleLeftMotorStrength() { return data.fFiringRumbleLeftMotorStrength; }
	// The shell casing model (thiscall, 0x4600D0, a folded accessor of this + 0x1D0
	// registered as TESDataHandler::GetMediaLocationControllerList).
	TESModel* GetShellCasingModel();
	uint8_t GetIsLoopingReload() const;
	float GetSpeed() const;
	// The weapon's reach.
	float GetReach() { return data.fReach; }
	float GetIronSightFOV() const;
	SpellItem* GetCriticalEffect() const;
	SpellItem* GetVATSSpecialAttackEffect() const;
	void SetVATSSpecialAttackEffect(SpellItem* apVal);
	TESSound* GetAttackFailSound() const;
	void SetAttackFailSound(TESSound* apSnd);
	float GetSemiAutomaticFireDelayMin() const;
	float GetSemiAutomaticFireDelayMax() const;
	// data.fShotsPerSec, read through the shared +0x134 float getter (thiscall, ST0, 0x821640;
	// src registers it as TESCreature::GetTurnSpeed).
	float GetShotsPerSecond() const;
	float GetAnimShotsPerSecond() const;
	float GetAnimJamTime() const;
	float GetCriticalChanceMult() const;
	ActorValue::Index GetWeaponSkill() const;
	float GetIronSightUseMult() const;
	WEAPON_TYPE GetWeaponType() const;
	uint16_t GetCriticalDamage() const;
	// The combat style category of the weapon; ranged weapons whose explosion passes both danger settings are
	// ranged-explosive.
	COMBAT_WEAPON_TYPE GetCombatWeaponType();

	void SetAttackSound(TESSound* apSnd);
	void SetAttackSound2(TESSound* apSnd);
	void SetModAttackSound(TESSound* apSnd);
	void SetIdleSound(TESSound* apSnd);
	void SetEquipSound(TESSound* apSnd);
	void SetUnEquipSound(TESSound* apSnd);
	void Set1stPersonObject(TESObjectSTAT* apObject);
	void SetImpactDataSet(BGSImpactDataSet* apSet);
	// Not named in the PDB or IDA; name chosen here. The attack loop sound: the only sound field no other setter
	// writes, and 0x521650 sits between SetAttackSound2 and SetAttackFailSound (thiscall, ret 4, 0x521650).
	void SetAttackLoopSound(TESSound* apSound);

	bool GetPreventUseInNormalCombat() const;
	bool GetNPCsUseAmmo() const;
	bool GetIsPlayerOnly() const;
	bool IsAutomaticWeapon() const;
	bool GetBurstShot() const;
	bool GetIsPlayable() const;
	bool IsEmbeddedWeapon();
	// The name of the embedded weapon's node, or null when the weapon is not embedded.
	const char* GetEmbeddedWeaponNode();
	// The body-part condition actor value of an embedded weapon, -1 when the weapon is not
	// embedded.
	ActorValue::Index GetEmbeddedWeaponConditionValue();
	bool GetHideBackpack();
	void Set1stPersonModObject(uint8_t iModSlots, TESObjectSTAT* apObject);
	bool HasScope();
	// The scope's model.
	TESModel* GetScopeModel();
	bool HasModScope();
	// The first-person static. Same function as MiddleHighProcess::GetHeadAnims (0x8D8B00), registered there.
	TESObjectSTAT* Get1stPersonObject();
	// The first-person static of the mod combination iModSlots, or null.
	TESObjectSTAT* Get1stPersonModObject(uint8_t iModSlots);
	// The model of the mod combination iSlots; abNoHandToHand is the PC build's extra
	// argument (no PDB name) that skips the hand-to-hand world model (thiscall, ret 8, 0x522DF0).
	TESModelTextureSwap* GetModTESModel(uint8_t iSlots, bool abNoHandToHand);

	TESSound* GetAttackSound(bool bModSound) const;
	TESSound* GetAttackSound2(bool bModSound) const;
	TESSound* GetAttackSound2D(bool bModSound) const;

	bool IsRangedWeapon() const;
	bool IsGunWeapon() const;
	bool IsThrownWeapon() const;

	void SetItemMod(int32_t aiModSlot, TESObjectIMOD* apItemMod);
	WEAPON_MOD_EFFECT GetModEffect(uint32_t iModSlot) const;
	float GetAttackSpeed(bool abModValue) const;
	ANIM_GROUP_ENUM GetReloadAnim(bool bModValue) const;
	int32_t GetFormClipRounds(bool bModValue) const;
	TESModel* GetWorldTESModel();
	// The PDB's GetProjectile(Actor*): 0x525A90 hands its holder to GetCurrentAmmo(Actor*) unchanged. Its
	// GetProjectile(ActorValueOwner*) is GetProjectile_ (0x525AE0) below.
	BGSProjectile* GetProjectile(Actor* apWeaponHolder);
	// IDA name; not in the PDB. GetProjectile for the actor that owns apWeaponHolder, an
	// actor's ActorValueOwner (null stays null; thiscall, ret 4, 0x525AE0).
	// Inferred type: apWeaponHolder: ActorValueOwner* from 0x644ef3 (parameter 0 of CombatFormulas::CalcWeaponDamage),
	// its members used at 0x525ae0
	BGSProjectile* GetProjectile_(ActorValueOwner* apWeaponHolder);
	void SetWeaponType(WEAPON_TYPE aeType);
	// The node name used by the projectile.
	const char* GetFireNodeName();

	// Not yet reconstructed: the value the weapon's mods add for a mod effect (`auiValueIndex` 0: first value, 1: second value).
	float GetModEffectValue(WEAPON_MOD_EFFECT eEffect, uint8_t cValue);
	// The weight, less the weight mod's value when bModValue. The exe tests the whole byte; bool would assume 0/1
	// (the PDB has bool).
	float GetFormWeight(bool bModValue);
	// The ammo the weapon's holder has loaded, or the weapon's own ammo.
	TESAmmo* GetCurrentAmmo(Actor* apWeaponHolder);
	// The projectiles one shot fires: the weapon's, or the loaded ammo's times the rounds per shot, plus the
	// split-beam mod's when bUseModValue.
	uint8_t GetNumProjectiles(bool bUseModValue, bool bIgnoreAmmo, Actor* apWeaponHolder);
	uint8_t GetAmmoPerShot();
	// The minimum spread, less the spread mods' decrease (at least 0) when bModValue.
	float GetMinSpreadDegrees(bool bModValue);
	// The spread, less the spread mods' decrease (at least 0) when bModValue.
	float GetSpreadDegrees(bool bModValue);
	float GetAmmoRegenRate(bool bModValue);
	bool IsMeleeWeapon();
	// The weapon's health, plus the max-health mod's when bModValue.
	int32_t GetFormHealth(bool bModValue);
	// Out of line: the exe calls it (and GetMaxRange) rather than reading the field.
	float GetMinRange();
	// Whether the weapon's range ignores the combat style's range multipliers (the data's
	// second flags, bit 0x20; thiscall, ret 0, 0x647790).
	bool GetIsRangeFixed();
	// The attack animation group, the data's byte zero-extended: the exe's 0x51F5F0 returns it in the whole of eax
	// (movzx).
	ANIM_GROUP_ENUM GetAttackAnim();
	uint8_t GetHandGripAnim() const;
	TESSound* GetBlockSound() const;
	TESSound* GetIdleSound() { return pIdleSound; }
	TESSound* GetEquipSound() { return pEquipSound; }
	TESSound* GetUnequipSound() const;
	float GetAnimReloadTime() { return data.fAnimReloadTime; }
	float GetMaxRange();
	uint32_t GetStrRequired() const;
	// Inferred type: return: float from 0x7eb9fc (stored into fSpecialAttackAPCost)
	float GetVATSAP() const;
	// Inferred type: return: float: used as one at 0x7eb987
	float GetVATSSkill() const;
	float GetFiringRumbleDuration() { return data.fFiringRumbleDuration; }
	float GetFiringRumbleRightMotorStrength() { return data.fFiringRumbleRightMotorStrength; }
	TESSound* Get3DLoopingShotSound() const;
	float GetLimbDamageMult() { return data.fLimbDamageMult; }
	float GetAimArc();
	TESObjectIMOD* GetModItem(uint32_t iModSlot);
	void SetScopeEffect(TESEffectShader* apEffect);
	void SetNPCsUseAmmo(bool abVal);
	void SetNumProjectiles(uint8_t acVal);
	bool IsTwoHandedWeapon();
	uint8_t GetVATSToHitChance(bool bModValue);
	bool GetCantDrop();
	bool GetNoJamAfterReload();
	bool GetIsMinorCrime();
	bool HasNightScope();
	const char* GetVATSAttackName();
	bool IsActionPointOverride();
	void SetMeleeBlockSound(TESSound* apSnd);
	uint32_t GetModifiedSoundLevel(bool abSilent, bool abSilencer);
	// Inferred type: Material: HavokMaterialType: passed to BGSImpactDataSet::GetImpactDataForMaterial 0 at 0x522bbf
	BGSImpactData* GetImpactDataForMaterial(HavokMaterialType Material);
	// pImpactDataSet: the exe's shared one-dword getter at +0x24C (thiscall, 0x8D8AC0).
	BGSImpactDataSet* GetImpactDataSet();
	void SetCriticalEffect(SpellItem* apVal);
	int32_t GetPowerAttackAnimOverride();
	NiAVObject* GetFireNode(NiAVObject* apObject);
	// The point a released projectile or thrown weapon leaves the actor at: the actor's location raised by its
	// height, then the weapon bone's or the fire node's world position when there is one.
	NiPoint3 CalculateReleasePoint(TESObjectREFR* apSource);
	double GetModValue(int32_t a1, char a2);
	uint8_t GetCritDamageFlags();
	// Inferred type: return: SOUND_LEVEL: stored into TESObjectWEAP::eSoundLevel (this of
	// TESObjectWEAP::Copy) at 0x522414
	SOUND_LEVEL GetSoundLevel(char a1);
	void SetAutomaticWeapon(bool abVal);
	float GetKillImpulse();
	float GetKillImpulseDistance();

	OBJ_WEAP data;
	OBJ_WEAP_CRITICAL criticalData;
	TESModel ShellCasingModel;
	TESModel ScopeModel;
	TESModel WorldModel;
	TESEffectShader* pScopeEffect;
	TESSound* pAttackSound;
	TESSound* pAttackSound2;
	TESSound* pAttackSound2D;
	TESSound* pAttackLoop;
	TESSound* pAttackFailSound;
	TESSound* pMeleeBlockSound;
	TESSound* pIdleSound;
	TESSound* pEquipSound;
	TESSound* pUnEquipSound;
	TESSound* pModSilencedSound;
	TESSound* pModSilencedSound2;
	TESSound* pModSilencedSound2D;
	BGSImpactDataSet* pImpactDataSet;
	TESObjectSTAT* p1stPersonObject;
	TESObjectSTAT* p1stPersonMod1Object;
	TESObjectSTAT* p1stPersonMod2Object;
	TESObjectSTAT* p1stPersonMod3Object;
	TESObjectSTAT* p1stPersonMod12Object;
	TESObjectSTAT* p1stPersonMod23Object;
	TESObjectSTAT* p1stPersonMod13Object;
	TESObjectSTAT* p1stPersonMod123Object;
	TESModelTextureSwap WorldModelMod1;
	TESModelTextureSwap WorldModelMod2;
	TESModelTextureSwap WorldModelMod3;
	TESModelTextureSwap WorldModelMod12;
	TESModelTextureSwap WorldModelMod13;
	TESModelTextureSwap WorldModelMod23;
	TESModelTextureSwap WorldModelMod123;
	TESObjectIMOD* pModObjectOne;
	TESObjectIMOD* pModObjectTwo;
	TESObjectIMOD* pModObjectThree;
	BSStringT<char> EmbeddedWeaponNode;
	SOUND_LEVEL eSoundLevel;
	BSStringT<char> VATSAttackName;
	OBJ_WEAP_VATS_SPECIAL specialVATSData;
	uint8_t bIsLoopingReload;
	uint8_t padding385[3];

	static inline WeaponNameArray pWeaponNames{"HandToHandMelee", "OneHandMelee", "TwoHandMelee", "OneHandPistol",
		"OneHandPistolEnergy", "TwoHandRifle", "TwoHandAutomatic", "TwoHandRifleEnergy", "TwoHandHandle",
		"TwoHandLauncher", "OneHandGrenade", "OneHandMine", "OneHandLunchboxMine", "OneHandThrown"};
};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSBipedModelList{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSPreloadable{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSEquipType{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSRepairItemList{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSClipRoundsForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_BGSAmmoForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESAttackDamageForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESHealthForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESWeightForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESEnchantableForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectWEAP_vftable{};

// The game settings TESObjectWEAP.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bLaserSights;
extern SettingT<INISettingCollection> bAimSights;
extern SettingT<INISettingCollection> bForceNPCsUseAmmo;
