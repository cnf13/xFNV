#include "Fallout/Interface/Menus/TraitSelectMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

TraitSelectMenu::TraitSelectMenu() : Menu(), m_CurrentlySelectedTile(TI_NO_TILE), m_CurrentlyCheckedTile(TI_NO_TILE) {}

// Clears pMe before the Menu base's destructor runs.
TraitSelectMenu::~TraitSelectMenu() {
    TraitSelectMenu::pMe = nullptr;
}

void TraitSelectMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 17) {
        pTiles[aiID] = apTile;
    }
}

// A click without a target tile on one of the 18 buttons takes the button's tile (stored back into apTarget, which
// nothing reads afterwards). Buttons 0-12 toggle their box; button 14 closes the menu.
void TraitSelectMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 18) {
        apTarget = pTiles[aiID];
    }
    if (aiID >= 0) {
        if (aiID <= 12) {
            ToggleCheckBox(aiID);
        }
        else if (aiID == 14) {
            InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
            Close();
        }
    }
}

// A press without a target tile on one of the 18 buttons takes the button's tile (stored back into apTarget, which
// nothing reads afterwards); nothing else happens.
void TraitSelectMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 18) {
        apTarget = pTiles[aiID];
    }
}

// The wheel scrolls the list by 0.05 per notch direction.
void TraitSelectMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    if (InterfaceManager::Instance()->GetMouseWheel() < 0) {
        ScrollForm(0.05f);
    }
    if (InterfaceManager::Instance()->GetMouseWheel() > 0) {
        ScrollForm(-0.05f);
    }
}

void TraitSelectMenu::DoIdle() {
    if (pTiles[13]) {
        int32_t iUT_CurrentValue = Tile::TextToTrait("_current_value");
        if (mfCurrentYScroll * 10.0 != pTiles[13]->GetFloat(iUT_CurrentValue)) {
            float fScrollDelta = pTiles[13]->GetFloat(iUT_CurrentValue) * 0.10000000149011612 - mfCurrentYScroll;
            ScrollForm(fScrollDelta);
        }
    }
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        if (!InterfaceManager::Instance()->GetMouseVisible()) {
            InterfaceManager::Instance();
            if (InterfaceManager::Has360Controller()) {
                if (!InterfaceManager::Instance()->GetMouseVisible()) {
                    std::abs(lastControllerState.Gamepad.sThumbLY);
                    std::abs(lastControllerState.Gamepad.sThumbRY);
                }
            }
        }
    }
    if (m_CurrentlyCheckedTile != TraitSelectMenu::TI_NO_TILE && pTiles[17]) {
        uint32_t nCheckMarkTimer = BSGetTickCount() - m_nCheckmarkAnimStartTime;
        if (nCheckMarkTimer >= 50) {
            if (nCheckMarkTimer >= 100) {
                pTiles[17]->Set(4032, 64.0f, true);
                if (nCheckMarkTimer >= 150) {
                    pTiles[17]->Set(4031, 32.0f, true);
                }
                else {
                    pTiles[17]->Set(4031, 0.0f, true);
                }
            }
            else {
                pTiles[17]->Set(4032, 0.0f, true);
                pTiles[17]->Set(4031, 32.0f, true);
            }
        }
        else {
            pTiles[17]->Set(4032, 0.0f, true);
            pTiles[17]->Set(4031, 0.0f, true);
        }
    }

    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        if (!InterfaceManager::Instance()->GetMouseVisible()) {
            uint32_t nDeltaTime = BSGetTickCount() - m_nLastMenuUpdateTime;
            m_nLastMenuUpdateTime = BSGetTickCount();
            if (mfCurrentYScroll != m_fDesiredYScroll) {
                float fMaxScrollChange = static_cast<double>(nDeltaTime) * 0.0010000000474974513;
                float fScrollDelta = m_fDesiredYScroll - mfCurrentYScroll;
                if (fMaxScrollChange < static_cast<double>(fScrollDelta)) {
                    fScrollDelta = static_cast<double>(nDeltaTime) * 0.0010000000474974513;
                }
                if (fScrollDelta < -fMaxScrollChange) {
                    fScrollDelta = -fMaxScrollChange;
                }
                ScrollForm(fScrollDelta);
            }
        }
    }
}

int32_t TraitSelectMenu::GetClass() {
    return 1076;
}

// Up and down move the selection; A toggles a trait's box or presses Done; B is taken and ignored.
bool TraitSelectMenu::DoGamepad(int32_t aiID, float afValue) {
    bool bHandled = false;
    switch (aiID) {
        case 1:
            Navigate(1);
            bHandled = true;
            break;
        case 2:
            Navigate(0);
            bHandled = true;
            break;
        case 9:
            if (m_CurrentlySelectedTile >= TI_TRAIT_HEAD_01 && m_CurrentlySelectedTile <= TI_TRAIT_GROSSMOTOR_04) {
                ToggleCheckBox(m_CurrentlySelectedTile);
            }
            else if (m_CurrentlySelectedTile == TI_BUTTON_DONE) {
                DoClick(TI_BUTTON_DONE, pTiles[TI_BUTTON_DONE]);
            }
            bHandled = true;
            break;
        case 10: bHandled = true; break;
    }
    return bHandled;
}

void TraitSelectMenu::SetBoxAsChecked(int32_t aiID, bool bForceRefresh) {
    if (bForceRefresh || m_CurrentlyCheckedTile != aiID) {
        m_nCheckmarkAnimStartTime = BSGetTickCount();
        if (pTiles[17]) {
            if (aiID == -1) {
                pTiles[17]->Set(4003, 0);
                BSAudio::QInstance().GetSoundHandleByName("UITraitSelectErase", 289u).Play(false);
            }
            else if (pTiles[aiID]) {
                pTiles[17]->Set(4003, 1);
                float fY = pTiles[aiID]->GetFloat(4002) - 8.0;
                pTiles[17]->Set(4002, fY, true);
                BSAudio::QInstance().GetSoundHandleByName("UITraitSelectCheckmark", 289u).Play(false);
            }
        }
        m_CurrentlyCheckedTile = static_cast<TraitSelectMenu::TILE_INDEX>(aiID);
    }
}

void TraitSelectMenu::ToggleCheckBox(int32_t aiID) {
    if (m_CurrentlyCheckedTile == aiID) {
        ClearCheckBox();
    }
    else {
        SetBoxAsChecked(aiID, false);
    }
}

void TraitSelectMenu::ScrollForm(float afDeltaY) {
    mfCurrentYScroll = mfCurrentYScroll + afDeltaY;
    if (mfCurrentYScroll > 1.0) {
        mfCurrentYScroll = 1.0f;
    }
    if (mfCurrentYScroll < 0.0) {
        mfCurrentYScroll = 0.0f;
    }
    Tile* ptile = Menu::GetMenuByClass(1076);
    if (ptile) {
        float fMenuTop = ptile->GetFloat(Tile::kTileValue_User0);
        ptile->GetFloat(Tile::kTileValue_User1);
        ptile->GetFloat(Tile::kTileValue_User2);
        ptile->GetFloat(Tile::kTileValue_User3);
        float fScrollPaperDist = ptile->GetFloat(Tile::kTileValue_User4);
        if (pTiles[15]) {
            float fMainRectYAdjust = fMenuTop - mfCurrentYScroll * fScrollPaperDist;
            pTiles[15]->Set(4002, fMainRectYAdjust, true);
        }
        Interface::PlayMenuSound(4);
        if (pTiles[13]) {
            int32_t iUT_CurrentValue = Tile::TextToTrait("_current_value");
            float fYInScrollBarSpace = mfCurrentYScroll * 10.0;
            pTiles[13]->Set(iUT_CurrentValue, fYInScrollBarSpace, false);
            int32_t iUT_SetInCode = Tile::TextToTrait("_SetInCode");
            pTiles[13]->Poke(iUT_SetInCode, 1.0f);
        }
    }
}

void TraitSelectMenu::ClearCheckBox() {
    SetBoxAsChecked(-1, false);
}

bool TraitSelectMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

void TraitSelectMenu::SetButtonAsHighlighted(int32_t aiID, char abForceRefresh) {
    if (abForceRefresh || m_CurrentlySelectedTile != aiID) {
        if (m_CurrentlySelectedTile != TraitSelectMenu::TI_NO_TILE && pTiles[m_CurrentlySelectedTile]) {
            pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User1, 0);
        }

        m_CurrentlySelectedTile = static_cast<TraitSelectMenu::TILE_INDEX>(aiID);
        if (m_CurrentlySelectedTile != TraitSelectMenu::TI_NO_TILE) {
            if (pTiles[m_CurrentlySelectedTile]) {
                pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User1, 1);
            }
            InterfaceManager::Instance();
            if (InterfaceManager::Has360Controller()) {
                if (!InterfaceManager::Instance()->GetMouseVisible()) {
                    Tile* pMenuTile = Menu::GetMenuByClass(1076);
                    if (pMenuTile && pTiles[15]) {
                        pMenuTile->GetFloat(Tile::kTileValue_User4);
                        float fHeight = pTiles[15]->GetFloat(4016);
                        float fTileY = pTiles[m_CurrentlySelectedTile]->GetFloat(4002);
                        m_fDesiredYScroll = fHeight == 0.0 ? 0.0f : fTileY / fHeight;
                    }
                }
            }
        }
    }
}

void TraitSelectMenu::Navigate(char abUp) {
    if (abUp) {
        switch (m_CurrentlySelectedTile) {
            case TraitSelectMenu::TI_TRAIT_HEAD_01: {
                return;
            }
            case TraitSelectMenu::TI_TRAIT_HEAD_02:
            case TraitSelectMenu::TI_TRAIT_HEAD_03:
            case TraitSelectMenu::TI_TRAIT_HEAD_04:
            case TraitSelectMenu::TI_TRAIT_BODY_01:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_01:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_02:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_03:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_04:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_01:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_02:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_03:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_04: {
                SetButtonAsHighlighted(m_CurrentlySelectedTile - 1, 0);
                break;
            }
            case TraitSelectMenu::TI_BUTTON_DONE: {
                SetButtonAsHighlighted(12, 0);
                break;
            }
            default: {
                SetButtonAsHighlighted(0, 0);
                break;
            }
        }
    }
    else {
        switch (m_CurrentlySelectedTile) {
            case TraitSelectMenu::TI_TRAIT_HEAD_01:
            case TraitSelectMenu::TI_TRAIT_HEAD_02:
            case TraitSelectMenu::TI_TRAIT_HEAD_03:
            case TraitSelectMenu::TI_TRAIT_HEAD_04:
            case TraitSelectMenu::TI_TRAIT_BODY_01:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_01:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_02:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_03:
            case TraitSelectMenu::TI_TRAIT_FINEMOTOR_04:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_01:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_02:
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_03: {
                SetButtonAsHighlighted(m_CurrentlySelectedTile + 1, 0);
                break;
            }
            case TraitSelectMenu::TI_TRAIT_GROSSMOTOR_04: {
                SetButtonAsHighlighted(14, 0);
                break;
            }
            case TraitSelectMenu::TI_BUTTON_DONE: {
                return;
            }
            default: {
                SetButtonAsHighlighted(0, 0);
                break;
            }
        }
    }
}

Tile* TraitSelectMenu::Create() {
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    int32_t iClassStatic = TraitSelectMenu::GetClassStatic();
    Tile* pOldMenuTile = Menu::GetMenuByClass(iClassStatic);
    if (pOldMenuTile) {
        delete pOldMenuTile;
    }
    Tile* pTile = pInterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\trait_select_menu.xml");
    pMe = static_cast<TraitSelectMenu*>(pTile->GetMenu());
    if (pMe) {
        int32_t iClass = pMe->GetClass();
        if (iClass == TraitSelectMenu::GetClassStatic()) {
            pMe->Menu::SetMenuTile(static_cast<TileMenu*>(pTile), false);
            if (!pMe->IsWellFormedMenu()) {
                Warning("MENUS: Companion Wheel Menu Creation Failed... Are your menu and art resources up to date?");
                return nullptr;
            }
            pMe->mfCurrentYScroll = 0.0f;
            pMe->m_fDesiredYScroll = 0.0f;
            pMe->SetButtonAsHighlighted(0, 0);
            pMe->m_nLastMenuUpdateTime = BSGetTickCount();
            BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
            if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                float afValue = Tile::GetMaximumDepth();
                pTile->Set(4013, afValue, true);
            }
            pMe->Menu::PrepForVisibility(false);
            return pTile;
        }
    }
    if (pMe && pMe->GetMenuTile()) {
        delete pMe;
    }
    return nullptr;
}

void TraitSelectMenu::Close() {
    if (pMe) {
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            ReferenceProcessLists.Update3DAfterResting();
        }
        pMe->GetMenuTile()->Set(6002, 1);
        pMe->StartFadeOut();
    }
}

int32_t TraitSelectMenu::GetClassStatic() {
    return 1076;
}
