#include "Fallout/Interface/Menus/TutorialMenu.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "SDK/MSVC/rtti.h"

SettingT<INIPrefSettingCollection> TutorialMenu::bHelpEnabled("bHelpEnabled:Interface", true);

void TutorialMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 8) {
        pTiles[aiID] = apTile;
    }
}

void TutorialMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 8) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 1: {
            NextPage();
            break;
        }
        case 2: {
            PrevPage();
            break;
        }
        case 3: {
            TutorialMenu::Close();
            break;
        }
    }
}

int32_t TutorialMenu::GetClass() {
    return 1059;
}

bool TutorialMenu::DoGamepad(int32_t aiButton, float afValue) {
    return false;
}

bool TutorialMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(static_cast<Interface::enumMenus>(GetClass())) || aiButton != 10) {
        return false;
    }
    InterfaceManager::Instance();
    InterfaceManager::ClearMenuButton(Interface::eButtonB);
    TutorialMenu::Close();
    return true;
}

void TutorialMenu::NextPage() {
    SetPage(iCurrentPage + 1);
}

void TutorialMenu::PrevPage() {
    SetPage(iCurrentPage - 1);
}

void TutorialMenu::SetPage(uint32_t auiPage) {
    if (auiPage < iNumPages) {
        BSSimpleList<TESForm*>* pformlist;
        BSStringT<char> strPages;
        iCurrentPage = auiPage;
        strPages.SPrintF("%d/%d", auiPage + 1, iNumPages);
        pTiles[5]->Set(4036, strPages.pString, true);
        for (pformlist = pHelpManual; pformlist; pformlist = pformlist->m_pkNext) {
            if (!auiPage--) {
                break;
            }
        }
        if (pformlist) {
            BGSMessage* pMessage = dynamic_cast<BGSMessage*>(pformlist->m_item);
            SetTitleAndText(pMessage);
            UpdatePrevNext();
        }
        else {
            Warning(WARN_MENUS, "End of Help Manual reached while searching for page %d in help manual.", auiPage);
        }
    }
}

// Shows apMessage's name in pTiles[4] and its description in pTiles[0] (trait 4030 set when the text starts with
// '<'), and scrolls pTiles[6] back to the top.
void TutorialMenu::SetTitleAndText(BGSMessage* apMessage) {
    if (!apMessage) {
        return;
    }
    if (pTiles[4]) {
        pTiles[4]->Set(4036, static_cast<TESFullName*>(apMessage)->GetFullName(), true);
    }
    if (pTiles[0]) {
        const char* pText = static_cast<TESDescription*>(apMessage)->GetDescription(nullptr, DESC_ID);
        if (pText) {
            pTiles[0]->Set(4030, static_cast<int32_t>(*pText == '<'));
            pTiles[0]->Set(4036, pText, true);
        }
    }
    if (pTiles[6]) {
        int32_t iCurrentValue = Tile::AddUserTrait("_current_value", -1);
        pTiles[6]->Set(iCurrentValue, 0.0f, false);
    }
}

void TutorialMenu::UpdatePrevNext() {
    pTiles[2]->Set(4015, static_cast<int32_t>(iCurrentPage != 0));
    pTiles[1]->Set(4015, static_cast<int32_t>(iCurrentPage + 1 < iNumPages));
}

void TutorialMenu::Close() {
    if (pMe) {
        pMe->GetMenuTile()->Set(6002, 1);
        if (Interface::GetCurrentRenderedMenu()) {
            pMe->InstantFadeOut();
        }
        else {
            pMe->StartFadeOut();
        }
        StartMenu::ChooseBack();
    }
}

bool TutorialMenu::Create(BGSMessage* apMessage, bool abHelpManual) {
    TutorialMenu* pTutorialMenu;
    Tile* pold_menu_tile = Menu::GetMenuByClass(TutorialMenu::GetClassStatic());
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    Tile* ptile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\tutorial_menu.xml");
    if (ptile) {
        pTutorialMenu = static_cast<TutorialMenu*>(ptile->GetMenu());
    }
    else {
        pTutorialMenu = nullptr;
    }
    TutorialMenu::pMe = pTutorialMenu;
    if (pTutorialMenu) {
        int32_t iClass = TutorialMenu::pMe->GetClass();
        if (iClass == TutorialMenu::GetClassStatic()) {
            TileMenu* pTileMenu = dynamic_cast<TileMenu*>(ptile);
            TutorialMenu::pMe->SetMenuTile(pTileMenu, false);
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                ptile->Set(4013, Tile::GetMaximumDepth(), true);
            }
            const char* pcManual = sVDSGManual.String();
            TutorialMenu::pMe->pTiles[7]->Set(4036, pcManual, true);
            if (abHelpManual) {
                TESForm* pForm;
                if (InterfaceManager::Has360Controller()) {
                    pForm = TESForm::GetFormByNumericID(357u);
                }
                else {
                    pForm = TESForm::GetFormByNumericID(355u);
                }
                BGSListForm* plistform = dynamic_cast<BGSListForm*>(pForm);
                if (plistform) {
                    TutorialMenu::pMe->pHelpManual = &plistform->ListOfForms;
                    if (!apMessage) {
                        apMessage = dynamic_cast<BGSMessage*>(TutorialMenu::pMe->pHelpManual->m_item);
                    }
                }
                const char* pcNext = sNext.String();
                TutorialMenu::pMe->pTiles[1]->Set(4036, pcNext, true);
                const char* pcPrevious = sPrevious.String();
                TutorialMenu::pMe->pTiles[2]->Set(4036, pcPrevious, true);
            }
            else {
                TutorialMenu::pMe->pTiles[1]->Set(4003, 0);
                TutorialMenu::pMe->pTiles[2]->Set(4003, 0);
            }
            const char* pcClose = sCloseButton.String();
            TutorialMenu::pMe->pTiles[3]->Set(4036, pcClose, true);
            if (!apMessage) {
                Warning(WARN_MENUS, "Warning:  Unable to find valid starting message for Tutorial Menu.");
            }
            TutorialMenu::pMe->iNumPages = 0;
            TutorialMenu::pMe->iCurrentPage = 0;
            bool bfound = false;
            for (BSSimpleList<TESForm*>* pformlist = TutorialMenu::pMe->pHelpManual; pformlist && pformlist->m_item;
                 pformlist = pformlist->m_pkNext) {
                if (!bfound && pformlist->m_item == apMessage) {
                    TutorialMenu::pMe->iCurrentPage = TutorialMenu::pMe->iNumPages;
                    bfound = true;
                }
                ++TutorialMenu::pMe->iNumPages;
            }
            if (bfound) {
                BSStringT<char> strtmp;
                strtmp.SPrintF("%d/%d", TutorialMenu::pMe->iCurrentPage + 1, TutorialMenu::pMe->iNumPages);
                TutorialMenu::pMe->pTiles[5]->Set(4036, strtmp.pString, true);
            }
            else {
                TutorialMenu::pMe->pTiles[5]->Set(4003, 0);
                TutorialMenu::pMe->iNumPages = 0;
            }
            if (apMessage) {
                TutorialMenu::pMe->SetTitleAndText(apMessage);
                TutorialMenu::pMe->UpdatePrevNext();
                InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
                TutorialMenu::pMe->PrepForVisibility(false);
                return true;
            }

            if (TutorialMenu::pMe) {
                delete TutorialMenu::pMe;
            }
            return false;
        }
    }

    if (TutorialMenu::pMe && TutorialMenu::pMe->pMenu && TutorialMenu::pMe) {
        delete TutorialMenu::pMe;
    }

    return false;
}

int32_t TutorialMenu::GetClassStatic() {
    return 1059;
}
