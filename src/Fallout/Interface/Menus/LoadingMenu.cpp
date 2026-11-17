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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <stdint.h>

// Cleared once the player tiles have been laid out for a load screen.
static bool bUpdatePlayerTilePositions = true;
LoadingMenuDisplayData LoadingMenu::DisplayData;
LoadingMenu::WelcomeScreenSettings LoadingMenu::pWelcomeScreensA = {
	SettingT<INISettingCollection>("sWelcomeScreen1:Loading", "loading_screen01"),
	SettingT<INISettingCollection>("sWelcomeScreen2:Loading", "loading_screen_bethsoft"),
	SettingT<INISettingCollection>("sWelcomeScreen3:Loading", "loading_screen_BGS"),
	SettingT<INISettingCollection>("sWelcomeScreen4:Loading", "loading_screen_legal"),
	SettingT<INISettingCollection>("sMainMenuBackground:Loading", "main_background"),
};
SettingT<INISettingCollection> gs_sTitleMusic_Loading("sTitleMusic:Loading", "MainTitle");
SettingT<INISettingCollection> LoadingMenu::sInitialSound("sInitialSound:Loading",
	"fx\\ui\\loadscreen\\initial\\ui_loadscreen_initial.wav");
SettingT<INISettingCollection> LoadingMenu::iMaxScreens("iMaxScreens:Loading", 4);
SettingT<INISettingCollection> LoadingMenu::iMaxScreens_MainMenu("iMaxScreens_MainMenu:Loading", 20);
SettingT<INISettingCollection> LoadingMenu::fLoadingTextUpdateInterval("fLoadingTextUpdateInterval:Loading", 10.0f);
SettingT<INISettingCollection> gs_fLoadingBkgdUpdateInterval_Loading("fLoadingBkgdUpdateInterval:Loading", 10.0f);
SettingT<INISettingCollection> gs_fMainMenuBkgdUpdateInterval_Loading("fMainMenuBkgdUpdateInterval:Loading", 10.0f);
SettingT<INISettingCollection> gs_fLoadingInitUpdateInterval_Loading("fLoadingInitUpdateInterval:Loading", 4.0f);
SettingT<INISettingCollection> LoadingMenu::iNumLocationSpecificScreens("iNumLocationSpecificScreens:Loading", 1);

LoadingMenuThread::LoadingMenuThread() : bFlagSuspend(0), bFlagSuspended(0), bExit(0) {
	SetName("LoadingMenu");
	Initialize(BSThread::SMALL_STACK_SIZE, "LoadingMenu", 0);
	SetThreadProcessor(5u);
	SetThreadPriority(-1);
}

LoadingMenuThread::~LoadingMenuThread() {}

uint32_t LoadingMenuThread::ThreadProc() {
	uint32_t uinext = BSGetTickCount();
	while (!bExit) {
		if (BSGetTickCount() < uinext) {
			uint32_t v1 = BSGetTickCount();
			BSSleep(uinext - v1);
		}
		if (!bFlagSuspend ||
			(LoadingMenu::bInitialLoad &&
				(!*sStartingCell.String() && static_cast<Tile3D*>(LoadingMenu::pMe->pTiles[0])->Tile3D::IsPlaying()))) {
			uinext = BSGetTickCount() + 50;
		} else {
			MainThreadSema.Signal();
			bFlagSuspended = 1;
			BackgroundThreadSema.Wait();
			SetThreadPriority(-1);
			bFlagSuspended = 0;
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
		bFlagSuspend = (bool)0;
		(&BackgroundThreadSema)->Signal();
	}
}

bool LoadingMenuThread::GetByte49() {
	return LoadingMenu::pLoadingThread && LoadingMenu::pLoadingThread->QIsSuspended();
}

LoadingMenu::LoadingMenu()
	: pTiles{}, SaveBackgroundColor(0.0f, 0.0f, 0.0f, 0.0f), pNextLoadScreen(&LoadScreens), uiNextBackgroundSwap(0),
	  uiFadeInComplete(0), pLocation(0), pSlide1TSP(0), pSlide2TSP(0), uiCurrTextureIndex(0), usCurIndex(0), usFlags(0),
	  m_TileCrossFadeStartTime(0), m_SlideCount(0), m_DelayedTipMatteUpdate(0) {
	for (int32_t j = 0; j < 43; ++j) {
		eIDOrder[j] = MiscStatManager::MS_EQUIPMENT_DAMAGE_REPAIRED;
	}
	NiRenderer::GetRenderer()->GetBackgroundColor(SaveBackgroundColor);
	SetFlag(LoadingMenu::LM_INITLOAD_COMPLETE, !LoadingMenu::bInitialLoad);
	SetFlag(LoadingMenu::LM_FADE_IN, 1);
	pCurrentLoadScreen = pNextLoadScreen->m_item;
	if (pCurrentLoadScreen) {
		TESLoadScreenType* LoadScreenType = pCurrentLoadScreen->pLoadScreenType;
		if (LoadScreenType) {
			CalcLoadScreenTypeTextWindowFaces(&LoadScreenType->data);
		}
	}
}

LoadingMenu::~LoadingMenu() {
	LoadingMenu::SuspendBackgroundThread();
	for (int32_t i = 0; i < 4; ++i) {
		if (spHoldTexture[i].m_pObject) {
			BSTexturePalette::RemoveTexture(spHoldTexture[i].m_pObject);
		}
	}
	Interface::bIsLoadingNewGame = 0;
	NiRenderer::GetRenderer()->SetBackgroundColor(SaveBackgroundColor);
	if (StartMenu::MainMenuIsActive() || (LoadingMenu::bInitialLoad && *sStartingCell.String())) {
		if (BSAudio::QInstance().bAudioEnabled) {
			FalloutAudioMedia::MediaFadeOut(4000, MEDIA_STANDBY);
		}
	}
	if (FalloutAudio::GetNewGameMusic()) {
		FalloutAudio::SetNewGameMusic(0);
		FalloutAudio::SetMainMenuMusic(0);
		FalloutAudioMedia::MediaFadeOut(4000, MEDIA_STANDBY);
	}
	if (GetFlag(LoadingMenu::LM_AUDIO_THREADING)) {
		if (BSAudio::QInstance().bMultiThread) {
			BSAudio::QInstance().SetMultiThreaded(0);
		}
	}
	BSAudio::QInstance().UnMute(1000u);
	LoadingMenu::bInitialLoad = 0;
	FlushLoadScreenList();
	SlideSound.Stop();
	SlideSound.Release();
	InitialSlideSound.Release();
	pLocation = 0;
	LoadingMenu::pMe = 0;
	if (pPlayer) {
		if (pPlayer->patch04DebugQuest) {
			pPlayer->patch04DebugQuest->SetEnabled(1);
			pPlayer->patch04DebugQuest = 0;
		}
	}
}

void LoadingMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (apTile && aiID < 38 && aiID >= 0)
		this->pTiles[aiID] = apTile;
}

int32_t LoadingMenu::GetClass() {
	return 1007;
}

bool LoadingMenu::GetFlag(Flag aeFlag) {
	return (this->usFlags & (1 << aeFlag)) != 0;
}

void LoadingMenu::SetFlag(Flag aeFlag, uint8_t abOn) {
	if (abOn)
		this->usFlags |= 1 << aeFlag;
	else
		this->usFlags &= ~(1 << aeFlag);
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
	while (position)
		this->StatsTilesList.GetNext(position)->Set(0xFA3 /* kTileValue_Visible */, bVisible);
}

void LoadingMenu::UpdateObjectiveTilesVisibilities(bool bVisible) {
	NiTListItem<Tile*>* position = this->ObjectiveTilesList.m_pkHead;
	while (position)
		this->ObjectiveTilesList.GetNext(position)->Set(0xFA3 /* kTileValue_Visible */, bVisible);
}

void LoadingMenu::UpdateXPProgressTilesVisibilities(bool bVisible) {
	NiTListItem<Tile*>* position = this->XPProgressTilesList.m_pkHead;
	while (position)
		this->XPProgressTilesList.GetNext(position)->Set(0xFA3 /* kTileValue_Visible */, bVisible);
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
		Tile* pTile_2 = TipTilesList02.GetNext(piter);
		if (pTile_2) {
			pTile_2->Set(4003, static_cast<int32_t>(bVisible));
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
	LoadingMenu::bDoTileUpdate = 1;
	if ((m_DelayedTipMatteUpdate & 1) != 0) {
		m_DelayedTipMatteUpdate &= ~1u;
		float v12 = pTiles[34]->GetFloat(4002) - 15.0;
		pTiles[36]->Set(4002, v12, 1);
		float v11 = pTiles[34]->GetFloat(4016) + 30.0;
		pTiles[36]->Set(4016, v11, 1);
	}
	if ((m_DelayedTipMatteUpdate & 2) != 0) {
		m_DelayedTipMatteUpdate &= ~2u;
		float v10 = pTiles[35]->GetFloat(4002) - 15.0;
		pTiles[37]->Set(4002, v10, 1);
		float v9 = pTiles[35]->GetFloat(4016) + 30.0;
		pTiles[37]->Set(4016, v9, 1);
	}
	uint32_t CurrTime = BSGetTickCount();
	if (!GetFlag(LoadingMenu::LM_FADE_IN)) {
		uint32_t CrossTime = CurrTime - m_TileCrossFadeStartTime;
		fTileAlpha = static_cast<double>(CrossTime) * 0.00044999999227002263;
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
		float fSlide02Alpha_2 = fSlide02Alpha;
		TESLoadScreen** v1 = &pNextLoadScreen->m_item;
		UpdatePlayerTileAlphas(*v1, fSlide02Alpha_2, 2);
		LoadingMenu::bDoTileUpdate = 1;
	}
	if (CurrTime - m_TileCrossFadeStartTime >= 3000) {
		if (uiNextBackgroundSwap) {
			if (GetFlag(LoadingMenu::LM_FADE_IN)) {
				FadeIn();
			} else {
				if (!GetFlag(LoadingMenu::LM_WHEEL_ONLY_MODE) && BSGetTickCount() >= uiNextBackgroundSwap) {
					Tile3D* p3dtile = static_cast<Tile3D*>(pTiles[0]);
					if (p3dtile->IsPlaying()) {
						if (!LoadingMenu::bInitialLoad && pPlayCredits->fValue == 0.0 &&
							!StartMenu::MainMenuIsActive() &&
							(!GetFlag(LoadingMenu::LM_RESET_MENU) || pTiles[34]->IsTrue(4003))) {
							SetScreenInfo(0);
						}
						AdvanceScreen();
						SetNextSlide(0);
						p3dtile->pAniManager->DeactivateSequence(p3dtile->pCurrentSequence, 0.0f);
						NextSwap();
						if (LoadingMenu::bInitialLoad && pNextLoadScreen == &LoadScreens) {
							SetFlag(LoadingMenu::LM_INITLOAD_COMPLETE, 1);
						}
					} else {
						if ((!LoadingMenu::bInitialLoad || !GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE)) &&
							!GetFlag(LoadingMenu::LM_RESET_MENU) && !GetFlag(LoadingMenu::LM_STATIC_MENU)) {
							bool v5;
							uiNextBackgroundSwap = BSGetTickCount();
							ToggleFlag(LoadingMenu::LM_FORWARD_ANIM);
							if (GetFlag(LoadingMenu::LM_FORWARD_ANIM)) {
								v5 = p3dtile->PlaySequence("Forward");
							} else {
								v5 = p3dtile->PlaySequence("Backward");
							}
							if (v5) {
								m_TileCrossFadeStartTime = BSGetTickCount();
								++m_SlideCount;
								if (GetFlag(LoadingMenu::LM_NEW_LOADSCREEN_LIST)) {
									SetFlag(LoadingMenu::LM_STATIC_MENU, LoadScreens.ItemsInList() == 1);
									SetFlag(LoadingMenu::LM_NEW_LOADSCREEN_LIST, 0);
								}
								FNVD_CONSTRUCTED_LOCAL(NiUpdateData, data, nullptr, 0.0f, 1, 0);
								p3dtile->GetModel()->Update(data);
							} else {
								const char* afMax;
								if (GetFlag(LoadingMenu::LM_FORWARD_ANIM)) {
									afMax = "Forward";
								} else {
									afMax = "Backward";
								}
								Warning("MENUS: (Loading Menu) Unable to play sequence %s", afMax);
							}
						}
					}
				}
			}
		} else {
			if (LoadingMenu::bInitialLoad) {
				while (!TES::pInstance) {
					BSSleep(5u);
				}
				SetNextSlide(pSlide1TSP);
			}
			if (uiFadeInComplete || !GetFlag(LoadingMenu::LM_FADE_IN)) {
				uiFadeInComplete = BSGetTickCount();
			} else {
				FadeIn();
			}
			uiNextBackgroundSwap = uiFadeInComplete;
		}
	} else {
		const char* v2;
		if (!LoadingMenu::bInitialLoad ||
			(GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE) ||
				(pMoviePlayer->GetPlayingSequence() ||
					((v2 = BGSMoviePlayer::sMainMenuMovieIntro.String(), !BSstrlen(v2)) ||
						bMainMenuMovieIntroStarted)))) {
			if (LoadingMenu::bInitialLoad && !GetFlag(LoadingMenu::LM_INITLOAD_COMPLETE) &&
				!pMoviePlayer->GetPlayingSequence() && !bMainMenuMovieIntroStarted) {
				const char* v4 = BGSMoviePlayer::sMainMenuMovieIntro.String();
				if (!BSstrlen(v4)) {
					char cfilename_2[64];
					ourMemset(cfilename_2, 0, 64u);
					BSsprintf(cfilename_2, 64, "Data\\Music\\Special\\%s.mp3", gs_sTitleMusic_Loading.String());
					FalloutAudio::SetMainMenuMusic(1);
					FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, cfilename_2, 0, 1, 1, 0.0f, 0);
					FalloutAudioMedia::MediaMenuUnPause();
					FalloutAudioMedia::fMediaAVolume = 0.6f;
					FalloutAudioMedia::fMediaBVolume = 0.6f;
					bMainMenuMovieIntroStarted = 1;
				}
			}
		} else {
			char cfilename[64];
			ourMemset(cfilename, 0, 64u);
			BSsprintf(cfilename, 64, "Data\\Music\\Special\\%s.mp3", gs_sTitleMusic_Loading.String());
			FalloutAudio::SetMainMenuMusic(1);
			FalloutAudioMedia::MediaOpen(MEDIA_MAINMENU, cfilename, 0, 1, 1, 0.0f, 0);
			FalloutAudioMedia::MediaMenuUnPause();
			FalloutAudioMedia::fMediaAVolume = 0.6f;
			FalloutAudioMedia::fMediaBVolume = 0.6f;
			pTiles[0]->GetModel()->SetAppCulled(1);
			pTiles[32]->Set(4009, 0.0f, 1);
			pTiles[33]->Set(4009, 255.0f, 1);
			const char* v3 = BGSMoviePlayer::sMainMenuMovieIntro.String();
			pMoviePlayer->StartSequence(v3, 0, -1, 0, 0, 0.0f);
			bMainMenuMovieIntroStarted = 1;
			pTiles[11]->Set(4003, 1);
			LoadingMenu::bDoTileUpdate = 1;
		}
	}
	if (pTiles[11]->IsVisible()) {
		ResetLoadingPinwheelAnimation();
	}
	BSAudio* v6 = &BSAudio::QInstance();
	v6->Update(1);
	FalloutAudioMedia::Update();
}

// The initial load (outside the main menu) shows the welcome screens, skipping the first three when the main menu
// has an intro movie; other loads pick from the candidate screens, retried without the location when it yields too
// few. An empty list gets the first welcome screen. The exe calls BSSimpleList's IsEmpty (0x8256D0), AddTail
// (0x905820), ItemsInList (0x5AE380), AddHead (0x5AE3D0) and GetItem (0x6815C0) out of line; src's template copies
// have the same effects.
void LoadingMenu::MakeLoadScreenList() {
	bool bCleared = false;
	if (!this->LoadScreens.IsEmpty()) {
		this->FlushLoadScreenList();
		bCleared = true;
	}
	this->SetFlag(LM_USE_MAINMENU_SCREENS, StartMenu::MainMenuIsActive() || appMain->GameReset());
	if (LoadingMenu::bInitialLoad && !StartMenu::MainMenuIsActive()) {
		WelcomeScreenID eID = LM_INDIAN_HEAD;
		if (BSstrlen(BGSMoviePlayer::sMainMenuMovieIntro.String()))
			eID = LM_LEGAL;
		for (; eID < LM_NUM_WELCOME_SCREENS; eID++) {
			const char* pFolder = eID == LM_MAIN_BACKGROUND ? "Main" : "Loading";
			this->strBuffer.SPrintF("Interface\\%s\\%s.dds", pFolder, LoadingMenu::pWelcomeScreensA[eID].String());
			void* pMemory = ourMalloc(sizeof(TESLoadScreen));
			TESLoadScreen* pLoadScreen = pMemory ? ::new (pMemory) TESLoadScreen : nullptr;
			pLoadScreen->SetTextureName(this->strBuffer.String());
			pLoadScreen->SetTemporary();
			this->LoadScreens.AddTail(pLoadScreen);
		}
	} else {
		alignas(BSSimpleArray<TESLoadScreen*>) uint8_t aCandidatesStorage[sizeof(BSSimpleArray<TESLoadScreen*>)];
		BSSimpleArray<TESLoadScreen*>* pCandidates = ::new (aCandidatesStorage) BSSimpleArray<TESLoadScreen*>;
		this->BuildCandidateList(*pCandidates);
		this->AddCandidates(*pCandidates);
		// The exe compares the count unsigned (jae).
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
		void* pMemory = ourMalloc(sizeof(TESLoadScreen));
		TESLoadScreen* pLoadScreen = pMemory ? ::new (pMemory) TESLoadScreen : nullptr;
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

// pTiles[0] is read before the tests. The exe calls BSSimpleList's GetNext (0x726070) out of line, twice per step.
bool LoadingMenu::ForceFinish() {
	Tile3D* pBackground = static_cast<Tile3D*>(this->pTiles[0]);
	if (!LoadingMenu::bInitialLoad || this->GetFlag(LM_INITLOAD_COMPLETE) || pBackground->IsPlaying())
		return false;
	while (this->pNextLoadScreen->GetNext())
		this->pNextLoadScreen = this->pNextLoadScreen->GetNext();
	this->SetNextSlide(nullptr);
	this->uiNextBackgroundSwap = BSGetTickCount();
	return true;
}

// AddTail takes its item by reference: the exe pushes the address of each tile slot.
void LoadingMenu::InitXPProgressTilesList() {
	this->XPProgressTilesList.RemoveAll();
	this->XPProgressTilesList.AddTail(this->pTiles[16]);
	this->XPProgressTilesList.AddTail(this->pTiles[28]);
	this->XPProgressTilesList.AddTail(this->pTiles[29]);
	this->XPProgressTilesList.AddTail(this->pTiles[15]);
	this->XPProgressTilesList.AddTail(this->pTiles[10]);
	this->XPProgressTilesList.AddTail(this->pTiles[30]);
}

// The exe calls the out-of-line AutoMemContext constructor and destructor on a stack context (with this file's path
// and line 0x423) and ignores the constructor's result; the texture pointer is released before the context. It calls
// NiPointer's constructor, assignment and destructor, and BSSimpleList's GetItem out of line; src's inline copies
// have the same effects. The index of spHoldTexture is read, then read again for the increment and for the unsigned
// wrap test.
void LoadingMenu::SetNextSlide(TileShaderProperty* apTSP) {
	alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
	AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
	::new (aucContext) AutoMemContext(MEM_CONTEXT_INTERFACE, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\Menus\\LoadingMenu.cpp", 1059);
	{
		this->strBuffer.SPrintF("%s\\%s", "Textures", this->pNextLoadScreen->GetItem()->GetTextureName());
		NiPointer<NiSourceTexture> spTexture(nullptr);
		TES::pInstance->CreateTextureImage(this->strBuffer.String(), spTexture, false, false);
		NiPointer<NiTexture>* pspHoldTexture = &this->spHoldTexture[this->uiCurrTextureIndex];
		++this->uiCurrTextureIndex;
		if (this->uiCurrTextureIndex >= 4)
			this->uiCurrTextureIndex = 0;
		if (pspHoldTexture->operator->())
			BSTexturePalette::RemoveTexture(pspHoldTexture->operator->());
		NiSourceTexture* pTexture = spTexture.operator->();
		// The exe calls NiPointer's operator= out of line (0x66B0D0).
		*pspHoldTexture = pTexture;
		// Odd slides go to the second slide tile.
		if (this->m_SlideCount % 2)
			apTSP = this->pTiles[33]->GetShaderProperty();
		else
			apTSP = this->pTiles[32]->GetShaderProperty();
		if (apTSP)
			apTSP->SetTileTexture(spTexture.operator->());
	}
	pContext->~AutoMemContext();
}

// `delete` is the deleting destructor with flag 1 after a null test.
void LoadingMenu::FlushLoadScreenList() {
	for (BSSimpleList<TESLoadScreen*>* pNode = &this->LoadScreens; pNode && pNode->GetItem();
		pNode = pNode->GetNext()) {
		if (pNode->GetItem()->GetTemporary())
			delete pNode->GetItem();
	}
	this->LoadScreens.RemoveAll();
	this->pNextLoadScreen = &this->LoadScreens;
}

// The result is kept in a byte that each comparison overwrites. The exe calls BSSimpleList's GetItem (0x6815C0)
// and GetNext (0x726070) out of line.
bool LoadingMenu::OK2Add(TESLoadScreen* apLoadScreen) {
	bool bOK = apLoadScreen && apLoadScreen->GetFormType() == LSCR_ID;
	for (BSSimpleList<TESLoadScreen*>* pNode = &this->LoadScreens; bOK && pNode && pNode->GetItem();
		pNode = pNode->GetNext()) {
		bOK = BSstrcmp(static_cast<TESLoadScreen*>(apLoadScreen)->GetTextureName(),
				  pNode->GetItem()->GetTextureName()) != 0;
	}
	return bOK;
}

// eBackground also keeps the slide tiles. Each visibility goes through Tile::Set's float form.
void LoadingMenu::HideAllBut(TileIndex aeTileIndex, bool abShowPlayerTiles) {
	for (int32_t i = 0; i < NUM_TILES; ++i) {
		const int32_t iVisible =
			i == aeTileIndex || (aeTileIndex == eBackground && (i == eTileSlide01 || i == eTileSlide02));
		this->pTiles[i]->Set(Tile::kTileValue_Visible, static_cast<float>(iVisible), false);
	}
	this->SetFlag(LM_SHOW_PLAYER_TILES, abShowPlayerTiles);
}

void LoadingMenu::FadeIn3D(NiAVObject* apObject, float afPct) {
	if (!apObject)
		return;
	NiNode* pNode = apObject->IsNode();
	// The exe compares unsigned (jae).
	for (uint32_t i = 0; pNode && i < static_cast<uint32_t>(pNode->GetArrayCount()); ++i)
		this->FadeIn3D(pNode->GetAt(i), afPct);
	if (apObject->IsGeometry()) {
		NiProperty* pProperty = apObject->GetProperty(NiShadeProperty::GetType());
		if (pProperty)
			static_cast<TileShaderProperty*>(pProperty)->SetTileAlpha(afPct);
	}
}

void LoadingMenu::GetPlayerTileDefaultSettings() {
	for (int32_t i = 0; i < 38; ++i) {
		int32_t Float = 0;
		int32_t v2 = 0;
		int32_t v3 = 0;
		int32_t v4 = 0;
		FONTS v5 = FONT_2;
		uint8_t v7 = 4;
		if (pTiles[i]) {
			Float = static_cast<int32_t>(pTiles[i]->GetFloat(4001));
			v2 = static_cast<int32_t>(pTiles[i]->GetFloat(4002));
			v3 = static_cast<int32_t>(pTiles[i]->GetFloat(4016));
			v4 = static_cast<int32_t>(pTiles[i]->GetFloat(4017));
			v5 = static_cast<FONTS>(static_cast<int32_t>(pTiles[i]->GetFloat(4025)));
			v7 = static_cast<int32_t>(pTiles[i]->GetFloat(4023));
		}
		DefaultTileSettings[i].nOriginX = Float;
		DefaultTileSettings[i].nOriginY = v2;
		DefaultTileSettings[i].nHeight = v3;
		DefaultTileSettings[i].nWidth = v4;
		DefaultTileSettings[i].eFontType = v5;
		DefaultTileSettings[i].eJustification = v7;
	}
}

// The stacking type is read twice, each time through Tile::GetFloat. Interface mode 3 is kept, anything else
// becomes 2. Tile 0 is the slide background, tile 11 the pinwheel. When Slide01 is a node, the exe takes Slide02's
// first child without testing that Slide02 is a node too. The renderer's lock calls are NiDX9Renderer's. The
// background tile is loaded once for PlaySequence and the update.
void LoadingMenu::_Create(TESForm* apLocation) {
	this->uiNextBackgroundSwap = 0;
	if (this->pMenu->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
		this->pMenu->GetFloat(Tile::kTileValue_StackingType) == 102.0)
		this->pMenu->Set(Tile::kTileValue_Depth, Tile::GetMaximumDepth(), true);
	if (InterfaceManager::Instance()->GetCurrentMode() != 3)
		InterfaceManager::Instance()->InternalSetMenuMode(InterfaceManager::eMenuModeOn);
	HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_LOAD);
	HUDMainMenu::SetInfoForRef(nullptr, false, false);
	HUDMainMenu::HandleHealthbarForCrosshairActor_(nullptr);
	if (pPlayer && !pMoviePlayer->GetPlayingSequence())
		pMoviePlayer->Stop();
	TileMenu* pSleepWaitMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::eSleepWaitMenu));
	if (pSleepWaitMenu)
		delete pSleepWaitMenu;
	this->pLocation = apLocation;

	if (LoadingMenu::bInitialLoad || pPlayCredits->GetValue() != 0.0) {
		this->HideAllBut(eBackground, false);
	} else if (Interface::bIsLoadingNewGame || appMain->GameReset()) {
		this->ShowPlayerTiles(false);
		static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->LockRenderer();
		this->ResetLoadingPinwheelAnimation();
		static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
	} else {
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
			default:
				break;
			}
		}
		if (this->GetFlag(LM_WHEEL_ONLY_MODE)) {
			this->HideAllBut(ePinwheel, false);
			this->SetFlag(LM_FADE_IN, 0);
			this->uiFadeInComplete = BSGetTickCount();
			LoadingMenu::bDoTileUpdate = true;
		} else {
			this->ShowPlayerTiles(true);
			this->PrepPlayerTiles();
		}
		static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->LockRenderer();
		this->ResetLoadingPinwheelAnimation();
		static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
	}
	NiRenderer::GetRenderer()->SetBackgroundColor(NiColorA_BLACK);

	// The scale is read before the model.
	if (this->pTiles[ePinwheel]->GetModel()) {
		const float fScale = fLoadingWheelScale.operator float();
		this->pTiles[ePinwheel]->GetModel()->SetScale(fScale);
		this->pTiles[ePinwheel]->Set(Tile::kTileValue_User0, fLoadingWheelScale.Float(), true);
		this->pTiles[ePinwheel]->GetModel()->SetRotate(1.5707964f, 1.0f, 0.0f, 0.0f);
	} else {
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
		} else {
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
		NiUpdateData kUpdateData;
		kUpdateData.~NiUpdateData();
		::new (&kUpdateData) NiUpdateData(0.0f, true, false);
		pBackground->GetModel()->Update(kUpdateData);
		this->FadeIn();
		this->uiFadeInComplete = 0;
	}
	LoadingMenu::ShowChanges();

	// Audio runs on its own thread for the initial load and loads without a location (unless a starting cell is
	// set); other loads also fade the 3D sounds out.
	if ((LoadingMenu::bInitialLoad || !apLocation) && BSAudio::QInstance().IsAudioEnabled() &&
		!*sStartingCell.String()) {
		if (!BSAudio::QInstance().IsMultiThreaded()) {
			BSAudio::QInstance().SetMultiThreaded(true);
			this->SetFlag(LM_AUDIO_THREADING, 1);
		}
	} else if (!LoadingMenu::bInitialLoad && BSAudio::QInstance().IsAudioEnabled()) {
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
			int32_t ClassStatic = StartMenu::GetClassStatic();
			Tile* MenuByClass = Menu::GetMenuByClass(ClassStatic);
			StartMenu* pstartMenu = static_cast<StartMenu*>(MenuByClass->GetMenu());
			if (pstartMenu) {
				if (!pstartMenu->IsFlagSet(StartMenu::FLAG_INGAME)) {
					delete pstartMenu;
				}
			}
		}
		if (appMain->GameReset()) {
			ShowPlayerTiles(0);
		} else {
			ShowPlayerTiles(1);
			PrepPlayerTiles();
		}
		pLocation = apLocation;
		MakeLoadScreenList();
		pLocation = apLocation;
		uiNextBackgroundSwap = BSGetTickCount();
		++m_SlideCount;
		SetScreenInfo(0);
		AdvanceScreen();
		SetNextSlide(0);
		NextSwap();
		++m_SlideCount;
		SetScreenInfo(0);
		AdvanceScreen();
		SetNextSlide(0);
		NextSwap();
		SetFlag(LoadingMenu::LM_RESET_MENU, 0);
		NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
		Renderer_->LockRenderer();
		if (!pTiles[11]->IsVisible()) {
			pTiles[11]->Set(4003, 1);
			ResetLoadingPinwheelAnimation();
		}
		InterfaceManager* v5 = InterfaceManager::Instance();
		v5->pCursor->Set(4003, 0);
		Tile::UpdateAll(1);
		NiDX9Renderer* v6 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
		v6->UnlockRenderer();
		LoadingMenu::bDoTileUpdate = 1;
		LoadingMenu::ResumeBackgroundThread();
	}
}

void LoadingMenu::ShowPlayerTiles(bool abShow) {
	SetFlag(LoadingMenu::LM_SHOW_PLAYER_TILES, abShow);
}

void LoadingMenu::PrepPlayerTiles() {
	const char* Src;
	if (!pSaveLoadGame || !pSaveLoadGame->GetSaveGameLoading()) {
		LoadingMenu::FillDisplayData();
	}
	pTiles[10]->Set(Tile::kTileValue_User0, LoadingMenu::DisplayData.fLevelProgress, 1);
	// The exe calls the unsigned overload (0x715C60) with the signed level.
	pTiles[10]->Set(Tile::kTileValue_User1, static_cast<uint32_t>(LoadingMenu::DisplayData.iLevel));
	if (LoadingMenu::DisplayData.pCurrentObjective) {
		Src = LoadingMenu::DisplayData.pCurrentObjective->GetDisplayText();
	} else {
		Src = sNone.String();
	}
	pTiles[3]->Set(4036, Src, 1);
	const char* afValue = sCurrentObjective.String();
	pTiles[14]->Set(4036, afValue, 1);
	const char* v1 = sLevelProgress.String();
	pTiles[15]->Set(4036, v1, 1);
}

void LoadingMenu::SetScreenInfo(bool abRefreshCurrent) {
	// The exe hashes the texture name and reduces it modulo 100000, then never uses the result.
	uint32_t uiHash = 0;
	for (const char* pnext = pNextLoadScreen->m_item->GetTextureName(); *pnext; ++pnext) {
		uiHash = std::tolower(*pnext) + 33 * uiHash;
	}
	uiHash %= 100000;
	if (m_SlideCount % 2) {
		m_DelayedTipMatteUpdate |= 2;
		TESDescription* v5 = static_cast<TESDescription*>(pNextLoadScreen->m_item);
		const char* v3 = v5->GetDescription(0, DESC_ID);
		pTiles[35]->Set(4036, v3, 1);
	} else {
		m_DelayedTipMatteUpdate |= 1;
		TESDescription* v6 = static_cast<TESDescription*>(pNextLoadScreen->m_item);
		const char* v2 = v6->GetDescription(0, DESC_ID);
		pTiles[34]->Set(4036, v2, 1);
	}
	const char* v4 = sVDSGPlate.String();
	pTiles[13]->Set(4036, v4, 1);
	LoadingMenu::bDoTileUpdate = 1;
}

void LoadingMenu::AdvanceScreen() {
	pCurrentLoadScreen = pNextLoadScreen->m_item;
	pNextLoadScreen = pNextLoadScreen->m_pkNext;
	if (!pNextLoadScreen) {
		pNextLoadScreen = &LoadScreens;
	}
	if (pNextLoadScreen) {
		if (pNextLoadScreen->m_item) {
			UpdatePlayerTileSettings(pCurrentLoadScreen);
		}
	}
}

void LoadingMenu::FadeIn() {
	NiRenderedTexture* menuBGTexture;
	float afTime;
	static float g_loadingMenuMenuFade = (float)(pMenu->GetFloat(4033) * 1000.0);
	if (!uiFadeInComplete) {
		if (appMain->GetMenuBGTexture() || LoadingMenu::bInitialLoad) {
			menuBGTexture = appMain->GetMenuBGTexture();
			static_cast<TileImage*>(pTiles[12])->SetSourceTexture(menuBGTexture);
			uiFadeInComplete = (uint32_t)((int64_t)(((double)BSGetTickCount()) + g_loadingMenuMenuFade));
		} else {
			uiFadeInComplete = BSGetTickCount();
		}
	}
	if (BSGetTickCount() >= uiFadeInComplete) {
		FadeIn(pMenu, (float)1.0);
		SetFlag(LoadingMenu::LM_FADE_IN, 0);
		++m_SlideCount;
	} else {
		afTime = (float)(1.0 - (((double)(uiFadeInComplete - BSGetTickCount())) / g_loadingMenuMenuFade));
		FadeIn(pMenu, afTime);
	}
}

void LoadingMenu::FadeIn(Tile* apTile, float afTime) {
	if (apTile) {
		if (apTile != pTiles[33]) {
			float afAlphaa = BSMIN(afTime, 1.0f);
			float afAlpha = BSMAX(afAlphaa, 0.0f);
			if (apTile->GetType() == 904) {
				FadeIn3D(apTile->GetModel(), afAlpha);
			} else {
				if (apTile == pTiles[12]) {
					afAlpha = 1.0 - afAlpha;
				}
				float v3 = afAlpha * 255.0;
				apTile->Set(4009, v3, 1);
				LoadingMenu::bDoTileUpdate = 1;
				for (NiTListItem<Tile*>* piter = apTile->xChildren.m_pkHead; piter;
					FadeIn(apTile->xChildren.GetNext(piter), afAlpha)) {
					;
				}
			}
		}
	}
}

// The exe calls BSSimpleList's GetItem (0x6815C0) out of line, and the load screen type getter through the body
// it shares with TES::GetInterior.
void LoadingMenu::UpdatePlayerTileSettings(TESLoadScreen* pLoadScreen) {
	if (!this->GetFlag(LM_SHOW_PLAYER_TILES))
		return;
	if (!pLoadScreen) {
		if (this->pNextLoadScreen)
			pLoadScreen = this->pNextLoadScreen->GetItem();
		else
			pLoadScreen = this->LoadScreens.GetItem();
		if (!pLoadScreen)
			return;
	}
	if (pLoadScreen->GetLoadScreenType()) {
		LoadScreenType_Data* pData = pLoadScreen->GetLoadScreenTypeData();
		this->CalcLoadScreenTypeTextWindowFaces(pData);
		switch (pLoadScreen->GetLoadScreenTypeDataType()) {
		case LSDT_XP_PROGRESS:
			this->InitXPProgressTilesSettings(pData);
			break;
		case LSDT_OBJECTIVE:
			this->InitObjectiveTilesSettings(pData);
			break;
		case LSDT_TIP:
			this->InitTipTilesSettings(pData);
			break;
		case LSDT_STATS:
			this->InitStatsTilesSettings(pData);
			break;
		default:
			break;
		}
	}
	bUpdatePlayerTilePositions = false;
}

void LoadingMenu::CalcLoadScreenTypeTextWindowFaces(const LoadScreenType_Data* apData) {
	if (apData) {
		float fCurrentScreenWidth = InterfaceManager::GetScreenWidth();
		float fCurrentScreenHeight = InterfaceManager::GetScreenHeight();
		float fSafeWidth = fCurrentScreenWidth - InterfaceManager::GetScreenSafeX();
		float fSafeHeight = fCurrentScreenHeight - InterfaceManager::GetScreenSafeY();
		float fConvertedXPos =
			static_cast<double>(apData->DisplayData1.uDisplayPositionX) / 1280.0 * fCurrentScreenWidth;
		float fConvertedYPos =
			static_cast<double>(apData->DisplayData1.uDisplayPositionY) / 960.0 * fCurrentScreenHeight;
		float fConvertedWidth = static_cast<double>(apData->DisplayData1.uDisplayWidth) / 1280.0 * fCurrentScreenWidth;
		float fConvertedHeight =
			static_cast<double>(apData->DisplayData1.uDisplayHeight) / 1280.0 * fCurrentScreenHeight;
		if (apData->DataType) {
			float a2 = InterfaceManager::GetScreenSafeX();
			iMinX = static_cast<int32_t>(BSMAX(fConvertedXPos, a2));
			float a2a = InterfaceManager::GetScreenSafeY();
			iMinY = static_cast<int32_t>(BSMAX(fConvertedYPos, a2a));
			float v5 = static_cast<double>(iMinX) + fConvertedWidth;
			iMaxX = static_cast<int32_t>(BSMIN(v5, fSafeWidth));
			float v4 = static_cast<double>(iMinY) + fConvertedHeight;
			iMaxY = static_cast<int32_t>(BSMIN(v4, fSafeHeight));
		}
	} else {
		InitializationFlow("WARNING: LoadingMenu::CalcLoadScreenTypeTextWindowVerts() received a NULL apData.");
	}
}

void LoadingMenu::InitStatsTilesSettings(LoadScreenType_Data* apData) {
	pTiles[31]->Set(4001, static_cast<float>(iMinX), 1);
	pTiles[31]->Set(4017, static_cast<float>(iMaxX - iMinX), 1);
	pTiles[31]->Set(4002, static_cast<float>(iMinY), 1);
	pTiles[31]->Set(4016, static_cast<float>(iMaxY - iMinY), 1);
	pTiles[17]->Set(4025, apData->DisplayData1.eDisplayFont);
	pTiles[17]->Set(4018, apData->DisplayData1.TextColorR, 1);
	pTiles[17]->Set(4019, apData->DisplayData1.TextColorG, 1);
	pTiles[17]->Set(4020, apData->DisplayData1.TextColorB, 1);
	pTiles[18]->Set(4025, apData->DisplayData2.eDisplayFont);
	pTiles[17]->Set(4018, apData->DisplayData2.TextColorR, 1);
	pTiles[17]->Set(4019, apData->DisplayData2.TextColorG, 1);
	pTiles[17]->Set(4020, apData->DisplayData2.TextColorB, 1);
	float fStatRowHeight = pTiles[31]->GetFloat(4016) / static_cast<double>(apData->uNumStatsToDisplay);
	float fYOffset = 0.0f;
	NiTListItem<Tile*>* pLabelIter = pTiles[17]->xChildren.m_pkTail;
	NiTListItem<Tile*>* pValIter = pTiles[18]->xChildren.m_pkTail;
	uint32_t nRowCount = 1;
	while (pLabelIter && pValIter) {
		Tile* pLabelChild = pTiles[17]->xChildren.GetPrev(pLabelIter);
		Tile* pValChild = pTiles[18]->xChildren.GetPrev(pValIter);
		if (pLabelChild) {
			if (pValChild) {
				bool bIsVisible = 1;
				if (nRowCount > apData->uNumStatsToDisplay) {
					bIsVisible = 0;
				} else {
					pLabelChild->Set(4002, fYOffset, 1);
					pValChild->Set(4002, fYOffset, 1);
					fYOffset = fYOffset + fStatRowHeight;
					pLabelChild->AddNeedsUpdate(8);
					pValChild->AddNeedsUpdate(8);
					++nRowCount;
				}
				pLabelChild->Set(4003, static_cast<int32_t>(bIsVisible));
				pValChild->Set(4003, static_cast<int32_t>(bIsVisible));
			}
		}
	}
	pTiles[31]->Set(4090, apData->DisplayData1.fDisplayRotationRads, 1);
	pTiles[31]->RecursiveRotationUpdate();
}

void LoadingMenu::InitObjectiveTilesSettings(LoadScreenType_Data* apData) {
	NiPoint3 nip3Dims;
	FontManager* Singleton = FontManager::Instance();
	float fFontHeight = Singleton->pFont[apData->DisplayData1.eDisplayFont]->fFontHeight;
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
				pTile->Set(4001, fX, 1);
				pTile->Set(4017, static_cast<float>(iMaxX - iMinX), 1);
				float afWrapwidtha = pTile->GetFloat(4017);
				pTile->Set(4026, afWrapwidtha, 1);
				pTile->Set(4002, static_cast<float>(iMinY), 1);
				pTile->Set(4025, apData->DisplayData1.eDisplayFont);
				pTile->Set(4023, apData->DisplayData1.uJustification);
				pTile->Set(4018, apData->DisplayData1.TextColorR, 1);
				pTile->Set(4019, apData->DisplayData1.TextColorG, 1);
				pTile->Set(4020, apData->DisplayData1.TextColorB, 1);
				pTile->Set(4085, -1);
				pTile->AddNeedsUpdate(8);
			} else {
				if (pTile == pTiles[3]) {
					float fWrapWidth = static_cast<float>(iMaxX - iMinX);
					int32_t eDisplayFont = apData->DisplayData1.eDisplayFont;
					const char* v4 = sCurrentObjective.String();
					FontManager* v3 = FontManager::Instance();
					v3->CalculateStringDimensions(&nip3Dims, v4, eDisplayFont, fWrapWidth, 0);
					pTile->Set(4002, nip3Dims.y, 1);
				}
			}
		}
	}
	pTiles[27]->Set(4090, apData->DisplayData1.fDisplayRotationRads, 1);
	pTiles[27]->RecursiveRotationUpdate();
}

void LoadingMenu::InitXPProgressTilesSettings(LoadScreenType_Data* apData) {
	NiPoint3 v24;
	NiPoint3 v25;
	NiPoint3 nip3Dims;
	NiTListItem<Tile*>* piter = XPProgressTilesList.m_pkHead;
	while (piter) {
		Tile* pTile = XPProgressTilesList.GetNext(piter);
		if (pTile) {
			if (pTile == pTiles[16]) {
				pTile->Set(4001, static_cast<float>(iMinX), 1);
				double iMaxX = static_cast<double>(this->iMaxX);
				float v21 = iMaxX - pTile->GetFloat(4001);
				pTile->Set(4017, v21, 1);
				float afWrapwidtha = pTile->GetFloat(4017);
				pTile->Set(4026, afWrapwidtha, 1);
				pTile->Set(4002, static_cast<float>(iMinY), 1);
				double iMaxY = static_cast<double>(this->iMaxY);
				float v19 = iMaxY - pTile->GetFloat(4002);
				pTile->Set(4016, v19, 1);
			} else {
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
					pTile->Set(4001, fX, 1);
					pTile->Set(4023, apData->DisplayData1.uJustification);
					pTile->Set(4025, apData->DisplayData1.eDisplayFont);
					pTile->Set(4017, static_cast<float>(this->iMaxX - iMinX), 1);
					float afWrapwidthd = pTile->GetFloat(4017);
					pTile->Set(4026, afWrapwidthd, 1);
					float afWrapwidthe = pTile->GetFloat(4026);
					int32_t eDisplayFont = apData->DisplayData1.eDisplayFont;
					const char* v5 = sLevelProgress.String();
					FontManager* Singleton = FontManager::Instance();
					Singleton->CalculateStringDimensions(&nip3Dims, v5, eDisplayFont, afWrapwidthe, 0);
					float fHeight = nip3Dims.y;
					pTile->Set(4016, nip3Dims.y, 1);
				} else {
					if (pTile == pTiles[10]) {
						char apDest[264];
						char apText[264];
						// The exe converts to a 64-bit integer (fistp qword) and passes its low dword.
						BSsprintf(apDest, 260, "%i", static_cast<int32_t>(static_cast<int64_t>(pTile->GetFloat(4101))));
						float afWrapwidthf = pTile->GetFloat(4026);
						int32_t v7 = apData->DisplayData1.eDisplayFont;
						FontManager* v3 = FontManager::Instance();
						float fCurrLevelWidth = v3->CalculateStringDimensions(&v25, apDest, v7, afWrapwidthf, 0)->x;
						pTile->Set(4001, fCurrLevelWidth, 1);
						// The exe converts to a 64-bit integer (fistp qword) and passes its low dword.
						BSsprintf(apText, 260, "%i", static_cast<int32_t>(static_cast<int64_t>(pTile->GetFloat(4102))));
						float v17 = (static_cast<double>(this->iMaxX - iMinX) - fCurrLevelWidth) / 2.0;
						int32_t v8 = apData->DisplayData1.eDisplayFont;
						FontManager* v4 = FontManager::Instance();
						float fNextLevelWidth = v4->CalculateStringDimensions(&v24, apText, v8, v17, 0)->x;
						float v16 = static_cast<double>(this->iMaxX - iMinX) - (fCurrLevelWidth + fCurrLevelWidth) -
							fNextLevelWidth;
						pTile->Set(4017, v16, 1);
					}
				}
				pTile->Set(4018, apData->DisplayData1.TextColorR, 1);
				pTile->Set(4019, apData->DisplayData1.TextColorG, 1);
				pTile->Set(4020, apData->DisplayData1.TextColorB, 1);
				pTile->Set(4085, -1);
				pTile->AddNeedsUpdate(8);
			}
		}
	}
	pTiles[16]->Set(4090, apData->DisplayData1.fDisplayRotationRads, 1);
	pTiles[16]->RecursiveRotationUpdate();
}

void LoadingMenu::InitTipTilesSettings(LoadScreenType_Data* apData) {
	NiTListItem<Tile*>* piter = TipTilesList.m_pkHead;
	while (piter) {
		Tile* pTile = TipTilesList.GetNext(piter);
		if (pTile) {
			float iMaxX = 0.0f;
			uint32_t uJustification = apData->DisplayData1.uJustification;
			if (uJustification == 1) {
				float iMinX = static_cast<float>(this->iMinX);
				pTile->Set(4001, iMinX, 1);
			} else {
				if (uJustification == 2) {
					float v23 = static_cast<double>(this->iMinX) + static_cast<double>(this->iMaxX - this->iMinX) / 2.0;
					pTile->Set(4001, v23, 1);
				} else {
					if (uJustification == 4) {
						iMaxX = static_cast<float>(this->iMaxX);
					}
					pTile->Set(4001, iMaxX, 1);
				}
			}
			pTile->Set(4017, static_cast<float>(this->iMaxX - this->iMinX), 1);
			float afValuea = pTile->GetFloat(4017);
			pTile->Set(4026, afValuea, 1);
			pTile->Set(4002, static_cast<float>(iMinY), 1);
			double iMaxY = static_cast<double>(this->iMaxY);
			float v13 = iMaxY - pTile->GetFloat(4002);
			pTile->Set(4016, v13, 1);
			float afValuec = pTile->GetFloat(4016);
			pTile->Set(4027, afValuec, 1);
			pTile->Set(4090, apData->DisplayData1.fDisplayRotationRads, 1);
			pTile->Set(4025, apData->DisplayData1.eDisplayFont);
			pTile->Set(4023, apData->DisplayData1.uJustification);
			pTile->Set(4018, apData->DisplayData1.TextColorR, 1);
			pTile->Set(4019, apData->DisplayData1.TextColorG, 1);
			pTile->Set(4020, apData->DisplayData1.TextColorB, 1);
			pTile->Set(4085, -1);
			pTile->AddNeedsUpdate(8);
		}
	}
	piter = TipTilesList02.m_pkHead;
	while (piter) {
		Tile* pTile_2 = TipTilesList02.GetNext(piter);
		if (pTile_2) {
			float v17 = 0.0f;
			uint32_t v12 = apData->DisplayData1.uJustification;
			if (v12 == 1) {
				float v18 = static_cast<float>(this->iMinX);
				pTile_2->Set(4001, v18, 1);
			} else {
				if (v12 == 2) {
					float v19 = static_cast<double>(this->iMinX) + static_cast<double>(this->iMaxX - this->iMinX) / 2.0;
					pTile_2->Set(4001, v19, 1);
				} else {
					if (v12 == 4) {
						v17 = static_cast<float>(this->iMaxX);
					}
					pTile_2->Set(4001, v17, 1);
				}
			}
			pTile_2->Set(4017, static_cast<float>(this->iMaxX - this->iMinX), 1);
			float afValuee = pTile_2->GetFloat(4017);
			pTile_2->Set(4026, afValuee, 1);
			pTile_2->Set(4002, static_cast<float>(iMinY), 1);
			double v11 = static_cast<double>(this->iMaxY);
			float v10 = v11 - pTile_2->GetFloat(4002);
			pTile_2->Set(4016, v10, 1);
			float afValueg = pTile_2->GetFloat(4016);
			pTile_2->Set(4027, afValueg, 1);
			pTile_2->Set(4090, apData->DisplayData1.fDisplayRotationRads, 1);
			pTile_2->Set(4025, apData->DisplayData1.eDisplayFont);
			pTile_2->Set(4023, apData->DisplayData1.uJustification);
			pTile_2->Set(4018, apData->DisplayData1.TextColorR, 1);
			pTile_2->Set(4019, apData->DisplayData1.TextColorG, 1);
			pTile_2->Set(4020, apData->DisplayData1.TextColorB, 1);
			pTile_2->Set(4085, -1);
			pTile_2->AddNeedsUpdate(8);
		}
	}
}

void LoadingMenu::BuildCandidateList(BSSimpleArray<TESLoadScreen*, 1024>& aListOut) {
	aListOut.Clear(0);
	for (BSSimpleList<TESLoadScreen*>* pnode = pDataHandler->GetLoadScreenList(); pnode && pnode->m_item;
		pnode = pnode->m_pkNext) {
		TESLoadScreen** v2;
		if (!GetFlag(LoadingMenu::LM_USE_MAINMENU_SCREENS) || (v2 = &pnode->m_item, !(*v2)->GetDisplaysOnMainMenu())) {
			TESLoadScreen** v3;
			TESObjectCELL* pLocation;
			if (GetFlag(LoadingMenu::LM_USE_MAINMENU_SCREENS)) {
				continue;
			}
			if (!this->pLocation ||
				(pLocation = static_cast<TESObjectCELL*>(this->pLocation),
					(v3 = &pnode->m_item, !(*v3)->IsValidForCell(pLocation)))) {
				if (this->pLocation) {
					continue;
				}
				TESLoadScreen** v4 = &pnode->m_item;
				if (!(*v4)->IsNonLocationSpecific()) {
					continue;
				}
			}
		}
		uint32_t nIndex = aListOut.Add(pnode->m_item);
		TESLoadScreen** v5 = &pnode->m_item;
		LOAD_SCREEN_DATA_TYPE eCurrentType = (*v5)->GetLoadScreenTypeDataType();
		if ((Interface::bIsLoadingNewGame || appMain->GameReset()) && eCurrentType != LSDT_TIP) {
			if (eCurrentType) {
				aListOut.RemoveFast(nIndex, 1u);
			}
		}
	}
}

// The limit is iMaxScreens, or iMaxScreens_MainMenu for main-menu screens; a load into a location that may show
// generic screens caps it at iNumLocationSpecificScreens. The exe counts the list once and then counts the screens it
// adds, and removes each drawn candidate from the array whether it was added or not.
void LoadingMenu::AddCandidates(BSSimpleArray<TESLoadScreen*, 1024>& aCandidateList) {
	uint32_t uiMaxScreens = LoadingMenu::iMaxScreens.UInt();
	if (this->GetFlag(LM_USE_MAINMENU_SCREENS)) {
		uiMaxScreens = LoadingMenu::iMaxScreens_MainMenu.UInt();
	} else if (this->pLocation && !TESLoadScreen::QUseLocationSpecificScreensOnly()) {
		uiMaxScreens = BSMIN(uiMaxScreens, static_cast<uint32_t>(LoadingMenu::iNumLocationSpecificScreens.UInt()));
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
	float finterval;
	if (StartMenu::MainMenuIsActive()) {
		finterval = gs_fMainMenuBkgdUpdateInterval_Loading.operator float();
	} else {
		if (LoadingMenu::bInitialLoad) {
			finterval = gs_fLoadingInitUpdateInterval_Loading.operator float();
		} else {
			finterval = gs_fLoadingBkgdUpdateInterval_Loading.operator float();
		}
	}
	uiNextBackgroundSwap = static_cast<int64_t>(finterval * 1000.0) + BSGetTickCount();
}

void LoadingMenu::InitPlayerTileSettings() {
	UpdatePlayerTileSettings(pCurrentLoadScreen);
}

void LoadingMenu::UpdatePlayerTileVisibilities(TESLoadScreen* pLoadScreen, bool bVisible) {
	if (GetFlag(LoadingMenu::LM_SHOW_PLAYER_TILES)) {
		if (pLoadScreen ||
			(!pNextLoadScreen ? pLoadScreen = LoadScreens.m_item : pLoadScreen = pNextLoadScreen->m_item,
				pLoadScreen)) {
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
					goto LABEL_13;
				}
				}
			} else {
			LABEL_13:;
				UpdateObjectiveTilesVisibilities(0);
				UpdateStatsTilesVisibilities(0);
				UpdateTipTilesVisibilities(0);
				UpdateXPProgressTilesVisibilities(0);
			}
		}
	}
}

void LoadingMenu::UpdatePlayerTileAlphas(TESLoadScreen* pLoadScreen, float afAlpha, int32_t anSlide) {
	if (GetFlag(LoadingMenu::LM_SHOW_PLAYER_TILES)) {
		if (pLoadScreen ||
			(!pNextLoadScreen ? pLoadScreen = LoadScreens.m_item : pLoadScreen = pNextLoadScreen->m_item,
				pLoadScreen)) {
			if (pLoadScreen->GetLoadScreenType()) {
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
			Tile* pTile_2 = TipTilesList02.GetNext(piter);
			if (pTile_2) {
				if (pTile_2->GetFloat(4003) == 0.0) {
					pTile_2->Set(4003, 1);
				}
				FadeIn(pTile_2, afAlpha);
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

// The exe reloads pMe for every call after storing it. The menu found by GetMenuByClass is returned as is; a menu
// tile of another class is deleted when it has a menu tile (the exe's GetMenuTile is the shared getter 0x726070).
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
		if (pMenu && pMenu->GetMenuTile())
			delete pMenu;
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
	LoadingMenu::pMe->UpdateObjectiveTilesVisibilities(0);
	LoadingMenu::pMe->UpdateTipTilesVisibilities(0);
	LoadingMenu::pMe->UpdateStatsTilesVisibilities(0);
	LoadingMenu::pMe->UpdateXPProgressTilesVisibilities(0);
	LoadingMenu::pMe->_Create(apLocation);
	LoadingMenu::pMe->SetFlag(LM_ENABLE_LOADING_MENU_COMMAND, abScripted);
	if (!LoadingMenu::pLoadingThread) {
		void* pMemory = ourMalloc(sizeof(LoadingMenuThread));
		LoadingMenu::pLoadingThread = pMemory ? ::new (pMemory) LoadingMenuThread : nullptr;
	} else if (abBackgroundUpdate) {
		ResumeBackgroundThread();
	}
	// The two slide tiles start transparent.
	LoadingMenu::pMe->pTiles[32]->Set(Tile::kTileValue_Alpha, 0);
	LoadingMenu::pMe->pTiles[33]->Set(Tile::kTileValue_Alpha, 0);
	return pFile;
}

void LoadingMenu::ShowChanges() {
	NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
	Renderer_->LockRenderer();
	BSCriticalSection* pTileCritSect = Tile::GetTileUpdateAllCriticalSectionLock();
	if (pTileCritSect->TryLock()) {
		if (TryEnter(&NiObjectNET::ExtraDataCrit.m_kCriticalSection, nullptr)) {
			if (!LoadingMenu::bDoTileUpdate || BSTexturePalette::TryLock()) {
				if (LoadingMenu::bDoTileUpdate) {
					Tile::UpdateAll(1);
					BSTexturePalette::Unlock();
				}
				MTRenderingSystem::QInstance()->StartThreads();
				for (int32_t i = 1; i < 23; ++i) {
					MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, i);
					MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
				}
				LoadingMenu::bRendering = 1;
				InterfaceManager* v4 = InterfaceManager::Instance();
				v4->Click(0, 0);
				LoadingMenu::bRendering = 0;
				MTRenderingSystem::QInstance()->WaitForThreads();
				NiDX9Renderer* v5 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
				v5->UnlockRenderer();
				pTileCritSect->Unlock();
				NiObjectNET::ExtraDataCrit.Unlock();
			} else {
				NiObjectNET::ExtraDataCrit.Unlock();
				pTileCritSect->Unlock();
				NiDX9Renderer* v3 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
				v3->UnlockRenderer();
			}
		} else {
			pTileCritSect->Unlock();
			NiDX9Renderer* v2 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
			v2->UnlockRenderer();
		}
	} else {
		NiDX9Renderer* v1 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
		v1->UnlockRenderer();
	}
}

void LoadingMenu::FlagReset(bool abOn) {
	if (LoadingMenu::pMe) {
		LoadingMenu::pMe->SetFlag(LoadingMenu::LM_RESET_MENU, abOn);
	}
}

void LoadingMenu::SuspendBackgroundThread() {
	if (LoadingMenu::pMe && LoadingMenu::pLoadingThread && LoadingMenu::pLoadingThread->QIsThreadActive() &&
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
			Warning(WARN_MENUS, "Loading Menu:  Invalid Level Progress Values\n  LowXP=%d, HighXP=%d, CurrXP=%.2f",
				iLowXP, iHighXP, fCurrXP);
		}
		return (fCurrXP - static_cast<double>(iLowXP)) / static_cast<double>(iHighXP - iLowXP);
	}
	return 0.0f;
}

void LoadingMenu::SetupForStartMenu(int32_t) {
	if (LoadingMenu::pMe)
		LoadingMenu::pMe->HideAllBut(eBackground, false);
}

void LoadingMenu::SetScriptMode(uint8_t abScriptMode) {
	// The exe passes the argument's byte zero-extended (movzx), whatever its value: read it as a raw byte, not as
	// a value clang may assume is already zero-extended.
	if (LoadingMenu::pMe)
		LoadingMenu::pMe->SetFlag(LM_ENABLE_LOADING_MENU_COMMAND, abScriptMode);
}

bool LoadingMenu::IsInScriptMode() {
	return LoadingMenu::pMe && LoadingMenu::pMe->GetFlag(LM_ENABLE_LOADING_MENU_COMMAND);
}

bool LoadingMenu::QResetFlagged() {
	return LoadingMenu::pMe && LoadingMenu::pMe->GetFlag(LM_RESET_MENU);
}

void LoadingMenu::SetWorldspaceAndChangedFlag(TESWorldSpace* apWorldSpace) {
	if (LoadingMenu::pMe && apWorldSpace && !LoadingMenu::pMe->pLocation) {
		FlagReset(1);
		LoadingMenu::pMe->Reset(apWorldSpace);
	}
}

void LoadingMenu::ResumeBackgroundThread() {
	if (LoadingMenu::pMe && LoadingMenu::pLoadingThread)
		LoadingMenu::pLoadingThread->Unsuspend();
}

void LoadingMenu::SetupForMainMenu(bool abCreate) {
	if (LoadingMenu::pMe) {
		LoadingMenu::SuspendBackgroundThread();
	} else {
		if (abCreate) {
			LoadingMenu::Create(0, 0, 0);
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
	uint32_t FormHealth_0;
	TESQuest* pquest;
	uint32_t iindex;
	bool buseLocationSpecificScreensOnly;
	LoadingMenu::FillDisplayData();
	if (LoadingMenu::DisplayData.pCurrentObjective) {
		pquest = LoadingMenu::DisplayData.pCurrentObjective->pOwnerQuest;
	} else {
		pquest = 0;
	}
	if (LoadingMenu::DisplayData.pCurrentObjective) {
		FormHealth_0 = LoadingMenu::DisplayData.pCurrentObjective->iIndex;
	} else {
		FormHealth_0 = 0;
	}
	iindex = FormHealth_0;
	apSaveGameBuffer->SaveFormID(pquest, 0u);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iindex), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&LoadingMenu::DisplayData.fLevelProgress), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&LoadingMenu::DisplayData.iLevel), 4u, 0);
	buseLocationSpecificScreensOnly = TESLoadScreen::QUseLocationSpecificScreensOnly();
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&buseLocationSpecificScreensOnly), 1u, 0);
}

void LoadingMenu::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	BGSQuestObjective* Objective;
	uint32_t iindex;
	bool buseLocationSpecificScreensOnly;
	iindex = 0;
	uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
	TESQuest* pquest = dynamic_cast<TESQuest*>(FormByNumericID);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iindex), 4u);
	if (pquest) {
		Objective = pquest->GetObjective(iindex);
	} else {
		Objective = 0;
	}
	LoadingMenu::DisplayData.pCurrentObjective = Objective;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&LoadingMenu::DisplayData.fLevelProgress), 4u);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&LoadingMenu::DisplayData.iLevel), 4u);
	if (LoadingMenu::DisplayData.iLevel >= iMaxCharacterLevel.UInt()) {
		LoadingMenu::DisplayData.fLevelProgress = 0.0f;
	}
	buseLocationSpecificScreensOnly = 0;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&buseLocationSpecificScreensOnly), 1u);
	SetIsLocationSpecificLoadScreensOnly(buseLocationSpecificScreensOnly);
	if (LoadingMenu::pMe) {
		LoadingMenu::pMe->pTiles[11]->Set(4003, 1);
		LoadingMenu::pMe->SetFlag(LoadingMenu::LM_STATIC_MENU, 0);
	}
	LoadingMenu::FlagReset(1);
}

LoadingMenu::WelcomeScreenID operator++(LoadingMenu::WelcomeScreenID& aeID, int32_t) {
	LoadingMenu::WelcomeScreenID eOld = aeID;
	aeID = static_cast<LoadingMenu::WelcomeScreenID>(aeID + 1);
	return eOld;
}

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
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
	return (bool)((uint8_t)LoadingMenu::bLocationSpecificLoadScreensOnly);
}

bool Main::GameReset() {
	return (bool)((uint8_t)bResetGame);
}
