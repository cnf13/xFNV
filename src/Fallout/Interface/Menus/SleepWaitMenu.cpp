#include "Fallout/Interface/Menus/SleepWaitMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TES.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"

#include <cctype>

SleepWaitMenu::SleepWaitMenu()
  : Menu(), fElapsedSecs(0.0f), bRestingActive(false), bButtonReleased(false), fPrevQuantity(0.0f) {
    SleepWaitMenu::iUT_CurrentValue = Tile::TextToTrait("_current_value");
}

SleepWaitMenu::~SleepWaitMenu() {
    SleepWaitMenu::pMe = nullptr;
}

void SleepWaitMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 5) {
        pTiles[aiID] = apTile;
    }
}

void SleepWaitMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 6) {
        apTarget = pTiles[aiID];
    }
    if (aiID == 4) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
        if ((this->bSleepingInBed && gs_bSaveOnRest_GamePlay.Bool()) ||
            (!this->bSleepingInBed && gs_bSaveOnWait_GamePlay.Bool())) {
            pSaveLoadManager->Autosave();
        }
        if (this->bSleepingInBed) {
            pFaderManager->CreateFader(FADER_BLACK_UNDER_MENUS, fFadeToBlackFadeSeconds.Float(), false);
        }
        bRestingActive = true;
        pTiles[4]->Set(4015, 0);
        pTiles[1]->Set(4015, 0);
        bool bSleepingInBed = this->bSleepingInBed;
        int32_t iHours = static_cast<int32_t>(pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue));
        pPlayer->SetSleeping(iHours + 1, bSleepingInBed);
        if (this->bSleepingInBed) {
            MiscStatManager::Increment(MiscStatManager::MS_TIMES_SLEPT);
        }
        if (!pPlayer->GetInChargen()) {
            ReferenceProcessLists.EndAllNonPlayerConversations();
        }
        ReferenceProcessLists.GetActiveMagicProjectileCount();
    }
    else if (aiID == 5) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_CANCEL);
        pPlayer->SetSleeping(0, false);
        SleepWaitMenu::Close();
    }
}

void SleepWaitMenu::DoIdle() {
    if (fPrevQuantity != pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue)) {
        const char* pHoursText;
        char acBuffer[268];
        if (pTiles[4]->IsTrue(4015)) {
            Interface::PlayMenuSound(4);
        }
        fPrevQuantity = pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue);
        if (!(fPrevQuantity > 0.0)) {
            pHoursText = sHour.String();
        }
        else {
            pHoursText = sHours.String();
        }
        float fQuantity = fPrevQuantity + 1.0;
        BSsprintf(acBuffer, 260u, "%d %s", FloatToInt(fQuantity), pHoursText);
        pTiles[2]->Set(4036, acBuffer, true);
    }
}

int32_t SleepWaitMenu::GetClass() {
    return 1012;
}

bool SleepWaitMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (pTiles[4]->IsTrue(4015) && (aiButton == 15 || aiButton == 16)) {
        float fStep = aiButton == 15 ? -0x1p+1f : 0x1p+1f;
        float fNewValue = pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue) + fStep;
        pTiles[1]->Set(SleepWaitMenu::iUT_CurrentValue, fNewValue, false);
        int32_t iSetInCodeTrait = Tile::TextToTrait("_SetInCode");
        pTiles[1]->Poke(iSetInCodeTrait, 1.0);
        return true;
    }
    return false;
}

bool SleepWaitMenu::UpdateSleep() {
    if (appMain->GetControls()->GetUserAction(Controls::UACTN_RESTMENU, 2)) {
        bButtonReleased = true;
    }
    if (((bButtonReleased && appMain->GetControls()->GetUserAction(Controls::UACTN_RESTMENU, 1)) ||
         (bRestingActive && !pPlayer->IsSleepingorResting())) &&
        !Interface::IsConsoleVisible()) {
        DoClick(5, nullptr);
        return false;
    }

    if (bRestingActive) {
        fElapsedSecs = appTimer.fDelta + fElapsedSecs;
        if (!(fElapsedSecs >= 1.0)) {
            return false;
        }

        fElapsedSecs = 0.0f;
        TES::pInstance->RunScripts();
        if (pPlayer->IsHardcore()) {
            pPlayer->UpdateHardcoreMode();
        }
        pPlayer->PlayerSleep();
        int32_t iHoursLeft = pPlayer->GetSleepHours() - 1;
        int32_t iTrait = Tile::TextToTrait("_current_value");
        pTiles[1]->Set(iTrait, iHoursLeft);
        iTrait = Tile::TextToTrait("_SetInCode");
        pTiles[1]->Poke(iTrait, 1.0f);
        SetCurrentTime();
        return true;
    }

    return false;
}

bool SleepWaitMenu::UpdateSleeping() {
    TileMenu* pMenuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(SleepWaitMenu::GetClassStatic()));
    SleepWaitMenu* pSleepMenu = nullptr;
    if (pMenuByClass) {
        pSleepMenu = static_cast<SleepWaitMenu*>(pMenuByClass->Tile::GetMenu());
    }
    if (!InterfaceManager::Instance()) {
        return false;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return false;
    }
    if (pSleepMenu && pSleepMenu->UpdateSleep()) {
        return true;
    }
    return false;
}

void SleepWaitMenu::SetCurrentTime() {
    const char* pAMPM;
    char acBuffer[268];
    float fHour = appCalendar.GetHour();
    int32_t iHour = static_cast<int32_t>(fHour);
    int32_t iMinutes = static_cast<int32_t>((fHour - static_cast<double>(iHour)) * 60.0);
    if (!(fHour >= 0.0) || !(fHour < 1.0)) {
        if (fHour >= 13.0) {
            iHour -= 12;
        }
    }
    else {
        iHour = 12;
    }
    if (!(fHour < 12.0)) {
        pAMPM = sPMTime.String();
    }
    else {
        pAMPM = sAMTime.String();
    }
    BSsprintf(
        acBuffer,
        260,
        "%s, %s, %d:%02d %s",
        appCalendar.GetDayOfWeekString(),
        appCalendar.GetDateString().String(),
        iHour,
        iMinutes,
        pAMPM
    );
    pTiles[3]->Set(4036, acBuffer, true);
}

bool SleepWaitMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

void SleepWaitMenu::Close() {
    if (pMe) {
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            ReferenceProcessLists.Update3DAfterResting();
        }
        if (pMe->bSleepingInBed) {
            pFaderManager->RemoveFader(FADER_BLACK_UNDER_MENUS, false);
        }
        pMe->GetMenuTile()->Set(6002, 1);
        pMe->StartFadeOut();
    }
}

int32_t SleepWaitMenu::GetClassStatic() {
    return 1012;
}

Tile* SleepWaitMenu::Create(bool abSleep) {
    InterfaceManager* pin = InterfaceManager::Instance();
    Tile* pOldMenuTile = Menu::GetMenuByClass(SleepWaitMenu::GetClassStatic());
    if (pOldMenuTile) {
        delete pOldMenuTile;
    }
    Tile* pTile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\sleep_wait_menu.xml");
    SleepWaitMenu::pMe = static_cast<SleepWaitMenu*>(pTile->GetMenu());
    if (SleepWaitMenu::pMe && SleepWaitMenu::pMe->GetClass() == SleepWaitMenu::GetClassStatic()) {
        SleepWaitMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
        if (SleepWaitMenu::pMe->IsWellFormedMenu()) {
            const char* pModeText;
            char acModeText[264];
            char acBuffer[264];
            BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
            if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                pTile->Set(4013, Tile::GetMaximumDepth(), true);
            }
            SleepWaitMenu::pMe->bSleepingInBed = abSleep;
            if (abSleep) {
                pModeText = sSleep.String();
            }
            else {
                pModeText = sWait.String();
            }
            BSstrcpy(acModeText, 260, pModeText);
            BSsprintf(acBuffer, 260, "%s %s?", sHowManyWait.String(), acModeText);
            SleepWaitMenu::pMe->pTiles[0]->Set(4036, acBuffer, true);
            acModeText[0] = std::toupper(acModeText[0]);
            SleepWaitMenu::pMe->pTiles[4]->Set(4036, acModeText, true);
            BSsprintf(acBuffer, 260, "1 %s", sHour.String());
            SleepWaitMenu::pMe->pTiles[2]->Set(4036, acBuffer, true);
            SleepWaitMenu::pMe->SetCurrentTime();
            const char* pCancelText = sCancel.String();
            SleepWaitMenu::pMe->pTiles[5]->Set(4036, pCancelText, true);
            float fAlpha = fMenuBackgroundOpacity.Float() * 255.0;
            Tile* pBackground = SleepWaitMenu::pMe->pMenu->GetChildByName("SWM_Background");
            pBackground->Set(4009, fAlpha, true);
            SleepWaitMenu::pMe->PrepForVisibility(false);
            return pTile;
        }

        Warning("MENUS: Sleep Wait Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if (SleepWaitMenu::pMe && SleepWaitMenu::pMe->pMenu && SleepWaitMenu::pMe) {

        delete SleepWaitMenu::pMe;
    }

    return nullptr;
}
