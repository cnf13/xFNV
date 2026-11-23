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
    else if (aiID == 12) {
        pScrollMarker = apTile;
    }
}

void BookMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
    if (aiID == 12) {
        InterfaceManager* pim = InterfaceManager::Instance();
        double dMarkerY = pScrollMarker->Tile::GetRealY();
        fScrollOffset = static_cast<float>(dMarkerY - pim->GetCursorY());
    }
}

void BookMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (aiID == 31) {
        Close();
    }
    else if (aiID == 32) {
        int16_t sCount = pBook->TESObjectREFR::GetExtra()->GetCount();
        pPlayer->PickUpObject(pBook, sCount, false);
        Close();
    }
}

void BookMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    if (aiID == 12) {
        double dMarkerY = static_cast<double>(InterfaceManager::Instance()->GetCursorY()) + fScrollOffset;
        double dRelativeY = dMarkerY - pScrollBar->Tile::GetRealY();
        float fPosition = static_cast<float>(dRelativeY / pScrollMarker->Tile::GetFloat(4108));
        pScrollBar->Tile::Set(4105, -999999999);
        pScrollBar->Tile::Set(4105, static_cast<int32_t>(fPosition));
        pScrollBar->Tile::Set(4105, 0);
    }
}

void BookMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    InterfaceManager* pim = InterfaceManager::Instance();
    if (pMenu->IsTrue(Tile::kTileValue_User0)) {
        int32_t iPage = static_cast<int32_t>(pMenu->GetFloat(Tile::kTileValue_User9));
        if (pim->GetMouseWheel() <= 0) {
            pMenu->Set(Tile::kTileValue_User6, 2);
        }
        else {
            pMenu->Set(Tile::kTileValue_User6, -2);
        }
        pMenu->Set(Tile::kTileValue_User6, 0);
        if (static_cast<double>(iPage) != pMenu->GetFloat(Tile::kTileValue_User9)) {
            int32_t iSoundType = static_cast<int32_t>(pMenu->GetFloat(Tile::kTileValue_User8));
            if (iSoundType) {
                InterfaceManager::Instance();
                InterfaceManager::PlayMenuSound(static_cast<MENU_SOUNDS>(iSoundType));
            }
        }
    }
    else {
        int32_t iOldY = static_cast<int32_t>(pScrollBar->GetFloat(Tile::kTileValue_User7));
        int32_t iMouseWheel = pim->GetMouseWheel();
        pScrollMarker->Set(Tile::kTileValue_User9, iMouseWheel / -120);
        pScrollMarker->Set(Tile::kTileValue_User9, 0);
        if (static_cast<double>(iOldY) != pScrollBar->GetFloat(Tile::kTileValue_User7)) {
            DoLeave(aiID, apTarget);
            InterfaceManager::Instance()->ClearOverTileTarget(false);
        }
    }
}

bool BookMenu::DoKey(int32_t aikey) {
    if (aikey != 32) {
        return false;
    }
    Close();
    return true;
}

int32_t BookMenu::GetClass() {
    return 1026;
}

bool BookMenu::IsWellFormedMenu() {
    return pScrollBar && pScrollMarker;
}

void BookMenu::Close() {
    TileMenu* pTileMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(1026));
    if (pTileMenu && pTileMenu->Tile::GetMenu()) {
        pTileMenu->Tile::Set(6002, 1);
        pTileMenu->Tile::GetMenu()->StartFadeOut();
        if (pScroll->IsScroll()) {
            InterfaceManager::PlayMenuSound(MENU_SOUND_SCROLL_CLOSE);
        }
        else {
            InterfaceManager::PlayMenuSound(MENU_SOUND_BOOK_CLOSE);
        }
    }
}

void BookMenu::SetupTextAndButtons(TESObjectBOOK* apBook, Actor* apActor) {
    if (apBook->IsScroll()) {
        pMenu->Set(Tile::kTileValue_User0, 0);
    }
    else {
        pMenu->Set(Tile::kTileValue_User0, 1);
    }
    const char* pcDescription = apBook->GetDescription();
    pMenu->Set(Tile::kTileValue_User2, pcDescription, true);
    pBook = apActor;
    if (apActor && !apBook->GetCantTake()) {
        pMenu->Set(Tile::kTileValue_User3, 1);
    }
    else {
        pMenu->Set(Tile::kTileValue_User3, 0);
    }
    BSStringT<char> kDescription(apBook->GetDescription());
    pMenu->Set(Tile::kTileValue_User6, -0x1869F);
    pMenu->Set(Tile::kTileValue_User6, 0);
}

void BookMenu::UseWorkbook() {
    BSStringT<char> axWorkbookPath("Data\\Menus\\workbook.html");
    BSFile xinput(axWorkbookPath.pString, NiFile::READ_ONLY, 0x4000u, false);
    BSStringT<char> xdata;
    xinput.BSFile::Open(false, false);
    if (xinput.m_bGood) {
        xinput.BSFile::Read(xdata, 0xFFFFFFFFu);
        pMenu->Set(Tile::kTileValue_User2, xdata.pString, true);
        pMenu->Set(Tile::kTileValue_User6, -0x1869F);
        pMenu->Set(Tile::kTileValue_User6, 0);
    }
    else {
        Interface::PrintLine("ERROR: Opening workbook file: %s \n", axWorkbookPath.String());
    }
}

Tile* BookMenu::Create(TESObjectBOOK* apBook, TESObjectREFR* apRef) {
    Tile* pOldMenuTile = Menu::GetMenuByClass(1026);
    if (pOldMenuTile) {
        delete pOldMenuTile;
    }
    if (!apBook) {
        return nullptr;
    }
    InterfaceManager* pin = InterfaceManager::Instance();
    float fTopDepth = Tile::GetMaximumDepth();
    Tile* pTile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\book_menu.xml");
    BookMenu* pMe = static_cast<BookMenu*>(pTile->GetMenu());
    pMe->pScroll = apBook;
    if (pMe && pMe->GetClass() == 1026) {
        TileMenu* pTileMenu = dynamic_cast<TileMenu*>(pTile);
        pMe->SetMenuTile(pTileMenu, false);
        BookMenu* pBookMenu = dynamic_cast<BookMenu*>(static_cast<Menu*>(pMe));
        if (pBookMenu->IsWellFormedMenu()) {
            if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                pTile->Set(4013, fTopDepth, true);
            }
            pBookMenu->SetupTextAndButtons(apBook, static_cast<Actor*>(apRef));
            if (pBookMenu->pScroll->IsScroll()) {
                InterfaceManager::PlayMenuSound(MENU_SOUND_SCROLL_OPEN);
            }
            else {
                InterfaceManager::PlayMenuSound(MENU_SOUND_BOOK_OPEN);
            }
            pMe->PrepForVisibility(false);
            return pTile;
        }

        Warning("MENUS: Book Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if (pMe && pMe->pMenu) {
        delete pMe;
    }

    return nullptr;
}

const char* TESObjectBOOK::GetDescription() {
    return GetDescription(nullptr, DESC_ID);
}
