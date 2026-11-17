#include "Fallout/Interface/Menus/BookMenu.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/rtti.h"

BookMenu::BookMenu() : Menu(), pScrollBar(nullptr), pScrollMarker(nullptr) {}

BookMenu::~BookMenu() {}

void BookMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID == 11) {
        pScrollBar = apTile;
    }
    else {
        if (aiID == 12) {
            pScrollMarker = apTile;
        }
    }
}

void BookMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
    double realY;
    InterfaceManager* singleton;
    if (aiID == 12) {
        singleton = InterfaceManager::Instance();
        realY = (double)pScrollMarker->Tile::GetRealY();
        fScrollOffset = (float)(realY - ((double)singleton->GetCursorY()));
    }
}

void BookMenu::DoClick(int32_t aiID, Tile* apTarget) {
    ExtraDataList* extra;
    int16_t count;
    if (aiID == 31) {
        Close();
    }
    else {
        if (aiID == 32) {
            extra = unk30->TESObjectREFR::GetExtra();
            count = extra->GetCount();
            pPlayer->PickUpObject(unk30, (int32_t)count, (bool)0);
            Close();
        }
    }
}

void BookMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    double v3;
    double v4;
    float v6;
    InterfaceManager* singleton;
    if (aiID == 12) {
        singleton = InterfaceManager::Instance();
        v4 = ((double)singleton->GetCursorY()) + fScrollOffset;
        v3 = v4 - pScrollBar->Tile::GetRealY();
        v6 = (float)(v3 / pScrollMarker->Tile::GetFloat(4108));
        pScrollBar->Tile::Set(4105, (-999999999));
        pScrollBar->Tile::Set(4105, (int32_t)v6);
        pScrollBar->Tile::Set(4105, 0);
    }
}

void BookMenu::DoWheelMove(int32_t a1, Tile* a2) {
    InterfaceManager* pim = InterfaceManager::Instance();
    if (pMenu->IsTrue(Tile::kTileValue_User0)) {
        int32_t ipage = static_cast<int32_t>(pMenu->GetFloat(Tile::kTileValue_User9));
        if (pim->GetMouseWheel() <= 0) {
            pMenu->Set(Tile::kTileValue_User6, 2);
        }
        else {
            pMenu->Set(Tile::kTileValue_User6, -2);
        }
        pMenu->Set(Tile::kTileValue_User6, 0);
        if (static_cast<double>(ipage) != pMenu->GetFloat(Tile::kTileValue_User9)) {
            int32_t isound_type = static_cast<int32_t>(pMenu->GetFloat(Tile::kTileValue_User8));
            if (isound_type) {
                InterfaceManager::Instance();
                InterfaceManager::PlayMenuSound(static_cast<MENU_SOUNDS>(isound_type));
            }
        }
    }
    else {
        int32_t iold_y = static_cast<int32_t>(pScrollBar->GetFloat(Tile::kTileValue_User7));
        int32_t MouseWheel = pim->GetMouseWheel();
        pScrollMarker->Set(Tile::kTileValue_User9, MouseWheel / -120);
        pScrollMarker->Set(Tile::kTileValue_User9, 0);
        if (static_cast<double>(iold_y) != pScrollBar->GetFloat(Tile::kTileValue_User7)) {
            DoLeave(a1, a2);
            InterfaceManager* v4 = InterfaceManager::Instance();
            v4->ClearOverTileTarget(0);
        }
    }
}

bool BookMenu::DoKey(int32_t aikey) {
    if (aikey != 32) {
        return (bool)0;
    }
    Close();
    return (bool)1;
}

int32_t BookMenu::GetClass() {
    return 1026;
}

bool BookMenu::IsWellFormedMenu() {
    return (bool)((uint8_t)(pScrollBar && pScrollMarker));
}

void BookMenu::Close() {
    Menu* menu;
    TileMenu* menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(1026));
    if (menuByClass) {
        if (menuByClass->Tile::GetMenu()) {
            menuByClass->Tile::Set(6002, 1);
            menu = menuByClass->Tile::GetMenu();
            menu->StartFadeOut();
            if (pBook->IsScroll()) {
                InterfaceManager::PlayMenuSound(MENU_SOUND_SCROLL_CLOSE);
            }
            else {
                InterfaceManager::PlayMenuSound(MENU_SOUND_BOOK_CLOSE);
            }
        }
    }
}

void BookMenu::SetupTextAndButtons(TESObjectBOOK* a1, Actor* a2) {
    if (a1->IsScroll()) {
        pMenu->Set(Tile::kTileValue_User0, 0);
    }
    else {
        pMenu->Set(Tile::kTileValue_User0, 1);
    }
    const char* Description = a1->GetDescription();
    pMenu->Set(Tile::kTileValue_User2, Description, 1);
    unk30 = a2;
    if (a2 && !a1->GetCantTake()) {
        pMenu->Set(Tile::kTileValue_User3, 1);
    }
    else {
        pMenu->Set(Tile::kTileValue_User3, 0);
    }
    const char* v4 = a1->GetDescription();
    BSStringT<char> apString(v4);
    pMenu->Set(Tile::kTileValue_User6, -0x1869F);
    pMenu->Set(Tile::kTileValue_User6, 0);
}

void BookMenu::UseWorkbook() {
    BSStringT<char> axWorkbookPath("Data\\Menus\\workbook.html");
    BSFile xinput(axWorkbookPath.pString, NiFile::READ_ONLY, 0x4000u, static_cast<bool>(0));
    BSStringT<char> xdata;
    int32_t idatalen = 0;
    xinput.BSFile::Open(0, 0);
    if (xinput.m_bGood) {
        xinput.BSFile::Read(xdata, 0xFFFFFFFFu);
        idatalen = xdata.GetLength();
        pMenu->Set(Tile::kTileValue_User2, xdata.pString, static_cast<bool>(1));
        pMenu->Set(Tile::kTileValue_User6, -0x1869F);
        pMenu->Set(Tile::kTileValue_User6, 0);
    }
    else {
        Interface::PrintLine("ERROR: Opening workbook file: %s \n", axWorkbookPath.String());
    }
}

Tile* BookMenu::Create(TESObjectBOOK* apBook, TESObjectREFR* apRef) {
    Tile* pold_menu_tile = Menu::GetMenuByClass(1026);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    if (!apBook) {
        return 0;
    }
    InterfaceManager* pin = InterfaceManager::Instance();
    float ftop_depth = Tile::GetMaximumDepth();
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\book_menu.xml");
    BookMenu* pme = static_cast<BookMenu*>(ptile->GetMenu());
    pme->pBook = apBook;
    if (pme && pme->GetClass() == 1026) {
        TileMenu* v3 = dynamic_cast<TileMenu*>(ptile);
        pme->SetMenuTile(v3, 0);
        BookMenu* pme_2 = dynamic_cast<BookMenu*>(static_cast<Menu*>(pme));
        if (pme_2->IsWellFormedMenu()) {
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                ptile->Set(4013, ftop_depth, 1);
            }
            pme_2->SetupTextAndButtons(apBook, static_cast<Actor*>(apRef));
            if (pme_2->pBook->IsScroll()) {
                InterfaceManager::PlayMenuSound(MENU_SOUND_SCROLL_OPEN);
            }
            else {
                InterfaceManager::PlayMenuSound(MENU_SOUND_BOOK_OPEN);
            }
            pme->PrepForVisibility(0);
            return ptile;
        }
        else {
            Warning("MENUS: Book Menu Creation Failed... Are your menu and art resources up to date?");
            return 0;
        }
    }
    else {
        if (pme) {
            if (pme->pMenu) {
                delete pme;
            }
        }
        return 0;
    }
}

const char* TESObjectBOOK::GetDescription() {
    return GetDescription(0, DESC_ID);
}
