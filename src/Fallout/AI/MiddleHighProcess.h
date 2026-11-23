#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/MiddleLowProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class ActiveEffect;
class Actor;
class ArrowProjectile;
class bhkCharacterController;
class BSBound;
class BSFaceGenNiNode;
class BSPerformanceTimer;
class BSShaderPPLightingProperty;
class HitData;
class MagicCaster;
class MagicItem;
class MagicTarget;
class MobileObject;
class PackageTarget;
class PatrolActorPackageData;
class QueuedItem;
class SpellItem;
class TESIdleForm;
class TESObjectCELL;
class TESObjectWEAP;
class TESPackage;
class TESWorldSpace;

enum ANIM_GROUP_ENUM : int32_t;

// the process of an actor that is close enough to be animated: the weapon,
// the furniture, the magic and the 3D-related state of the actor
class MiddleHighProcess : public MiddleLowProcess {
public:
    // sleep and sit state changes the animation side of the actor reports
    // through GetPostAnimationActions
    enum {
        kPostAnimationSitSleepActive = 0x8,
    };

    // Each movement anim group has two bits in iMovementAnimations.
    enum MOVEMENT_ANIM_FLAGS : uint32_t {
        MOVEMENT_ANIM_LOADED = 0x1,
        MOVEMENT_ANIM_NOT_LOADED = 0x2,
    };

    // A dismembered limb whose hiding waits for its timer (the items of DeferredHideLimbList).
    struct DeferredHideLimb {
        DeferredHideLimb();
        ~DeferredHideLimb();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        float fDismemberTimer;
        BGSBodyPart::LIMB_ENUM eLimbIndex;
        NiPointer<NiNode> spDismemberedLimbRoot;
        NiPointer<NiNode> spReplacementLimb;
        bool bExplosion;
    };

    MiddleHighProcess();
    ~MiddleHighProcess() override;

    void HandleDeletedReference(TESObjectREFR* apRef) override;
    void Update(MobileObject* apRef, float fTimeDelta) override;
    void Update(MobileObject* apRef) override;
    bool CheckforNewPackage(Actor* apActor, bool abFlag) override;
    MagicItem* GetCurrentPackageSpell() override;
    void SetCurrentPackageSpell(MagicItem* apSpell) override;
    void ClearFaceAnimationData() override;
    bool GetIdleDoneOnce() override;
    void SetIdleDoneOnce(bool abflag) override;
    void ClearFurniture(Actor* apActor) override;
    bool SetTargetForPackage(Actor* apActor) override;
    TESObjectREFR* SetLocationForPackage(Actor* apActor, bool bSecondLoc) override;
    bool GetPickpocketed() override;
    void SetPickpocketed(bool abflag) override;
    void SetTarget(TESObjectREFR* apRef) override;
    void SetAcquireObject(ObjectstoAcquire* apacquire) override;
    ObjectstoAcquire* GetSavedAcquireObject() override {
        return pSavedAcquireObject;
    }
    void SetSavedAcquireObject(ObjectstoAcquire* apobject) override;
    BSFaceGenAnimationData* GetFaceAnimationData(Actor* apActor) override;
    ItemChange* GetCurrentWeapon() override;
    ItemChange* GetCurrentAmmo() override;
    bool SetCurrentWeapon(ItemChange* apweap, NiNode* ap3D, bool bForce) override;
    bool SetCurrentAmmo(ItemChange* apammo) override;
    void ResolveEquipmentQueue(Actor* apActor) override;
    bool IsItemQueued(TESBoundObject* apObject, ExtraDataList* apExtra, bool abEquipped) override;
    void AddQueuedItem(
        Actor* apActor,
        bool abEquip,
        TESBoundObject* apObject,
        int32_t aiNumber,
        ExtraDataList* apExtra,
        bool abNow,
        bool abForceEquip,
        bool abFromMenus,
        bool abForceUnequip,
        bool abEquipCalling,
        bool abPlayPickUpSounds
    ) override;
    void RemoveQueuedItem(TESBoundObject* apObject, ExtraDataList* apExtra) override;
    void RemoveAllQueuedItems() override;
    NiAVObject* GetFireNode() override {
        return pFireNode;
    }
    void SetFireNode(NiAVObject* apFireNode) override;
    void StoreBonePointers(NiControllerManager* apManager, NiNode* ap3D) override;
    NiNode* GetWeaponBone(BipedAnim* apBiped) override;
    NiNode* GetRightWeaponBone(BipedAnim* apBiped) override;
    bool IsCurrentWeaponGrenade() override;
    bool IsCurrentWeaponMine() override;
    bool IsCurrentWeaponGrenadeOrMine() override;
    bool IsCurrentWeaponThrown() override;
    Animation* GetAnimation() override;
    void ForceWeaponDrawnSheathed(bool abDrawn, BipedAnim* apBiped, Animation* apAnim, Actor* apActor) override;
    void UpdateReparentWeapon(Actor* apActor) override;
    TESPackage* GetRunOncePackage() override;
    void SetRunOncePackage(TESPackage* apPack, MobileObject* apActor) override;
    void ClearRunOncePackage() override;
    void SetRunOnceProcedureIndex(int32_t aiNewIndex) override;
    int32_t GetRunOnceProcedureIndex() override;
    void AddToRunOnceProcedureIndex(int32_t aiOffset) override;
    bool IsPackageDone(Actor* apActor) override;
    void SetupNewPackage() override;
    ActorPackageData* GetRunOncePackageInstanceData() override;
    ActorPackageData* GetInstanceDataThatIsRunning() override;
    ActorPackage* GetActorPackageThatIsRunning() override;
    TESPackage* GetPackageThatIsRunning() override;
    int32_t GetProcedureIndexRunning() override;
    void SetProcedureIndexRunning(int32_t ainumb) override;
    void AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb) override;
    bhkCharacterController* GetCharController() override;
    void SetCharController(NiPointer<bhkCharacterController> apControl) override;
    void EndMoveMessage(Actor* apActor) override;
    void ProcessFollow(Actor* apActor, bool abflag, int32_t emovemode, bool abpursuit) override;
    void ProcessPursue(Actor* apActor, bool abNewpackage, bool abInterateIndex, int32_t emovemode) override;
    void ProcessFlee(Actor* apActor) override;
    void ProcessGetUpOneHour(Actor* apActor) override;
    bool SitOrSleep(Actor* apActor) override;
    bool ShouldDisableCharController(TESObjectREFR* apFurnitureRef, uint32_t aiFurnitureNUm) override;
    bool ProcessGetUp(Actor* apActor) override;
    void ProcessTravel(
        Actor* apActor,
        bool abNewpackage,
        bool abInterateindex,
        int32_t aemovemode,
        bool abSecondLoc
    ) override;
    bool EnterCombat(
        Actor* apActor,
        Actor* apTarget,
        bool abDontCheckForAmbush,
        bool absendalarm,
        CombatGroup* apGroup,
        bool abfleeing,
        bool abForceFlee,
        bool abIsAggressor,
        char acTeam,
        bool bforcecombat,
        bool bIgnoreDefensivePackage,
        bool abCastSpell,
        TESPackage* apPackage
    ) override;
    bool IsRunningRunOnce() override;
    int32_t GetWeaponCondition(Actor* apActor) override;
    TESIdleForm* GetLastIdlePlayed() override;
    void SetLastIdlePlayed(TESIdleForm* apIdle) override;
    BSSimpleList<ActiveEffect*>* GetActiveEffectList() override;
    void CastAbility(MagicCaster* apCaster, SpellItem* apSpell, bool abLoadCast) override;
    void
    TransferDisease(MagicCaster* apCaster, SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) override;
    MagicItem* GetCurrentSpell() override;
    void SetCurrentSpell(MagicItem* apCurrentSpell) override;
    MagicTarget* GetDesiredTarget() override;
    void SetDesiredTarget(MagicTarget* apDesiredTarget) override;
    bool GetShouldCheckMagicNode() override;
    void SetShouldCheckMagicNode(bool abCheck) override;
    void SetIronSights(bool abIronSights) override;
    bool GetIronSights() override;
    bool HasMovementAnim(ANIM_GROUP_ENUM aeMovementAnim) override;
    KNOCK_STATE_ENUM GetKnockState() override;
    void SetKnockState(int8_t aeKnockState) override;
    void KnockExplosion(Actor* apActor, NiPoint3 aSourceLocation, float afMagnitude) override;
    void UpdateKnockState(Actor* apActor) override;
    void ClearKnockState(Actor* apActor) override;
    void SetAimSequence(AIM_ENUM aeDirection, BSAnimGroupSequence* apSeq) override;
    void UpdateAimSequences(Actor* apActor) override;
    float GetAimLooking() override;
    void AddGunDrift(float afDegrees) override;
    void ClearAutomaticShotsToFire(bool abStartShooting) override;
    uint8_t GetAutomaticShotsToFire(Actor* apActor) override;
    void SetShotsToFire(uint8_t acShots) override;
    uint8_t GetShotsToFire() override;
    float GetAutomaticShotDelay() override;
    bool GetWantWeaponDrawn() override;
    void SetWantWeaponDrawn(bool abWantDrawn) override;
    bool GetWeaponDrawn() override;
    void SetWeaponDrawn(Actor* apActor, bool abDrawn) override;
    void Set3DUpdateFlag(RESET_3D_FLAGS aeFlag) override;
    void Clear3DUpdateFlag(RESET_3D_FLAGS aeFlag) override;
    void ClearAll3DUpdateFlags() override;
    bool Get3DUpdateFlag(RESET_3D_FLAGS aeFlag) override;
    uint8_t GetAll3DUpdateFlags() override {
        return cUpdate3DModel;
    }
    SIT_SLEEP_STATE GetSitSleepState() override;
    void SetSitSleepState(
        Actor* apActor,
        SIT_SLEEP_STATE aeState,
        TESObjectREFR* apFurnitureRef,
        uint8_t acFurnitureIndex
    ) override;
    uint32_t GetFurnitureMarkerID() override;
    TESObjectREFR* GetCurrentFurniture() override;
    void RemoveFurnitureRef(TESObjectREFR* apFurnitureRef) override;
    uint32_t GetCurrentFurnitureIndex() override;
    FurnitureMark* GetFurnitureMarker() override;
    bool LoadDynamicIdle(Actor* apActor) override;
    void LoadSitSleepIdle(Actor* apActor, Animation* apAnimation) override;
    bool HasBeenAttacked() override;
    void SetBeenAttacked(bool abFlag) override;
    BSShaderPPLightingProperty* GetLightingShaderProperty() override;
    void SetLightingShaderProperty(BSShaderPPLightingProperty* aplight) override;
    void CalculateLightValue(Actor* apActor, bool abIgnoreTorch) override;
    void ClearCurrentDataforProcess(Actor* apActor) override;
    void SetCommandingActor(Actor* apActor) override;
    Actor* GetCommandingActor() override;
    void SetLastBoundWeapon(TESObjectWEAP* apBoundWeapon) override;
    TESObjectWEAP* GetLastBoundWeapon() override {
        return pLastBoundWeapon;
    }
    void SetFurniture(TESObjectREFR* pref, uint8_t aiindex, FurnitureMark* apMarker) override;
    void RemoveAndAttachItems(MobileObject* apMob) override;
    uint16_t GetSaveSize(uint32_t aiFlags, MobileObject* pmob) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void SaveGame(uint32_t aiFlags, MobileObject* pmob) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) override;
    void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(uint32_t aiFlags, MobileObject* pmob) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    TESEffectShader* GetWeaponEnchantmentVisuals() override;
    void SetWeaponEnchantmentVisuals(TESEffectShader* apEffect) override;
    bool GetShouldRefreshMagicShadersEffects() override;
    void SetShouldRefreshMagicShadersEffects(bool abWeapon, bool abBody, bool abLoading) override;
    void RefreshMagicShaderEffects(Actor* apActor) override;
    void SetBeginIdlesPlayed(bool abFlag) override;
    bool GetBeginIdlesPlayed() override;
    void SetEndIdlesPlayed(bool abFlag) override;
    bool GetEndIdlesPlayed() override;
    float GetAlphaMult() override {
        return fAlphaMult;
    }
    void SetAlphaMult(float afAlpha) override;
    float GetScriptRefractionPower() override {
        return fScriptRefractPower;
    }
    void SetScriptRefractionPower(float afRefractPower) override;
    bool ShouldCheckFlare(BSSimpleList<ActiveEffect*>* apList, MagicSystem::FlareType aeFlareType) override;
    void SetRefreshFlareFlags() override;
    BSBound* GetBSBound() override;
    void SetBSBound(BSBound* apBound) override;
    void SetForceNextUpdate(bool abForce) override;
    bool GetForceNextUpdate() override {
        return bForceNextUpdate;
    }
    void SetIsSummonedCreature(bool abValue) override;
    bool GetIsSummonedCreature() override {
        return bSummonedCreature;
    }
    void AddAttachedArrow(ArrowProjectile* apArrow) override;
    void RemoveAttachedArrow(ArrowProjectile* apArrow) override;
    void MoveAttachedArrowsToParentCell(TESObjectREFR* apAttachedRef) override;

    BSSimpleList<ArrowProjectile*>* GetAttachedArrowList() override {
        return pAttachedArrowList;
    }

    void StoreDamageNodes(NiNode* apNode, BGSBodyPartData* apBodyPartData) override;
    void StoreDismemberedLimbNode(BGSBodyPart::LIMB_ENUM aeLimb, NiNode* apNode) override;
    NiNode* GetDamageNode(BGSBodyPart::LIMB_ENUM aeIndex) override;
    NiAVObject* GetHeadNode() override {
        return pHeadNode;
    }
    NiAVObject* GetTorsoNode() override;
    bool IsSustainedFire() override;
    void SetSustainedFire(bool abvalue) override;
    void UpdateArmorWorn(Actor* apActor) override;
    bool IsHeavyBodyArmorWorn() override;
    bool IsPowerBodyArmorWorn() override;
    bool IsPowerArmorHelmetWorn() override;
    bool HasBackPackWorn() override;
    void TryReserveProjectileClones(Actor* apActor) override;
    void SetAnimActionSuccess(int32_t aivalue) override;
    void DeferHideDismemberedLimb(
        BGSBodyPart::LIMB_ENUM aeLimbIndex,
        NiNode* apDismemberedLimbRoot,
        NiAVObject* apReplacementLimb,
        bool abExplosion
    ) override;
    void UpdateDeferredHideDismemberedLimbs(Actor* apActor, float afDeltaTime) override;
    void SetCreatureLipSynchAnim(LipSynchAnim* apCreatureLipsynchAnim) override;
    LipSynchAnim* GetCreatureLipSynchAnim() override;

    void SetCreatureLipSynchStartTime(uint32_t aiCreatureLipsynchStartTime) override {
        iCreatureLipsynchStartTime = aiCreatureLipsynchStartTime;
    }

    uint32_t GetCreatureLipSynchStartTime() override {
        return iCreatureLipsynchStartTime;
    }
    void AddRadiationMagicDelta(float afValue) override;
    void RemoveRadiationMagicDelta(float afValue) override;
    float GetRadiationMagicDelta() override;
    void SetRadiationWaterDelta(float afValue) override;
    float GetRadiationWaterDelta() override;
    void SetRadiationDelta(float afValue) override;
    float GetRadiationDelta() override;
    HitData* GetLastHitData() override;
    void SetLastHitData(HitData* apData) override;
    void ClearLastHitData() override;
    HitData* GetLastAttackHitData() override;
    void SetLastAttackHitData(HitData* apData) override;
    void ClearLastAttackHitData() override;
    int32_t GetWeaponConditionStage() override;
    void SetWeaponConditionStage(int32_t aiStage) override;
    BSFaceGenNiNode* GetFaceNode() override {
        return pFaceNode;
    }
    void SetFaceNode(BSFaceGenNiNode* apNode) override;
    BSFaceGenNiNode* GetFaceSkinnedNode() override {
        return pFaceNodeSkinned;
    }
    void SetFaceSkinnedNode(BSFaceGenNiNode* apNode) override {
        pFaceNodeSkinned = apNode;
    }
    NiAVObject* GetHeadAnims() override;
    void SetHeadAnims(NiAVObject* apObj) override {
        pAnimFace = apObj;
    }
    void CheckCachedValueFlags(Actor* apActor) override;
    void ProcessEscort(Actor* apActor) override;
    void ProcessSleep(Actor* apActor) override;
    void ProcessEat(Actor* apActor, TESObjectREFR* apLocRef) override;
    void ProcessCombat(Actor* apActor) override;
    void ProcessAlarm(Actor* apActor) override;
    bool ProcessActivate(Actor* apActor, bool abInterateindex) override;
    void ProcessAcquire(Actor* apActor) override;
    void ProcessFleeNonCombat(Actor* apActor) override;
    void ProcessSandBox(Actor* apActor) override;
    void ProcessMovementBlocked(Actor* apActor) override;
    void ProcessPatrol(Actor* apActor) override;
    void ProcessNotify(Actor* apActor) override;
    bool CheckIfHasObject(Actor* apActor, int32_t ainumb) override;
    TESObjectREFR* GetTargetForPackage(Actor* apActor, TESPackage* apPackage) override;
    TESObjectREFR* GetLocationForPackage(Actor* apActor, TESPackage* apPackage, bool bSecondLoc) override;
    void EvaluateOrderAcquireList(Actor* apActor) override;
    bool ProcessCombatOneHour(Actor* apActor, float afTimeDelta) override;
    virtual void SetAnimation(Animation* apAnim);
    // start pathfinding to a point
    virtual bool
    SetupPathFinding(Actor* apActor, NiPoint3 aDestPoint, TESObjectCELL* apDestIntCell, TESWorldSpace* apDestWorld);
    // set the animation active flag
    virtual void SetAnimationActiveFlag(bool) {}
    virtual void ResetTarget();
    // run the buy object procedure
    virtual void ProcessBuyObject(Actor* apActor);
    // run the steal procedure
    virtual void ProcessSteal(Actor* apActor);
    // run the pickpocket procedure
    virtual void ProcessPickPocket(Actor* apActor);
    // run the dialogue procedure
    virtual void ProcessDialogue(MobileObject* apActor);
    // run the dialogue activate procedure
    virtual bool ProcessDialogueActivate(Actor* apActor);
    // run the use furniture procedure
    virtual bool ProcessUseFurniture(Actor* apActor, TESObjectREFR* apFurniture);
    // run the remove worn procedure
    virtual void ProcessRemoveWorn(Actor* apActor, bool bInterateindex, bool bOnlyifClothes);
    // run the wander procedure
    virtual void ProcessWander(Actor*, const NiPoint3*, float) {}
    virtual bool ProcessSandBoxDialogue(Actor*, TESObjectREFR*) {
        return true;
    }
    // start the patrol package
    virtual bool ProcessPatrolInit(Actor* apActor, PatrolActorPackageData* apData);
    virtual void ProcessPatrolStartTravelToNextRef(Actor* apActor, PatrolActorPackageData* apData);
    virtual void ProcessPatrolTravelingToNextRef(Actor* apActor, PatrolActorPackageData* apData);
    virtual void ProcessPatrolStartAtRefBehavior(Actor* apActor, PatrolActorPackageData* apData);
    virtual void ProcessPatrolAtRef(Actor* apActor, PatrolActorPackageData* apData);
    // run the use idle marker procedure
    virtual void ProcessUseIdleMarker(Actor*, TESObjectREFR*) {}
    // run one hour of accompany while not in high
    virtual void ProcessAccompanyOneHour(Actor* apActor);
    using MiddleLowProcess::ProcessRemoveWorn;

    BSAnimGroupSequence* GetAimSequence(AIM_ENUM aeDireciton);
    void SetDetectLifeDetected(bool abDetected);
    bool GetDetectLifeDetected();
    void SetWeaponFlags();
    // Saves the character controller's state into apSaveGameBuffer.
    void SaveCharController(BGSSaveFormBuffer* apSaveGameBuffer);
    // Restores the character controller's saved state from apLoadGameBuffer.
    void LoadCharController(BGSLoadFormBuffer* apLoadGameBuffer);
    // Restores the character controller's saved state for pmob from the old-format save.
    void LoadCharController(MobileObject* pmob);
    // Saves the character controller's state into the old-format save data: the true and wanted
    // states, the listener flags, the position when the collision was disabled, the velocities and the fall and pitch
    // values.
    void SaveCharController(MobileObject* pmob);

    bool ReloadDynamicIdleOn1stPerson(Actor* apActor);
    // Empty: does nothing.
    void ForceBoneScale();
    void ClearReservation();
    // Clears the damage root nodes, the head node and the torso node.
    void ClearDamageNodes();
    void MiddleHighCopy(MiddleHighProcess* apProcess);
    void ReparentWeapon(bool abDrawn, BipedAnim* apBiped, Animation* apAnim, Actor* apActor);
    void ClearFurnitureMarker();
    uint16_t GetCharControllerSaveSize(MobileObject* pmob);
    bool ProcessEscortOneHour(Actor* apActor);
    bool ProcessEatOneHour(Actor* apActor);
    bool ProcessActivateOneHour(Actor* apActor);
    void AlignToFurnitureAndAddPostAnimSitSleep(Actor* apActor);
    bool ShouldPickPackIdle(Actor* apActor);
    void PickPackageIdles(Actor* apActor, bool abForceIdle);

protected:
    bool ProcessFollowOneHour(Actor* apActor, float afDelta, bool abInterateIndex);
    uint8_t DidPathingFail(Actor* apActor);

public:
    TESObjectREFR* SetTargetData(Actor* apActor, TESPackage* apPackage, PackageTarget* ptarg);
    void FindBedChairs(Actor* apActor, bool abSit, TESObjectREFR* apRefLoc);

    static bool EvaluateReferenceforSitSleep(TESObjectREFR* apRef, void* aiFilterData);

    // 0xC8 actors detected as dead
    BSSimpleList<void*> DeadDetectList;
    // 0xD0 chairs and beds the actor knows about
    BSSimpleList<void*> RefListChairBed;
    // 0xD8 time until the actor stops pursuing
    float fPursueTimer;
    // 0xDC weight of the equipment the actor wears
    float fEquippedWeight;
    // 0xE0 the idle of the package was played once
    bool bDoneOnce;
    // 0xE1 the actor aims at its target
    bool bAimingTarget;
    // 0xE2 the weapon has to be attached to the other node
    bool bNeedReparentWeapon;
    // 0xE3 the actor has a weapon drawn
    bool bDrawn;
    // 0xE4 the package that runs once, over the current one
    ActorPackage RunOncePackage;
    // 0xFC where the target was last seen
    NiPoint3 lastSeenPostion;
    // 0x108 the kind of item the package uses
    PACKAGE_OBJECT_TYPE eUseItem;
    // 0x10C the idle played last
    TESIdleForm* pLastIdlePlayed;
    // 0x110 restarts the package idle timer
    bool bPickPackIdle;
    // 0x114 the weapon the actor holds
    ItemChange* pWeapon;
    // 0x118 the ammo the actor uses
    ItemChange* pAmmo;
    // 0x11C preloaded weapon model
    NiPointer<IOTask> spWeaponPreload;
    // 0x120 reserved projectile clones
    NiPointer<BSCloneReserver> spReserveProjectileClones;
    // 0x124 the weapon is a one hand grenade
    bool bWeaponGrenade;
    // 0x125 the weapon is a one hand mine
    bool bWeaponMine;
    // 0x126 the weapon is a one hand thrown weapon
    bool bWeaponThrown;
    // 0x127 the actor wears heavy armor
    bool bHeavyArmorWorn;
    // 0x128 the actor wears a power armor torso
    bool bPowerArmorWorn;
    // 0x129 the actor wears a power armor helmet
    bool bPowerArmorHelmet;
    // 0x12A the actor wears a backpack
    bool bHasBackPackWorn;
    // 0x12C node the weapon is attached to
    NiNode* pWeaponBone;
    // 0x130 node the projectiles come out of
    NiAVObject* pFireNode;
    // 0x134 the actor was told to draw the weapon
    bool bWantWeaponDrawn;
    // 0x135 the weapon is drawn
    bool bWeaponDrawn;
    // 0x138 the character controller of the actor
    NiPointer<bhkCharacterController> spCharController;
    // 0x13C the knocked state; sign extended by GetKnockState
    int8_t cKnockState;
    // 0x13D the sit/sleep state
    uint8_t cSitSleepState;
    // 0x140 the furniture the actor uses
    TESObjectREFR* pCurrentFurniture;
    // 0x144 the marker of the furniture the actor uses
    uint8_t cCurrentFurnitureIndex;
    // 0x148 where on the furniture the actor sits or sleeps
    FurnitureMark FurnitureMarker;
    // 0x158 the actor that commands this one
    Actor* pCommandingActor;
    // 0x15C the weapon that was bound last
    TESObjectWEAP* pLastBoundWeapon;
    // 0x160 the spell the actor casts
    MagicItem* pCurrentSpell;
    // 0x164 the spell a package makes the actor cast
    MagicItem* pCurrentPackageSpell;
    // 0x168 the magic node has to be checked
    bool bCheckMagicNode;
    // 0x16C the shader of the enchantment of the weapon
    TESEffectShader* pCurrentWeaponEffect;
    // 0x170 alpha multiplier of the actor
    float fAlphaMult;
    // 0x174 refraction power a script set
    float fScriptRefractPower;
    // 0x178 face animation data of the actor, cached
    BSFaceGenAnimationData* m_pFaceAnimationData;
    // 0x17C the magic shader effects have to be refreshed
    bool bRefreshMagicShaderEffects;
    // 0x17D the hit effect has to be initialized
    bool bRefreshMagicShaderEffectBody;
    // 0x17E the weapon effect shader has to be updated
    bool bRefreshMagicShaderEffectWeapon;
    // 0x17F the loading effect has to be refreshed
    uint8_t bRefreshMagicShaderEffectLoading;
    // 0x180 the flare flags have to be refreshed
    bool bRefreshFlareFlags;
    // 0x184 the flare flags
    uint32_t uiFlareFlags;
    // 0x188 the actor was pickpocketed
    bool bPickPocketed;
    // 0x189 the actor changed its clothes
    bool bDoneClothesChange;
    // 0x18A life detection found the actor
    bool bDetectLifeDetected;
    // 0x18B the actor is a summoned creature
    bool bSummonedCreature;
    // 0x18C the 3D update flags, one bit each
    uint8_t cUpdate3DModel;
    // 0x18D the next update runs regardless of the timers
    bool bForceNextUpdate;
    // 0x190 number of the idle the package plays
    uint32_t iPackageIdleNumber;
    // 0x194 the idle the actor plays
    TESIdleForm* pCurrentIdle;
    // 0x198 time until the package idle repeats
    float fPackageIdleTimer;
    // 0x19C the begin idles of the package were played
    bool bPlayedBeginIdles;
    // 0x19D the end idles of the package were played
    bool bPlayedEndIdles;
    // 0x1A0 the lip sync animation of a creature
    LipSynchAnim* pCreatureLipsynchAnim;
    // 0x1A4 when the lip sync of a creature started
    uint32_t iCreatureLipsynchStartTime;
    // 0x1A8 the reservation slot of the actor
    uint32_t iReservationSlot;
    // 0x1AC arrows stuck in the actor
    BSSimpleList<ArrowProjectile*>* pAttachedArrowList;
    // 0x1B0 limbs waiting to be hidden
    BSSimpleList<DeferredHideLimb*> DeferredHideLimbList;
    // 0x1B8 the active magic effects
    BSSimpleList<ActiveEffect*>* pActiveEffectList;
    // 0x1BC the target of the spell the actor casts
    MagicTarget* pDesiredTarget;
    // 0x1C0 the animation of the actor
    Animation* pAnimation;
    // 0x1C4 sequences the aim animations use
    BSAnimGroupSequence* pAimSequence[3];
    // 0x1D0 angle the aim follows the look target at
    float fAimLooking;
    // 0x1D4 delay between the shots of an automatic weapon
    float fAutomaticShotDelay;
    // 0x1D8 shots left to fire
    uint8_t cShotsToFire;
    // 0x1D9 the actor fires an automatic weapon
    bool bSustainedFire;
    // 0x1DA the actor was attacked
    bool bBeenAttacked;
    // 0x1DC the limb root nodes of the actor, damage is applied to
    NiNode* pDamageRootNode[15];
    // 0x218 the head node
    NiAVObject* pHeadNode;
    // 0x21C the torso node
    NiAVObject* pTorsoNode;
    // 0x220 the lighting property of the actor
    NiPointer<BSShaderPPLightingProperty> pLightingProperty;
    // 0x224 the bound of the actor
    BSBound* pBSBound;
    // 0x228 the actor aims down the sights
    bool bIronSights;
    // 0x229
    uint8_t byte229;
    // 0x22A the result of the animation action
    uint16_t iAnimActionSuccess;
    // 0x22C the movement animations
    uint32_t iMovementAnimations;
    // 0x230 items waiting to be equipped or unequipped
    BSSimpleList<QueuedItem*>* pListItemstoEquipUnequip;
    // 0x234 radiation of the area
    float fRadiationDelta;
    // 0x238 radiation from magic effects
    float fRadiationMagicDelta;
    // 0x23C radiation from the water
    float fRadiationWaterDelta;
    // 0x240 the last hit the actor took
    HitData* pLastHitData;
    // 0x244 condition stage of the weapon
    uint32_t iWeaponConditionStage;
    // 0x248 the facegen node of the actor
    BSFaceGenNiNode* pFaceNode;
    // 0x24C the skinned facegen node
    BSFaceGenNiNode* pFaceNodeSkinned;
    // 0x250 the face animation shape
    NiAVObject* pAnimFace;
    // 0x254 the last hit the actor dealt
    HitData* pLastAttackHitData;
    // 0x258 when the chair and bed list was last scanned
    uint32_t nFurnitureListTimer;

    // The weapon bone of the player's third-person 3D.
    static inline NiNode* pThirdPersonPlayerWeaponNode;
};

// The game settings MiddleHighProcess.cpp defines.
extern SettingT<INISettingCollection> fAimChaseLookingMult;
extern SettingT<INISettingCollection> fAimUpDegrees;
extern SettingT<INISettingCollection> fAimDownDegrees;
extern BSSimpleList<TESObjectREFR*> MiddleHighTempRefListChairBed;

extern BSPerformanceTimer AIProcessing_MiddleHighUpdate;
