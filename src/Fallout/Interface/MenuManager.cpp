#include "Fallout/Interface/MenuManager.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
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
#include "Fallout/Interface/Menus/QuantityMenu.h"
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
#include "Fallout/Interface/Menus/TextEntryMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Interface/Menus/TraitSelectMenu.h"
#include "Fallout/Interface/Menus/TutorialMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cstdint>

using ActorValueListBox = ListBox<ActorValue::Index>;

using LoveTesterTextItemArray = BSSimpleArray<LoveTesterMenu::LoveTesterTextItem*>;

MenuManager::MenuManager() {
    MenuManager::Init();
}

MenuManager::~MenuManager() {
    MenuManager::Release();
}

Menu* MenuManager::GetFrontmostMenu() {
    Menu* pTop = nullptr;
    float fDepth = -1e+08f;
    for (NiTListItem<Tile*>* pElem = InterfaceManager::Instance()->GetMenusRoot()->xChildren.m_pkHead; pElem;
         pElem = InterfaceManager::Instance()->GetMenusRoot()->xChildren.GetNextPos(pElem)) {
        Tile* pMenuTile = InterfaceManager::Instance()->GetMenusRoot()->xChildren.Get(pElem);
        Menu* pMenu = pMenuTile ? pMenuTile->GetMenu() : nullptr;
        if (pMenu &&
            pMenu->pMenu &&
            static_cast<double>(fDepth) < pMenu->pMenu->GetFloat(4013) &&
            pMenu->pMenu->GetFloat(4007) != 6008.0 &&
            pMenu->xFadeState != Interface::FADING_OUT &&
            pMenu->xFadeState != Interface::FADE_HIDDEN) {
            fDepth = pMenu->pMenu->GetFloat(4013);
            pTop = pMenu;
        }
    }
    return pTop;
}

bool MenuManager::IsMenuOpen(int32_t aiClass) {
    Menu* pMenu;
    NiTListItem<Tile*>* pElem = InterfaceManager::Instance()->GetMenusRoot()->xChildren.m_pkHead;
    do {
        if (!pElem) {
            return false;
        }
        Tile* pMenuTile = InterfaceManager::Instance()->GetMenusRoot()->xChildren.GetNext(pElem);
        pMenu = pMenuTile ? pMenuTile->GetMenu() : nullptr;
    }
    while (!pMenu || !pMenu->pMenu || pMenu->GetClass() != aiClass);
    return pMenu->pMenu->IsTrue(4003) && pMenu->xFadeState != Interface::FADING_OUT;
}

Menu* MenuManager::CreateMenuByClass(int32_t aiMenuID) {
    void* pMenu;
    switch (aiMenuID) {
        case 1001: {
            pMenu = new MessageMenu;
            break;
        }
        case 1002: {
            pMenu = new InventoryMenu;
            break;
        }
        case 1003: {
            pMenu = new StatsMenu;
            break;
        }
        case 1004: {
            pMenu = new HUDMainMenu;
            break;
        }
        case 1007: {
            pMenu = new LoadingMenu;
            break;
        }
        case 1008: {
            pMenu = new ContainerMenu;
            break;
        }
        case 1009: {
            pMenu = new DialogMenu;
            break;
        }
        case 1012: {
            pMenu = new SleepWaitMenu;
            break;
        }
        case 1013: {
            pMenu = new StartMenu;
            break;
        }
        case 1014: {
            pMenu = new LockPickMenu;
            break;
        }
        case 1016: {
            pMenu = new QuantityMenu;
            break;
        }
        case 1023: {
            pMenu = new MapMenu;
            break;
        }
        case 1026: {
            pMenu = new BookMenu;
            break;
        }
        case 1027: {
            pMenu = new LevelUpMenu;
            break;
        }
        case 1035: {
            pMenu = new RepairMenu;
            break;
        }
        case 1036: {
            pMenu = new RaceSexMenu;
            break;
        }
        case 1047: {
            pMenu = new CreditsMenu;
            break;
        }
        case 1048: {
            pMenu = new CharGenMenu;
            break;
        }
        case 1051: {
            pMenu = new TextEditMenu;
            break;
        }
        case 1053: {
            pMenu = new BarterMenu;
            break;
        }
        case 1054: {
            pMenu = new SurgeryMenu;
            break;
        }
        case 1055: {
            pMenu = new HackingMenu;
            break;
        }
        case 1056: {
            pMenu = new VATSMenu;
            break;
        }
        case 1057: {
            pMenu = new ComputersMenu;
            break;
        }
        case 1058: {
            pMenu = new RepairServicesMenu;
            break;
        }
        case 1059: {
            pMenu = new TutorialMenu;
            break;
        }
        case 1060: {
            pMenu = new SPECIALBookMenu;
            break;
        }
        case 1061: {
            pMenu = new ItemModMenu;
            break;
        }
        case 1074: {
            pMenu = new LoveTesterMenu;
            break;
        }
        case 1075: {
            pMenu = new CompanionWheelMenu;
            break;
        }
        case 1076: {
            pMenu = new TraitSelectMenu;
            break;
        }
        case 1077: {
            pMenu = new RecipeMenu;
            break;
        }
        case 1080: {
            pMenu = new SlotMachineMenu;
            break;
        }
        case 1081: {
            pMenu = new BlackJackMenu;
            break;
        }
        case 1082: {
            pMenu = new RouletteMenu;
            break;
        }
        case 1083: {
            pMenu = new CaravanMenu;
            break;
        }
        case 1084: {
            pMenu = new TraitMenu;
            break;
        }
        default: {
            Warning("MENUS: Unknown menu class!");
            pMenu = nullptr;
            break;
        }
    }
    return static_cast<Menu*>(pMenu);
}

Tile* MenuManager::ReloadMenu(int32_t aiMenuClass) {
    Tile* pMenu = Menu::GetMenuByClass(aiMenuClass);
    bool bPauseMenu = StartMenu::PauseMenuIsActive();
    if (pMenu) {
        delete pMenu;
    }
    pMenu = nullptr;
    if (MapMenu::GetClassStatic() == aiMenuClass) {
        pMenu = Interface::CreateMapMenu();
        float fSecs = pMenu->GetFloat(4033);
        InterfaceManager::NewTimer(pMenu->GetMenu(), fSecs);
        pMenu->GetMenu()->xFadeState = Interface::FADING_IN;
    }
    else {
        switch (aiMenuClass) {
            case 1035: {
                pMenu = Interface::CreateRepairMenu();
                float fSecs = pMenu->GetFloat(4033);
                InterfaceManager::NewTimer(pMenu->GetMenu(), fSecs);
                pMenu->GetMenu()->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1061: {
                pMenu = Interface::CreateItemModMenu();
                float fSecs = pMenu->GetFloat(4033);
                InterfaceManager::NewTimer(pMenu->GetMenu(), fSecs);
                pMenu->GetMenu()->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1002: {
                pMenu = Interface::CreateInventoryMenu();
                float fSecs = pMenu->GetFloat(4033);
                InterfaceManager::NewTimer(pMenu->GetMenu(), fSecs);
                pMenu->GetMenu()->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1003: {
                pMenu = Interface::CreateStatsMenu();
                float fSecs = pMenu->GetFloat(4033);
                InterfaceManager::NewTimer(pMenu->GetMenu(), fSecs);
                pMenu->GetMenu()->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1004: {
                return HUDMainMenu::Create();
            }
            default: {
                if (StartMenu::GetClassStatic() == aiMenuClass && StartMenu::Create(bPauseMenu, false)) {
                    return Menu::GetMenuByClass(aiMenuClass);
                }

                switch (aiMenuClass) {
                    case 1007: {
                        return LoadingMenu::Create(nullptr, true, false);
                    }
                    case 1027: {
                        return LevelUpMenu::Create();
                    }
                    case 1008: {
                        ContainerMenu* pContainer = dynamic_cast<ContainerMenu*>(Menu::GetMenuByClass(1008));
                        if (pContainer) {
                            return ContainerMenu::Create(pContainer->pContainerRef, Interface::CMM_STANDARD);
                        }
                        break;
                    }
                    case 1026: {
                        BookMenu* pBookMenu = dynamic_cast<BookMenu*>(Menu::GetMenuByClass(1008));
                        if (pBookMenu) {
                            TESObjectREFR* pRef = pBookMenu->pBook;
                            TESObjectBOOK* pBook = dynamic_cast<TESObjectBOOK*>(pRef->GetObjectReference());
                            delete pBookMenu;
                            return BookMenu::Create(pBook, pRef);
                        }
                        break;
                    }
                }

                break;
            }
        }
    }
    return pMenu;
}

bool MenuManager::DeleteMenu(int32_t aiMenuClass) {
    Tile* pMenu = Menu::GetMenuByClass(aiMenuClass);
    if (!pMenu) {
        return false;
    }
    delete pMenu;
    return true;
}

void MenuManager::Init() {
    Menu::xMenuList.SetSize(84u);
}

// The manager singleton; created on demand when abCreate is set.
MenuManager* MenuManager::Instance(bool abCreate) {
    if (!abCreate || MenuManager::pInstance) {
        return MenuManager::pInstance;
    }
    MenuManager::pInstance = new MenuManager;
    MenuManager::Init();
    return MenuManager::pInstance;
}

void MenuManager::Release() {
    MenuManager::pInstance = nullptr;
}

void MenuManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void MenuManager::Free() {
    if (MenuManager::pInstance) {
        delete MenuManager::pInstance;
        MenuManager::pInstance = nullptr;
    }
}

ListBoxBase::ListBoxBase() {}

// Puts apListBox first in the open list boxes; the list boxes' constructors call it.
void ListBoxBase::Append(ListBoxBase* apListBox) {
    InterfaceManager::xActiveListBoxes.AddHead(apListBox);
}

// Takes apListBox out of the open list boxes; the list boxes' destructors call it.
void ListBoxBase::Remove(ListBoxBase* apListBox) {
    InterfaceManager::xActiveListBoxes.Remove(apListBox);
}

LoveTesterMenu::LoveTesterMenu()
  : pTiles{}, eState(LoveTesterMenu::LTMS_START), eXBoxIndexState(LoveTesterMenu::LTMS_STRENGTH), fFrameAnimTimer(0.0f),
    bAnimFinished(true), iAvailablePoints(0), pDynamicGeometry(nullptr), pCabinetGeometry(nullptr) {}

QuantityMenu::QuantityMenu() {}

QuantityMenu::~QuantityMenu() {
    QuantityMenu::pMe = nullptr;
}

int32_t QuantityMenu::GetClassStatic() {
    return 1016;
}

SPECIALBookMenu::SpecialBookPage::SpecialBookPage() : Buttons(), TextItems() {}

// The arrays release themselves.
SPECIALBookMenu::SpecialBookPage::~SpecialBookPage() {}

SPECIALBookMenu::SPECIALBookMenu()
  : Menu(), pTiles(), eState(SBMS_START), eXBoxIndexState(SBMS_STRENGTH), fFrameAnimTimer(0.0f), bAnimFinished(true),
    iAvailablePoints(0), pBookGeometry(nullptr), Pages(), spAVValues(), spPointsText(), spNextPageButtonImage(),
    spPreviousPageButtonImage(), spOpenCloseBookButtonImage(), spAVLeftArrowButtonImage(), spAVRightArrowButtonImage() {
}

StatsMenu::NamedEffectsList::NamedEffectsList() : BSSimpleList<EffectSetting*>() {}

StatsMenu::NamedEffectsList::~NamedEffectsList() {
    Flush();
}

void StatsMenu::NamedEffectsList::Flush() {
    RemoveAll();
}

void StatsMenu::NamedEffectsList::Register(ActiveEffect* apActiveEffect) {
    EffectSetting* pEffectSetting = apActiveEffect->GetEffectSetting();
    if (!IsInList(pEffectSetting)) {
        Insert(apActiveEffect->GetEffectSetting(), &StatsMenu_sortingFunction);
    }
}

StatsMenu::StatusEffectList::StatusEffectList() : BSSimpleList<StatsMenu::StatusEffect*>() {}

void* StatsMenu::BodyPartController::DeleteInstance() {
    void* pOldInstance = StatsMenu::BodyPartController::pInstance;
    if (StatsMenu::BodyPartController::pInstance) {
        delete StatsMenu::BodyPartController::pInstance;
    }
    StatsMenu::BodyPartController::pInstance = nullptr;
    return pOldInstance;
}

StatsMenu::StatsMenu()
  : Menu(), pHealingOptionsA(), xNamedEffects(), pRadiationEffect(nullptr), pDehydrationEffect(nullptr),
    pHungerEffect(nullptr), pSleepEffect(nullptr), xStatusEffects(), pTiles(), lbSpecial(), lbSkills(), lbPerks(),
    lbGeneral(), lbStatusEffects(), lbReputation(), bLimbMode(false), bActiveEffectsUpdatedThisFrame(false) {}

SurgeryMenu::SurgeryMenu()
  : Menu(), eAnimation(SMA_NONE), pTiles(), ToolLocation(0.0f, 0.0f), Resistance(0.0f, 0.0f), fToolMovement(0.0f),
    uiWoundBurnTime(0), uiWoundCloseTime(0), bBurning(false), bToolBuriedInFlesh(false), BurningFleshSoundHand(),
    BurningWoundSoundHand(), fPainPercent(0.0f), uiPainTimer(0), uiHitPointTimer(0), uiDamageHealthTime(0),
    pStimPaks(nullptr), eCurrentDamagedPart(ActorValue::LEFT_ATTACK_CONDITION), eCurrentDamageLevel(SMWD_LIGHT),
    pSkinGeometry(), pFrameGeometry(nullptr), pSkinControllerManager(nullptr), pScar(nullptr), fFrameAnimTimer(0.0f),
    spPlayerLights(nullptr), spPlayer3DParent(nullptr), fSavedPlayerScale(0.0f), bPlayer3DCulled(false),
    bPipboyLightEffectCulled(false) {
    SurgeryMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
    SurgeryMenu::sp3DSkin = static_cast<NiNode*>(nullptr);
    SurgeryMenu::spPlayer3D = static_cast<NiAVObject*>(nullptr);
}

void SurgeryMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID > 0 && aiID <= 18) {
        pTiles[GetIndex(aiID)] = apTile;
    }
}

// The zero-based slot of body part aiID (1-18); any other ID gives 18. It is a member function, though it never reads
// the menu.
SurgeryMenu::TileIndex SurgeryMenu::GetIndex(int32_t aiID) {
    if (aiID <= 0 || aiID > 18) {
        return SurgeryMenu::SM_NUM_TILES;
    }
    return static_cast<SurgeryMenu::TileIndex>(aiID - 1);
}

TutorialMenu::TutorialMenu() : Menu(), pTiles(), iCurrentPage(0), iNumPages(0), pHelpManual(nullptr) {}

TutorialMenu::~TutorialMenu() {
    TutorialMenu::pMe = nullptr;
}

VATSMenu::VATSMenu() : pTiles{}, bHeadingReached(false), bConcentratedFirePerk(false), pLastLimb(nullptr) {}
