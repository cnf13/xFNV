#include "Fallout/Interface/Menus/RouletteMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

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

// The setting key is misspelt sFinsihBetText.
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
    RouletteMenu::iUT_X = Tile::AddUserTrait("_X", -1);
}

RouletteMenu::~RouletteMenu() {
    // MAX_PATH bytes.
    char modelPath[260];
    SetLastAntiCheatTime();
    pModelLoader->ReleaseFile(RouletteMenu::pCasino->GetModel(9)->GetModel());
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_Roulette\\NV_Roulette-Bet.NIF");
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_Roulette\\NV_Roulette-Points.NIF");
    pModelLoader->ReleaseFile(RouletteMenu::pCasino->GetModel(6)->GetModel());
    char* tempstr = nullptr;
    BSstrcpy(modelPath, 260, RouletteMenu::pCasino->GetModel(6)->GetModel());
    for (int32_t j = 0; j < BSstrlen(modelPath); ++j) {
        if (modelPath[j] == '.') {
            tempstr = &modelPath[j];
            break;
        }
    }
    if (tempstr) {
        BSstrcpy(tempstr, 15, "_Shadow.NIF");
        pModelLoader->ReleaseFile(modelPath);
    }
    RouletteMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(RouletteMenu::spModel.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    RouletteMenu::spModel->DetachAllChildren();
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

void RouletteMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 10) {
        pTiles[aiID] = apTile;
    }
}

void RouletteMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (eRouletteState == RouletteMenu::RO_IDLE) {
        switch (aiID) {
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
                int32_t i = GetBetIndex();
                if (i < 159 && RemoveBet(i)) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMERouletteDecreaseBet", 289u).Play(false);
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
                if (pBetList && !pBetList->IsEmpty()) {
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
                            iCurrentBet = iCurrentBet >= 500 ? iCurrentBet + 100 : iCurrentBet + 25;
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
                    BSAudio::QInstance().GetSoundHandleByName("GAMERouletteValidBet", 289u).Play(false);
                }
                RouletteMenu::pMe->pTiles[0]->Poke(RouletteMenu::iUT_X, 1.0f);
                RouletteMenu::pMe->pTiles[0]->Set(RouletteMenu::iUT_X, iCurrentBet);
                break;
            }
            case 7: {
                int32_t iTempBet = iCurrentBet;
                if (iCurrentBet > 10) {
                    if (iCurrentBet > 40) {
                        if (iCurrentBet > 100) {
                            if (iCurrentBet > 500) {
                                iCurrentBet = iCurrentBet % 100 ? 100 * (iCurrentBet / 100) : iCurrentBet - 100;
                            }
                            else {
                                iCurrentBet = iCurrentBet % 25 ? 25 * (iCurrentBet / 25) : iCurrentBet - 25;
                            }
                        }
                        else {
                            iCurrentBet = iCurrentBet % 10 ? 10 * (iCurrentBet / 10) : iCurrentBet - 10;
                        }
                    }
                    else {
                        iCurrentBet = iCurrentBet % 5 ? 5 * (iCurrentBet / 5) : iCurrentBet - 5;
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
                if (iTempBet != iCurrentBet) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMERouletteValidBet", 289u).Play(false);
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
                break;
            }
        }
    }
}

void RouletteMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 0.0f, true);
}

void RouletteMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 40.0f, true);
}

// Runs the state machine: the spin animation, paying out the bets, moving the cursor chip over the table while idle,
// and settling the chips with the player on closing. The payout test reads the debug axis (always 0) and ignores its
// 0x7FFF maximum.
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
                this->pTableAnimManager
                    ->ActivateSequence(this->pTableAnimations[this->iSpinAnimIndex], 0, false, 1.0f, 0.0f, nullptr);
                this->bCleanTable = true;
                this->ShiftWheelTexture(this->iResultValue);
            }
            NiUpdateData kSpinUpdate(ftotalStateSecs, true, false);
            spModel->GetAt(0)->Update(kSpinUpdate);
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
                this->fxTranslate = static_cast<float>(
                    this->fxTranslate - static_cast<double>(fxAxisRatio) * movementDamper
                );
                this->fyTranslate = static_cast<float>(
                    this->fyTranslate - static_cast<double>(fyAxisRatio) * movementDamper
                );
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
                NiUpdateData kCursorUpdate(0.0f, false, false);
                spModel->GetAt(2)->Update(kCursorUpdate);
                if (this->iNumBetsSet < 10) {
                    float fChipZ = spModel->GetAt(this->iNextModelIndex)->GetTranslate().z;
                    spModel->GetAt(this->iNextModelIndex)->SetTranslate(this->fxTranslate, this->fyTranslate, fChipZ);
                    spModel->GetAt(this->iNextModelIndex + 10)
                        ->SetTranslate(this->fxTranslate, this->fyTranslate, 0.0f);
                    NiUpdateData kChipUpdate(0.0f, false, false);
                    spModel->GetAt(this->iNextModelIndex)->Update(kChipUpdate);
                    NiUpdateData kShadowUpdate(0.0f, false, false);
                    spModel->GetAt(this->iNextModelIndex + 10)->Update(kShadowUpdate);
                }
                int32_t iBetIndex = this->GetBetIndex();
                if (iBetIndex < 159) {
                    if (this->pCurrentBetFace) {
                        this->pCurrentBetFace->SetAppCulled(true);
                    }
                    this->pCurrentBetFace = BSUtilities::GetObjectByName(
                        this->pRouletteGridGeometry,
                        this->betTileInfoArray[iBetIndex].faceName
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
                    strMessage,
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                    nullptr,
                    2.0f,
                    false
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
                    pCasino->GetChipType(),
                    nullptr,
                    -iChange,
                    false,
                    false,
                    nullptr,
                    nullptr,
                    nullptr,
                    true,
                    false
                );
                alignas(BSStringT<char>) uint8_t aStringStorage[sizeof(BSStringT<char>)];
                BSStringT<char>* pstring = ::new (aStringStorage) BSStringT<char>;
                const char* pRemove = sRemoveItemfromInventory.String();
                const char* pPlural = sPlural.String();
                const char* pName = TESFullName::GetFullName(pCasino->GetChipType());
                pstring->SPrintF("%i %s%s %s", -iChange, pName, pPlural, pRemove);
                const char* pSound = pPlayer->GetPickUpSoundName(pCasino->GetChipType(), true, false);
                Interface::ShowMessage(
                    pstring->String(),
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                    pSound,
                    2.0f,
                    false
                );
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
                        pstring->String(),
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
                        pSound,
                        2.0f,
                        false
                    );
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
    char acEarningsText[264];
    int32_t iTotalWinnings = RouletteMenu::pCasinoData->iTotalWinnings;
    const char* pEarnings = sCasinoEarningsText.String();
    BSsprintf(acEarningsText, 260u, "%s %s%i", RouletteMenu::pCasino->GetFullName(), pEarnings, iTotalWinnings);
    int32_t ilen = BSstrlen(acEarningsText);
    if (ilen < 23) {
        pEarnings = sCasinoEarningsText.String();
        BSsprintf(acEarningsText, 260u, "%s %s", RouletteMenu::pCasino->GetFullName(), pEarnings);
        for (int32_t i = 0; i < 23 - ilen; ++i) {
            acEarningsText[ilen - 1 + i] = ' ';
            acEarningsText[i + ilen] = '\0';
        }
        BSsprintf(acEarningsText, 260u, "%s%i", acEarningsText, RouletteMenu::pCasinoData->iTotalWinnings);
    }
    pTiles[2]->Set(RouletteMenu::iUT_Value, acEarningsText, true);
}

int32_t RouletteMenu::GetBetIndex() {
    int32_t i;
    for (i = 0; i < 144; ++i) {
        float fDistance = (pCursorGeometry->GetWorldTranslate() - betTileInfoArray[i].location).Length();
        if (fValidRadius >= static_cast<double>(fDistance)) {
            break;
        }
    }
    if (i == 144) {
        while (i < 159) {
            float fDistance = (pCursorGeometry->GetWorldTranslate() - betTileInfoArray[i].location).Length();
            if (fSpecialRadius[i - 144] >= static_cast<double>(fDistance)) {
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
    for (i = 0; i < 38 && iNumberStrip[i] != aiResult; ++i) {}
    i = 38 - i;
    if (iSpinAnimIndex) {
        if (iSpinAnimIndex == 1) {
            i += 3;
        }
        else if (iSpinAnimIndex == 2) {
            i += 10;
        }
    }
    else {
        i += 5;
    }
    std::memcpy(&akRot, &NiMatrix3::IDENTITY, sizeof(akRot));
    float fAngle = static_cast<float>(i * 9.473684210526315 * 0.01745329238474369);
    akRot.MakeZRotation(fAngle);
    NiAVObject* pWheel = BSUtilities::GetObjectByName(pRouletteTableGeometry, "Roulette_Wheel:2");
    if (pWheel) {
        pWheel->SetRotate(akRot);
    }
}

void RouletteMenu::SetTileTextAlpha(RouletteMenu::TileIndex TI, float alpha) {
    NiTListItem<Tile*>* iter = RouletteMenu::pMe->pTiles[TI]->xChildren.m_pkHead;
    for (NiTPointerList<Tile*>* pTileList =
             static_cast<NiTPointerList<Tile*>*>(&RouletteMenu::pMe->pTiles[TI]->xChildren);
         iter && pTileList && pTileList->GetHead();
         iter = pTileList->GetNextPos(iter)) {
        pTileList->Get(iter)->Set(4009, alpha, true);
    }
}

void RouletteMenu::ResetBetChips() {
    for (int32_t i = 0; i < 10; ++i) {
        pBetGeometry[i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), -5.0f);
        pBetShadowGeometry[i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), -5.0f);
    }
    NiUpdateData kUpdateData(0.0, false, false);
    RouletteMenu::spModel->Update(kUpdateData);
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
    float fChipZ = static_cast<double>(level) * 0.18000000000000002;
    pBetGeometry[iNextModelIndex - 3]->SetTranslate(kTranslate.x, kTranslate.y, fChipZ);
    pBetShadowGeometry[iNextModelIndex - 3]->SetTranslate(kTranslate.x, kTranslate.y, 0.0f);
    NiUpdateData update1(0.0f, false, false);
    RouletteMenu::spModel->Update(update1);
}

void RouletteMenu::Prepare3DElements() {
    char modelPath[252];
    const char* pTableModel = RouletteMenu::pCasino->GetModel(9)->GetModel();
    pRouletteTableGeometry = pModelLoader->LoadFile(pTableModel, LOD_MULT_NONE, true, false, false, false);
    pRouletteGridGeometry = pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_Roulette\\NV_Roulette-Points.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    pCursorGeometry = pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_Roulette\\NV_Roulette-Bet.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    const char* pChipModel = RouletteMenu::pCasino->GetModel(6)->GetModel();
    pBetGeometry[0] = pModelLoader->LoadFile(pChipModel, LOD_MULT_NONE, true, false, false, false);
    pRouletteGridGeometry->SetTranslate(0.0f, 0.0f, 0.1f);
    pCursorGeometry->SetTranslate(0.0f, 0.0f, 0.1f);
    pValidBetFace = BSUtilities::GetObjectByName(pCursorGeometry, "Bet_Selector:0");
    pInvalidBetFace = BSUtilities::GetObjectByName(pCursorGeometry, "Bet_Invalid:0");
    pInvalidBetFace->SetAppCulled(true);
    char* tempstr = nullptr;
    BSstrcpy(modelPath, 260, RouletteMenu::pCasino->GetModel(6)->GetModel());
    for (int32_t j = 0; j < BSstrlen(modelPath); ++j) {
        if (modelPath[j] == 46) {
            tempstr = &modelPath[j];
            break;
        }
    }
    if (tempstr) {
        BSstrcpy(tempstr, 15, "_Shadow.NIF");
        pBetShadowGeometry[0] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    }
    for (int32_t i = 1; i < 10; ++i) {
        pBetGeometry[i] = static_cast<NiAVObject*>(pBetGeometry[0]->Clone());
        pBetShadowGeometry[i] = static_cast<NiAVObject*>(pBetShadowGeometry[0]->Clone());
    }
    NiNode* pTableNode = static_cast<NiNode*>(pRouletteTableGeometry);
    RouletteMenu::sp3DCamera = static_cast<NiCamera*>(BSUtilities::GetObjectByName(pTableNode, "object4"));
    RouletteMenu::spModel = new NiNode(0);
    RouletteMenu::spModel->AttachChild(pRouletteTableGeometry, 1);
    RouletteMenu::spModel->AttachChild(pRouletteGridGeometry, 1);
    RouletteMenu::spModel->AttachChild(pCursorGeometry, 1);
    for (int32_t i = 0; i < 10; ++i) {
        RouletteMenu::spModel->AttachChild(pBetGeometry[i], 1);
    }
    for (int32_t i = 0; i < 10; ++i) {
        RouletteMenu::spModel->AttachChild(pBetShadowGeometry[i], 1);
    }
    iNextModelIndex = 3;
    ResetBetChips();
    SetBetChip();
    NiUpdateData kUpdateData(0.0f, false, false);
    RouletteMenu::spModel->Update(kUpdateData);
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
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(RouletteMenu::spModel.m_pObject, 1);
    float fradius = RouletteMenu::spModel->GetWorldBound().m_fRadius * 4.0;
    NiNode* pModel = RouletteMenu::spModel.m_pObject;
    BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pModel, fradius);
    SetupPossibleBets();
}

NiCamera* RouletteMenu::Create3DCamera() {
    NiRect<float> kViewPort;
    NiCamera* pCamera = new NiCamera;
    ::new (&kViewPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pCamera->SetViewPort(kViewPort);
    NiFrustum kFrustum(false);
    kFrustum.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    pCamera->SetMaxFarNearRatio(kFrustum.m_fFar / kFrustum.m_fNear);
    int32_t iHeight = BSShaderManager::bLetterbox ? BSShaderManager::iLetterboxHeight : Renderer::GetScreenHeight();
    float aspectRatio = static_cast<double>(iHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fViewAngle = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    kFrustum.m_fLeft = -std::tan(fViewAngle) * 0.75f;
    kFrustum.m_fRight = std::tan(fViewAngle) * 0.75f;
    double dTan = std::tan(fViewAngle);
    kFrustum.m_fBottom = -dTan * aspectRatio * 0.75f;
    dTan = std::tan(fViewAngle);
    kFrustum.m_fTop = dTan * aspectRatio * 0.75f;
    kFrustum.m_bOrtho = false;
    pCamera->SetViewFrustum(kFrustum);
    NiPoint3 kWorldUp(0.0f, 0.0f, 1.0f);
    NiPoint3 kWorldPt(0.0f, 0.0f, 1.0f);
    pCamera->LookAtWorldPoint(kWorldPt, kWorldUp);
    pCamera->SetMinNearPlaneDist(1.0f);
    NiUpdateData kUpdateData(0.0f, false, false);
    pCamera->Update(kUpdateData);
    NiFixedString kName("Surgery3DCamera");
    pCamera->SetName(kName);
    return pCamera;
}

bool RouletteMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

// KEEP (unregistered): the caller, RouletteMenu::DoClick, relies on this body leaving the this pointer in its
// register after the call; clang does not preserve that register.
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
    if (iNumBetsSet >= 10 || aiIndex >= 159 || aiIndex < 0 || !iCurrentBet) {
        return false;
    }
    for (RouletteMenu::RouletteBet*& rpItem : pBetList->ItemsUntilNull()) {
        if (rpItem->betIndex == aiIndex) {
            return false;
        }
    }
    NiUpdateData update1(0.0f, false, false);
    RouletteMenu::RouletteBet* newBet = static_cast<RouletteMenu::RouletteBet*>(
        ourMalloc(sizeof(RouletteMenu::RouletteBet))
    );
    if (newBet) {
        std::memset(newBet, 0, sizeof(RouletteMenu::RouletteBet));
    }
    newBet->betValue = iCurrentBet;
    newBet->betIndex = aiIndex;
    newBet->modelIndex = iNextModelIndex;
    if (pBetList && !pBetList->IsEmpty()) {
        pBetList->AddHead(newBet);
    }
    else {
        pBetList->SetItem(newBet);
    }
    SetBetChip();
    ++iNumBetsSet;
    ++iNextModelIndex;
    iTotalBet += iCurrentBet;
    BSAudio::QInstance().GetSoundHandleByName("GAMERouletteIncreaseBet", 289u).Play(false);
    if (iNextModelIndex >= 13) {
        iNextModelIndex = 3;
    }
    while (!IsBetChipHidden(iNextModelIndex) && iNumBetsSet < 10) {
        if (++iNextModelIndex >= 13) {
            iNextModelIndex = 3;
        }
    }
    if (iNumBetsSet < 10 && iTotalBet < iMaxBet) {
        NiPoint3 kCursorPos = RouletteMenu::spModel->GetAt(2u)->GetTranslate();
        RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex))
            ->SetTranslate(kCursorPos.x, kCursorPos.y, 0.0f);
        RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex + 10))
            ->SetTranslate(kCursorPos.x, kCursorPos.y, 0.0f);
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
bool RouletteMenu::IsBetChipHidden(int32_t aiIndex) {
    return pBetGeometry[aiIndex - 3]->GetTranslate().z < -3.0;
}

void RouletteMenu::SetupPossibleBets() {
    uint32_t auiNumberStrip[38];
    char cLetter = 'A';
    char cTens = '0';
    char cOnes = '1';
    for (RouletteMenu::BetTileInfo& rInfo : betTileInfoArray) {
        rInfo.location = NiPoint3::ZERO;
    }
    for (int32_t j = 0; j < 159; ++j) {
        char blockName[8];
        BSsprintf(blockName, 8, "%c%c%c:0", cLetter, cTens, cOnes);
        if (cLetter == 'B' || cLetter == 'D' || cLetter == 'F') {
            if (cOnes % 2) {
                if (j % 24) {
                    int32_t iNumber = 3 * ((j - 1) % 24 / 2) + (j / 24 + 1) / 2;
                    int32_t iAdjacent = iNumber + 3;
                    const char* pPayout = sPayoutText.String();
                    const char* pBet = sBetText.String();
                    BSsprintf(
                        betTileInfoArray[j].cBetTiles,
                        64,
                        "%s: %i,%i \n%s: 17:1",
                        pBet,
                        iNumber,
                        iAdjacent,
                        pPayout
                    );
                    betTileInfoArray[j].numValueList.AddHead(iNumber);
                    betTileInfoArray[j].numValueList.AddHead(iAdjacent);
                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                    betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_TWO;
                }
                else {
                    int32_t iNumber = 3 * ((j + 1) % 24 / 2) + (j / 24 + 1) / 2;
                    if (cLetter == 'D') {
                        const char* pPayout = sPayoutText.String();
                        const char* pBet = sBetText.String();
                        BSsprintf(
                            betTileInfoArray[j].cBetTiles,
                            64,
                            "%s: %s,%i \n%s: 11:1",
                            "0,00",
                            pBet,
                            iNumber,
                            pPayout
                        );
                        betTileInfoArray[j].numValueList.AddHead(37);
                        betTileInfoArray[j].numValueList.AddHead(0);
                        betTileInfoArray[j].numValueList.AddHead(iNumber);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_THREE;
                    }
                    else {
                        if (cLetter == 'F') {
                            const char* pPayout = sPayoutText.String();
                            const char* pBet = sBetText.String();
                            BSsprintf(
                                betTileInfoArray[j].cBetTiles,
                                64,
                                "%s: %s,%i \n%s: 17:1",
                                "00",
                                pBet,
                                iNumber,
                                pPayout
                            );
                            betTileInfoArray[j].numValueList.AddHead(37);
                        }
                        else {
                            const char* pPayout = sPayoutText.String();
                            const char* pBet = sBetText.String();
                            BSsprintf(
                                betTileInfoArray[j].cBetTiles,
                                64,
                                "%s: %s,%i \n%s: 17:1",
                                a0_0,
                                pBet,
                                iNumber,
                                pPayout
                            );
                            betTileInfoArray[j].numValueList.AddHead(0);
                        }
                        betTileInfoArray[j].numValueList.AddHead(iNumber);
                        betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                        betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_TWO;
                    }
                }
            }
            else {
                int32_t iNumber = 3 * (j % 24 / 2) + (j / 24 + 1) / 2;
                const char* pPayout = sPayoutText.String();
                const char* pBet = sBetText.String();
                BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: %i \n%s: 35:1", pBet, iNumber, pPayout);
                betTileInfoArray[j].numValueList.AddHead(iNumber);
                if (iNumber % 2 || iNumber >= 11) {
                    if (iNumber >= 11) {
                        if ((iNumber & 1) != 0 || iNumber >= 19) {
                            if (iNumber >= 19) {
                                if ((iNumber & 1) != 0 || iNumber >= 28) {
                                    if (iNumber >= 28) {
                                        if ((iNumber & 1) != 0) {
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
        else if (cLetter == 'A') {
            if (j % 2 == 1) {
                int32_t iNumber = 3 * (j % 24 / 2) + 1;
                const char* pPayout = sPayoutText.String();
                const char* pBet = sBetText.String();
                BSsprintf(
                    betTileInfoArray[j].cBetTiles,
                    64,
                    "%s: %i,%i,%i \n%s: 11:1",
                    pBet,
                    iNumber,
                    iNumber + 1,
                    iNumber + 2,
                    pPayout
                );
                betTileInfoArray[j].numValueList.AddHead(iNumber);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 1);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 2);
                betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_THREE;
            }
            else if (j) {
                int32_t iNumber = 3 * ((j - 1) % 24 / 2) + 1;
                const char* pPayout = sPayoutText.String();
                const char* pBet = sBetText.String();
                BSsprintf(
                    betTileInfoArray[j].cBetTiles,
                    64,
                    "%s: %i,%i,%i,%i,%i,%i \n%s: 5:1",
                    pBet,
                    iNumber,
                    iNumber + 1,
                    iNumber + 2,
                    iNumber + 3,
                    iNumber + 4,
                    iNumber + 5,
                    pPayout
                );
                betTileInfoArray[j].numValueList.AddHead(iNumber);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 1);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 2);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 3);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 4);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 5);
                betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_SIX;
            }
            else {
                const char* pPayout = sPayoutText.String();
                const char* pBet = sBetText.String();
                BSsprintf(betTileInfoArray[j].cBetTiles, 64, "%s: 0,00,1,2,3 \n%s: 6:1", pBet, pPayout);
                betTileInfoArray[j].numValueList.AddHead(0);
                betTileInfoArray[j].numValueList.AddHead(37);
                betTileInfoArray[j].numValueList.AddHead(1);
                betTileInfoArray[j].numValueList.AddHead(2);
                betTileInfoArray[j].numValueList.AddHead(3);
                betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_FIVE;
            }
        }
        else if (cLetter != 'S') {
            if (cOnes % 2) {
                if (j % 24) {
                    int32_t iNumber = 3 * ((j - 1) % 24 / 2) + (j / 24 + 1) / 2;
                    const char* pPayout = sPayoutText.String();
                    const char* pBet = sBetText.String();
                    BSsprintf(
                        betTileInfoArray[j].cBetTiles,
                        64,
                        "%s: %i,%i,%i,%i \n%s: 8:1",
                        pBet,
                        iNumber,
                        iNumber + 1,
                        iNumber + 3,
                        iNumber + 4,
                        pPayout
                    );
                    betTileInfoArray[j].numValueList.AddHead(iNumber);
                    betTileInfoArray[j].numValueList.AddHead(iNumber + 1);
                    betTileInfoArray[j].numValueList.AddHead(iNumber + 3);
                    betTileInfoArray[j].numValueList.AddHead(iNumber + 4);
                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                    betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_FOUR;
                }
                else {
                    char acZeroText[4];
                    int32_t iFirst = (cLetter != 'C') + 1;
                    int32_t iSecond = (cLetter != 'C') + 2;
                    int32_t iZero = cLetter != 'C' ? 37 : 0;
                    if (cLetter == 'C') {
                        BSstrcpy(acZeroText, 3, a0_0);
                    }
                    else {
                        BSstrcpy(acZeroText, 3, "00");
                    }
                    const char* pPayout = sPayoutText.String();
                    const char* pBet = sBetText.String();
                    BSsprintf(
                        betTileInfoArray[j].cBetTiles,
                        64,
                        "%s: %s,%i,%i \n%s: 11:1",
                        pBet,
                        acZeroText,
                        iFirst,
                        iSecond,
                        pPayout
                    );
                    betTileInfoArray[j].numValueList.AddHead(iFirst);
                    betTileInfoArray[j].numValueList.AddHead(iSecond);
                    betTileInfoArray[j].numValueList.AddHead(iZero);
                    betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                    betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_THREE;
                }
            }
            else {
                int32_t iNumber = 3 * (j % 24 / 2) + (j / 24 + 1) / 2;
                const char* pPayout = sPayoutText.String();
                const char* pBet = sBetText.String();
                BSsprintf(
                    betTileInfoArray[j].cBetTiles,
                    64,
                    "%s: %i,%i \n%s: 17:1",
                    pBet,
                    iNumber,
                    iNumber + 1,
                    pPayout
                );
                betTileInfoArray[j].numValueList.AddHead(iNumber);
                betTileInfoArray[j].numValueList.AddHead(iNumber + 1);
                betTileInfoArray[j].tileColor = RouletteMenu::RO_COLOR_NONE;
                betTileInfoArray[j].betPayoutRatio = RouletteMenu::RO_TWO;
            }
        }
        if (++cOnes > '9') {
            cOnes = '0';
            ++cTens;
        }
        if (cOnes == '5' && cTens == '2') {
            ++cLetter;
            if (j >= 143) {
                cLetter = 'S';
            }
            cTens = '0';
            cOnes = '1';
        }
        NiAVObject* pBlock = BSUtilities::GetObjectByName(pRouletteGridGeometry, static_cast<const char*>(blockName));
        if (!pBlock) {
            NiFixedString kName(static_cast<const char*>(blockName));
            pBlock = pRouletteGridGeometry->GetObjectByName(kName);
        }
        pBlock->SetAppCulled(true);
        betTileInfoArray[j].location = pBlock->GetWorldBound().GetCenter();
        BSstrcpy(betTileInfoArray[j].faceName, 8, blockName);
    }
    fRadius = (betTileInfoArray[0].location - betTileInfoArray[1].location).Length();
    fValidRadius = fRadius * 0.49000000953674316;
    for (int32_t k = 144; k < 159; ++k) {
        float fWidth;
        float fHeight;
        if (k == 144) {
            fWidth = betTileInfoArray[144].location.x - betTileInfoArray[0].location.x;
            fHeight = betTileInfoArray[144].location.y - betTileInfoArray[145].location.y;
            if (fWidth < 0.0) {
                fWidth = fWidth * -0.49000000953674316;
            }
            else {
                fWidth = fWidth * 0.49000000953674316;
            }
            if (fHeight < 0.0) {
                fHeight = fHeight * -0.49000000953674316;
            }
            else {
                fHeight = fHeight * 0.49000000953674316;
            }
            float fMinRadius = fHeight > static_cast<double>(fWidth) ? fWidth : fHeight;
            fSpecialRadius[2] = fMinRadius;
            fSpecialRadius[1] = fMinRadius;
            fSpecialRadius[0] = fMinRadius;
            const char* pPayout = sPayoutText.String();
            const char* pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: 00 \n%s: 35:1", pBet, pPayout);
            pPayout = sPayoutText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 1].cBetTiles, 64, "%s: 0,00 \n%s: 17:1", pBet, pPayout);
            pPayout = sPayoutText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 2].cBetTiles, 64, "%s: 0 \n%s: 35:1", pBet, pPayout);
            betTileInfoArray[k].numValueList.AddHead(37);
            betTileInfoArray[k + 1].numValueList.AddHead(37);
            betTileInfoArray[k + 1].numValueList.AddHead(0);
            betTileInfoArray[k + 2].numValueList.AddHead(0);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 1].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 2].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_ONE;
            betTileInfoArray[k + 1].betPayoutRatio = RouletteMenu::RO_TWO;
            betTileInfoArray[k + 2].betPayoutRatio = RouletteMenu::RO_ONE;
        }
        if (k == 147) {
            fWidth = betTileInfoArray[147].location.x - betTileInfoArray[150].location.y;
            fHeight = betTileInfoArray[147].location.y - betTileInfoArray[0].location.y;
            if (fWidth < 0.0) {
                fWidth = fWidth * -0.49000000953674316;
            }
            else {
                fWidth = fWidth * 0.49000000953674316;
            }
            if (fHeight < 0.0) {
                fHeight = fHeight * -0.49000000953674316;
            }
            else {
                fHeight = fHeight * 0.49000000953674316;
            }
            float fMinRadius = fHeight > static_cast<double>(fWidth) ? fWidth : fHeight;
            fSpecialRadius[9] = fMinRadius;
            fSpecialRadius[6] = fMinRadius;
            fSpecialRadius[3] = fMinRadius;
            const char* pPayout = sPayoutText.String();
            const char* pText = sFirstDozenText.String();
            const char* pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: %s \n%s: 2:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = sSecondDozenText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 3].cBetTiles, 64, "%s: %s \n%s: 2:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = sThirdDozenText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 6].cBetTiles, 64, "%s: %s \n%s: 2:1", pBet, pText, pPayout);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 3].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 6].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 3].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 6].betPayoutRatio = RouletteMenu::RO_TWELVE;
        }
        if (k == 148) {
            fWidth = betTileInfoArray[148].location.x - betTileInfoArray[149].location.y;
            fHeight = betTileInfoArray[148].location.y - betTileInfoArray[3].location.y;
            if (fWidth < 0.0) {
                fWidth = fWidth * -0.49000000953674316;
            }
            else {
                fWidth = fWidth * 0.49000000953674316;
            }
            if (fHeight < 0.0) {
                fHeight = fHeight * -0.49000000953674316;
            }
            else {
                fHeight = fHeight * 0.49000000953674316;
            }
            float fMinRadius = fHeight > static_cast<double>(fWidth) ? fWidth : fHeight;
            fSpecialRadius[11] = fMinRadius;
            fSpecialRadius[10] = fMinRadius;
            fSpecialRadius[8] = fMinRadius;
            fSpecialRadius[7] = fMinRadius;
            fSpecialRadius[5] = fMinRadius;
            fSpecialRadius[4] = fMinRadius;
            const char* pPayout = sPayoutText.String();
            const char* pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: 1-18 \n%s: 1:1", pBet, pPayout);
            pPayout = sPayoutText.String();
            const char* pText = sEvenText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 1].cBetTiles, 64, "%s: %s \n%s: 1:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = sRedText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 3].cBetTiles, 64, "%s: %s \n%s: 1:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = gs_sBlacktext.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 4].cBetTiles, 64, "%s: %s \n%s: 1:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = gs_sOddtext.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 6].cBetTiles, 64, "%s: %s \n%s: 1:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 7].cBetTiles, 64, "%s: 19-36 \n%s: 1:1", pBet, pPayout);
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
            fWidth = betTileInfoArray[156].location.x - betTileInfoArray[23].location.x;
            fHeight = betTileInfoArray[156].location.y - betTileInfoArray[157].location.y;
            if (fWidth < 0.0) {
                fWidth = fWidth * -0.49000000953674316;
            }
            else {
                fWidth = fWidth * 0.49000000953674316;
            }
            if (fHeight < 0.0) {
                fHeight = fHeight * -0.49000000953674316;
            }
            else {
                fHeight = fHeight * 0.49000000953674316;
            }
            float fMinRadius = fHeight > static_cast<double>(fWidth) ? fWidth : fHeight;
            fSpecialRadius[14] = fMinRadius;
            fSpecialRadius[13] = fMinRadius;
            fSpecialRadius[12] = fMinRadius;
            const char* pPayout = sPayoutText.String();
            const char* pText = sTwoToOneText.String();
            const char* pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k].cBetTiles, 64, "%s: %s \n%s: 2:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = sTwoToOneText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 1].cBetTiles, 64, "%s: %s \n%s: 2:1", pBet, pText, pPayout);
            pPayout = sPayoutText.String();
            pText = sTwoToOneText.String();
            pBet = sBetText.String();
            BSsprintf(betTileInfoArray[k + 2].cBetTiles, 64, "%s: %s \n%s: 2:1", pBet, pText, pPayout);
            betTileInfoArray[k].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 1].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k + 2].tileColor = RouletteMenu::RO_COLOR_NONE;
            betTileInfoArray[k].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 1].betPayoutRatio = RouletteMenu::RO_TWELVE;
            betTileInfoArray[k + 2].betPayoutRatio = RouletteMenu::RO_TWELVE;
        }
    }
    auiNumberStrip[0] = 14;
    auiNumberStrip[1] = 2;
    auiNumberStrip[2] = 0;
    auiNumberStrip[3] = 28;
    auiNumberStrip[4] = 9;
    auiNumberStrip[5] = 26;
    auiNumberStrip[6] = 30;
    auiNumberStrip[7] = 11;
    auiNumberStrip[8] = 7;
    auiNumberStrip[9] = 20;
    auiNumberStrip[10] = 32;
    auiNumberStrip[11] = 17;
    auiNumberStrip[12] = 5;
    auiNumberStrip[13] = 22;
    auiNumberStrip[14] = 34;
    auiNumberStrip[15] = 15;
    auiNumberStrip[16] = 3;
    auiNumberStrip[17] = 24;
    auiNumberStrip[18] = 36;
    auiNumberStrip[19] = 13;
    auiNumberStrip[20] = 1;
    auiNumberStrip[21] = 37;
    auiNumberStrip[22] = 27;
    auiNumberStrip[23] = 10;
    auiNumberStrip[24] = 25;
    auiNumberStrip[25] = 29;
    auiNumberStrip[26] = 12;
    auiNumberStrip[27] = 8;
    auiNumberStrip[28] = 19;
    auiNumberStrip[29] = 31;
    auiNumberStrip[30] = 18;
    auiNumberStrip[31] = 6;
    auiNumberStrip[32] = 21;
    auiNumberStrip[33] = 33;
    auiNumberStrip[34] = 16;
    auiNumberStrip[35] = 4;
    auiNumberStrip[36] = 23;
    auiNumberStrip[37] = 35;
    ourMemcpy(iNumberStrip, auiNumberStrip, sizeof(iNumberStrip));
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
        float fChipZ = RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex))->GetTranslate().z;
        float fY = fyTranslate;
        float fX = fxTranslate;
        RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex))->SetTranslate(fX, fY, fChipZ);
        fY = fyTranslate;
        fX = fxTranslate;
        RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex + 10))->SetTranslate(fX, fY, 0.0f);
        NiUpdateData kChipUpdate(0.0f, 0, 0);
        RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex))->Update(kChipUpdate);
        NiUpdateData kShadowUpdate(0.0f, 0, 0);
        RouletteMenu::spModel->GetAt(static_cast<uint32_t>(iNextModelIndex + 10))->Update(kShadowUpdate);
    }
    --iNumBetsSet;
    RouletteMenu::RouletteBet* delBet = blist->m_item;
    blist->RemoveHead();
    ourFree(delBet);
    delBet = nullptr;
    return true;
}

// Geometry pointers are indexed from pRouletteTableGeometry by model index: model aiIndex is pBetGeometry[aiIndex - 3]
// and its shadow pBetShadowGeometry[aiIndex - 3].
void RouletteMenu::RemoveBetNif(int32_t aiIndex) {
    pBetGeometry[aiIndex - 3]->SetTranslate(0.0f, 0.0f, -5.0f);
    pBetShadowGeometry[aiIndex - 3]->SetTranslate(0.0f, 0.0f, -5.0f);
    NiUpdateData kChipUpdate(0.0f, 0, 0);
    pBetGeometry[aiIndex - 3]->Update(kChipUpdate);
    NiUpdateData kShadowUpdate(0.0f, 0, 0);
    pBetShadowGeometry[aiIndex - 3]->Update(kShadowUpdate);
}

void RouletteMenu::SetWinningSlotLuckyAndAnimToPlay() {
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

void RouletteMenu::DrawScene() {
    NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pSceneGraph->pCuller;
    pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* pCamera = RouletteMenu::sp3DCamera.m_pObject;
    NiCamera::HandleEyeFinityFOVScale(pCamera, 0.15f);
    NiPointer<NiRefObject> spAccumulator(static_cast<NiRefObject*>(InterfaceManager::Instance()->Get3dAccumulator()));
    pCuller->SetAccumulator(static_cast<NiAccumulator*>(spAccumulator.m_pObject));
    NiCamera* pRenderCamera = pCamera ? pCamera : pSceneGraph->GetCamera();
    BSShaderUtil::AccumulateScene(pRenderCamera, BSShaderManager::GetShadowSceneNode(1), *pCuller);
    BSShaderUtil::RenderScene(pRenderCamera, static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject), false);
    pCuller->SetAccumulator(nullptr);
    spAccumulator = static_cast<NiRefObject*>(nullptr);
    pCuller->PopCullMode();
}

void RouletteMenu::Close() {
    if (RouletteMenu::pMe) {
        InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 255.0f, true);
        RouletteMenu::pMe->eRouletteState = RO_CLOSING_MENU;
    }
}

// Finds (or adds at the head of the player's list) the casino's record, refuses a barred, broke or under-winning player
// with a HUD message and a sound, then builds the menu from roulette_menu.xml and fills its tiles. With no casino list
// at all, SetItem is called on null. abDebugMode is not read. The settings and the remaining time are
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
        CasinoData* pNewData = new CasinoData;
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
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
            nullptr,
            2.0f,
            false
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
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
            nullptr,
            2.0f,
            false
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
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
            nullptr,
            2.0f,
            false
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
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
            nullptr,
            2.0f,
            false
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
