#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Actor;
class ActorCause;
class ActorPackageData;
class AlchemyItem;
class Animation;
class AudioBuoyMarkerData;
class AudioMarker;
class BGSAcousticSpace;
class BGSEncounterZone;
class BGSLoadFormBuffer;
class BGSMusicType;
class BGSPrimitive;
class BGSSaveFormBuffer;
class BGSSaveGameBuffer;
class BGSTalkingActivator;
class bhkWorld;
class BSExtraData;
class BSMultiBound;
class BSMultiBoundRoom;
class BSOcclusionPlane;
class BSPortal;
class CellMopp;
struct CollisionData;
class Crime;
class DoorTeleportData;
class ExtraAction;
class ExtraActorCause;
class ExtraAmmo;
class ExtraAnim;
class ExtraAnimSave;
class ExtraAudioBuoyMarker;
class ExtraAudioMarker;
class ExtraDismemberedLimbs;
class ExtraFactionChanges;
class ExtraFollower;
class ExtraFollowerSwimBreadcrumbs;
class ExtraLeveledItem;
class ExtraLight;
class ExtraLock;
class ExtraMapMarker;
class ExtraNavMeshPortal;
class ExtraRefractionProperty;
class ExtraSayTopicInfoOnceADay;
class ExtraSayToTopicInfo;
class ExtraStartingPosition;
class ExtraTalkingActor;
class ExtraTeleport;
class ImpactSwap;
class InventoryChanges;
class LightRefData;
class LockData;
class MapMarkerData;
class MenuTopic;
class MultiBoundMarkerData;
class NiNode;
class NiRenderedTexture;
struct OcclusionPlaneLinkedRefData;
struct PatrolRefData;
struct PortalLinkedRefData;
struct RADIO_DATA;
class RagDollData;
struct REF_ACTIVATE_DATA;
struct REF_REFLECTED_DATA;
struct REF_REFLECTOR_DATA;
struct RoomRefData;
class SayOnceTopicInfos;
class Script;
class ScriptLocals;
class SeenData;
class TESActorBase;
class TESAmmo;
class TESBoundObject;
class TESClimate;
class TESCombatStyle;
class TESFile;
class TESForm;
class TESGlobal;
class TESImageSpace;
class TESModel;
class TESObjectCELL;
class TESObjectREFR;
class TESPackage;
class TESRegionList;
class TESTopic;
class TESTopicInfo;
class TESWaterForm;
class TESWorldSpace;
class TrespassPackage;
class WaterZone;
struct WORLD_LOCATION;

template <class TKEY, class TVAL>
class NiTMap;

// The calling thread's id (GetCurrentThreadId).
uint32_t BSGetCurrentThreadID();

using D3DLOCKED_RECT = struct _D3DLOCKED_RECT;

// The action bits a reference's ExtraAction holds.
enum OBJECT_ACTION : int32_t {
    OBJECT_ACTION_USE_DEFAULT = 0x1,
    OBJECT_ACTION_ACTIVATE = 0x2,
    OBJECT_ACTION_OPEN = 0x4,
    OBJECT_ACTION_OPEN_BY_DEFAULT = 0x8,
};

// One decal placed on a reference: the decal reference, the hit point and the surface normal.
struct REF_DECAL_DATA {
    REF_DECAL_DATA();

    // Swaps the record before and after it is saved byte-swapped.
    void Endian();

    TESObjectREFR* pDecalRef;
    NiPoint3 Intersect;
    NiPoint3 Normal;
};

// An occlusion plane's or a portal's shape (XOCP, XPTL): half extents, centre, rotation axis and angle.
struct OCCLUSION_PLANE_DATA {
    OCCLUSION_PLANE_DATA();

    // Byte-swaps the nine floats read from a big-endian file.
    void Endian();

    NiPoint2 rHalfExtents;
    NiPoint3 rCenter;
    NiPoint3 rAxis;
    float fAngle;
};

// The kinds of data an ExtraDataList holds; the value is the bit index in the list's presence bitfield.
enum EXTRA_DATA_TYPE : uint8_t {
    EXTRA_NONE = 0,
    EXTRA_HAVOK = 1,
    EXTRA_CELL3D = 2,
    EXTRA_WATERTYPE = 3,
    EXTRA_REGIONLIST = 4,
    EXTRA_SEENDATA = 5,
    EXTRA_EDITOR_ID = 6,
    EXTRA_CELLMUSICTYPE = 7,
    EXTRA_CLIMATE = 8,
    EXTRA_CELL_MID_LOW_PROCESS = 9,
    EXTRA_CELL_CANOPYSHADOW_MASK = 10,
    EXTRA_CELLDETACHTIME = 11,
    EXTRA_PERSISTENT_CELL = 12,
    EXTRA_SCRIPT = 13,
    EXTRA_ACTION = 14,
    EXTRA_STARTINGPOSITION = 15,
    EXTRA_ANIM = 16,
    EXTRA_BIPED = 17,
    EXTRA_USEDMARKERS = 18,
    EXTRA_DISTANTDATA = 19,
    EXTRA_RAGDOLLDATA = 20,
    EXTRA_CONTAINER_CHANGES = 21,
    EXTRA_WORN = 22,
    EXTRA_WORN_LEFT = 23,
    EXTRA_PACKAGESTARTLOC = 24,
    EXTRA_PACKAGE = 25,
    EXTRA_TRESPASS_PACKAGE = 26,
    EXTRA_RUN_ONCE_PACKAGES = 27,
    EXTRA_REFERENCE_POINTER = 28,
    EXTRA_FOLLOWER = 29,
    EXTRA_LEVCREA_MOD = 30,
    EXTRA_GHOST = 31,
    EXTRA_ORIGINAL_REFERENCE = 32,
    EXTRA_OWNERSHIP = 33,
    EXTRA_GLOBAL = 34,
    EXTRA_RANK = 35,
    EXTRA_COUNT = 36,
    EXTRA_HEALTH = 37,
    EXTRA_USES = 38,
    EXTRA_TIMELEFT = 39,
    EXTRA_CHARGE = 40,
    EXTRA_LIGHT = 41,
    EXTRA_LOCK = 42,
    EXTRA_TELEPORT = 43,
    EXTRA_MAPMARKER = 44,
    EXTRA_ANIM_SAVE = 45,
    EXTRA_LEVELEDCREATURE = 46,
    EXTRA_LEVELITEM = 47,
    EXTRA_SCALE = 48,
    EXTRA_SEED = 49,
    EXTRA_MAGICCASTER = 50,
    EXTRA_MAGICTARGET = 51,
    EXTRA_MASTERFILECELL = 52,
    EXTRA_PLAYERCRIMELIST = 53,
    EXTRA_UNUSED = 54,
    EXTRA_ENABLESTATEPARENT = 55,
    EXTRA_ENABLESTATECHILDREN = 56,
    EXTRA_ITEMDROPPER = 57,
    EXTRA_DROPPEDITEMLIST = 58,
    EXTRA_TELEPORTMARKER = 59,
    EXTRA_MERCHANTCONTAINER = 60,
    EXTRA_SAVED_HAVOK_DATA = 61,
    EXTRA_CANNOTWEAR = 62,
    EXTRA_POISON = 63,
    EXTRA_MAGIC_LIGHT = 64,
    EXTRA_LASTFINISHEDSEQUENCE = 65,
    EXTRA_SAVED_ANIMATION = 66,
    EXTRA_NORTHROTATION = 67,
    EXTRA_XTARGET = 68,
    EXTRA_FRIEND_HITS = 69,
    EXTRA_HEAD_TRACK_TARGET = 70,
    EXTRA_BOUND_ARMOR = 71,
    EXTRA_REFRACTION_PROPERTY = 72,
    EXTRA_STARTINGWORLDORCELL = 73,
    EXTRA_HOT_KEY = 74,
    EXTRA_EDITOR_REF3D_DATA = 75,
    EXTRA_EDITOR_REFMOVE_DATA = 76,
    EXTRA_INFO_GENERAL_TOPIC = 77,
    EXTRA_NO_RUMORS = 78,
    EXTRA_SOUND = 79,
    EXTRA_TERMINALSTATE = 80,
    EXTRA_LINKED_REF = 81,
    EXTRA_LINKED_REF_CHILDREN = 82,
    EXTRA_ACTIVATE_REF = 83,
    EXTRA_ACTIVATE_REF_CHILDREN = 84,
    EXTRA_TALKING_ACTOR = 85,
    EXTRA_OBJECT_HEALTH = 86,
    EXTRA_DECAL_REFS = 87,
    EXTRA_DECAL_TARGETS = 88,
    EXTRA_IMAGESPACE = 89,
    EXTRA_NAVMESH_PORTAL = 90,
    EXTRA_MODEL_SWAP = 91,
    EXTRA_RADIUS = 92,
    EXTRA_RADIATION = 93,
    EXTRA_FACTION_CHANGES = 94,
    EXTRA_DISMEMBERED_LIMBS = 95,
    EXTRA_ACTOR_CAUSE = 96,
    EXTRA_MULTIBOUND = 97,
    EXTRA_MULTIBOUND_DATA = 98,
    EXTRA_MULTIBOUND_REF = 99,
    EXTRA_CELLENVIRONMENT_TYPE = 100,
    EXTRA_REFLECTED_REFS = 101,
    EXTRA_REFLECTOR_REFS = 102,
    EXTRA_EMITTANCE_SOURCE = 103,
    EXTRA_RADIO_DATA = 104,
    EXTRA_COMBAT_STYLE = 105,
    EXTRA_LINK_COLOR_PAIR = 106,
    EXTRA_PRIMITIVE = 107,
    EXTRA_OPENCLOSEACTIVATE_REF = 108,
    EXTRA_ANIMNOTE_RECEIVER = 109,
    EXTRA_AMMO = 110,
    EXTRA_PATROL_REF_DATA = 111,
    EXTRA_PACKAGE_DATA = 112,
    EXTRA_OCCLUSION_PLANE = 113,
    EXTRA_COLLISION_DATA = 114,
    EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO = 115,
    EXTRA_ENCOUNTERZONE = 116,
    EXTRA_SAY_TO_TOPIC_INFO = 117,
    EXTRA_OCCLUSION_PLANE_REF_DATA = 118,
    EXTRA_PORTAL_REF_DATA = 119,
    EXTRA_PORTAL = 120,
    EXTRA_ROOM = 121,
    EXTRA_HEALTH_PERC = 122,
    EXTRA_ROOM_REF_DATA = 123,
    EXTRA_GUARDED_REF_DATA = 124,
    EXTRA_CREATURE_AWAKE_SOUND = 125,
    EXTRA_WATER_ZONE_MAP = 126,
    EXTRA_UNUSED2 = 127,
    EXTRA_IGNORED_BY_SANDBOX = 128,
    EXTRA_CELL_ACOUSTIC_SPACE = 129,
    EXTRA_RESERVEDMARKERS = 130,
    EXTRA_WEAPON_IDLE_SOUND = 131,
    EXTRA_WATER_LIGHT_REFS = 132,
    EXTRA_LIT_WATER_REFS = 133,
    EXTRA_WEAPON_ATTACK_SOUND = 134,
    EXTRA_ACTIVATE_LOOP_SOUND = 135,
    EXTRA_PATROL_REF_IN_USE_DATA = 136,
    EXTRA_ASHPILE_REF = 137,
    EXTRA_CREATURE_MOVEMENT_SOUND = 138,
    EXTRA_FOLLOWER_SWIM_BREADCRUMBS = 139,
    EXTRA_IMPACTSWAP = 140,
    EXTRA_WEAPON_MOD_SLOTS = 141,
    EXTRA_WEAPON_IS_MODDING = 142,
    EXTRA_SECURITRON_FACE = 143,
    EXTRA_AUDIOMARKER = 144,
    EXTRA_AUDIOBUOYMARKER = 145,
    EXTRA_SPECIAL_RENDER_FLAGS = 146,
    EXTRA_DATATYPECOUNT = 147,
};

// One item of extra data on a reference, cell or inventory item; a node of a BaseExtraList's singly linked list.
class BSExtraData {
public:
    BSExtraData(uint8_t acType);
    virtual ~BSExtraData();

    // true if apCompare differs from this one (null or another type counts as different)
    virtual bool Compare(BSExtraData* apCompare);

    uint8_t GetExtraType() const;
    BSExtraData* GetNext();
    void SetNext(BSExtraData* apNext);

    static void operator delete(void* apMemory);

    uint8_t cEtype;
    uint8_t padding05[3];
    BSExtraData* pNext;
};

// A linked list of BSExtraData plus a bitfield of the types present, for quick tests.
class BaseExtraList {
public:
    BaseExtraList();
    virtual ~BaseExtraList();

    // Removes the SayTopicInfoOnceADay extra data, if any.
    void RemoveExtra();
    // The number of extra data items in the list, counted under the extra data lock.
    uint32_t ItemsInList();
    // true if no extra data is attached
    bool IsExtraListEmpty() const;
    bool HasExtra(uint8_t acType) const;
    void SetHasExtra(uint8_t acType, bool abVal);
    BSExtraData* GetExtraData(uint8_t acType);
    void RemoveExtra(uint8_t aeCode);
    // unlinks apExtra from the list, destroying it when abDelete is set
    void RemoveExtra(BSExtraData* apExtra, bool abDelete);
    BSExtraData* GetHead();
    // Unlinks every item, destroying them when abDelete is set, and clears the presence bits.
    void RemoveAll(bool abDelete);
    // The variant of RemoveAll that TESObjectREFR::ClearData uses.
    void RemoveAllDefault(bool abDelete);
    // Links apExtra in (high-use types at the head, the rest at the tail) and returns it.
    BSExtraData* AddExtra(BSExtraData* apExtra);
    // The item before the first one of type aeCode: null if that is the head, the tail if there is none.
    BSExtraData* GetPrevExtraData(uint8_t acType);
    void RemoveCount();

    static void operator delete(void* apMemory);

protected:
    // The per-thread 'last extra' cache: the list it belongs to and the last item found per type.
    static BSExtraData* GetLastExtra(BaseExtraList* apList, int32_t aeType);
    static void SetLastExtra(BaseExtraList* apList, BSExtraData* apExtra);
    static void ClearLastExtra(BaseExtraList* apList, int32_t aeType);
    static void ClearLastExtraAll(BaseExtraList* apList);
    // Drops this thread's cache when another thread has removed an item since it was filled.
    static void CheckDirty();
    // true for the types AddExtra links at the head of the list
    static bool IsHighUseExtra(int32_t aeType);

    // The thread-local statics pLastExtraList, iThreadDirty and ppLastExtraData (an array of EXTRA_DATATYPECOUNT
    // pointers).
    static BaseExtraList*& GetThreadLastExtraList();
    static int32_t& GetThreadDirty();
    static BSExtraData** GetThreadLastExtraData();

public:
    BSExtraData* pHead;
    // One bit per EXTRA_DATA_TYPE, 168 types; codes beyond that are never present.
    uint8_t iFlags[21];
    uint8_t padding1D[3];

    // Guards every list's links and the 'last extra' caches.
    static BSSpinLock ExtraCritSection;
    // Bumped whenever an item is removed; a thread whose iThreadDirty differs drops its cache.
    static inline int32_t iDirty;
};

class ExtraDataList : public BaseExtraList {
public:
    ExtraDataList();
    ~ExtraDataList() override;

    // Resolves the list's form IDs after a load; apBaseObject is the item's base object.
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer, TESForm* apBaseObject);
    // Saves the list's extra data.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Resolves the list's form IDs after a TES-format load; the reference and its base object are passed along.
    void InitLoadGameTES(uint32_t aiFlags, uint32_t aiCurrentFlags, TESObjectREFR* apRef, TESBoundObject* apBaseObject);
    // Finishes the list's TES-format load for apRef.
    void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, TESObjectREFR* apRef);
    // Writes the list's extra data chunks into the form record being saved.
    void Save();
    // Writes the saveable extra data the change flags aiFlags select into the old save.
    void SaveGame(uint32_t aiFlags, TESObjectREFR* apRef);
    // Reads the extra data the change flags aiFlags select from the old save (TESSaveLoadGame), with the block
    // header and size checks when the save uses blocks.
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, TESObjectREFR* apRef);
    // Drops the extra data the change flags aiFlags select, back to its unchanged state.
    void Revert(uint32_t aiFlags, TESObjectREFR* apRef);
    // Replaces the room extra data's RoomRefData.
    void SetRoomLinkedRefData(RoomRefData* apData);
    // Sets the starting worldspace or cell, or removes the extra data when null.
    void SetStartingWorldOrCell(TESForm* apWorldOrCell);
    // Removes the package extra data.
    void RemovePackageExtra();
    // Replaces the occlusion plane extra data's linked references.
    void SetOcclusionPlaneLinkedRefData(OcclusionPlaneLinkedRefData* apData);
    // The master room reference, found through the linked rooms; apVisited holds the rooms
    // already searched.
    TESObjectREFR* GetMasterRoomRecurse(NiTMap<TESObjectREFR*, bool>* apRoomsTouchedMap);
    // Replaces the portal extra data's linked references.
    void SetPortalLinkedRefData(PortalLinkedRefData* apLinkedRefData);
    // Sets or clears the reflection bit of apRef in the reflected refs.
    void SetReflectedByRef(TESObjectREFR* apRef, bool bValue);
    // Adds or removes apRef in the lit water refs.
    void SetLitWaterRef(TESObjectREFR* apRef, bool bValue);
    // Sets the lock extra data's lock, freeing the old one.
    ExtraLock* SetLockPtr(LockData* apLock);
    // Records a decal on apRef at apOrigin in direction apDirection.
    void AddDecalRef(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal);
    // Sets the spell effect light extra data's light.
    ExtraLight* SetSpellEffectLightPtr(LightRefData* apLight);
    // The starting rotation of apRef's starting position extra data (class result).
    NiPoint3 GetStartingAngle(TESObjectREFR* apObject);
    // The starting location of apRef's starting position extra data (class result).
    NiPoint3 GetStartingLocation(TESObjectREFR* apObject);
    // The friend hit count.
    int32_t GetFriendHitCount();
    // Stores apData in the seen data extra, adding or removing it.
    void SetSeenData(SeenData* apData);
    // Whether the water zone map counts apZone as entered (a positive count).
    bool GetInWaterZone(WaterZone* apZone);
    // The first water zone in the water zone map, or null.
    WaterZone* GetFirstWaterZone();
    // Whether apUser may use the patrol reference.
    bool GetPatrolRefCanBeUsedBy(const TESObjectREFR* apUser);
    // The terminal state's lock level; -1 without that extra data.
    char GetTerminalLockState();
    // Sets the terminal lock override; 0 clears it and removes the extra once the terminal is locked.
    void SetTerminalLockState(char acLockState);
    // Sets the terminal state; 0 clears it and removes the extra once the terminal is locked.
    void SetTerminalState(uint8_t acState);
    // Sets the combat style, or removes the extra data when null.
    void SetCombatStyle(TESCombatStyle* apCombatStyle);
    // Marks the patrol reference as in use by apUser; a null user removes the mark.
    void SetPatrolRefInUse(TESObjectREFR* apUser);
    void SetGhost(bool abGhost);
    void SetOpenCloseActivateRef(TESObjectREFR* apRef);
    // A null reference removes the extra.
    void SetAshPileRef(TESObjectREFR* apRef);
    void SetHotKey(char acValue);
    void SetRunOnceExtraPackage(TESPackage* apPack, char acDay);
    uint32_t GetExtraSayToResponseID();
    void RemoveSayToInfoExtra();
    TESTopicInfo* GetExtraSayToTopicInfo();
    bool HasFollower(Actor* apActor);
    // Drops apItem from the dropped item list extra; the extra goes when the list empties.
    void RemoveDroppedItem(TESObjectREFR* apItem);
    // Drops apChild from the linked ref children extra; the extra goes when the list empties.
    void RemoveLinkedRefChild(TESObjectREFR* apChild);
    bool HasLeveledItem();
    bool HasLevCreaCalced();
    bool GetGhost();
    bool GetCanNotWear();
    void RemoveContainerChanges();
    void RemoveScript();
    void RemoveLastFinishedSequence();
    void RemoveLeveledItem();

    // the action bits kept in the ExtraAction; 1 when there is none
    uint32_t GetActionFlag();
    bool GetAction(OBJECT_ACTION aeType);
    void SetAction(OBJECT_ACTION aeType);
    void ClearAction(OBJECT_ACTION aeType);

    ExtraAnimSave* GetAnimSave();
    ExtraFactionChanges* GetFactionChangesExtra();
    ExtraDismemberedLimbs* GetDismembermentExtra();
    ExtraAmmo* GetAmmoExtra();
    ExtraSayToTopicInfo* GetSayToExtra();
    ExtraSayTopicInfoOnceADay* GetExtraSayTopicInfoOnceADayExtra();
    ExtraLeveledItem* GetLeveledItem();
    ExtraRefractionProperty* GetRefractionPropertyExtra();

    Animation* GetAnimation();
    // Stores apAnim in the reference's ExtraAnim, deleting the one it held, or adds a new
    // ExtraAnim holding it.
    ExtraAnim* SetAnimPtr(Animation* apAnim);
    // Removes the reference's ExtraAnim.
    void RemoveAnimPtr() {
        BaseExtraList::RemoveExtra(EXTRA_ANIM);
    }
    LightRefData* GetLight();
    LockData* GetLock();
    RADIO_DATA* GetRadioData();
    DoorTeleportData* GetTeleport();
    MapMarkerData* GetMapMarkerData();
    BGSMusicType* GetMusicType();
    TESImageSpace* GetImageSpace();
    TESObjectREFR* GetEnableStateParent();
    BSSimpleList<TESObjectREFR*>* GetEnableStateChildren();
    TESObjectREFR* GetLinkedRef();
    BSSimpleList<TESObjectREFR*>* GetLinkedRefChildren();
    BSSimpleList<REF_ACTIVATE_DATA*>* GetActivateParentList();
    BSSimpleList<REF_ACTIVATE_DATA*>* GetActivateRefChildren();
    BSSimpleList<REF_DECAL_DATA*>* GetDecalRefs();
    BGSPrimitive* GetPrimitive();
    TESActorBase* GetLevCreaOriginalBase();
    uint32_t GetDetachTime();
    BGSEncounterZone* GetEncounterZone();
    TESForm* GetEmittanceSource();
    TESObjectREFR* GetMultiBoundRef();
    // The portal references linked to the room marker, or null.
    BSSimpleList<TESObjectREFR*>* GetRoomLinkedPortalRefList();
    TESObjectREFR* GetPortalLinkedRoomRef(int32_t aiWhich);
    MultiBoundMarkerData* GetMultiBoundData();
    TESModel* GetModelSwap();
    TESCombatStyle* GetCombatStyle();
    InventoryChanges* GetContainerChanges();
    // The bytes the extra data takes in a save.
    uint16_t GetSaveSize(uint32_t aiFlags, TESObjectREFR* apRef);
    TESForm* GetOwner();
    TESGlobal* GetGlobal();
    ExtraFollower* GetFollowerExtra();
    Script* GetScript();
    ScriptLocals* GetScriptLocals();
    void SetScriptLocals(ScriptLocals* apScriptlocals);
    WORLD_LOCATION* GetPackageStartLocation();
    TESObjectREFR* GetReferencePointer();
    // The package of the list's package extra, or null without one.
    TESPackage* GetPackageExtra();
    // Whether the package extra's action is complete; false without one.
    bool GetPackageExtraComplete();
    BSSimpleList<Crime*>* GetPlayerCrimeList();
    TESObjectCELL* GetPersistentCell();
    RagDollData* GetRagDollData();
    // Stores a copy of apRagData as the rag doll extra.
    void SetRagDollDataFromCopy(RagDollData* apRagData);
    char* GetSavedHavokData();
    // Stores the saved Havok state buffer, or removes the extra when apBuffer is null.
    void SetSavedHavokData(char* apBuffer);
    // Stores the saved animation state buffer, or removes the extra when apBuffer is null.
    void SetSavedAnimation(char* apBuffer);
    // The object's health, or -1 without the extra.
    float GetObjectHealth();
    PatrolRefData* GetPatrolRefData();
    CollisionData* GetCollisionData();
    AudioMarker* GetAudioMarkerData();
    ImpactSwap* GetImpactSwap();
    BSSimpleList<TESObjectREFR*>* GetLitWaterRefs();
    BSSimpleList<TESObjectREFR*>* GetDroppedItemList();
    // Sets the reference that dropped this item; null removes the extra.
    void SetItemDropper(TESObjectREFR* apDropper);
    // Adds apItem to the list of items this reference dropped.
    void AddDroppedItem(TESObjectREFR* apItem);
    void RemoveDroppedItemList();
    TESObjectREFR* GetAshPileRef();
    // The first weapon reference in the dropped item list, or null.
    TESObjectREFR* GetDroppedWeapon();
    // null when there is no editor ID extra
    char* GetExtraEditorID();
    BSOcclusionPlane* GetOcclusionPlane();
    TESObjectREFR* GetXTarget();
    uint8_t GetWeaponModFlags();
    bool GetWeaponModSlotActive(uint8_t slot);
    // true when the left-hand worn extra is present; with abWornLeft clear, either worn extra counts
    bool GetWorn(bool abwornleft);
    // true when there is no room data, the room is flagged as master, or it has no portals
    bool GetRoomIsMaster();
    // outputs default to a multiplier of 1.0 and an all-below-force of 0
    void GetLevCreaMultiplier(float& afOutMult, int32_t& aiOutAllBelowForce);
    // The room marker's linked portals and rooms, or null.
    RoomRefData* GetRoomLinkedRefData();
    void RemoveNonPersistentCellData();
    BGSAcousticSpace* GetAcousticSpace();
    TESWaterForm* GetWaterType();
    int32_t GetCanopyShadowMask(NiRenderedTexture*& apOutTex, _D3DLOCKED_RECT*& apOutRect);
    SeenData* GetSeenData();
    TESActorBase* GetLevCreaTemplate();
    ExtraActorCause* GetActorCauseExtra();
    BSMultiBound* GetMultiBound();
    // A null bound removes the extra.
    void SetMultiBound(BSMultiBound* apBound);
    TESObjectREFR* GetRandomTeleportMarker();
    // Sets the random teleport marker; null removes the extra.
    void SetRandomTeleportMarker(TESObjectREFR* apMarker);
    TESObjectREFR* GetMerchantContainer();
    // Drops apChild from the enable state children extra; the extra goes when the list empties.
    void RemoveEnableStateChild(TESObjectREFR* apChild);
    // Drops apChild from the activate ref children extra.
    void RemoveActivateRefChild(TESObjectREFR* apChild);
    // The reflector refs extra's list, or null without the extra.
    BSSimpleList<REF_REFLECTOR_DATA*>* GetReflectorRefs();
    // Clears the trespass package extra: marks its package as created (deleting it from the save outside
    // loading) and removes the extra.
    void RemoveTrespassPackage();
    // Drops apRef's activate data from the activate ref extra; the extra goes when its list empties.
    void RemoveActivateParent(TESObjectREFR* apRef);
    // Drops apActor from the follower extra's list.
    void RemoveFollower(Actor* apActor);
    void SetEnableStateInverse(bool abVal);
    TESObjectREFR* GetItemDropper();
    TESObjectREFR* GetActionRef();
    ExtraTalkingActor* GetTalkingActorExtra();
    TESObjectREFR* SetInWaterZone(WaterZone* apZone, bool abValue);
    bool GetIgnoredBySandbox();
    TESTopic* GetExtraSayToTopic();
    void SetActionRef(TESObjectREFR* apRefIn);
    void SetEncounterZone(BGSEncounterZone* apZone);
    // replaces the copyable extras with copies of apCopy's
    void CopyList(ExtraDataList* apCopy);
    // Adds a copy of apCopy when it is copyable (a type-specific setter per extra type).
    void AddExtraCopy(BSExtraData* apCopy);
    // Copies the reference-relevant extras of apCopy into this list (the script, with cloned locals, unless
    // abremove moves it), removing them from apCopy when abremove is set.
    void CopyListForReference(ExtraDataList* apCopy, bool abremove);
    // false for the types a copy leaves out (havok, 3D, container changes, ...); true for a null extra
    bool IsCopyableExtra(BSExtraData* apExtra);
    // Holds apChanges in the container changes extra; a null apChanges does nothing.
    void SetContainerChanges(InventoryChanges* apChanges);
    // true when every extra is of a type kept on a default container instance (worn too with bwornignore)
    bool IsExtraDefaultforContainer(bool bwornignore);
    // Adds or removes the worn extra, or the left-hand one with abwearleft.
    void SetWorn(bool abWorn, bool abwearleft);
    // true when apCompare holds an extra this list lacks or holds differently; with abCompareScriptAndOwnerData
    // clear, any script or owner extra in apCompare counts as a difference
    bool CompareListForContainer(ExtraDataList* apCompare, bool abCompareScriptAndOwnerData, bool bThrown);
    bool CompareList(ExtraDataList* apCompare);
    // A count of 0 or 1 removes the count extra.
    void SetCount(int16_t aiCount);
    // Adds or removes the can-not-wear extra.
    void SetCanNotWear(bool abCantWear);
    void RemoveOwnership();
    void SetReferencePointer(TESObjectREFR* apRef);
    // The hotkey slot, -1 when there is no hotkey extra.
    char GetHotKey();
    void RemoveHotKey();
    // The climate extra's climate, or null.
    TESClimate* GetClimate();
    TESRegionList* GetRegionList();
    // Sets the region list extra; a different old list is destroyed.
    void SetRegionList(TESRegionList* apList);
    // Sets the script extra.
    void SetScript(Script* apScript);
    // Unlinks every copyable extra, destroying it when abdelete is set.
    void RemoveAllCopyableExtra(bool abdelete);
    // returns the written angle
    NiPoint3 SetStartingAngle(TESObjectREFR* apObject, NiPoint3 newAngle);
    NiPoint3 SetStartingLocation(TESObjectREFR* apObject, NiPoint3 newLocation);

    // -1 when there is no leveled item extra
    int32_t GetExtraLevelItemIndex();
    void SetLevelItemExtraFlag(bool bflag);
    bool IsEnableStateInverse();
    void SetEnableStateParentFlags(uint8_t acFlags);
    // 1.0 when there is no health extra
    float GetHealthPerc();
    // -1.0 when there is no health extra
    float GetHealth();
    float GetRadius();
    float GetActivateChildrenTimer();
    int32_t GetRank();
    int16_t GetCount();
    // Records apCell as the persistent cell of the reference; a null apCell removes the extra.
    void SetPersistentCell(TESObjectCELL* apCell);
    // Reads one extra-data chunk of a reference record from apFile.
    void Load(TESFile* apFile, TESForm* apOwner);
    // Sets the impact swap extra; null removes it.
    void SetImpactSwap(ImpactSwap* apImpactSwap);

    // drops the run-once packages not given on day acday, and those whose start hour plus duration reaches 21
    void CheckRemoveRunOnceDayPackage(char acday);
    bool IsInRunOnceDayPackageList(TESPackage* apPackage);
    // a null parent removes the extra
    void SetEnableStateParent(TESObjectREFR* apParent);
    void AddEnableStateChild(TESObjectREFR* apChild);
    void SetWaterType(TESWaterForm* apWater);
    void SetLinkedRef(TESObjectREFR* apRef);
    void AddLinkedRefChild(TESObjectREFR* apChild);
    void SetActivateTextOverride(const char* pTextOverride);
    void AddActivateRefChild(TESObjectREFR* apChild);
    // sets or clears the refraction bit of apRef in the reflector refs; an entry left with no bits is removed
    void SetRefractedByRef(TESObjectREFR* apRef, bool bValue);
    // sets or clears the refraction bit of apRef in the reflected refs; an entry left with no bits is removed
    void SetRefractsRef(TESObjectREFR* apRef, bool bValue);
    // Sets or clears the reflection bit of apRef in the reflected refs.
    void SetReflectsRef(TESObjectREFR* apRef, bool bValue);
    // Adds apRef to or removes it from the water light refs.
    void SetWaterLightRef(TESObjectREFR* apRef, bool bValue);
    void AddPrimitive(BGSPrimitive* apPrimitive);
    // replaces the patrol data extra; a null apPatrolRefData only removes it
    void SetPatrolRefData(PatrolRefData* apPatrolRefData);
    void SetMerchantContainer(TESObjectREFR* apRef);
    // aiVal is a LEV_CREA_MODIFIER; LCM_NONE removes the extra
    void SetLevCreaModifier(int32_t aiVal);
    void SetLeveledCreature(TESActorBase* apOriginalBase, TESActorBase* apTemplate);
    // 0 removes the extra
    void SetNorthRotation(float afVal);
    void SetXTarget(TESObjectREFR* apTarget);
    void SetEmittanceSource(TESForm* apSource);
    void SetMultiBoundRef(TESObjectREFR* apRef);
    // frees the marker data it replaces
    void SetMultiBoundData(MultiBoundMarkerData* apData);
    // 0 removes the extra
    void SetRadius(float afRadius);
    // 0 removes the extra
    void SetRadiation(float afRadiation);
    // the player is never added
    void AddFollower(Actor* apActor);
    void SetLastFinishedSequence(const char* apSequenceName);
    // The last finished sequence's name, or null.
    const char* GetLastFinishedSequence();
    // Adds a navmesh portal extra holding a copy of apPortal's portal.
    void SetNavMeshPortal(ExtraNavMeshPortal* apPortal);
    void AddFactionChangesExtra();
    void SetIgnoredBySandbox(bool abVal);

    // a null apData removes the extra
    void SetRadioData(RADIO_DATA* apData);
    // the light extra; it frees the light it replaces
    ExtraLight* SetLightPtr(LightRefData* apLight);
    // the teleport extra (still the removed one when a null apTeleport removes it); frees the data it replaces
    ExtraTeleport* SetTeleportPtr(DoorTeleportData* apTeleport);
    // the map marker extra, null when a null apMapData removes it; frees the data it replaces
    ExtraMapMarker* SetMapMarkerPtr(MapMarkerData* apMapData);
    // the audio marker extra, null when a null apAudioData removes it; frees the data it replaces
    ExtraAudioMarker* SetAudioMarkerPtr(AudioMarker* apAudioData);
    // a null owner removes the extra
    void SetOwnership(TESForm* apOwner);
    // Sets the ownership global.
    void SetGlobal(TESGlobal* apGlobal);
    // Sets the ownership rank.
    void SetRank(int32_t aiRank);
    // Sets or clears the cell 3D extra's node.
    void SetCell3D(NiNode* apObjectRoot);
    // Sets the canopy shadow mask's state and texture; a zero aeState (an
    // ExtraCellCanopyShadowMask::CELL_STATE) removes the extra, otherwise *apOutRect gets the extra's
    // D3DLOCKED_RECT.
    void SetCanopyShadowMask(int32_t aeState, NiRenderedTexture* apTex, _D3DLOCKED_RECT*& apOutRect);
    // Sets or clears the trespass package extra.
    void SetTrespassPackageExtra(TrespassPackage* apPack);
    // Adds apRef to the room ref extra's linked rooms.
    void AddRoomLinkedRoomRef(TESObjectREFR* apRef);
    // Sets or clears the collision data extra.
    void SetCollisionData(CollisionData* apCollisionData);
    // Adds apInfo to the said-once-a-day topic infos.
    void SetExtraSayTopicInfoOnceADayExtra(SayOnceTopicInfos* apInfo);
    // Sets or clears the audio buoy marker extra.
    ExtraAudioBuoyMarker* SetAudioBuoyMarkerPtr(AudioBuoyMarkerData* apAudioBuoyData);
    void SetHealth(float afHealth);
    // Each setter below updates the extra data when the list has it and otherwise adds a new one.
    void SetUses(uint8_t acUses);
    void SetTimeLeft(float afTimeLeft);
    void SetCharge(float afCharge);
    // 255 removes the extra.
    void SetSeedIndex(uint8_t aiSeed);
    void SetObjectHealth(float afHealth);
    // A null climate removes the extra.
    void SetClimate(TESClimate* apClimate);
    void SetNoRumors(bool abVal);
    // 0 removes the extra.
    void SetDetachTime(uint32_t aiTime);
    void SetOriginalReferenceExtra(TESObjectREFR* apRef);
    void SetInfoGeneralTopic(MenuTopic* apItem);
    void SetModelSwap(TESModel* apModelSwap, TESForm* apModelSwapForm);
    // 1.0 removes the extra.
    void SetScale(float afScale);
    void SetRefractionPropertyExtra(bool abflag, float afRefractionPower);
    void SetWeaponModSlot(uint8_t slot);
    void SetWeaponModFlags(uint8_t flags);
    // A null reference removes an existing extra; a non-null one is marked as targeted.
    void SetHeadTrackTargetExtra(TESObjectREFR* apRef);
    void SetActivateParentOnly(bool abVal);
    void SetIsModding(bool bModding);
    // A null ammo or a zero count removes the extra.
    void SetAmmoExtra(TESAmmo* apAmmoType, int32_t aiCount);
    AudioBuoyMarkerData* GetAudioBuoyMarkerData();
    // -1 without the extra.
    float GetCharge();
    // 1 without the extra.
    float GetScale();
    // 0 without the extra.
    float GetRadiation();
    TESObjectREFR* GetHeadTrackTargetExtra();
    bool GetPackageExtraActivate();
    bool GetPackageExtraDoneOnce();
    TrespassPackage* GetTrespassPackage();
    TESObjectREFR* GetOpenCloseActivateRef();
    // Sets the timer of the activate ref children extra, when the list has one.
    void SetActivateChildrenTimer(float afTimer);
    BSSimpleList<REF_REFLECTED_DATA*>* GetReflectedRefs();
    BSSimpleList<TESObjectREFR*>* GetWaterLightRefs();
    char* GetSavedAnimation();
    bool IsActivateParentOnly();
    void SetRoomIsMaster(bool abValue);
    BSSimpleList<TESObjectREFR*>* GetRoomLinkedRoomRefList();
    void RemoveRoomLinkedPortalRef(TESObjectREFR* apRef);
    TESObjectREFR* GetOriginalReference();
    // 255 without the extra.
    uint8_t GetSeedIndex();
    uint8_t GetTerminalState();
    // an invalid handle removes the extra
    void SetCreatureAwakeSound(const BSSoundHandle& afSound);
    // an invalid handle removes the extra
    void SetSound(const BSSoundHandle& afSound);
    // the location form is apWorld, or apIntCell without one; afzrotation is only kept by a new extra
    void
    SetPackageStartLocation(TESWorldSpace* apWorld, TESObjectCELL* apIntCell, const NiPoint3& aPt, float afzrotation);
    // 1.0 removes the extra
    void SetHealthPerc(float afHealthPerc);
    // a null world removes the havok extra
    void SetbhkWorld(bhkWorld* apWorld);
    bhkWorld* GetbhkWorld();
    void SetCellMopp(CellMopp* apMopp);
    CellMopp* GetCellMopp();
    // a null type removes the extra
    void SetMusicType(BGSMusicType* apType);
    // a null space removes the extra
    void SetAcousticSpace(BGSAcousticSpace* apSpace);
    // a null image space removes the extra
    void SetImageSpace(TESImageSpace* apImageSpace);
    // a null ID removes the extra
    void SetEditorID(const char* apEditorID);
    // also clears the extra's default flag
    void SetLevelItemExtra(int32_t aiIndex);
    // reads the rag doll from apRef; a null apRef removes the extra
    void SetRagDollData(TESObjectREFR* apRef);

    // Resolves the form IDs a loaded list holds for its owner (a reference or a cell) and drops the extras whose
    // forms are missing.
    void InitItem(TESForm* apOwner);
    // a copy of the sound extra's handle, or a cleared handle
    void GetSound(BSSoundHandle& result);
    // a copy of the creature awake sound extra's handle, or a cleared handle
    void GetCreatureAwakeSound(BSSoundHandle& result);
    // A copy of the weapon attack sound extra's handle, or a cleared handle.
    void GetWeaponAttackSound(BSSoundHandle& result);
    // Stores afSound in the weapon attack sound extra; an invalid handle removes it.
    void SetWeaponAttackSound(const BSSoundHandle& afSound);
    // the magic light extra's light
    LightRefData* GetSpellEffectLight();
    // 0.0 when there is no north rotation extra
    float GetNorthRotation();
    // The occlusion plane extra's linked references, or null.
    OcclusionPlaneLinkedRefData* GetOcclusionPlaneLinkedRefData();
    // Loads the saved animation into apRef and drops the extra; false when there is none.
    bool RestoreSavedAnimation(TESObjectREFR* apRef);
    // Loads the saved Havok state into apRef and drops the extra; false when there is none.
    bool RestoreSavedHavokData(TESObjectREFR* apRef);
    void RemoveSavedHavokData();
    // The world space or cell the reference started in, or null.
    TESForm* GetStartingWorldOrCell();
    // Records apRef's current world space or cell as its starting one.
    void SetStartingWorldOrCellForRef(TESObjectREFR* apRef);
    // Adds apRef to the portal list of this room's linked data.
    void AddRoomLinkedPortalRef(TESObjectREFR* apRef);
    // the water zone map extra's map, or null
    NiTMap<WaterZone*, int32_t>* QWaterZoneMap();
    // Set or clear the portal and room extras.
    void SetPortal(BSPortal* apPortal);
    void SetRoom(BSMultiBoundRoom* apRoom);
    // The room, portal and portal-link extras' data, or null.
    BSMultiBoundRoom* GetRoom();
    BSPortal* GetPortal();
    PortalLinkedRefData* GetPortalLinkedRefData();
    // The master room of this room's linked data, or null.
    TESObjectREFR* GetMasterRoom();
    // Sets or clears the occlusion plane extra.
    void SetOcclusionPlane(BSOcclusionPlane* apPlane);
    // Remove the light and the magic light extras.
    void RemoveLightPtr();
    void RemoveSpellEffectLightPtr();
    // Copies the creature movement sound extra's handle into arSound.
    void GetCreatureMovementSound(BSSoundHandle& result);
    // Stores arSound in the creature movement sound extra.
    void SetCreatureMovementSound(const BSSoundHandle& afSound);
    void RemoveAnimSave();
    void RemoveLockPtr();
    void RemoveTeleportPtr();
    void RemoveHealth();
    void RemovePoison();
    void RemoveScale();
    void RemoveGhost();
    void RemoveCannotWearExtra();
    void RemovePackageStartLocation();
    void RemoveStartingWorldOrCell();
    void RemoveObjectHealth();
    void RemoveExtraRunOncePackageList();
    void ClearRumorsExtra();
    void RemoveFriendHitsExtra();
    void RemoveSavedAnimation();
    void RemoveInfoGeneral();
    void RemoveModelSwap();
    TESObjectREFR* GetLinkedPlaneRef(int32_t aiEdge);
    void RemoveAllSaveableData(uint32_t aiFlags, TESObjectREFR* apRef);
    void RemoveOriginalReferenceExtra();
    void RemoveWorn(bool ableftworn);
    void RemoveReferencePointer();
    REF_ACTIVATE_DATA* GetActivateData(TESObjectREFR* apRef);
    float GetActivateDelay(TESObjectREFR* apRef);
    void RemoveFollowerExtra();
    void RemoveTalkingActorExtra();
    void RemoveIsModding();
    AlchemyItem* GetPoison();
    // Whether the list's ownership extra names an owner other than the default.
    bool HasOnlyOwnershipNonDefault();
    // Whether the list has a weapon mod flags extra with flags
    // set.
    uint8_t HasWeaponMods();
    // Sets the poison extra to apPoison, adding one when missing.
    void SetPoison(AlchemyItem* apPoison);
    void SetPackageExtraTarget(TESObjectREFR* apRef);
    void SetPackageExtraIndex(int32_t aiIndex);
    int32_t GetPackageExtraIndex();
    TESObjectREFR* GetPackageExtraTarget();
    void SetPackageExtraActionComplete(bool abFlag);
    ActorPackageData* GetActorPackageData();
    void SetExtraSayToResponseIDOnExtra(uint32_t anResponseID);
    // Sets or adds the say-to topic info extra.
    void SetExtraSayToInfoExtra(TESTopicInfo* apInfo);
    // Sets the topic of the say-to topic info extra.
    void SetExtraSayToTopicOnExtra(TESTopic* apTopic);
    bool ShouldPopInWhenEnabledByParent();
    bool ShouldPopOutWhenDisabledByParent();
    void RemoveActorPackageData();
    void NotifyGuardsAboutActivation(TESObjectREFR* apObject, TESObjectREFR* apActionRef);
    void RemoveNavMeshPortal();
    void RemoveDismembermentExtra();
    void SetTalkingActorExtra(BGSTalkingActivator* apTalkAct, TESObjectREFR* apRef);
    void SetPackageExtra(
        TESPackage* apPack,
        int32_t aiIndex,
        TESObjectREFR* apRef,
        bool abcomplete,
        bool abactivated,
        bool abDoneOnce
    );
    void SetDistantDataNormal(const NiPoint3& aNormal);
    void SetLinkedPlaneRef(TESObjectREFR* apRef, int32_t aiEdge);
    void SetPortalLinkedRoomRef(int32_t aiWhich, TESObjectREFR* apRef);
    void SetActorPackageData(ActorPackageData* apData);
    int32_t AddFriendHit();
    void RevertUnsavedExtra(BGSLoadFormBuffer* apLoadGameBuffer);
    // Finishes the reference's load from the save buffer.
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    ExtraDismemberedLimbs* AddDismembermentExtra();
    void SetActorCauseExtra(ActorCause* apActorCause);
    ExtraFollowerSwimBreadcrumbs* GetOrCreateExtraFollowerSwimBreadcrumbs();

protected:
    // 1 removes an action extra that has no action reference
    void SetActionFlag(OBJECT_ACTION aeType);
    // the action extra, added when there is none
    ExtraAction* SetActionPtr();
    // the starting position extra, added from apObject when there is none
    ExtraStartingPosition* SetStartingPositionPtr(TESObjectREFR* apObject);

public:
    // Removes apActor's player crime list data.
    void RemovePlayerCrimeListExtra(Actor* apActor);
    void CopyListForContainer(ExtraDataList* apCopy, bool abDontRemove);
    void ClearTrespassPackage(TESPackage* apPackage);
    void AddGuard(Actor* apActor);
    ExtraNavMeshPortal* GetNavMeshPortal();
    void DuplicateExtraListForContainer(ExtraDataList* apCopy);
    void LoadGameBGS(BGSLoadFormBuffer* a1);
    void GetWeaponIdleSound(BSSoundHandle& result);
    void SetWeaponIdleSound(const BSSoundHandle& afSound);
    void GetActivateLoopSound(BSSoundHandle& result);
    void SetActivateLoopSound(const BSSoundHandle& afSound);
    void ClearScriptLocals();
    void AddToPlayerCrimeList(Crime* apCrime, float afTimeStamp);
    void AddActivateParent(TESObjectREFR* apRef);
    void SetActivateDelay(TESObjectREFR* apRef, float afVal);
    BSStringT<char> GetActivateTextOverride();
    void Revert_0042DAE0(BGSLoadFormBuffer* apLoadGameBuffer);
    MenuTopic* GetInfoGeneralTopic(TESObjectREFR* apSpeaker);
    void RemoveExtraSayTopicInfoOnceADayExtra();
};

// Each extra data type's change flag, indexed by type 0 to 146 (EXTRA_SPECIAL_RENDER_FLAGS). The elements are int32_t
// because BGSChangeFlags::iFlags and the CheckFlags argument they are passed to are int.
using xDataTypeToChangeFlag_t = int32_t[147];
inline xDataTypeToChangeFlag_t xDataTypeToChangeFlag{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    -2147482624,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1024,
    0,
    2048,
    -2147483647 - 1,
    -2147483647 - 1,
    -2147483647 - 1,
    1024,
    2048,
    1073741824,
    -2147483647 - 1,
    0,
    1088,
    1088,
    1088,
    1024,
    1024,
    1024,
    1024,
    1024,
    1024,
    4096,
    131072,
    -2147483647 - 1,
    0,
    262144,
    1024,
    1024,
    0,
    -2147483647 - 1,
    -2147483647 - 1,
    0,
    -2147483647 - 1,
    0,
    0,
    0,
    -2147483647 - 1,
    0,
    0,
    4096,
    0,
    1024,
    1024,
    0,
    0,
    0,
    0,
    0,
    -2147483647 - 1,
    -2147483647 - 1,
    0,
    0,
    1024,
    1024,
    0,
    0,
    -2147483647 - 1,
    -2147483647 - 1,
    0,
    -2147483647 - 1,
    0,
    0,
    0,
    67108864,
    -2147483647 - 1,
    -2147483647 - 1,
    0,
    0,
    0,
    0,
    -2147483647 - 1,
    1073741824,
    1073741824,
    -2147483647 - 1,
    131072,
    -2147483647 - 1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    -2147483647 - 1,
    0,
    2048,
    0,
    -2147483647 - 1,
    0,
    0,
    -2147483647 - 1,
    536870912,
    -2147483647 - 1,
    0,
    0,
    0,
    0,
    0,
    0,
    -2147483647 - 1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    -2147483647 - 1,
    0,
    -2147483647 - 1,
    0,
    1024,
    0,
    -2147483647 - 1,
    0,
    0,
    -2147483647 - 1
};
