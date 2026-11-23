#include "Fallout/Interface/Menus/StartMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSDevices/Xenon/XGamerProfile.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSContentPackage.h"
#include "BSSystemUtilities/BSDLCSystemUtility.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"
#include "BSSystemUtilities/BSSaveDataSystemUtilityFile.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/MessageMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/SurgeryMenu.h"
#include "Fallout/Interface/Menus/TextEntryMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Interface/Menus/TutorialMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "platform/Paths.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <new>

// The function-local timers (unguarded, zero-initialised): DoIdle's save message timer, ActionMappingIdle's hat repeat
// time (PC only) and SaveLoadIdle's save timer.
static uint32_t uitimer;
static uint32_t uiNextHatTime;
static uint32_t uitimer0;

// The ini setting of the gamepad rumble.
static SettingT<INIPrefSettingCollection> bGamePadRumble("bGamePadRumble:Controls", true);

// The labels and value strings of the options CreateMenuElements builds.
static SettingT<GameSettingCollection> sMouseSensitivity("sMouseSensitivity", "Mouse Sensitivity");
static SettingT<GameSettingCollection> sExitGameAffirm("sExitGameAffirm", "Exit Game");
static SettingT<GameSettingCollection> sActionMapping("sActionMapping", "Action Mapping");

// The upper-cased blue and green color names of the HUD and Pip-Boy color options, filled on first use.
static BSStringT<char> g_strBlue;
static BSStringT<char> g_strGreen;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> sCharGenQuest("sCharGenQuest:General", "00014E85");
SettingT<INISettingCollection> fGammaMin("fGammaMin:Display", 1.4f);
SettingT<INISettingCollection> fGammaMax("fGammaMax:Display", 0.6f);
SettingT<INISettingCollection> fObjectLODMin("fObjectLODMin:LOD", 1.0f);
SettingT<INISettingCollection> fObjectLODMax("fObjectLODMax:LOD", 15.0f);
SettingT<INISettingCollection> fObjectLODDefault("fObjectLODDefault:LOD", 5.0f);
SettingT<INISettingCollection> fItemLODMin("fItemLODMin:LOD", 1.0f);
SettingT<INISettingCollection> fItemLODMax("fItemLODMax:LOD", 15.0f);
SettingT<INISettingCollection> fItemLODDefault("fItemLODDefault:LOD", 2.0f);
SettingT<INISettingCollection> fActorLODMin("fActorLODMin:LOD", 2.0f);
SettingT<INISettingCollection> fActorLODMax("fActorLODMax:LOD", 15.0f);
SettingT<INISettingCollection> fActorLODDefault("fActorLODDefault:LOD", 6.5f);
SettingT<INISettingCollection> bFadeInScreenShot("bFadeInScreenShot:General", false);
NiTPointerList<Tile*> StartMenu::xHiddenTiles;
BSSimpleArray<StartMenu::Option*> StartMenu::pOptionsA;
BSSimpleArray<StartMenu::UserOption*> StartMenu::pUserOptionsA;
BSSimpleArray<StartMenu::VideoRes> StartMenu::kVideoResolutions;
HUDColorTable g_HUDColors = {
    MakeRGBA(0x1A, 0xFF, 0x80, 0xFF),
    MakeRGBA(0x2E, 0xCF, 0xFF, 0xFF),
    MakeRGBA(0xFF, 0xB6, 0x42, 0xFF),
    MakeRGBA(0xC5, 0xFF, 0xFF, 0xFF)
};

bool StartMenu::MainMenuIdle() {
    if (this->IsFlagSet(FLAG_INGAME)) {
        return false;
    }
#if XFNV_FIX_FAST_MAIN_MENU
    // lStewieAl's Tweaks and Engine Fixes, Faster Main Menu (bFasterTitleMenu; also the standalone "Faster Main Menu",
    // Nexus 67811, and its option to skip the delay till the last load screen): the start-up's welcome screens
    // (legal, Bethesda, Obsidian, "Please stand by") each stay up for fLoadingInitUpdateInterval plus a cross-fade,
    // about 50 s before the menu's items show, though the menu already exists. This skips to the last screen as soon
    // as the menu exists, as the game's own click below does (items at ~8 s in a headless run).
    if (this->IsFlagSet(FLAG_IIS_PREFADE) && LoadingMenu::pMe) {
        LoadingMenu::pMe->ForceFinish();
    }
#else
    // Mouse button 0, just released.
    if (this->IsFlagSet(FLAG_IIS_PREFADE) &&
        LoadingMenu::pMe &&
        appMain->GetControls()->GetMouseButton(0, Controls::isDepressed)) {
        LoadingMenu::pMe->ForceFinish();
    }
#endif
    if (this->IsFlagSet(FLAG_FADE_IN) &&
        !this->pTiles[eMainTitle]->HasFadeControl(Tile::kTileValue_Alpha) &&
        (!pPlayCredits || !(pPlayCredits->GetValue() > 0.0))) {
        const float fAlpha = this->pTiles[eMainTitle]->GetFloat(Tile::kTileValue_Alpha);
        if (fAlpha == 0.0) {
            this->Fade(eMainTitle, true, Tile::kTileValue_Alpha, 1.0f);
            LoadingMenu::SetupForStartMenu(1);
        }
        else if (fAlpha == 255.0f || XFNV_FIX_FAST_MAIN_MENU) {
            this->_UpdateProfileSpecificData();
            int32_t iMaxLevel = iMaxCharacterLevel.UInt();
            const int32_t iMinMaxLevel = 30;
            iMaxLevel += pDataHandler->GetNumDLCs() * 5;
            iMaxLevel = BSMIN(BSMAX(iMaxLevel, iMinMaxLevel), 50);
            iMaxCharacterLevel.UInt() = iMaxLevel;
            LoadingMenu::bInitialLoad = false;
            this->SetFlag(FLAG_FADE_IN, 0);
        }
    }
    if (this->IsFlagSet(FLAG_DLC_WAIT_MESSAGE)) {
        TileMenu* pMessageMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eMessageMenu));
        if (!pMessageMenu) {
            // No button list and no terminator: only the nine fixed arguments are passed.
            Interface::CreateMessageMenu(
                static_cast<const char*>(sLoadingContentMessage),
                nullptr,
                nullptr,
                nullptr,
                0,
                0x12,
                0.0f,
                0.0f,
                ""
            );
        }
        else if (pMessageMenu->GetMenu()->GetFadeState() == 1) {
            this->SetFlag(FLAG_DLC_WAIT_MESSAGE, 0);
            ArchiveManager::ReloadArchiveFilenamesAndIndices();
            StartMenu::LoadDownloadedContent();
        }
    }
    if (this->IsFlagSet(FLAG_NEW_DLC_INSTALLED)) {
        StartMenu::pMe->SetFlag(FLAG_DLC_WAIT_MESSAGE, 1);
        StartMenu::pMe->SetFlag(FLAG_NEW_DLC_INSTALLED, 0);
    }
    if (this->IsFlagSet(FLAG_DLC_CORRUPT_MESSAGE) && !Menu::GetMenuByClass(Interface::eMessageMenu)) {
        this->SetFlag(FLAG_DLC_CORRUPT_MESSAGE, 0);
        const char* pOk = sOk.String();
        const char* pText = sCorruptContentMessage.String();
        Interface::CreateMessageMenu(
            pText,
            nullptr,
            nullptr,
            nullptr,
            0,
            0x12,
            0.0f,
            0.0f,
            pOk,
            static_cast<const char*>(nullptr)
        );
    }
    return true;
}

StartMenu::Option::Option(const char* apDisplayText, void (*apf)(), uint32_t aiModes)
  : pDisplayText(apDisplayText), pfChooseFn(apf), iModes(aiModes) {}

StartMenu::Option::~Option() {}

bool StartMenu::Option::IsForModes(uint32_t aiModes) {
    return (aiModes & iModes) != 0;
}

StartMenu::UserOption::UserOption(
    const char* apDisplayText,
    void (*apCallBack)(UserOption*),
    uint32_t aiModes,
    UserOptionType aeType,
    uint32_t aiNumSettings,
    uint32_t aiArrowSpacing,
    ...
)
  : Option(apDisplayText, nullptr, aiModes), iVal(0), iDefaultVal(0), eType(aeType), iNumSettings(aiNumSettings),
    iArrowSpacing(aiArrowSpacing), pfChangeFn(apCallBack) {
    // pTemplate and pValueStrings deliberately stay unset for another type.
    if (aeType < UOPT_TOGGLE) {
        return;
    }
    if (aeType <= UOPT_TOGGLE_NOWRAP) {
        pTemplate = "lb_toggle_template";
        // The allocation size is count times the pointer size, saturated to 0xFFFFFFFF on overflow, as the game's
        // operator new[] does.
        uint64_t uiSize = static_cast<uint64_t>(iNumSettings) * sizeof(const char*);
        pValueStrings = static_cast<const char**>(
            ourMalloc(uiSize > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(uiSize))
        );
        va_list args;
        va_start(args, aiArrowSpacing);
        for (uint32_t i = 0; i < iNumSettings; ++i) {
            pValueStrings[i] = va_arg(args, const char*);
        }
        va_end(args);
    }
    else if (aeType == UOPT_METER) {
        pTemplate = "lb_meter_template";
        pValueStrings = nullptr;
    }
}

// The value strings are a raw buffer the option owns.
StartMenu::UserOption::~UserOption() {
    ourFree(pValueStrings);
}

StartMenu::OptionListBox::OptionListBox() : ListBox<StartMenu::Option*>() {}

// Buttons 15 and 16 have no highlight target in the option lists.
Tile* StartMenu::OptionListBox::GetHighlightTarget(int32_t aiButton) {
    if (aiButton == 15 || aiButton == 16) {
        return nullptr;
    }
    return ListBox<Option*>::GetHighlightTarget(aiButton);
}

StartMenu::IdleTimer::IdleTimer() : uiT0(0), uiPaused(0) {}

uint32_t StartMenu::IdleTimer::GetElapsedTime() {
    if (!static_cast<uint8_t>(GetStarted())) {
        return 0;
    }
    if (static_cast<uint8_t>(GetPaused())) {
        return uiPaused - uiT0;
    }

    return BSGetTickCount() - uiT0;
}

void StartMenu::IdleTimer::Stop() {
    uiPaused = 0;
    uiT0 = 0;
}

void StartMenu::IdleTimer::Start() {
    uiT0 = BSGetTickCount();
    uiPaused = 0;
}

int32_t StartMenu::IdleTimer::GetStarted() {
    return uiT0 != 0;
}

int32_t StartMenu::IdleTimer::GetPaused() {
    return uiPaused != 0;
}

StartMenu::IdleTimer* StartMenu::IdleTimer::Instance() {
    if (!StartMenu::IdleTimer::pInstance) {
        StartMenu::IdleTimer::pInstance = new StartMenu::IdleTimer;
    }
    return StartMenu::IdleTimer::pInstance;
}

void StartMenu::IdleTimer::Delete() {
    if (StartMenu::IdleTimer::pInstance) {
        ourFree(StartMenu::IdleTimer::pInstance);
    }
    StartMenu::IdleTimer::pInstance = nullptr;
}

StartMenu::VideoRes::VideoRes() : unk00(), unk08(0), unk0A(0) {}

// unk00 releases itself.
StartMenu::VideoRes::~VideoRes() {}

StartMenu::StartMenu()
  : Menu(), pTiles(), lbMain(), lbSettings(), lbConfirm(), lbOptions(), lbDownloads(), lbSaveLoad(),
    pDirtyOption(nullptr), uiFlags(0), fSaveLoadThreshold(0.0f), pSaveGame(nullptr), pBrightnessOption(nullptr),
    spAlphaMap(nullptr), pDownloadsOption(nullptr), pLoadOption(nullptr), pContinueOption(nullptr),
    uiCurrentControlsDevice(255), pRumbleOption(nullptr), ENABLED_TRAIT(Tile::AddUserTrait("_enabled", -1)) {
    g_startMenuLastControllerInputTime = 0;
    BSAudio::QInstance().PauseWorldVoices();
}

// The list boxes, spAlphaMap and the Menu base release themselves.
StartMenu::~StartMenu() {
    if (IsFlagSet(StartMenu::FLAG_RESET_HUD_OPACITY)) {
        HUDMainMenu::ResetHUDOpacity(fHudOpacity);
    }
    BSAudio::QInstance().UnPauseWorldVoices();
    if (StartMenu::MainMenuIsActive()) {
        FalloutAudio::SetMainMenuMusic(false);
        FalloutAudioMedia::MediaFadeOut(2000, MEDIA_STANDBY);
    }
    pSaveLoadManager->DeleteSaveGameList();
    StartMenu::pMe = nullptr;
    StartMenu::xHiddenTiles.RemoveAll();
    StartMenu::IdleTimer::Instance();
    StartMenu::IdleTimer::Delete();
}

void StartMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < NUM_TILES) {
        pTiles[aiID] = apTile;
    }
}

void StartMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
    if (apTarget && (aiID == 100 || aiID == 101)) {
        apTarget->Set(4085, -1);
    }
}

void StartMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (aiID == 5) {
        StartMenu::ChooseBack();
    }
    if (!IsFlagSet(StartMenu::FLAG_DLC_WAIT_MESSAGE)) {
        switch (aiID) {
            case 4: {
                if (lbSaveLoad.IsEnabled()) {
                    SetFlag(StartMenu::FLAG_DELETE_MODE, 1);
                    ConfirmSaveLoad();
                    SetFlag(StartMenu::FLAG_DELETE_MODE, 0);
                }
                else if (IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
                    const char* pYes = sYes.String();
                    const char* pNo = sNo.String();
                    Interface::CreateMessageMenu(
                        static_cast<const char*>(sResetToDefaults),
                        static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr),
                        &StartMenu::ConfirmResetToDefaults,
                        0,
                        18,
                        0.0f,
                        0.0f,
                        pNo,
                        pYes,
                        nullptr
                    );
                }
                else {
                    lbOptions.Traverse(&StartMenu::ResetToDefaults, 0, std::numeric_limits<int32_t>::max());
                }
                break;
            }
            case 15: {
                if (lbOptions.IsVisible()) {
                    lbOptions.Traverse(&StartMenu::DeleteOption, 0, std::numeric_limits<int32_t>::max());
                    NextControlsDevice();
                    PopulateControlsListBox();
                }
                break;
            }
            case 1: {
                HideOptionsContainer();
                StartMenu::ChooseSettings();
                break;
            }
            default: {
                if (!aiID && !IsFlagSet(StartMenu::FLAG_LOADING_SAVEGAMES)) {
                    HideOptionsContainer();
                    if (StartMenu::pMe->lbDownloads.IsEnabled()) {
                        StartMenu::pMe->Fade(StartMenu::eDLCContainer, false, 0x80000000u, -1.0f);
                        StartMenu::pMe->lbDownloads.SetEnabled(false);
                    }
                    Fade(StartMenu::eSettingsContainer, false, 0x80000000u, -1.0f);
                    lbSettings.SetEnabled(false);
                    lbSettings.DeselectAll(true);
                    lbSaveLoad.SetEnabled(true);
                    // Highlight is called through a pointer so that it goes through the list's vtable; a call on the
                    // member itself would be devirtualised.
                    ListBox<BGSSaveLoadFileEntry*>* pSaveLoad = &lbSaveLoad;
                    pSaveLoad->Highlight(nullptr);
                    SetFlag(StartMenu::FLAG_SAVELOAD_BACKOUT, 0);
                    UpdateHighlightedSaveGameInfo();
                    lbSaveLoad.SetEnabled(false);
                    Fade(StartMenu::eSaveLoadContainer, false, 0x80000000u, -1.0f);
                    lbConfirm.SetEnabled(false);
                    Fade(StartMenu::eConfirmContainer, false, 0x80000000u, -1.0f);
                    lbMain.SetEnabled(true);
                    lbMain.DeselectAll(true);
                    Fade(StartMenu::eBackButton, false, 0x80000000u, -1.0f);
                    if (IsFlagSet(StartMenu::FLAG_OPTIONS_DIRTY)) {
                        StartMenu::SaveSettings();
                        SetFlag(StartMenu::FLAG_OPTIONS_DIRTY, 0);
                    }
                }
                break;
            }
        }
        if (apTarget && BSGetTickCount() - g_startMenuLastControllerInputTime > 500) {
            ListBox<StartMenu::Option*>* pOptionLBs[4];
            pOptionLBs[0] = &lbMain;
            pOptionLBs[1] = &lbSettings;
            pOptionLBs[2] = &lbConfirm;
            pOptionLBs[3] = &lbOptions;
            for (ListBox<StartMenu::Option*>*& pListBox : pOptionLBs) {
                StartMenu::Option* pOpt = pListBox->GetDataFor(apTarget);
                if (pOpt && pOpt->pfChooseFn) {
                    if (pOpt->pfChooseFn != &StartMenu::ChooseActionMap) {
                        pListBox->Select(apTarget);
                    }
                    if (pListBox != &lbConfirm && pOpt->pfChooseFn != &StartMenu::ChooseBack) {
                        pListBox->SetEnabled(false);
                    }
                    pOpt->pfChooseFn();
                    g_startMenuLastControllerInputTime = BSGetTickCount();
                    return;
                }
            }
            pSaveGame = lbSaveLoad.GetDataFor(apTarget);
            if (pSaveGame && (IsFlagSet(StartMenu::FLAG_SAVE_MODE) || !pSaveGame->IsCorrupt())) {
                ConfirmSaveLoad();
            }
            else if (!lbSaveLoad.IsListItem(apTarget) || !IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
                if (lbDownloads.IsListItem(apTarget) && lbDownloads.GetDataFor(apTarget) == -1) {
                    uint32_t uiPlayerIndex = XGamerProfile::Instance()->m_PlayerIndex;
                    BSSystemUtility::QInstance().QDLCUtility().ShowMarketplace(uiPlayerIndex);
                }
            }
            else if (lbSaveLoad.NumItems() <= 1000 || !lbSaveLoad.NumItems()) {
                apTarget->Set(Tile::kTileValue_String, sMenuDisplayShortXBoxSaveMessage.String(), true);
                lbSaveLoad.Select(apTarget);
                pTiles[6]->Set(Tile::kTileValue_User0, 0);
                StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVE_WAIT_MESSAGE, 1);
            }
            else {
                return;
            }
            StartMenu::UserOption* pUOpt = dynamic_cast<StartMenu::UserOption*>(
                lbOptions.GetDataFor(apTarget->pParent)
            );
            if (pUOpt &&
                (aiID == 100 ||
                 aiID == 101 ||
                 pUOpt->eType == StartMenu::UserOption::UOPT_TOGGLE ||
                 (pUOpt->eType == StartMenu::UserOption::UOPT_TOGGLE_NOWRAP &&
                  pUOpt->iVal < pUOpt->iNumSettings - 1))) {
                pUOpt->iVal = (2 * (aiID != 100) - 1 + (pUOpt->iNumSettings + pUOpt->iVal)) % pUOpt->iNumSettings;
                pDirtyOption = pUOpt;
                float fBrightness = pMenu->GetFloat(Tile::kTileValue_Brightness);
                apTarget->Set(Tile::kTileValue_Brightness, fBrightness, true);
            }
        }
    }
}

// The highlight is a virtual call on the list, so the call is made through a pointer.
void StartMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (apTarget && apTarget->pParent && this->lbOptions.IsListItem(apTarget->pParent)) {
        ListBoxBase* pOptions = &this->lbOptions;
        pOptions->Highlight(apTarget->pParent);
    }
    if (this->lbSaveLoad.IsListItem(apTarget)) {
        if (InterfaceManager::Has360Controller()) {
            this->UpdateHighlightedSaveGameInfo();
        }
        else {
            const int32_t iCursorY = InterfaceManager::Instance()->GetCursorY();
            if (static_cast<double>(this->pTiles[eSaveLoadContainer]->GetRealY()) <= static_cast<double>(iCursorY)) {
                this->UpdateHighlightedSaveGameInfo();
            }
        }
    }
    if (this->lbOptions.IsEnabled() && this->lbOptions.GetHighlightedData() == this->pBrightnessOption) {
        this->pTiles[eBrightnessGradient]->Set(Tile::kTileValue_Visible, 1);
    }
}

void StartMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    bool bArrow = aiID == 100 || aiID == 101;
    if (apTarget && bArrow) {
        float fValue = pMenu->GetFloat(4085);
        apTarget->Set(4085, fValue, true);
    }

    if (!InterfaceManager::Has360Controller() && lbSaveLoad.IsListItem(apTarget)) {
        UpdateHighlightedSaveGameInfo();
    }
    if (!bArrow && pTiles[17]) {
        pTiles[17]->Set(4003, 0);
    }
}

void StartMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    if (aiID == 102) {
        StartMenu::UserOption* pUOpt = dynamic_cast<StartMenu::UserOption*>(lbOptions.GetHighlightedData());
        if (pUOpt) {
            Tile* pParent = apTarget->pParent;
            if (pParent == lbOptions.GetHighlightedItem()) {
                int32_t iOld = pUOpt->iVal;
                double dScale = apTarget->pParent->GetFloat(Tile::kTileValue_User3);
                double dCursorX = static_cast<double>(InterfaceManager::Instance()->GetCursorX());
                double dOffset = (dCursorX - apTarget->GetRealX()) * dScale;
                float fX = dOffset / apTarget->GetFloat(4017);
                pUOpt->iVal = static_cast<int32_t>(std::ceil(fX));
                if (pUOpt->iVal < 0) {
                    pUOpt->iVal = 0;
                }
                else if (pUOpt->iVal > pUOpt->iNumSettings - 1) {
                    pUOpt->iVal = pUOpt->iNumSettings - 1;
                }
                if (iOld != pUOpt->iVal) {
                    pDirtyOption = pUOpt;
                }
            }
        }
    }
}

void StartMenu::DoIdle() {
    if (!LoadingMenu::pLoadingThread->QIsSuspended()) {
        return;
    }
    if (StartMenu::pTextEntryMenu) {
        StartMenu::pTextEntryMenu->DoIdle();
        if (StartMenu::pTextEntryMenu->IsDone()) {
            delete StartMenu::pTextEntryMenu;
            StartMenu::pTextEntryMenu = nullptr;
        }
    }
    if (LoadingMenu::pMe) {
        LoadingMenu::pMe->Update();
        if (this->IsFlagSet(FLAG_IIS_PREFADE) && LoadingMenu::pMe->GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE)) {
            LoadingMenu::pMe->MakeLoadScreenList();
            this->SetFlag(FLAG_IIS_PREFADE, 0);
            this->SetFlag(FLAG_FADE_IN, 1);
        }
    }
    this->MainMenuIdle();
    if (this->ActionMappingIdle()) {
        return;
    }
    if (this->IsFlagSet(FLAG_SHOWING_CREDITS)) {
        if (this->pTiles[eMainTitle]->GetFadeFinished(Tile::kTileValue_Alpha) &&
            this->pTiles[eMainTitle]->GetFloat(Tile::kTileValue_Alpha) == 0.0 &&
            !Menu::GetMenuByClass(Interface::eCreditsMenu)) {
            LoadingMenu::ResumeBackgroundThread();
            CreditsMenu::Create();
        }
        return;
    }
    const bool bSaveLoadUp = this->SaveLoadIdle();
    if (!bSaveLoadUp && this->IsFlagSet(static_cast<Flag>(FLAG_SAVE_WAIT_MESSAGE | FLAG_SAVE_WAIT_SAVING))) {
        if (this->IsFlagSet(FLAG_SAVE_WAIT_MESSAGE)) {
            TileMenu* pMessageMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eMessageMenu));
            // Menu fade state 1: shown, its fade-in finished.
            if (pMessageMenu && pMessageMenu->GetMenu()->GetFadeState() == 1) {
                this->SetFlag(FLAG_SAVE_WAIT_MESSAGE, 0);
                this->SetFlag(FLAG_SAVE_WAIT_SAVING, 1);
                uitimer = BSGetTickCount() + 3000;
                StartMenu::SaveSettings();
            }
            else if (!MessageMenu::GetQueueHead()) {
                StartMenu::SaveSettings();
                this->SetFlag(FLAG_SAVE_WAIT_MESSAGE, 0);
                this->SetFlag(FLAG_OPTIONS_DIRTY, 0);
            }
        }
        else if (BSGetTickCount() >= uitimer) {
            Interface::CloseMessageMenu();
            this->SetFlag(FLAG_SAVE_WAIT_SAVING, 0);
            this->SetFlag(FLAG_OPTIONS_DIRTY, 0);
        }
    }
    if (this->pDirtyOption) {
        this->DisplayOption(this->pDirtyOption, nullptr);
        if (this->pDirtyOption->pfChangeFn) {
            this->pDirtyOption->pfChangeFn(this->pDirtyOption);
            this->SetFlag(FLAG_OPTIONS_DIRTY, 1);
        }
        this->pDirtyOption = nullptr;
    }
    if (this->IsFlagSet(FLAG_MOUNT_DLC)) {
        TileMenu* pMessageTile = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eMessageMenu));
        Menu* pMessageMenu = pMessageTile ? pMessageTile->GetMenu() : nullptr;
        if (pMessageMenu && pMessageMenu->GetFadeState() == 1) {
            StartMenu::pMe->SetFlag(FLAG_MOUNT_DLC, 0);
            BSSystemUtility::QInstance().QDLCUtility().MountDLC(0, &StartMenu::MountDLCCallback);
        }
    }
}

bool StartMenu::DoKey(int32_t aiKey) {
    return IsFlagSet(StartMenu::FLAG_IIS_PREFADE) && LoadingMenu::pMe && LoadingMenu::pMe->ForceFinish();
}

int32_t StartMenu::GetClass() {
    return 1013;
}

// The jump table runs aiButton 3 to 16; 6, 7, 8, 10, 13 and 14 return false like the default.
bool StartMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::eStartMenu)) {
        return false;
    }
    if (this->IsFlagSet(FLAG_DLC_WAIT_MESSAGE)) {
        return true;
    }
    if (this->IsFlagSet(FLAG_SAVE_WAIT_SAVING) && this->IsFlagSet(FLAG_SAVE_MODE)) {
        return true;
    }
    if (aiButton > 0) {
        if (this->IsFlagSet(FLAG_IIS_PREFADE) && LoadingMenu::pMe) {
            return LoadingMenu::pMe->ForceFinish();
        }
        // Device 3 is the 360 controller.
        if (StartMenu::pMe->uiCurrentControlsDevice == 3 && this->WaitingForControlRemapButtonPress()) {
            const uint32_t uiAction = this->lbOptions.GetHighlightedData()->iModes;
            const int32_t iKey = ConvertSpecialInputKeyToControllerKey(aiButton);
            if (appMain->GetControls()->SetControl(uiAction, 3, static_cast<uint8_t>(iKey))) {
                if (InterfaceManager::Has360Controller()) {
                    this->pTiles[eControlsHeader]->Set(Tile::kTileValue_Visible, 0);
                }
                else {
                    this->pTiles[eControlsHeader]
                        ->Set(Tile::kTileValue_String, pDeviceNamesA[this->uiCurrentControlsDevice]->String(), true);
                }
                this->lbOptions.SetEnabled(true);
                this->lbOptions.DeselectAll(true);
                this->lbOptions.Traverse(&StartMenu::SetActionForDevice, 0, std::numeric_limits<int32_t>::max());
                this->SetFlag(FLAG_OPTIONS_DIRTY, 1);
                this->SetFlag(FLAG_USER_IS_ACTION_MAPPING, 1);
                this->pTiles[eBackButton]->Set(Tile::kTileValue_Target, 1);
            }
            else if (!Menu::GetMenuByClass(Interface::eMessageMenu)) {
                // Menu sound 10 plays UIPopUpMessageGeneral (PC numbering).
                InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                const char* pOk = static_cast<const char*>(sOk);
                const char* pText = sButtonLocked.String();
                Interface::CreateMessageMenu(
                    pText,
                    nullptr,
                    nullptr,
                    nullptr,
                    1,
                    0x12,
                    0.0f,
                    0.0f,
                    pOk,
                    static_cast<const char*>(nullptr)
                );
            }
            return true;
        }
    }
    switch (aiButton) {
        case 3:
        case 4: {
            ListBoxBase* pOptions = &this->lbOptions;
            if (!this->lbOptions.IsEnabled() || !pOptions->GetHighlightedItem()) {
                return false;
            }
            const int32_t iID = (aiButton != 4) + eLeftArrow;
            Tile* pArrow = static_cast<Tile*>(pOptions->GetHighlightedItem())->GetChildByID(iID);
            if (pArrow && pArrow->IsVisible() && pArrow->IsTrue(Tile::kTileValue_Target)) {
                this->DoClick(iID, pArrow);
                pArrow->PlayTileSound(Tile::kTileValue_ClickSound);
                return true;
            }
            if (iID == eRightArrow &&
                this->lbOptions.GetHighlightedData() &&
                this->lbOptions.GetHighlightedData()->pfChooseFn == &StartMenu::ChooseActionMap) {
                this->DoClick(-1, static_cast<Tile*>(pOptions->GetHighlightedItem()));
                // Plays the sound on the arrow even when there is none.
                pArrow->PlayTileSound(Tile::kTileValue_ClickSound);
                return true;
            }
            return false;
        }
        case 5:
        case 9:
            if (this->IsFlagSet(FLAG_INGAME) ||
                !this->pTiles[eWelcomeText]->IsTrue(Tile::kTileValue_Visible) ||
                this->pTiles[eWelcomeText]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
                return false;
            }
            this->_UpdateProfileSpecificData();
            IdleTimer::Instance()->Stop();
            return true;
        case 11:
            if (!this->pTiles[eDeviceButton]->IsVisible() ||
                !this->pTiles[eDeviceButton]->IsTrue(Tile::kTileValue_Target)) {
                return false;
            }
            this->pTiles[eDeviceButton]->PlayTileSound(Tile::kTileValue_ClickSound);
            this->DoClick(eDeviceButton, this->pTiles[eDeviceButton]);
            return true;
        case 12:
            if (!this->pTiles[eDefaultsButton]->IsVisible() ||
                !this->pTiles[eDefaultsButton]->IsTrue(Tile::kTileValue_Target)) {
                return false;
            }
            this->pTiles[eDefaultsButton]->PlayTileSound(Tile::kTileValue_ClickSound);
            this->DoClick(eDefaultsButton, this->pTiles[eDefaultsButton]);
            return true;
        case 15:
        case 16: {
            ListBoxBase* pOptions = &this->lbOptions;
            if (!this->lbOptions.IsEnabled()) {
                return false;
            }
            UserOption* pOption = dynamic_cast<StartMenu::UserOption*>(this->lbOptions.GetHighlightedData());
            if (!pOption || pOption->eType != UserOption::UOPT_METER) {
                return false;
            }
            int32_t iStep = static_cast<int32_t>(BSRoundToNearest(
                static_cast<Tile*>(pOptions->GetHighlightedItem())->GetFloat(Tile::kTileValue_User1),
                1.0f
            ));
            int32_t iID = eRightArrow;
            if (aiButton == 15) {
                iStep *= -1;
                iID = eLeftArrow;
            }
            pOption->iVal += iStep;
            if (pOption->iVal < 0) {
                pOption->iVal = 0;
            }
            else if (static_cast<uint32_t>(pOption->iVal) >= pOption->iNumSettings) {
                pOption->iVal = pOption->iNumSettings - 1;
            }
            this->pDirtyOption = pOption;
            static_cast<Tile*>(pOptions->GetHighlightedItem())
                ->GetChildByID(iID)
                ->PlayTileSound(Tile::kTileValue_ClickSound);
            return true;
        }
        default: return false;
    }
}

// Button 10 ends a running action mapping, or goes back while the back button is the target, at most once per
// half second (an unsigned difference of tick counts).
bool StartMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton != 10) {
        return false;
    }
    if (this->IsFlagSet(FLAG_USER_IS_ACTION_MAPPING)) {
        this->SetFlag(FLAG_USER_IS_ACTION_MAPPING, 0);
        return true;
    }
    if (!this->pTiles[eBackButton]->IsTrue(Tile::kTileValue_Target) ||
        BSGetTickCount() - g_startMenuLastControllerInputTime <= 500) {
        return false;
    }
    InterfaceManager::Instance()->ClearMenuButton(Interface::eButtonB);
    StartMenu::ChooseBack();
    g_startMenuLastControllerInputTime = BSGetTickCount();
    return true;
}

void StartMenu::PrepSaveLoad() {
    if (lbSaveLoad.IsEmpty()) {
        Tile* pTile = lbSaveLoad.AddItem(0, nullptr, nullptr, "lb_newsave_template");
        pTile->Set(4036, sPleaseStandBy.String(), true);
        pTile->Set(ENABLED_TRAIT, 0);
        lbSaveLoad.SetEnabled(true);
        Fade(StartMenu::eSaveLoadContainer, true, Tile::kTileValue_Invalid, -1.0f);
        Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
        const char* pDelete = sDelete.String();
        pTiles[4]->Set(4036, pDelete, true);
        pTiles[5]->Set(4015, 0);
        SetFlag(StartMenu::FLAG_LOADING_SAVEGAMES, 1);
    }
    else {
        lbSaveLoad.Filter(nullptr);
        FinishPrepSaveLoad();
    }
}

void StartMenu::NextControlsDevice() {
    for (int8_t i = 4; i; --i) {
        IncrementControlsDevice(&this->uiCurrentControlsDevice);
        if (this->IsControlsDeviceValid()) {
            break;
        }
    }
}

bool StartMenu::IsControlsDeviceValid() {
    if (this->uiCurrentControlsDevice == 0xFF) {
        return false;
    }
    if (InterfaceManager::Has360Controller()) {
        return this->uiCurrentControlsDevice == 3;
    }
    if (this->uiCurrentControlsDevice == 3) {
        return false;
    }
    return appMain->GetControls()->HasInputInterfaces(this->uiCurrentControlsDevice) != 0;
}

// The AutoMemContext constructor and destructor are called explicitly, as in Create, and each option is read through
// the array's item getter at every use. Warning is an empty stub; it is passed 0x12 before the message. 0x40000000 is a
// PC flag bit that the Flag enum does not name.
void StartMenu::_Create(bool abInGame, bool abForceReload) {
    using enum Tile::eTileValue;

    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Menus\\StartMenu.cpp",
        426
    );
    TES::pInstance
        ->CreateTextureImage("Textures\\Interface\\Main\\screenshot_alpha_frame.dds", this->spAlphaMap, false, false);
    this->SetFlag(FLAG_INGAME, abInGame);
    this->SetFlag(FLAG_FORCE_RELOAD, abForceReload);
    this->SetFlag(LoadingMenu::bInitialLoad && !abInGame ? FLAG_IIS_PREFADE : FLAG_FADE_IN, 1);
    this->lbMain.Initialize(this->pTiles[eMainContainer], "lb_item_template");
    this->lbSettings.Initialize(this->pTiles[eSettingsContainer], "lb_item_template");
    this->lbConfirm.Initialize(this->pTiles[eConfirmContainer], "lb_item_template");
    this->lbOptions.Initialize(this->pTiles[eOptionsContainer], nullptr);
    this->lbSaveLoad.Initialize(this->pTiles[eSaveLoadContainer], "lb_saveload_template");
    this->lbDownloads.Initialize(this->pTiles[eDLCContainer], "lb_item_template");
    this->SetFlag(FLAG_DLC_NEEDS_UPDATE, !abInGame);

    char acPath[0x104];
    if (pSaveLoadManager) {
        pSaveLoadManager->GetSaveDirectoryPath(acPath);
    }
    else {
        BSstrcpy(acPath, 0x104, BSUtilities::GetPersonalPath());
        BSstrcat(acPath, 0x104, sLocalSavePath.String());
    }
    BSstrcat(acPath, 0x104, "*");
    BSstrcat(acPath, 0x104, ".fos");
    const bool bSavesExist = !platform::FindFiles(acPath).empty();

    for (Option* pOption : StartMenu::pOptionsA) {
        Tile* pTile = nullptr;
        const bool bDisabled = (pOption->pfChooseFn == &StartMenu::ChooseSave &&
                                !pSaveLoadManager->IsSavingAllowed(false)) ||
            (pOption->pfChooseFn == &StartMenu::ChooseContinue && (abInGame ? abForceReload : !bSavesExist)) ||
            (pOption->pfChooseFn == &StartMenu::ChooseLoad && !bSavesExist) ||
            (pOption->pfChooseFn == &StartMenu::ChooseHelp && Menu::GetMenuByClass(TutorialMenu::GetClassStatic()));
        if ((abInGame && pOption->IsForModes(MODE_PAUSE)) || (!abInGame && pOption->IsForModes(MODE_MAIN))) {
            pTile = this->lbMain.AddItem(pOption, nullptr, nullptr, nullptr);
        }
        else if (pOption->IsForModes(MODE_SETTINGS)) {
            pTile = this->lbSettings.AddItem(pOption, nullptr, nullptr, nullptr);
        }
        if (pOption->pfChooseFn == &StartMenu::ChooseDownloads) {
            this->pDownloadsOption = pTile;
        }
        else if (pOption->pfChooseFn == &StartMenu::ChooseContinue) {
            this->pContinueOption = pTile;
        }
        else if (pOption->pfChooseFn == &StartMenu::ChooseLoad) {
            this->pLoadOption = pTile;
        }
        if (pTile) {
            pTile->Set(kTileValue_String, pOption->pDisplayText, true);
            if (bDisabled) {
                pTile->Set(this->ENABLED_TRAIT, 0);
            }
        }
    }

    const uint32_t uiMainCount = this->lbMain.NumItems();
    this->pTiles[eMainContainer]->Set(Tile::AddUserTrait("_number_of_visible_items", -1), uiMainCount);
    const uint32_t uiSettingsCount = this->lbSettings.NumItems();
    this->pTiles[eSettingsContainer]->Set(Tile::AddUserTrait("_number_of_visible_items", -1), uiSettingsCount);
    this->pTiles[eVersionText]->Set(kTileValue_String, "1.4.0.525", true);
    this->pTiles[eWelcomeText]->Set(kTileValue_String, sPressStart.String(), true);
    this->pTiles[eConfirmWarning]->Set(kTileValue_String, sConfirmWarning.String(), true);
    this->pTiles[eDeviceButton]->Set(kTileValue_String, sDevice.String(), true);
    this->pTiles[eBackButton]->Set(kTileValue_String, sBack.String(), true);
    this->pTiles[eRedeemCodeButton]->Set(kTileValue_Visible, 0);
    const bool bLowRes = InterfaceManager::GetRealScreenWidth() <= 640.0;
    if (bLowRes && this->pTiles[eMainTitle]) {
        const float fY = static_cast<float>(this->pTiles[eMainTitle]->GetFloat(kTileValue_Y) + 25.0);
        this->pTiles[eMainTitle]->Set(kTileValue_Y, fY, true);
    }
    this->pMenu->Set(kTileValue_User0, Interface::GetAspectRatioRatio() > 1.333299994468689 ? 1 : 0);

    if (abInGame) {
        this->pTiles[eMainTitle]->Set(kTileValue_Visible, 0);
        this->pMenu->Set(kTileValue_Alpha, 0x40);
        this->Fade(eMainContainer, true, static_cast<int32_t>(0x80000000), -1.0f);
        this->lbMain.SetEnabled(true);
        this->InitializeUserOptions();
        if (this->pTiles[ePauseBackground]->GetModel()) {
            this->pTiles[ePauseBackground]->GetModel()->SetScale(1.3333f);
        }
        else {
            Warning("Pause Menu: Background tile's model was NULL");
        }
        for (int32_t i = 0; i < 10; ++i) {
            const int32_t iMenuID = InterfaceManager::Instance()->GetEnterStack(i);
            if (iMenuID == StartMenu::GetClassStatic()) {
                break;
            }
            if (!StartMenu::ShouldHideMenu(iMenuID)) {
                continue;
            }
            Tile* pMenuTile = Menu::GetMenuByClass(iMenuID);
            if (pMenuTile && pMenuTile->GetMenu() && pMenuTile->GetMenu()->GetFadeState() == 1) {
                pMenuTile->Set(kTileValue_Visible, 0);
                StartMenu::xHiddenTiles.AddTail(pMenuTile);
            }
        }
        if (Interface::IsInRenderedMenu()) {
            appMain->RenderMenuBackground();
        }
    }
    else {
        pVATS.SetMode(VATS::VATS_MODE_NONE, false);
        this->pTiles[eMainTitle]->Set(kTileValue_Visible, 1);
        if (Interface::IsTopMenuID(Interface::eStartMenu)) {
            this->pTiles[eMainTitle]->Set(kTileValue_Alpha, 0.0f, true);
        }
        else {
            this->pTiles[eMainTitle]->Set(kTileValue_Alpha, 255.0f, true);
        }
        this->SetFlag(FLAG_SAVE_OPTIONS, 1);
    }

    if (this->IsFlagSet(FLAG_FORCE_RELOAD)) {
        StartMenu::ChooseLoad();
    }
    if (StartMenu::MainMenuIsActive()) {
        Interface::DisableMessageMenu(false);
    }
    else {
        MessageMenu::SetPauseMenuMode(true);
    }
    if (Interface::InDialog()) {
        Interface::SetDialogPause(true);
    }
    if (pPlayCredits && pPlayCredits->GetValue() != 0.0) {
        StartMenu::pMe->SetFlag(FLAG_SHOWING_CREDITS, 1);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_END_GAME_CREDITS, 1);
        char acMusic[0x40];
        ourMemset(acMusic, 0, 0x40);
        BSsprintf(acMusic, 0x40, "Data\\Music\\Special\\EndCredits.mp3");
        FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, acMusic, 2000, true, true, 0.0f, 0);
        FalloutAudioMedia::MediaPlay();
    }
    if (!StartMenu::pMe->IsFlagSet(FLAG_INGAME)) {
        this->pTiles[eMainContainer]->Set(Tile::AddUserTrait("_xbox_extra", -1), 25);
    }
    this->PrepForVisibility(false);
    BSNetworkSystemUtility::QInstance().RegisterCallback(&StartMenu::OnNetworkEvent);
    if (pSaveLoadGame) {
        pSaveLoadGame->SetSaveLoadFailed(false);
    }
    pContext->~AutoMemContext();
}

bool StartMenu::IsFlagSet(Flag aeFlag) {
    return (this->uiFlags & aeFlag) != 0;
}

void StartMenu::SetFlag(Flag aeFlag, uint8_t abOn) {
    if (abOn) {
        this->uiFlags |= aeFlag;
    }
    else {
        this->uiFlags &= ~aeFlag;
    }
}

void StartMenu::PopulateOptionsListBox(uint32_t auiModes) {
    _XINPUT_STATE pState;
    ListBox<StartMenu::Option*>* pOptions = &lbOptions;
    pOptions->RemoveAll();
    lbOptions.SetEnabled(true);
    Fade(StartMenu::eOptionsContainer, true, 0x80000000u, -1.0f);
    const char* pDefaultsText = sDefaults.String();
    pTiles[eDefaultsButton]->Set(Tile::kTileValue_String, pDefaultsText, true);
    for (UserOption* pOption : StartMenu::pUserOptionsA) {
        if (!pOption->IsForModes(auiModes)) {
            continue;
        }
        // The controller and rumble options are left out when XInputGetState fails (no controller in port 0).
        if ((pOption->pfChangeFn == &SetXboxController || pOption->pfChangeFn == &StartMenu::SetRumble) &&
            XInputGetState(0, &pState)) {
            pRumbleOption = nullptr;
            continue;
        }
        const char* pTemplate = pOption->pTemplate;
        const char* pDisplayText = pOption->pDisplayText;
        Tile* pItem = lbOptions.AddItem(pOption, pDisplayText, nullptr, pTemplate);
        if (pOption->pfChangeFn == &StartMenu::SetRumble) {
            pRumbleOption = pItem;
            bool bHas360Controller = InterfaceManager::Has360Controller();
            pItem->Set(static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT), static_cast<int32_t>(bHas360Controller));
        }
        if (pOption->pfChangeFn == &StartMenu::SetHardcore) {
            bool bEnabled = !StartMenu::MainMenuIsActive() &&
                (pPlayer && !pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY));
            pItem->Set(Tile::kTileValue_Target, static_cast<int32_t>(bEnabled));
            pItem->Set(static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT), static_cast<int32_t>(bEnabled));
            NiTListItem<Tile*>* pPos = pItem->xChildren.m_pkHead;
            NiTPointerList<Tile*>* pChildren = &pItem->xChildren;
            for (; pPos && (pChildren && pChildren->GetHead()); pPos = pChildren->GetNextPos(pPos)) {
                pChildren->Get(pPos)->Set(
                    static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT),
                    static_cast<int32_t>(bEnabled)
                );
                pChildren->Get(pPos)->Set(Tile::kTileValue_Target, static_cast<int32_t>(bEnabled));
            }
        }
        if (pOption->eType == StartMenu::UserOption::UOPT_METER) {
            uint32_t uiMaxValue = pOption->iNumSettings - 1;
            pItem->Set(Tile::kTileValue_User3, uiMaxValue);
        }
        if (pOption->iArrowSpacing) {
            uint32_t uiArrowSpacing = pOption->iArrowSpacing;
            pItem->Set(Tile::kTileValue_User4, uiArrowSpacing);
        }
        DisplayOption(pOption, pItem);
    }
    for (Option* pOption : StartMenu::pOptionsA) {
        if (pOption->IsForModes(auiModes)) {
            Tile* pItem = lbOptions.AddItem(pOption, pOption->pDisplayText, nullptr, "lb_meter_template");
            pItem->SetName(pOption->pDisplayText);
            pItem->Set(Tile::kTileValue_User3, 16);
            Tile* pLeftArrow = pItem->GetChildByID(eLeftArrow);
            Tile::Value* pVisible = pLeftArrow->GetValue(Tile::kTileValue_Visible);
            pVisible->ClearActions();
            pVisible->SetFloat(0.0f, true);
            Tile::Value* pTarget = pLeftArrow->GetValueQ(Tile::kTileValue_Target);
            pTarget->ClearActions();
            pTarget->SetFloat(0.0f, true);
            Tile* pMeter = pItem->GetChildByID(eMeter);
            pVisible = pMeter->GetValue(Tile::kTileValue_Visible);
            pVisible->ClearActions();
            pVisible->SetFloat(0.0f, true);
            pTarget = pMeter->GetValueQ(Tile::kTileValue_Target);
            pTarget->ClearActions();
            pTarget->SetFloat(0.0f, true);
            Tile* pRightArrow = pItem->GetChildByID(eRightArrow);
            pVisible = pRightArrow->GetValue(Tile::kTileValue_Visible);
            pVisible->ClearActions();
            pVisible->SetFloat(1.0f, true);
            pTarget = pRightArrow->GetValueQ(Tile::kTileValue_Target);
            pTarget->ClearActions();
            pTarget->SetFloat(0.0f, true);
        }
    }
    // The widest centre of the shown sub menus, stored to a float slot after each BSMAX.
    float fCenterX = 0.0f;
    if ((auiModes & MODE_GAMEPLAY) != 0) {
        fCenterX = BSMAX(fCenterX, pMenu->GetFloat(Tile::kTileValue_User1));
    }
    if ((auiModes & MODE_DISPLAY) != 0) {
        fCenterX = BSMAX(fCenterX, pMenu->GetFloat(Tile::kTileValue_User2));
    }
    if ((auiModes & MODE_AUDIO) != 0) {
        fCenterX = BSMAX(fCenterX, pMenu->GetFloat(Tile::kTileValue_User3));
    }
    if ((auiModes & MODE_CONTROLS) != 0) {
        fCenterX = BSMAX(fCenterX, pMenu->GetFloat(Tile::kTileValue_User4));
    }
    int32_t iCenterXTrait = Tile::AddUserTrait("_center_x", -1);
    pMenu->Set(iCenterXTrait, fCenterX, true);
}

void StartMenu::UpdateCanLoadStatus() {
    this->pContinueOption->Set(this->ENABLED_TRAIT, 1);
    this->pLoadOption->Set(this->ENABLED_TRAIT, 1);
}

// Device numbers are 0 keyboard, 1 mouse, 2 joystick (uiCurrentControlsDevice); menu sound 10 plays
// UIPopUpMessageGeneral (PC numbering).
bool StartMenu::ActionMappingIdle() {
    if (!this->IsFlagSet(FLAG_ACTION_MAPPING)) {
        return false;
    }
    if (!this->pTiles[eOptionsContainer]->GetFadeFinished(Tile::AddUserTrait("_alpha", -1))) {
        return true;
    }
    if (this->pTiles[eOptionsContainer]->GetFloat(Tile::AddUserTrait("_alpha", -1)) == 0.0) {
        this->PopulateControlsListBox();
        this->Fade(eOptionsContainer, true, Tile::AddUserTrait("_alpha", -1), 0.125f);
        this->pTiles[eDeviceButton]->Set(
            Tile::kTileValue_User0,
            static_cast<int32_t>(!InterfaceManager::Has360Controller())
        );
    }
    else if (this->WaitingForControlRemapButtonPress() && !Interface::IsConsoleVisible()) {
        bool bMapped = false;
        const uint32_t uiAction = this->lbOptions.GetHighlightedData()->iModes;
        switch (this->uiCurrentControlsDevice) {
            case 0:
                for (int32_t iKey = 0; !(iKey & 0x100); ++iKey) {
                    if (appMain->GetControls()->GetKeyboardButton(iKey, Controls::isPressed)) {
                        // Key 1 (escape) counts as mapped even when it cannot be set.
                        bMapped = appMain->GetControls()->SetControl(uiAction, 0, static_cast<uint8_t>(iKey)) ||
                            iKey == 1;
                        InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                        if (!bMapped) {
                            const char* pOk = static_cast<const char*>(sOk);
                            const char* pText = sKeyLocked.String();
                            Interface::CreateMessageMenu(
                                pText,
                                nullptr,
                                nullptr,
                                nullptr,
                                1,
                                0x12,
                                0.0f,
                                0.0f,
                                pOk,
                                static_cast<const char*>(nullptr)
                            );
                        }
                        break;
                    }
                }
                break;
            case 1:
                for (int32_t iButton = 0; !bMapped && iButton < appMain->GetControls()->GetNumMouseButtons();
                     ++iButton) {
                    if (appMain->GetControls()->GetMouseButton(iButton, Controls::isPressed)) {
                        InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                        bMapped = appMain->GetControls()->SetControl(uiAction, 1, static_cast<uint8_t>(iButton));
                    }
                }
                if (appMain->GetControls()->GetUserAction(Controls::UACTN_UP, Controls::isPressed)) {
                    bMapped = true;
                    appMain->GetControls()->SetActionMapping(uiAction, 1, 0xFF);
                    InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                }
                break;
            case 2:
                for (int32_t iButton = 0; !bMapped && iButton < appMain->GetControls()->GetButtonCount(0); ++iButton) {
                    if (appMain->GetControls()->GetJoyStickButton(0, iButton, Controls::isPressed)) {
                        bMapped = appMain->GetControls()->SetControl(uiAction, 2, static_cast<uint8_t>(iButton));
                        InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                    }
                }
                for (int32_t iHat = 0; !bMapped && iHat < appMain->GetControls()->GetHatCount(0); ++iHat) {
                    const int32_t iDirection = appMain->GetControls()->GetJoystickHat(0, iHat);
                    bMapped = iDirection != -1 &&
                        BSGetTickCount() >= uiNextHatTime &&
                        appMain->GetControls()->SetControl(uiAction, 2, static_cast<uint8_t>(iDirection));
                    InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                    if (bMapped) {
                        uiNextHatTime = GetTickCount() + 100;
                    }
                }
                if (appMain->GetControls()->GetUserAction(Controls::UACTN_UP, Controls::isPressed)) {
                    bMapped = true;
                    appMain->GetControls()->SetActionMapping(uiAction, 2, 0xFF);
                    InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                }
                break;
        }
        if (bMapped) {
            this->lbOptions.SetEnabled(true);
            this->lbOptions.DeselectAll(true);
            this->lbOptions.Traverse(&StartMenu::SetActionForDevice, 0, std::numeric_limits<int32_t>::max());
            this->pTiles[eControlsHeader]
                ->Set(Tile::kTileValue_String, pDeviceNamesA[this->uiCurrentControlsDevice]->String(), true);
            this->SetFlag(FLAG_OPTIONS_DIRTY, 1);
        }
    }
    if (InterfaceManager::Has360Controller() && this->WaitingForControlRemapButtonPress()) {
        const int32_t iStickX = appMain->GetControls()->GetControllerValue(0, 9);
        const int32_t iStickY = appMain->GetControls()->GetControllerValue(0, 10);
        if (BSAbs(iStickX) > 0x21F1 || BSAbs(iStickY) > 0x21F1) {
            InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
            const char* pOk = static_cast<const char*>(sOk);
            const char* pText = sButtonLocked.String();
            Interface::CreateMessageMenu(
                pText,
                nullptr,
                nullptr,
                nullptr,
                1,
                0x12,
                0.0f,
                0.0f,
                pOk,
                static_cast<const char*>(nullptr)
            );
        }
    }
    return true;
}

bool StartMenu::SaveLoadIdle() {
    if (!this->lbSaveLoad.IsVisible()) {
        return false;
    }
    if (this->IsFlagSet(FLAG_SAVE_WAIT_MESSAGE)) {
        uitimer0 = BSGetTickCount() + 3000;
        this->SetFlag(FLAG_SAVE_WAIT_MESSAGE, 0);
        this->SetFlag(FLAG_SAVE_WAIT_SAVING, 1);
    }
    else if (this->IsFlagSet(FLAG_SAVE_WAIT_SAVING)) {
        if (!this->IsFlagSet(FLAG_SAVE_SAVING) &&
            ((this->pSaveGame && this->pSaveGame->SaveGame()) ||
             (!this->pSaveGame && pSaveLoadManager->SaveGame(nullptr, -1, false)))) {
            this->SetFlag(FLAG_SAVE_SAVING, 1);
        }
        else if (!this->IsFlagSet(FLAG_SAVE_SAVING)) {
            const char* pOk = sOk.String();
            const char* pText = sDeviceRemoved.String();
            Interface::CreateMessageMenu(
                pText,
                nullptr,
                nullptr,
                &StartMenu::FailedSaveCallbackFunction,
                1,
                0,
                0.0f,
                0.0f,
                pOk,
                static_cast<const char*>(nullptr)
            );
            this->SetFlag(FLAG_MEM_CARD_REMOVED, 1);
            this->SetFlag(FLAG_SAVE_WAIT_SAVING, 0);
            this->SetFlag(FLAG_SAVE_SAVING, 0);
        }
        const uint32_t uiNow = BSGetTickCount();
        if (uiNow >= uitimer0 && uiNow - uitimer0 >= 1000) {
            uitimer0 = BSGetTickCount() + 1000;
        }
        if (uiNow >= uitimer0) {
            StartMenu::Close(true);
            this->SetFlag(FLAG_SAVE_WAIT_SAVING, 0);
            this->SetFlag(FLAG_SAVE_SAVING, 0);
        }
    }
    else if (
        this->IsFlagSet(FLAG_LOADING_SAVEGAMES) &&
        !this->pTiles[eSaveLoadContainer]->HasFadeControl(Tile::AddUserTrait("_alpha", -1))
    ) {
        this->PopulateSaveLoadListBox();
        this->FinishPrepSaveLoad();
        this->SetFlag(FLAG_LOADING_SAVEGAMES, 0);
    }
    else if (this->IsFlagSet(FLAG_SAVELOAD_BACKOUT) && !this->lbConfirm.IsVisible()) {
        this->ReturnToSaveLoad();
    }
    else {
        if (this->lbSaveLoad.IsEnabled()) {
            const int32_t iTrait = Tile::AddUserTrait("_current_value", -1);
            Tile* pScrollBar = this->lbSaveLoad.GetScrollBar();
            const float fValue = pScrollBar->GetFloat(iTrait);
            if (this->fSaveLoadThreshold < fValue) {
                const int32_t iStart = static_cast<int32_t>(
                    static_cast<double>(this->lbSaveLoad.NumVisible()) + this->fSaveLoadThreshold
                );
                const int32_t iEnd = static_cast<int32_t>(fValue - this->fSaveLoadThreshold) + iStart;
                this->lbSaveLoad.Traverse(&StartMenu::InitializeSaveLoadListItem, iStart, iEnd);
                this->fSaveLoadThreshold = fValue;
            }
            // Mouse button 1, just released, on a save: the same as the defaults button.
            if (appMain->GetControls()->GetMouseButton(1, Controls::isDepressed) &&
                this->lbSaveLoad.GetHighlightedData()) {
                this->DoClick(eDefaultsButton, nullptr);
            }
        }
        if (InterfaceManager::Has360Controller() && IdleTimer::Instance()->GetElapsedTime() >= 500) {
            if (this->SetSaveGameScreenshot(nullptr)) {
                this->Fade(eScreenShot, true, Tile::kTileValue_Alpha, -1.0f);
            }
            IdleTimer::Instance()->Stop();
        }
    }
    return true;
}

// The highlighted item is read through the list's virtual function, so the call is made through a pointer.
bool StartMenu::WaitingForControlRemapButtonPress() {
    if (!Interface::IsTopMenuID(Interface::eStartMenu) || !this->IsFlagSet(FLAG_ACTION_MAPPING)) {
        return false;
    }
    ListBoxBase* pOptions = &this->lbOptions;
    return this->lbOptions.IsSelected(static_cast<Tile*>(pOptions->GetHighlightedItem()));
}

// The save list's RemoveAll is virtual, so the call is made through a pointer. The credits tests are "not above zero",
// so a NaN credits value counts as no credits.
void StartMenu::_UpdateProfileSpecificData() {
    if (!this->IsFlagSet(FLAG_SHOWING_CREDITS)) {
        ListBox<BGSSaveLoadFileEntry*>* pSaveLoad = &this->lbSaveLoad;
        pSaveLoad->RemoveAll();
        if (!this->IsFlagSet(FLAG_IIS_PREFADE)) {
            this->_BackOutToMainListbox();
        }
        if (!this->IsFlagSet(FLAG_INGAME)) {
            const bool bOnline = BSSystemUtility::QInstance().QNetworkUtility().QOnline();
            if (bOnline && StartMenu::pMe && !StartMenu::pMe->IsFlagSet(FLAG_DLC_WAIT_MESSAGE)) {
                StartMenu::pMe->SetFlag(FLAG_DLC_WAIT_MESSAGE, 1);
                StartMenu::bDLCListConstructed = true;
            }
            this->SetFlag(FLAG_DLC_NEEDS_UPDATE, 1);
            this->UpdateCanLoadStatus();
        }
        this->InitializeUserOptions();
        this->SetFlag(FLAG_OPTIONS_DIRTY, 0);
    }
    if (this->IsFlagSet(FLAG_DONT_FORCE_BACK)) {
        this->SetFlag(FLAG_DONT_FORCE_BACK, 0);
        return;
    }
    if (!pPlayCredits || !(pPlayCredits->GetValue() > 0.0)) {
        StartMenu::bRestartingGame = false;
        this->Fade(eMainContainer, true, static_cast<int32_t>(0x80000000), -1.0f);
        this->lbMain.SetEnabled(true);
    }
    if (this->IsFlagSet(FLAG_SHOWING_CREDITS) && (!pPlayCredits || !(pPlayCredits->GetValue() > 0.0))) {
        this->lbMain.DeselectAll(true);
        this->SetFlag(FLAG_SHOWING_CREDITS, 0);
    }
}

void StartMenu::PopulateControlsListBox() {
    Controls::USERACTIONS eAction;
    ListBox<StartMenu::Option*>* pOptions = &lbOptions;
    pOptions->RemoveAll();
    lbOptions.SetEnabled(true);
    for (eAction = Controls::UACTN_FORWARD; eAction < Controls::UACTN_UP; ++eAction) {
        if ((uiCurrentControlsDevice == 3 &&
             (eAction == Controls::UACTN_MENUMODE ||
              eAction == Controls::UACTN_ACTIVATE ||
              eAction == Controls::UACTN_USE ||
              eAction == Controls::UACTN_READY_RELOAD_WEAPON ||
              eAction == Controls::UACTN_CROUCH ||
              eAction == Controls::UACTN_JUMP ||
              eAction == Controls::UACTN_TOGGLEPOV ||
              eAction == Controls::UACTN_RESTMENU ||
              eAction == Controls::UACTN_BLOCK ||
              eAction == Controls::UACTN_VATSMENU ||
              eAction == Controls::UACTN_GRAB)) ||
            (uiCurrentControlsDevice != 3 && Controls::Remappable(eAction))) {
            StartMenu::Option* pOption = new StartMenu::Option(
                psUserActions[eAction]->String(),
                &StartMenu::ChooseUserAction,
                eAction
            );
            Tile* pLBItem = lbOptions.AddItem(pOption, pOption->pDisplayText, nullptr, "lb_item_template");
            pLBItem->SetName(psUserActions[eAction]->String());
        }
    }
    lbOptions.Traverse(&StartMenu::SetActionForDevice, 0, std::numeric_limits<int32_t>::max());
    if (!InterfaceManager::Has360Controller()) {
        const char* pDeviceName = pDeviceNamesA[uiCurrentControlsDevice]->String();
        pTiles[eControlsHeader]->Set(Tile::kTileValue_String, pDeviceName, true);
        pTiles[eControlsHeader]->Set(Tile::kTileValue_Visible, 1);
    }
}

void StartMenu::PopulateSaveLoadListBox() {
    if (LoadingMenu::pMe) {
        LoadingMenu::ResumeBackgroundThread();
    }
    pSaveLoadManager->SetupSavesList();
    for (BSSimpleList<BGSSaveLoadFileEntry*>* pNode = pSaveLoadManager->pSaveGameList; pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        lbSaveLoad.AddItem(pNode->m_item, nullptr, nullptr, nullptr);
    }
}

void StartMenu::PopulateConfirmationListBox(uint32_t auiModes, uint8_t aucFlags) {
    uint32_t uiWidth = (aucFlags & CONFIRMATION_WIDE) != 0 ? 168 : 80;
    double dX = static_cast<double>(static_cast<int32_t>(pTiles[0]->GetRealX()) - uiWidth);
    int32_t iX = static_cast<int32_t>(dX - InterfaceManager::GetScreenSafeX());
    Tile* pItem = static_cast<Tile*>(lbMain.GetHighlightedItem());
    int32_t iY = static_cast<int32_t>(pItem->GetRealY()) - 60;
    PopulateConfirmationListBox_0(auiModes, iX, iY, uiWidth, (aucFlags & CONFIRMATION_SHOW_WARNING) != 0);
}

void StartMenu::PopulateConfirmationListBox(uint32_t auiModes, int32_t aiRealX, int32_t aiRealY, uint8_t aucFlags) {
    const uint32_t uiTileValue = (aucFlags & CONFIRMATION_WIDE) ? 0xA8 : 0x55;
    this->PopulateConfirmationListBox_0(
        auiModes,
        aiRealX,
        aiRealY,
        uiTileValue,
        (aucFlags & CONFIRMATION_SHOW_WARNING) != 0
    );
}

void StartMenu::HideOptionsContainer() {
    this->pTiles[eVersionText]->Set(Tile::kTileValue_Visible, 0);
    this->lbOptions.SetEnabled(false);
    this->Fade(eOptionsContainer, false, static_cast<int32_t>(0x80000000), -1.0f);
    if (this->IsFlagSet(FLAG_ACTION_MAPPING)) {
        this->lbOptions.Traverse(&StartMenu::DeleteOption, 0, std::numeric_limits<int32_t>::max());
        this->SetFlag(FLAG_ACTION_MAPPING, 0);
        this->pTiles[eDeviceButton]->Set(Tile::kTileValue_User0, 0);
        this->pTiles[eControlsHeader]->Set(Tile::kTileValue_Visible, 0);
    }
}

uint32_t StartMenu::PopulateConfirmationListBox_0(
    uint32_t auiModes,
    int32_t aiX,
    int32_t aiY,
    uint32_t aiTileValue,
    bool abVisible
) {
    uint32_t uiSize;
    ListBox<StartMenu::Option*>* pConfirm = &lbConfirm;
    pConfirm->RemoveAll();
    lbConfirm.SetEnabled(true);
    Fade(StartMenu::eConfirmContainer, true, static_cast<int32_t>(0x80000000), -1.0f);
    float fX = static_cast<double>(aiX) - pTiles[eConfirmContainer]->pParent->GetRealX();
    pTiles[eConfirmContainer]->Set(Tile::kTileValue_X, fX, true);
    float fY = static_cast<double>(aiY) - pTiles[eConfirmContainer]->pParent->GetRealY();
    pTiles[eConfirmContainer]->Set(Tile::kTileValue_Y, fY, true);
    pTiles[eConfirmContainer]->Set(Tile::kTileValue_Width, aiTileValue);
    // Passes the raw byte of abVisible; read as bool, it would be reduced to 0 or 1.
    pTiles[eConfirmWarning]->Set(Tile::kTileValue_Visible, static_cast<int32_t>(static_cast<uint8_t>(abVisible)));
    for (int32_t i = 0;; ++i) {
        uiSize = StartMenu::pOptionsA.iSize;
        if (i >= uiSize) {
            break;
        }
        if (StartMenu::pOptionsA.GetAt(i)->IsForModes(auiModes)) {
            Tile* pLBItem = lbConfirm.AddItem(StartMenu::pOptionsA.GetAt(i), nullptr, nullptr, nullptr);
            pLBItem->Set(Tile::kTileValue_String, StartMenu::pOptionsA.GetAt(i)->pDisplayText, true);
            pLBItem->SetName(StartMenu::pOptionsA.GetAt(i)->pDisplayText);
        }
    }
    return uiSize;
}

void StartMenu::FinishPrepSaveLoad() {
    if (IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
        uint32_t uiNumItems = lbSaveLoad.NumItems();
        if (uiNumItems) {
            uint32_t uiEnabledTrait = this->ENABLED_TRAIT;
            lbSaveLoad.GetFirstItem()->Set(uiEnabledTrait, 1);
        }
        for (int32_t i = lbSaveLoad.NumItems() - 1; i >= 0; --i) {
            // GetTileAt, Highlight and ScrollToHighlight are called through the list's vtable; a call on the member
            // itself would be devirtualised.
            Tile* tTile = lbSaveLoad.GetTileAt(static_cast<uint32_t>(i), false);
            BGSSaveLoadFileEntry* tSave = lbSaveLoad.GetDataAt(i, false);
            if (tTile && tSave) {
                if (tSave->IsAutosave()) {
                    lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                    --uiNumItems;
                }
                else if (tSave->IsSystemSave()) {
                    lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                    --uiNumItems;
                }
            }
        }
        if (uiNumItems >= 1000) {
            lbSaveLoad.GetFirstItem()->AddNeedsUpdate(8);
            Tile* pTile = lbSaveLoad.GetTileAt(1u, false);
            lbSaveLoad.Highlight(pTile);
            lbSaveLoad.ScrollToHighlight();
        }
        if (uiNumItems) {
            char acText[68];
            const char* pSavesRemaining = sSavesRemaining.String();
            BSsprintf(acText, 64, "%s %u/%i %s", sNewSave.String(), uiNumItems - 1, 1000, pSavesRemaining);
            lbSaveLoad.GetFirstItem()->Set(4036, acText, true);
        }
    }
    else if (lbSaveLoad.GetFirstItem()) {
        const char* pNoSaves = sNoSaves.String();
        lbSaveLoad.GetFirstItem()->Set(4036, pNoSaves, true);
        uint32_t uiEnabledTrait = this->ENABLED_TRAIT;
        lbSaveLoad.GetFirstItem()->Set(uiEnabledTrait, 0);
    }
    lbSaveLoad.SetEnabled(true);
    lbSaveLoad.DeselectAll(true);
    lbSaveLoad.Filter(&StartMenu::SaveLoadFilterFn);
    if (InterfaceManager::Has360Controller()) {
        Tile* pFirstItem = lbSaveLoad.GetFirstItem();
        InterfaceManager::Instance()->SetCurrentFocusTarget(pFirstItem, 4035, true);
        int32_t iValueTrait = Tile::AddUserTrait("_current_value", -1);
        lbSaveLoad.pScrollBar->Set(iValueTrait, 0.0f, false);
        int32_t iSetInCodeTrait = Tile::AddUserTrait("_SetInCode ", -1);
        lbSaveLoad.pScrollBar->Poke(iSetInCodeTrait, 1.0f);
    }
    fSaveLoadThreshold = 0.0f;
    lbSaveLoad.Traverse(&StartMenu::InitializeSaveLoadListItem, 0, static_cast<int32_t>(lbSaveLoad.NumVisible()));
    pTiles[6]->Set(Tile::kTileValue_User0, 1);
    if (IsFlagSet(StartMenu::FLAG_LOADING_SAVEGAMES)) {
        pTiles[5]->Set(4015, 1);
    }
    else {
        Fade(StartMenu::eSaveLoadContainer, true, Tile::kTileValue_Invalid, -1.0f);
        Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
        const char* pDelete = sDelete.String();
        pTiles[4]->Set(4036, pDelete, true);
    }
}

void StartMenu::DisplayOption(StartMenu::UserOption* apOption, Tile* apTile) {
    if (apOption && !apTile) {
        StartMenu::UserOption* pData = apOption;
        apTile = lbOptions.GetTileFor(pData);
    }

    if (apOption && apTile) {
        if (apOption->eType == StartMenu::UserOption::UOPT_METER) {
            apTile->Set(Tile::kTileValue_User0, apOption->iVal);
        }
        else if (apOption->pValueStrings && apOption->iVal >= 0 && apOption->iVal <= apOption->iNumSettings - 1) {
            apTile->Set(Tile::kTileValue_User0, apOption->pValueStrings[apOption->iVal], true);
        }
        if (apOption->eType != StartMenu::UserOption::UOPT_TOGGLE) {
            if (!apOption->iVal) {
                apTile->Set(Tile::kTileValue_User2, 2);
                return;
            }
            if (apOption->iVal == apOption->iNumSettings - 1) {
                apTile->Set(Tile::kTileValue_User2, 0);
                return;
            }
        }
        apTile->Set(Tile::kTileValue_User2, 1);
    }
}

// The texture's sizes are read through virtual calls, so the calls are made through a base pointer.
bool StartMenu::SetSaveGameScreenshot(BGSSaveLoadFileEntry* apSaveGame) {
    if (!apSaveGame) {
        apSaveGame = this->lbSaveLoad.GetHighlightedData();
    }
    NiTexture* pScreenshot = apSaveGame ? apSaveGame->GetScreenshot() : nullptr;
    if (!pScreenshot) {
        return false;
    }
    TileImage* pImage = static_cast<TileImage*>(this->pTiles[eScreenShot]);
    pImage->SetSourceTexture(pScreenshot);
    NiTexture* pTexture = pScreenshot;
    pImage->Set(Tile::kTileValue_Height, pTexture->GetHeight());
    pImage->Set(Tile::kTileValue_Width, pTexture->GetWidth());
    if (!static_cast<TileShaderProperty*>(pImage->GetShaderProp())->GetAlphaTexture()) {
        NiSourceTexture* pAlphaMap = this->spAlphaMap;
        static_cast<TileShaderProperty*>(pImage->GetShaderProp())->SetAlphaMap(pAlphaMap);
    }
    return true;
}

// Stores the defaulted trait and time back into its arguments.
void StartMenu::Fade(TileIndex aeIndex, bool abFadeIn, int32_t aiTrait, float afFadeTime) {
    if (aiTrait == static_cast<int32_t>(0x80000000)) {
        aiTrait = Tile::AddUserTrait("_alpha", -1);
    }
    if (afFadeTime < 0.0) {
        afFadeTime = 0.25f;
    }
    if (!this->pTiles[aeIndex]) {
        return;
    }
    if (abFadeIn && this->pTiles[aeIndex]->GetFloat(aiTrait) < 255.0) {
        this->pTiles[aeIndex]->AddFadeControl(aiTrait, 0.0f, 255.0f, afFadeTime, Tile::TFCT_STANDARD);
    }
    else if (!abFadeIn && this->pTiles[aeIndex]->GetFloat(aiTrait) > 0.0) {
        this->pTiles[aeIndex]->AddFadeControl(aiTrait, 255.0f, 0.0f, afFadeTime, Tile::TFCT_STANDARD);
    }
}

void StartMenu::ReturnToSaveLoad() {
    lbSaveLoad.SetEnabled(true);
    Tile* pItem = static_cast<Tile*>(lbSaveLoad.GetHighlightedItem());
    lbSaveLoad.Deselect(pItem);
    pTiles[6]->Set(Tile::kTileValue_User0, 1);
    if (!InterfaceManager::Has360Controller()) {
        SetFlag(StartMenu::FLAG_SAVELOAD_BACKOUT, 0);
    }
}

void StartMenu::InitializeUserOptions() {
    this->SetFlag(FLAG_SAVE_OPTIONS, 0);
    for (uint32_t i = 0; i < StartMenu::pUserOptionsA.iSize; ++i) {
        if (StartMenu::pUserOptionsA.GetAt(i)->pfChangeFn) {
            StartMenu::pUserOptionsA.GetAt(i)->pfChangeFn(StartMenu::pUserOptionsA.GetAt(i));
        }
        if (StartMenu::pUserOptionsA.GetAt(i)->pfChangeFn == &StartMenu::SetBrightness) {
            this->pBrightnessOption = StartMenu::pUserOptionsA.GetAt(i);
        }
    }
    this->SetFlag(FLAG_SAVE_OPTIONS, 1);
}

void StartMenu::UpdateHighlightedSaveGameInfo() {
    if (IsFlagSet(StartMenu::FLAG_SAVELOAD_BACKOUT)) {
        SetFlag(StartMenu::FLAG_SAVELOAD_BACKOUT, 0);
    }
    else {
        bool bScreenshotVis = false;
        bool bNameVis = false;
        uint8_t ucLevelVis = 0;
        uint8_t ucPlayTimeVis = 0;
        uint8_t ucVersionVis = 0;
        BGSSaveLoadFileEntry* pHighlightedSaveGame = lbSaveLoad.GetHighlightedData();
        if (pHighlightedSaveGame) {
            if (InterfaceManager::Has360Controller() && bFadeInScreenShot) {
                StartMenu::IdleTimer::Instance()->Start();
                bScreenshotVis = true;
                pTiles[11]->Set(4009, 0.0f, true);
            }
            else {
                bScreenshotVis = SetSaveGameScreenshot(pHighlightedSaveGame);
            }
            if (pHighlightedSaveGame->IsCorrupt()) {
                const char* pMessage = sSaveGameCorruptMenuMessage.String();
                pTiles[12]->Set(4036, pMessage, true);
                bNameVis = true;
                ucLevelVis = 0;
                ucPlayTimeVis = 0;
                ucVersionVis = 0;
            }
            else {
                uint8_t aucSeparator[4];
                const char* pPlayerName = pHighlightedSaveGame->GetPlayerName();
                aucSeparator[0] = 0;
                aucSeparator[1] = 0;
                if (!BSstrcmp(sLanguage.String(), "FRENCH")) {
                    aucSeparator[0] = 32;
                }
                pTiles[12]->Set(4036, pPlayerName, true);
                bNameVis = pPlayerName != nullptr;
                BSStringT<char> kText;
                const char* pPlayerTitle = pHighlightedSaveGame->GetPlayerTitle();
                uint32_t uiPlayerLevel = pHighlightedSaveGame->GetPlayerLevel();
                kText.SPrintF("%s %d%s: %s", sLevelAbbrev.String(), uiPlayerLevel, aucSeparator, pPlayerTitle);
                pTiles[18]->Set(4036, kText.pString, true);
                ucLevelVis = 1;
                const char* pPlayTime = pHighlightedSaveGame->GetPlayTime();
                kText.SPrintF("%s%s: %s", sPlayTime.String(), aucSeparator, pPlayTime);
                pTiles[19]->Set(4036, kText.pString, true);
                ucPlayTimeVis = 1;
                kText.SPrintF("%s%s: %s", sVersionNumber.String(), aucSeparator, &pHighlightedSaveGame->pcLanguage);
                pTiles[22]->Set(4036, kText.pString, true);
                ucVersionVis = 1;
            }
        }
        else if (InterfaceManager::Has360Controller()) {
            StartMenu::IdleTimer::Instance()->Stop();
        }
        int32_t iHasSave = pHighlightedSaveGame != nullptr;
        int32_t iTargetTrait = Tile::AddUserTrait("_target", -1);
        pTiles[4]->Set(iTargetTrait, iHasSave);
        pTiles[11]->Set(4003, static_cast<int32_t>(bScreenshotVis));
        pTiles[12]->Set(4003, static_cast<int32_t>(bNameVis));
        pTiles[18]->Set(4003, static_cast<int32_t>(ucLevelVis));
        pTiles[19]->Set(4003, static_cast<int32_t>(ucPlayTimeVis));
        pTiles[22]->Set(4003, static_cast<int32_t>(ucVersionVis));
    }
}

// Once past the visibility and flag tests, the loop only retests whether the main list is enabled.
void StartMenu::_BackOutToMainListbox() {
    if (!this->lbMain.IsVisible() || this->IsFlagSet(FLAG_DONT_FORCE_BACK)) {
        return;
    }
    while (!this->lbMain.IsEnabled()) {
        StartMenu::ChooseBack();
        if (this->IsFlagSet(FLAG_SAVELOAD_BACKOUT)) {
            this->ReturnToSaveLoad();
            this->SetFlag(FLAG_SAVELOAD_BACKOUT, 0);
        }
    }
}

void StartMenu::ConfirmSaveLoad() {
    uint32_t uiMode;
    const char* pText;
    uint8_t ucFlags = 0;
    if (IsFlagSet(StartMenu::FLAG_DELETE_MODE)) {
        uiMode = 2048;
        pText = sConfirmDelete.String();
    }
    else if (IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
        uiMode = 1024;
        pText = sConfirmSave.String();
    }
    else {
        uiMode = 512;
        pText = sConfirmLoad.String();
        if (IsFlagSet(StartMenu::FLAG_INGAME)) {
            ucFlags = CONFIRMATION_SHOW_WARNING;
        }
    }
    // GetHighlightedItem is called through the list's vtable each time.
    if (lbSaveLoad.GetHighlightedItem()) {
        Tile* pItem = static_cast<Tile*>(lbSaveLoad.GetHighlightedItem());
        lbSaveLoad.Select(pItem);
        lbSaveLoad.SetEnabled(false);
        pTiles[6]->Set(Tile::kTileValue_User0, 0);
        pTiles[8]->Set(4036, pText, true);
        pTiles[8]->UpdateTile(false);
        pItem = static_cast<Tile*>(lbSaveLoad.GetHighlightedItem());
        double dRealY = pItem->GetRealY();
        pItem = static_cast<Tile*>(lbSaveLoad.GetHighlightedItem());
        int32_t iY = static_cast<int32_t>(
            pItem->GetFloat(4016) * static_cast<double>(((ucFlags & CONFIRMATION_SHOW_WARNING) != 0) + 1) + dRealY
        );
        double dRealX = pTiles[6]->GetRealX();
        double dX = dRealX - pTiles[8]->GetFloat(4001);
        dX = pTiles[8]->GetFloat(4017) + dX;
        dX = lbSaveLoad.pScrollBar->GetFloat(4017) + dX;
        int32_t iTextOffsetTrait = Tile::AddUserTrait("_text_offset", -1);
        dX = pTiles[6]->GetFloat(iTextOffsetTrait) + dX;
        PopulateConfirmationListBox(uiMode, static_cast<int32_t>(dX), iY, ucFlags);
    }
}

void StartMenu::ShowDLCList() {
    if (!lbDownloads.NumItems()) {
        lbDownloads.AddItem(-1, sOldDownloadsAvailable.String(), nullptr, nullptr);
    }
    lbDownloads.SetEnabled(true);
    Fade(StartMenu::eDLCContainer, true, Tile::kTileValue_Invalid, -1.0f);
    Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
}

void StartMenu::Music() {
    if (!IsFlagSet(StartMenu::FLAG_INGAME)) {
        if (FalloutAudioMedia::MediaIsPlaying(MEDIA_MAINMENU, true)) {
            char acMusic[68];
            ourMemset(acMusic, 0, 64u);
            BSsprintf(acMusic, 64u, "Data\\Music\\Special\\MainTitle.mp3");
            FalloutAudio::SetMainMenuMusic(true);
            FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, acMusic, 1000u, true, true, 0.0f, 0u);
            FalloutAudioMedia::MediaPlay();
        }
        SetFlag(StartMenu::FLAG_FADE_IN, 1);
        SetFlag(StartMenu::FLAG_SHOWING_CREDITS, 0);
        StartMenu::bRestartingGame = true;
        pPlayCredits->SetValue(0.0f);
        LoadingMenu::SuspendBackgroundThread();
    }
}

int32_t StartMenu::GetClassStatic() {
    return 1013;
}

// The AutoMemContext is constructed on a stack buffer (with this file and line 0x151) and the constructor's result is
// ignored. A menu fading in (8) or out (2) that is not stacked as 6008 stops the build.
bool StartMenu::Create(bool abInGame, bool abForceReload) {
    using enum Tile::eTileValue;

    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Menus\\StartMenu.cpp",
        337
    );
    g_startMenuLastControllerInputTime = BSGetTickCount();
    if (!abInGame && !LoadingMenu::bInitialLoad) {
        LoadingMenu::SetupForMainMenu(true);
    }
    if (pSaveLoadManager) {
        pSaveLoadManager->SetSystemUtilitySaveCompleteCallback(&StartMenu::SysUtilSaveCompleteCallback);
        pSaveLoadManager->SetSystemUtilityLoadCompleteCallback(&StartMenu::SysUtilSaveCompleteCallback);
    }
    if (StartMenu::pOptionsA.IsEmpty()) {
        StartMenu::CreateMenuElements();
    }
    Tile* pRoot = InterfaceManager::Instance()->GetMenusRoot();
    NiTListItem<Tile*>* pPos = pRoot->xChildren.GetHeadPos();
    while (pPos) {
        Tile* pChild = pRoot->xChildren.GetNext(pPos);
        Menu* pChildMenu = pChild ? pChild->GetMenu() : nullptr;
        if (!pChildMenu || !pChildMenu->GetMenuTile()) {
            continue;
        }
        const int32_t iFadeState = pChildMenu->GetFadeState();
        if ((iFadeState == 8 || iFadeState == 2) && pChild->GetFloat(kTileValue_StackingType) != 6008.0) {
            pContext->~AutoMemContext();
            return false;
        }
    }
    TileMenu* pOldMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eStartMenu));
    if (pOldMenu) {
        delete pOldMenu;
    }
    const float fMaxDepth = Tile::GetMaximumDepth();
    Tile::Lock();
    Tile* pFile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\Options\\start_menu.xml");
    Menu* pMenu = pFile->GetMenu();
    if (pMenu && pMenu->GetClass() == StartMenu::GetClassStatic()) {
        pMenu->SetMenuTile(static_cast<TileMenu*>(pFile), false);
        StartMenu::pMe = static_cast<StartMenu*>(pMenu);
        if (pFile->GetFloat(kTileValue_StackingType) == 6006.0 || pFile->GetFloat(kTileValue_StackingType) == 102.0) {
            pFile->Set(kTileValue_Depth, fMaxDepth, true);
        }
        StartMenu::pMe->_Create(abInGame, abForceReload);
        Tile::Unlock();
        pContext->~AutoMemContext();
        return true;
    }
    if (pMenu && pMenu->GetMenuTile()) {
        delete pMenu;
    }
    Tile::Unlock();
    pContext->~AutoMemContext();
    return false;
}

bool StartMenu::ShouldHideMenu(int32_t aiMenuID) {
    uint8_t ucHide;
    switch (aiMenuID) {
        case 1008:
        case 1009:
        case 1012:
        case 1014:
        case 1016:
        case 1027:
        case 1047:
        case 1048:
        case 1051:
        case 1053:
        case 1056:
        case 1059:
        case 1075:
        case 1076:
        case 1077:
        case 1080:
        case 1081:
        case 1082:
        case 1083:
        case 1084: {
            ucHide = 1;
            break;
        }
        default: {
            ucHide = 0;
            break;
        }
    }
    return ucHide;
}

void StartMenu::SetLoadMissingContent(uint8_t abOn) {
    if (StartMenu::pMe) {
        StartMenu::pMe->SetFlag(FLAG_LOAD_MISSING_CONTENT, abOn);
    }
}

bool StartMenu::PauseMenuIsActive() {
    return StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_INGAME);
}

bool StartMenu::MainMenuIsActive() {
    return StartMenu::pMe && !StartMenu::pMe->IsFlagSet(FLAG_INGAME);
}

bool StartMenu::IsStartMenuSaving() {
    return StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_WAIT_SAVING) &&
        StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_MODE);
}

bool StartMenu::CreditsMenuIsActive() {
    return StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SHOWING_CREDITS);
}

bool StartMenu::IsInitialLoading() {
    return LoadingMenu::bInitialLoad;
}

bool StartMenu::SaveLoadFilterFn(BGSSaveLoadFileEntry* apSaveGame) {
    if (!StartMenu::pMe) {
        return true;
    }
    if (!StartMenu::pMe->IsFlagSet(FLAG_SAVE_MODE) && !apSaveGame && StartMenu::pMe->lbSaveLoad.NumItems() > 1) {
        return true;
    }
    return false;
}

void StartMenu::Close(bool abDeleteOnFade) {
    NiTListItem<Tile*>* pIter;
    if (StartMenu::PauseMenuIsActive() && Interface::GetCurrentRenderedMenu()) {
        FOPipboyManager* pRenderedMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu());
        if (pRenderedMenu == Interface::GetPipboy()) {
            Interface::GetPipboy()->ResetColors();
        }
    }

    if (StartMenu::pMe && !StartMenu::pMe->WaitingForControlRemapButtonPress()) {
        if (Interface::InDialog()) {
            Interface::SetDialogPause(false);
        }
        if (!StartMenu::pMe->IsFlagSet(StartMenu::FLAG_FORCE_RELOAD)) {
            if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
                StartMenu::pMe->lbOptions.Traverse(&StartMenu::DeleteOption, 0, std::numeric_limits<int32_t>::max());
            }
            if (StartMenu::PauseMenuIsActive()) {
                MessageMenu::SetPauseMenuMode(false);
            }
            for (pIter = StartMenu::xHiddenTiles.m_pkHead; pIter;
                 StartMenu::xHiddenTiles.GetNext(pIter)->Set(4003, 1)) {
                ;
            }
            StartMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, static_cast<int32_t>(abDeleteOnFade));
            StartMenu::pMe->StartFadeOut();
            StartMenu::pMe = nullptr;
        }
        if (Interface::IsInRenderedMenu()) {
            appMain->RenderMenuBackground();
        }
    }
}

void StartMenu::ChooseUserAction() {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
        const char* pPressControl = sPressControl.String();
        StartMenu::pMe->pTiles[16]->Set(4036, pPressControl, true);
        StartMenu::pMe->pTiles[16]->Set(4003, 1);
        if (InterfaceManager::Has360Controller() && StartMenu::pMe->uiCurrentControlsDevice == 3) {
            StartMenu::pMe->pTiles[5]->Set(4015, 0);
        }
    }
}

void StartMenu::LoadDownloadedContent() {
    BSSystemUtility::QInstance().QDLCUtility().LoadDLC(0, 0xFFFFFFFFu);
    BSSystemUtility::QInstance().QDLCUtility().MountDLC(0, nullptr);
    if (pDataHandler->AddDownloadedContentToFileList()) {
        bool bAllowChanges = pSaveLoadGame->SetGlobalAllowChanges(false);
        pDataHandler->CompileFiles(true);
        pDataHandler->FixAllScriptIDs();
        pSaveLoadGame->SetGlobalAllowChanges(bAllowChanges);
        ReferenceProcessLists.InitPackageLocations();
        pDataHandler->UpdateAutoCalcNPCs();
    }
    pIdleManager->InitRootIdleArrays();
    MessageMenu::ClearMessageQueue();
    Interface::CloseMessageMenu();
    BSDLCSystemUtility* pDLC = &BSSystemUtility::QInstance().QDLCUtility();
    if (!pDLC->m_corruptDLC.IsEmpty()) {
        StartMenu::pMe->SetFlag(StartMenu::FLAG_DLC_CORRUPT_MESSAGE, 1);
    }
    ListBox<int32_t>* pDownloads = &StartMenu::pMe->lbDownloads;
    pDownloads->RemoveAll();
    const char* pOldDownloads = sOldDownloadsAvailable.String();
    StartMenu::pMe->lbDownloads.AddItem(-1, pOldDownloads, nullptr, nullptr);
    for (BSContentPackage* pPackage : pDLC->m_validDLC) {
        if (pPackage) {
            char acName[264];
            const char16_t* pDisplayName = pPackage->DisplayName();
            rt::wcstombs(acName, pDisplayName, 260u);
            StartMenu::pMe->lbDownloads.AddItem(0, acName, nullptr, nullptr);
        }
    }
}

void StartMenu::SaveSettings() {
    char (*pRendererFile)[260] = &SettingT<RendererSettingCollection>::GetCollection()->pSettingFile;
    RendererSettingCollection* pRendererSettings = SettingT<RendererSettingCollection>::GetCollection();
    pRendererSettings->SaveAll(reinterpret_cast<char*>(pRendererFile));
    char (*pPrefFile)[260] = &SettingT<INIPrefSettingCollection>::GetCollection()->pSettingFile;
    INIPrefSettingCollection* pPrefSettings = SettingT<INIPrefSettingCollection>::GetCollection();
    if (pPrefSettings->SaveAll(reinterpret_cast<char*>(pPrefFile))) {
        appMain->GetControls()->SaveControls();
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_RESTART_REQUIRED)) {
            const char* pOk = static_cast<const char*>(sOk);
            Interface::CreateMessageMenu(
                static_cast<const char*>(sMustRestart),
                static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr),
                static_cast<void (*)()>(nullptr),
                0,
                18,
                0.0f,
                0.0f,
                pOk,
                nullptr
            );
            StartMenu::pMe->SetFlag(StartMenu::FLAG_RESTART_REQUIRED, 0);
        }
    }
    else {
        const char* pOk = static_cast<const char*>(sOk);
        Interface::CreateMessageMenu(
            static_cast<const char*>(sMenuDisplayFailSaveSettings),
            static_cast<const char*>(nullptr),
            static_cast<const char*>(nullptr),
            static_cast<void (*)()>(nullptr),
            0,
            18,
            0.0f,
            0.0f,
            pOk,
            nullptr
        );
    }
}

// Builds the main, pause, confirmation and settings options.
void StartMenu::CreateMenuElements() {
    if (!g_strBlue.pString) {
        g_strBlue.Set(sBlue.String(), 0);
        g_strBlue.MakeUpper();
        g_strGreen.Set(sGreen.String(), 0);
        g_strGreen.MakeUpper();
    }
    StartMenu::DestroyMenuElements();
    Option* pOption;
    UserOption* pUserOption;
    pOption = new Option(static_cast<const char*>(sContinue), &StartMenu::ChooseContinue, MODE_MAIN | MODE_PAUSE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sNew), &StartMenu::ChooseNew, MODE_MAIN);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sSave), &StartMenu::ChooseSave, MODE_PAUSE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sLoad), &StartMenu::ChooseLoad, MODE_MAIN | MODE_PAUSE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sSettings), &StartMenu::ChooseSettings, MODE_MAIN | MODE_PAUSE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sHelp), &StartMenu::ChooseHelp, MODE_PAUSE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sCrew), &MenuButton_Credits, MODE_MAIN);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sDownloads), &StartMenu::ChooseDownloads, MODE_MAIN);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sQuit), &StartMenu::ChooseQuit, MODE_MAIN | MODE_PAUSE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sGameplay), &StartMenu::ChooseGameplay, MODE_SETTINGS);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sDisplay), &MenuButton_Display, MODE_SETTINGS);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sAudio), &MenuButton_Audio, MODE_SETTINGS);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sControls), &StartMenu::ChooseControls, MODE_SETTINGS);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sYes), &StartMenu::ConfirmNewGame, MODE_CFM_NEW);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sYes), &StartMenu::ConfirmContinue, MODE_CFM_CONT);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sYes), &StartMenu::ConfirmLoadGame, MODE_CFM_LOAD);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sYes), &StartMenu::ConfirmSaveGame, MODE_CFM_SAVE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sYes), &StartMenu::ConfirmDeleteGm, MODE_CFM_DELETE);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(
        static_cast<const char*>(sNo),
        &StartMenu::ChooseBack,
        MODE_CFM_NEW | MODE_CFM_CONT | MODE_CFM_LOAD | MODE_CFM_SAVE | MODE_CFM_DELETE
    );
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sMainMenu), &StartMenu::ChooseMainMenu, MODE_CFM_QUITP);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(
        static_cast<const char*>(sExitGameAffirm),
        &StartMenu::ChooseExitGame,
        MODE_CFM_QUITM | MODE_CFM_QUITP
    );
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sCancel), &StartMenu::ChooseBack, MODE_CFM_QUITM | MODE_CFM_QUITP);
    StartMenu::pOptionsA.Add(pOption);
    pOption = new Option(static_cast<const char*>(sActionMapping), &StartMenu::ChooseActionMap, MODE_CONTROLS);
    StartMenu::pOptionsA.Add(pOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sKillCamMode),
        &StartMenu::SetKillCam,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        3,
        0x55,
        sKillCamModeNone.String(),
        sKillCamModePlayer.String(),
        sKillCamModeCinematic.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sHardcore),
        &StartMenu::SetHardcore,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sDifficulty),
        &StartMenu::SetDifficulty,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        5,
        0x55,
        sLockLevelNameVeryEasy.String(),
        sLockLevelNameEasy.String(),
        sNormal.String(),
        sLockLevelNameHard.String(),
        sLockLevelNameVeryHard.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sSaveOnRest),
        &StartMenu::SetSaveOnRest,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sSaveOnWait),
        &StartMenu::SetSaveOnWait,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sSaveOnTravel),
        &StartMenu::SetSaveOnTravel,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sTrueIronSights),
        &StartMenu::SetTrueIronSights,
        MODE_GAMEPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sBrightness),
        &StartMenu::SetBrightness,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sHUDOpacity),
        &StartMenu::SetHUDOpacity,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sHUDColor),
        &StartMenu::SetHUDColor,
        MODE_DISPLAY,
        UserOption::UOPT_TOGGLE,
        4,
        0x37,
        g_strGreen.pString,
        g_strBlue.pString,
        sAmber.String(),
        sWhite.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sPipboyColor),
        &StartMenu::SetPipboyColor,
        MODE_DISPLAY,
        UserOption::UOPT_TOGGLE,
        4,
        0x37,
        g_strGreen.pString,
        g_strBlue.pString,
        sAmber.String(),
        sWhite.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sTextureSize),
        &SetTextureSize,
        MODE_DISPLAY,
        UserOption::UOPT_TOGGLE,
        3,
        0x37,
        sLarge.String(),
        sMedium.String(),
        sSmall.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sActorFade),
        &SetActorFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sItemFade),
        &StartMenu::SetItemFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sObjectFade),
        &StartMenu::SetObjectFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sGrassFade),
        &StartMenu::SetGrassFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sShadowFade),
        &StartMenu::SetShadowFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sLightFade),
        &StartMenu::SetLightFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sSpecularityFade),
        &StartMenu::SetSpecularityFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sObjectLODFade),
        &StartMenu::SetObjectLODFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sTreeLODFade),
        &StartMenu::SetTreeLODFade,
        MODE_DISPLAY,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sCrosshair),
        &StartMenu::SetCrosshair,
        MODE_DISPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sDialogSubtitles),
        &StartMenu::SetDialogSubtitles,
        MODE_DISPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sGeneralSubtitles),
        &StartMenu::SetGeneralSubtitles,
        MODE_DISPLAY,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sMasterVolume),
        &StartMenu::SetMasterVol,
        MODE_AUDIO,
        UserOption::UOPT_METER,
        26,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sMusicVolume),
        &StartMenu::SetMusicVol,
        MODE_AUDIO,
        UserOption::UOPT_METER,
        26,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sFootstepsVolume),
        &StartMenu::SetFootstepVol,
        MODE_AUDIO,
        UserOption::UOPT_METER,
        26,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sVoiceVolume),
        &StartMenu::SetVoiceVol,
        MODE_AUDIO,
        UserOption::UOPT_METER,
        26,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sEffectsVolume),
        &StartMenu::SetFXVol,
        MODE_AUDIO,
        UserOption::UOPT_METER,
        26,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sRadioVolume),
        &StartMenu::SetRadioVol,
        MODE_AUDIO,
        UserOption::UOPT_METER,
        26,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sCameraPitch),
        &StartMenu::SetCameraPitch,
        MODE_CONTROLS,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sMouseSensitivity),
        &StartMenu::SetMouseSensitivity,
        MODE_CONTROLS,
        UserOption::UOPT_METER,
        16,
        0
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sDisableXBoxController),
        &SetXboxController,
        MODE_CONTROLS,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
    pUserOption = new UserOption(
        static_cast<const char*>(sRumble),
        &StartMenu::SetRumble,
        MODE_CONTROLS,
        UserOption::UOPT_TOGGLE,
        2,
        0,
        sOff.String(),
        sOn.String()
    );
    StartMenu::pUserOptionsA.Add(pUserOption);
}

void StartMenu::DestroyMenuElements() {
    for (Option* pOption : StartMenu::pOptionsA) {
        delete pOption;
    }
    StartMenu::pOptionsA.Clear(true);
    for (UserOption* pOption : StartMenu::pUserOptionsA) {
        delete pOption;
    }
    StartMenu::pUserOptionsA.Clear(true);
    StartMenu::kVideoResolutions.Clear(true);
}

void StartMenu::SetRumble(UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        Controls::EnableRumble(static_cast<bool>(bGamePadRumble = (apUOpt->iVal == 1)));
    }
    else {
        apUOpt->iVal = static_cast<bool>(bGamePadRumble);
        apUOpt->iDefaultVal = static_cast<bool>(XGamerProfile::Instance()->GetRumbleDefault());
    }
}

// The meter's fraction is iVal / (iNumSettings - 1), the count widened as unsigned, rounded to a float local; loading
// rounds (iNumSettings - 1) times the fraction to the nearest step and truncates it.
void StartMenu::SetHUDOpacity(UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fOpacity = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        fHudOpacity = fOpacity;
        StartMenu::pMe->SetFlag(FLAG_RESET_HUD_OPACITY, true);
    }
    else {
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fHudOpacity.Float());
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * 1.0);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// The multiplier spans fItemLODMin..fItemLODMax; loading reads it back through QFadeOutMultiplier.
void StartMenu::SetItemFade(UserOption* apUOpt) {
    float fMin = fItemLODMin.Float();
    float fMax = fItemLODMax.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fMult = static_cast<float>((static_cast<double>(fMax) - fMin) * fPercent + fMin);
        BSFadeNode::SetFadeOutMultiplier(LOD_MULT_ITEMS, fMult);
        fLODFadeOutMultItems = fMult;
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(BSFadeNode::QFadeOutMultiplier(LOD_MULT_ITEMS)) - fMin) /
            (static_cast<double>(fMax) - fMin)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(fItemLODDefault.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// The multiplier spans fObjectLODMin..fObjectLODMax; loading reads it back through QFadeOutMultiplier.
void StartMenu::SetObjectFade(UserOption* apUOpt) {
    float fMin = fObjectLODMin.Float();
    float fMax = fObjectLODMax.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fMult = static_cast<float>((static_cast<double>(fMax) - fMin) * fPercent + fMin);
        BSFadeNode::SetFadeOutMultiplier(LOD_MULT_OBJECTS, fMult);
        fLODFadeOutMultObjects = fMult;
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(BSFadeNode::QFadeOutMultiplier(LOD_MULT_OBJECTS)) - fMin) /
            (static_cast<double>(fMax) - fMin)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(fObjectLODDefault.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// The distance spans fGrassMinStartFadeDistance..fGrassMaxStartFadeDistance and is stored through Float.
void StartMenu::SetGrassFade(UserOption* apUOpt) {
    float fMin = fGrassMinStartFadeDistance.Float();
    float fMax = fGrassMaxStartFadeDistance.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fDistance = static_cast<float>((static_cast<double>(fMax) - fMin) * fPercent + fMin);
        fGrassStartFadeDistance.Float() = fDistance;
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(fGrassStartFadeDistance.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(fGrassDefaultStartFadeDistance.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// The fade start spans the minimum..maximum fade starts; applying it also sets the shader manager's fade start and
// end (the start plus fShadowLODRange).
void StartMenu::SetShadowFade(UserOption* apUOpt) {
    float fMin = Renderer::fShadowLODMinStartFade.Float();
    float fMax = Renderer::fShadowLODMaxStartFade.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fStart = static_cast<float>((static_cast<double>(fMax) - fMin) * fPercent + fMin);
        Renderer::fShadowLODStartFade.Float() = fStart;
        BSShaderManager::fShadowLODStartFade = fStart;
        BSShaderManager::fShadowLODEnd = static_cast<float>(
            static_cast<double>(fStart) + Renderer::fShadowLODRange.Float()
        );
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(Renderer::fShadowLODStartFade.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(Renderer::fShadowLODDefaultStartFade.Float()) - fMin) /
            (static_cast<double>(fMax) - fMin)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// As SetShadowFade, for the light LOD fade.
void StartMenu::SetLightFade(UserOption* apUOpt) {
    float fMin = Renderer::fLightLODMinStartFade.Float();
    float fMax = Renderer::fLightLODMaxStartFade.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fStart = static_cast<float>((static_cast<double>(fMax) - fMin) * fPercent + fMin);
        Renderer::fLightLODStartFade.Float() = fStart;
        BSShaderManager::fLightLODStartFade = fStart;
        BSShaderManager::fLightLODEnd = static_cast<float>(
            static_cast<double>(fStart) + Renderer::fLightLODRange.Float()
        );
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(Renderer::fLightLODStartFade.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(Renderer::fLightLODDefaultStartFade.Float()) - fMin) /
            (static_cast<double>(fMax) - fMin)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// As SetShadowFade, for the specular LOD fade.
void StartMenu::SetSpecularityFade(UserOption* apUOpt) {
    float fMin = Renderer::fSpecularLODMinStartFade.Float();
    float fMax = Renderer::fSpecularLODMaxStartFade.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fStart = static_cast<float>((static_cast<double>(fMax) - fMin) * fPercent + fMin);
        Renderer::fSpecularLODStartFade.Float() = fStart;
        BSShaderManager::fSpecularLODStartFade = fStart;
        BSShaderManager::fSpecularLODEnd = static_cast<float>(
            static_cast<double>(fStart) + Renderer::fSpecularLODRange.Float()
        );
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(Renderer::fSpecularLODStartFade.Float()) - fMin) / (static_cast<double>(fMax) - fMin)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(Renderer::fSpecularLODDefaultStartFade.Float()) - fMin) /
            (static_cast<double>(fMax) - fMin)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// As SetSpecularityFade, for the object LOD (block load) distance.
void StartMenu::SetObjectLODFade(UserOption* apUOpt) {
    float fLow = fLowBlockLoadDistanceLow.Float();
    float fHigh = fHighBlockLoadDistanceLow.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fDistance = static_cast<float>((static_cast<double>(fHigh) - fLow) * fPercent + fLow);
        fBlockLoadDistanceLow.Float() = fDistance;
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(fBlockLoadDistanceLow.Float()) - fLow) / (static_cast<double>(fHigh) - fLow)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(fDefaultBlockLoadDistanceLow.Float()) - fLow) / (static_cast<double>(fHigh) - fLow)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// As SetObjectLODFade, for the tree load distance.
void StartMenu::SetTreeLODFade(UserOption* apUOpt) {
    float fLow = fLowTreeLoadDistance.Float();
    float fHigh = fHighTreeLoadDistance.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        float fDistance = static_cast<float>((static_cast<double>(fHigh) - fLow) * fPercent + fLow);
        fTreeLoadDistance.Float() = fDistance;
    }
    else {
        float fPercent = static_cast<float>(
            (static_cast<double>(fTreeLoadDistance.Float()) - fLow) / (static_cast<double>(fHigh) - fLow)
        );
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = static_cast<float>(
            (static_cast<double>(fDefaultTreeLoadDistance.Float()) - fLow) / (static_cast<double>(fHigh) - fLow)
        );
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

// fMouseSensitivity = fraction * 0.0095 + 0.0005, both double constants; the default fraction is 0.15789475f.
void StartMenu::SetMouseSensitivity(UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SAVE_OPTIONS)) {
        float fPercent = static_cast<float>(apUOpt->iVal / static_cast<double>(apUOpt->iNumSettings - 1));
        double dSensitivity = fPercent * 0.009499999694526196 + 0.0005000000237487257;
        fMouseSensitivity.Float() = static_cast<float>(dSensitivity);
    }
    else {
        float fPercent = static_cast<float>((fMouseSensitivity.Float() - 0.0005000000237487257) / 0.009499999694526196);
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fValue, 1.0f));
        fPercent = 0.15789475f;
        float fDefault = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * fPercent);
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
    }
}

void StartMenu::ChooseBack() {
    if (StartMenu::pMe && StartMenu::pMe->lbMain.IsVisible()) {
        if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_DONT_FORCE_BACK)) {
            const char* pOk = sOk.String();
            Interface::CreateMessageMenu(
                sNoDeviceSelected.String(),
                static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr),
                static_cast<void (*)()>(nullptr),
                0,
                18,
                0.0f,
                0.0f,
                pOk,
                nullptr
            );
        }
        StartMenu::pMe->SetFlag(StartMenu::FLAG_DONT_FORCE_BACK, 0);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_LIVE_MENU, 0);
        StartMenu::pMe->pTiles[10]->Set(4003, 0);
        if (StartMenu::pMe->lbOptions.IsEnabled() || StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
            StartMenu::pMe->HideOptionsContainer();
            StartMenu::ChooseSettings();
            return;
        }
        if (StartMenu::pMe->lbConfirm.IsEnabled()) {
            StartMenu::pMe->lbConfirm.SetEnabled(false);
            StartMenu::pMe->Fade(StartMenu::eConfirmContainer, false, Tile::kTileValue_Invalid, -1.0f);
            if (StartMenu::pMe->lbSaveLoad.IsVisible()) {
                StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVELOAD_BACKOUT, 1);
                return;
            }
        }
        else if (StartMenu::pMe->lbSettings.IsEnabled()) {
            StartMenu::pMe->Fade(StartMenu::eSettingsContainer, false, Tile::kTileValue_Invalid, -1.0f);
            StartMenu::pMe->lbSettings.SetEnabled(false);
        }
        else if (StartMenu::pMe->lbDownloads.IsEnabled()) {
            StartMenu::pMe->Fade(StartMenu::eDLCContainer, false, Tile::kTileValue_Invalid, -1.0f);
            StartMenu::pMe->lbDownloads.SetEnabled(false);
        }
        else if (StartMenu::pMe->lbSaveLoad.IsEnabled()) {
            StartMenu::pMe->UpdateCanLoadStatus();
            // Highlight and RemoveAll are called through a pointer to the list so that they dispatch virtually; a call
            // on the member itself would be devirtualised.
            StartMenu::pMe->lbSaveLoad.Highlight(nullptr);
            StartMenu::pMe->UpdateHighlightedSaveGameInfo();
            StartMenu::pMe->lbSaveLoad.SetEnabled(false);
            StartMenu::pMe->Fade(StartMenu::eSaveLoadContainer, false, Tile::kTileValue_Invalid, -1.0f);
            StartMenu::pMe->lbSaveLoad.RemoveAll();
        }
        else if (!StartMenu::pMe->lbMain.IsEnabled()) {
            if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVELOAD_BACKOUT)) {
                return;
            }
        }
        else {
            if (StartMenu::PauseMenuIsActive() && !StartMenu::pMe->IsFlagSet(StartMenu::FLAG_FORCE_RELOAD)) {
                StartMenu::Close(true);
            }
            return;
        }
        StartMenu::pMe->lbMain.SetEnabled(true);
        StartMenu::pMe->lbMain.DeselectAll(true);
        StartMenu::pMe->Fade(StartMenu::eBackButton, false, Tile::kTileValue_Invalid, -1.0f);
        if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_OPTIONS_DIRTY)) {
            StartMenu::SaveSettings();
            StartMenu::pMe->SetFlag(StartMenu::FLAG_OPTIONS_DIRTY, 0);
        }
    }
}

// The PC build keeps the tests of the console's DLC handling but does nothing with them.
void StartMenu::OnNetworkEvent(BSNetworkSystemUtility::NetEvent apEvent, uint32_t apParam) {
    if (apEvent == BSNetworkSystemUtility::NetworkStatusChanged && StartMenu::pMe && StartMenu::pMe->pDownloadsOption) {
        StartMenu::pMe->IsFlagSet(FLAG_INGAME);
    }
}

bool StartMenu::IsInPreIIS() {
    return StartMenu::MainMenuIsActive() && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_IIS_PREFADE);
}

void StartMenu::ConfirmResetToDefaults() {
    if (StartMenu::pMe && Interface::GetMessageMenuResult() == 1) {
        uint32_t uiCurrentControlsDevice = StartMenu::pMe->uiCurrentControlsDevice;
        appMain->GetControls()->InitDefaultKeybinds(uiCurrentControlsDevice);
        StartMenu::pMe->SetFlag(FLAG_OPTIONS_DIRTY, 1);
        (&StartMenu::pMe->lbOptions)
            ->ListBox<StartMenu::Option*>::Traverse(
                &StartMenu::SetActionForDevice,
                0,
                std::numeric_limits<int32_t>::max()
            );
    }
}

void StartMenu::ResetToDefaults(Tile* apTile, StartMenu::Option* apOption) {
    StartMenu::UserOption* pUserOption = dynamic_cast<StartMenu::UserOption*>(apOption);
    if (pUserOption && pUserOption->iVal != pUserOption->iDefaultVal) {
        pUserOption->iVal = pUserOption->iDefaultVal;
        StartMenu::pMe->DisplayOption(pUserOption, apTile);
        if (pUserOption->pfChangeFn) {
            pUserOption->pfChangeFn(pUserOption);
            StartMenu::pMe->SetFlag(StartMenu::FLAG_OPTIONS_DIRTY, 1);
        }
    }
}

void StartMenu::GoBackIfInActionMapping() {
    if (StartMenu::pMe) {
        bool bHas360Controller = InterfaceManager::Has360Controller();
        StartMenu::pMe->pMenu->Set(Tile::kTileValue_User5, static_cast<int32_t>(bHas360Controller));
        Controls::EnableRumble(InterfaceManager::Has360Controller());
        if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
            StartMenu::ChooseBack();
        }
    }
}

void StartMenu::SetActionForDevice(Tile* apTile, StartMenu::Option* apOption) {
    if (StartMenu::pMe) {
        BSStringT<char> kText;
        apTile->Set(Tile::kTileValue_User0, 1);
        uint32_t uiCurrentControlsDevice = StartMenu::pMe->uiCurrentControlsDevice;
        // The action-mapping options keep their action in iModes.
        uint32_t uiAction = apOption->iModes;
        uint8_t ucKey = appMain->GetControls()->RemapAction(
            static_cast<Controls::USERACTIONS>(uiAction),
            uiCurrentControlsDevice
        );
        if (ucKey != 255) {
            if (!StartMenu::pMe->uiCurrentControlsDevice && ucKey < 238u && psKeyboard[ucKey]) {
                kText.SPrintF("%s", psKeyboard[ucKey]->String());
            }
            else if (StartMenu::pMe->uiCurrentControlsDevice == StartMenu::MODE_MAIN && ucKey < 9u && psMouse[ucKey]) {
                kText.SPrintF("%s", psMouse[ucKey]->String());
            }
            else if (StartMenu::pMe->uiCurrentControlsDevice != StartMenu::MODE_PAUSE) {
                const char* pButton = nullptr;
                if (StartMenu::pMe->uiCurrentControlsDevice == 3) {
                    switch (ucKey) {
                        case 7: {
                            pButton = "back";
                            break;
                        }
                        case 8: {
                            pButton = "ls";
                            break;
                        }
                        case 9: {
                            pButton = "rs";
                            break;
                        }
                        case 10: {
                            pButton = "a";
                            break;
                        }
                        case 11: {
                            pButton = "b";
                            break;
                        }
                        case 12: {
                            pButton = "x";
                            break;
                        }
                        case 13: {
                            pButton = "y";
                            break;
                        }
                        case 14: {
                            pButton = "rb";
                            break;
                        }
                        case 15: {
                            pButton = "lb";
                            break;
                        }
                        case 16: {
                            pButton = "lt";
                            break;
                        }
                        case 17: {
                            pButton = "rt";
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                if (pButton) {
                    kText.SPrintF("Interface\\Shared\\Button\\glow_general_button_%s.dds", pButton);
                    apTile->Set(Tile::kTileValue_User0, 2);
                }
                else {
                    kText.SPrintF("%d", ucKey);
                }
            }
            else if (ucKey < appMain->GetControls()->GetButtonCount(0)) {
                kText.SPrintF("%s %d", sButton.String(), ucKey + 1);
            }
            else if (ucKey >= 32u && ucKey < 40u && psWeaponTypes[ucKey + 2]) {
                kText.SPrintF("%s", psWeaponTypes[ucKey + 2]->String());
            }
            else {
                kText.Set("--", 0);
            }
        }
        else {
            kText.Set("--", 0);
        }
        apTile->Set(Tile::kTileValue_User1, kText.pString, true);
    }
}

void StartMenu::ChooseContinue() {
    if (StartMenu::MainMenuIsActive()) {
        const char* pText = sConfirmContinue.String();
        StartMenu::pMe->pTiles[8]->Set(4036, pText, true);
        StartMenu::pMe->PopulateConfirmationListBox(256u, 0);
    }
    else {
        StartMenu::Close(true);
    }
}

void StartMenu::ChooseNew() {
    if (StartMenu::pMe) {
        const char* pText = sConfirmNew.String();
        StartMenu::pMe->pTiles[8]->Set(4036, pText, true);
        StartMenu::pMe->PopulateConfirmationListBox(128u, 0);
    }
}

void StartMenu::ChooseDownloads() {
    if (StartMenu::pMe) {
        BSSimpleList<TESFile*>* pIter = pDataHandler->GetFileList();
        StartMenu::pMe->lbDownloads.RemoveAll();
        const char* pOldDownloads = sOldDownloadsAvailable.String();
        StartMenu::pMe->lbDownloads.AddItem(-1, pOldDownloads, nullptr, nullptr);
        while (pIter) {
            TESFile* pFile = pIter->m_item;
            if (pFile && pFile->GetMaybeShouldBeReloaded()) {
                char* pSummary = pFile->GetSummary()->pString;
                StartMenu::pMe->lbDownloads.AddItem(0, pSummary, nullptr, nullptr);
            }

            pIter = pIter->m_pkNext;
        }
        StartMenu::pMe->lbDownloads.SetEnabled(true);
        StartMenu::pMe->Fade(StartMenu::eDLCContainer, true, Tile::kTileValue_Invalid, -1.0f);
        StartMenu::pMe->Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
    }
}

void StartMenu::ChooseLoad() {
    if (StartMenu::pMe) {
        StartMenu::pMe->pTiles[15]->Set(Tile::kTileValue_User0, 0);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVE_MODE, 0);
        StartMenu::pMe->PrepSaveLoad();
    }
}

void StartMenu::ChooseSave() {
    if (StartMenu::pMe) {
        StartMenu::pMe->pTiles[15]->Set(Tile::kTileValue_User0, 0);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVE_MODE, 1);
        StartMenu::pMe->PrepSaveLoad();
    }
}

void StartMenu::ChooseSettings() {
    if (StartMenu::pMe) {
        StartMenu::pMe->lbSettings.SetEnabled(true);
        StartMenu::pMe->Fade(StartMenu::eSettingsContainer, true, Tile::kTileValue_Invalid, -1.0f);
        StartMenu::pMe->lbSettings.DeselectAll(true);
        StartMenu::pMe->Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
    }
}

void StartMenu::ChooseHelp() {
    StartMenu::pMe->pTiles[eVersionText]->Set(Tile::kTileValue_Visible, 1);
    BGSMessage* pHelpMsg = nullptr;
    for (int32_t i = 0; !pHelpMsg && i < 10; ++i) {
        Interface::enumMenus eMenuID = static_cast<Interface::enumMenus>(
            InterfaceManager::Instance()->GetEnterStack(i)
        );
        if (eMenuID == StartMenu::GetClassStatic()) {
            break;
        }
        Interface::TutorialMessageID eMsgID = Interface::TUT_COUNT;
        if (eMenuID == Interface::eMainFour) {
            if (Interface::IsMenuIDVisible(Interface::eStatsMenu, 0)) {
                eMsgID = InterfaceManager::Instance()->xTutorialManager.Find(Interface::eStatsMenu);
            }
            else if (Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0)) {
                eMsgID = InterfaceManager::Instance()->xTutorialManager.Find(Interface::ePipBoyRepair);
            }
            else if (Interface::IsMenuIDVisible(Interface::eInventoryMenu, 0)) {
                eMsgID = InterfaceManager::Instance()->xTutorialManager.Find(Interface::eInventoryMenu);
            }
            else if (Interface::IsMenuIDVisible(Interface::ePipBoyData, 0)) {
                eMsgID = InterfaceManager::Instance()->xTutorialManager.Find(Interface::ePipBoyData);
            }
        }
        else {
            eMsgID = InterfaceManager::Instance()->xTutorialManager.Find(eMenuID);
        }
        if (eMsgID < Interface::TUT_COUNT) {
            TESForm* pListForm;
            bool bFound;
            BGSListForm* pList;
            uint32_t uiSearchID;
            TESForm* pForm = TESForm::GetFormByNumericID(eMsgID + 360);
            BGSMessage* pStartMsg = dynamic_cast<BGSMessage*>(pForm);
            if (pStartMsg) {
                uiSearchID = InterfaceManager::Has360Controller() ? 357 : 355;
                pListForm = TESForm::GetFormByNumericID(uiSearchID);
                pList = dynamic_cast<BGSListForm*>(pListForm);
                if (pList) {
                    bFound = false;
                    for (TESForm* const& rpForm : pList->ListOfForms.ItemsUntilNull()) {
                        if (rpForm == pStartMsg) {
                            bFound = true;
                            break;
                        }
                    }
                    if (!bFound) {
                        continue;
                    }
                }
            }
            pHelpMsg = pStartMsg;
        }
    }
    TutorialMenu::Create(pHelpMsg, true);
}

void StartMenu::ChooseQuit() {
    if (StartMenu::pMe) {
        const char* pText = sConfirmQuit.String();
        StartMenu::pMe->pTiles[8]->Set(4036, pText, true);
        if (StartMenu::PauseMenuIsActive()) {
            StartMenu::pMe->PopulateConfirmationListBox(
                StartMenu::MODE_CFM_QUITP,
                StartMenu::CONFIRMATION_WIDE | StartMenu::CONFIRMATION_SHOW_WARNING
            );
        }
        else {
            StartMenu::pMe->PopulateConfirmationListBox(StartMenu::MODE_CFM_QUITM, StartMenu::CONFIRMATION_WIDE);
        }
    }
}

void StartMenu::ChooseMainMenu() {
    char acMusic[68];
    if (StartMenu::pMe) {
        delete StartMenu::pMe;
    }
    Interface::HideMenus();
    Interface::CloseAllSupportMenus(false);
    BSAudio::QInstance().KillAllOfType(0xFFFFFFFFu, 0);
    FalloutAudioMedia::MediaClear();
    pPlayer->GetMediaLocationController();
    if (XGamerProfile::Instance()) {
        XGamerProfile::Instance()->SetPlayingFlag(false);
        XGamerProfile::Instance()->ForcePresenceUpdate();
    }
    StartMenu::bRestartingGame = true;
    appMain->ResetGame();
    TES::pInstance->ShowLoadingMenu(true, nullptr, false);
    ourMemset(acMusic, 0, 64u);
    BSsprintf(acMusic, 64u, "Data\\Music\\Special\\MainTitle.mp3");
    FalloutAudio::SetMainMenuMusic(true);
    FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, acMusic, 0x1388u, true, true, 0.0f, 0);
    FalloutAudioMedia::MediaPlay();
}

void StartMenu::ChooseExitGame() {
    InterfaceManager* pIM = InterfaceManager::Instance();
    pIM->GetMenusRoot()->GetModel()->SetAppCulled(true);
    pIM->pCursor->GetModel()->SetAppCulled(true);
    pIM->Click(nullptr, nullptr);
    appMain->QuitGame();
}

void StartMenu::ChooseActionMap() {
    if (StartMenu::pMe) {
        StartMenu::pMe->Fade(StartMenu::eOptionsContainer, false, Tile::kTileValue_Invalid, -1.0f);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_ACTION_MAPPING, 1);
        if (!StartMenu::pMe->IsControlsDeviceValid()) {
            StartMenu::pMe->NextControlsDevice();
        }
    }
}

void StartMenu::ChooseGameplay() {
    if (StartMenu::pMe) {
        StartMenu::pMe->lbSettings.SetEnabled(false);
        StartMenu::pMe->PopulateOptionsListBox(8);
    }
}

void StartMenu::ChooseControls() {
    if (StartMenu::pMe) {
        StartMenu::pMe->lbSettings.SetEnabled(false);
        StartMenu::pMe->pTiles[15]->Set(Tile::kTileValue_User0, 0);
        StartMenu::pMe->PopulateOptionsListBox(64);
    }
}

void StartMenu::SetDifficulty(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        int32_t& riDifficulty = gs_iDifficulty_GamePlay.Int();
        riDifficulty = apUOpt->iVal;
        pPlayer->SetDifficultyLevel(static_cast<GamePlayFormulas::DifficultyLevel>(riDifficulty));
    }
    else {
        apUOpt->iVal = pPlayer->GetGameDifficulty();
        apUOpt->iDefaultVal = 2;
    }
}

void StartMenu::SetKillCam(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        int32_t& riKillCam = gs_iKillCamera_GamePlay.Int();
        riKillCam = apUOpt->iVal;
        pPlayer->ToggleKillCamMode(static_cast<KILL_CAM_TYPE>(riKillCam));
    }
    else {
        apUOpt->iVal = pPlayer->GetKillCamMode();
        apUOpt->iDefaultVal = 2;
    }
}

void StartMenu::SetHardcore(StartMenu::UserOption* apOption) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        pPlayer->SetHardcoreCallback(apOption->iVal);
    }
    else {
        apOption->iVal = pPlayer->IsHardcore();
        apOption->iDefaultVal = 0;
    }
}

void StartMenu::SetCrosshair(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bCrossHair_GamePlay = apUOpt->iVal == 1;
        uint32_t uiVisible = gs_bCrossHair_GamePlay;
        HUDMainMenu::SetVisibilityFlag(HUDMainMenu::HV_CROSSHAIR, uiVisible);
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bCrossHair_GamePlay);
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetSaveOnRest(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bSaveOnRest_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bSaveOnRest_GamePlay);
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetSaveOnWait(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bSaveOnWait_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bSaveOnWait_GamePlay);
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetSaveOnTravel(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bSaveOnTravel_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bSaveOnTravel_GamePlay);
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetDialogSubtitles(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bDialogueSubtitles_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bDialogueSubtitles_GamePlay);
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetGeneralSubtitles(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        static_cast<SettingT<INIPrefSettingCollection>*>(&gs_bGeneralSubtitles_Gameplay)
            ->SettingT<INIPrefSettingCollection>::operator=(apUOpt->iVal == 1);
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bGeneralSubtitles_Gameplay);
        apUOpt->iDefaultVal = 0;
    }
}

void StartMenu::SetTrueIronSights(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bTrueIronSights_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bTrueIronSights_GamePlay);
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetHUDColor(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        uint32_t uiColor = g_HUDColors[apUOpt->iVal];
        SystemColorManager* pColorManager = SystemColorManager::GetInstance();
        pColorManager->SetColor(1, uiColor);
        InterfaceManager::Instance()->GetMenusRoot()->RecursiveColorUpdate(1, 0.0f);
    }
    else {
        apUOpt->iDefaultVal = 2;
        apUOpt->iVal = 0;
        while (true) {
            int32_t iVal = apUOpt->iVal;
            SystemColorManager* pColorManager = SystemColorManager::GetInstance();
            if (g_HUDColors[iVal] == pColorManager->GetColor(1)) {
                break;
            }
            if (++apUOpt->iVal >= apUOpt->iNumSettings) {
                apUOpt->iVal = apUOpt->iDefaultVal;
                return;
            }
        }
    }
}

void StartMenu::SetPipboyColor(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        uint32_t uiColor = g_HUDColors[apUOpt->iVal];
        SystemColorManager* pColorManager = SystemColorManager::GetInstance();
        pColorManager->SetColor(4, uiColor);
        InterfaceManager::Instance()->GetMenusRoot()->RecursiveColorUpdate(4, 0.0f);
    }
    else {
        apUOpt->iDefaultVal = 2;
        apUOpt->iVal = 0;
        while (true) {
            int32_t iVal = apUOpt->iVal;
            SystemColorManager* pColorManager = SystemColorManager::GetInstance();
            if (g_HUDColors[iVal] == pColorManager->GetColor(4)) {
                break;
            }
            if (++apUOpt->iVal >= apUOpt->iNumSettings) {
                apUOpt->iVal = apUOpt->iDefaultVal;
                return;
            }
        }
    }
}

void StartMenu::SetBrightness(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        float fMin = fGammaMin.Float();
        float fPct = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
        float fGamma = (fGammaMax.Float() - fMin) * fPct + fMin;
        Renderer::SetGamma(fGamma);
    }
    else {
        double dCurrentGamma = Get_fGamma();
        double dOffset = dCurrentGamma - fGammaMin.Float();
        float fPct = dOffset / (fGammaMax.Float() - fGammaMin.Float());
        float fVal = static_cast<double>(apUOpt->iNumSettings - 1) * fPct;
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
        dOffset = 1.0 - fGammaMin.Float();
        fPct = dOffset / (fGammaMax.Float() - fGammaMin.Float());
        fVal = static_cast<double>(apUOpt->iNumSettings - 1) * fPct;
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
    }
}

void StartMenu::SetMasterVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float fVolume = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float fNewVolume = (gs_fDefaultMasterVolume_Audio = fVolume);
            BSAudio::QInstance().SetMasterVolume(fNewVolume);
        }
        else {
            BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float fVal = pAudio->GetMasterVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
            float fDefault = static_cast<double>(apUOpt->iNumSettings - 1) * 1.0;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
        }
    }
    else {
        float fVolume = gs_fDefaultMasterVolume_Audio;
        BSAudio::QInstance().SetMasterVolume(fVolume);
    }
}

void StartMenu::SetFootstepVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float fVolume = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float fNewVolume = (gs_fDefaultFootVolume_Audio = fVolume);
            BSAudio::QInstance().SetFootVolume(fNewVolume);
        }
        else {
            BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float fVal = pAudio->GetFootVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
            float fDefault = static_cast<double>(apUOpt->iNumSettings - 1) * 0.5;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
        }
    }
    else {
        float fVolume = gs_fDefaultFootVolume_Audio;
        BSAudio::QInstance().SetFootVolume(fVolume);
    }
}

void StartMenu::SetMusicVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float fVolume = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float fNewVolume = (gs_fDefaultMusicVolume_Audio = fVolume);
            BSAudio::QInstance().SetMusicVolume(fNewVolume);
        }
        else {
            BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float fVal = pAudio->GetMusicVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
            float fDefault = static_cast<double>(apUOpt->iNumSettings - 1) * 0.6000000238418579;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
        }
    }
    else {
        float fVolume = gs_fDefaultMusicVolume_Audio;
        BSAudio::QInstance().SetMusicVolume(fVolume);
    }
}

void StartMenu::SetVoiceVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float fVolume = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float fNewVolume = (gs_fDefaultVoiceVolume_Audio = fVolume);
            BSAudio::QInstance().SetVoiceVolume(fNewVolume);
        }
        else {
            BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float fVal = pAudio->GetVoiceVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
            float fDefault = static_cast<double>(apUOpt->iNumSettings - 1) * 0.6000000238418579;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
        }
    }
    else {
        float fVolume = gs_fDefaultVoiceVolume_Audio;
        BSAudio::QInstance().SetVoiceVolume(fVolume);
    }
}

void StartMenu::SetFXVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float fVolume = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float fNewVolume = (gs_fDefaultEffectsVolume_Audio = fVolume);
            BSAudio::QInstance().SetEffectVolume(fNewVolume);
        }
        else {
            BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float fVal = pAudio->GetEffectsVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
            float fDefault = static_cast<double>(apUOpt->iNumSettings - 1) * 1.0;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
        }
    }
    else {
        float fVolume = gs_fDefaultEffectsVolume_Audio;
        BSAudio::QInstance().SetEffectVolume(fVolume);
    }
}

void StartMenu::SetRadioVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float fVolume = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float fNewVolume = (gs_fDefaultRadioVolume_Audio = fVolume);
            BSAudio::QInstance().SetRadioVolume(fNewVolume);
        }
        else {
            BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float fVal = pAudio->GetRadioVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
            float fDefault = static_cast<double>(apUOpt->iNumSettings - 1) * 0.5;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fDefault, 1.0f));
        }
    }
    else {
        float fVolume = gs_fDefaultRadioVolume_Audio;
        BSAudio::QInstance().SetRadioVolume(fVolume);
    }
}

void StartMenu::SetCameraPitch(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bInvertYValues_Controls = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = static_cast<bool>(gs_bInvertYValues_Controls);
        apUOpt->iDefaultVal = 0;
    }
}

void StartMenu::ConfirmContinue() {
    if (!pSaveLoadManager->pSaveGameList || !pSaveLoadManager->pSaveGameList->m_item) {
        pSaveLoadManager->SetupSavesList();
    }
    if (pSaveLoadManager->pSaveGameList && pSaveLoadManager->pSaveGameList->m_item) {
        if (pSaveLoadManager->pSaveGameList->m_item->IsCorrupt()) {
            const char* pOk = sOk.String();
            Interface::CreateMessageMenu(
                sSaveGameCorruptMenuMessage.String(),
                static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr),
                static_cast<void (*)()>(nullptr),
                0,
                5,
                0.0f,
                0.0f,
                pOk,
                nullptr
            );
        }
        else {
            ArchiveManager::CleanupArchiveStringsAfterStartup();
            pSaveLoadManager->pSaveGameList->m_item->LoadGame();
        }
    }
    else {
        const char* pOk = sOk.String();
        Interface::CreateMessageMenu(
            sSaveNotAvailable.String(),
            static_cast<const char*>(nullptr),
            static_cast<const char*>(nullptr),
            static_cast<void (*)()>(nullptr),
            0,
            5,
            0.0f,
            0.0f,
            pOk,
            nullptr
        );
    }
    StartMenu::ChooseBack();
}

void StartMenu::ConfirmNewGame() {
    char* pStopString;
    uint32_t uiCharGenQuest = std::strtol(sCharGenQuest.String(), &pStopString, 16);
    TESForm* pForm = TESForm::GetFormByNumericID(uiCharGenQuest);
    TESQuest* pCharGenQuest = nullptr;
    if (pPlayer) {
        pPlayer->SetDisableFastTravelResetsOnCellChange(0);
        pPlayer->SetCanFastTravel(true);
    }
    if (pForm && pForm->GetFormType() == QUST_ID) {
        pCharGenQuest = dynamic_cast<TESQuest*>(pForm);
    }

    if (pCharGenQuest) {
        StartMenu::Close(false);
        ArchiveManager::CleanupArchiveStringsAfterStartup();
        Interface::bIsLoadingNewGame = true;
        pPlayer->SetAllowEGMCacheClear(false);
        TESNPC* pPlayerBase = dynamic_cast<TESNPC*>(pPlayer->GetObjectReference());
        pPlayerBase->SetFullName(static_cast<const char*>(sDefaultPlayerName));
        pCharGenQuest->SetEnabled(1);
        LoadingMenu::pMe->pTiles[11]->Set(4003, 1);
        LoadingMenu::FlagReset(true);
        InitializationFlow();
        FalloutAudio::SetMainMenuMusic(false);
        FalloutAudioMedia::MediaClose();
        if (XGamerProfile::Instance()) {
            XGamerProfile::Instance()->SetPlayingFlag(true);
            XGamerProfile::Instance()->ForcePresenceUpdate();
        }
        FalloutAudio::SetNewGameMusic(true);
        InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_STATS, 1003, 512u);
        InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_DATA, 1023, 512u);
    }
    else {
        Warning("QUESTS: Unable to find character generation quest '%s'.", sCharGenQuest.String());
        StartMenu::ChooseBack();
    }
}

void StartMenu::ConfirmLoadGame() {
    if (StartMenu::pMe && StartMenu::pMe->pSaveGame) {
        ArchiveManager::CleanupArchiveStringsAfterStartup();
        BGSSaveLoadFileEntry* pSaveFile = StartMenu::pMe->pSaveGame;
        if (pPlayer && StartMenu::PauseMenuIsActive()) {
            TESNPC* pTempPlayer = dynamic_cast<TESNPC*>(pPlayer->GetObjectReference());
            if (pTempPlayer && pTempPlayer->GetFullName() && pSaveFile->GetPlayerName()) {
                const char* pPlayerName = pSaveFile->GetPlayerName();
                if (BSstrcmp(pTempPlayer->GetFullName(), pPlayerName)) {
                    TES::pInstance->ShowLoadingMenu(true, nullptr, false);
                }
            }
        }

        bool bResult = pSaveFile->LoadGame();
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_INGAME) && bResult) {
            StartMenu::Close(true);
        }
        else {
            if (!bResult && StartMenu::pMe) {
                StartMenu::pMe->IsFlagSet(StartMenu::FLAG_LOAD_MISSING_CONTENT);
            }

            StartMenu::ChooseBack();
        }
    }
}

void StartMenu::ConfirmSaveGame() {
    if (StartMenu::pMe) {
        StartMenu::pMe->lbConfirm.RemoveAll();
        const char* pMessage = sMenuDisplayShortXBoxSaveMessage.String();
        StartMenu::pMe->pTiles[8]->Set(4036, pMessage, true);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVE_WAIT_MESSAGE, 1);
    }
}

void StartMenu::ConfirmDeleteGm() {
    if (StartMenu::pMe && StartMenu::pMe->lbSaveLoad.GetHighlightedItem()) {
        ListBox<BGSSaveLoadFileEntry*>* pLB = &StartMenu::pMe->lbSaveLoad;
        StartMenu::pMe->lbSaveLoad.GetHighlightedData()->DeleteGame();
        Tile* pItem = static_cast<Tile*>(pLB->GetHighlightedItem());
        pLB->RemoveItem(pItem);
        StartMenu::ChooseBack();
        uint32_t uiNumItems = -1;
        uint32_t uiCount = pLB->NumUnfiltered();
        uint32_t uiVisible = pLB->NumVisible();
        if (uiCount) {
            bool bAtThreshold = false;
            if (uiCount >= uiVisible) {
                int32_t iValueTrait = Tile::AddUserTrait("_current_value", -1);
                bAtThreshold = StartMenu::pMe->fSaveLoadThreshold == pLB->pScrollBar->GetFloat(iValueTrait);
            }
            if (bAtThreshold) {
                int32_t iIndex = static_cast<int32_t>(
                    static_cast<double>(uiVisible) + StartMenu::pMe->fSaveLoadThreshold - 1.0
                );
                pLB->Traverse(&StartMenu::InitializeSaveLoadListItem, iIndex, iIndex + 1);
                uiNumItems = StartMenu::pMe->lbSaveLoad.NumItems();
                if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
                    for (int32_t i = 0; i < StartMenu::pMe->lbSaveLoad.NumItems(); ++i) {
                        Tile* tTile = StartMenu::pMe->lbSaveLoad.GetTileAt(static_cast<uint32_t>(i), false);
                        BGSSaveLoadFileEntry* tSave = StartMenu::pMe->lbSaveLoad.GetDataAt(i, false);
                        if (tTile && tSave) {
                            if (tSave->IsAutosave()) {
                                StartMenu::pMe->lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                                --uiNumItems;
                            }
                            else if (tSave->IsSystemSave()) {
                                StartMenu::pMe->lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                                --uiNumItems;
                            }
                        }
                    }
                }
            }
            else if (StartMenu::pMe->fSaveLoadThreshold != 0.0) {
                StartMenu::pMe->fSaveLoadThreshold = StartMenu::pMe->fSaveLoadThreshold - 1.0;
            }
        }
        else {
            StartMenu::pMe->ReturnToSaveLoad();
            StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVELOAD_BACKOUT, 0);
            StartMenu::ChooseBack();
        }
        if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_MODE) && StartMenu::pMe->lbSaveLoad.NumItems()) {
            char acText[68];
            if (uiNumItems == -1) {
                uiNumItems = StartMenu::pMe->lbSaveLoad.NumItems();
                for (int32_t i = 0; i < StartMenu::pMe->lbSaveLoad.NumItems(); ++i) {
                    Tile* tTile = StartMenu::pMe->lbSaveLoad.GetTileAt(static_cast<uint32_t>(i), false);
                    BGSSaveLoadFileEntry* tSave = StartMenu::pMe->lbSaveLoad.GetDataAt(i, false);
                    if (tTile && tSave) {
                        if (tSave->IsAutosave()) {
                            --uiNumItems;
                        }
                        else if (tSave->IsSystemSave()) {
                            --uiNumItems;
                        }
                    }
                }
            }
            --uiNumItems;
            const char* pSavesRemaining = sSavesRemaining.String();
            BSsprintf(acText, 64, "%s %u/%i %s", sNewSave.String(), uiNumItems, 1000, pSavesRemaining);
            StartMenu::pMe->lbSaveLoad.GetFirstItem()->Set(4036, acText, true);
            StartMenu::pMe->lbSaveLoad.GetFirstItem()->AddNeedsUpdate(8);
        }
    }
}

void StartMenu::MountDLCCallback(uint32_t which, uint32_t last, bool valid, const BSContentPackage* package) {
    if (which == -1 && last == -1) {
        Interface::CloseMessageMenu();
        StartMenu::pMe->ShowDLCList();
    }
    else {
        char acName[132];
        const char16_t* pDisplayName = package->DisplayName();
        rt::wcstombs(acName, pDisplayName, 128u);
        StartMenu::pMe->lbDownloads.AddItem(0, acName, nullptr, nullptr);
    }
}

void StartMenu::FailedSaveCallbackFunction() {
    StartMenu::pMe->SetFlag(StartMenu::FLAG_MEM_CARD_REMOVED, 0);
    MessageMenu::ClearMessageQueue();
    Interface::CloseMessageMenu();
    StartMenu::ChooseBack();
}

void StartMenu::InitializeSaveLoadListItem(Tile* apTile, BGSSaveLoadFileEntry* apSaveGame) {
    if (apTile && apSaveGame) {
        int32_t iInitializedTrait = Tile::AddUserTrait("_initialized", -1);
        if (!apTile->IsTrue(iInitializedTrait)) {
            const char* pLocation;
            if (apSaveGame->IsAutosave()) {
                apTile->Set(Tile::kTileValue_User0, sAutosaveAbbrev.String(), true);
            }
            else if (apSaveGame->IsQuicksave()) {
                apTile->Set(Tile::kTileValue_User0, sQuicksaveAbbrev.String(), true);
            }
            else if (apSaveGame->IsSystemSave()) {
                apTile->Set(Tile::kTileValue_User0, gs_sSystemSaveAbbrev.String(), true);
            }
            else {
                BSStringT<char> kStrTmp;
                kStrTmp.SPrintF("#%04d", apSaveGame->GetSaveGameNumber());
                apTile->Set(Tile::kTileValue_User0, kStrTmp.pString, true);
            }
            if (apSaveGame->IsCorrupt()) {
                pLocation = sSaveGameCorrupt.String();
            }
            else {
                pLocation = apSaveGame->GetLocation();
            }
            apTile->Set(Tile::kTileValue_User1, pLocation, true);
            iInitializedTrait = Tile::AddUserTrait("_initialized", -1);
            apTile->Set(iInitializedTrait, 1);
        }
    }
}

void StartMenu::SysUtilSaveCompleteCallback(
    BSSaveDataSystemUtility::RESULT aeResult,
    BSSaveDataSystemUtilityFile*& apSaveDataFile
) {
    if (StartMenu::pMe) {
        StartMenu::pMe->SetFlag(StartMenu::FLAG_SYSUTILSAVECOMPLETE, 1);
    }
}

void StartMenu::DeleteOption(Tile* apTile, StartMenu::Option* apOption) {
    if (apOption) {
        delete apOption;
    }
}

void MenuButton_Credits() {
    if (StartMenu::pMe) {
        StartMenu::pMe->SetFlag(StartMenu::FLAG_SHOWING_CREDITS, 1u);
        StartMenu::pMe->Fade(StartMenu::eMainTitle, false, 4009, -1.0f);
        StartMenu::pMe->Fade(StartMenu::eMainContainer, false, 0x80000000u, -1.0f);
        StartMenu::pMe->Fade(StartMenu::eWelcomeText, false, 4009, -1.0f);
    }
}

void MenuButton_Display() {
    if (StartMenu::pMe) {
        StartMenu::pMe->pTiles[10]->Set(4003, 1);
        StartMenu::pMe->lbSettings.SetEnabled(false);
        StartMenu::pMe->PopulateOptionsListBox(16);
    }
}

void MenuButton_Audio() {
    if (StartMenu::pMe) {
        StartMenu::pMe->lbSettings.SetEnabled(false);
        StartMenu::pMe->PopulateOptionsListBox(32);
    }
}

bool GetIsControllerDisabled() {
    return gs_bDisable360Controller;
}

uint32_t* IncrementControlsDevice(uint32_t* apDevice) {
    if (static_cast<int32_t>(++*apDevice) >= 4) {
        *apDevice = 0;
    }
    return apDevice;
}

void SetXboxController(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        bool bEnabled = apUOpt->iVal == 1;
        bool bDisable = apUOpt->iVal != 1;
        InterfaceManager::Instance()->Disable360Controller(bDisable);
        if (StartMenu::pMe->pRumbleOption) {
            StartMenu::pMe->pRumbleOption->Set(
                static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT),
                static_cast<int32_t>(bEnabled)
            );
        }
    }
    else {
        InterfaceManager::Instance();
        apUOpt->iVal = !GetIsControllerDisabled();
        InterfaceManager::Instance();
        char cControllerDisabled = GetIsControllerDisabled();
        appMain->GetControls()->bIsControllerDisabled = cControllerDisabled;
        appMain->GetControls();
        InterfaceManager::Instance();
        bool bDisabled = GetIsControllerDisabled();
        Controls::EnableRumble(!bDisabled);
        apUOpt->iDefaultVal = 1;
    }
}

float Get_fGamma() {
    return gs_fGamma_Display;
}

void SetTextureSize(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        Renderer::iTexMipMapSkip = apUOpt->iVal;
        StartMenu::pMe->SetFlag(StartMenu::FLAG_RESTART_REQUIRED, NiDX9SourceTextureData::uiMipMapSkip != apUOpt->iVal);
    }
    else {
        apUOpt->iVal = NiDX9SourceTextureData::uiMipMapSkip;
        apUOpt->iDefaultVal = 0;
    }
}

void SetActorFade(StartMenu::UserOption* apUOpt) {
    float fActorLODMin = ::fActorLODMin.Float();
    float fActorLODMax = ::fActorLODMax.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        float fPct = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
        float fMult = (fActorLODMax - fActorLODMin) * fPct + fActorLODMin;
        BSFadeNode::SetFadeOutMultiplier(LOD_MULT_ACTORS, fMult);
        fLODFadeOutMultActors = fMult;
    }
    else {
        float fPct = (BSFadeNode::QFadeOutMultiplier(LOD_MULT_ACTORS) - fActorLODMin) / (fActorLODMax - fActorLODMin);
        float fVal = static_cast<double>(apUOpt->iNumSettings - 1) * fPct;
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
        fPct = (fActorLODDefault.Float() - fActorLODMin) / (fActorLODMax - fActorLODMin);
        fVal = static_cast<double>(apUOpt->iNumSettings - 1) * fPct;
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(fVal, 1.0f));
    }
}

void Controls::EnableRumble(bool bTF) {
    if (Controls::pInstance) {
        Controls::pInstance->bIsControllerEnabled = bTF;
    }
}

Interface::TutorialMessageID InterfaceManager::TutorialManager::Find(Interface::enumMenus aeMenu) {
    Interface::TutorialMessageID i;
    for (i = Interface::TUT_PIPBOY_STATUS; i < Interface::TUT_COUNT && tMessages[i].uiMenuOffset + 1001 != aeMenu;
         i = static_cast<Interface::TutorialMessageID>(i + 1)) {
        ;
    }
    return i;
}

void BGSSaveLoadManager::SetSystemUtilitySaveCompleteCallback(
    void (*apCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&)
) {
    pSysUtilSaveCompleteCallback = apCallback;
}

void BGSSaveLoadManager::SetSystemUtilityLoadCompleteCallback(
    void (*apCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&)
) {
    pSysUtilLoadCompleteCallback = apCallback;
}
