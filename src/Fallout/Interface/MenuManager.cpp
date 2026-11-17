#include "Fallout/Interface/MenuManager.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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
    Menu* ptop = nullptr;
    float depth = -1e+08f;
    for (NiTListItem<Tile*>* elem = InterfaceManager::Instance()->GetMenusRoot()->xChildren.m_pkHead; elem;
         elem = InterfaceManager::Instance()->GetMenusRoot()->xChildren.GetNextPos(elem)) {
        Tile* pmenutile = InterfaceManager::Instance()->GetMenusRoot()->xChildren.Get(elem);
        Menu* pmenu = nullptr;
        if (pmenutile) {
            pmenu = pmenutile->GetMenu();
        }
        if ((pmenu) && (pmenu->pMenu) &&
            (static_cast<double>(depth) < pmenu->pMenu->GetFloat(4013) && pmenu->pMenu->GetFloat(4007) != 6008.0 &&
             pmenu->xFadeState != Interface::FADING_OUT && pmenu->xFadeState != Interface::FADE_HIDDEN)) {
            depth = pmenu->pMenu->GetFloat(4013);
            ptop = pmenu;
        }
    }
    return ptop;
}

bool MenuManager::IsMenuOpen(int32_t aiClass) {
    Menu* pmenu;
    NiTListItem<Tile*>* elem = InterfaceManager::Instance()->GetMenusRoot()->xChildren.m_pkHead;
    do {
        if (!elem) {
            return false;
        }
        Tile* pmenutile = InterfaceManager::Instance()->GetMenusRoot()->xChildren.GetNext(elem);
        pmenu = nullptr;
        if (pmenutile) {
            pmenu = pmenutile->GetMenu();
        }
    }
    while (!pmenu || (!pmenu->pMenu || pmenu->GetClass() != aiClass));
    return pmenu->pMenu->IsTrue(4003) && pmenu->xFadeState != Interface::FADING_OUT;
}

Menu* MenuManager::CreateMenuByClass(int32_t aiMenuID) {
    void* result;
    switch (aiMenuID) {
        case 1001: {
            MessageMenu* v38;
            MessageMenu* v75 = static_cast<MessageMenu*>(ourMalloc(sizeof(MessageMenu)));
            if (v75) {
                v38 = ::new (v75) MessageMenu;
            }
            else {
                v38 = nullptr;
            }
            result = v38;
            break;
        }
        case 1002: {
            InventoryMenu* v35;
            InventoryMenu* v72 = static_cast<InventoryMenu*>(ourMalloc(sizeof(InventoryMenu)));
            if (v72) {
                v35 = ::new (v72) InventoryMenu;
            }
            else {
                v35 = nullptr;
            }
            result = v35;
            break;
        }
        case 1003: {
            StatsMenu* v34;
            StatsMenu* v71 = static_cast<StatsMenu*>(ourMalloc(sizeof(StatsMenu)));
            if (v71) {
                v34 = ::new (v71) StatsMenu;
            }
            else {
                v34 = nullptr;
            }
            result = v34;
            break;
        }
        case 1004: {
            HUDMainMenu* v33;
            HUDMainMenu* v70 = static_cast<HUDMainMenu*>(ourMalloc(sizeof(HUDMainMenu)));
            if (v70) {
                v33 = ::new (v70) HUDMainMenu;
            }
            else {
                v33 = nullptr;
            }
            result = v33;
            break;
        }
        case 1007: {
            LoadingMenu* v32;
            LoadingMenu* v69 = static_cast<LoadingMenu*>(ourMalloc(sizeof(LoadingMenu)));
            if (v69) {
                v32 = ::new (v69) LoadingMenu;
            }
            else {
                v32 = nullptr;
            }
            result = v32;
            break;
        }
        case 1008: {
            ContainerMenu* v31;
            ContainerMenu* v68 = static_cast<ContainerMenu*>(ourMalloc(sizeof(ContainerMenu)));
            if (v68) {
                v31 = ::new (v68) ContainerMenu;
            }
            else {
                v31 = nullptr;
            }
            result = v31;
            break;
        }
        case 1009: {
            DialogMenu* v29;
            DialogMenu* v66 = static_cast<DialogMenu*>(ourMalloc(sizeof(DialogMenu)));
            if (v66) {
                v29 = ::new (v66) DialogMenu;
            }
            else {
                v29 = nullptr;
            }
            result = v29;
            break;
        }
        case 1012: {
            SleepWaitMenu* v28;
            SleepWaitMenu* v65 = static_cast<SleepWaitMenu*>(ourMalloc(sizeof(SleepWaitMenu)));
            if (v65) {
                v28 = ::new (v65) SleepWaitMenu;
            }
            else {
                v28 = nullptr;
            }
            result = v28;
            break;
        }
        case 1013: {
            StartMenu* started;
            StartMenu* v64 = static_cast<StartMenu*>(ourMalloc(sizeof(StartMenu)));
            if (v64) {
                started = ::new (v64) StartMenu;
            }
            else {
                started = nullptr;
            }
            result = started;
            break;
        }
        case 1014: {
            LockPickMenu* v26;
            LockPickMenu* v63 = static_cast<LockPickMenu*>(ourMalloc(sizeof(LockPickMenu)));
            if (v63) {
                v26 = ::new (v63) LockPickMenu;
            }
            else {
                v26 = nullptr;
            }
            result = v26;
            break;
        }
        case 1016: {
            QuantityMenu* v25;
            QuantityMenu* apMemory = static_cast<QuantityMenu*>(ourMalloc(sizeof(QuantityMenu)));
            if (apMemory) {
                v25 = ::new (apMemory) QuantityMenu;
            }
            else {
                v25 = nullptr;
            }
            result = v25;
            break;
        }
        case 1023: {
            MapMenu* v36;
            MapMenu* v73 = static_cast<MapMenu*>(ourMalloc(sizeof(MapMenu)));
            if (v73) {
                v36 = ::new (v73) MapMenu;
            }
            else {
                v36 = nullptr;
            }
            result = v36;
            break;
        }
        case 1026: {
            BookMenu* v24;
            BookMenu* v61 = static_cast<BookMenu*>(ourMalloc(sizeof(BookMenu)));
            if (v61) {
                v24 = ::new (v61) BookMenu;
            }
            else {
                v24 = nullptr;
            }
            result = v24;
            break;
        }
        case 1027: {
            LevelUpMenu* v23;
            LevelUpMenu* v60 = static_cast<LevelUpMenu*>(ourMalloc(sizeof(LevelUpMenu)));
            if (v60) {
                v23 = ::new (v60) LevelUpMenu;
            }
            else {
                v23 = nullptr;
            }
            result = v23;
            break;
        }
        case 1035: {
            RepairMenu* v21;
            RepairMenu* v58 = static_cast<RepairMenu*>(ourMalloc(sizeof(RepairMenu)));
            if (v58) {
                v21 = ::new (v58) RepairMenu;
            }
            else {
                v21 = nullptr;
            }
            result = v21;
            break;
        }
        case 1036: {
            RaceSexMenu* v22;
            RaceSexMenu* v59 = static_cast<RaceSexMenu*>(ourMalloc(sizeof(RaceSexMenu)));
            if (v59) {
                v22 = ::new (v59) RaceSexMenu;
            }
            else {
                v22 = nullptr;
            }
            result = v22;
            break;
        }
        case 1047: {
            CreditsMenu* v39;
            CreditsMenu* v76 = static_cast<CreditsMenu*>(ourMalloc(sizeof(CreditsMenu)));
            if (v76) {
                v39 = ::new (v76) CreditsMenu;
            }
            else {
                v39 = nullptr;
            }
            result = v39;
            break;
        }
        case 1048: {
            CharGenMenu* v14;
            CharGenMenu* v51 = static_cast<CharGenMenu*>(ourMalloc(sizeof(CharGenMenu)));
            if (v51) {
                v14 = ::new (v51) CharGenMenu;
            }
            else {
                v14 = nullptr;
            }
            result = v14;
            break;
        }
        case 1051: {
            TextEditMenu* v3;
            TextEditMenu* v40 = static_cast<TextEditMenu*>(ourMalloc(sizeof(TextEditMenu)));
            if (v40) {
                v3 = ::new (v40) TextEditMenu;
            }
            else {
                v3 = nullptr;
            }
            result = v3;
            break;
        }
        case 1053: {
            BarterMenu* v30;
            BarterMenu* v67 = static_cast<BarterMenu*>(ourMalloc(sizeof(BarterMenu)));
            if (v67) {
                v30 = ::new (v67) BarterMenu;
            }
            else {
                v30 = nullptr;
            }
            result = v30;
            break;
        }
        case 1054: {
            SurgeryMenu* v16;
            SurgeryMenu* v53 = static_cast<SurgeryMenu*>(ourMalloc(sizeof(SurgeryMenu)));
            if (v53) {
                v16 = ::new (v53) SurgeryMenu;
            }
            else {
                v16 = nullptr;
            }
            result = v16;
            break;
        }
        case 1055: {
            HackingMenu* v18;
            HackingMenu* v55 = static_cast<HackingMenu*>(ourMalloc(sizeof(HackingMenu)));
            if (v55) {
                v18 = ::new (v55) HackingMenu;
            }
            else {
                v18 = nullptr;
            }
            result = v18;
            break;
        }
        case 1056: {
            VATSMenu* v15;
            VATSMenu* v52 = static_cast<VATSMenu*>(ourMalloc(sizeof(VATSMenu)));
            if (v52) {
                v15 = ::new (v52) VATSMenu;
            }
            else {
                v15 = nullptr;
            }
            result = v15;
            break;
        }
        case 1057: {
            ComputersMenu* v17;
            ComputersMenu* v54 = static_cast<ComputersMenu*>(ourMalloc(sizeof(ComputersMenu)));
            if (v54) {
                v17 = ::new (v54) ComputersMenu;
            }
            else {
                v17 = nullptr;
            }
            result = v17;
            break;
        }
        case 1058: {
            RepairServicesMenu* v19;
            RepairServicesMenu* v56 = static_cast<RepairServicesMenu*>(ourMalloc(sizeof(RepairServicesMenu)));
            if (v56) {
                v19 = ::new (v56) RepairServicesMenu;
            }
            else {
                v19 = nullptr;
            }
            result = v19;
            break;
        }
        case 1059: {
            TutorialMenu* v37;
            TutorialMenu* v74 = static_cast<TutorialMenu*>(ourMalloc(sizeof(TutorialMenu)));
            if (v74) {
                v37 = ::new (v74) TutorialMenu;
            }
            else {
                v37 = nullptr;
            }
            result = v37;
            break;
        }
        case 1060: {
            SPECIALBookMenu* v13;
            SPECIALBookMenu* v50 = static_cast<SPECIALBookMenu*>(ourMalloc(sizeof(SPECIALBookMenu)));
            if (v50) {
                v13 = ::new (v50) SPECIALBookMenu;
            }
            else {
                v13 = nullptr;
            }
            result = v13;
            break;
        }
        case 1061: {
            ItemModMenu* v20;
            ItemModMenu* v57 = static_cast<ItemModMenu*>(ourMalloc(sizeof(ItemModMenu)));
            if (v57) {
                v20 = ::new (v57) ItemModMenu;
            }
            else {
                v20 = nullptr;
            }
            result = v20;
            break;
        }
        case 1074: {
            LoveTesterMenu* v11;
            LoveTesterMenu* v48 = static_cast<LoveTesterMenu*>(ourMalloc(sizeof(LoveTesterMenu)));
            if (v48) {
                v11 = ::new (v48) LoveTesterMenu;
            }
            else {
                v11 = nullptr;
            }
            result = v11;
            break;
        }
        case 1075: {
            CompanionWheelMenu* v12;
            CompanionWheelMenu* v49 = static_cast<CompanionWheelMenu*>(ourMalloc(sizeof(CompanionWheelMenu)));
            if (v49) {
                v12 = ::new (v49) CompanionWheelMenu;
            }
            else {
                v12 = nullptr;
            }
            result = v12;
            break;
        }
        case 1076: {
            TraitSelectMenu* v10;
            TraitSelectMenu* v47 = static_cast<TraitSelectMenu*>(ourMalloc(sizeof(TraitSelectMenu)));
            if (v47) {
                v10 = ::new (v47) TraitSelectMenu;
            }
            else {
                v10 = nullptr;
            }
            result = v10;
            break;
        }
        case 1077: {
            RecipeMenu* v4;
            RecipeMenu* v41 = static_cast<RecipeMenu*>(ourMalloc(sizeof(RecipeMenu)));
            if (v41) {
                v4 = ::new (v41) RecipeMenu;
            }
            else {
                v4 = nullptr;
            }
            result = v4;
            break;
        }
        case 1080: {
            SlotMachineMenu* v9;
            SlotMachineMenu* v46 = static_cast<SlotMachineMenu*>(ourMalloc(sizeof(SlotMachineMenu)));
            if (v46) {
                v9 = ::new (v46) SlotMachineMenu;
            }
            else {
                v9 = nullptr;
            }
            result = v9;
            break;
        }
        case 1081: {
            BlackJackMenu* v8;
            BlackJackMenu* v45 = static_cast<BlackJackMenu*>(ourMalloc(sizeof(BlackJackMenu)));
            if (v45) {
                v8 = ::new (v45) BlackJackMenu;
            }
            else {
                v8 = nullptr;
            }
            result = v8;
            break;
        }
        case 1082: {
            RouletteMenu* v6;
            RouletteMenu* v43 = static_cast<RouletteMenu*>(ourMalloc(sizeof(RouletteMenu)));
            if (v43) {
                v6 = ::new (v43) RouletteMenu;
            }
            else {
                v6 = nullptr;
            }
            result = v6;
            break;
        }
        case 1083: {
            CaravanMenu* v5;
            CaravanMenu* v42 = static_cast<CaravanMenu*>(ourMalloc(sizeof(CaravanMenu)));
            if (v42) {
                v5 = ::new (v42) CaravanMenu;
            }
            else {
                v5 = nullptr;
            }
            result = v5;
            break;
        }
        case 1084: {
            TraitMenu* v7;
            TraitMenu* v44 = static_cast<TraitMenu*>(ourMalloc(sizeof(TraitMenu)));
            if (v44) {
                v7 = ::new (v44) TraitMenu;
            }
            else {
                v7 = nullptr;
            }
            result = v7;
            break;
        }
        default: {
            Warning("MENUS: Unknown menu class!");
            result = nullptr;
            break;
        }
    }
    return static_cast<Menu*>(result);
}

Tile* MenuManager::ReloadMenu(int32_t aiMenuClass) {
    Tile* pmenu = Menu::GetMenuByClass(aiMenuClass);
    bool bpausemenu = StartMenu::PauseMenuIsActive();
    if (pmenu) {
        delete pmenu;
    }
    Tile* pmenu_2 = nullptr;
    if (MapMenu::GetClassStatic() == aiMenuClass) {
        pmenu_2 = Interface::CreateMapMenu();
        float afSecs = pmenu_2->GetFloat(4033);
        Menu* Menu_ = pmenu_2->GetMenu();
        InterfaceManager::NewTimer(Menu_, afSecs);
        Menu* v3 = pmenu_2->GetMenu();
        v3->xFadeState = Interface::FADING_IN;
    }
    else {
        switch (aiMenuClass) {
            case 1035: {
                pmenu_2 = Interface::CreateRepairMenu();
                float afSecsa = pmenu_2->GetFloat(4033);
                Menu* v4 = pmenu_2->GetMenu();
                InterfaceManager::NewTimer(v4, afSecsa);
                Menu* v5 = pmenu_2->GetMenu();
                v5->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1061: {
                pmenu_2 = Interface::CreateItemModMenu();
                float afSecsb = pmenu_2->GetFloat(4033);
                Menu* v6 = pmenu_2->GetMenu();
                InterfaceManager::NewTimer(v6, afSecsb);
                Menu* v7 = pmenu_2->GetMenu();
                v7->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1002: {
                pmenu_2 = Interface::CreateInventoryMenu();
                float afSecsc = pmenu_2->GetFloat(4033);
                Menu* v8 = pmenu_2->GetMenu();
                InterfaceManager::NewTimer(v8, afSecsc);
                Menu* v9 = pmenu_2->GetMenu();
                v9->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1003: {
                pmenu_2 = Interface::CreateStatsMenu();
                float afSecsd = pmenu_2->GetFloat(4033);
                Menu* v10 = pmenu_2->GetMenu();
                InterfaceManager::NewTimer(v10, afSecsd);
                Menu* v11 = pmenu_2->GetMenu();
                v11->xFadeState = Interface::FADING_IN;
                break;
            }
            case 1004: {
                return HUDMainMenu::Create();
            }
            default: {
                if (StartMenu::GetClassStatic() == aiMenuClass && StartMenu::Create(bpausemenu, false)) {
                    return Menu::GetMenuByClass(aiMenuClass);
                }

                switch (aiMenuClass) {
                    case 1007: {
                        return LoadingMenu::Create(nullptr, true, false);
                    }
                    case 1027: {
                        return LevelUpMenu::CreateMenu();
                    }
                    case 1008: {
                        Tile* MenuByClass = Menu::GetMenuByClass(1008);
                        ContainerMenu* pcontainer = dynamic_cast<ContainerMenu*>(MenuByClass);
                        if (pcontainer) {
                            return ContainerMenu::Create(pcontainer->pContainerRef, Interface::CMM_STANDARD);
                        }
                        break;
                    }
                    case 1026: {
                        Tile* v13 = Menu::GetMenuByClass(1008);
                        BookMenu* pbookmenu = dynamic_cast<BookMenu*>(v13);
                        if (pbookmenu) {
                            TESObjectREFR* pRef = pbookmenu->unk30;
                            TESObjectBOOK* pbook = dynamic_cast<TESObjectBOOK*>(pRef->GetObjectReference());
                            delete pbookmenu;
                            return BookMenu::Create(pbook, pRef);
                        }
                        break;
                    }
                }

                break;
            }
        }
    }
    return pmenu_2;
}

bool MenuManager::DeleteMenu(int32_t aiMenuClass) {
    Tile* MenuByClass = Menu::GetMenuByClass(aiMenuClass);
    if (!MenuByClass) {
        return false;
    }
    delete MenuByClass;
    return true;
}

void MenuManager::Init() {
    Menu::xMenuList.SetSize(84u);
}

// The manager singleton; created on demand when abCreate is set.
MenuManager* MenuManager::Instance(bool abCreate) {
    MenuManager* v2;
    if ((!abCreate) || MenuManager::pInstance) {
        return MenuManager::pInstance;
    }
    void* apMemory = ourMalloc(sizeof(MenuManager));
    if (apMemory) {
        v2 = ::new (apMemory) MenuManager;
    }
    else {
        v2 = nullptr;
    }
    MenuManager::pInstance = v2;
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

int32_t QuantityMenu::GetClass_007ABDA0() {
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

void StatsMenu::NamedEffectsList::Register(ActiveEffect* a1) {
    EffectSetting* item = a1->GetEffectSetting();
    if (!IsInList(item)) {
        Insert(a1->GetEffectSetting(), &StatsMenu_sortingFunction);
    }
}

StatsMenu::StatusEffectList::StatusEffectList() : BSSimpleList<StatsMenu::StatusEffect*>() {}

void* StatsMenu::BodyPartController::DeleteInstance() {
    void* result = StatsMenu::BodyPartController::pInstance;
    if (StatsMenu::BodyPartController::pInstance) {
        delete StatsMenu::BodyPartController::pInstance;
    }
    StatsMenu::BodyPartController::pInstance = nullptr;
    return result;
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

void SurgeryMenu::AttachTileByID(int32_t a1, Tile* a2) {
    if (a1 > 0 && a1 <= 18) {
        pTiles[GetIndex(a1)] = a2;
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
