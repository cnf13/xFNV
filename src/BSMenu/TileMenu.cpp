#include "BSMenu/TileMenu.h"

#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileRect.h"
#include "Fallout/Interface/Interface.h"

// Both arguments are unused, as the game does.
TileMenu::TileMenu(Tile* p, char* x) : pMenu(nullptr) {}

TileMenu::~TileMenu() {
    if (pMenu) {
        int32_t v3 = pMenu->GetClass();
        if (v3 >= 1001 && v3 <= 1084) {
            Menu::pMenusVisible[v3 - 1001] = 0;
        }
        Menu::SetMenuByClass(pMenu->GetClass(), 0);
        pMenu->SetMenuTile(0, 0);
        delete pMenu;
    }
    if ((uiFlags & RELEASED) == 0) {
        Release();
    }
}

void TileMenu::Init(Tile* apParent, const char* apName, Tile* apReplacedTile) {
    Tile::Init(apParent, apName, apReplacedTile);
    this->Set(kTileValue_X, 0.0f, true);
    this->Set(kTileValue_Y, 0.0f, true);
    this->Set(kTileValue_Locus, 1.0f, true);
    this->Set(kTileValue_MenuFade, 0.25f, true);
    this->Set(kTileValue_SystemColor, 1.0f, true);
    this->Set(kTileValue_Visible, 0.0f, true);
    this->SetNeedsUpdate(CHANGE_POSITION | CHANGE_CREATE);
    this->pMenu = nullptr;
}

NiNode* TileMenu::MakeNode() {
    return TileRect::MakeNode();
}

int32_t TileMenu::GetType() {
    return kTileValue_Menu;
}

Tile* TileMenu::PostParse(int32_t aiTrait, float afValue, const char* axString) {
    if (aiTrait == kTileValue_Class) {
        if (!this->pMenu) {
            this->pMenu = static_cast<Menu*>(Interface::CreateMenuByClass(static_cast<int32_t>(afValue)));
        }
        if (!this->pMenu) {
            Interface::PrintLine<>("ERROR: Cant find menu class!");
        }
        else {
            this->pMenu->SetMenuTile(this, false);
            this->pMenu->iClass = this->pMenu->GetClass();
            // Stacking types 102 and 103 make the menu modal, drawn above every other menu.
            if (this->GetFloat(kTileValue_StackingType) == 102.0 || this->GetFloat(kTileValue_StackingType) == 103.0) {
                this->pMenu->iIsModal = this->pMenu->GetClass();
                this->Set(kTileValue_Depth, Menu::GetMaxDepth(), true);
            }
            Menu::SetMenuByClass(static_cast<int32_t>(afValue), this);
            return this;
        }
    }
    else if (this->pMenu && aiTrait >= 10000) {
        // User traits go to the menu.
        this->pMenu->OnXMLDataChange(aiTrait);
    }
    if (this->pMenu && aiTrait == kTileValue_Visible) {
        const bool visible = afValue != 0.0;
        const int32_t menuClass = this->pMenu->GetClass();
        if (menuClass >= 1001 && menuClass <= 1084) {
            Menu::pMenusVisible[menuClass - 1001] = visible;
        }
    }
    return nullptr;
}

void TileMenu::ForceTextureRelease() {
    if ((!pMenu) || ((((uint32_t)pMenu->GetClass()) != 1004) && (((uint32_t)pMenu->GetClass()) != 1007))) {
        Tile::ForceTextureRelease();
    }
}

void TileMenu::SetMenu(Menu* m) {
    this->pMenu = m;
}
