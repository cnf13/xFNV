#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <stdint.h>

class BSFile;
class MiscSaveStat;
// The names of the initial global data blocks (GLOBAL_DATA 0-11), which
// BGSSaveLoadGlobalData::GetGlobalDataName returns.
class SavedFormData;

// What the save game statistics keep per saved form: its change flags, how many bytes it took and the version it was
// saved with. The struct is packed, sizeof 25.
#pragma pack(push, 1)
class SavedFormData {
public:
    SavedFormData();

    uint32_t GetTotalSize();
    void GetFormName(char* apName, uint32_t aiNameSize);

    uint32_t iFormID;
    BGSChangeFlags iChangeFlags;
    uint32_t iSize;
    uint32_t iOverhead;
    uint32_t iDebugOverhead;
    TESForm* pForm;
    uint8_t cVersion;
};
#pragma pack(pop)

// What the save game statistics keep per miscellaneous save item: its name, the bytes it took and its debug overhead.
// GetTotalSize adds the two sizes.
class MiscSaveStat {
public:
    uint32_t GetTotalSize();

    char pName[260];
    uint32_t iSize;
    uint32_t iDebugOverhead;
};

int32_t MiscSaveStatCompare(MiscSaveStat* apCompare1, MiscSaveStat* apCompare2);
// The saved forms' sort order, passed to Insert.
int32_t SavedFormDataCompare(SavedFormData* apCompare1, SavedFormData* apCompare2);

class BGSSaveLoadStatsMap : public NiTMap<ENUM_FORM_ID, BSSimpleList<SavedFormData*>*> {
public:
    BGSSaveLoadStatsMap();
    ~BGSSaveLoadStatsMap() override;

    void OutputStats();
    void SaveForm(TESForm* apForm, BGSChangeFlags aiChangeFlags, BGSSaveFormBuffer* apSaveFormBuffer);
    void WriteFile(const char* apText);
    void CloseFile();
    void BuildStatsMap();
    void AddForm(
        uint32_t aiFormID,
        TESForm* apForm,
        ENUM_FORM_ID aeFormType,
        BGSChangeFlags aiChangeFlags,
        uint32_t aiSize,
        uint32_t aiOverhead,
        uint32_t aiDebugOverhead,
        uint8_t acVersion
    );
    void AddMiscSaveStat(const char* apName, uint32_t aiSize, uint32_t aiDebugOverhead);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void OutputSaveLoadStats(const char* apSaveFileName);

    BSSimpleList<MiscSaveStat*> MiscSaveStats;
    BSFile* pFile;
};

using NiTMapFormIDSavedFormDataList = NiTMap<ENUM_FORM_ID, BSSimpleList<SavedFormData*>*>;
using NiTMapBaseFormIDSavedFormDataList = NiTMapBase<
    DFALL<NiTMapItem<ENUM_FORM_ID, BSSimpleList<SavedFormData*>*>>,
    ENUM_FORM_ID,
    BSSimpleList<SavedFormData*>*>;

// The save game's form types, in save order (55 entries).
using cSaveGameFormTypes_t = uint32_t[55];
inline cSaveGameFormTypes_t cSaveGameFormTypes{58,  59,  60,  61,  62,  63,  64,  57,  70,  71,  42,  43,  21, 22,
                                               23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  34,  39,  40, 41,
                                               46,  47,  48,  49,  97,  7,   8,   73,  67,  85,  44,  45,  52, 78,
                                               103, 104, 105, 106, 107, 108, 109, 110, 113, 114, 115, 116, 117};

// The save index of each form type, 0xFF for the types a save does not hold.
using cSaveGameFormTypeArray_t = uint8_t[121];
inline cSaveGameFormTypeArray_t cSaveGameFormTypeArray{};
inline uint32_t iSaveGameFormTypeCount;
