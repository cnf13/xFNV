#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BGSSaveLoadFile;
class TESGlobal;

class BGSSaveLoadGlobalData {
public:
    // The global data blocks of a save game: the initial ones (0-11) before the forms, the final ones after them.
    enum GLOBAL_DATA : uint32_t {
        GLOBAL_DATA_INITIAL_START = 0,
        GLOBAL_DATA_MISC_STATS = 0,
        GLOBAL_DATA_LOCATION = 1,
        GLOBAL_DATA_TES = 2,
        GLOBAL_DATA_GLOBALS = 3,
        GLOBAL_DATA_PROCESS_LISTS = 4,
        GLOBAL_DATA_COMBAT = 5,
        GLOBAL_DATA_INTERFACE = 6,
        GLOBAL_DATA_EFFECTS = 7,
        GLOBAL_DATA_WEATHER = 8,
        GLOBAL_DATA_ACTOR_CAUSES = 9,
        GLOBAL_DATA_RADIO = 10,
        GLOBAL_DATA_AUDIO = 11,
        GLOBAL_DATA_INITIAL_END = 11,
        GLOBAL_DATA_INITIAL_COUNT = 12,
        GLOBAL_DATA_FINAL_START = 1000,
        GLOBAL_DATA_TEMP_EFFECTS = 1000,
        GLOBAL_DATA_FINAL_END = 1000,
        GLOBAL_DATA_FINAL_COUNT = 1,
    };

    // Loads one global data block from the save buffer; false for the blocks with nothing to
    // load.
    static bool LoadGlobalData(uint32_t auiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer);
    // Reads aiCount global data blocks from apFile.
    static void LoadGlobalData(BGSSaveLoadFile* apFile, uint32_t aiCount);
    // Starts loading one global data block before the forms; false for the blocks with
    // nothing to start.
    static bool InitGlobalData(uint32_t aiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer);
    static void InitGlobalDataRange(uint32_t, uint32_t);
    // Finishes loading one global data block after the forms; false for the blocks with nothing to finish.
    static bool FinishLoadGlobalData(uint32_t aiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer);
    // Reverts one global data block; GLOBAL_DATA_GLOBALS only on a full revert. False for the blocks with nothing
    // to revert.
    static bool RevertGlobalData(uint32_t aiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer, bool abFullRevert);
    // Reverts every global data block.
    static void RevertGlobalData(bool abFullRevert);
    // Reverts the global variables.
    static void RevertGlobals();
    static void InitInitialGlobalData();
    static void FinishLoadInitialGlobalData();
    // Finishes loading the blocks aiStart to aiEnd (exclusive) after the forms.
    static void FinishLoadGlobalDataRange(uint32_t aiStart, uint32_t aiEnd);
    // Reverts the blocks aiStart to aiEnd.
    static void RevertGlobalDataRange(uint32_t aiStart, uint32_t aiEnd, bool abFullRevert);
    static bool SaveGlobalData(uint32_t aiGlobalDataType, BGSSaveGameBuffer* apSaveGameBuffer);
    // The name of global data block aiType, for the save statistics.
    static const char* GetGlobalDataName(uint32_t aiType);
    static void SaveLocationData(BGSSaveGameBuffer* apSaveGameBuffer);
    static void LoadLocationData(BGSLoadGameBuffer* apLoadGameBuffer);
    static void SaveGlobals(BGSSaveGameBuffer* apSaveGameBuffer);
    static void LoadGlobals(BGSLoadGameBuffer* apLoadGameBuffer);
    static void SaveEffects(BGSSaveGameBuffer* apSaveGameBuffer);
    static void LoadEffects(BGSLoadGameBuffer* apLoadGameBuffer);

    static inline char* pGlobalDataInitialNames[12]{
        const_cast<char*>("Player Location"),
        const_cast<char*>("TES"),
        const_cast<char*>("Global Variables"),
        const_cast<char*>("Process Lists"),
        const_cast<char*>("Combat"),
        const_cast<char*>("Interface"),
        const_cast<char*>("Effects"),
        const_cast<char*>("Misc Stats"),
        const_cast<char*>("Weather"),
        const_cast<char*>("Actor Causes"),
        const_cast<char*>("Radio"),
        const_cast<char*>("Audio")
    };
    static inline char* pGlobalDataFinalNames[1]{const_cast<char*>("Temp Effects")};
};

// The global-variable map's base and its vtable. Defined in BGSSaveLoadGlobalData.cpp.
using NiTMapBaseTESGlobalBool = NiTMapBase<DFALL<NiTMapItem<TESGlobal*, bool>>, TESGlobal*, bool>;

// The per-system reverts that RevertGlobalData calls.
namespace RevertGlobalData {
    // Removes the active image space modifiers.
    void ImageSpaceModifiers();
} // namespace RevertGlobalData
