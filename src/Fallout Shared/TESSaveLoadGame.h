#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/SaveDataDebugInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <stdint.h>

class ChangeData;
class ChangesMap;
class CreatedReferenceData;
class ExteriorCellNewReferencesMap;
struct FormAndFlags;
class InteriorCellNewReferencesMap;
class LoadFormHeader;
class MovedReferenceData;
class NumericIDBufferMap;
struct SaveBufferHeader;
class TESForm;
class TESObjectCELL;
class TESObjectREFR;
class TESPackage;

template <class T>
class NiTLargePrimitiveArray;

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
    void* GetBuffer();
    uint32_t GetFlags();
    void SetBuffer(void* apBuffer);
    void AddChange(uint32_t aiChange);

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    uint32_t iFlags;
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
// Packed, like SaveFormHeader. Declared `class` to match the existing forward declarations.
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

// The four bytes in front of each changed form's data in the save buffer: the data size, the form type and the version.
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

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);
};

// The references new to each interior cell, as lists of form IDs keyed by the cell's form ID.
class InteriorCellNewReferencesMap : public NiTPointerMap<uint32_t, BSSimpleList<uint32_t>*> {
public:
    InteriorCellNewReferencesMap();
    ~InteriorCellNewReferencesMap() override;

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);
};

// The references new to each exterior cell, as lists of reference data keyed by the cell's coordinates.
class ExteriorCellNewReferencesMap : public NiTPointerMap<uint32_t, BSSimpleList<ExteriorCellReferenceData*>*> {
public:
    ExteriorCellNewReferencesMap();
    ~ExteriorCellNewReferencesMap() override;

    // Frees through ourFree.
    static void operator delete(void* apMemory);
};

// The save loader's lists-by-ID maps. Their constructors are defined in TESSaveLoadGame.cpp.
using NiTMapBaseUIntBSSimpleListUInt = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BSSimpleList<uint32_t>*>;
using NiTMapBaseUIntBSSimpleListExteriorCellReferenceData =
    NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BSSimpleList<ExteriorCellReferenceData*>*>;
using NiTMapBaseUInt8BSSimpleListLoadFormHeader =
    NiTMapBase<NiTPointerAllocator<size_t>, uint8_t, BSSimpleList<LoadFormHeader*>*>;

// An expired exterior cell's grid coordinates: the cell's X and Y.
struct ExpiredCellData {
    int32_t iCellX;
    int32_t iCellY;
};

// The expired cell data map (a cell's data keyed by the cell's form ID).
using NiTMapBaseDFALLUIntExpiredCellDataList =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, BSSimpleList<ExpiredCellData*>*>>, uint32_t, BSSimpleList<ExpiredCellData*>*>;
using NiTMapUIntExpiredCellDataList = NiTMap<uint32_t, BSSimpleList<ExpiredCellData*>*>;

// A save game file: a BSFile that remembers the file time of the save it was read from. Only some of its members are
// declared.
class SaveGameFile : public BSFile {
public:
    SaveGameFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiBufferSize);
    ~SaveGameFile() override;

    // Sets the file time and marks it present. Returns this, which the caller reads.
    SaveGameFile* SetFileTime(_FILETIME aFileTime);
    _FILETIME GetFileTime();
    bool GetHasFileTime();

    // The deleting destructor frees through ourFree.
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
    // What a file is opened for; CleanupLoadSave switches on it, 0 to 3.
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
    // Whether a save is being loaded. In this build it always returns false.
    bool GetLoading();
    // In this build the body is empty.
    void SetLoading(bool abValue);
    // Applies the save's changes to apForm.
    bool LoadForm(TESForm* apForm);
    void SetCurrentlyLoadingFormHeader(LoadFormHeader* apHeader);
    // Loads apForm's initial data (position, cell, ...) for its change flags aiFlags.
    void LoadInitialData(TESForm* apForm, uint32_t aiFlags);
    // Loads the save's data of apCell.
    bool LoadCell(TESObjectCELL* apCell);
    // Initialises the forms of apInitArray.
    void
    InitForms(NiTLargePrimitiveArray<FormAndFlags*>* apInitArray, FormAndFlags* apPlayerFlags, bool abForceLoadModels);
    // Runs the queued animation, controller and Havok loads.
    void LoadQueuedData(bool abUpdateWorld);
    bool GetInitArrayExists();
    void StartLoadingSequence();
    // Queues apForm for initialisation after the load.
    void AddInitItem(TESForm* apForm);
    // In this build UnloadForm does nothing and always returns false.
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
    // The header of the form being saved (m_pCurrentlySavingFormHeader).
    SaveFormHeader* GetCurrentlySavingFormHeader();
    // The header of the form being loaded (m_pCurrentlyLoadingFormHeader).
    LoadFormHeader* GetCurrentlyLoadingFormHeader();
    // Whether m_iRevertState is 0.
    bool IsUnk048Zero();
    // m_bReconstructedAllObjects.
    bool GetReconstructedAllObjects() {
        return m_bReconstructedAllObjects;
    }
    // Writes aiSize bytes of apData to the old-format save.
    void SaveGameDataOLD(const void* apData, uint32_t aiSize);
    // Returns the write position in the save buffer, m_pBuffer.
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
    bool GetUseNumericIDArray();
    TESForm* CreateReferenceFromFile(uint32_t aiFormID, MovedReferenceData* apMovedData);
    uint32_t AddNumericIDToArray(uint32_t aiFormID);
    bool SaveGame(BSFile* pFile, const char* apFileName, bool abLogStats);
    bool ResetReferencePosition(TESObjectREFR* apRef, bool abTemporaryMove);
    void SaveGlobalData(BSFile* apFile);
    void SaveFinalData(BSFile* a1);
    uint32_t CheckFlags(TESForm* apForm, uint32_t aiFlags);
    void CreateChangedReference(uint32_t aiReferenceID);
    // SaveGame passes the form it saves and its change flags as a whole 32-bit value. Only a form that is not a
    // reference is dynamic-cast to a cell.
    void SaveInitialData(TESForm* apForm, uint32_t aiFlags);
    // The extra save size of apForm for the change flags aiFlags.
    int32_t GetInitialDataSaveSize(TESForm* apForm, uint32_t aiFlags);
    void SavePluginList(BSFile* apFile);
    void SaveHeader(BSFile* apFile, const char* apFileName);
    uint32_t GetCreatedExteriorCellFormID(uint32_t aiWorldSpaceID, int32_t aiCellX, int32_t aiCellY);
    void SaveNumericIDArrays(BSFile* a1);
    // Keeps the name of the save apFile.
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
    void SaveGlobals(BSFile* apFile);
    TESForm* CreateReference(uint32_t aiFormID, CreatedReferenceData* apCreatedData);
    // Hashes the form IDs of apRef's biped head, hair, upper body and hands; the callers (Actor::SaveGame_0,
    // Actor::LoadGame) pass the actor.
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
    void GetSaveGameDisplayData(
        BSFile* apFile,
        uint32_t* apSaveGameNumber,
        char* apFileName,
        char* apLevel,
        char* apTotalPlayingTime
    );
    // Returns ReadSaveGameDataOLD's save-game texture (the one caller ignores it).
    NiRefObject* GetSaveGameData(
        BSFile* apFile,
        uint32_t* a3,
        char* Dst,
        char* apDest,
        char* a6,
        char* a7,
        char* a8,
        char* a9,
        uint32_t* a10,
        char a11
    );
    NiRefObject* ReadSaveGameDataOLD(
        BSFile* a1,
        uint32_t aiSize,
        uint32_t* a3,
        void* a4,
        uint16_t* a5,
        char* Dst,
        float* a7,
        _SYSTEMTIME* a8,
        uint32_t* a9,
        uint32_t* a10
    );
    char RetrieveSaveGameData(BSFile* apFile, int32_t* arIndex, char* apDest, char* Dst);
    void GenerateSaveGameFileName(char* apFileName);
    uint32_t LoadHeader(BSFile* apFile, bool abFullLoad);
    void AddCreatedBaseObject(TESForm* apForm);
    void SaveCreatedBaseObjects(BSFile* apFile);
    int16_t AddWorldSpaceIDToArray(int32_t a1);
    int32_t GetWorldSpaceIDFromArray(uint16_t a1);
    void RemoveFormFromInitFormArray(TESForm* apForm);

    // Frees through ourFree.
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
    // Records the aiChange flags for apForm and returns its change data. Callers ignore the result.
    ChangeData* AddChange(TESForm* apForm, uint32_t aiChange);

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);
};

// The engine's TESSaveLoadGame (null when there is none).
inline TESSaveLoadGame* pSaveLoadGameOLD;

// Held while a form's changes are applied.
extern BSCriticalSection kLoadLock;

// The game settings TESSaveLoadGame.cpp defines.
extern SettingT<INISettingCollection> bDebugSaveBuffer;
inline int32_t iLoadSequenceStartTime;
using rdata_1081AA8_t = const char[4];
inline rdata_1081AA8_t rdata_1081AA8{" - "};
