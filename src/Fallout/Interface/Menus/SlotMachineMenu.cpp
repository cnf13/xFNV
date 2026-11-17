#include "Fallout/Interface/Menus/SlotMachineMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BlackJackMenu.h"
#include "Fallout/Interface/Menus/RouletteMenu.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdint>

NiPointer<NiNode> SlotMachineMenu::spSlotModel;
NiPointer<NiCamera> SlotMachineMenu::sp3DCamera;

SlotMachineMenu::SlotMachineMenu()
  : Menu(), eSlotState(SL_INTRO_ANIM), uiPrevIdleTick(0), iNextSoundIndex(1), pSlotReelTextures(),
    iCheatToggle(pPlayer->iCasinoCheatLevel) {
    g_slotMachineMenuTrait_Value = Tile::AddUserTrait("_Value", -1);
    SlotMachineMenu::iUT_X = Tile::AddUserTrait("_x", -1);
    if (SlotMachineMenu::pCasino->GetChipType()) {
        TESCasinoChips* ChipType = SlotMachineMenu::pCasino->GetChipType();
        iChipCount = pPlayer->GetInventoryObjectCount(ChipType);
    }
}

SlotMachineMenu::~SlotMachineMenu() {
    SetLastAntiCheatTime();
    TESModelTextureSwap* Model_ = SlotMachineMenu::pCasino->GetModel(7);
    const char* v1 = Model_->GetModel();
    pModelLoader->ReleaseFile(v1);
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel01.NIF");
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel02.NIF");
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel03.NIF");
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(SlotMachineMenu::spSlotModel.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    SlotMachineMenu::spSlotModel.m_pObject->DetachAllChildren();
    pSlotMachineGeometry = nullptr;
    SlotMachineMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
    SlotMachineMenu::spSlotModel = static_cast<NiNode*>(nullptr);
    pSlotAnimManager = nullptr;
    for (int32_t i = 0; i < 3; ++i) {
        pReelGeometry[i] = nullptr;
        pSlotReelAnim[i] = nullptr;
    }
    SlotMachineMenu::pCasino = nullptr;
    SlotMachineMenu::pMe = nullptr;
}

void SlotMachineMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (((uint32_t)aiID) <= 9) {
        pTiles[aiID] = apTile;
    }
}

void SlotMachineMenu::DoClick(int32_t a1, Tile* a2) {
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a12, std::nullopt, std::nullopt);
    if (eSlotState == SlotMachineMenu::SL_IDLE) {
        switch (a1) {
            case 3: {
                if (iChipCount >= this->iCurrentBet) {
                    iNextSoundIndex = 1;
                    eSpinResult = SpinReels();
                    bLucky = CalculateShouldWin(eSpinResult);
                    eSpinResult = InterpretSpin();
                    eSlotState = SlotMachineMenu::SL_SPIN_REELS_ANIM;
                    MiscStatManager::Increment(MiscStatManager::MS_SLOTS_GAMES_PLAYED);
                }
                break;
            }
            case 4: {
                int32_t iCurrentBet = this->iCurrentBet;
                if (this->iCurrentBet >= 10) {
                    if (this->iCurrentBet >= 40) {
                        if (this->iCurrentBet >= 100) {
                            int32_t v3;
                            if (this->iCurrentBet >= 500) {
                                v3 = this->iCurrentBet + 100;
                            }
                            else {
                                v3 = this->iCurrentBet + 25;
                            }
                            this->iCurrentBet = v3;
                        }
                        else {
                            this->iCurrentBet += 10;
                        }
                    }
                    else {
                        this->iCurrentBet += 5;
                    }
                }
                else {
                    ++this->iCurrentBet;
                }
                if (this->iCurrentBet > iChipCount) {
                    this->iCurrentBet = iChipCount;
                }
                if (this->iCurrentBet > g_slotMachineMaxBet) {
                    this->iCurrentBet = g_slotMachineMaxBet;
                }
                SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, this->iCurrentBet);
                if (iCurrentBet != this->iCurrentBet) {
                    int32_t v14;
                    char a22[264];
                    BSsprintf(a22, 260, "GAMESlotsIncreaseBet0%i", iNextSoundIndex);
                    BSAudio* v4 = &BSAudio::QInstance();
                    BSSoundHandle* SoundHandleByName = v4->GetSoundHandleByName(&a12, a22, 289u);
                    SoundHandleByName->Play(false);
                    if (iNextSoundIndex + 1 <= 3) {
                        v14 = iNextSoundIndex + 1;
                    }
                    else {
                        v14 = 1;
                    }
                    iNextSoundIndex = v14;
                }
                break;
            }
            case 5: {
                int32_t v21 = this->iCurrentBet;
                if (this->iCurrentBet > 10) {
                    if (this->iCurrentBet > 40) {
                        if (this->iCurrentBet > 100) {
                            if (this->iCurrentBet > 500) {
                                int32_t v10;
                                if (this->iCurrentBet % 100) {
                                    v10 = 100 * (this->iCurrentBet / 100);
                                }
                                else {
                                    v10 = this->iCurrentBet - 100;
                                }
                                this->iCurrentBet = v10;
                            }
                            else {
                                int32_t v11;
                                if (this->iCurrentBet % 25) {
                                    v11 = 25 * (this->iCurrentBet / 25);
                                }
                                else {
                                    v11 = this->iCurrentBet - 25;
                                }
                                this->iCurrentBet = v11;
                            }
                        }
                        else {
                            int32_t v12;
                            if (this->iCurrentBet % 10) {
                                v12 = 10 * (this->iCurrentBet / 10);
                            }
                            else {
                                v12 = this->iCurrentBet - 10;
                            }
                            this->iCurrentBet = v12;
                        }
                    }
                    else {
                        int32_t v13;
                        if (this->iCurrentBet % 5) {
                            v13 = 5 * (this->iCurrentBet / 5);
                        }
                        else {
                            v13 = this->iCurrentBet - 5;
                        }
                        this->iCurrentBet = v13;
                    }
                }
                else {
                    --this->iCurrentBet;
                }
                if (this->iCurrentBet < g_slotMachineMinBet) {
                    this->iCurrentBet = g_slotMachineMinBet;
                }
                SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, this->iCurrentBet);
                if (v21 != this->iCurrentBet) {
                    BSAudio* v5 = &BSAudio::QInstance();
                    BSSoundHandle* v9 = v5->GetSoundHandleByName(&arHandle, "GAMESlotsDecreaseBet", 289u);
                    v9->Play(false);
                    iNextSoundIndex = 1;
                }
                break;
            }
            case 6: {
                eSlotState = SlotMachineMenu::SL_PAYOUT_IN_ANIM;
                break;
            }
            case 7: {
                if (this->iCurrentBet != g_slotMachineMaxBet) {
                    int32_t v7;
                    char apDest[268];
                    this->iCurrentBet = g_slotMachineMaxBet;
                    if (this->iCurrentBet > iChipCount) {
                        this->iCurrentBet = iChipCount;
                    }
                    SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                    SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, this->iCurrentBet);
                    BSsprintf(apDest, 260, "GAMESlotsIncreaseBet0%i", iNextSoundIndex);
                    BSAudio* v6 = &BSAudio::QInstance();
                    BSSoundHandle* v8 = v6->GetSoundHandleByName(&th_s, apDest, 289u);
                    v8->Play(false);
                    if (iNextSoundIndex + 1 <= 3) {
                        v7 = iNextSoundIndex + 1;
                    }
                    else {
                        v7 = 1;
                    }
                    iNextSoundIndex = v7;
                }
                break;
            }
            case 8: {
                SlotMachineMenu::Close();
                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        if (eSlotState == SlotMachineMenu::SL_PAYOUT_IN_ANIM) {
            eSlotState = SlotMachineMenu::SL_PAYOUT_OUT_ANIM;
        }
    }
}

void SlotMachineMenu::DoIdle() {
    FNVD_INLINED_LOCAL(BSSoundHandle, v50, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, v51, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    int32_t v76 = 0;
    float v80 = 1.0f;
    Interface::enumMenus v1 = static_cast<Interface::enumMenus>(GetClass());
    bool bisMenuOnTop = Interface::IsTopMenuID(v1);
    if (uiPrevIdleTick) {
        if (bisMenuOnTop) {
            int32_t ielaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
            if (SlotMachineMenu::eLastStage == eSlotState) {
                SlotMachineMenu::fTimeDelta =
                    static_cast<double>(ielaspedMillis) / 1000.0 + SlotMachineMenu::fTimeDelta;
            }
            else {
                SlotMachineMenu::fTimeDelta = 0.0f;
            }
        }
    }
    else {
        v76 = 0;
        SlotMachineMenu::fTimeDelta = 0.0f;
        SlotMachineMenu::eLastStage = 0;
    }
    int32_t v79 = 0;
    uiPrevIdleTick = appTimer.iLastTime;
    SlotMachineMenu::eLastStage = eSlotState;
    if (!bisMenuOnTop) {
        InterfaceManager* v2 = InterfaceManager::Instance();
        v2->pCursor->Set(4009, 255.0f, true);
    }
    switch (eSlotState) {
        case SlotMachineMenu::SL_INTRO_ANIM: {
            eSlotState = SlotMachineMenu::SL_IDLE;
            break;
        }
        case SlotMachineMenu::SL_SPIN_REELS_ANIM: {
            if ((pSlotLeverAnim) && (pSlotReelAnim[0])) {
                if (pSlotCoinAnim) {
                    pSlotAnimManager->DeactivateSequence(pSlotCoinAnim, 0.0f);
                }
                if (SlotMachineMenu::fTimeDelta > 1.0) {
                    pTiles[9]->Set(4003, 0);
                }
                NiUpdateData UpdateData1(SlotMachineMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(SlotMachineMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(SlotMachineMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData4(SlotMachineMenu::fTimeDelta, true, false);
                NiAVObject* v15 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(0);
                v15->Update(UpdateData1);
                if (SlotMachineMenu::fTimeDelta == 0.0 && pTiles[1]->GetFloat(SlotMachineMenu::iUT_X) > 0.0) {
                    BSAudio* v16 = &BSAudio::QInstance();
                    BSSoundHandle* SoundHandleByName = v16->GetSoundHandleByName(&a1, "GameSlotsPullLever", 289u);
                    SoundHandleByName->Play(false);
                    bSwapTextures = true;
                }
                if (SlotMachineMenu::fTimeDelta == 0.0 || pTiles[1]->GetFloat(SlotMachineMenu::iUT_X) > 0.0) {
                    NiAVObject* v17 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(1u);
                    v17->Update(UpdateData2);
                    NiAVObject* v18 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(2u);
                    v18->Update(UpdateData3);
                    NiAVObject* v19 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(3u);
                    v19->Update(UpdateData4);
                }
                if (static_cast<double>(SlotMachineMenu::fTimeDelta) >= pSlotLeverAnim->m_fEndKeyTime &&
                    bSwapTextures) {
                    bSwapTextures = false;
                    SwapFrontfacingTextures();
                }
                if (static_cast<double>(SlotMachineMenu::fTimeDelta) >= pSlotLeverAnim->m_fEndKeyTime &&
                    static_cast<double>(SlotMachineMenu::fTimeDelta) >= pSlotReelAnim[0]->m_fEndKeyTime) {
                    eSlotState = SlotMachineMenu::SL_CALC_RESULTS;
                    SwapBackfacingTextures();
                }
            }

            break;
        }
        case SlotMachineMenu::SL_CALC_RESULTS: {
            int32_t winnings = 0;
            eSpinResult = InterpretSpin();
            if (eSpinResult == SlotMachineMenu::SL_THREE_MATCH) {
                switch (eReelResults[0]) {
                    case SlotMachineMenu::SL_A: {
                        winnings = 10 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    case SlotMachineMenu::SL_B: {
                        winnings = 20 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    case SlotMachineMenu::SL_C: {
                        winnings = 30 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    case SlotMachineMenu::SL_D: {
                        winnings = 40 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    case SlotMachineMenu::SL_E: {
                        winnings = 60 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    case SlotMachineMenu::SL_F: {
                        winnings = 100 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    case SlotMachineMenu::SL_WILD: {
                        winnings = 10 * iCurrentBet;
                        iChipCount += winnings;
                        break;
                    }
                    default: {
                        iChipCount -= iCurrentBet;
                        break;
                    }
                }
            }
            else {
                if (eSpinResult == SlotMachineMenu::SL_WILD_PAIR) {
                    winnings = 5 * iCurrentBet;
                    iChipCount += winnings;
                }
                else {
                    if (eSpinResult == SlotMachineMenu::SL_WILD_SYM_) {
                        winnings = 2 * iCurrentBet;
                    }
                    else {
                        winnings = -iCurrentBet;
                    }
                    iChipCount += winnings;
                }
            }
            pTiles[1]->Poke(SlotMachineMenu::iUT_X, 1.0f);
            pTiles[1]->Set(SlotMachineMenu::iUT_X, iChipCount);
            g_slotMachineCasinoStats->iTotalWinnings += winnings;
            UpdateEarningsString();
            if (eSpinResult <= SlotMachineMenu::SL_WINNING_VALUES) {
                char apDest[264];
                if (bLucky) {
                    BSAudio* v27 = &BSAudio::QInstance();
                    BSSoundHandle* v44 = v27->GetSoundHandleByName(&v51, "GAMESlotsLose", 289u);
                    v44->Play(false);
                    const char* fEaseOutTimed = sPlural.String();
                    const char* timed = sCasinoChipText.String();
                    BSsprintf(apDest, 260, "%s %d %s%s", sUnluckyLoseText.String(), -winnings, timed, fEaseOutTimed);
                }
                else {
                    const char* fEaseOutTimee = sPlural.String();
                    const char* timee = sCasinoChipText.String();
                    BSsprintf(apDest, 260, "%s %d %s%s", sYouLoseText.String(), -winnings, timee, fEaseOutTimee);
                }
                SlotMachineMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                SlotMachineMenu::pMe->pTiles[9]->Set(4036, apDest, true);
                SlotMachineMenu::pMe->pTiles[9]->Set(4003, 1);
                eSlotState = SlotMachineMenu::SL_IDLE;
            }
            else {
                char Src[264];
                if (bLucky) {
                    const char* fEaseOutTimeb = sPlural.String();
                    const char* timeb = sCasinoChipText.String();
                    BSsprintf(Src, 260, "%s %d %s%s", sLuckyWinText.String(), winnings, timeb, fEaseOutTimeb);
                }
                else {
                    const char* fEaseOutTimec = sPlural.String();
                    const char* timec = sCasinoChipText.String();
                    BSsprintf(Src, 260, "%s %d %s%s", sYouWinText.String(), winnings, timec, fEaseOutTimec);
                }
                SlotMachineMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                SlotMachineMenu::pMe->pTiles[9]->Set(4036, Src, true);
                SlotMachineMenu::pMe->pTiles[9]->Set(4003, 1);
                eSlotState = SlotMachineMenu::SL_WIN_ANIM;
            }
            break;
        }
        case SlotMachineMenu::SL_WIN_ANIM: {
            if (pSlotCoinAnim) {
                pSlotAnimManager->ActivateSequence(pSlotCoinAnim, 0, false, 1.0f, 0.0f, nullptr);
                if (pSlotLeverAnim) {
                    pSlotAnimManager->DeactivateSequence(pSlotLeverAnim, 0.0f);
                }
                NiUpdateData UpdateData1_2(SlotMachineMenu::fTimeDelta, true, false);
                NiAVObject* v20 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(0);
                v20->Update(UpdateData1_2);
                if (SlotMachineMenu::fTimeDelta == 0.0) {
                    switch (eSpinResult) {
                        case SlotMachineMenu::SL_WILD_SYM_: {
                            BSAudio* v21 = &BSAudio::QInstance();
                            BSSoundHandle* v47 = v21->GetSoundHandleByName(&arHandle, "GAMESlotsWinSmall", 289u);
                            v47->Play(false);
                            break;
                        }
                        case SlotMachineMenu::SL_WILD_PAIR: {
                            BSAudio* v22 = &BSAudio::QInstance();
                            BSSoundHandle* v46 = v22->GetSoundHandleByName(&th_s, "GAMESlotsWinMed", 289u);
                            v46->Play(false);
                            break;
                        }
                        case SlotMachineMenu::SL_THREE_MATCH: {
                            BSAudio* v23 = &BSAudio::QInstance();
                            BSSoundHandle* v45 = v23->GetSoundHandleByName(&apResult, "GAMESlotsWinJackpot", 289u);
                            v45->Play(false);
                            break;
                        }
                    }
                }
                if (SlotMachineMenu::fTimeDelta > 1.0) {
                    pTiles[9]->Set(4003, 0);
                }
                if (static_cast<double>(SlotMachineMenu::fTimeDelta) >= pSlotCoinAnim->m_fEndKeyTime) {
                    pSlotAnimManager->DeactivateSequence(pSlotCoinAnim, 0.0f);
                    if (pSlotLeverAnim) {
                        pSlotAnimManager->ActivateSequence(pSlotLeverAnim, 0, false, 1.0f, 0.0f, nullptr);
                    }
                    int32_t iMax = (&SlotMachineMenu::pCasino->data)->iMaxWinnings;
                    int32_t iCurrentWinnings = g_slotMachineCasinoStats->iTotalWinnings;
                    int32_t iWinningLevel = g_slotMachineCasinoStats->iWinningLevel;
                    for (int32_t i_3 = 1; i_3 < 5; ++i_3) {
                        if (i_3 > iWinningLevel) {
                            if (static_cast<double>(iCurrentWinnings) >=
                                static_cast<double>(iMax) * (static_cast<double>(i_3) * 0.25)) {
                                g_slotMachineCasinoStats->iWinningLevel = i_3;
                                bRunScriptOnExit = true;
                            }
                        }
                        else {
                            bRunScriptOnExit = false;
                        }
                    }
                    if (bRunScriptOnExit) {
                        SlotMachineMenu::Close();
                    }
                    else {
                        eSlotState = SlotMachineMenu::SL_IDLE;
                    }
                }
            }
            else {
                eSlotState = SlotMachineMenu::SL_IDLE;
            }
            break;
        }
        case SlotMachineMenu::SL_PAYOUT_IN_ANIM: {
            if (pSlotPayoutListAnim[0]) {
                if (SlotMachineMenu::fTimeDelta == 0.0) {
                    pTiles[9]->Set(4003, 1);
                    const char* v24 = sSlotPressAnyButtonText.String();
                    pTiles[9]->Set(4036, v24, true);
                    pSlotAnimManager->DeactivateAll(0.0f);
                    pSlotAnimManager->ActivateSequence(pSlotPayoutListAnim[0], 0, false, 1.0f, 0.0f, nullptr);
                }
                if (!(static_cast<double>(SlotMachineMenu::fTimeDelta) <= pSlotPayoutListAnim[0]->m_fEndKeyTime)) {
                    pSlotAnimManager->DeactivateSequence(pSlotPayoutListAnim[0], 0.0f);
                }
                else {
                    NiUpdateData UpdateData1_3(SlotMachineMenu::fTimeDelta, true, false);
                    NiAVObject* v25 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(0);
                    v25->Update(UpdateData1_3);
                }
            }
            break;
        }
        case SlotMachineMenu::SL_PAYOUT_OUT_ANIM: {
            if (pSlotPayoutListAnim[1]) {
                if (SlotMachineMenu::fTimeDelta == 0.0) {
                    pTiles[9]->Set(4003, 0);
                    pSlotAnimManager->DeactivateAll(0.0f);
                    pSlotAnimManager->ActivateSequence(pSlotPayoutListAnim[1], 0, false, 1.0f, 0.0f, nullptr);
                }
                if (!(static_cast<double>(SlotMachineMenu::fTimeDelta) <= pSlotPayoutListAnim[1]->m_fEndKeyTime)) {
                    pSlotAnimManager->DeactivateSequence(pSlotPayoutListAnim[1], 0.0f);
                    pSlotAnimManager->ActivateSequence(pSlotLeverAnim, 0, false, 1.0f, 0.0f, nullptr);
                    eSlotState = SlotMachineMenu::SL_IDLE;
                }
                else {
                    NiUpdateData UpdateData1_4(SlotMachineMenu::fTimeDelta, true, false);
                    NiAVObject* v26 = SlotMachineMenu::spSlotModel.m_pObject->GetAt(0);
                    v26->Update(UpdateData1_4);
                }
            }
            break;
        }
        case SlotMachineMenu::SL_IDLE: {
            if (SlotMachineMenu::fTimeDelta > 1.0) {
                pTiles[9]->Set(4003, 0);
            }
            if (iChipCount < iCurrentBet) {
                iCurrentBet = 1;
                SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, iCurrentBet);
            }
            if (!iChipCount) {
                char msg[264];
                BSsprintf(msg, 260, sGamblingBrokeText.String());
                Interface::ShowMessage(
                    msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
                );
                BSAudio* v28 = &BSAudio::QInstance();
                BSSoundHandle* v43 = v28->GetSoundHandleByName(&v50, "UIPopUpMessageGeneral", 289u);
                v43->Play(false);
                SlotMachineMenu::Close();
            }
            break;
        }
        case SlotMachineMenu::SL_CLOSING_MENU: {
            if (xFadeState != Interface::FADING_OUT) {
                InterfaceManager* v3 = InterfaceManager::Instance();
                v3->pCursor->Set(4009, 255.0f, true);
                pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
                StartFadeOut();
                TESCasinoChips* ChipType = SlotMachineMenu::pCasino->GetChipType();
                int32_t i = iChipCount - pPlayer->GetInventoryObjectCount(ChipType);
                if (i >= 0) {
                    if (i > 0) {
                        TESContainer tempcont;
                        TESCasinoChips* v10 = SlotMachineMenu::pCasino->GetChipType();
                        tempcont.AddObject(v10, i, nullptr);
                        if (g_slotMachineCasinoStats->iTotalWinnings >= SlotMachineMenu::pCasino->GetMaxWinnings()) {
                            tempcont.AddContainerToReference(pPlayer, true);
                            BSStringT<char> pstring_2;
                            const char* fEaseOutTimea = sGamblingBannedText.String();
                            const char* timea = sAddItemtoInventory.String();
                            const char* a4 = sPlural.String();
                            TESCasinoChips* v11 = SlotMachineMenu::pCasino->GetChipType();
                            const char* FullName = TESFullName::GetFullName(v11);
                            pstring_2.SPrintF("%i %s%s %s\n%s", i, FullName, a4, timea, fEaseOutTimea);
                            TESCasinoChips* v13 = SlotMachineMenu::pCasino->GetChipType();
                            const char* PickUpSoundName = pPlayer->GetPickUpSoundName(v13, true, false);
                            Interface::ShowMessage(
                                pstring_2.pString, 0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
                                PickUpSoundName, 2.0f, false
                            );
                        }
                        else {
                            tempcont.AddContainerToReference(pPlayer, false);
                        }
                    }
                }
                else {
                    TESCasinoChips* v5 = SlotMachineMenu::pCasino->GetChipType();
                    pPlayer->RemoveItem(v5, nullptr, -i, false, false, nullptr, nullptr, nullptr, true, false);
                    BSStringT<char> pstring;
                    const char* fEaseOutTime = sRemoveItemfromInventory.String();
                    const char* time = sPlural.String();
                    TESCasinoChips* v6 = SlotMachineMenu::pCasino->GetChipType();
                    const char* v7 = TESFullName::GetFullName(v6);
                    pstring.SPrintF("%i %s%s %s", -i, v7, time, fEaseOutTime);
                    TESCasinoChips* v8 = SlotMachineMenu::pCasino->GetChipType();
                    const char* v9 = pPlayer->GetPickUpSoundName(v8, true, false);
                    Interface::ShowMessage(
                        pstring.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", v9, 2.0f,
                        false
                    );
                }
                if (bRunScriptOnExit) {
                    TESQuest* pScript = SlotMachineMenu::pCasino->GetQuest();
                    if (pScript) {
                        pScript->SetEnabled(1);
                    }
                }
            }
            break;
        }
        default: {
            return;
        }
    }
}

int32_t SlotMachineMenu::GetClass() {
    return 1080;
}

SlotMachineMenu::SlotResults SlotMachineMenu::SpinReels() {
    for (int32_t i = 0; i < 3; ++i) {
        eReelStartSymbols[i] = eReelResults[i];
        eReelResults[i] = eActiveReelSymbols[RandomInt(0, 2 * iNumActiveSymbols - 1)];
    }
    return InterpretSpin();
}

SlotMachineMenu::SlotResults SlotMachineMenu::InterpretSpin() {
    int32_t v3 = 0;
    for (int32_t i = 0; i < 3; ++i) {
        if (eReelResults[i] == 12) {
            ++v3;
        }
    }
    if (eReelResults[0] == eReelResults[1] &&
        (eReelResults[0] == eReelResults[2] && !(static_cast<int32_t>(eReelResults[1]) % 2))) {
        return SlotMachineMenu::SL_THREE_MATCH;
    }
    if (v3 == 2) {
        return SlotMachineMenu::SL_WILD_PAIR;
    }
    if (v3 == 1) {
        return SlotMachineMenu::SL_WILD_SYM_;
    }
    if (eReelResults[0] == 12 || (eReelResults[1] == 12 || eReelResults[2] == 12)) {
        return SlotMachineMenu::SL_WILD_NULL_NULL;
    }
    // A pair of equal even symbols.
    if ((eReelResults[1] == eReelResults[2] && !(static_cast<int32_t>(eReelResults[1]) % 2)) ||
        (eReelResults[0] == eReelResults[2] && !(static_cast<int32_t>(eReelResults[0]) % 2)) ||
        (eReelResults[1] == eReelResults[0] && !(static_cast<int32_t>(eReelResults[0]) % 2))) {
        return SlotMachineMenu::SL_PAIR;
    }
    return SlotMachineMenu::SL_NO_MATCH;
}

char SlotMachineMenu::InitializeCasinoData() {
    char apDest[264];
    char acPath[268];
    if (!SlotMachineMenu::pCasino) {
        return 0;
    }
    for (int32_t index = 0; index < 7; ++index) {
        if (SlotMachineMenu::pCasino) {
            BSsprintf(apDest, 260, "%s%s", "Textures\\", SlotMachineMenu::pCasino->GetTexture(index)->GetTextureName());
            SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, pSlotReelTextures[index], false, false);
        }
    }
    BSsprintf(apDest, 260, "%sNV_SlotMachine-Symbol%s.dds", "Textures\\terminals\\NV_SlotMachine\\", "S");
    SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, pSlotReelTextures[7], false, false);
    TESModelTextureSwap* Model_ = SlotMachineMenu::pCasino->GetModel(7);
    const char* v1 = Model_->GetModel();
    pSlotMachineGeometry = (NiAVObject*)pModelLoader->LoadFile(v1, LOD_MULT_NONE, true, false, false, false);
    pReelGeometry[0] = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel01.NIF", LOD_MULT_NONE, true, false, false, false
    );
    pReelGeometry[1] = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel02.NIF", LOD_MULT_NONE, true, false, false, false
    );
    pReelGeometry[2] = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel03.NIF", LOD_MULT_NONE, true, false, false, false
    );
    bRunScriptOnExit = false;
    return SetupActiveReels();
}

char SlotMachineMenu::IsWellFormedMenu() {
    int32_t i;
    for (i = 0; i < 10; ++i) {
        if (!pTiles[i]) {
            return 0;
        }
    }
    return 1;
}

char SlotMachineMenu::SetupActiveReels() {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    uint32_t v13[7];
    int32_t v12 = 0;
    for (int32_t i = 0; i < 28; ++i) {
        eActiveReelSymbols[i] = NUM_SYMBOLS;
    }
    uint32_t v14 = 0;
    int32_t iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_A;
    v13[0] = iSlotStop_A;
    iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_B;
    v13[1] = iSlotStop_A;
    iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_C;
    v13[2] = iSlotStop_A;
    iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_D;
    v13[3] = iSlotStop_A;
    iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_E;
    v13[4] = iSlotStop_A;
    iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_F;
    v13[5] = iSlotStop_A;
    iSlotStop_A = (&SlotMachineMenu::pCasino->data)->iSlotStop_W;
    v13[6] = iSlotStop_A;
    iNumActiveSymbols = 7;
    for (int32_t j = 0; j < 7; ++j) {
        if (!v13[j]) {
            --iNumActiveSymbols;
        }
    }
    while (v12 < 28) {
        int32_t v8 = v12;
        v14 = 0;
        for (int32_t k = 0; k < 7; ++k) {
            if (v13[k]) {
                eActiveReelSymbols[v12++] = static_cast<ReelSymbols>(v14);
                eActiveReelSymbols[v12++] = static_cast<ReelSymbols>(++v14);
                --v13[k];
            }
            ++v14;
        }
        if (v8 == v12) {
            char msg[268];
            BSsprintf(
                msg, 260u,
                "Casino Form is not setup properly. # of stops for the slot reels must sum to 14. Exiting out of Slot "
                "machine menu."
            );
            Interface::ShowMessage(
                msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
            );
            BSAudio* v1 = &BSAudio::QInstance();
            BSSoundHandle* SoundHandleByName = v1->GetSoundHandleByName(&a1, "UIPopUpMessageGeneral", 289u);
            SoundHandleByName->Play(false);
            SlotMachineMenu::Close();
            return 0;
        }
    }
    for (int32_t m = 0; m < 3; ++m) {
        eReelResults[m] = static_cast<ReelSymbols>(2 * m);
        eReelStartSymbols[m] = NUM_SYMBOLS;
    }
    return 1;
}

void SlotMachineMenu::Prepare3DElements() {
    NiNode* v18;
    FNVD_INLINED_LOCAL(NiFixedString, v20, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, v21, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, v22, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, v23, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    NiMatrix3 mat;
    NiMatrix3 ecx0;
    NiControllerManager* v34[3];
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        v18 = ::new (apMemory) NiNode(0);
    }
    else {
        v18 = nullptr;
    }
    NiRefObject* v27 = (NiRefObject*)v18;
    SlotMachineMenu::spSlotModel.operator=(v18);
    NiNode* v17 = SlotMachineMenu::spSlotModel.m_pObject;
    v17->AttachChild(pSlotMachineGeometry, 1);
    NiNode* v16 = SlotMachineMenu::spSlotModel.m_pObject;
    v16->AttachChild(pReelGeometry[0], 1);
    NiNode* v15 = SlotMachineMenu::spSlotModel.m_pObject;
    v15->AttachChild(pReelGeometry[1], 1);
    NiNode* v14 = SlotMachineMenu::spSlotModel.m_pObject;
    v14->AttachChild(pReelGeometry[2], 1);
    ecx0.MakeXRotation(1.5707964f);
    mat.MakeZRotation(1.5707964f);
    SlotMachineMenu::spSlotModel.m_pObject->SetRotate(ecx0 * mat);
    SlotMachineMenu::spSlotModel.m_pObject->SetTranslate(175.0f, -18.0f, 0.0f);
    NiUpdateData arData(0.0f, false, false);
    SlotMachineMenu::spSlotModel.m_pObject->Update(arData);
    NiControllerManager* Controller =
        static_cast<NiControllerManager*>(pSlotMachineGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (Controller) {
        pSlotAnimManager = Controller;
        NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Forward");
        pSlotLeverAnim = pSlotAnimManager->GetSequenceByName(*kName);
        apString.~NiFixedString();
        NiFixedString* v12 = ::new (static_cast<void*>(&v23)) NiFixedString("Backward");
        pSlotCoinAnim = pSlotAnimManager->GetSequenceByName(*v12);
        v23.~NiFixedString();
        NiFixedString* v11 = ::new (static_cast<void*>(&v22)) NiFixedString("Payout_In");
        pSlotPayoutListAnim[0] = pSlotAnimManager->GetSequenceByName(*v11);
        v22.~NiFixedString();
        NiFixedString* v10 = ::new (static_cast<void*>(&v21)) NiFixedString("Payout_Out");
        pSlotPayoutListAnim[1] = pSlotAnimManager->GetSequenceByName(*v10);
        v21.~NiFixedString();
        pSlotAnimManager->DeactivateAll(0.0f);
        if (pSlotLeverAnim) {
            pSlotAnimManager->ActivateSequence(pSlotLeverAnim, 0, true, 1.0f, 0.0f, nullptr);
        }
        NiTimeController::StartAnimations(pSlotMachineGeometry);
    }
    for (int32_t i = 0; i < 3; ++i) {
        NiControllerManager* v2 =
            static_cast<NiControllerManager*>(pReelGeometry[i]->GetController(&NiControllerManager::ms_RTTI));
        v34[i] = v2;
    }
    for (int32_t j = 0; j < 3; ++j) {
        if (v34[j]) {
            NiControllerManager* v36[3];
            v36[j] = v34[j];
            NiFixedString* v9 = ::new (static_cast<void*>(&v20)) NiFixedString("Forward");
            NiControllerSequence* SequenceByName = v36[j]->GetSequenceByName(*v9);
            pSlotReelAnim[j] = SequenceByName;
            v20.~NiFixedString();
            v36[j]->DeactivateAll(0.0f);
            if (pSlotReelAnim[j]) {
                v36[j]->ActivateSequence(pSlotReelAnim[j], 0, true, 1.0f, 0.0f, nullptr);
            }
            NiTimeController::StartAnimations(pReelGeometry[j]);
        }
    }
    NiUpdateData arUpdateData(0.0f, false, false);
    SlotMachineMenu::spSlotModel.m_pObject->Update(arUpdateData);
    SlotMachineMenu::sp3DCamera.operator=(Create3DCamera());
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    ShadowSceneNode_->AttachChild(SlotMachineMenu::spSlotModel.m_pObject, 1);
    const NiBound* WorldBound = &SlotMachineMenu::spSlotModel.m_pObject->GetWorldBound();
    float v32 = WorldBound->m_fRadius * 3.0;
    NiNode* a2 = SlotMachineMenu::spSlotModel.m_pObject;
    ShadowSceneNode* v6 = BSShaderManager::GetShadowSceneNode(1);
    v6->AddAllLightsFromNode_Recurse(a2, v32);
    SwapFrontfacingTextures();
    SwapBackfacingTextures();
}

void SlotMachineMenu::SwapBackfacingTextures() {
    for (int32_t i = 0; i < 3; ++i) {
        if (eReelResults[i] - eReelStartSymbols[i]) {
            BSShaderPPLightingProperty* v25;
            int32_t v27;
            int32_t v28;
            BSShaderPPLightingProperty* v29;
            int32_t v31;
            int32_t v32;
            BSShaderPPLightingProperty* v33;
            int32_t v35;
            int32_t v36;
            BSShaderPPLightingProperty* v37;
            int32_t v39;
            int32_t v40;
            BSShaderPPLightingProperty* v41;
            int32_t v43;
            int32_t v44;
            BSShaderPPLightingProperty* v45;
            int32_t v47;
            int32_t v48;
            BSShaderPPLightingProperty* v49;
            int32_t v51;
            int32_t v52;
            BSShaderPPLightingProperty* v53;
            int32_t v55;
            int32_t v56;
            int32_t v58;
            int32_t v59;
            int32_t v60;
            int32_t v61;
            char blockName[264];
            int32_t v71;
            int32_t v72;
            int32_t v73;
            int32_t v75;
            if (eReelResults[i] % 2) {
                v61 = (eReelResults[i] + 3) / 2;
                v71 = (eReelResults[i] + 4) / 2;
                v59 = (eReelResults[i] + 5) / 2;
                v73 = (eReelResults[i] + 6) / 2;
                v75 = 7;
                v72 = 7;
                v60 = 7;
                v58 = 7;
                if (v61 > 6) {
                    v61 -= 7;
                }
                if (v71 > 6) {
                    v71 -= 7;
                }
                if (v59 > 6) {
                    v59 -= 7;
                }
                if (v73 > 6) {
                    v73 -= 7;
                }
            }
            else {
                v58 = eReelResults[i] / 2 + 3;
                v60 = eReelResults[i] / 2 + 4;
                v72 = eReelResults[i] / 2 + 5;
                v75 = eReelResults[i] / 2 + 6;
                v73 = 7;
                v59 = 7;
                v71 = 7;
                v61 = 7;
                if (v58 > 6) {
                    v58 -= 7;
                }
                if (v60 > 6) {
                    v60 -= 7;
                }
                if (v72 > 6) {
                    v72 -= 7;
                }
                if (v75 > 6) {
                    v75 -= 7;
                }
            }
            if (2 * i + 4 <= 13) {
                v56 = 2 * i + 4;
            }
            else {
                v56 = 2 * i - 10;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v56);
            NiNode* ObjectByName = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (ObjectByName->GetProperty(3)) {
                NiProperty* Property = ObjectByName->GetProperty(3);
                int32_t v54 = false;
                if (static_cast<NiShadeProperty*>(Property)->GetShaderPropertyType() >= 8) {
                    NiProperty* v2 = ObjectByName->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v2)->GetShaderPropertyType() <= 12) {
                        v54 = true;
                    }
                }
                v55 = v54;
            }
            else {
                v55 = 0;
            }
            if (v55) {
                v53 = static_cast<BSShaderPPLightingProperty*>(ObjectByName->GetProperty(3));
            }
            else {
                v53 = nullptr;
            }
            BSShaderPPLightingProperty* v74 = v53;
            if (v53) {
                NiSourceTexture* v17 = pSlotReelTextures[v58].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v17);
            }
            if (2 * i + 5 <= 13) {
                v52 = 2 * i + 5;
            }
            else {
                v52 = 2 * i - 9;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v52);
            NiNode* v63 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v63->GetProperty(3)) {
                NiProperty* v3 = v63->GetProperty(3);
                int32_t v50 = false;
                if (static_cast<NiShadeProperty*>(v3)->GetShaderPropertyType() >= 8) {
                    NiProperty* v4 = v63->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v4)->GetShaderPropertyType() <= 12) {
                        v50 = true;
                    }
                }
                v51 = v50;
            }
            else {
                v51 = 0;
            }
            if (v51) {
                v49 = static_cast<BSShaderPPLightingProperty*>(v63->GetProperty(3));
            }
            else {
                v49 = nullptr;
            }
            v74 = v49;
            if (v49) {
                NiSourceTexture* v18 = pSlotReelTextures[v61].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v18);
            }
            if (2 * i + 6 <= 13) {
                v48 = 2 * i + 6;
            }
            else {
                v48 = 2 * i - 8;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v48);
            NiNode* v64 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v64->GetProperty(3)) {
                NiProperty* v5 = v64->GetProperty(3);
                int32_t v46 = false;
                if (static_cast<NiShadeProperty*>(v5)->GetShaderPropertyType() >= 8) {
                    NiProperty* v6 = v64->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v6)->GetShaderPropertyType() <= 12) {
                        v46 = true;
                    }
                }
                v47 = v46;
            }
            else {
                v47 = 0;
            }
            if (v47) {
                v45 = static_cast<BSShaderPPLightingProperty*>(v64->GetProperty(3));
            }
            else {
                v45 = nullptr;
            }
            v74 = v45;
            if (v45) {
                NiSourceTexture* v19 = pSlotReelTextures[v60].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v19);
            }
            if (2 * i + 7 <= 13) {
                v44 = 2 * i + 7;
            }
            else {
                v44 = 2 * i - 7;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v44);
            NiNode* v65 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v65->GetProperty(3)) {
                NiProperty* v7 = v65->GetProperty(3);
                int32_t v42 = false;
                if (static_cast<NiShadeProperty*>(v7)->GetShaderPropertyType() >= 8) {
                    NiProperty* v8 = v65->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v8)->GetShaderPropertyType() <= 12) {
                        v42 = true;
                    }
                }
                v43 = v42;
            }
            else {
                v43 = 0;
            }
            if (v43) {
                v41 = static_cast<BSShaderPPLightingProperty*>(v65->GetProperty(3));
            }
            else {
                v41 = nullptr;
            }
            v74 = v41;
            if (v41) {
                NiSourceTexture* v20 = pSlotReelTextures[v71].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v20);
            }
            if (2 * i + 8 <= 13) {
                v40 = 2 * i + 8;
            }
            else {
                v40 = 2 * i - 6;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v40);
            NiNode* v66 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v66->GetProperty(3)) {
                NiProperty* v9 = v66->GetProperty(3);
                int32_t v38 = false;
                if (static_cast<NiShadeProperty*>(v9)->GetShaderPropertyType() >= 8) {
                    NiProperty* v10 = v66->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v10)->GetShaderPropertyType() <= 12) {
                        v38 = true;
                    }
                }
                v39 = v38;
            }
            else {
                v39 = 0;
            }
            if (v39) {
                v37 = static_cast<BSShaderPPLightingProperty*>(v66->GetProperty(3));
            }
            else {
                v37 = nullptr;
            }
            v74 = v37;
            if (v37) {
                NiSourceTexture* v21 = pSlotReelTextures[v72].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v21);
            }
            if (2 * i + 9 <= 13) {
                v36 = 2 * i + 9;
            }
            else {
                v36 = 2 * i - 5;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v36);
            NiNode* v67 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v67->GetProperty(3)) {
                NiProperty* v11 = v67->GetProperty(3);
                int32_t v34 = false;
                if (static_cast<NiShadeProperty*>(v11)->GetShaderPropertyType() >= 8) {
                    NiProperty* v12 = v67->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v12)->GetShaderPropertyType() <= 12) {
                        v34 = true;
                    }
                }
                v35 = v34;
            }
            else {
                v35 = 0;
            }
            if (v35) {
                v33 = static_cast<BSShaderPPLightingProperty*>(v67->GetProperty(3));
            }
            else {
                v33 = nullptr;
            }
            v74 = v33;
            if (v33) {
                NiSourceTexture* v22 = pSlotReelTextures[v59].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v22);
            }
            if (2 * i + 10 <= 13) {
                v32 = 2 * i + 10;
            }
            else {
                v32 = 2 * i - 4;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v32);
            NiNode* v68 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v68->GetProperty(3)) {
                NiProperty* v13 = v68->GetProperty(3);
                int32_t v30 = false;
                if (static_cast<NiShadeProperty*>(v13)->GetShaderPropertyType() >= 8) {
                    NiProperty* v14 = v68->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v14)->GetShaderPropertyType() <= 12) {
                        v30 = true;
                    }
                }
                v31 = v30;
            }
            else {
                v31 = 0;
            }
            if (v31) {
                v29 = static_cast<BSShaderPPLightingProperty*>(v68->GetProperty(3));
            }
            else {
                v29 = nullptr;
            }
            v74 = v29;
            if (v29) {
                NiSourceTexture* v23 = pSlotReelTextures[v75].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v23);
            }
            if (2 * i + 11 <= 13) {
                v28 = 2 * i + 11;
            }
            else {
                v28 = 2 * i - 3;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v28);
            NiNode* v69 = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pReelGeometry[i], static_cast<const char*>(blockName))
            );
            if (v69->GetProperty(3)) {
                NiProperty* v15 = v69->GetProperty(3);
                int32_t v26 = false;
                if (static_cast<NiShadeProperty*>(v15)->GetShaderPropertyType() >= 8) {
                    NiProperty* v16 = v69->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(v16)->GetShaderPropertyType() <= 12) {
                        v26 = true;
                    }
                }
                v27 = v26;
            }
            else {
                v27 = 0;
            }
            if (v27) {
                v25 = static_cast<BSShaderPPLightingProperty*>(v69->GetProperty(3));
            }
            else {
                v25 = nullptr;
            }
            v74 = v25;
            if (v25) {
                NiSourceTexture* v24 = pSlotReelTextures[v73].m_pObject;
                v74->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v24);
            }
        }
    }
}

NiCamera* SlotMachineMenu::Create3DCamera() {
    int32_t fHeight;
    NiCamera* v10;
    NiPoint3 v12;
    NiPoint3 dst;
    NiRect<float> a2;
    void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
    if (apMemory) {
        v10 = ::new (apMemory) NiCamera;
    }
    else {
        v10 = nullptr;
    }
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

// Each reel whose result changed gets the six visible faces of its cylinder retextured. The shade property's type is
// tested for the lighting shader (8 to 12), fetching the property again for each test, and the lighting property's
// SetTexture is called through the vtable.
void SlotMachineMenu::SwapFrontfacingTextures() {
    for (int32_t i = 0; i < 3; ++i) {
        if (eReelResults[i] - eReelStartSymbols[i]) {
            int32_t v49;
            int32_t v50;
            int32_t v51;
            int32_t v54;
            int32_t v55;
            int32_t v56;
            char blockName[268];
            if (eReelResults[i] % 2) {
                v55 = (eReelResults[i] + 1) / 2;
                v56 = (eReelResults[i] + 3) / 2;
                v51 = (eReelResults[i] - 1) / 2;
                v54 = 7;
                v50 = 7;
                v49 = 7;
                if (v55 > 6) {
                    v55 -= 7;
                }
                if (v56 > 6) {
                    v56 -= 7;
                }
                if (v51 < 0) {
                    v51 += 7;
                }
            }
            else {
                v49 = eReelResults[i] / 2;
                v50 = eReelResults[i] / 2 + 1;
                v54 = eReelResults[i] / 2 - 1;
                v51 = 7;
                v56 = 7;
                v55 = 7;
                if (v50 > 6) {
                    v50 -= 7;
                }
                if (v54 < 0) {
                    v54 += 7;
                }
            }
            int32_t v41;
            if (2 * i + 3 <= 13) {
                v41 = 2 * i + 3;
            }
            else {
                v41 = 2 * i - 11;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v41);
            NiAVObject* pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            BSShaderPPLightingProperty* pProperty = pFace->GetProperty(NiProperty::SHADE) &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >=
                        8 &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12
                ? static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE))
                : nullptr;
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[v56].m_pObject);
            }
            int32_t v37;
            if (2 * i + 2 <= 13) {
                v37 = 2 * i + 2;
            }
            else {
                v37 = 2 * i - 12;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v37);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = pFace->GetProperty(NiProperty::SHADE) &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >=
                        8 &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12
                ? static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE))
                : nullptr;
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[v50].m_pObject);
            }
            int32_t v33;
            if (2 * i + 1 <= 13) {
                v33 = 2 * i + 1;
            }
            else {
                v33 = 2 * i - 13;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v33);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = pFace->GetProperty(NiProperty::SHADE) &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >=
                        8 &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12
                ? static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE))
                : nullptr;
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[v55].m_pObject);
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, 2 * i);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = pFace->GetProperty(NiProperty::SHADE) &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >=
                        8 &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12
                ? static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE))
                : nullptr;
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[v49].m_pObject);
            }
            int32_t v26;
            if (2 * i - 1 >= 0) {
                v26 = 2 * i - 1;
            }
            else {
                v26 = 2 * i + 13;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v26);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = pFace->GetProperty(NiProperty::SHADE) &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >=
                        8 &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12
                ? static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE))
                : nullptr;
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[v51].m_pObject);
            }
            int32_t v22;
            if (2 * i - 2 >= 0) {
                v22 = 2 * i - 2;
            }
            else {
                v22 = 2 * i + 12;
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, v22);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = pFace->GetProperty(NiProperty::SHADE) &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >=
                        8 &&
                    static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12
                ? static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE))
                : nullptr;
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[v54].m_pObject);
            }
        }
    }
}

// a2 is compared signed, except in the switch.
[[clang::disable_tail_calls]] bool SlotMachineMenu::CalculateShouldWin(SlotMachineMenu::SlotResults a2) {
    bool result;
    int32_t v22 = std::abs(10 * (iLuckSkill - 5));
    if (v22 + RandomInt(0, 100) <= 100) {
        return false;
    }
    SlotMachineMenu::ReelSymbols v19 = eReelResults[0];
    SlotMachineMenu::ReelSymbols v20 = eReelResults[1];
    SlotMachineMenu::ReelSymbols v21 = eReelResults[2];
    if (iLuckSkill - 5 < 0) {
        if (static_cast<int32_t>(a2) >= 4) {
            if (static_cast<int32_t>(a2) <= 5) {
                for (int32_t i = 0; i < 3; ++i) {
                    if (eReelResults[i] == SlotMachineMenu::SL_WILD) {
                        eReelResults[i] = SlotMachineMenu::SL_WILDB;
                    }
                }
                bLucky = true;
            }
            else {
                if (a2 == 6) {
                    SlotMachineMenu::ReelSymbols v7;
                    if (eReelResults[0] + 1 <= 6) {
                        v7 = static_cast<SlotMachineMenu::ReelSymbols>(eReelResults[0] + 1);
                    }
                    else {
                        v7 = SlotMachineMenu::SL_A;
                    }
                    eReelResults[2] = v7;
                    eReelResults[1] = v7;
                    eReelResults[0] = v7;
                    bLucky = true;
                }
            }
        }
        return false;
    }
    switch (a2) {
        case 0: {
            while (static_cast<int32_t>(SpinReels()) < static_cast<int32_t>(a2)) {
                ;
            }
            SlotMachineMenu::SlotResults v2 = InterpretSpin();
            return CalculateShouldWin(v2);
        }
        case 1:
        case 5: {
            if (eReelResults[0] == eReelResults[1]) {
                int32_t v14 = iLuckSkill - 5;
                int32_t v4 = std::abs(eReelResults[0] - eReelResults[2]);
                if (v4 * v4 > iLuckSkill) {
                    return false;
                }
                eReelResults[2] = eReelResults[0];
                return true;
            }

            if (eReelResults[0] == eReelResults[2]) {
                int32_t v13 = iLuckSkill - 5;
                int32_t v5 = std::abs(eReelResults[0] - eReelResults[1]);
                if (v5 * v5 > iLuckSkill) {
                    return false;
                }
                eReelResults[1] = eReelResults[0];
                return true;
            }

            int32_t v12 = iLuckSkill - 5;
            int32_t v6 = std::abs(eReelResults[1] - eReelResults[0]);
            if (v6 * v6 > iLuckSkill) {
                return false;
            }
            eReelResults[0] = eReelResults[1];
            return true;
        }
        case 2: {
            if (eReelResults[0] == SlotMachineMenu::SL_WILD) {
                eReelResults[1] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                eReelResults[2] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
            }
            else {
                if (eReelResults[1] == SlotMachineMenu::SL_WILD) {
                    eReelResults[0] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                    eReelResults[2] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                }
                else {
                    eReelResults[0] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                    eReelResults[1] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                }
            }
            return static_cast<int32_t>(InterpretSpin()) > static_cast<int32_t>(a2);
        }
        case 4: {
            int32_t v8;
            int32_t v9;
            int32_t v18 = 0;
            int32_t v17 = -1;
            for (int32_t j = 0; j < 3; ++j) {
                if (eReelResults[j] == SlotMachineMenu::SL_WILD) {
                    v18 = j;
                }
                if (eReelResults[j] % 2) {
                    v17 = j;
                }
            }
            if (v18 - 1 < 0) {
                v9 = 2;
            }
            else {
                v9 = v18 - 1;
            }
            if (v18 + 1 >= 2) {
                v8 = 0;
            }
            else {
                v8 = v18 + 1;
            }
            int32_t v16 = std::abs(eReelResults[v9] - eReelResults[v8]);
            if (v16 <= iLuckSkill && v17 < 0) {
                eReelResults[v9] = eReelResults[v8];
                return true;
            }
            if (v16 > iLuckSkill) {
                return false;
            }
            if (v9 == v17) {
                eReelResults[v9] = eReelResults[v8];
            }
            else {
                eReelResults[v8] = eReelResults[v9];
            }
            result = true;
            break;
        }
        default: {
            return false;
        }
    }
    return result;
}

void SlotMachineMenu::UpdateEarningsString() {
    char Src[264];
    int32_t iTotalWinnings = g_slotMachineCasinoStats->iTotalWinnings;
    const char* v1 = sCasinoEarningsText.String();
    BSsprintf(Src, 260, "%s %s%i", SlotMachineMenu::pCasino->GetFullName(), v1, iTotalWinnings);
    int32_t v7 = BSstrlen(Src);
    if (v7 < 23) {
        const char* v2 = sCasinoEarningsText.String();
        BSsprintf(Src, 260, "%s %s", SlotMachineMenu::pCasino->GetFullName(), v2);
        for (int32_t i = 0; i < 23 - v7; ++i) {
            Src[v7 - 1 + i] = 32;
            Src[i + v7] = 0;
        }
        BSsprintf(Src, 260, "%s%i", Src, g_slotMachineCasinoStats->iTotalWinnings);
    }
    pTiles[2]->Set(g_slotMachineMenuTrait_Value, Src, true);
}

int32_t SlotMachineMenu::GetClassStatic() {
    return 1080;
}

void SlotMachineMenu::Close() {
    if (pMe) {
        pMe->eSlotState = SL_CLOSING_MENU;
        InterfaceManager::Instance()->GetCursorTile()->Set(Tile::kTileValue_Alpha, 255.0f, true);
    }
}

void SlotMachineMenu::DrawScene() {
    NiCamera* v6;
    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
    Renderer_->ClearBuffer(nullptr, 4u);
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* a1 = WorldSceneGraph->pCuller;
    a1->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* apCamera = SlotMachineMenu::sp3DCamera.m_pObject;
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
    apMemory.operator=(static_cast<NiRefObject*>(nullptr));
    a1->PopCullMode();
}

Tile* SlotMachineMenu::Create(
    TESCasino* apCasino,
    int32_t aiMinBet,
    int32_t aiMaxBet,
    int32_t aiMinWinnings,
    bool abDebugMode
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, v36, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    BSSimpleList<CasinoData*>* cl;
    Tile* pold_menu_tile = Menu::GetMenuByClass(1080);
    InterfaceManager* pin = InterfaceManager::Instance();
    g_slotMachineMaxBet = aiMaxBet;
    g_slotMachineMinBet = aiMinBet;
    SlotMachineMenu::pCasino = apCasino;
    g_slotMachineCasinoStats = nullptr;
    for (cl = pPlayer->GetCasinoDataList(); cl && cl->m_item; cl = cl->m_pkNext) {
        if (!cl->m_item->casinoID || !TESForm::GetFormByNumericID(cl->m_item->casinoID)) {
            Interface::ShowMessage(
                "Invalid Chip in CasinoData List!", 0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
            );
            return nullptr;
        }
        int32_t CastingType = SlotMachineMenu::pCasino->iFormID;
        if (CastingType == cl->m_item->casinoID) {
            g_slotMachineCasinoStats = cl->m_item;
            break;
        }
        if (!cl->m_pkNext) {
            break;
        }
    }
    if (!g_slotMachineCasinoStats) {
        CasinoData* cd;
        void* apMemory = ourMalloc(sizeof(CasinoData));
        if (apMemory) {
            cd = ::new (apMemory) CasinoData;
        }
        else {
            cd = nullptr;
        }
        CasinoData* cd_2 = cd;
        cd->casinoID = SlotMachineMenu::pCasino->iFormID;
        cd_2->iTotalWinnings = 0;
        cl = pPlayer->GetCasinoDataList();
        if (cl && !cl->IsEmpty()) {
            cl->AddHead(cd_2);
        }
        else {
            cl->SetItem(cd_2);
        }
        g_slotMachineCasinoStats = cd_2;
    }
    if (pPlayer->CasinoAntiCheatCheck()) {
        char msg[264];
        int32_t afValue = pPlayer->GetCasinoCheatTimeRemaining();
        BSsprintf(msg, 260, "%s\nTime Remaining: %i", sSlotAntiCheatText.String(), afValue);
        Interface::ShowMessage(
            msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio* v7 = &BSAudio::QInstance();
        BSSoundHandle* SoundHandleByName = v7->GetSoundHandleByName(&a1, "UIPopUpMessageGeneral", 289u);
        SoundHandleByName->Play(false);
        return nullptr;
    }

    CASINO_DATA* Data = &SlotMachineMenu::pCasino->data;
    if (g_slotMachineCasinoStats->iTotalWinnings < Data->iMaxWinnings) {
        TESCasinoChips* ChipType = SlotMachineMenu::pCasino->GetChipType();
        if (pPlayer->GetInventoryObjectCount(ChipType) >= g_slotMachineMinBet) {
            if (g_slotMachineCasinoStats->iTotalWinnings < aiMinWinnings && aiMinWinnings) {
                char apString[264];
                BSsprintf(apString, 260, sGamblingMinWinText.String());
                Interface::ShowMessage(
                    apString, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f,
                    false
                );
                BSAudio* v12 = &BSAudio::QInstance();
                BSSoundHandle* v31 = v12->GetSoundHandleByName(&apResult, "UIPopUpMessageGeneral", 289u);
                v31->Play(false);
                return nullptr;
            }

            int32_t v13;
            if (pold_menu_tile) {
                delete pold_menu_tile;
            }
            Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\slot_machine_menu.xml");
            SlotMachineMenu::pMe = static_cast<SlotMachineMenu*>(ptile->GetMenu());
            if (SlotMachineMenu::pMe &&
                ((v13 = SlotMachineMenu::pMe->GetClass(), v13 == SlotMachineMenu::GetClassStatic()) &&
                 SlotMachineMenu::pCasino)) {
                SlotMachineMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
                if (SlotMachineMenu::pMe->IsWellFormedMenu()) {
                    char Src[64];
                    if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                        float time = Tile::GetMaximumDepth();
                        ptile->Set(4013, time, true);
                    }
                    BSAudio* v14 = &BSAudio::QInstance();
                    BSSoundHandle* v30 = v14->GetSoundHandleByName(&v36, "GAMESlotsActivate", 289u);
                    v30->Play(false);
                    SlotMachineMenu::pMe->iLuckSkill = pPlayer->GetClampedActorValue(ActorValue::LUCK);
                    SlotMachineMenu::pMe->iCurrentBet = g_slotMachineMinBet;
                    const char* v15 = sCurrentBetText.String();
                    SlotMachineMenu::pMe->pTiles[0]->Set(g_slotMachineMenuTrait_Value, v15, true);
                    SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                    SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, g_slotMachineMinBet);
                    const char* v16 = sChipCountText.String();
                    SlotMachineMenu::pMe->pTiles[1]->Set(g_slotMachineMenuTrait_Value, v16, true);
                    SlotMachineMenu::pMe->pTiles[1]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                    TESCasinoChips* v17 = SlotMachineMenu::pCasino->GetChipType();
                    int32_t ItemCountinContainer = pPlayer->GetInventoryObjectCount(v17);
                    SlotMachineMenu::pMe->pTiles[1]->Set(SlotMachineMenu::iUT_X, ItemCountinContainer);
                    if (BSstrlen(SlotMachineMenu::pCasino->GetFullName())) {
                        const char* v19 = sCasinoEarningsText.String();
                        BSsprintf(Src, 64, "%s %s", SlotMachineMenu::pCasino->GetFullName(), v19);
                    }
                    else {
                        BSsprintf(Src, 64, "%s Earnings: ", SlotMachineMenu::pCasino->GetFormEditorID());
                    }
                    SlotMachineMenu::pMe->pTiles[2]->Set(g_slotMachineMenuTrait_Value, Src, true);
                    Tile* TileByName = Tile::GetTileByName(SlotMachineMenu::pMe->pTiles[2], "SMM_TotalEarningsValue");
                    TileByName->Set(4003, 0);
                    SlotMachineMenu::pMe->UpdateEarningsString();
                    const char* v21 = sSpinText.String();
                    SlotMachineMenu::pMe->pTiles[3]->Set(4036, v21, true);
                    const char* v22 = sIncreaseBetText.String();
                    SlotMachineMenu::pMe->pTiles[4]->Set(4036, v22, true);
                    const char* v23 = sDecreaseBetText.String();
                    SlotMachineMenu::pMe->pTiles[5]->Set(4036, v23, true);
                    const char* v24 = sPayoutListText.String();
                    SlotMachineMenu::pMe->pTiles[6]->Set(4036, v24, true);
                    const char* v25 = gs_sBetMax.String();
                    SlotMachineMenu::pMe->pTiles[7]->Set(4036, v25, true);
                    const char* v26 = sSlotPressAnyButtonText.String();
                    SlotMachineMenu::pMe->pTiles[9]->Set(4036, v26, true);
                    if (SlotMachineMenu::pMe->pTiles[8]) {
                        const char* v27 = sExit.String();
                        SlotMachineMenu::pMe->pTiles[8]->Set(4036, v27, true);
                    }
                    if (SlotMachineMenu::pMe->InitializeCasinoData()) {
                        SlotMachineMenu::pMe->Prepare3DElements();
                        SlotMachineMenu::pMe->PrepForVisibility(false);
                        return ptile;
                    }

                    SlotMachineMenu::Close();
                    return nullptr;
                }

                Warning(
                    "MENUS: SlotMachine Menu Creation Failed... Are your menu and art resources up to "
                    "date?"
                );
                return nullptr;
            }

            if ((SlotMachineMenu::pMe) && (SlotMachineMenu::pMe->pMenu) && (SlotMachineMenu::pMe)) {
                delete SlotMachineMenu::pMe;
            }

            return nullptr;
        }

        char apText[264];
        BSsprintf(apText, 260, sGamblingBrokeText.String());
        Interface::ShowMessage(
            apText, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio* v11 = &BSAudio::QInstance();
        BSSoundHandle* v32 = v11->GetSoundHandleByName(&th_s, "UIPopUpMessageGeneral", 289u);
        v32->Play(false);
        return nullptr;
    }

    char apDest[264];
    BSsprintf(apDest, 260, sGamblingBannedText.String());
    Interface::ShowMessage(
        apDest, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
    );
    BSAudio* v9 = &BSAudio::QInstance();
    BSSoundHandle* v33 = v9->GetSoundHandleByName(&arHandle, "UIPopUpMessageGeneral", 289u);
    v33->Play(false);
    return nullptr;
}
