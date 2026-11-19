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
#include "BSCore/TLSData.h"
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
#include <new>

static uint32_t uiOpenTime;

// One byte that no code reads or writes, between pTextEntryMenu and bRestartingGame.
static bool data_11DAACC;

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
    MakeRGBA(0x1A, 0xFF, 0x80, 0xFF), MakeRGBA(0x2E, 0xCF, 0xFF, 0xFF), MakeRGBA(0xFF, 0xB6, 0x42, 0xFF),
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
    if (this->IsFlagSet(FLAG_IIS_PREFADE) && LoadingMenu::pMe &&
        appMain->GetControls()->GetMouseButton(0, Controls::isDepressed)) {
        LoadingMenu::pMe->ForceFinish();
    }
#endif
    if (this->IsFlagSet(FLAG_FADE_IN) && !this->pTiles[eMainTitle]->HasFadeControl(Tile::kTileValue_Alpha) &&
        (!pPlayCredits || !(pPlayCredits->GetValue() > 0.0))) {
        const float fAlpha = this->pTiles[eMainTitle]->GetFloat(Tile::kTileValue_Alpha);
        if (fAlpha == 0.0) {
            this->Fade(eMainTitle, true, Tile::kTileValue_Alpha, 1.0f);
            LoadingMenu::SetupForStartMenu(1);
        }
        else if (fAlpha == 255.0) {
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
                static_cast<const char*>(sLoadingContentMessage), nullptr, nullptr, nullptr, 0, 0x12, 0.0f, 0.0f, ""
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
            pText, nullptr, nullptr, nullptr, 0, 0x12, 0.0f, 0.0f, pOk, static_cast<const char*>(nullptr)
        );
    }
    return true;
}

StartMenu::Option::Option(const char* apDisplayText, void (*apf)(), uint32_t aiModes)
  : pDisplayText(apDisplayText), pfChooseFn(apf), iModes(aiModes) {}

StartMenu::Option::~Option() {}

bool StartMenu::Option::IsForModes(uint32_t aiModes) {
    return ((aiModes & iModes) != 0);
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
    // pTemplate and pValueStrings deliberately stay unset for another type, as the game does.
    if (aeType < UOPT_TOGGLE) {
        return;
    }
    if (aeType <= UOPT_TOGGLE_NOWRAP) {
        pTemplate = "lb_toggle_template";
        // The allocation size is count times the pointer size, saturated to 0xFFFFFFFF on overflow, as the game's
        // operator new[] does.
        uint64_t uiSize = static_cast<uint64_t>(iNumSettings) * sizeof(const char*);
        pValueStrings =
            static_cast<const char**>(ourMalloc(uiSize > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(uiSize)));
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
    uiPaused = (uint32_t)0;
    uiT0 = (uint32_t)0;
}

void StartMenu::IdleTimer::Start() {
    uint32_t result = BSGetTickCount();
    uiT0 = result;
    uiPaused = (uint32_t)0;
}

int32_t StartMenu::IdleTimer::GetStarted() {
    return uiT0 != 0;
}

int32_t StartMenu::IdleTimer::GetPaused() {
    return uiPaused != 0;
}

StartMenu::IdleTimer* StartMenu::IdleTimer::Instance() {
    if (!StartMenu::IdleTimer::pInstance) {
        StartMenu::IdleTimer* pTimer;
        void* pMemory = ourMalloc(sizeof(StartMenu::IdleTimer));
        if (pMemory) {
            pTimer = ::new (pMemory) StartMenu::IdleTimer;
        }
        else {
            pTimer = nullptr;
        }
        StartMenu::IdleTimer::pInstance = pTimer;
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
    uiCurrentControlsDevice(255), unk1CC(nullptr), ENABLED_TRAIT(Tile::AddUserTrait("_enabled", -1)) {
    g_startMenuLastControllerInputTime = 0;
    BSAudio::QInstance().PauseWorldVoices();
}

// The list boxes, spAlphaMap and the Menu base release themselves.
StartMenu::~StartMenu() {
    if (IsFlagSet(StartMenu::FLAG_RESET_HUD_OPACITY)) {
        HUDMainMenu::ResetHUDOpacity(fHudOpacity.operator float());
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
    if ((apTarget) && ((aiID == 100) || (aiID == 101))) {
        apTarget->Set(4085, (-1));
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
                else {
                    if (IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
                        const char* afValue = sYes.String();
                        const char* v6 = sNo.String();
                        Interface::CreateMessageMenu(
                            sResetToDefaults.operator const char*(), static_cast<const char*>(nullptr),
                            static_cast<const char*>(nullptr), &StartMenu::ConfirmResetToDefaults, 0, 18, 0.0f, 0.0f,
                            v6, afValue, nullptr
                        );
                    }
                    else {
                        lbOptions.Traverse(&StartMenu::ResetToDefaults, 0, 0x7FFFFFFF);
                    }
                }
                break;
            }
            case 15: {
                if (lbOptions.IsVisible()) {
                    lbOptions.Traverse(&StartMenu::DeleteOption, 0, 0x7FFFFFFF);
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
            pOptionLBs[0] = (ListBox<StartMenu::Option*>*)&lbMain;
            pOptionLBs[1] = (ListBox<StartMenu::Option*>*)&lbSettings;
            pOptionLBs[2] = (ListBox<StartMenu::Option*>*)&lbConfirm;
            pOptionLBs[3] = (ListBox<StartMenu::Option*>*)&lbOptions;
            for (uint32_t i = 0; i < 4; ++i) {
                StartMenu::Option* popt = pOptionLBs[i]->GetDataFor(apTarget);
                if (popt && popt->pfChooseFn) {
                    if (popt->pfChooseFn != &StartMenu::ChooseActionMap) {
                        pOptionLBs[i]->Select(apTarget);
                    }
                    if (pOptionLBs[i] != &lbConfirm && popt->pfChooseFn != &StartMenu::ChooseBack) {
                        pOptionLBs[i]->SetEnabled(false);
                    }
                    popt->pfChooseFn();
                    g_startMenuLastControllerInputTime = BSGetTickCount();
                    return;
                }
            }
            pSaveGame = lbSaveLoad.GetDataFor(apTarget);
            if (pSaveGame && (IsFlagSet(StartMenu::FLAG_SAVE_MODE) || !pSaveGame->IsCorrupt())) {
                ConfirmSaveLoad();
                goto LABEL_47;
            }
            if (!lbSaveLoad.IsListItem(apTarget) || !IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
                if (lbDownloads.IsListItem(apTarget) && lbDownloads.GetDataFor(apTarget) == -1) {
                    uint32_t afFadeTime = XGamerProfile::Instance()->m_PlayerIndex;
                    BSSystemUtility* v4 = &BSSystemUtility::QInstance();
                    BSDLCSystemUtility* v5 = &v4->QDLCUtility();
                    v5->ShowMarketplace(afFadeTime);
                }
                goto LABEL_47;
            }
            if (lbSaveLoad.NumItems() <= 1000 || !lbSaveLoad.NumItems()) {
                const char* v3;
                v3 = sMenuDisplayShortXBoxSaveMessage.String();
                apTarget->Set(Tile::kTileValue_String, v3, true);
                lbSaveLoad.Select(apTarget);
                pTiles[6]->Set(Tile::kTileValue_User0, 0);
                StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVE_WAIT_MESSAGE, 1);
            LABEL_47:;
                StartMenu::UserOption* puopt =
                    dynamic_cast<StartMenu::UserOption*>(lbOptions.GetDataFor(apTarget->pParent));
                if ((puopt) &&
                    (aiID == 100 ||
                     (aiID == 101 ||
                      (puopt->eType == StartMenu::UserOption::UOPT_TOGGLE ||
                       (puopt->eType == StartMenu::UserOption::UOPT_TOGGLE_NOWRAP &&
                        puopt->iVal < puopt->iNumSettings - 1))))) {
                    puopt->iVal = (2 * (aiID != 100) - 1 + (puopt->iNumSettings + puopt->iVal)) % puopt->iNumSettings;
                    pDirtyOption = puopt;
                    float afValuea = pMenu->GetFloat(Tile::kTileValue_Brightness);
                    apTarget->Set(Tile::kTileValue_Brightness, afValuea, true);
                }
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
    bool v4 = aiID == 100 || aiID == 101;
    if ((apTarget) && (v4)) {
        float afValue = pMenu->GetFloat(4085);
        apTarget->Set(4085, afValue, true);
    }

    if (!InterfaceManager::Has360Controller() && lbSaveLoad.IsListItem(apTarget)) {
        UpdateHighlightedSaveGameInfo();
    }
    if ((!v4) && (pTiles[17])) {
        pTiles[17]->Set(4003, 0);
    }
}

void StartMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    if (aiID == 102) {
        StartMenu::UserOption* puopt = dynamic_cast<StartMenu::UserOption*>(lbOptions.GetHighlightedData());
        if (puopt) {
            Tile* Parent = apTarget->pParent;
            if (Parent == (&lbOptions)->GetHighlightedItem()) {
                int32_t iold = puopt->iVal;
                double Float = apTarget->pParent->GetFloat(Tile::kTileValue_User3);
                InterfaceManager* v4 = InterfaceManager::Instance();
                double CursorX = static_cast<double>(v4->GetCursorX());
                double v6 = (CursorX - apTarget->GetRealX()) * Float;
                float afX = v6 / apTarget->GetFloat(4017);
                puopt->iVal = static_cast<int32_t>(std::ceil(afX));
                if (puopt->iVal >= 0) {
                    if (puopt->iVal > puopt->iNumSettings - 1) {
                        puopt->iVal = puopt->iNumSettings - 1;
                    }
                }
                else {
                    puopt->iVal = 0;
                }
                if (iold != puopt->iVal) {
                    pDirtyOption = puopt;
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
    return IsFlagSet(StartMenu::FLAG_IIS_PREFADE) && (LoadingMenu::pMe && LoadingMenu::pMe->ForceFinish());
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
                    this->pTiles[eControlsHeader]->Set(
                        Tile::kTileValue_String, pDeviceNamesA[this->uiCurrentControlsDevice]->String(), true
                    );
                }
                this->lbOptions.SetEnabled(true);
                this->lbOptions.DeselectAll(true);
                this->lbOptions.Traverse(&StartMenu::SetActionForDevice, 0, 0x7FFFFFFF);
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
                    pText, nullptr, nullptr, nullptr, 1, 0x12, 0.0f, 0.0f, pOk, static_cast<const char*>(nullptr)
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
            if (iID == eRightArrow && this->lbOptions.GetHighlightedData() &&
                this->lbOptions.GetHighlightedData()->pfChooseFn == &StartMenu::ChooseActionMap) {
                this->DoClick(-1, static_cast<Tile*>(pOptions->GetHighlightedItem()));
                // Plays the sound on the arrow even when there is none, as the game does.
                pArrow->PlayTileSound(Tile::kTileValue_ClickSound);
                return true;
            }
            return false;
        }
        case 5:
        case 9:
            if (this->IsFlagSet(FLAG_INGAME) || !this->pTiles[eWelcomeText]->IsTrue(Tile::kTileValue_Visible) ||
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
                static_cast<Tile*>(pOptions->GetHighlightedItem())->GetFloat(Tile::kTileValue_User1), 1.0f
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
        Tile* v4 = lbSaveLoad.AddItem(0, nullptr, nullptr, "lb_newsave_template");
        const char* v1 = sPleaseStandBy.String();
        v4->Set(4036, v1, true);
        v4->Set(ENABLED_TRAIT, 0);
        lbSaveLoad.SetEnabled(true);
        Fade(StartMenu::eSaveLoadContainer, true, Tile::kTileValue_Invalid, -1.0f);
        Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
        const char* v2 = sDelete.String();
        pTiles[4]->Set(4036, v2, true);
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
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Menus\\StartMenu.cpp",
        426
    );
    TES::pInstance->CreateTextureImage(
        "Textures\\Interface\\Main\\screenshot_alpha_frame.dds", this->spAlphaMap, false, false
    );
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

    for (uint32_t i = 0; i < StartMenu::pOptionsA.GetSize(); ++i) {
        Tile* pTile = nullptr;
        const bool bDisabled = (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseSave &&
                                !pSaveLoadManager->IsSavingAllowed(false)) ||
            (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseContinue &&
             (abInGame ? abForceReload : !bSavesExist)) ||
            (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseLoad && !bSavesExist) ||
            (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseHelp &&
             Menu::GetMenuByClass(TutorialMenu::GetClassStatic()));
        if ((abInGame && StartMenu::pOptionsA.GetAt(i)->IsForModes(MODE_PAUSE)) ||
            (!abInGame && StartMenu::pOptionsA.GetAt(i)->IsForModes(MODE_MAIN))) {
            pTile = this->lbMain.AddItem(StartMenu::pOptionsA.GetAt(i), nullptr, nullptr, nullptr);
        }
        else if (StartMenu::pOptionsA.GetAt(i)->IsForModes(MODE_SETTINGS)) {
            pTile = this->lbSettings.AddItem(StartMenu::pOptionsA.GetAt(i), nullptr, nullptr, nullptr);
        }
        if (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseDownloads) {
            this->pDownloadsOption = pTile;
        }
        else if (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseContinue) {
            this->pContinueOption = pTile;
        }
        else if (StartMenu::pOptionsA.GetAt(i)->pfChooseFn == &StartMenu::ChooseLoad) {
            this->pLoadOption = pTile;
        }
        if (pTile) {
            pTile->Set(kTileValue_String, StartMenu::pOptionsA.GetAt(i)->pDisplayText, true);
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
        StartMenu::pMe->SetFlag(static_cast<StartMenu::Flag>(0x40000000), 1);
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
    for (uint32_t uiIndex = 0; uiIndex < StartMenu::pUserOptionsA.iSize; ++uiIndex) {
        if (!StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->IsForModes(auiModes)) {
            continue;
        }
        // The controller and rumble options are left out when XInputGetState fails (no controller in port 0).
        if ((StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->pfChangeFn == &SetXboxController ||
             StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->pfChangeFn == &StartMenu::SetRumble) &&
            XInputGetState(0, &pState)) {
            unk1CC = nullptr;
            continue;
        }
        const char* pTemplate = StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->pTemplate;
        const char* pDisplayText = StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->pDisplayText;
        Tile* pItem = lbOptions.AddItem(
            StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex)), pDisplayText, nullptr, pTemplate
        );
        if (StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->pfChangeFn == &StartMenu::SetRumble) {
            unk1CC = pItem;
            bool bHas360Controller = InterfaceManager::Has360Controller();
            pItem->Set(static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT), static_cast<int32_t>(bHas360Controller));
        }
        if (StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->pfChangeFn == &SettingsMenu_ToggleHardcore) {
            bool bEnabled = !StartMenu::MainMenuIsActive() &&
                (pPlayer && !pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY));
            pItem->Set(Tile::kTileValue_Target, static_cast<int32_t>(bEnabled));
            pItem->Set(static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT), static_cast<int32_t>(bEnabled));
            NiTListItem<Tile*>* pPos = pItem->xChildren.m_pkHead;
            NiTPointerList<Tile*>* pChildren = &pItem->xChildren;
            for (; pPos && (pChildren && pChildren->GetHead()); pPos = pChildren->GetNextPos(pPos)) {
                pChildren->Get(pPos)->Set(
                    static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT), static_cast<int32_t>(bEnabled)
                );
                pChildren->Get(pPos)->Set(Tile::kTileValue_Target, static_cast<int32_t>(bEnabled));
            }
        }
        if (StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->eType == StartMenu::UserOption::UOPT_METER) {
            uint32_t uiMaxValue = StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->iNumSettings - 1;
            pItem->Set(Tile::kTileValue_User3, uiMaxValue);
        }
        if (StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->iArrowSpacing) {
            uint32_t uiArrowSpacing = StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex))->iArrowSpacing;
            pItem->Set(Tile::kTileValue_User4, uiArrowSpacing);
        }
        DisplayOption(StartMenu::pUserOptionsA.GetAt(static_cast<int32_t>(uiIndex)), pItem);
    }
    for (uint32_t j = 0; j < StartMenu::pOptionsA.iSize; ++j) {
        if (StartMenu::pOptionsA.GetAt(static_cast<int32_t>(j))->IsForModes(auiModes)) {
            Tile* pItem = lbOptions.AddItem(
                StartMenu::pOptionsA.GetAt(static_cast<int32_t>(j)),
                StartMenu::pOptionsA.GetAt(static_cast<int32_t>(j))->pDisplayText, nullptr, "lb_meter_template"
            );
            pItem->SetName(StartMenu::pOptionsA.GetAt(static_cast<int32_t>(j))->pDisplayText);
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
            Tile::kTileValue_User0, static_cast<int32_t>(!InterfaceManager::Has360Controller())
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
                        bMapped =
                            appMain->GetControls()->SetControl(uiAction, 0, static_cast<uint8_t>(iKey)) || iKey == 1;
                        InterfaceManager::PlayMenuSound(MENU_SOUND_MESSAGE);
                        if (!bMapped) {
                            const char* pOk = static_cast<const char*>(sOk);
                            const char* pText = sKeyLocked.String();
                            Interface::CreateMessageMenu(
                                pText, nullptr, nullptr, nullptr, 1, 0x12, 0.0f, 0.0f, pOk,
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
                    bMapped = iDirection != -1 && BSGetTickCount() >= uiNextHatTime &&
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
            this->lbOptions.Traverse(&StartMenu::SetActionForDevice, 0, 0x7FFFFFFF);
            this->pTiles[eControlsHeader]->Set(
                Tile::kTileValue_String, pDeviceNamesA[this->uiCurrentControlsDevice]->String(), true
            );
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
                pText, nullptr, nullptr, nullptr, 1, 0x12, 0.0f, 0.0f, pOk, static_cast<const char*>(nullptr)
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
                pText, nullptr, nullptr, &StartMenu::FailedSaveCallbackFunction, 1, 0, 0.0f, 0.0f, pOk,
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
                const int32_t iStart =
                    static_cast<int32_t>(static_cast<double>(this->lbSaveLoad.NumVisible()) + this->fSaveLoadThreshold);
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
              (eAction == Controls::UACTN_ACTIVATE ||
               (eAction == Controls::UACTN_USE ||
                (eAction == Controls::UACTN_READY_RELOAD_WEAPON ||
                 (eAction == Controls::UACTN_CROUCH ||
                  (eAction == Controls::UACTN_JUMP ||
                   (eAction == Controls::UACTN_TOGGLEPOV ||
                    (eAction == Controls::UACTN_RESTMENU ||
                     (eAction == Controls::UACTN_BLOCK ||
                      (eAction == Controls::UACTN_VATSMENU || eAction == Controls::UACTN_GRAB))))))))))) ||
            (uiCurrentControlsDevice != 3 && Controls::Remappable(eAction))) {
            StartMenu::Option* poption;
            StartMenu::Option* apMem = static_cast<StartMenu::Option*>(ourMalloc(sizeof(StartMenu::Option)));
            if (apMem) {
                uint32_t v4 = eAction;
                const char* v1 = psUserActions[eAction]->String();
                poption = ::new (apMem) StartMenu::Option(v1, &StartMenu::ChooseUserAction, v4);
            }
            else {
                poption = nullptr;
            }
            Tile* plbitem = lbOptions.AddItem(poption, poption->pDisplayText, nullptr, "lb_item_template");
            const char* v2 = psUserActions[eAction]->String();
            plbitem->SetName(v2);
        }
    }
    lbOptions.Traverse(&StartMenu::SetActionForDevice, 0, 0x7FFFFFFF);
    if (!InterfaceManager::Has360Controller()) {
        const char* v3 = pDeviceNamesA[uiCurrentControlsDevice]->String();
        pTiles[eControlsHeader]->Set(Tile::kTileValue_String, v3, true);
        pTiles[eControlsHeader]->Set(Tile::kTileValue_Visible, 1);
    }
}

void StartMenu::PopulateSaveLoadListBox() {
    if (LoadingMenu::pMe) {
        LoadingMenu::ResumeBackgroundThread();
    }
    pSaveLoadManager->SetupSavesList();
    for (BSSimpleList<BGSSaveLoadFileEntry*>* pnode = pSaveLoadManager->pSaveGameList; pnode && pnode->m_item;
         pnode = pnode->m_pkNext) {
        BGSSaveLoadFileEntry** v1 = &pnode->m_item;
        lbSaveLoad.AddItem(*v1, nullptr, nullptr, nullptr);
    }
}

void StartMenu::PopulateConfirmationListBox(uint32_t auiModes, uint8_t aucFlags) {
    uint32_t iwidth = (aucFlags & 1) != 0 ? 168 : 80;
    double v5 = static_cast<double>(static_cast<int32_t>(pTiles[0]->GetRealX()) - iwidth);
    int32_t ix = static_cast<int32_t>(v5 - InterfaceManager::GetScreenSafeX());
    Tile* v3 = static_cast<Tile*>((&lbMain)->GetHighlightedItem());
    int32_t v4 = static_cast<int32_t>(v3->GetRealY()) - 60;
    PopulateConfirmationListBox_0(auiModes, ix, v4, iwidth, (aucFlags & 2) != 0);
}

void StartMenu::PopulateConfirmationListBox(uint32_t auiModes, int32_t aiRealX, int32_t aiRealY, uint8_t aucFlags) {
    const uint32_t uiTileValue = (aucFlags & 1) ? 0xA8 : 0x55;
    this->PopulateConfirmationListBox_0(auiModes, aiRealX, aiRealY, uiTileValue, (aucFlags & 2) != 0);
}

void StartMenu::HideOptionsContainer() {
    this->pTiles[eVersionText]->Set(Tile::kTileValue_Visible, 0);
    this->lbOptions.SetEnabled(false);
    this->Fade(eOptionsContainer, false, static_cast<int32_t>(0x80000000), -1.0f);
    if (this->IsFlagSet(FLAG_ACTION_MAPPING)) {
        this->lbOptions.Traverse(&StartMenu::DeleteOption, 0, 0x7FFFFFFF);
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
    uint32_t result;
    ListBox<StartMenu::Option*>* pConfirm = &lbConfirm;
    pConfirm->RemoveAll();
    lbConfirm.SetEnabled(true);
    Fade(StartMenu::eConfirmContainer, true, static_cast<int32_t>(0x80000000), -1.0f);
    float afValue = static_cast<double>(aiX) - pTiles[eConfirmContainer]->pParent->GetRealX();
    pTiles[eConfirmContainer]->Set(Tile::kTileValue_X, afValue, true);
    float v7 = static_cast<double>(aiY) - pTiles[eConfirmContainer]->pParent->GetRealY();
    pTiles[eConfirmContainer]->Set(Tile::kTileValue_Y, v7, true);
    pTiles[eConfirmContainer]->Set(Tile::kTileValue_Width, aiTileValue);
    // Passes the raw byte of abVisible; read as bool, it would be reduced to 0 or 1.
    pTiles[eConfirmWarning]->Set(Tile::kTileValue_Visible, static_cast<int32_t>(static_cast<uint8_t>(abVisible)));
    for (int32_t i = 0;; ++i) {
        result = StartMenu::pOptionsA.iSize;
        if (i >= result) {
            break;
        }
        if (StartMenu::pOptionsA.GetAt(i)->IsForModes(auiModes)) {
            Tile* plbitem = lbConfirm.AddItem(StartMenu::pOptionsA.GetAt(i), nullptr, nullptr, nullptr);
            plbitem->Set(Tile::kTileValue_String, StartMenu::pOptionsA.GetAt(i)->pDisplayText, true);
            plbitem->SetName(StartMenu::pOptionsA.GetAt(i)->pDisplayText);
        }
    }
    return result;
}

void StartMenu::FinishPrepSaveLoad() {
    if (IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
        uint32_t numitems = lbSaveLoad.NumItems();
        if (numitems) {
            uint32_t ENABLED_TRAIT = this->ENABLED_TRAIT;
            Tile* FirstItem = lbSaveLoad.GetFirstItem();
            FirstItem->Set(ENABLED_TRAIT, 1);
        }
        for (int32_t i = lbSaveLoad.NumItems() - 1; i >= 0; --i) {
            // GetTileAt, Highlight and ScrollToHighlight are called through the list's vtable; a call on the member
            // itself would be devirtualised.
            Tile* tTile = (&lbSaveLoad)->GetTileAt(static_cast<uint32_t>(i), false);
            BGSSaveLoadFileEntry* tSave = lbSaveLoad.GetDataAt(i, false);
            if ((tTile) && (tSave)) {
                if (tSave->IsAutosave()) {
                    lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                    --numitems;
                }
                else {
                    if (tSave->IsSystemSave()) {
                        lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                        --numitems;
                    }
                }
            }
        }
        if (numitems >= 1000) {
            Tile* v2 = lbSaveLoad.GetFirstItem();
            v2->AddNeedsUpdate(8);
            Tile* v3 = (&lbSaveLoad)->GetTileAt(1u, false);
            (&lbSaveLoad)->Highlight(v3);
            (&lbSaveLoad)->ScrollToHighlight();
        }
        if (numitems) {
            char Src[68];
            const char* afValue = sSavesRemaining.String();
            BSsprintf(Src, 64, "%s %u/%i %s", sNewSave.String(), numitems - 1, 1000, afValue);
            Tile* v4 = lbSaveLoad.GetFirstItem();
            v4->Set(4036, Src, true);
        }
    }
    else {
        if (lbSaveLoad.GetFirstItem()) {
            const char* v13 = sNoSaves.String();
            Tile* v5 = lbSaveLoad.GetFirstItem();
            v5->Set(4036, v13, true);
            uint32_t v14 = this->ENABLED_TRAIT;
            Tile* v6 = lbSaveLoad.GetFirstItem();
            v6->Set(v14, 0);
        }
    }
    lbSaveLoad.SetEnabled(true);
    lbSaveLoad.DeselectAll(true);
    lbSaveLoad.Filter(&StartMenu::SaveLoadFilterFn);
    if (InterfaceManager::Has360Controller()) {
        Tile* v11 = lbSaveLoad.GetFirstItem();
        InterfaceManager* v7 = InterfaceManager::Instance();
        v7->SetCurrentFocusTarget(v11, 4035, true);
        int32_t v8 = Tile::AddUserTrait("_current_value", -1);
        lbSaveLoad.pScrollBar->Set(v8, 0.0f, false);
        int32_t v9 = Tile::AddUserTrait("_SetInCode ", -1);
        lbSaveLoad.pScrollBar->Poke(v9, 1.0f);
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
        const char* v10 = sDelete.String();
        pTiles[4]->Set(4036, v10, true);
    }
}

void StartMenu::DisplayOption(StartMenu::UserOption* apOption, Tile* apTile) {
    StartMenu::UserOption* aData;
    if ((apOption) && (!apTile)) {
        aData = apOption;
        apTile = lbOptions.GetTileFor(aData);
    }

    if (apOption && apTile) {
        if (apOption->eType == StartMenu::UserOption::UOPT_METER) {
            apTile->Set(Tile::kTileValue_User0, apOption->iVal);
        }
        else {
            if ((apOption->pValueStrings) && (apOption->iVal >= 0 && apOption->iVal <= apOption->iNumSettings - 1)) {
                apTile->Set(Tile::kTileValue_User0, apOption->pValueStrings[apOption->iVal], true);
            }
        }
        if (apOption->eType == StartMenu::UserOption::UOPT_TOGGLE) {
            goto LABEL_18;
        }
        if (!apOption->iVal) {
            apTile->Set(Tile::kTileValue_User2, 2);
            return;
        }
        if (apOption->iVal == apOption->iNumSettings - 1) {
            apTile->Set(Tile::kTileValue_User2, 0);
        }
        else {
        LABEL_18:;
            apTile->Set(Tile::kTileValue_User2, 1);
        }
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
        NiSourceTexture* pAlphaMap = this->spAlphaMap.operator->();
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
    Tile* v1 = static_cast<Tile*>((&lbSaveLoad)->GetHighlightedItem());
    lbSaveLoad.Deselect(v1);
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
        bool bscreenshotvis = false;
        bool v14 = false;
        uint8_t v15 = 0;
        uint8_t v16 = 0;
        uint8_t v17 = 0;
        BGSSaveLoadFileEntry* phighlightedSaveGame = lbSaveLoad.GetHighlightedData();
        if (phighlightedSaveGame) {
            if (InterfaceManager::Has360Controller() && bFadeInScreenShot.operator bool()) {
                StartMenu::IdleTimer* v1 = StartMenu::IdleTimer::Instance();
                v1->Start();
                bscreenshotvis = true;
                pTiles[11]->Set(4009, 0.0f, true);
            }
            else {
                bscreenshotvis = SetSaveGameScreenshot(phighlightedSaveGame);
            }
            if (phighlightedSaveGame->IsCorrupt()) {
                const char* v2 = sSaveGameCorruptMenuMessage.String();
                pTiles[12]->Set(4036, v2, true);
                v14 = true;
                v15 = 0;
                v16 = 0;
                v17 = 0;
            }
            else {
                uint8_t v11[4];
                const char* pplayerName = phighlightedSaveGame->GetPlayerName();
                v11[0] = 0;
                v11[1] = 0;
                const char* v3 = sLanguage.String();
                if (!BSstrcmp(v3, "FRENCH")) {
                    v11[0] = 32;
                }
                pTiles[12]->Set(4036, pplayerName, true);
                v14 = pplayerName != nullptr;
                BSStringT<char> v13;
                const char* PlayerTitle = phighlightedSaveGame->GetPlayerTitle();
                uint32_t PlayerLevel = phighlightedSaveGame->GetPlayerLevel();
                v13.SPrintF("%s %d%s: %s", sLevelAbbrev.String(), PlayerLevel, v11, PlayerTitle);
                pTiles[18]->Set(4036, v13.pString, true);
                v15 = 1;
                const char* PlayTime = phighlightedSaveGame->GetPlayTime();
                v13.SPrintF("%s%s: %s", sPlayTime.String(), v11, PlayTime);
                pTiles[19]->Set(4036, v13.pString, true);
                v16 = 1;
                v13.SPrintF("%s%s: %s", sVersionNumber.String(), v11, &phighlightedSaveGame->pcLanguage);
                pTiles[22]->Set(4036, v13.pString, true);
                v17 = 1;
            }
        }
        else {
            if (InterfaceManager::Has360Controller()) {
                StartMenu::IdleTimer* v4 = StartMenu::IdleTimer::Instance();
                v4->Stop();
            }
        }
        int32_t v9 = phighlightedSaveGame != nullptr;
        int32_t v5 = Tile::AddUserTrait("_target", -1);
        pTiles[4]->Set(v5, v9);
        pTiles[11]->Set(4003, static_cast<int32_t>(bscreenshotvis));
        pTiles[12]->Set(4003, static_cast<int32_t>(v14));
        pTiles[18]->Set(4003, static_cast<int32_t>(v15));
        pTiles[19]->Set(4003, static_cast<int32_t>(v16));
        pTiles[22]->Set(4003, static_cast<int32_t>(v17));
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
    uint32_t imode;
    const char* ptext;
    uint8_t ucflags = 0;
    if (IsFlagSet(StartMenu::FLAG_DELETE_MODE)) {
        imode = 2048;
        ptext = sConfirmDelete.String();
    }
    else {
        if (IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
            imode = 1024;
            ptext = sConfirmSave.String();
        }
        else {
            imode = 512;
            ptext = sConfirmLoad.String();
            if (IsFlagSet(StartMenu::FLAG_INGAME)) {
                ucflags = 2;
            }
        }
    }
    // GetHighlightedItem is called through the list's vtable each time.
    if ((&lbSaveLoad)->GetHighlightedItem()) {
        Tile* v1 = static_cast<Tile*>((&lbSaveLoad)->GetHighlightedItem());
        lbSaveLoad.Select(v1);
        lbSaveLoad.SetEnabled(false);
        pTiles[6]->Set(Tile::kTileValue_User0, 0);
        pTiles[8]->Set(4036, ptext, true);
        pTiles[8]->UpdateTile(false);
        Tile* v2 = static_cast<Tile*>((&lbSaveLoad)->GetHighlightedItem());
        double RealY = v2->GetRealY();
        Tile* v3 = static_cast<Tile*>((&lbSaveLoad)->GetHighlightedItem());
        int32_t v6 = static_cast<int32_t>(v3->GetFloat(4016) * static_cast<double>(((ucflags & 2) != 0) + 1) + RealY);
        double RealX = pTiles[6]->GetRealX();
        double v9 = RealX - pTiles[8]->GetFloat(4001);
        double v8 = pTiles[8]->GetFloat(4017) + v9;
        double v7 = lbSaveLoad.pScrollBar->GetFloat(4017) + v8;
        int32_t v4 = Tile::AddUserTrait("_text_offset", -1);
        double v5 = pTiles[6]->GetFloat(v4) + v7;
        PopulateConfirmationListBox(imode, static_cast<int32_t>(v5), v6, ucflags);
    }
}

void StartMenu::ShowDLCList() {
    if (!lbDownloads.NumItems()) {
        const char* v1 = sOldDownloadsAvailable.String();
        lbDownloads.AddItem(-1, v1, nullptr, nullptr);
    }
    lbDownloads.SetEnabled(true);
    Fade(StartMenu::eDLCContainer, true, Tile::kTileValue_Invalid, -1.0f);
    Fade(StartMenu::eBackButton, true, Tile::kTileValue_Invalid, -1.0f);
}

void StartMenu::Music() {
    if (!IsFlagSet(StartMenu::FLAG_INGAME)) {
        if (FalloutAudioMedia::MediaIsPlaying(MEDIA_MAINMENU, true)) {
            char apDest[68];
            ourMemset(apDest, 0, 64u);
            BSsprintf(apDest, 64u, "Data\\Music\\Special\\MainTitle.mp3");
            FalloutAudio::SetMainMenuMusic(true);
            FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, apDest, 1000u, true, true, 0.0f, 0u);
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
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Menus\\StartMenu.cpp",
        337
    );
    g_startMenuLastControllerInputTime = BSGetTickCount();
    if (abInGame) {
        uiOpenTime = BSGetTickCount();
    }
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
    uint8_t result;
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
            result = 1;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return (bool)result;
}

void StartMenu::SetFlag20000000(uint8_t abOn) {
    if (StartMenu::pMe) {
        StartMenu::pMe->SetFlag(FLAG_20000000, abOn);
    }
}

bool StartMenu::PauseMenuIsActive() {
    return StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_INGAME);
}

bool StartMenu::MainMenuIsActive() {
    return StartMenu::pMe && !StartMenu::pMe->IsFlagSet(FLAG_INGAME);
}

bool StartMenu::IsStartMenuSaving() {
    return (bool)((uint8_t)(StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_WAIT_SAVING) &&
                            StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_MODE)));
}

bool StartMenu::CreditsMenuIsActive() {
    return StartMenu::pMe && StartMenu::pMe->IsFlagSet(FLAG_SHOWING_CREDITS);
}

bool StartMenu::IsInitialLoading() {
    return (bool)((uint8_t)LoadingMenu::bInitialLoad);
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
    NiTListItem<Tile*>* piter;
    if ((StartMenu::PauseMenuIsActive()) && (Interface::GetCurrentRenderedMenu())) {
        FOPipboyManager* CurrentRenderedMenu = static_cast<FOPipboyManager*>(Interface::GetCurrentRenderedMenu());
        if (CurrentRenderedMenu == Interface::GetPipboy()) {
            FOPipboyManager* Pipboy = Interface::GetPipboy();
            Pipboy->ResetColors();
        }
    }

    if ((StartMenu::pMe) && (!StartMenu::pMe->WaitingForControlRemapButtonPress())) {
        if (Interface::InDialog()) {
            Interface::SetDialogPause(false);
        }
        if (!StartMenu::pMe->IsFlagSet(StartMenu::FLAG_FORCE_RELOAD)) {
            if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
                StartMenu::pMe->lbOptions.Traverse(&StartMenu::DeleteOption, 0, 0x7FFFFFFF);
            }
            if (StartMenu::PauseMenuIsActive()) {
                MessageMenu::SetPauseMenuMode(false);
            }
            for (piter = StartMenu::xHiddenTiles.m_pkHead; piter;
                 StartMenu::xHiddenTiles.GetNext(piter)->Set(4003, 1)) {
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
    if ((StartMenu::pMe) && (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING))) {
        const char* v0 = sPressControl.String();
        StartMenu::pMe->pTiles[16]->Set(4036, v0, true);
        StartMenu::pMe->pTiles[16]->Set(4003, 1);
        if (InterfaceManager::Has360Controller() && StartMenu::pMe->uiCurrentControlsDevice == 3) {
            StartMenu::pMe->pTiles[5]->Set(4015, 0);
        }
    }
}

void StartMenu::LoadDownloadedContent() {
    uint32_t pdevice = 0;
    BSSystemUtility* v0 = &BSSystemUtility::QInstance();
    BSDLCSystemUtility* dlc = &v0->QDLCUtility();
    dlc->LoadDLC(0, 0xFFFFFFFFu);
    BSSystemUtility* v2 = &BSSystemUtility::QInstance();
    BSDLCSystemUtility* dlc_2 = &v2->QDLCUtility();
    dlc_2->MountDLC(0, nullptr);
    if (pDataHandler->AddDownloadedContentToFileList()) {
        bool ballowChanges = pSaveLoadGame->SetGlobalAllowChanges(false);
        pDataHandler->CompileFiles(true);
        pDataHandler->FixAllScriptIDs();
        pSaveLoadGame->SetGlobalAllowChanges(ballowChanges);
        ReferenceProcessLists.InitPackageLocations();
        pDataHandler->UpdateAutoCalcNPCs();
    }
    pIdleManager->InitRootIdleArrays();
    MessageMenu::ClearMessageQueue();
    Interface::CloseMessageMenu();
    BSSystemUtility* v4 = &BSSystemUtility::QInstance();
    BSDLCSystemUtility* dlc_3 = &v4->QDLCUtility();
    if (!(&dlc_3->m_corruptDLC)->IsEmpty()) {
        StartMenu::pMe->SetFlag(StartMenu::FLAG_DLC_CORRUPT_MESSAGE, 1);
    }
    ListBox<int32_t>* pDownloads = &StartMenu::pMe->lbDownloads;
    pDownloads->RemoveAll();
    const char* v5 = sOldDownloadsAvailable.String();
    StartMenu::pMe->lbDownloads.AddItem(-1, v5, nullptr, nullptr);
    BSContentPackage* v12 = nullptr;
    for (BSContentPackage* package : *&dlc_3->m_validDLC) {
        if (package) {
            char Dest[264];
            const char16_t* Source = package->DisplayName();
            rt::wcstombs(Dest, Source, 260u);
            char* apcItemString = Dest;
            StartMenu::pMe->lbDownloads.AddItem(0, Dest, nullptr, nullptr);
        }
    }
}

void StartMenu::SaveSettings() {
    char (*ActorPackageThatIsRunning)[260] = &SettingT<RendererSettingCollection>::GetCollection()->pSettingFile;
    RendererSettingCollection* Collection = SettingT<RendererSettingCollection>::GetCollection();
    Collection->SaveAll(reinterpret_cast<char*>(ActorPackageThatIsRunning));
    char (*v6)[260] = &SettingT<INIPrefSettingCollection>::GetCollection()->pSettingFile;
    INIPrefSettingCollection* v1 = SettingT<INIPrefSettingCollection>::GetCollection();
    if (v1->SaveAll(reinterpret_cast<char*>(v6))) {
        Controls* Controls_ = appMain->GetControls();
        Controls_->SaveControls();
        if ((StartMenu::pMe) && (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_RESTART_REQUIRED))) {
            const char* v3 = sOk.operator const char*();
            Interface::CreateMessageMenu(
                sMustRestart.operator const char*(), static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr), static_cast<void (*)()>(nullptr), 0, 18, 0.0f, 0.0f, v3, nullptr
            );
            StartMenu::pMe->SetFlag(StartMenu::FLAG_RESTART_REQUIRED, 0);
        }
    }
    else {
        const char* v4 = sOk.operator const char*();
        Interface::CreateMessageMenu(
            sMenuDisplayFailSaveSettings.operator const char*(), static_cast<const char*>(nullptr),
            static_cast<const char*>(nullptr), static_cast<void (*)()>(nullptr), 0, 18, 0.0f, 0.0f, v4, nullptr
        );
    }
}

// Builds the main, pause, confirmation and settings options. Each toggle's value strings are read before its label,
// last first, as the game evaluates the variadic call's arguments.
void StartMenu::CreateMenuElements() {
    if (!g_strBlue.pString) {
        g_strBlue.Set(sBlue.String(), 0);
        g_strBlue.MakeUpper();
        g_strGreen.Set(sGreen.String(), 0);
        g_strGreen.MakeUpper();
    }
    StartMenu::DestroyMenuElements();
    void* pMem;
    Option* pOption;
    UserOption* pUserOption;
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem)
            Option(static_cast<const char*>(sContinue), &StartMenu::ChooseContinue, MODE_MAIN | MODE_PAUSE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sNew), &StartMenu::ChooseNew, MODE_MAIN);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sSave), &StartMenu::ChooseSave, MODE_PAUSE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sLoad), &StartMenu::ChooseLoad, MODE_MAIN | MODE_PAUSE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem)
            Option(static_cast<const char*>(sSettings), &StartMenu::ChooseSettings, MODE_MAIN | MODE_PAUSE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sHelp), &StartMenu::ChooseHelp, MODE_PAUSE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sCrew), &MenuButton_Credits, MODE_MAIN);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sDownloads), &StartMenu::ChooseDownloads, MODE_MAIN);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sQuit), &StartMenu::ChooseQuit, MODE_MAIN | MODE_PAUSE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sGameplay), &StartMenu::ChooseGameplay, MODE_SETTINGS);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sDisplay), &MenuButton_Display, MODE_SETTINGS);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sAudio), &MenuButton_Audio, MODE_SETTINGS);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sControls), &StartMenu::ChooseControls, MODE_SETTINGS);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sYes), &StartMenu::ConfirmNewGame, MODE_CFM_NEW);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sYes), &StartMenu::ConfirmContinue, MODE_CFM_CONT);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sYes), &StartMenu::ConfirmLoadGame, MODE_CFM_LOAD);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sYes), &StartMenu::ConfirmSaveGame, MODE_CFM_SAVE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sYes), &StartMenu::ConfirmDeleteGm, MODE_CFM_DELETE);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(
            static_cast<const char*>(sNo), &StartMenu::ChooseBack,
            MODE_CFM_NEW | MODE_CFM_CONT | MODE_CFM_LOAD | MODE_CFM_SAVE | MODE_CFM_DELETE
        );
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(static_cast<const char*>(sMainMenu), &StartMenu::ChooseMainMenu, MODE_CFM_QUITP);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem) Option(
            static_cast<const char*>(sExitGameAffirm), &StartMenu::ChooseExitGame, MODE_CFM_QUITM | MODE_CFM_QUITP
        );
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption = ::new (pMem)
            Option(static_cast<const char*>(sCancel), &StartMenu::ChooseBack, MODE_CFM_QUITM | MODE_CFM_QUITP);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(Option));
    pOption = nullptr;
    if (pMem) {
        pOption =
            ::new (pMem) Option(static_cast<const char*>(sActionMapping), &StartMenu::ChooseActionMap, MODE_CONTROLS);
    }
    StartMenu::pOptionsA.Add(pOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pCinematic = sKillCamModeCinematic.String();
        const char* pPlayerText = sKillCamModePlayer.String();
        const char* pNone = sKillCamModeNone.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sKillCamMode), &StartMenu::SetKillCam, MODE_GAMEPLAY, UserOption::UOPT_TOGGLE, 3,
            0x55, pNone, pPlayerText, pCinematic
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sHardcore), &SettingsMenu_ToggleHardcore, MODE_GAMEPLAY, UserOption::UOPT_TOGGLE,
            2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pVeryHard = sLockLevelNameVeryHard.String();
        const char* pHard = sLockLevelNameHard.String();
        const char* pNormal = sNormal.String();
        const char* pEasy = sLockLevelNameEasy.String();
        const char* pVeryEasy = sLockLevelNameVeryEasy.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sDifficulty), &StartMenu::SetDifficulty, MODE_GAMEPLAY, UserOption::UOPT_TOGGLE, 5,
            0x55, pVeryEasy, pEasy, pNormal, pHard, pVeryHard
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sSaveOnRest), &StartMenu::SetSaveOnRest, MODE_GAMEPLAY, UserOption::UOPT_TOGGLE, 2,
            0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sSaveOnWait), &StartMenu::SetSaveOnWait, MODE_GAMEPLAY, UserOption::UOPT_TOGGLE, 2,
            0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sSaveOnTravel), &StartMenu::SetSaveOnTravel, MODE_GAMEPLAY,
            UserOption::UOPT_TOGGLE, 2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sTrueIronSights), &StartMenu::SetTrueIronSights, MODE_GAMEPLAY,
            UserOption::UOPT_TOGGLE, 2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sBrightness), &StartMenu::SetBrightness, MODE_DISPLAY, UserOption::UOPT_METER, 16,
            0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sHUDOpacity), &StartMenu::SetHUDOpacity, MODE_DISPLAY, UserOption::UOPT_METER, 16,
            0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pWhite = sWhite.String();
        const char* pAmber = sAmber.String();
        const char* pBlue = g_strBlue.pString;
        const char* pGreen = g_strGreen.pString;
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sHUDColor), &StartMenu::SetHUDColor, MODE_DISPLAY, UserOption::UOPT_TOGGLE, 4,
            0x37, pGreen, pBlue, pAmber, pWhite
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pWhite = sWhite.String();
        const char* pAmber = sAmber.String();
        const char* pBlue = g_strBlue.pString;
        const char* pGreen = g_strGreen.pString;
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sPipboyColor), &StartMenu::SetPipboyColor, MODE_DISPLAY, UserOption::UOPT_TOGGLE,
            4, 0x37, pGreen, pBlue, pAmber, pWhite
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pSmall = sSmall.String();
        const char* pMedium = sMedium.String();
        const char* pLarge = sLarge.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sTextureSize), &SetTextureSize, MODE_DISPLAY, UserOption::UOPT_TOGGLE, 3, 0x37,
            pLarge, pMedium, pSmall
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sActorFade), &SetActorFade, MODE_DISPLAY, UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sItemFade), &StartMenu::SetItemFade, MODE_DISPLAY, UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sObjectFade), &StartMenu::SetObjectFade, MODE_DISPLAY, UserOption::UOPT_METER, 16,
            0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sGrassFade), &StartMenu::SetGrassFade, MODE_DISPLAY, UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sShadowFade), &StartMenu::SetShadowFade, MODE_DISPLAY, UserOption::UOPT_METER, 16,
            0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sLightFade), &StartMenu::SetLightFade, MODE_DISPLAY, UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sSpecularityFade), &StartMenu::SetSpecularityFade, MODE_DISPLAY,
            UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sObjectLODFade), &StartMenu::SetObjectLODFade, MODE_DISPLAY,
            UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sTreeLODFade), &StartMenu::SetTreeLODFade, MODE_DISPLAY, UserOption::UOPT_METER,
            16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sCrosshair), &StartMenu::SetCrosshair, MODE_DISPLAY, UserOption::UOPT_TOGGLE, 2, 0,
            pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sDialogSubtitles), &StartMenu::SetDialogSubtitles, MODE_DISPLAY,
            UserOption::UOPT_TOGGLE, 2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sGeneralSubtitles), &StartMenu::SetGeneralSubtitles, MODE_DISPLAY,
            UserOption::UOPT_TOGGLE, 2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sMasterVolume), &StartMenu::SetMasterVol, MODE_AUDIO, UserOption::UOPT_METER, 26, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sMusicVolume), &StartMenu::SetMusicVol, MODE_AUDIO, UserOption::UOPT_METER, 26, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sFootstepsVolume), &StartMenu::SetFootstepVol, MODE_AUDIO, UserOption::UOPT_METER,
            26, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sVoiceVolume), &StartMenu::SetVoiceVol, MODE_AUDIO, UserOption::UOPT_METER, 26, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sEffectsVolume), &StartMenu::SetFXVol, MODE_AUDIO, UserOption::UOPT_METER, 26, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sRadioVolume), &StartMenu::SetRadioVol, MODE_AUDIO, UserOption::UOPT_METER, 26, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sCameraPitch), &StartMenu::SetCameraPitch, MODE_CONTROLS, UserOption::UOPT_TOGGLE,
            2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sMouseSensitivity), &StartMenu::SetMouseSensitivity, MODE_CONTROLS,
            UserOption::UOPT_METER, 16, 0
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sDisableXBoxController), &SetXboxController, MODE_CONTROLS,
            UserOption::UOPT_TOGGLE, 2, 0, pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
    pMem = ourMalloc(sizeof(UserOption));
    pUserOption = nullptr;
    if (pMem) {
        const char* pOn = sOn.String();
        const char* pOff = sOff.String();
        pUserOption = ::new (pMem) UserOption(
            static_cast<const char*>(sRumble), &StartMenu::SetRumble, MODE_CONTROLS, UserOption::UOPT_TOGGLE, 2, 0,
            pOff, pOn
        );
    }
    StartMenu::pUserOptionsA.Add(pUserOption);
}

void StartMenu::DestroyMenuElements() {
    for (uint32_t i = 0; i < StartMenu::pOptionsA.GetSize(); ++i) {
        delete StartMenu::pOptionsA.GetAt(i);
    }
    StartMenu::pOptionsA.Clear(true);
    for (uint32_t i = 0; i < StartMenu::pUserOptionsA.GetSize(); ++i) {
        delete StartMenu::pUserOptionsA.GetAt(i);
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
        float* pOpacity = &fHudOpacity.Float();
        float fValue = static_cast<float>(static_cast<double>(apUOpt->iNumSettings - 1) * *pOpacity);
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
        BSShaderManager::fShadowLODEnd =
            static_cast<float>(static_cast<double>(fStart) + Renderer::fShadowLODRange.Float());
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
        BSShaderManager::fLightLODEnd =
            static_cast<float>(static_cast<double>(fStart) + Renderer::fLightLODRange.Float());
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
        BSShaderManager::fSpecularLODEnd =
            static_cast<float>(static_cast<double>(fStart) + Renderer::fSpecularLODRange.Float());
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
            const char* v0 = sOk.String();
            Interface::CreateMessageMenu(
                sNoDeviceSelected.String(), static_cast<const char*>(nullptr), static_cast<const char*>(nullptr),
                static_cast<void (*)()>(nullptr), 0, 18, 0.0f, 0.0f, v0, nullptr
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
            goto LABEL_24;
        }
        if (StartMenu::pMe->lbSettings.IsEnabled()) {
            StartMenu::pMe->Fade(StartMenu::eSettingsContainer, false, Tile::kTileValue_Invalid, -1.0f);
            StartMenu::pMe->lbSettings.SetEnabled(false);
            goto LABEL_24;
        }
        if (StartMenu::pMe->lbDownloads.IsEnabled()) {
            StartMenu::pMe->Fade(StartMenu::eDLCContainer, false, Tile::kTileValue_Invalid, -1.0f);
            StartMenu::pMe->lbDownloads.SetEnabled(false);
            goto LABEL_24;
        }
        if (StartMenu::pMe->lbSaveLoad.IsEnabled()) {
            StartMenu::pMe->UpdateCanLoadStatus();
            // Highlight and RemoveAll are called through a pointer to the list so that they dispatch virtually; a call
            // on the member itself would be devirtualised.
            (&StartMenu::pMe->lbSaveLoad)->Highlight(nullptr);
            StartMenu::pMe->UpdateHighlightedSaveGameInfo();
            StartMenu::pMe->lbSaveLoad.SetEnabled(false);
            StartMenu::pMe->Fade(StartMenu::eSaveLoadContainer, false, Tile::kTileValue_Invalid, -1.0f);
            (&StartMenu::pMe->lbSaveLoad)->RemoveAll();
        LABEL_24:;
            StartMenu::pMe->lbMain.SetEnabled(true);
            StartMenu::pMe->lbMain.DeselectAll(true);
            StartMenu::pMe->Fade(StartMenu::eBackButton, false, Tile::kTileValue_Invalid, -1.0f);
            if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_OPTIONS_DIRTY)) {
                StartMenu::SaveSettings();
                StartMenu::pMe->SetFlag(StartMenu::FLAG_OPTIONS_DIRTY, 0);
            }
            return;
        }
        if (!StartMenu::pMe->lbMain.IsEnabled()) {
            if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVELOAD_BACKOUT)) {
                return;
            }
            goto LABEL_24;
        }
        if (StartMenu::PauseMenuIsActive() && !StartMenu::pMe->IsFlagSet(StartMenu::FLAG_FORCE_RELOAD)) {
            StartMenu::Close(true);
        }
    }
}

// The PC build keeps the tests of the console's DLC handling but does nothing with them, as the game does.
void StartMenu::OnNetworkEvent(BSNetworkSystemUtility::NetEvent apEvent, uint32_t apParam) {
    if (apEvent == BSNetworkSystemUtility::NetworkStatusChanged && StartMenu::pMe && StartMenu::pMe->pDownloadsOption) {
        StartMenu::pMe->IsFlagSet(FLAG_INGAME);
    }
}

bool StartMenu::IsInPreIIS() {
    return (bool)((uint8_t)(StartMenu::MainMenuIsActive() &&
                            StartMenu::pMe->IsFlagSet(static_cast<StartMenu::Flag>(0x80000u))));
}

void StartMenu::ConfirmResetToDefaults() {
    Controls* controls;
    uint32_t uiCurrentControlsDevice;
    if ((StartMenu::pMe) && (Interface::GetMessageMenuResult() == 1)) {
        uiCurrentControlsDevice = StartMenu::pMe->uiCurrentControlsDevice;
        controls = appMain->GetControls();
        controls->InitDefaultKeybinds(uiCurrentControlsDevice);
        StartMenu::pMe->SetFlag(FLAG_OPTIONS_DIRTY, 1);
        (&StartMenu::pMe->lbOptions)
            ->ListBox<StartMenu::Option*>::Traverse(&StartMenu::SetActionForDevice, 0, 2147483647);
    }
}

void StartMenu::ResetToDefaults(Tile* apTile, StartMenu::Option* apOption) {
    StartMenu::UserOption* puserOption = dynamic_cast<StartMenu::UserOption*>(apOption);
    if ((puserOption) && (puserOption->iVal != puserOption->iDefaultVal)) {
        puserOption->iVal = puserOption->iDefaultVal;
        StartMenu::pMe->DisplayOption(puserOption, apTile);
        if (puserOption->pfChangeFn) {
            puserOption->pfChangeFn(puserOption);
            StartMenu::pMe->SetFlag(StartMenu::FLAG_OPTIONS_DIRTY, 1);
        }
    }
}

void StartMenu::GoBackIfInActionMapping() {
    if (StartMenu::pMe) {
        bool v0 = InterfaceManager::Has360Controller();
        StartMenu::pMe->pMenu->Set(Tile::kTileValue_User5, static_cast<int32_t>(v0));
        bool v1 = InterfaceManager::Has360Controller();
        Controls::EnableRumble(v1);
        if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_ACTION_MAPPING)) {
            StartMenu::ChooseBack();
        }
    }
}

void StartMenu::SetActionForDevice(Tile* apTile, StartMenu::Option* apOption) {
    if (StartMenu::pMe) {
        BSStringT<char> v8;
        apTile->Set(Tile::kTileValue_User0, 1);
        uint32_t uiCurrentControlsDevice = StartMenu::pMe->uiCurrentControlsDevice;
        // The action-mapping options keep their action in iModes.
        uint32_t uiAction = apOption->iModes;
        Controls* Controls_ = appMain->GetControls();
        uint8_t v7 = Controls_->RemapAction(static_cast<Controls::USERACTIONS>(uiAction), uiCurrentControlsDevice);
        if (v7 != 255) {
            if (!StartMenu::pMe->uiCurrentControlsDevice && (v7 < 238u && psKeyboard[v7])) {
                v8.SPrintF("%s", psKeyboard[v7]->String());
                goto LABEL_45;
            }
            if (StartMenu::pMe->uiCurrentControlsDevice == StartMenu::MODE_MAIN && (v7 < 9u && psMouse[v7])) {
                v8.SPrintF("%s", psMouse[v7]->String());
                goto LABEL_45;
            }
            if (StartMenu::pMe->uiCurrentControlsDevice != StartMenu::MODE_PAUSE) {
                if (StartMenu::pMe->uiCurrentControlsDevice == 3) {
                    const char* v6 = nullptr;
                    switch (v7) {
                        case 7: {
                            goto LABEL_41;
                        }
                        case 8: {
                            goto LABEL_29;
                        }
                        case 9: {
                            goto LABEL_31;
                        }
                        case 10: {
                            goto LABEL_33;
                        }
                        case 11: {
                            goto LABEL_35;
                        }
                        case 12: {
                            goto LABEL_37;
                        }
                        case 13: {
                            goto LABEL_39;
                        }
                        case 14: {
                            goto LABEL_27;
                        }
                        case 15: {
                            goto LABEL_25;
                        }
                        case 16: {
                            v6 = "lt";
                            goto LABEL_23;
                        }
                        case 17: {
                        LABEL_23:;
                            if (!v6) {
                                v6 = "rt";
                            }
                        LABEL_25:;
                            if (!v6) {
                                v6 = "lb";
                            }
                        LABEL_27:;
                            if (!v6) {
                                v6 = "rb";
                            }
                        LABEL_29:;
                            if (!v6) {
                                v6 = "ls";
                            }
                        LABEL_31:;
                            if (!v6) {
                                v6 = "rs";
                            }
                        LABEL_33:;
                            if (!v6) {
                                v6 = "a";
                            }
                        LABEL_35:;
                            if (!v6) {
                                v6 = "b";
                            }
                        LABEL_37:;
                            if (!v6) {
                                v6 = "x";
                            }
                        LABEL_39:;
                            if (!v6) {
                                v6 = "y";
                            }
                        LABEL_41:;
                            if (!v6) {
                                v6 = "back";
                            }
                            v8.SPrintF("Interface\\Shared\\Button\\glow_general_button_%s.dds", v6);
                            apTile->Set(Tile::kTileValue_User0, 2);
                            break;
                        }
                        default: {
                            goto LABEL_44;
                        }
                    }
                }
                else {
                LABEL_44:;
                    v8.SPrintF("%d", v7);
                }
                goto LABEL_45;
            }
            Controls* v3 = appMain->GetControls();
            if (v7 < v3->GetButtonCount(0)) {
                v8.SPrintF("%s %d", sButton.String(), v7 + 1);
                goto LABEL_45;
            }
            if (v7 >= 32u && (v7 < 40u && psWeaponTypes[v7 + 2])) {
                v8.SPrintF("%s", psWeaponTypes[v7 + 2]->String());
                goto LABEL_45;
            }
        }
        v8.Set("--", 0);
    LABEL_45:;
        apTile->Set(Tile::kTileValue_User1, v8.pString, true);
    }
}

void StartMenu::ChooseContinue() {
    if (StartMenu::MainMenuIsActive()) {
        const char* v0 = sConfirmContinue.String();
        StartMenu::pMe->pTiles[8]->Set(4036, v0, true);
        StartMenu::pMe->PopulateConfirmationListBox(256u, 0);
    }
    else {
        StartMenu::Close(true);
    }
}

void StartMenu::ChooseNew() {
    if (StartMenu::pMe) {
        const char* v0 = sConfirmNew.String();
        StartMenu::pMe->pTiles[8]->Set(4036, v0, true);
        StartMenu::pMe->PopulateConfirmationListBox(128u, 0);
    }
}

void StartMenu::ChooseDownloads() {
    if (StartMenu::pMe) {
        BSSimpleList<TESFile*>* ecx0 = pDataHandler->GetFileList();
        (&StartMenu::pMe->lbDownloads)->RemoveAll();
        const char* v0 = sOldDownloadsAvailable.String();
        StartMenu::pMe->lbDownloads.AddItem(-1, v0, nullptr, nullptr);
        while (ecx0) {
            TESFile* v2 = ecx0->m_item;
            if ((v2) && (v2->GetMaybeShouldBeReloaded())) {
                char* v1 = v2->GetSummary()->pString;
                StartMenu::pMe->lbDownloads.AddItem(0, v1, nullptr, nullptr);
            }

            ecx0 = ecx0->m_pkNext;
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
    BGSMessage* pstartMsg_2 = nullptr;
    for (int32_t i = 0; !pstartMsg_2; ++i) {
        if (i >= 10) {
            break;
        }
        InterfaceManager* v0 = InterfaceManager::Instance();
        Interface::enumMenus iid = static_cast<Interface::enumMenus>(v0->GetEnterStack(i));
        if (iid == StartMenu::GetClassStatic()) {
            break;
        }
        Interface::TutorialMessageID emsgID = Interface::TUT_COUNT;
        if (iid == Interface::eMainFour) {
            if (Interface::IsMenuIDVisible(Interface::eStatsMenu, 0)) {
                InterfaceManager* v1 = InterfaceManager::Instance();
                emsgID = v1->xTutorialManager.Find(Interface::eStatsMenu);
            }
            else {
                if (Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0)) {
                    InterfaceManager* v2 = InterfaceManager::Instance();
                    emsgID = v2->xTutorialManager.Find(Interface::ePipBoyRepair);
                }
                else {
                    if (Interface::IsMenuIDVisible(Interface::eInventoryMenu, 0)) {
                        InterfaceManager* v3 = InterfaceManager::Instance();
                        emsgID = v3->xTutorialManager.Find(Interface::eInventoryMenu);
                    }
                    else {
                        if (Interface::IsMenuIDVisible(Interface::ePipBoyData, 0)) {
                            InterfaceManager* v4 = InterfaceManager::Instance();
                            emsgID = v4->xTutorialManager.Find(Interface::ePipBoyData);
                        }
                    }
                }
            }
        }
        else {
            InterfaceManager* v5 = InterfaceManager::Instance();
            emsgID = v5->xTutorialManager.Find(iid);
        }
        if (emsgID < Interface::TUT_COUNT) {
            TESForm* v7;
            bool bFound;
            BGSListForm* pList;
            uint32_t aiSearchID;
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(emsgID + 360);
            BGSMessage* pstartMsg = dynamic_cast<BGSMessage*>(FormByNumericID);
            if (!pstartMsg) {
                goto LABEL_27;
            }
            aiSearchID = InterfaceManager::Has360Controller() ? 357 : 355;
            v7 = TESForm::GetFormByNumericID(aiSearchID);
            pList = dynamic_cast<BGSListForm*>(v7);
            if (!pList) {
                goto LABEL_27;
            }
            bFound = false;
            for (TESForm* const& rpForm : pList->ListOfForms.ItemsUntilNull()) {
                if (rpForm == pstartMsg) {
                    bFound = true;
                    break;
                }
            }
            if (bFound) {
            LABEL_27:;
                pstartMsg_2 = pstartMsg;
            }
        }
    }
    TutorialMenu::Create(pstartMsg_2, true);
}

void StartMenu::ChooseQuit() {
    if (StartMenu::pMe) {
        const char* v0 = sConfirmQuit.String();
        StartMenu::pMe->pTiles[8]->Set(4036, v0, true);
        if (StartMenu::PauseMenuIsActive()) {
            StartMenu::pMe->PopulateConfirmationListBox(StartMenu::MODE_CFM_QUITP, 3);
        }
        else {
            StartMenu::pMe->PopulateConfirmationListBox(StartMenu::MODE_CFM_QUITM, 1);
        }
    }
}

void StartMenu::ChooseMainMenu() {
    char apDest[68];
    if (StartMenu::pMe) {
        delete StartMenu::pMe;
    }
    Interface::HideMenus();
    Interface::CloseAllSupportMenus(false);
    BSAudio* v0 = &BSAudio::QInstance();
    v0->KillAllOfType(0xFFFFFFFFu, 0);
    FalloutAudioMedia::MediaClear();
    pPlayer->GetMediaLocationController();
    if (XGamerProfile::Instance()) {
        XGamerProfile* v1 = XGamerProfile::Instance();
        v1->SetPlayingFlag(false);
        XGamerProfile* v2 = XGamerProfile::Instance();
        v2->ForcePresenceUpdate();
    }
    StartMenu::bRestartingGame = true;
    appMain->ResetGame();
    TES::pInstance->ShowLoadingMenu(true, nullptr, false);
    ourMemset(apDest, 0, 64u);
    BSsprintf(apDest, 64u, "Data\\Music\\Special\\MainTitle.mp3");
    FalloutAudio::SetMainMenuMusic(true);
    FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, apDest, 0x1388u, true, true, 0.0f, 0);
    FalloutAudioMedia::MediaPlay();
}

void StartMenu::ChooseExitGame() {
    InterfaceManager* pim = InterfaceManager::Instance();
    pim->GetMenusRoot()->GetModel()->SetAppCulled(true);
    pim->pCursor->GetModel()->SetAppCulled(true);
    pim->Click(nullptr, nullptr);
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

void StartMenu::SetCrosshair(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bCrossHair_GamePlay = apUOpt->iVal == 1;
        uint32_t v1 = gs_bCrossHair_GamePlay.operator uint32_t();
        HUDMainMenu::SetVisibilityFlag(64, v1);
    }
    else {
        apUOpt->iVal = gs_bCrossHair_GamePlay.operator bool();
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetSaveOnRest(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bSaveOnRest_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = gs_bSaveOnRest_GamePlay.operator bool();
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetSaveOnWait(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bSaveOnWait_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = gs_bSaveOnWait_GamePlay.operator bool();
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetSaveOnTravel(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bSaveOnTravel_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = gs_bSaveOnTravel_GamePlay.operator bool();
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetDialogSubtitles(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bDialogueSubtitles_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = gs_bDialogueSubtitles_GamePlay.operator bool();
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetGeneralSubtitles(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        static_cast<SettingT<INIPrefSettingCollection>*>(&gs_bGeneralSubtitles_Gameplay)
            ->SettingT<INIPrefSettingCollection>::operator=(apUOpt->iVal == 1);
    }
    else {
        apUOpt->iVal = gs_bGeneralSubtitles_Gameplay.operator bool();
        apUOpt->iDefaultVal = 0;
    }
}

void StartMenu::SetTrueIronSights(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bTrueIronSights_GamePlay = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = gs_bTrueIronSights_GamePlay.operator bool();
        apUOpt->iDefaultVal = 1;
    }
}

void StartMenu::SetHUDColor(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        uint32_t afInheritedVal = g_HUDColors[apUOpt->iVal];
        SystemColorManager* Instance = SystemColorManager::GetInstance();
        Instance->SetColor_00719120(1, afInheritedVal);
        InterfaceManager* v2 = InterfaceManager::Instance();
        v2->GetMenusRoot()->RecursiveColorUpdate(1, 0.0f);
    }
    else {
        apUOpt->iDefaultVal = 2;
        apUOpt->iVal = 0;
        while (true) {
            int32_t iVal = apUOpt->iVal;
            SystemColorManager* v4 = SystemColorManager::GetInstance();
            if (g_HUDColors[iVal] == v4->GetColor_007190A0(1)) {
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
        uint32_t afInheritedVal = g_HUDColors[apUOpt->iVal];
        SystemColorManager* Instance = SystemColorManager::GetInstance();
        Instance->SetColor_00719120(4, afInheritedVal);
        InterfaceManager* v2 = InterfaceManager::Instance();
        v2->GetMenusRoot()->RecursiveColorUpdate(4, 0.0f);
    }
    else {
        apUOpt->iDefaultVal = 2;
        apUOpt->iVal = 0;
        while (true) {
            int32_t iVal = apUOpt->iVal;
            SystemColorManager* v4 = SystemColorManager::GetInstance();
            if (g_HUDColors[iVal] == v4->GetColor_007190A0(4)) {
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
        float fmin = fGammaMin.Float();
        float* v1 = &fGammaMax.Float();
        float fpct = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
        float v9 = (*v1 - fmin) * fpct + fmin;
        Renderer::SetGamma(v9);
    }
    else {
        double fGamma = Get_fGamma();
        double v7 = fGamma - fGammaMin.Float();
        float* v2 = &fGammaMax.Float();
        float v10 = v7 / (*v2 - fGammaMin.Float());
        float afVal = static_cast<double>(apUOpt->iNumSettings - 1) * v10;
        apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
        double v5 = 1.0 - fGammaMin.Float();
        float* v3 = &fGammaMax.Float();
        float v11 = v5 / (*v3 - fGammaMin.Float());
        float v4 = static_cast<double>(apUOpt->iNumSettings - 1) * v11;
        apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v4, 1.0f));
    }
}

void StartMenu::SetMasterVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float v8 = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float afNearesta = (gs_fDefaultMasterVolume_Audio = v8).operator float();
            BSAudio* v2 = &BSAudio::QInstance();
            v2->SetMasterVolume(afNearesta);
        }
        else {
            BSWin32Audio* v3 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float afVal = v3->GetMasterVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
            float v6 = static_cast<double>(apUOpt->iNumSettings - 1) * 1.0;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v6, 1.0f));
        }
    }
    else {
        float afNearest = gs_fDefaultMasterVolume_Audio.operator float();
        BSAudio* v1 = &BSAudio::QInstance();
        v1->SetMasterVolume(afNearest);
    }
}

void StartMenu::SetFootstepVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float v8 = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float afNearesta = (gs_fDefaultFootVolume_Audio = v8).operator float();
            BSAudio* v2 = &BSAudio::QInstance();
            v2->SetFootVolume(afNearesta);
        }
        else {
            BSWin32Audio* v3 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float afVal = v3->GetFootVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
            float v6 = static_cast<double>(apUOpt->iNumSettings - 1) * 0.5;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v6, 1.0f));
        }
    }
    else {
        float afNearest = gs_fDefaultFootVolume_Audio.operator float();
        BSAudio* v1 = &BSAudio::QInstance();
        v1->SetFootVolume(afNearest);
    }
}

void StartMenu::SetMusicVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float v8 = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float afNearesta = (gs_fDefaultMusicVolume_Audio = v8).operator float();
            BSAudio* v2 = &BSAudio::QInstance();
            v2->SetMusicVolume(afNearesta);
        }
        else {
            BSWin32Audio* v3 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float afVal = v3->GetMusicVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
            float v6 = static_cast<double>(apUOpt->iNumSettings - 1) * 0.6000000238418579;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v6, 1.0f));
        }
    }
    else {
        float afNearest = gs_fDefaultMusicVolume_Audio.operator float();
        BSAudio* v1 = &BSAudio::QInstance();
        v1->SetMusicVolume(afNearest);
    }
}

void StartMenu::SetVoiceVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float v8 = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float afNearesta = (gs_fDefaultVoiceVolume_Audio = v8).operator float();
            BSAudio* v2 = &BSAudio::QInstance();
            v2->SetVoiceVolume(afNearesta);
        }
        else {
            BSWin32Audio* v3 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float afVal = v3->GetVoiceVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
            float v6 = static_cast<double>(apUOpt->iNumSettings - 1) * 0.6000000238418579;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v6, 1.0f));
        }
    }
    else {
        float afNearest = gs_fDefaultVoiceVolume_Audio.operator float();
        BSAudio* v1 = &BSAudio::QInstance();
        v1->SetVoiceVolume(afNearest);
    }
}

void StartMenu::SetFXVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float v8 = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float afNearesta = (gs_fDefaultEffectsVolume_Audio = v8).operator float();
            BSAudio* v2 = &BSAudio::QInstance();
            v2->SetEffectVolume(afNearesta);
        }
        else {
            BSWin32Audio* v3 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float afVal = v3->GetEffectsVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
            float v6 = static_cast<double>(apUOpt->iNumSettings - 1) * 1.0;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v6, 1.0f));
        }
    }
    else {
        float afNearest = gs_fDefaultEffectsVolume_Audio.operator float();
        BSAudio* v1 = &BSAudio::QInstance();
        v1->SetEffectVolume(afNearest);
    }
}

void StartMenu::SetRadioVol(StartMenu::UserOption* apUOpt) {
    if (apUOpt) {
        if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
            float v8 = static_cast<double>(apUOpt->iVal) / static_cast<double>(apUOpt->iNumSettings - 1);
            float afNearesta = (gs_fDefaultRadioVolume_Audio = v8).operator float();
            BSAudio* v2 = &BSAudio::QInstance();
            v2->SetRadioVolume(afNearesta);
        }
        else {
            BSWin32Audio* v3 = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
            float afVal = v3->GetRadioVolume() * static_cast<double>(apUOpt->iNumSettings - 1);
            apUOpt->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
            float v6 = static_cast<double>(apUOpt->iNumSettings - 1) * 0.5;
            apUOpt->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v6, 1.0f));
        }
    }
    else {
        float afNearest = gs_fDefaultRadioVolume_Audio.operator float();
        BSAudio* v1 = &BSAudio::QInstance();
        v1->SetRadioVolume(afNearest);
    }
}

void StartMenu::SetCameraPitch(StartMenu::UserOption* apUOpt) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        gs_bInvertYValues_Controls = apUOpt->iVal == 1;
    }
    else {
        apUOpt->iVal = gs_bInvertYValues_Controls.operator bool();
        apUOpt->iDefaultVal = 0;
    }
}

void StartMenu::ConfirmContinue() {
    if (!pSaveLoadManager->pSaveGameList || !pSaveLoadManager->pSaveGameList->m_item) {
        pSaveLoadManager->SetupSavesList();
    }
    if (pSaveLoadManager->pSaveGameList && pSaveLoadManager->pSaveGameList->m_item) {
        BGSSaveLoadFileEntry** v0 = &pSaveLoadManager->pSaveGameList->m_item;
        if ((*v0)->IsCorrupt()) {
            const char* v2 = sOk.String();
            Interface::CreateMessageMenu(
                sSaveGameCorruptMenuMessage.String(), static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr), static_cast<void (*)()>(nullptr), 0, 5, 0.0f, 0.0f, v2, nullptr
            );
        }
        else {
            ArchiveManager::CleanupArchiveStringsAfterStartup();
            BGSSaveLoadFileEntry** v1 = &pSaveLoadManager->pSaveGameList->m_item;
            (*v1)->LoadGame();
        }
    }
    else {
        const char* v3 = sOk.String();
        Interface::CreateMessageMenu(
            sSaveNotAvailable.String(), static_cast<const char*>(nullptr), static_cast<const char*>(nullptr),
            static_cast<void (*)()>(nullptr), 0, 5, 0.0f, 0.0f, v3, nullptr
        );
    }
    StartMenu::ChooseBack();
}

void StartMenu::ConfirmNewGame() {
    char* pstopString;
    const char* v0 = sCharGenQuest.String();
    uint32_t icharGenQuest = std::strtol(v0, &pstopString, 16);
    TESForm* pform = TESForm::GetFormByNumericID(icharGenQuest);
    TESQuest* pcharGenQuest = nullptr;
    if (pPlayer) {
        pPlayer->SetDisableFastTravelResetsOnCellChange(0);
        pPlayer->SetCanFastTravel(true);
    }
    if ((pform) && (pform->GetFormType() == QUST_ID)) {
        pcharGenQuest = dynamic_cast<TESQuest*>(pform);
    }

    if (pcharGenQuest) {
        StartMenu::Close(false);
        ArchiveManager::CleanupArchiveStringsAfterStartup();
        Interface::bIsLoadingNewGame = true;
        pPlayer->SetAllowEGMCacheClear(false);
        TESNPC* pplayerBase = dynamic_cast<TESNPC*>(pPlayer->GetObjectReference());
        pplayerBase->SetFullName(sDefaultPlayerName.operator const char*());
        pcharGenQuest->SetEnabled(1);
        LoadingMenu::pMe->pTiles[11]->Set(4003, 1);
        LoadingMenu::FlagReset(true);
        InitializationFlow();
        FalloutAudio::SetMainMenuMusic(false);
        FalloutAudioMedia::MediaClose();
        if (XGamerProfile::Instance()) {
            XGamerProfile* v1 = XGamerProfile::Instance();
            v1->SetPlayingFlag(true);
            XGamerProfile* v2 = XGamerProfile::Instance();
            v2->ForcePresenceUpdate();
        }
        FalloutAudio::SetNewGameMusic(true);
        InterfaceManager* v3 = InterfaceManager::Instance();
        v3->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_STATS, 1003, 512u);
        InterfaceManager* v4 = InterfaceManager::Instance();
        v4->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_DATA, 1023, 512u);
    }
    else {
        Warning("QUESTS: Unable to find character generation quest '%s'.", sCharGenQuest.String());
        StartMenu::ChooseBack();
    }
}

void StartMenu::ConfirmLoadGame() {
    if ((StartMenu::pMe) && (StartMenu::pMe->pSaveGame)) {
        ArchiveManager::CleanupArchiveStringsAfterStartup();
        BGSSaveLoadFileEntry* psavegame = StartMenu::pMe->pSaveGame;
        if ((pPlayer) && (StartMenu::PauseMenuIsActive())) {
            TESNPC* tempPlayer = dynamic_cast<TESNPC*>(pPlayer->GetObjectReference());
            if ((tempPlayer) && (tempPlayer->GetFullName()) && (psavegame->GetPlayerName())) {
                const char* PlayerName = psavegame->GetPlayerName();
                if (BSstrcmp(tempPlayer->GetFullName(), PlayerName)) {
                    TES::pInstance->ShowLoadingMenu(true, nullptr, false);
                }
            }
        }

        bool bresult = psavegame->LoadGame();
        if (StartMenu::pMe && (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_INGAME) && bresult)) {
            StartMenu::Close(true);
        }
        else {
            if ((!bresult) && (StartMenu::pMe)) {
                StartMenu::pMe->IsFlagSet(StartMenu::FLAG_20000000);
            }

            StartMenu::ChooseBack();
        }
    }
}

void StartMenu::ConfirmSaveGame() {
    if (StartMenu::pMe) {
        (&StartMenu::pMe->lbConfirm)->RemoveAll();
        const char* v0 = sMenuDisplayShortXBoxSaveMessage.String();
        StartMenu::pMe->pTiles[8]->Set(4036, v0, true);
        StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVE_WAIT_MESSAGE, 1);
    }
}

void StartMenu::ConfirmDeleteGm() {
    if ((StartMenu::pMe) && ((&StartMenu::pMe->lbSaveLoad)->GetHighlightedItem())) {
        ListBox<BGSSaveLoadFileEntry*>* plb = &StartMenu::pMe->lbSaveLoad;
        StartMenu::pMe->lbSaveLoad.GetHighlightedData()->DeleteGame();
        Tile* v0 = static_cast<Tile*>(plb->GetHighlightedItem());
        plb->RemoveItem(v0);
        StartMenu::ChooseBack();
        uint32_t numitems = -1;
        uint32_t icnt = plb->NumUnfiltered();
        uint32_t ivis = plb->NumVisible();
        if (icnt) {
            int32_t v1;
            if (icnt >= ivis &&
                (v1 = Tile::AddUserTrait("_current_value", -1),
                 StartMenu::pMe->fSaveLoadThreshold == plb->pScrollBar->GetFloat(v1))) {
                int32_t iindex =
                    static_cast<int32_t>(static_cast<double>(ivis) + StartMenu::pMe->fSaveLoadThreshold - 1.0);
                plb->Traverse(&StartMenu::InitializeSaveLoadListItem, iindex, iindex + 1);
                numitems = StartMenu::pMe->lbSaveLoad.NumItems();
                if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_MODE)) {
                    for (int32_t i = 0; i < StartMenu::pMe->lbSaveLoad.NumItems(); ++i) {
                        Tile* tTile = (&StartMenu::pMe->lbSaveLoad)->GetTileAt(static_cast<uint32_t>(i), false);
                        BGSSaveLoadFileEntry* tSave = StartMenu::pMe->lbSaveLoad.GetDataAt(i, false);
                        if ((tTile) && (tSave)) {
                            if (tSave->IsAutosave()) {
                                StartMenu::pMe->lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                                --numitems;
                            }
                            else {
                                if (tSave->IsSystemSave()) {
                                    StartMenu::pMe->lbSaveLoad.RemoveAt(static_cast<uint32_t>(i), false);
                                    --numitems;
                                }
                            }
                        }
                    }
                }
            }
            else {
                if (StartMenu::pMe->fSaveLoadThreshold != 0.0) {
                    StartMenu::pMe->fSaveLoadThreshold = StartMenu::pMe->fSaveLoadThreshold - 1.0;
                }
            }
        }
        else {
            StartMenu::pMe->ReturnToSaveLoad();
            StartMenu::pMe->SetFlag(StartMenu::FLAG_SAVELOAD_BACKOUT, 0);
            StartMenu::ChooseBack();
        }
        if ((StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_MODE)) && (StartMenu::pMe->lbSaveLoad.NumItems())) {
            char Src[68];
            if (numitems == -1) {
                numitems = StartMenu::pMe->lbSaveLoad.NumItems();
                for (int32_t i_2 = 0; i_2 < StartMenu::pMe->lbSaveLoad.NumItems(); ++i_2) {
                    Tile* tTile_2 = (&StartMenu::pMe->lbSaveLoad)->GetTileAt(static_cast<uint32_t>(i_2), false);
                    BGSSaveLoadFileEntry* tSave_2 = StartMenu::pMe->lbSaveLoad.GetDataAt(i_2, false);
                    if ((tTile_2) && (tSave_2)) {
                        if (tSave_2->IsAutosave()) {
                            --numitems;
                        }
                        else {
                            if (tSave_2->IsSystemSave()) {
                                --numitems;
                            }
                        }
                    }
                }
            }
            --numitems;
            const char* v4 = sSavesRemaining.String();
            BSsprintf(Src, 64, "%s %u/%i %s", sNewSave.String(), numitems, 1000, v4);
            Tile* FirstItem = StartMenu::pMe->lbSaveLoad.GetFirstItem();
            FirstItem->Set(4036, Src, true);
            Tile* v3 = StartMenu::pMe->lbSaveLoad.GetFirstItem();
            v3->AddNeedsUpdate(8);
        }
    }
}

void StartMenu::MountDLCCallback(uint32_t which, uint32_t last, bool valid, const BSContentPackage* package) {
    if (which == -1 && last == -1) {
        Interface::CloseMessageMenu();
        StartMenu::pMe->ShowDLCList();
    }
    else {
        char Dest[132];
        const char16_t* Source = package->DisplayName();
        rt::wcstombs(Dest, Source, 128u);
        StartMenu::pMe->lbDownloads.AddItem(0, Dest, nullptr, nullptr);
    }
}

void StartMenu::FailedSaveCallbackFunction() {
    StartMenu::pMe->SetFlag(StartMenu::FLAG_MEM_CARD_REMOVED, 0);
    MessageMenu::ClearMessageQueue();
    Interface::CloseMessageMenu();
    StartMenu::ChooseBack();
}

void StartMenu::InitializeSaveLoadListItem(Tile* apTile, BGSSaveLoadFileEntry* apSaveGame) {
    if ((apTile) && (apSaveGame)) {
        int32_t v2 = Tile::AddUserTrait("_initialized", -1);
        if (!apTile->IsTrue(v2)) {
            const char* Src;
            if (apSaveGame->IsAutosave()) {
                const char* v3 = sAutosaveAbbrev.String();
                apTile->Set(Tile::kTileValue_User0, v3, true);
            }
            else {
                if (apSaveGame->IsQuicksave()) {
                    const char* v4 = sQuicksaveAbbrev.String();
                    apTile->Set(Tile::kTileValue_User0, v4, true);
                }
                else {
                    if (apSaveGame->IsSystemSave()) {
                        const char* v5 = gs_sSystemSaveAbbrev.String();
                        apTile->Set(Tile::kTileValue_User0, v5, true);
                    }
                    else {
                        BSStringT<char> strtmp;
                        strtmp.SPrintF("#%04d", apSaveGame->GetSaveGameNumber());
                        apTile->Set(Tile::kTileValue_User0, strtmp.pString, true);
                    }
                }
            }
            if (apSaveGame->IsCorrupt()) {
                Src = sSaveGameCorrupt.String();
            }
            else {
                Src = apSaveGame->GetLocation();
            }
            apTile->Set(Tile::kTileValue_User1, Src, true);
            int32_t v6 = Tile::AddUserTrait("_initialized", -1);
            apTile->Set(v6, 1);
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
        StartMenu::pMe->SetFlag(static_cast<StartMenu::Flag>(0x10000u), 1u);
        StartMenu::pMe->Fade(StartMenu::eMainTitle, false, 4009, (-0x1p+0f));
        StartMenu::pMe->Fade(StartMenu::eMainContainer, false, 0x80000000u, (-0x1p+0f));
        StartMenu::pMe->Fade(StartMenu::eWelcomeText, false, 4009, (-0x1p+0f));
    }
}

void MenuButton_Display() {
    if (StartMenu::pMe) {
        StartMenu::pMe->pTiles[10]->Set(4003, 1);
        (&StartMenu::pMe->lbSettings)->SetEnabled(false);
        StartMenu::pMe->PopulateOptionsListBox(16);
    }
}

void MenuButton_Audio() {
    if (StartMenu::pMe) {
        (&StartMenu::pMe->lbSettings)->SetEnabled(false);
        StartMenu::pMe->PopulateOptionsListBox(32);
    }
}

bool GetIsControllerDisabled() {
    return (bool)((uint8_t)(&gs_bDisable360Controller)->operator bool());
}

uint32_t* IncrementControlsDevice(uint32_t* apDevice) {
    if (static_cast<int32_t>(++*apDevice) >= 4) {
        *apDevice = 0;
    }
    return apDevice;
}

void SetXboxController(StartMenu::UserOption* a1) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        bool v5 = a1->iVal == 1;
        bool v3 = a1->iVal != 1;
        InterfaceManager* v1 = InterfaceManager::Instance();
        v1->Disable360Controller(v3);
        if (StartMenu::pMe->unk1CC) {
            StartMenu::pMe->unk1CC->Set(static_cast<int32_t>(StartMenu::pMe->ENABLED_TRAIT), static_cast<int32_t>(v5));
        }
    }
    else {
        InterfaceManager::Instance();
        a1->iVal = !GetIsControllerDisabled();
        InterfaceManager::Instance();
        char IsControllerDisabled = GetIsControllerDisabled();
        appMain->GetControls()->bIsControllerDisabled = IsControllerDisabled;
        appMain->GetControls();
        InterfaceManager::Instance();
        bool v2 = GetIsControllerDisabled();
        Controls::EnableRumble(!v2);
        a1->iDefaultVal = 1;
    }
}

float Get_fGamma() {
    return gs_fGamma_Display.operator float();
}

void SetTextureSize(StartMenu::UserOption* a1) {
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        Renderer::iTexMipMapSkip = a1->iVal;
        StartMenu::pMe->SetFlag(StartMenu::FLAG_RESTART_REQUIRED, NiDX9SourceTextureData::uiMipMapSkip != a1->iVal);
    }
    else {
        a1->iVal = NiDX9SourceTextureData::uiMipMapSkip;
        a1->iDefaultVal = 0;
    }
}

void SetActorFade(StartMenu::UserOption* a1) {
    float fActorLODMin = ::fActorLODMin.Float();
    float fActorLODMax = ::fActorLODMax.Float();
    if (StartMenu::pMe && StartMenu::pMe->IsFlagSet(StartMenu::FLAG_SAVE_OPTIONS)) {
        float v5 = static_cast<double>(a1->iVal) / static_cast<double>(a1->iNumSettings - 1);
        float v6 = (fActorLODMax - fActorLODMin) * v5 + fActorLODMin;
        BSFadeNode::SetFadeOutMultiplier(LOD_MULT_ACTORS, v6);
        fLODFadeOutMultActors = v6;
    }
    else {
        float v3 = (BSFadeNode::QFadeOutMultiplier(LOD_MULT_ACTORS) - fActorLODMin) / (fActorLODMax - fActorLODMin);
        float afVal = static_cast<double>(a1->iNumSettings - 1) * v3;
        a1->iVal = static_cast<int32_t>(BSRoundToNearest(afVal, 1.0f));
        float v4 = (fActorLODDefault.Float() - fActorLODMin) / (fActorLODMax - fActorLODMin);
        float v1 = static_cast<double>(a1->iNumSettings - 1) * v4;
        a1->iDefaultVal = static_cast<int32_t>(BSRoundToNearest(v1, 1.0f));
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
