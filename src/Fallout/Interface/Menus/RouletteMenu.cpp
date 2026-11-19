#include "Fallout/Interface/Menus/RouletteMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>
#include <cstdint>
#include <cstring>

static RouletteMenu::RouletteState eprevRouletteState{RouletteMenu::RO_IDLE};
static bool bInvalidBet;
static float ftotalStateSecs;

// spinningWheelSound's guard (bit 0).
static uint32_t uiSpinningWheelSoundGuard;

// The setting key is misspelt sFinsihBetText, as the game does.
static SettingT<GameSettingCollection> sFinishBetText("sFinsihBetText", "Finish Bet");
NiPointer<NiCamera> RouletteMenu::sp3DCamera;
NiPointer<NiNode> RouletteMenu::spModel;

RouletteMenu::BetTileInfo::BetTileInfo() {}

RouletteMenu::RouletteMenu()
  : eRouletteState(RouletteMenu::RO_IDLE), uiPrevIdleTick(0), pBetList(new BSSimpleList<RouletteMenu::RouletteBet*>),
    pCurrentBetFace(nullptr), fRadius(0.0f), fValidRadius(0.0f), fxTranslate(0.0f), fyTranslate(0.0f),
    fzTranslate(0.0f), iNextModelIndex(0), iNumBetsSet(0), iCurrentBet(0), iTotalBet(0), iChipCount(0), iLuckValue(0),
    iTotalWinnings(0), iMinBet(0), bLucky(false), iCheatToggle(pPlayer->iCasinoCheatLevel) {
    RouletteMenu::iUT_Value = Tile::AddUserTrait("_Value", -1);
    RouletteMenu::iUT_X = Tile::AddUserTrait(rdata_1075F74, -1);
}

RouletteMenu::~RouletteMenu() {
    // MAX_PATH bytes.
    char Dst[260];
    SetLastAntiCheatTime();
    pModelLoader->ReleaseFile(RouletteMenu::pCasino->GetModel(9)->GetModel());
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_Roulette\\NV_Roulette-Bet.NIF");
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_Roulette\\NV_Roulette-Points.NIF");
    pModelLoader->ReleaseFile(RouletteMenu::pCasino->GetModel(6)->GetModel());
    char* tempstr = nullptr;
    BSstrcpy(Dst, 260, RouletteMenu::pCasino->GetModel(6)->GetModel());
    for (int32_t j = 0; j < BSstrlen(Dst); ++j) {
        if (Dst[j] == '.') {
            tempstr = &Dst[j];
            break;
        }
    }
    if (tempstr) {
        BSstrcpy(tempstr, 15, "_Shadow.NIF");
        pModelLoader->ReleaseFile(Dst);
    }
    RouletteMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(RouletteMenu::spModel.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    RouletteMenu::spModel.m_pObject->DetachAllChildren();
    RouletteMenu::spModel = static_cast<NiNode*>(nullptr);
    pRouletteTableGeometry = nullptr;
    pRouletteGridGeometry = nullptr;
    pCursorGeometry = nullptr;
    pCurrentBetFace = nullptr;
    pValidBetFace = nullptr;
    pInvalidBetFace = nullptr;
    // The bet list is the menu's own heap object. It is emptied only once, before it is freed; the game empties it
    // again after freeing it (a read of freed memory), which is deliberately not done here.
    if (pBetList) {
        if (!pBetList->IsEmpty()) {
            pBetList->RemoveAll();
        }
        delete pBetList;
    }
    pBetList = nullptr;
    pTableAnimManager = nullptr;
    for (NiControllerSequence*& pAnimation : pTableAnimations) {
        pAnimation = nullptr;
    }
    for (int32_t i = 0; i < 10; ++i) {
        pBetGeometry[i] = nullptr;
        pBetShadowGeometry[i] = nullptr;
    }
    RouletteMenu::pCasinoData = nullptr;
    RouletteMenu::pCasino = nullptr;
    RouletteMenu::pMe = nullptr;
}

void RouletteMenu::AttachTileByID(int32_t a1, Tile* a2) {
    if (static_cast<uint32_t>(a1) <= 10) {
        pTiles[a1] = a2;
    }
}

void RouletteMenu::DoClick(int32_t a1, Tile* a2) {
    if (eRouletteState == RouletteMenu::RO_IDLE) {
        switch (a1) {
            case 3: {
                int32_t i = GetBetIndex();
                if (i < 159 && iTotalBet < iMaxBet && SetBet(i)) {
                    RouletteMenu::pMe->pTiles[10]->Poke(RouletteMenu::iUT_X, 1.0f);
                    RouletteMenu::pMe->pTiles[10]->Set(RouletteMenu::iUT_X, iTotalBet);
                    if (iCurrentBet + iTotalBet >= iMaxBet) {
                        iCurrentBet = iMaxBet - iTotalBet;
                        RouletteMenu::pMe->pTiles[0]->Poke(RouletteMenu::iUT_X, 1.0f);
                        RouletteMenu::pMe->pTiles[0]->Set(RouletteMenu::iUT_X, iCurrentBet);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_PLACEBET_CNTRL, 128.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 128.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 128.0f);
                    }
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_REMOVEBET_CNTRL, 255.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_FINISHBETS_CNTRL, 255.0f);
                    if (!iCurrentBet) {
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_PLACEBET_CNTRL, 128.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 128.0f);
                    }
                    if (iNumBetsSet == 10) {
                        iCurrentBet = 0;
                        RouletteMenu::pMe->pTiles[0]->Poke(RouletteMenu::iUT_X, 1.0f);
                        RouletteMenu::pMe->pTiles[0]->Set(RouletteMenu::iUT_X, iCurrentBet);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_PLACEBET_CNTRL, 128.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 128.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 128.0f);
                    }
                }
                break;
            }
            case 4: {
                int32_t i_2 = GetBetIndex();
                if (i_2 < 159 && RemoveBet(i_2)) {
                    BSAudio* v3 = &BSAudio::QInstance();
                    v3->GetSoundHandleByName("GAMERouletteDecreaseBet", 289u).Play(false);
                    RouletteMenu::pMe->pTiles[10]->Poke(RouletteMenu::iUT_X, 1.0f);
                    RouletteMenu::pMe->pTiles[10]->Set(RouletteMenu::iUT_X, iTotalBet);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 255.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_PLACEBET_CNTRL, 255.0f);
                    if (!pBetList->ItemsInList()) {
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_FINISHBETS_CNTRL, 128.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_REMOVEBET_CNTRL, 128.0f);
                    }
                    if (iNumBetsSet == 9) {
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_FINISHBETS_CNTRL, 255.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_REMOVEBET_CNTRL, 255.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_PLACEBET_CNTRL, 255.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 255.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 255.0f);
                        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_EXIT_CNTRL, 255.0f);
                    }
                }
                break;
            }
            case 5: {
                if ((pBetList) && (!pBetList->IsEmpty())) {
                    SetWinningSlotLuckyAndAnimToPlay();
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_FINISHBETS_CNTRL, 128.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_REMOVEBET_CNTRL, 128.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_PLACEBET_CNTRL, 128.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 128.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 128.0f);
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_EXIT_CNTRL, 128.0f);
                    eRouletteState = RouletteMenu::RO_SPIN_ANIM;
                    MiscStatManager::Increment(MiscStatManager::MS_ROULETTE_GAMES_PLAYED);
                }

                break;
            }
            case 6: {
                int32_t iTempBet = iCurrentBet;
                if (iCurrentBet >= 10) {
                    if (iCurrentBet >= 40) {
                        if (iCurrentBet >= 100) {
                            int32_t v4;
                            if (iCurrentBet >= 500) {
                                v4 = iCurrentBet + 100;
                            }
                            else {
                                v4 = iCurrentBet + 25;
                            }
                            iCurrentBet = v4;
                        }
                        else {
                            iCurrentBet += 10;
                        }
                    }
                    else {
                        iCurrentBet += 5;
                    }
                }
                else {
                    ++iCurrentBet;
                }
                RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 255.0f);
                if (iTotalBet + iCurrentBet >= iChipCount) {
                    iCurrentBet = iChipCount - iTotalBet;
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 128.0f);
                }
                if (iCurrentBet + iTotalBet >= iMaxBet) {
                    iCurrentBet = iMaxBet - iTotalBet;
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 128.0f);
                }
                if (iNumBetsSet != 10) {
                    SetBetChip();
                }
                if (iTempBet != iCurrentBet) {
                    BSAudio* v5 = &BSAudio::QInstance();
                    v5->GetSoundHandleByName("GAMERouletteValidBet", 289u).Play(false);
                }
                RouletteMenu::pMe->pTiles[0]->Poke(RouletteMenu::iUT_X, 1.0f);
                RouletteMenu::pMe->pTiles[0]->Set(RouletteMenu::iUT_X, iCurrentBet);
                break;
            }
            case 7: {
                int32_t iTempBet_2 = iCurrentBet;
                if (iCurrentBet > 10) {
                    if (iCurrentBet > 40) {
                        if (iCurrentBet > 100) {
                            if (iCurrentBet > 500) {
                                int32_t v8;
                                if (iCurrentBet % 100) {
                                    v8 = 100 * (iCurrentBet / 100);
                                }
                                else {
                                    v8 = iCurrentBet - 100;
                                }
                                iCurrentBet = v8;
                            }
                            else {
                                int32_t v9;
                                if (iCurrentBet % 25) {
                                    v9 = 25 * (iCurrentBet / 25);
                                }
                                else {
                                    v9 = iCurrentBet - 25;
                                }
                                iCurrentBet = v9;
                            }
                        }
                        else {
                            int32_t v10;
                            if (iCurrentBet % 10) {
                                v10 = 10 * (iCurrentBet / 10);
                            }
                            else {
                                v10 = iCurrentBet - 10;
                            }
                            iCurrentBet = v10;
                        }
                    }
                    else {
                        int32_t v11;
                        if (iCurrentBet % 5) {
                            v11 = 5 * (iCurrentBet / 5);
                        }
                        else {
                            v11 = iCurrentBet - 5;
                        }
                        iCurrentBet = v11;
                    }
                }
                else {
                    --iCurrentBet;
                }
                RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 255.0f);
                if (iCurrentBet <= iMinBet && iTotalBet + iMinBet < iMaxBet) {
                    iCurrentBet = iMinBet;
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 128.0f);
                }
                if (iCurrentBet <= 0) {
                    iCurrentBet = 0;
                    RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_DECREASEBET_CNTRL, 128.0f);
                }
                if (iNumBetsSet != 10) {
                    SetBetChip();
                }
                if (iTempBet_2 != iCurrentBet) {
                    BSAudio* v6 = &BSAudio::QInstance();
                    v6->GetSoundHandleByName("GAMERouletteValidBet", 289u).Play(false);
                }
                RouletteMenu::pMe->pTiles[0]->Poke(RouletteMenu::iUT_X, 1.0f);
                RouletteMenu::pMe->pTiles[0]->Set(RouletteMenu::iUT_X, iCurrentBet);
                break;
            }
            case 8: {
                RouletteMenu::Close();
                break;
            }
            default: {
                return;
            }
        }
    }
}

void RouletteMenu::DoEnter(int32_t a1, Tile* a2) {
    InterfaceManager* singleton = InterfaceManager::Instance();
    Tile* processLevel = singleton->GetCursorTile();
    processLevel->Set(4009, (float)0.0, true);
}

void RouletteMenu::DoLeave(int32_t a1, Tile* a2) {
    InterfaceManager* singleton = InterfaceManager::Instance();
    Tile* processLevel = singleton->GetCursorTile();
    processLevel->Set(4009, 40.0f, true);
}

// Runs the state machine: the spin animation, paying out the bets, moving the cursor chip over the table while idle,
// and settling the chips with the player on closing. The payout test reads the debug axis (always 0) and ignores its
// 0x7FFF maximum, as the game does.
void RouletteMenu::DoIdle() {
    int32_t ielaspedMillis = 0;
    float fxAxisRatio = 1.0f;
    float fyAxisRatio = 1.0f;
    // The function-static spinningWheelSound is unused.
    static BSSoundHandle spinningWheelSound;
    bool bisMenuOnTop = Interface::IsTopMenuID(static_cast<Interface::enumMenus>(this->GetClass()));
    if (!this->uiPrevIdleTick) {
        ielaspedMillis = 0;
        ftotalStateSecs = 0.0f;
    }
    else if (bisMenuOnTop) {
        ielaspedMillis = static_cast<int32_t>(appTimer.GetTime() - this->uiPrevIdleTick);
        if (ielaspedMillis > 1000) {
            ielaspedMillis = 0;
        }
        if (eprevRouletteState == this->eRouletteState) {
            ftotalStateSecs = static_cast<float>(ielaspedMillis / 1000.0 + ftotalStateSecs);
        }
        else {
            ftotalStateSecs = 0.0f;
        }
    }
    if (bisMenuOnTop) {
        InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 0.0f, true);
    }
    else {
        InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 255.0f, true);
    }
    int32_t ixAxis = 0;
    int32_t iyAxis = 0;
    int32_t iDebugAxis = 0;
    if (InterfaceManager::Instance()->Has360Controller()) {
        ixAxis = appMain->GetControls()->GetControllerValue(0, 7);
        iyAxis = appMain->GetControls()->GetControllerValue(0, 8);
        // A dead zone up to 7849 inclusive (NaN keeps the value).
        if (NiAbs(static_cast<float>(ixAxis)) <= 7849.0) {
            ixAxis = 0;
        }
        if (NiAbs(static_cast<float>(iDebugAxis)) <= 7849.0) {
            iDebugAxis = 0;
        }
        fxAxisRatio = static_cast<float>(-ixAxis / 32767.0);
        if (NiAbs(static_cast<float>(iyAxis)) <= 7849.0) {
            iyAxis = 0;
        }
        fyAxisRatio = static_cast<float>(-iyAxis / 32767.0);
    }
    else {
        double dMouseX = -appMain->GetControls()->GetMouseAxis(1);
        double dScreenWidth = InterfaceManager::GetScreenWidth();
        fxAxisRatio = static_cast<float>(dScreenWidth / InterfaceManager::GetRealScreenWidth() * dMouseX / 10.0);
        double dMouseY = appMain->GetControls()->GetMouseAxis(2);
        double dScreenWidthY = InterfaceManager::GetScreenWidth();
        fyAxisRatio = static_cast<float>(dScreenWidthY / InterfaceManager::GetRealScreenWidth() * dMouseY / 10.0);
    }
    this->uiPrevIdleTick = appTimer.GetTime();
    eprevRouletteState = this->eRouletteState;
    switch (this->eRouletteState) {
        case RO_SPIN_ANIM: {
            if (!this->pTableAnimations[this->iSpinAnimIndex]) {
                break;
            }
            // Only an exact zero (not NaN) starts the spin.
            if (ftotalStateSecs == 0.0) {
                BSAudio::QInstance()
                    .GetSoundHandleByName(
                        "GAMERouletteSpin",
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT
                    )
                    .Play(false);
                this->pTiles[9]->Set(Tile::kTileValue_Visible, 0);
                this->pTableAnimManager->ActivateSequence(
                    this->pTableAnimations[this->iSpinAnimIndex], 0, false, 1.0f, 0.0f, nullptr
                );
                this->bCleanTable = true;
                this->ShiftWheelTexture(this->iResultValue);
            }
            NiUpdateData UpdateData1;
            UpdateData1.~NiUpdateData();
            ::new (&UpdateData1) NiUpdateData(ftotalStateSecs, true, false);
            spModel->GetAt(0)->Update(UpdateData1);
            // Past half the spin (not NaN) clears the table once.
            if (ftotalStateSecs > this->pTableAnimations[this->iSpinAnimIndex]->GetEndKeyTime() / 2.0 &&
                this->bCleanTable) {
                this->ResetBetChips();
                this->iNextModelIndex = 3;
                const NiPoint3& rCursor = spModel->GetAt(2)->GetTranslate();
                float fLocX = rCursor.x;
                float fLocY = rCursor.y;
                spModel->GetAt(this->iNextModelIndex)->SetTranslate(fLocX, fLocY, 0.0f);
                this->SetBetChip();
                this->bCleanTable = false;
            }
            if (ftotalStateSecs > this->pTableAnimations[this->iSpinAnimIndex]->GetEndKeyTime()) {
                this->eRouletteState = RO_DETERMINE_RESULTS;
                this->pTableAnimManager->DeactivateSequence(this->pTableAnimations[this->iSpinAnimIndex], 0.0f);
            }
            break;
        }
        case RO_DETERMINE_RESULTS: {
            this->eRouletteState = RO_IDLE;
            int32_t iCurrentWinnings = 0;
            RouletteMenu::pMe->SetTileTextAlpha(TI_FINISHBETS_CNTRL, 255.0f);
            RouletteMenu::pMe->SetTileTextAlpha(TI_PLACEBET_CNTRL, 255.0f);
            RouletteMenu::pMe->SetTileTextAlpha(TI_REMOVEBET_CNTRL, 128.0f);
            // The bet is compared signed.
            if (this->iCurrentBet < this->iMinBet) {
                RouletteMenu::pMe->SetTileTextAlpha(TI_DECREASEBET_CNTRL, 128.0f);
            }
            else {
                RouletteMenu::pMe->SetTileTextAlpha(TI_DECREASEBET_CNTRL, 255.0f);
            }
            if (this->iCurrentBet > this->iMaxBet || this->iCurrentBet == this->iChipCount) {
                RouletteMenu::pMe->SetTileTextAlpha(TI_DECREASEBET_CNTRL, 128.0f);
            }
            else {
                RouletteMenu::pMe->SetTileTextAlpha(TI_DECREASEBET_CNTRL, 255.0f);
            }
            RouletteMenu::pMe->SetTileTextAlpha(TI_EXIT_CNTRL, 255.0f);
            while (this->pBetList && this->pBetList->GetItem()) {
                bool won = false;
                int32_t betindex = this->pBetList->GetItem()->betIndex;
                BetTileInfo* betInfo = &this->betTileInfoArray[betindex];
                if (betindex < 147) {
                    for (BSSimpleList<int32_t>* valueList = &betInfo->numValueList; valueList;
                         valueList = valueList->GetNext()) {
                        if (this->iResultValue == valueList->GetItem()) {
                            won = true;
                            break;
                        }
                    }
                }
                else {
                    switch (betindex) {
                        case 147:
                            if (this->iResultValue < 13 && this->iResultValue > 0) {
                                won = true;
                            }
                            break;
                        case 148:
                            if (this->iResultValue > 0 && this->iResultValue < 19) {
                                won = true;
                            }
                            break;
                        case 149:
                            if (this->iResultValue % 2 == 0) {
                                won = true;
                            }
                            break;
                        case 150:
                            if (this->iResultValue < 25 && this->iResultValue > 12) {
                                won = true;
                            }
                            break;
                        case 151: {
                            int32_t i;
                            for (i = 0; i < 38 && this->iNumberStrip[i] != this->iResultValue; ++i) {}
                            if (i % 2 == 0) {
                                won = true;
                            }
                            break;
                        }
                        case 152: {
                            int32_t i;
                            for (i = 0; i < 38 && this->iNumberStrip[i] != this->iResultValue; ++i) {}
                            if (i % 2 == 1) {
                                won = true;
                            }
                            break;
                        }
                        case 153:
                            if (this->iResultValue < 37 && this->iResultValue > 24) {
                                won = true;
                            }
                            break;
                        case 154:
                            if (this->iResultValue % 2 == 1) {
                                won = true;
                            }
                            break;
                        case 155:
                            if (this->iResultValue > 18 && this->iResultValue < 37) {
                                won = true;
                            }
                            break;
                        case 156:
                            if ((this->iResultValue - 1) % 3 == 0) {
                                won = true;
                            }
                            break;
                        case 157:
                            if ((this->iResultValue - 2) % 3 == 0) {
                                won = true;
                            }
                            break;
                        case 158:
                            if (this->iResultValue % 3 == 0) {
                                won = true;
                            }
                            break;
                        default: break;
                    }
                }
                if ((won || iDebugAxis != 0 || this->iCheatToggle > 0) && this->iCheatToggle >= 0) {
                    int32_t iPayout = betInfo->betPayoutRatio + 1;
                    iCurrentWinnings += iPayout * this->pBetList->GetItem()->betValue;
                }
                this->pBetList->RemoveHead();
            }
            iCurrentWinnings -= this->iTotalBet;
            this->iChipCount += iCurrentWinnings;
            this->pTiles[1]->Poke(iUT_X, 1.0f);
            this->pTiles[1]->Set(iUT_X, this->iChipCount);
            pCasinoData->iTotalWinnings += iCurrentWinnings;
            this->UpdateEarningsString();
            char winmsg[260];
            if (iCurrentWinnings > 0) {
                const char* pPlural = sPlural.String();
                const char* pChips = sCasinoChipText.String();
                if (this->bLucky) {
                    const char* pWin = sLuckyWinText.String();
                    BSsprintf(winmsg, 0x104, "%s %d %s%s", pWin, iCurrentWinnings, pChips, pPlural);
                }
                else {
                    const char* pWin = sYouWinText.String();
                    BSsprintf(winmsg, 0x104, "%s %d %s%s", pWin, iCurrentWinnings, pChips, pPlural);
                }
                BSAudio::QInstance()
                    .GetSoundHandleByName(
                        "GAMERouletteWin",
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT
                    )
                    .Play(false);
                RouletteMenu::pMe->pTiles[9]->Poke(Tile::kTileValue_String, 1.0f);
                RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_String, winmsg, true);
                RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_Visible, 1);
            }
            else if (iCurrentWinnings < 0) {
                const char* pPlural = sPlural.String();
                const char* pChips = sCasinoChipText.String();
                const char* pLose = sYouLoseText.String();
                BSsprintf(winmsg, 0x104, "%s %d %s%s", pLose, -iCurrentWinnings, pChips, pPlural);
                RouletteMenu::pMe->pTiles[9]->Poke(Tile::kTileValue_String, 1.0f);
                RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_String, winmsg, true);
                RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_Visible, 1);
            }
            else {
                BSsprintf(winmsg, 0x104, "%s", sYouBreakEvenText.String());
                RouletteMenu::pMe->pTiles[9]->Poke(Tile::kTileValue_String, 1.0f);
                RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_String, winmsg, true);
                RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_Visible, 1);
            }
            this->iNumBetsSet = 0;
            this->iNextModelIndex = 3;
            this->iTotalBet = 0;
            this->iCurrentBet = 0;
            RouletteMenu::pMe->pTiles[10]->Poke(iUT_X, 1.0f);
            RouletteMenu::pMe->pTiles[10]->Set(iUT_X, this->iTotalBet);
            if (!this->iCurrentBet) {
                this->iCurrentBet = this->iMinBet;
                // The bet is converted as a signed int.
                RouletteMenu::pMe->pTiles[0]->Poke(iUT_X, static_cast<float>(this->iCurrentBet));
                RouletteMenu::pMe->pTiles[0]->Set(iUT_X, this->iCurrentBet);
            }
            this->eRouletteState = RO_IDLE;
            int32_t iMax = pCasino->GetData()->iMaxWinnings;
            int32_t totalWon = pCasinoData->iTotalWinnings;
            int32_t iWinningLevel = pCasinoData->iWinningLevel;
            for (int32_t i = 1; i < 5; ++i) {
                if (i <= iWinningLevel) {
                    this->bRunScriptOnExit = false;
                }
                else if (totalWon >= iMax * (i * 0.25)) {
                    // A total below the quarter (not NaN) leaves the level.
                    pCasinoData->iWinningLevel = static_cast<int16_t>(i);
                    this->bRunScriptOnExit = true;
                }
            }
            if (this->bRunScriptOnExit) {
                RouletteMenu::Close();
            }
            RouletteMenu::pMe->SetTileTextAlpha(TI_INCREASEBET_CNTRL, 255.0f);
            break;
        }
        case RO_IDLE: {
            // A nonzero or NaN ratio moves the cursor chip.
            if (fxAxisRatio != 0.0 || fyAxisRatio != 0.0) {
                float movementDamper = 0.35f;
                this->fxTranslate =
                    static_cast<float>(this->fxTranslate - static_cast<double>(fxAxisRatio) * movementDamper);
                this->fyTranslate =
                    static_cast<float>(this->fyTranslate - static_cast<double>(fyAxisRatio) * movementDamper);
                // The bounds are compared as doubles; the compares are strict and a NaN value is kept.
                if (this->fxTranslate < -27.5) {
                    this->fxTranslate = -27.5f;
                }
                if (this->fyTranslate < -13.300000190734863) {
                    this->fyTranslate = -13.3f;
                }
                if (this->fxTranslate > 28.0) {
                    this->fxTranslate = 28.0f;
                }
                if (this->fyTranslate > 13.100000381469727) {
                    this->fyTranslate = 13.1f;
                }
                spModel->GetAt(2)->SetTranslate(this->fxTranslate, this->fyTranslate, this->fzTranslate);
                NiUpdateData kCursorUpdate;
                kCursorUpdate.~NiUpdateData();
                ::new (&kCursorUpdate) NiUpdateData(0.0f, false, false);
                spModel->GetAt(2)->Update(kCursorUpdate);
                if (this->iNumBetsSet < 10) {
                    float fChipZ = spModel->GetAt(this->iNextModelIndex)->GetTranslate().z;
                    spModel->GetAt(this->iNextModelIndex)->SetTranslate(this->fxTranslate, this->fyTranslate, fChipZ);
                    spModel->GetAt(this->iNextModelIndex + 10)
                        ->SetTranslate(this->fxTranslate, this->fyTranslate, 0.0f);
                    NiUpdateData kChipUpdate;
                    kChipUpdate.~NiUpdateData();
                    ::new (&kChipUpdate) NiUpdateData(0.0f, false, false);
                    spModel->GetAt(this->iNextModelIndex)->Update(kChipUpdate);
                    NiUpdateData kShadowUpdate;
                    kShadowUpdate.~NiUpdateData();
                    ::new (&kShadowUpdate) NiUpdateData(0.0f, false, false);
                    spModel->GetAt(this->iNextModelIndex + 10)->Update(kShadowUpdate);
                }
                int32_t iBetIndex = this->GetBetIndex();
                if (iBetIndex < 159) {
                    if (this->pCurrentBetFace) {
                        this->pCurrentBetFace->SetAppCulled(true);
                    }
                    this->pCurrentBetFace = BSUtilities::GetObjectByName(
                        this->pRouletteGridGeometry, this->betTileInfoArray[iBetIndex].faceName
                    );
                    if (!this->pCurrentBetFace) {
                        alignas(NiFixedString) uint8_t aNameStorage[sizeof(NiFixedString)];
                        NiFixedString* pName = ::new (static_cast<void*>(aNameStorage))
                            NiFixedString(this->betTileInfoArray[iBetIndex].faceName);
                        this->pCurrentBetFace = this->pRouletteGridGeometry->GetObjectByName(*pName);
                        pName->~NiFixedString();
                    }
                    this->pTiles[9]->Set(Tile::kTileValue_String, this->betTileInfoArray[iBetIndex].cBetTiles, true);
                    if (this->pValidBetFace->GetAppCulled()) {
                        if (bInvalidBet) {
                            bInvalidBet = !bInvalidBet;
                            BSAudio::QInstance()
                                .GetSoundHandleByName(
                                    "GAMERouletteValidBet",
                                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                        BSGameSound::SOUND_FLAG_ONE_SHOT
                                )
                                .Play(false);
                        }
                        this->pValidBetFace->SetAppCulled(false);
                        this->pInvalidBetFace->SetAppCulled(true);
                    }
                    this->pCurrentBetFace->SetAppCulled(false);
                }
                else {
                    char acBetText[260];
                    const char* pPayout = sPayoutText.String();
                    const char* pBet = sBetText.String();
                    BSsprintf(acBetText, 0x104, "%s: \n%s: ", pBet, pPayout);
                    this->pTiles[9]->Set(Tile::kTileValue_String, acBetText, true);
                    if (this->pInvalidBetFace->GetAppCulled()) {
                        if (!bInvalidBet) {
                            bInvalidBet = !bInvalidBet;
                            BSAudio::QInstance()
                                .GetSoundHandleByName(
                                    "GAMERouletteInvalidBet",
                                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                        BSGameSound::SOUND_FLAG_ONE_SHOT
                                )
                                .Play(false);
                        }
                        this->pValidBetFace->SetAppCulled(true);
                        this->pInvalidBetFace->SetAppCulled(false);
                    }
                    if (this->pCurrentBetFace) {
                        this->pCurrentBetFace->SetAppCulled(true);
                    }
                }
            }
            if (this->iChipCount < this->iCurrentBet + this->iTotalBet) {
                this->iCurrentBet = this->iChipCount - this->iTotalBet;
                RouletteMenu::pMe->pTiles[0]->Poke(iUT_Value, 1.0f);
                RouletteMenu::pMe->pTiles[0]->Set(iUT_Value, this->iCurrentBet);
                RouletteMenu::pMe->SetTileTextAlpha(TI_INCREASEBET_CNTRL, 128.0f);
            }
            if (this->iChipCount < this->iMinBet) {
                char strMessage[260];
                // The setting's text is the format itself.
                BSsprintf(strMessage, 0x104, sGamblingBrokeText.String());
                Interface::ShowMessage(
                    strMessage, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr,
                    2.0f, false
                );
                BSAudio::QInstance()
                    .GetSoundHandleByName(
                        "UIPopUpMessageGeneral",
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT
                    )
                    .Play(false);
                RouletteMenu::Close();
            }
            break;
        }
        case RO_CLOSING_MENU: {
            if (this->GetFadeState() == 2) {
                break;
            }
            InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 255.0f, true);
            // 6002 is a menu trait with no known name.
            this->GetMenuTile()->Set(6002, 1);
            this->StartFadeOut();
            int32_t iChange = this->iChipCount - pPlayer->GetInventoryObjectCount(pCasino->GetChipType());
            if (iChange < 0) {
                pPlayer->RemoveItem(
                    pCasino->GetChipType(), nullptr, -iChange, false, false, nullptr, nullptr, nullptr, true, false
                );
                alignas(BSStringT<char>) uint8_t aStringStorage[sizeof(BSStringT<char>)];
                BSStringT<char>* pstring = ::new (aStringStorage) BSStringT<char>;
                const char* pRemove = sRemoveItemfromInventory.String();
                const char* pPlural = sPlural.String();
                const char* pName = TESFullName::GetFullName(pCasino->GetChipType());
                pstring->SPrintF("%i %s%s %s", -iChange, pName, pPlural, pRemove);
                const char* pSound = pPlayer->GetPickUpSoundName(pCasino->GetChipType(), true, false);
                Interface::ShowMessage(
                    pstring->String(), 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", pSound,
                    2.0f, false
                );
                // SWEEP-INLINE 0x4037D0
                pstring->~BSStringT<char>();
            }
            else if (iChange > 0) {
                alignas(TESContainer) uint8_t aContainerStorage[sizeof(TESContainer)];
                TESContainer* tempcont = ::new (aContainerStorage) TESContainer;
                tempcont->AddObject(pCasino->GetChipType(), iChange, nullptr);
                int32_t iMaxWinnings = pCasino->GetMaxWinnings();
                if (pCasinoData->iTotalWinnings < iMaxWinnings) {
                    tempcont->AddContainerToReference(pPlayer, false);
                }
                else {
                    tempcont->AddContainerToReference(pPlayer, true);
                    alignas(BSStringT<char>) uint8_t aStringStorage[sizeof(BSStringT<char>)];
                    BSStringT<char>* pstring = ::new (aStringStorage) BSStringT<char>;
                    const char* pBanned = sGamblingBannedText.String();
                    const char* pAdd = sAddItemtoInventory.String();
                    const char* pPlural = sPlural.String();
                    const char* pName = TESFullName::GetFullName(pCasino->GetChipType());
                    pstring->SPrintF("%i %s%s %s\n%s", iChange, pName, pPlural, pAdd, pBanned);
                    const char* pSound = pPlayer->GetPickUpSoundName(pCasino->GetChipType(), true, false);
                    Interface::ShowMessage(
                        pstring->String(), 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
                        pSound, 2.0f, false
                    );
                    // SWEEP-INLINE 0x4037D0
                    pstring->~BSStringT<char>();
                }
                tempcont->~TESContainer();
            }
            if (this->bRunScriptOnExit) {
                TESQuest* pScript = pCasino->GetQuest();
                if (pScript) {
                    pScript->SetEnabled(true);
                }
            }
            break;
        }
        default: break;
    }
}

int32_t RouletteMenu::GetClass() {
    return 1082;
}

void RouletteMenu::UpdateEarningsString() {
    char Src[264];
    int32_t iTotalWinnings = RouletteMenu::pCasinoData->iTotalWinnings;
    const char* v1 = sCasinoEarningsText.String();
    BSsprintf(Src, 260u, "%s %s%i", RouletteMenu::pCasino->GetFullName(), v1, iTotalWinnings);
    int32_t ilen = BSstrlen(Src);
    if (ilen < 23) {
        const char* v2 = sCasinoEarningsText.String();
        BSsprintf(Src, 260u, "%s %s", RouletteMenu::pCasino->GetFullName(), v2);
        for (int32_t i = 0; i < 23 - ilen; ++i) {
            Src[ilen - 1 + i] = 32;
            Src[i + ilen] = 0;
        }
        BSsprintf(Src, 260u, "%s%i", Src, RouletteMenu::pCasinoData->iTotalWinnings);
    }
    pTiles[2]->Set(RouletteMenu::iUT_Value, Src, true);
}

int32_t RouletteMenu::GetBetIndex() {
    int32_t i;
    for (i = 0; i < 144; ++i) {
        NiPoint3 result;
        NiPoint3* p_location = &betTileInfoArray[i].location;
        const NiPoint3* WorldTranslate = &pCursorGeometry->GetWorldTranslate();
        result = *WorldTranslate - *p_location;
        float v9 = result.Length();
        if (fValidRadius >= static_cast<double>(v9)) {
            break;
        }
    }
    if (i == 144) {
        while (i < 159) {
            NiPoint3 v8;
            NiPoint3* v5 = &betTileInfoArray[i].location;
            const NiPoint3* v2 = &pCursorGeometry->GetWorldTranslate();
            v8 = *v2 - *v5;
            float v7 = v8.Length();
            if (fSpecialRadius[i - 144] >= static_cast<double>(v7)) {
                break;
            }
            ++i;
        }
    }
    return i;
}

void RouletteMenu::ShiftWheelTexture(int32_t aiResult) {
    NiMatrix3 akRot;
    int32_t i;
    for (i = 0; (i < 38) && (iNumberStrip[i] != aiResult); ++i) {
        ;
    }
    i = 38 - i;
    if (iSpinAnimIndex) {
        if (iSpinAnimIndex == 1) {
            i += 3;
        }
        else {
            if (iSpinAnimIndex == 2) {
                i += 10;
            }
        }
    }
    else {
        i += 5;
    }
    std::memcpy(&akRot, &NiMatrix3_IDENTITY, sizeof(akRot));
    float v2 = (float)((((double)i) * 9.473684210526315) * 0.01745329238474369);
    (&akRot)->MakeZRotation(v2);
    NiAVObject* objectByName = BSUtilities::GetObjectByName(pRouletteTableGeometry, "Roulette_Wheel:2");
    if (objectByName) {
        objectByName->SetRotate(akRot);
    }
}

void RouletteMenu::SetTileTextAlpha(RouletteMenu::TileIndex TI, float alpha) {
    NiTListItem<Tile*>* iter = RouletteMenu::pMe->pTiles[TI]->xChildren.m_pkHead;
    for (NiTPointerList<Tile*>* pTileList =
             static_cast<NiTPointerList<Tile*>*>(&RouletteMenu::pMe->pTiles[TI]->xChildren);
         iter && (pTileList && pTileList->GetHead()); iter = pTileList->GetNextPos(iter)) {
        pTileList->Get(iter)->Set(4009, alpha, true);
    }
}

void RouletteMenu::ResetBetChips() {
    for (int32_t i = 0; i < 10; ++i) {
        pBetGeometry[i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), -5.0f);
        pBetShadowGeometry[i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), -5.0f);
    }
    NiUpdateData arData(0.0, false, false);
    NiAVObject* v1 = RouletteMenu::spModel.m_pObject;
    v1->Update(arData);
}

// Geometry pointers are indexed from pRouletteTableGeometry by model index (spModel's child index): models 3-12 are
// pBetGeometry and their shadows pBetShadowGeometry.
void RouletteMenu::SetBetChip() {
    int32_t level = 0;
    if (iCurrentBet >= 5) {
        if (iCurrentBet >= 10) {
            if (iCurrentBet >= 25) {
                if (iCurrentBet >= 40) {
                    if (iCurrentBet >= 70) {
                        if (iCurrentBet >= 100) {
                            if (iCurrentBet >= 250) {
                                if (iCurrentBet >= 500) {
                                    if (iCurrentBet >= 1000) {
                                        level = 10;
                                    }
                                    else {
                                        level = 9;
                                    }
                                }
                                else {
                                    level = 8;
                                }
                            }
                            else {
                                level = 7;
                            }
                        }
                        else {
                            level = 6;
                        }
                    }
                    else {
                        level = 5;
                    }
                }
                else {
                    level = 4;
                }
            }
            else {
                level = 3;
            }
        }
        else {
            level = 2;
        }
    }
    else {
        level = 1;
    }
    NiPoint3 kTranslate = pBetGeometry[iNextModelIndex - 3]->GetTranslate();
    float v2 = static_cast<double>(level) * 0.18000000000000002;
    pBetGeometry[iNextModelIndex - 3]->SetTranslate(kTranslate.x, kTranslate.y, v2);
    pBetShadowGeometry[iNextModelIndex - 3]->SetTranslate(kTranslate.x, kTranslate.y, 0.0f);
    NiUpdateData update1(0.0f, false, false);
    RouletteMenu::spModel.m_pObject->Update(update1);
}

void RouletteMenu::Prepare3DElements() {
    NiNode* v17;
    char modelPath[252];
    TESModelTextureSwap* Model_ = RouletteMenu::pCasino->GetModel(9);
    const char* v1 = Model_->GetModel();
    pRouletteTableGeometry = (NiAVObject*)pModelLoader->LoadFile(v1, LOD_MULT_NONE, true, false, false, false);
    pRouletteGridGeometry = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_Roulette\\NV_Roulette-Points.NIF", LOD_MULT_NONE, true, false, false, false
    );
    pCursorGeometry = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_Roulette\\NV_Roulette-Bet.NIF", LOD_MULT_NONE, true, false, false, false
    );
    TESModelTextureSwap* v19 = RouletteMenu::pCasino->GetModel(6);
    const char* v2 = v19->GetModel();
    pBetGeometry[0] = (NiAVObject*)pModelLoader->LoadFile(v2, LOD_MULT_NONE, true, false, false, false);
    pRouletteGridGeometry->SetTranslate(0.0f, 0.0f, 0.1f);
    pCursorGeometry->SetTranslate(0.0f, 0.0f, 0.1f);
    pValidBetFace = BSUtilities::GetObjectByName(pCursorGeometry, "Bet_Selector:0");
    pInvalidBetFace = BSUtilities::GetObjectByName(pCursorGeometry, "Bet_Invalid:0");
    pInvalidBetFace->SetAppCulled(true);
    char* tempstr = nullptr;
    TESModelTextureSwap* v18 = RouletteMenu::pCasino->GetModel(6);
    const char* v3 = v18->GetModel();
    BSstrcpy(modelPath, 260, v3);
    for (int32_t j = 0; j < BSstrlen(modelPath); ++j) {
        if (modelPath[j] == 46) {
            tempstr = &modelPath[j];
            break;
        }
    }
    if (tempstr) {
        BSstrcpy(tempstr, 15, "_Shadow.NIF");
        pBetShadowGeometry[0] =
            (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    }
    for (int32_t i = 1; i < 10; ++i) {
        pBetGeometry[i] = static_cast<NiAVObject*>(pBetGeometry[0]->Clone());
        pBetShadowGeometry[i] = static_cast<NiAVObject*>(pBetShadowGeometry[0]->Clone());
    }
    NiNode* pTableNode = static_cast<NiNode*>(pRouletteTableGeometry);
    RouletteMenu::sp3DCamera = static_cast<NiCamera*>(BSUtilities::GetObjectByName(pTableNode, "object4"));
    v17 = new NiNode(0);
    NiRefObject* v26 = (NiRefObject*)v17;
    RouletteMenu::spModel = v17;
    NiNode* v16 = RouletteMenu::spModel.m_pObject;
    v16->AttachChild(pRouletteTableGeometry, 1);
    NiNode* v15 = RouletteMenu::spModel.m_pObject;
    v15->AttachChild(pRouletteGridGeometry, 1);
    NiNode* v14 = RouletteMenu::spModel.m_pObject;
    v14->AttachChild(pCursorGeometry, 1);
    for (int32_t i_2 = 0; i_2 < 10; ++i_2) {
        NiNode* v13 = RouletteMenu::spModel.m_pObject;
        v13->AttachChild(pBetGeometry[i_2], 1);
    }
    for (int32_t i_3 = 0; i_3 < 10; ++i_3) {
        NiNode* v12 = RouletteMenu::spModel.m_pObject;
        v12->AttachChild(pBetShadowGeometry[i_3], 1);
    }
    iNextModelIndex = 3;
    ResetBetChips();
    SetBetChip();
    NiUpdateData UpdateData1(0.0f, false, false);
    RouletteMenu::spModel.m_pObject->Update(UpdateData1);
    NiTimeController* pTableAnimController = pRouletteTableGeometry->GetController(&NiControllerManager::ms_RTTI);
    if (pTableAnimController) {
        pTableAnimManager = static_cast<NiControllerManager*>(pTableAnimController);
        pTableAnimations[0] = pTableAnimManager->GetSequenceByName("Spin_1");
        pTableAnimations[1] = pTableAnimManager->GetSequenceByName("Spin_2");
        pTableAnimations[2] = pTableAnimManager->GetSequenceByName("Spin_3");
        pTableAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pRouletteTableGeometry);
    }
    if (!RouletteMenu::sp3DCamera.m_pObject) {
        RouletteMenu::sp3DCamera = Create3DCamera();
    }
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    ShadowSceneNode_->AttachChild(RouletteMenu::spModel.m_pObject, 1);
    const NiBound* WorldBound = &RouletteMenu::spModel.m_pObject->GetWorldBound();
    float fradius = WorldBound->m_fRadius * 4.0;
    NiNode* a2 = RouletteMenu::spModel.m_pObject;
    ShadowSceneNode* v6 = BSShaderManager::GetShadowSceneNode(1);
    v6->AddAllLightsFromNode_Recurse(a2, fradius);
    SetupPossibleBets();
}

NiCamera* RouletteMenu::Create3DCamera() {
    int32_t fHeight;
    NiCamera* v10;
    NiPoint3 v12;
    NiPoint3 dst;
    NiRect<float> a2;
    v10 = new NiCamera;
    ::new (&a2) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    v10->SetViewPort(a2);
    NiFrustum kFrustum(false);
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    kFrustum.m_fFar = WorldSceneGraph->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    float v8 = kFrustum.m_fFar / kFrustum.m_fNear;
    v10->SetMaxFarNearRatio(v8);
    if (BSShaderManager::bLetterbox) {
        fHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        fHeight = Renderer::GetScreenHeight();
    }
    float aspectRatio = static_cast<double>(fHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float v16 = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    kFrustum.m_fLeft = -std::tan(v16) * 0.75f;
    kFrustum.m_fRight = std::tan(v16) * 0.75f;
    double v1 = std::tan(v16);
    kFrustum.m_fBottom = -v1 * aspectRatio * 0.75f;
    double v2 = std::tan(v16);
    kFrustum.m_fTop = v2 * aspectRatio * 0.75f;
    kFrustum.m_bOrtho = false;
    v10->SetViewFrustum(kFrustum);
    NiPoint3* z = ::new (&dst) NiPoint3(0.0f, 0.0f, 1.0f);
    NiPoint3* v3 = ::new (&v12) NiPoint3(0.0f, 0.0f, 1.0f);
    v10->LookAtWorldPoint(*v3, *z);
    v10->SetMinNearPlaneDist(1.0f);
    NiUpdateData arData(0.0f, false, false);
    v10->Update(arData);
    NiFixedString apString("Surgery3DCamera");
    NiFixedString* kName = &apString;
    v10->SetName(*kName);
    return v10;
}

bool RouletteMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

// KEEP (unregistered): the exe's caller, RouletteMenu::DoClick (0x7BC870), reads ecx after the call, relying on this
// body leaving it holding this; clang does not preserve ecx.
// A luck of 5 is neutral; above it the spin is lucky, below it unlucky.
void RouletteMenu::SpinWheel() {
    iSpinAnimIndex = RandomInt(0, 3);
    iResultValue = RandomInt(0, 38);
    int32_t iLuck = iLuckValue - 5;
    if (iLuck == 0) {
        bUnlucky = false;
        bLucky = false;
    }
    else if (iLuck > 0) {
        bUnlucky = false;
        bLucky = true;
    }
    else if (iLuck < 0) {
        bUnlucky = true;
        bLucky = false;
    }
}

bool RouletteMenu::SetBet(int32_t aiIndex) {
    RouletteMenu::RouletteBet* newBet_2;
    if (iNumBetsSet >= 10 || aiIndex >= 159 || aiIndex < 0 || !iCurrentBet) {
        return false;
    }
    for (RouletteMenu::RouletteBet*& rpItem : pBetList->ItemsUntilNull()) {
        if (rpItem->betIndex == aiIndex) {
            return false;
        }
    }
    NiUpdateData update1(0.0f, false, false);
    float x = betTileInfoArray[aiIndex].location.x;
    float y = betTileInfoArray[aiIndex].location.y;
    float z = betTileInfoArray[aiIndex].location.z;
    size_t Size = 12;
    RouletteMenu::RouletteBet* newBet =
        static_cast<RouletteMenu::RouletteBet*>(ourMalloc(sizeof(RouletteMenu::RouletteBet)));
    if (newBet) {
        std::memset(newBet, 0, Size);
        newBet_2 = newBet;
    }
    else {
        newBet_2 = nullptr;
    }
    RouletteMenu::RouletteBet* newBet_3 = newBet_2;
    newBet_2->betValue = iCurrentBet;
    newBet_3->betIndex = aiIndex;
    newBet_3->modelIndex = iNextModelIndex;
    if (pBetList && !pBetList->IsEmpty()) {
        pBetList->AddHead(newBet_3);
    }
    else {
        pBetList->SetItem(newBet_3);
    }
    SetBetChip();
    ++iNumBetsSet;
    ++iNextModelIndex;
    iTotalBet += iCurrentBet;
    BSAudio* v3 = &BSAudio::QInstance();
    v3->GetSoundHandleByName("GAMERouletteIncreaseBet", 289u).Play(false);
    if (iNextModelIndex >= 13) {
        iNextModelIndex = 3;
    }
    while (!IsBetChipHidden(iNextModelIndex) && iNumBetsSet < 10) {
        if (++iNextModelIndex >= 13) {
            iNextModelIndex = 3;
        }
    }
    if (iNumBetsSet < 10 && iTotalBet < iMaxBet) {
        NiPoint3 v16 = RouletteMenu::spModel.m_pObject->GetAt(2u)->GetTranslate();
        float a3 = v16.y;
        float a2 = v16.x;
        NiAVObject* v4 = RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex));
        v4->SetTranslate(a2, a3, 0.0f);
        float a3a = v16.y;
        float a2a = v16.x;
        NiAVObject* v5 = RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex + 10));
        v5->SetTranslate(a2a, a3a, 0.0f);
        SetBetChip();
    }
    if (iCurrentBet + iTotalBet > iChipCount) {
        iCurrentBet = iChipCount - iTotalBet;
        RouletteMenu::pMe->pTiles[0]->Poke(RouletteMenu::iUT_X, 1.0f);
        RouletteMenu::pMe->pTiles[0]->Set(RouletteMenu::iUT_X, iCurrentBet);
        RouletteMenu::pMe->SetTileTextAlpha(RouletteMenu::TI_INCREASEBET_CNTRL, 128.0f);
    }
    return true;
}

// The chip's translate is read through the model-index table from pRouletteTableGeometry: model aiIndex is
// pBetGeometry[aiIndex - 3].
bool RouletteMenu::IsBetChipHidden(int32_t a1) {
    return pBetGeometry[a1 - 3]->GetTranslate().z < -3.0;
}

void RouletteMenu::SetupPossibleBets() {
    uint32_t apSrc[38];
    char v141 = 65;
    char v140 = 48;
    char v136 = 49;
    for (RouletteMenu::BetTileInfo& rInfo : betTileInfoArray) {
        NiPoint3* p_location = &rInfo.location;
        p_location->x = NiPoint3_ZERO.x;
        p_location->y = NiPoint3_ZERO.y;
        p_location->z = NiPoint3_ZERO.z;
    }
    for (int32_t j = 0; j < 159; ++j) {
        char blockName[8];
        BSsprintf(blockName, 8, "%c%c%c:0", v141, v140, v136);
        if (v141 == 66 || (v141 == 68 || v141 == 70)) {
            if (v136 % 2) {
                if (j % 24) {
                    int32_t item = 3 * ((j - 1) % 24 / 2) + (j / 24 + 1) / 2;
                    int32_t v131 = item + 3;
                    const char* v63 = sPayoutText.String();
                    int32_t v40 = v131;
                    int32_t v28 = item;
                    const char* v22 = sBetText.String();
                    BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %i,%i \n%s: 17:1", v22, v28, v40, v63);
                    betTileInfoArray[j].numValueList.AddHead(item);
                    betTileInfoArray[j].numValueList.AddHead(v131);
                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                    betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_TWO;
                }
                else {
                    int32_t v130 = 3 * ((j + 1) % 24 / 2) + (j / 24 + 1) / 2;
                    if (v141 == 68) {
                        const char* v64 = sPayoutText.String();
                        int32_t v41 = v130;
                        const char* v29 = sBetText.String();
                        BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %s,%i \n%s: 11:1", "0,00", v29, v41, v64);
                        int32_t v114 = 37;
                        betTileInfoArray[j].numValueList.AddHead(v114);
                        int32_t v113 = 0;
                        betTileInfoArray[j].numValueList.AddHead(v113);
                        betTileInfoArray[j].numValueList.AddHead(v130);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_THREE;
                    }
                    else {
                        if (v141 == 70) {
                            const char* v65 = sPayoutText.String();
                            int32_t v42 = v130;
                            const char* v30 = sBetText.String();
                            BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %s,%i \n%s: 17:1", "00", v30, v42, v65);
                            int32_t v112 = 37;
                            betTileInfoArray[j].numValueList.AddHead(v112);
                        }
                        else {
                            const char* v66 = sPayoutText.String();
                            int32_t v43 = v130;
                            const char* v31 = sBetText.String();
                            BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %s,%i \n%s: 17:1", a0_0, v31, v43, v66);
                            int32_t v111 = 0;
                            betTileInfoArray[j].numValueList.AddHead(v111);
                        }
                        betTileInfoArray[j].numValueList.AddHead(v130);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_TWO;
                    }
                }
            }
            else {
                int32_t a2 = 3 * (j % 24 / 2) + (j / 24 + 1) / 2;
                const char* v62 = sPayoutText.String();
                int32_t v39 = a2;
                const char* v27 = sBetText.String();
                BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %i \n%s: 35:1", v27, v39, v62);
                betTileInfoArray[j].numValueList.AddHead(a2);
                if (a2 % 2 || a2 >= 11) {
                    if (a2 >= 11) {
                        if ((a2 & 1) != 0 || a2 >= 19) {
                            if (a2 >= 19) {
                                if ((a2 & 1) != 0 || a2 >= 28) {
                                    if (a2 >= 28) {
                                        if ((a2 & 1) != 0) {
                                            betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_RED;
                                        }
                                        else {
                                            betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_BLACK;
                                        }
                                    }
                                    else {
                                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_RED;
                                    }
                                }
                                else {
                                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_BLACK;
                                }
                            }
                            else {
                                betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_BLACK;
                            }
                        }
                        else {
                            betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_RED;
                        }
                    }
                    else {
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_RED;
                    }
                }
                else {
                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_BLACK;
                }
                betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_ONE;
            }
        }
        else {
            if (v141 == 65) {
                if (j % 2 == 1) {
                    int32_t v129 = 3 * (j % 24 / 2) + 1;
                    const char* v67 = sPayoutText.String();
                    int32_t v44 = v129 + 2;
                    int32_t v32 = v129 + 1;
                    int32_t v23 = v129;
                    const char* v18 = sBetText.String();
                    BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %i,%i,%i \n%s: 11:1", v18, v23, v32, v44, v67);
                    betTileInfoArray[j].numValueList.AddHead(v129);
                    int32_t v110 = v129 + 1;
                    betTileInfoArray[j].numValueList.AddHead(v110);
                    int32_t v109 = v129 + 2;
                    betTileInfoArray[j].numValueList.AddHead(v109);
                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                    betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_THREE;
                }
                else {
                    if (j) {
                        int32_t v128 = 3 * ((j - 1) % 24 / 2) + 1;
                        const char* v68 = sPayoutText.String();
                        int32_t v45 = v128 + 5;
                        int32_t v33 = v128 + 4;
                        int32_t v24 = v128 + 3;
                        int32_t v19 = v128 + 2;
                        int32_t v16 = v128 + 1;
                        int32_t v15 = v128;
                        const char* v14 = sBetText.String();
                        BSsprintf(
                            betTileInfoArray[j].cBetTiles, 64, "%s: %i,%i,%i,%i,%i,%i \n%s: 5:1", v14, v15, v16, v19,
                            v24, v33, v45, v68
                        );
                        betTileInfoArray[j].numValueList.AddHead(v128);
                        int32_t v108 = v128 + 1;
                        betTileInfoArray[j].numValueList.AddHead(v108);
                        int32_t v107 = v128 + 2;
                        betTileInfoArray[j].numValueList.AddHead(v107);
                        int32_t v106 = v128 + 3;
                        betTileInfoArray[j].numValueList.AddHead(v106);
                        int32_t v105 = v128 + 4;
                        betTileInfoArray[j].numValueList.AddHead(v105);
                        int32_t v104 = v128 + 5;
                        betTileInfoArray[j].numValueList.AddHead(v104);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_SIX;
                    }
                    else {
                        const char* v69 = sPayoutText.String();
                        const char* v46 = sBetText.String();
                        BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: 0,00,1,2,3 \n%s: 6:1", v46, v69);
                        int32_t v103 = 0;
                        betTileInfoArray[j].numValueList.AddHead(v103);
                        int32_t v102 = 37;
                        betTileInfoArray[j].numValueList.AddHead(v102);
                        int32_t v101 = 1;
                        betTileInfoArray[j].numValueList.AddHead(v101);
                        int32_t v100 = 2;
                        betTileInfoArray[j].numValueList.AddHead(v100);
                        int32_t v99 = 3;
                        betTileInfoArray[j].numValueList.AddHead(v99);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_FIVE;
                    }
                }
            }
            else {
                if (v141 != 83) {
                    if (v136 % 2) {
                        if (j % 24) {
                            int32_t v124 = 3 * ((j - 1) % 24 / 2) + (j / 24 + 1) / 2;
                            int32_t v122 = v124 + 1;
                            int32_t v125 = v124 + 3;
                            int32_t v123 = v124 + 4;
                            const char* v71 = sPayoutText.String();
                            int32_t v48 = v123;
                            int32_t v35 = v125;
                            int32_t v26 = v122;
                            int32_t v20 = v124;
                            const char* v17 = sBetText.String();
                            BSsprintf(
                                betTileInfoArray[j].cBetTiles, 64, "%s: %i,%i,%i,%i \n%s: 8:1", v17, v20, v26, v35, v48,
                                v71
                            );
                            betTileInfoArray[j].numValueList.AddHead(v124);
                            betTileInfoArray[j].numValueList.AddHead(v122);
                            betTileInfoArray[j].numValueList.AddHead(v125);
                            betTileInfoArray[j].numValueList.AddHead(v123);
                            betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                            betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_FOUR;
                        }
                        else {
                            char Dst[4];
                            int32_t v119 = (v141 != 67) + 1;
                            int32_t v118 = (v141 != 67) + 2;
                            int32_t v121 = v141 != 67 ? 37 : 0;
                            if (v141 == 67) {
                                BSstrcpy(Dst, 3, a0_0);
                            }
                            else {
                                BSstrcpy(Dst, 3, "00");
                            }
                            const char* v72 = sPayoutText.String();
                            int32_t v49 = v118;
                            int32_t v36 = v119;
                            const char* v21 = sBetText.String();
                            BSsprintf(
                                betTileInfoArray[j].cBetTiles, 64, "%s: %s,%i,%i \n%s: 11:1", v21, Dst, v36, v49, v72
                            );
                            betTileInfoArray[j].numValueList.AddHead(v119);
                            betTileInfoArray[j].numValueList.AddHead(v118);
                            betTileInfoArray[j].numValueList.AddHead(v121);
                            betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                            betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_THREE;
                        }
                    }
                    else {
                        int32_t v127 = 3 * (j % 24 / 2) + (j / 24 + 1) / 2;
                        int32_t v126 = v127 + 1;
                        const char* v70 = sPayoutText.String();
                        int32_t v47 = v126;
                        int32_t v34 = v127;
                        const char* v25 = sBetText.String();
                        BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %i,%i \n%s: 17:1", v25, v34, v47, v70);
                        betTileInfoArray[j].numValueList.AddHead(v127);
                        betTileInfoArray[j].numValueList.AddHead(v126);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_TWO;
                    }
                }
            }
        }
        if (++v136 > 57) {
            v136 = 48;
            ++v140;
        }
        if (v136 == 53 && v140 == 50) {
            ++v141;
            if (j >= 143) {
                v141 = 83;
            }
            v140 = 48;
            v136 = 49;
        }
        NiAVObject* ObjectByName =
            BSUtilities::GetObjectByName(pRouletteGridGeometry, static_cast<const char*>(blockName));
        if (!ObjectByName) {
            NiFixedString apString(static_cast<const char*>(blockName));
            NiFixedString* v92 = &apString;
            ObjectByName = pRouletteGridGeometry->GetObjectByName(*v92);
        }
        ObjectByName->SetAppCulled(true);
        const NiPoint3* Center = &ObjectByName->GetWorldBound().GetCenter();
        betTileInfoArray[j].location = *Center;
        BSstrcpy(betTileInfoArray[j].faceName, 8, blockName);
    }
    NiPoint3 v142 = betTileInfoArray[0].location - betTileInfoArray[1].location;
    fRadius = v142.Length();
    fValidRadius = fRadius * 0.49000000953674316;
    for (int32_t k = 144; k < 159; ++k) {
        float v115;
        float v116;
        if (k == 144) {
            float v91;
            v115 = betTileInfoArray[144].location.x - betTileInfoArray[0].location.x;
            v116 = betTileInfoArray[144].location.y - betTileInfoArray[145].location.y;
            if (!(v115 < 0.0)) {
                v115 = v115 * 0.49000000953674316;
            }
            else {
                v115 = v115 * -0.49000000953674316;
            }
            if (!(v116 < 0.0)) {
                v116 = v116 * 0.49000000953674316;
            }
            else {
                v116 = v116 * -0.49000000953674316;
            }
            if (!(v116 > static_cast<double>(v115))) {
                v91 = v116;
            }
            else {
                v91 = v115;
            }
            fSpecialRadius[2] = v91;
            fSpecialRadius[1] = v91;
            fSpecialRadius[0] = v91;
            const char* v73 = sPayoutText.String();
            const char* v3 = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: 00 \n%s: 35:1", v3, v73);
            const char* v74 = sPayoutText.String();
            const char* v4 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 1].cBetTiles, 64, "%s: 0,00 \n%s: 17:1", v4, v74);
            const char* v75 = sPayoutText.String();
            const char* v50 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 2].cBetTiles, 64, "%s: 0 \n%s: 35:1", v50, v75);
            int32_t v97 = 37;
            betTileInfoArray[k].numValueList.AddHead(v97);
            int32_t v96 = 37;
            betTileInfoArray[k + 1].numValueList.AddHead(v96);
            int32_t v95 = 0;
            betTileInfoArray[k + 1].numValueList.AddHead(v95);
            int32_t v94 = 0;
            betTileInfoArray[k + 2].numValueList.AddHead(v94);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 1].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 2].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_ONE;
            betTileInfoArray[k + 1].betPayoutRatio = RouletteMenu::RO_TWO;
            betTileInfoArray[k + 2].betPayoutRatio = RouletteMenu::RO_ONE;
        }
        if (k == 147) {
            float v90;
            v115 = betTileInfoArray[147].location.x - betTileInfoArray[150].location.y;
            v116 = betTileInfoArray[147].location.y - betTileInfoArray[0].location.y;
            if (!(v115 < 0.0)) {
                v115 = v115 * 0.49000000953674316;
            }
            else {
                v115 = v115 * -0.49000000953674316;
            }
            if (!(v116 < 0.0)) {
                v116 = v116 * 0.49000000953674316;
            }
            else {
                v116 = v116 * -0.49000000953674316;
            }
            if (!(v116 > static_cast<double>(v115))) {
                v90 = v116;
            }
            else {
                v90 = v115;
            }
            fSpecialRadius[9] = v90;
            fSpecialRadius[6] = v90;
            fSpecialRadius[3] = v90;
            const char* v76 = sPayoutText.String();
            const char* v51 = sFirstDozenText.String();
            const char* v5 = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: %s \n%s: 2:1", v5, v51, v76);
            const char* v77 = sPayoutText.String();
            const char* v52 = sSecondDozenText.String();
            const char* v6 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 3].cBetTiles, 64, "%s: %s \n%s: 2:1", v6, v52, v77);
            const char* v78 = sPayoutText.String();
            const char* v53 = sThirdDozenText.String();
            const char* v37 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 6].cBetTiles, 64, "%s: %s \n%s: 2:1", v37, v53, v78);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 3].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 6].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 3].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 6].betPayoutRatio = RouletteMenu::RO_TWELVE;
        }
        if (k == 148) {
            float v89;
            v115 = betTileInfoArray[148].location.x - betTileInfoArray[149].location.y;
            v116 = betTileInfoArray[148].location.y - betTileInfoArray[3].location.y;
            if (!(v115 < 0.0)) {
                v115 = v115 * 0.49000000953674316;
            }
            else {
                v115 = v115 * -0.49000000953674316;
            }
            if (!(v116 < 0.0)) {
                v116 = v116 * 0.49000000953674316;
            }
            else {
                v116 = v116 * -0.49000000953674316;
            }
            if (!(v116 > static_cast<double>(v115))) {
                v89 = v116;
            }
            else {
                v89 = v115;
            }
            fSpecialRadius[11] = v89;
            fSpecialRadius[10] = v89;
            fSpecialRadius[8] = v89;
            fSpecialRadius[7] = v89;
            fSpecialRadius[5] = v89;
            fSpecialRadius[4] = v89;
            const char* v79 = sPayoutText.String();
            const char* v7 = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: 1-18 \n%s: 1:1", v7, v79);
            const char* v80 = sPayoutText.String();
            const char* v54 = sEvenText.String();
            const char* v8 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 1].cBetTiles, 64, "%s: %s \n%s: 1:1", v8, v54, v80);
            const char* v81 = sPayoutText.String();
            const char* v55 = sRedText.String();
            const char* v9 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 3].cBetTiles, 64, "%s: %s \n%s: 1:1", v9, v55, v81);
            const char* v82 = sPayoutText.String();
            const char* v56 = gs_sBlacktext.String();
            const char* v10 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 4].cBetTiles, 64, "%s: %s \n%s: 1:1", v10, v56, v82);
            const char* v83 = sPayoutText.String();
            const char* v57 = gs_sOddtext.String();
            const char* v11 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 6].cBetTiles, 64, "%s: %s \n%s: 1:1", v11, v57, v83);
            const char* v84 = sPayoutText.String();
            const char* v58 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 7].cBetTiles, 64, "%s: 19-36 \n%s: 1:1", v58, v84);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 1].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 3].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 4].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 6].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 7].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_EIGHTEEN;
            betTileInfoArray[k + 1].betPayoutRatio = RouletteMenu::RO_EIGHTEEN;
            betTileInfoArray[k + 3].betPayoutRatio = RouletteMenu::RO_EIGHTEEN;
            betTileInfoArray[k + 4].betPayoutRatio = RouletteMenu::RO_EIGHTEEN;
            betTileInfoArray[k + 6].betPayoutRatio = RouletteMenu::RO_EIGHTEEN;
            betTileInfoArray[k + 7].betPayoutRatio = RouletteMenu::RO_EIGHTEEN;
        }
        if (k == 156) {
            float v88;
            v115 = betTileInfoArray[156].location.x - betTileInfoArray[23].location.x;
            v116 = betTileInfoArray[156].location.y - betTileInfoArray[157].location.y;
            if (!(v115 < 0.0)) {
                v115 = v115 * 0.49000000953674316;
            }
            else {
                v115 = v115 * -0.49000000953674316;
            }
            if (!(v116 < 0.0)) {
                v116 = v116 * 0.49000000953674316;
            }
            else {
                v116 = v116 * -0.49000000953674316;
            }
            if (!(v116 > static_cast<double>(v115))) {
                v88 = v116;
            }
            else {
                v88 = v115;
            }
            fSpecialRadius[14] = v88;
            fSpecialRadius[13] = v88;
            fSpecialRadius[12] = v88;
            const char* v85 = sPayoutText.String();
            const char* v59 = sTwoToOneText.String();
            const char* v12 = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: %s \n%s: 2:1", v12, v59, v85);
            const char* v86 = sPayoutText.String();
            const char* v60 = sTwoToOneText.String();
            const char* v13 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 1].cBetTiles, 64, "%s: %s \n%s: 2:1", v13, v60, v86);
            const char* v87 = sPayoutText.String();
            const char* v61 = sTwoToOneText.String();
            const char* v38 = sBetText.String();
            BSsprintf(betTileInfoArray[k + 2].cBetTiles, 64, "%s: %s \n%s: 2:1", v38, v61, v87);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 1].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 2].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 1].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 2].betPayoutRatio = RouletteMenu::RO_TWELVE;
        }
    }
    apSrc[0] = 14;
    apSrc[1] = 2;
    apSrc[2] = 0;
    apSrc[3] = 28;
    apSrc[4] = 9;
    apSrc[5] = 26;
    apSrc[6] = 30;
    apSrc[7] = 11;
    apSrc[8] = 7;
    apSrc[9] = 20;
    apSrc[10] = 32;
    apSrc[11] = 17;
    apSrc[12] = 5;
    apSrc[13] = 22;
    apSrc[14] = 34;
    apSrc[15] = 15;
    apSrc[16] = 3;
    apSrc[17] = 24;
    apSrc[18] = 36;
    apSrc[19] = 13;
    apSrc[20] = 1;
    apSrc[21] = 37;
    apSrc[22] = 27;
    apSrc[23] = 10;
    apSrc[24] = 25;
    apSrc[25] = 29;
    apSrc[26] = 12;
    apSrc[27] = 8;
    apSrc[28] = 19;
    apSrc[29] = 31;
    apSrc[30] = 18;
    apSrc[31] = 6;
    apSrc[32] = 21;
    apSrc[33] = 33;
    apSrc[34] = 16;
    apSrc[35] = 4;
    apSrc[36] = 23;
    apSrc[37] = 35;
    ourMemcpy(iNumberStrip, apSrc, sizeof(iNumberStrip));
    bRunScriptOnExit = false;
}

bool RouletteMenu::RemoveBet(int32_t aiIndex) {
    BSSimpleList<RouletteMenu::RouletteBet*>* blist;
    if (aiIndex >= 159 || aiIndex < 0) {
        return false;
    }
    for (blist = pBetList;; blist = blist->m_pkNext) {
        if (!blist || !blist->m_item) {
            return false;
        }
        int32_t comp = blist->m_item->betIndex;
        if (comp == aiIndex) {
            break;
        }
    }
    iTotalBet -= blist->m_item->betValue;
    RemoveBetNif(blist->m_item->modelIndex);
    if (iNumBetsSet == 10) {
        iNextModelIndex = blist->m_item->modelIndex;
        SetBetChip();
        float a4 = (&RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex))->GetTranslate())->z;
        float a3 = fyTranslate;
        float a2 = fxTranslate;
        NiAVObject* v3 = RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex));
        v3->SetTranslate(a2, a3, a4);
        float a3a = fyTranslate;
        float a2a = fxTranslate;
        NiAVObject* v4 = RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex + 10));
        v4->SetTranslate(a2a, a3a, 0.0f);
        NiUpdateData apMemory(0.0f, 0, 0);
        NiAVObject* v5 = RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex));
        v5->Update(apMemory);
        NiUpdateData v15(0.0f, 0, 0);
        NiAVObject* v6 = RouletteMenu::spModel.m_pObject->GetAt(static_cast<uint32_t>(iNextModelIndex + 10));
        v6->Update(v15);
    }
    --iNumBetsSet;
    RouletteMenu::RouletteBet* delBet = blist->m_item;
    blist->RemoveHead();
    ourFree(delBet);
    delBet = nullptr;
    return true;
}

// Geometry pointers are indexed from pRouletteTableGeometry by model index: model a1 is pBetGeometry[a1 - 3] and its
// shadow pBetShadowGeometry[a1 - 3].
void RouletteMenu::RemoveBetNif(int32_t a1) {
    pBetGeometry[a1 - 3]->SetTranslate(0.0f, 0.0f, -5.0f);
    pBetShadowGeometry[a1 - 3]->SetTranslate(0.0f, 0.0f, -5.0f);
    NiUpdateData apMemory(0.0f, 0, 0);
    pBetGeometry[a1 - 3]->Update(apMemory);
    NiUpdateData v5(0.0f, 0, 0);
    pBetShadowGeometry[a1 - 3]->Update(v5);
}

void RouletteMenu::SetWinningSlotLuckyAndAnimToPlay() {
    iSpinAnimIndex = RandomInt(0, 3);
    iResultValue = RandomInt(0, 38);
    int32_t v3 = iLuckValue - 5;
    if (v3 == 0) {
        bUnlucky = false;
        bLucky = false;
    }
    else if (v3 > 0) {
        bUnlucky = false;
        bLucky = true;
    }
    else if (v3 < 0) {
        bUnlucky = true;
        bLucky = false;
    }
}

void RouletteMenu::DrawScene() {
    NiCamera* v6;
    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
    Renderer_->ClearBuffer(nullptr, 4u);
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* a1 = WorldSceneGraph->pCuller;
    a1->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* apCamera = RouletteMenu::sp3DCamera.m_pObject;
    NiCamera::HandleEyeFinityFOVScale(apCamera, 0.15f);
    InterfaceManager* v1 = InterfaceManager::Instance();
    NiPointer<NiRefObject> apMemory(static_cast<NiRefObject*>(v1->Get3dAccumulator()));
    NiAccumulator* afScale = static_cast<NiAccumulator*>(apMemory.m_pObject);
    a1->SetAccumulator(afScale);
    if (apCamera) {
        v6 = apCamera;
    }
    else {
        v6 = WorldSceneGraph->GetCamera();
    }
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    BSShaderUtil::AccumulateScene(v6, ShadowSceneNode_, *a1);
    BSShaderAccumulator* v3 = static_cast<BSShaderAccumulator*>(apMemory.m_pObject);
    BSShaderUtil::RenderScene(v6, v3, false);
    a1->SetAccumulator(nullptr);
    apMemory = static_cast<NiRefObject*>(nullptr);
    a1->PopCullMode();
}

void RouletteMenu::Close() {
    InterfaceManager* singleton;
    Tile* processLevel;
    if (RouletteMenu::pMe) {
        singleton = InterfaceManager::Instance();
        processLevel = singleton->GetCursorTile();
        processLevel->Set(4009, 255.0f, true);
        RouletteMenu::pMe->eRouletteState = RO_CLOSING_MENU;
    }
}

// Finds (or adds at the head of the player's list) the casino's record, refuses a barred, broke or under-winning player
// with a HUD message and a sound, then builds the menu from roulette_menu.xml and fills its tiles. With no casino list
// at all, SetItem is called on null, as the game does. abDebugMode is not read. The settings and the remaining time are
// read in right-to-left order.
Tile* RouletteMenu::Create(
    TESCasino* apCasino,
    int32_t aiMinBet,
    int32_t aiMaxBet,
    int32_t aiMinWinnings,
    bool abDebugMode
) {
    Tile* pOldMenu = Menu::GetMenuByClass(Interface::eRouletteMenu);
    InterfaceManager* pManager = InterfaceManager::Instance();
    pCasino = apCasino;
    BSSimpleList<CasinoData*>* pIter = pPlayer->GetCasinoDataList();
    while (pIter && pIter->GetItem()) {
        uint32_t uiCasinoID = pCasino->GetFormID();
        if (uiCasinoID == pIter->GetItem()->casinoID) {
            pCasinoData = pIter->GetItem();
            break;
        }
        if (!pIter->GetNext()) {
            break;
        }
        pIter = pIter->GetNext();
    }
    if (!pCasinoData) {
        void* pMemory = ourMalloc(sizeof(CasinoData));
        CasinoData* pNewData = pMemory ? ::new (pMemory) CasinoData : nullptr;
        pNewData->casinoID = pCasino->GetFormID();
        pNewData->iTotalWinnings = 0;
        BSSimpleList<CasinoData*>* pList = pPlayer->GetCasinoDataList();
        if (!pList || pList->IsEmpty()) {
            pList->SetItem(pNewData);
        }
        else {
            pList->AddHead(pNewData);
        }
        pCasinoData = pNewData;
    }
    if (pPlayer->CasinoAntiCheatCheck()) {
        char acMessage[260];
        int32_t iTimeRemaining = pPlayer->GetCasinoCheatTimeRemaining();
        const char* pTimeText = sAntiCheatTimeRemainingText.String();
        const char* pCheatText = sRouletteAntiCheatText.String();
        BSsprintf(acMessage, 0x104, "%s\n%s%i", pCheatText, pTimeText, iTimeRemaining);
        Interface::ShowMessage(
            acMessage, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio::QInstance()
            .GetSoundHandleByName(
                "UIPopUpMessageGeneral",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            )
            .Play(false);
        return nullptr;
    }
    // Takes the casino's data before it reads the record.
    CASINO_DATA* pData = pCasino->GetData();
    if (pCasinoData->iTotalWinnings >= pData->iMaxWinnings) {
        char acMessage[260];
        // The setting's text is the format itself.
        BSsprintf(acMessage, 0x104, sGamblingBannedText.String());
        Interface::ShowMessage(
            acMessage, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio::QInstance()
            .GetSoundHandleByName(
                "UIPopUpMessageGeneral",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            )
            .Play(false);
        return nullptr;
    }
    if (pPlayer->GetInventoryObjectCount(pCasino->GetChipType()) < aiMinBet) {
        char acMessage[260];
        BSsprintf(acMessage, 0x104, sGamblingBrokeText.String());
        Interface::ShowMessage(
            acMessage, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio::QInstance()
            .GetSoundHandleByName(
                "UIPopUpMessageGeneral",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            )
            .Play(false);
        return nullptr;
    }
    if (pCasinoData->iTotalWinnings < aiMinWinnings && aiMinWinnings) {
        char acMessage[260];
        BSsprintf(acMessage, 0x104, sGamblingMinWinText.String());
        Interface::ShowMessage(
            acMessage, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio::QInstance()
            .GetSoundHandleByName(
                "UIPopUpMessageGeneral",
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            )
            .Play(false);
        return nullptr;
    }
    if (pOldMenu) {
        delete pOldMenu;
    }
    Tile* pFile = pManager->GetMenusRoot()->ReadFile("Data\\Menus\\roulette_menu.xml");
    RouletteMenu::pMe = static_cast<RouletteMenu*>(pFile->GetMenu());
    if (!RouletteMenu::pMe || RouletteMenu::pMe->GetClass() != GetClassStatic()) {
        // A menu of another class is deleted when it has a menu tile.
        if (RouletteMenu::pMe && RouletteMenu::pMe->GetMenuTile()) {
            delete RouletteMenu::pMe;
        }
        return nullptr;
    }
    RouletteMenu::pMe->iMinBet = aiMinBet;
    RouletteMenu::pMe->iMaxBet = aiMaxBet;
    RouletteMenu::pMe->iCurrentBet = aiMinBet;
    RouletteMenu::pMe->iChipCount = pPlayer->GetInventoryObjectCount(pCasino->GetChipType());
    RouletteMenu::pMe->iLuckValue = static_cast<ActorValueOwner*>(pPlayer)->GetClampedActorValue(ActorValue::LUCK);
    RouletteMenu::pMe->iTotalBet = 0;
    RouletteMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pFile), false);
    if (!RouletteMenu::pMe->IsWellFormedMenu()) {
        Warning("MENUS: Roulette Menu Creation Failed... Are your menu and art resources up to date?");
        RouletteMenu::pMe->eRouletteState = RO_CLOSING_MENU;
        return nullptr;
    }
    // Stacking types 6006 and 102 put the menu on top (double compares; NaN matches neither).
    if (pFile->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
        pFile->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
        pFile->Set(Tile::kTileValue_Depth, Tile::GetMaximumDepth(), true);
    }
    RouletteMenu::pMe->pTiles[0]->Set(iUT_Value, sCurrentBetText.String(), true);
    RouletteMenu::pMe->pTiles[0]->Poke(iUT_X, 1.0f);
    RouletteMenu::pMe->pTiles[0]->Set(iUT_X, RouletteMenu::pMe->iCurrentBet);
    RouletteMenu::pMe->pTiles[10]->Set(iUT_Value, sTotalBetText.String(), true);
    RouletteMenu::pMe->pTiles[10]->Poke(iUT_X, 1.0f);
    RouletteMenu::pMe->pTiles[10]->Set(iUT_X, 0);
    RouletteMenu::pMe->pTiles[1]->Set(iUT_Value, sChipCountText.String(), true);
    RouletteMenu::pMe->pTiles[1]->Poke(iUT_X, 1.0f);
    RouletteMenu::pMe->pTiles[1]->Set(iUT_X, RouletteMenu::pMe->iChipCount);
    // The casino's name, or its editor ID when it has none.
    char acEarnings[64];
    if (BSstrlen(static_cast<TESFullName*>(pCasino)->GetFullName())) {
        const char* pEarningsText = sCasinoEarningsText.String();
        BSsprintf(acEarnings, 0x40, "%s %s", static_cast<TESFullName*>(pCasino)->GetFullName(), pEarningsText);
    }
    else {
        const char* pEarningsText = sCasinoEarningsText.String();
        BSsprintf(acEarnings, 0x40, "%s %s", pCasino->GetFormEditorID(), pEarningsText);
    }
    RouletteMenu::pMe->pTiles[2]->Set(iUT_Value, acEarnings, true);
    Tile::GetTileByName(RouletteMenu::pMe->pTiles[2], "ROM_TotalEarningsValue")->Set(Tile::kTileValue_Visible, 0);
    Tile::GetTileByName(RouletteMenu::pMe->pTiles[2], "ROM_TotalEarningsValue")->AddNeedsUpdate(Tile::CHANGE_COLOR);
    RouletteMenu::pMe->UpdateEarningsString();
    RouletteMenu::pMe->pTiles[3]->Set(Tile::kTileValue_String, sPlaceBetText.String(), true);
    RouletteMenu::pMe->pTiles[4]->Set(Tile::kTileValue_String, sRemoveBetText.String(), true);
    RouletteMenu::pMe->pTiles[5]->Set(Tile::kTileValue_String, sFinishBetText.String(), true);
    RouletteMenu::pMe->pTiles[6]->Set(Tile::kTileValue_String, sIncreaseBetText.String(), true);
    RouletteMenu::pMe->pTiles[7]->Set(Tile::kTileValue_String, sDecreaseBetText.String(), true);
    RouletteMenu::pMe->pTiles[8]->Set(Tile::kTileValue_String, sExit.String(), true);
    RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_String, " ", true);
    RouletteMenu::pMe->pTiles[9]->Set(Tile::kTileValue_Visible, 1);
    RouletteMenu::pMe->SetTileTextAlpha(TI_FINISHBETS_CNTRL, 128.0f);
    RouletteMenu::pMe->SetTileTextAlpha(TI_REMOVEBET_CNTRL, 128.0f);
    RouletteMenu::pMe->SetTileTextAlpha(TI_DECREASEBET_CNTRL, 128.0f);
    RouletteMenu::pMe->Prepare3DElements();
    RouletteMenu::pMe->PrepForVisibility(false);
    return pFile;
}

int32_t RouletteMenu::GetClassStatic() {
    return Interface::eRouletteMenu;
}
