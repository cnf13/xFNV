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
        TESCasinoChips* pChipType = SlotMachineMenu::pCasino->GetChipType();
        iChipCount = pPlayer->GetInventoryObjectCount(pChipType);
    }
}

SlotMachineMenu::~SlotMachineMenu() {
    SetLastAntiCheatTime();
    const char* pModelName = SlotMachineMenu::pCasino->GetModel(7)->GetModel();
    pModelLoader->ReleaseFile(pModelName);
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel01.NIF");
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel02.NIF");
    pModelLoader->ReleaseFile("Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel03.NIF");
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(SlotMachineMenu::spSlotModel.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    SlotMachineMenu::spSlotModel->DetachAllChildren();
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
    if (static_cast<uint32_t>(aiID) <= 9) {
        pTiles[aiID] = apTile;
    }
}

void SlotMachineMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (eSlotState == SlotMachineMenu::SL_IDLE) {
        switch (aiID) {
            case 3: {
                if (iChipCount >= iCurrentBet) {
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
                int32_t iOldBet = iCurrentBet;
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
                if (iCurrentBet > iChipCount) {
                    iCurrentBet = iChipCount;
                }
                if (iCurrentBet > g_slotMachineMaxBet) {
                    iCurrentBet = g_slotMachineMaxBet;
                }
                SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, iCurrentBet);
                if (iOldBet != iCurrentBet) {
                    char acSoundName[264];
                    BSsprintf(acSoundName, 260, "GAMESlotsIncreaseBet0%i", iNextSoundIndex);
                    BSAudio::QInstance().GetSoundHandleByName(acSoundName, 289u).Play(false);
                    iNextSoundIndex = iNextSoundIndex + 1 <= 3 ? iNextSoundIndex + 1 : 1;
                }
                break;
            }
            case 5: {
                int32_t iOldBet = iCurrentBet;
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
                if (iCurrentBet < g_slotMachineMinBet) {
                    iCurrentBet = g_slotMachineMinBet;
                }
                SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, iCurrentBet);
                if (iOldBet != iCurrentBet) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMESlotsDecreaseBet", 289u).Play(false);
                    iNextSoundIndex = 1;
                }
                break;
            }
            case 6: {
                eSlotState = SlotMachineMenu::SL_PAYOUT_IN_ANIM;
                break;
            }
            case 7: {
                if (iCurrentBet != g_slotMachineMaxBet) {
                    char acSoundName[268];
                    iCurrentBet = g_slotMachineMaxBet;
                    if (iCurrentBet > iChipCount) {
                        iCurrentBet = iChipCount;
                    }
                    SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                    SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, iCurrentBet);
                    BSsprintf(acSoundName, 260, "GAMESlotsIncreaseBet0%i", iNextSoundIndex);
                    BSAudio::QInstance().GetSoundHandleByName(acSoundName, 289u).Play(false);
                    iNextSoundIndex = iNextSoundIndex + 1 <= 3 ? iNextSoundIndex + 1 : 1;
                }
                break;
            }
            case 8: {
                SlotMachineMenu::Close();
                break;
            }
            default: {
                break;
            }
        }
    }
    else if (eSlotState == SlotMachineMenu::SL_PAYOUT_IN_ANIM) {
        eSlotState = SlotMachineMenu::SL_PAYOUT_OUT_ANIM;
    }
}

void SlotMachineMenu::DoIdle() {
    Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
    bool bisMenuOnTop = Interface::IsTopMenuID(eMenu);
    if (uiPrevIdleTick) {
        if (bisMenuOnTop) {
            int32_t ielaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
            if (SlotMachineMenu::eLastStage == eSlotState) {
                SlotMachineMenu::fTimeDelta = static_cast<double>(ielaspedMillis) / 1000.0 +
                    SlotMachineMenu::fTimeDelta;
            }
            else {
                SlotMachineMenu::fTimeDelta = 0.0f;
            }
        }
    }
    else {
        SlotMachineMenu::fTimeDelta = 0.0f;
        SlotMachineMenu::eLastStage = 0;
    }
    uiPrevIdleTick = appTimer.iLastTime;
    SlotMachineMenu::eLastStage = eSlotState;
    if (!bisMenuOnTop) {
        InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
    }
    switch (eSlotState) {
        case SlotMachineMenu::SL_INTRO_ANIM: {
            eSlotState = SlotMachineMenu::SL_IDLE;
            break;
        }
        case SlotMachineMenu::SL_SPIN_REELS_ANIM: {
            if (pSlotLeverAnim && pSlotReelAnim[0]) {
                if (pSlotCoinAnim) {
                    pSlotAnimManager->DeactivateSequence(pSlotCoinAnim, 0.0f);
                }
                if (SlotMachineMenu::fTimeDelta > 1.0) {
                    pTiles[9]->Set(4003, 0);
                }
                NiUpdateData kUpdateData1(SlotMachineMenu::fTimeDelta, true, false);
                NiUpdateData kUpdateData2(SlotMachineMenu::fTimeDelta, true, false);
                NiUpdateData kUpdateData3(SlotMachineMenu::fTimeDelta, true, false);
                NiUpdateData kUpdateData4(SlotMachineMenu::fTimeDelta, true, false);
                SlotMachineMenu::spSlotModel->GetAt(0)->Update(kUpdateData1);
                if (SlotMachineMenu::fTimeDelta == 0.0 && pTiles[1]->GetFloat(SlotMachineMenu::iUT_X) > 0.0) {
                    BSAudio::QInstance().GetSoundHandleByName("GameSlotsPullLever", 289u).Play(false);
                    bSwapTextures = true;
                }
                if (SlotMachineMenu::fTimeDelta == 0.0 || pTiles[1]->GetFloat(SlotMachineMenu::iUT_X) > 0.0) {
                    SlotMachineMenu::spSlotModel->GetAt(1u)->Update(kUpdateData2);
                    SlotMachineMenu::spSlotModel->GetAt(2u)->Update(kUpdateData3);
                    SlotMachineMenu::spSlotModel->GetAt(3u)->Update(kUpdateData4);
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
            int32_t iWinnings = 0;
            eSpinResult = InterpretSpin();
            if (eSpinResult == SlotMachineMenu::SL_THREE_MATCH) {
                switch (eReelResults[0]) {
                    case SlotMachineMenu::SL_A: {
                        iWinnings = 10 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    case SlotMachineMenu::SL_B: {
                        iWinnings = 20 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    case SlotMachineMenu::SL_C: {
                        iWinnings = 30 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    case SlotMachineMenu::SL_D: {
                        iWinnings = 40 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    case SlotMachineMenu::SL_E: {
                        iWinnings = 60 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    case SlotMachineMenu::SL_F: {
                        iWinnings = 100 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    case SlotMachineMenu::SL_WILD: {
                        iWinnings = 10 * iCurrentBet;
                        iChipCount += iWinnings;
                        break;
                    }
                    default: {
                        iChipCount -= iCurrentBet;
                        break;
                    }
                }
            }
            else if (eSpinResult == SlotMachineMenu::SL_WILD_PAIR) {
                iWinnings = 5 * iCurrentBet;
                iChipCount += iWinnings;
            }
            else {
                if (eSpinResult == SlotMachineMenu::SL_WILD_SYM_) {
                    iWinnings = 2 * iCurrentBet;
                }
                else {
                    iWinnings = -iCurrentBet;
                }
                iChipCount += iWinnings;
            }
            pTiles[1]->Poke(SlotMachineMenu::iUT_X, 1.0f);
            pTiles[1]->Set(SlotMachineMenu::iUT_X, iChipCount);
            g_slotMachineCasinoStats->iTotalWinnings += iWinnings;
            UpdateEarningsString();
            if (eSpinResult <= SlotMachineMenu::SL_WINNING_VALUES) {
                char acMessage[264];
                if (bLucky) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMESlotsLose", 289u).Play(false);
                    const char* pPlural = sPlural.String();
                    const char* pChipText = sCasinoChipText.String();
                    BSsprintf(acMessage, 260, "%s %d %s%s", sUnluckyLoseText.String(), -iWinnings, pChipText, pPlural);
                }
                else {
                    const char* pPlural = sPlural.String();
                    const char* pChipText = sCasinoChipText.String();
                    BSsprintf(acMessage, 260, "%s %d %s%s", sYouLoseText.String(), -iWinnings, pChipText, pPlural);
                }
                SlotMachineMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                SlotMachineMenu::pMe->pTiles[9]->Set(4036, acMessage, true);
                SlotMachineMenu::pMe->pTiles[9]->Set(4003, 1);
                eSlotState = SlotMachineMenu::SL_IDLE;
            }
            else {
                char acMessage[264];
                if (bLucky) {
                    const char* pPlural = sPlural.String();
                    const char* pChipText = sCasinoChipText.String();
                    BSsprintf(acMessage, 260, "%s %d %s%s", sLuckyWinText.String(), iWinnings, pChipText, pPlural);
                }
                else {
                    const char* pPlural = sPlural.String();
                    const char* pChipText = sCasinoChipText.String();
                    BSsprintf(acMessage, 260, "%s %d %s%s", sYouWinText.String(), iWinnings, pChipText, pPlural);
                }
                SlotMachineMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                SlotMachineMenu::pMe->pTiles[9]->Set(4036, acMessage, true);
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
                NiUpdateData kUpdateData(SlotMachineMenu::fTimeDelta, true, false);
                SlotMachineMenu::spSlotModel->GetAt(0)->Update(kUpdateData);
                if (SlotMachineMenu::fTimeDelta == 0.0) {
                    switch (eSpinResult) {
                        case SlotMachineMenu::SL_WILD_SYM_: {
                            BSAudio::QInstance().GetSoundHandleByName("GAMESlotsWinSmall", 289u).Play(false);
                            break;
                        }
                        case SlotMachineMenu::SL_WILD_PAIR: {
                            BSAudio::QInstance().GetSoundHandleByName("GAMESlotsWinMed", 289u).Play(false);
                            break;
                        }
                        case SlotMachineMenu::SL_THREE_MATCH: {
                            BSAudio::QInstance().GetSoundHandleByName("GAMESlotsWinJackpot", 289u).Play(false);
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
                    int32_t iMax = SlotMachineMenu::pCasino->data.iMaxWinnings;
                    int32_t iCurrentWinnings = g_slotMachineCasinoStats->iTotalWinnings;
                    int32_t iWinningLevel = g_slotMachineCasinoStats->iWinningLevel;
                    for (int32_t i = 1; i < 5; ++i) {
                        if (i > iWinningLevel) {
                            if (static_cast<double>(iCurrentWinnings) >=
                                static_cast<double>(iMax) * (static_cast<double>(i) * 0.25)) {
                                g_slotMachineCasinoStats->iWinningLevel = i;
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
                    const char* pPressAnyButtonText = sSlotPressAnyButtonText.String();
                    pTiles[9]->Set(4036, pPressAnyButtonText, true);
                    pSlotAnimManager->DeactivateAll(0.0f);
                    pSlotAnimManager->ActivateSequence(pSlotPayoutListAnim[0], 0, false, 1.0f, 0.0f, nullptr);
                }
                if (static_cast<double>(SlotMachineMenu::fTimeDelta) <= pSlotPayoutListAnim[0]->m_fEndKeyTime) {
                    NiUpdateData kUpdateData(SlotMachineMenu::fTimeDelta, true, false);
                    SlotMachineMenu::spSlotModel->GetAt(0)->Update(kUpdateData);
                }
                else {
                    pSlotAnimManager->DeactivateSequence(pSlotPayoutListAnim[0], 0.0f);
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
                if (static_cast<double>(SlotMachineMenu::fTimeDelta) <= pSlotPayoutListAnim[1]->m_fEndKeyTime) {
                    NiUpdateData kUpdateData(SlotMachineMenu::fTimeDelta, true, false);
                    SlotMachineMenu::spSlotModel->GetAt(0)->Update(kUpdateData);
                }
                else {
                    pSlotAnimManager->DeactivateSequence(pSlotPayoutListAnim[1], 0.0f);
                    pSlotAnimManager->ActivateSequence(pSlotLeverAnim, 0, false, 1.0f, 0.0f, nullptr);
                    eSlotState = SlotMachineMenu::SL_IDLE;
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
                    msg,
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                    nullptr,
                    2.0f,
                    false
                );
                BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
                SlotMachineMenu::Close();
            }
            break;
        }
        case SlotMachineMenu::SL_CLOSING_MENU: {
            if (xFadeState != Interface::FADING_OUT) {
                InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
                pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
                StartFadeOut();
                TESCasinoChips* pChipType = SlotMachineMenu::pCasino->GetChipType();
                int32_t iChipChange = iChipCount - pPlayer->GetInventoryObjectCount(pChipType);
                if (iChipChange >= 0) {
                    if (iChipChange > 0) {
                        TESContainer tempcont;
                        tempcont.AddObject(SlotMachineMenu::pCasino->GetChipType(), iChipChange, nullptr);
                        if (g_slotMachineCasinoStats->iTotalWinnings >= SlotMachineMenu::pCasino->GetMaxWinnings()) {
                            tempcont.AddContainerToReference(pPlayer, true);
                            BSStringT<char> kMessage;
                            const char* pBannedText = sGamblingBannedText.String();
                            const char* pAddedText = sAddItemtoInventory.String();
                            const char* pPlural = sPlural.String();
                            const char* pFullName = TESFullName::GetFullName(SlotMachineMenu::pCasino->GetChipType());
                            kMessage
                                .SPrintF("%i %s%s %s\n%s", iChipChange, pFullName, pPlural, pAddedText, pBannedText);
                            pChipType = SlotMachineMenu::pCasino->GetChipType();
                            const char* pPickUpSoundName = pPlayer->GetPickUpSoundName(pChipType, true, false);
                            Interface::ShowMessage(
                                kMessage.pString,
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
                                pPickUpSoundName,
                                2.0f,
                                false
                            );
                        }
                        else {
                            tempcont.AddContainerToReference(pPlayer, false);
                        }
                    }
                }
                else {
                    pChipType = SlotMachineMenu::pCasino->GetChipType();
                    pPlayer->RemoveItem(
                        pChipType,
                        nullptr,
                        -iChipChange,
                        false,
                        false,
                        nullptr,
                        nullptr,
                        nullptr,
                        true,
                        false
                    );
                    BSStringT<char> kMessage;
                    const char* pRemovedText = sRemoveItemfromInventory.String();
                    const char* pPlural = sPlural.String();
                    const char* pFullName = TESFullName::GetFullName(SlotMachineMenu::pCasino->GetChipType());
                    kMessage.SPrintF("%i %s%s %s", -iChipChange, pFullName, pPlural, pRemovedText);
                    pChipType = SlotMachineMenu::pCasino->GetChipType();
                    const char* pPickUpSoundName = pPlayer->GetPickUpSoundName(pChipType, true, false);
                    Interface::ShowMessage(
                        kMessage.pString,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                        pPickUpSoundName,
                        2.0f,
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
            break;
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
    int32_t iWildCount = 0;
    for (SlotMachineMenu::ReelSymbols eResult : eReelResults) {
        if (eResult == SlotMachineMenu::SL_WILD) {
            ++iWildCount;
        }
    }
    if (eReelResults[0] == eReelResults[1] &&
        eReelResults[0] == eReelResults[2] &&
        !(static_cast<int32_t>(eReelResults[1]) % 2)) {
        return SlotMachineMenu::SL_THREE_MATCH;
    }
    if (iWildCount == 2) {
        return SlotMachineMenu::SL_WILD_PAIR;
    }
    if (iWildCount == 1) {
        return SlotMachineMenu::SL_WILD_SYM_;
    }
    if (eReelResults[0] == SlotMachineMenu::SL_WILD ||
        eReelResults[1] == SlotMachineMenu::SL_WILD ||
        eReelResults[2] == SlotMachineMenu::SL_WILD) {
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
    char acTextureName[264];
    char acPath[268];
    if (!SlotMachineMenu::pCasino) {
        return 0;
    }
    for (int32_t index = 0; index < 7; ++index) {
        if (SlotMachineMenu::pCasino) {
            BSsprintf(
                acTextureName,
                260,
                "%s%s",
                "Textures\\",
                SlotMachineMenu::pCasino->GetTexture(index)->GetTextureName()
            );
            SwapPlatformLanguageTexturePath(acTextureName, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, pSlotReelTextures[index], false, false);
        }
    }
    BSsprintf(acTextureName, 260, "%sNV_SlotMachine-Symbol%s.dds", "Textures\\terminals\\NV_SlotMachine\\", "S");
    SwapPlatformLanguageTexturePath(acTextureName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, pSlotReelTextures[7], false, false);
    const char* pModelName = SlotMachineMenu::pCasino->GetModel(7)->GetModel();
    pSlotMachineGeometry = pModelLoader->LoadFile(pModelName, LOD_MULT_NONE, true, false, false, false);
    pReelGeometry[0] = pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel01.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    pReelGeometry[1] = pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel02.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    pReelGeometry[2] = pModelLoader->LoadFile(
        "Meshes\\terminals\\NV_SlotMachine\\NV_SlotMachine-Reel03.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    bRunScriptOnExit = false;
    return SetupActiveReels();
}

char SlotMachineMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 10; ++i) {
        if (!pTiles[i]) {
            return 0;
        }
    }
    return 1;
}

char SlotMachineMenu::SetupActiveReels() {
    uint32_t auiSlotStops[7];
    int32_t iActiveSymbol = 0;
    for (SlotMachineMenu::ReelSymbols& eSymbol : eActiveReelSymbols) {
        eSymbol = NUM_SYMBOLS;
    }
    uint32_t uiReelSymbol = 0;
    auiSlotStops[0] = SlotMachineMenu::pCasino->data.iSlotStop_A;
    auiSlotStops[1] = SlotMachineMenu::pCasino->data.iSlotStop_B;
    auiSlotStops[2] = SlotMachineMenu::pCasino->data.iSlotStop_C;
    auiSlotStops[3] = SlotMachineMenu::pCasino->data.iSlotStop_D;
    auiSlotStops[4] = SlotMachineMenu::pCasino->data.iSlotStop_E;
    auiSlotStops[5] = SlotMachineMenu::pCasino->data.iSlotStop_F;
    auiSlotStops[6] = SlotMachineMenu::pCasino->data.iSlotStop_W;
    iNumActiveSymbols = 7;
    for (uint32_t uiSymbol : auiSlotStops) {
        if (!uiSymbol) {
            --iNumActiveSymbols;
        }
    }
    while (iActiveSymbol < 28) {
        int32_t iPassStart = iActiveSymbol;
        uiReelSymbol = 0;
        for (int32_t k = 0; k < 7; ++k) {
            if (auiSlotStops[k]) {
                eActiveReelSymbols[iActiveSymbol++] = static_cast<ReelSymbols>(uiReelSymbol);
                eActiveReelSymbols[iActiveSymbol++] = static_cast<ReelSymbols>(++uiReelSymbol);
                --auiSlotStops[k];
            }
            ++uiReelSymbol;
        }
        if (iPassStart == iActiveSymbol) {
            char msg[268];
            BSsprintf(
                msg,
                260u,
                "Casino Form is not setup properly. # of stops for the slot reels must sum to 14. Exiting out of Slot "
                "machine menu."
            );
            Interface::ShowMessage(
                msg,
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                nullptr,
                2.0f,
                false
            );
            BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
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
    NiMatrix3 mat;
    NiMatrix3 kXRotation;
    NiControllerManager* pReelManagers[3];
    SlotMachineMenu::spSlotModel = new NiNode(0);
    SlotMachineMenu::spSlotModel->AttachChild(pSlotMachineGeometry, 1);
    SlotMachineMenu::spSlotModel->AttachChild(pReelGeometry[0], 1);
    SlotMachineMenu::spSlotModel->AttachChild(pReelGeometry[1], 1);
    SlotMachineMenu::spSlotModel->AttachChild(pReelGeometry[2], 1);
    kXRotation.MakeXRotation(1.5707964f);
    mat.MakeZRotation(1.5707964f);
    SlotMachineMenu::spSlotModel->SetRotate(kXRotation * mat);
    SlotMachineMenu::spSlotModel->SetTranslate(175.0f, -18.0f, 0.0f);
    NiUpdateData arData(0.0f, false, false);
    SlotMachineMenu::spSlotModel->Update(arData);
    NiControllerManager* pController = static_cast<NiControllerManager*>(
        pSlotMachineGeometry->GetController(&NiControllerManager::ms_RTTI)
    );
    if (pController) {
        pSlotAnimManager = pController;
        pSlotLeverAnim = pSlotAnimManager->GetSequenceByName("Forward");
        pSlotCoinAnim = pSlotAnimManager->GetSequenceByName("Backward");
        pSlotPayoutListAnim[0] = pSlotAnimManager->GetSequenceByName("Payout_In");
        pSlotPayoutListAnim[1] = pSlotAnimManager->GetSequenceByName("Payout_Out");
        pSlotAnimManager->DeactivateAll(0.0f);
        if (pSlotLeverAnim) {
            pSlotAnimManager->ActivateSequence(pSlotLeverAnim, 0, true, 1.0f, 0.0f, nullptr);
        }
        NiTimeController::StartAnimations(pSlotMachineGeometry);
    }
    for (int32_t i = 0; i < 3; ++i) {
        pReelManagers[i] = static_cast<NiControllerManager*>(
            pReelGeometry[i]->GetController(&NiControllerManager::ms_RTTI)
        );
    }
    for (int32_t j = 0; j < 3; ++j) {
        if (pReelManagers[j]) {
            NiControllerManager* pReelManager = pReelManagers[j];
            pSlotReelAnim[j] = pReelManager->GetSequenceByName("Forward");
            pReelManager->DeactivateAll(0.0f);
            if (pSlotReelAnim[j]) {
                pReelManager->ActivateSequence(pSlotReelAnim[j], 0, true, 1.0f, 0.0f, nullptr);
            }
            NiTimeController::StartAnimations(pReelGeometry[j]);
        }
    }
    NiUpdateData arUpdateData(0.0f, false, false);
    SlotMachineMenu::spSlotModel->Update(arUpdateData);
    SlotMachineMenu::sp3DCamera = Create3DCamera();
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(SlotMachineMenu::spSlotModel.m_pObject, 1);
    float fLightRadius = SlotMachineMenu::spSlotModel->GetWorldBound().m_fRadius * 3.0;
    NiNode* pModel = SlotMachineMenu::spSlotModel.m_pObject;
    BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pModel, fLightRadius);
    SwapFrontfacingTextures();
    SwapBackfacingTextures();
}

void SlotMachineMenu::SwapBackfacingTextures() {
    for (int32_t i = 0; i < 3; ++i) {
        if (eReelResults[i] - eReelStartSymbols[i]) {
            int32_t iTexturePlus4;
            int32_t iTexturePlus5;
            int32_t iTexturePlus6;
            int32_t iTexturePlus7;
            int32_t iTexturePlus8;
            int32_t iTexturePlus9;
            int32_t iTexturePlus10;
            int32_t iTexturePlus11;
            char blockName[264];
            if (eReelResults[i] % 2) {
                iTexturePlus5 = (eReelResults[i] + 3) / 2;
                iTexturePlus7 = (eReelResults[i] + 4) / 2;
                iTexturePlus9 = (eReelResults[i] + 5) / 2;
                iTexturePlus11 = (eReelResults[i] + 6) / 2;
                iTexturePlus10 = 7;
                iTexturePlus8 = 7;
                iTexturePlus6 = 7;
                iTexturePlus4 = 7;
                if (iTexturePlus5 > 6) {
                    iTexturePlus5 -= 7;
                }
                if (iTexturePlus7 > 6) {
                    iTexturePlus7 -= 7;
                }
                if (iTexturePlus9 > 6) {
                    iTexturePlus9 -= 7;
                }
                if (iTexturePlus11 > 6) {
                    iTexturePlus11 -= 7;
                }
            }
            else {
                iTexturePlus4 = eReelResults[i] / 2 + 3;
                iTexturePlus6 = eReelResults[i] / 2 + 4;
                iTexturePlus8 = eReelResults[i] / 2 + 5;
                iTexturePlus10 = eReelResults[i] / 2 + 6;
                iTexturePlus11 = 7;
                iTexturePlus9 = 7;
                iTexturePlus7 = 7;
                iTexturePlus5 = 7;
                if (iTexturePlus4 > 6) {
                    iTexturePlus4 -= 7;
                }
                if (iTexturePlus6 > 6) {
                    iTexturePlus6 -= 7;
                }
                if (iTexturePlus8 > 6) {
                    iTexturePlus8 -= 7;
                }
                if (iTexturePlus10 > 6) {
                    iTexturePlus10 -= 7;
                }
            }
            int32_t iFace = 2 * i + 4 <= 13 ? 2 * i + 4 : 2 * i - 10;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            NiAVObject* pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            BSShaderPPLightingProperty* pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus4].m_pObject);
            }
            iFace = 2 * i + 5 <= 13 ? 2 * i + 5 : 2 * i - 9;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus5].m_pObject);
            }
            iFace = 2 * i + 6 <= 13 ? 2 * i + 6 : 2 * i - 8;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus6].m_pObject);
            }
            iFace = 2 * i + 7 <= 13 ? 2 * i + 7 : 2 * i - 7;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus7].m_pObject);
            }
            iFace = 2 * i + 8 <= 13 ? 2 * i + 8 : 2 * i - 6;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus8].m_pObject);
            }
            iFace = 2 * i + 9 <= 13 ? 2 * i + 9 : 2 * i - 5;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus9].m_pObject);
            }
            iFace = 2 * i + 10 <= 13 ? 2 * i + 10 : 2 * i - 4;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty
                    ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus10].m_pObject);
            }
            iFace = 2 * i + 11 <= 13 ? 2 * i + 11 : 2 * i - 3;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty
                    ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus11].m_pObject);
            }
        }
    }
}

NiCamera* SlotMachineMenu::Create3DCamera() {
    NiPoint3 kTarget;
    NiPoint3 kUp;
    NiRect<float> kViewPort;
    NiCamera* pCamera = new NiCamera;
    ::new (&kViewPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pCamera->SetViewPort(kViewPort);
    NiFrustum kFrustum(false);
    kFrustum.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    float fFarNearRatio = kFrustum.m_fFar / kFrustum.m_fNear;
    pCamera->SetMaxFarNearRatio(fFarNearRatio);
    int32_t iHeight = BSShaderManager::bLetterbox ? BSShaderManager::iLetterboxHeight : Renderer::GetScreenHeight();
    float fAspectRatio = static_cast<double>(iHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fFOV = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    kFrustum.m_fLeft = -std::tan(fFOV) * 0.75f;
    kFrustum.m_fRight = std::tan(fFOV) * 0.75f;
    double dTan = std::tan(fFOV);
    kFrustum.m_fBottom = -dTan * fAspectRatio * 0.75f;
    dTan = std::tan(fFOV);
    kFrustum.m_fTop = dTan * fAspectRatio * 0.75f;
    kFrustum.m_bOrtho = false;
    pCamera->SetViewFrustum(kFrustum);
    ::new (&kUp) NiPoint3(0.0f, 0.0f, 1.0f);
    ::new (&kTarget) NiPoint3(0.0f, 0.0f, 1.0f);
    pCamera->LookAtWorldPoint(kTarget, kUp);
    pCamera->SetMinNearPlaneDist(1.0f);
    NiUpdateData arData(0.0f, false, false);
    pCamera->Update(arData);
    NiFixedString kName("Surgery3DCamera");
    pCamera->SetName(kName);
    return pCamera;
}

// Each reel whose result changed gets the six visible faces of its cylinder retextured. The shade property's type is
// tested for the lighting shader (8 to 12), fetching the property again for each test, and the lighting property's
// SetTexture is called through the vtable.
void SlotMachineMenu::SwapFrontfacingTextures() {
    for (int32_t i = 0; i < 3; ++i) {
        if (eReelResults[i] - eReelStartSymbols[i]) {
            int32_t iTexturePlus3;
            int32_t iTexturePlus2;
            int32_t iTexturePlus1;
            int32_t iTexture0;
            int32_t iTextureMinus1;
            int32_t iTextureMinus2;
            char blockName[268];
            if (eReelResults[i] % 2) {
                iTexturePlus1 = (eReelResults[i] + 1) / 2;
                iTexturePlus3 = (eReelResults[i] + 3) / 2;
                iTextureMinus1 = (eReelResults[i] - 1) / 2;
                iTextureMinus2 = 7;
                iTexturePlus2 = 7;
                iTexture0 = 7;
                if (iTexturePlus1 > 6) {
                    iTexturePlus1 -= 7;
                }
                if (iTexturePlus3 > 6) {
                    iTexturePlus3 -= 7;
                }
                if (iTextureMinus1 < 0) {
                    iTextureMinus1 += 7;
                }
            }
            else {
                iTexture0 = eReelResults[i] / 2;
                iTexturePlus2 = eReelResults[i] / 2 + 1;
                iTextureMinus2 = eReelResults[i] / 2 - 1;
                iTextureMinus1 = 7;
                iTexturePlus3 = 7;
                iTexturePlus1 = 7;
                if (iTexturePlus2 > 6) {
                    iTexturePlus2 -= 7;
                }
                if (iTextureMinus2 < 0) {
                    iTextureMinus2 += 7;
                }
            }
            int32_t iFace = 2 * i + 3 <= 13 ? 2 * i + 3 : 2 * i - 11;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            NiAVObject* pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            BSShaderPPLightingProperty* pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus3].m_pObject);
            }
            iFace = 2 * i + 2 <= 13 ? 2 * i + 2 : 2 * i - 12;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus2].m_pObject);
            }
            iFace = 2 * i + 1 <= 13 ? 2 * i + 1 : 2 * i - 13;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexturePlus1].m_pObject);
            }
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, 2 * i);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTexture0].m_pObject);
            }
            iFace = 2 * i - 1 >= 0 ? 2 * i - 1 : 2 * i + 13;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty
                    ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTextureMinus1].m_pObject);
            }
            iFace = 2 * i - 2 >= 0 ? 2 * i - 2 : 2 * i + 12;
            BSsprintf(blockName, 260, "Cylinder0%i:%i", 6 - i, iFace);
            pFace = BSUtilities::GetObjectByName(pReelGeometry[i], blockName);
            pProperty = nullptr;
            if (pFace->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() >= 8 &&
                static_cast<NiShadeProperty*>(pFace->GetProperty(NiProperty::SHADE))->GetShaderPropertyType() <= 12) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(NiProperty::SHADE));
            }
            if (pProperty) {
                pProperty
                    ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pSlotReelTextures[iTextureMinus2].m_pObject);
            }
        }
    }
}

// aeSpinResult is compared signed, except in the switch.
[[clang::disable_tail_calls]] bool SlotMachineMenu::CalculateShouldWin(SlotMachineMenu::SlotResults aeSpinResult) {
    int32_t iLuckBonus = std::abs(10 * (iLuckSkill - 5));
    if (iLuckBonus + RandomInt(0, 100) <= 100) {
        return false;
    }
    if (iLuckSkill - 5 < 0) {
        if (static_cast<int32_t>(aeSpinResult) >= 4) {
            if (static_cast<int32_t>(aeSpinResult) <= 5) {
                for (SlotMachineMenu::ReelSymbols& eResult : eReelResults) {
                    if (eResult == SlotMachineMenu::SL_WILD) {
                        eResult = SlotMachineMenu::SL_WILDB;
                    }
                }
                bLucky = true;
            }
            else if (aeSpinResult == 6) {
                SlotMachineMenu::ReelSymbols eSymbol;
                if (eReelResults[0] + 1 <= 6) {
                    eSymbol = static_cast<SlotMachineMenu::ReelSymbols>(eReelResults[0] + 1);
                }
                else {
                    eSymbol = SlotMachineMenu::SL_A;
                }
                eReelResults[2] = eSymbol;
                eReelResults[1] = eSymbol;
                eReelResults[0] = eSymbol;
                bLucky = true;
            }
        }
        return false;
    }
    switch (aeSpinResult) {
        case 0: {
            while (static_cast<int32_t>(SpinReels()) < static_cast<int32_t>(aeSpinResult)) {
                ;
            }
            return CalculateShouldWin(InterpretSpin());
        }
        case 1:
        case 5: {
            if (eReelResults[0] == eReelResults[1]) {
                int32_t iDistance = std::abs(eReelResults[0] - eReelResults[2]);
                if (iDistance * iDistance > iLuckSkill) {
                    return false;
                }
                eReelResults[2] = eReelResults[0];
                return true;
            }

            if (eReelResults[0] == eReelResults[2]) {
                int32_t iDistance = std::abs(eReelResults[0] - eReelResults[1]);
                if (iDistance * iDistance > iLuckSkill) {
                    return false;
                }
                eReelResults[1] = eReelResults[0];
                return true;
            }

            int32_t iDistance = std::abs(eReelResults[1] - eReelResults[0]);
            if (iDistance * iDistance > iLuckSkill) {
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
            else if (eReelResults[1] == SlotMachineMenu::SL_WILD) {
                eReelResults[0] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                eReelResults[2] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
            }
            else {
                eReelResults[0] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
                eReelResults[1] = eActiveReelSymbols[2 * RandomInt(0, iNumActiveSymbols)];
            }
            return static_cast<int32_t>(InterpretSpin()) > static_cast<int32_t>(aeSpinResult);
        }
        case 4: {
            int32_t iWildReel = 0;
            int32_t iBlankReel = -1;
            for (int32_t j = 0; j < 3; ++j) {
                if (eReelResults[j] == SlotMachineMenu::SL_WILD) {
                    iWildReel = j;
                }
                if (eReelResults[j] % 2) {
                    iBlankReel = j;
                }
            }
            int32_t iPrevReel = iWildReel - 1 < 0 ? 2 : iWildReel - 1;
            int32_t iNextReel = iWildReel + 1 >= 2 ? 0 : iWildReel + 1;
            int32_t iDistance = std::abs(eReelResults[iPrevReel] - eReelResults[iNextReel]);
            if (iDistance <= iLuckSkill && iBlankReel < 0) {
                eReelResults[iPrevReel] = eReelResults[iNextReel];
                return true;
            }
            if (iDistance > iLuckSkill) {
                return false;
            }
            if (iPrevReel == iBlankReel) {
                eReelResults[iPrevReel] = eReelResults[iNextReel];
            }
            else {
                eReelResults[iNextReel] = eReelResults[iPrevReel];
            }
            return true;
        }
        default: {
            return false;
        }
    }
}

void SlotMachineMenu::UpdateEarningsString() {
    char acEarnings[264];
    int32_t iTotalWinnings = g_slotMachineCasinoStats->iTotalWinnings;
    const char* pEarningsText = sCasinoEarningsText.String();
    BSsprintf(acEarnings, 260, "%s %s%i", SlotMachineMenu::pCasino->GetFullName(), pEarningsText, iTotalWinnings);
    int32_t iLength = BSstrlen(acEarnings);
    if (iLength < 23) {
        pEarningsText = sCasinoEarningsText.String();
        BSsprintf(acEarnings, 260, "%s %s", SlotMachineMenu::pCasino->GetFullName(), pEarningsText);
        for (int32_t i = 0; i < 23 - iLength; ++i) {
            acEarnings[iLength - 1 + i] = 32;
            acEarnings[i + iLength] = 0;
        }
        BSsprintf(acEarnings, 260, "%s%i", acEarnings, g_slotMachineCasinoStats->iTotalWinnings);
    }
    pTiles[2]->Set(g_slotMachineMenuTrait_Value, acEarnings, true);
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
    NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
    SceneGraph* pWorldSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pWorldSceneGraph->pCuller;
    pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* pCamera = SlotMachineMenu::sp3DCamera.m_pObject;
    NiCamera::HandleEyeFinityFOVScale(pCamera, 0.15f);
    NiPointer<NiRefObject> spAccumulator(static_cast<NiRefObject*>(InterfaceManager::Instance()->Get3dAccumulator()));
    pCuller->SetAccumulator(static_cast<NiAccumulator*>(spAccumulator.m_pObject));
    NiCamera* pSceneCamera = pCamera ? pCamera : pWorldSceneGraph->GetCamera();
    BSShaderUtil::AccumulateScene(pSceneCamera, BSShaderManager::GetShadowSceneNode(1), *pCuller);
    BSShaderUtil::RenderScene(pSceneCamera, static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject), false);
    pCuller->SetAccumulator(nullptr);
    spAccumulator = static_cast<NiRefObject*>(nullptr);
    pCuller->PopCullMode();
}

Tile* SlotMachineMenu::Create(
    TESCasino* apCasino,
    int32_t aiMinBet,
    int32_t aiMaxBet,
    int32_t aiMinWinnings,
    bool abDebugMode
) {
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
                "Invalid Chip in CasinoData List!",
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                nullptr,
                2.0f,
                false
            );
            return nullptr;
        }
        if (SlotMachineMenu::pCasino->iFormID == cl->m_item->casinoID) {
            g_slotMachineCasinoStats = cl->m_item;
            break;
        }
        if (!cl->m_pkNext) {
            break;
        }
    }
    if (!g_slotMachineCasinoStats) {
        CasinoData* cd = new CasinoData;
        cd->casinoID = SlotMachineMenu::pCasino->iFormID;
        cd->iTotalWinnings = 0;
        cl = pPlayer->GetCasinoDataList();
        if (cl && !cl->IsEmpty()) {
            cl->AddHead(cd);
        }
        else {
            cl->SetItem(cd);
        }
        g_slotMachineCasinoStats = cd;
    }
    if (pPlayer->CasinoAntiCheatCheck()) {
        char msg[264];
        int32_t iTimeRemaining = pPlayer->GetCasinoCheatTimeRemaining();
        BSsprintf(msg, 260, "%s\nTime Remaining: %i", sSlotAntiCheatText.String(), iTimeRemaining);
        Interface::ShowMessage(
            msg,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
            nullptr,
            2.0f,
            false
        );
        BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
        return nullptr;
    }

    if (g_slotMachineCasinoStats->iTotalWinnings < SlotMachineMenu::pCasino->data.iMaxWinnings) {
        TESCasinoChips* pChipType = SlotMachineMenu::pCasino->GetChipType();
        if (pPlayer->GetInventoryObjectCount(pChipType) >= g_slotMachineMinBet) {
            if (g_slotMachineCasinoStats->iTotalWinnings < aiMinWinnings && aiMinWinnings) {
                char acMessage[264];
                BSsprintf(acMessage, 260, sGamblingMinWinText.String());
                Interface::ShowMessage(
                    acMessage,
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                    nullptr,
                    2.0f,
                    false
                );
                BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
                return nullptr;
            }

            if (pold_menu_tile) {
                delete pold_menu_tile;
            }
            Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\slot_machine_menu.xml");
            SlotMachineMenu::pMe = static_cast<SlotMachineMenu*>(ptile->GetMenu());
            if (SlotMachineMenu::pMe &&
                SlotMachineMenu::pMe->GetClass() == SlotMachineMenu::GetClassStatic() &&
                SlotMachineMenu::pCasino) {
                SlotMachineMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
                if (SlotMachineMenu::pMe->IsWellFormedMenu()) {
                    char acEarningsName[64];
                    if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                        ptile->Set(4013, Tile::GetMaximumDepth(), true);
                    }
                    BSAudio::QInstance().GetSoundHandleByName("GAMESlotsActivate", 289u).Play(false);
                    SlotMachineMenu::pMe->iLuckSkill = pPlayer->GetClampedActorValue(ActorValue::LUCK);
                    SlotMachineMenu::pMe->iCurrentBet = g_slotMachineMinBet;
                    const char* pCurrentBetText = sCurrentBetText.String();
                    SlotMachineMenu::pMe->pTiles[0]->Set(g_slotMachineMenuTrait_Value, pCurrentBetText, true);
                    SlotMachineMenu::pMe->pTiles[0]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                    SlotMachineMenu::pMe->pTiles[0]->Set(SlotMachineMenu::iUT_X, g_slotMachineMinBet);
                    const char* pChipCountText = sChipCountText.String();
                    SlotMachineMenu::pMe->pTiles[1]->Set(g_slotMachineMenuTrait_Value, pChipCountText, true);
                    SlotMachineMenu::pMe->pTiles[1]->Poke(SlotMachineMenu::iUT_X, 1.0f);
                    pChipType = SlotMachineMenu::pCasino->GetChipType();
                    int32_t iPlayerChips = pPlayer->GetInventoryObjectCount(pChipType);
                    SlotMachineMenu::pMe->pTiles[1]->Set(SlotMachineMenu::iUT_X, iPlayerChips);
                    if (BSstrlen(SlotMachineMenu::pCasino->GetFullName())) {
                        const char* pEarningsText = sCasinoEarningsText.String();
                        BSsprintf(acEarningsName, 64, "%s %s", SlotMachineMenu::pCasino->GetFullName(), pEarningsText);
                    }
                    else {
                        BSsprintf(acEarningsName, 64, "%s Earnings: ", SlotMachineMenu::pCasino->GetFormEditorID());
                    }
                    SlotMachineMenu::pMe->pTiles[2]->Set(g_slotMachineMenuTrait_Value, acEarningsName, true);
                    Tile::GetTileByName(SlotMachineMenu::pMe->pTiles[2], "SMM_TotalEarningsValue")->Set(4003, 0);
                    SlotMachineMenu::pMe->UpdateEarningsString();
                    const char* pSpinText = sSpinText.String();
                    SlotMachineMenu::pMe->pTiles[3]->Set(4036, pSpinText, true);
                    const char* pIncreaseBetText = sIncreaseBetText.String();
                    SlotMachineMenu::pMe->pTiles[4]->Set(4036, pIncreaseBetText, true);
                    const char* pDecreaseBetText = sDecreaseBetText.String();
                    SlotMachineMenu::pMe->pTiles[5]->Set(4036, pDecreaseBetText, true);
                    const char* pPayoutListText = sPayoutListText.String();
                    SlotMachineMenu::pMe->pTiles[6]->Set(4036, pPayoutListText, true);
                    const char* pBetMaxText = gs_sBetMax.String();
                    SlotMachineMenu::pMe->pTiles[7]->Set(4036, pBetMaxText, true);
                    const char* pPressAnyButtonText = sSlotPressAnyButtonText.String();
                    SlotMachineMenu::pMe->pTiles[9]->Set(4036, pPressAnyButtonText, true);
                    if (SlotMachineMenu::pMe->pTiles[8]) {
                        const char* pExitText = sExit.String();
                        SlotMachineMenu::pMe->pTiles[8]->Set(4036, pExitText, true);
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

            if (SlotMachineMenu::pMe && SlotMachineMenu::pMe->pMenu && SlotMachineMenu::pMe) {
                delete SlotMachineMenu::pMe;
            }

            return nullptr;
        }

        char acMessage[264];
        BSsprintf(acMessage, 260, sGamblingBrokeText.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
            nullptr,
            2.0f,
            false
        );
        BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
        return nullptr;
    }

    char acMessage[264];
    BSsprintf(acMessage, 260, sGamblingBannedText.String());
    Interface::ShowMessage(
        acMessage,
        0,
        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
        nullptr,
        2.0f,
        false
    );
    BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
    return nullptr;
}
