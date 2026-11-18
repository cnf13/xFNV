#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class Actor;
class ActorCause;
class Animation;
class BGSEncounterZone;
class BGSHavokSaveData;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BGSWorldLocation;
class bhkNiCollisionObject;
class bhkSimpleShapePhantom;
class BipedAnim;
class BSAnimNoteReceiver;
class BSFaceGenNiNode;
class BSMultiBoundRoom;
struct FurnitureMark;
class HavokSaveData;
class MagicCaster;
class MagicTarget;
class MobileObject;
class MultiBoundMarkerData;
class NiControllerManager;
class NiGeometry;
class NiLight;
class NiPointLight;
class Script;
class ScriptLocals;
class TargetEntry;
class TESActorBase;
class TESAmmo;
class TESContainer;
class TESGlobal;
class TESIdleForm;
class TESObjectCELL;
class TESObjectWEAP;
class TESSound;
class TESTopic;
class TESTopicInfo;
class TESWorldSpace;
class TrapEntry;
class WaterZone;

enum ANIM_GROUP_WEAPON_ENUM : uint32_t;
enum RADIO_RANGE_TYPE : uint32_t;

enum SIT_SLEEP_STATE : uint32_t {
    SIT_SLEEP_NORMAL = 0x0,
    SIT_SLEEP_LOAD_SIT_IDLE = 0x1,
    SIT_SLEEP_WANT_TO_SIT = 0x2,
    SIT_SLEEP_WAITING_FOR_SIT_ANIM = 0x3,
    SIT_SLEEP_IS_SITTING = 0x4,
    SIT_SLEEP_WANT_TO_STAND = 0x5,
    SIT_SLEEP_LOAD_SLEEP_IDLE = 0x6,
    SIT_SLEEP_WANT_TO_SLEEP = 0x7,
    SIT_SLEEP_WAITING_FOR_SLEEP_ANIM = 0x8,
    SIT_SLEEP_IS_SLEEPING = 0x9,
    SIT_SLEEP_WANT_TO_WAKE = 0xA,
    SIT_SLEEP_COUNT = 0xB,
};

// The bits of LOADED_REF_DATA::iFlags.
enum LOADED_DATA_FLAGS : uint32_t {
    LOADED_DATA_HAVOK_INITED = 0x1,
    LOADED_DATA_HAS_ADDON_FLAGS = 0x2,
    LOADED_DATA_PRIMITIVE = 0x4,
};

// The runtime state of a reference that has a 3D representation.
struct LOADED_REF_DATA {
    LOADED_REF_DATA();
    ~LOADED_REF_DATA();

    void SetFlag(uint32_t aiFlag, bool abValue);
    bool GetFlag(uint32_t aiFlag);

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    TESObjectREFR* pCurrentWaterObject;
    uint32_t iUnderwaterCount;
    float fRelevantWaterHeight;
    float fCachedRadius;
    // bit set tested by TESObjectREFR::GetLoadedDataFlag
    uint32_t iFlags;
    NiPointer<NiNode> m_spData3D;
    NiPointer<bhkSimpleShapePhantom> spPhantom;
};

// A placed instance of a base object in the world.
class TESObjectREFR : public TESForm, public TESChildCell {
public:
    // The base object, placement and orientation of the reference.
    struct ReferenceData {
        TESBoundObject* pObjectReference;
        NiPoint3 kRotation;
        NiPoint3 kPosition;
    };

    TESObjectREFR();
    ~TESObjectREFR() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    bool SavesBefore(TESForm* apForm) override;
    bool SavesBefore(FORM* apGroupFORM) override;
    TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
    void PostDuplicateProcess(NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
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
    void InitItem() override;
    ENUM_FORM_ID GetSavedFormType() override;
    void GetFormDetailedString(BSStringT<char>& aString) override;
    bool GetOnLocalMap() override;
    void SetOnLocalMap(bool) override {}
    void SetDelete(bool abDeleted) override;
    void SetAltered(bool altered) override;
    void SetQuestObject(bool) override {}
    bool IsReference() override {
        return true;
    }
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) override;
    void CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) override;
    const char* GetFormEditorID() override;

    // Overrides TESChildCell's only virtual function.
    TESObjectCELL* GetSaveParentCell() override;

    virtual bool GetEditorLocation(
        NiPoint3& arOriginalLocation,
        NiPoint3& arOriginalAngle,
        TESForm*& arpLocationForm,
        TESForm* apLocationForm
    );
    // The seventh flag, abForceSubtitles, exists only in the PC build.
    virtual BSSoundHandle VoiceSoundFunction(
        TESTopic* apTopic,
        Actor* apTarget,
        bool abPlayerhead,
        bool abAlwaysPlay,
        bool abForceCorrectDuration,
        bool abSayTo,
        bool abForceSubtitles
    );
    virtual void UpdateSoundCallBack();
    virtual void DamageObject(float afDamage, bool abSelfDamage);
    virtual bool GetCastShadows();
    virtual void SetCastShadows(bool abVal);
    virtual bool GetMotionBlur();
    virtual void SetMotionBlur(bool abVal);
    // Not named GetDangerous/GetObstacle/GetQuestObject because those names would override TESForm's virtual functions.
    virtual bool IsDangerous();
    virtual bool IsObstacle();
    virtual bool IsQuestObject();
    virtual void SetActorCause(ActorCause* apActorCause);
    virtual ActorCause* GetActorCause();
    virtual NiPoint3 GetStartingAngle();
    virtual NiPoint3 GetStartingLocation();
    virtual void SetStartingPosition(NiPoint3 aposition);
    virtual void UpdateRefLight();
    virtual TESObjectREFR* RemoveItem(
        TESBoundObject* apObject,
        ExtraDataList* apExtra,
        int32_t aiNumber,
        bool abStealing,
        bool abDropWorld,
        TESObjectREFR* apContainerObj,
        const NiPoint3* apPoint,
        const NiPoint3* apRotate,
        bool abDelete,
        bool abPreferStolen
    );
    virtual void RemoveItemType(ENUM_FORM_ID aeType, bool abStealing, int32_t aiNumber);
    virtual bool AddWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra, bool abForceEquip);
    virtual bool RemoveWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra);
    virtual void DoTrap(TrapEntry*, TargetEntry*) {}
    virtual void AddObjecttoContainer(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aicount);
    virtual NiPoint3 GetLookingAtLocation();
    virtual MagicCaster* GetMagicCaster();
    virtual MagicTarget* GetMagicTarget();
    virtual bool IsChild(bool) {
        return false;
    }
    virtual TESActorBase* GetTemplateActorBase() {
        return nullptr;
    }
    virtual void SetTemplateActorBase(TESActorBase*) {}
    virtual BSFaceGenNiNode* GetFaceNodeBiped(NiNode* apHeadNode);
    virtual BSFaceGenNiNode* GetFaceNodeSkinned(NiNode* apHeadNode);
    virtual BSFaceGenNiNode* GetFaceNode(NiNode* apHeadNode);
    virtual BSFaceGenAnimationData* GetFaceAnimationData(NiNode* apHeadNode);
    virtual bool ClampToGround();
    virtual bool DetachHavok();
    virtual void InitHavok();
    virtual NiAVObject* Load3D(bool abBackgroundLoading);
    // replaces the reference's 3D with apObject
    virtual void Set3D(NiAVObject* apObject, bool abTestProcessLevel);
    virtual NiNode* Get3D();
    virtual bool HasRandomAnim();
    virtual NiPoint3 GetBoundMin();
    virtual NiPoint3 GetBoundMax();
    virtual void UpdateAnimation();
    virtual Animation* GetAnimation();
    virtual BipedAnim* GetBiped() {
        return nullptr;
    }
    virtual BipedAnim* GetCurrentBiped();
    virtual void SetBiped(BipedAnim*) {}
    virtual const NiPoint3& GetLocationOnReference() {
        return data.kPosition;
    }
    virtual void SetRunsInLow(bool) {}
    virtual void MoveHavok(bool abForceRec);
    virtual void SetActionComplete(bool) {}
    virtual void SetMovementComplete(bool) {}
    virtual void ResetInventory(bool abLeveledOnly);
    virtual NiAVObject* GetFireNode() {
        return nullptr;
    }
    virtual void SetFireNode(NiAVObject*) {}
    virtual SIT_SLEEP_STATE GetSitSleepState() {
        return SIT_SLEEP_NORMAL;
    }
    virtual bool IsNpc() {
        return false;
    }
    virtual bool IsCreature() {
        return false;
    }
    virtual bool IsExplosion() {
        return false;
    }
    virtual bool IsProjectile();
    virtual void SetParentCell(TESObjectCELL* apCell);
    // the flag goes as a 4-byte value
    virtual bool IsDead(bool bNotEssential);
    virtual bool IsKnockedOut() {
        return false;
    }
    virtual bool IsParalyzed() {
        return false;
    }
    virtual BSAnimNoteReceiver* CreateAnimNoteReceiver();
    virtual BSAnimNoteReceiver* GetAnimNoteReceiever();

    // Whether the reference can use the idle apIdle.
    bool CanUseIdle(TESIdleForm* apIdle);
    // Gathers the havok state to save into arData.
    void GatherHavokSaveData(BGSHavokSaveData* apHavokSaveData);
    // The number of items the reference holds.
    int32_t GetInventoryCount(bool abBarter, bool abPlayableCount);
    // Loads the reference's animation state.
    void LoadAnimation(BGSLoadGameBuffer* apLoadGameBuffer);
    // Loads the reference's havok state from apLoadGameBuffer, or warns when it has no 3D or its body count changed.
    void LoadHavokDataBGS(BGSLoadGameBuffer* apLoadGameBuffer);
    // Saves the reference's havok flags, body count and each body's state into apSaveGameBuffer.
    void SaveHavokData(BGSSaveFormBuffer* apSaveGameBuffer);
    // Writes each rigid body's state, gathered into apHavokSaveData by GetHavokDataSaveSize,
    // into the old save.
    void SaveHavokData(HavokSaveData* apHavokSaveData);
    // Shows or hides the link lines of the reference, its linked
    // reference and its linked children, recursively; apVisited (a local map when null) stops cycles.
    // KEEP: for a reference not yet in apVisited, BSMapBase::GetAt (0x57C850) leaves its bool output unset and the
    // exe passes that uninitialised stack byte to SetAt.
    void SetLinkLineRecurse(bool abShow, BSMap<TESObjectREFR*, bool>* apVisited);
    // Loads the reference's havok state from aiSaveSize bytes of the save buffer.
    void LoadHavokData(uint16_t aiSaveSize);
    void EndSequence(NiControllerSequence* apSequence);
    // Whether marker aiMarkerIndex is used or, unless abIgnoreReservations, reserved.
    bool GetMarkerUsed(uint32_t aiMarkerIndex, bool abIgnoreReservations);
    // Activates the reference for apActionRef.
    bool Activate(TESObjectREFR* apActionRef, bool abIdFlag, TESBoundObject* apObjecttoGet, int32_t aiCount);
    ExtraDataList* GetExtra();
    // The light of a spell effect on the reference, from its extra data.
    LightRefData* GetSpellEffectLight();
    // Saves the 3D's controller manager animation into the old save system.
    void SaveAttachedAnimation();
    // The bytes the attached animation takes in a save.
    uint16_t GetAttachedAnimationSaveSize();
    // The bytes the Havok data takes in a save; fills apHavokSaveData (a local record when null) with the bone
    // counts and flags.
    uint16_t GetHavokDataSaveSize(HavokSaveData* apHavokSaveData);
    // Saves the last finished sequence and the 3D's controller manager animation.
    void SaveAnimation(BGSSaveFormBuffer* apSaveGameBuffer);
    // Flags the reference deleted and altered, then drops its 3D.
    void MarkAsDeleted();
    // The script attached through the reference's extra data.
    Script* GetScriptPointer();
    TESBoundObject* GetObjectReference();
    // The actor's Health actor value, else the reference's or base object's health; -1 without any.
    float GetHealth();
    // Returns the base object, rotation and position (data).
    ReferenceData* GetData();
    TESObjectCELL* GetParentCell();
    const NiPoint3& GetAngleOnReference();
    // Writes the map name of the reference's location into aOutMapName.
    void GetMapNameForLocation(BSStringT<char>& aOutMapName);
    BSExtraData* GetExtraData(uint8_t aType);

    bool ShouldPlaySound();
    void SetTargeted(bool abVal);
    void SetHasTemp3D(bool abVal);
    bool HasTemp3D();
    bool GetPersistFlagValue();
    bool GetDroppedRef();
    bool GetVisibleDistant();
    // Sets the 3D without the bookkeeping Set3D does: obstacles, room and portal extras, then Havok.
    void Set3DSimple(NiAVObject* ap3D);
    bool GetCenterOnCreation();
    bool IsFurniture();
    bool Is3DCritical();
    void SetAngleOnReferenceX(float afx);
    float GetRelevantWaterHeight();

    ScriptLocals* GetScriptVariables();
    TESForm* GetMyOwner();
    // Sets the owner (CHANGE_REFR_EXTRA_OWNERSHIP); a linked door loses its owner.
    void SetOwner(TESForm* apOwner);
    // Sets the ownership global; a linked door loses its global.
    void SetGlobal(TESGlobal* apGlobal);
    // Sets the ownership rank; a linked door's rank is reset to -1.
    void SetRank(int32_t aiRank);
    DoorTeleportData* GetTeleport();
    // Adds a teleport extra to the reference and returns its data.
    DoorTeleportData* AddTeleport();
    // Removes the reference's teleport extra.
    void RemoveTeleport();
    // Removes the reference's ownership extra.
    void RemoveOwner();
    // Shows or hides the reference's linked-reference line.
    void SetLinkLine(bool abShow);
    // Shows or hides the reference's activate-parent lines.
    void SetActivateLine(bool abShow);
    // Shows or hides the activate-parent lines of the reference and
    // of its activate children, recursively.
    void SetActivateLineRecurse(bool abShow);
    // Fits the attached multibound or room to the reference's bound and transform.
    void FixAttachedBSObjectSizeAndTransform();
    AudioMarker* GetExtraAudioMarker();
    LockData* GetMyLock();
    TESForm* GetEmittanceSource();
    TESObjectREFR* GetLinkedRef();
    TESObjectREFR* GetEnableStateParent();
    bool GetEnableStateInverse();
    BSSimpleList<TESObjectREFR*>* GetEnableStateChildList();
    LightRefData* GetLight();
    void SetLight(NiLight* pLight);
    bool GetAction(OBJECT_ACTION type);
    bool IsActivatingChildren();
    // the world space or cell recorded as the package start location, or null if it is the other kind
    TESWorldSpace* GetPackageStartLocationWorld();
    TESObjectCELL* GetPackageStartLocationInteriorCell();
    // The coordinates recorded as the package start location, else the reference's own position.
    const NiPoint3& GetPackageStartLocationCoord();
    TESObjectREFR* GetRadioStationPositionRef();
    float GetRadioStationRadius();
    RADIO_RANGE_TYPE GetRadioStationRangeType();

    const char* GetFullName();
    // The kind of crosshair prompt the reference gets.
    int32_t GetRefPromptType();
    // the model path of the reference, null when it has no model
    const char* GetModel();
    bool IsMarker();
    // Whether the reference's 3D loads first: visible from a distance, or a non-actor, non-marker object larger
    // than 700 units.
    bool Is3DHighPriority();
    void SetIsInvisibleLODMultType(bool abIsInvisibleLODMultType);
    bool GetRefPersists();
    float GetScale();
    // Sets the reference's scale.
    void SetScale(float newscale);
    float GetModelBoundSize();
    float GetDistanceFromPoint(const NiPoint3& aPt);
    // The cached radius, else the light's or the radiation, map or audio marker's radius (0 for other bases).
    float GetRadius();
    // The distance to apRef; the highest float when apRef is null, deleted, disabled (unless abDisabled) or, unless
    // bSameSpace, in another interior cell or worldspace.
    float GetDistanceFromReference(TESObjectREFR* apRef, bool abDisabled, bool bSameSpace);
    bool HasLeveledCreatureCalced();
    bool GetIsImposter();
    bool GetIsStingerLOD();
    bool GetIsInvisibleLODMultType();
    // The room the reference's multibound belongs to, or null.
    BSMultiBoundRoom* GetMultiBoundRoom();
    // The rotation matrix of the reference's angle; actors turn only about z.
    NiMatrix3 GetOrientation();

    bool IsMultiBoundRef();
    TESObjectREFR* GetMultiBoundRef();
    MultiBoundMarkerData* GetMultiBoundData();
    // The primitive's radii (after syncing the multibound data to them), else the data's or the default half extents.
    const NiPoint3& GetMultiBoundHalfExtent();
    // Whether the 3D's world bound lies inside apBound.
    bool QBoundInsideMultibound(BSMultiBound* apBound);
    // Whether the reference's position lies inside apBound.
    bool QCenterInsideMultibound(BSMultiBound* apBound);
    // Fits the multibound shapes below apObject to the reference's bound; whether it changed
    // any.
    bool UpdateMultiboundShapeTransformsRecursive(NiAVObject* apObject);
    // Precomputes the axis-aligned bounds of the tri strips shapes below ap3D.
    void PreCalcTriStripsShapeAabb(NiAVObject* ap3D);
    void SetMultiBoundRef(TESObjectREFR* apRef);
    TESObjectREFR* GetXTarget();
    LockData* GetLock();
    // The lock-pick difficulty of the reference's lock (LockData::GetLockLevel for this
    // reference), 0 without a lock.
    int32_t GetLockLevel();
    ExtraLock* GetExtraLock();
    void Lock();
    void UnLock();

    // the world space of the cell, else the cell itself (persistent cells only give their world space)
    TESForm* GetSpace();
    TESWorldSpace* GetWorldSpace();
    MapMarkerData* GetMapMarkerData();
    bool GetInterior();
    BGSEncounterZone* GetEncounterZone();
    void SetEncounterZone(BGSEncounterZone* apZone);

    void CopyExtraDataList(ExtraDataList* apCopy);
    void SetActionRef(TESObjectREFR* apRefIn);
    void SetAction(OBJECT_ACTION type);
    void ClearAction(OBJECT_ACTION type);

    // GetLocationOnReference's own code (&data.kPosition), for non-virtual calls of it.
    NiPoint3* QLocationOnReference() {
        return &data.kPosition;
    }
    void SetLocationOnReferenceSimple(const NiPoint3& arPoint);
    // Sets the reference's rotation about Y.
    void SetAngleOnReferenceY(float afy);
    void SetAngleOnReferenceZ(float afz);
    void SetAngleOnReference(NiPoint3 aPoint);
    void SetObjectReference(TESBoundObject* apObject);

    void SetLoadedDataFlag(uint32_t aiFlag, bool abValue);
    bool GetLoadedDataFlag(uint32_t aiFlag);
    bool QUnderwater();
    // Sets the underwater flag of the loaded data to abVal.
    void SetUnderwater(bool abVal);

    int32_t GetInventoryObjectCount(TESBoundObject* apObject);
    bool HasQuestObject();
    bool HasScript();
    bool WearingObject(TESBoundObject* apObject);
    ItemChange* GetInventoryItem(TESBoundObject* apObject, uint32_t aId);

    // original calls
    TESContainer* HasContainer();
    TESModel* GetTESModel();
    // The owner, else the teleport door's, encounter zone's or cell's owner.
    TESForm* GetOwner();
    bool GetOriginalOpenByDefaultState();
    void AddLockChange();
    // Releases what the reference built around its 3D before the 3D goes.
    void Release3DRelatedData();
    // Frees the furniture markers the reference reserves.
    void FreeUpAllMarkers();
    // Whether the reference has a finished sequence or its controller manager plays one.
    bool IsPlayingAnimation();
    // TESObjectREFR's own Get3D, called non-virtually: the background loading thread's 3D for the reference it is
    // loading, else the loaded 3D.
    NiNode* Get3D_Body();
    // Queues or updates the reference's 3D after a load or enable.
    void UpdateReference3D();
    // Attaches the weapon's 3D to the reference's.
    void AttachWeapon(TESObjectWEAP* apWeapon);
    bool IsCrimeToActivate();
    void AttachAmmo(TESAmmo* apAmmo);
    // Its body is empty.
    void RemoveAmmo();
    // Detaches the weapon's 3D from the reference's.
    void RemoveWeapon();
    // Whether pOwner owns the reference (it or its template is the owner), or belongs to the owning faction when
    // busefaction or the faction allows selling; an unowned reference is everyone's.
    bool IsAnOwner(Actor* pOwner, bool busefaction);
    // The first furniture marker that is enabled and free (and, unless abIgnoreReservations, not reserved), -1 when
    // none is; a non-furniture base has only marker 0.
    int32_t GetFirstFreeMarkerIndex(bool abIgnoreReservations);
    bool HasFreeMarker(bool abIgnoreReservations);
    // The free furniture marker (enabled, not used and, unless abIgnoreReservations, not reserved) of an allowed kind
    // that is closest to aLocation: copied to aOutMarker, its index to aiOutMarkerIndex. A furniture reference
    // without 3D, or any other reference unless abIgnoreReservations, gets its own position and index -1.
    bool GetClosestFreeMarker(
        const NiPoint3& aLocation,
        bool abSitOK,
        bool abSleepOK,
        FurnitureMark& aOutMarker,
        uint32_t& aiOutMarkerIndex,
        bool abIgnoreReservations
    );
    // Builds the animation of the reference's 3D (see the body).
    void InitAnimation();
    // Marks furniture marker aiMarkerIndex used or free, adding the extra when needed and dropping it once empty.
    void SetMarkerUsed(uint32_t aiMarkerIndex, bool abVal);
    // Detaches every BGSDecalNode child of the reference's 3D and clears its slot (ApplyAllDecals calls it first).
    void ClearAllDecals();
    // Clears the reference's decals, then places each of its decal refs again as a permanent simple decal.
    // abSetDecalData is unused.
    void ApplyAllDecals(bool abSetDecalData);
    // Reloads the models of an NPC or creature reference; for other references the base object replaces its own.
    bool ReplaceModel();
    // Reserves or frees furniture marker aiMarkerIndex.
    void SetMarkerReserved(uint32_t aiMarkerIndex, bool abVal);
    // Copies furniture marker aiIndex into aOutMarker; false when there is none.
    bool GetMarkerAtIndex(int32_t aiIndex, FurnitureMark& aOutMarker);
    // The topic the reference says to its target; marks change flag 0x80000000.
    void SetSayToTopic(TESTopic* apTopic);
    // The info the reference says to its target; marks change flag 0x80000000.
    void SetSayToTopicInfo(TESTopicInfo* apInfo);
    // The response the reference says to its target; marks change flag 0x80000000.
    void SetSayToResponseID(uint32_t anResponseID);
    // Whether the reference must persist (hardcoded markers, radios, doors, low-level actors...).
    bool MustRefPersist();
    void Enable();
    // Disables the reference.
    void Disable();
    // Replaces the reference's animation; returns the new one.
    Animation* SetAnimation(Animation* apAnim);
    // The KF files of the reference's animations for the weapon type.
    BSSimpleList<const char*>* BuildKFFileList(ANIM_GROUP_WEAPON_ENUM aeWeapType);
    void SetRefPersists(bool abVal);
    bool MustBeVisibleDistant();
    void SetVisibleDistant(bool abVal);
    // The level the reference's leveled contents are calculated for.
    uint32_t GetCalcLevel(bool abAllowShift);
    void SetParentCellSimple(TESObjectCELL* apCell);
    // False when following the enable state parents from this reference loops.
    bool CheckEnableParentLoop();
    bool CanHaveSound();
    void AttachSound(bool abAttach);
    // Updates the sounds of the reference's addon nodes when its 3D can have them.
    void UpdateAddonNodeSounds(bool abDetachAll);
    void UpdateAddonNodeSoundsRecursive(NiAVObject* apRoot, bool abDetachAll);
    void AddLight(bool abMagic);
    void RemoveLight(bool abMagic);
    // Runs the reference's object script and its inventory's; true when one ran.
    bool RunScript();
    bool UpdateChildActivates();
    // the node named apSearch under apRoot, searching the root's own name too
    NiAVObject* GetActorBone(NiAVObject* apRoot, const char* apSearch);
    NiAVObject* GetActorBone(NiAVObject* apRoot, NiFixedString& apSearch);
    // The rotation scaled by the base NPC's race (compiled out: a copy of apRotIn).
    NiMatrix3 MultipleMatrixByRace(NiMatrix3& apRotIn);
    // gives the reference its base object's script and locals, and inits its inventory's scripts
    void InitScript();
    TESGlobal* GetOwnershipGlobal();
    int32_t GetOwnershipRank();
    // whether the reference's light or 3D takes its emittance from outside
    bool CheckUsesExternalEmittance();
    // propagates this reference's disabled state to its enable state children, recursively
    void UpdateEnableStateChildrenFlagsRecursive();
    void
    SetPackageStartLocation(TESWorldSpace* apWorld, TESObjectCELL* apIntCell, const NiPoint3& aPt, float afzrotation);
    void SetLeveledCreature(TESActorBase* apOriginalBase, TESActorBase* apTemplate);
    // swaps a created leveled creature base back to the base it was calculated from
    void RestoreLeveledCreatureOriginalBase();
    // Releases the loaded data's 3D and phantom and deletes it.
    void FreeLoadedData();
    // drops the reference's light (or its magic light) from the world scene and its extra data
    void KillLight(bool abMagic);
    // Removes the reference from every water zone and water group it is in; abTraverseAllZones goes through the
    // cell's water listener instead of the reference's own zone map.
    void RemoveFromAllWater(bool abTraverseAllZones);
    // Whether the base is an actor base made by or using a leveled template.
    bool IsLeveledCreature();
    // Whether the container or actor base respawns; never for leveled creatures.
    bool GetRespawn();
    // stores the 3D in the loaded data, or frees the loaded data when ap3D is null
    void Set3DVerySimple(NiAVObject* ap3D);
    // false for statics (other than markers), static collections, trees, flora and furniture not created in game
    bool CanBeMoved();
    // moves the reference; while a save loads, or for a position far outside the world, it goes back to its
    // editor location instead
    void SetLocationOnReference(const NiPoint3& arPoint);
    bool MoveToEditorLocation(TESForm* apLocationForm);
    // Moves the 3D to the reference's position.
    void Update3DPosition();
    // Allocates pLoadedData when it is null.
    void CreateLoadedData();
    // Releases the reference's trap listeners.
    void CleanUpTraps();
    // Sets or clears the dropped flag.
    void SetDroppedRef(bool abVal);
    // Build the Havok bodies of a placeable water, a collision marker, an acoustic space and a primitive trigger.
    void InitHavokForPlaceableWater();
    void InitHavokForCollisionObject();
    void InitHavokForAcousticSpace();
    void InitHavokForPrimitiveTrigger();
    // Compiled out: returns false.
    bool CheckAndAddHavokChangeFlag(NiAVObject* apRoot);
    // Applies the rag doll pose (pragdata, else the stored one) to the 3D; without 3D, stores a copy of pragdata.
    void RestoreRagDollData(RagDollData* pragdata);
    // Records apSequenceName as the last finished sequence, unless a save is loading and one is recorded already.
    void SetLastFinishedSequence(const char* apSequenceName);
    bool GetIsOfType(TESBoundObject* apObj);
    void SetCenterOnCreation(bool abVal);
    bool GetTargeted();
    BGSEncounterZone* GetLocationEncounterZone();
    void FreeMultiBound();
    TESObjectREFR* GetRandomTeleportMarker();
    void SetLinkedRef(TESObjectREFR* apRef);
    TESObjectREFR* GetMerchantContainer();
    void SetMerchantContainer(TESObjectREFR* apRef);
    void SetEnableStateParent(TESObjectREFR* apParent);
    void SetEnableStateInverse(bool abVal);
    float GetRadioStaticPct();
    uint32_t GetActionFlag();
    TESObjectREFR* GetActionRef();
    bool HasObjectId(uint32_t aId);
    bool GetInaccessible();
    bool GetIsHiddenDoor();
    bool IsCellLoaded();
    TESTopic* GetSayToTopic();
    BSMultiBound* GetMultiBound();
    void SetRandomTeleportMarker(TESObjectREFR* apMarker);
    void MarkAsPickedUp();
    // AddReferenceItem(apObject, aiNumber, abFlag, abWearit).
    void AddReferenceItem(TESObjectREFR* apObject, int32_t aiNumber, bool abFlag, bool abWearit);
    // Whether the owner (apOwner, else the reference's own) is evil.
    bool IsOwnerEvil(TESForm* apOwner);
    void AddBoundObjectItem(TESBoundObject* apObject, ExtraDataList* apExtra, int32_t aiNumber);
    void SetLocationOnReferenceZ(float afZ);
    int32_t GetGoldCount();
    int32_t GetCount();
    uint32_t GetSayToResponseID();
    TESTopicInfo* GetSayToTopicInfo();
    TESObjectREFR* GetRelevantWaterType();
    void SetRelevantWaterType(TESObjectREFR* a2);
    // Whether the requested limb is dismembered; aeLimb is passed on unchanged to ExtraDismemberedLimbs::Dismembered.
    bool GetDismembered(BGSBodyPart::LIMB_ENUM aeLimb);
    // Resets the reference's animation.
    void ResetAnimation_Old();
    // Gets the reference's world location.
    BGSWorldLocation* GetWorldLocation(BGSWorldLocation* apLocation);
    // Shows the "LightOff" node of apObj when abStateOn is set and the "LightOn" node when it is not (it culls them
    // with the flag).
    void SetLight(NiAVObject* apObj, bool abStateOn);
    // The arrival position of the door this reference's door links to; the zero point without one.
    const NiPoint3& GetLinkedDoorTeleportPosition();
    // Marks limb aeLimb removed or not in the dismembered-limbs extra data, when there is one.
    void SetLimbRemoved(BGSBodyPart::LIMB_ENUM aeLimb, bool abRemoved);
    void SetOrientation(NiMatrix3 rot);
    void ClearInterpolators();
    bool GetVATStarget();
    MobileObject* AddTalkingActivatorMobileObjectREFR();
    float GetWeightInContainer(bool abIsHardcore);
    bool HasSaidTopicInfo(TESTopicInfo* apTopicInfo);
    bool SetSequencePosition(NiControllerManager* apManager, const char* apSequence, float afPosition);
    void SetSeed(uint32_t a1);
    bool HasRandomSound();
    void LoadHavokDataPreVersion81(uint16_t aiSaveSize);
    void ResetAnimation();
    void SetIsImposter(bool abIsImposter);
    const NiPoint3& GetLinkedDoorTeleportRotation();
    LockData* AddLock();
    uint32_t GetSeed();
    void InitChildActivates(TESObjectREFR* apActionRef);
    void AddDecalRef(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal);
    NiPoint3 GetDeltaForAngle(float afAngle);
    void SetSpellEffectLight(NiPointLight* pLight);
    void SetDismembered(
        BGSBodyPart::LIMB_ENUM aeLimb,
        TESForm* apDeathObject,
        CAUSE_OF_DEATH aeDeath,
        BGSBodyPart::LIMB_ENUM aeLastHitLimb,
        bool abExplodedLimb
    );
    // Returns CAUSE_OF_DEATH_NONE (-1) without a dismemberment extra; callers treat the result as signed.
    CAUSE_OF_DEATH GetCauseOfDeath();
    ItemChange* GetInventoryItem_005754A0(int32_t aiIndex, bool abBarter);
    // Fills *a3 with the inventory (the barter container when a1) and *a2 with a new iterator over it; the caller,
    // ListBox<ItemChange *>::BuildFromContainer, deletes the iterator with IterData's deleting destructor.
    void* GetInventoryIterator(char a1, InventoryChanges::IterData** a2, InventoryChanges** a3);
    void ShowPrimitive(bool abShow);
    MobileObject* GetMobileObjectExtra();
    void SetInZone(WaterZone* apZone, bool abValue);
    bool GetIgnoredBySandbox();
    // Greets the player with apGreetTopic: an actor speaks it with subtitles, another reference plays its voice.
    void SayForceGoodbyeGreetingToPlayer(TESTopic* apGreetTopic, bool abForceSubtitle, bool abForceHeadTrack);
    void SetHealth(float afVal);

    // Whether the vertices of apObject are inside apMulti; abFoundGeom is set when the object has geometry.
    static bool QVerticesInsideMultiboundRecurse(
        NiAVObject* apObject,
        BSMultiBound* apMulti,
        bool& abFoundGeom,
        bool abAllVertices
    );
    // Starts every animation controller under apObject at phase afPhase.
    static void RandomizeAddons(NiAVObject* apObj, float afPhase);
    // Saves the playing sequences of apManager at afTime into apSaveGameBuffer.
    static void
    SaveControllerManager(NiControllerManager* apManager, BGSSaveGameBuffer* apSaveGameBuffer, float afTime);
    // Saves the playing sequences of apManager at afTime into the old save system (-1 means the current game time).
    static void SaveControllerManager(NiControllerManager* apManager, float afTime);
    // Restores apManager's sequences from the old save at afTime (the cell time for -1): activates each saved
    // sequence found by name, catches apObj3D's particles up to afTime and updates it; an unknown sequence's data
    // is skipped.
    static void LoadControllerManager(NiControllerManager* apManager, NiAVObject* apObj3D, float afTime);
    // Records apDropper as the dropper of apDroppedItem, unless the dropper is the player or the item persists or
    // is a quest object.
    static void SetDroppedItemData(TESObjectREFR* apDropper, TESObjectREFR* apDroppedItem);
    // The say-to sound callback: apRef carries the speaker's form ID; marks its say-to voice done.
    static void SayToCallBack(void* apRef, bool abSucceeded);
    // Returns the reference whose 3D holds the object, or null.
    static TESObjectREFR* FindReferenceFor3D(NiAVObject* apAVObjectIn);
    // Whether apNode's BSXFlags mark it as an addon node.
    static bool HasAddonFlags(NiNode* apNode);
    // Stops the moving sounds attached under apNode; true when apNode itself had one.
    static bool RemoveSounds(NiNode* apNode);
    // Detaches the addon nodes under apNode; returns true when apNode itself had one.
    static bool RemoveAddonNodes(NiNode* apNode);
    // The LOD multiplier class of the reference that owns ap3D, by its base object's form type.
    static uint32_t GetLightingObjectType(NiAVObject* ap3D);
    // The reference the calling thread is loading in the background, or null.
    static TESObjectREFR* GetBackgroundLoadingRef();
    static uint16_t GetControllerManagerSaveSize(NiControllerManager* apManager);
    // Sets this thread's background-loading reference and its 3D.
    static void SetBackgroundLoadingRef(TESObjectREFR* apRef, NiAVObject* ap3D);
    // Moves apRef into the interior cell or the world space.
    static void MoveRefToNewSpace(TESObjectREFR* apRef, TESObjectCELL* apInterior, TESWorldSpace* apWorld);
    // A new reference of form type aeType (REFR_ID, ACHR_ID, the projectile types, ...), with a process when abadd.
    static TESObjectREFR* CreateReference(uint8_t aeType, bool abadd);
    static bool AddMasterParticleAddonNodes(NiNode* apNode);
    static bool RemoveMasterParticleAddonNodes(NiNode* apNode);
    // whether ap3D, one of its properties or any node below it has a time controller
    static bool HasTimeControllers(NiAVObject* ap3D);
    // bhkWorld's add handler: leaves out the clutter (or everything) the Havok settings say to skip
    static void FilteredAddObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // The bhkWorld walk handlers of the old Havok save: count the rigid bodies (active and inactive) into the
    // HavokSaveData in arData.uData[0]; write one body's state to the old save; read it back from a save older than
    // version 81.
    static void GetCollisionObjectSize(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    static void
    SaveHavokDataForCollisionObjectOld(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    static void
    LoadHavokDataForCollisionObjectPreVersion81(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // The calling thread's background loading reference and its 3D, held per thread.
    static TESObjectREFR*& GetThreadBackgroundLoadingRef();
    static NiNode*& GetThreadBackgroundLoading3D();
    // Clears the calling thread's background loading reference, then its 3D.
    static void ClearBackgroundLoadingRef();
    // Attaches the add-on nodes under apNode.
    static bool AddAddonNodes(NiNode* apNode);
    static bool IsReferenceFormType(int32_t aeFormType);
    // Whether apNode has addon nodes.
    static bool HasAddonNodes(NiNode* apNode);
    static void SetTerminalUnlocked(TESObjectREFR* a1, char a2);
    static bool IsHacked(TESObjectREFR* a1);
    static bool IsLockedOut(TESObjectREFR* a1);
    static void LoadControllerManagerBGS(NiControllerManager* a1, BGSLoadGameBuffer* a2, float a3);
    // The DoObjectRec callback of LoadHavokData for one collision object.
    static void LoadHavokDataForCollisionObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // The DoObjectRec callback of SaveHavokData for one collision object: writes a rigid body's position and rotation
    // (not for the collision root), its flags byte when the save data's flags have bit 4, and its velocities when it
    // is active.
    static void SaveHavokDataForCollisionObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Removes apRef from TESObjectCELL::MultiboundTestMap, whose key
    // it is; Release3DRelatedData passes `this`.
    static char RemoveRefFromMultiboundTestMap(TESObjectREFR* apRef);
    static NiPoint3 GetBoundMax_0056FC30(NiAVObject* apObj);
    static void TransChangeCallback(bhkNiCollisionObject* apCollision, uint32_t aiChanged);
    static void DebugDisplayCallback(NiGeometry* ap3D, const NiColor& color);
    static void
    GatherHavokSaveDataForCollisionObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);

    TESSound* pRandomSound;
    ReferenceData data;
    float fRefScale;
    TESObjectCELL* pParentCell;
    ExtraDataList m_Extra;
    LOADED_REF_DATA* pLoadedData;

    // The ini preference that shows the editor primitives of the loaded references.
    static SettingT<INISettingCollection> bPrimitivesOn;
    static BSCriticalSection kFurnitureLock;
};

void UpdateLODMultInvisibility(TESObjectREFR* apRef, NiNode* apNode);
// hkQuaternion::operator=(q), defined in TESObjectREFR.cpp. It is a free function because a user-provided operator=
// would make hkQuaternion non-trivially copyable for every user.
void hkQuaternion_operator_assign(hkQuaternion* apThis, const hkQuaternion& q);
void TESObjectREFR_LoadHavokDataForCollisionObjectFunc(bhkNiCollisionObject* a1, bhkWorld::ObjectRecData& a2);

// The Havok state of a reference a save game records: its bone counts by motion type and the loading buffer.
class BGSHavokSaveData {
public:
    BGSHavokSaveData();

    uint8_t cFlags;
    uint8_t padding01[3];
    uint32_t iActiveBoneCount;
    uint32_t iInactiveBoneCount;
    uint32_t iKeyFramedBoneCount;
    TESObjectREFR* pRef;
    NiAVObject* pObj3D;
    bhkNiCollisionObject* pCollisionRoot;
    void* pSaveLoadBuffer;
};

// The older Havok save record of a reference: its flags, active and inactive bone counts and the reference, 3D and
// collision root it was gathered from.
class HavokSaveData {
public:
    HavokSaveData();

    uint8_t cFlags;
    uint16_t sActiveBoneCount;
    uint16_t sInactiveBoneCount;
    TESObjectREFR* pRef;
    NiAVObject* pObj3D;
    bhkNiCollisionObject* pCollisionRoot;
};

// The DATA chunk of a reference's form record: its position, then its rotation (0x18 bytes).
struct REFR_PLACEMENT_DATA {
    REFR_PLACEMENT_DATA();

    // Byte-swaps the six floats.
    void Endian();

    NiPoint3 kPosition;
    NiPoint3 kRotation;
};

// The marker base forms recognised by IsMultiBoundRef and GetXTarget.
inline TESForm* pBoundMarker;
inline TESForm* pRoomMarker;
inline TESForm* pXMarker;
inline TESForm* pXMarkerHeading;

// The game settings TESObjectREFR.cpp defines.
extern SettingT<GameSettingCollection> iCalcLevelAdjustUp;
extern SettingT<GameSettingCollection> iCalcLevelAdjustDown;
extern SettingT<INISettingCollection> bPreventHavokAddAll;
extern SettingT<INISettingCollection> bFileControllerOnRoot;
extern SettingT<INISettingCollection> bPreventHavokAddClutter;
inline bool bScriptActivate;
extern NiPoint3 TESObjectREFR_boundMax;
