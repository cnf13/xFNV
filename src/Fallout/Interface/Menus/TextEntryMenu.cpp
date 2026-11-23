#include "Fallout/Interface/Menus/TextEntryMenu.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout/Interface/EditableTextField.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "SDK/MSVC/rtti.h"

BSStringT<char> TextEditMenu::sEnteredText;

TextEditMenu::TextEditMenu() : Menu(), pTiles(), EditField(), menuCallback(nullptr) {
    sEnteredText.Set(nullptr, 0);
}

TextEditMenu::~TextEditMenu() {}

void TextEditMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 3) {
        pTiles[aiID] = apTile;
    }
}

void TextEditMenu::DoClick(int32_t aiID, Tile* apTile) {
    // Looks the clicked tile up when none is given.
    if (!apTile && aiID >= 0 && aiID < 3) {
        apTile = pTiles[aiID];
    }
    if (aiID == 1) {
        sEnteredText.Set(EditField.GetEnteredText(), 0);
        TextEditMenu::Close();
    }
}

void TextEditMenu::DoIdle() {
    if (EditField.EditingText()) {
        EditField.BlinkCursor();
        UpdateText();
    }
}

bool TextEditMenu::DoKey(int32_t aiKey) {
    if (aiKey == -0x7FFFFFF8) {
        if (pTiles[1]->IsTrue(4015)) {
            DoClick(1, pTiles[1]);
            return true;
        }

        return false;
    }

    if (EditField.EditingText()) {
        EditField.HandleKey(aiKey);
        UpdateText();
        return true;
    }

    return false;
}

int32_t TextEditMenu::GetClass() {
    return 1051;
}

bool TextEditMenu::IsDone() {
    return GetText() != nullptr;
}

void TextEditMenu::UpdateText() {
    char* pDisplayText = EditField.GetDisplayText();
    pTiles[0]->Set(4036, pDisplayText, true);
    if (menuCallback) {
        const char* pEnteredText = EditField.GetEnteredText();
        bool bValid = menuCallback(pEnteredText);
        pTiles[1]->Set(4015, static_cast<int32_t>(bValid));
    }
}

const char* TextEditMenu::GetText() {
    return sEnteredText.pString;
}

BSStringT<char>* TextEditMenu::ResetText() {
    return &(sEnteredText = nullptr);
}

int32_t TextEditMenu::GetMenuID() {
    return 1051;
}

void TextEditMenu::Close() {
    if (TextEditMenu::pInstance) {
        TextEditMenu::pInstance->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        TextEditMenu::pInstance->StartFadeOut();
        TextEditMenu::pInstance = nullptr;
    }
}

char TextEditMenu::Create(const char* apTitle, const char* apDefaultText, bool (*apFunc)(const char*)) {
    Tile* pOldMenu = Menu::GetMenuByClass(1051);
    if (pOldMenu) {
        delete pOldMenu;
    }
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    float fMaxDepth = Tile::GetMaximumDepth();
    Tile* pMenuTile = pInterfaceManager->pMenusRoot->ReadFile("Data\\Menus\\Dialog\\TextEditMenu.xml");
    Menu* pNewMenu = pMenuTile->GetMenu();
    if (pNewMenu) {
        int32_t iClass = pNewMenu->GetClass();
        if (iClass == TextEditMenu::GetMenuID()) {
            TileMenu* pTileMenu = dynamic_cast<TileMenu*>(pMenuTile);
            pNewMenu->SetMenuTile(pTileMenu, false);
            TextEditMenu::pInstance = dynamic_cast<TextEditMenu*>(pNewMenu);
            if (pMenuTile->GetFloat(4007) == 6006.0 || pMenuTile->GetFloat(4007) == 102.0) {
                pMenuTile->Set(4013, fMaxDepth, true);
            }
            TextEditMenu::pInstance->pTiles[2]->Set(4036, apTitle, true);
            TextEditMenu::pInstance->pTiles[0]->Set(4036, apDefaultText, true);
            double dMaxSize = TextEditMenu::pInstance->pTiles[0]->GetFloat(4026);
            TextEditMenu::pInstance->EditField.SetMaxSize(static_cast<int32_t>(dMaxSize));
            const char* pText = TextEditMenu::pInstance->pTiles[0]->GetString(4036);
            TextEditMenu::pInstance->EditField.SetText(pText);
            TextEditMenu::pInstance->EditField.SetEditing(true);
            TextEditMenu::pInstance->EditField.bClearOnKey = true;
            const char* pOkText = sOk.String();
            TextEditMenu::pInstance->pTiles[1]->Set(4036, pOkText, true);
            TextEditMenu::pInstance->menuCallback = apFunc;
            TextEditMenu::pInstance->UpdateText();
            pNewMenu->PrepForVisibility(false);
            return 1;
        }
    }

    if (pNewMenu && pNewMenu->pMenu) {
        delete pNewMenu;
    }

    return 0;
}
