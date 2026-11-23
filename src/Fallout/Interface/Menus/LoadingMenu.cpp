#include "Fallout/Interface/Menus/LoadingMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSThread.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/Tile3D.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSMovie/MoviePlayer.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESLoadScreen.h"
#include "Fallout Shared/TESLoadScreenType.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Misc/BGSMoviePlayer.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cstdint>

LoadingMenuDisplayData LoadingMenu::DisplayData;
LoadingMenu::WelcomeScreenSettings LoadingMenu::pWelcomeScreensA = {
    SettingT<INISettingCollection>("sWelcomeScreen1:Loading", "loading_screen01"),
    SettingT<INISettingCollection>("sWelcomeScreen2:Loading", "loading_screen_bethsoft"),
    SettingT<INISettingCollection>("sWelcomeScreen3:Loading", "loading_screen_BGS"),
    SettingT<INISettingCollection>("sWelcomeScreen4:Loading", "loading_screen_legal"),
    SettingT<INISettingCollection>("sMainMenuBackground:Loading", "main_background"),
};
SettingT<INISettingCollection> gs_sTitleMusic_Loading("sTitleMusic:Loading", "MainTitle");
SettingT<INISettingCollection>
    LoadingMenu::sInitialSound("sInitialSound:Loading", "fx\\ui\\loadscreen\\initial\\ui_loadscreen_initial.wav");
SettingT<INISettingCollection> LoadingMenu::iMaxScreens("iMaxScreens:Loading", 4);
SettingT<INISettingCollection> LoadingMenu::iMaxScreens_MainMenu("iMaxScreens_MainMenu:Loading", 20);
SettingT<INISettingCollection> LoadingMenu::fLoadingTextUpdateInterval("fLoadingTextUpdateInterval:Loading", 10.0f);
SettingT<INISettingCollection> gs_fLoadingBkgdUpdateInterval_Loading("fLoadingBkgdUpdateInterval:Loading", 10.0f);
SettingT<INISettingCollection> gs_fMainMenuBkgdUpdateInterval_Loading("fMainMenuBkgdUpdateInterval:Loading", 10.0f);
SettingT<INISettingCollection> gs_fLoadingInitUpdateInterval_Loading("fLoadingInitUpdateInterval:Loading", 4.0f);
SettingT<INISettingCollection> LoadingMenu::iNumLocationSpecificScreens("iNumLocationSpecificScreens:Loading", 1);

LoadingMenuThread::LoadingMenuThread() : bFlagSuspend(false), bFlagSuspended(false), bExit(false) {
    SetName("LoadingMenu");
    Initialize(BSThread::SMALL_STACK_SIZE, "LoadingMenu", false);
    SetThreadProcessor(5u);
    SetThreadPriority(-1);
}

LoadingMenuThread::~LoadingMenuThread() {}

uint32_t LoadingMenuThread::ThreadProc() {
    uint32_t uiNext = BSGetTickCount();
    while (!bExit) {
        if (BSGetTickCount() < uiNext) {
            uint32_t uiNow = BSGetTickCount();
            BSSleep(uiNext - uiNow);
        }
        if (!bFlagSuspend ||
            (LoadingMenu::bInitialLoad &&
             !*sStartingCell.String() &&
             static_cast<Tile3D*>(LoadingMenu::pMe->pTiles[0])->Tile3D::IsPlaying())) {
            uiNext = BSGetTickCount() + 50;
        }
        else {
            MainThreadSema.Signal();
            bFlagSuspended = true;
            BackgroundThreadSema.Wait();
            SetThreadPriority(-1);
            bFlagSuspended = false;
        }
        LoadingMenu::pMe->Update();
        LoadingMenu::ShowChanges();
    }
    return 0;
}

bool LoadingMenuThread::QIsSuspended() {
    return bFlagSuspended;
}

void LoadingMenuThread::Unsuspend() {
    if (bFlagSuspended) {
        bFlagSuspend = false;
        BackgroundThreadSema.Signal();
    }
}

bool LoadingMenuThread::GetByte49() {
    return LoadingMenu::pLoadingThread && LoadingMenu::pLoadingThread->QIsSuspended();
}

LoadingMenu::LoadingMenu()
  : pTiles{}, SaveBackgroundColor(0.0f, 0.0f, 0.0f, 0.0f), pNextLoadScreen(&LoadScreens), uiNextBackgroundSwap(0),
    uiFadeInComplete(0), pLocation(nullptr), pSlide1TSP(nullptr), pSlide2TSP(nullptr), uiCurrTextureIndex(0),
    usCurIndex(0), usFlags(0), m_TileCrossFadeStartTime(0), m_SlideCount(0), m_DelayedTipMatteUpdate(0) {
    for (MiscStatManager::MiscStatID& eID : eIDOrder) {
        eID = MiscStatManager::MS_EQUIPMENT_DAMAGE_REPAIRED;
    }
    NiRenderer::GetRenderer()->GetBackgroundColor(SaveBackgroundColor);
    SetFlag(LoadingMenu::LM_INITLOAD_COMPLETE, !LoadingMenu::bInitialLoad);
    SetFlag(LoadingMenu::LM_FADE_IN, 1);
    pCurrentLoadScreen = pNextLoadScreen->m_item;
    if (pCurrentLoadScreen) {
        TESLoadScreenType* pLoadScreenType = pCurrentLoadScreen->pLoadScreenType;
        if (pLoadScreenType) {
            CalcLoadScreenTypeTextWindowFaces(&pLoadScreenType->data);
        }
    }
}

LoadingMenu::~LoadingMenu() {
    LoadingMenu::SuspendBackgroundThread();
    for (const NiPointer<NiTexture>& spTexture : spHoldTexture) {
        if (spTexture.m_pObject) {
            BSTexturePalette::RemoveTexture(spTexture.m_pObject);
        }
    }
    Interface::bIsLoadingNewGame = false;
    NiRenderer::GetRenderer()->SetBackgroundColor(SaveBackgroundColor);
    if ((StartMenu::MainMenuIsActive() || (LoadingMenu::bInitialLoad && *sStartingCell.String())) &&
        BSAudio::QInstance().bAudioEnabled) {
        FalloutAudioMedia::MediaFadeOut(4000, MEDIA_STANDBY);
    }

    if (FalloutAudio::GetNewGameMusic()) {
        FalloutAudio::SetNewGameMusic(false);
        FalloutAudio::SetMainMenuMusic(false);
        FalloutAudioMedia::MediaFadeOut(4000, MEDIA_STANDBY);
    }
    if (GetFlag(LoadingMenu::LM_AUDIO_THREADING) && BSAudio::QInstance().bMultiThread) {
        BSAudio::QInstance().SetMultiThreaded(false);
    }

    BSAudio::QInstance().UnMute(1000u);
    LoadingMenu::bInitialLoad = false;
    FlushLoadScreenList();
    SlideSound.Stop();
    SlideSound.Release();
    InitialSlideSound.Release();
    pLocation = nullptr;
    LoadingMenu::pMe = nullptr;
    if (pPlayer && pPlayer->patch04DebugQuest) {
        pPlayer->patch04DebugQuest->SetEnabled(1);
        pPlayer->patch04DebugQuest = nullptr;
    }
}

void LoadingMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (apTile && aiID < 38 && aiID >= 0) {
        this->pTiles[aiID] = apTile;
    }
}

int32_t LoadingMenu::GetClass() {
    return 1007;
}

bool LoadingMenu::GetFlag(Flag aeFlag) {
    return (this->usFlags & (1 << aeFlag)) != 0;
}

void LoadingMenu::SetFlag(Flag aeFlag, uint8_t abOn) {
    if (abOn) {
        this->usFlags |= 1 << aeFlag;
    }
    else {
        this->usFlags &= ~(1 << aeFlag);
    }
}

void LoadingMenu::ToggleFlag(Flag aeFlag) {
    this->usFlags ^= 1 << aeFlag;
}

void LoadingMenu::InitStatsTilesList() {
    this->StatsTilesList.RemoveAll();
    this->StatsTilesList.AddTail(this->pTiles[17]);
    this->StatsTilesList.AddTail(this->pTiles[18]);
}

void LoadingMenu::InitObjectiveTilesList() {
    this->ObjectiveTilesList.RemoveAll();
    this->ObjectiveTilesList.AddTail(this->pTiles[27]);
    this->ObjectiveTilesList.AddTail(this->pTiles[3]);
    this->ObjectiveTilesList.AddTail(this->pTiles[14]);
}

void LoadingMenu::InitTipTilesList() {
    this->TipTilesList.RemoveAll();
    this->TipTilesList.AddTail(this->pTiles[34]);
    this->TipTilesList02.RemoveAll();
    this->TipTilesList02.AddTail(this->pTiles[35]);
}

void LoadingMenu::UpdateStatsTilesVisibilities(bool bVisible) {
    NiTListItem<Tile*>* position = this->StatsTilesList.m_pkHead;
    while (position) {
        this->StatsTilesList.GetNext(position)->Set(0xFA3 /* kTileValue_Visible */, bVisible);
    }
}

void LoadingMenu::UpdateObjectiveTilesVisibilities(bool bVisible) {
    NiTListItem<Tile*>* position = this->ObjectiveTilesList.m_pkHead;
    while (position) {
        this->ObjectiveTilesList.GetNext(position)->Set(0xFA3 /* kTileValue_Visible */, bVisible);
    }
}

void LoadingMenu::UpdateXPProgressTilesVisibilities(bool bVisible) {
    NiTListItem<Tile*>* position = this->XPProgressTilesList.m_pkHead;
    while (position) {
        this->XPProgressTilesList.GetNext(position)->Set(0xFA3 /* kTileValue_Visible */, bVisible);
    }
}

void LoadingMenu::UpdateTipTilesVisibilities(bool bVisible) {
    NiTListItem<Tile*>* piter = TipTilesList.m_pkHead;
    while (piter) {
        Tile* pTile = TipTilesList.GetNext(piter);
        if (pTile) {
            pTile->Set(4003, static_cast<int32_t>(bVisible));
        }
    }
    piter = TipTilesList02.m_pkHead;
    while (piter) {
        Tile* pTile = TipTilesList02.GetNext(piter);
        if (pTile) {
            pTile->Set(4003, static_cast<int32_t>(bVisible));
        }
    }
    pTiles[36]->Set(4003, static_cast<int32_t>(bVisible));
    pTiles[37]->Set(4003, static_cast<int32_t>(bVisible));
}

void LoadingMenu::ResetLoadingPinwheelAnimation() {
    static_cast<Tile3D*>(this->pTiles[ePinwheel])->UpdateSequence(-1.0f, false);
}

void LoadingMenu::Update() {
    float fTileAlpha;
    float fSlide02Alpha;
    float fSlide01Alpha;
    LoadingMenu::bDoTileUpdate = true;
    if ((m_DelayedTipMatteUpdate & 1) != 0) {
        m_DelayedTipMatteUpdate &= ~1u;
        float fMatteY = pTiles[34]->GetFloat(4002) - 15.0;
        pTiles[36]->Set(4002, fMatteY, true);
        float fMatteHeight = pTiles[34]->GetFloat(4016) + 30.0;
        pTiles[36]->Set(4016, fMatteHeight, true);
    }
    if ((m_DelayedTipMatteUpdate & 2) != 0) {
        m_DelayedTipMatteUpdate &= ~2u;
        float fMatteY = pTiles[35]->GetFloat(4002) - 15.0;
        pTiles[37]->Set(4002, fMatteY, true);
        float fMatteHeight = pTiles[35]->GetFloat(4016) + 30.0;
        pTiles[37]->Set(4016, fMatteHeight, true);
    }
    uint32_t uiCurrTime = BSGetTickCount();
    if (!GetFlag(LoadingMenu::LM_FADE_IN)) {
        uint32_t uiCrossTime = uiCurrTime - m_TileCrossFadeStartTime;
        fTileAlpha = static_cast<double>(uiCrossTime) * 0.00044999999227002263;
        if (!m_TileCrossFadeStartTime) {
            fTileAlpha = 1.0f;
        }
        BSClamp(fTileAlpha, 0.0f, 1.0f);
        if (!(m_SlideCount % 2)) {
            fTileAlpha = 1.0 - fTileAlpha;
        }
        fSlide01Alpha = fTileAlpha;
        BSClamp(fSlide01Alpha, 0.0f, 1.0f);
        if (pTiles[32]) {
            pTiles[32]->Set(4009, static_cast<int32_t>(fSlide01Alpha * 255.0));
        }
        fSlide02Alpha = 1.0 - fTileAlpha;
        BSClamp(fSlide02Alpha, 0.0f, 1.0f);
        if (pTiles[33]) {
            pTiles[33]->Set(4009, static_cast<int32_t>(fSlide02Alpha * 255.0));
        }
        UpdatePlayerTileAlphas(pCurrentLoadScreen, fSlide01Alpha, 1);
        UpdatePlayerTileAlphas(pNextLoadScreen->m_item, fSlide02Alpha, 2);
        LoadingMenu::bDoTileUpdate = true;
    }
    if (uiCurrTime - m_TileCrossFadeStartTime >= 3000) {
        if (uiNextBackgroundSwap) {
            if (GetFlag(LoadingMenu::LM_FADE_IN)) {
                FadeIn();
            }
            else if (!GetFlag(LoadingMenu::LM_WHEEL_ONLY_MODE) && BSGetTickCount() >= uiNextBackgroundSwap) {
                Tile3D* p3DTile = static_cast<Tile3D*>(pTiles[0]);
                if (p3DTile->IsPlaying()) {
                    if (!LoadingMenu::bInitialLoad &&
                        pPlayCredits->fValue == 0.0 &&
                        !StartMenu::MainMenuIsActive() &&
                        (!GetFlag(LoadingMenu::LM_RESET_MENU) || pTiles[34]->IsTrue(4003))) {
                        SetScreenInfo(false);
                    }
                    AdvanceScreen();
                    SetNextSlide(nullptr);
                    p3DTile->pAniManager->DeactivateSequence(p3DTile->pCurrentSequence, 0.0f);
                    NextSwap();
                    if (LoadingMenu::bInitialLoad && pNextLoadScreen == &LoadScreens) {
                        SetFlag(LoadingMenu::LM_INITLOAD_COMPLETE, 1);
                    }
                }
                else if (
                    (!LoadingMenu::bInitialLoad || !GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE)) &&
                    !GetFlag(LoadingMenu::LM_RESET_MENU) &&
                    !GetFlag(LoadingMenu::LM_STATIC_MENU)
                ) {
                    bool bPlayed;
                    uiNextBackgroundSwap = BSGetTickCount();
                    ToggleFlag(LoadingMenu::LM_FORWARD_ANIM);
                    if (GetFlag(LoadingMenu::LM_FORWARD_ANIM)) {
                        bPlayed = p3DTile->PlaySequence("Forward");
                    }
                    else {
                        bPlayed = p3DTile->PlaySequence("Backward");
                    }
                    if (bPlayed) {
                        m_TileCrossFadeStartTime = BSGetTickCount();
                        ++m_SlideCount;
                        if (GetFlag(LoadingMenu::LM_NEW_LOADSCREEN_LIST)) {
                            SetFlag(LoadingMenu::LM_STATIC_MENU, LoadScreens.ItemsInList() == 1);
                            SetFlag(LoadingMenu::LM_NEW_LOADSCREEN_LIST, 0);
                        }
                        NiUpdateData data(0.0f, true, false);
                        p3DTile->GetModel()->Update(data);
                    }
                    else {
                        const char* pSequenceName = GetFlag(LoadingMenu::LM_FORWARD_ANIM) ? "Forward" : "Backward";
                        Warning("MENUS: (Loading Menu) Unable to play sequence %s", pSequenceName);
                    }
                }
            }
        }
        else {
            if (LoadingMenu::bInitialLoad) {
                while (!TES::pInstance) {
                    BSSleep(5u);
                }
                SetNextSlide(pSlide1TSP);
            }
            if (uiFadeInComplete || !GetFlag(LoadingMenu::LM_FADE_IN)) {
                uiFadeInComplete = BSGetTickCount();
            }
            else {
                FadeIn();
            }
            uiNextBackgroundSwap = uiFadeInComplete;
        }
    }
    else if (
        !LoadingMenu::bInitialLoad ||
        GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE) ||
        pMoviePlayer->GetPlayingSequence() ||
        !BSstrlen(BGSMoviePlayer::sMainMenuMovieIntro.String()) ||
        bMainMenuMovieIntroStarted
    ) {
        if (LoadingMenu::bInitialLoad &&
            !GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE) &&
            !pMoviePlayer->GetPlayingSequence() &&
            !bMainMenuMovieIntroStarted) {
            if (!BSstrlen(BGSMoviePlayer::sMainMenuMovieIntro.String())) {
                char cfilename[64];
                ourMemset(cfilename, 0, 64u);
                BSsprintf(cfilename, 64, "Data\\Music\\Special\\%s.mp3", gs_sTitleMusic_Loading.String());
                FalloutAudio::SetMainMenuMusic(true);
                FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, cfilename, 0, true, true, 0.0f, 0);
                FalloutAudioMedia::MediaMenuUnPause();
                FalloutAudioMedia::fMediaAVolume = 0.6f;
                FalloutAudioMedia::fMediaBVolume = 0.6f;
                bMainMenuMovieIntroStarted = true;
            }
        }
    }
    else {
        char cfilename[64];
        ourMemset(cfilename, 0, 64u);
        BSsprintf(cfilename, 64, "Data\\Music\\Special\\%s.mp3", gs_sTitleMusic_Loading.String());
        FalloutAudio::SetMainMenuMusic(true);
        FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, cfilename, 0, true, true, 0.0f, 0);
        FalloutAudioMedia::MediaMenuUnPause();
        FalloutAudioMedia::fMediaAVolume = 0.6f;
        FalloutAudioMedia::fMediaBVolume = 0.6f;
        pTiles[0]->GetModel()->SetAppCulled(true);
        pTiles[32]->Set(4009, 0.0f, true);
        pTiles[33]->Set(4009, 255.0f, true);
        const char* pIntroMovie = BGSMoviePlayer::sMainMenuMovieIntro.String();
        pMoviePlayer->StartSequence(pIntroMovie, false, -1, false, false, 0.0f);
        bMainMenuMovieIntroStarted = true;
        pTiles[11]->Set(4003, 1);
        LoadingMenu::bDoTileUpdate = true;
    }
    if (pTiles[11]->IsVisible()) {
        ResetLoadingPinwheelAnimation();
    }
    BSAudio::QInstance().Update(true);
    FalloutAudioMedia::Update();
}

// The initial load (outside the main menu) shows the welcome screens, skipping the first three when the main menu has
// an intro movie; other loads pick from the candidate screens, retried without the location when it yields too few. An
// empty list gets the first welcome screen.
void LoadingMenu::MakeLoadScreenList() {
    bool bCleared = false;
    if (!this->LoadScreens.IsEmpty()) {
        this->FlushLoadScreenList();
        bCleared = true;
    }
    this->SetFlag(LM_USE_MAINMENU_SCREENS, StartMenu::MainMenuIsActive() || appMain->GameReset());
    if (LoadingMenu::bInitialLoad && !StartMenu::MainMenuIsActive()) {
        WelcomeScreenID eID = LM_INDIAN_HEAD;
        if (BSstrlen(BGSMoviePlayer::sMainMenuMovieIntro.String())) {
            eID = LM_LEGAL;
        }
        for (; eID < LM_NUM_WELCOME_SCREENS; eID++) {
            const char* pFolder = eID == LM_MAIN_BACKGROUND ? "Main" : "Loading";
            this->strBuffer.SPrintF("Interface\\%s\\%s.dds", pFolder, LoadingMenu::pWelcomeScreensA[eID].String());
            TESLoadScreen* pLoadScreen = new TESLoadScreen;
            pLoadScreen->SetTextureName(this->strBuffer.String());
            pLoadScreen->SetTemporary();
            this->LoadScreens.AddTail(pLoadScreen);
        }
    }
    else {
        alignas(BSSimpleArray<TESLoadScreen*>) uint8_t aCandidatesStorage[sizeof(BSSimpleArray<TESLoadScreen*>)];
        BSSimpleArray<TESLoadScreen*>* pCandidates = ::new (aCandidatesStorage) BSSimpleArray<TESLoadScreen*>;
        this->BuildCandidateList(*pCandidates);
        this->AddCandidates(*pCandidates);
        // The count is compared unsigned.
        if (this->pLocation &&
            static_cast<uint32_t>(this->LoadScreens.ItemsInList()) < LoadingMenu::iMaxScreens.UInt() &&
            !TESLoadScreen::QUseLocationSpecificScreensOnly()) {
            this->pLocation = nullptr;
            this->BuildCandidateList(*pCandidates);
            this->AddCandidates(*pCandidates);
        }
        pCandidates->~BSSimpleArray<TESLoadScreen*>();
    }
    if (this->LoadScreens.IsEmpty()) {
        this->strBuffer.SPrintF("Interface\\Loading\\%s.dds", LoadingMenu::pWelcomeScreensA[LM_INDIAN_HEAD].String());
        TESLoadScreen* pLoadScreen = new TESLoadScreen;
        pLoadScreen->SetTextureName(this->strBuffer.String());
        pLoadScreen->SetTemporary();
        this->LoadScreens.AddHead(pLoadScreen);
    }
    if (bCleared && !static_cast<Tile3D*>(this->pTiles[0])->IsPlaying()) {
        this->SetNextSlide(nullptr);
        this->NextSwap();
    }
    this->SetFlag(LM_NEW_LOADSCREEN_LIST, 1);
    this->pCurrentLoadScreen = this->LoadScreens.GetItem();
    this->InitPlayerTileSettings();
    this->UpdatePlayerTileVisibilities(this->pCurrentLoadScreen, true);
}

// pTiles[0] is read before the tests.
bool LoadingMenu::ForceFinish() {
    Tile3D* pBackground = static_cast<Tile3D*>(this->pTiles[0]);
    if (!LoadingMenu::bInitialLoad || this->GetFlag(LM_INITLOAD_COMPLETE) || pBackground->IsPlaying()) {
        return false;
    }
    while (this->pNextLoadScreen->GetNext()) {
        this->pNextLoadScreen = this->pNextLoadScreen->GetNext();
    }
    this->SetNextSlide(nullptr);
    this->uiNextBackgroundSwap = BSGetTickCount();
    return true;
}

// AddTail takes its item by reference, so each tile slot is passed by address.
void LoadingMenu::InitXPProgressTilesList() {
    this->XPProgressTilesList.RemoveAll();
    this->XPProgressTilesList.AddTail(this->pTiles[16]);
    this->XPProgressTilesList.AddTail(this->pTiles[28]);
    this->XPProgressTilesList.AddTail(this->pTiles[29]);
    this->XPProgressTilesList.AddTail(this->pTiles[15]);
    this->XPProgressTilesList.AddTail(this->pTiles[10]);
    this->XPProgressTilesList.AddTail(this->pTiles[30]);
}

// An AutoMemContext is constructed on the stack (with this file's path and line 0x423) and the constructor's result is
// ignored; the texture pointer is released before the context. The index of spHoldTexture is read, then read again for
// the increment and for the unsigned wrap test.
void LoadingMenu::SetNextSlide(TileShaderProperty* apTSP) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Menus\\LoadingMenu.cpp",
        1059
    );
    {
        this->strBuffer.SPrintF("%s\\%s", "Textures", this->pNextLoadScreen->GetItem()->GetTextureName());
        NiPointer<NiSourceTexture> spTexture(nullptr);
        TES::pInstance->CreateTextureImage(this->strBuffer.String(), spTexture, false, false);
        NiPointer<NiTexture>* pspHoldTexture = &this->spHoldTexture[this->uiCurrTextureIndex];
        ++this->uiCurrTextureIndex;
        if (this->uiCurrTextureIndex >= 4) {
            this->uiCurrTextureIndex = 0;
        }
        if (*pspHoldTexture) {
            BSTexturePalette::RemoveTexture(*pspHoldTexture);
        }
        NiSourceTexture* pTexture = spTexture;
        *pspHoldTexture = pTexture;
        // Odd slides go to the second slide tile.
        if (this->m_SlideCount % 2) {
            apTSP = this->pTiles[33]->GetShaderProperty();
        }
        else {
            apTSP = this->pTiles[32]->GetShaderProperty();
        }
        if (apTSP) {
            apTSP->SetTileTexture(spTexture);
        }
    }
    pContext->~AutoMemContext();
}

// `delete` is the deleting destructor with flag 1 after a null test.
void LoadingMenu::FlushLoadScreenList() {
    for (BSSimpleList<TESLoadScreen*>* pNode = &this->LoadScreens; pNode && pNode->GetItem();
         pNode = pNode->GetNext()) {
        if (pNode->GetItem()->GetTemporary()) {
            delete pNode->GetItem();
        }
    }
    this->LoadScreens.RemoveAll();
    this->pNextLoadScreen = &this->LoadScreens;
}

// The result is kept in a byte that each comparison overwrites.
bool LoadingMenu::OK2Add(TESLoadScreen* apLoadScreen) {
    bool bOK = apLoadScreen && apLoadScreen->GetFormType() == LSCR_ID;
    for (BSSimpleList<TESLoadScreen*>* pNode = &this->LoadScreens; bOK && pNode && pNode->GetItem();
         pNode = pNode->GetNext()) {
        bOK = BSstrcmp(apLoadScreen->GetTextureName(), pNode->GetItem()->GetTextureName()) != 0;
    }
    return bOK;
}

// eBackground also keeps the slide tiles. Each visibility goes through Tile::Set's float form.
void LoadingMenu::HideAllBut(TileIndex aeTileIndex, bool abShowPlayerTiles) {
    for (int32_t i = 0; i < NUM_TILES; ++i) {
        const int32_t iVisible = i == aeTileIndex ||
            (aeTileIndex == eBackground && (i == eTileSlide01 || i == eTileSlide02));
        this->pTiles[i]->Set(Tile::kTileValue_Visible, static_cast<float>(iVisible), false);
    }
    this->SetFlag(LM_SHOW_PLAYER_TILES, abShowPlayerTiles);
}

void LoadingMenu::FadeIn3D(NiAVObject* apObject, float afPct) {
    if (!apObject) {
        return;
    }
    NiNode* pNode = apObject->IsNode();
    // The compare is unsigned.
    for (uint32_t i = 0; pNode && i < pNode->GetArrayCount(); ++i) {
        this->FadeIn3D(pNode->GetAt(i), afPct);
    }
    if (apObject->IsGeometry()) {
        NiProperty* pProperty = apObject->GetProperty(NiShadeProperty::GetType());
        if (pProperty) {
            static_cast<TileShaderProperty*>(pProperty)->SetTileAlpha(afPct);
        }
    }
}

void LoadingMenu::GetPlayerTileDefaultSettings() {
    for (int32_t i = 0; i < 38; ++i) {
        int32_t iOriginX = 0;
        int32_t iOriginY = 0;
        int32_t iHeight = 0;
        int32_t iWidth = 0;
        FONTS eFontType = FONT_2;
        uint8_t ucJustification = 4;
        if (pTiles[i]) {
            iOriginX = static_cast<int32_t>(pTiles[i]->GetFloat(4001));
            iOriginY = static_cast<int32_t>(pTiles[i]->GetFloat(4002));
            iHeight = static_cast<int32_t>(pTiles[i]->GetFloat(4016));
            iWidth = static_cast<int32_t>(pTiles[i]->GetFloat(4017));
            eFontType = static_cast<FONTS>(static_cast<int32_t>(pTiles[i]->GetFloat(4025)));
            ucJustification = static_cast<int32_t>(pTiles[i]->GetFloat(4023));
        }
        DefaultTileSettings[i].nOriginX = iOriginX;
        DefaultTileSettings[i].nOriginY = iOriginY;
        DefaultTileSettings[i].nHeight = iHeight;
        DefaultTileSettings[i].nWidth = iWidth;
        DefaultTileSettings[i].eFontType = eFontType;
        DefaultTileSettings[i].eJustification = ucJustification;
    }
}

// The stacking type is read twice, each time through Tile::GetFloat. Interface mode 3 is kept, anything else becomes 2.
// Tile 0 is the slide background, tile 11 the pinwheel. When Slide01 is a node, Slide02's first child is taken without
// testing that Slide02 is a node too. The renderer's lock calls are NiDX9Renderer's. The background
// tile is loaded once for PlaySequence and the update.
void LoadingMenu::_Create(TESForm* apLocation) {
    this->uiNextBackgroundSwap = 0;
    if (this->pMenu->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
        this->pMenu->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
        this->pMenu->Set(Tile::kTileValue_Depth, Tile::GetMaximumDepth(), true);
    }
    if (InterfaceManager::Instance()->GetCurrentMode() != 3) {
        InterfaceManager::Instance()->InternalSetMenuMode(InterfaceManager::eMenuModeOn);
    }
    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_LOAD);
    HUDMainMenu::SetInfoForRef(nullptr, false, false);
    HUDMainMenu::HandleHealthbarForCrosshairActor_(nullptr);
    if (pPlayer && !pMoviePlayer->GetPlayingSequence()) {
        pMoviePlayer->Stop();
    }
    TileMenu* pSleepWaitMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eSleepWaitMenu));
    if (pSleepWaitMenu) {
        delete pSleepWaitMenu;
    }
    this->pLocation = apLocation;

    if (LoadingMenu::bInitialLoad || pPlayCredits->GetValue() != 0.0) {
        this->HideAllBut(eBackground, false);
    }
    else if (Interface::bIsLoadingNewGame || appMain->GameReset()) {
        this->ShowPlayerTiles(false);
        static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->LockRenderer();
        this->ResetLoadingPinwheelAnimation();
        static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
    }
    else {
        if (this->pLocation && !pSaveLoadGame->GetSaveGameLoading()) {
            switch (this->pLocation->GetFormType()) {
                case CELL_ID:
                    this->SetFlag(LM_WHEEL_ONLY_MODE, static_cast<TESObjectCELL*>(this->pLocation)->GetInterior());
                    break;
                case REFR_ID:
                case ACHR_ID:
                case ACRE_ID:
                case PMIS_ID:
                case PGRE_ID:
                case PBEA_ID:
                case PFLA_ID:
                case PCBE_ID:
                    this->SetFlag(LM_WHEEL_ONLY_MODE, static_cast<TESObjectREFR*>(this->pLocation)->GetInterior());
                    break;
                default: break;
            }
        }
        if (this->GetFlag(LM_WHEEL_ONLY_MODE)) {
            this->HideAllBut(ePinwheel, false);
            this->SetFlag(LM_FADE_IN, 0);
            this->uiFadeInComplete = BSGetTickCount();
            LoadingMenu::bDoTileUpdate = true;
        }
        else {
            this->ShowPlayerTiles(true);
            this->PrepPlayerTiles();
        }
        static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->LockRenderer();
        this->ResetLoadingPinwheelAnimation();
        static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
    }
    NiRenderer::GetRenderer()->SetBackgroundColor(NiColorA::BLACK);

    // The scale is read before the model.
    if (this->pTiles[ePinwheel]->GetModel()) {
        const float fScale = fLoadingWheelScale;
        this->pTiles[ePinwheel]->GetModel()->SetScale(fScale);
        this->pTiles[ePinwheel]->Set(Tile::kTileValue_User0, fLoadingWheelScale.Float(), true);
        this->pTiles[ePinwheel]->GetModel()->SetRotate(1.5707964f, 1.0f, 0.0f, 0.0f);
    }
    else {
        Warning(WARN_MENUS, "WARNING:  Loading Menu wheel does not have a model.");
    }

    if (!this->GetFlag(LM_WHEEL_ONLY_MODE)) {
        NiNode* pModel = this->pTiles[0]->GetModel();
        NiFixedString kSlide01("Slide01");
        NiAVObject* pSlide = pModel->GetObjectByName(kSlide01);
        NiNode* pSlideNode = pSlide->IsNode();
        if (pSlideNode) {
            int32_t iType = NiShadeProperty::GetType();
            this->pSlide1TSP = static_cast<TileShaderProperty*>(pSlideNode->GetAtChecked(0)->GetProperty(iType));
            pModel = this->pTiles[0]->GetModel();
            NiFixedString kSlide02("Slide02");
            pSlideNode = static_cast<NiNode*>(pModel->GetObjectByName(kSlide02));
            iType = NiShadeProperty::GetType();
            this->pSlide2TSP = static_cast<TileShaderProperty*>(pSlideNode->GetAtChecked(0)->GetProperty(iType));
        }
        else {
            this->pSlide1TSP = static_cast<TileShaderProperty*>(pSlide->GetProperty(NiShadeProperty::GetType()));
            pModel = this->pTiles[0]->GetModel();
            NiFixedString kSlide02("Slide02");
            pSlide = pModel->GetObjectByName(kSlide02);
            this->pSlide2TSP = static_cast<TileShaderProperty*>(pSlide->GetProperty(NiShadeProperty::GetType()));
        }
        this->MakeLoadScreenList();
        if (!LoadingMenu::bInitialLoad) {
            this->SetScreenInfo(false);
            this->AdvanceScreen();
            this->SetNextSlide(nullptr);
            this->NextSwap();
        }
        this->pTiles[0]->GetModel()->SetScale(1.3333f);
        this->pTiles[0]->GetModel()->SetRotate(1.5707964f, 1.0f, 0.0f, 0.0f);
        // The width is stored as a double before the height is read; the ratio is rounded to float.
        const double dWidth = Interface::GetRealScreenWidth();
        const float fAspect = static_cast<float>(dWidth / Interface::GetRealScreenHeight());
        this->pTiles[0]->Set(Tile::kTileValue_User0, fAspect, true);
        Tile3D* pBackground = static_cast<Tile3D*>(this->pTiles[0]);
        pBackground->PlaySequence("Left");
        NiUpdateData kUpdateData(0.0f, true, false);
        pBackground->GetModel()->Update(kUpdateData);
        this->FadeIn();
        this->uiFadeInComplete = 0;
    }
    LoadingMenu::ShowChanges();

    // Audio runs on its own thread for the initial load and loads without a location (unless a starting cell is
    // set); other loads also fade the 3D sounds out.
    if ((LoadingMenu::bInitialLoad || !apLocation) &&
        BSAudio::QInstance().IsAudioEnabled() &&
        !*sStartingCell.String()) {
        if (!BSAudio::QInstance().IsMultiThreaded()) {
            BSAudio::QInstance().SetMultiThreaded(true);
            this->SetFlag(LM_AUDIO_THREADING, 1);
        }
    }
    else if (!LoadingMenu::bInitialLoad && BSAudio::QInstance().IsAudioEnabled()) {
        if (!BSAudio::QInstance().IsMultiThreaded()) {
            BSAudio::QInstance().SetMultiThreaded(true);
            this->SetFlag(LM_AUDIO_THREADING, 1);
        }
        BSAudio::QInstance().MuteType(BSGameSound::SOUND_FLAG_IS_3D, 1000, 6000);
    }
    this->m_TileCrossFadeStartTime = 0;
    this->pTiles[0]->GetModel()->SetAppCulled(true);
}

void LoadingMenu::Reset(TESForm* apLocation) {
    if (GetFlag(LoadingMenu::LM_RESET_MENU)) {
        LoadingMenu::SuspendBackgroundThread();
        m_TileCrossFadeStartTime = BSGetTickCount() - 4000;
        if (Interface::bIsLoadingNewGame || pSaveLoadGame->GetSaveGameLoading()) {
            Tile* pStartMenuTile = Menu::GetMenuByClass(StartMenu::GetClassStatic());
            StartMenu* pStartMenu = static_cast<StartMenu*>(pStartMenuTile->GetMenu());
            if (pStartMenu && !pStartMenu->IsFlagSet(StartMenu::FLAG_INGAME)) {
                delete pStartMenu;
            }
        }
        if (appMain->GameReset()) {
            ShowPlayerTiles(false);
        }
        else {
            ShowPlayerTiles(true);
            PrepPlayerTiles();
        }
        pLocation = apLocation;
        MakeLoadScreenList();
        pLocation = apLocation;
        uiNextBackgroundSwap = BSGetTickCount();
        ++m_SlideCount;
        SetScreenInfo(false);
        AdvanceScreen();
        SetNextSlide(nullptr);
        NextSwap();
        ++m_SlideCount;
        SetScreenInfo(false);
        AdvanceScreen();
        SetNextSlide(nullptr);
        NextSwap();
        SetFlag(LoadingMenu::LM_RESET_MENU, 0);
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->LockRenderer();
        if (!pTiles[11]->IsVisible()) {
            pTiles[11]->Set(4003, 1);
            ResetLoadingPinwheelAnimation();
        }
        InterfaceManager::Instance()->pCursor->Set(4003, 0);
        Tile::UpdateAll(true);
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->UnlockRenderer();
        LoadingMenu::bDoTileUpdate = true;
        LoadingMenu::ResumeBackgroundThread();
    }
}

void LoadingMenu::ShowPlayerTiles(bool abShow) {
    SetFlag(LoadingMenu::LM_SHOW_PLAYER_TILES, abShow);
}

void LoadingMenu::PrepPlayerTiles() {
    const char* pObjectiveText;
    if (!pSaveLoadGame || !pSaveLoadGame->GetSaveGameLoading()) {
        LoadingMenu::FillDisplayData();
    }
    pTiles[10]->Set(Tile::kTileValue_User0, LoadingMenu::DisplayData.fLevelProgress, true);
    // The signed level is passed to the unsigned overload.
    pTiles[10]->Set(Tile::kTileValue_User1, static_cast<uint32_t>(LoadingMenu::DisplayData.iLevel));
    if (LoadingMenu::DisplayData.pCurrentObjective) {
        pObjectiveText = LoadingMenu::DisplayData.pCurrentObjective->GetDisplayText();
    }
    else {
        pObjectiveText = sNone.String();
    }
    pTiles[3]->Set(4036, pObjectiveText, true);
    const char* pObjectiveLabel = sCurrentObjective.String();
    pTiles[14]->Set(4036, pObjectiveLabel, true);
    const char* pProgressLabel = sLevelProgress.String();
    pTiles[15]->Set(4036, pProgressLabel, true);
}

void LoadingMenu::SetScreenInfo(bool abRefreshCurrent) {
    // Hashes the texture name and then never uses the result.
    uint32_t uiHash = 0;
    for (const char* pNext = pNextLoadScreen->m_item->GetTextureName(); *pNext; ++pNext) {
        uiHash = std::tolower(*pNext) + 33 * uiHash;
    }
    uiHash %= 100000;
    if (m_SlideCount % 2) {
        m_DelayedTipMatteUpdate |= 2;
        TESDescription* pDescription = static_cast<TESDescription*>(pNextLoadScreen->m_item);
        const char* pText = pDescription->GetDescription(nullptr, DESC_ID);
        pTiles[35]->Set(4036, pText, true);
    }
    else {
        m_DelayedTipMatteUpdate |= 1;
        TESDescription* pDescription = static_cast<TESDescription*>(pNextLoadScreen->m_item);
        const char* pText = pDescription->GetDescription(nullptr, DESC_ID);
        pTiles[34]->Set(4036, pText, true);
    }
    const char* pPlate = sVDSGPlate.String();
    pTiles[13]->Set(4036, pPlate, true);
    LoadingMenu::bDoTileUpdate = true;
}

void LoadingMenu::AdvanceScreen() {
    pCurrentLoadScreen = pNextLoadScreen->m_item;
    pNextLoadScreen = pNextLoadScreen->m_pkNext;
    if (!pNextLoadScreen) {
        pNextLoadScreen = &LoadScreens;
    }
    if (pNextLoadScreen && pNextLoadScreen->m_item) {
        UpdatePlayerTileSettings(pCurrentLoadScreen);
    }
}

void LoadingMenu::FadeIn() {
    static float g_loadingMenuMenuFade = static_cast<float>(pMenu->GetFloat(4033) * 1000.0);
    if (!uiFadeInComplete) {
        if (appMain->GetMenuBGTexture() || LoadingMenu::bInitialLoad) {
            NiRenderedTexture* menuBGTexture = appMain->GetMenuBGTexture();
            static_cast<TileImage*>(pTiles[12])->SetSourceTexture(menuBGTexture);
            uiFadeInComplete = static_cast<uint32_t>(
                static_cast<int64_t>(static_cast<double>(BSGetTickCount()) + g_loadingMenuMenuFade)
            );
        }
        else {
            uiFadeInComplete = BSGetTickCount();
        }
    }
    if (BSGetTickCount() >= uiFadeInComplete) {
        FadeIn(pMenu, 1.0f);
        SetFlag(LoadingMenu::LM_FADE_IN, 0);
        ++m_SlideCount;
    }
    else {
        float fTime = static_cast<float>(
            1.0 - static_cast<double>(uiFadeInComplete - BSGetTickCount()) / g_loadingMenuMenuFade
        );
        FadeIn(pMenu, fTime);
    }
}

void LoadingMenu::FadeIn(Tile* apTile, float afTime) {
    if (apTile && apTile != pTiles[33]) {
        float fAlpha = BSMIN(afTime, 1.0f);
        fAlpha = BSMAX(fAlpha, 0.0f);
        if (apTile->GetType() == 904) {
            FadeIn3D(apTile->GetModel(), fAlpha);
        }
        else {
            if (apTile == pTiles[12]) {
                fAlpha = 1.0 - fAlpha;
            }
            float fAlphaByte = fAlpha * 255.0;
            apTile->Set(4009, fAlphaByte, true);
            LoadingMenu::bDoTileUpdate = true;
            for (NiTListItem<Tile*>* piter = apTile->xChildren.m_pkHead; piter;
                 FadeIn(apTile->xChildren.GetNext(piter), fAlpha)) {
                ;
            }
        }
    }
}

void LoadingMenu::UpdatePlayerTileSettings(TESLoadScreen* pLoadScreen) {
    if (!this->GetFlag(LM_SHOW_PLAYER_TILES)) {
        return;
    }
    if (!pLoadScreen) {
        if (this->pNextLoadScreen) {
            pLoadScreen = this->pNextLoadScreen->GetItem();
        }
        else {
            pLoadScreen = this->LoadScreens.GetItem();
        }
        if (!pLoadScreen) {
            return;
        }
    }
    if (pLoadScreen->GetLoadScreenType()) {
        LoadScreenType_Data* pData = pLoadScreen->GetLoadScreenTypeData();
        this->CalcLoadScreenTypeTextWindowFaces(pData);
        switch (pLoadScreen->GetLoadScreenTypeDataType()) {
            case LSDT_XP_PROGRESS: this->InitXPProgressTilesSettings(pData); break;
            case LSDT_OBJECTIVE: this->InitObjectiveTilesSettings(pData); break;
            case LSDT_TIP: this->InitTipTilesSettings(pData); break;
            case LSDT_STATS: this->InitStatsTilesSettings(pData); break;
            default: break;
        }
    }
}

void LoadingMenu::CalcLoadScreenTypeTextWindowFaces(const LoadScreenType_Data* apData) {
    if (apData) {
        float fCurrentScreenWidth = InterfaceManager::GetScreenWidth();
        float fCurrentScreenHeight = InterfaceManager::GetScreenHeight();
        float fSafeWidth = fCurrentScreenWidth - InterfaceManager::GetScreenSafeX();
        float fSafeHeight = fCurrentScreenHeight - InterfaceManager::GetScreenSafeY();
        float fConvertedXPos = static_cast<double>(apData->DisplayData1.uDisplayPositionX) / 1280.0 *
            fCurrentScreenWidth;
        float fConvertedYPos = static_cast<double>(apData->DisplayData1.uDisplayPositionY) / 960.0 *
            fCurrentScreenHeight;
        float fConvertedWidth = static_cast<double>(apData->DisplayData1.uDisplayWidth) / 1280.0 * fCurrentScreenWidth;
        float fConvertedHeight = static_cast<double>(apData->DisplayData1.uDisplayHeight) / 1280.0 *
            fCurrentScreenHeight;
        if (apData->DataType) {
            float fSafeX = InterfaceManager::GetScreenSafeX();
            iMinX = static_cast<int32_t>(BSMAX(fConvertedXPos, fSafeX));
            float fSafeY = InterfaceManager::GetScreenSafeY();
            iMinY = static_cast<int32_t>(BSMAX(fConvertedYPos, fSafeY));
            float fMaxX = static_cast<double>(iMinX) + fConvertedWidth;
            iMaxX = static_cast<int32_t>(BSMIN(fMaxX, fSafeWidth));
            float fMaxY = static_cast<double>(iMinY) + fConvertedHeight;
            iMaxY = static_cast<int32_t>(BSMIN(fMaxY, fSafeHeight));
        }
    }
    else {
        InitializationFlow("WARNING: LoadingMenu::CalcLoadScreenTypeTextWindowVerts() received a NULL apData.");
    }
}

void LoadingMenu::InitStatsTilesSettings(LoadScreenType_Data* apData) {
    pTiles[31]->Set(4001, static_cast<float>(iMinX), true);
    pTiles[31]->Set(4017, static_cast<float>(iMaxX - iMinX), true);
    pTiles[31]->Set(4002, static_cast<float>(iMinY), true);
    pTiles[31]->Set(4016, static_cast<float>(iMaxY - iMinY), true);
    pTiles[17]->Set(4025, apData->DisplayData1.eDisplayFont);
    pTiles[17]->Set(4018, apData->DisplayData1.TextColorR, true);
    pTiles[17]->Set(4019, apData->DisplayData1.TextColorG, true);
    pTiles[17]->Set(4020, apData->DisplayData1.TextColorB, true);
    pTiles[18]->Set(4025, apData->DisplayData2.eDisplayFont);
    pTiles[17]->Set(4018, apData->DisplayData2.TextColorR, true);
    pTiles[17]->Set(4019, apData->DisplayData2.TextColorG, true);
    pTiles[17]->Set(4020, apData->DisplayData2.TextColorB, true);
    float fStatRowHeight = pTiles[31]->GetFloat(4016) / static_cast<double>(apData->uNumStatsToDisplay);
    float fYOffset = 0.0f;
    NiTListItem<Tile*>* pLabelIter = pTiles[17]->xChildren.m_pkTail;
    NiTListItem<Tile*>* pValIter = pTiles[18]->xChildren.m_pkTail;
    uint32_t nRowCount = 1;
    while (pLabelIter && pValIter) {
        Tile* pLabelChild = pTiles[17]->xChildren.GetPrev(pLabelIter);
        Tile* pValChild = pTiles[18]->xChildren.GetPrev(pValIter);
        if (pLabelChild && pValChild) {
            bool bIsVisible = true;
            if (nRowCount > apData->uNumStatsToDisplay) {
                bIsVisible = false;
            }
            else {
                pLabelChild->Set(4002, fYOffset, true);
                pValChild->Set(4002, fYOffset, true);
                fYOffset = fYOffset + fStatRowHeight;
                pLabelChild->AddNeedsUpdate(8);
                pValChild->AddNeedsUpdate(8);
                ++nRowCount;
            }
            pLabelChild->Set(4003, static_cast<int32_t>(bIsVisible));
            pValChild->Set(4003, static_cast<int32_t>(bIsVisible));
        }
    }
    pTiles[31]->Set(4090, apData->DisplayData1.fDisplayRotationRads, true);
    pTiles[31]->RecursiveRotationUpdate();
}

void LoadingMenu::InitObjectiveTilesSettings(LoadScreenType_Data* apData) {
    FontManager* pFontManager = FontManager::Instance();
    NiTListItem<Tile*>* piter = ObjectiveTilesList.m_pkHead;
    while (piter) {
        Tile* pTile = ObjectiveTilesList.GetNext(piter);
        if (pTile) {
            if (pTile == pTiles[27]) {
                float fX = 0.0f;
                uint32_t uJustification = apData->DisplayData1.uJustification;
                switch (uJustification) {
                    case 1u: {
                        fX = static_cast<float>(iMinX);
                        break;
                    }
                    case 2u: {
                        fX = static_cast<double>(iMinX) + static_cast<double>(iMaxX - iMinX) / 2.0;
                        break;
                    }
                    case 4u: {
                        fX = static_cast<float>(iMaxX);
                        break;
                    }
                }
                pTile->Set(4001, fX, true);
                pTile->Set(4017, static_cast<float>(iMaxX - iMinX), true);
                float fWrapWidth = pTile->GetFloat(4017);
                pTile->Set(4026, fWrapWidth, true);
                pTile->Set(4002, static_cast<float>(iMinY), true);
                pTile->Set(4025, apData->DisplayData1.eDisplayFont);
                pTile->Set(4023, apData->DisplayData1.uJustification);
                pTile->Set(4018, apData->DisplayData1.TextColorR, true);
                pTile->Set(4019, apData->DisplayData1.TextColorG, true);
                pTile->Set(4020, apData->DisplayData1.TextColorB, true);
                pTile->Set(4085, -1);
                pTile->AddNeedsUpdate(8);
            }
            else if (pTile == pTiles[3]) {
                float fWrapWidth = static_cast<float>(iMaxX - iMinX);
                int32_t eDisplayFont = apData->DisplayData1.eDisplayFont;
                const char* pLabel = sCurrentObjective.String();
                pFontManager = FontManager::Instance();
                NiPoint3 nip3Dims = pFontManager->CalculateStringDimensions(pLabel, eDisplayFont, fWrapWidth, 0);
                pTile->Set(4002, nip3Dims.y, true);
            }
        }
    }
    pTiles[27]->Set(4090, apData->DisplayData1.fDisplayRotationRads, true);
    pTiles[27]->RecursiveRotationUpdate();
}

void LoadingMenu::InitXPProgressTilesSettings(LoadScreenType_Data* apData) {
    NiTListItem<Tile*>* piter = XPProgressTilesList.m_pkHead;
    while (piter) {
        Tile* pTile = XPProgressTilesList.GetNext(piter);
        if (pTile) {
            if (pTile == pTiles[16]) {
                pTile->Set(4001, static_cast<float>(iMinX), true);
                double dMaxX = static_cast<double>(this->iMaxX);
                float fWidth = dMaxX - pTile->GetFloat(4001);
                pTile->Set(4017, fWidth, true);
                float fWrapWidth = pTile->GetFloat(4017);
                pTile->Set(4026, fWrapWidth, true);
                pTile->Set(4002, static_cast<float>(iMinY), true);
                double dMaxY = static_cast<double>(this->iMaxY);
                float fHeight = dMaxY - pTile->GetFloat(4002);
                pTile->Set(4016, fHeight, true);
            }
            else {
                if (pTile == pTiles[15]) {
                    float fX = 0.0f;
                    uint32_t uJustification = apData->DisplayData1.uJustification;
                    switch (uJustification) {
                        case 1u: {
                            fX = 0.0f;
                            break;
                        }
                        case 2u: {
                            fX = static_cast<double>(this->iMaxX - iMinX) / 2.0;
                            break;
                        }
                        case 4u: {
                            fX = static_cast<float>(this->iMaxX - iMinX);
                            break;
                        }
                    }
                    pTile->Set(4001, fX, true);
                    pTile->Set(4023, apData->DisplayData1.uJustification);
                    pTile->Set(4025, apData->DisplayData1.eDisplayFont);
                    pTile->Set(4017, static_cast<float>(this->iMaxX - iMinX), true);
                    float fWrapWidth = pTile->GetFloat(4017);
                    pTile->Set(4026, fWrapWidth, true);
                    fWrapWidth = pTile->GetFloat(4026);
                    int32_t eDisplayFont = apData->DisplayData1.eDisplayFont;
                    const char* pLabel = sLevelProgress.String();
                    NiPoint3 nip3Dims = FontManager::Instance()
                                            ->CalculateStringDimensions(pLabel, eDisplayFont, fWrapWidth, 0);
                    pTile->Set(4016, nip3Dims.y, true);
                }
                else if (pTile == pTiles[10]) {
                    char cCurrLevel[264];
                    char cNextLevel[264];
                    // Converts to a 64-bit integer and passes its low 32 bits.
                    BSsprintf(cCurrLevel, 260, "%i", static_cast<int32_t>(static_cast<int64_t>(pTile->GetFloat(4101))));
                    float fWrapWidth = pTile->GetFloat(4026);
                    int32_t eDisplayFont = apData->DisplayData1.eDisplayFont;
                    FontManager* pFontManager = FontManager::Instance();
                    float fCurrLevelWidth = pFontManager
                                                ->CalculateStringDimensions(cCurrLevel, eDisplayFont, fWrapWidth, 0)
                                                .x;
                    pTile->Set(4001, fCurrLevelWidth, true);
                    // Converts to a 64-bit integer and passes its low 32 bits.
                    BSsprintf(cNextLevel, 260, "%i", static_cast<int32_t>(static_cast<int64_t>(pTile->GetFloat(4102))));
                    float fNextWrapWidth = (static_cast<double>(this->iMaxX - iMinX) - fCurrLevelWidth) / 2.0;
                    eDisplayFont = apData->DisplayData1.eDisplayFont;
                    pFontManager = FontManager::Instance();
                    float fNextLevelWidth = pFontManager
                                                ->CalculateStringDimensions(cNextLevel, eDisplayFont, fNextWrapWidth, 0)
                                                .x;
                    float fWidth = static_cast<double>(this->iMaxX - iMinX) - (fCurrLevelWidth + fCurrLevelWidth) -
                        fNextLevelWidth;
                    pTile->Set(4017, fWidth, true);
                }
                pTile->Set(4018, apData->DisplayData1.TextColorR, true);
                pTile->Set(4019, apData->DisplayData1.TextColorG, true);
                pTile->Set(4020, apData->DisplayData1.TextColorB, true);
                pTile->Set(4085, -1);
                pTile->AddNeedsUpdate(8);
            }
        }
    }
    pTiles[16]->Set(4090, apData->DisplayData1.fDisplayRotationRads, true);
    pTiles[16]->RecursiveRotationUpdate();
}

void LoadingMenu::InitTipTilesSettings(LoadScreenType_Data* apData) {
    NiTListItem<Tile*>* piter = TipTilesList.m_pkHead;
    while (piter) {
        Tile* pTile = TipTilesList.GetNext(piter);
        if (pTile) {
            float fX = 0.0f;
            uint32_t uJustification = apData->DisplayData1.uJustification;
            if (uJustification == 1) {
                float fMinX = static_cast<float>(this->iMinX);
                pTile->Set(4001, fMinX, true);
            }
            else if (uJustification == 2) {
                float fCenterX = static_cast<double>(this->iMinX) +
                    static_cast<double>(this->iMaxX - this->iMinX) / 2.0;
                pTile->Set(4001, fCenterX, true);
            }
            else {
                if (uJustification == 4) {
                    fX = static_cast<float>(this->iMaxX);
                }
                pTile->Set(4001, fX, true);
            }
            pTile->Set(4017, static_cast<float>(this->iMaxX - this->iMinX), true);
            float fWidth = pTile->GetFloat(4017);
            pTile->Set(4026, fWidth, true);
            pTile->Set(4002, static_cast<float>(iMinY), true);
            double dMaxY = static_cast<double>(this->iMaxY);
            float fHeight = dMaxY - pTile->GetFloat(4002);
            pTile->Set(4016, fHeight, true);
            fHeight = pTile->GetFloat(4016);
            pTile->Set(4027, fHeight, true);
            pTile->Set(4090, apData->DisplayData1.fDisplayRotationRads, true);
            pTile->Set(4025, apData->DisplayData1.eDisplayFont);
            pTile->Set(4023, apData->DisplayData1.uJustification);
            pTile->Set(4018, apData->DisplayData1.TextColorR, true);
            pTile->Set(4019, apData->DisplayData1.TextColorG, true);
            pTile->Set(4020, apData->DisplayData1.TextColorB, true);
            pTile->Set(4085, -1);
            pTile->AddNeedsUpdate(8);
        }
    }
    piter = TipTilesList02.m_pkHead;
    while (piter) {
        Tile* pTile = TipTilesList02.GetNext(piter);
        if (pTile) {
            float fX = 0.0f;
            uint32_t uJustification = apData->DisplayData1.uJustification;
            if (uJustification == 1) {
                float fMinX = static_cast<float>(this->iMinX);
                pTile->Set(4001, fMinX, true);
            }
            else if (uJustification == 2) {
                float fCenterX = static_cast<double>(this->iMinX) +
                    static_cast<double>(this->iMaxX - this->iMinX) / 2.0;
                pTile->Set(4001, fCenterX, true);
            }
            else {
                if (uJustification == 4) {
                    fX = static_cast<float>(this->iMaxX);
                }
                pTile->Set(4001, fX, true);
            }
            pTile->Set(4017, static_cast<float>(this->iMaxX - this->iMinX), true);
            float fWidth = pTile->GetFloat(4017);
            pTile->Set(4026, fWidth, true);
            pTile->Set(4002, static_cast<float>(iMinY), true);
            double dMaxY = static_cast<double>(this->iMaxY);
            float fHeight = dMaxY - pTile->GetFloat(4002);
            pTile->Set(4016, fHeight, true);
            fHeight = pTile->GetFloat(4016);
            pTile->Set(4027, fHeight, true);
            pTile->Set(4090, apData->DisplayData1.fDisplayRotationRads, true);
            pTile->Set(4025, apData->DisplayData1.eDisplayFont);
            pTile->Set(4023, apData->DisplayData1.uJustification);
            pTile->Set(4018, apData->DisplayData1.TextColorR, true);
            pTile->Set(4019, apData->DisplayData1.TextColorG, true);
            pTile->Set(4020, apData->DisplayData1.TextColorB, true);
            pTile->Set(4085, -1);
            pTile->AddNeedsUpdate(8);
        }
    }
}

void LoadingMenu::BuildCandidateList(BSSimpleArray<TESLoadScreen*, 1024>& aListOut) {
    aListOut.Clear(false);
    for (BSSimpleList<TESLoadScreen*>* pNode = pDataHandler->GetLoadScreenList(); pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        if (!GetFlag(LoadingMenu::LM_USE_MAINMENU_SCREENS) || !pNode->m_item->GetDisplaysOnMainMenu()) {
            if (GetFlag(LoadingMenu::LM_USE_MAINMENU_SCREENS)) {
                continue;
            }
            if (!this->pLocation || !pNode->m_item->IsValidForCell(static_cast<TESObjectCELL*>(this->pLocation))) {
                if (this->pLocation) {
                    continue;
                }
                if (!pNode->m_item->IsNonLocationSpecific()) {
                    continue;
                }
            }
        }
        uint32_t nIndex = aListOut.Add(pNode->m_item);
        LOAD_SCREEN_DATA_TYPE eCurrentType = pNode->m_item->GetLoadScreenTypeDataType();
        if ((Interface::bIsLoadingNewGame || appMain->GameReset()) && eCurrentType != LSDT_TIP && eCurrentType) {
            aListOut.RemoveFast(nIndex, 1u);
        }
    }
}

// The limit is iMaxScreens, or iMaxScreens_MainMenu for main-menu screens; a load into a location that may show generic
// screens caps it at iNumLocationSpecificScreens. Counts the list once and then counts the screens it adds, and removes
// each drawn candidate from the array whether it was added or not.
void LoadingMenu::AddCandidates(BSSimpleArray<TESLoadScreen*, 1024>& aCandidateList) {
    uint32_t uiMaxScreens = LoadingMenu::iMaxScreens.UInt();
    if (this->GetFlag(LM_USE_MAINMENU_SCREENS)) {
        uiMaxScreens = LoadingMenu::iMaxScreens_MainMenu.UInt();
    }
    else if (this->pLocation && !TESLoadScreen::QUseLocationSpecificScreensOnly()) {
        uiMaxScreens = BSMIN(uiMaxScreens, LoadingMenu::iNumLocationSpecificScreens.UInt());
    }
    uint32_t uiCount = static_cast<uint32_t>(this->LoadScreens.ItemsInList());
    while (uiCount < uiMaxScreens && aCandidateList.GetSize()) {
        uint32_t uiIndex = RandomUnsignedInt(aCandidateList.GetSize());
        if (this->OK2Add(static_cast<TESLoadScreen*>(aCandidateList.GetAt(uiIndex)))) {
            this->LoadScreens.AddTail(aCandidateList.GetAt(uiIndex));
            ++uiCount;
        }
        aCandidateList.RemoveFast(uiIndex, 1);
    }
}

void LoadingMenu::NextSwap() {
    float fInterval;
    if (StartMenu::MainMenuIsActive()) {
        fInterval = gs_fMainMenuBkgdUpdateInterval_Loading;
    }
    else if (LoadingMenu::bInitialLoad) {
        fInterval = gs_fLoadingInitUpdateInterval_Loading;
    }
    else {
        fInterval = gs_fLoadingBkgdUpdateInterval_Loading;
    }
    uiNextBackgroundSwap = static_cast<int64_t>(fInterval * 1000.0) + BSGetTickCount();
}

void LoadingMenu::InitPlayerTileSettings() {
    UpdatePlayerTileSettings(pCurrentLoadScreen);
}

void LoadingMenu::UpdatePlayerTileVisibilities(TESLoadScreen* pLoadScreen, bool bVisible) {
    if (GetFlag(LoadingMenu::LM_SHOW_PLAYER_TILES)) {
        if (!pLoadScreen) {
            pLoadScreen = !pNextLoadScreen ? LoadScreens.m_item : pNextLoadScreen->m_item;
        }
        if (pLoadScreen) {
            if (bVisible) {
                switch (pLoadScreen->GetLoadScreenTypeDataType()) {
                    case LSDT_XP_PROGRESS: {
                        UpdateObjectiveTilesVisibilities(!bVisible);
                        UpdateStatsTilesVisibilities(!bVisible);
                        UpdateTipTilesVisibilities(!bVisible);
                        UpdateXPProgressTilesVisibilities(bVisible);
                        break;
                    }
                    case LSDT_OBJECTIVE: {
                        UpdateObjectiveTilesVisibilities(bVisible);
                        UpdateStatsTilesVisibilities(!bVisible);
                        UpdateTipTilesVisibilities(!bVisible);
                        UpdateXPProgressTilesVisibilities(!bVisible);
                        break;
                    }
                    case LSDT_TIP: {
                        UpdateObjectiveTilesVisibilities(!bVisible);
                        UpdateStatsTilesVisibilities(!bVisible);
                        UpdateTipTilesVisibilities(bVisible);
                        UpdateXPProgressTilesVisibilities(!bVisible);
                        break;
                    }
                    case LSDT_STATS: {
                        UpdateObjectiveTilesVisibilities(!bVisible);
                        UpdateStatsTilesVisibilities(bVisible);
                        UpdateTipTilesVisibilities(!bVisible);
                        UpdateXPProgressTilesVisibilities(!bVisible);
                        break;
                    }
                    default: {
                        UpdateObjectiveTilesVisibilities(false);
                        UpdateStatsTilesVisibilities(false);
                        UpdateTipTilesVisibilities(false);
                        UpdateXPProgressTilesVisibilities(false);
                        break;
                    }
                }
            }
            else {
                UpdateObjectiveTilesVisibilities(bVisible);
                UpdateStatsTilesVisibilities(bVisible);
                UpdateTipTilesVisibilities(bVisible);
                UpdateXPProgressTilesVisibilities(bVisible);
            }
        }
    }
}

void LoadingMenu::UpdatePlayerTileAlphas(TESLoadScreen* pLoadScreen, float afAlpha, int32_t anSlide) {
    if (GetFlag(LoadingMenu::LM_SHOW_PLAYER_TILES)) {
        if (!pLoadScreen) {
            pLoadScreen = !pNextLoadScreen ? LoadScreens.m_item : pNextLoadScreen->m_item;
        }
        if (pLoadScreen && pLoadScreen->GetLoadScreenType()) {
            switch (pLoadScreen->GetLoadScreenTypeDataType()) {
                case LSDT_XP_PROGRESS: {
                    UpdateXPProgressTilesAlphas(afAlpha, anSlide);
                    break;
                }
                case LSDT_OBJECTIVE: {
                    UpdateObjectiveTilesAlphas(afAlpha, anSlide);
                    break;
                }
                case LSDT_TIP: {
                    UpdateTipTilesAlphas(afAlpha, anSlide);
                    break;
                }
                case LSDT_STATS: {
                    UpdateStatsTilesAlphas(afAlpha, anSlide);
                    break;
                }
                default: {
                    return;
                }
            }
        }
    }
}

void LoadingMenu::UpdateStatsTilesAlphas(float afAlpha, int32_t anSlide) {
    if (anSlide == 1) {
        NiTListItem<Tile*>* piter = StatsTilesList.m_pkHead;
        while (piter) {
            Tile* pTile = StatsTilesList.GetNext(piter);
            if (pTile) {
                if (pTile->GetFloat(4003) == 0.0) {
                    pTile->Set(4003, 1);
                }
                FadeIn(pTile, afAlpha);
            }
        }
    }
}

void LoadingMenu::UpdateObjectiveTilesAlphas(float afAlpha, int32_t anSlide) {
    if (anSlide == 1) {
        NiTListItem<Tile*>* piter = ObjectiveTilesList.m_pkHead;
        while (piter) {
            Tile* pTile = ObjectiveTilesList.GetNext(piter);
            if (pTile) {
                if (pTile->GetFloat(4003) == 0.0) {
                    pTile->Set(4003, 1);
                }
                FadeIn(pTile, afAlpha);
            }
        }
    }
}

void LoadingMenu::UpdateTipTilesAlphas(float afAlpha, int32_t anSlide) {
    NiTListItem<Tile*>* piter;
    if (anSlide == 1) {
        piter = TipTilesList.m_pkHead;
        while (piter) {
            Tile* pTile = TipTilesList.GetNext(piter);
            if (pTile) {
                if (pTile->GetFloat(4003) == 0.0) {
                    pTile->Set(4003, 1);
                }
                FadeIn(pTile, afAlpha);
            }
        }
        FadeIn(pTiles[36], afAlpha);
    }
    if (anSlide == 2) {
        piter = TipTilesList02.m_pkHead;
        while (piter) {
            Tile* pTile = TipTilesList02.GetNext(piter);
            if (pTile) {
                if (pTile->GetFloat(4003) == 0.0) {
                    pTile->Set(4003, 1);
                }
                FadeIn(pTile, afAlpha);
            }
        }
        FadeIn(pTiles[37], afAlpha);
    }
}

void LoadingMenu::UpdateXPProgressTilesAlphas(float afAlpha, int32_t anSlide) {
    NiTListItem<Tile*>* piter;
    if (anSlide == 1) {
        piter = XPProgressTilesList.m_pkHead;
        while (piter) {
            Tile* pTile = XPProgressTilesList.GetNext(piter);
            if (pTile) {
                if (pTile->GetFloat(4003) == 0.0) {
                    pTile->Set(4003, 1);
                }
                FadeIn(pTile, afAlpha);
            }
        }
    }
}

int32_t LoadingMenu::GetClassStatic() {
    return 1007;
}

// The menu found by GetMenuByClass is returned as is; a menu tile of another class is deleted when it has a menu tile.
Tile* LoadingMenu::Create(TESForm* apLocation, bool abBackgroundUpdate, bool abScripted) {
    TileMenu* pOldMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(GetClassStatic()));
    if (pOldMenu) {
        LoadingMenu::pMe->SetFlag(LM_RESET_MENU, 1);
        LoadingMenu::pMe->Reset(apLocation);
        LoadingMenu::pMe->SetFlag(LM_ENABLE_LOADING_MENU_COMMAND, abScripted);
        return pOldMenu;
    }

    LoadingMenu::bSuppressWarnings = true;
    Tile* pFile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\loading_menu.xml");
    LoadingMenu::bSuppressWarnings = false;
    Menu* pMenu = pFile->GetMenu();
    if (!pMenu || pMenu->GetClass() != GetClassStatic()) {
        if (pMenu && pMenu->GetMenuTile()) {
            delete pMenu;
        }
        return nullptr;
    }

    InterfaceManager::SetCursorAlpha(0);
    pMenu->SetMenuTile(static_cast<TileMenu*>(pFile), false);
    LoadingMenu::pMe = static_cast<LoadingMenu*>(pMenu);
    LoadingMenu::pMe->GetPlayerTileDefaultSettings();
    LoadingMenu::pMe->InitStatsTilesList();
    LoadingMenu::pMe->InitObjectiveTilesList();
    LoadingMenu::pMe->InitTipTilesList();
    LoadingMenu::pMe->InitXPProgressTilesList();
    LoadingMenu::pMe->UpdateObjectiveTilesVisibilities(false);
    LoadingMenu::pMe->UpdateTipTilesVisibilities(false);
    LoadingMenu::pMe->UpdateStatsTilesVisibilities(false);
    LoadingMenu::pMe->UpdateXPProgressTilesVisibilities(false);
    LoadingMenu::pMe->_Create(apLocation);
    LoadingMenu::pMe->SetFlag(LM_ENABLE_LOADING_MENU_COMMAND, abScripted);
    if (!LoadingMenu::pLoadingThread) {
        LoadingMenu::pLoadingThread = new LoadingMenuThread;
    }
    else if (abBackgroundUpdate) {
        ResumeBackgroundThread();
    }
    // The two slide tiles start transparent.
    LoadingMenu::pMe->pTiles[32]->Set(Tile::kTileValue_Alpha, 0);
    LoadingMenu::pMe->pTiles[33]->Set(Tile::kTileValue_Alpha, 0);
    return pFile;
}

void LoadingMenu::ShowChanges() {
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
    pRenderer->LockRenderer();
    BSCriticalSection* pTileCritSect = Tile::GetTileUpdateAllCriticalSectionLock();
    if (pTileCritSect->TryLock()) {
        if (TryEnter(&NiObjectNET::ExtraDataCrit.m_kCriticalSection, nullptr)) {
            if (!LoadingMenu::bDoTileUpdate || BSTexturePalette::TryLock()) {
                if (LoadingMenu::bDoTileUpdate) {
                    Tile::UpdateAll(true);
                    BSTexturePalette::Unlock();
                }
                MTRenderingSystem::QInstance()->StartThreads();
                for (int32_t i = 1; i < 23; ++i) {
                    MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
                    MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
                }
                LoadingMenu::bRendering = 1;
                InterfaceManager::Instance()->Click(nullptr, nullptr);
                LoadingMenu::bRendering = 0;
                MTRenderingSystem::QInstance()->WaitForThreads();
                pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->UnlockRenderer();
                pTileCritSect->Unlock();
                NiObjectNET::ExtraDataCrit.Unlock();
            }
            else {
                NiObjectNET::ExtraDataCrit.Unlock();
                pTileCritSect->Unlock();
                pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->UnlockRenderer();
            }
        }
        else {
            pTileCritSect->Unlock();
            pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
            pRenderer->UnlockRenderer();
        }
    }
    else {
        pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->UnlockRenderer();
    }
}

void LoadingMenu::FlagReset(bool abOn) {
    if (LoadingMenu::pMe) {
        LoadingMenu::pMe->SetFlag(LoadingMenu::LM_RESET_MENU, abOn);
    }
}

void LoadingMenu::SuspendBackgroundThread() {
    if (LoadingMenu::pMe &&
        LoadingMenu::pLoadingThread &&
        LoadingMenu::pLoadingThread->QIsThreadActive() &&
        !LoadingMenu::pLoadingThread->bFlagSuspend) {
        LoadingMenu::pLoadingThread->SetThreadPriority(1);
        LoadingMenu::pLoadingThread->bFlagSuspend = true;
        LoadingMenu::pLoadingThread->MainThreadSema.Wait();
    }
}

// Warns when the player's experience is below its level's.
float LoadingMenu::GetPlayerLevelProgress() {
    uint16_t usLevel = pPlayer->GetLevel();
    if (usLevel < iMaxCharacterLevel.Int()) {
        int32_t iLevel = pPlayer->GetLevel();
        int32_t iLowXP = GamePlayFormulas::GetRequiredExperiencePoints(iLevel);
        int32_t iHighXP = GamePlayFormulas::GetRequiredExperiencePoints(iLevel + 1);
        float fCurrXP = pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS);
        if (fCurrXP < static_cast<double>(iLowXP)) {
            Warning(
                WARN_MENUS,
                "Loading Menu:  Invalid Level Progress Values\n  LowXP=%d, HighXP=%d, CurrXP=%.2f",
                iLowXP,
                iHighXP,
                fCurrXP
            );
        }
        return (fCurrXP - static_cast<double>(iLowXP)) / static_cast<double>(iHighXP - iLowXP);
    }
    return 0.0f;
}

void LoadingMenu::SetupForStartMenu(int32_t) {
    if (LoadingMenu::pMe) {
        LoadingMenu::pMe->HideAllBut(eBackground, false);
    }
}

void LoadingMenu::SetScriptMode(uint8_t abScriptMode) {
    if (LoadingMenu::pMe) {
        LoadingMenu::pMe->SetFlag(LM_ENABLE_LOADING_MENU_COMMAND, abScriptMode);
    }
}

bool LoadingMenu::IsInScriptMode() {
    return LoadingMenu::pMe && LoadingMenu::pMe->GetFlag(LM_ENABLE_LOADING_MENU_COMMAND);
}

bool LoadingMenu::QResetFlagged() {
    return LoadingMenu::pMe && LoadingMenu::pMe->GetFlag(LM_RESET_MENU);
}

void LoadingMenu::SetWorldspaceAndChangedFlag(TESWorldSpace* apWorldSpace) {
    if (LoadingMenu::pMe && apWorldSpace && !LoadingMenu::pMe->pLocation) {
        FlagReset(true);
        LoadingMenu::pMe->Reset(apWorldSpace);
    }
}

void LoadingMenu::ResumeBackgroundThread() {
    if (LoadingMenu::pMe && LoadingMenu::pLoadingThread) {
        LoadingMenu::pLoadingThread->Unsuspend();
    }
}

void LoadingMenu::SetupForMainMenu(bool abCreate) {
    if (LoadingMenu::pMe) {
        LoadingMenu::SuspendBackgroundThread();
    }
    else {
        if (abCreate) {
            LoadingMenu::Create(nullptr, false, false);
        }
    }
    if (LoadingMenu::pMe) {
        LoadingMenu::pMe->HideAllBut(eBackground, false);
    }
}

void LoadingMenu::SetThreadExiting() {
    if (pLoadingThread) {
        pLoadingThread->bExit = true;
        pLoadingThread->Unsuspend();
    }
}

void LoadingMenu::FillDisplayData() {
    LoadingMenu::DisplayData.pCurrentObjective = pPlayer->GetCurrentQuestObjective();
    LoadingMenu::DisplayData.fLevelProgress = GetPlayerLevelProgress();
    LoadingMenu::DisplayData.iLevel = pPlayer->GetLevel();
}

void LoadingMenu::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    bool buseLocationSpecificScreensOnly;
    LoadingMenu::FillDisplayData();
    TESQuest* pquest = LoadingMenu::DisplayData.pCurrentObjective
        ? LoadingMenu::DisplayData.pCurrentObjective->pOwnerQuest
        : nullptr;
    uint32_t iindex = LoadingMenu::DisplayData.pCurrentObjective ? LoadingMenu::DisplayData.pCurrentObjective->iIndex
                                                                 : 0;
    apSaveGameBuffer->SaveFormID(pquest, 0u);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iindex), sizeof(iindex), 0);
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&LoadingMenu::DisplayData.fLevelProgress),
        sizeof(LoadingMenu::DisplayData.fLevelProgress),
        0
    );
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&LoadingMenu::DisplayData.iLevel),
        sizeof(LoadingMenu::DisplayData.iLevel),
        0
    );
    buseLocationSpecificScreensOnly = TESLoadScreen::QUseLocationSpecificScreensOnly();
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&buseLocationSpecificScreensOnly),
        sizeof(buseLocationSpecificScreensOnly),
        0
    );
}

void LoadingMenu::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    bool buseLocationSpecificScreensOnly;
    uint32_t iindex = 0;
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    TESQuest* pquest = dynamic_cast<TESQuest*>(pForm);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iindex), sizeof(iindex));
    LoadingMenu::DisplayData.pCurrentObjective = pquest ? pquest->GetObjective(iindex) : nullptr;
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&LoadingMenu::DisplayData.fLevelProgress),
        sizeof(LoadingMenu::DisplayData.fLevelProgress)
    );
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&LoadingMenu::DisplayData.iLevel),
        sizeof(LoadingMenu::DisplayData.iLevel)
    );
    if (LoadingMenu::DisplayData.iLevel >= iMaxCharacterLevel.UInt()) {
        LoadingMenu::DisplayData.fLevelProgress = 0.0f;
    }
    buseLocationSpecificScreensOnly = false;
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&buseLocationSpecificScreensOnly),
        sizeof(buseLocationSpecificScreensOnly)
    );
    SetIsLocationSpecificLoadScreensOnly(buseLocationSpecificScreensOnly);
    if (LoadingMenu::pMe) {
        LoadingMenu::pMe->pTiles[11]->Set(4003, 1);
        LoadingMenu::pMe->SetFlag(LoadingMenu::LM_STATIC_MENU, 0);
    }
    LoadingMenu::FlagReset(true);
}

LoadingMenu::WelcomeScreenID operator++(LoadingMenu::WelcomeScreenID& aeID, int32_t) {
    LoadingMenu::WelcomeScreenID eOld = aeID;
    aeID = static_cast<LoadingMenu::WelcomeScreenID>(aeID + 1);
    return eOld;
}

int32_t TryEnter(_RTL_CRITICAL_SECTION* apThis, [[maybe_unused]] void* apEdx) {
    return TryEnterCriticalSection(apThis);
}

LoadScreenType_Data* TESLoadScreen::GetLoadScreenTypeData() {
    return &pLoadScreenType->data;
}

// Returns whether the form's main-menu display flag is set.
bool TESLoadScreen::GetDisplaysOnMainMenu() {
    return (this->GetFormFlags() & 0x400) != 0;
}

bool TESLoadScreen::QUseLocationSpecificScreensOnly() {
    return LoadingMenu::bLocationSpecificLoadScreensOnly;
}

bool Main::GameReset() {
    return bResetGame;
}
