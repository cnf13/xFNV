#include "Fallout/Interface/Interface.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSMovie/MoviePlayer.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectIMOD.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESRecipe.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/FOHUDEffectManager.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/MenuConsole.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Interface/Menus/BookMenu.h"
#include "Fallout/Interface/Menus/CaravanMenu.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Fallout/Interface/Menus/CompanionWheelMenu.h"
#include "Fallout/Interface/Menus/ComputersMenu.h"
#include "Fallout/Interface/Menus/ContainerMenu.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Fallout/Interface/Menus/ItemModMenu.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/LockPickMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/MessageMenu.h"
#include "Fallout/Interface/Menus/PlayerNameEntryMenu.h"
#include "Fallout/Interface/Menus/RaceSexMenu.h"
#include "Fallout/Interface/Menus/RecipeMenu.h"
#include "Fallout/Interface/Menus/RepairMenu.h"
#include "Fallout/Interface/Menus/RepairServicesMenu.h"
#include "Fallout/Interface/Menus/RouletteMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/SlotMachineMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/SurgeryMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

#include <SDL3/SDL.h>
#include <cstdarg>
#include <cstdint>

SettingT<INIPrefSettingCollection> bDialogueSubtitles("bDialogueSubtitles:GamePlay", true);
SettingT<INIPrefSettingCollection> bGeneralSubtitles("bGeneralSubtitles:GamePlay", false);
SettingT<INIPrefSettingCollection> bCrossHair("bCrossHair:GamePlay", true);
SettingT<INIPrefSettingCollection> bSaveOnRest("bSaveOnRest:GamePlay", true);
SettingT<INIPrefSettingCollection> bSaveOnWait("bSaveOnWait:GamePlay", true);
SettingT<INIPrefSettingCollection> bSaveOnTravel("bSaveOnTravel:GamePlay", true);
static SettingT<INIPrefSettingCollection> bSaveOnInteriorExteriorSwitch("bSaveOnInteriorExteriorSwitch:GamePlay", true);
SettingT<INIPrefSettingCollection> bDisableDynamicCrosshair("bDisableDynamicCrosshair:GamePlay", false);
SettingT<INIPrefSettingCollection> bTrueIronSights("bTrueIronSights:GamePlay", true);
Interface::QUEUED_OPEN Interface::QueuedMenuOpen;
Interface::QUEUED_OPEN Interface::QueuedMessageOpen;
BSSpinLock Interface::QueuedMenuLock;

int32_t BSMAX_2(int32_t aiA, char acB) {
    if (aiA <= acB) {
        return acB;
    }

    return aiA;
}

void ModalMessageMenuCallback() {
    iModalMessageMenuResult = Interface::GetMessageMenuResult();
}

Interface::QUEUED_OPEN::QUEUED_OPEN()
  : eOpenType(Interface::OPEN_NONE), pMenuRef(nullptr), pGreetTopic(nullptr), pSpeakeras(nullptr),
    eRSMContext(RSM_CONTEXT_NONE), eContainerMode(Interface::CMM_STANDARD) {}

// Saves before version 12 hold a string instead of the tutorial flags; it is read into a local and dropped. LoadData
// copies raw bytes, so each local is passed as an untyped char buffer.
void Interface::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    char acOldString[268];
    char acTutorialFlags[100];
    int32_t iHUDValue = 0;
    uint8_t ucStatsTab = 0;
    uint8_t ucInventoryTab = 0;
    uint8_t ucMapTab = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iHUDValue), sizeof(iHUDValue));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucStatsTab), sizeof(ucStatsTab));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucInventoryTab), sizeof(ucInventoryTab));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucMapTab), sizeof(ucMapTab));
    InterfaceManager::Instance()->GetMenusRoot()->Set(6001, iHUDValue);
    StatsMenu::SetCurrentStatsPage(ucStatsTab);
    InventoryMenu::SetCurrentInventoryPage(ucInventoryTab);
    MapMenu::SetCurrentMapPage(ucMapTab);
    float fLocalMagnification = 0.0f;
    float fWorldMagnification = 0.0f;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fLocalMagnification), sizeof(fLocalMagnification));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWorldMagnification), sizeof(fWorldMagnification));
    MapMenu::SetLocalMapMagnification(fLocalMagnification);
    MapMenu::SetWorldMapMagnification(fWorldMagnification);
    // The version is zero-extended and compared signed.
    if (static_cast<int32_t>(apLoadGameBuffer->GetVersion()) < 12) {
        apLoadGameBuffer->LoadString(acOldString);
        return;
    }
    ourMemset(acTutorialFlags, 0, 100);
    uint32_t uiSize = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiSize), sizeof(uiSize));
    apLoadGameBuffer->LoadData(acTutorialFlags, uiSize);
    InterfaceManager::Instance()->xTutorialManager.Load(acTutorialFlags);
}

bool Interface::ShowSubtitle(
    const char* apString,
    BSSoundHandle ahandle,
    NiPoint3 aSpeakerLoc,
    TESObjectREFR* apTarget,
    bool abForceShow
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp",
        2284
    );
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit && HUDMainMenu::pme) {
        HUDMainMenu::pme->ShowSubtitle(apString, ahandle, aSpeakerLoc, apTarget, abForceShow);
    }
    return false;
}

void Interface::SetMapMenuVisible(bool abVisibility, bool abAnyVisibleState) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        Tile* pStat = Menu::GetMenuByClass(1003);
        Tile* pInv = Menu::GetMenuByClass(1002);
        Tile* pRepair = Menu::GetMenuByClass(1035);
        Tile* pItemMod = Menu::GetMenuByClass(1061);
        Tile* pMap = Menu::GetMenuByClass(1023);
        bool bIsVis = Interface::IsMenuIDVisible(
            Interface::ePipBoyData,
            abAnyVisibleState ? Interface::FADE_NOT_HIDDEN : 0
        );
        if (!abVisibility || bIsVis) {
            if (!abVisibility && bIsVis && pMap) {
                pMap->GetMenu()->StartFadeOut();
                pMap->GetMenu()->DoLeave(0, nullptr);
            }
        }
        else {
            if (pStat) {
                pStat->GetMenu()->StartFadeOut();
                StatsMenu::ResetHealingMode();
            }
            if (pRepair) {
                pRepair->GetMenu()->StartFadeOut();
                RepairMenu::ResetMenu();
            }
            if (pItemMod) {
                pItemMod->GetMenu()->StartFadeOut();
                ItemModMenu::ResetMenu();
            }
            if (pInv) {
                pInv->GetMenu()->StartFadeOut();
            }
            if (!pMap) {
                pMap = Interface::CreateMapMenu();
            }
            if (pMap) {
                InterfaceManager::Instance()->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1023);
                pMap->GetMenu()->StartFadeIn();
            }
            MapMenu::UpdateMenu();
        }
        if (abVisibility) {
            InterfaceManager::Instance()->GetPipboy()->TriggerRandomEffect();
            InterfaceManager::Instance()->GetPipboy()->ActivateButton(3u);
            if (InterfaceManager::Instance()->GetPipboy()->GetInitialized()) {
                MapMenu::SetKnobPosition(false);
            }
        }
    }
}

void Interface::OpenConsole() {
    if (InterfaceManager::Instance() &&
        InterfaceManager::Instance()->IsDoneFirstInit() &&
        MenuConsole::Instance(false)) {
        if (!MenuConsole::Instance(true)->IsVisible()) {
            MenuConsole::Instance(true)->ToggleVisible();
            InterfaceManager::Instance()->AddToEnterStack(3);
        }
    }
}

void Interface::SetInventoryMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        TileMenu* pStat = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
        TileMenu* pInv = static_cast<TileMenu*>(Menu::GetMenuByClass(1002));
        TileMenu* pRepair = static_cast<TileMenu*>(Menu::GetMenuByClass(1035));
        TileMenu* pItemMod = static_cast<TileMenu*>(Menu::GetMenuByClass(1061));
        TileMenu* pMap = static_cast<TileMenu*>(Menu::GetMenuByClass(1023));
        uint8_t ucIsVis = Interface::IsMenuIDVisible(
            Interface::eInventoryMenu,
            abAnyVisibleState ? Interface::FADE_NOT_HIDDEN : 0
        );
        if (abVisibility) {
            if (!pInv) {
                pInv = static_cast<TileMenu*>(Interface::CreateInventoryMenu());
            }
            if (!ucIsVis) {
                if (pStat) {
                    pStat->GetMenu()->StartFadeOut();
                    StatsMenu::ResetHealingMode();
                }
                if (pMap) {
                    pMap->GetMenu()->StartFadeOut();
                }
                if (pRepair) {
                    pRepair->GetMenu()->StartFadeOut();
                    RepairMenu::ResetMenu();
                }
                if (pItemMod) {
                    pItemMod->GetMenu()->StartFadeOut();
                    ItemModMenu::ResetMenu();
                }
                if (pInv) {
                    pInv->GetMenu()->StartFadeIn();
                }
            }
            if (pInv) {
                InventoryMenu::UpdateList();
                InterfaceManager::Instance()->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1002);
            }
            if (abTriggerPipboyEffect) {
                InterfaceManager::Instance()->GetPipboy()->TriggerRandomEffect();
                InterfaceManager::Instance()->GetPipboy()->ActivateButton(2u);
            }
            if (InterfaceManager::Instance()->GetPipboy()->GetInitialized()) {
                InventoryMenu::SetKnobPosition(false);
            }
        }
        else if (ucIsVis && pInv) {
            pInv->GetMenu()->StartFadeOut();
            InventoryMenu::SavePageState();
        }
    }
}

void Interface::SetStatsMenuVisible(bool abVisibility, bool abAnyVisibleState) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        Tile* pStat = Menu::GetMenuByClass(1003);
        Tile* pInv = Menu::GetMenuByClass(1002);
        Tile* pRepair = Menu::GetMenuByClass(1035);
        Tile* pItemMod = Menu::GetMenuByClass(1061);
        Tile* pMap = Menu::GetMenuByClass(1023);
        bool bIsVis = Interface::IsMenuIDVisible(
            Interface::eStatsMenu,
            abAnyVisibleState ? Interface::FADE_NOT_HIDDEN : 0
        );
        if (!abVisibility || bIsVis) {
            if (!abVisibility && bIsVis && pStat) {
                pStat->GetMenu()->StartFadeOut();
            }
        }
        else {
            if (pInv) {
                pInv->GetMenu()->StartFadeOut();
            }
            if (pRepair) {
                pRepair->GetMenu()->StartFadeOut();
                RepairMenu::ResetMenu();
            }
            if (pItemMod) {
                pItemMod->GetMenu()->StartFadeOut();
                ItemModMenu::ResetMenu();
            }
            if (pMap) {
                pMap->GetMenu()->StartFadeOut();
            }
            if (!pStat) {
                pStat = Interface::CreateStatsMenu();
            }
            if (pStat) {
                InterfaceManager::Instance()->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1003);
                StatsMenu* pStatsMenu = static_cast<StatsMenu*>(pStat->GetMenu());
                pStatsMenu->UpdateAll();
                pStat->GetMenu()->StartFadeIn();
            }
        }
        if (abVisibility) {
            InterfaceManager::Instance()->GetPipboy()->TriggerRandomEffect();
            InterfaceManager::Instance()->GetPipboy()->ActivateButton(1u);
            if (InterfaceManager::Instance()->GetPipboy()->GetInitialized()) {
                StatsMenu::SetKnobPosition(false);
            }
        }
    }
}

char Interface::GetInventoryPageNumber() {
    if (!InterfaceManager::Instance()) {
        return -1;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return -1;
    }
    return InterfaceManager::Instance()->GetInventoryPageNumber();
}

int32_t Interface::GetCurrentMainMenu() {
    if (!InterfaceManager::Instance()) {
        return -1;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return -1;
    }
    return InterfaceManager::Instance()->GetCurrentMainMenu();
}

uint32_t Interface::GetContainerMode() {
    return ContainerMenu::GetContainerMode();
}

void Interface::ForceReload() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        StartMenu::Create(true, true);
    }
}

void Interface::UpdateActorValue(ActorValue::Index aeWhich) {
    if (InterfaceManager::Instance() &&
        InterfaceManager::Instance()->IsDoneFirstInit() &&
        Interface::IsMenuIDVisible(Interface::eStatsMenu, Interface::FADE_NOT_HIDDEN)) {
        TileMenu* pStat = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
        StatsMenu* pStatsMenu = nullptr;
        if (pStat) {
            pStatsMenu = dynamic_cast<StatsMenu*>(pStat->GetMenu());
        }
        if (pStatsMenu) {
            if (aeWhich == ActorValue::ACTOR_VALUE_COUNT) {
                pStatsMenu->UpdateAllActorValues();
            }
            else {
                pStatsMenu->UpdateActorValue(aeWhich);
            }
        }
    }
}

void Interface::SetCursorPos(float afX, float afY) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->SetCursorPos(afX, afY);
    }
}

char Interface::GetMessageMenuResult() {
    char cResult = InterfaceManager::Instance()->cLastMessageButtonClicked;
    InterfaceManager::Instance()->cLastMessageButtonClicked = -1;
    return cResult;
}

void Interface::ToggleDebugTextVisible() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->ToggleDebugTextVisible();
    }
}

void Interface::FinishLoadGame(BGSLoadGameBuffer* /*apLoadGameBuffer*/) {
    StatsMenu::UpdatePerks();
    StatsMenu::UpdatePlayerName();
    StatsMenu::UpdatePlayerTitle();
    MapMenu::SetPlayerMarker();
    MapMenu::ClearSelectedNote();
    MapMenu::ClearMapMemory();
    SurgeryMenu::ResetAllLimbs();
}

bool Interface::IsTopOfMenuStackReady() {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    if (!InterfaceManager::Instance()->bFirstInit) {
        return false;
    }
    Tile* pMenu = nullptr;
    int32_t iClass = InterfaceManager::Instance()->GetEnterStackTop();
    if (iClass == 1) {
        if (Interface::IsMenuIDVisible(Interface::eStatsMenu, 0)) {
            pMenu = Menu::GetMenuByClass(1003);
        }
        else if (Interface::IsMenuIDVisible(Interface::eInventoryMenu, 0)) {
            pMenu = Menu::GetMenuByClass(1002);
        }
        else if (Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0)) {
            pMenu = Menu::GetMenuByClass(1035);
        }
        else if (Interface::IsMenuIDVisible(Interface::eItemModMenu, 0)) {
            pMenu = Menu::GetMenuByClass(1061);
        }
        else if (Interface::IsMenuIDVisible(Interface::ePipBoyData, 0)) {
            pMenu = Menu::GetMenuByClass(1023);
        }
    }
    else {
        pMenu = Menu::GetMenuByClass(iClass);
    }
    return pMenu && pMenu->GetMenu() && pMenu->GetMenu()->xFadeState == Interface::FADE_SHOWN;
}

Tile* Interface::CreateContainerMenu(TESObjectREFR* apContainer, Interface::CONTAINER_MENU_MODE aeMode) {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return nullptr;
    }
    if (aeMode == CMM_STANDARD &&
        !apContainer->IsActor() &&
        apContainer->GetOwner() &&
        !apContainer->IsAnOwner(pPlayer, true)) {
        aeMode = CMM_PICKPOCKET;
    }

    return ContainerMenu::Create(apContainer, aeMode);
}

bool Interface::CreateHackingMenu(TESObjectREFR* apTerminalRef) {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    if (!InterfaceManager::Instance()->bFirstInit) {
        return false;
    }
    Tile* pMenu = HackingMenu::Create(apTerminalRef);
    if (bUseRenderedTerminals.Bool()) {
        Tile::UpdateAll(false);
        FORenderedTerminal* prm = new FORenderedTerminal;
        prm->pMenuTile = pMenu;
        prm->SetMenuRoot(pMenu->GetModel());
        InterfaceManager::Instance()->EnterRenderedMenu(prm);
    }
    return true;
}

bool Interface::CreateComputersMenu(TESObjectREFR* apTerminalRef) {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    if (!InterfaceManager::Instance()->bFirstInit) {
        return false;
    }
    Tile* pMenu = ComputersMenu::Create(apTerminalRef);
    Tile::UpdateAll(false);
    if (bUseRenderedTerminals.Bool()) {
        FORenderedTerminal* prm = new FORenderedTerminal;
        prm->pMenuTile = pMenu;
        prm->SetMenuRoot(pMenu->GetModel());
        InterfaceManager::Instance()->EnterRenderedMenu(prm);
    }
    return true;
}

void Interface::CreateRaceSexMenu(RaceSexMenuContext aeContext) {
    if (DialogMenu::IsDialogActive()) {
        DialogMenu::ServiceStart();
    }
    if (iNumHWThreads.Int() <= 1 || Interface::QueuedMenuOpen.eOpenType) {
        RaceSexMenu::Create(aeContext);
    }
    else {
        Interface::QueuedMenuOpen.eOpenType = Interface::OPEN_RACESEX;
        Interface::QueuedMenuOpen.eRSMContext = aeContext;
    }
}

void Interface::DrawLockpickMenu3D() {
    LockPickMenu::Draw3DElements();
}

void Interface::DrawSlotMachineMenu3D() {
    SlotMachineMenu::DrawScene();
}

void Interface::DrawBlackJackMenu3D() {
    BlackJackMenu::DrawScene();
}

void Interface::DrawRouletteMenu3D() {
    RouletteMenu::DrawScene();
}

void Interface::DrawCaravanMenu3D() {
    CaravanMenu::Draw3DElements();
}

Tile* Interface::CreateDialogMenu(TESObjectREFR* apSpeaker, TESTopic* apGreetTopic, MobileObject* apSpeakeras) {
    appTimer.SetGlobalTimeMultiplier(1.0, true);
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {

        return static_cast<TileMenu*>(
            DialogMenu::Create(static_cast<Actor*>(apSpeaker), apGreetTopic, static_cast<Actor*>(apSpeakeras))
        );
    }

    return nullptr;
}

// The pip-boy must be closed, unless the request is the pause or message menu. The player name entry request builds
// its menu and tile here.
void Interface::HandleQueuedMenuOpen() {
    QueuedMenuLock.Lock(nullptr);
    if (QueuedMenuOpen.eOpenType != OPEN_NONE &&
        (InterfaceManager::Instance()->ePipboyState == InterfaceManager::PS_CLOSED ||
         QueuedMenuOpen.eOpenType == OPEN_PAUSE ||
         QueuedMenuOpen.eOpenType == OPEN_MESSAGE)) {
        bool bSuccess = false;
        switch (QueuedMenuOpen.eOpenType) {
            case OPEN_CONTAINER:
                bSuccess = CreateContainerMenu(QueuedMenuOpen.pMenuRef, QueuedMenuOpen.eContainerMode) != nullptr;
                break;
            case OPEN_HACKING: bSuccess = CreateHackingMenu(QueuedMenuOpen.pMenuRef); break;
            case OPEN_COMPUTERS: bSuccess = CreateComputersMenu(QueuedMenuOpen.pMenuRef); break;
            case OPEN_DIALOG:
                if (!InterfaceManager::Instance()->IsInMenuMode()) {
                    bSuccess = CreateDialogMenu(
                                   QueuedMenuOpen.pMenuRef,
                                   QueuedMenuOpen.pGreetTopic,
                                   QueuedMenuOpen.pSpeakeras
                               ) != nullptr;
                }
                break;
            case OPEN_RACESEX: bSuccess = RaceSexMenu::Create(QueuedMenuOpen.eRSMContext) != nullptr; break;
            case OPEN_PAUSE:
                bSuccess = StartMenu::Create(true, false);
                if (bSuccess) {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_START);
                }
                break;
            case OPEN_MESSAGE: bSuccess = MessageMenu::Create(); break;
            case OPEN_COMPANION_WHEEL:
                bSuccess = CompanionWheelMenu::CreateCompanionWheelMenu(dynamic_cast<Actor*>(QueuedMenuOpen.pMenuRef));
                break;
            case OPEN_PLAYER_NAME_ENTRY: {
                TileMenu* pTile = new TileMenu(nullptr, nullptr);
                PlayerNameEntryMenu* pMenu = new PlayerNameEntryMenu;
                pTile->SetParent(InterfaceManager::Instance()->GetMenusRoot(), nullptr);
                pTile->SetName("Player Name Entry Menu");
                pMenu->SetMenuTile(pTile, true);
                pTile->SetMenu(pMenu);
                bSuccess = true;
                break;
            }
            default: break;
        }
        if (bSuccess) {
            QueuedMenuOpen.eOpenType = OPEN_NONE;
            QueuedMenuOpen.pMenuRef = nullptr;
        }
    }
    if (QueuedMessageOpen.eOpenType != OPEN_NONE &&
        (InterfaceManager::Instance()->ePipboyState == InterfaceManager::PS_CLOSED ||
         QueuedMessageOpen.eOpenType == OPEN_PAUSE ||
         QueuedMessageOpen.eOpenType == OPEN_MESSAGE)) {
        bool bSuccess = false;
        if (QueuedMessageOpen.eOpenType == OPEN_MESSAGE) {
            bSuccess = MessageMenu::Create();
        }
        if (bSuccess) {
            QueuedMessageOpen.eOpenType = OPEN_NONE;
            QueuedMessageOpen.pMenuRef = nullptr;
        }
    }
    QueuedMenuLock.Unlock();
}

void Interface::Init(bool abInitialLoad) {
    alignas(AutoMemContext) uint8_t kContextMemory[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (kContextMemory) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp",
        176
    );
    InterfaceManager::Initialize(1, abInitialLoad);
    pContext->~AutoMemContext();
}

void Interface::InitSecond() {
    alignas(AutoMemContext) uint8_t kContextMemory[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (kContextMemory) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp",
        189
    );
    InterfaceManager::Initialize(2, true);
    pContext->~AutoMemContext();
}

void Interface::Click(BSRenderedTexture* ap3DTarget, BSRenderedTexture* pMenuFXTexture) {
    bool bRendered = false;
    if (InterfaceManager::Instance() != nullptr &&
        InterfaceManager::Instance()->IsDoneFirstInit() &&
        !pMoviePlayer->GetPlayingSequence()) {
        InterfaceManager::Instance()->Click(ap3DTarget, pMenuFXTexture);
        bRendered = true;
    }
    if (!bRendered) {
        // 23 is the render threads' interface stage.
        MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 23);
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 23);
    }
    InterfaceManager::Instance()->SetClickMultithreaded(false);
}

void Interface::CloseConsole() {
    if (InterfaceManager::Instance() == nullptr) {
        return;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return;
    }
    if (MenuConsole::Instance(false) == nullptr) {
        return;
    }
    if (!MenuConsole::Instance(true)->IsVisible()) {
        return;
    }
    MenuConsole::Instance(true)->ToggleVisible();
    InterfaceManager::Instance()->PopFromEnterStack(eConsoleMenu, false);
}

void Interface::QueueMenuCreate(
    OPEN_REQUEST aeMenuType,
    TESObjectREFR* apMenuRef,
    TESTopic* apGreeting,
    MobileObject* apSpeakAs,
    Interface::CONTAINER_MENU_MODE aeContainerMode,
    bool bModeless
) {
    if (aeMenuType == Interface::OPEN_MESSAGE) {
        Interface::QueuedMessageOpen.eOpenType = Interface::OPEN_MESSAGE;
        Interface::QueuedMessageOpen.pMenuRef = apMenuRef;
        Interface::QueuedMessageOpen.pGreetTopic = apGreeting;
        Interface::QueuedMessageOpen.pSpeakeras = apSpeakAs;
        Interface::QueuedMessageOpen.eContainerMode = aeContainerMode;
    }
    else {
        Interface::QueuedMenuOpen.eOpenType = aeMenuType;
        Interface::QueuedMenuOpen.pMenuRef = apMenuRef;
        Interface::QueuedMenuOpen.pGreetTopic = apGreeting;
        Interface::QueuedMenuOpen.pSpeakeras = apSpeakAs;
        Interface::QueuedMenuOpen.eContainerMode = static_cast<CONTAINER_MENU_MODE>(aeContainerMode);
    }
    if (!AILinearTaskThreadManager::ShouldQueue3DTask()) {
        Interface::HandleQueuedMenuOpen();
        Interface::ClearMenuQueue();
    }
}

// The folder the interface's tile paths are built from.
const char* Interface::GetMeshesFolder() {
    return "Meshes";
}

// True while a loading menu exists.
bool Interface::IsInGameLoadingMenuOpen() {
    return LoadingMenu::pMe != nullptr;
}

TESObjectREFR* Interface::GetEnemyActor() {
    return HUDMainMenu::GetEnemyActor();
}

void Interface::SetEnemyActor(Actor* apActor) {
    HUDMainMenu::SetEnemyActor(apActor);
}

int32_t Interface::GetDetectionLevel() {
    return HUDMainMenu::iDetectionLevel;
}

void Interface::InitGunScope(TESModel* apModelFilename) {
    HUDMainMenu::InitGunScope(apModelFilename);
}

// Matches apString first as a setting's name, then as a user action setting's text.
int32_t Interface::MatchTextReplaceConstantToGameSettings(const char* apString) {
    Setting* pSetting = SettingT<GameSettingCollection>::GetCollection()->GetPtr(apString);
    int32_t i;
    for (i = 0; i < 28 && psUserActions[i] != pSetting; ++i) {}
    if (i == 28) {
        for (i = 0; i < 28; ++i) {
            if (!_tcscmp(psUserActions[i]->String(), apString)) {
                break;
            }
        }
    }
    return i;
}

float Interface::GetScreenHeight() {
    return InterfaceManager::GetScreenHeight();
}

float Interface::GetScreenWidth() {
    return InterfaceManager::GetScreenWidth();
}

void Interface::PlayMenuSound(int32_t aiSoundIndex) {
    InterfaceManager::PlayMenuSound(static_cast<MENU_SOUNDS>(aiSoundIndex));
}

double Interface::GetRealScreenHeight() {
    return InterfaceManager::GetRealScreenHeight();
}

double Interface::GetRealScreenWidth() {
    return InterfaceManager::GetRealScreenWidth();
}

bool Interface::IsWideScreen() {
    return Renderer::GetWidescreen();
}

// The Float setting is printed with "%i" as a double.
bool Interface::TestConstantForGameSettings(const char* apString, char* axStringOut) {
    if (apString == nullptr || *apString == '\0') {
        return false;
    }
    int32_t iOffset = 0;
    if (apString[iOffset] == '&') {
        ++iOffset;
    }
    if (apString[iOffset] == '-') {
        ++iOffset;
    }
    char cBuffer[260];
    BSstrcpy(cBuffer, 260, &apString[iOffset]);
    // The length is tested so that a name that strips to "" does not read the byte before cBuffer.
    uint32_t uiLength = BSstrlen(cBuffer);
    if (uiLength != 0 && cBuffer[uiLength - 1] == ';') {
        cBuffer[BSstrlen(cBuffer) - 1] = '\0';
    }
    Setting* pSetting = SettingT<GameSettingCollection>::GetCollection()->GetPtr(cBuffer);
    if (pSetting == nullptr) {
        BSstrcat(cBuffer, 260, ":LANGUAGE");
        pSetting = SettingT<INISettingCollection>::GetCollection()->GetPtr(cBuffer);
    }
    if (pSetting == nullptr) {
        return false;
    }
    if (cBuffer[0] == 's' || cBuffer[0] == 'S') {
        ourStrcpy(axStringOut, pSetting->String());
    }
    else if (cBuffer[0] == 'i' || cBuffer[0] == 'I') {
        std::sprintf(cBuffer, "%i", pSetting->UInt());
        ourStrcpy(axStringOut, cBuffer);
    }
    else if (cBuffer[0] == 'f' || cBuffer[0] == 'F') {
        std::sprintf(cBuffer, "%i", pSetting->Float());
        ourStrcpy(axStringOut, cBuffer);
    }
    else if (cBuffer[0] == 'b' || cBuffer[0] == 'B') {
        // Compares the setting's raw byte with 1: a bool compare would only test for zero, so the byte is read as
        // untyped memory.
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&pSetting->Bool())) == 1) {
            ourStrcpy(axStringOut, "&true;");
        }
        else {
            ourStrcpy(axStringOut, "&false;");
        }
    }
    return true;
}

bool Interface::FindTextReplacementString(
    const char* apString,
    char* apOutBuffer,
    uint32_t aiBufferSize,
    bool abAddPushText
) {
    bool bFound = false;
    if (apString && *apString && apOutBuffer && aiBufferSize) {
        int32_t iIndex = Interface::MatchTextReplaceConstantToGameSettings(apString);
        if (iIndex < 28) {
            Interface::GetControlPushTextFor(iIndex, apOutBuffer, abAddPushText);
            bFound = true;
        }
        if (!bFound) {
            TESGlobal* pGlob = static_cast<TESGlobal*>(TESForm::GetFormByEditorID(apString));
            if (pGlob && pGlob->GetFormType() == GLOB_ID) {
                if (pGlob->GetType() == 102) {
                    BSsprintf(apOutBuffer, static_cast<int32_t>(aiBufferSize), "%.2f", pGlob->fValue);
                }
                else {
                    BSsprintf(
                        apOutBuffer,
                        static_cast<int32_t>(aiBufferSize),
                        "%i",
                        static_cast<int32_t>(pGlob->fValue)
                    );
                }
                bFound = true;
            }
        }
        if (!bFound && !BSstricmp(apString, "PCName")) {
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), pPlayer->GetFullName());
            bFound = true;
        }
        if (!bFound && !BSstricmp(apString, "PCRace")) {
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), pPlayer->GetRaceFullName());
            bFound = true;
        }
        if (!bFound && !BSstricmp(apString, "PCSex")) {
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), pSexNameGS[pPlayer->GetSex()]->String());
            bFound = true;
        }
        if (!bFound && !BSstricmp(apString, "PCSexPronoun")) {
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), pSexPronounGS[pPlayer->GetSex()]->String());
            bFound = true;
        }
        if (!bFound && !BSstricmp(apString, "PCSexPossessive")) {
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), pSexPossessiveGS[pPlayer->GetSex()]->String());
            bFound = true;
        }
        if (!bFound) {
            return Interface::TestConstantForGameSettings(apString, apOutBuffer);
        }
    }

    return bFound;
}

float Interface::GetAspectRatioRatio() {
    return InterfaceManager::Instance()->fAspectRatioRatio;
}

void Interface::SetGunScopeVisible(bool abVisibleFlag) {
    HUDMainMenu::SetGunScopeVisible(abVisibleFlag);
}

void Interface::ClearGunScope() {
    HUDMainMenu::ClearGunScope();
}

float Interface::GetRenderedMenuScreenWidth() {
    return 1280.0f;
}

float Interface::GetRenderedMenuScreenHeight() {
    NiRenderer* pRenderer = NiRenderer::GetRenderer();
    NiRenderTargetGroup* pHeightGroup = pRenderer->GetDefaultRenderTargetGroup();
    NiRenderTargetGroup* pWidthGroup = pRenderer->GetDefaultRenderTargetGroup();
    double dWidth = static_cast<double>(pWidthGroup->GetWidth(0));
    double dAspectRatio = static_cast<double>(pHeightGroup->GetHeight(0)) / dWidth;
    return Interface::GetRenderedMenuScreenWidth() * dAspectRatio;
}

float Interface::SizeOfAScreenPixelInCrazyAssMenuCoordinates() {
    static float fSize = [&] {
        double dScreenHeight = Interface::GetScreenHeight();
        return static_cast<float>(dScreenHeight / Renderer::GetScreenHeight());
    }();
    return fSize;
}

void Interface::SetTextWasCreated(bool abSet) {
    InterfaceManager::Instance()->bTextWasCreated = abSet;
}

void Interface::SetRootMapSpace(TESWorldSpace* apNewSpace) {
    MapMenu::SetRootMapSpace(apNewSpace);
}

bool Interface::PreloadWorldMap() {
    return false;
}

void Interface::DrawHUDMenu3D() {
    HUDMainMenu::Draw3DElements();
}

bool Interface::IsGunScopeVisible() {
    return HUDMainMenu::IsGunScopeVisible();
}

bool Interface::Has360Controller() {
    return InterfaceManager::Has360Controller();
}

// The visibility table holds one flag per menu id; ids outside the menu range are never visible.
char Interface::IsMenuVisible(enumMenus aeMenu) {
    if (aeMenu < eMessageMenu || aeMenu > eTraitMenu) {
        return 0;
    }
    return static_cast<char>(Menu::pMenusVisible[aeMenu - eMessageMenu]);
}

// The main menus answer for each other, and a menu the player is dragging out of is never visible.
// InterfaceManager::Instance is called afresh for each use.
bool Interface::IsMenuIDVisible(enumMenus aeMenuID, uint32_t abFadeStatesThatCountAsVisible) {
    if (InterfaceManager::Instance() == nullptr || !InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    if (aeMenuID == eMainFour) {
        return IsPipBoyOpened() &&
            (IsMenuIDVisible(eStatsMenu, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(eInventoryMenu, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(ePipBoyRepair, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(eItemModMenu, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(ePipBoyData, abFadeStatesThatCountAsVisible));
    }
    if (abFadeStatesThatCountAsVisible != 0) {
        Tile* pMenuTile = Menu::GetMenuByClass(aeMenuID);
        if (pMenuTile != nullptr &&
            pMenuTile->GetMenu() != nullptr &&
            (pMenuTile->GetMenu()->GetFadeState() & static_cast<int32_t>(abFadeStatesThatCountAsVisible)) == 0) {
            return false;
        }
    }
    return IsMenuVisible(aeMenuID) != 0;
}

// Calls InterfaceManager::Instance afresh for each use.
Interface::enumMenus Interface::GetTopMenuID() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<enumMenus>(InterfaceManager::Instance()->GetEnterStackTop());
    }
    return eNoMenu;
}

// Calls InterfaceManager::Instance afresh for each use.
Interface::enumMenus Interface::GetMenuFromStack(int32_t aiIndex) {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<enumMenus>(InterfaceManager::Instance()->GetEnterStack(aiIndex));
    }
    return eNoMenu;
}

// Calls InterfaceManager::Instance afresh for each use.
Interface::enumMenus Interface::GetMenuModeType() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<enumMenus>(InterfaceManager::Instance()->GetEnterStack(0));
    }
    return eNoMenu;
}

uint32_t Interface::GetMenuCount() {
    return 10;
}

// Calls InterfaceManager::Instance afresh for each use.
bool Interface::IsTopMenuID(enumMenus aeMenuID) {
    if (InterfaceManager::Instance() == nullptr || !InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    int32_t iTop = InterfaceManager::Instance()->GetEnterStackTop();
    if (iTop == eMainFour &&
        (aeMenuID == eStatsMenu ||
         aeMenuID == eInventoryMenu ||
         aeMenuID == ePipBoyRepair ||
         aeMenuID == ePipBoyData ||
         aeMenuID == eItemModMenu))
    // The four main menus are all shown at once, so any of them counts as the top one.
    {
        return IsMenuIDVisible(aeMenuID, 0);
    }

    return iTop == aeMenuID;
}

void Interface::AddToEnterStack(int32_t aiID, [[maybe_unused]] bool abForceRemoval) {
    // The PC body never reads the removal flag its callers push.
    InterfaceManager::Instance()->AddToEnterStack(aiID);
}

int32_t Interface::PopFromEnterStack(int32_t aiID, bool abForceRemoval) {
    return InterfaceManager::Instance()->PopFromEnterStack(aiID, abForceRemoval);
}

void Interface::ClearTimer(void* apPointer) {
    InterfaceManager::Instance();
    InterfaceManager::ClearTimer(static_cast<Menu*>(apPointer));
}

void Interface::RecursiveFade(NiNode* apNode, float afAmount, float afLastAlpha) {
    InterfaceManager::Instance()->RecursiveFade(apNode, afAmount, afLastAlpha);
}

// Calls InterfaceManager::Instance afresh for each use.
bool Interface::IsInMenuMode() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->IsInMenuMode();
    }
    return false;
}

void Interface::ShowMenus() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        int32_t iFrontmost = static_cast<int32_t>(
            InterfaceManager::Instance()->GetMenusRoot()->GetFloat(Tile::kTileValue_MenuLevel)
        );
        if (!iFrontmost) {
            InterfaceManager::Instance()->PreLoadMenus();
            iFrontmost = 1003;
        }
        switch (iFrontmost) {
            case 1002:
            case 1035:
            case 1061: {
                Interface::SetInventoryMenuVisible(true, false, true);
                break;
            }
            case 1003: {
                Interface::SetStatsMenuVisible(true, false);
                break;
            }
            case 1023: {
                Interface::SetMapMenuVisible(true, false);
                break;
            }
            default: break;
        }
    }
}

// The console prints the line when it is open, or when CombatController's verbose messages are on.
// MenuConsole::Instance(true) is called again for the print, after the va_list is built.
void Interface::PrintLine(const char* apFormat, ...) {
    if (InterfaceManager::Instance() == nullptr) {
        return;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return;
    }
    if ((MenuConsole::Instance(false) != nullptr && MenuConsole::Instance(true)->IsVisible()) ||
        CombatController::GetShowVerboseMessages()) {
        va_list xlist;
        va_start(xlist, apFormat);
        MenuConsole::Instance(true)->PrintLine(apFormat, xlist);
        va_end(xlist);
    }
}

// KEEP: not registered. XFNV_REPLACES has no xfnv::describe overload for a C-variadic function
// (`void (*)(BSStringT<char>, ...)`), so the harness cannot register this body. The string's text goes to the
// console when it is open, or when CombatController's verbose messages are on, and the by-value string is released
// on every path (at a single join point).
void Interface::PrintLine(BSStringT<char> aString, ...) {
    if (InterfaceManager::Instance() != nullptr &&
        InterfaceManager::Instance()->IsDoneFirstInit() &&
        ((MenuConsole::Instance(false) != nullptr && MenuConsole::Instance(true)->IsVisible()) ||
         CombatController::GetShowVerboseMessages())) {
        va_list xlist;
        va_start(xlist, aString);
        // Takes the text before it fetches the console.
        const char* pText = aString.String();
        MenuConsole* pConsole = MenuConsole::Instance(true);
        pConsole->PrintLine(pText, xlist);
        va_end(xlist);
    }
}

// KEEP: not registered. XFNV_REPLACES cannot describe this signature (no matching xfnv::describe), so the harness
// cannot register the body; callers reach the exe's through the template forwarder in Interface.h. The result
// callback is the fourth argument, the sound name is null, the box is modal at -1/-1 and the opacity and width come
// after the first button's text.
bool Interface::CreateMessageMenu(
    const char* aText,
    const char* apcIconFilename,
    const char* apcTitle,
    void (*afpResult)(),
    int32_t aiFirstButtonResult,
    int32_t aiWarningContext,
    float afBackgroundTextureOpacityOverride,
    float afMenuWidthOverride,
    const char* axFirst,
    ...
) {
    if (InterfaceManager::Instance() == nullptr) {
        return false;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    va_list xlist;
    va_start(xlist, axFirst);
    bool bResult = MessageMenu::AddMessageToQueue(
        const_cast<char*>(aText),
        const_cast<char*>(apcIconFilename),
        const_cast<char*>(apcTitle),
        nullptr,
        afpResult,
        aiFirstButtonResult != 0,
        aiWarningContext,
        const_cast<char*>(axFirst),
        &xlist,
        true,
        -1,
        -1,
        0.0f,
        afBackgroundTextureOpacityOverride,
        afMenuWidthOverride
    );
    va_end(xlist);
    return bResult;
}

// KEEP: not registered, for the same reason as CreateMessageMenu above (it is C-variadic too). The only difference
// is that the sound name (the fourth argument) is passed through.
bool Interface::CreateMessageMenuSound(
    const char* aText,
    const char* apcIconFilename,
    const char* apcTitle,
    const char* apcSound,
    void (*afpResult)(),
    int32_t aiFirstButtonResult,
    int32_t aiWarningContext,
    float afBackgroundTextureOpacityOverride,
    float afMenuWidthOverride,
    const char* axFirst,
    ...
) {
    if (InterfaceManager::Instance() == nullptr) {
        return false;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    va_list xlist;
    va_start(xlist, axFirst);
    bool bResult = MessageMenu::AddMessageToQueue(
        const_cast<char*>(aText),
        const_cast<char*>(apcIconFilename),
        const_cast<char*>(apcTitle),
        const_cast<char*>(apcSound),
        afpResult,
        aiFirstButtonResult != 0,
        aiWarningContext,
        const_cast<char*>(axFirst),
        &xlist,
        true,
        -1,
        -1,
        0.0f,
        afBackgroundTextureOpacityOverride,
        afMenuWidthOverride
    );
    va_end(xlist);
    return bResult;
}

// KEEP: not registered. XFNV_REPLACES has no xfnv::describe overload for a C-variadic function
// (`char (*)(const char*, int, int, const char*, ...)`), so the harness cannot register this body. The result byte
// is cleared to -1, the fade to black is removed at once, the box is queued with ModalMessageMenuCallback, and the
// USER32 queue is pumped with PM_REMOVE until that callback answers. The result byte is returned as it stands, so the
// return type is the byte's, not bool.
char Interface::CreateModalMessageMenu(
    const char* aText,
    int32_t aiFirstButtonResult,
    int32_t aiWarningContext,
    const char* axFirst,
    ...
) {
    iModalMessageMenuResult = -1;
    bool bQueued = false;
    pFaderManager->RemoveFader(FADER_TO_BLACK, true);
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        va_list xlist;
        va_start(xlist, axFirst);
        bQueued = MessageMenu::AddMessageToQueue(
            aText,
            nullptr,
            nullptr,
            nullptr,
            &ModalMessageMenuCallback,
            aiFirstButtonResult != 0,
            aiWarningContext,
            axFirst,
            &xlist,
            true,
            -1,
            -1,
            0.0f,
            0.0f,
            0.0f
        );
        va_end(xlist);
    }
    if (!bQueued) {
        return 0;
    }
    while (iModalMessageMenuResult == -1) {
        // Pumps the window's messages into SDL's event queue; WinMain's loop hands them to WinProc_SDL.
        SDL_PumpEvents();
        appMain->GetControls()->UpdateValues();
        Interface::PreIdleStuff();
        Interface::Idle();
        Interface::PostIdleStuff();
        Interface::Click(nullptr, nullptr);
        Interface::SwapBuffers();
    }
    return iModalMessageMenuResult;
}

void Interface::CloseMessageMenu() {
    MessageMenu::ClearMessageQueueHead();
}

Tile* Interface::GetMenusRoot() {
    return InterfaceManager::Instance()->GetMenusRoot();
}

NiNode* Interface::GetInterfaceRoot() {
    return InterfaceManager::Instance()->GetInterfaceRoot();
}

// While the pip-boy's rendered menu is up the interface is not rendering into its own scene graph.
bool Interface::IsInRenderedMenu() {
    if (IsInPipboyMenu() && !GetPipboy()->GetInitialized()) {
        return false;
    }
    return InterfaceManager::Instance()->IsInRenderedMenu();
}

// The manager is fetched and dropped; the check itself is static.
bool Interface::IsCurrentRenderedMenuTopmost() {
    InterfaceManager::Instance();
    return InterfaceManager::IsCurrentRenderedMenuTopmost();
}

// The texts after axFirstText are read twice: once to build and measure the buttons, once to place them.
void Interface::InitializeTabline(Tile* apTabline, int32_t aiStartingID, const char* axFirstText, ...) {
    BSStringT<char> sText;
    int32_t iTabsTotalWidth = 0;
    va_list vList;
    va_start(vList, axFirstText);
    sText.Set(axFirstText, 0);
    int32_t iTabCt = 0;
    for (; sText.GetLength(); ++iTabCt) {
        Tile* pTemp = apTabline->GetMenu()->RenderTemplate(apTabline, "TabButtonTemplate", nullptr);
        pTemp->Set(4036, sText.String(), true);
        pTemp->SetName(sText.String());
        pTemp->Set(Tile::kTileValue_ListIndex, iTabCt);
        pTemp->Set(Tile::kTileValue_ID, aiStartingID + iTabCt);
        Tile::UpdateAll(false);
        iTabsTotalWidth += static_cast<int32_t>(pTemp->GetFloat(Tile::kTileValue_Width));
        sText = va_arg(vList, const char*);
    }
    va_end(vList);
    int32_t iLeftLineLength = static_cast<int32_t>(
        (apTabline->GetFloat(Tile::kTileValue_Width) - iTabsTotalWidth) / (iTabCt + 1)
    );
    int32_t iCurrX = iLeftLineLength;
    apTabline->Set(Tile::TextToTrait("_LeftLineLength"), iLeftLineLength);
    apTabline->Set(Tile::TextToTrait("_ButtonCount"), iTabCt);
    va_start(vList, axFirstText);
    sText.Set(axFirstText, 0);
    while (sText.GetLength()) {
        Tile* pTemp = apTabline->GetChildByName(sText.String());
        pTemp->Set(Tile::TextToTrait("_x"), iCurrX);
        iCurrX += static_cast<int32_t>(pTemp->GetFloat(Tile::kTileValue_Width)) + iLeftLineLength;
        sText = va_arg(vList, const char*);
    }
    va_end(vList);
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
FOPipboyManager* Interface::GetPipboy() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->GetPipboy();
    }
    return nullptr;
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
FOVATSEffectManager* Interface::GetVATSEffect() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->GetVATSEffect();
    }
    return nullptr;
}

FOHUDEffectManager* Interface::GetHUDEffects() {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return nullptr;
    }
    return &InterfaceManager::Instance()->HUDEffects;
}

FORenderedMenu* Interface::GetCurrentRenderedMenu() {
    return InterfaceManager::Instance()->GetCurrentRenderedMenu();
}

// InterfaceManager::Instance is called afresh for the test and the flag.
bool Interface::IsInPipboyMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return IsPipBoyOpening() || IsPipBoyOpened();
    }
    return false;
}

// The pip-boy is "opening" while its screen is being built and "opened" once it is up.
bool Interface::IsPipBoyOpening() {
    return InterfaceManager::Instance()->ePipboyState == 2;
}

bool Interface::IsPipBoyOpened() {
    return InterfaceManager::Instance()->ePipboyState == 3;
}

bool Interface::IsDebugTextVisible() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->IsDebugTextVisible();
    }
    return false;
}

bool Interface::IsConsoleVisible() {
    if (InterfaceManager::Instance() == nullptr || !InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    if (MenuConsole::Instance(false) == nullptr) {
        return false;
    }
    return MenuConsole::Instance(true)->IsVisible();
}

TESObjectREFR* Interface::GetPickREFR() {
    InterfaceManager* pManager = InterfaceManager::Instance();
    if (pManager != nullptr && pManager->IsDoneFirstInit()) {
        return pManager->GetPickedREFR();
    }
    return nullptr;
}

Tile* Interface::GetCursorTile() {
    return InterfaceManager::Instance()->GetCursorTile();
}

// The interface must not keep pointing at a tile that is going away.
void Interface::TileIsBeingDeleted(Tile* apTile) {
    InterfaceManager* pManager = InterfaceManager::Instance();
    if (pManager->pOverTileTarget == apTile) {
        pManager->pOverTileTarget = nullptr;
        pManager->pOverTileMenu = nullptr;
    }
    if (pManager->pDragOverTileTarget == apTile) {
        pManager->pDragOverTileTarget = nullptr;
        pManager->pDragOverTileMenu = nullptr;
    }
    if (pManager->GetCurrentFocusTarget() == apTile) {
        pManager->ClearCurrentFocusTarget();
    }
    pManager->RemoveTileFromUpdateList(apTile);
}

void Interface::RefreshActivateREFR() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->RefreshActivateREFR();
    }
}

void Interface::SetPickLength(uint32_t auiLength) {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->SetPickDistance(static_cast<int32_t>(auiLength));
    }
}

void Interface::SetCurrentFocusTarget(Tile* apTile) {
    InterfaceManager::Instance()->SetCurrentFocusTarget(apTile, Tile::kTileValue_Mouseover, true);
}

void Interface::DisableMessageMenu(bool abDisable) {
    MessageMenu_SetDisabled(abDisable);
}

char Interface::GetLoadingMenuVisible() {
    return IsMenuVisible(eLoadingMenu);
}

void Interface::AddMenuFade(Menu* apMenu, float afSecs) {
    InterfaceManager::NewTimer(apMenu, afSecs);
}

void Interface::UpdateAllTimers() {
    InterfaceManager::Instance()->UpdateAllTimers();
}

void Interface::SetNeedToUpdate(bool abSet) {
    InterfaceManager::Instance()->NeedToUpdate(abSet);
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::Accumulate() {
    if (InterfaceManager::Instance() == nullptr || !InterfaceManager::Instance()->IsDoneFirstInit()) {
        return;
    }
    if (!pMoviePlayer->GetPlayingSequence()) {
        InterfaceManager::Instance()->Accumulate();
    }
}

void Interface::AddAlphaProperty(NiAVObject* apObject) {
    apObject->AttachProperty(InterfaceManager::Instance()->GetAlphaProp());
}

void Interface::ClearTempModel() {
    if (pTmpModel != nullptr) {
        delete pTmpModel;
    }
    pTmpModel = nullptr;
}

// The model loader's copy when the loader is up and the interface is not inside LastMinuteUpdate; otherwise the file
// is loaded through a stack BSStream into a new pTmpModel, freeing the previous one.
NiNode* Interface::LoadModelFile(const char* apFilename, bool& arbFromModelLoader) {
    arbFromModelLoader = false;
    if (pModelLoader != nullptr && !bLastMinuteUpdate) {
        arbFromModelLoader = true;
        return pModelLoader->LoadFile(apFilename, LOD_MULT_NONE, true, false, false, false);
    }
    if (!FileFinder::Locate(apFilename, nullptr, FileFinder::SKIP_NONE, FileFinder::ARCHIVE_TYPE_ANY)) {
        return nullptr;
    }
    alignas(BSStream) uint8_t aStreamStorage[sizeof(BSStream)];
    BSStream* pStream = ::new (aStreamStorage) BSStream;
    // Calls BSStream's Load directly: the stream is a local object.
    if (!pStream->BSStream::Load(apFilename, nullptr)) {
        pStream->~BSStream();
        return nullptr;
    }
    Model* pOldModel = pTmpModel;
    if (pOldModel != nullptr) {
        delete pOldModel;
    }
    pTmpModel = new Model(apFilename, pStream, true, false);
    NiNode* pRoot = pTmpModel->GetModel();
    pStream->~BSStream();
    return pRoot;
}

NiNode* Interface::CopyOrDeepCopyNode(NiNode* apNode) {
    BSCloningProcess kCloneProc(1.0f);
    if (HasMorpherController(apNode)) {
        return static_cast<NiNode*>(TES::pInstance->CreateDeepCopySameTextures(apNode, kCloneProc));
    }

    return static_cast<NiNode*>(apNode->Clone(kCloneProc));
}

bool Interface::PlayNifSounds(NiControllerSequence* apSequence) {
    return BSAnimGroupSequence::PlaySounds(apSequence, nullptr);
}

// InterfaceManager::Instance is called afresh for the test and the flag.
bool Interface::InDialog() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return DialogMenu::IsDialogActive();
    }
    return false;
}

void Interface::SetDialogPause(bool abPause) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        TileMenu* pDialog = static_cast<TileMenu*>(Menu::GetMenuByClass(1009));
        DialogMenu* pDialogMenu = nullptr;
        if (pDialog) {
            pDialogMenu = dynamic_cast<DialogMenu*>(pDialog->Tile::GetMenu());
        }
        if (pDialogMenu) {
            pDialogMenu->SetPause(abPause);
        }
    }
}

// Clears the HUD's messages and subtitles once the interface is up.
void Interface::ClearAllSubtitlesAndMessages() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit() && HUDMainMenu::pme) {
        HUDMainMenu::pme->ClearAllSubtitlesAndMessages();
    }
}

bool Interface::ShowMessage(
    const char* apString,
    int32_t aeEmotion,
    const char* apIconFilename,
    const char* apSoundName,
    float afShowSeconds,
    bool abMoveToHead
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp",
        2305
    );
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit() && HUDMainMenu::pme) {
        // Interface::ShowMessage takes aeEmotion as an int and passes it on as the HUD's emotion.
        HUDMainMenu::pme->ShowMessage(
            apString,
            static_cast<HUDMainMenu::HUD_MESSAGE_EMOTION>(aeEmotion),
            apIconFilename,
            apSoundName,
            afShowSeconds,
            abMoveToHead
        );
    }
    return false;
}

void Interface::SetRegionText(TESRegion* apRegion, BSStringT<char> xDisplayText, bool bUpdateLastRegion) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        HUDMainMenu::HUDRegionData kRegionData;
        kRegionData.bUpdateLastRegion = bUpdateLastRegion;
        kRegionData.pRegion = apRegion;
        if (xDisplayText.pString) {
            BSstrcpy(kRegionData.strDisplayText, 260, xDisplayText.pString);
        }
        else {
            kRegionData.strDisplayText[0] = 0;
        }
        TESRegion::UpdateHUDRegionIndicator(kRegionData);
    }
}

void Interface::CreateSleepMenu(bool abSleep) {
    if (InterfaceManager::Instance() &&
        InterfaceManager::Instance()->bFirstInit &&
        Interface::IsPipboyClosed()) {
        Tile* pTile = Menu::GetMenuByClass(1012);
        SleepWaitMenu* pMenu = nullptr;
        if (pTile) {
            pMenu = dynamic_cast<SleepWaitMenu*>(pTile->GetMenu());
        }
        if (pMenu) {
            delete pMenu;
        }
        SleepWaitMenu::Create(abSleep);
    }
}

Tile* Interface::CreateMapMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return MapMenu::Create();
    }
    return nullptr;
}

Tile* Interface::CreateRepairMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return RepairMenu::Create();
    }
    return nullptr;
}

Tile* Interface::CreateItemModMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return ItemModMenu::Create();
    }
    return nullptr;
}

Tile* Interface::CreateInventoryMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InventoryMenu::Create();
    }
    return nullptr;
}

Tile* Interface::CreateStatsMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return StatsMenu::Create();
    }
    return nullptr;
}

// InterfaceManager::Instance is called afresh for the test and the flag.
void Interface::CreateStartMenu() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        StartMenu::Create(false, false);
    }
}

void Interface::CreateLoadingMenu(TESWorldSpace* apWorldSpace, bool abScripted) {
    DisableMessageMenu(true);
    LoadingMenu::Create(apWorldSpace, true, abScripted);
}

void Interface::CloseLoadingMenu() {
    // the menu is read once; its destructor clears the global
    delete LoadingMenu::pMe;
    DisableMessageMenu(false);
}

// InterfaceManager::Instance is deliberately called twice, once for the null test and once for the receiver.
void Interface::CloseContainer() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        ContainerMenu::Close();
    }
}

void Interface::ForceTextureRelease() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->ForceTextureRelease();
    }
}

bool Interface::GetFirstChanceTextureRelease() {
    return InterfaceManager::Instance()->GetFirstChanceTextureRelease();
}

void Interface::SetFirstChanceTextureRelease(bool abSet) {
    InterfaceManager::Instance()->SetFirstChanceTextureRelease(abSet);
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::ClearReferencePointers(TESObjectREFR* apObject) {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->ClearReferencePointers(apObject);
    }
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::PreIdleStuff() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->PreIdleStuff();
    }
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::Idle() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->Idle();
    }
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::PostIdleStuff() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->PostIdleStuff();
    }
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::LastMinuteUpdate() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->LastMinuteUpdate();
    }
}

void Interface::HideMenus() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager* pim = InterfaceManager::Instance();
        TileMenu* pStat = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
        TileMenu* pInv = static_cast<TileMenu*>(Menu::GetMenuByClass(1002));
        TileMenu* pRepair = static_cast<TileMenu*>(Menu::GetMenuByClass(1035));
        TileMenu* pItemMod = static_cast<TileMenu*>(Menu::GetMenuByClass(1061));
        TileMenu* pMap = static_cast<TileMenu*>(Menu::GetMenuByClass(1023));
        if (pStat) {
            pStat->GetMenu()->SetFadeState(Interface::FADE_HIDDEN);
            pStat->GetMenu()->Menu::GetMenuTile()->Set(Tile::kTileValue_Visible, 0);
        }
        if (pInv) {
            pInv->GetMenu()->SetFadeState(Interface::FADE_HIDDEN);
            pInv->GetMenu()->Menu::GetMenuTile()->Set(Tile::kTileValue_Visible, 0);
        }
        if (pRepair) {
            pRepair->GetMenu()->SetFadeState(Interface::FADE_HIDDEN);
            pRepair->GetMenu()->Menu::GetMenuTile()->Set(Tile::kTileValue_Visible, 0);
        }
        if (pItemMod) {
            pItemMod->GetMenu()->SetFadeState(Interface::FADE_HIDDEN);
            pItemMod->GetMenu()->Menu::GetMenuTile()->Set(Tile::kTileValue_Visible, 0);
        }
        if (pMap) {
            pMap->GetMenu()->SetFadeState(Interface::FADE_HIDDEN);
            pMap->GetMenu()->Menu::GetMenuTile()->Set(Tile::kTileValue_Visible, 0);
        }
        Interface::CloseContainer();
        if (StartMenu::PauseMenuIsActive()) {
            StartMenu::Close(true);
        }
        pim->EnterRenderedMenu(nullptr);
    }
}

// Isolates the glow (or pip-boy) elements of every menu under the menus root and culls the debug text and the cursor
// unless neither is asked for. InterfaceManager::Instance is called afresh at each use.
void Interface::IsolateMenuElements(bool abGlowOnly, bool abPipBoyOnly) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager* pim = InterfaceManager::Instance();
        if (abGlowOnly && !abPipBoyOnly) {
            pim->xGlowBranchTiles.RemoveAll();
        }
        Tile* pMenusRoot = InterfaceManager::Instance()->GetMenusRoot();
        NiTListItem<Tile*>* pos = pMenusRoot->xChildren.m_pkHead;
        while (pos) {
            Tile* pmenutile = pMenusRoot->xChildren.GetNext(pos);
            Interface::IsolateElementsOfMenu(pmenutile, abGlowOnly, abPipBoyOnly, true);
        }
        if (!abGlowOnly && !abPipBoyOnly) {
            if (InterfaceManager::Instance()->GetDebugTextRoot()) {
                InterfaceManager::Instance()->GetDebugTextRoot()->SetAppCulled(false);
            }
            if (InterfaceManager::Instance()->GetCursorRoot()) {
                InterfaceManager::Instance()->GetCursorRoot()->SetAppCulled(false);
            }
        }
        else {
            if (InterfaceManager::Instance()->GetDebugTextRoot()) {
                InterfaceManager::Instance()->GetDebugTextRoot()->SetAppCulled(true);
            }
            if (InterfaceManager::Instance()->GetCursorRoot()) {
                InterfaceManager::Instance()->GetCursorRoot()->SetAppCulled(true);
            }
        }
    }
}

// Makes every menu and its glow (ID 110) and no-glow (ID 111) children visible again and culls each model by its
// visibility traits. Calls InterfaceManager::Instance afresh at each use.
void Interface::RestoreMenuElements() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        Tile* pMenusRoot = InterfaceManager::Instance()->GetMenusRoot();
        NiTListItem<Tile*>* pos = pMenusRoot->xChildren.m_pkHead;
        while (pos) {
            Tile* pmenutile = pMenusRoot->xChildren.GetNext(pos);
            Tile* pglow = pmenutile->GetChildByID(110);
            Tile* pnoglow = pmenutile->GetChildByID(111);
            pmenutile->Set(Tile::kTileValue_IsolatedVisible, 1);
            if (pglow) {
                pglow->Set(Tile::kTileValue_IsolatedVisible, 1);
            }
            if (pnoglow) {
                pnoglow->Set(Tile::kTileValue_IsolatedVisible, 1);
            }
            if (pmenutile->GetModel()) {
                bool bCull = pmenutile->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pmenutile->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pmenutile->GetModel()->SetAppCulled(bCull);
            }
            if (pglow && pglow->GetModel()) {
                bool bCull = pglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pglow->GetModel()->SetAppCulled(bCull);
            }

            if (pnoglow && pnoglow->GetModel()) {
                bool bCull = pnoglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pnoglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pnoglow->GetModel()->SetAppCulled(bCull);
            }
        }
    }
}

void Interface::EmergencyCloseAllMenusAndBreakStuff() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->EmergencyCloseAllMenusAndBreakStuff();
    }
}

// Calls InterfaceManager::Instance afresh for the test, the flag and the call.
void Interface::ResetPipboy() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->ReinitialisePipboyManager();
    }
}

Menu* Interface::CreateMenuByClass(int32_t aiMenuClass) {
    return MenuManager::Instance(true)->CreateMenuByClass(aiMenuClass);
}

// The tabs are clamped to the menus' four pages through BSMAX and BSMIN; each step fetches the interface manager again.
// A main menu id the interface does not know falls back to the stats menu.
void Interface::SetMainMenuDefaults(char aiStatsTab, char aiInventoryTab, char aiMapTab, int32_t aiWhichMenu) {
    if (InterfaceManager::Instance() == nullptr || !InterfaceManager::Instance()->IsDoneFirstInit()) {
        return;
    }
    char cTab = static_cast<char>(BSMIN(4, BSMAX_2(0, aiStatsTab)));
    InterfaceManager::Instance()->SetStatsPageNumber(cTab);
    cTab = static_cast<char>(BSMIN(4, BSMAX_2(0, aiInventoryTab)));
    InterfaceManager::Instance()->SetInventoryPageNumber(cTab);
    cTab = static_cast<char>(BSMIN(4, BSMAX_2(0, aiMapTab)));
    InterfaceManager::Instance()->SetMapPageNumber(cTab);
    if (aiWhichMenu != eStatsMenu && aiWhichMenu != eInventoryMenu && aiWhichMenu != ePipBoyData) {
        aiWhichMenu = eStatsMenu;
    }
    InterfaceManager::Instance()->SetCurrentMainMenu(aiWhichMenu);
}

char Interface::GetStatsMenuVisible() {
    return Interface::IsMenuVisible(Interface::eStatsMenu);
}

void Interface::SwapBuffers() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        pMoviePlayer->GetPlayingSequence();
    }
}

void Interface::UpdateScreenShape(int32_t isizex, int32_t isizey) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->UpdateScreenShape(isizex, isizey);
    }
}

void Interface::SetMenuSystemVisible(bool abVisible) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->GetMenusRoot()->GetModel()->NiAVObject::SetAppCulled(!abVisible);
    }
}

void Interface::ToggleSafeZone(int32_t abForceSetTo) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->ToggleSafeZone(abForceSetTo);
    }
}

char Interface::GetInventoryMenuVisible() {
    return Interface::IsMenuVisible(Interface::eInventoryMenu);
}

void Interface::DumpMenuPackingInfo() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->DumpMenuPackingInfo();
    }
}

char Interface::GetStatsPageNumber() {
    if (!InterfaceManager::Instance()) {
        return -1;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return -1;
    }
    return InterfaceManager::Instance()->GetStatsPageNumber();
}

char Interface::GetMagicPageNumber() {
    if (!InterfaceManager::Instance()) {
        return -1;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return -1;
    }
    return InterfaceManager::Instance()->GetMagicPageNumber();
}

char Interface::GetMapPageNumber() {
    if (!InterfaceManager::Instance()) {
        return -1;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return -1;
    }
    return InterfaceManager::Instance()->GetMapPageNumber();
}

void Interface::SetSurgeryCursorPos(int32_t aiXPos, int32_t aiYPos) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        SurgeryMenu::SetCursorPos(aiXPos, aiYPos);
    }
}

void Interface::CreateLevelUpMenu() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        LevelUpMenu::Create();
    }
}

void Interface::CreateCharGenMenu(
    int32_t aiItemType,
    int32_t aiMaxPoints,
    bool abOnlyAddValues,
    bool abShowTaggedSkills
) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        CharGenMenu::Create(aiItemType, aiMaxPoints, abOnlyAddValues, abShowTaggedSkills);
    }
}

NiNode* Interface::GetDebugTextRoot() {
    return InterfaceManager::Instance()->GetDebugTextRoot();
}

void Interface::ReleaseModelFile(const char* apFilename) {
    if (pModelLoader) {
        pModelLoader->ReleaseFile(apFilename);
    }
}

void Interface::DrawSurgeryMenu3D() {
    SurgeryMenu::Draw3DElements();
}

bool Interface::GetIsVATSRescanning() {
    return VATSMenu::GetIsARescan();
}

bool Interface::IsActivatePickFuzzy() {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    return InterfaceManager::Instance()->IsActivatePickFuzzy();
}

void Interface::ClearMenuQueue() {
    Interface::QueuedMenuOpen.eOpenType = Interface::OPEN_NONE;
    Interface::QueuedMenuOpen.pMenuRef = nullptr;
    Interface::QueuedMessageOpen.eOpenType = Interface::OPEN_NONE;
    Interface::QueuedMessageOpen.pMenuRef = nullptr;
}

void Interface::SetCrosshairRef(intptr_t aiCrosshairRef) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->SetCrosshairRef(aiCrosshairRef);
    }
}

void Interface::Free() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Free();
    }
}

void Interface::CreateRecipeMenu(TESObjectREFR* apVendor, TESForm* pCategory) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        RecipeMenu::Create(apVendor, static_cast<TESRecipeCategory*>(pCategory));
    }
}

InterfaceManager*
Interface::SetPipboyPickResults(uint8_t aucOverPipBoyScreen, float afMouseX, float afMouseY, int32_t aiMenuToSet) {
    return InterfaceManager::Instance()->SetPipboyPickResults(aucOverPipBoyScreen, afMouseX, afMouseY, aiMenuToSet);
}

void Interface::MessageMenuYesToAll() {
    MessageMenu::ProcessYesToAll();
}

TESObjectREFR* Interface::GetActivateREFR() {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return nullptr;
    }
    return InterfaceManager::Instance()->pActivateRef;
}

void Interface::CloseSleepMenu() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        TileMenu* pTile = static_cast<TileMenu*>(Menu::GetMenuByClass(1012));
        SleepWaitMenu* pMenu = nullptr;
        if (pTile) {
            pMenu = dynamic_cast<SleepWaitMenu*>(pTile->Tile::GetMenu());
        }
        if (pMenu) {
            SleepWaitMenu::Close();
        }
    }
}

void Interface::CloseVATSMenu() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        TileMenu* pTile = static_cast<TileMenu*>(Menu::GetMenuByClass(1056));
        VATSMenu* pMenu = nullptr;
        if (pTile) {
            pMenu = dynamic_cast<VATSMenu*>(pTile->Tile::GetMenu());
        }
        if (pMenu) {
            VATSMenu::Close();
        }
    }
}

// Closes every support menu: walks the menus root's child tiles and removes the tile of every menu that is not
// always up. The start menu's tile goes only while its pause menu is up; the SPECIAL book and love tester menus run
// their own close code first.
void Interface::CloseAllSupportMenus([[maybe_unused]] bool abExceptSubtitleMenu) {
    bClosingAllSupportMenus = 1;
    Tile* pMenusRoot = InterfaceManager::Instance()->GetMenusRoot();
    NiTListItem<Tile*>* pNode = pMenusRoot->xChildren.GetHeadPos();
    while (pNode != nullptr) {
        Tile* pTile = pMenusRoot->xChildren.GetNext(pNode);
        if (pTile == nullptr) {
            continue;
        }
        // GetMenu is called twice: once for the null test, once for the class.
        if (pTile->GetMenu() == nullptr) {
            continue;
        }
        switch (pTile->GetMenu()->GetClass()) {
            case eInventoryMenu:
            case eStatsMenu:
            case eHUDMainMenu:
            case eLoadingMenu:
            case ePipBoyData:
            case ePipBoyRepair:
            case eItemModMenu: continue;
            case eStartMenu:
                if (StartMenu::PauseMenuIsActive()) {
                    Tile* pRemoved = pMenusRoot->xChildren.Remove(pTile);
                    if (pRemoved != nullptr) {
                        delete pRemoved;
                    }
                }
                break;
            case eSPECIALBookMenu: {
                SPECIALBookMenu* pBookMenu = dynamic_cast<SPECIALBookMenu*>(pTile->GetMenu());
                if (pBookMenu != nullptr) {
                    pBookMenu->Exit();
                }
                Tile* pRemoved = pMenusRoot->xChildren.Remove(pTile);
                if (pRemoved != nullptr) {
                    delete pRemoved;
                }
                break;
            }
            case eLoveTesterMenu: {
                LoveTesterMenu* pLoveTesterMenu = dynamic_cast<LoveTesterMenu*>(pTile->GetMenu());
                if (pLoveTesterMenu != nullptr) {
                    pLoveTesterMenu->Close();
                }
                Tile* pRemoved = pMenusRoot->xChildren.Remove(pTile);
                if (pRemoved != nullptr) {
                    delete pRemoved;
                }
                break;
            }
            default: {
                Tile* pRemoved = pMenusRoot->xChildren.Remove(pTile);
                if (pRemoved != nullptr) {
                    delete pRemoved;
                }
                break;
            }
        }
    }
    bClosingAllSupportMenus = 0;
}

void Interface::SaveGame() {
    int8_t cStatsMenuTab = Interface::GetStatsPageNumber();
    int8_t cInventoryMenuTab = Interface::GetInventoryPageNumber();
    int8_t cMagicMenuTab = Interface::GetMagicPageNumber();
    int8_t cMapMenuTab = Interface::GetMapPageNumber();
    int32_t iLastPipboyTab = Interface::GetCurrentMainMenu();
    if (cStatsMenuTab < 1) {
        cStatsMenuTab = 1;
    }
    if (cInventoryMenuTab < 1) {
        cInventoryMenuTab = 1;
    }
    if (cMagicMenuTab < 1) {
        cMagicMenuTab = 1;
    }
    if (cMapMenuTab < 1) {
        cMapMenuTab = 1;
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cStatsMenuTab, sizeof(cStatsMenuTab));
    pSaveLoadGameOLD->SaveGameDataOLD(&cInventoryMenuTab, sizeof(cInventoryMenuTab));
    pSaveLoadGameOLD->SaveGameDataOLD(&cMagicMenuTab, sizeof(cMagicMenuTab));
    pSaveLoadGameOLD->SaveGameDataOLD(&cMapMenuTab, sizeof(cMapMenuTab));
    pSaveLoadGameOLD->SaveGameDataOLD(&iLastPipboyTab, sizeof(iLastPipboyTab));
}

bool Interface::ContainerIsRecipe() {
    return Menu::GetMenuByClass(1077) != nullptr;
}

bool Interface::ContainerIsBarter() {
    return Menu::GetMenuByClass(1053) != nullptr;
}

void Interface::SetDragDroppingState(int32_t abSetTo) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->SetDragDroppingState(abSetTo);
    }
}

void Interface::UpdateInventory() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        if (Interface::GetInventoryMenuVisible()) {
            InventoryMenu::UpdateList();
        }
        else if (Interface::GetStatsMenuVisible()) {
            Tile* pStat = Menu::GetMenuByClass(1003);
            StatsMenu* pStatsMenu = static_cast<StatsMenu*>(pStat->GetMenu());
            pStatsMenu->UpdateHealingOptions(StatsMenu::SM_HO_COUNT);
        }
        else if (Interface::IsMenuIDVisible(Interface::eBarterMenu, 0)) {
            BarterMenu::ForceBarterUpdate(true);
        }
        else if (Interface::IsMenuIDVisible(Interface::eRecipeMenu, 0)) {
            RecipeMenu::ForceRecipeUpdate(true);
        }
        else if (Interface::IsMenuIDVisible(Interface::eContainerMenu, 0)) {
            ContainerMenu::ForceContainerUpdate();
        }
    }
}

void Interface::SetItemModMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        TileMenu* pStat = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
        TileMenu* pInv = static_cast<TileMenu*>(Menu::GetMenuByClass(1002));
        TileMenu* pRepair = static_cast<TileMenu*>(Menu::GetMenuByClass(1035));
        TileMenu* pMap = static_cast<TileMenu*>(Menu::GetMenuByClass(1023));
        TileMenu* pItemMod = static_cast<TileMenu*>(Menu::GetMenuByClass(1061));
        uint8_t ucIsVis = Interface::IsMenuIDVisible(
            Interface::eItemModMenu,
            abAnyVisibleState ? Interface::FADE_NOT_HIDDEN : 0
        );
        if (abVisibility) {
            if (!pItemMod) {
                pItemMod = static_cast<TileMenu*>(Interface::CreateItemModMenu());
            }
            if (!ucIsVis) {
                if (pStat) {
                    pStat->GetMenu()->StartFadeOut();
                    StatsMenu::ResetHealingMode();
                }
                if (pMap) {
                    pMap->GetMenu()->StartFadeOut();
                }
                if (pRepair) {
                    pRepair->GetMenu()->StartFadeOut();
                }
                if (pItemMod) {
                    pItemMod->GetMenu()->StartFadeIn();
                }
                if (pInv) {
                    pInv->GetMenu()->StartFadeOut();
                }
            }
            if (pItemMod) {
                InterfaceManager::Instance()->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1061);
            }
            if (abTriggerPipboyEffect) {
                InterfaceManager::Instance()->GetPipboy()->TriggerRandomEffect();
                InterfaceManager::Instance()->GetPipboy()->ActivateButton(2u);
            }
            if (InterfaceManager::Instance()->GetPipboy()->GetInitialized()) {
                InventoryMenu::SetKnobPosition(false);
            }
        }
        else if (ucIsVis && pItemMod) {
            pItemMod->GetMenu()->StartFadeOut();
        }
    }
}

int32_t Interface::GetDragDroppingState() {
    if (!InterfaceManager::Instance()) {
        return 0;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return 0;
    }
    return static_cast<int32_t>(InterfaceManager::Instance()->GetDragDroppingState());
}

void Interface::GetControlPushTextFor(int32_t aiIndex, char* axReturn, bool abAddPushText) {
    if (aiIndex >= 0 && aiIndex <= 28) {
        SettingT<GameSettingCollection>* pButtonSetting = nullptr;
        *axReturn = 0;
        if (InterfaceManager::Has360Controller()) {
            Controls* pControls = appMain->GetControls();
            pButtonSetting = psXBoxGamepadText[pControls->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 3u)];
        }
        else {
            uint8_t ucButton = appMain->GetControls()->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 1u);
            if (ucButton >= 9u) {
                ucButton = appMain->GetControls()->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 0);
                if (ucButton >= 238u) {
                    ucButton = appMain->GetControls()->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 2u);
                    if (ucButton < 8u) {
                        pButtonSetting = psJoystick[ucButton];
                    }
                }
                else {
                    pButtonSetting = static_cast<SettingT<GameSettingCollection>*>(psKeyboard[ucButton]);
                }
            }
            else {
                pButtonSetting = psMouse[ucButton];
            }
        }
        if (pButtonSetting && pButtonSetting->String()) {
            ourStrcpy(axReturn, pButtonSetting->String());
        }

        if (abAddPushText) {
            BSStringT<char> xTemp;
            if (*axReturn) {
                xTemp = axReturn;
            }
            else {
                xTemp = sPCControlsTextNone.String();
            }
            if (pButtonSetting && pButtonSetting->Key() && BSstristr(pButtonSetting->Key(), "trigger") != nullptr) {
                std::sprintf(axReturn, "%s %s", sPCControlsTriggerPrefix.String(), xTemp.String());
            }
            else {
                std::sprintf(axReturn, "%s %s", sPCControlsTextPrefix.String(), xTemp.String());
            }
        }
    }
}

// Shows only the glow (or the no-glow) children of apMenuTile and, with abPipBoyOnly, only a pip-boy menu (ID 112).
// apMenuTile's children are walked with the menus root's child list as GetNext's receiver (it only advances the
// position); this receiver is kept deliberately.
void Interface::IsolateElementsOfMenu(
    Tile* apMenuTile,
    bool abGlowOnly,
    bool abPipBoyOnly,
    bool abUpdateGlowBranchList
) {
    if (apMenuTile) {
        Tile* pMenusRoot = InterfaceManager::Instance()->GetMenusRoot();
        InterfaceManager* pim = InterfaceManager::Instance();
        bool bIsPipBoy = apMenuTile->GetFloat(Tile::kTileValue_ID) == 112.0;
        if ((!abPipBoyOnly || bIsPipBoy) && (abPipBoyOnly || !bIsPipBoy)) {
            apMenuTile->Set(Tile::kTileValue_IsolatedVisible, 1);
        }
        else {
            apMenuTile->Set(Tile::kTileValue_IsolatedVisible, 0);
        }
        NiTListItem<Tile*>* pos = apMenuTile->xChildren.m_pkHead;
        while (pos) {
            Tile* ptile = pMenusRoot->xChildren.GetNext(pos);
            Tile* pglow = ptile->GetFloat(Tile::kTileValue_ID) == 110.0 ? ptile : nullptr;
            Tile* pnoglow = ptile->GetFloat(Tile::kTileValue_ID) == 111.0 ? ptile : nullptr;
            if (pglow) {
                if (abGlowOnly) {
                    pglow->Set(Tile::kTileValue_IsolatedVisible, 1);
                    if (abUpdateGlowBranchList && !abPipBoyOnly && !bIsPipBoy) {
                        pim->xGlowBranchTiles.AddTail(pglow);
                    }
                }
                else {
                    pglow->Set(Tile::kTileValue_IsolatedVisible, 0);
                }
            }
            if (pnoglow) {
                if (abGlowOnly) {
                    pnoglow->Set(Tile::kTileValue_IsolatedVisible, 0);
                }
                else {
                    pnoglow->Set(Tile::kTileValue_IsolatedVisible, 1);
                }
            }
            if (pglow && pglow->GetModel()) {
                bool bCull = pglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pglow->GetModel()->SetAppCulled(bCull);
            }

            if (pnoglow && pnoglow->GetModel()) {
                bool bCull = abGlowOnly ||
                    pnoglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pnoglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pnoglow->GetModel()->SetAppCulled(bCull);
            }
        }
        if (apMenuTile->GetModel()) {
            bool bCull = apMenuTile->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                apMenuTile->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
            apMenuTile->GetModel()->SetAppCulled(bCull);
        }
    }
}

void Interface::SetTargetREFR(TESObjectREFR* apRef) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        BSStringT<char> xMessage;
        if (apRef) {
            int32_t iFormID = apRef->iFormID;
            xMessage.SPrintF("\"%s\" (%08x)", apRef->GetFullName(), iFormID);
            int32_t iFont = iDebugTextFont.Int() - 1;
            FontManager::Instance()->GetFont(iFont)->GetBaseLine();
            float fY = static_cast<float>(iDebugTextTopBottomOffset.Int());
            char* pMessage = xMessage.pString;
            DebugText::Instance(true)->Print(pMessage, 640.0f, fY, 2, -1, -1.0f, 0, nullptr);
        }
        InterfaceManager::Instance()->SetTargetREFR(apRef);
    }
}

NiPoint3 Interface::GetActivatePickLocation() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        return InterfaceManager::Instance()->GetActivatePickLocation();
    }

    return NiPoint3::ZERO;
}

void Interface::SetFirstChanceTextureRelease() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        InterfaceManager::Instance()->SetFirstChanceTextureRelease(true);
        Tile::UpdateAll(false);
    }
}

void Interface::SetRepairMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        Tile* pStat = Menu::GetMenuByClass(1003);
        Tile* pInv = Menu::GetMenuByClass(1002);
        Tile* pRepair = Menu::GetMenuByClass(1035);
        Tile* pItemMod = Menu::GetMenuByClass(1061);
        Tile* pMap = Menu::GetMenuByClass(1023);
        bool bIsVisible = Interface::IsMenuIDVisible(
            Interface::ePipBoyRepair,
            abAnyVisibleState ? Interface::FADE_NOT_HIDDEN : 0
        );
        if (abVisibility) {
            if (!pRepair) {
                pRepair = Interface::CreateRepairMenu();
            }
            if (!bIsVisible) {
                if (pStat) {
                    pStat->GetMenu()->StartFadeOut();
                    StatsMenu::ResetHealingMode();
                }
                if (pMap) {
                    pMap->GetMenu()->StartFadeOut();
                }
                if (pItemMod) {
                    pItemMod->GetMenu()->StartFadeOut();
                }
                if (pRepair) {
                    pRepair->GetMenu()->StartFadeIn();
                }
                if (pInv) {
                    pInv->GetMenu()->StartFadeOut();
                }
            }
            if (pRepair) {
                InterfaceManager::Instance()->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1035);
            }
            if (abTriggerPipboyEffect) {
                InterfaceManager::Instance()->GetPipboy()->TriggerRandomEffect();
                InterfaceManager::Instance()->GetPipboy()->ActivateButton(2u);
            }
            if (InterfaceManager::Instance()->GetPipboy()->GetInitialized()) {
                InventoryMenu::SetKnobPosition(false);
            }
        }
        else if (bIsVisible && pRepair) {
            pRepair->GetMenu()->StartFadeOut();
        }
    }
}

void Interface::CreateRepairServicesMenu(Actor* apVendor) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        RepairServicesMenu::Create(apVendor);
    }
}

void Interface::CreateVATSMenu() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        Tile* pTile = Menu::GetMenuByClass(1056);
        VATSMenu* pMenu = nullptr;
        if (pTile) {
            pMenu = dynamic_cast<VATSMenu*>(pTile->GetMenu());
        }
        if (pMenu) {
            delete pMenu;
        }
        VATSMenu::Create();
    }
}

bool Interface::SetInfoForRef(TESObjectREFR* apObject, bool abTelekinesis, bool abSameOkay) {
    return InterfaceManager::Instance() &&
        InterfaceManager::Instance()->IsDoneFirstInit() &&
        HUDMainMenu::SetInfoForRef(apObject, abTelekinesis, abSameOkay);
}

void Interface::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    char acTutorialFlags[104];
    int32_t iData = static_cast<int32_t>(
        InterfaceManager::Instance()->GetMenusRoot()->GetFloat(Tile::kTileValue_MenuLevel)
    );
    uint8_t ucStatsPage = StatsMenu::GetCurrentStatsPage();
    uint8_t ucInventoryPage = InventoryMenu::GetCurrentInventoryPage();
    uint8_t ucMapPage = MapMenu::GetCurrentMapPage();
    apSaveGameBuffer->SaveDataEndian(&iData, sizeof(iData), 0);
    apSaveGameBuffer->SaveDataEndian(&ucStatsPage, sizeof(ucStatsPage), 0);
    apSaveGameBuffer->SaveDataEndian(&ucInventoryPage, sizeof(ucInventoryPage), 0);
    apSaveGameBuffer->SaveDataEndian(&ucMapPage, sizeof(ucMapPage), 0);
    float fLocalMapMagnification = MapMenu::GetLocalMapMagnification();
    float fWorldMapMagnification = MapMenu::GetWorldMapMagnification();
    apSaveGameBuffer->SaveDataEndian(&fLocalMapMagnification, sizeof(fLocalMapMagnification), 0);
    apSaveGameBuffer->SaveDataEndian(&fWorldMapMagnification, sizeof(fWorldMapMagnification), 0);
    ourMemset(acTutorialFlags, 0, 100u);
    uint32_t uiSettingsSize = InterfaceManager::Instance()->xTutorialManager.GetSaveLoadSize();
    InterfaceManager::Instance()->xTutorialManager.Save(acTutorialFlags);
    apSaveGameBuffer->SaveDataEndian(&uiSettingsSize, sizeof(uiSettingsSize), 0);
    apSaveGameBuffer->SaveDataEndian(acTutorialFlags, uiSettingsSize, 0);
}

void Interface::ShowDDTIcon(bool abArmorPiercing) {
    HUDMainMenu::ShowDDTIcon(abArmorPiercing);
}

void Interface::ShowEnemyDDTIcon(bool abArmorPiercing) {
    HUDMainMenu::ShowEnemyDDTIcon(abArmorPiercing);
}

int16_t Interface::GetSaveSize() {
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x5Du) {
        return 9;
    }
    return 8;
}

// The terminal's fade is its geometry's last fade percent, or fully shown without geometry.
float FORenderedTerminal::GetFadeAmount() {
    return spGeometry ? spGeometry->GetLastFadePercent() : 1.0f;
}

uint32_t InterfaceManager::TutorialManager::GetSaveLoadSize() {
    return BSMIN(1, 1) + 5;
}

void InterfaceManager::TutorialManager::Reset() {
    for (MessageData& rMessage : tMessages) {
        rMessage.bShowMessage = 0;
        rMessage.bMessageShown = 0;
        rMessage.uiShowDelay = 0;
    }
    uiNextDisplay = 0;
    eDisplayID = Interface::TUT_COUNT;
}

bool InterfaceManager::IsDoneFirstInit() {
    return bFirstInit;
}

NiNode* InterfaceManager::GetInterfaceRoot() {
    return pInterfaceRoot;
}

NiAlphaProperty* InterfaceManager::GetAlphaProp() {
    return spAlphaProp.m_pObject;
}

FOVATSEffectManager* InterfaceManager::GetVATSEffect() {
    return &VATSEffect;
}

Tile* InterfaceManager::GetCurrentFocusTarget() {
    return pMouseOverTarget;
}

void InterfaceManager::SetDragDroppingState(int32_t aiSetTo) {
    iDragDroppingState = aiSetTo;
}

uint32_t InterfaceManager::GetDragDroppingState() {
    return iDragDroppingState;
}

MobileObject* DialogMenu::GetSpeaker() {
    return pSpeaker;
}

void InventoryMenu::SetCurrentTab(int32_t aiTab) {
    if (pMe) {
        int32_t iTab = BSMIN(aiTab, 4);
        pMe->pTiles[TI_TABLINE]->Set(iUT_CurrentTab, iTab);
        pMe->iCurrTab = iTab;
    }
}

void InventoryMenu::SetCurrentInventoryPage(uint8_t aucTab) {
    SetCurrentTab(aucTab);
}

uint8_t MapMenu::GetCurrentMapPage() {
    if (pMe) {
        return static_cast<uint8_t>(pMe->iCurrentPage - TI_TAB_LOCALMAP);
    }
    return 4;
}

// Disables the message menu: closes the open one and, while nothing is queued to show, reopens the next message on
// the main thread (or queues the menu when the game runs more than one hardware thread).
void MessageMenu_SetDisabled(uint8_t abDisable) {
    bMessageMenuDisabled = abDisable;
    if (MessageMenu::pMe && bMessageMenuDisabled) {
        MessageMenu::Close();
    }

    if (!MessageMenu::pMe && !bMessageMenuDisabled && MessageMenu::GetQueueHead()) {

        // The compare on the setting's value is signed.
        if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
            Interface::QueueMenuCreate(
                Interface::OPEN_MESSAGE,
                nullptr,
                nullptr,
                nullptr,
                Interface::CMM_STANDARD,
                false
            );
        }
        else {
            MessageMenu::Create();
        }
    }
}

double GetEyefinityScreenAspect() {
    return 0.75f;
}

// The Eyefinity group is drawn at 4:3 per screen: the frustum's width is scaled by 0.75 and by one screen's aspect
// ratio, and the vertical field of view is afScale times fDefaultFOV in radians, widened by 1.1 and capped at
// 1.53938043117523193359375. Compares and rounds in double; NaN takes the cap.
void HandleEyeFinityFOVScale(NiCamera* apCamera, float afScale) {
    if (!apCamera) {
        return;
    }
    if (!Renderer::is_bEyeFinity()) {
        return;
    }
    float fAspectRatio = static_cast<float>(GetEyefinityScreenAspect());
    apCamera->AdjustAspectRatio(fAspectRatio, false);
    double dWidth = Renderer::GetEyefinitySLSWidth();
    float fScreenRatio = static_cast<float>(dWidth / Renderer::GetEyefinitySLSHeight());
    double dFOV;
    if (afScale * BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369049072265625 * 1.10000002384185791015625 <
        1.53938043117523193359375) {
        dFOV = afScale * BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369049072265625 *
            1.10000002384185791015625;
    }
    else {
        dFOV = 1.53938043117523193359375;
    }
    float fFOV = static_cast<float>(dFOV);
    NiFrustum kFrustum(false);
    kFrustum.m_fLeft = -std::tan(fFOV) * fAspectRatio * fScreenRatio;
    kFrustum.m_fRight = std::tan(fFOV) * fAspectRatio * fScreenRatio;
    kFrustum.m_fBottom = -std::tan(fFOV) * fAspectRatio;
    kFrustum.m_fTop = std::tan(fFOV) * fAspectRatio;
    kFrustum.m_bOrtho = false;
    kFrustum.m_fNear = 1.0f;
    kFrustum.m_fFar = 5000.0f;
    apCamera->SetViewFrustum(kFrustum);
}

// These five are declared in InterfaceManager.h, so they keep external linkage: defined inside the anonymous
// namespace above they are different, internal entities, and the header's declarations stay undefined.
void Interface::ToggleFullHelp() {
    if (InterfaceManager::Instance()) {
        if (InterfaceManager::Instance()->IsDoneFirstInit()) {
            // Tests the flag as a raw byte: any byte but 0 reads as set, and it stores 0 or 1.
            bool bNewFullHelp = static_cast<uint8_t>(InterfaceManager::Instance()->bFullHelp) == 0;
            InterfaceManager::Instance()->bFullHelp = bNewFullHelp;
        }
    }
}

bool Interface::GetFullHelp() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->bFullHelp;
    }

    return false;
}

bool Interface::GetFogOfWarEnabled() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return MapMenu::QFogOfWar();
    }

    return false;
}

void Interface::SetFogOfWarEnabled(bool abFogOfWar) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        MapMenu::SetFogOfWar(abFogOfWar);
    }
}

// Returns null on both failed tests: the null singleton, then IsDoneFirstInit.
void Interface::CreateBarterMenu(TESObjectREFR* apRef, int32_t aiMode) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        BarterMenu::Create(apRef, aiMode);
    }
}

void Interface::CreateLockPickMenu(TESObjectREFR* apRef, int32_t aiLockLevel, bool abFlag) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        LockPickMenu::Create(apRef, aiLockLevel, abFlag);
    }
}

TESObjectREFR* Interface::InDialogWith() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        DialogMenu* pDialogMenu = nullptr;
        Tile* pMenuTile = Menu::GetMenuByClass(1009);
        if (pMenuTile) {
            Menu* pMenu = pMenuTile->GetMenu();
            pDialogMenu = dynamic_cast<DialogMenu*>(pMenu);
        }
        if (pDialogMenu) {
            return pDialogMenu->pSpeaker;
        }
    }

    return nullptr;
}

bool Interface::UpdateSleeping() {
    TileMenu* pMenuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(SleepWaitMenu::GetClassStatic()));
    SleepWaitMenu* pSleepMenu = nullptr;
    if (pMenuByClass) {
        pSleepMenu = static_cast<SleepWaitMenu*>(pMenuByClass->Tile::GetMenu());
    }
    if (!InterfaceManager::Instance()) {
        return false;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    if (pSleepMenu && pSleepMenu->UpdateSleeping()) {
        return true;
    }
    return false;
}

void Interface::SetCrosshairTargetType(TESObjectREFR* apRef) {
    if (pPlayer && pPlayer->Get3D()) {
        HUDMainMenu::SetTargetType(apRef, false);
    }
}

void Interface::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    bIsReverting = true;
    Interface::ClearAllSubtitlesAndMessages();
    HUDMainMenu::Revert();
    if (Interface::InDialog()) {
        MobileObject* pPartner = static_cast<MobileObject*>(Interface::InDialogWith());
        pPlayer->FocusOnActor(pPartner, 0.0f, true);
        MenuTopicManager::Destroy();
    }
    Interface::CloseVATSMenu();
    VATSMenu::ClearTargetList();
    Interface::CloseAllSupportMenus(false);
    MessageMenu::ClearMessageQueue();
    StatsMenu::ClearPerks();
    MapMenu::ClearSelectedNote();
    MapMenu::ClearAudioLogSubtites();
    Interface::PopFromEnterStack(1, true);
    InterfaceManager::Instance()->EnterRenderedMenu(nullptr);
    InterfaceManager::Instance()->ePipboyState = InterfaceManager::PS_CLOSED;
    InterfaceManager::Instance()->xTutorialManager.Reset();
}

// Menus laid out in screen pixels rather than the 1280-wide menu space: the pip-boy menu (id 0x70), the computers,
// hacking and race/sex menus (classes 0x421, 0x41F, 0x40C). Both values are truncated before the compares.
bool Interface::IsRenderedMenu(Menu* apMenu) {
    TileMenu* menuTile = apMenu ? apMenu->GetMenuTile() : nullptr;
    if (!menuTile) {
        return false;
    }
    const int32_t id = static_cast<int32_t>(menuTile->GetFloat(Tile::kTileValue_ID));
    const int32_t menuClass = static_cast<int32_t>(menuTile->GetFloat(Tile::kTileValue_Class));
    return id == 0x70 || menuClass == 0x421 || menuClass == 0x41F || menuClass == 0x40C;
}

// The item data tile's last 13 children, last first, are its stat rows: 0 damage resistance, 1 damage per second, 2
// weight, 3 value, 4 condition, 5 ammo or armor class, 6 effects, 7 to 9 the three mod rows, 10 strength requirement
// (may be missing), 11 damage, 12 damage threshold; missing children are null. Each row gets its value and title, and
// uiVisible picks the rows shown. It reads apItem's value, weight and object before the null checks.
void Interface::PopulateItemStatsDisplay(ItemChange* apItem, Tile* apTile) {
    uint32_t uiVisible = 0;
    int32_t iTitleTrait = Tile::TextToTrait("_Title");
    int32_t iValueTrait = Tile::TextToTrait("_Value");
    int32_t iConditionTrait = Tile::kTileValue_User5;
    float fMod2Offset = 0.0f;
    float fMod3Offset = 0.0f;
    float fValue = apItem->GetItemValue();
    bool bHardcore = pPlayer->IsHardcore();
    float fWeight = TESWeightForm::GetFormWeight(apItem->GetItemChangeObject(), bHardcore);
    if (!apItem || !apTile) {
        return;
    }
    Tile* pTiles[13];
    NiTListItem<Tile*>* pPos = apTile->xChildren.GetTailPos();
    int32_t i;
    for (i = 0; pPos && i < 13; ++i) {
        pTiles[i] = apTile->xChildren.GetPrev(pPos);
    }
    for (; i < 13; ++i) {
        pTiles[i] = nullptr;
    }
    Tile* pArrows = Tile::GetTileByName(apTile, "CNDArrows");
    Tile* pMeter = Tile::GetTileByName(apTile, "ConditionMeter");
    pArrows->Set(Tile::kTileValue_Height, pMeter->GetFloat(Tile::kTileValue_Height), true);
    pArrows->Set(Tile::kTileValue_Y, pMeter->GetFloat(Tile::kTileValue_Y), true);
    bool bHasMods = apItem->GetModSlots() != 0;
    switch (apItem->GetItemChangeObject()->GetFormType()) {
        case AMMO_ID: {
            uiVisible = 0xC;
            TESAmmo* pAmmo = static_cast<TESAmmo*>(apItem->GetItemChangeObject());
            char acAmmoEffects[256];
            pAmmo->BuildMenuString(acAmmoEffects, 0x100);
            if (BSstrlen(acAmmoEffects) > 0) {
                pTiles[6]->Set(iValueTrait, acAmmoEffects, true);
                uiVisible |= 0x40;
            }
            else {
                uiVisible &= ~0x40u;
            }
            break;
        }
        case WEAP_ID: {
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apItem->GetItemChangeObject());
            pTiles[4]->Set(Tile::kTileValue_User6, 1);
            // The player's loaded ammo and the player as the weapon's holder, when the item is the player's weapon.
            TESAmmo* pLoadedAmmo = nullptr;
            Actor* pHolder = nullptr;
            if (pPlayer->GetCurrentAIProcess() &&
                pPlayer->GetCurrentAIProcess()->GetCurrentWeapon() &&
                pWeapon == pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()->GetItemChangeObject()) {
                TESAmmo* pAmmo;
                if (pPlayer->GetCurrentAIProcess()->GetCurrentAmmo()) {
                    pAmmo = static_cast<TESAmmo*>(
                        pPlayer->GetCurrentAIProcess()->GetCurrentAmmo()->GetItemChangeObject()
                    );
                }
                else {
                    pAmmo = nullptr;
                }
                pLoadedAmmo = pAmmo;
                pHolder = pPlayer;
            }
            fWeight = pWeapon->GetFormWeight(apItem->HasModEffectActive(WEAPON_MOD_DECREASE_WEIGHT));
            if (fWeight >= 10.0) {
                float fHeavyMult = 1.0f;
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &fHeavyMult);
                fWeight = fWeight * fHeavyMult;
            }
            float fCondition = static_cast<float>(apItem->GetItemHealth(true) / 100.0);
            uint32_t uiStrRequired = pWeapon->GetStrRequired();
            float fDamage = 0.0f;
            float fDPS = 0.0f;
            fDPS = CombatUtilities::CalculateItemDPS(
                pPlayer,
                pWeapon,
                fCondition,
                true,
                apItem,
                false,
                false,
                -1,
                0.0f,
                0.0f,
                false,
                false,
                pLoadedAmmo
            );
            // A NaN condition still asks for the damage.
            if (fCondition <= 0.0) {
                fDamage = 0.0f;
            }
            else {
                fDamage = CombatFormulas::CalcWeaponDamageForDisplay(pWeapon, fCondition, apItem, pLoadedAmmo);
            }
            char acBuffer[128];
            BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(BSRoundToNearest(fDPS, 1.0f)));
            pTiles[1]->Set(iValueTrait, acBuffer, true);
            Actor* pProjectileHolder = pHolder;
            uint32_t uiProjectiles = pWeapon->GetNumProjectiles(
                apItem->HasModEffectActive(WEAPON_MOD_SPLIT_BEAM),
                false,
                pProjectileHolder
            );
            if (uiProjectiles > 1) {
                float fPerProjectile = static_cast<float>(fDamage / static_cast<double>(uiProjectiles));
                BSsprintf(acBuffer, 0x80, "%.1fx%d", fPerProjectile, uiProjectiles);
            }
            else {
                BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(BSRoundToNearest(fDamage, 1.0f)));
            }
            pTiles[11]->Set(iValueTrait, acBuffer, true);
            pTiles[4]->Set(iConditionTrait, BSMIN(fCondition, 1.0f), true);
            float fStrBonus = 0.0f;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::WEAPON_HANDLING_STRENGTH_BONUS,
                pPlayer,
                pWeapon,
                &fStrBonus
            );
            // The bonus is converted by truncation (a float to unsigned conversion), and the difference is clamped at 0
            // on its sign bit.
            uint32_t uiStrBonus = static_cast<uint32_t>(static_cast<int64_t>(fStrBonus));
            uiStrRequired -= uiStrBonus;
            if (static_cast<int32_t>(uiStrRequired) < 0) {
                uiStrRequired = 0;
            }
            BSsprintf(acBuffer, 0x80, "%d", uiStrRequired);
            if (pTiles[10]) {
                pTiles[10]->Set(iValueTrait, acBuffer, true);
            }
            if (pWeapon->GetCurrentAmmo(pHolder)) {
                ItemChange* pWeaponInfo = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                ItemChange* pAmmoInfo = pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
                const char* pAmmoName = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(pHolder))->GetShortName();
                TESForm* pAmmoForm = pWeapon->GetCurrentAmmo(pHolder);
                InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
                int32_t iAmmoCount = pChanges->GetObjectCount(static_cast<TESBoundObject*>(pAmmoForm));
                int32_t iClipRounds = 0;
                if (!pAmmoName || !*pAmmoName) {
                    pAmmoName = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(pHolder))->TESFullName::GetFullName();
                }
                // The item is the equipped weapon when both share their first instance: the loaded rounds are the ammo
                // item's count, else a clip's worth of the carried ammo.
                if (pWeaponInfo &&
                    pWeaponInfo->GetExtraDataList() &&
                    apItem->GetExtraDataList() &&
                    pWeaponInfo->GetExtraDataList()->GetItem() == apItem->GetExtraDataList()->GetItem()) {
                    int32_t iLoaded;
                    if (pAmmoInfo) {
                        iLoaded = pAmmoInfo->GetCount();
                    }
                    else {
                        iLoaded = 0;
                    }
                    iClipRounds = iLoaded;
                }
                else {
                    iClipRounds = BSMIN(
                        pWeapon->GetFormClipRounds(apItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE)),
                        iAmmoCount
                    );
                }
                bool bRegenerates = pWeapon->GetAmmoRegenRate(false) != 0.0;
                if (bRegenerates) {
                    BSsprintf(acBuffer, 0x80, "%s", pAmmoName);
                }
                else {
                    BSsprintf(acBuffer, 0x80, "%s (%i/%i)", pAmmoName, iClipRounds, iAmmoCount - iClipRounds);
                }
            }
            else {
                BSstrcpy(acBuffer, 0x80, "--");
            }
            pTiles[5]->Set(iTitleTrait, acBuffer, true);
            uiVisible = 0xC3E;
            break;
        }
        case ARMO_ID:
        case CLOT_ID: {
            pTiles[4]->Set(Tile::kTileValue_User6, 0);
            float fCondition = static_cast<float>(apItem->GetItemHealth(true) / 100.0);
            uiVisible = 0x3C;
            char acBuffer[128];
            BSstrcpy(acBuffer, 0x80, "--");
            TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(apItem->GetItemChangeObject());
            if (pArmor) {
                if (pArmor->IsHeavy()) {
                    BSstrcpy(acBuffer, 0x80, pGSArmorWeight[1]->String());
                }
                else if (pArmor->IsMedium()) {
                    BSstrcpy(acBuffer, 0x80, pGSArmorWeight[2]->String());
                }
                else {
                    BSstrcpy(acBuffer, 0x80, pGSArmorWeight[0]->String());
                }
            }
            pTiles[5]->Set(iTitleTrait, acBuffer, true);
            float fArmorRating = CombatFormulas::CalcArmorRating(
                static_cast<uint16_t>(
                    static_cast<int32_t>(static_cast<TESObjectARMO*>(apItem->GetItemChangeObject())->GetArmorRating())
                ),
                fCondition
            );
            float fThreshold = CombatFormulas::CalcArmorRating(
                static_cast<uint16_t>(static_cast<int32_t>(
                    static_cast<TESObjectARMO*>(apItem->GetItemChangeObject())->GetDamageThreshold()
                )),
                fCondition
            );
            if (fArmorRating == 0.0 && fThreshold == 0.0) {
                uiVisible |= 1;
                BSstrcpy(acBuffer, 0x80, "--");
                pTiles[0]->Set(iValueTrait, acBuffer, true);
            }
            else {
                if (fArmorRating != 0.0) {
                    BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(FloatCeil(fArmorRating)));
                    pTiles[0]->Set(iValueTrait, acBuffer, true);
                    uiVisible |= 1;
                }
                if (fThreshold != 0.0) {
                    BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(FloatCeil(fThreshold)));
                    pTiles[12]->Set(iValueTrait, acBuffer, true);
                    uiVisible |= 0x1000;
                }
            }
            uint32_t uiMaxHealth;
            if (TESHealthForm::GetFormHealth(apItem->GetItemChangeObject())) {
                uiMaxHealth = TESHealthForm::GetFormHealth(apItem->GetItemChangeObject());
            }
            else {
                uiMaxHealth = 1;
            }
            float fHealthRatio = static_cast<float>(apItem->GetItemHealth(false) / static_cast<double>(uiMaxHealth));
            pTiles[4]->Set(iConditionTrait, BSMIN(1.0f, fHealthRatio), true);
            break;
        }
        default: uiVisible = 0xC; break;
    }
    EffectItemList* pEffects = nullptr;
    if (apItem->GetItemChangeObject()->GetFormType() == INGR_ID) {
        pEffects = static_cast<IngredientItem*>(apItem->GetItemOwnership());
    }
    else if (apItem->GetItemChangeObject()->GetFormType() == ALCH_ID) {
        pEffects = static_cast<AlchemyItem*>(apItem->GetItemChangeObject());
    }
    else {
        EnchantmentItem* pEnchantment = TESEnchantableForm::GetFormEnchanting(apItem->GetItemChangeObject());
        if (pEnchantment && !pEnchantment->GetFlag(EnchantmentItem::HIDE_EFFECT)) {
            pEffects = pEnchantment;
        }
    }
    if (pEffects && pEffects->GetItem()) {
        char acEffects[256];
        pEffects->BuildMenuString(acEffects, 0x100);
        if (BSstrlen(acEffects) > 0) {
            pTiles[6]->Set(iValueTrait, acEffects, true);
            uiVisible |= 0x40;
        }
        else {
            uiVisible &= ~0x40u;
        }
    }
    else if (apItem->GetItemChangeObject()->GetFormType() != AMMO_ID) {
        uiVisible &= ~0x40u;
    }
    if (apItem->GetItemChangeObject()->GetFormType() == IMOD_ID) {
        TESObjectIMOD* pItemMod = static_cast<TESObjectIMOD*>(apItem->GetItemChangeObject());
        pTiles[6]->Set(iValueTrait, pItemMod->GetDescription(nullptr, DESC_ID), true);
        uiVisible |= 0x40;
    }
    if (bHasMods) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apItem->GetItemChangeObject());
        TESObjectIMOD* pMod = nullptr;
        uint8_t ucModSlots = apItem->GetModSlots();
        bool bTitleSet = false;
        char acModName[128];
        if (ucModSlots & 1) {
            uiVisible |= 0x80;
            pTiles[7]->Set(iTitleTrait, sModEffects.String(), true);
            bTitleSet = true;
            fMod2Offset = static_cast<float>(fMod2Offset + 41.0);
            fMod3Offset = static_cast<float>(fMod3Offset + 41.0);
            pMod = pWeapon->GetModItem(1);
            if (pMod) {
                BSstrcpy(acModName, 0x80, pMod->TESFullName::GetFullName());
            }
            else {
                BSstrcpy(acModName, 0x80, "Mod 1");
            }
            pTiles[7]->Set(iValueTrait, acModName, true);
        }
        if (ucModSlots & 2) {
            uiVisible |= 0x100;
            pMod = pWeapon->GetModItem(2);
            if (pMod) {
                BSstrcpy(acModName, 0x80, pMod->TESFullName::GetFullName());
            }
            else {
                BSstrcpy(acModName, 0x10, "Mod 2");
            }
            pTiles[8]->Set(iValueTrait, acModName, true);
            if (!bTitleSet) {
                pTiles[8]->Set(iTitleTrait, sModEffects.String(), true);
                bTitleSet = true;
            }
            fMod3Offset = static_cast<float>(fMod3Offset + 41.0);
        }
        if (ucModSlots & 4) {
            pMod = pWeapon->GetModItem(4);
            if (pMod) {
                BSstrcpy(acModName, 0x80, pMod->TESFullName::GetFullName());
            }
            else {
                BSstrcpy(acModName, 0x10, "Mod 3");
            }
            pTiles[9]->Set(iValueTrait, acModName, true);
            uiVisible |= 0x200;
            if (!bTitleSet) {
                pTiles[9]->Set(iTitleTrait, sModEffects.String(), true);
                bTitleSet = true;
            }
        }
        uiVisible &= ~0x40u;
    }
    else {
        uiVisible &= ~0x80u;
    }
    char acValue[16];
    if (fValue > 0.0) {
        // The count times the value at x87 precision, passed as a double.
        if (fValue < 1.0) {
            BSsprintf(acValue, 0x10, "%.1f", static_cast<double>(apItem->GetCount()) * fValue);
        }
        else {
            BSsprintf(acValue, 0x10, "%.0f", static_cast<double>(apItem->GetCount()) * fValue);
        }
    }
    else {
        BSstrcpy(acValue, 0x10, "--");
    }
    pTiles[3]->Set(iValueTrait, acValue, true);
    float fLightItems = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_LIGHT_ITEMS_WEIGH_NOTHING, pPlayer, &fLightItems);
    if (fLightItems > 0.0) {
        if (fPackRatThreshold.Float() >= fWeight) {
            fWeight = fWeight * fPackRatModifier.Float();
        }
    }
    char acWeight[32];
    if (fWeight > 0.0) {
        BSsprintf(acWeight, 0x20, "%.2f", static_cast<double>(apItem->GetCount()) * fWeight);
    }
    else {
        BSstrcpy(acWeight, 0x20, "--");
    }
    pTiles[2]->Set(iValueTrait, acWeight, true);
    if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
        pTiles[0]->Set(iTitleTrait, sInventoryDamageResistance.String(), true);
        pTiles[1]->Set(iTitleTrait, sInventoryDamagePerSecond.String(), true);
        pTiles[11]->Set(iTitleTrait, sInventoryDamage.String(), true);
        pTiles[2]->Set(iTitleTrait, sInventoryWeightUpper.String(), true);
        pTiles[3]->Set(iTitleTrait, sInventoryValue.String(), true);
        pTiles[4]->Set(iTitleTrait, sInventoryCondition.String(), true);
        pTiles[6]->Set(iTitleTrait, sInventoryEffects.String(), true);
        pTiles[7]->Set(iTitleTrait, sModEffects.String(), true);
        pTiles[10]->Set(iTitleTrait, sInventoryStrReq.String(), true);
    }
    pTiles[0]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 1) != 0));
    pTiles[12]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x1000) != 0));
    pTiles[1]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 2) != 0));
    pTiles[11]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x800) != 0));
    pTiles[2]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 4) != 0));
    pTiles[3]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 8) != 0));
    pTiles[4]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x10) != 0));
    pTiles[5]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x20) != 0));
    pTiles[6]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x40) != 0));
    pTiles[7]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x80) != 0));
    pTiles[8]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x100) != 0));
    pTiles[9]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x200) != 0));
    if (pTiles[10]) {
        pTiles[10]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x400) != 0));
        pTiles[10]->Set(Tile::kTileValue_Alpha, 255.0f, true);
    }
    float fY = static_cast<float>(apTile->GetFloat(Tile::kTileValue_Height) / 2.0 - 20.0);
    if ((uiVisible & 0x10) || (uiVisible & 0x20)) {
        fY = fY + fY;
    }
    pTiles[6]->Set(Tile::kTileValue_Y, fY, true);
    pTiles[7]->Set(Tile::kTileValue_Y, fY, true);
    float fMod2Y = fY + fMod2Offset;
    pTiles[8]->Set(Tile::kTileValue_Y, fMod2Y, true);
    float fMod3Y = fY + fMod3Offset;
    pTiles[9]->Set(Tile::kTileValue_Y, fMod3Y, true);
}

TESObjectREFR* Interface::GetTelekinesisREFR() {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return nullptr;
    }
    return InterfaceManager::Instance()->pTelekinesisRef;
}

bool Interface::IsPipboyClosed() {
    return InterfaceManager::Instance()->ePipboyState == 0;
}
