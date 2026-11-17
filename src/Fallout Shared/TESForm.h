#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
struct CHUNK;
struct FORM;
struct FORM_GROUP;
class TESBoundObject;
class TESFile;
class TESObjectREFR;

enum CHUNK_ID : uint32_t;

template <class T> class NiTLargePrimitiveArray;

uint16_t Min(uint16_t a1, uint16_t a2);

// One entry of the engine's table of form type names; `uiFormString` is the type's four-character record code.
struct FormEnumString {
	uint8_t cFormID;
	uint8_t padding01[3];
	char* pFormString;
	uint32_t uiFormString;
};

// The form types (TESForm::cFormType), by their record codes; FORM_ID_COUNT is the size of the engine's table.
enum ENUM_FORM_ID {
	NO_FORM = 0x0,
	TES4_ID = 0x1,
	GRUP_ID = 0x2,
	GMST_ID = 0x3,
	TXST_ID = 0x4,
	MICN_ID = 0x5,
	GLOB_ID = 0x6,
	CLAS_ID = 0x7,
	FACT_ID = 0x8,
	HDPT_ID = 0x9,
	HAIR_ID = 0xA,
	EYES_ID = 0xB,
	RACE_ID = 0xC,
	SOUN_ID = 0xD,
	ASPC_ID = 0xE,
	SKIL_ID = 0xF,
	MGEF_ID = 0x10,
	SCPT_ID = 0x11,
	LTEX_ID = 0x12,
	ENCH_ID = 0x13,
	SPEL_ID = 0x14,
	ACTI_ID = 0x15,
	TACT_ID = 0x16,
	TERM_ID = 0x17,
	ARMO_ID = 0x18,
	BOOK_ID = 0x19,
	CLOT_ID = 0x1A,
	CONT_ID = 0x1B,
	DOOR_ID = 0x1C,
	INGR_ID = 0x1D,
	LIGH_ID = 0x1E,
	MISC_ID = 0x1F,
	STAT_ID = 0x20,
	SCOL_ID = 0x21,
	MSTT_ID = 0x22,
	PWAT_ID = 0x23,
	GRAS_ID = 0x24,
	TREE_ID = 0x25,
	FLOR_ID = 0x26,
	FURN_ID = 0x27,
	WEAP_ID = 0x28,
	AMMO_ID = 0x29,
	NPC__ID = 0x2A,
	CREA_ID = 0x2B,
	LVLC_ID = 0x2C,
	LVLN_ID = 0x2D,
	KEYM_ID = 0x2E,
	ALCH_ID = 0x2F,
	IDLM_ID = 0x30,
	NOTE_ID = 0x31,
	COBJ_ID = 0x32,
	PROJ_ID = 0x33,
	LVLI_ID = 0x34,
	WTHR_ID = 0x35,
	CLMT_ID = 0x36,
	REGN_ID = 0x37,
	NAVI_ID = 0x38,
	CELL_ID = 0x39,
	REFR_ID = 0x3A,
	ACHR_ID = 0x3B,
	ACRE_ID = 0x3C,
	PMIS_ID = 0x3D,
	PGRE_ID = 0x3E,
	PBEA_ID = 0x3F,
	PFLA_ID = 0x40,
	WRLD_ID = 0x41,
	LAND_ID = 0x42,
	NAVM_ID = 0x43,
	TLOD_ID = 0x44,
	DIAL_ID = 0x45,
	INFO_ID = 0x46,
	QUST_ID = 0x47,
	IDLE_ID = 0x48,
	PACK_ID = 0x49,
	CSTY_ID = 0x4A,
	LSCR_ID = 0x4B,
	LVSP_ID = 0x4C,
	ANIO_ID = 0x4D,
	WATR_ID = 0x4E,
	EFSH_ID = 0x4F,
	TOFT_ID = 0x50,
	EXPL_ID = 0x51,
	DEBR_ID = 0x52,
	IMGS_ID = 0x53,
	IMAD_ID = 0x54,
	FLST_ID = 0x55,
	PERK_ID = 0x56,
	BPTD_ID = 0x57,
	ADDN_ID = 0x58,
	AVIF_ID = 0x59,
	RADS_ID = 0x5A,
	CAMS_ID = 0x5B,
	CPTH_ID = 0x5C,
	VTYP_ID = 0x5D,
	IPCT_ID = 0x5E,
	IPDS_ID = 0x5F,
	ARMA_ID = 0x60,
	ECZN_ID = 0x61,
	MESG_ID = 0x62,
	RGDL_ID = 0x63,
	DOBJ_ID = 0x64,
	LGTM_ID = 0x65,
	MUSC_ID = 0x66,
	IMOD_ID = 0x67,
	REPU_ID = 0x68,
	PCBE_ID = 0x69,
	RCPE_ID = 0x6A,
	RCCT_ID = 0x6B,
	CHIP_ID = 0x6C,
	CSNO_ID = 0x6D,
	LSCT_ID = 0x6E,
	MSET_ID = 0x6F,
	ALOC_ID = 0x70,
	CHAL_ID = 0x71,
	AMEF_ID = 0x72,
	CCRD_ID = 0x73,
	CMNY_ID = 0x74,
	CDCK_ID = 0x75,
	DEHY_ID = 0x76,
	HUNG_ID = 0x77,
	SLPD_ID = 0x78,
	FORM_ID_COUNT = 0x79,
};

// The save-game change flags passed to TESForm::AddChange; only some of the values are declared.
enum CHANGE_TYPE : uint32_t {
	CHANGE_FORM_FLAGS = 0x1,
	CHANGE_REFR_MOVE = 0x2,
	CHANGE_REFR_CELL_CHANGED = 0x8,
	CHANGE_REFR_SCALE = 0x10,
	CHANGE_REFR_INVENTORY = 0x20,
	CHANGE_REFR_LEVELED_INVENTORY = 0x8000000,
	CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS = 0x20000,
	CHANGE_ACTOR_DISPOSITION_MODIFIERS = 0x80000,
	CHANGE_ACTOR_PERMANENT_MODIFIERS = 0x800000,
	CHANGE_ACTOR_OVERRIDE_MODIFIERS = 0x400000,
	CHANGE_REFR_HAVOK_MOVE = 0x4,
	CHANGE_OBJECT_EXTRA_LOCK = 0x1000,
	// The door's teleport extra, which ExtraDataList::LoadGame reads.
	CHANGE_DOOR_EXTRA_TELEPORT = 0x20000,
	CHANGE_REFR_EXTRA_OWNERSHIP = 0x40,
	CHANGE_REFR_ANIMATION = 0x10000000,
	CHANGE_REFR_EXTRA_GAME_ONLY = 0x80000000,
	CHANGE_ACTOR_BASE_DATA = 0x2,
	CHANGE_ACTOR_BASE_ATTRIBUTES = 0x4,
	CHANGE_ACTOR_BASE_AIDATA = 0x8,
	CHANGE_ACTOR_BASE_FULLNAME = 0x20,
	CHANGE_BASE_OBJECT_FULLNAME = 0x4,
	CHANGE_CELL_FULLNAME = 0x4,
	CHANGE_NPC_SKILLS = 0x200,
	CHANGE_NPC_CLASS = 0x400,
	CHANGE_NPC_FACE = 0x800,
	CHANGE_NPC_GENDER = 0x1000000,
	CHANGE_NPC_RACE = 0x2000000,
	CHANGE_OBJECT_OPEN_DEFAULT_STATE = 0x400000,
	// A door's or open/close object's current open state.
	CHANGE_OBJECT_OPEN_STATE = 0x800000,
	CHANGE_TALKING_ACTIVATOR_SPEAKER = 0x800000,
	CHANGE_TOPIC_SAIDONCE = 0x80000000,
	CHANGE_CELL_SEENDATA = 0x80000000,
	CHANGE_CELL_DETACHTIME = 0x40000000,
	CHANGE_FACTION_FLAGS = 0x2,
	CHANGE_FACTION_CRIME_COUNTS = 0x80000000,
	CHANGE_BOOK_TEACHES_SKILL = 0x20,
	CHANGE_WATER_REMAPPED = 0x80000000,
	CHANGE_ENCOUNTER_ZONE_FLAGS = 0x2,
	CHANGE_ENCOUNTER_ZONE_GAME_DATA = 0x80000000,
	CHANGE_ACTOR_EXTRA_LEVELED_ACTOR = 0x40000,
	CHANGE_REFR_EXTRA_CREATED_ONLY = 0x40000000,
	CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN = 0x4000000,
	CHANGE_OBJECT_EMPTY = 0x200000,
	CHANGE_FORM_LIST_ADDED_FORM = 0x80000000,
	CHANGE_LEVELED_LIST_ADDED_OBJECT = 0x80000000,
	CHANGE_NOTE_READ = 0x80000000,
	CHANGE_QUEST_STAGES = 0x80000000,
	CHANGE_QUEST_SCRIPT = 0x40000000,
	CHANGE_QUEST_OBJECTIVES = 0x20000000,
	CHANGE_PACKAGE_NEVER_RUN = 0x80000000,
	CHANGE_PACKAGE_WAITING = 0x40000000,
	CHANGE_ACTOR_DAMAGE_MODIFIERS = 0x200000,
	CHANGE_ACTOR_TEMP_MODIFIERS = 0x100000,
	CHANGE_REFR_EXTRA_ENCOUNTER_ZONE = 0x20000000,
	CHANGE_OBJECT_EXTRA_ITEM_DATA = 0x400,
	CHANGE_OBJECT_EXTRA_AMMO = 0x800,
	CHANGE_CELL_EXTERIOR_CHAR = 0x20000000,
	CHANGE_CELL_EXTERIOR_SHORT = 0x10000000,
	CHANGE_ACTOR_LIFESTATE = 0x400,
	CHANGE_ACTOR_EXTRA_PACKAGE_DATA = 0x800,
};

using FormEnumStringTable = FormEnumString[FORM_ID_COUNT];

class TESForm : public BaseFormComponent {
public:
	// The iFormFlags bits.
	enum FormFlag : uint32_t {
		kFormFlag_Master = 0x1,
		kFormFlag_Altered = 0x2,
		kFormFlag_Initialized = 0x8,
		kFormFlag_Deleted = 0x20,
		kFormFlags_Known = 0x40,
		kFormFlags_80 = 0x80,
		kFormFlags_DroppedRef = 0x100,
		kFormFlags_CastShadows = 0x200,
		kFormFlags_QuestItem = 0x400,
		kFormFlags_IsRefPersistent = 0x400,
		kFormFlags_MotionBlur = 0x200,
		kFormFlag_Disabled = 0x800,
		kFormFlag_Ignored = 0x1000,
		kFormFlag_Temporary = 0x4000,
		kFormFlag_PartialForm = 0x4000,
		kFormFlag_MustBeVisibleDistant = 0x8000,
		kFormFlags_HavokDeath = 0x10000,
		kFormFlags_NeedToChangeProcess = 0x20000,
		FORM_FLAG_RANDOM_ANIM = 0x10000,
		FORM_FLAG_DANGEROUS = 0x20000,
		kFormFlags_Compressed = 0x40000,
		kFormFlags_Targeted = 0x40000,
		kFormFlag_HasTemp3D = 0x80000,
		kFormFlags_CenterRefOnCreation = 0x100000,
		kFormFlag_StillLoading = 0x200000,
		kFormFlag_Destroyed = 0x800000,
		kFormFlags_2000000 = 0x2000000,
		kFormFlags_IsVATSTargettable = 0x4000000,
		kFormFlags_DisableFade = 0x8000000,
		kFormFlags_10000000 = 0x10000000,
		kFormFlags_TalkingActivator_IsContinuousBroadcast = 0x40000000,
	};

	// the map type of pAllForms (an alias: a bare template comma would split the macro's arguments)
	using AllFormsMap = NiTPointerMap<uint32_t, TESForm*>;

	TESForm();
	virtual ~TESForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	virtual void InitializeData() {}
	virtual void ClearData() {}
	virtual bool ProcessBeforeSave() { return true; }
	// reads the form's record from apFile
	virtual bool Load(TESFile*) { return true; }
	virtual bool LoadPartial(TESFile*) { return true; }
	virtual void Save() {}
	virtual bool Save(TESFile* apFile);
	virtual bool LoadEdit(TESFile* apFile);
	virtual bool SaveEdit(TESFile* apFile);
	virtual bool SavesBefore(TESForm* apForm);
	virtual bool SavesBefore(FORM* apGroupFORM);
	virtual TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap);
	virtual void PostDuplicateProcess(NiTPointerMap<TESForm*, TESForm*>*) {}
	virtual void AddChange(int32_t aiChangeFlags);
	virtual void RemoveChange(int32_t aiChangeFlags);
	virtual uint16_t GetSaveSize(uint32_t aiFlags);
	virtual void SaveGame(uint32_t aiFlags);
	virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	virtual void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags);
	virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	virtual void InitLoadGame(uint32_t, uint32_t) {}
	virtual void InitLoadGame(BGSLoadFormBuffer*) {}
	virtual void FinishInitLoadGame(uint32_t, uint32_t) {}
	virtual void Revert(uint32_t aiFlags);
	virtual void Revert(BGSLoadFormBuffer*) {}
	virtual void LoadQueuedFormData(uint16_t) {}
	virtual bool FindInFileFast(TESFile*) { return false; }
	virtual void CheckSaveGame(BGSSaveFormBuffer*) {}
	virtual void FinishLoadGame(BGSLoadFormBuffer*) {}
	// finishes the form once every data file is loaded
	virtual void InitItem() {}
	// The type the form is saved under (TESObjectREFR's override maps a reference to its base's reference type).
	virtual ENUM_FORM_ID GetSavedFormType() { return GetFormType(); }
	virtual void GetFormDetailedString(BSStringT<char>& aString);
	virtual bool GetQuestObject();
	virtual bool GetHasSpokenFlag();
	virtual bool GetHavokDeath();
	virtual bool GetRandomAnim();
	virtual bool GetNeedtoChangeProcess();
	virtual bool GetDangerous();
	virtual bool GetHasPLSpecTex();
	virtual bool GetObstacle();
	virtual bool GetContinuousBroadcast();
	virtual bool GetOnLocalMap();
	virtual void SetOnLocalMap(bool abVal);
	virtual NiColor* GetEmittanceColor() { return nullptr; }
	virtual void SetDelete(bool abDeleted);
	virtual void SetAltered(bool abAltered);
	virtual void SetQuestObject(bool abQuest);
	virtual void SetHasSpokenFlag(bool abSpoke);
	virtual void SetHavokDeath(bool abDeath);
	virtual void SetNeedToChangeProcess(bool abChange);
	virtual void SaveObjectBound() {}
	virtual void LoadObjectBound(TESFile*) {}
	virtual bool IsBoundObject() { return false; }
	virtual bool IsObject() { return false; }
	virtual bool IsMagicItem() { return false; }
	virtual bool IsReference() { return false; }
	virtual bool IsArmorAddon() { return false; }
	virtual bool IsActorBase() { return false; }
	virtual bool IsMobileObject() { return false; }
	virtual bool IsActor() { return false; }
	virtual uint32_t GetRefCount() { return 0; }
	virtual void Copy(TESForm* apCopy);
	virtual bool Compare(TESForm* apCopy);
	virtual bool BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly);
	virtual void CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup);
	virtual bool IsParentForm() { return false; }
	virtual bool IsParentFormTree() { return false; }
	virtual bool IsFormTypeChild(uint8_t) { return false; }
	virtual bool Activate(TESObjectREFR*, TESObjectREFR*, bool, TESBoundObject*, int32_t) { return false; }
	virtual void SetFormID(uint32_t aiID, bool abUpdateFile);
	virtual const char* GetObjectTypeName() { return ""; }
	// Returns an empty string; overrides call this base version.
	virtual const char* GetFormEditorID();
	virtual bool SetFormEditorID(const char*) { return true; }

	// Loads aiSize bytes of the form's data from the old save format, passing the buffer on to
	// TESSaveLoadGame::LoadGameDataOLD. This void* overload is used for all element types (bytes, floats, strings).
	void LoadGameDataOLD(void* apData, int32_t aiSize);
	// Sets or clears the VATS target override form flag (0x4000000), then marks change flag 1.
	void SetVATSTargetOverride(bool abVal);
	// Returns cFormType, zero-extended.
	ENUM_FORM_ID GetFormType() const;
	void SetFormType(uint8_t aType);
	TESFile* GetFile(int32_t aiIndex) const;
	// The length of the editor ID, 0 while a call is already in progress.
	uint32_t GetFormEditorIDLength();
	// The list of files that define or override the form: &pSourceFiles.
	BSSimpleList<TESFile*>* GetFileList();
	// Empties the list of files that define or override the form.
	void ClearFiles();

	uint32_t GetFormFlags() const;
	void SetFormFlags(uint32_t aiFlags);
	bool GetMaster() const;
	void SetMaster(bool abMaster);
	bool GetAltered() const;
	bool GetInitialized() const;
	void SetInitialized(bool abInit);
	bool GetDelete() const;
	bool GetDisabled() const;
	bool GetDisabledOrDeleted() const;
	bool GetTemporary() const;
	bool GetMustBeVisibleDistant() const;
	bool IsStillLoading() const;
	void SetStillLoading(bool abVal);
	bool GetDestroyed() const;

	bool GetNoCollision() const;
	// Sets or clears form flag 0x10.
	void SetNoCollision(bool abNoCollision);
	bool GetLODTree() const;
	bool GetDestructible() const;
	void SetResetDestruct(bool abVal);
	// Sets the empty flag of a harvested form.
	void SetEmpty(bool abEmpty);
	bool GetDisableFade() const;
	bool GetFireOff() const;
	bool GetReflectedByAutoWater();
	bool GetRefractedByAutoWater();
	// Sets or clears form flag 0x80000 of a base form (TESNPC, TESCreature).
	void SetHasTemp3D(bool abVal);

	// Sets or clears kFormFlag_Destroyed, then marks change flag 1.
	void SetDestroyed(bool abVal);
	void SetDestructible(bool abVal);
	void SetDisabled(bool abDisabled);

	uint32_t GetFileCount() const;
	TESFile* GetOwnerMaster() const;
	void LoadForm(TESFile* apFile);
	// Reads the current chunk of apFile into apData, asSize bytes.
	void LoadData(TESFile* apFile, void* apData, uint16_t asSize);

	void ForceChange(int32_t aiChangeFlags);

	uint32_t GetFormID() const;
	uint32_t GetFormIDWithoutIndex() const;
	bool GetEmpty() const;
	// Marks the form temporary and drops it from the form tables and the save changes.
	void SetTemporary();
	// Removes the form from the form map, the altered forms and the data handler, unless it is temporary.
	void RemoveFromDataStructures();
	// Clears, then initialises, the data of every component the form type has.
	void ClearDataAllComponents();
	void InitializeDataAllComponents();
	// Records apFile as the file the form was last loaded from.
	void SetFile(TESFile* apFile);
	// The bytes the form's value, health, weight, quality, damage, attribute and clip components add to its data.
	uint32_t GetDataSizeAdded();
	// The name of the form's type (formEnumString[cFormType].pFormString).
	const char* GetFormTypeString() const;
	// Whether this form is a built-in form: IsDefaultForm(GetFormID()).
	bool IsDefaultForm();
	void __SaveData(void* apData, uint32_t aiSize);
	void SaveNumericID(uint32_t* apNumericID, int32_t aiSize);
	void LoadNumericID(uint32_t* apNumericID, int32_t aiSize);
	void SaveData();
	void StartForm();
	void CopyAllComponents(TESForm* apCopy);
	bool CompareAllComponents(TESForm* apCompare);
	// Appends apData's aiSize bytes to the save buffer of the global save/load game.
	void SaveGameDataOLD(const void* apData, int32_t aiSize);
	// Finishes the record StartForm began.
	void CloseForm();
	void SetFireOff(bool abVal);
	bool FormIDMatchesWithoutIndex(uint32_t aiID) const;
	bool IsRecentPCDrop() const;
	bool GetVATSTargetOverride() const;
	void* GetFormBuffer();
	uint32_t GetFormBufferSize();
	void FreeFormBuffer();
	int32_t GetLODFormID();
	void SetDisableFade(bool abVal);
	bool ExpandChunk(CHUNK* apChunk, uint16_t asExpandSize);
	void SetInPlaceableWater(bool abVal);
	void SetRecentPCDrop(bool abVal);

	// The name of form type aiFormType.
	static const char* GetFormTypeString(uint8_t aiFormType);

	// the engine's built-in forms (player, gold, ...) have IDs 1 to 0x7FF
	static bool IsDefaultForm(uint32_t aiID);
	static TESForm* GetFormByNumericID(uint32_t aiSearchID);

	static ENUM_FORM_ID GetFormTypeFromFormString(uint32_t aiFormString);
	static TESForm* GetFormByEditorID(const char* apSearchID);
	static void AddCompileIndex(uint32_t& aiID, TESFile* apFile);
	static void AddChunkArray(CHUNK_ID aeChunkID, const char* apData, uint32_t aiSize);
	static bool QEndianSwapOnSave();
	// Appends a chunk header and aiSize bytes of apData to the form buffer.
	static void __AddChunkData(CHUNK_ID aeChunkID, const void* apData, uint32_t aiSize);
	// Adds a one-byte chunk.
	static void AddChunk(CHUNK_ID aeChunkID, uint8_t aiData);
	// Adds a two-byte chunk, swapped when saving byte-swapped.
	static void AddChunk(CHUNK_ID aeChunkID, uint16_t asData);
	// Adds a four-byte chunk, swapped when saving byte-swapped.
	static void AddChunk(CHUNK_ID aeChunkID, uint32_t aiData);
	static void AddChunk(CHUNK_ID aeChunkID, float afData);
	static void AddChunk(CHUNK_ID aeChunkID, int32_t aiData);
	static void AddChunk(CHUNK_ID aeChunkID);
	// Whether apForm is an interior cell or a worldspace's interior.
	static bool IsInteriorCellOrWorldspace(TESForm* apForm);
	// The length of apForm's full name, 0 when it has no
	// TESFullName.
	static uint32_t GetNameLen(TESForm* apForm);
	static void AddChunkArray(CHUNK_ID aeChunkID, const uint32_t* apData, uint32_t aiCount);
	static void AddChunkArray(CHUNK_ID aeChunkID, const float* apData, uint32_t aiCount);
	// The 2-byte overload: passes the array on to AddChunkArray16.
	static void AddChunkArray(CHUNK_ID aeChunkID, const uint16_t* apData, uint32_t aiCount);
	static void AddChunkArray(CHUNK_ID aeChunkID, const int16_t* apData, uint32_t aiCount);
	static void CompressSaveBuffer();

protected:
	// Adds a chunk of aiCount 4-byte values, swapped on a copy when saving byte-swapped.
	static void AddChunkArray32(CHUNK_ID aeChunkID, const void* apData, uint32_t aiCount);
	// Adds a chunk of aiCount 2-byte values, swapped on a copy when saving byte-swapped.
	static void AddChunkArray16(CHUNK_ID aeChunkID, const void* apData, uint32_t aiCount);
	// Initializes the global form maps and altered-form array.
	static void InitializeFormDataStructures();

public:
	static void ReleaseFormDataStructures();

	uint8_t cFormType;
	uint8_t padding05[1];
	uint8_t unk6;
	uint8_t padding07[1];
	uint32_t iFormFlags;
	uint32_t iFormID;
	// the files that define or override this form, in load order
	BSSimpleList<TESFile*> pSourceFiles;

	// the last record code looked up by GetFormTypeFromFormString and the type it mapped to
	static inline uint32_t iLastFormString;
	static inline int32_t iLastFormType;
	// every form by form ID, and every form that has an editor ID by that ID
	static inline AllFormsMap* pAllForms;
	static inline BSTCaseInsensitiveStringMap<TESForm*>* pAllFormsByEditorID;
	// whether form IDs were rewritten (cleared when the data files are compiled)
	static inline bool bFormIDsBashed;
	static inline bool bEndianSwapOnSave;
	// every form with unsaved changes
	static inline NiTLargePrimitiveArray<TESForm*>* pAlteredForms;
	// Set while GetFormEditorIDLength reads the editor ID, so a re-entrant call returns 0. One per thread: other
	// threads read names at the same time, and a shared flag would make their calls return 0.
	static thread_local bool bIsReadingName;
	static inline FORM* pFormBuffer;
	static inline uint32_t iBufferSize;
	static inline uint32_t iTotalFormCount;
};

inline FormEnumStringTable formEnumString{{0, {}, const_cast<char*>("NONE")}, {1, {}, const_cast<char*>("TES4")},
	{2, {}, const_cast<char*>("GRUP")}, {3, {}, const_cast<char*>("GMST")}, {4, {}, const_cast<char*>("TXST")},
	{5, {}, const_cast<char*>("MICN")}, {6, {}, const_cast<char*>("GLOB")}, {7, {}, const_cast<char*>("CLAS")},
	{8, {}, const_cast<char*>("FACT")}, {9, {}, const_cast<char*>("HDPT")}, {10, {}, const_cast<char*>("HAIR")},
	{11, {}, const_cast<char*>("EYES")}, {12, {}, const_cast<char*>("RACE")}, {13, {}, const_cast<char*>("SOUN")},
	{14, {}, const_cast<char*>("ASPC")}, {15, {}, const_cast<char*>("SKIL")}, {16, {}, const_cast<char*>("MGEF")},
	{17, {}, const_cast<char*>("SCPT")}, {18, {}, const_cast<char*>("LTEX")}, {19, {}, const_cast<char*>("ENCH")},
	{20, {}, const_cast<char*>("SPEL")}, {21, {}, const_cast<char*>("ACTI")}, {22, {}, const_cast<char*>("TACT")},
	{23, {}, const_cast<char*>("TERM")}, {24, {}, const_cast<char*>("ARMO")}, {25, {}, const_cast<char*>("BOOK")},
	{26, {}, const_cast<char*>("CLOT")}, {27, {}, const_cast<char*>("CONT")}, {28, {}, const_cast<char*>("DOOR")},
	{29, {}, const_cast<char*>("INGR")}, {30, {}, const_cast<char*>("LIGH")}, {31, {}, const_cast<char*>("MISC")},
	{32, {}, const_cast<char*>("STAT")}, {33, {}, const_cast<char*>("SCOL")}, {34, {}, const_cast<char*>("MSTT")},
	{35, {}, const_cast<char*>("PWAT")}, {36, {}, const_cast<char*>("GRAS")}, {37, {}, const_cast<char*>("TREE")},
	{38, {}, const_cast<char*>("FLOR")}, {39, {}, const_cast<char*>("FURN")}, {40, {}, const_cast<char*>("WEAP")},
	{41, {}, const_cast<char*>("AMMO")}, {42, {}, const_cast<char*>("NPC_")}, {43, {}, const_cast<char*>("CREA")},
	{44, {}, const_cast<char*>("LVLC")}, {45, {}, const_cast<char*>("LVLN")}, {46, {}, const_cast<char*>("KEYM")},
	{47, {}, const_cast<char*>("ALCH")}, {48, {}, const_cast<char*>("IDLM")}, {49, {}, const_cast<char*>("NOTE")},
	{50, {}, const_cast<char*>("COBJ")}, {51, {}, const_cast<char*>("PROJ")}, {52, {}, const_cast<char*>("LVLI")},
	{53, {}, const_cast<char*>("WTHR")}, {54, {}, const_cast<char*>("CLMT")}, {55, {}, const_cast<char*>("REGN")},
	{56, {}, const_cast<char*>("NAVI")}, {57, {}, const_cast<char*>("CELL")}, {58, {}, const_cast<char*>("REFR")},
	{59, {}, const_cast<char*>("ACHR")}, {60, {}, const_cast<char*>("ACRE")}, {61, {}, const_cast<char*>("PMIS")},
	{62, {}, const_cast<char*>("PGRE")}, {63, {}, const_cast<char*>("PBEA")}, {64, {}, const_cast<char*>("PFLA")},
	{65, {}, const_cast<char*>("WRLD")}, {66, {}, const_cast<char*>("LAND")}, {67, {}, const_cast<char*>("NAVM")},
	{68, {}, const_cast<char*>("TLOD")}, {69, {}, const_cast<char*>("DIAL")}, {70, {}, const_cast<char*>("INFO")},
	{71, {}, const_cast<char*>("QUST")}, {72, {}, const_cast<char*>("IDLE")}, {73, {}, const_cast<char*>("PACK")},
	{74, {}, const_cast<char*>("CSTY")}, {75, {}, const_cast<char*>("LSCR")}, {76, {}, const_cast<char*>("LVSP")},
	{77, {}, const_cast<char*>("ANIO")}, {78, {}, const_cast<char*>("WATR")}, {79, {}, const_cast<char*>("EFSH")},
	{80, {}, const_cast<char*>("TOFT")}, {81, {}, const_cast<char*>("EXPL")}, {82, {}, const_cast<char*>("DEBR")},
	{83, {}, const_cast<char*>("IMGS")}, {84, {}, const_cast<char*>("IMAD")}, {85, {}, const_cast<char*>("FLST")},
	{86, {}, const_cast<char*>("PERK")}, {87, {}, const_cast<char*>("BPTD")}, {88, {}, const_cast<char*>("ADDN")},
	{89, {}, const_cast<char*>("AVIF")}, {90, {}, const_cast<char*>("RADS")}, {91, {}, const_cast<char*>("CAMS")},
	{92, {}, const_cast<char*>("CPTH")}, {93, {}, const_cast<char*>("VTYP")}, {94, {}, const_cast<char*>("IPCT")},
	{95, {}, const_cast<char*>("IPDS")}, {96, {}, const_cast<char*>("ARMA")}, {97, {}, const_cast<char*>("ECZN")},
	{98, {}, const_cast<char*>("MESG")}, {99, {}, const_cast<char*>("RGDL")}, {100, {}, const_cast<char*>("DOBJ")},
	{101, {}, const_cast<char*>("LGTM")}, {102, {}, const_cast<char*>("MUSC")}, {103, {}, const_cast<char*>("IMOD")},
	{104, {}, const_cast<char*>("REPU")}, {105, {}, const_cast<char*>("PCBE")}, {106, {}, const_cast<char*>("RCPE")},
	{107, {}, const_cast<char*>("RCCT")}, {108, {}, const_cast<char*>("CHIP")}, {109, {}, const_cast<char*>("CSNO")},
	{110, {}, const_cast<char*>("LSCT")}, {111, {}, const_cast<char*>("MSET")}, {112, {}, const_cast<char*>("ALOC")},
	{113, {}, const_cast<char*>("CHAL")}, {114, {}, const_cast<char*>("AMEF")}, {115, {}, const_cast<char*>("CCRD")},
	{116, {}, const_cast<char*>("CMNY")}, {117, {}, const_cast<char*>("CDCK")}, {118, {}, const_cast<char*>("DEHY")},
	{119, {}, const_cast<char*>("HUNG")}, {120, {}, const_cast<char*>("SLPD")}};
// The "Normal" text shared by every file that uses "Normal". It is a global because its address is compared and stored
// (NiTexturingProperty's map names, TESScriptFunctions' state names, sNormal's default, Normal::Name).
using NormalText = const char[7];
inline NormalText aNormal_0{"Normal"};

inline bool bEnumStringsChecked;
