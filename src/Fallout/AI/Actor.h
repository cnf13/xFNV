#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout/AI/LipSyncBackgroundManager.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ModifierList.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <stdint.h>

class ActiveEffect;
class Actor;
class ActorMover;
class AlchemyItem;
class Animation;
class BGSPerk;
class BGSPerkEntry;
class bhkRagdollController;
class bhkRagdollPenetrationUtil;
class BSAnimGroupSequence;
class BSAnimNote;
class BSPerformanceTimer;
struct CastPowerItem;
class CombatController;
class CombatGroup;
class ContinuousBeamProjectile;
class Crime;
class DispositionParamData;
struct FurnitureMark;
class HitData;
class PathingAvoidNodeArray;
class PathingLocation;
class PathingRequest;
class PlayerCharacter;
class SpellItem;
class TESClass;
class TESCombatStyle;
class TESFaction;
class TESNPC;
class TESObjectBOOK;
class TESObjectCELL;
class TESObjectWEAP;
class TESPackage;
class TESRace;
class TESSkill;
class TESWorldSpace;
class TrespassPackage;
class WeaponObject;

enum ACTION_FLAGS : int32_t;
enum BIPED_OBJECT : int32_t;
enum COMBAT_WEAPON_TYPE : int32_t;

// How much of an actor a line-of-sight check sees.
enum ACTOR_SEGMENT_INVIEW : int32_t {
    ACTOR_SEGMENT_INVIEW_BOTTOM = 0,
    ACTOR_SEGMENT_INVIEW_MIDDLE = 1,
    ACTOR_SEGMENT_INVIEW_TOP = 2,
    ACTOR_SEGMENT_INVIEW_NONE = 3,
    ACTOR_SEGMENT_INVIEW_GROUND = 4,
};

// The ambient behaviors a package with selective fallout behavior can switch off (Actor::ShouldSkipFallOutBehavior).
enum FALLOUT_BEHAVIORS : int32_t {
    FALLOUT_BEHAVIORS_HELLOS_TO_PLAYER = 0,
    FALLOUT_BEHAVIORS_RANDOM_CONVERSATIONS = 1,
    FALLOUT_BEHAVIORS_OBSERVE_COMBAT_BEHAVIOR = 2,
    FALLOUT_BEHAVIORS_NOT_USED = 3,
    FALLOUT_BEHAVIORS_REACTION_TO_PLAYER_ACTIONS = 4,
    FALLOUT_BEHAVIORS_FRIENDLY_FIRE_COMMENTS = 5,
    FALLOUT_BEHAVIORS_AGGRO_RADIUS_BEHAVIOR = 6,
    FALLOUT_BEHAVIORS_IDLE_CHATTER = 7,
    FALLOUT_BEHAVIORS_AVOID_RADIATION = 8,
    FALLOUT_BEHAVIORS_TOTAL = 9,
};

// The disposition inputs a caller already has for the actor whose disposition is asked; Actor::GetDisposition reads
// them instead of the actor's own.
class DispositionParamData {
public:
    TESRace* pRace;
    TESActorBase* pActorBase;
    TESNPC* pNpc;
    TESForm* pOwner;
    int32_t iBaseDisposition;
    int32_t iResponsibility;
};

// How well one actor has detected another.
enum DETECTION_LEVEL : int32_t {
    DETECTION_LOST = 0,
    DETECTION_UNSEEN = 1,
    DETECTION_NOTICED = 2,
    DETECTION_SEEN = 3,
    DETECTION_COUNT = 4,
};

// Where an actor is in dying and dying's aftermath.
enum ACTOR_LIFE_STATE : uint32_t {
    ACTOR_LIFE_STATE_ALIVE = 0x0,
    ACTOR_LIFE_STATE_DYING = 0x1,
    ACTOR_LIFE_STATE_DEAD = 0x2,
    ACTOR_LIFE_STATE_UNCONSCIOUS = 0x3,
    ACTOR_LIFE_STATE_REANIMATE = 0x4,
    ACTOR_LIFE_STATE_RESTRAINED = 0x5,
    ACTOR_LIFE_STATE_ESSENTIAL_DOWN = 0x6,
};

// The values passed to SetVisibilityMask.
enum ACTOR_VISIBILITY_MASK : uint32_t {
    ACTOR_VISIBILITY_FRUSTUM = 0x1,
    ACTOR_VISIBILITY_OCCLUSION = 0x2,
    ACTOR_VISIBILITY_FADED = 0x4,
    ACTOR_VISIBILITY_ALL = 0x7,
};

enum ACTOR_CRITICAL_STAGE : uint32_t {
    ACS_NONE = 0x0,
    ACS_GOO_START = 0x1,
    ACS_GOO_END = 0x2,
    ACS_DISINTEGRATE_START = 0x3,
    ACS_DISINTEGRATE_END = 0x4,
    ACS_COUNT = 0x5,
};

// How an actor's factions make it react to another actor in a fight.
enum FIGHT_REACTION : int32_t {
    FIGHT_REACTION_NEUTRAL = 0,
    FIGHT_REACTION_ENEMY = 1,
    FIGHT_REACTION_ALLY = 2,
    FIGHT_REACTION_FRIEND = 3,
    FIGHT_REACTION_COUNT = 4,
};

// Which use GetGunSkill rates the actor's gun handling for.
enum GUN_SKILL_ENUM : int32_t {
    GUN_SKILL_HUD = 0x0,
    GUN_SKILL_GUN = 0x1,
    GUN_SKILL_ACTOR = 0x2,
    GUN_SKILL_VATS = 0x3,
};

// What the actor's process is playing an animation for.
enum ANIMATION_ACTION : int32_t {
    ANIM_ACTION_NONE = -1,
    ANIM_ACTION_EQUIP_WEAPON = 0x0,
    ANIM_ACTION_UNEQUIP_WEAPON = 0x1,
    ANIM_ACTION_ATTACK = 0x2,
    ANIM_ACTION_ATTACK_WAIT_FOR_EJECT = 0x3,
    ANIM_ACTION_ATTACK_FOLLOW_THROUGH = 0x4,
    ANIM_ACTION_ATTACK_THROW = 0x5,
    ANIM_ACTION_ATTACK_THROW_ATTACH = 0x6,
    ANIM_ACTION_BLOCK = 0x7,
    ANIM_ACTION_RECOIL = 0x8,
    ANIM_ACTION_RELOAD = 0x9,
    ANIM_ACTION_STAGGER = 0xA,
    ANIM_ACTION_DODGE = 0xB,
    ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM = 0xC,
    ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE = 0xD,
    ANIM_ACTION_FORCE_SCRIPT_ANIM = 0xE,
    ANIM_ACTION_RELOAD_LOOP_START = 0xF,
    ANIM_ACTION_RELOAD_LOOP_END = 0x10,
    ANIM_ACTION_RELOAD_LOOP = 0x11,
    ANIM_ACTION_COUNT = 0x12,
};

// Interface of the values an actor keeps cached on its process (radius, aggression, paralysis, ...); a base with only a
// vtable pointer and no virtual destructor. Each virtual function computes the value when the cache is dirty.
class CachedValuesOwner {
public:
    CachedValuesOwner();

    virtual float CalculateCachedRadius() {
        return 0.0f;
    }
    virtual float CalculateCachedWidth() {
        return 0.0f;
    }
    virtual float CalculateCachedLength() {
        return 0.0f;
    }
    virtual float CalculateCachedForwardLength() {
        return 0.0f;
    }
    virtual float CalculateCachedWeaponDPS() {
        return 0.0f;
    }
    virtual float CalculateCachedMedicineEffectivenessMult() {
        return 0.0f;
    }
    virtual float CalculateCachedSurvivalEffectivenessMult() {
        return 0.0f;
    }
    virtual float CalculateCachedParalysis() {
        return 0.0f;
    }
    virtual float CalculateCachedHealingRate() {
        return 0.0f;
    }
    virtual float CalculateCachedEndurance() {
        return 0.0f;
    }
    virtual float CalculateCachedPerceptionCondition() {
        return 0.0f;
    }
    virtual float CalculateCachedEyeLevel() {
        return 0.0f;
    }
    virtual int32_t CalculateCachedActorAggression() {
        return 0;
    }
    virtual int32_t CalculateCachedActorAssistance() {
        return 0;
    }
    virtual float CalculateCachedWalkSpeed() {
        return 0.0f;
    }
    virtual float CalculateCachedRunSpeed() {
        return 0.0f;
    }
    virtual bool CalculateCachedConditionPreventsRun() {
        return false;
    }

    void CalculateAll();
};

// One actor's disposition adjustment toward another. Actor::ModifyDispositionTowardActor allocates it and stores the
// amount and the actor; GetDispositionModifierTowardActor compares the actor and reads the amount as a float.
struct DispositionModifier {
    int32_t modifieramount;
    Actor* pactormodified;
};

// A power the actor has cast and that cannot be cast again until its time runs out. While a save loads, pPower holds
// the spell's form ID, which Actor::InitLoadGame resolves.
struct CastPowerItem {
    SpellItem* pPower;
    float fTimeRemaining;
};

// An NPC or creature in the world. Its AI state lives in the process it points to; the fields here are what must
// survive the actor changing process level.
class Actor : public MobileObject,
              public MagicCaster,
              public MagicTarget,
              public ActorValueOwner,
              public CachedValuesOwner {
public:
    // The ring of positions GetLocationOnReference hands out for swimming actors.
    using CachedPositionArray = NiPoint3[5];
    using MobileObject::EvaluatePackage;

    // The flag bits of iMajorCrimes; the bits below them count the major crimes.
    enum : uint32_t {
        ACTOR_FLAG_ALWAYS_SHOW_SUBTITLES = 0x40000000,
        ACTOR_FLAG_REF_ESSENTIAL = 0x80000000,
    };

    Actor(bool abAddToProcessList);
    ~Actor() override;

    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
    void Revert(uint32_t aiFlags) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void CheckSaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    bool IsActor() override {
        return true;
    }
    bool GetEditorLocation(
        NiPoint3& arOriginalLocation,
        NiPoint3& arOriginalAngle,
        TESForm*& arpLocationForm,
        TESForm* apLocationForm
    ) override;
    NiPoint3 GetStartingAngle() override;
    NiPoint3 GetStartingLocation() override;
    void UpdateRefLight() override;
    bool AddWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra, bool abForceEquip) override;
    void DoTrap(TrapEntry* apTrap, TargetEntry* apTarget) override;
    void AddObjecttoContainer(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aicount) override;
    NiPoint3 GetLookingAtLocation() override;
    MagicCaster* GetMagicCaster() override;
    MagicTarget* GetMagicTarget() override;
    bool IsChild(bool) override {
        return false;
    }
    TESActorBase* GetTemplateActorBase() override;
    void SetTemplateActorBase(TESActorBase* apTemplate) override {
        pTemplateActorBase = apTemplate;
    }
    NiAVObject* Load3D(bool abBackgroundLoading) override;
    Animation* GetAnimation() override;
    const NiPoint3& GetLocationOnReference() override;
    void SetActionComplete(bool abValue) override;
    void SetMovementComplete(bool abValue) override;
    void ResetInventory(bool abLeveledOnly) override;
    NiAVObject* GetFireNode() override;
    void SetFireNode(NiAVObject* apFireNode) override;
    SIT_SLEEP_STATE GetSitSleepState() override {
        return cCurrentSitSleepState;
    }
    bool IsDead(bool abNotEssential) override;
    bool IsKnockedOut() override;
    bool IsParalyzed() override;

    bool MovetoHigh() override;
    bool MovetoLow() override;
    bool MovetoMiddleLow() override;
    bool MovetoMiddleHigh() override;
    bhkCharacterController* Move(float afDeltaTime, NiPoint3& arDeltaMove, uint32_t aiMoveDir) override;
    bhkCharacterController* Jump() override;
    void Process(float afDelta) override;
    bool ChangeProcessLevel() override;
    void UpdateInDialogue(float afDelta, DialogueResponse* pResponse, bool abChangeAnimationEndVoice) override;
    void UpdateAnimation(float afDelta) override;
    bool GetEVPBuffered() override {
        return bEVPBuffered;
    }
    bool IsInCombat(bool abSearching) override;
    bool InitiateDialogue(
        MobileObject* apTarget,
        PackageLocation* pPackLoc,
        PackageLocation* pPackSecondLoc,
        bool bHeadTrack,
        bool bMove,
        bool bflag,
        TESTopic* pTopic,
        bool abTargetContinueMoving,
        bool abTargetMoveTowardStarter
    ) override;
    bool InitiateDialogue(TESPackage* pDialoguePackage) override;
    float SpeakSoundFunction(
        const char* apSoundFile,
        BSSoundHandle& aSoundHand,
        DIALOGUE_EMOTION aeEmotiontype,
        uint32_t aiEmotionValue,
        int32_t aiStringLength,
        TESIdleForm* apResponseSpeakerIdle,
        TESIdleForm* apResponseListenerIdle,
        TESObjectREFR* apTarget,
        bool abSetEmotion,
        bool ab2D,
        bool abQueue,
        bool abLip,
        bool abUseIdleManager
    ) override;
    void EndDialogue() override;
    void SetRunOnceExtraPackage(TESPackage* apPack, char acDay) override;
    bool HasEditorLocation() override;
    TESWorldSpace* GetEditorLocationWorld() override;
    TESObjectCELL* GetEditorLocationInteriorCell() override;
    NiPoint3 GetEditorLocationCoord() override;
    void EvaluateDetection(float fnumber) override;
    float GetDetectionTimer() override;
    void ReduceDetectionTimer() override;
    // PlayerCharacter's override calls it directly.
    float GetHeading(bool abIgnoreImmobile) override;
    void SetHeading(float afAngle) override;
    void SetHeading(TESObjectREFR* apTargetReference) override;
    TESObjectREFR* GetCurrentTarget() override;
    bool IsAtPoint(const NiPoint3& aPoint, float afRadius, bool abExpandRadius, bool abAlwaysTestHeight) override;
    int32_t GetSpeakingEmotion() override {
        return iEmotion;
    }
    void SetSpeakingEmotion(int32_t aiEmotion) override {
        iEmotion = aiEmotion;
    }
    int32_t GetSpeakingEmotionValue() override {
        return iEmotionValue;
    }
    void SetSpeakingEmotionValue(int32_t aiEmotionValue) override {
        iEmotionValue = aiEmotionValue;
    }
    bool IsDying() override;
    void PutCreatedPackage(TESPackage* apPack, bool abTempPack, bool abIsaCreatedPackage) override;
    void Update(float afDelta) override;
    void CleanUpPointersOnDisable() override;
    MobileObject* SetUpTalkingActivatorActor(MobileObject* apTarget, MobileObject*& aptalkingactivator) override;

    void CastAbility(SpellItem* apSpell, bool abLoadCast) override;
    void TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) override;
    void StartCast() override;
    bool CheckCast(
        MagicItem* apSpell,
        float* afEffectStrength,
        MagicItem::CannotCastReason* apReason,
        bool abUseBaseMagicka
    ) override;
    TESObjectREFR* GetCasterStatsObject() override {
        return this;
    }
    NiNode* GetMagicNode() override;
    MagicItem* GetCurrentSpell() override;
    float GetMagicEffectivenessModifier(bool, float) override;
    void SetCurrentSpell(MagicItem* apCurrentSpell) override;
    MagicTarget* GetDesiredTarget() override;
    void SetDesiredTarget(MagicTarget* apDesiredTarget) override;

    bool AddTarget(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect, bool abAreaTarget) override;
    TESObjectREFR* GetTargetStatsObject() override {
        return this;
    }
    BSSimpleList<ActiveEffect*>* GetActiveEffectList() override;
    bool MagicTargetIsActor() override {
        return true;
    }
    bool IsInvulnerable() override;
    void EffectAdded(ActiveEffect* apEffect) override;
    void EffectRemoved(ActiveEffect* apEffect) override;
    void EffectReflected(MagicCaster* apCaster, ActiveEffect* apEffect) override;
    void EffectAbsorbed(MagicCaster* apCaster, ActiveEffect* apEffect) override;
    float CheckResistance(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect) override;

    int32_t GetBaseActorValue(ActorValue::Index aiWhich) override;
    float GetBaseActorFloatValue(ActorValue::Index aiWhich) override;
    int32_t GetActorValue(ActorValue::Index aiWhich) override;
    float GetActorFloatValue(ActorValue::Index aiWhich) override;
    float GetTemporaryModifier(ActorValue::Index aiWhich) override;
    float GetDamageModifier(ActorValue::Index aiWhich) override;
    float GetPermanentModifier(ActorValue::Index aiWhich) override;
    TESForm* GetAsForm() override {
        return this;
    }
    uint16_t GetActorLevel() override;

    float CalculateCachedRadius() override;
    float CalculateCachedWidth() override;
    float CalculateCachedLength() override;
    float CalculateCachedForwardLength() override;
    float CalculateCachedWeaponDPS() override;
    float CalculateCachedMedicineEffectivenessMult() override;
    float CalculateCachedSurvivalEffectivenessMult() override;
    float CalculateCachedParalysis() override;
    float CalculateCachedHealingRate() override;
    float CalculateCachedEndurance() override;
    float CalculateCachedPerceptionCondition() override;
    float CalculateCachedEyeLevel() override;
    int32_t CalculateCachedActorAggression() override;
    int32_t CalculateCachedActorAssistance() override;
    float CalculateCachedWalkSpeed() override;
    float CalculateCachedRunSpeed() override;
    bool CalculateCachedConditionPreventsRun() override;

    virtual bool IsActoraGuard() {
        return false;
    }
    virtual void SetActorGuard(bool) {}
    virtual void SetEquippedWeight(float) {}
    virtual float GetEquippedWeight() {
        return 0.0f;
    }
    virtual int32_t GetFame() {
        return 0;
    }
    virtual int32_t GetInfamy() {
        return 0;
    }
    virtual void SetIgnoreCrime(bool abFlag) {
        bIgnoreCrime = abFlag;
    }
    virtual bool GetIgnoreCrime() {
        return bIgnoreCrime;
    }
    virtual void Resurrect(bool abResetInventory, bool abAttach3D, bool abGetupResurect);
    virtual void SetAdvanceNumber(int32_t) {}
    virtual int32_t GetAdvanceNumber() {
        return 0;
    }
    virtual void SetSkillAdvanceDelayed(ActorValue::Index) {}
    virtual ActorValue::Index GetSkillAdvanceDelayed() {
        return ActorValue::ACTOR_VALUE_COUNT;
    }
    virtual bool DoDamage(float afHealthDamage, float afFatigueDamage, Actor* apSource);
    virtual void UseActionPoints(ActionPoints::Action) {}
    virtual void UseActionPoints(float) {}
    virtual int32_t GetDisposition(Actor* apTargetActor, DispositionParamData* apData);
    virtual void UpdateMovement(float afDelta, bool abSleeping);
    virtual void UpdateNonRenderSafeMovement(float afDelta, bool abSleeping);
    virtual void UpdateAnimationMovementNoWorldUpdate();
    virtual float GetTurningSpeed() = 0;
    virtual bool IsOverEncumbered() {
        return false;
    }
    virtual bool IsHumanoidCreature() {
        return false;
    }
    virtual bool IsPc() {
        return false;
    }
    virtual bool GetCannibal() {
        return false;
    }
    virtual void SetCannibal(bool) {}
    virtual bool GetSandman() {
        return false;
    }
    virtual void SetSandman(bool) {}
    virtual void InitiateSandmanPackage(Actor*, TESObjectREFR*, FurnitureMark*, uint8_t) {}
    virtual void InitiateCannibalPackage(Actor*) {}
    virtual TESRace* GetRace() {
        return nullptr;
    }
    virtual float GetReach();
    virtual void SetRefraction(bool abEnable, float afRefractionPower);
    virtual void SetRagDoll(bool) {}
    virtual bool HasRagDoll() {
        return true;
    }
    virtual int32_t GetActorType() = 0;
    virtual void SetActorValue(ActorValue::Index aiWhich, int32_t aiValue);
    virtual void SetActorValue(ActorValue::Index aiWhich, float afValue);
    virtual void TempModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker);
    virtual void TempModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker);
    virtual void PermanentModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker);
    virtual void PermanentModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker);
    virtual void DamageModActorValue(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker);
    virtual void DamageModActorValue(ActorValue::Index aiWhich, float afValue, Actor* apAttacker);
    virtual void ModActorBaseValue(ActorValue::Index aiWhich, int32_t aiValue);
    virtual void ModActorBaseValue(ActorValue::Index aiWhich, float afValue);
    virtual ItemChange* GetBestWeapon(COMBAT_WEAPON_TYPE aeType);
    virtual ItemChange* GetBestAmmo();
    virtual void ResetArmorRating() {}
    virtual bool DamageEquipment(ItemChange* apEquipment, float afDamage, bool abIgnoreArmorSkill);
    virtual TESObjectREFR* DropObject(
        TESObject* apobject,
        ExtraDataList* apExtra,
        int32_t ainumber,
        const NiPoint3* apPoint,
        const NiPoint3* apRotate
    );
    virtual void PickUpObject(TESObjectREFR* apobjREFR, int32_t aicount, bool abflag);
    virtual void CastScroll(TESObjectBOOK* apScroll, MagicTarget* apTarget);
    virtual bool CheckCastWhenStrikesEnchantment(
        ItemChange* apWeapon,
        Actor* apTarget,
        Projectile* apProjectile,
        bool& abInsufficientCharge
    );
    virtual void UpdateWornEnchantments(float) {}
    virtual bool AddSpell(SpellItem* apSpellIn);
    virtual bool RemoveSpell(SpellItem* apSpellIn);
    virtual bool ReloadWeapon(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue);
    virtual bool ReloadWeaponNV(TESObjectWEAP* apWeapon, int32_t aiReloadAnim, bool bUseModValue, bool bFromInventory);
    virtual uint32_t UseAmmo(int32_t aiShotCount);
    virtual void CheckTempModifiers() {}
    virtual CombatGroup* GetCombatGroup();
    virtual void SetCombatGroup(CombatGroup* apGroup);
    virtual bool InitiateTresPassPackage(TrespassPackage*) {
        return false;
    }
    virtual void UpdateNonRenderSafeDialogueUpdate(float afDelta);
    virtual BSSoundHandle
    VoiceSoundFunctionEx(TESTopic* apTopic, Actor* apTarget, bool abIs2D, bool abNeverSubtitle, int32_t, bool abSayTo);
    virtual void InitiateSpectator() {}
    virtual void InitiateFlee(
        TESObjectREFR* apFleeRef,
        bool abrunonce,
        bool abknows,
        bool abCombatMode,
        TESObjectCELL* apcell,
        TESObjectREFR* apRef,
        float affleeFromDist,
        float affleeToDist
    );
    virtual void InitiateSearchForAttacker(TESObjectREFR* apAttacker, NiPoint3 apLocation, bool abAttacked);
    virtual void InitiateGetUpPackage();
    virtual void SetAlpha(float afNewAlpha);
    virtual float GetAlpha();
    virtual void StartCombat(
        Actor* apTarget,
        CombatGroup* apGroup,
        bool abForceCombat,
        bool abIsAggressor,
        bool abfleeing,
        int32_t iPriority,
        bool bCastSpell,
        TESPackage* apPackage
    );
    virtual CombatController* GetCombatController();
    virtual Actor* GetCombatTarget();
    virtual void UpdateCombat();
    virtual void StopCombat(Actor* apAttacker);
    virtual void PerformOneRoundofCombat();
    virtual float GetArmorRating() {
        return 0.0f;
    }
    virtual float GetArmorThreshold() {
        return 0.0f;
    }
    virtual int32_t GetAttackDamage() {
        return 0;
    }
    virtual bool IsTresPassing() {
        return false;
    }
    virtual void SetTresPassing() {}
    virtual bool GetUsesAttackPercents() {
        return false;
    }
    // aeType is a POWER_ATTACK_TYPE
    virtual uint8_t GetPowerAttackPercent(uint32_t) {
        return 0;
    }
    virtual float CalculateWalkSpeed();
    virtual float CalculateRunSpeed();
    virtual void ModifyDispositionTowardActor(Actor* apActor, float afamount);
    virtual float GetDispositionModifierTowardActor(Actor* apActor);
    virtual void RemoveActorInDispositionModifier(Actor* apActor);
    virtual void SetStartingPosition();
    virtual bool HasBeenAttacked();
    virtual void SetBeenAttacked(bool abFlag);
    virtual void
    HitMe(Actor* apAttacker, float afDamage, float afBlocked, const NiPoint3& arStartPos, const NiPoint3& arStopPos);
    // The uint32_t parameters, aiAction and aeSkillAction, are SKILL_ACTIONs.
    virtual void UseSkill(ActorValue::Index, float, TESSkill*) {}
    virtual void UseSkill(ActorValue::Index, uint32_t, float) {}
    virtual void UseSkill(ActorValue::Index, uint32_t) {}
    virtual void RewardExperience(int32_t) {}
    virtual float GetBaseValueOverride(ActorValue::Index aiWhich, bool& abExists);
    virtual void SetBaseValueOverride(ActorValue::Index aiWhich, float afValue);
    virtual void AddGunWobble();
    virtual void AddPerk(BGSPerk*, uint8_t, bool) {}
    virtual void RemovePerk(BGSPerk*, bool) {}
    virtual uint8_t GetPerkRank(BGSPerk*, bool) {
        return 0;
    }
    virtual void AddPerkEntry(BGSPerkEntry*, bool) {}
    virtual void RemovePerkEntry(BGSPerkEntry*, bool) {}
    virtual BSSimpleList<BGSPerkEntry*>* GetPerkEntryList(uint8_t, bool) {
        return nullptr;
    }
    virtual void StartAnimOn1stPerson(uint16_t, ACTION_FLAGS) {}
    virtual bool IsImmobile();
    virtual void HandleHealthDamage(Actor* apAttacker, float afDamage);
    virtual void AttackedBy(Actor* apAttacker, ActiveEffect* apHostileEffect);
    virtual void HandleBlockedAttack(
        float afFullDamage,
        float afPercentageBlocked,
        Actor* apBlockingActor,
        Projectile* apProjectile
    );
    virtual void PrecacheData();
    virtual void ProcessTracking(float afDelta);
    virtual void ProcessEmotions(float afDelta);
    virtual void CreateActorMover();
    virtual void DestroyActorMover();
    virtual NiPoint3 CalculateAnticipatedLocation(float afDeltaT);

    // The cached values, read from the process when it caches them, else computed: width, length, forward length,
    // perception condition and eye level.
    float GetWidth();
    float GetLength();
    float GetForwardLength();
    float GetPerceptionCondition();
    float GetEyeLevel();
    // Writes the eye position to aOrigin and the look direction to aDirection, the camera
    // offset included when abIncludeCameraOffset.
    void GetEyeVector(NiPoint3& aOrigin, NiPoint3& aDirection, bool abIncludeCameraOffset);
    void QueueReplacementLocomotions(bool abForceReplaceAnimation);
    bool GetAnimationMoveDelta(NiPoint3& aMoveDelta);
    // The pick-up (abPickUp) or put-down sound's editor name for apobjREFR, null when it has
    // none.
    const char* GetPickUpSoundName(TESBoundObject* apobjREFR, bool abPickUp, bool abUse);
    bool CanAttackActor(Actor* apActor);
    float CalculateCombatStrength(float afDamagePerSecond);
    float GetCurrentSpeed();
    bool QueueAttack(ANIM_GROUP_ENUM aeAttack);
    void ClearMoveModeAll();
    // Called from Actor's and PlayerCharacter's actor value changes, none of which needs a result.
    void CheckSkillLevelChange(ActorValue::Index aiWhich, bool abBaseValueModified);
    // Applies the ragdoll constraint template at apNodeRDT to the actor's 3D; false without 3D or extra data.
    bool ApplyConstrTemplate(NiNode* apNodeRDT);
    bool IsProcessingTresPassAlarm();
    // Queues the unequip of aiNumber of apObject.
    void QueueUnEquipObject(
        TESBoundObject* apObject,
        int32_t aiNumber,
        ExtraDataList* apExtra,
        bool abForceUnequip,
        bool abEquipCalling,
        bool abPlayPickUpSounds
    );
    // Applies the hit apData to the actor: damage, limb damage, criticals, knockback, blocking, blood, rumble and
    // combat reactions. cMeleeEffect 1 forces a hit reaction, 2 applies the cross-skill limb damage multiplier.
    void HitMe(HitData* apData, char cMeleeEffect);
    // Kills the actor, crediting apAttacker with afDamage.
    void Kill(Actor* apAttacker, float afDamage);
    ACTOR_LIFE_STATE GetLifeState();
    bool IsUnConscious();
    bool IsRestrained();
    // True while the actor waits on its package.
    bool IsWaiting();
    // The bone of the actor's skeleton nearest a hit from aStartingLocation at aImpactLocation.
    NiNode* GetClosestBone(NiPoint3 aStartingLocation, NiPoint3 aImpactLocation);
    // Whether the knocked-down movement is taken: sets the controller to follow the ragdoll and rotates the
    // movement delta arMovementDelta into the actor's Z rotation.
    bool GetKnockedDownMovementDelta(NiPoint3& aMovementDelta);
    // The body part apNode belongs to.
    BGSBodyPart::LIMB_ENUM GetDamageLocation(NiNode* apNode);
    // Moves the actor by the movement of apAnim over afDelta.
    void UpdateAnimationMovement(Animation* apAnim, float afDelta);
    // True when the actor's cell reset time has passed by aiCurrentTime, in game hours.
    bool ShouldReset(uint32_t aiCurrentTime);
    bool IsReanimated();
    bool IsInCombat();
    bool IsPlayerTeammate();
    bool IsSwimming();
    // Whether the actor can speak.
    bool CanSpeak();
    // Stops the actor's movement.
    void EndMovement();
    bool IsSleeping();
    bool IsAlarmed();
    bool IsMoving();
    bool IsTurning();
    bool IsEating();
    // The reaction of this actor's factions towards apActor; airank receives the rank and
    // the two faction pointers the factions that decided it.
    int32_t GetFactionReaction(Actor* apActor, int32_t& airank, TESFaction*& apthisfaction, TESFaction*& aptargfaction);
    // An empty body.
    void HandleInvalidPosition();
    // Whether the actor can see apTarget in every direction.
    uint8_t Has360LOS(Actor* apTarget);
    // Sets a pathfinding goal from a prepared request.
    bool SetPathfindingGoal(PathingRequest* apRequest);
    // Sets a pathfinding goal at the reference apPathingRequest, within afTargetRadius.
    bool SetPathfindingGoal(TESObjectREFR* apPathingRequest, float afTargetRadius, PathingAvoidNodeArray* apAvoidNodes);
    // Current health over maximum health.
    float GetHealthPercentage();
    // The name of the package the actor is running.
    const char* GetCurrentPackageName();
    // Whether the actor is speaking.
    bool GetIsTalking();
    // The caps the actor has to barter with.
    int32_t GetBarterGold();
    // The limb that took the killing blow, LIMB_NONE when none: the dismemberment extra's eLastHitLimb.
    BGSBodyPart::LIMB_ENUM GetKillingBlowLimb();
    // The minor and major crime counts of the actor.
    int32_t GetMinorCrimeCount();
    int32_t GetMajorCrimeCount();
    // The number of major crimes: iMajorCrimes without its two flag bits.
    int32_t GetMajorCrimesNumber();
    // Sets the alert state.
    void SetAlert(bool abflag);
    // Whether apGeometry is skinned to the limb node apLimb.
    bool IsSkinnedToLimb(NiAVObject* apSkin, NiNode* apLimb);
    // Sets the alpha of the nodes below apNode that are not lines. afAlpha is passed to BSShaderProperty::SetAlpha.
    void SetNonLinesAlpha(NiNode* apNode, float afAlpha);
    // Sets the movement flags of the actor's mover.
    void SetMoveMode(uint32_t auiMoveMode);
    // Clears the movement flags auiMoveMode of the actor's mover.
    void ClearMoveMode(uint32_t auiMoveMode);
    bool IsWeaponDrawn();
    bool IsRefractive();
    // Moves the actor onto the navmesh and clear of the player; false when it may not move.
    bool MoveToSafePoint(bool abForceMove);
    bool GetGhost();
    bool GetIronSights();
    // Raises or lowers the weapon to the sights.
    void SetIronSights(bool abIronSights, bool abPostAnim, bool abCheckAnimBlendOut);
    // The spread or wobble scale of the actor's gun handling for aeType.
    float GetGunSkill(GUN_SKILL_ENUM aeType);
    // The medicine skill's effectiveness multiplier of a medicine's effects, cached by the process when it can.
    float GetMedicineEffectivenessMultiplier();
    // The survival skill's effectiveness multiplier of a food's effects, cached by the process when it can.
    float GetSurvivalEffectivenessMultiplier();
    bool IsAttacking();
    // Whether the actor belongs to apFaction.
    bool IsInFaction(TESFaction* apFaction);
    // Expels the actor from apFaction.
    void RemoveFaction(TESFaction* apFaction);
    bool GetTurretBehavior();
    // Sets bTurretBehavior.
    void SetTurretBehavior(bool bVal);
    // Builds the actor's ragdoll controller for apObject (an immobile creature with a yaw bone and no ragdoll data
    // becomes a turret instead), with its IK systems when abSystemInspect.
    void CreateRagdollInstance(
        NiAVObject* apObject,
        bool abDeleteCurrInst,
        bool abSystemInspect,
        bool abForceUniqueSkeleton
    );
    // The process's GetShouldCheckMagicNode, false without a process.
    bool GetShouldCheckMagicNode();
    // Updates the actor's magic over the time since its last update (an hour of game time while sleeping, a
    // millisecond in the companion menu): radiation, the current spell, the magic node, the cast light, the active
    // effects, and the action points, health and fatigue regeneration of a living actor.
    void UpdateMagic(bool abSleeping, bool abCompanionMenu);
    // Looks up the actor's magic node.
    void CheckMagicNode();
    void RestoreHealth(float afSecondsElapsed);
    // Regenerates fatigue over afSecondsElapsed.
    void RestoreFatigue(float afSecondsElapsed);
    // Regenerates action points over afSecondsElapsed.
    void RestoreActionPoints(float afSecondsElapsed);
    // The animation weapon kind of the actor's current weapon: none for a dead actor, one without a process or one
    // below middle-high processing, ANIM_GROUP_WEAPON_COUNT for the player, hand-to-hand without a weapon.
    ANIM_GROUP_WEAPON_ENUM GetCurrentWeaponAnimType();

    // True when the actor is flagged essential in its runtime flags.
    bool GetRefEssential();

    // The animation action the process is playing, -1 without a process.
    ANIMATION_ACTION GetAnimAction();
    // The biped VATS::Update hands GetWeaponBone; it always returns null.
    BipedAnim* GetVATSBiped();
    BSFaceGenAnimationData* GetFaceAnimationData();
    // With 3D in a loaded cell: below high moves to high, high calls one of its own virtual functions with 1 and
    // returns false; true otherwise.
    bool ConditionalFree(bool abArg);
    // Plays the pick-up (abPickUp) or use (abUse) sound of apobjREFR.
    void PlayPickUpSound(TESBoundObject* apobjREFR, bool abPickUp, bool abUse);
    // Raises the alarm for taking ainumb of apObj from apRef, owned by apowner.
    void StealAlarm(TESObjectREFR* apRef, TESBoundObject* apObj, int32_t ainumb, int32_t aiTotal, TESForm* apowner);
    // Runs the block post-animation action on the player.
    void PostAnimBlock();
    // Starts or stops blocking; true with an animation, else the combat controller's blocking is set to the actor's
    // state.
    bool SetBlock(bool abBlock);
    HitData* GetLastAttackHitData();
    HitData* GetLastHitData();
    uint32_t GetMoveMode();
    float GetRadius();
    // Sets bSwimming from the swim bit of the move mode.
    void SetSwimming();
    // Plays the actor's pain reaction: the pain voice with abVoice, the player's pain blur
    // with abBlur.
    void TriggerPain(bool abVoice, bool abBlur);

    TESPackage* GetPackageSetAsPcurrent();
    TESObjectREFR* GetCurrentPackageTarget();
    // Sets the process's current target.
    void SetCurrentTarget(TESObjectREFR* apRef);
    // eCriticalStage.
    ACTOR_CRITICAL_STAGE GetCriticalStage() {
        return eCriticalStage;
    }
    PackageLocation* GetCurrentPackageLocation();

    void SetCachedSitSleepState(SIT_SLEEP_STATE aeState);
    void SetVisibilityMask(bool abVal, int32_t aiVisMask);
    void SetWalkRunFlagDirty();
    void ResetDamagePerSecond();
    void ResetCachedWeaponCondition();
    void ResetRadiationResistanceMult();
    void ResetCachedGunSkill();
    void StopMoving();
    void ClearLookAtTarget();

    // The combat style of the reference's extra data, else the current package's, else the base form's.
    TESCombatStyle* GetCombatStyle();
    // Drops the process's current weapon and ammo.
    void ClearProcessItems();
    // Whether a leg is crippled (and crippled limbs are not ignored).
    bool IsCrippled();
    void AddFollower(Actor* apFollower);
    // The agility- and perk-scaled reload (equip) speed, set on the actor's animations.
    void SetReloadSpeed();
    void SetEquipSpeed();
    // Whether the actor is close enough to the player for high detail (a BOOL).
    int32_t CheckHighDetail();
    // Applies the refraction, or the process's alpha and fade, to the actor's 3D.
    void UpdateAlpha();
    // Debug-only; does nothing on PC. TES::AttachModel calls it on a freshly attached actor.
    void UpdateCurrentDebugGeometry();
    // Drops the objects attached to the actor's skeleton when it dies.
    void DropAddOnObjects();
    // The actor's group id for the animation group aeGroup.
    uint16_t GetAnimGroup(ANIM_GROUP_ENUM aeGroup, ItemChange* apWeaponItem, bool abCheckHandToHand, Animation* apAnim);
    // Sets the action the process plays aeAction for.
    void SetAnimAction(ANIMATION_ACTION aeAction, BSAnimGroupSequence* apAnimSeq);
    // Whether the actor's weapon uses up ammo.
    bool ShouldWeaponUseAmmo(TESObjectWEAP* apWeapon);
    // Whether the save should keep the actor's havok state for these change flags.
    bool CheckShouldSaveHavokData(uint32_t aiFlags);
    // The change flags the running package apPackage adds for the save (TESSaveLoadGame::CheckFlags ORs them in);
    // always returns 0.
    uint32_t GetPackageSaveFlags(TESPackage* apPackage);
    // Equips apObject through the process's equip queue where it can, otherwise now.
    void QueueEquipObject(
        TESBoundObject* apObject,
        int32_t aiNumber,
        ExtraDataList* apExtra,
        bool abNow,
        bool abForceEquip,
        bool abPlayPickUpSounds
    );
    // Advances the actor mover by afDelta and keeps a robot's or giant's movement sound in step with its movement.
    void UpdateActorMover(float afDelta);
    // Equips aiNumber of apObject (the instance apExtra).
    void EquipObject(
        TESBoundObject* apObject,
        int32_t aiNumber,
        ExtraDataList* apExtra,
        bool abNow,
        bool abForceEquip,
        bool abPlayPickUpSounds
    );
    // Unequips aiNumber of apObject (the instance apExtra).
    bool UnEquipObject(
        TESBoundObject* apObject,
        int32_t aiNumber,
        ExtraDataList* apExtra,
        bool abForceUnequip,
        bool abEquipCalling,
        bool abPlayPickUpSounds
    );
    // Takes aiNumber of the worn apObject (the instance apExtra) off through the inventory's RemoveWorn and unloads its
    // enchantment; true without apObject. Not an override of TESObjectREFR's three-argument virtual: UnEquipObject
    // calls it directly.
    bool RemoveWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra, bool ableft, bool abforce);
    // Registers HandleGrabIKNote for grab-IK notes and hooks the receiver to the 3D's controller manager.
    void RegisterGrabIKNote();
    // The fight reaction of the actor's factions (with their changes) to apActor's; abFromCrime is set when the
    // player is an enemy of one of the actor's factions.
    FIGHT_REACTION GetFactionFightReaction(Actor* apActor, bool& abFromCrime);
    // Merges apList1 with the faction changes apList2 into apFactionArray (at most aiArraySize factions) and returns
    // the count.
    uint32_t IntegrateFactionLists(
        TESFaction** apFactionArray,
        uint32_t aiArraySize,
        BSSimpleList<FACTION_RANK*>* apList1,
        BSSimpleList<FACTION_RANK*>* apList2
    );
    // Whether the actor is fleeing: running a flee package, or (unless abFleePack) a flee-like package or a combat
    // controller that flees. The player's flee package is cleared instead.
    bool IsFleeing(bool abFleePack);

    // The base form's movement and other traits.
    bool DoesWalk();
    bool DoesFly();
    bool SwimsOnly();
    bool BreathesAboveWater();
    TESActorBase::SEX GetSex();
    bool IsAutoCalc();
    // Whether the actor cannot be killed: its (original levelled) base is essential or the reference is flagged so; a
    // teammate of the player outside hardcore mode always is.
    bool GetEssential();
    // Whether the dead actor comes back once its cell resets.
    bool GetRespawn();
    // Records the actor's current position, heading and cell or world space as its editor location.
    void SetEditorLocation();
    // Sets the editor location to aCoord and afZRot, in apInterior when it is an interior, else in apWorld.
    void SetEditorLocation(TESWorldSpace* apWorld, TESObjectCELL* apInterior, const NiPoint3& aCoord, float afZRot);
    // The energy level of the actor's base form's AI data.
    int32_t GetActorBaseEnergyLevel();
    // Whether the actor's hits always cause a hit reaction (bForceHitReaction).
    bool GetForceHitReaction();
    // The difficulty modifier of a change afValue to the actor value aiWhich by apAttacker; on PC it returns afValue
    // unchanged.
    float DifficultyLevelCheckModHealth(ActorValue::Index aiWhich, float afValue, Actor* apAttacker);
    // DifficultyLevelCheckModHealth for an integer change, floored back to an integer.
    int32_t DifficultyLevelCheckModHealthI(ActorValue::Index aiWhich, int32_t aiValue, Actor* apAttacker);
    // The actor facing this one inside its melee reach (the weapon's, or the actor's own unarmed).
    Actor* FindMeleeTarget();
    // Makes the character controller unpushable while the actor can't move (a speed multiplier of 0).
    void CheckInCombat();
    // Whether the queued or playing special idle plays in the movement, special idle or whole body section.
    bool IsPlayingLowerBodySpecialIdle();
    // Adds to apPacks the packages that should have run since afStartdate.
    void GetMissedPacks(BSSimpleList<TESPackage*>* apPacks, float afStartdate);

    bool DoesSwim();
    // Whether the process runs a run-once package.
    bool IsRunningRunOnce();

    void ReloadTargets(bool abReload);
    // Updates the effects of the equipped weapon's condition.
    void UpdateWeaponConditionEffects(bool abForce);
    // Points the actor's process at the per-pixel lighting property in its 3D.
    void InitLightingPropertyPtr(NiAVObject* apObject);
    // The level of the actor's base.
    uint16_t GetLevel();
    // Puts the backup package on the actor unless it is restrained, unconscious or getting up.
    void InitiateBackUpPackage();
    // The current fatigue actor value.
    float GetFatigue();
    // The current fatigue over its base value, 1 when the base is 0.
    float GetFatiguePercentage();
    // Whether the actor's lighting is redone after its next scene graph update; reads bUpdateLighting. Returns the byte
    // as stored, unnormalised: the field is not always 0 or 1, and callers test it nonzero.
    uint8_t QUpdateLighting();
    void SetUpdateLighting(bool abUpdate);
    // Whether the actor may use the weapon.
    bool CanUseWeapon(TESObjectWEAP* apWeapon, bool abInInventory);
    float GetMaxCarryWeight();
    // Whether the actor has no rumors.
    bool HasNoRumors();
    // Removes the enemy flag from all factions.
    void RemoveEnemyFlagFromAllFactions();
    // The actor's endurance (CalculateCachedEndurance's value), cached by the process when it has cached values.
    float GetEndurance();
    // Whether the actor can run: not while a run-preventing condition holds, while blocking, or (the player) while
    // aiming down the sights.
    bool CanRun();
    // Whether a condition prevents the actor from running; always returns false.
    bool GetConditionPreventsRun();
    // Whether the actor may swim: in water already, by its combat state, or by its current package.
    bool IsAllowedToSwim();
    // Restores an actor value.
    void RestoreActorValue(ActorValue::Index aiWhich, float afAmount);
    double GetHealRate();
    // Whether the actor skips the requested fallout behavior.
    bool ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS aeFalloutType);
    // The actor's base speed multiplier, the integer base value.
    float GetActorBaseSpeedMult();
    // The actor's assistance setting.
    int32_t GetAssistance();
    bool IsAngryWithPlayer();
    // Whether apActor should be helped.
    bool GetShouldHelp(Actor* apActor);
    // Whether apActor is in the actor's combat group member array.
    bool IsCombatGroupMember(Actor* apActor);
    // The actor's damage per second.
    float GetDamagePerSecond();
    // Handles a hit from apAttacker.
    void CombatHit(Actor* apTarget, bool abPowerAttack, Projectile* apProjectile, char cMeleeEffect);
    // Swings the actor's melee attack: queued to the 3D task thread when it runs, else the VATS paralyzing palm,
    // the debug hit vector, the attack sound level and, for an in-combat attack, the hit on the melee target or the
    // touch spell.
    void MeleeAttack(bool abPowerAttack, bool abInCombatAttack);
    // The position of the equipped weapon, in world space with abWorldSpace.
    NiPoint3 GetWeaponPosition(bool abWorldSpace);
    // Hits the object in front of the actor with a melee attack: knocks a clutter body, places the impact decal,
    // effect and sounds, damages a destructible, or plays a hit sound on an actor; whether it hit something.
    bool ObjectHit(bool abPowerAttack);
    // Plays the weapon's impact sound at aLocation for a hit on apTarget's body apBody of
    // material eTargetMat.
    void
    FakeWeaponHitSound(NiPoint3 aLocation, TESObjectREFR* apTarget, bhkRigidBody* apBody, HavokMaterialType eTargetMat);
    // Finds the cast-power item for apSpell.
    CastPowerItem* GetCastPowerItem(SpellItem* apPower);
    // Selects animations for the actor.
    void PickAnimations(float afMoveScale, float afTurnScale);
    // Sets the pathfinding goal.
    bool SetPathfindingGoal(
        const NiPoint3& apPosition,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float afRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    // The actor's base form.
    TESActorBase* GetActorBase();
    // Raises the alarm on apRef for a pickpocket attempt.
    void PickpocketAlarm(TESObjectREFR* apRef, TESBoundObject* apObj, int32_t ainumb);
    // The class of a character's NPC base form; null for creatures.
    TESClass* GetClass();
    // Whether the actor's move mode is sneaking, but not swimming.
    bool IsSneaking();
    // Whether the actor is in combat with apActor.
    bool IsInCombatWithActor(Actor* apActor);
    // Whether the actor should attack apActor; the reaction is returned in arReaction.
    bool GetShouldAttackActor(Actor* apActor, bool bAttacked, FIGHT_REACTION& aeReaction, bool abIgnoreCombat);
    // The detection level of the actor against apActor; with abRunFormula the detection formula runs and the
    // detection lists are updated, otherwise the level last stored is returned.
    int32_t GetDetectionLevelAgainstActor(
        bool abRunFormula,
        Actor* apActor,
        bool& abLineofSight,
        bool binCombat,
        bool abattacked,
        bool abCombat,
        bool* ap360LineOfSight
    );
    // Whether the actor sees pTarget; the segment seen is returned in aeInView.
    bool LineOfSight(
        bool abRunFormula,
        TESObjectREFR* pTarget,
        bool abUseEyeLevel,
        ACTOR_SEGMENT_INVIEW* aeInView,
        bool abfull360
    );
    // The current process's package, as the combat controller (a CombatController is a TESPackage); unchecked,
    // unlike GetCombatController.
    CombatController* QCombatController();
    // Whether arPoint is inside the actor's view cone of afConeFOV radians.
    bool IsPointInViewCone(const NiPoint3& arPoint, float afConeFOV);
    // Whether the actor has a line of sight to arPos.
    bool CalculateLOS(NiPoint3* arPos, bool abEyeHeight, bool abSkipConeTest);
    // The worn item in the slot aePart, or null; the caller deletes it.
    ItemChange* GetArmorBeingWorn(BIPED_OBJECT aePart);
    // The noise weight of the actor's boots, given its body armour pItem.
    int32_t GetBootWeight(ItemChange* pItem);
    // The noise of the actor's current action (iActionValue).
    int32_t GetActionSoundValue();
    // Whether aPoint in apCell is at least afHeight (a fraction of the actor's height) under water.
    bool IsPointDeepUnderWater(const NiPoint3& aPoint, TESObjectCELL* apCell, float afHeight);
    // The depth of the height afPoint under the water of apCell (the actor's own water without a cell or in its own
    // cell) as a fraction of the actor's height, at most 1; 0 above the water.
    float IsPointDeepUnderWater(float afPoint, TESObjectCELL* apCell);
    // Queues the hurt (abHurt) or normal locomotion animations.
    void QueueReplacementLocomotion(bool abHurt, QueuedFile* apParent);
    // Queues one animation, for apCellOrCoords' cell when it names one.
    void
    QueueReplacementLocomotion(bool abHurt, Animation* apAnimation, const char* apCellOrCoords, QueuedFile* apParent);
    // bSearchingInCombat.
    uint8_t GetSearchingInCombat() {
        return bSearchingInCombat;
    }
    // Returns pCurrentCombatTarget.
    Actor* GetCurrentCombatTarget();

    // Whether the actor can move: not paralysed, dead, knocked out, restrained, unconscious, sitting or sleeping,
    // nor busy with a lower-body or special idle animation; the player cannot while the pip-boy is up.
    bool CanMove();
    // The actor's height: the bound's height times the scale, cached on a high process.
    float GetHeight();
    // Moves the 3D to the actor's position and rotation and updates its multibound rooms and lighting.
    void UpdateActor3DPosition();
    // The actor's aggression (an ACTOR_AGGRESSION), cached by the process when it can.
    ACTOR_AGGRESSION GetActorAggression();
    // Rebuilds the actor's animation from its KF files (queued to the task thread when 3D tasks are queued).
    void ResetLoadedAnimations();
    // Moves a non-disabled actor to where its current (or, with abUseInitialPackage, initial) package wants it.
    void InitPackageLocations(bool abUseInitialPackage);
    // Clears the in-combat flag; an essential NPC that was in combat is healed with abRestoreHealth.
    void ClearInCombat(bool abRestoreHealth);
    // The weapon of the process's current weapon entry, or null.
    TESObjectWEAP* GetCurrentWeapon();
    ItemChange* GetBestFoodItem();
    bool GetBlocked();
    bool HasBackPackWorn();
    bool IsPowerBodyArmorWorn();
    bool IsAnimIdleQuequed();
    WeaponObject* SetHavokWeapon();
    WeaponObject* SetHavokWeapon(bool abConstraintActive);
    bool RequestRotate(float afTargetAngle, bool abForceTurn);
    // Turns the actor towards akPosition.
    bool RequestRotate(NiPoint3 akPosition, bool abForceTurn);
    // Makes the actor a ghost (no collision, no hits) or not.
    void SetGhost(bool abflag);
    // Whether the actor wants its weapon drawn.
    void SetWantWeaponDrawn(bool abTF);
    // Starts the attack group aeAttack; true when it started.
    bool StartAttack(ANIM_GROUP_ENUM aeAttack);
    // Sets the actor's alpha (fade) multiplier.
    void SetAlphaMult(float afAlpha);
    uint8_t DidPathingFail();
    uint8_t IsWaitingOnPath();
    uint8_t IsStoppingMovement();
    PathingRequest* GetCurrentPathingRequest();
    uint8_t GetResetAI();
    void SetLifeState(ACTOR_LIFE_STATE aeLifeState);
    // Sets up the fields every constructor starts from; returns this.
    Actor* ConstructorInitialize(bool abAddToProcessList);
    // Applies the goo or disintegration critical stage to the actor's 3D (queued when 3D tasks are queued).
    void ApplyCriticalStage();
    // Rebuilds the dismembered limbs of a loaded actor from its dismemberment extra data.
    void CreateDismemberedLimbs();
    // Pushes the dismembered limbs' nodes away from the actor at fCombatDismemberedLimbVelocity.
    void UpdateDismemberedLimbVel();
    // Dismembers aeLimb, by damage to aeDismemberLocation.
    void DismemberLimb(
        ActorValue::Index aeDismemberLocation,
        BGSBodyPart::LIMB_ENUM aeLimb,
        bool abExplode,
        bool abBodyBloodSpray,
        bool abSpawnEffects,
        bool abExplodeImmediate
    );
    // Shows the limb's nodes under apDismemberedLimbRoot.
    void UnhideDismemberedLimb(BGSBodyPart::LIMB_ENUM aeLimbIndex, NiNode* apDismemberedLimbRoot);
    // Detaches the limb's root from the actor's 3D.
    bool RemoveDismemberedLimb(BGSBodyPart::LIMB_ENUM aeLimb);
    // Collects the bones of the limb under apLimb into the file's list, recursing into the limb's children.
    void FillBoneArray(NiNode* apLimb, bool abRecursing);

    // Heals the actor's health and limbs fully.
    void RestoreFullHealthAndConditions();
    // Whether the actor was summoned by a spell.
    bool CheckSummonedActor();
    // Dispels the actor's permanent magic.
    void DispelPermanentMagic();
    // Casts the actor's permanent magic, only its worn enchantments with abWornEnchantmentsOnly.
    void CastPermanentMagic(bool abWornEnchantmentsOnly);
    // Whether the worn item's enchantment may be cast on the actor.
    bool CanCastWornEnchantment(ItemChange* apEnchantedItem);
    // Whether apObject's enchantment may be cast on the actor; always returns true.
    bool CanCastWornEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra);
    // Preloads the effects of the actor's permanent spells, worn enchantments and every-actor ability.
    void PreloadPermanentMagic();
    // Unloads the permanent magic of an actor leaving the high or middle-high process.
    void UnloadPermanentMagic();
    // Records the current time as the last time the actor was seen.
    void SetLastSeenTime();
    // Ends the interrupting package (dialogue, combat, ...).
    void EndInterruptPackage(bool bNotRunOnceDialogue);
    // Moves the actor, and its followers, to apMarker's position, angle and cell.
    void MoveActorAndFollowers(TESObjectREFR* apMarker, bool abUseFollowerSpacing, bool abMoveFollowersEvenIfVisible);
    // Moves the actor, and its followers, to aLocation and angle afAngle in the given cell
    // and world space.
    void MoveActorAndFollowers(
        const NiPoint3& aLocation,
        float afAngle,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldspace,
        bool abUseFollowerSpacing,
        bool abMoveFollowersEvenIfVisible
    );
    // The value of the actor's script variable asVariableName; -1 without one.
    double GetLocalVariableOnActor(const char* asVariableName);
    // Clears the forced move mode of the actor's mover.
    void ClearForcedMoveMode();
    // The death bookkeeping of an actor that dies while moving process.
    void DoDeathStuff();
    // Runs the actions the animation update queued.
    void HandlePostAnimationActions();
    // Whether the current package continues while the player is
    // near.
    bool IsContinuingPackagePCNear();
    void HandleCombatProcessLevelChange(PROCESS_TYPE aeOldLevel, PROCESS_TYPE aeNewLevel);
    bool IsFollowing();
    bool IsFollowing(Actor* apLeader);
    // Whether the actor escorts someone who fell behind.
    bool IsEscorterBehind();
    float GetWalkSpeed();
    float GetRunSpeed();
    float GetSwimWalkSpeed();
    float GetSwimRunSpeed();
    // Starts the package that takes the actor up for air.
    void InitiateSurfacePackage();
    // Updates the actor's combat data for the frame.
    void UpdateCombatData();
    // The factor on the radiation the actor takes.
    float GetRadiationResistanceMult();
    // bForceSneak.
    bool GetForceSneak() {
        return bForceSneak;
    }
    // The actor's flying speed: 0 when over-encumbered.
    float GetFlySpeed();
    // Weighs the flight inventory and returns the weight factor passed to ActorValueOwner::CalculateFlySpeed.
    float RecalculateFlyInventoryWeight();
    // A VATS check of the area around apTarget.
    float GetVATSAreaFree(TESObjectREFR* apTarget, float afAngle);
    // Whether apTarget is visible for VATS.
    float GetVATSTargetVisible(TESObjectREFR* apTarget, float afAngle);
    // Whether the actor wants its weapon out; swimming ends the player's VATS playback.
    bool GetWantWeaponDrawn();
    // Whether the actor can be knocked down now.
    bool CanKnockDown();
    // Seats the actor at once on furniture marker aiIndex of apFurniture, in bed when
    // abSleeping.
    bool PutActorInChairBedQuick(TESObjectREFR* apFurniture, FurnitureMark* apmarker, int32_t aiIndex, bool abSleeping);
    // Unlocks or locks the doors the current package asks for.
    void UnlockLockDoorsProcedure();
    void UpdateRagdollAnimation(bool abDriveRagdoll);
    void GetOutofFurnitureQuick();
    // Re-evaluates the actor's package, at once when abImmediate, resetting the AI when
    // abResetAI.
    void EvaluatePackage(bool abImmediate, bool abResetAI);
    // True when the actor should turn to track a head track target, apRef being the explicit
    // target or null for the process's current one.
    bool ShouldRotateToTrack(TESObjectREFR* pRef);
    // The reference the actor is turning to face, or null.
    TESObjectREFR* GetHeadingTarget();
    // The reference the actor should look at by default.
    TESObjectREFR* GetBestDefaultHeadTrackTarget();
    bool IsInWater();
    bool IsIgnoreFriendlyHits();
    void SetForceSneak(bool abflag);
    bool FightsInWater();
    bool ShouldModulateVoice();
    TESPackage* GetPackage();
    TESPackage* GetCurrentEditorPackage();
    bool IsRunning();
    void ResetConditionPreventsRun();
    bool IsFightingOrAlarmed();
    float GetSneakHeight(bool abEyeLevel);
    int32_t GetGoldAmount();
    void PayGoldToActor(Actor* apRef, int32_t aiamount);
    ItemChange* GetRandomWornArmor(bool abHeadHit);
    void SetCriticalStage(ACTOR_CRITICAL_STAGE aeStage);
    void ClearLastHitData();
    bool GetAlert();
    bool IsinSpectator();
    bool IsSurfacing();
    bool IsReloading();
    // Queues a reload of the current weapon.
    void QueueReload();
    bool GetOffersServicesNow();
    void SetUnConscious(bool abFlag);
    void SetRestrained(bool abFlag);
    void SetForceUpdateQuest(bool abvalue);
    bool IsVisible(int32_t aiVisMask);
    void FlushReloadTargetQueque();
    Actor* GetCommandingActor();
    bool IsPathValid() const;
    void ForceStopMoving();
    void ResumeMoving();
    bool IsPathingComplete() const;
    bool IsPathing() const;
    bool IsRotating() const;
    bool IsWaitingForAvoidance() const;
    void SetPathingDoorReference(TESObjectREFR* apDoor);
    bool IsHeavyBodyArmorWorn();
    bool IsPowerArmorHelmetWorn();
    bool HasFollower(Actor* apFollower);
    // Places blood decals on the weapon that dealt afDamage to apTarget in a melee hit.
    void CreateBloodOnWeapon(float afDamage, HitData* apHitData, Actor* apTarget);
    // Stores apData as the last attack hit data of the actor's process.
    void SetLastAttackHitData(HitData* apData);
    // Plays the hit sound of apHitData.
    void PlayHitSound(HitData* apHitData);
    // Spawns the blood of a hit of afDamage.
    void CreateBlood(float afDamage, HitData* apHitData);
    // Staggers the actor.
    void Stagger();
    // Pushes the killed target of apData with the weapon's kill force.
    void DoWeaponKillForce(HitData* apData);
    // Explodes or dismembers the limb apHitData crippled, killed
    // by apKiller with the cause aeCauseOfDeath.
    void DismemberLimbs(HitData* apData, TESForm* apDeathObject, CAUSE_OF_DEATH aeCause);
    // False.
    bool CheckAttackDisarm(Actor* apTarget, uint32_t aeWeaponSkill);
    // False.
    bool CheckBlockDisarm(Actor* apTarget);
    // False.
    bool CheckBlockStagger();
    bool IsFollowAccompanyPackageTarget(TESForm* apTarget);
    void FadeIn();
    void FadeOut();
    float GetFadePercent();
    void IncrementMinorCrimes();
    void SetActorRefEssential(bool abEssential);
    void SetAlwaysShowSubtitles(bool abShow);
    int32_t GetShowSubtitles();
    bool CanSneak();
    void SetEditorLocationForm(TESForm* apSpace);
    bool IsNotOutOfBreath();
    void SetWalkSpeed(float afSpeed);
    void SetRunSpeed(float afSpeed);
    bool HasObjects(
        TESBoundObject* apObj,
        PACKAGE_OBJECT_TYPE aeFormID,
        int32_t ainumber,
        uint32_t aId,
        PACKAGE_OBJECT_TYPE& aeMatchId
    );
    bool ShouldTalkTo(TESObjectREFR* apRef);
    void HandleDeletedReferenceForActorMover(TESObjectREFR* apRef);
    bool GetCurrentPathfindingGoal(PathingLocation* apRetLocation);
    void BuildRequest(
        PathingRequest* apRequest,
        const NiPoint3& aGoalLocation,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float afTargetRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    // Returns ActorMover::GetCurrentNodeIndex's result.
    uint32_t GetCurrentNodeIndex();
    char GetPathingLocationAt(int32_t aiNodeIndex, NiPoint3* apLocation);
    PathingLocation* GetFailedPathDestination();
    void SetLookAtTarget(NiPoint3 aPoint);
    bool SetPathfindingHide(const PathingRequest* apRequest);
    void SetActionSoundValue(int32_t aiValue);
    void
    ActivatedReference(TESObjectREFR* apRef, TESBoundObject* apObject, int32_t aiCount, bool abActivatedScriptOnly);
    void SetPlayerTeammate(bool abValue);
    void IncrementMajorCrimes();
    bool IsInCastPowerList(SpellItem* apPower);
    int32_t ModifyMoveModeForFollow(int32_t emovemode, float afbaseRadius, float afDistToTarget);
    void SetCombatStyle(TESCombatStyle* apCombatStyle);
    bool CanChildUseObject(TESObjectREFR* apObjectRef);
    void SetIgnoreFriendlyHits(bool abSet);
    NiPoint3* GetActorMoverOverrideMovement();
    float DifficultyLevelAdjustHealthModifier(float afValue, Actor* apAttacker);
    bool GetForceRun();
    void InitiateMovementBlockedPackage();
    void RemoveFleeTarget(TESObjectREFR* apRef);
    void SetHasBeenEaten(bool abFlag);
    int32_t GetDetectionLevel(Actor* apActor);
    float GetCurrentAmmoClipPercent();
    void StopAttack();
    bool
    SetPathfindingGoalAndAngle(TESObjectREFR* apReference, float afTargetRadius, PathingAvoidNodeArray* apAvoidNodes);
    void DeleteCastPowers();
    void BlockHit(bool abBlockAttack);
    bool BreathesUnderwater();
    void SetLastHitData(HitData* apData);
    int32_t GetCurrentAmmoClipCount();
    bool SetLocalVariableOnActor(const char* asVariableName, double afNewValue);
    void ClearDispositionModifiers();
    void ClearLastAttackHitData();
    void ResetFaceData();
    DETECTION_LEVEL GetDetectionLevelValue(Actor* apActor);
    void AlertAccompanyToPackageChange();
    void ForceMoveMode(uint32_t auiMoveMode);
    void InitiateDoNothingPackage();
    void StartGreetingPlayer(TESTopic* apTopic);
    bool IsTryingToEnterFurniture();
    bool GetHasBeenEaten();
    ANIM_GROUP_ENUM GetIronSightsAttack(Animation* apAnim, ANIM_GROUP_ENUM aeAttack, TESObjectWEAP* apWeapon);
    void AddFaction(TESFaction* apFaction, int32_t aiRank);
    char IsEvil();
    void AddDeathItems();
    bool HasBlood();
    const char* GetRaceFullName();
    NiAVObject*
    ExplodeLimb(BGSBodyPart::LIMB_ENUM aeLimbIndex, NiAVObject* apOldLimb, NiTransform* aTrans, bool abSpawnEffects);
    void HideDismemberedLimb(
        BGSBodyPart::LIMB_ENUM aeLimbIndex,
        NiNode* apLimbRoot,
        NiAVObject* apLimbReplacement,
        bool abExplode
    );
    void ClearDismemberedLimbs();
    void AddFactionMinorCrime(int32_t aiNum, bool bModRep);
    int32_t GetFactionRank(TESFaction* apFaction, bool abPC);
    bool HasFactionThatCaresAboutCrime();
    void SetFactionRank(TESFaction* apFaction, uint8_t acRank);
    void ModFactionRank(TESFaction* apFaction, uint8_t acModify, bool abPC);
    void ProcessDamageImpacts();
    bool UpdateRagdollAnimState(bool abConsciousState, float afDelta);
    void Eat(TESBoundObject* apfood, ExtraDataList* pextra, bool abNow);
    void SetShouldCheckMagicNode(bool abCheck);
    bool HasShadow();
    bool IsJumping();
    void AdjustSpeedForFollowing(float& fspeed, float afBaseRadius, float afDistToTarget);
    bool PutItemInReference(
        TESBoundObject* apObject,
        ExtraDataList* apExtra,
        TESObjectREFR* apRef,
        int32_t aiNumber,
        uint32_t aId
    );
    ANIM_GROUP_ENUM Dodge(uint8_t acMoveDir);
    double GetAnimActionTime(ANIM_GROUP_ENUM aeGroup);
    void InitiateAggroRadiusGuardPackage(Actor* apActor);
    void InitiateAvoidPackage(TESObjectREFR* apRadiation, float afTimer);
    bool CanTalk();
    ItemChange* GetItemBeingEscorted();
    float ComputeTargetDesirability(TESObjectREFR* apRef);
    bool CanHeadTrack(TESObjectREFR* pRef, bool abCheckLineOfSight);
    void ForceIdleAnimation();
    bool IsInCombatantFaction();
    void ClearDeathState();
    bool CanSkipActorUpdate();
    void StartWeaponGroup(ANIM_GROUP_ENUM aeBaseAnimGroup, Animation* apAnim);
    bool SetPathfindingGoalAndAngle_0(
        const NiPoint3& aGoalLocation,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float afAngleToFace,
        float afTargetRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    bool GetCurrentPathfindingGoal(NiPoint3* apRetLocation);
    void BuildRequest_0(
        PathingRequest* apRequest,
        const NiPoint3& aGoalLocation,
        TESForm* apSpace,
        float afTargetRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    void Dismember(
        HitData* apData,
        ActorValue::Index aeDismemberLocation,
        BGSBodyPart::LIMB_ENUM aeLimb,
        TESForm* apDeathObject,
        CAUSE_OF_DEATH aeDeath,
        int32_t aiDismemberBonus,
        int32_t aiExplodeBonus,
        bool abTorsoExplodes
    );
    void ScriptDismember(CAUSE_OF_DEATH aeCause, BGSBodyPart::LIMB_ENUM aeLimb, Actor* apKiller);
    void HandleMajorCrimeFactionReputations(int32_t aiNum, char acModRep);
    bool SetFactionsThatCareAboutCrime(Actor* apActor, Crime* apCrime);
    void InitiateUseWeaponPackage(
        TESBoundObject* apWeap,
        TESObjectREFR* apLocRef,
        TESObjectREFR* apTargRef,
        int32_t aiAttackTimes,
        TESObjectREFR* apTargetLocRef,
        bool abCrouch,
        bool abHoldFire,
        bool abAlwaysHit,
        bool abDoNoDamage
    );
    void AddBackCrime(Crime* apCrime);
    bool ShouldActorAvoidRadiation();
    void InitiateAlarm(Crime* apCrime, int32_t aiProcedureIndex, bool abSendAlarm, Actor* apActor);
    void InitiatePickPocketWarning(Actor* apActor);
    void InitiateStealWarning(Actor* apActor, TESForm* apOwner);
    void AttackAlarm(TESObjectREFR* apRef, bool abMinor, bool bModdispthis);
    void MurderAlarm(TESObjectREFR* apRef);
    int32_t TrespassAlarm(TESObjectREFR* apRef, TESForm* apOwnership, int32_t aiCrime);
    bool DrinkPotion(AlchemyItem* apPotion, ExtraDataList* pextra, bool abNow);
    void CastAlchemy(MagicItem* apPotionIngredient, bool abLast);
    // Casts the worn enchantment of an armour or clothing item; apExtra is unused.
    void CheckEquipWornEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra);
    void AttachCastingArt(MagicItem* apSpell);
    float GetAlphaMult();
    void CheckDeletedCaster(MagicCaster* apMagicCaster);
    void RemoveCastPower(SpellItem* apPower);
    void DispellSpellsForRace();

    // Resets the armor rating of the actor behind apOwner. An actor value modified callback: RegisterActorValue stores
    // it, and it is called with the callback's full argument list, of which it reads only apOwner.
    static void ResetActorDTDR(
        ActorValueOwner* apOwner,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    // The armor threshold of the actor behind apOwner. An actor value derivation function: aiWhich is supplied by the
    // callback's callers and not read.
    static float GetTotalArmorDT(ActorValueOwner* apOwner, ActorValue::Index aiWhich);
    // The ActorValueOwner base value callback: the armor rating of the actor behind apActor. aiWhich is passed by the
    // callback's callers and not read.
    static float GetBaseDamageResistance(ActorValueOwner* apActor, ActorValue::Index aiWhich);
    // The ActorValueOwner modified callback: marks the cached assistance of the actor behind apActor stale. Only
    // apActor is read; the callback's callers pass the others.
    static void AssistanceModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    // The ActorValueOwner modified callback for the rads: swaps the radiation sickness when the stage changes and tells
    // the player. apActorSource is pushed by the callback's callers and not read.
    static void CheckRadiationLevelChange(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    // True when apActor exists, can speak and should not attack the player (the activate
    // check of TESNPC and TESCreature).
    static bool IsConsciousAndNotAttackingPlayer(Actor* apActor);
    static float GetAttackSpeedMult(Actor* apActor, TESObjectWEAP* apWeapon, int8_t aiHand, ItemChange* apItem);
    // The weight of apActor's inventory, an actor value callback.
    static float GetInventoryWeight(ActorValueOwner* apActor, ActorValue::Index aiWhich);
    // Gets the distance for an animation group.
    static bool GetDistanceForAnimGroup(Actor* apActor, ANIM_GROUP_ENUM aeGroup, NiPoint3* apDistance);
    // Queues the hurt locomotion when a body part's condition drops to 0 and the replaced locomotion when it
    // recovers.
    static void QueueReplacementLocomotion(
        Actor* apActor,
        float afPreviousBodyPartCondition,
        float afNewBodyPartCondition,
        float afOtherBodyPartCondition
    );
    // Removes a picked-up reference from the world: deactivates its 3D, then deletes it or marks it picked up.
    static void RemovePickedUpObjectFromWorld(TESObjectREFR* apPickedUpRef, bool abDelete);
    // The actor value owner's damage modifier clamped for the actor value aeAVIndex.
    static float CheckClampDamageModifier(ActorValueOwner* apOwner, ActorValue::Index aeAVIndex, float afValue);
    // The duration of the animation group's sequence; -1 for a group that is no power attack.
    static float GetAnimGroupDuration(Actor* apActor, ANIM_GROUP_ENUM aeGroup);
    static bool IsUsingNonMeleeWeapon(Actor* apActor);
    // The end-of-line callback of the player's dialogue: ends the speaking actor's line.
    static void ChangeDialogueAnimation(void* apContext, bool abSucceeded);
    static void ConditionModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    // ConditionModifiedCallback, then the replacement locomotion for a changed mobility condition.
    static void MobilityConditionModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void PlayLandJumpSound(PlayerCharacter* apCharacter, HavokMaterialType aeMaterial);
    static void HealthFatigueModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void MedicineSkillModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void SurvivalSkillModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void ParalysisModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void HealingRateModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void EnduranceModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void AggressionModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void RadiationResistanceModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void InventoryWeightModifiedCallback(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    // ActorValue change handlers of DEHYDRATION, HUNGER and SLEEPDEPREVATION.
    static void CheckDehydrationLevelChange(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void CheckHungerLevelChange(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void CheckSleepDeprevationLevelChange(
        ActorValueOwner* apActor,
        ActorValue::Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static void HandleGrabIKNote(TESObjectREFR* apObjRefr, BSAnimNote* apNote);

    bhkRagdollController* pRagdollController;
    bhkRagdollPenetrationUtil* pPenetrationDetection;
    DIALOGUE_EMOTION ePersuasionEmotion;
    float fEmotionValue;
    bool bProcessMe;
    Actor* pMyKiller;
    bool bMurderAlarm;
    float fCheckMyDeadBodyTimer;
    float fDeadBodyAlarm;

    // The actor value modifiers that never expire (a ModifierList by value, despite the p prefix).
    ModifierList pPermanentModifiers;

    // Actor values forced to a base value (a ModifierList by value).
    ModifierList pBaseValueOverrides;
    bool bBlockPostAnim;
    bool bReloadTargetQueued;
    bool bRunsInLow;
    BSSimpleList<void*> CastPowers;
    // The disposition adjustments toward other actors.
    BSSimpleList<DispositionModifier*> DispModifierList;
    bool bInCombat;
    ACTOR_LIFE_STATE eLifeState;
    ACTOR_CRITICAL_STAGE eCriticalStage;
    ANIM_GROUP_ENUM eQueuedattack;
    float fLastUpdate;
    bool bDeadFlag;
    uint32_t iVisFlags;
    uint32_t iLastSeenTime;
    bool bForceRun;
    bool bForceSneak;
    bool bForceUpdateQuestTarget;
    uint8_t bSearchingInCombat;
    Actor* pCurrentCombatTarget;
    BSSimpleArray<Actor*>* pCurrentCombatTargetArray;
    BSSimpleArray<Actor*>* pCurrentCombatMemberArray;
    bool bAttackOnNextTheft;
    // The day (bits 0-8), month (9-12) and year (13 up) of the last theft; the alarms test it signed.
    int32_t iThiefCrimeStamp;
    uint32_t iMinorCrimes;

    // Runtime flag bits of the actor (ACTOR_FLAG_*) above the major-crime count, despite the name.
    uint32_t iMajorCrimes;
    bool bIgnoreCrime;
    bool bEVPBuffered;
    uint8_t bResetAI;
    TESActorBase* pTemplateActorBase;
    bool bInWater;
    bool bSwimming;
    bool bUpdateLighting;
    uint32_t iActionValue;
    float fTimeronAction;
    float fHeadTrackTimer;
    bool bWasInFrustum;
    bool bShouldRotateToTrack;
    NiPoint3 EditorLocCoord;
    float fEditorLocZRot;
    TESForm* pEditorLocForm;
    bool bSetOnDeath;
    bool bContainerReset;
    float fGunSkillGun;
    float fGunSkillHUD;
    float fGunSkillActor;
    float fGunSkillVATS;
    float fCurrentWeaponCondition;
    bool bFootIKInRange;
    bool bPlayerTeammate;
    bool bLightingUpdatedNonMoving;
    ActorMover* pActorMover;
    // Declared void* rather than HitData*: of the Actor/Character/Creature/PlayerCharacter functions only
    // ConstructorInitialize touches it (clears it); nothing reads it as a HitData.
    void* pLastHitData;
    TESPackage* pInitialPackage;
    float fRadiationResistanceMult;
    ContinuousBeamProjectile* pContinuousBeamPersistant;
    uint32_t iEmotion;
    uint32_t iEmotionValue;
    SIT_SLEEP_STATE cCurrentSitSleepState;
    bool bTurretBehavior;
    bool bForceHitReaction;

    // The INI settings and the cooldown timestamp of the hit dialogue sound.
    static SettingT<INISettingCollection> fDialogHitSoundCooldownMax;
    static SettingT<INISettingCollection> fDialogHitSoundCooldownMin;
    static inline uint32_t uiDialogHitSoundCooldownNext;
    static inline int32_t LastPositionID;
    static CachedPositionArray CachedPositions;
    // Cleared by Main::RunScripts every frame.
    static inline bool bAllPointsVisible;
    // The actor speaking a dialogue line to the player.
    static inline Actor* pTalkingToPlayer;
    static BSCriticalSection kSpeakSoundCriticalSection;
    // An immobile actor's line of sight ignores hits nearer than this.
    static inline float fImmobileLOSMinHitDistance = 95.0f;
};

// Folds apFaction's reaction to apOtherFaction into aeReaction: a neutral reaction takes
// it, an enemy one any non-neutral one, a friend one only an ally one.
FIGHT_REACTION
UpdateFightReaction(FIGHT_REACTION aeCurrentFightReaction, TESFaction* apFaction1, TESFaction* apFaction2);
bool Cmd_GetIgnoreFriendlyHits(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool IsActorOfFaction(TESObjectREFR* apRef, TESForm* apOwnership, Actor* pactor);
bool IsActorOfFactionList(BSSimpleList<FACTION_RANK*>* pList, Actor* pActor, const char* debugReason);

// The challenge limb (BGSBodyPart::LIMB_CHALLENGE) of each body part slot, indexed by the BGSBodyPart::LIMB_ENUM value
// plus 1: entry 0 is for no limb (-1). The array has 16 entries.
using eLimbForChallengeArray = uint32_t[16];
inline eLimbForChallengeArray eLimbForChallenge{0, 2, 1, 1, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 1, 6};

// The challenge limbs' display names, indexed by BGSBodyPart::LIMB_CHALLENGE.
using pChallengeLimbNamesArray = const char* [7];
inline pChallengeLimbNamesArray pChallengeLimbNames{"None", "Head", "Chest", "Left Arm", "Right Arm", "Legs", "Weapon"};

// The game settings Actor.cpp defines.
extern SettingT<INISettingCollection> bTrackAllDeaths;
extern SettingT<INISettingCollection> bUseMovementBlockedPackage;
extern SettingT<INISettingCollection> bDisableNPCAttacks;
extern SettingT<INISettingCollection> bDisableHeadTracking;
extern SettingT<INISettingCollection> iDetectionHighNumPicks;
extern SettingT<INISettingCollection> fHardLandingDamageThreshold;
extern SettingT<INISettingCollection> fUpdateDelayNewTargetSecondsMin;
extern SettingT<INISettingCollection> bPlayStaggers;
extern SettingT<INISettingCollection> fJumpAnimDelay;
extern SettingT<INISettingCollection> fMinBloodDamage;
extern SettingT<INISettingCollection> fHitVectorDelay;
extern SettingT<INISettingCollection> bPlayHitLocationIdles;
extern SettingT<INISettingCollection> iMaxHiPerfCombatCount;
extern SettingT<INISettingCollection> fUpdateDelayNewTargetSecondsMax;
extern SettingT<INISettingCollection> bDisableAllGore;
extern SettingT<INISettingCollection> iDetectionPicks;
extern SettingT<INISettingCollection> fUpdateDelaySecondsMin;
extern SettingT<INISettingCollection> fUpdateDelaySecondsMax;
extern SettingT<INISettingCollection> iShowHitVector;
extern SettingT<INISettingCollection> iUpdateActorsPerFrame;
extern SettingT<INISettingCollection> bDismemberOneLimb;
extern SettingT<INISettingCollection> bVATSSmartCameraCheckDebug;
inline int32_t iActorsDismemberedThisFrame;

// Set while Havok is skipped for the frame; the 3D is then not moved to the reference's position.
inline bool bSkipHavokThisFrame;

// The node names of the body part limbs, by BGSBodyPart::LIMB_ENUM.
using pLimbEnumNames_t = const char* [15];
inline pLimbEnumNames_t pLimbEnumNames{
    "DismemberedLimb0 Torso",
    "DismemberedLimb1 Head1",
    "DismemberedLimb2 Head2",
    "DismemberedLimb3 LeftArm1",
    "DismemberedLimb4 LeftArm2",
    "DismemberedLimb5 RightArm1",
    "DismemberedLimb6 RightArm2",
    "DismemberedLimb7 LeftLeg1",
    "DismemberedLimb8 LeftLeg2",
    "DismemberedLimb9 LeftLeg3",
    "DismemberedLimb10 RightLeg1",
    "DismemberedLimb11 RightLeg2",
    "DismemberedLimb12 RightLeg3",
    "DismemberedLimb13 Brain",
    "DismemberedLimb14 Weapon"
};

extern SettingT<INIPrefSettingCollection> gs_bTrueIronSights_GamePlay;

// The bone list the dismemberment code walks: how many nodes it holds and the nodes.
inline uint32_t uiLimbCount;
using pLimbBones_t = NiNode* [256];
inline pLimbBones_t pLimbBones{};

// The player's camera nodes (PlayerCharacter.cpp overlays them too).
inline NiAVObject* p3rdPersonCamera;

// Seconds until a follower barks about stealthing again.
inline float time_followerStealthing;

// The worn slots whose enchantments are preloaded (head, hair, upper body, left and right hand).
using WornBodyParts_t = BIPED_OBJECT[5];

extern WornBodyParts_t WornBodyParts;

extern BSPerformanceTimer AIProcessing_UpdateAnimation;

extern BSPerformanceTimer CombatAI_Total;

extern BSPerformanceTimer AIProcessing_UpdateMovement;

extern BSPerformanceTimer AIProcessing_UpdateAnimationMovement;
