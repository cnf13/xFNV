#pragma once

#include "BSCore/BSStringT.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSClipRoundsForm.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"

class Actor;
class BGSImpactData;
class BGSProjectile;
class SpellItem;
class TESAmmo;
class TESEffectShader;
class TESObjectIMOD;
class TESObjectSTAT;
class TESSound;

enum ANIM_GROUP_ENUM : int32_t;
enum WEAPON_MOD_EFFECT : int32_t;

// What a weapon's hit does to the target's limbs (OBJ_WEAP::eHitBehavior).
enum WEAPONHITBEHAVIOR : uint32_t {
    WHB_NORMAL = 0,
    WHB_DISMEMBER_ONLY = 1,
    WHB_EXPLODE_ONLY = 2,
    WHB_NO_DISMEMBER_OR_EXPLODE = 3,
};

// The controller rumble's wave form while a weapon fires (OBJ_WEAP::eRumblePattern).
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
    // Byte swaps the critical damage and the chance multiplier; the effect's form ID stays as read.
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
    // Byte swaps the three floats; the effect's form ID and the bytes stay as read.
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
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjecttoGet,
        int32_t aicount
    ) override;
    const char* GetObjectTypeName() override;

    MagicSystem::CastingType GetCastingType() override {
        return static_cast<MagicSystem::CastingType>(2);
    }
    // Overrides BGSPreloadable's QueueFiles.
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
    // Fires the weapon from apSource, an actor or any other reference (it tests IsActor on it; the FireWeapon script
    // command passes any reference): picks up the spent ammo item for the player, works out the release point, angles
    // and spread, launches each projectile, then uses up the ammo.
    void Fire(TESObjectREFR* apSource);
    float GetFiringRumbleLeftMotorStrength() {
        return data.fFiringRumbleLeftMotorStrength;
    }
    TESModel* GetShellCasingModel();
    uint8_t GetIsLoopingReload() const;
    float GetSpeed() const;
    // The weapon's reach.
    float GetReach() {
        return data.fReach;
    }
    float GetIronSightFOV() const;
    SpellItem* GetCriticalEffect() const;
    SpellItem* GetVATSSpecialAttackEffect() const;
    void SetVATSSpecialAttackEffect(SpellItem* apVal);
    TESSound* GetAttackFailSound() const;
    void SetAttackFailSound(TESSound* apSnd);
    float GetSemiAutomaticFireDelayMin() const;
    float GetSemiAutomaticFireDelayMax() const;
    // Returns data.fShotsPerSec.
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
    // Sets the attack loop sound.
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
    // The first-person static.
    TESObjectSTAT* Get1stPersonObject();
    // The first-person static of the mod combination iModSlots, or null.
    TESObjectSTAT* Get1stPersonModObject(uint8_t iModSlots);
    // The model of the mod combination iSlots; abNoHandToHand skips the hand-to-hand world model.
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
    // GetProjectile(Actor*) hands its holder to GetCurrentAmmo(Actor*) unchanged. GetProjectile(ActorValueOwner*) is
    // GetProjectile_ below.
    BGSProjectile* GetProjectile(Actor* apWeaponHolder);
    // GetProjectile for the actor that owns apWeaponHolder, an actor's ActorValueOwner (null stays null).
    BGSProjectile* GetProjectile_(ActorValueOwner* apWeaponHolder);
    void SetWeaponType(WEAPON_TYPE aeType);
    // The node name used by the projectile.
    const char* GetFireNodeName();

    // The value the weapon's mods add for a mod effect (0: first value, 1: second value).
    float GetModEffectValue(WEAPON_MOD_EFFECT eEffect, uint8_t cValue);
    // The weight, less the weight mod's value when bModValue.
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
    float GetMinRange();
    // Whether the weapon's range ignores the combat style's range multipliers (the data's second flags, bit 0x20).
    bool GetIsRangeFixed();
    // The attack animation group, the data's byte zero-extended.
    ANIM_GROUP_ENUM GetAttackAnim();
    uint8_t GetHandGripAnim() const;
    TESSound* GetBlockSound() const;
    TESSound* GetIdleSound() {
        return pIdleSound;
    }
    TESSound* GetEquipSound() {
        return pEquipSound;
    }
    TESSound* GetUnequipSound() const;
    float GetAnimReloadTime() {
        return data.fAnimReloadTime;
    }
    float GetMaxRange();
    uint32_t GetStrRequired() const;
    float GetVATSAP() const;
    float GetVATSSkill() const;
    float GetFiringRumbleDuration() {
        return data.fFiringRumbleDuration;
    }
    float GetFiringRumbleRightMotorStrength() {
        return data.fFiringRumbleRightMotorStrength;
    }
    TESSound* Get3DLoopingShotSound() const;
    float GetLimbDamageMult() {
        return data.fLimbDamageMult;
    }
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
    BGSImpactData* GetImpactDataForMaterial(HavokMaterialType Material);
    BGSImpactDataSet* GetImpactDataSet();
    void SetCriticalEffect(SpellItem* apVal);
    int32_t GetPowerAttackAnimOverride();
    NiAVObject* GetFireNode(NiAVObject* apObject);
    // The point a released projectile or thrown weapon leaves the actor at: the actor's location raised by its
    // height, then the weapon bone's or the fire node's world position when there is one.
    NiPoint3 CalculateReleasePoint(TESObjectREFR* apSource);
    double GetModValue(int32_t a1, char a2);
    uint8_t GetCritDamageFlags();
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

    static inline WeaponNameArray pWeaponNames{"HandToHandMelee",     "OneHandMelee",        "TwoHandMelee",
                                               "OneHandPistol",       "OneHandPistolEnergy", "TwoHandRifle",
                                               "TwoHandAutomatic",    "TwoHandRifleEnergy",  "TwoHandHandle",
                                               "TwoHandLauncher",     "OneHandGrenade",      "OneHandMine",
                                               "OneHandLunchboxMine", "OneHandThrown"};
};

// The game settings TESObjectWEAP.cpp defines.
extern SettingT<INISettingCollection> bLaserSights;
extern SettingT<INISettingCollection> bAimSights;
extern SettingT<INISettingCollection> bForceNPCsUseAmmo;
