#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Havok/CellMopp.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout Shared/TESFullName.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "SDK/MSVC/ehvec.h"

class Actor;
class ActorCause;
class ActorPackageData;
class AlchemyItem;
class Animation;
class AudioBuoyMarkerData;
class BGSAcousticSpace;
class BSAnimNoteReceiver;
class BSMultiBoundRoom;
class BSOcclusionPlane;
class Crime;
struct FACTION_RANK;
class ImpactSwap;
class MediaLocationController;
class MenuTopic;
class MobileObject;
class NiLight;
class NiPointLight;
struct RagDollBone;
class TESAmmo;
class TESBoundObject;
class TESClimate;
class TESFaction;
class TESKey;
class TESObjectREFR;
class TESRegionList;
class TESReputation;
class TESTopic;
class TESTopicInfo;
class TESWaterForm;
class TESWorldSpace;
class TrespassPackage;
class WaterZone;

enum RADIO_RANGE_TYPE : uint32_t;

// The concrete kinds of BSExtraData. Only some of their fields are declared; each declared field is a pointer or value
// stored directly after the BSExtraData header unless noted.

struct RADIO_DATA {
	RADIO_DATA();

	// Copies apCopy's fields.
	void Copy(RADIO_DATA* apCopy);
	// Swaps the four dwords.
	void Endian();

	float fRadius;
	RADIO_RANGE_TYPE eRangeType;
	float fStaticPct;
	TESObjectREFR* pPositionRef;
};

struct WORLD_LOCATION {
	WORLD_LOCATION();

	// Swaps the first four dwords.
	void Endian();

	TESForm* pLocationForm;
	NiPoint3 locPt;
	float fZRot;
};

class ExtraAction : public BSExtraData {
public:
	ExtraAction();

	bool Compare(BSExtraData* apCompare) override;

	uint8_t eAction;
	uint8_t padding0D[3];
	TESObjectREFR* pActionRef;
};

class ExtraActivateRef : public BSExtraData {
public:
	ExtraActivateRef();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraActivateRef() override;

	// The activate data of apRef in ParentList, or null.
	REF_ACTIVATE_DATA* GetData(TESObjectREFR* apRef);
	// Copies apCopy's activate parents, flags and text into this extra.
	void Copy(BSExtraData* apCopy);

	// REF_ACTIVATE_DATA items: the activate parents (form IDs until InitItem) and their delays
	BSSimpleList<REF_ACTIVATE_DATA*> ParentList;
	uint8_t cActivateFlags;
	uint8_t padding15[3];
	BSStringT<char> ActivateTextOverride;
};

// A reference an activate parent activates, and the delay before it does.
struct REF_ACTIVATE_DATA {
	REF_ACTIVATE_DATA();

	// Swaps the reference and the delay.
	void Endian();

	TESObjectREFR* pActivateRef;
	float fActivateDelay;
};

// The item of apList whose reference is apRef, or null.
REF_ACTIVATE_DATA* GetActivateDataFromList(TESObjectREFR* apRef, BSSimpleList<REF_ACTIVATE_DATA*>* apList);
// The lock difficulty (0 very easy to 5 requires key) of a lock level, from the gs_iLockLevelMax* settings.
int32_t GetLockPickLevel(int32_t aiLockLevel);
uint32_t getLockLevelName(int32_t a1);
void FreeActivateList(BSSimpleList<REF_ACTIVATE_DATA*>* apList);

class ExtraActivateRefChildren : public BSExtraData {
public:
	ExtraActivateRefChildren();
	~ExtraActivateRefChildren() override;

	BSSimpleList<REF_ACTIVATE_DATA*> ChildList;
	float fActivateChildrenTimer;
};

class ExtraAmmo : public BSExtraData {
public:
	ExtraAmmo();
	ExtraAmmo(TESAmmo* apAmmo, int32_t aiCount);

	bool Compare(BSExtraData* apCompare) override;

	TESAmmo* pAmmo;
	// the rounds loaded per instance of the weapon
	int32_t iCount;
};

class ExtraAnim : public BSExtraData {
public:
	ExtraAnim(Animation* apAnim);
	~ExtraAnim() override;

	Animation* pAnimation;
};

class ExtraAnimNoteReceiver : public BSExtraData {
public:
	ExtraAnimNoteReceiver(BSAnimNoteReceiver* apReceiver);
	~ExtraAnimNoteReceiver() override;

	BSAnimNoteReceiver* pReceiver;
};

class ExtraAudioMarker : public BSExtraData {
public:
	ExtraAudioMarker(AudioMarker* apAudioData);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraAudioMarker() override;

	AudioMarker* pAudioData;
};

class ExtraCellImageSpace : public BSExtraData {
public:
	ExtraCellImageSpace(TESImageSpace* apImageSpace);

	bool Compare(BSExtraData* apCompare) override;

	TESImageSpace* pImageSpace;
};

class ExtraCellImpactSwap : public BSExtraData {
public:
	ExtraCellImpactSwap();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraCellImpactSwap() override;

	ImpactSwap* pImpactSwap;
};

class ExtraCellMusicType : public BSExtraData {
public:
	ExtraCellMusicType(BGSMusicType* apType);

	bool Compare(BSExtraData* apCompare) override;

	BGSMusicType* pType;
};

class ExtraCellWaterType : public BSExtraData {
public:
	ExtraCellWaterType();

	bool Compare(BSExtraData* apCompare) override;

	TESWaterForm* pWater;
};

// A cell's canopy shadow mask: the state, the rendered texture holding the mask and the locked rect the renderer
// reads it through (the PC's D3DLOCKED_RECT is 8 bytes; the constructor clears only the pointer half).
class ExtraCellCanopyShadowMask : public BSExtraData {
public:
	enum CELL_STATE : int32_t {
		CS_NONE = 0x0,
		CS_PARTIAL = 0x1,
		CS_FULL = 0x2,
	};

	ExtraCellCanopyShadowMask(ExtraCellCanopyShadowMask::CELL_STATE aeState, NiRenderedTexture* apTex);
	~ExtraCellCanopyShadowMask() override;

	CELL_STATE eState;
	NiPointer<NiRenderedTexture> spMask;
	D3DLOCKED_RECT Rect;
};

// A reference's collision layer override.
struct CollisionData {
	CollisionData();

	COL_LAYER QLayer();
	void SetLayer(COL_LAYER aeLayer);
	// Swaps the layer.
	void Endian();

	COL_LAYER eLayer;
};

class ExtraCollisionData : public BSExtraData {
public:
	ExtraCollisionData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraCollisionData() override;

	CollisionData* pCollisionData;
};

class ExtraCombatStyle : public BSExtraData {
public:
	ExtraCombatStyle(TESCombatStyle* apCombatStyle);

	bool Compare(BSExtraData* apCompare) override;

	TESCombatStyle* pCombatStyle;
};

class ExtraContainerChanges : public BSExtraData {
public:
	ExtraContainerChanges(InventoryChanges* apChanges);
	~ExtraContainerChanges() override;

	InventoryChanges* pChanges;
};

class ExtraCount : public BSExtraData {
public:
	ExtraCount();
	ExtraCount(int16_t aiCount);

	bool Compare(BSExtraData* apCompare) override;

	int16_t iCount;
};

class ExtraDecalRefs : public BSExtraData {
public:
	ExtraDecalRefs();

	bool Compare(BSExtraData* apCompare) override;

	// Resolves the decal references' form IDs for apOwner.
	void InitItem(TESForm* apOwner);
	// The decal entry of apRef, else null.
	REF_DECAL_DATA* GetRefDecalData(TESObjectREFR* apRef);
	// Moves apRef's decal entry to aIntersect and aNormal, adding the entry at the head when there is none.
	void SetRefDecalData(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal);
	int32_t ClearDecalData();
	// Copies apCopy's decal entries into this extra.
	void Copy(BSExtraData* apCopy);

	BSSimpleList<REF_DECAL_DATA*> DecalRefList;
};

class ExtraDetachTime : public BSExtraData {
public:
	ExtraDetachTime();

	int32_t iTime;
};

class ExtraDroppedItemList : public BSExtraData {
public:
	ExtraDroppedItemList();
	~ExtraDroppedItemList() override;

	BSSimpleList<TESObjectREFR*> DroppedItemList;
};

class ExtraEmittanceSource : public BSExtraData {
public:
	ExtraEmittanceSource();

	bool Compare(BSExtraData* apCompare) override;

	// A light or a region: ExtraDataList::InitItem keeps only a LIGH or a REGN form.
	TESForm* pSource;
};

class ExtraEnableStateChildren : public BSExtraData {
public:
	ExtraEnableStateChildren();
	~ExtraEnableStateChildren() override;

	BSSimpleList<TESObjectREFR*> ChildList;
};

class ExtraEnableStateParent : public BSExtraData {
public:
	ExtraEnableStateParent();

	bool Compare(BSExtraData* apCompare) override;

	// true when the reference is enabled whenever its parent is disabled
	bool IsInverse() const;
	// Sets or clears the inverse flag.
	void SetInverse(bool abVal);
	bool ShouldPopIn();
	bool ShouldPopOut();

	TESObjectREFR* pParent;
	uint8_t cFlags;
};

// The XESP chunk: the enable parent's form ID and its flags.
struct REF_ENABLE_PARENT_DATA {
	// Swaps the parent's form ID.
	void Endian();

	TESObjectREFR* pParent;
	uint8_t cFlags;
	uint8_t padding05[3];
};

class ExtraEncounterZone : public BSExtraData {
public:
	ExtraEncounterZone();

	bool Compare(BSExtraData* apCompare) override;

	BGSEncounterZone* pZone;
};

class ExtraFollower : public BSExtraData {
public:
	ExtraFollower();
	~ExtraFollower() override;

	BSSimpleList<Actor*>* pActorlist;
};

class ExtraGlobal : public BSExtraData {
public:
	ExtraGlobal();
	ExtraGlobal(TESGlobal* apGlobal);

	bool Compare(BSExtraData* apCompare) override;

	TESGlobal* pGlobal;
};

class ExtraHealth : public BSExtraData {
public:
	ExtraHealth();
	ExtraHealth(float afHealth);

	bool Compare(BSExtraData* apCompare) override;

	float fHealth;
};

class ExtraHealthPerc : public BSExtraData {
public:
	ExtraHealthPerc();

	bool Compare(BSExtraData* apCompare) override;

	float fHealthPerc;
};

class ExtraIgnoredBySandbox : public BSExtraData {
public:
	ExtraIgnoredBySandbox();
};

class ExtraLastFinishedSequence : public BSExtraData {
public:
	ExtraLastFinishedSequence(const char* apSequenceName);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraLastFinishedSequence() override;

	char* pLastSequenceName;
};

class ExtraLeveledCreature : public BSExtraData {
public:
	ExtraLeveledCreature();

	TESActorBase* pOriginalBase;
	TESActorBase* pTemplate;
};

class ExtraLeveledItem : public BSExtraData {
public:
	ExtraLeveledItem();
	ExtraLeveledItem(int32_t aiIndex);

	bool Compare(BSExtraData* apCompare) override;

	int32_t iIndex;
	uint8_t bdefault;
};

class ExtraLight : public BSExtraData {
public:
	ExtraLight(LightRefData* apLight);
	~ExtraLight() override;

	LightRefData* pLight;
};

class ExtraLinkedRef : public BSExtraData {
public:
	ExtraLinkedRef();

	bool Compare(BSExtraData* apCompare) override;

	TESObjectREFR* pLinkedRef;
};

class ExtraLinkedRefChildren : public BSExtraData {
public:
	ExtraLinkedRefChildren();
	~ExtraLinkedRefChildren() override;

	BSSimpleList<TESObjectREFR*> ChildList;
};

class ExtraLitWaterRefs : public BSExtraData {
public:
	ExtraLitWaterRefs();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraLitWaterRefs() override;

	// Resolves the lit water references' form IDs for apOwner.
	void InitItem(TESForm* apOwner);
	// Copies apCopy's references into this extra.
	void Copy(BSExtraData* apCopy);
	// Adds apRef to RefList unless it is in it already; Copy calls
	// it for each of the other extra's references.
	void AddReference(TESObjectREFR* apRef);
	void ClearReferenceData();

	BSSimpleList<TESObjectREFR*> RefList;
};

// A lock's strength, key and state.
class LockData {
public:
	LockData();

	// Swaps the lock's dwords.
	void Endian();
	// Whether the lock is broken.
	bool IsBroken();
	// Whether the locked bit is set.
	bool IsLocked();
	// The lock-pick difficulty of the lock's level for apOwner (GetLockPickLevel of GetLevel).
	int32_t GetLockLevel(TESObjectREFR* apOwner);
	// The lock's level: the base level, plus apOwner's level (the player's without one) times fLeveledLockMult
	// for a leveled lock, capped at 99.
	int32_t GetLevel(TESObjectREFR* apOwner);
	// sets or clears the locked bit; unlocking also resets the number of tries
	void SetLocked(bool abLocked);

	uint8_t cLockLevel;
	uint8_t padding01[3];
	TESKey* pKey;
	uint8_t cFlags;
	uint8_t padding09[3];
	uint32_t iNumTries;
	uint32_t iTimesUnlocked;
};

class ExtraLock : public BSExtraData {
public:
	ExtraLock();
	ExtraLock(LockData* apLock);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraLock() override;

	void UnLockDoor();
	void LockDoor();

	LockData* pLock;
};

class ExtraMapMarker : public BSExtraData {
public:
	ExtraMapMarker(MapMarkerData* apMapData);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraMapMarker() override;

	MapMarkerData* pMapData;
};

class ExtraMerchantContainer : public BSExtraData {
public:
	ExtraMerchantContainer();

	bool Compare(BSExtraData* apCompare) override;

	TESObjectREFR* pContainer;
};

class ExtraModelSwap : public BSExtraData {
public:
	ExtraModelSwap();
	ExtraModelSwap(TESModel* apModelSwap, TESForm* apModelSwapForm);

	TESModel* pModelSwap;
	TESForm* pModelSwapForm;
};

class ExtraMultiBoundData : public BSExtraData {
public:
	ExtraMultiBoundData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraMultiBoundData() override;

	MultiBoundMarkerData* pBound;
};

class ExtraMultiBoundRef : public BSExtraData {
public:
	ExtraMultiBoundRef();

	bool Compare(BSExtraData* apCompare) override;

	TESObjectREFR* pBoundRef;
};

class ExtraNorthRotation : public BSExtraData {
public:
	ExtraNorthRotation();

	float fNorthRot;
};

class ExtraOwnership : public BSExtraData {
public:
	ExtraOwnership();
	ExtraOwnership(TESForm* apOwner);

	bool Compare(BSExtraData* apCompare) override;

	TESForm* pOwner;
};

class ExtraPackage : public BSExtraData {
public:
	ExtraPackage();
	ExtraPackage(TESPackage* apPack, int32_t aiIndex, TESObjectREFR* apRef, bool abcomplete, bool abactivated,
		bool abDoneOnce);

	TESPackage* pPack;
	int32_t iindex;
	TESObjectREFR* pTarg;
	bool bActionComplete;
	bool bActivated;
	bool bDoneOnce;
};

class ExtraRagDollData : public BSExtraData {
public:
	ExtraRagDollData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraRagDollData() override;

	RagDollData* pRagDollData;
};

class ExtraPackageStartLocation : public BSExtraData {
public:
	ExtraPackageStartLocation();
	ExtraPackageStartLocation(TESWorldSpace* apWorld, TESObjectCELL* apIntCell, const NiPoint3& aPt, float afzrotation);

	bool Compare(BSExtraData* apCompare) override;

	WORLD_LOCATION worldLoc;
};

class ExtraPatrolRefData : public BSExtraData {
public:
	ExtraPatrolRefData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraPatrolRefData() override;

	void SetPatrolRefData(PatrolRefData* apPatrolRefData);
	void Clear();

	PatrolRefData* pPatrolData;
};

class ExtraPersistentCell : public BSExtraData {
public:
	ExtraPersistentCell(TESObjectCELL* apCell);

	TESObjectCELL* pPersistentCell;
};

class ExtraPlayerCrimeList : public BSExtraData {
public:
	ExtraPlayerCrimeList();
	ExtraPlayerCrimeList(Crime* apCrime);
	~ExtraPlayerCrimeList() override;

	BSSimpleList<Crime*>* pCrime;
};

class ExtraPrimitive : public BSExtraData {
public:
	ExtraPrimitive(BGSPrimitive* apPrimitive);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraPrimitive() override;

	BGSPrimitive* pPrimitive;
};

class ExtraRadiation : public BSExtraData {
public:
	ExtraRadiation();
	ExtraRadiation(float afRadiation);

	bool Compare(BSExtraData* apCompare) override;

	float fRadiation;
};

class ExtraRadioData : public BSExtraData {
public:
	ExtraRadioData();

	bool Compare(BSExtraData* apCompare) override;

	RADIO_DATA Data;
};

class ExtraRadius : public BSExtraData {
public:
	ExtraRadius();
	ExtraRadius(float afRadius);

	bool Compare(BSExtraData* apCompare) override;

	float fRadius;
};

class ExtraRank : public BSExtraData {
public:
	ExtraRank();
	ExtraRank(int32_t aiRank);

	bool Compare(BSExtraData* apCompare) override;

	int32_t iRank;
};

class ExtraReferencePointer : public BSExtraData {
public:
	ExtraReferencePointer();
	ExtraReferencePointer(TESObjectREFR* apRef);

	TESObjectREFR* pRef;
};

// A reference that reflects or refracts this one (iEffectFlags: 1 reflects, 2 refracts).
struct REF_REFLECTOR_DATA {
	REF_REFLECTOR_DATA();

	// Swaps the reference's form ID and the flags.
	void Endian();

	TESObjectREFR* pRef;
	uint32_t iEffectFlags;
};

// A reference this one reflects or refracts (iEffectFlags: 1 reflects, 2 refracts).
struct REF_REFLECTED_DATA {
	REF_REFLECTED_DATA();

	TESObjectREFR* pRef;
	uint32_t iEffectFlags;
};

class ExtraReflectedRefs : public BSExtraData {
public:
	ExtraReflectedRefs();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraReflectedRefs() override;

	// Copies apCopy's references into this extra.
	void Copy(BSExtraData* apCopy);
	void RemoveReference(TESObjectREFR* apRef);
	void SetReference(TESObjectREFR* apRef, uint32_t aiEffectFlags);
	void ClearReferenceData();
	REF_REFLECTED_DATA* GetRefData(TESObjectREFR* apRef);

	BSSimpleList<REF_REFLECTED_DATA*> RefList;
};

class ExtraReflectorRefs : public BSExtraData {
public:
	ExtraReflectorRefs();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraReflectorRefs() override;

	// Copies apCopy's references into this extra.
	void Copy(BSExtraData* apCopy);
	void RemoveReference(TESObjectREFR* apRef);
	void SetReference(TESObjectREFR* apRef, uint32_t aiEffectFlags);
	// Resolves the reflector references' form IDs for apOwner.
	void InitItem(TESForm* apOwner);
	REF_REFLECTOR_DATA* GetRefData(TESObjectREFR* apRef);
	void ClearReferenceData();

	BSSimpleList<REF_REFLECTOR_DATA*> RefList;
};

// A package an actor runs once, on the day it was given.
struct RunOncePackage {
	TESPackage* pPackage;
	int8_t cday;
};

class ExtraRunOncePacks : public BSExtraData {
public:
	ExtraRunOncePacks();
	~ExtraRunOncePacks() override;

	// Adds apPack to run once on day acDay.
	void AddRunOncePack(TESPackage* apPack, char acDay);

	BSSimpleList<RunOncePackage*>* pPackageList;
};

class ExtraScript : public BSExtraData {
public:
	ExtraScript();
	ExtraScript(Script* apScript);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraScript() override;

	Script* pScript;
	ScriptLocals* pScriptVars;
};

class ExtraTeleport : public BSExtraData {
public:
	ExtraTeleport();
	ExtraTeleport(DoorTeleportData* apTeleport);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraTeleport() override;

	DoorTeleportData* pTeleportData;
};

class ExtraEditorID : public BSExtraData {
public:
	ExtraEditorID(const char* apEditorID);

	BSStringT<char> cEditorID;
};

class ExtraOcclusionPlane : public BSExtraData {
public:
	ExtraOcclusionPlane();

	NiPointer<BSOcclusionPlane> spPlane;
};

class ExtraWeaponModFlags : public BSExtraData {
public:
	ExtraWeaponModFlags();
	ExtraWeaponModFlags(uint8_t acSlot);

	bool Compare(BSExtraData* apCompare) override;

	// nonzero if any of the mod slot bits in aucSlotMask is active
	bool GetWeaponModActive(uint8_t slot);
	// Whether any weapon mod is active.
	bool HasWeaponMods();
	void SetWeaponModFlags(uint8_t flags);
	void SetWeaponModActive(const uint8_t slot);

	uint8_t cWeaponModsActive;
	uint8_t padding0D[3];
};

class ExtraXTarget : public BSExtraData {
public:
	ExtraXTarget();

	bool Compare(BSExtraData* apCompare) override;

	TESObjectREFR* pTarget;
};

class ExtraUsedMarkers : public BSExtraData {
public:
	ExtraUsedMarkers();

	// marks (or unmarks) one of the 30 furniture markers as in use
	void SetMarkerUsed(uint32_t aiMarkerIndex, bool abVal);
	// Whether furniture marker aiMarkerIndex is in use.
	bool GetMarkerUsed(uint32_t aiMarkerIndex);

	uint32_t iUsedMarkers;
};

class ExtraSpecialRenderFlags : public BSExtraData {
public:
	// Bits of m_flags.
	enum SPECIAL_RENDER_FLAG : uint32_t {
		SRF_IMPOSTER = 0x2,
		SRF_STINGER_LOD = 0x4,
		SRF_INVISIBLE_LOD_MULT_TYPE = 0x8,
	};

	ExtraSpecialRenderFlags();

	bool Compare(BSExtraData* apCompare) override;
	virtual void InitItem(TESForm*) {}
	virtual void Copy(BSExtraData* apCopy);

	int32_t GetFlags();
	void SetFlags(int32_t aflags);

	uint32_t m_flags;
	float m_fNodeMaxFadeDistance;
};

class ExtraStartingPosition : public BSExtraData {
public:
	ExtraStartingPosition(TESObjectREFR* apObject);

	bool Compare(BSExtraData* apCompare) override;

	NiPoint3 startPosition;
	NiPoint3 startRotation;
};

enum LEV_CREA_MODIFIER : int32_t {
	LCM_EASY,
	LCM_MEDIUM,
	LCM_HARD,
	LCM_BOSS,
	LCM_NONE,
	LCM_COUNT,
};

class ExtraLevCreaModifier : public BSExtraData {
public:
	ExtraLevCreaModifier();

	bool Compare(BSExtraData* apCompare) override;

	// the multiplier is a game setting chosen by the modifier; an original call
	float GetMultiplier();
	// 1 for the "all" modifier, 0 for none, -1 for any other
	int32_t GetAllBelowForce();

	LEV_CREA_MODIFIER eModifier;
};

// The rooms and portals of a multibound room marker.
struct RoomRefData {
	RoomRefData();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSimpleList<TESObjectREFR*> kPortalList;
	BSSimpleList<TESObjectREFR*> kRoomList;
	uint8_t ucRoomCount;
	uint8_t padding11[3];
};

class ExtraRoomRefData : public BSExtraData {
public:
	ExtraRoomRefData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraRoomRefData() override;

	void Clear();

	RoomRefData* pData;
};

class ExtraSayToTopicInfo : public BSExtraData {
public:
	ExtraSayToTopicInfo();
	ExtraSayToTopicInfo(TESTopicInfo* apTopicInfo);

	void SetTopic(TESTopic* apTopic);

	TESTopicInfo* pInfo;
	TESTopic* pTopic;
	uint32_t nResponseID;
	bool bVoiceDone;
	uint8_t padding19[3];
};

// A topic info said once a day, and when (the date packed as day | month << 9 | year << 13, then the hour).
class SayOnceTopicInfos {
public:
	SayOnceTopicInfos();
	SayOnceTopicInfos(TESTopicInfo* apInfo, int32_t aiDate, float afTime);

	TESTopicInfo* pInfo;
	int32_t iDateStamp;
	float fHourStamp;
};

// The once-a-day topic infos a reference has said.
class ExtraSayTopicInfoOnceADay : public BSExtraData {
public:
	ExtraSayTopicInfoOnceADay();
	ExtraSayTopicInfoOnceADay(SayOnceTopicInfos* apInfo);

	// Always returns false: two once-a-day lists never count as different.
	bool Compare(BSExtraData* apCompare) override;
	~ExtraSayTopicInfoOnceADay() override;

	BSSimpleList<SayOnceTopicInfos*>* pListofSaidOnceTopicInfos;
};

class ExtraWorn : public BSExtraData {
public:
	ExtraWorn();
};

class ExtraWornLeft : public BSExtraData {
public:
	ExtraWornLeft();
};

class ExtraCannotWear : public BSExtraData {
public:
	ExtraCannotWear();
};

// A cell's Havok world and its MOPP collision.
class ExtraHavok : public BSExtraData {
public:
	ExtraHavok(bhkWorld* apWorld);

	NiPointer<bhkWorld> spWorld;
	NiPointer<CellMopp> spCellMopp;
};

class ExtraCellAcousticSpace : public BSExtraData {
public:
	ExtraCellAcousticSpace(BGSAcousticSpace* apSpace);

	bool Compare(BSExtraData* apCompare) override;

	BGSAcousticSpace* pSpace;
};

class ExtraCellClimate : public BSExtraData {
public:
	ExtraCellClimate(TESClimate* apClimate);

	bool Compare(BSExtraData* apCompare) override;

	TESClimate* pClimate;
};

class ExtraPoison : public BSExtraData {
public:
	ExtraPoison();
	ExtraPoison(AlchemyItem* apPoison);

	bool Compare(BSExtraData* apCompare) override;

	AlchemyItem* pPoison;
};

class ExtraRandomTeleportMarker : public BSExtraData {
public:
	ExtraRandomTeleportMarker();

	bool Compare(BSExtraData* apCompare) override;

	TESObjectREFR* pMarker;
};

// The references an occlusion plane is linked to on each edge; form IDs until the item is initialised.
struct OcclusionPlaneLinkedRefData {
	OcclusionPlaneLinkedRefData();

	// Swaps the four form IDs read from a
	// big-endian file.
	void Endian();

	TESObjectREFR* pLinkedRefs[4];
};

class ExtraOcclusionPlaneRefData : public BSExtraData {
public:
	ExtraOcclusionPlaneRefData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraOcclusionPlaneRefData() override;

	// Deletes pData.
	void Clear();

	OcclusionPlaneLinkedRefData* pData;
};

// The two rooms a portal links; form IDs until the item is initialised.
struct PortalLinkedRefData {
	PortalLinkedRefData();

	// Swaps both form IDs.
	void Endian();

	TESObjectREFR* pLinkedRefs[2];
};

class ExtraPortalRefData : public BSExtraData {
public:
	ExtraPortalRefData();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraPortalRefData() override;

	void Clear();

	PortalLinkedRefData* pData;
};

// The XPRD chunk: the time a patroller waits at the patrol point.
struct REF_PATROL_DATA {
	REF_PATROL_DATA();

	// Swaps the time.
	void Endian();
	float* SetTimeAtRef(float* a1);

	float fTimeAtRef;
};

// A patrol point's wait time and what the patroller does there.
struct PatrolRefData {
	PatrolRefData();
	PatrolRefData(const REF_PATROL_DATA& aData);

	// Resolves the patrol actions' form IDs for apOwner.
	void InitItem(TESForm* apOwner);
	// Copies apCopy's wait time and action.
	void Copy(PatrolRefData* apCopy);
	Script* GetOnPatrolScript();
	float GetTimeAtRef();
	// The patrol action's topic (OnPatrol.pTopic).
	TESTopic* GetOnPatrolTopic();
	bool Compare(PatrolRefData* apCompare);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	float fTimeAtRef;
	PackageEventAction OnPatrol;
};

// The XPRM chunk: a primitive's radii, colour and shape.
struct PRIMITIVE_DATA {
	PRIMITIVE_DATA();

	// Swaps the eight dwords.
	void Endian();

	NiPoint3 aRadii;
	NiColorA aColor;
	BGSPrimitive::PRIMITIVE_TYPE eType;
};

class ExtraSound : public BSExtraData {
public:
	ExtraSound(BSSoundHandle aSound);

	BSSoundHandle pHandle;
};

class ExtraCreatureAwakeSound : public BSExtraData {
public:
	ExtraCreatureAwakeSound(BSSoundHandle aSound);

	BSSoundHandle pHandle;
};

// The saved animation state of a reference, kept as a save-game sub-buffer until the 3D is loaded.
class ExtraSavedAnimation : public BSExtraData {
public:
	ExtraSavedAnimation();
	~ExtraSavedAnimation() override;

	// Releases the saved buffer.
	void DeleteBuffer();

	char* pAnimationBuffer;
};

// The saved Havok state of a reference, kept as a save-game sub-buffer until the 3D is loaded.
class ExtraSavedHavokData : public BSExtraData {
public:
	ExtraSavedHavokData();
	~ExtraSavedHavokData() override;

	void DeleteBuffer();

	char* pHavokBuffer;
};

// The water zones a reference is in, each with an entry count, and the highest of them.
class ExtraWaterZoneMap : public BSExtraData {
public:
	ExtraWaterZoneMap();

	// Records apWaterZone as in or out of the map.
	void SetInWaterZone(WaterZone* apZone, bool abValue);

	NiTMap<WaterZone*, int32_t> WaterZoneMap;
	WaterZone* pHighestWaterZone;
};

// A reference's light (the engine's REFR_LIGHT).
class LightRefData {
public:
	LightRefData();

	// Whether the light is apLight's.
	bool IsEqual(NiPointLight** apLight);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiPointer<NiLight> spLight;
	float fWantDimmer;
};

// A map marker's name, flags, type and reputation.
class MapMarkerData {
public:
	MapMarkerData();

	// Copies the name, flags, type and reputation of apCopy.
	void Copy(MapMarkerData* apCopy);
	bool GetVisible();
	bool GetTravelLoc();
	// Sets or clears fast travel to the marker.
	void SetTravelLoc(bool abVal);
	// Stores cFlags.
	void SetFlags(uint8_t acFlags) { cFlags = acFlags; }
	// The reputation the marker belongs to; at load, its form ID.
	TESReputation* GetReputation();
	void SetReputation(TESReputation* pRep);
	int32_t GetType();
	void ResetFlags();
	const char* GetLocationName();
	void SetIsVisible(uint8_t a2);
	// Reads the XMRK chunks of apFile's current record.
	bool Load(TESFile* apFile);
	// Writes the XMRK chunks: FNAM (cFlags), FULL (the name), TNAM (sType) and, with a reputation, WMI1 (its form ID).
	// ExtraDataList::Save calls it on the extra's data.
	void Save();
	bool Compare(MapMarkerData* apCompare);
	bool GetHidden();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	TESFullName LocationName;
	uint8_t cFlags;
	uint8_t cOriginalFlags;
	uint16_t sType;
	TESReputation* pReputation;
};

// An audio marker's data (the engine's AudioMarkerData).
class AudioMarker {
public:
	AudioMarker();

	// Returns the media location controller field, which holds the controller's form ID until it is looked up.
	MediaLocationController* GetLocationControllerID();
	// The media location controller form.
	TESForm* GetMediaLocationController();
	// Copies apCopy's name, then its controller, flag and trigger percentages.
	void Copy(AudioMarker* apCopy);
	// Reads the MMRK chunks of apFile's current record.
	bool Load(TESFile* apFile);
	// Writes the MMRK chunks FULL (the name), CNAM (the controller, when set), BNAM (bUseControllerValues), MNAM and
	// NNAM (the trigger percentages). ExtraDataList::Save calls it on the extra's data.
	void Save();
	// Whether apCompare differs: its name, controller, flag and trigger percentages. ExtraAudioMarker::Compare passes
	// the other extra's pAudioData.
	bool Compare(AudioMarker* apCompare);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	TESFullName kName;
	float fLayer2TriggerPerc;
	float fLayer3TriggerPerc;
	float fRadius;
	NiPoint3 unk18;
	MediaLocationController* pMediaLocationController;
	bool bUseControllerValues;
	uint8_t padding29[3];
	// The buoy markers. The element type is not known: the list is only constructed and destroyed.
	BSSimpleList<void*> lBuoyMarkers;
};

// The bone rotations of a reference's rag doll.
class RagDollData {
public:
	RagDollData();
	~RagDollData();

	// Reads the bone rotations from apRef's rag doll; true when every bone was filled.
	bool UpdateDataFromReference(TESObjectREFR* apRef);
	// Writes the bone rotations into apRef's rag doll.
	bool UpdateReferenceFromData(TESObjectREFR* apRef);
	// Reads the XRGD bone array and the XRGB Bip01 rotation of the current record of apFile; true when either was
	// read.
	bool Load(TESFile* apFile);
	void CreateRotation(TESObjectREFR* apRef);
	// Replaces the bones and the Bip01 rotation with apCopy's.
	void Copy(RagDollData* apCopy);
	// Frees the bones, then allocates one per rag doll bone of apRef's 3D and fills them from it.
	void CreateRagDollBoneArray(TESObjectREFR* apRef);
	// Fills the bones from apNode's tree from bone aiBoneIndex on.
	bool UpdateRagDollDataFromNode(NiNode* apNode, uint32_t& aiBoneIndex, bool abInit);
	// Reads the bone count, then the bones, from the old-format save.
	void LoadGame();
	void UpdateBip01NodeFromRagdollData(TESObjectREFR* apRef);
	void Save();
	bool Compare(RagDollData* apCompare);
	bool UpdateNodeFromRagDollData(NiNode* apNode, uint32_t& aiBoneIndex);

	// The number of nodes from apNode down (apNode included) that have a collision object.
	static uint8_t CountRagDollBones(NiNode* apNode);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint8_t iBoneCount;
	uint8_t padding01[3];
	RagDollBone* pBoneArray;
	NiPoint3 Bip01Rot;
};

// The animation group and the time the saved animation resumes at.
struct ANIM_SAVE_DATA {
	uint16_t eGroup;
	uint8_t padding02[2];
	float fTime;
};

// A reference's saved animation state. TESObjectREFR::InitAnimation resumes the 3D at data.fTime.
class ExtraAnimSave : public BSExtraData {
public:
	ANIM_SAVE_DATA data;
};

// How an actor died.
enum CAUSE_OF_DEATH : int32_t {
	CAUSE_OF_DEATH_NONE = -1,
	CAUSE_OF_DEATH_EXPLOSION = 0,
	CAUSE_OF_DEATH_GUN = 1,
	CAUSE_OF_DEATH_BLUNT_WEAPON = 2,
	CAUSE_OF_DEATH_HAND_TO_HAND = 3,
	CAUSE_OF_DEATH_OBJECT_IMPACT = 4,
	CAUSE_OF_DEATH_POISON = 5,
	CAUSE_OF_DEATH_RADIATION = 6,
	CAUSE_OF_DEATH_COUNT = 7,
};

// One dismembered limb of an actor: which limb, whether it was blown off, and the objects its gore spawned.
class DismemberedLimb {
public:
	DismemberedLimb();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint8_t cLimb;
	bool bLimbExploded;
	bool bObjectArrayIdentical;
	bool bLimbRemoved;
	BSSimpleArray<TESBoundObject*> ObjectArray;
};

// An actor's dismembered limbs and how it died.
class ExtraDismemberedLimbs : public BSExtraData {
public:
	ExtraDismemberedLimbs();
	~ExtraDismemberedLimbs() override;

	// Sets whether limb aeLimb is removed.
	void RemoveLimb(BGSBodyPart::LIMB_ENUM aeLimb, bool abRemoved);
	// The limb at aiIndex.
	DismemberedLimb* GetDismemberedLimb(uint32_t aiIndex);
	uint32_t GetDismemberedLimbCount();
	// Whether limb aeLimb's bit is set in sLimbs.
	bool Dismembered(BGSBodyPart::LIMB_ENUM aeLimb);
	void SetCauseofDeath(CAUSE_OF_DEATH aeDeath) { eCauseofDeath = aeDeath; }
	void SetEaten(bool a1);
	void Dismember(TESObjectREFR* a1, BGSBodyPart::LIMB_ENUM a2, bool a3);
	bool CheckDismemberedLimbsIdentical(ExtraDismemberedLimbs* apDismemberedLimbs);

	uint16_t sLimbs;
	CAUSE_OF_DEATH eCauseofDeath;
	TESForm* pDeathObject;
	BGSBodyPart::LIMB_ENUM eLastHitLimb;
	bool bEaten;
	BSSimpleArray<DismemberedLimb*> DismemberedLimbs;
};

class ExtraFactionChanges : public BSExtraData {
public:
	ExtraFactionChanges();
	~ExtraFactionChanges() override;

	// Marks the actor expelled from apFaction.
	void ExpelFromFaction(TESFaction* apFaction);
	// Whether the changes put the actor in apFaction.
	bool GetIsInFaction(TESFaction* apFaction);
	// Whether the changes expel the actor from apFaction.
	bool GetIsExpelled(TESFaction* apFaction);
	// Puts the actor in apFaction at aiRank.
	void AddToFaction(TESFaction* apFaction, int32_t aiRank);
	void ExpelFromAllFactions(TESActorBase* apActorBase);
	void SetFactionRank(TESFaction* apFaction, int32_t aiRank);

	BSSimpleList<FACTION_RANK*>* pFactionChanges;
};

// The refraction strength of a refractive reference.
class ExtraRefractionProperty : public BSExtraData {
public:
	ExtraRefractionProperty(float afRefractionPower);

	float fRefractionPower;
};

// The actor the reference's effect is blamed on. Holds an NiPointer<ActorCause>.
class ExtraActorCause : public BSExtraData {
public:
	ExtraActorCause();
	~ExtraActorCause() override;

	// Stores apCause as the extra's cause.
	void SetActorCause(ActorCause* apCause);
	// The cause, spActorCause.
	ActorCause* GetActorCause() { return spActorCause; }

	NiPointer<ActorCause> spActorCause;
};

// The audio data of a buoy marker.
class ExtraAudioBuoyMarker : public BSExtraData {
public:
	ExtraAudioBuoyMarker(AudioBuoyMarkerData* apAudioBuoyData);

	bool Compare(BSExtraData* apCompare) override;
	~ExtraAudioBuoyMarker() override;

	AudioBuoyMarkerData* pAudioBuoyData;
};

// The charge left in an energy weapon or other chargeable item.
class ExtraCharge : public BSExtraData {
public:
	ExtraCharge();
	ExtraCharge(float afCharge);

	bool Compare(BSExtraData* apCompare) override;

	float fCharge;
};

// Where a follower's leader was standing when it left its last breadcrumb.
enum BREADCRUMB_STATE : int32_t {
	BREADCRUMB_STATE_INVALID = 0x0,
	BREADCRUMB_STATE_ON_GROUND = 0x1,
	BREADCRUMB_STATE_IN_WATER = 0x2,
};

// One stretch of the leader's path: where it started and ended, and whether it entered the water there.
struct ExtraFollowerSwimBreadcrumb {
	ExtraFollowerSwimBreadcrumb();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiPoint3 StartLocation;
	uint32_t StartNavMeshID;
	NiPoint3 EndLocation;
	uint32_t EndNavMeshID;
	bool bEnteringWater;
};

// The trail a swimming follower tracks its leader by: the leader's last state, location and navmesh and the list of
// breadcrumbs it left.
class ExtraFollowerSwimBreadcrumbs : public BSExtraData {
public:
	ExtraFollowerSwimBreadcrumbs();
	~ExtraFollowerSwimBreadcrumbs() override;

	// The first breadcrumb in CrumbList.
	const ExtraFollowerSwimBreadcrumb* GetFirstCrumb();
	void RemoveFirstCrumb();
	bool UpdateLeaderInfo(BREADCRUMB_STATE aeLeaderState, NiPoint3 aLeaderLocation, uint32_t aLeaderNavMeshID);

	BREADCRUMB_STATE eLeaderState;
	NiPoint3 LeaderLocation;
	uint32_t LeaderNavMeshID;
	BSSimpleList<ExtraFollowerSwimBreadcrumb*> CrumbList;
};

// When the actor was last hit by friends, so that repeated friendly fire inside the minimum interval counts once.
class ExtraFriendHits : public BSExtraData {
public:
	ExtraFriendHits();
	~ExtraFriendHits() override;

	// The number of recorded hits.
	int32_t GetHitCount();
	// Records a hit now unless the last one is younger than fFriendMinimumLastHitTime.
	void AddHit();
	// Drops the hits older than fFriendHitTimer.
	void RemoveOldHits();

	BSSimpleArray<CombatTimeStamp, 1024> Hits;
};

// Marks a reference whose actor has no rumors to tell (bNoRumors).
class ExtraHasNoRumors : public BSExtraData {
public:
	ExtraHasNoRumors();
	ExtraHasNoRumors(bool abNoRumors);
	~ExtraHasNoRumors() override;

	bool bNoRumors;
};

// The hot key an item is bound to.
class ExtraHotKey : public BSExtraData {
public:
	ExtraHotKey();
	ExtraHotKey(char acHotKey);
	~ExtraHotKey() override;

	bool Compare(BSExtraData* apCompare) override;

	int8_t chotkey;
	uint8_t padding0D[3];
};

// Marks a weapon that is being modded.
class ExtraModdingItem : public BSExtraData {
public:
	ExtraModdingItem(bool abFlag);
	~ExtraModdingItem() override;

	bool Compare(BSExtraData* apCompare) override;

	void SetItemModding(bool bSet);

	bool bIsModding;
	uint8_t padding0D[3];
};

// The health of an object that is not an actor.
class ExtraObjectHealth : public BSExtraData {
public:
	ExtraObjectHealth();
	ExtraObjectHealth(float afHealth);

	float fHealth;
};

// The reference a copy of a reference was made from.
class ExtraOriginalReference : public BSExtraData {
public:
	ExtraOriginalReference(TESObjectREFR* apReference);

	bool Compare(BSExtraData* apCompare) override;

	TESObjectREFR* pReference;
};

// The package data an actor is running.
class ExtraPackageData : public BSExtraData {
public:
	ExtraPackageData();
	ExtraPackageData(ActorPackageData* apActorPackageData);

	ActorPackageData* pActorPackageData;
};

// Marks a patrol reference as in use. Stores the form ID of its argument.
class ExtraPatrolRefInUseData : public BSExtraData {
public:
	ExtraPatrolRefInUseData(TESObjectREFR* apUser);
	~ExtraPatrolRefInUseData() override;

	bool Compare(BSExtraData* apCompare) override;

	// Whether apRef may use the patrol reference.
	bool GetCanBeUsedBy(const TESObjectREFR* apPotentialUser);

	uint32_t User;
};

// The scale of a reference.
class ExtraScale : public BSExtraData {
public:
	ExtraScale();
	ExtraScale(float afScale);

	bool Compare(BSExtraData* apCompare) override;

	float fScale;
};

// The personality and mood a securitron's face shows.
class ExtraSecuritronFace : public BSExtraData {
public:
	ExtraSecuritronFace();
	~ExtraSecuritronFace() override;

	void SetPersonality(const char* asPersonality);
	void SetMood(const char* asMood);
	// Applies the personality and mood to the face of apMesh.
	void ApplyFace(NiAVObject* apMesh);
	BSStringT<char>* GetPersonality(BSStringT<char>* apResult);
	BSStringT<char>* GetMood(BSStringT<char>* apResult);

	BSStringT<char> msPersonality;
	BSStringT<char> msMood;
};

// The handle of the sound a creature makes when it moves.
class ExtraCreatureMovementSound : public BSExtraData {
public:
	ExtraCreatureMovementSound(BSSoundHandle aHandle);

	BSSoundHandle pHandle;
};

// The handle of the sound an idle weapon makes. The constructor takes the handle by value.
class ExtraWeaponIdleSound : public BSExtraData {
public:
	ExtraWeaponIdleSound(BSSoundHandle aHandle);

	BSSoundHandle pHandle;
};

// The handle of the sound an activation loops. The constructor takes the handle by value.
class ExtraActivateLoopSound : public BSExtraData {
public:
	ExtraActivateLoopSound(BSSoundHandle aHandle);

	BSSoundHandle pHandle;
};

// The object standing in for the actor a dialogue is with.
class ExtraTalkingActor : public BSExtraData {
public:
	ExtraTalkingActor();
	ExtraTalkingActor(BGSTalkingActivator* apBaseForm, TESObjectREFR* apActor);

	// Makes the mobile object of apBaseForm that stands in for apActor.
	MobileObject* CreateLocalMobileObjectCopy(BGSTalkingActivator* apTalkact, TESObjectREFR* apRef);

	MobileObject* pTalkObject;
};

// The terminal state of a reference (cState and cLockOverride).
class ExtraTerminalState : public BSExtraData {
public:
	ExtraTerminalState();
	~ExtraTerminalState() override;

	bool IsLockedState();

	uint8_t cState;
	uint8_t cLockOverride;
	uint8_t padding0E[2];
};

// The time left of a timed reference.
class ExtraTimeLeft : public BSExtraData {
public:
	ExtraTimeLeft();
	ExtraTimeLeft(float afTime);
	~ExtraTimeLeft() override;

	bool Compare(BSExtraData* apCompare) override;

	float fTime;
};

// The package that makes the actor trespass.
class ExtraTresPassPackage : public BSExtraData {
public:
	ExtraTresPassPackage();
	ExtraTresPassPackage(TrespassPackage* apPack);
	~ExtraTresPassPackage() override;

	TrespassPackage* pPack;
};

// The number of uses a form instance has left.
class ExtraUses : public BSExtraData {
public:
	ExtraUses();
	ExtraUses(uint8_t acUses);
	~ExtraUses() override;

	bool Compare(BSExtraData* apCompare) override;

	uint8_t cUses;
	uint8_t padding0D[3];
};

// The reference an actor's head tracks.
class ExtraHeadingTarget : public BSExtraData {
public:
	ExtraHeadingTarget();
	ExtraHeadingTarget(TESObjectREFR* apHeadingTarget);
	~ExtraHeadingTarget() override;

	TESObjectREFR* pHeadingTarget;
};

// The seed a reference's random generation uses.
class ExtraSeed : public BSExtraData {
public:
	ExtraSeed(uint8_t aiSeed);
	~ExtraSeed() override;

	bool Compare(BSExtraData* apCompare) override;

	uint8_t iSeed;
	uint8_t padding0D[3];
};

// The general-topic info a reference's dialogue menu shows.
class ExtraInfoGeneralTopic : public BSExtraData {
public:
	ExtraInfoGeneralTopic();
	ExtraInfoGeneralTopic(MenuTopic* apItem);
	~ExtraInfoGeneralTopic() override;

	MenuTopic* pInfoGen;
};

// The normal of the distant terrain a reference stands on.
class ExtraDistantData : public BSExtraData {
public:
	ExtraDistantData();

	NiPoint3 kNormal;
};

// The regions a reference is in.
class ExtraRegionList : public BSExtraData {
public:
	ExtraRegionList(TESRegionList* apList);

	TESRegionList* pList;
};

// The references of the lit water a reference has.
class ExtraWaterLightRefs : public BSExtraData {
public:
	ExtraWaterLightRefs();

	bool Compare(BSExtraData* apCompare) override;
	~ExtraWaterLightRefs() override;

	// Copies apCopy's references into this extra.
	void Copy(BSExtraData* apCopy);
	void AddReference(TESObjectREFR* apRef);
	void ClearReferenceData();

	BSSimpleList<TESObjectREFR*> RefList;
};

// The cell node a cell's 3D hangs from.
class ExtraCell3D : public BSExtraData {
public:
	ExtraCell3D();
	~ExtraCell3D() override;

	NiPointer<NiNode> spCellNode;
};

// The multibound a reference sits in.
class ExtraMultiBound : public BSExtraData {
public:
	ExtraMultiBound();
	~ExtraMultiBound() override;

	NiPointer<BSMultiBound> spBound;
};

// The multibound room a reference sits in.
class ExtraRoom : public BSExtraData {
public:
	ExtraRoom();
	~ExtraRoom() override;

	NiPointer<BSMultiBoundRoom> spRoom;
};

// The reference that dropped the item.
class ExtraItemDropper : public BSExtraData {
public:
	ExtraItemDropper();
	~ExtraItemDropper() override;

	TESObjectREFR* pDropper;
};

// Marks a reference as a ghost.
class ExtraGhost : public BSExtraData {
public:
	ExtraGhost();
	~ExtraGhost() override;
};

// The world space or cell a reference started in.
class ExtraStartingWorldOrCell : public BSExtraData {
public:
	ExtraStartingWorldOrCell();
	~ExtraStartingWorldOrCell() override;

	TESForm* pStartingWorldOrCell;
};

// The reference that opened or closed this one.
class ExtraOpenCloseActivateRef : public BSExtraData {
public:
	ExtraOpenCloseActivateRef();
	~ExtraOpenCloseActivateRef() override;

	TESObjectREFR* pActivateRef;
};

// The ash pile left by a disintegrated reference.
class ExtraAshPileRef : public BSExtraData {
public:
	ExtraAshPileRef();
	~ExtraAshPileRef() override;

	TESObjectREFR* pAshPileRef;
};

// The sound a weapon's attack plays on the reference.
class ExtraWeaponAttackSound : public BSExtraData {
public:
	ExtraWeaponAttackSound(BSSoundHandle apSound);
	~ExtraWeaponAttackSound() override;

	BSSoundHandle pHandle;
};

// The editor's record of a moved reference: its angle and location, and the location it was moved from.
class ExtraEditorRefMoveData : public BSExtraData {
public:
	ExtraEditorRefMoveData(TESObjectREFR* apRef);
	~ExtraEditorRefMoveData() override;

	NiPoint3 realAngle;
	NiPoint3 realLocation;
	NiPoint3 oldLocation;
};

// The navmesh and triangle a door's portal leads to. Loaded and saved as the navmesh's form ID, which the pathfinder
// still reads as an ID at run time (NavMeshInfoSearch::GetNodeConnections passes the first member to
// NavMeshInfoMap::GetNavMeshInfo).
struct NAVMESH_PORTAL {
	NAVMESH_PORTAL();
	NAVMESH_PORTAL(const NAVMESH_PORTAL& aPortal);

	void Endian();

	union {
		uint32_t NavMeshID;
		NavMesh* pNavMesh;
	};

	uint16_t iTriIndex;
	uint8_t padding06[2];
};

// The navmesh portal a door reference connects.
class ExtraNavMeshPortal : public BSExtraData {
public:
	ExtraNavMeshPortal(NAVMESH_PORTAL aPortal);
	~ExtraNavMeshPortal() override;

	NAVMESH_PORTAL Portal;
};

// The room portal a portal reference is.
class ExtraPortal : public BSExtraData {
public:
	ExtraPortal();
	~ExtraPortal() override;

	NiPointer<BSPortal> spPortal;
};

// How many references keep a cell's processes at middle low.
class ExtraProcessMiddleLow : public BSExtraData {
public:
	ExtraProcessMiddleLow();
	~ExtraProcessMiddleLow() override;

	int32_t iRefCount;
};

// The markers of a furniture reference that actors have reserved.
class ExtraReservedMarkers : public BSExtraData {
public:
	ExtraReservedMarkers();
	~ExtraReservedMarkers() override;

	uint32_t iReservedMarkers;
};

using LeveledCreatureMultiplierSettings = Setting* [LCM_NONE];
extern LeveledCreatureMultiplierSettings pLevCreaModMult;

// The names of RADIO_RANGE_TYPE's values (BGSTalkingActivator.h), one per value below RADIO_RANGE_COUNT (5); no PC
// code reads them.
using RadioRangeTypeNameArray = const char* [5];
inline RadioRangeTypeNameArray pRadioRangeTypeNames{"Radius", "Everywhere", "Worldspace and Linked Interiors",
	"Linked Interiors", "Current Cell Only"};

// The game settings ExtraDataObjects.cpp defines.
extern SettingT<GameSettingCollection> iLockLevelMaxVeryHard;
extern SettingT<GameSettingCollection> sLockLevelNameVeryHard;
extern SettingT<GameSettingCollection> fLeveledLockMult;
extern SettingT<GameSettingCollection> fLeveledActorMultMedium;
extern SettingT<GameSettingCollection> fLeveledActorMultEasy;
extern SettingT<GameSettingCollection> sLockLevelNameVeryEasy;
extern SettingT<GameSettingCollection> iLockLevelMaxAverage;
extern SettingT<GameSettingCollection> fLeveledActorMultHard;
extern SettingT<GameSettingCollection> iLockLevelMaxImpossible;
extern SettingT<GameSettingCollection> iLockLevelMaxEasy;
extern SettingT<GameSettingCollection> sLockLevelNameImpossible;
extern SettingT<GameSettingCollection> iLockLevelMaxVeryEasy;
extern SettingT<GameSettingCollection> sLockLevelNameAverage;
extern SettingT<GameSettingCollection> sLockLevelNameEasy;
extern SettingT<GameSettingCollection> fLeveledActorMultBoss;
extern SettingT<GameSettingCollection> sLockLevelNameHard;
extern SettingT<GameSettingCollection> iLockLevelMaxHard;
