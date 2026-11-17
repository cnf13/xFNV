#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/SaveDataDebugInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class ChangeData;
class ChangesMap;
class CreatedReferenceData;
class ExteriorCellNewReferencesMap;
struct FormAndFlags;
class InteriorCellNewReferencesMap;
class LoadFormHeader;
class MovedReferenceData;
template <class T> class NiTLargePrimitiveArray;
class NumericIDBufferMap;
struct SaveBufferHeader;
class TESForm;
class TESObjectCELL;
class TESObjectREFR;
class TESPackage;

// A created exterior cell: its form ID and grid coordinates, 12 bytes (TESSaveLoadGame::GetCreatedExteriorCellFormID
// returns +0 of the entry whose +4 and +8 match).
struct ExteriorCellReferenceData {
	uint32_t iFormID;
	int32_t iCellX;
	int32_t iCellY;
};

// One form's recorded save-game changes: the change flags and the buffer of their data.
class ChangeData {
public:
	ChangeData();
	~ChangeData();

	// Clears the aiChange flags, unless the changes already have a buffer.
	void RemoveChange(uint32_t aiChange);
	// pBuffer; the exe's shared one-field getter 0x726070 (thiscall, ret 0).
	void* GetBuffer();
	// iFlags; the exe's shared one-field getter 0x559450 (thiscall, ret 0).
	uint32_t GetFlags();
	// Sets pBuffer; the exe's shared one-field setter 0x6ECD40 (thiscall, ret 4).
	void SetBuffer(void* apBuffer);
	void AddChange(uint32_t aiChange);

	// The exe's deleting destructor (0x854FE0) frees through ourFree.
	static void operator delete(void* apMemory);

	uint32_t iFlags;
	// The PDB names this member without a type this tool reports; IDA has no struct for the class.
	void* pBuffer;
};

// The header the save writes in front of each changed form; packed, so iFlags sits unaligned at +5.
#pragma pack(push, 1)
struct SaveFormHeader {
	uint32_t iFormID;
	uint8_t cFormType;
	uint32_t iFlags;
	uint8_t cVersion;
};
#pragma pack(pop)

// The header the save reads in front of each changed form: SaveFormHeader followed by the size of the form's data.
// Packed, like SaveFormHeader. Declared `class` to match the forward declarations src already has.
#pragma pack(push, 1)
class LoadFormHeader {
public:
	LoadFormHeader() = default;
	LoadFormHeader(const SaveBufferHeader* apBufferHeader, uint32_t aiFormID, uint32_t aiFlags);

	uint32_t iFormID;
	uint8_t cFormType;
	uint32_t iFlags;
	uint8_t cVersion;
	uint16_t iSize;
};
#pragma pack(pop)

// Not named in the PDB or IDA; name chosen here. The four bytes in front of each changed form's data in the save
// buffer (the PDB's `bufferHeader`): the data size, the form type and the version.
struct SaveBufferHeader {
	uint16_t iSize;
	uint8_t cFormType;
	uint8_t cVersion;
};

// A loaded form waiting for its initialisation, with its change flags and save version.
struct FormAndFlags {
	FormAndFlags(TESForm* apForm, uint32_t aiFlags, uint32_t aiOldFlags, uint8_t acVersion);

	TESForm* pForm;
	uint32_t iFlags;
	uint32_t iOldFlags;
	uint8_t cVersion;
};

// A map from a numeric ID to a buffer the save-game code queues for loading. It adds only its destructor to
// NiTPointerMap.
class NumericIDBufferMap : public NiTPointerMap<uint32_t, void*> {
public:
	NumericIDBufferMap();
	~NumericIDBufferMap() override;

	// The exe's deleting destructor (0x855580) frees through ourFree.
	static void operator delete(void* apMemory);
};

// The references new to each interior cell, as lists of form IDs keyed by the cell's form ID.
class InteriorCellNewReferencesMap : public NiTPointerMap<uint32_t, BSSimpleList<uint32_t>*> {
public:
	InteriorCellNewReferencesMap();
	~InteriorCellNewReferencesMap() override;

	// The exe's deleting destructor (0x8552E0) frees through ourFree.
	static void operator delete(void* apMemory);
};

// The references new to each exterior cell, as lists of reference data keyed by the cell's coordinates.
class ExteriorCellNewReferencesMap : public NiTPointerMap<uint32_t, BSSimpleList<ExteriorCellReferenceData*>*> {
public:
	ExteriorCellNewReferencesMap();
	~ExteriorCellNewReferencesMap() override;

	// The exe's deleting destructor (0x855410) frees through ourFree.
	static void operator delete(void* apMemory);
};

// The exe's out-of-line constructors of the save loader's lists-by-ID maps, and their vtables (the PDB's
// NiTMapBase<NiTPointerAllocator<unsigned int>,unsigned int,BSSimpleList<unsigned int> *>,
// NiTMapBase<NiTPointerAllocator<unsigned int>,unsigned int,BSSimpleList<struct ExteriorCellReferenceData *> *> and
// NiTMapBase<NiTPointerAllocator<unsigned int>,unsigned char,BSSimpleList<struct LoadFormHeader *> *>). Defined in
// TESSaveLoadGame.cpp.
using NiTMapBaseUIntBSSimpleListUInt = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BSSimpleList<uint32_t>*>;
using NiTMapBaseUIntBSSimpleListExteriorCellReferenceData =
	NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BSSimpleList<ExteriorCellReferenceData*>*>;
using NiTMapBaseUInt8BSSimpleListLoadFormHeader =
	NiTMapBase<NiTPointerAllocator<size_t>, uint8_t, BSSimpleList<LoadFormHeader*>*>;

// An expired exterior cell's grid coordinates, 8 bytes (the exe allocates 8 and copies the cell's X and Y into it).
struct ExpiredCellData {
	int32_t iCellX;
	int32_t iCellY;
};

// The expired cell data map (a cell's data keyed by the cell's form ID): its vtables and the exe's out-of-line
// constructor and destructors, which write them. Defined in TESSaveLoadGame.cpp.
using NiTMapBaseDFALLUIntExpiredCellDataList =
	NiTMapBase<DFALL<NiTMapItem<uint32_t, BSSimpleList<ExpiredCellData*>*>>, uint32_t, BSSimpleList<ExpiredCellData*>*>;
using NiTMapUIntExpiredCellDataList = NiTMap<uint32_t, BSSimpleList<ExpiredCellData*>*>;

// A save game file: a BSFile that remembers the file time of the save it was read from. Only the members the
// reconstructed functions use are declared.
class SaveGameFile : public BSFile {
public:
	SaveGameFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiBufferSize);
	~SaveGameFile() override;

	// Sets the file time and marks it present. The exe leaves this in eax and its caller (0x85FC90) reads it, so
	// the body returns this.
	SaveGameFile* SetFileTime(_FILETIME aFileTime);
	_FILETIME GetFileTime();
	bool GetHasFileTime();

	// The exe's deleting destructor (0x85FB80) frees through ourFree.
	static void operator delete(void* apMemory);

	bool bHasFileTime;
	uint8_t padding159[3];
	_FILETIME FileTime;
};

// A save's player summary: the name, level, game date and playing time the load menu shows.
struct SavedPlayerData {
	char pPlayerName[260];
	uint16_t iLevel;
	uint8_t padding106[2];
	float fGameDate;
	uint32_t iTotalPlayingTime;
};

// The original engine's save-game manager, which this build still keeps alongside BGSSaveLoadGame. It is mostly
// reached through the calls below.
class TESSaveLoadGame {
public:
	// What a file is opened for (a dword; CleanupLoadSave 0x8578B0 switches on it, 0 to 3).
	enum ACTION_TYPE : int32_t {
		ACTION_SAVE = 0,
		ACTION_LOAD = 1,
		ACTION_READ = 2,
		ACTION_DELETE = 3,
		ACTION_TOTAL = 4,
	};

	TESSaveLoadGame();
	~TESSaveLoadGame();

	// Writes to apString the names of the aiFlags changes of apForm, a form of type
	// acFormType.
	void BuildChangesString(char* apString, TESForm* apForm, uint32_t aiFlags, uint8_t acFormType, bool abVerbose);
	// Opens apFile for a load or save.
	BSFile* PrepForLoadSave(BSFile* apFile, const char* asFileName, ACTION_TYPE aeAction);
	// Closes apFile after a load or save.
	void CleanupLoadSave(BSFile* apFile, ACTION_TYPE aeAction);
	// Allocates a buffer of aiSize bytes under the save-load heap context.
	void* CreateBuffer(uint32_t aiSize);
	// Frees apBlock.
	void DestroyBuffer(void* apBuffer);
	void DeleteForm(TESForm* apForm);
	uint32_t ReadFile(BSFile* apFile, void* apData, uint32_t aiSize);
	uint32_t WriteFile(BSFile* apFile, const void* apData, uint32_t aiSize);
	// Whether a save is being loaded. In this build the exe's body is the shared
	// "return false" stub (0x47C850). thiscall, ret 0.
	bool GetLoading();
	// In this build the exe's body is the shared empty stub (0x4534F0). thiscall, ret 4.
	void SetLoading(bool abValue);
	// Applies the save's changes to apForm.
	bool LoadForm(TESForm* apForm);
	// Sets m_pCurrentlyLoadingFormHeader; the exe's shared one-field setter 0x4FB090
	// (thiscall, ret 4).
	void SetCurrentlyLoadingFormHeader(LoadFormHeader* apHeader);
	// Loads apForm's initial data (position, cell, ...) for its change flags aiFlags.
	void LoadInitialData(TESForm* apForm, uint32_t aiFlags);
	// Loads the save's data of apCell.
	bool LoadCell(TESObjectCELL* apCell);
	// Initialises the forms of apInitArray.
	void InitForms(NiTLargePrimitiveArray<FormAndFlags*>* apInitArray, FormAndFlags* apPlayerFlags,
		bool abForceLoadModels);
	// Runs the queued animation, controller and Havok loads.
	void LoadQueuedData(bool abUpdateWorld);
	bool GetInitArrayExists();
	// thiscall, ret 0.
	void StartLoadingSequence();
	// Queues apForm for initialisation after the load. thiscall, ret 4.
	void AddInitItem(TESForm* apForm);
	// In this build the exe's body is the shared "return false" stub (0x8D0370). thiscall,
	// ret 4.
	bool UnloadForm(TESForm* apForm);
	// Drops the save's changes of apForm.
	void RemoveChanges(TESForm* apForm, bool abForceRemove);
	void SaveNumericID(uint32_t* apFormID, uint32_t aiSize);
	bool LoadNumericID(uint32_t* apFormID, uint32_t aiSize);
	// A new package of type cPackType with the save's form ID aiFormID, for a package the
	// save created.
	TESPackage* CreatePackage(uint32_t aiFormID, uint8_t cPackType);
	bool UseSaveGameBlocks();
	void LoadGameDataOLD(void* apData, uint32_t aiSize);
	// Runs the save/load test over every cell.
	void TestAllCells(uint32_t aiControl);
	// The header of the form being saved (m_pCurrentlySavingFormHeader, +0x88); the exe
	// calls the engine's shared getter of +0x88 (thiscall, 0x4FD3E0).
	SaveFormHeader* GetCurrentlySavingFormHeader();
	// The header of the form being loaded (m_pCurrentlyLoadingFormHeader, +0x84); the exe
	// calls the engine's shared getter of +0x84, the same function as InterfaceManager::GetCursorRoot (thiscall,
	// ret 0, 0x4FD3C0).
	LoadFormHeader* GetCurrentlyLoadingFormHeader();
	// IDA name; not in the PDB. Whether m_iRevertState (+0x48) is 0; the exe tests it twice (thiscall, ret 0,
	// 0x55F5B0).
	bool IsUnk048Zero();
	// m_bReconstructedAllObjects.
	bool GetReconstructedAllObjects() { return m_bReconstructedAllObjects; }
	// Writes aiSize bytes of apData to the old-format save.
	void SaveGameDataOLD(const void* apData, uint32_t aiSize);
	// The write position in the save buffer, m_pBuffer (+0x14); the exe calls the shared
	// one-field getter 0x825C00 (thiscall, ret 0).
	void* GetBufferPointer();
	// True when a coordinate of apPoint is infinite or NaN.
	bool IsInvalidPoint(NiPoint3* apPoint);
	// Puts a reference with an infinite or NaN location back at its editor or starting place (or the origin), and
	// zeroes an invalid angle, warning about each.
	void SafetyCheckReferencePositionAndAngle(TESObjectREFR* apRef);
	void QueueBufferLoad(NumericIDBufferMap* apMap, uint32_t aiFormID, uint16_t aiSize);
	void QueueRemoveChange(uint32_t aiChange);
	void ResetVersions();
	bool GetSavingAllowed();
	uint32_t GetNumericIDFromArray(uint32_t aiFormIDIndex);
	uint8_t GetCurrentVersion();
	// Queues apForm for deletion once the load ends.
	void AddFormToDeferredDeletionsList(TESForm* apForm);
	void LoadBuffer(BSFile* apFile, void* apBuffer, uint32_t aiSize);
	void QueueAttachedAnimationLoad(TESObjectREFR* apRef, uint16_t aiSize);
	void QueueCharControllerLoad(TESObjectREFR* apRef, uint16_t aiSize);
	void QueueHavokDataLoad(TESObjectREFR* apRef, uint16_t aiSize);
	char* GetMostRecentSaveGame();
	void AdvanceBuffer(uint32_t aiSize);
	void ClearFormID(uint32_t aiFormID);
	void QueueAnimationLoad(TESObjectREFR* apRef, uint16_t aiSize);
	void SetCurrentVersion(uint8_t acVersion);
	void ResetCurrentVersion();
	void RemoveFormFromDeferredDeletionList(TESForm* apForm);
	// Allocates aiSize + 2 bytes with ourMalloc and stores aiSize in the first two.
	void* CreateQueuedBuffer(uint16_t aiSize);
	// Name not in the PDB (the field at +0x81); folded accessor, IDA: MobileObject::IsTalkingActivator.
	bool GetUseNumericIDArray();
	TESForm* CreateReferenceFromFile(uint32_t aiFormID, MovedReferenceData* apMovedData);
	uint32_t AddNumericIDToArray(uint32_t aiFormID);
	bool SaveGame(BSFile* pFile, const char* apFileName, bool abLogStats);
	bool ResetReferencePosition(TESObjectREFR* apRef, bool abTemporaryMove);
	void SaveGlobalData(BSFile* apFile);
	void SaveFinalData(BSFile* a1);
	uint32_t CheckFlags(TESForm* apForm, uint32_t aiFlags);
	void CreateChangedReference(uint32_t aiReferenceID);
	// The exe's SaveGame passes the form it saves and its change flags as a whole dword (0x857065). The Xbox build's
	// also saves a cell's data; the PC exe's only dynamic-casts a form that is no reference to a cell.
	void SaveInitialData(TESForm* apForm, uint32_t aiFlags);
	// Not named in the PDB or IDA; name chosen here. The extra save size of apForm for the change flags aiFlags
	// (thiscall, ret 8: the exe's SaveGame and BuildChangesString pass the form and the flags dword).
	int32_t GetInitialDataSaveSize(TESForm* apForm, uint32_t aiFlags);
	void SavePluginList(BSFile* apFile);
	void SaveHeader(BSFile* apFile, const char* apFileName);
	uint32_t GetCreatedExteriorCellFormID(uint32_t aiWorldSpaceID, int32_t aiCellX, int32_t aiCellY);
	void SaveNumericIDArrays(BSFile* a1);
	// Keeps the name of the save apFile (thiscall, ret 4; SaveGame ignores eax).
	void SetMostRecentSaveGame(BSFile* apFile);
	// Drops the saved changes of references left in expired cells.
	void CleanUpExpiredData();

protected:
	void LoadAttachedAnimationForReference(TESObjectREFR* apRef, void* apBuffer);
	// Reads the two-byte size at apBuffer through the engine's save-load object and loads apRef's havok state.
	void LoadHavokDataForReference(TESObjectREFR* apRef, void* apBuffer);
	// Finds the highest save number among the save games and stores the next one in
	// m_iNextSaveNumber, which it returns.
	uint32_t CalculateNextSaveNumber();
	uint32_t GetNextSaveNumber();
	void SaveBuffer(BSFile* apFile, void* apBuffer, uint32_t aiSize);
	uint32_t CheckNewReference(TESForm* apForm, uint32_t aiFlags);

public:
	void DeleteSaveGame(BSFile* apFile, const char* sFileName);
	uint32_t ReverseConvertNumericID(uint32_t aiFormID);
	// Inferred type: apFile: BSFile* from 0x858167 (parameter 0 of TESSaveLoadGame::SaveGlobalData)
	void SaveGlobals(BSFile* apFile);
	TESForm* CreateReference(uint32_t aiFormID, CreatedReferenceData* apCreatedData);
	// Not named in the PDB or IDA; name chosen here. Hashes the form IDs of apRef's biped head, hair, upper body and
	// hands; the exe's callers (Actor::SaveGame_0, Actor::LoadGame) pass the actor.
	int32_t ComputeArmorHash(TESObjectREFR* apRef);
	void LoadAnimations();
	void LoadAnimationForReference(TESObjectREFR* apRef, uint32_t aiFormID, void* apBuffer);
	void LoadAttachedAnimations();
	void LoadCharControllers();
	void LoadHavokData();
	void QueueFormForFinalization(TESForm* apForm);
	void FinalizeQueuedForms();
	void BuildSaveGameList();
	void DeleteSaveGameList();
	void GetSaveGameDisplayData(BSFile* apFile, uint32_t* apSaveGameNumber, char* apFileName, char* apLevel,
		char* apTotalPlayingTime);
	// Returns ReadSaveGameDataOLD's save-game texture (the one caller ignores it).
	// Inferred type: apFile: BSFile* from 0x85ff38 (parameter 0 of TESSaveLoadGame::GetSaveGameDisplayData); Dst: char*
	// from 0x86013c (passed to BSstrcpy 0); apDest: char* from 0x85ff38 (parameter 3 of
	// TESSaveLoadGame::GetSaveGameDisplayData); a6: char* from 0x860174 (passed to BSstrcpy 0); a7: char* from 0x8602a4
	// (passed to BSsprintf 0); a8: char* from 0x860350 (passed to BSsprintf 0); a9: char* from 0x85ff38 (parameter 4 of
	// TESSaveLoadGame::GetSaveGameDisplayData)
	NiRefObject* GetSaveGameData(BSFile* apFile, uint32_t* a3, char* Dst, char* apDest, char* a6, char* a7, char* a8,
		char* a9, uint32_t* a10, char a11);
	// Inferred type: a1: BSFile* from 0x860233 (parameter 0 of TESSaveLoadGame::GetSaveGameData); Dst: char* from
	// 0x8604fc (passed to BSstrcpy 0)
	NiRefObject* ReadSaveGameDataOLD(BSFile* a1, uint32_t aiSize, uint32_t* a3, void* a4, uint16_t* a5, char* Dst,
		float* a7, _SYSTEMTIME* a8, uint32_t* a9, uint32_t* a10);
	// Inferred type: apFile: BSFile* from 0x85ff08 (parameter 0 of TESSaveLoadGame::GetSaveGameDisplayData), its
	// members used at 0x8607fe; apDest: char* from 0x85ff08 (parameter 3 of TESSaveLoadGame::GetSaveGameDisplayData);
	// Dst: char* from 0x85ff08 (parameter 4 of TESSaveLoadGame::GetSaveGameDisplayData)
	char RetrieveSaveGameData(BSFile* apFile, int32_t* arIndex, char* apDest, char* Dst);
	void GenerateSaveGameFileName(char* apFileName);
	uint32_t LoadHeader(BSFile* apFile, bool abFullLoad);
	void AddCreatedBaseObject(TESForm* apForm);
	void SaveCreatedBaseObjects(BSFile* apFile);
	int16_t AddWorldSpaceIDToArray(int32_t a1);
	// Inferred type: a1: uint16_t: unsigned uses from 0x861cda
	int32_t GetWorldSpaceIDFromArray(uint16_t a1);
	void RemoveFormFromInitFormArray(TESForm* apForm);

	// The exe's deleting destructor (0x450BF0) frees through ourFree.
	static void operator delete(void* apMemory);
	static int32_t SaveGameCompare(SaveGameFile* apCompare1, SaveGameFile* apCompare2);

	ChangesMap* m_pChanges;
	ChangesMap* m_pNewChanges;
	InteriorCellNewReferencesMap* m_pInteriorCellMap;
	ExteriorCellNewReferencesMap* m_pExteriorCellMap;
	ExteriorCellNewReferencesMap* m_pCreatedExteriorCells;
	void* m_pBuffer;
	uint32_t m_iSaveLoadStates;
	bool m_bAllowChanges;
	uint8_t padding1D[3];
	NiTLargePrimitiveArray<FormAndFlags*>* m_pInitArray;
	BSSimpleList<uint32_t> m_MovedReferencesList;
	BSSimpleList<uint32_t> m_CreatedBaseObjectsList;
	BSSimpleList<TESForm*> m_DeferredDeleteList;
	BSSimpleList<TESForm*> m_DeferredInitActorPackageList;
	SaveDataDebugInfo* m_pSaveLoadStats;
	int32_t m_iRevertState;
	uint8_t m_iSavedPluginCount;
	uint8_t padding4D[3];
	uint8_t* m_pFileIndexArray;
	uint32_t m_iQueuedRemoveChanges;
	NumericIDBufferMap* m_pAnimationMap;
	NumericIDBufferMap* m_pAttachedAnimationMap;
	NumericIDBufferMap* m_pCharControllerMap;
	NumericIDBufferMap* m_pHavokDataMap;
	BSSimpleList<TESForm*> m_QueuedFormList;
	BSSimpleList<SaveGameFile*>* m_pSaveGameList;
	uint8_t m_cMajorVersion;
	uint8_t m_cMinorVersion;
	uint8_t padding76[2];
	NiTLargePrimitiveArray<uint32_t>* m_pNumericIDArray;
	NiTLargePrimitiveArray<uint32_t>* m_pWorldSpaceIDArray;
	uint8_t m_cCurrentVersion;
	bool m_bUseNumericIDArray;
	uint8_t padding82[2];
	LoadFormHeader* m_pCurrentlyLoadingFormHeader;
	SaveFormHeader* m_pCurrentlySavingFormHeader;
	uint32_t m_iNextSaveNumber;
	uint32_t m_iFileStartPosition;
	uint32_t m_iSimulationFileSize;
	_SYSTEMTIME m_OriginalSaveTime;
	uint32_t m_iOriginalSaveVersion;
	bool m_bReconstructedAllObjects;
	bool m_bLoadingFromMainMenu;
	bool m_bAutosaveDisabledDueToLackOfSpace;
	bool m_bQuickloading;
	NiTLargePrimitiveArray<TESForm*>* m_pInitItemArray;
	SavedPlayerData m_SavedPlayerData;
	char* m_pMostRecentSaveGame;
};

// The changes a save game records, keyed by form ID (the PC's twin of BGSSaveLoadChangesMap).
class ChangesMap : public NiTPointerMap<uint32_t, ChangeData*> {
public:
	ChangesMap();
	~ChangesMap() override;

	virtual void RemoveAllChanges();

	// The changes recorded for auiFormID, or null.
	ChangeData* GetChanges(uint32_t auiFormID);
	// The changes recorded for apForm, or null.
	ChangeData* GetChanges(TESForm* apForm);
	// Clears the aiChange flags recorded for apForm.
	bool RemoveChange(TESForm* apForm, uint32_t aiChange);
	// Drops the changes recorded for aiFormID; true when there were some.
	bool RemoveChanges(uint32_t aiFormID, bool abForceRemove);
	// Drops the changes recorded for apForm through the form ID overload; true when there were some.
	bool RemoveChanges(TESForm* apForm, bool abForceRemove);
	// Records the aiChange flags for apForm and returns its change data (thiscall, ret 8,
	// 0x855010). The reconstructed callers ignore the result.
	ChangeData* AddChange(TESForm* apForm, uint32_t aiChange);

	// The exe's deleting destructor (0x854ED0) frees through ourFree.
	static void operator delete(void* apMemory);
};

inline constexpr fnvd::Vtable NumericIDBufferMap_vftable{};
inline constexpr fnvd::Vtable InteriorCellNewReferencesMap_vftable{};
inline constexpr fnvd::Vtable ExteriorCellNewReferencesMap_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseUIntBSSimpleListUInt_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseUIntBSSimpleListExteriorCellReferenceData_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseUInt8BSSimpleListLoadFormHeader_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseDFALLUIntExpiredCellDataList_vftable{};
inline constexpr fnvd::Vtable NiTMapUIntExpiredCellDataList_vftable{};
inline constexpr fnvd::Vtable SaveGameFile_vftable{};

// The engine's TESSaveLoadGame (null when there is none).
inline TESSaveLoadGame* pSaveLoadGameOLD;

// IDA name TESSaveLoadGame::kLoadLock; not in the PDB. Held while a form's changes are applied.
extern BSCriticalSection kLoadLock;
inline constexpr fnvd::Vtable ChangesMap_vftable{};

// The game settings TESSaveLoadGame.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDebugSaveBuffer;
inline int32_t iLoadSequenceStartTime;
using rdata_1081AA8_t = const char[4];
inline rdata_1081AA8_t rdata_1081AA8{" - "};
