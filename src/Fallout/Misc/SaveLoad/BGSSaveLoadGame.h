#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class Actor;
class BGSLoadFormBuffer;
class BGSReconstructFormsInAllFilesMap;
class BGSSaveLoadChangesMap;
class BGSSaveLoadFile;
class BGSSaveLoadFormIDMap;
class BGSSaveLoadHistory;
class BGSSaveLoadQueuedSubBufferMap;
class BGSSaveLoadReferencesMap;
class TESObjectCELL;
class TESObjectREFR;

// The kinds of form sub-buffer BGSSaveLoadGame::QueueSubBuffer defers to the end of the load.
enum QUEUED_SUB_BUFFER_TYPE : uint32_t {
    QUEUED_SUB_BUFFER_ANIMATION = 0,
    QUEUED_SUB_BUFFER_CHAR_CONTROLLER = 1,
    QUEUED_SUB_BUFFER_HAVOK_DATA = 2,
    QUEUED_SUB_BUFFER_COUNT = 3,
};

// Where a save game's sections start and how many forms they hold. BGSSaveLoadGame::SaveGame writes it twice, 0x6E
// bytes each time: the fields and ReservedSpace without the tail padding.
struct SaveGameFormInfo {
    SaveGameFormInfo();

    uint32_t iFormIDMapOffset;
    uint32_t iHistoryOffset;
    uint32_t iInitialGlobalDataOffset;
    uint32_t iFormDataOffset;
    uint32_t iFinalGlobalDataOffset;
    uint32_t iInitialGlobalFormCount;
    uint32_t iFinalGlobalFormCount;
    uint32_t iFormCount;
    uint32_t iSecondaryDataSize;
    uint8_t ReservedSpace[0x4A];
};

// The save-game manager: which forms changed since load, and the state flags of a load or save in progress.
class BGSSaveLoadGame {
public:
    // Bits of uiGlobalFlags.
    enum GLOBAL_FLAGS : uint32_t {
        GLOBAL_DISALLOW_CHANGES = 0x1,
        GLOBAL_SAVEGAME_LOADING = 0x2,
        GLOBAL_SAVEGAME_SAVING = 0x4,
        GLOBAL_INITING_FORMS = 0x8,
        GLOBAL_DEFER_INIT_FORMS = 0x10,
        GLOBAL_POSITIONING_PLAYER = 0x20,
        GLOBAL_PLAYER_LOCATION_INVALID = 0x40,
        GLOBAL_SAVELOAD_FAILED = 0x80,
    };

    BGSSaveLoadGame();
    ~BGSSaveLoadGame();

    // Queues the package locations of apRef to be initialised.
    void QueueInitPackageLocations(Actor* apActor);
    // Deletes apForm.
    void DeleteForm(TESForm* apForm);
    // Drops apRef's form ID from the map at +0x34.
    void CancelInitPackageLocations(TESObjectREFR* apRef);
    void AddChange(TESForm* apForm, BGSChangeFlags aiChanges, bool abForceAdd);
    void RemoveChange(TESForm* apForm, BGSChangeFlags aiChanges);
    void RemoveChanges(TESForm* apForm);
    // whether apForm had any of aiChanges recorded before the load
    bool GetOldChange(TESForm* apForm, BGSChangeFlags aiChanges);
    void QueueSubBuffer(QUEUED_SUB_BUFFER_TYPE aeType, BGSLoadFormBuffer* apLoadFormBuffer, TESForm* apParentForm);
    // Reverts every change and the global data, as a full revert.
    void RevertAllChanges();
    // Records aiNewFormID as the new form ID of aiOldFormID; warns when aiOldFormID already has one.
    void AddChangedFormID(uint32_t aiOldFormID, uint32_t aiNewFormID);
    bool ShouldAddChanges(TESForm* apForm, bool abForceAdd);
    // Whether apForm has any of the change flags aiChanges recorded; the flags are passed by value.
    bool GetChange(TESForm* apForm, BGSChangeFlags aiChanges);
    bool IsFormIDInUse(uint32_t aiFormID);
    bool HasReferenceChangedCells(uint32_t aiFormID);

    bool GetSaveGameLoading() const;
    bool GetSaveGameSaving() const;
    bool GetDeferInitForms() const;
    bool GetGlobalAllowChanges() const;
    // Whether the forms are being initialised after a load.
    bool GetInitingForms() const;
    // Sets or clears the inited-forms state.
    void SetInitingForms(bool abInitingForms);
    void SetSaveLoadFailed(bool abFailed);
    // Allows or disallows changes; returns whether they were allowed before.
    bool SetGlobalAllowChanges(bool abValue);

    // Whether changes are allowed in the current thread and globally.
    bool GetAllowChanges();
    // Whether apForm's changes are tracked: changes are allowed and the form is not temporary.
    bool ShouldRemoveChanges(TESForm* apForm);
    // Whether the calling thread is loading the references a save moved (a bit of the thread's TLS block).
    bool GetLoadingMovedRefs();
    // Whether the calling thread is reconstructing forms (a bit of the thread's TLS block).
    bool GetReconstructingForms();
    // Sets whether the calling thread is reconstructing forms.
    void SetReconstructingForms(bool abValue);
    // Sets whether the calling thread is loading moved references.
    void SetLoadingMovedRefs(bool abValue);
    // Drops the form's recorded changes when it unloads.
    void UnloadForm(TESForm* apForm, bool abUnloadWhileLoadingSaveGame);
    // Whether the calling thread may record changes.
    bool GetThreadAllowChanges();
    // Sets whether the calling thread may record changes; returns the old value.
    bool SetThreadAllowChanges(bool abValue);
    // Applies the save's changes to apForm.
    void LoadForm(TESForm* apForm);
    // Writes the save game to apFile: the version, the plugins, the header, the global data, every changed form,
    // the form ID maps and the history, then the header again with its offsets filled in.
    void SaveGame(BGSSaveLoadFile* apFile);
    // Writes the loaded plugins' names to apFile.
    void SavePluginList(BGSSaveLoadFile* apFile);
    // Reverts every changed form and replaces the changes map with apChangesMap.
    void RevertCurrentChanges(BGSSaveLoadChangesMap* apNewChangesMap, BGSSaveLoadFormIDMap* apOldFormIDMap);
    void PerformQueuedInitPackageLocations();
    void ClearForm(TESForm* apForm);
    void LoadCell(TESObjectCELL* apCell);
    // Initialises the forms queued during the load.
    void InitForms(bool abLoadModels);
    bool GetPositioningPlayerCharacter();
    void SetDeferInitForms(bool abValue);
    bool GetPlayerLocationInvalid();
    uint8_t GetCurrentMinorVersion();
    void SetSaveGameSaving(bool abValue);
    void SetSaveGameLoading(bool abValue);
    void SetPlayerLocationInvalid(bool abValue);
    uint8_t GetSaveMod(uint8_t a2);
    bool GetSaveLoadFailed();
    void TogglePositioningPlayer(char a2);
    void ClearFormID(uint32_t aiFormID);
    void SaveGameWarningFunction(const char* apText, va_list aList);
    uint32_t GetChangedFormID(uint32_t aiOldFormID);
    // Removes the havok changes of references in cells whose respawn time has passed.
    void CleanupExpiredData();
    // Loads the save game in apFile: the plugins, the form ID maps, the history, the global data and every changed
    // form. False when a plugin is missing (unless abIgnoreMissingContent) or a form fails to load.
    bool LoadGame(BGSSaveLoadFile* apFile, bool abIgnoreMissingContent);

protected:
    // Reads the save's plugin list; false when a plugin is missing.
    bool LoadPluginList(BGSSaveLoadFile* apFile);
    // Checks the initial data of the created or moved reference apLoadGameBuffer loads: recreates, reconstructs,
    // defers or deletes it; the result of reading the initial data (0 when none was read).
    uint32_t CheckInitialData(
        BGSSaveLoadFile* apFile,
        BGSLoadFormBuffer* apLoadGameBuffer,
        BGSSaveLoadFormIDMap* apOldFormIDMap
    );
    // Handles the changes of *arpForm that cannot be reverted; false when the form has to be reconstructed.
    bool HandleUnrevertibleChanges(TESForm*& arpForm, BGSChangeFlags aiUnrevertibleChanges, bool abRevertOnly);

public:
    // Whether apForm's changes are kept in a buffer when it unloads: not for a temporary form or a non-permanent
    // projectile.
    bool ShouldUnloadForm(TESForm* apForm);

    // Creates and installs the save/load singleton.
    static void Create();
    // Deletes the save/load singleton and clears it.
    static void Destroy();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BGSSaveLoadChangesMap* pChangesMap;
    BGSSaveLoadChangesMap* pOldChangesMap;
    BGSSaveLoadFormIDMap* pFormIDMap;
    BGSSaveLoadFormIDMap* pWorldspaceFormIDMap;
    BGSSaveLoadReferencesMap* pReferencesMap;
    BGSSaveLoadQueuedSubBufferMap* pQueuedSubBuffersMap;
    // the new form ID of each changed form ID
    NiTMap<uint32_t, uint32_t>* pChangedFormIDMap;
    BGSSaveLoadHistory* pHistory;
    BGSReconstructFormsInAllFilesMap* pReconstructForms;
    // the buffers of the forms a load reads, in save order
    BSSimpleArray<BGSLoadFormBuffer*, 1024> FormBufferArray;
    // The actors whose package locations are initialised after the load, by form ID (the constructor builds it with
    // 0x25 buckets).
    NiTMap<uint32_t, Actor*> QueuedInitPackageLocationsActorMap;
    uint8_t cFileIndexArray[255];
    uint8_t cReverseFileIndexArray[255];
    uint8_t padding242[2];
    uint32_t uiGlobalFlags;
    uint8_t cCurrentMinorVersion;
    uint8_t padding249[3];
};

// The actor map's base and its vtable. Defined in BGSSaveLoadGame.cpp.
using NiTMapBaseUIntActor = NiTMapBase<DFALL<NiTMapItem<uint32_t, Actor*>>, uint32_t, Actor*>;

inline BGSSaveLoadGame* pSaveLoadGame;
