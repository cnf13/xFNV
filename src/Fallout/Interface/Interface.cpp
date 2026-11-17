#include "Fallout/Interface/Interface.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
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
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRecipe.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

#include <SDL3/SDL.h>
#include <cstdarg>
#include <cstdint>

SettingT<INIPrefSettingCollection> gs_bDialogueSubtitles_GamePlay("bDialogueSubtitles:GamePlay", true);
SettingT<INIPrefSettingCollection> gs_bGeneralSubtitles_Gameplay("bGeneralSubtitles:GamePlay", false);
SettingT<INIPrefSettingCollection> gs_bCrossHair_GamePlay("bCrossHair:GamePlay", true);
SettingT<INIPrefSettingCollection> gs_bSaveOnRest_GamePlay("bSaveOnRest:GamePlay", true);
SettingT<INIPrefSettingCollection> gs_bSaveOnWait_GamePlay("bSaveOnWait:GamePlay", true);
SettingT<INIPrefSettingCollection> gs_bSaveOnTravel_GamePlay("bSaveOnTravel:GamePlay", true);
static SettingT<INIPrefSettingCollection> bSaveOnInteriorExteriorSwitch("bSaveOnInteriorExteriorSwitch:GamePlay", true);
SettingT<INIPrefSettingCollection> gs_bDisableDynamicCrosshair_GamePlay("bDisableDynamicCrosshair:GamePlay", false);
SettingT<INIPrefSettingCollection> gs_bTrueIronSights_GamePlay("bTrueIronSights:GamePlay", true);
Interface::QUEUED_OPEN Interface::QueuedMenuOpen;
Interface::QUEUED_OPEN Interface::QueuedMessageOpen;
BSSpinLock Interface::QueuedMenuLock;

char GetIsMapMenuFogOfWar() {
    InterfaceManager* singleton;
    if (InterfaceManager::Instance() && ((singleton = InterfaceManager::Instance()), singleton->IsDoneFirstInit())) {
        return (int8_t)GetMapMenuFogOfWar();
    }

    return 0;
}

int32_t BSMAX_2(int32_t a1, char a2) {
    if (a1 <= a2) {
        return a2;
    }

    return a1;
}

void ModalMessageMenuCallback() {
    iModalMessageMenuResult = Interface::GetMessageMenuResult();
}

void ToggleMapFogOfWar(bool abFogOfWar) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v1 = InterfaceManager::Instance();
        if (v1->bFirstInit) {
            SetMapMenuFogOfWar(abFogOfWar);
        }
    }
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
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iHUDValue), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucStatsTab), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucInventoryTab), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ucMapTab), 1);
    InterfaceManager::Instance()->GetMenusRoot()->Set(6001, iHUDValue);
    StatsMenu::SetCurrentStatsPage(ucStatsTab);
    InventoryMenu::SetCurrentInventoryPage(ucInventoryTab);
    MapMenu::SetCurrentMapPage(ucMapTab);
    float fLocalMagnification = 0.0f;
    float fWorldMagnification = 0.0f;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fLocalMagnification), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fWorldMagnification), 4);
    MapMenu::SetLocalMapMagnification(fLocalMagnification);
    MapMenu::SetWorldMapMagnification(fWorldMagnification);
    // The version is zero-extended and compared signed.
    if (static_cast<int32_t>(apLoadGameBuffer->GetVersion()) < 12) {
        apLoadGameBuffer->LoadString(acOldString);
        return;
    }
    ourMemset(acTutorialFlags, 0, 100);
    uint32_t uiSize = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiSize), 4);
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
    FNVD_INLINED_LOCAL(BSSoundHandle, v7, std::nullopt, std::nullopt);
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp", 2284
    );
    if (InterfaceManager::Instance()) {
        InterfaceManager* v5 = InterfaceManager::Instance();
        if ((v5->bFirstInit) && (HUDMainMenu::pme)) {
            v7 = ahandle;
            HUDMainMenu::pme->ShowSubtitle(apString, v7, aSpeakerLoc, apTarget, abForceShow);
        }
    }
    bool v12 = false;
    return v12;
}

void Interface::SetMapMenuVisible(bool abVisibility, bool abAnyVisibleState) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v2 = InterfaceManager::Instance();
        if (v2->bFirstInit) {
            Tile* pstat = Menu::GetMenuByClass(1003);
            Tile* pinv = Menu::GetMenuByClass(1002);
            Tile* prepair = Menu::GetMenuByClass(1035);
            Tile* pitemmod = Menu::GetMenuByClass(1061);
            Tile* pmap = Menu::GetMenuByClass(1023);
            bool bis_vis = Interface::IsMenuIDVisible(Interface::ePipBoyData, abAnyVisibleState ? 11 : 0);
            if (!abVisibility || bis_vis) {
                if ((!abVisibility && bis_vis) && (pmap)) {
                    Menu* Menu_ = pmap->GetMenu();
                    Menu_->StartFadeOut();
                    Menu* v16 = pmap->GetMenu();
                    v16->DoLeave(0, nullptr);
                }
            }
            else {
                if (pstat) {
                    Menu* v3 = pstat->GetMenu();
                    v3->StartFadeOut();
                    StatsMenu::Close();
                }
                if (prepair) {
                    Menu* v4 = prepair->GetMenu();
                    v4->StartFadeOut();
                    RepairMenu::ResetMenu();
                }
                if (pitemmod) {
                    Menu* v5 = pitemmod->GetMenu();
                    v5->StartFadeOut();
                    ItemModMenu::ResetMenu();
                }
                if (pinv) {
                    Menu* v6 = pinv->GetMenu();
                    v6->StartFadeOut();
                }
                if (!pmap) {
                    pmap = Interface::CreateMapMenu();
                }
                if (pmap) {
                    InterfaceManager* v7 = InterfaceManager::Instance();
                    v7->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1023);
                    Menu* v8 = pmap->GetMenu();
                    v8->StartFadeIn();
                }
                MapMenu::UpdateMenu();
            }
            if (abVisibility) {
                InterfaceManager* v10 = InterfaceManager::Instance();
                FOPipboyManager* Pipboy = v10->GetPipboy();
                Pipboy->TriggerRandomEffect();
                InterfaceManager* v12 = InterfaceManager::Instance();
                FOPipboyManager* v13 = v12->GetPipboy();
                v13->ActivateButton(3u);
                InterfaceManager* v14 = InterfaceManager::Instance();
                FOPipboyManager* v15 = v14->GetPipboy();
                if (v15->GetInitialized()) {
                    MapMenu::SetKnobPosition(false);
                }
            }
        }
    }
}

void Interface::OpenConsole() {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v0 = InterfaceManager::Instance();
        if ((v0->IsDoneFirstInit()) && (MenuConsole::Instance(false))) {
            MenuConsole* v1 = MenuConsole::Instance(true);
            if (!v1->IsVisible()) {
                MenuConsole* v2 = MenuConsole::Instance(true);
                v2->ToggleVisible();
                InterfaceManager* v3 = InterfaceManager::Instance();
                v3->AddToEnterStack(3);
            }
        }
    }
}

void Interface::SetInventoryMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect) {
    InterfaceManager* singleton;
    Menu* menu;
    Menu* v5;
    Menu* v6;
    Menu* v7;
    Menu* v8;
    InterfaceManager* v9;
    Tile* combatState;
    InterfaceManager* v11;
    FOPipboyManager* pipboy;
    InterfaceManager* v13;
    FOPipboyManager* v14;
    InterfaceManager* v15;
    FOPipboyManager* v16;
    Menu* v17;
    TileMenu* inventoryMenu;
    TileMenu* v19;
    uint8_t isMenuIDVisible;
    TileMenu* menuByClass;
    TileMenu* v22;
    TileMenu* v23;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
            inventoryMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(1002));
            v19 = static_cast<TileMenu*>(Menu::GetMenuByClass(1035));
            v23 = static_cast<TileMenu*>(Menu::GetMenuByClass(1061));
            v22 = static_cast<TileMenu*>(Menu::GetMenuByClass(1023));
            isMenuIDVisible =
                (uint8_t)Interface::IsMenuIDVisible(Interface::eInventoryMenu, abAnyVisibleState ? 11 : 0);
            if (abVisibility) {
                if (!inventoryMenu) {
                    inventoryMenu = static_cast<TileMenu*>(Interface::CreateInventoryMenu());
                }
                if (!isMenuIDVisible) {
                    if (menuByClass) {
                        menu = menuByClass->GetMenu();
                        menu->StartFadeOut();
                        StatsMenu::Close();
                    }
                    if (v22) {
                        v5 = v22->GetMenu();
                        v5->StartFadeOut();
                    }
                    if (v19) {
                        v6 = v19->GetMenu();
                        v6->StartFadeOut();
                        RepairMenu::ResetMenu();
                    }
                    if (v23) {
                        v7 = v23->GetMenu();
                        v7->StartFadeOut();
                        ItemModMenu::ResetMenu();
                    }
                    if (inventoryMenu) {
                        v8 = inventoryMenu->GetMenu();
                        v8->StartFadeIn();
                    }
                }
                if (inventoryMenu) {
                    InventoryMenu::UpdateList();
                    v9 = InterfaceManager::Instance();
                    combatState = v9->GetMenusRoot();
                    combatState->Set(6001, 1002);
                }
                if (abTriggerPipboyEffect) {
                    v11 = InterfaceManager::Instance();
                    pipboy = v11->GetPipboy();
                    pipboy->TriggerRandomEffect();
                    v13 = InterfaceManager::Instance();
                    v14 = v13->GetPipboy();
                    v14->ActivateButton(2u);
                }
                v15 = InterfaceManager::Instance();
                v16 = v15->GetPipboy();
                if (v16->GetInitialized()) {
                    InventoryMenu::SetKnobPosition(false);
                }
            }
            else {
                if ((isMenuIDVisible) && (inventoryMenu)) {
                    v17 = inventoryMenu->GetMenu();
                    v17->StartFadeOut();
                    InventoryMenu::SavePageState();
                }
            }
        }
    }
}

void Interface::SetStatsMenuVisible(bool abVisibility, bool abAnyVisibleState) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v2 = InterfaceManager::Instance();
        if (v2->bFirstInit) {
            Tile* pstat = Menu::GetMenuByClass(1003);
            Tile* pinv = Menu::GetMenuByClass(1002);
            Tile* prepair = Menu::GetMenuByClass(1035);
            Tile* pitemmod = Menu::GetMenuByClass(1061);
            Tile* pmap = Menu::GetMenuByClass(1023);
            bool bis_vis = Interface::IsMenuIDVisible(Interface::eStatsMenu, abAnyVisibleState ? 11 : 0);
            if (!abVisibility || bis_vis) {
                if ((!abVisibility && bis_vis) && (pstat)) {
                    Menu* Menu_ = pstat->GetMenu();
                    Menu_->StartFadeOut();
                }
            }
            else {
                if (pinv) {
                    Menu* v3 = pinv->GetMenu();
                    v3->StartFadeOut();
                }
                if (prepair) {
                    Menu* v4 = prepair->GetMenu();
                    v4->StartFadeOut();
                    RepairMenu::ResetMenu();
                }
                if (pitemmod) {
                    Menu* v5 = pitemmod->GetMenu();
                    v5->StartFadeOut();
                    ItemModMenu::ResetMenu();
                }
                if (pmap) {
                    Menu* v6 = pmap->GetMenu();
                    v6->StartFadeOut();
                }
                if (!pstat) {
                    pstat = Interface::CreateStatsMenu();
                }
                if (pstat) {
                    InterfaceManager* v7 = InterfaceManager::Instance();
                    v7->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1003);
                    StatsMenu* v8 = static_cast<StatsMenu*>(pstat->GetMenu());
                    v8->UpdateAll();
                    Menu* v9 = pstat->GetMenu();
                    v9->StartFadeIn();
                }
            }
            if (abVisibility) {
                InterfaceManager* v11 = InterfaceManager::Instance();
                FOPipboyManager* Pipboy = v11->GetPipboy();
                Pipboy->TriggerRandomEffect();
                InterfaceManager* v13 = InterfaceManager::Instance();
                FOPipboyManager* v14 = v13->GetPipboy();
                v14->ActivateButton(1u);
                InterfaceManager* v15 = InterfaceManager::Instance();
                FOPipboyManager* v16 = v15->GetPipboy();
                if (v16->GetInitialized()) {
                    StatsMenu::SetKnobPosition(false);
                }
            }
        }
    }
}

char Interface::GetInventoryPageNumber() {
    if (!InterfaceManager::Instance()) {
        return (-1);
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return (-1);
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return (int8_t)((uint8_t)v1->GetInventoryPageNumber());
}

int32_t Interface::GetLastPipboyTab() {
    if (!InterfaceManager::Instance()) {
        return -1;
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return -1;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return v1->GetLastPipboyTab();
}

uint32_t Interface::GetContainerMode() {
    return GetContainerMenuMode();
}

void Interface::ForceReload() {
    InterfaceManager* singleton;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            StartMenu::Create(true, true);
        }
    }
}

void Interface::UpdateActorValue(ActorValue::Index aeWhich) {
    InterfaceManager* singleton;
    Menu* menu;
    void* v3;
    TileMenu* menuByClass;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if ((singleton->IsDoneFirstInit()) &&
            (Interface::IsMenuIDVisible(Interface::eStatsMenu, static_cast<Interface::FADE_STATE>(11)))) {
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
            if (menuByClass) {
                menu = menuByClass->GetMenu();
                v3 = dynamic_cast<StatsMenu*>(menu);
            }
            else {
                v3 = nullptr;
            }
            if (v3) {
                if (((int32_t)aeWhich) == 77) {
                    static_cast<StatsMenu*>(v3)->UpdateAllActorValues();
                }
                else {
                    static_cast<StatsMenu*>(v3)->UpdateActorValue(aeWhich);
                }
            }
        }
    }
}

void Interface::SetCursorPos(float afX, float afY) {
    InterfaceManager* singleton;
    InterfaceManager* v3;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v3 = InterfaceManager::Instance();
            v3->SetCursorPos(afX, afY);
        }
    }
}

char Interface::GetMessageMenuResult() {
    char cresult = InterfaceManager::Instance()->cLastMessageButtonClicked;
    InterfaceManager::Instance()->cLastMessageButtonClicked = -1;
    return cresult;
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
    InterfaceManager* v0 = InterfaceManager::Instance();
    if (!v0->bFirstInit) {
        return false;
    }
    Tile* MenuByClass = nullptr;
    InterfaceManager* v1 = InterfaceManager::Instance();
    int32_t aiClass = v1->GetEnterStackTop();
    if (aiClass == 1) {
        if (Interface::IsMenuIDVisible(Interface::eStatsMenu, 0)) {
            MenuByClass = Menu::GetMenuByClass(1003);
        }
        else {
            if (Interface::IsMenuIDVisible(Interface::eInventoryMenu, 0)) {
                MenuByClass = Menu::GetMenuByClass(1002);
            }
            else {
                if (Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0)) {
                    MenuByClass = Menu::GetMenuByClass(1035);
                }
                else {
                    if (Interface::IsMenuIDVisible(Interface::eItemModMenu, 0)) {
                        MenuByClass = Menu::GetMenuByClass(1061);
                    }
                    else {
                        if (Interface::IsMenuIDVisible(Interface::ePipBoyData, 0)) {
                            MenuByClass = Menu::GetMenuByClass(1023);
                        }
                    }
                }
            }
        }
    }
    else {
        MenuByClass = Menu::GetMenuByClass(aiClass);
    }
    return MenuByClass && MenuByClass->GetMenu() && MenuByClass->GetMenu()->xFadeState == Interface::FADE_SHOWN;
}

Tile* Interface::CreateContainerMenu(TESObjectREFR* apContainer, Interface::CONTAINER_MENU_MODE aeMode) {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return nullptr;
    }
    if (((aeMode == CMM_STANDARD) && (!apContainer->IsActor())) && (apContainer->GetOwner()) &&
        (!apContainer->IsAnOwner(pPlayer, true))) {
        aeMode = CMM_PICKPOCKET;
    }

    return ContainerMenu::Create(apContainer, aeMode);
}

bool Interface::CreateHackingMenu(TESObjectREFR* apTerminalRef) {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    if (!v1->bFirstInit) {
        return false;
    }
    Tile* pmenu = HackingMenu::Create(apTerminalRef);
    if (bUseRenderedTerminals.Bool()) {
        FORenderedTerminal* prm;
        Tile::UpdateAll(false);
        FORenderedTerminal* apMemory = static_cast<FORenderedTerminal*>(ourMalloc(sizeof(FORenderedTerminal)));
        if (apMemory) {
            prm = ::new (apMemory) FORenderedTerminal;
        }
        else {
            prm = nullptr;
        }
        prm->pMenuTile = pmenu;
        prm->SetMenuRoot(pmenu->GetModel());
        InterfaceManager* v2 = InterfaceManager::Instance();
        v2->EnterRenderedMenu(prm);
    }
    return true;
}

bool Interface::CreateComputersMenu(TESObjectREFR* apTerminalRef) {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    if (!v1->bFirstInit) {
        return false;
    }
    Tile* pmenu = ComputersMenu::Create(apTerminalRef);
    Tile::UpdateAll(false);
    if (bUseRenderedTerminals.Bool()) {
        FORenderedTerminal* prm;
        FORenderedTerminal* apMemory = static_cast<FORenderedTerminal*>(ourMalloc(sizeof(FORenderedTerminal)));
        if (apMemory) {
            prm = ::new (apMemory) FORenderedTerminal;
        }
        else {
            prm = nullptr;
        }
        prm->pMenuTile = pmenu;
        prm->SetMenuRoot(pmenu->GetModel());
        InterfaceManager* v2 = InterfaceManager::Instance();
        v2->EnterRenderedMenu(prm);
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
    InterfaceManager* singleton;
    (&appTimer)->SetGlobalTimeMultiplier(1.0, true);
    if (InterfaceManager::Instance() && ((singleton = InterfaceManager::Instance()), singleton->IsDoneFirstInit())) {
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
         QueuedMenuOpen.eOpenType == OPEN_PAUSE || QueuedMenuOpen.eOpenType == OPEN_MESSAGE)) {
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
                                   QueuedMenuOpen.pMenuRef, QueuedMenuOpen.pGreetTopic, QueuedMenuOpen.pSpeakeras
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
                void* pTileMemory = ourMalloc(sizeof(TileMenu));
                TileMenu* pTile = pTileMemory ? ::new (pTileMemory) TileMenu(nullptr, nullptr) : nullptr;
                void* pMenuMemory = ourMalloc(sizeof(PlayerNameEntryMenu));
                PlayerNameEntryMenu* pMenu = pMenuMemory ? ::new (pMenuMemory) PlayerNameEntryMenu : nullptr;
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
         QueuedMessageOpen.eOpenType == OPEN_PAUSE || QueuedMessageOpen.eOpenType == OPEN_MESSAGE)) {
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
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp", 176
    );
    InterfaceManager::Initialize(1, abInitialLoad);
    pContext->~AutoMemContext();
}

void Interface::InitSecond() {
    alignas(AutoMemContext) uint8_t kContextMemory[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (kContextMemory) AutoMemContext(
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp", 189
    );
    InterfaceManager::Initialize(2, true);
    pContext->~AutoMemContext();
}

void Interface::Click(BSRenderedTexture* ap3DTarget, BSRenderedTexture* pMenuFXTexture) {
    bool bRendered = false;
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit() &&
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
    if (((int32_t)aeMenuType) == 7) {
        Interface::QueuedMessageOpen.eOpenType = Interface::OPEN_MESSAGE;
        Interface::QueuedMessageOpen.pMenuRef = apMenuRef;
        Interface::QueuedMessageOpen.pGreetTopic = apGreeting;
        Interface::QueuedMessageOpen.pSpeakeras = apSpeakAs;
        Interface::QueuedMessageOpen.eContainerMode = static_cast<CONTAINER_MENU_MODE>(aeContainerMode);
    }
    else {
        Interface::QueuedMenuOpen.eOpenType = static_cast<Interface::OPEN_REQUEST>((int32_t)aeMenuType);
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
    bool bfound = false;
    if ((apString) && (*apString) && (apOutBuffer) && (aiBufferSize)) {
        int32_t iindex = Interface::MatchTextReplaceConstantToGameSettings(apString);
        if (iindex < 28) {
            Interface::GetControlPushTextFor(iindex, apOutBuffer, abAddPushText);
            bfound = true;
        }
        if (!bfound) {
            TESGlobal* pglob = static_cast<TESGlobal*>(TESForm::GetFormByEditorID(apString));
            if ((pglob) && (pglob->GetFormType() == GLOB_ID)) {
                if (pglob->GetType() == 102) {
                    BSsprintf(apOutBuffer, static_cast<int32_t>(aiBufferSize), "%.2f", pglob->fValue);
                }
                else {
                    BSsprintf(
                        apOutBuffer, static_cast<int32_t>(aiBufferSize), "%i", static_cast<int32_t>(pglob->fValue)
                    );
                }
                bfound = true;
            }
        }
        if (!bfound && !BSstricmp(apString, "PCName")) {
            const char* FullName = pPlayer->GetFullName();
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), FullName);
            bfound = true;
        }
        if (!bfound && !BSstricmp(apString, "PCRace")) {
            const char* RaceFullName = pPlayer->GetRaceFullName();
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), RaceFullName);
            bfound = true;
        }
        if (!bfound && !BSstricmp(apString, "PCSex")) {
            TESActorBase::SEX Sex = pPlayer->GetSex();
            const char* v11 = pSexNameGS[Sex]->String();
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), v11);
            bfound = true;
        }
        if (!bfound && !BSstricmp(apString, "PCSexPronoun")) {
            TESActorBase::SEX v6 = pPlayer->GetSex();
            const char* v7 = pSexPronounGS[v6]->String();
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), v7);
            bfound = true;
        }
        if (!bfound && !BSstricmp(apString, "PCSexPossessive")) {
            TESActorBase::SEX v8 = pPlayer->GetSex();
            const char* v12 = pSexPossessiveGS[v8]->String();
            BSstrcpy(apOutBuffer, static_cast<int32_t>(aiBufferSize), v12);
            bfound = true;
        }
        if (!bfound) {
            return Interface::TestConstantForGameSettings(apString, apOutBuffer);
        }
    }

    return bfound;
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
    static float fsize = [&] {
        double screenHeight;
        screenHeight = Interface::GetScreenHeight();
        return (float)(screenHeight / ((double)Renderer::GetScreenHeight()));
    }();
    return fsize;
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
        return (IsPipBoyOpened()) &&
            (IsMenuIDVisible(eStatsMenu, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(eInventoryMenu, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(ePipBoyRepair, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(eItemModMenu, abFadeStatesThatCountAsVisible) ||
             IsMenuIDVisible(ePipBoyData, abFadeStatesThatCountAsVisible));
    }
    if (abFadeStatesThatCountAsVisible != 0) {
        Tile* pMenuTile = Menu::GetMenuByClass(aeMenuID);
        if (pMenuTile != nullptr && pMenuTile->GetMenu() != nullptr &&
            (pMenuTile->GetMenu()->GetFadeState() & static_cast<int32_t>(abFadeStatesThatCountAsVisible)) == 0) {
            return false;
        }
    }
    return IsMenuVisible(aeMenuID) != 0;
}

// Calls InterfaceManager::Instance afresh for each use, as the game does.
Interface::enumMenus Interface::GetTopMenuID() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<enumMenus>(InterfaceManager::Instance()->GetEnterStackTop());
    }
    return eNoMenu;
}

// Calls InterfaceManager::Instance afresh for each use, as the game does.
Interface::enumMenus Interface::GetMenuFromStack(int32_t aiIndex) {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<enumMenus>(InterfaceManager::Instance()->GetEnterStack(aiIndex));
    }
    return eNoMenu;
}

// Calls InterfaceManager::Instance afresh for each use, as the game does.
Interface::enumMenus Interface::GetMenuModeType() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<enumMenus>(InterfaceManager::Instance()->GetEnterStack(0));
    }
    return eNoMenu;
}

uint32_t Interface::GetMenuCount() {
    return 10;
}

// Calls InterfaceManager::Instance afresh for each use, as the game does.
bool Interface::IsTopMenuID(enumMenus aeMenuID) {
    if (InterfaceManager::Instance() == nullptr || !InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    int32_t aiTop = InterfaceManager::Instance()->GetEnterStackTop();
    if ((aiTop == eMainFour) &&
        (aeMenuID == eStatsMenu || aeMenuID == eInventoryMenu || aeMenuID == ePipBoyRepair || aeMenuID == ePipBoyData ||
         aeMenuID == eItemModMenu))
    // The four main menus are all shown at once, so any of them counts as the top one.
    {
        return IsMenuIDVisible(aeMenuID, static_cast<FADE_STATE>(0));
    }

    return aiTop == aeMenuID;
}

void Interface::AddToEnterStack(int32_t aiID, bool abForceRemoval) {
    // The PC body never reads the removal flag its callers push.
    static_cast<void>(abForceRemoval);
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
    InterfaceManager* singleton = InterfaceManager::Instance();
    singleton->RecursiveFade(apNode, afAmount, afLastAlpha);
}

// Calls InterfaceManager::Instance afresh for each use, as the game does.
bool Interface::IsInMenuMode() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->IsInMenuMode();
    }
    return false;
}

void Interface::ShowMenus() {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v0 = InterfaceManager::Instance();
        if (v0->bFirstInit) {
            InterfaceManager* pim = InterfaceManager::Instance();
            int32_t ifrontmost = static_cast<int32_t>(pim->GetMenusRoot()->GetFloat(Tile::kTileValue_MenuLevel));
            if (!ifrontmost) {
                InterfaceManager* v1 = InterfaceManager::Instance();
                v1->PreLoadMenus();
                ifrontmost = 1003;
            }
            switch (ifrontmost) {
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
                default: {
                    return;
                }
            }
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

// KEEP: not registered. FNVD_REPLACES has no fnvd::describe overload for a C-variadic function
// (`void (*)(BSStringT<char>, ...)`), so the harness cannot register this body. It follows the asm: the string's
// text goes to the console when it is open, or when CombatController's verbose messages are on, and the by-value
// string is released on every path (the exe's single join point at 0x703D23).
void Interface::PrintLine(BSStringT<char> aString, ...) {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit() &&
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

// KEEP: not registered. FNVD_REPLACES cannot describe this signature (no matching fnvd::describe), so the harness
// cannot register the body; callers reach the exe's through the template forwarder in Interface.h. It follows the
// asm: the result callback is the fourth argument, the sound name is null (0x703ED6), the box is modal at -1/-1 and
// the opacity and width come after the first button's text.
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
        const_cast<char*>(aText), const_cast<char*>(apcIconFilename), const_cast<char*>(apcTitle), nullptr, afpResult,
        aiFirstButtonResult != 0, aiWarningContext, const_cast<char*>(axFirst), &xlist, true, -1, -1, 0.0f,
        afBackgroundTextureOpacityOverride, afMenuWidthOverride
    );
    va_end(xlist);
    return bResult;
}

// KEEP: not registered, for the same reason as CreateMessageMenu above (0x703F10 is C-variadic too). The only
// difference in the asm is that the sound name (the fourth argument) is passed through at 0x703F66.
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
        const_cast<char*>(aText), const_cast<char*>(apcIconFilename), const_cast<char*>(apcTitle),
        const_cast<char*>(apcSound), afpResult, aiFirstButtonResult != 0, aiWarningContext, const_cast<char*>(axFirst),
        &xlist, true, -1, -1, 0.0f, afBackgroundTextureOpacityOverride, afMenuWidthOverride
    );
    va_end(xlist);
    return bResult;
}

// KEEP: not registered. FNVD_REPLACES has no fnvd::describe overload for a C-variadic function
// (`char (*)(const char*, int, int, const char*, ...)`), so the harness cannot register this body. It follows the asm:
// the result byte is cleared to -1 (0x704016), the fade to black is removed at once (0x70402B), the box is queued with
// ModalMessageMenuCallback, and the USER32 queue is pumped with PM_REMOVE until that callback answers. The exe
// returns the result byte as it stands (0x704123), so the return type is the byte's, not bool.
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
            aText, nullptr, nullptr, nullptr, &ModalMessageMenuCallback, aiFirstButtonResult != 0, aiWarningContext,
            axFirst, &xlist, true, -1, -1, 0.0f, 0.0f, 0.0f
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

// The manager is fetched and dropped, as the game does; the check itself is static.
bool Interface::IsCurrentRenderedMenuTopmost() {
    InterfaceManager::Instance();
    return InterfaceManager::IsCurrentRenderedMenuTopmost();
}

// The texts after axFirstText are read twice: once to build and measure the buttons, once to place them.
void Interface::InitializeTabline(Tile* apTabline, int32_t aiStartingID, const char* axFirstText, ...) {
    BSStringT<char> sText;
    int32_t itabsTotalWidth = 0;
    va_list vList;
    va_start(vList, axFirstText);
    sText.Set(axFirstText, 0);
    int32_t itabCt = 0;
    for (; sText.GetLength(); ++itabCt) {
        Tile* pTemp = apTabline->GetMenu()->RenderTemplate(apTabline, "TabButtonTemplate", nullptr);
        pTemp->Set(4036, sText.String(), true);
        pTemp->SetName(sText.String());
        pTemp->Set(4012, itabCt);
        pTemp->Set(4010, aiStartingID + itabCt);
        Tile::UpdateAll(false);
        itabsTotalWidth += static_cast<int32_t>(pTemp->GetFloat(4017));
        sText = va_arg(vList, const char*);
    }
    va_end(vList);
    int32_t ileftLineLength = static_cast<int32_t>((apTabline->GetFloat(4017) - itabsTotalWidth) / (itabCt + 1));
    int32_t icurrX = ileftLineLength;
    apTabline->Set(Tile::TextToTrait("_LeftLineLength"), ileftLineLength);
    apTabline->Set(Tile::TextToTrait("_ButtonCount"), itabCt);
    va_start(vList, axFirstText);
    sText.Set(axFirstText, 0);
    while (sText.GetLength()) {
        Tile* pTemp = apTabline->GetChildByName(sText.String());
        pTemp->Set(Tile::TextToTrait("_x"), icurrX);
        icurrX += static_cast<int32_t>(pTemp->GetFloat(4017)) + ileftLineLength;
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
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return nullptr;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return &v1->HUDEffects;
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

TESObjectREFR* Interface::GetConsoleSelection() {
    InterfaceManager* pManager = InterfaceManager::Instance();
    if (pManager != nullptr && pManager->IsDoneFirstInit()) {
        return pManager->GetConsoleSelection();
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
    void* pMemory = ourMalloc(sizeof(Model));
    pTmpModel = pMemory ? ::new (pMemory) Model(apFilename, pStream, true, false) : nullptr;
    NiNode* pRoot = pTmpModel->GetModel();
    pStream->~BSStream();
    return pRoot;
}

NiNode* Interface::CopyOrDeepCopyNode(NiNode* apNode) {
    NiNode* v2;
    NiNode* v3;
    BSCloningProcess apCloneProc;
    apCloneProc.~BSCloningProcess();
    ::new (&apCloneProc) BSCloningProcess(1.0);
    int32_t v5 = 0;
    if (HasMorpherController(apNode)) {
        v3 = static_cast<NiNode*>(TES::pInstance->CreateDeepCopySameTextures(apNode, apCloneProc));
        v5 = (-1);
        return v3;
    }

    v2 = static_cast<NiNode*>(apNode->Clone(apCloneProc));
    v5 = (-1);
    return v2;
}

bool Interface::PlayNifSounds(NiControllerSequence* apSequence) {
    return (bool)((uint8_t)BSAnimGroupSequence::PlaySounds(apSequence, nullptr));
}

// InterfaceManager::Instance is called afresh for the test and the flag.
bool Interface::InDialog() {
    if (InterfaceManager::Instance() != nullptr && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return DialogMenu::IsDialogActive();
    }
    return false;
}

void Interface::SetDialogPause(bool abPause) {
    InterfaceManager* singleton;
    Menu* menu;
    DialogMenu* v3;
    TileMenu* menuByClass;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1009));
            if (menuByClass) {
                menu = menuByClass->Tile::GetMenu();
                v3 = dynamic_cast<DialogMenu*>(menu);
            }
            else {
                v3 = nullptr;
            }
            if (v3) {
                v3->SetPause(abPause);
            }
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
    AutoMemContext apMemory(
        MEM_CONTEXT_INTERFACE, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Interface.cpp", 2305
    );
    if (InterfaceManager::Instance()) {
        InterfaceManager* v6 = InterfaceManager::Instance();
        if ((v6->IsDoneFirstInit()) && (HUDMainMenu::pme)) {
            // Interface::ShowMessage takes aeEmotion as an int and passes it on as the HUD's emotion.
            HUDMainMenu::pme->ShowMessage(
                apString, static_cast<HUDMainMenu::HUD_MESSAGE_EMOTION>(aeEmotion), apIconFilename, apSoundName,
                afShowSeconds, abMoveToHead
            );
        }
    }
    return false;
}

void Interface::SetRegionText(TESRegion* apRegion, BSStringT<char> xDisplayText, bool bUpdateLastRegion) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v3 = InterfaceManager::Instance();
        if (v3->bFirstInit) {
            HUDMainMenu::HUDRegionData a1;
            a1.bUpdateLastRegion = bUpdateLastRegion;
            a1.pRegion = apRegion;
            if (xDisplayText.pString) {
                BSstrcpy(a1.strDisplayText, 260, xDisplayText.pString);
            }
            else {
                a1.strDisplayText[0] = 0;
            }
            TESRegion::UpdateHUDRegionIndicator(a1);
        }
    }
}

void Interface::CreateSleepMenu(bool abSleep) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v1 = InterfaceManager::Instance();
        if (v1->bFirstInit && InterfaceManager::IsPipboyClosed()) {
            SleepWaitMenu* pmenu;
            Tile* ptile = Menu::GetMenuByClass(1012);
            if (ptile) {
                Menu* Menu_ = ptile->GetMenu();
                pmenu = dynamic_cast<SleepWaitMenu*>(Menu_);
            }
            else {
                pmenu = nullptr;
            }
            if (pmenu) {
                delete pmenu;
            }
            SleepWaitMenu::Create(abSleep);
        }
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

// InterfaceManager::Instance is deliberately called twice, once for the null test and once for the receiver, as the
// game does.
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
    InterfaceManager* singleton = InterfaceManager::Instance();
    singleton->SetFirstChanceTextureRelease(abSet);
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
    InterfaceManager* singleton;
    Menu* menu;
    Menu* v2;
    Tile* v3;
    Menu* v4;
    Menu* v5;
    Tile* v6;
    Menu* v7;
    Menu* v8;
    Tile* v9;
    Menu* v10;
    Menu* v11;
    Tile* v12;
    Menu* v13;
    Menu* v14;
    Tile* v15;
    TileMenu* v16;
    TileMenu* v17;
    TileMenu* menuByClass;
    InterfaceManager* v19;
    TileMenu* v20;
    TileMenu* v21;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v19 = InterfaceManager::Instance();
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
            v16 = static_cast<TileMenu*>(Menu::GetMenuByClass(1002));
            v17 = static_cast<TileMenu*>(Menu::GetMenuByClass(1035));
            v21 = static_cast<TileMenu*>(Menu::GetMenuByClass(1061));
            v20 = static_cast<TileMenu*>(Menu::GetMenuByClass(1023));
            if (menuByClass) {
                menu = menuByClass->GetMenu();
                menu->SetFadeState(static_cast<Interface::FADE_STATE>(4));
                v2 = menuByClass->GetMenu();
                v3 = v2->Menu::GetMenuTile();
                v3->Set(4003, 0);
            }
            if (v16) {
                v4 = v16->GetMenu();
                v4->SetFadeState(static_cast<Interface::FADE_STATE>(4));
                v5 = v16->GetMenu();
                v6 = v5->Menu::GetMenuTile();
                v6->Set(4003, 0);
            }
            if (v17) {
                v7 = v17->GetMenu();
                v7->SetFadeState(static_cast<Interface::FADE_STATE>(4));
                v8 = v17->GetMenu();
                v9 = v8->Menu::GetMenuTile();
                v9->Set(4003, 0);
            }
            if (v21) {
                v10 = v21->GetMenu();
                v10->SetFadeState(static_cast<Interface::FADE_STATE>(4));
                v11 = v21->GetMenu();
                v12 = v11->Menu::GetMenuTile();
                v12->Set(4003, 0);
            }
            if (v20) {
                v13 = v20->GetMenu();
                v13->SetFadeState(static_cast<Interface::FADE_STATE>(4));
                v14 = v20->GetMenu();
                v15 = v14->Menu::GetMenuTile();
                v15->Set(4003, 0);
            }
            Interface::CloseContainer();
            if (StartMenu::PauseMenuIsActive()) {
                StartMenu::Close(true);
            }
            v19->EnterRenderedMenu(nullptr);
        }
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
// visibility traits. Calls InterfaceManager::Instance afresh at each use, as the game does.
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
            if ((pglow) && (pglow->GetModel())) {
                bool bCull = pglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pglow->GetModel()->SetAppCulled(bCull);
            }

            if ((pnoglow) && (pnoglow->GetModel())) {
                bool bCull = pnoglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pnoglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pnoglow->GetModel()->SetAppCulled(bCull);
            }
        }
    }
}

void Interface::EmergencyCloseAllMenusAndBreakStuff() {
    InterfaceManager* singleton;
    InterfaceManager* v1;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v1 = InterfaceManager::Instance();
            v1->EmergencyCloseAllMenusAndBreakStuff();
        }
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

// The tabs are clamped to the menus' four pages through BSMAX and BSMIN; each step fetches the interface manager again,
// as the game does. A main menu id the interface does not know falls back to the stats menu.
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
    InterfaceManager* singleton;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            pMoviePlayer->GetPlayingSequence();
        }
    }
}

void Interface::UpdateScreenShape(int32_t isizex, int32_t isizey) {
    InterfaceManager* singleton;
    InterfaceManager* v3;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v3 = InterfaceManager::Instance();
            v3->UpdateScreenShape(isizex, isizey);
        }
    }
}

void Interface::SetMenuSystemVisible(bool abVisible) {
    InterfaceManager* singleton;
    InterfaceManager* v2;
    Tile* combatState;
    NiNode* node;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v2 = InterfaceManager::Instance();
            combatState = v2->GetMenusRoot();
            node = combatState->GetModel();
            node->NiAVObject::SetAppCulled((uint8_t)(!abVisible));
        }
    }
}

void Interface::ToggleSafeZone(int32_t abForceSetTo) {
    InterfaceManager* singleton;
    InterfaceManager* v2;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v2 = InterfaceManager::Instance();
            v2->ToggleSafeZone(abForceSetTo);
        }
    }
}

char Interface::GetInventoryMenuVisible() {
    return Interface::IsMenuVisible(Interface::eInventoryMenu);
}

void Interface::DumpMenuPackingInfo() {
    InterfaceManager* singleton;
    InterfaceManager* v1;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v1 = InterfaceManager::Instance();
            v1->DumpMenuPackingInfo();
        }
    }
}

char Interface::GetStatsPageNumber() {
    if (!InterfaceManager::Instance()) {
        return (-1);
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return (-1);
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return v1->GetStatsPageNumber();
}

char Interface::GetMagicPageNumber() {
    if (!InterfaceManager::Instance()) {
        return (-1);
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return (-1);
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return (int8_t)((uint8_t)v1->GetMagicPageNumber());
}

char Interface::GetMapPageNumber() {
    if (!InterfaceManager::Instance()) {
        return (-1);
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return (-1);
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return (int8_t)v1->GetMapPageNumber();
}

void Interface::SetSurgeryCursorPos(int32_t aiXPos, int32_t aiYPos) {
    InterfaceManager* singleton;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            SurgeryMenu::SetCursorPos(aiXPos, aiYPos);
        }
    }
}

NiNode* Interface::GetDebugTextRoot() {
    InterfaceManager* singleton = InterfaceManager::Instance();
    return singleton->GetDebugTextRoot();
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
    return (bool)((int8_t)VATSMenu::GetIsARescan());
}

bool Interface::IsActivatePickFuzzy() {
    if (!InterfaceManager::Instance()) {
        return false;
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return false;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return (bool)((uint8_t)v1->IsActivatePickFuzzy());
}

void Interface::ClearMenuQueue() {
    Interface::QueuedMenuOpen.eOpenType = Interface::OPEN_NONE;
    Interface::QueuedMenuOpen.pMenuRef = nullptr;
    Interface::QueuedMessageOpen.eOpenType = Interface::OPEN_NONE;
    Interface::QueuedMessageOpen.pMenuRef = nullptr;
}

void Interface::SetCrosshairRef(intptr_t a2) {
    InterfaceManager* singleton;
    InterfaceManager* v2;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v2 = InterfaceManager::Instance();
            v2->SetCrosshairRef(a2);
        }
    }
}

void Interface::Free() {
    InterfaceManager* singleton;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            InterfaceManager::Free();
        }
    }
}

void Interface::CreateRecipeMenu(TESObjectREFR* apVendor, TESForm* pCategory) {
    InterfaceManager* singleton;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            RecipeMenu::Create(apVendor, static_cast<TESRecipeCategory*>(pCategory));
        }
    }
}

InterfaceManager* Interface::SetQueuedPipboyTab(uint8_t a1, float a2, float a3, int32_t a4) {
    InterfaceManager* singleton = InterfaceManager::Instance();
    return singleton->SetQueuedPipboyTab(a1, a2, a3, a4);
}

void Interface::MessageMenuYesToAll() {
    MessageMenu::ProcessYesToAll();
}

TESObjectREFR* Interface::GetActivateREFR() {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return nullptr;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return v1->pActivateRef;
}

void Interface::CloseSleepMenu() {
    InterfaceManager* singleton;
    Menu* menu;
    SleepWaitMenu* v2;
    TileMenu* menuByClass;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1012));
            if (menuByClass) {
                menu = menuByClass->Tile::GetMenu();
                v2 = dynamic_cast<SleepWaitMenu*>(menu);
            }
            else {
                v2 = nullptr;
            }
            if (v2) {
                SleepWaitMenu::Close();
            }
        }
    }
}

void Interface::CloseVATSMenu() {
    InterfaceManager* singleton;
    Menu* menu;
    VATSMenu* v2;
    TileMenu* menuByClass;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1056));
            if (menuByClass) {
                menu = menuByClass->Tile::GetMenu();
                v2 = dynamic_cast<VATSMenu*>(menu);
            }
            else {
                v2 = nullptr;
            }
            if (v2) {
                VATSMenu::Close();
            }
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

void Interface::SaveGameTES() {
    int8_t statsMenuTab = Interface::GetStatsPageNumber();
    int8_t inventoryMenuTab = Interface::GetInventoryPageNumber();
    int8_t byte14 = Interface::GetMagicPageNumber();
    int8_t mapMenuTab = Interface::GetMapPageNumber();
    int32_t lastPipboyTab = Interface::GetLastPipboyTab();
    if (((int32_t)statsMenuTab) < 1) {
        statsMenuTab = 1;
    }
    if (((int32_t)inventoryMenuTab) < 1) {
        inventoryMenuTab = 1;
    }
    if (((int32_t)byte14) < 1) {
        byte14 = 1;
    }
    if (((int32_t)mapMenuTab) < 1) {
        mapMenuTab = 1;
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&statsMenuTab, 1);
    pSaveLoadGameOLD->SaveGameDataOLD(&inventoryMenuTab, 1);
    pSaveLoadGameOLD->SaveGameDataOLD(&byte14, 1);
    pSaveLoadGameOLD->SaveGameDataOLD(&mapMenuTab, 1);
    pSaveLoadGameOLD->SaveGameDataOLD(&lastPipboyTab, 4);
}

bool Interface::ContainerIsRecipe() {
    return (Menu::GetMenuByClass(1077) != nullptr);
}

bool Interface::ContainerIsBarter() {
    return (Menu::GetMenuByClass(1053) != nullptr);
}

void Interface::SetDragDroppingState(int32_t abSetTo) {
    InterfaceManager* singleton;
    InterfaceManager* v2;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            v2 = InterfaceManager::Instance();
            v2->SetDragDroppingState((uint32_t)abSetTo);
        }
    }
}

void Interface::UpdateInventory() {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v0 = InterfaceManager::Instance();
        if (v0->bFirstInit) {
            if (Interface::GetInventoryMenuVisible()) {
                InventoryMenu::UpdateList();
            }
            else {
                if (Interface::GetStatsMenuVisible()) {
                    Tile* MenuByClass = Menu::GetMenuByClass(1003);
                    StatsMenu* Menu_ = static_cast<StatsMenu*>(MenuByClass->GetMenu());
                    Menu_->UpdateHealingOptions(StatsMenu::SM_HO_COUNT);
                }
                else {
                    if (Interface::IsMenuIDVisible(Interface::eBarterMenu, 0)) {
                        BarterMenu::ForceBarterUpdate(true);
                    }
                    else {
                        if (Interface::IsMenuIDVisible(Interface::eRecipeMenu, 0)) {
                            RecipeMenu::ForceRecipeUpdate(true);
                        }
                        else {
                            if (Interface::IsMenuIDVisible(Interface::eContainerMenu, 0)) {
                                ContainerMenu::ForceContainerUpdate();
                            }
                        }
                    }
                }
            }
        }
    }
}

void Interface::SetItemModMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect) {
    InterfaceManager* singleton;
    Menu* menu;
    Menu* v5;
    Menu* v6;
    Menu* v7;
    Menu* v8;
    InterfaceManager* v9;
    Tile* combatState;
    InterfaceManager* v11;
    FOPipboyManager* pipboy;
    InterfaceManager* v13;
    FOPipboyManager* v14;
    InterfaceManager* v15;
    FOPipboyManager* v16;
    Menu* v17;
    TileMenu* v18;
    TileMenu* itemModMenu;
    TileMenu* v20;
    uint8_t isMenuIDVisible;
    TileMenu* menuByClass;
    TileMenu* v23;
    if (InterfaceManager::Instance()) {
        singleton = InterfaceManager::Instance();
        if (singleton->IsDoneFirstInit()) {
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1003));
            v18 = static_cast<TileMenu*>(Menu::GetMenuByClass(1002));
            v20 = static_cast<TileMenu*>(Menu::GetMenuByClass(1035));
            v23 = static_cast<TileMenu*>(Menu::GetMenuByClass(1023));
            itemModMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(1061));
            isMenuIDVisible = (uint8_t)Interface::IsMenuIDVisible(Interface::eItemModMenu, abAnyVisibleState ? 11 : 0);
            if (abVisibility) {
                if (!itemModMenu) {
                    itemModMenu = static_cast<TileMenu*>(Interface::CreateItemModMenu());
                }
                if (!isMenuIDVisible) {
                    if (menuByClass) {
                        menu = menuByClass->GetMenu();
                        menu->StartFadeOut();
                        StatsMenu::Close();
                    }
                    if (v23) {
                        v5 = v23->GetMenu();
                        v5->StartFadeOut();
                    }
                    if (v20) {
                        v6 = v20->GetMenu();
                        v6->StartFadeOut();
                    }
                    if (itemModMenu) {
                        v7 = itemModMenu->GetMenu();
                        v7->StartFadeIn();
                    }
                    if (v18) {
                        v8 = v18->GetMenu();
                        v8->StartFadeOut();
                    }
                }
                if (itemModMenu) {
                    v9 = InterfaceManager::Instance();
                    combatState = v9->GetMenusRoot();
                    combatState->Set(6001, 1061);
                }
                if (abTriggerPipboyEffect) {
                    v11 = InterfaceManager::Instance();
                    pipboy = v11->GetPipboy();
                    pipboy->TriggerRandomEffect();
                    v13 = InterfaceManager::Instance();
                    v14 = v13->GetPipboy();
                    v14->ActivateButton(2u);
                }
                v15 = InterfaceManager::Instance();
                v16 = v15->GetPipboy();
                if (v16->GetInitialized()) {
                    InventoryMenu::SetKnobPosition(false);
                }
            }
            else {
                if ((isMenuIDVisible) && (itemModMenu)) {
                    v17 = itemModMenu->GetMenu();
                    v17->StartFadeOut();
                }
            }
        }
    }
}

int32_t Interface::GetDragDroppingState() {
    if (!InterfaceManager::Instance()) {
        return 0;
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return 0;
    }
    InterfaceManager* v1 = InterfaceManager::Instance();
    return (int32_t)v1->GetDragDroppingState();
}

void Interface::GetControlPushTextFor(int32_t aiIndex, char* axReturn, bool abAddPushText) {
    if (aiIndex >= 0 && aiIndex <= 28) {
        SettingT<GameSettingCollection>* pbuttonSetting = nullptr;
        *axReturn = 0;
        if (InterfaceManager::Has360Controller()) {
            Controls* pControls = appMain->GetControls();
            pbuttonSetting = psXBoxGamepadText[pControls->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 3u)];
        }
        else {
            Controls* v4 = appMain->GetControls();
            uint8_t v10 = v4->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 1u);
            if (v10 >= 9u) {
                Controls* v5 = appMain->GetControls();
                v10 = v5->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 0);
                if (v10 >= 238u) {
                    Controls* v6 = appMain->GetControls();
                    v10 = v6->RemapAction(static_cast<Controls::USERACTIONS>(aiIndex), 2u);
                    if (v10 < 8u) {
                        pbuttonSetting = psJoystick[v10];
                    }
                }
                else {
                    pbuttonSetting = static_cast<SettingT<GameSettingCollection>*>(psKeyboard[v10]);
                }
            }
            else {
                pbuttonSetting = psMouse[v10];
            }
        }
        if ((pbuttonSetting) && (pbuttonSetting->String())) {
            const char* v7 = pbuttonSetting->String();
            ourStrcpy(axReturn, v7);
        }

        if (abAddPushText) {
            BSStringT<char> xtemp;
            if (*axReturn) {
                xtemp.operator=(axReturn);
            }
            else {
                const char* v8 = sPCControlsTextNone.String();
                xtemp.operator=(v8);
            }
            if (pbuttonSetting && pbuttonSetting->Key() && BSstristr(pbuttonSetting->Key(), "trigger") != nullptr) {
                std::sprintf(axReturn, "%s %s", sPCControlsTriggerPrefix.String(), xtemp.String());
            }
            else {
                std::sprintf(axReturn, "%s %s", sPCControlsTextPrefix.String(), xtemp.String());
            }
        }
    }
}

// Shows only the glow (or the no-glow) children of apMenuTile and, with abPipBoyOnly, only a pip-boy menu (ID 112).
// apMenuTile's children are walked with the menus root's child list as GetNext's receiver (it only advances the
// position); this receiver is kept deliberately, as the game does.
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
            if ((pglow) && (pglow->GetModel())) {
                bool bCull = pglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
                    pglow->GetFloat(Tile::kTileValue_IsolatedVisible) == 0.0;
                pglow->GetModel()->SetAppCulled(bCull);
            }

            if ((pnoglow) && (pnoglow->GetModel())) {
                bool bCull = abGlowOnly || pnoglow->GetFloat(Tile::kTileValue_Visible) == 0.0 ||
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
    if (InterfaceManager::Instance()) {
        InterfaceManager* v1 = InterfaceManager::Instance();
        if (v1->bFirstInit) {
            BSStringT<char> xMessage;
            if (apRef) {
                int32_t CastingType = apRef->iFormID;
                xMessage.SPrintF("\"%s\" (%08x)", apRef->GetFullName(), CastingType);
                int32_t v8 = iDebugTextFont.Int() - 1;
                FontManager* Singleton = FontManager::Instance();
                Singleton->GetFont(v8)->GetBaseLine();
                float a4 = static_cast<float>(iDebugTextTopBottomOffset.Int());
                char* v5 = xMessage.pString;
                DebugText* v3 = DebugText::Instance(true);
                v3->Print(v5, 640.0f, a4, 2, -1, -1.0f, 0, nullptr);
            }
            InterfaceManager* v4 = InterfaceManager::Instance();
            v4->SetTargetREFR(apRef);
        }
    }
}

NiPoint3* Interface::GetActivatePickLocation(NiPoint3* apResult) {
    InterfaceManager* v1;
    if (InterfaceManager::Instance() && (v1 = InterfaceManager::Instance(), v1->bFirstInit)) {
        InterfaceManager* v2 = InterfaceManager::Instance();
        ::new (apResult) NiPoint3(v2->GetActivatePickLocation());
        return apResult;
    }

    *apResult = NiPoint3_ZERO;
    return apResult;
}

void Interface::SetFirstChanceTextureRelease_00703920() {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v0 = InterfaceManager::Instance();
        if (v0->IsDoneFirstInit()) {
            InterfaceManager* v1 = InterfaceManager::Instance();
            v1->SetFirstChanceTextureRelease(true);
            Tile::UpdateAll(false);
        }
    }
}

void Interface::SetRepairMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v3 = InterfaceManager::Instance();
        if (v3->bFirstInit) {
            Tile* pstat = Menu::GetMenuByClass(1003);
            Tile* pinv = Menu::GetMenuByClass(1002);
            Tile* prepair = Menu::GetMenuByClass(1035);
            Tile* pitemmod = Menu::GetMenuByClass(1061);
            Tile* pmap = Menu::GetMenuByClass(1023);
            bool bisVisible = Interface::IsMenuIDVisible(Interface::ePipBoyRepair, abAnyVisibleState ? 11 : 0);
            if (abVisibility) {
                if (!prepair) {
                    prepair = Interface::CreateRepairMenu();
                }
                if (!bisVisible) {
                    if (pstat) {
                        Menu* Menu_ = pstat->GetMenu();
                        Menu_->StartFadeOut();
                        StatsMenu::Close();
                    }
                    if (pmap) {
                        Menu* v5 = pmap->GetMenu();
                        v5->StartFadeOut();
                    }
                    if (pitemmod) {
                        Menu* v6 = pitemmod->GetMenu();
                        v6->StartFadeOut();
                    }
                    if (prepair) {
                        Menu* v7 = prepair->GetMenu();
                        v7->StartFadeIn();
                    }
                    if (pinv) {
                        Menu* v8 = pinv->GetMenu();
                        v8->StartFadeOut();
                    }
                }
                if (prepair) {
                    InterfaceManager* v9 = InterfaceManager::Instance();
                    v9->GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, 1035);
                }
                if (abTriggerPipboyEffect) {
                    InterfaceManager* v10 = InterfaceManager::Instance();
                    FOPipboyManager* Pipboy = v10->GetPipboy();
                    Pipboy->TriggerRandomEffect();
                    InterfaceManager* v12 = InterfaceManager::Instance();
                    FOPipboyManager* v13 = v12->GetPipboy();
                    v13->ActivateButton(2u);
                }
                InterfaceManager* v14 = InterfaceManager::Instance();
                FOPipboyManager* v15 = v14->GetPipboy();
                if (v15->GetInitialized()) {
                    InventoryMenu::SetKnobPosition(false);
                }
            }
            else {
                if ((bisVisible) && (prepair)) {
                    Menu* v16 = prepair->GetMenu();
                    v16->StartFadeOut();
                }
            }
        }
    }
}

void Interface::CreateRepairServicesMenu(Actor* apVendor) {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v1 = InterfaceManager::Instance();
        if (v1->IsDoneFirstInit()) {
            RepairServicesMenu::Create(apVendor);
        }
    }
}

void Interface::CreateVATSMenu() {
    if (InterfaceManager::Instance()) {
        InterfaceManager* v0 = InterfaceManager::Instance();
        if (v0->bFirstInit) {
            VATSMenu* pmenu;
            Tile* ptile = Menu::GetMenuByClass(1056);
            if (ptile) {
                Menu* Menu_ = ptile->GetMenu();
                pmenu = dynamic_cast<VATSMenu*>(Menu_);
            }
            else {
                pmenu = nullptr;
            }
            if (pmenu) {
                delete pmenu;
            }
            VATSMenu::Create();
        }
    }
}

bool Interface::SetInfoForRef(TESObjectREFR* apObject, bool abTelekinesis, bool abSameOkay) {
    InterfaceManager* v3;
    bool result = InterfaceManager::Instance() && (v3 = InterfaceManager::Instance(), v3->IsDoneFirstInit()) &&
        HUDMainMenu::SetInfoForRef(apObject, abTelekinesis, abSameOkay);
    return result;
}

void Interface::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t isettingsSize;
    uint8_t cstatsPage;
    char apDest[104];
    int32_t iData;
    uint8_t cinventoryPage;
    float fworldMapMagnification;
    uint8_t cmapPage;
    float flocalMapMagnification;
    InterfaceManager* v1 = InterfaceManager::Instance();
    iData = static_cast<int32_t>(v1->GetMenusRoot()->GetFloat(Tile::kTileValue_MenuLevel));
    cstatsPage = StatsMenu::GetCurrentStatsPage();
    cinventoryPage = InventoryMenu::GetCurrentInventoryPage();
    cmapPage = MapMenu::GetCurrentMapPage();
    apSaveGameBuffer->SaveDataEndian(&iData, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&cstatsPage, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cinventoryPage, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&cmapPage, 1u, 0);
    flocalMapMagnification = MapMenu::GetLocalMapMagnification();
    fworldMapMagnification = MapMenu::GetWorldMapMagnification();
    apSaveGameBuffer->SaveDataEndian(&flocalMapMagnification, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fworldMapMagnification, 4u, 0);
    ourMemset(apDest, 0, 100u);
    InterfaceManager* v2 = InterfaceManager::Instance();
    isettingsSize = v2->xTutorialManager.GetSaveLoadSize();
    InterfaceManager* v3 = InterfaceManager::Instance();
    v3->xTutorialManager.Save(apDest);
    apSaveGameBuffer->SaveDataEndian(&isettingsSize, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(apDest, isettingsSize, 0);
}

void Interface::ShowDDTIcon(bool abArmorPiercing) {
    HUDMainMenu::ShowDDTIcon(abArmorPiercing);
}

void Interface::ShowEnemyDDTIcon(bool abArmorPiercing) {
    HUDMainMenu::ShowEnemyDDTIcon(abArmorPiercing);
}

int16_t Interface::GetSaveSize() {
    int16_t v1 = 8;
    if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) >= 0x5Du) {
        return 9;
    }
    return v1;
}

uint8_t Controls::RemapAction(USERACTIONS eAction, uint32_t uiIndex) {
    return ucKeyBinds[uiIndex][eAction];
}

// The terminal's fade is its geometry's last fade percent, or fully shown without geometry.
float FORenderedTerminal::GetFadeAmount() {
    float fFade;
    if (spGeometry.operator->()) {
        fFade = spGeometry.operator->()->GetLastFadePercent();
    }
    else {
        fFade = 1.0f;
    }
    return fFade;
}

uint32_t InterfaceManager::TutorialManager::GetSaveLoadSize() {
    return BSMIN(1, 1) + 5;
}

void InterfaceManager::TutorialManager::Reset() {
    for (uint32_t i = 0; i < 41; ++i) {
        tMessages[i].bShowMessage = 0;
        tMessages[i].bMessageShown = 0;
        tMessages[i].uiShowDelay = 0;
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

void InterfaceManager::SetDragDroppingState(int32_t a1) {
    iDragDroppingState = a1;
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
    if ((MessageMenu::pMe) && (bMessageMenuDisabled)) {
        MessageMenu::Close();
    }

    if ((!MessageMenu::pMe && !bMessageMenuDisabled) && (MessageMenu::GetQueueHead())) {
        // The compare on the setting's value is signed.
        if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
            Interface::QueueMenuCreate(
                Interface::OPEN_MESSAGE, nullptr, nullptr, nullptr, Interface::CMM_STANDARD, false
            );
        }
        else {
            MessageMenu::Create();
        }
    }
}
