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
        else {
            return false;
        }
    }
    else {
        if (EditField.EditingText()) {
            EditField.HandleKey(aiKey);
            UpdateText();
            return true;
        }
        else {
            return false;
        }
    }
}

int32_t TextEditMenu::GetClass() {
    return 1051;
}

bool TextEditMenu::IsDone() {
    return GetText() != nullptr;
}

void TextEditMenu::UpdateText() {
    char* v1 = EditField.GetDisplayText();
    pTiles[0]->Set(4036, v1, true);
    if (menuCallback) {
        const char* v2 = EditField.GetEnteredText();
        bool v3 = menuCallback(v2);
        pTiles[1]->Set(4015, static_cast<int32_t>(v3));
    }
}

const char* TextEditMenu::GetText() {
    return sEnteredText.pString;
}

BSStringT<char>* TextEditMenu::ResetText() {
    return &sEnteredText.operator=(nullptr);
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
    int32_t v3;
    Tile* MenuByClass = Menu::GetMenuByClass(1051);
    if (MenuByClass) {
        delete MenuByClass;
    }
    InterfaceManager* v9 = InterfaceManager::Instance();
    float afValue = Tile::GetMaximumDepth();
    Tile* th_s = v9->pMenusRoot->ReadFile("Data\\Menus\\Dialog\\TextEditMenu.xml");
    Menu* Menu_ = th_s->GetMenu();
    if (Menu_ && (v3 = Menu_->GetClass(), v3 == TextEditMenu::GetMenuID())) {
        TileMenu* v4 = dynamic_cast<TileMenu*>(th_s);
        Menu_->SetMenuTile(v4, false);
        TextEditMenu::pInstance = dynamic_cast<TextEditMenu*>(Menu_);
        if (th_s->GetFloat(4007) == 6006.0 || th_s->GetFloat(4007) == 102.0) {
            th_s->Set(4013, afValue, true);
        }
        TextEditMenu::pInstance->pTiles[2]->Set(4036, apTitle, true);
        TextEditMenu::pInstance->pTiles[0]->Set(4036, apDefaultText, true);
        double Float = TextEditMenu::pInstance->pTiles[0]->GetFloat(4026);
        TextEditMenu::pInstance->EditField.SetMaxSize(static_cast<int32_t>(Float));
        const char* String = TextEditMenu::pInstance->pTiles[0]->GetString(4036);
        TextEditMenu::pInstance->EditField.SetText(String);
        TextEditMenu::pInstance->EditField.SetEditing(true);
        TextEditMenu::pInstance->EditField.bClearOnKey = true;
        const char* v7 = sOk.String();
        TextEditMenu::pInstance->pTiles[1]->Set(4036, v7, true);
        TextEditMenu::pInstance->menuCallback = apFunc;
        TextEditMenu::pInstance->UpdateText();
        Menu_->PrepForVisibility(false);
        return 1;
    }
    else {
        if (Menu_) {
            if (Menu_->pMenu) {
                delete Menu_;
            }
        }
        return 0;
    }
}
