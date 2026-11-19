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
    if (((uint32_t)aiID) <= 5) {
        pTiles[aiID] = apTile;
    }
}

void SleepWaitMenu::DoClick(int32_t a1, Tile* a2) {
    if (!a2 && a1 >= 0 && a1 < 6) {
        a2 = pTiles[a1];
    }
    if (a1 == 4) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
        if ((this->bSleepingInBed && gs_bSaveOnRest_GamePlay.Bool()) ||
            (!this->bSleepingInBed && gs_bSaveOnWait_GamePlay.Bool())) {
            pSaveLoadManager->Autosave();
        }
        if (this->bSleepingInBed) {
            float* v3 = &fFadeToBlackFadeSeconds.Float();
            pFaderManager->CreateFader(FADER_BLACK_UNDER_MENUS, *v3, false);
        }
        bRestingActive = true;
        pTiles[4]->Set(4015, 0);
        pTiles[1]->Set(4015, 0);
        bool bSleepingInBed = this->bSleepingInBed;
        int32_t Float = static_cast<int32_t>(pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue));
        pPlayer->SetSleeping(Float + 1, bSleepingInBed);
        if (this->bSleepingInBed) {
            MiscStatManager::Increment(MiscStatManager::MS_TIMES_SLEPT);
        }
        if (!pPlayer->GetInChargen()) {
            ReferenceProcessLists.EndAllNonPlayerConversations();
        }
        ReferenceProcessLists.GetActiveMagicProjectileCount();
    }
    else {
        if (a1 == 5) {
            InterfaceManager::PlayMenuSound(MENU_SOUND_CANCEL);
            pPlayer->SetSleeping(0, false);
            SleepWaitMenu::Close();
        }
    }
}

void SleepWaitMenu::DoIdle() {
    if (fPrevQuantity != pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue)) {
        const char* v2;
        char Src[268];
        if (pTiles[4]->IsTrue(4015)) {
            Interface::PlayMenuSound(4);
        }
        fPrevQuantity = pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue);
        if (!(fPrevQuantity > 0.0)) {
            v2 = sHour.String();
        }
        else {
            v2 = sHours.String();
        }
        float someFloat = fPrevQuantity + 1.0;
        BSsprintf(Src, 260u, "%d %s", FloatToInt(someFloat), v2);
        pTiles[2]->Set(4036, Src, true);
    }
}

int32_t SleepWaitMenu::GetClass() {
    return 1012;
}

bool SleepWaitMenu::DoGamepad(int32_t a2, float a3) {
    int32_t v3;
    float v5;
    float v6;
    int8_t v8 = 0;
    if (pTiles[4]->IsTrue(4015) && ((a2 == 15) || (a2 == 16))) {
        if (a2 == 15) {
            v6 = (-0x1p+1f);
        }
        else {
            v6 = 0x1p+1f;
        }
        v5 = pTiles[1]->GetFloat(SleepWaitMenu::iUT_CurrentValue) + v6;
        pTiles[1]->Set(SleepWaitMenu::iUT_CurrentValue, v5, false);
        v3 = Tile::TextToTrait("_SetInCode");
        pTiles[1]->Poke(v3, 1.0);
        return true;
    }
    return (bool)v8;
}

bool SleepWaitMenu::UpdateSleep() {
    Controls* v2;
    Controls* Controls_ = appMain->GetControls();
    if (Controls_->GetUserAction(Controls::UACTN_RESTMENU, 2)) {
        bButtonReleased = true;
    }
    if (((bButtonReleased && (v2 = appMain->GetControls(), v2->GetUserAction(Controls::UACTN_RESTMENU, 1))) ||
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
        int32_t afValue = pPlayer->GetSleepHours() - 1;
        int32_t v4 = Tile::TextToTrait("_current_value");
        pTiles[1]->Set(v4, afValue);
        int32_t v5 = Tile::TextToTrait("_SetInCode");
        pTiles[1]->Poke(v5, 1.0f);
        SetCurrentTime();
        return true;
    }

    return false;
}

bool SleepWaitMenu::UpdateSleeping() {
    SleepWaitMenu* menu;
    int32_t v0 = SleepWaitMenu::GetClassStatic();
    TileMenu* menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(v0));
    if (menuByClass) {
        menu = static_cast<SleepWaitMenu*>(menuByClass->Tile::GetMenu());
    }
    else {
        menu = nullptr;
    }
    int8_t v3 = 0;
    if (!InterfaceManager::Instance()) {
        return (bool)v3;
    }
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (!singleton->IsDoneFirstInit()) {
        return (bool)v3;
    }
    if (menu && menu->UpdateSleep()) {
        return true;
    }
    return (bool)v3;
}

void SleepWaitMenu::SetCurrentTime() {
    const char* pAMPM;
    char Src[268];
    float fHour = appCalendar.GetHour();
    int32_t v9 = static_cast<int32_t>(fHour);
    int32_t iMinutes = static_cast<int32_t>((fHour - static_cast<double>(v9)) * 60.0);
    if (!(fHour >= 0.0) || !(fHour < 1.0)) {
        if (fHour >= 13.0) {
            v9 -= 12;
        }
    }
    else {
        v9 = 12;
    }
    if (!(fHour < 12.0)) {
        pAMPM = sPMTime.String();
    }
    else {
        pAMPM = sAMTime.String();
    }
    BSsprintf(
        Src, 260, "%s, %s, %d:%02d %s", appCalendar.GetDayOfWeekString(), appCalendar.GetDateString().String(), v9,
        iMinutes, pAMPM
    );
    pTiles[3]->Set(4036, Src, true);
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
    int32_t v2;
    InterfaceManager* pin = InterfaceManager::Instance();
    int32_t ClassStatic = SleepWaitMenu::GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\sleep_wait_menu.xml");
    SleepWaitMenu::pMe = static_cast<SleepWaitMenu*>(ptile->GetMenu());
    if (SleepWaitMenu::pMe && (v2 = SleepWaitMenu::pMe->GetClass(), v2 == SleepWaitMenu::GetClassStatic())) {
        SleepWaitMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        if (SleepWaitMenu::pMe->IsWellFormedMenu()) {
            const char* v5;
            char Dst[264];
            char Src[264];
            BSAudio* v4 = &BSAudio::QInstance();
            v4->GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                float afValue = Tile::GetMaximumDepth();
                ptile->Set(4013, afValue, true);
            }
            SleepWaitMenu::pMe->bSleepingInBed = abSleep;
            if (abSleep) {
                v5 = sSleep.String();
            }
            else {
                v5 = sWait.String();
            }
            BSstrcpy(Dst, 260, v5);
            BSsprintf(Src, 260, "%s %s?", sHowManyWait.String(), Dst);
            SleepWaitMenu::pMe->pTiles[0]->Set(4036, Src, true);
            Dst[0] = std::toupper(Dst[0]);
            SleepWaitMenu::pMe->pTiles[4]->Set(4036, Dst, true);
            BSsprintf(Src, 260, "1 %s", sHour.String());
            SleepWaitMenu::pMe->pTiles[2]->Set(4036, Src, true);
            SleepWaitMenu::pMe->SetCurrentTime();
            const char* v6 = sCancel.String();
            SleepWaitMenu::pMe->pTiles[5]->Set(4036, v6, true);
            float v9 = fMenuBackgroundOpacity.Float() * 255.0;
            Tile* ChildByName = SleepWaitMenu::pMe->pMenu->GetChildByName("SWM_Background");
            ChildByName->Set(4009, v9, true);
            SleepWaitMenu::pMe->PrepForVisibility(false);
            return ptile;
        }

        Warning("MENUS: Sleep Wait Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if ((SleepWaitMenu::pMe) && (SleepWaitMenu::pMe->pMenu) && (SleepWaitMenu::pMe)) {
        delete SleepWaitMenu::pMe;
    }

    return nullptr;
}
