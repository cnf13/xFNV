#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Xenon/XGamerProfile.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Tile.h"
#include "BSSystem/BSSystemDir.h"
#include "BSSystem/BSSystemFile.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"
#include "BSSystemUtilities/BSSaveDataSystemUtilityFile.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExteriorCellLoader.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/MessageMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHistory.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadStructs.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/io.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

// The game settings this file defines, with the game's IDs and values, in the game's initialization order.
SettingT<INISettingCollection> bAllowScriptedAutosave("bAllowScriptedAutosave:SaveGame", true);
SettingT<INISettingCollection> bAllowScriptedForceSave("bAllowScriptedForceSave:SaveGame", true);
SettingT<INISettingCollection> sSaveGameSafeMarkerID("sSaveGameSafeMarkerID:SaveGame", "32D4");
SettingT<INISettingCollection> iSaveGameBackupCount("iSaveGameBackupCount:SaveGame", 1);
SettingT<INISettingCollection> bAllowProfileTransfer("bAllowProfileTransfer:SaveGame", false);
SettingT<INISettingCollection> bOutputSaveGameScreenshot("bOutputSaveGameScreenshot:SaveGame", false);
SettingT<INISettingCollection> bCopySaveGameToHostOrMemStick("bCopySaveGameToHostOrMemStick:SaveGame", false);
SettingT<INISettingCollection> bDisplayMissingContentDialogue("bDisplayMissingContentDialogue:SaveGame", true);

// The format's arguments go to vprintf as the argument list.
int32_t BSprintf(const char* apFormat, ...) {
    va_list varArgs;
    va_start(varArgs, apFormat);
    int32_t iResult = std::vprintf(apFormat, varArgs);
    va_end(varArgs);
    return iResult;
}

void BSprintf_ErrorReport(const char* apText, void* apErrorReportObject) {
    BSprintf(apText, apErrorReportObject);
}

void LoadGameMissingContentCallBack() {
    if (Interface::GetMessageMenuResult() == 1) {
        pSaveLoadManager->LoadGame(g_sMissingContentSaveName, aiDeviceID, bLoadGameMissingContentOutputStats, true);
    }
}

BGSSaveLoadManager::BGSSaveLoadManager()
  : pSaveGameList(nullptr), iSaveGameCount(0), iCurrentSaveGameNumber(0), bSimulatedMode(false),
    bAutosaveDisabledForDiskspace(false), bhandledCorrupt(false), cQueuedAutosave(0), cQueuedForceSave(0),
    cSystemSaveTimer(0), pMostRecentSaveGame(nullptr), iMostRecentSaveGameDeviceID(-1), bSysUtilHasSaveData(false),
    pSaveLoadFile(nullptr), bReturnedFromSysUtil(true), bSysUtilLoadDataComplete(false), bStartMenuLoading(false),
    pSysUtilSaveCompleteCallback(nullptr), pSysUtilLoadCompleteCallback(nullptr), pSaveName() {}

BGSSaveLoadManager::~BGSSaveLoadManager() {
    DeleteSaveGameList();
    ourFree(pMostRecentSaveGame);
}

void BGSSaveLoadManager::UpdateQueuedSaves() {
    if (cQueuedAutosave) {
        if (cQueuedAutosave <= 1u) {
            if (!pFaderManager->IsFaderVisible(FADER_TO_BLACK) && !pFaderManager->IsFaderVisible(FADER_TO_WHITE) &&
                !Interface::IsInGameLoadingMenuOpen()) {
                pSaveLoadManager->Autosave();
            }
        }
        else {
            --cQueuedAutosave;
        }
    }
    else {
        if (cQueuedForceSave) {
            if (cQueuedForceSave <= 1u) {
                if (!pFaderManager->IsFaderVisible(FADER_TO_BLACK) && !pFaderManager->IsFaderVisible(FADER_TO_WHITE) &&
                    !Interface::IsInGameLoadingMenuOpen()) {
                    pSaveLoadManager->SaveGame(nullptr, -1, false);
                    cQueuedForceSave = 0;
                }
            }
            else {
                --cQueuedForceSave;
            }
        }
        else {
            if (cSystemSaveTimer) {
                if (cSystemSaveTimer <= 1u) {
                    if (!pFaderManager->IsFaderVisible(FADER_TO_BLACK) &&
                        !pFaderManager->IsFaderVisible(FADER_TO_WHITE) && !Interface::IsInGameLoadingMenuOpen()) {
                        pSaveLoadManager->SystemSave();
                    }
                }
                else {
                    --cSystemSaveTimer;
                }
            }
        }
    }
}

bool BGSSaveLoadManager::LoadMostRecentSaveGame() {
    return (bool)((uint8_t)(HasMostRecentSaveGame() &&
                            LoadGame(pMostRecentSaveGame, iMostRecentSaveGameDeviceID, false, true)));
}

void BGSSaveLoadManager::Autosave() {
    cQueuedAutosave = 0;
    bhandledCorrupt = false;
    float afSecondsPassed = Main::QFrameAnimTime();
    Sky* CurrentAIProcess = TES::pInstance->GetSky();
    CurrentAIProcess->Update(afSecondsPassed);
    SaveGame("autosave", -1, false);
}

int8_t BGSSaveLoadManager::SystemSave() {
    char Src[268];
    cSystemSaveTimer = 0;
    bhandledCorrupt = false;
    float afSecondsPassed = Main::QFrameAnimTime();
    Sky* CurrentAIProcess = TES::pInstance->pSky;
    CurrentAIProcess->Update(afSecondsPassed);
    BGSSaveLoadManager::CreateSystemSaveName(Src);
    if (!pSaveName.GetLength()) {
        pSaveName.Set(Src, 0u);
    }
    return SaveGame(Src, -1, false);
}

void BGSSaveLoadManager::ScrubFileName(char* apFileName) {
    uint32_t v3 = BSstrlen(apFileName);
    for (uint32_t i = 0; i < v3; ++i) {
        if (apFileName[i] < 48) {
            apFileName[i] = 32;
        }
        if (apFileName[i] <= 122) {
            if (std::strchr("\t\\/:*<>?|\"+=@^[]`;", apFileName[i])) {
                if (apFileName[i] == 34) {
                    apFileName[i] = 39;
                }
                else {
                    apFileName[i] = 32;
                }
            }
        }
        else {
            apFileName[i] = 32;
        }
    }
}

bool BGSSaveLoadManager::IsSaveFileNameQuicksave(const char* apSaveFileName) {
    return apSaveFileName && !BSstrnicmp(apSaveFileName, "quicksave", BSstrlen("quicksave"));
}

// abDisplayOnly is tested for nonzero, not used as 0 or 1 to index the format.
void BGSSaveLoadManager::GetPlayTimeString(char* apPlayTimeString, bool abDisplayOnly) {
    uint32_t uiTime = pPlayer->GetTotalPlayingTime();
    uint32_t uiHours = uiTime / 3600000;
    uiTime -= uiHours * 3600000;
    uint32_t uiMinutes = uiTime / 60000;
    uiTime -= uiMinutes * 60000;
    uint32_t uiSeconds = uiTime / 1000;
    if (abDisplayOnly) {
        BSsprintf(apPlayTimeString, 0x104, "%03i.%02i.%02i", uiHours, uiMinutes, uiSeconds);
    }
    else {
        BSsprintf(apPlayTimeString, 0x104, "%02i.%02i.%02i", uiHours, uiMinutes, uiSeconds);
    }
}

bool BGSSaveLoadManager::IsSaveFileNameGenerated(const char* apSaveFileName) {
    char Dst[268];
    const char* v2 = sMenuDisplaySave.String();
    BSstrcpy(Dst, 260, v2);
    BSstrcat(Dst, 260, " ");
    bool result = false;
    if (apSaveFileName) {
        uint32_t v3 = BSstrlen(Dst);
        if (!BSstrnicmp(apSaveFileName, Dst, v3)) {
            return true;
        }
    }
    return result;
}

BGSSaveLoadFile*
BGSSaveLoadManager::OpenSaveFile(const char* apFileName, bool abWrite, SAVEFILE_TYPE aeBufferType, int32_t aiDeviceID) {
    int8_t fileName[268];
    uint32_t uiBufferSize;
    GetFullPath(apFileName, static_cast<char*>(static_cast<void*>(fileName)), abWrite);
    if (aeBufferType != SAVEFILE_SCREENSHOT) {
        if (aeBufferType == SAVEFILE_LOAD) {
            uiBufferSize = 2097152;
        }
        else {
            uiBufferSize = 65536;
        }
    }
    else {
        uiBufferSize = 65536;
    }
    if (abWrite && (rt::_access(static_cast<const char*>(static_cast<const void*>(fileName)), 0) != (-1))) {
        std::error_code kError;
        std::filesystem::remove(
            platform::NativePath(static_cast<const char*>(static_cast<const void*>(fileName))), kError
        );
    }
    BGSSaveLoadFile* result = BGSSaveLoadFile::CreateFileA(
        static_cast<const char*>(static_cast<const void*>(fileName)), abWrite, uiBufferSize
    );
    result->SetSaveFileName(apFileName);
    result->Open();
    return result;
}

void BGSSaveLoadManager::CloseSaveFile(BGSSaveLoadFile* apFile, bool abWrite) {
    char acSaveFileName[264];
    BSstrcpy(acSaveFileName, 260, &apFile->pSaveFileName[0]);
    if (apFile) {
        delete apFile;
    }
    if (abWrite) {
        char acFullPath[264];
        char acNewFileName[264];
        GetFullPath(acSaveFileName, acFullPath, abWrite);
        BSstrcpy(acNewFileName, 260, acFullPath);
        // Drops the written file's last four characters (its temporary extension), indexing with the signed length.
        acNewFileName[static_cast<int32_t>(BSstrlen(acNewFileName)) - 4] = 0;
        for (int32_t i = iSaveGameBackupCount.Int() - 1; i >= 0; --i) {
            char acOldFileName[264];
            char acBackupFileName[268];
            BSstrcpy(acOldFileName, 260, acNewFileName);
            for (int32_t j = 0; j < i; ++j) {
                BSstrcat(acOldFileName, 260, ".bak");
            }
            BSstrcpy(acBackupFileName, 260, acOldFileName);
            BSstrcat(acBackupFileName, 260, ".bak");
            if (rt::_access(acOldFileName, 0) != -1) {
                if (rt::_access(acBackupFileName, 0) != -1) {
                    std::error_code kError;
                    std::filesystem::remove(platform::NativePath(acBackupFileName), kError);
                }
                rt::rename(acOldFileName, acBackupFileName);
            }
        }
        rt::rename(acFullPath, acNewFileName);
        BSstrcpy(acFullPath, 260, acNewFileName);
    }
}

bool BGSSaveLoadManager::IsSaveFileNameAutosave(const char* apSaveFileName) {
    bool result = false;
    if (apSaveFileName) {
        uint32_t v3 = BSstrlen("autosave");
        if (!BSstrnicmp(apSaveFileName, "autosave", v3)) {
            return true;
        }
    }
    return result;
}

bool BGSSaveLoadManager::IsSaveFileNameSystemSave(const char* apSaveFileName) {
    bool result = false;
    if (apSaveFileName) {
        uint32_t v3 = BSstrlen("systemsave");
        if (!BSstrnicmp(apSaveFileName, "systemsave", v3)) {
            return true;
        }
    }
    return result;
}

bool BGSSaveLoadManager::HasMostRecentSaveGame() {
    return (pMostRecentSaveGame != nullptr);
}

bool BGSSaveLoadManager::SaveGame(const char* apFileName, int32_t aiDeviceID, bool abOutputStats) {
    bool bIsAutoSave = IsSaveFileNameAutosave(apFileName);
    bool bIsQuickSave = IsSaveFileNameQuicksave(apFileName);
    bool bIsSystemSave = IsSaveFileNameSystemSave(apFileName);
    bool v13 = false;
    if (bIsAutoSave || (cQueuedForceSave || bIsSystemSave)) {
        v13 = true;
    }
    if (IsSavingAllowed(bIsAutoSave)) {
        char Dst[268];
        // The generated name lives for the whole save (apFileName points at it).
        char apSaveFileName[271];
        if (!iCurrentSaveGameNumber) {
            SetupSavesList();
            DeleteSaveGameList();
        }
        if (!apFileName) {
            GenerateSaveFileName(apSaveFileName, false, false);
            apFileName = apSaveFileName;
        }
        BSstrcpy(Dst, 260, apFileName);
        pSaveLoadGame->CleanupExpiredData();
        uint32_t aiCount = pSaveLoadGame->pHistory->GetCount();
        StartSaveGame();
        BGSSaveLoadFile* apSaveFile = nullptr;
        if (pSaveLoadFile) {
            apSaveFile = pSaveLoadFile;
        }
        else {
            apSaveFile = OpenSaveFile(apFileName, true, SAVEFILE_SAVE, aiDeviceID);
        }
        if (apSaveFile) {
            if (v13) {
                bAutosaveDisabledForDiskspace = false;
                const char* v6 = sMenuDisplayShortXBoxSaveMessage.operator const char*();
                Interface::ShowMessage(
                    v6, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds", nullptr, 2.0f, true
                );
            }
            SaveGameHeader(apSaveFile);
            pSaveLoadGame->SaveGame(apSaveFile);
            if (pSaveLoadGame->GetSaveGameSaving()) {
                pSaveLoadGame->SetSaveGameSaving(false);
                const char* v7 = sSaveFailed.operator const char*();
                Interface::ShowMessage(
                    v7, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
                );
                EndSaveGame();
                return false;
            }
            else {
                EndSaveGame();
                if (apSaveFile != pSaveLoadFile) {
                    CloseSaveFile(apSaveFile, true);
                }
                if (abOutputStats) {
                    char apFullPath[270];
                    GetFullPath(apFileName, apFullPath, false);
                    BGSSaveLoadStatsMap::OutputSaveLoadStats(apFullPath);
                }
                pSaveLoadGame->pHistory->Rewind(aiCount);
                if (apSaveFile != pSaveLoadFile) {
                    if (pSaveGameList) {
                        AddSaveGame(Dst, aiDeviceID, iCurrentSaveGameNumber);
                    }
                }
                if (!bIsQuickSave && !bIsAutoSave) {
                    ++iCurrentSaveGameNumber;
                }
                SetMostRecentSaveGame(Dst, aiDeviceID);
                return true;
            }
        }
        else {
            EndSaveGame();
            return false;
        }
    }
    else {
        if (!v13) {
            const char* v4 = sCantSaveNow.String();
            Interface::ShowMessage(
                v4, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
            );
        }
        return false;
    }
}

void BGSSaveLoadManager::GetSaveDirectoryPath(char* apDirectoryPath) {
    const char* PersonalPath = BSUtilities::GetPersonalPath();
    BSstrcpy(apDirectoryPath, 260, PersonalPath);
    const char* v2 = sLocalSavePath.String();
    BSstrcat(apDirectoryPath, 260, v2);
    std::error_code kError;
    std::filesystem::create_directory(platform::NativePath(apDirectoryPath), kError);
}

// The player may not save while dead, dying, under AI control or in VATS; a manual save also needs every menu on
// the stack to be the console, the start menu or a message box without buttons.
bool BGSSaveLoadManager::IsSavingAllowed(bool abAutosave) {
    if (pPlayer) {
        if (pPlayer->GetLifeState() == ACTOR_LIFE_STATE_DEAD || pPlayer->GetLifeState() == ACTOR_LIFE_STATE_DYING) {
            return false;
        }
        if (pPlayer->GetUnderAIControl()) {
            return false;
        }
    }
    if (pVATS.IsInVats()) {
        return false;
    }
    if (!abAutosave) {
        uint32_t uiCount = Interface::GetMenuCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            int32_t eMenu = Interface::GetMenuFromStack(i);
            if (eMenu > Interface::eMessageMenu) {
                if (eMenu != Interface::eStartMenu) {
                    return false;
                }
            }
            else if (eMenu == Interface::eMessageMenu) {
                if (MessageMenu::GetCurrentButtonCount()) {
                    return false;
                }
            }
            else if (eMenu != Interface::eNoMenu && eMenu != Interface::eConsoleMenu) {
                return false;
            }
        }
    }
    return true;
}

int32_t BGSSaveLoadManager::GetMajorVersion() {
    return 48;
}

int32_t BGSSaveLoadManager::GetMinorVersion() {
    return 0x1B;
}

void BGSSaveLoadManager::GetFullFileName(const char* apFileName, char* apFullFileName) {
    BSstrcpy(apFullFileName, 260, apFileName);
    uint32_t v3 = BSstrlen(apFullFileName);
    if (v3 < 4 || BSstricmp(&apFullFileName[v3 - 4], ".fos")) {
        BSstrcat(apFullFileName, 260, ".fos");
    }
}

void BGSSaveLoadManager::GetVersionInfo(char* apVersionInfo, uint32_t aiSize) {
    _SYSTEMTIME kLocalTime;
    char acUserName[260];
    char acDate[260];
    // Both buffers start as "": the first character is the terminator, then the rest is cleared with a memset call.
    acUserName[0] = '\0';
    std::memset(&acUserName[1], 0, 259);
    acDate[0] = '\0';
    std::memset(&acDate[1], 0, 259);
    // GetUserNameA's name, at most the buffer less its terminator; the buffer stays "" when it is unknown.
    platform::UserName().copy(acUserName, sizeof(acUserName) - 1);
    ::GetLocalTime(&kLocalTime);
    BSsprintf(
        acDate, 260, "%d/%d/%02d %02d:%02d:%02d", kLocalTime.wMonth, kLocalTime.wDay, kLocalTime.wYear,
        kLocalTime.wHour, kLocalTime.wMinute, kLocalTime.wSecond
    );
    BSsprintf(
        apVersionInfo, static_cast<int32_t>(aiSize), "%s - %s, EXE Version: %s, Save Version %i.%i", acUserName, acDate,
        "1.4.0.525", 48, 27
    );
}

void BGSSaveLoadManager::StartLoadGame() {
    pExteriorCellLoader->ClearData();
    pModelLoader->CancelAllFiles();
    Script::ClearButtonPressed();
    Script::ClearOptimizations();
    ScriptLocals::ClearOptimizations();
    BSAudio* v1 = &BSAudio::QInstance();
    v1->KillAllOfType(0xFFFFFFFFu, 0);
    BSAudio* v2 = &BSAudio::QInstance();
    bool bmultiThreadedAudio = v2->bMultiThread;
    BSAudio* v3 = &BSAudio::QInstance();
    v3->SetMultiThreaded(false);
    for (uint32_t i = 0; i < 10; ++i) {
        BSAudio* v4 = &BSAudio::QInstance();
        v4->Update(false);
    }
    BSAudio* v5 = &BSAudio::QInstance();
    v5->SetMultiThreaded(bmultiThreadedAudio);
    if (pVATS.IsInVats()) {
        pVATS.QuitVATSPlayback(true, false);
        pVATS.SetMode(VATS::VATS_MODE_NONE, false);
    }
    appTimer.SetGlobalTimeMultiplier(1.0f, true);
    if (pPlayer) {
        pPlayer->ResetKillCam(KILL_CAM_VATS, true);
    }
    TES::pInstance->ShowLoadingMenu(true, nullptr, false);
    while (true) {
        PathManager* v6 = PathManager::QInstance();
        if (v6->QIdle()) {
            break;
        }
        PathManager* v7 = PathManager::QInstance();
        v7->Update();
        NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
        Instance->Update();
    }
    PathManager* v9 = PathManager::QInstance();
    v9->CancelCurrentPathsAndPause();
}

// The message shows for 2 seconds with emotion 0, the neutral vault boy icon and no sound.
void BGSSaveLoadManager::Quicksave() {
    Interface::ShowMessage(
        sQuickSaving.String(), 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds", nullptr, 2.0f,
        false
    );
    SaveGame("quicksave", -1, false);
}

// The message shows for 2 seconds with emotion 0, the neutral vault boy icon and no sound.
bool BGSSaveLoadManager::Quickload() {
    Interface::ShowMessage(
        sQuickLoading.String(), 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds", nullptr, 2.0f,
        false
    );
    return LoadGame("quicksave", -1, false, true);
}

// KEEP: with a null apFileName the exe never fills its 260-byte name copy (the BSstrcpy at 0x85078F is skipped), yet
// a successful load still passes that uninitialised buffer to SetMostRecentSaveGame (an uninitialised read on that
// path).
bool BGSSaveLoadManager::LoadGame(
    const char* apFileName,
    int32_t aiDeviceID,
    bool abOutputStats,
    bool abIgnoreMissingContent
) {
    char acFileName[268];
    if (apFileName) {
        BSstrcpy(acFileName, 260, apFileName);
    }
    BGSSaveLoadFile* pFile = nullptr;
    if (pSaveLoadFile) {
        pFile = pSaveLoadFile;
    }
    else {
        pFile = OpenSaveFile(apFileName, false, BGSSaveLoadManager::SAVEFILE_LOAD, aiDeviceID);
    }
    bool bResult = false;
    if (pFile) {
        bool bIsConsoleVisible = Interface::IsConsoleVisible();
        if (!bDisplayMissingContentDialogue.Bool()) {
            abIgnoreMissingContent = true;
        }
        if (pFile->Open()) {
            if (LoadGameHeader(pFile)) {
                bStartMenuLoading = true;
                bResult = pSaveLoadGame->LoadGame(pFile, abIgnoreMissingContent);
                bStartMenuLoading = false;
                if (bResult) {
                    EndLoadGame();
                }
                else {
                    BSstrcpy(g_sMissingContentSaveName, 260, apFileName);
                    ::aiDeviceID = aiDeviceID;
                    bLoadGameMissingContentOutputStats = abOutputStats;
                    const char* pNo = sNo.String();
                    const char* pYes = sYes.String();
                    Interface::CreateMessageMenu(
                        sSaveGameContentIsMissing.String(), static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr), &LoadGameMissingContentCallBack, 1, 23, 0.0f, 0.0f, pYes,
                        pNo, nullptr
                    );
                }
            }
            else {
                bResult = false;
            }
        }
        if (pFile != pSaveLoadFile) {
            CloseSaveFile(pFile, false);
        }
        if (bResult) {
            if (abOutputStats) {
                char acFullPath[271];
                GetFullPath(apFileName, acFullPath, false);
                BGSSaveLoadStatsMap::OutputSaveLoadStats(acFullPath);
            }
        }
        if (bIsConsoleVisible) {
            Interface::OpenConsole();
            TESObjectREFR* pDebugTextReference = Main::pDebugTextReference;
            InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
            pInterfaceManager->SetTargetREFR(pDebugTextReference);
        }
        if (bResult) {
            SetMostRecentSaveGame(acFileName, aiDeviceID);
        }
    }
    appMain->KillMenuBGTexture();
    return bResult;
}

bool BGSSaveLoadManager::IsLoadingGame() {
    return (bool)((uint8_t)bStartMenuLoading);
}

void BGSSaveLoadManager::SetAutoSaveTimerTo1() {
    cQueuedAutosave = (uint8_t)1;
}

void BGSSaveLoadManager::SetForceSaveTimer() {
    cQueuedForceSave = (uint8_t)1;
}

void BGSSaveLoadManager::SetShouldSystemSave() {
    cSystemSaveTimer = (uint8_t)1;
}

bool BGSSaveLoadManager::CanSave(int32_t aiDeviceID) {
    return true;
}

bool BGSSaveLoadManager::IsSaveFileNameCustom(const char* apSaveFileName) {
    return (bool)((uint8_t)((!IsSaveFileNameGenerated(apSaveFileName)) &&
                            ((!IsSaveFileNameAutosave(apSaveFileName)) &&
                             ((!IsSaveFileNameQuicksave(apSaveFileName)) &&
                              (!IsSaveFileNameSystemSave(apSaveFileName))))));
}

void BGSSaveLoadManager::DeleteSaveFile(const char* apFileName, int32_t aiDeviceID) {
    int8_t fileName[268];
    GetFullPath(apFileName, static_cast<char*>(static_cast<void*>(fileName)), false);
    std::error_code kError;
    std::filesystem::remove(platform::NativePath(static_cast<const char*>(static_cast<const void*>(fileName))), kError);
}

void BGSSaveLoadManager::RemoveSaveGame(BGSSaveLoadFileEntry* apFileEntry) {
    if (pSaveGameList) {
        pSaveGameList->Remove(apFileEntry);
        if (apFileEntry) {
            delete apFileEntry;
        }
        iSaveGameCount = pSaveGameList->ItemsInList();
    }
}

void BGSSaveLoadManager::DeleteSaveGameList() {
    for (BGSSaveLoadFileEntry* apThis : *pSaveGameList) {
        if (apThis) {
            delete apThis;
        }
    }
    if (pSaveGameList) {
        pSaveGameList->RemoveAll();
        if (pSaveGameList) {
            delete pSaveGameList;
        }
        pSaveGameList = nullptr;
    }
    iSaveGameCount = 0;
}

void BGSSaveLoadManager::SetMostRecentSaveGame(const char* apSaveFileName, int32_t aiDeviceID) {
    uint32_t size;
    if (apSaveFileName) {
        if ((!pMostRecentSaveGame) || BSstrcmp(pMostRecentSaveGame, apSaveFileName)) {
            ourFree(pMostRecentSaveGame);
            size = BSstrlen(apSaveFileName) + 1;
            pMostRecentSaveGame = static_cast<char*>(static_cast<void*>(static_cast<int8_t*>(ourMalloc(size))));
            BSstrcpy(pMostRecentSaveGame, size, apSaveFileName);
            iMostRecentSaveGameDeviceID = aiDeviceID;
        }
    }
}

void BGSSaveLoadManager::GetFullPath(const char* apFileName, char* apFullPath, bool abWrite) {
    char Src[264];
    char PathName[268];
    GetFullFileName(apFileName, Src);
    GetSaveDirectoryPath(PathName);
    BSstrcpy(apFullPath, 260, PathName);
    BSstrcat(apFullPath, 260, Src);
    if (abWrite) {
        BSstrcat(apFullPath, 260, ".tmp");
    }
}

bool BGSSaveLoadManager::LoadGameHeader(BGSSaveLoadFile* apFile) {
    int32_t imajorVersion = BGSSaveLoadHeader::LoadHeader(apFile);
    if (imajorVersion <= 48 && imajorVersion >= 47) {
        return true;
    }
    if (imajorVersion > 0) {
        char apDest[264];
        if (imajorVersion >= 48) {
            BSsprintf(
                apDest, 260u,
                "The savegame cannot be loaded because it comes from a newer executable.  Its version is %i and the "
                "current version is %i.",
                imajorVersion, 48
            );
        }
        else {
            BSsprintf(
                apDest, 260u,
                "The savegame cannot be loaded because it is outdated.  Its version is %i and the current version is "
                "%i.",
                imajorVersion, 48
            );
        }
        Interface::CreateMessageMenu(
            apDest, static_cast<const char*>(nullptr), static_cast<const char*>(nullptr),
            static_cast<void (*)()>(nullptr), 0, 23, 0.0f, 0.0f, sOk.String(), nullptr
        );
    }
    else {
        const char* v3 = sOk.String();
        Interface::CreateMessageMenu(
            sXBoxLoadNoDeviceMessage.operator const char*(), static_cast<const char*>(nullptr),
            static_cast<const char*>(nullptr), static_cast<void (*)()>(nullptr), 0, 23, 0.0f, 0.0f, v3, nullptr
        );
    }
    return false;
}

void BGSSaveLoadManager::SetupSavesList() {
    char acFileName[264];
    if (pSaveGameList) {
        DeleteSaveGameList();
    }
    BSSimpleList<BGSSaveLoadFileEntry*>* pMemory =
        static_cast<BSSimpleList<BGSSaveLoadFileEntry*>*>(ourMalloc(sizeof(BSSimpleList<BGSSaveLoadFileEntry*>)));
    BSSimpleList<BGSSaveLoadFileEntry*>* pList;
    if (pMemory) {
        pList = ::new (pMemory) BSSimpleList<BGSSaveLoadFileEntry*>;
    }
    else {
        pList = nullptr;
    }
    pSaveGameList = pList;
    iCurrentSaveGameNumber = 1;
    GetSaveDirectoryPath(acFileName);
    BSstrcat(acFileName, 260, "*");
    BSstrcat(acFileName, 260, ".fos");
    std::vector<platform::FileEntry> kFound = platform::FindFiles(acFileName);
    // Nothing found: the list stays empty, unsorted and unnumbered.
    if (kFound.empty()) {
        return;
    }
    for (const platform::FileEntry& kEntry : kFound) {
        // Tests only the low 32 bits of the file size.
        if (static_cast<uint32_t>(kEntry.size)) {
            AddSaveGame(kEntry.name.c_str(), -1, 0);
        }
    }
    pSaveGameList->Sort(&BGSSaveLoadFileEntry::Compare);
    // The newest save numbers the next one, unless it is a quick, auto or system save.
    BGSSaveLoadFileEntry* pFirstSave = pSaveGameList->m_item;
    if (pFirstSave && iCurrentSaveGameNumber <= 1) {
        iCurrentSaveGameNumber = pFirstSave->GetSaveGameNumber();
        if (!pFirstSave->IsQuicksave() && !pFirstSave->IsAutosave() && !pFirstSave->IsSystemSave()) {
            iCurrentSaveGameNumber = iCurrentSaveGameNumber + 1;
        }
    }
}

void BGSSaveLoadManager::AddSaveGame(const char* apFileName, int32_t aiDeviceID, uint32_t aiSaveGameNumber) {
    BGSSaveLoadFileEntry* pfileEntry;
    char Dst[268];
    BSstrcpy(Dst, 260, apFileName);
    uint32_t ilength = BSstrlen(Dst);
    if (ilength > 4 && !BSstricmp(&Dst[ilength - 4], ".fos")) {
        Dst[ilength - 4] = 0;
    }
    BGSSaveLoadFileEntry* apMemory = static_cast<BGSSaveLoadFileEntry*>(ourMalloc(sizeof(BGSSaveLoadFileEntry)));
    if (apMemory) {
        pfileEntry = ::new (apMemory) BGSSaveLoadFileEntry(Dst, aiDeviceID);
    }
    else {
        pfileEntry = nullptr;
    }
    BGSSaveLoadFileEntry* pfileEntry_2 = pfileEntry;
    pfileEntry->CalculateFileTime();
    pSaveGameList->AddHead(pfileEntry_2);
    ++iSaveGameCount;
}

void BGSSaveLoadManager::GenerateSaveFileName(char* apSaveFileName, bool abDisplayOnly, bool abAutosave) {
    const char* v4;
    char apString[264];
    char Dst[268];
    const char* pplayerName = pPlayer->GetFullName();
    GetPlayerLocationName(Dst);
    GetPlayTimeString(apString, abDisplayOnly);
    if (abAutosave) {
        v4 = sMenuDisplayAutosaveName.String();
    }
    else {
        v4 = sMenuDisplaySave.String();
    }
    BSsprintf(apSaveFileName, 260u, "%s %i - %s, %s, %s", v4, iCurrentSaveGameNumber, pplayerName, Dst, apString);
    if (!abDisplayOnly) {
        if (BSstrlen(apSaveFileName) > 255) {
            apSaveFileName[255] = 0;
        }
        ScrubFileName(apSaveFileName);
    }
}

void BGSSaveLoadManager::StartSaveGame() {
    PathManager* v2;
    PathManager* v3;
    PathManager* singleton = PathManager::QInstance();
    singleton->Pause();
    pIOManager->Pause();
    while (true) {
        v2 = PathManager::QInstance();
        if (v2->QPaused()) {
            break;
        }
        v3 = PathManager::QInstance();
        v3->Update();
    }
}

void BGSSaveLoadManager::EndSaveGame() {
    pIOManager->Resume();
    PathManager::QInstance()->Resume();
}

void BGSSaveLoadManager::SaveGameHeader(BGSSaveLoadFile* apFile) {
    pSaveLoadGame->SetSaveGameSaving(true);
    BGSSaveLoadHeader::SaveHeader(apFile);
}

void BGSSaveLoadManager::EndLoadGame() {
    TESChallengeManager::LoadChallenges();
    appMain->UpdateNonRenderSafeAITasks();
    appMain->RunScripts();
    pModelLoader->ClearUnusedModels(false);
    TES::pInstance->UpdateFadeNodesForAttachedCells();
    appMain->ForceUpdateTextures();
    GridCellArray* FormFlags = TES::pInstance->GetGridCellA();
    FormFlags->UpdateDistantLandBlending();
    pVATS.ClearCommandList();
    TESWorldSpace* WorldSpace = pPlayer->GetWorldSpace();
    if (WorldSpace) {
        const NiPoint3* afFadeSpeed = &pPlayer->GetLocationOnReference();
        BGSTerrainManager* TerrainManager = WorldSpace->GetTerrainManager();
        TerrainManager->Update(*afFadeSpeed, BGSTerrainManager::UPDATE_ALL);
    }
    if (XGamerProfile::Instance()) {
        XGamerProfile* v3 = XGamerProfile::Instance();
        v3->SetPlayingFlag(true);
        XGamerProfile* v4 = XGamerProfile::Instance();
        v4->ForcePresenceUpdate();
    }
    float* v5 = &fFadeToBlackFadeSeconds.Float();
    pFaderManager->CreateFader(FADER_TO_BLACK, *v5, true);
    LoadingMenu::SuspendBackgroundThread();
    TES::pInstance->ShowLoadingMenu(false, nullptr, false);
    BSAudio* v6 = &BSAudio::QInstance();
    v6->UnPauseSynchTimer();
    FalloutAudioMedia::MediaMenuUnPause();
    ReferenceProcessLists.RebuildActorsCloseToPlayer();
    pSaveLoadGame->SetPlayerLocationInvalid(false);
    NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
    Instance->ForceUpdateAllObstacles();
    PathManager* v8 = PathManager::QInstance();
    v8->Resume();
}

// KEEP: the exe appends "Saves" and "/" to cpath ([ebp-0x11C], 0x8520E1) with BSstrcat without ever writing it
// first, so the directory name starts with whatever the stack held: an uninitialised read on every path.
// The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadManager::CopySaveGames(bool bFromMemoryStick) {
    char cpath[260];
    bool bcopySaveGamesToHostOrMemStick = bCopySaveGameToHostOrMemStick.Bool();
    bCopySaveGameToHostOrMemStick = false;
    BSstrcat(cpath, 260, "Saves");
    BSstrcat(cpath, 260, "/");
    BSSystemDir systemDir(cpath);
    bool bisComplete = false;
    systemDir.NextEntry(bisComplete);
    while (!bisComplete) {
        if (systemDir.QRegular()) {
            uint32_t uilength = BSstrlen(systemDir.FindData_.cFileName);
            if (!BSstrnicmp(systemDir.FindData_.cFileName + (uilength - 4), ".fos", 4u)) {
                char cfileName[260];
                BSsprintf(cfileName, 260, "%s%s", cpath, systemDir.FindData_.cFileName);
                BSSystemFile systemFile(cfileName, 0, 0, false);
                if (systemFile.eLastError) {
                    BSprintf(
                        "BGSSaveLoadManager::CopySaveGamesFromHost failed to open the save game file %s.\n", cfileName
                    );
                }
                else {
                    BGSSaveLoadFile* psaveFile;
                    uint64_t uifileSize = 0;
                    systemFile.GetSize(uifileSize);
                    BGSSaveLoadFile* pMemory = static_cast<BGSSaveLoadFile*>(ourMalloc(sizeof(BGSSaveLoadFile)));
                    if (pMemory) {
                        psaveFile = ::new (pMemory) BGSSaveLoadFile("SaveData", false, uifileSize);
                    }
                    else {
                        psaveFile = nullptr;
                    }
                    // The save file's BSFile is the save data utility's file (BGSSaveLoadFile::pFile, +0x104).
                    BSSaveDataSystemUtilityFile* psaveDataFile =
                        static_cast<BSSaveDataSystemUtilityFile*>(psaveFile->pFile);
                    uint64_t uibytesRead = 0;
                    systemFile.Read(psaveDataFile->pucMemBuffer, uifileSize, uibytesRead);
                    if (systemFile.eLastError) {
                        BSprintf(
                            "BGSSaveLoadManager::CopySaveGamesFromHost failed to read the save game file buffer "
                            "%s.\n",
                            cfileName
                        );
                    }
                    else {
                        const char* PlayerName;
                        const char* PlayerTitle;
                        const char* PlayTime;
                        BGSSaveLoadFileEntry* pentry;
                        char apcDetail[260];
                        char apDest[260];
                        BSsprintf(apDest, 260, "Copying the save game file to the target...\n\n");
                        BSstrcat(apDest, 260, cfileName);
                        BSSystemUtility* v6 = &BSSystemUtility::QInstance();
                        BSMsgDialogSystemUtility* MsgDialogUtility = v6->GetMsgDialogUtility();
                        MsgDialogUtility->DisplayMessage(apDest, 1000u);
                        BGSSaveLoadFileEntry* pEntryMemory =
                            static_cast<BGSSaveLoadFileEntry*>(ourMalloc(sizeof(BGSSaveLoadFileEntry)));
                        if (pEntryMemory) {
                            pentry = ::new (pEntryMemory) BGSSaveLoadFileEntry(cfileName, 0);
                        }
                        else {
                            pentry = nullptr;
                        }
                        BGSSaveLoadFileEntry* pentry_2 = pentry;
                        BGSSaveLoadHeader::LoadHeaderData(psaveFile, pentry);
                        pentry_2->SetLoaded();
                        psaveDataFile->SetDirectoryName("-SAVE-");
                        if (pentry_2->GetLocation() && BSstrlen(pentry_2->GetLocation())) {
                            psaveDataFile->SetSubTitle(pentry_2->GetLocation());
                        }
                        else {
                            psaveDataFile->SetSubTitle("Location");
                        }
                        if (pentry_2->GetPlayTime()) {
                            PlayTime = pentry_2->GetPlayTime();
                        }
                        else {
                            PlayTime = sPlayTime.String();
                        }
                        if (pentry_2->GetPlayerTitle()) {
                            PlayerTitle = pentry_2->GetPlayerTitle();
                        }
                        else {
                            PlayerTitle = "-";
                        }
                        if (pentry_2->GetPlayerName()) {
                            PlayerName = pentry_2->GetPlayerName();
                        }
                        else {
                            PlayerName = sDefaultPlayerName.String();
                        }
                        const char* v16 = sPlayTime.String();
                        uint32_t PlayerLevel = pentry_2->GetPlayerLevel();
                        const char* v9 = sLevelAbbrev.String();
                        BSsprintf(
                            apcDetail, 256, "%s\n%s %d: %s\n%s: %s", PlayerName, v9, PlayerLevel, PlayerTitle, v16,
                            PlayTime
                        );
                        psaveDataFile->SetDetail(apcDetail);
                        // The screenshot inside the loaded save data (BSMemoryFile::pucMemBuffer, +0x15C).
                        void* pScreenshot = psaveDataFile->pucMemBuffer + pentry_2->GetScreenshotOffset();
                        uint32_t ScreenshotHeight = pentry_2->GetScreenshotHeight();
                        uint32_t ScreenshotWidth = pentry_2->GetScreenshotWidth();
                        psaveDataFile->SetIconImage(ScreenshotWidth, ScreenshotHeight, pScreenshot);
                        psaveDataFile->SetBackgroundImagePath("DATA/SYSUTIL/SAVEBACK.PNG");
                        psaveDataFile->SetSaveCompleteCallback(&BGSSaveLoadManager::SysUtilSaveCompleteCallback);
                        if (pentry_2) {
                            delete pentry_2;
                        }
                        pSaveLoadFile = psaveFile;
                        BSSystemUtility* v12 = &BSSystemUtility::QInstance();
                        BSSaveDataSystemUtility* SaveDataUtility = v12->GetSaveDataUtility();
                        SaveDataUtility->AutoSaveData(psaveDataFile);
                        while (true) {
                            BSSystemUtility* v13 = &BSSystemUtility::QInstance();
                            BSSaveDataSystemUtility* v14 = v13->GetSaveDataUtility();
                            if (v14->bIsActive != 1) {
                                break;
                            }
                            BSSleep(10u);
                        }
                    }
                }
            }
        }
        systemDir.NextEntry(bisComplete);
    }
    bCopySaveGameToHostOrMemStick = bcopySaveGamesToHostOrMemStick;
}

void BGSSaveLoadManager::GetPlayerLocationName(char* apLocationName) {
    BSStringT<char> v2;
    pPlayer->GetMapNameForLocation(v2);
    if (v2.pString) {
        BSstrcpy(apLocationName, 260, v2.pString);
    }
    else {
        BSstrcpy(apLocationName, 260, "");
    }
}

void BGSSaveLoadManager::Destroy() {
    BGSSaveLoadManager* pManager = pSaveLoadManager;
    if (pManager) {
        delete pManager;
    }
    pSaveLoadManager = nullptr;
}

void BGSSaveLoadManager::CreateSystemSaveName(char* apDest) {
    BSsprintf(apDest, 260u, "%s %s", "systemsave", pPlayer->GetFullName());
    if (BSstrlen(apDest) > 27) {
        apDest[27] = 0;
    }
    pSaveLoadManager->ScrubFileName(apDest);
}

void BGSSaveLoadManager::Create() {
    BGSSaveLoadManager* Manager;
    BGSSaveLoadManager* apMemory = static_cast<BGSSaveLoadManager*>(ourMalloc(sizeof(BGSSaveLoadManager)));
    if (apMemory) {
        Manager = ::new (apMemory) BGSSaveLoadManager;
    }
    else {
        Manager = nullptr;
    }
    pSaveLoadManager = Manager;
    BSSaveDataSystemUtility* Singleton = BSSaveDataSystemUtility::GetSingleton();
    pSaveLoadManager->bSysUtilHasSaveData = Singleton->GetSaveDataEntries("-SAVE-", nullptr, 0) != 0;
    BSSaveDataSystemUtility::GetSingleton()->pDeleteCompleteCallback =
        &BGSSaveLoadManager::SysUtilDeleteCompleteCallback;
}

void BGSSaveLoadManager::SysUtilSaveCompleteCallback(
    BSSaveDataSystemUtility::RESULT aeResult,
    BSSaveDataSystemUtilityFile*& apSaveDataFile
) {
    if (pSaveLoadManager) {
        pSaveLoadManager->bReturnedFromSysUtil = true;
    }
    if (pSaveLoadManager->pSaveLoadFile) {
        BGSSaveLoadFile* apThis = pSaveLoadManager->pSaveLoadFile;
        if (apThis) {
            delete apThis;
        }
        pSaveLoadManager->pSaveLoadFile = nullptr;
    }
    if (pSaveLoadManager) {
        if (pSaveLoadManager->pSysUtilSaveCompleteCallback) {
            pSaveLoadManager->pSysUtilSaveCompleteCallback(aeResult, apSaveDataFile);
        }
    }
}

void BGSSaveLoadManager::SysUtilDeleteCompleteCallback(BSSaveDataSystemUtility::RESULT aeResult) {
    BSSaveDataSystemUtility* Singleton = BSSaveDataSystemUtility::GetSingleton();
    pSaveLoadManager->bSysUtilHasSaveData = Singleton->GetSaveDataEntries("-SAVE-", nullptr, 0) != 0;
}

BSSystemFile::ErrorCode BSSystemFile::Read(void* apvBuffer, uint64_t auiBytes, uint64_t& auiRead) {
    eLastError = DoRead(apvBuffer, auiBytes, auiRead);
    return eLastError;
}

void BSSaveDataSystemUtilityFile::SetIconImage(uint32_t auiWidth, uint32_t auiHeight, void* apvBuffer) {
    uiIconImageWidth = auiWidth;
    uiIconImageHeight = auiHeight;
    pvIconImageBuffer = apvBuffer;
}

void BSSaveDataSystemUtilityFile::SetSubTitle(const char* apcSubTitle) {
    BSstrcpy(static_cast<char*>(static_cast<void*>(cSubTitle)), 0x80u, apcSubTitle);
}

void BSSaveDataSystemUtilityFile::SetDirectoryName(const char* apcName) {
    BSstrcpy(cDirectoryName, 256, apcName);
}

void BSSaveDataSystemUtilityFile::SetDetail(const char* apcDetail) {
    BSstrcpy(cDetail, 1024, apcDetail);
}

void BSSaveDataSystemUtilityFile::SetBackgroundImagePath(const char* apcPath) {
    BSstrcpy(cBackgroundImagePath, 256, apcPath);
}

void BSSaveDataSystemUtilityFile::SetSaveCompleteCallback(
    void (*apSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&)
) {
    pSaveCompleteCallback = apSaveCompleteCallback;
}

uint32_t MessageMenu::GetCurrentButtonCount() {
    if (pMe) {
        return pMe->buttonList.NumUnfiltered();
    }
    else {
        return 0;
    }
}

void BGSSaveLoadFileEntry::GetFileTime(_FILETIME& arFileTime) {
    arFileTime = FileTime;
}
