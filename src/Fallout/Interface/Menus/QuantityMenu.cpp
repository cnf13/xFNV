#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

#include "BSAudio/BSAudio.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"

void QuantityMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 6) {
        pTiles[aiID] = apTile;
    }
}

// Handles the menu's buttons: 1 and 2 step the quantity down and up within [0, max]; 4 (accept) and 5 (cancel) report
// the quantity (or 0) to the owner's callback and close the menu. pTiles[aiID] is stored into apTarget when apTarget is
// null and aiID is in [0, 6); nothing reads it afterwards.
void QuantityMenu::DoClick(int32_t aiID, Tile* apTarget) {
    float fQuantity = pTiles[3]->GetFloat(4036);
    if (!apTarget && aiID >= 0 && aiID < 6) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 4:
        case 5:
            if (fpResultCallback) {
                int32_t iQuantity = aiID == 4 ? FloatToInt(fQuantity) : 0;
                fpResultCallback(iQuantity);
            }
            Close();
            break;
        case 1:
        case 2: {
            float fMax = pTiles[0]->GetFloat(4100);
            float fStep = aiID == 2 ? 1.0f : -1.0f;
            // The sum is stored to a float local before BSMAX.
            float fNew = fQuantity + fStep;
            pTiles[0]->Set(4102, BSMIN(BSMAX(fNew, 0.0f), fMax), true);
            pTiles[0]->Set(4080, 0.0f, true);
            break;
        }
        default: break;
    }
}

void QuantityMenu::DoIdle() {
    float fDelta = pTiles[3]->GetFloat(4036) - fPrevQuantity;
    double dAbsDelta = NiAbs(fDelta);
    if (pTiles[0]->GetFloat(4103) <= dAbsDelta) {
        Interface::PlayMenuSound(4);
        fPrevQuantity = pTiles[3]->GetFloat(4036);
    }
}

int32_t QuantityMenu::GetClass() {
    return 1016;
}

bool QuantityMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bhandled = false;
    switch (aiButton) {
        case 13:
        case 14: {
            float fquantity = pTiles[3]->GetFloat(4036);
            float fmaxAmount = pTiles[0]->GetFloat(Tile::kTileValue_User0);
            float fjumpSize = pTiles[0]->GetFloat(Tile::kTileValue_User4);
            float fSignedJump = aiButton == 14 ? fjumpSize : -fjumpSize;
            float fNewQuantity = fquantity + fSignedJump;
            fNewQuantity = BSMAX(fNewQuantity, 0.0f);
            fNewQuantity = BSMIN(fNewQuantity, fmaxAmount);
            pTiles[0]->Set(Tile::kTileValue_User2, fNewQuantity, true);
            pTiles[0]->Set(4080, 0.0f, true);
            bhandled = true;
            break;
        }
        case 15:
        case 16: {
            float fquantity = pTiles[3]->GetFloat(4036);
            float fmaxAmount = pTiles[0]->GetFloat(Tile::kTileValue_User0);
            float fstepSize = pTiles[0]->GetFloat(Tile::kTileValue_User3);
            float fSignedStep = aiButton == 16 ? fstepSize : -fstepSize;
            float fNewQuantity = fquantity + fSignedStep;
            fNewQuantity = BSMAX(fNewQuantity, 0.0f);
            fNewQuantity = BSMIN(fNewQuantity, fmaxAmount);
            pTiles[0]->Set(Tile::kTileValue_User2, fNewQuantity, true);
            pTiles[0]->Set(4080, 0.0f, true);
            bhandled = true;
            break;
        }
        default: {
            break;
        }
    }
    return bhandled;
}

bool QuantityMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton == 10) {
        DoClick(5, nullptr);
        return true;
    }
    return false;
}

bool QuantityMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

void QuantityMenu::Close() {
    if (pMe) {
        pMe->GetMenuTile()->Set(6002, 1);
        if (Interface::GetCurrentRenderedMenu()) {
            pMe->InstantFadeOut();
        }
        else {
            pMe->StartFadeOut();
        }
    }
}

TileMenu* QuantityMenu::Create(int32_t aiMaxAmount, QuantityMenu::ResultCallback afpCallback, int32_t aiStartAmount) {
    InterfaceManager* pin = InterfaceManager::Instance();
    bool boverMenu = false;
    int32_t iClass = GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(iClass);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    if (Interface::GetTopMenuID()) {
        boverMenu = true;
    }
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\quantity_menu.xml");
    QuantityMenu::pMe = static_cast<QuantityMenu*>(ptile->GetMenu());
    if (QuantityMenu::pMe && QuantityMenu::pMe->GetClass() == GetClassStatic()) {
        QuantityMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        if (QuantityMenu::pMe->IsWellFormedMenu()) {
            BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageWindow", 289u).Play(false);
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                float fMaxDepth = Tile::GetMaximumDepth();
                ptile->Set(4013, fMaxDepth, true);
            }
            int32_t istartQuantity = BSMIN(aiStartAmount, aiMaxAmount);
            int32_t istepSize = BSMAX(aiMaxAmount / 20, 1);
            int32_t ijumpSize = BSMAX(aiMaxAmount / 4, 1);
            if (istepSize > 10) {
                istepSize -= istepSize % 10;
            }
            QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User0, aiMaxAmount);
            QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User2, istartQuantity);
            QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User3, istepSize);
            QuantityMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User4, ijumpSize);
            QuantityMenu::pMe->pTiles[0]->Set(4080, 0.0f, true);
            QuantityMenu::fpResultCallback = afpCallback;
            const char* pcHowMany = sHowMany.String();
            Tile* pHowManyText = ptile->GetChildByName("QM_HowManyText");
            pHowManyText->Set(4036, pcHowMany, true);
            const char* pcOk = sOk.String();
            QuantityMenu::pMe->pTiles[4]->Set(4036, pcOk, true);
            const char* pcCancel = sCancel.String();
            QuantityMenu::pMe->pTiles[5]->Set(4036, pcCancel, true);
            if (boverMenu) {
                float fOpacity = fPopUpBackgroundOpacity.Float() * 255.0;
                Tile* pBackground = QuantityMenu::pMe->pMenu->GetChildByName("QM_Background");
                pBackground->Set(4009, fOpacity, true);
            }
            QuantityMenu::pMe->PrepForVisibility(false);
            return static_cast<TileMenu*>(ptile);
        }

        Warning("MENUS: Quantity Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if (QuantityMenu::pMe && QuantityMenu::pMe->pMenu && QuantityMenu::pMe) {
        delete QuantityMenu::pMe;
    }

    return nullptr;
}
