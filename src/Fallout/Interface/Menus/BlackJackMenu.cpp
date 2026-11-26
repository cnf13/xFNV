#include "Fallout/Interface/Menus/BlackJackMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
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
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCasinoChips.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/RouletteMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <cstdint>

NiPointer<NiCamera> BlackJackMenu::spCamera;
NiPointer<NiNode> BlackJackMenu::spSceneRoot;

BlackJackMenu::BlackJackCard::BlackJackCard()
  : uiValue(BL_CV_EMPTY), eSuit(5), spFrontTexture(nullptr), spBackTexture(nullptr) {}

BlackJackMenu::BlackJackCard& BlackJackMenu::BlackJackCard::operator=(const BlackJackCard& aOther) {
    uiValue = aOther.uiValue;
    eSuit = aOther.eSuit;
    spFrontTexture = aOther.spFrontTexture;
    spBackTexture = aOther.spBackTexture;
    return *this;
}

BlackJackMenu::BlackJackMenu() : Menu(), eBlackJackState(BL_IDLE), uiPrevIdleTick(0) {
    g_blackJackMenuTrait_Value = Tile::AddUserTrait("_Value", -1);
    BlackJackMenu::iUT_X = Tile::AddUserTrait("_x", -1);
    if (BlackJackMenu::pCasino->GetChipType()) {
        iChipCount = pPlayer->GetInventoryObjectCount(BlackJackMenu::pCasino->GetChipType());
    }
    ZeroFlags();
}

BlackJackMenu::~BlackJackMenu() {
    SetLastAntiCheatTime();
    BlackJackMenu::spCamera = nullptr;
    pModelLoader->ReleaseFile(BlackJackMenu::pCasino->GetModel(8)->GetModel());
    pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Hand1.NIF");
    pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Hand2.NIF");
    pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Dealer.NIF");
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(BlackJackMenu::spSceneRoot.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    BlackJackMenu::spSceneRoot->DetachAllChildren();
    BlackJackMenu::spSceneRoot = nullptr;
    pTableGeometry = nullptr;
    pHandGeometry = nullptr;
    pHandSplitGeometry = nullptr;
    pHandDealerGeometry = nullptr;
    pTableAnimManager = nullptr;
    pPlayerAnimManager = nullptr;
    pPlayerSplitAnimManager = nullptr;
    BlackJackMenu::spCamera = nullptr;
    for (int32_t i = 0; i < 8; ++i) {
        pPlayerAnimations[i] = nullptr;
        pPlayerSplitAnimations[i] = nullptr;
        pDealerAnimations[i] = nullptr;
    }
    // Each chip model and its "_Shadow.NIF" companion.
    for (int32_t i = 0; i < 6; ++i) {
        char acShadowPath[264];
        char* pExtension = nullptr;
        BSstrcpy(acShadowPath, 260, BlackJackMenu::pCasino->GetModel(i)->GetModel());
        for (int32_t j = 0; j < BSstrlen(acShadowPath); ++j) {
            if (acShadowPath[j] == '.') {
                pExtension = &acShadowPath[j];
                break;
            }
        }
        if (pExtension) {
            BSstrcpy(pExtension, 15, "_Shadow.NIF");
        }
        pModelLoader->ReleaseFile(BlackJackMenu::pCasino->GetModel(i)->GetModel());
        pModelLoader->ReleaseFile(acShadowPath);
        pBetChipGeometry[i] = nullptr;
        pBetShadowGeometry[i] = nullptr;
    }
    for (int32_t i = 0; i < 6; ++i) {
        pBetChipGeometry[i] = nullptr;
        pBetShadowGeometry[i] = nullptr;
    }
    pDealerReveal = nullptr;
    pTableAnimations[0] = nullptr;
    pTableAnimations[1] = nullptr;
    pDealerHand = nullptr;
    pPlayerHand = nullptr;
    pPlayerSplitHand = nullptr;
    g_blackJackCasinoStats = nullptr;
    BlackJackMenu::pCasino = nullptr;
    BlackJackMenu::pMe = nullptr;
    // The hand pointers are cleared above before they are tested, so these deletes never run.
    if (pPlayerHand) {
        delete pPlayerHand;
    }
    if (pPlayerSplitHand) {
        delete pPlayerSplitHand;
    }
    if (pDealerHand) {
        delete pDealerHand;
    }
}

void BlackJackMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 9) {
        pTiles[aiID] = apTile;
    }
}

void BlackJackMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (eBlackJackState == BlackJackMenu::BL_IDLE || eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
        switch (aiID) {
            case 3: {
                if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
                    eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                    SetFlag(BlackJackMenu::BLF_DISABLESURRENDER, true);
                    SetTileAlphasForStage(7);
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                    SetFlag(BlackJackMenu::BLF_CANSPLIT, false);
                    if (!IsFlagSet(BLF_SPLITHANDACTIVE) || IsFlagSet(BLF_DISABLEHAND_SPLIT)) {
                        if (!IsFlagSet(BLF_SPLITHANDACTIVE) && !IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                            int32_t cardindex = DealCard(pPlayerHand, 1, 0);
                            BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[cardindex];
                            pPlayerHand->AddTail(pCard);
                            int32_t hValue = GetHandValue(pPlayerHand);
                            if (hValue >= 21) {
                                SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                                if (hValue > 21) {
                                    eHandResults[0] = 4;
                                }
                            }
                            SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                            BSSimpleList<BlackJackMenu::BlackJackCard*>* cl = pPlayerHand;
                            int32_t cardCount = 0;
                            while (cl && cl->GetItem()) {
                                ++cardCount;
                                cl = cl->GetNext();
                            }
                            if (cardCount >= 7) {
                                SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                                pTiles[9]->Set(4036, "You've reached the max hand size", true);
                            }
                        }
                    }
                    else {
                        int32_t cardindex = DealCard(pPlayerSplitHand, 1, 0);
                        BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[cardindex];
                        pPlayerSplitHand->AddTail(pCard);
                        int32_t hValue = GetHandValue(pPlayerSplitHand);
                        if (hValue >= 21) {
                            SetFlag(BlackJackMenu::BLF_DISABLEHAND_SPLIT, true);
                            if (hValue > 21) {
                                eHandResults[1] = 4;
                            }
                        }
                        SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                        BSSimpleList<BlackJackMenu::BlackJackCard*>* cl = pPlayerSplitHand;
                        int32_t cardCount = 0;
                        while (cl && cl->GetItem()) {
                            ++cardCount;
                            cl = cl->GetNext();
                        }
                        if (cardCount >= 7) {
                            SetFlag(BlackJackMenu::BLF_DISABLEHAND_SPLIT, true);
                            pTiles[9]->Set(4036, "You've reached the max hand size", true);
                        }
                    }
                    SetTileAlphasForStage(10);
                }
                break;
            }
            case 4: {
                if (eBlackJackState == BlackJackMenu::BL_IDLE) {
                    eBlackJackState = BlackJackMenu::BL_DEALING_ANIM;
                    ZeroFlags();
                    bBadLuck = false;
                    bGoodLuck = false;
                    iCurrentSplitBet = 0;
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, true);
                    BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(8, 128.0f);
                    DealHand();
                }
                else if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
                    if ((!IsFlagSet(BLF_SPLITHANDACTIVE) || !IsFlagSet(BLF_CANDOUBLEDOWN_SPLIT)) &&
                        IsFlagSet(BLF_CANDOUBLEDOWN_MAIN)) {
                        iCurrentBet *= 2;
                        SetFlag(BlackJackMenu::BLF_DOUBLEDOWN_MAIN, true);
                        SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                        int32_t cardindex = DealCard(pPlayerHand, 1, 0);
                        BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[cardindex];
                        pPlayerHand->AddTail(pCard);
                        if (!IsFlagSet(BLF_DISABLEHAND_MAIN) && IsFlagSet(BLF_CANDOUBLEDOWN_MAIN)) {
                            CanDoubleDown(false);
                        }
                        eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                        SetFlag(BlackJackMenu::BLF_DISABLESURRENDER, true);
                        BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                        BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet + iCurrentSplitBet);
                        int32_t hValue = GetHandValue(pPlayerHand);
                        if (hValue > 21) {
                            eHandResults[0] = 4;
                        }
                        SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                    }
                    SetTileAlphasForStage(10);
                }
                break;
            }
            case 5: {
                if (eBlackJackState == BlackJackMenu::BL_IDLE) {
                    int32_t previousBet = iCurrentBet;
                    if (iCurrentBet >= 10) {
                        if (iCurrentBet >= 40) {
                            if (iCurrentBet >= 100) {
                                if (iCurrentBet >= 400) {
                                    iCurrentBet = iCurrentBet >= 1500 ? iCurrentBet + 500 : iCurrentBet + 100;
                                }
                                else {
                                    iCurrentBet += 25;
                                }
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
                    if (iCurrentBet > g_blackJackMaxBet) {
                        iCurrentBet = g_blackJackMaxBet;
                    }
                    if (previousBet < iCurrentBet) {
                        BSAudio::QInstance()
                            .GetSoundHandleByName(
                                "GAMEBlackJackIncreaseBet",
                                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                    BSGameSound::SOUND_FLAG_ONE_SHOT
                            )
                            .Play(false);
                    }
                    SetCorrectChipMeshesVisibleForCurrentBet();
                    BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
                }
                else if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN && IsFlagSet(BLF_CANSPLIT)) {
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                    eBlackJackState = BlackJackMenu::BL_SPLIT_ANIM;
                    SetFlag(BlackJackMenu::BLF_DISABLESURRENDER, true);
                    BSSimpleList<BlackJackMenu::BlackJackCard*>* clist = pPlayerHand->GetNext();
                    BlackJackMenu::BlackJackCard* cd = clist->GetItem();
                    iCurrentSplitBet = iCurrentBet;
                    if (pPlayerSplitHand && !pPlayerSplitHand->IsEmpty()) {
                        pPlayerSplitHand->AddHead(cd);
                    }
                    else {
                        pPlayerSplitHand->SetItem(cd);
                    }
                    SwapCardTextures(1, 1, cd);
                    int32_t cardindex = DealCard(pPlayerHand, 0, 0);
                    BlackJackMenu::BlackJackCard* pHandCard = &pDeckOfCards[cardindex];
                    clist->SetItem(pHandCard);
                    cardindex = DealCard(pPlayerSplitHand, 0, 0);
                    BlackJackMenu::BlackJackCard* pSplitCard = &pDeckOfCards[cardindex];
                    pPlayerSplitHand->AddTail(pSplitCard);
                    int32_t hValue1 = GetHandValue(pPlayerHand);
                    int32_t hValue2 = GetHandValue(pPlayerHand);
                    if (hValue1 == 21) {
                        eHandResults[0] = 5;
                        SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                    }
                    CanDoubleDown(false);
                    if (hValue2 == 21) {
                        eHandResults[1] = 5;
                        SetFlag(BlackJackMenu::BLF_DISABLEHAND_SPLIT, true);
                    }
                    CanDoubleDown(true);
                    SetFlag(BlackJackMenu::BLF_SPLIT, true);
                    SetFlag(BlackJackMenu::BLF_CANSPLIT, false);
                    HighlightSelectedDeck();
                    SetTileAlphasForStage(10);
                    BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet + iCurrentSplitBet);
                }
                break;
            }
            case 6: {
                if (eBlackJackState == BlackJackMenu::BL_IDLE) {
                    int32_t iPreviousBet = iCurrentBet;
                    if (iCurrentBet > 10) {
                        if (iCurrentBet > 40) {
                            if (iCurrentBet > 100) {
                                if (iCurrentBet > 400) {
                                    if (iCurrentBet > 1500) {
                                        iCurrentBet = iCurrentBet % 500 ? 500 * (iCurrentBet / 500) : iCurrentBet - 500;
                                    }
                                    else {
                                        iCurrentBet = iCurrentBet % 100 ? 100 * (iCurrentBet / 100) : iCurrentBet - 100;
                                    }
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
                    if (iCurrentBet < g_blackJackMinChipsToPlay) {
                        iCurrentBet = g_blackJackMinChipsToPlay;
                    }
                    if (iPreviousBet > iCurrentBet) {
                        BSAudio::QInstance()
                            .GetSoundHandleByName(
                                "GAMEBlackJackDecreaseBet",
                                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                    BSGameSound::SOUND_FLAG_ONE_SHOT
                            )
                            .Play(false);
                    }
                    SetCorrectChipMeshesVisibleForCurrentBet();
                    BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
                }
                else if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN && IsFlagSet(BLF_SPLIT)) {
                    if (!IsFlagSet(BLF_SPLITHANDACTIVE) || IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                        if (!IsFlagSet(BLF_SPLITHANDACTIVE) && !IsFlagSet(BLF_DISABLEHAND_SPLIT)) {
                            SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, true);
                        }
                    }
                    else {
                        SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, false);
                    }
                    HighlightSelectedDeck();
                    pTiles[9]->Set(4036, "", true);
                    SetTileAlphasForStage(10);
                }
                break;
            }
            case 7: {
                if (eBlackJackState == BlackJackMenu::BL_IDLE) {
                    iCurrentBet = g_blackJackMaxBet;
                    if (iCurrentBet > iChipCount) {
                        iCurrentBet = iChipCount;
                    }
                    SetCorrectChipMeshesVisibleForCurrentBet();
                    BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
                }
                if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN && !IsFlagSet(BLF_DISABLESURRENDER)) {
                    SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                    eHandResults[0] = 6;
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                    SetTileAlphasForStage(10);
                }
                break;
            }
            case 8: {
                if (eBlackJackState == BlackJackMenu::BL_IDLE) {
                    BlackJackMenu::SetShouldExit();
                }
                else if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                    SetTileAlphasForStage(10);
                }
                break;
            }
            default: {
                return;
            }
        }
    }
}

void BlackJackMenu::DoIdle() {
    static int32_t eprevBlackJackState;
    static float fStateSecs;
    Interface::enumMenus eMenuID = static_cast<Interface::enumMenus>(GetClass());
    bool bisMenuOnTop = Interface::IsTopMenuID(eMenuID);
    if (uiPrevIdleTick) {
        if (bisMenuOnTop) {
            int32_t ielaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
            if (ielaspedMillis > 1000) {
                ielaspedMillis = 0;
            }
            if (eprevBlackJackState == eBlackJackState) {
                fStateSecs = static_cast<double>(ielaspedMillis) / 1000.0 + fStateSecs;
            }
            else {
                fStateSecs = 0.0f;
            }
        }
    }
    else {
        BSSimpleList<BlackJackMenu::BlackJackCard*>* pHand = new BSSimpleList<BlackJackMenu::BlackJackCard*>;
        pPlayerHand = pHand;
        pHand = new BSSimpleList<BlackJackMenu::BlackJackCard*>;
        pPlayerSplitHand = pHand;
        pHand = new BSSimpleList<BlackJackMenu::BlackJackCard*>;
        pDealerHand = pHand;
        fStateSecs = 0.0f;
        eprevBlackJackState = 0;
    }
    uiPrevIdleTick = appTimer.iLastTime;
    eprevBlackJackState = eBlackJackState;
    if (!bisMenuOnTop) {
        InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
    }
    switch (eBlackJackState) {
        case BlackJackMenu::BL_DEALING_ANIM: {
            int32_t iAnim = iAnimIndex[2];
            float animOffset = pPlayerAnimations[iAnim]->m_fEndKeyTime / 2.0;
            static float animDelay = pPlayerAnimations[1]->m_fEndKeyTime;
            if (pDealerAnimations[iAnim] && pPlayerAnimations[iAnim]) {
                if (fStateSecs > 1.0) {
                    pTiles[9]->Set(4003, 0);
                }
                NiUpdateData UpdateData1(fStateSecs, true, false);
                BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1);
                if (fStateSecs == 0.0) {
                    NiUpdateData UpdateData0(0.0f, true, false);
                    BlackJackMenu::spSceneRoot->Update(UpdateData0);
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                    bCleanDealerHand = 1;
                    BSSimpleList<BlackJackMenu::BlackJackCard*>* pPl = pPlayerHand;
                    BSSimpleList<BlackJackMenu::BlackJackCard*>* pD = pDealerHand;
                    SwapCardTextures(0, 1, pPl->m_item);
                    pPl = pPl->m_pkNext;
                    SwapCardTextures(0, 2, pPl->m_item);
                    SwapCardTextures(2, 1, pD->m_item);
                    pD = pD->m_pkNext;
                    SwapCardTextures(2, 2, pD->m_item);
                    pPlayerAnimManager->ActivateSequence(pPlayerAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    pDealerAnimManager->ActivateSequence(pDealerAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    animDelay = pPlayerAnimations[1]->m_fEndKeyTime;
                }
                if (iAnimIndex[0] == 1) {
                    NiUpdateData UpdateData2(fStateSecs, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(1u)->Update(UpdateData2);
                }
                if (iAnimIndex[2] == 1 && animOffset <= static_cast<double>(fStateSecs)) {
                    if (bCleanDealerHand && static_cast<double>(fStateSecs) < pPlayerAnimations[1]->m_fEndKeyTime) {
                        bCleanDealerHand = 0;
                        BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                    }

                    float fTime = fStateSecs - animOffset;
                    NiUpdateData UpdateData3(fTime, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(3u)->Update(UpdateData3);
                }
                if (iAnimIndex[0] == 2 && static_cast<double>(fStateSecs) >= pPlayerAnimations[1]->m_fEndKeyTime) {
                    if (!bCleanDealerHand) {
                        double dEndKeyTime = pPlayerAnimations[1]->m_fEndKeyTime;
                        if (fStateSecs < dEndKeyTime + animOffset) {
                            bCleanDealerHand = 1;
                            BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                        }
                    }
                    float fTime = fStateSecs - pPlayerAnimations[iAnim]->m_fEndKeyTime;
                    NiUpdateData UpdateData2_2(fTime, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(1u)->Update(UpdateData2_2);
                }
                if (iAnimIndex[2] == 2) {
                    double dEndKeyTime = pPlayerAnimations[1]->m_fEndKeyTime;
                    if (fStateSecs >= dEndKeyTime + animOffset) {
                        if (bCleanDealerHand) {
                            bCleanDealerHand = 0;
                            BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardFlip", 289u).Play(false);
                        }
                        dEndKeyTime = pPlayerAnimations[1]->m_fEndKeyTime;
                        float fTime = fStateSecs - (dEndKeyTime + animOffset);
                        NiUpdateData UpdateData3_2(fTime, true, false);
                        BlackJackMenu::spSceneRoot->GetAt(3u)->Update(UpdateData3_2);
                    }
                }
                if (animDelay <= static_cast<double>(fStateSecs)) {
                    if (iAnimIndex[0] == 1) {
                        ++iAnimIndex[0];
                        pPlayerAnimManager->DeactivateSequence(pPlayerAnimations[iAnim], 0.0f);
                        pPlayerAnimManager
                            ->ActivateSequence(pPlayerAnimations[iAnimIndex[0]], 0, false, 1.0f, 0.0f, nullptr);
                        double dEndKeyTime = pDealerAnimations[1]->m_fEndKeyTime;
                        animDelay = dEndKeyTime + animOffset;
                    }
                    else if (iAnimIndex[2] == 1) {
                        ++iAnimIndex[2];
                        pDealerAnimManager->DeactivateSequence(pDealerAnimations[iAnim], 0.0f);
                        pDealerAnimManager
                            ->ActivateSequence(pDealerAnimations[iAnimIndex[2]], 0, false, 1.0f, 0.0f, nullptr);
                        double dEndKeyTime = pPlayerAnimations[1]->m_fEndKeyTime;
                        animDelay = pDealerAnimations[1]->m_fEndKeyTime + dEndKeyTime;
                    }
                    else if (iAnimIndex[0] == 2) {
                        ++iAnimIndex[0];
                        pPlayerAnimManager->DeactivateSequence(pPlayerAnimations[iAnim], 0.0f);
                        double dEndKeyTime = pDealerAnimations[1]->m_fEndKeyTime;
                        double dDelay = dEndKeyTime + animOffset;
                        animDelay = pDealerAnimations[2]->m_fEndKeyTime + dDelay;
                    }
                    else {
                        int32_t hValue = GetHandValue(pPlayerHand);
                        if (hValue == 21) {
                            eHandResults[0] = 5;
                            SetFlag(BlackJackMenu::BLF_BLACKJACK, true);
                        }
                        if (GetHandValue(pDealerHand) == 21) {
                            SetFlag(BlackJackMenu::BLF_BLACKJACK, true);
                            if (eHandResults[0] == 5) {
                                eHandResults[0] = 2;
                            }
                            else {
                                eHandResults[0] = 1;
                            }
                        }
                        if (!IsFlagSet(BLF_BLACKJACK)) {
                            BlackJackMenu::SetDealNotSwitchHands(0);
                        }
                        if (iChipCount < 2 * iCurrentBet) {
                            SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                            SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                            SetFlag(BlackJackMenu::BLF_CANSPLIT, false);
                        }
                        else if (hValue == 11) {
                            SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, true);
                        }
                        else {
                            CanSplitHand();
                        }
                        SetTileAlphasForStage(10);
                        animDelay = 0.0f;
                        ++iAnimIndex[2];
                        pDealerAnimManager->DeactivateSequence(pDealerAnimations[iAnim], 0.0f);
                        if (IsFlagSet(BLF_BLACKJACK)) {
                            eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                        }
                        else {
                            eBlackJackState = BlackJackMenu::BL_PLAYER_TURN;
                        }
                    }
                }
            }

            return;
        }
        case BlackJackMenu::BL_HIT_ANIM: {
            bool useSplit = IsFlagSet(BLF_SPLITHANDACTIVE);
            if (useSplit) {
                if (!pPlayerSplitAnimations[iAnimIndex[1]]) {
                    return;
                }
            }
            else if (!pPlayerAnimations[iAnimIndex[0]]) {
                return;
            }
            if (fStateSecs > 1.0) {
                pTiles[9]->Set(4003, 0);
            }
            NiUpdateData UpdateData1_2(fStateSecs, true, false);
            BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1_2);
            if (fStateSecs == 0.0) {
                BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                BSSimpleList<BlackJackMenu::BlackJackCard*>* pPl = nullptr;
                BlackJackMenu::BlackJackCard* cd = nullptr;
                int32_t counter = 0;
                int32_t ihand = 0;
                if (useSplit) {
                    ihand = 1;
                    pPlayerSplitAnimManager
                        ->ActivateSequence(pPlayerSplitAnimations[iAnimIndex[1]], 0, false, 1.0f, 0.0f, nullptr);
                    pPl = pPlayerSplitHand;
                    while (pPl && pPl->m_item && counter != iAnimIndex[1]) {
                        cd = pPl->m_item;
                        pPl = pPl->m_pkNext;
                        ++counter;
                    }
                }
                else {
                    pPlayerAnimManager
                        ->ActivateSequence(pPlayerAnimations[iAnimIndex[0]], 0, false, 1.0f, 0.0f, nullptr);
                    pPl = pPlayerHand;
                    while (pPl && pPl->m_item && counter != iAnimIndex[0]) {
                        cd = pPl->m_item;
                        pPl = pPl->m_pkNext;
                        ++counter;
                    }
                }
                // Walks on to the hand's last card once any card was counted.
                while (pPl && pPl->m_item && counter) {
                    cd = pPl->m_item;
                    pPl = pPl->m_pkNext;
                    ++counter;
                }
                SwapCardTextures(ihand, counter, cd);
            }
            NiUpdateData UpdateData2_3(fStateSecs, true, false);
            NiAVObject* pHandNode;
            if (useSplit) {
                pHandNode = BlackJackMenu::spSceneRoot->GetAt(2u);
            }
            else {
                pHandNode = BlackJackMenu::spSceneRoot->GetAt(1u);
            }
            pHandNode->Update(UpdateData2_3);
            if ((useSplit && static_cast<double>(fStateSecs) >= pPlayerSplitAnimations[iAnimIndex[1]]->m_fEndKeyTime) ||
                (!useSplit && static_cast<double>(fStateSecs) >= pPlayerAnimations[iAnimIndex[0]]->m_fEndKeyTime)) {
                if (useSplit) {
                    pPlayerSplitAnimManager->DeactivateSequence(pPlayerSplitAnimations[iAnimIndex[1]], 0.0f);
                    ++iAnimIndex[1];
                }
                else {
                    pPlayerAnimManager->DeactivateSequence(pPlayerAnimations[iAnimIndex[0]], 0.0f);
                    ++iAnimIndex[0];
                }
                if (IsFlagSet(BlackJackMenu::BLF_PLAYER_SPLIT_ANIM)) {
                    eBlackJackState = BlackJackMenu::BL_SPLIT_ANIM;
                }
                else if (IsFlagSet(BlackJackMenu::BLF_DEALERS_TURN)) {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                }
                else if (
                    IsFlagSet(BLF_SPLIT) && (IsFlagSet(BLF_DISABLEHAND_MAIN) && IsFlagSet(BLF_DISABLEHAND_SPLIT))
                ) {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                }
                else if (IsFlagSet(BLF_SPLIT) || !IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                    eBlackJackState = BlackJackMenu::BL_PLAYER_TURN;
                }
                else {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                }
            }
            return;
        }
        case BlackJackMenu::BL_SPLIT_ANIM: {
            if (pPlayerSplitAnimations[1] && pPlayerSplitAnimations[2] && pPlayerAnimations[2]) {
                if (iAnimIndex[1] == 1) {
                    SetFlag(BlackJackMenu::BLF_PLAYER_SPLIT_ANIM, true);
                    SetSideHandVisibility(true);
                    pPlayerAnimManager->ActivateSequence(pPlayerAnimations[2], 0, false, 1.0f, 0.0f, nullptr);
                    NiUpdateData update0(0.0f, true, false);
                    NiUpdateData update1(0.0f, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(3u)->Update(update1);
                    BlackJackMenu::spSceneRoot->GetAt(1u)->Update(update0);
                    SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, true);
                    --iAnimIndex[0];
                    eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                }
                else if (iAnimIndex[0] == 2) {
                    SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, false);
                    eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                }
                else if (iAnimIndex[1] == 2) {
                    SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, true);
                    eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                }
                else {
                    SetFlag(BlackJackMenu::BLF_PLAYER_SPLIT_ANIM, false);
                    SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, false);
                    if (eHandResults[0] == 5 && eHandResults[1] == 5) {
                        eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
                    }
                    else {
                        eBlackJackState = BlackJackMenu::BL_PLAYER_TURN;
                    }
                }
            }

            return;
        }
        case BlackJackMenu::BL_PLAYER_TURN: {
            if (eHandResults[0]) {
                if (IsFlagSet(BLF_SPLIT)) {
                    if (eHandResults[1]) {
                        eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                    }
                }
                else {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                }
            }
            if (IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                if (IsFlagSet(BLF_SPLIT)) {
                    if (IsFlagSet(BLF_DISABLEHAND_SPLIT)) {
                        eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                    }
                }
                else {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                }
            }
            return;
        }
        case BlackJackMenu::BL_CLEAN_TABLE_ANIM: {
            if (pPlayerAnimations[0] && (pPlayerSplitAnimations[0] && pDealerAnimations[0])) {
                if (fStateSecs > 1.0) {
                    pTiles[9]->Set(4003, 0);
                }
                if (fStateSecs == 0.0 && !bCleanDealerHand) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardCollect", 289u).Play(false);
                    NiUpdateData UpdateData0_2(0.0f, true, false);
                    BlackJackMenu::spSceneRoot->Update(UpdateData0_2);
                    pTableAnimManager->ActivateSequence(pTableAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    pPlayerAnimManager->ActivateSequence(pPlayerAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    pPlayerSplitAnimManager->ActivateSequence(pPlayerSplitAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    UpdateHandCardVisibility();
                }
                if (bCleanDealerHand || !(static_cast<double>(fStateSecs) < pPlayerAnimations[0]->m_fEndKeyTime)) {
                    if (bCleanDealerHand) {
                        double dElapsed = fStateSecs - fStartKeyTime;
                        if (pDealerAnimations[0]->m_fEndKeyTime > dElapsed) {
                            float fTime = fStateSecs - fStartKeyTime;
                            NiUpdateData UpdateData1_4(fTime, true, false);
                            fTime = fStateSecs - fStartKeyTime;
                            NiUpdateData UpdateData2_5(fTime, true, false);
                            BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1_4);
                            BlackJackMenu::spSceneRoot->GetAt(3u)->Update(UpdateData2_5);
                        }
                    }
                }
                else {
                    NiUpdateData UpdateData1_3(fStateSecs, true, false);
                    NiUpdateData UpdateData2_4(fStateSecs, true, false);
                    NiUpdateData UpdateData3_3(fStateSecs, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1_3);
                    BlackJackMenu::spSceneRoot->GetAt(1u)->Update(UpdateData2_4);
                    BlackJackMenu::spSceneRoot->GetAt(2u)->Update(UpdateData3_3);
                }
                if (!(static_cast<double>(fStateSecs) >= pPlayerAnimations[0]->m_fEndKeyTime) || bCleanDealerHand) {
                    double dElapsed = fStateSecs - fStartKeyTime;
                    if (pDealerAnimations[0]->m_fEndKeyTime <= dElapsed && bCleanDealerHand) {
                        pTableAnimManager->DeactivateSequence(pTableAnimations[1], 0.0f);
                        pDealerAnimManager->DeactivateSequence(pDealerAnimations[0], 0.0f);
                        pTableAnimManager->DeactivateAll(0.0f);
                        pDealerAnimManager->DeactivateAll(0.0f);
                        pPlayerAnimManager->DeactivateAll(0.0f);
                        pPlayerSplitAnimManager->DeactivateAll(0.0f);
                        ShowAllHandCards();
                        ZeroFlags();
                        SetSideHandVisibility(false);
                        bCleanDealerHand = 0;
                        BlackJackMenu::SetDealNotSwitchHands(1);
                        int32_t iMax = BlackJackMenu::pCasino->data.iMaxWinnings;
                        int32_t iCurrentWinnings = g_blackJackCasinoStats->iTotalWinnings;
                        int32_t iWinningLevel = g_blackJackCasinoStats->iWinningLevel;
                        for (int32_t i = 1; i < 5; ++i) {
                            if (i > iWinningLevel) {
                                if (static_cast<double>(iCurrentWinnings) >=
                                    static_cast<double>(iMax) * (static_cast<double>(i) * 0.25)) {
                                    g_blackJackCasinoStats->iWinningLevel = i;
                                    bRunScriptOnExit = true;
                                }
                            }
                            else {
                                bRunScriptOnExit = false;
                            }
                        }
                        if (bRunScriptOnExit) {
                            BlackJackMenu::SetShouldExit();
                        }
                        else {
                            eBlackJackState = BlackJackMenu::BL_IDLE;
                        }
                        for (int32_t& iAnim : iAnimIndex) {
                            iAnim = 1;
                        }
                    }
                }
                else {
                    pTableAnimManager->DeactivateSequence(pTableAnimations[0], 0.0f);
                    pTableAnimManager->ActivateSequence(pTableAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    pDealerAnimManager->ActivateSequence(pDealerAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardCollectDealer", 289u).Play(false);
                    bCleanDealerHand = 1;
                    fStartKeyTime = fStateSecs;
                    NiUpdateData UpdateData1_5(0.0f, true, false);
                    NiUpdateData UpdateData2_6(0.0f, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1_5);
                    BlackJackMenu::spSceneRoot->GetAt(3u)->Update(UpdateData2_6);
                }
            }
            else {
                eBlackJackState = BlackJackMenu::BL_IDLE;
            }
            return;
        }
        case BlackJackMenu::BL_DEALER_FLIP_ANIM: {
            if (fStateSecs > 1.0) {
                pTiles[9]->Set(4003, 0);
            }
            if (pDealerReveal) {
                NiUpdateData UpdateData1_6(fStateSecs, true, false);
                BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1_6);
                if (fStateSecs == 0.0) {
                    SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                    SetFlag(BlackJackMenu::BLF_DISABLEHAND_SPLIT, true);
                    ClearDeckHighlight();
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardFlip", 289u).Play(false);
                    pDealerAnimManager->ActivateSequence(pDealerReveal, 0, false, 1.0f, 0.0f, nullptr);
                }
                if (static_cast<double>(fStateSecs) < pDealerReveal->m_fEndKeyTime) {
                    NiUpdateData UpdateData2_7(fStateSecs, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(3u)->Update(UpdateData2_7);
                }
                if (static_cast<double>(fStateSecs) >= pDealerReveal->m_fEndKeyTime) {
                    pDealerAnimManager->DeactivateSequence(pDealerReveal, 0.0f);
                    eBlackJackState = BlackJackMenu::BL_DEALER_TURN;
                }
            }
            else {
                eBlackJackState = BlackJackMenu::BL_DEALER_TURN;
            }
            return;
        }
        case BlackJackMenu::BL_DEALER_HIT_ANIM: {
            if (fStateSecs > 1.0) {
                pTiles[9]->Set(4003, 0);
            }
            if (pDealerAnimations[iAnimIndex[2]]) {
                NiUpdateData UpdateData1_7(fStateSecs, true, false);
                BlackJackMenu::spSceneRoot->GetAt(0)->Update(UpdateData1_7);
                if (fStateSecs == 0.0) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardFlip", 289u).Play(false);
                    ClearDeckHighlight();
                    BSSimpleList<BlackJackMenu::BlackJackCard*>* pPl = pDealerHand;
                    BlackJackMenu::BlackJackCard* cd = nullptr;
                    int32_t counter = 0;
                    while (pPl && pPl->m_item) {
                        cd = pPl->m_item;
                        pPl = pPl->m_pkNext;
                        ++counter;
                    }
                    if (cd) {
                        SwapCardTextures(2, counter, cd);
                    }
                    pDealerAnimManager
                        ->ActivateSequence(pDealerAnimations[iAnimIndex[2]], 0, false, 1.0f, 0.0f, nullptr);
                }
                if (static_cast<double>(fStateSecs) < pDealerAnimations[iAnimIndex[2]]->m_fEndKeyTime) {
                    NiUpdateData UpdateData2_8(fStateSecs, true, false);
                    BlackJackMenu::spSceneRoot->GetAt(3u)->Update(UpdateData2_8);
                }
                if (static_cast<double>(fStateSecs) >= pDealerAnimations[iAnimIndex[2]]->m_fEndKeyTime) {
                    pDealerAnimManager->DeactivateSequence(pDealerAnimations[iAnimIndex[2]], 0.0f);
                    ++iAnimIndex[2];
                    if (GetHandValue(pDealerHand) > 21 || IsFlagSet(BlackJackMenu::BLF_PLAYER_BUST)) {
                        eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
                    }
                    else {
                        eBlackJackState = BlackJackMenu::BL_DEALER_TURN;
                    }
                }
            }
            else if (GetHandValue(pDealerHand) > 21 || IsFlagSet(BlackJackMenu::BLF_PLAYER_BUST)) {
                eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
            }
            else {
                eBlackJackState = BlackJackMenu::BL_DEALER_TURN;
            }
            return;
        }
        case BlackJackMenu::BL_IDLE: {
            if (static_cast<double>(52 * iNumOfDecks) * fBlackJackShuffle > static_cast<double>(iCardsRemaining)) {
                Shuffle();
            }
            if (fStateSecs > 1.0) {
                pTiles[9]->Set(4003, 0);
            }
            if (iChipCount < iCurrentBet) {
                iCurrentBet = iChipCount;
                BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
            }
            if (iChipCount < g_blackJackMinChipsToPlay) {
                char msg[268];
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
                BlackJackMenu::SetShouldExit();
            }
            return;
        }
        case BlackJackMenu::BL_DEALER_TURN: {
            SetFlag(BlackJackMenu::BLF_DEALERS_TURN, true);
            if (IsFlagSet(BLF_SPLIT) ? eHandResults[0] == 4 && eHandResults[1] == 4
                                     : eHandResults[0] == 4 || eHandResults[0] == 6) {
                SetFlag(BlackJackMenu::BLF_PLAYER_BUST, true);
                eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
                return;
            }
            if (IsFlagSet(BLF_BLACKJACK)) {
                eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
            }
            else {
                int32_t pValue = GetHandValue(pDealerHand);
                int32_t cardindex = 0;
                int32_t cardCount = 0;
                BSSimpleList<BlackJackMenu::BlackJackCard*>* cl = pDealerHand;
                bool bAce = false;
                while (cl && cl->m_item) {
                    ++cardCount;
                    if (cl->m_item->uiValue == BlackJackMenu::BL_A) {
                        bAce = true;
                    }
                    cl = cl->m_pkNext;
                }
                if ((pValue < 17 || (pValue == 17 && (bAce && !bHoldOnSoft17))) && cardCount < 7) {
                    cardindex = DealCard(pDealerHand, 1, 1);
                    BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[cardindex];
                    pDealerHand->AddTail(pCard);
                    eBlackJackState = BlackJackMenu::BL_DEALER_HIT_ANIM;
                }
                else {
                    eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
                }
            }
            return;
        }
        case BlackJackMenu::BL_DETERMINE_WINNER: {
            char acMessage[264];
            MiscStatManager::Increment(MiscStatManager::MS_BLACKJACK_GAMES_PLAYED);
            int32_t pScore = GetHandValue(pPlayerHand);
            int32_t dScore = GetHandValue(pDealerHand);
            int32_t winnings = 0;
            if (!eHandResults[0]) {
                if ((pScore <= dScore || pScore >= 22) && (pScore >= 22 || dScore <= 21)) {
                    if ((pScore >= dScore || dScore >= 22) && pScore <= 21) {
                        eHandResults[0] = 2;
                    }
                    else {
                        eHandResults[0] = 1;
                    }
                }
                else {
                    eHandResults[0] = 3;
                }
            }
            if (IsFlagSet(BLF_SPLIT)) {
                pScore = GetHandValue(pPlayerSplitHand);
                if (!eHandResults[1]) {
                    if ((pScore <= dScore || pScore >= 22) && (pScore >= 22 || dScore <= 21)) {
                        if ((pScore >= dScore || dScore >= 22) && pScore <= 21) {
                            eHandResults[1] = 2;
                        }
                        else {
                            eHandResults[1] = 1;
                        }
                    }
                    else {
                        eHandResults[1] = 3;
                    }
                }
            }
            switch (eHandResults[0]) {
                case 1u:
                case 4u: {
                    winnings -= iCurrentBet;
                    break;
                }
                case 3u: {
                    winnings += iCurrentBet;
                    break;
                }
                case 5u: {
                    winnings += static_cast<int32_t>(static_cast<double>(iCurrentBet) * fBlackJackWinRatio + 0.51);
                    break;
                }
                case 6u: {
                    winnings -= static_cast<int32_t>(static_cast<double>(iCurrentBet) / 2.0 + 0.51);
                    break;
                }
                default: {
                    break;
                }
            }
            switch (eHandResults[1]) {
                case 1u:
                case 4u: {
                    winnings -= iCurrentSplitBet;
                    break;
                }
                case 3u: {
                    winnings += iCurrentSplitBet;
                    break;
                }
                case 5u: {
                    winnings += static_cast<int32_t>(static_cast<double>(iCurrentSplitBet) * fBlackJackWinRatio + 0.5);
                    break;
                }
                default: {
                    break;
                }
            }
            iChipCount += winnings;
            BlackJackMenu::pMe->pTiles[1]->Poke(BlackJackMenu::iUT_X, 1.0f);
            BlackJackMenu::pMe->pTiles[1]->Set(BlackJackMenu::iUT_X, iChipCount);
            g_blackJackCasinoStats->iTotalWinnings += winnings;
            SetTileCasinoEarnings();
            if (winnings <= 0) {
                if (winnings >= 0) {
                    BSsprintf(acMessage, 260, "%s", sYouBreakEvenText.String());
                    BlackJackMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                    BlackJackMenu::pMe->pTiles[9]->Set(4036, acMessage, true);
                    BlackJackMenu::pMe->pTiles[9]->Set(4003, 1);
                }
                else {
                    if (bBadLuck) {
                        const char* pPlural = sPlural.String();
                        const char* pChipText = sCasinoChipText.String();
                        BSsprintf(
                            acMessage,
                            260,
                            "%s %d %s%s",
                            sUnluckyLoseText.String(),
                            -winnings,
                            pChipText,
                            pPlural
                        );
                    }
                    else {
                        const char* pPlural = sPlural.String();
                        const char* pChipText = sCasinoChipText.String();
                        BSsprintf(acMessage, 260, "%s %d %s%s", sYouLoseText.String(), -winnings, pChipText, pPlural);
                    }
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackLose", 289u).Play(false);
                    BlackJackMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                    BlackJackMenu::pMe->pTiles[9]->Set(4036, acMessage, true);
                    BlackJackMenu::pMe->pTiles[9]->Set(4003, 1);
                }
            }
            else {
                if (bGoodLuck) {
                    const char* pPlural = sPlural.String();
                    const char* pChipText = sCasinoChipText.String();
                    BSsprintf(acMessage, 260, "%s %d %s%s", sLuckyWinText.String(), winnings, pChipText, pPlural);
                }
                else {
                    const char* pPlural = sPlural.String();
                    const char* pChipText = sCasinoChipText.String();
                    BSsprintf(acMessage, 260, "%s %d %s%s", sYouWinText.String(), winnings, pChipText, pPlural);
                }
                BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackWin", 289u).Play(false);
                BlackJackMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                BlackJackMenu::pMe->pTiles[9]->Set(4036, acMessage, true);
                BlackJackMenu::pMe->pTiles[9]->Set(4003, 1);
            }
            pPlayerHand->RemoveAll();
            pDealerHand->RemoveAll();
            if (IsFlagSet(BLF_SPLIT)) {
                pPlayerSplitHand->RemoveAll();
                BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
                iCurrentSplitBet = 0;
            }
            if (IsFlagSet(BLF_DOUBLEDOWN_MAIN)) {
                iCurrentBet /= 2;
                BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
            }
            if (iChipCount < iCurrentBet) {
                iCurrentBet = iChipCount;
                BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
            }
            eBlackJackState = BlackJackMenu::BL_CLEAN_TABLE_ANIM;
            return;
        }
        case BlackJackMenu::BL_CLOSING_MENU: {
            if (!(fStateSecs < 1.0) && xFadeState != Interface::FADING_OUT) {
                InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
                pMenu->Set(6002, 1);
                StartFadeOut();
                TESCasinoChips* pChips = BlackJackMenu::pCasino->GetChipType();
                int32_t i = iChipCount - pPlayer->GetInventoryObjectCount(pChips);
                if (i >= 0) {
                    if (i > 0) {
                        TESContainer tempcont;
                        tempcont.AddObject(BlackJackMenu::pCasino->GetChipType(), i, nullptr);
                        if (g_blackJackCasinoStats->iTotalWinnings >= BlackJackMenu::pCasino->GetMaxWinnings()) {
                            tempcont.AddContainerToReference(pPlayer, true);
                            BSStringT<char> pstring;
                            const char* pBannedText = sGamblingBannedText.String();
                            const char* pAddText = sAddItemtoInventory.String();
                            const char* pPlural = sPlural.String();
                            const char* pChipName = TESFullName::GetFullName(BlackJackMenu::pCasino->GetChipType());
                            pstring.SPrintF("%i %s%s %s\n%s", i, pChipName, pPlural, pAddText, pBannedText);
                            pChips = BlackJackMenu::pCasino->GetChipType();
                            const char* pSoundName = pPlayer->GetPickUpSoundName(pChips, true, false);
                            Interface::ShowMessage(
                                pstring.pString,
                                0,
                                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
                                pSoundName,
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
                    pChips = BlackJackMenu::pCasino->GetChipType();
                    pPlayer->RemoveItem(pChips, nullptr, -i, false, false, nullptr, nullptr, nullptr, true, false);
                    BSStringT<char> pstring;
                    const char* pRemoveText = sRemoveItemfromInventory.String();
                    const char* pPlural = sPlural.String();
                    const char* pChipName = TESFullName::GetFullName(BlackJackMenu::pCasino->GetChipType());
                    pstring.SPrintF("%i %s%s %s", -i, pChipName, pPlural, pRemoveText);
                    pChips = BlackJackMenu::pCasino->GetChipType();
                    const char* pSoundName = pPlayer->GetPickUpSoundName(pChips, true, false);
                    Interface::ShowMessage(
                        pstring.pString,
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                        pSoundName,
                        2.0f,
                        false
                    );
                }
                if (bRunScriptOnExit) {
                    TESQuest* pScript = BlackJackMenu::pCasino->GetQuest();
                    if (pScript) {
                        pScript->SetEnabled(1);
                    }
                }
            }
            return;
        }
        default: {
            return;
        }
    }
}

int32_t BlackJackMenu::GetClass() {
    return 1081;
}

bool BlackJackMenu::IsFlagSet(int32_t aiFlag) {
    return (aiFlag & iFlags) != 0;
}

// The compares are signed.
int32_t BlackJackMenu::GetNumericCardValue(CardValue cv) {
    if (cv >= BL_J) {
        if (cv <= BL_K) {
            return 10;
        }
        if (cv == BL_A) {
            return 11;
        }
    }
    return cv + 2;
}

// Copies the casino's blackjack settings into the menu, loads the table's and the six chips' models (with a shadow
// model for each chip) and builds the front and back textures of every card of the shoe.
bool BlackJackMenu::InitCasinoData() {
    if (!pCasino) {
        return false;
    }
    fBlackJackWinRatio = pCasino->GetData()->fBJ_BlackjackPayout;
    fBlackJackShuffle = pCasino->GetData()->fBJ_ShufflePercent;
    iNumOfDecks = static_cast<uint32_t>(pCasino->GetData()->iBJNumDecks);
    bHoldOnSoft17 = pCasino->GetData()->bBJ_DealerHoleCard;
    TESModelTextureSwap* pTableModel = pCasino->GetModel(8);
    pTableGeometry = pModelLoader->LoadFile(pTableModel->GetModel(), LOD_MULT_NONE, true, false, false, false);
    char acModelPath[0x104];
    for (int32_t i = 0; i < 6; ++i) {
        TESModelTextureSwap* pChipModel = pCasino->GetModel(i);
        pBetChipGeometry[i] = pModelLoader->LoadFile(pChipModel->GetModel(), LOD_MULT_NONE, true, false, false, false);
        char* pDot = nullptr;
        TESModelTextureSwap* pShadowModel = pCasino->GetModel(i);
        BSstrcpy(acModelPath, 0x104, pShadowModel->GetModel());
        for (uint32_t j = 0; j < BSstrlen(acModelPath); ++j) {
            if (acModelPath[j] == '.') {
                pDot = &acModelPath[j];
                break;
            }
        }
        if (pDot) {
            BSstrcpy(pDot, 0xf, "_Shadow.NIF");
            pBetShadowGeometry[i] = pModelLoader->LoadFile(acModelPath, LOD_MULT_NONE, true, false, false, false);
        }
    }
    if (!iNumOfDecks) {
        iNumOfDecks = 1;
    }
    // Each float is compared against the double 0.0, so a NaN keeps its value.
    if (fBlackJackWinRatio == 0.0) {
        fBlackJackWinRatio = 1.5f;
    }
    if (fBlackJackShuffle == 0.0) {
        fBlackJackShuffle = 0.25f;
    }
    alignas(BlackJackCard) uint8_t aCardStorage[sizeof(BlackJackCard)];
    BlackJackCard* pCard = ::new (aCardStorage) BlackJackCard;
    char acSuits[4] = {'h', 'c', 'd', 's'};
    char acDir[0x20];
    char acName[0x20];
    char acStr[0x104];
    char acDest[0x104];
    char acPath[0x104];
    for (int32_t k = 0; k < iNumOfDecks; ++k) {
        BSstrcpy(acStr, 0x104, pCasino->GetTexture(k + 7)->GetTextureName());
        char* pSlash = std::strstr(acStr, "\\") + 1;
        pSlash = std::strstr(pSlash, "\\") + 1;
        int32_t n = 0;
        while (pSlash && *pSlash != '\\') {
            acDir[n++] = *pSlash++;
        }
        ++pSlash;
        acDir[n++] = '\\';
        acDir[n] = '\0';
        n = 0;
        while (pSlash && *pSlash != '_') {
            acName[n++] = *pSlash++;
        }
        acName[n] = '\0';
        BSsprintf(acStr, 0x104, "%s%s", "Textures\\Terminals\\NV_PlayingCards\\", acDir);
        char c1 = '0';
        char c2 = '1';
        n = 0;
        BSsprintf(acDest, 0x104, "%s%s_Back.dds", acStr, acName);
        alignas(NiPointer<NiSourceTexture>) uint8_t aBackStorage[sizeof(NiPointer<NiSourceTexture>)];
        NiPointer<NiSourceTexture>* pBack = ::new (aBackStorage) NiPointer<NiSourceTexture>(nullptr);
        SwapPlatformLanguageTexturePath(acDest, acPath, 0x104);
        TES::pInstance->CreateTextureImage(acPath, *pBack, false, false);
        for (int32_t m = 0; m < 52; ++m) {
            pCard->uiValue = static_cast<CardValue>((m + 12) % 13);
            pCard->eSuit = static_cast<uint32_t>(m / 13);
            BSsprintf(acDest, 0x104, "%s%s_%c%c%c.dds", acStr, acName, acSuits[n], c1, c2);
            SwapPlatformLanguageTexturePath(acDest, acPath, 0x104);
            TES::pInstance->CreateTextureImage(acPath, pCard->spFrontTexture, false, false);
            pCard->spBackTexture = *pBack;
            pDeckOfCards[52 * k + m] = *pCard;
            if (++c2 > '9') {
                c2 = '0';
                ++c1;
            }
            if (c1 == '1' && c2 == '4') {
                c1 = '0';
                c2 = '1';
                ++n;
            }
        }
        pBack->~NiPointer<NiSourceTexture>();
    }
    iCardsRemaining = 52 * iNumOfDecks;
    for (int32_t& iAnim : iAnimIndex) {
        iAnim = 1;
    }
    bRunScriptOnExit = false;
    pCard->~BlackJackCard();
    return true;
}

void BlackJackMenu::SetFlag(enumFlags aeFlag, bool abValue) {
    if (abValue) {
        iFlags |= aeFlag;
    }
    else {
        iFlags &= ~aeFlag;
    }
}

// The chip compares are signed; isSplitHand is tested again before each step.
bool BlackJackMenu::CanDoubleDown(bool isSplitHand) {
    bool bCanDoubleDown = true;
    if (isSplitHand && iChipCount < static_cast<int32_t>(iCurrentBet + iCurrentSplitBet * 2)) {
        bCanDoubleDown = false;
    }
    if (!isSplitHand && iChipCount < static_cast<int32_t>(iCurrentSplitBet + iCurrentBet * 2)) {
        bCanDoubleDown = false;
    }
    // Sets BLF_DOUBLEDOWN_SPLIT for the split hand rather than BLF_CANDOUBLEDOWN_SPLIT, which DoClick reads and clears;
    // this mismatch is deliberate.
    if (isSplitHand) {
        SetFlag(BLF_DOUBLEDOWN_SPLIT, bCanDoubleDown);
    }
    else {
        SetFlag(BLF_CANDOUBLEDOWN_MAIN, bCanDoubleDown);
    }
    return bCanDoubleDown;
}

void BlackJackMenu::ZeroFlags() {
    iFlags = 0;
}

void BlackJackMenu::SetSideHandVisibility(bool bVisible) {
    if (!bVisible) {
        pHandSplitGeometry->SetTranslate(0.0f, 0.0f, -5.0f);
    }
    else {
        pHandSplitGeometry->SetTranslate(0.0f, 0.0f, 0.0f);
    }
}

// The PC deals random cards, so a shuffle only refills the count.
void BlackJackMenu::Shuffle() {
    iCardsRemaining = 52 * iNumOfDecks;
}

void BlackJackMenu::ClearDeckHighlight() {
    NiAVObject* pArrow = BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0");
    pArrow->SetAppCulled(true);
    pArrow = BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0@#2");
    pArrow->SetAppCulled(true);
}

char BlackJackMenu::HasTiles() {
    int32_t i;
    for (i = 0; i < 10; ++i) {
        if (!pTiles[i]) {
            return 0;
        }
    }
    return 1;
}

int32_t BlackJackMenu::GetHandValue(BSSimpleList<BlackJackMenu::BlackJackCard*>* pHand) {
    int32_t points = 0;
    int32_t numOfAces = 0;
    while (pHand && pHand->m_item) {
        BlackJackMenu::BlackJackCard* cd = pHand->m_item;
        points += GetNumericCardValue(cd->uiValue);
        if (cd->uiValue == BlackJackMenu::BL_A) {
            ++numOfAces;
        }
        pHand = pHand->m_pkNext;
    }
    while (points > 21 && numOfAces > 0) {
        points -= 10;
        --numOfAces;
    }
    return points;
}

// KEEP: luckPool holds 5 entries, but the exe fills abs(iLuckSkill - 5) of them with no bound: a luck outside 0..10
// writes on into its own frame (bestLowIndex, the count, ...), which changes how long the loop runs. src cannot
// express those writes into its own locals.
uint32_t BlackJackMenu::DealCard(BSSimpleList<BlackJackMenu::BlackJackCard*>* pHand, char abUseLuck, char abDealer) {
    BlackJackMenu::BlackJackCard tCard;
    BlackJackMenu::BlackJackCard card;
    int32_t cardindex = 0;
    if (this->iCardsRemaining < 5) {
        Shuffle();
    }
    if (pHand && abUseLuck && (!abDealer || iLuckSkill <= 4) && (abDealer || iLuckSkill >= 6)) {
        int32_t luckPool[5];
        int32_t pointTotal = GetHandValue(pHand);
        int32_t bestHighIndex = 0;
        int32_t bestLowIndex = 0;
        int32_t bestHighTotal = pointTotal;
        int32_t bestLowTotal = pointTotal;
        int32_t num_lc = std::abs(iLuckSkill - 5);
        for (int32_t i = 0; i < num_lc; ++i) {
            luckPool[i] = RandomInt(0, this->iCardsRemaining);
        }
        for (int32_t i = 0; i < num_lc; ++i) {
            int32_t tCardValue = GetNumericCardValue(pDeckOfCards[luckPool[i]].uiValue);
            if (tCardValue == 11) {
                tCardValue = pointTotal + 11 <= 21 ? 11 : 1;
            }
            if (pointTotal <= 9) {
                int32_t newTotal = pointTotal + tCardValue;
                if (21 - (pointTotal + tCardValue) < bestHighTotal && newTotal < 22) {
                    bestHighTotal = 21 - newTotal;
                    bestHighIndex = i;
                }
                if (11 - newTotal < bestLowTotal && newTotal < 12) {
                    bestLowTotal = 11 - newTotal;
                    bestLowIndex = i;
                }
            }
            else if (pointTotal + tCardValue > bestHighTotal && pointTotal + tCardValue < 22) {
                bestHighTotal = pointTotal + tCardValue;
                bestHighIndex = i;
            }
            if (bestHighTotal >= 4) {
                if (bestLowTotal >= bestHighTotal || bestLowTotal >= 2) {
                    cardindex = luckPool[bestHighIndex];
                }
                else {
                    cardindex = luckPool[bestLowIndex];
                    if (bestLowIndex) {
                        if (abDealer) {
                            bBadLuck = true;
                        }
                        else {
                            bGoodLuck = true;
                        }
                    }
                }
            }
            else {
                cardindex = luckPool[bestHighIndex];
                if (bestHighIndex) {
                    if (abDealer) {
                        bBadLuck = true;
                    }
                    else {
                        bGoodLuck = true;
                    }
                }
            }
        }
    }
    else {
        int32_t iRemaining = this->iCardsRemaining;
        this->iCardsRemaining = iRemaining - 1;
        cardindex = RandomInt(0, iRemaining);
    }
    card = pDeckOfCards[cardindex];
    tCard = pDeckOfCards[this->iCardsRemaining];
    pDeckOfCards[this->iCardsRemaining] = card;
    pDeckOfCards[cardindex] = tCard;
    uint32_t uiDealt = this->iCardsRemaining;
    this->iCardsRemaining = uiDealt - 1;
    return uiDealt;
}

int32_t BlackJackMenu::DealHand() {
    eHandResults[0] = 0;
    eHandResults[1] = 0;
    uint32_t uiCardIndex = DealCard(nullptr, 0, 0);
    if (pDealerHand && !pDealerHand->IsEmpty()) {
        BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[uiCardIndex];
        pDealerHand->AddHead(pCard);
    }
    else {
        BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[uiCardIndex];
        pDealerHand->SetItem(pCard);
    }
    uiCardIndex = DealCard(nullptr, 0, 0);
    if (pPlayerHand && !pPlayerHand->IsEmpty()) {
        BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[uiCardIndex];
        pDealerHand->AddHead(pCard);
    }
    else {
        BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[uiCardIndex];
        pPlayerHand->SetItem(pCard);
    }
    uiCardIndex = DealCard(pDealerHand, 1, 1);
    BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[uiCardIndex];
    pDealerHand->AddTail(pCard);
    uiCardIndex = DealCard(pPlayerHand, 1, 0);
    pCard = &pDeckOfCards[uiCardIndex];
    pPlayerHand->AddTail(pCard);
    return GetHandValue(pPlayerHand);
}

void BlackJackMenu::SetTileCasinoEarnings() {
    char acEarnings[264];
    int32_t iTotalWinnings = g_blackJackCasinoStats->iTotalWinnings;
    const char* pEarningsText = sEarningsText.String();
    BSsprintf(acEarnings, 260u, "%s %s%i", BlackJackMenu::pCasino->GetFullName(), pEarningsText, iTotalWinnings);
    int32_t iLength = BSstrlen(acEarnings);
    if (iLength < 23) {
        pEarningsText = sEarningsText.String();
        BSsprintf(acEarnings, 260u, "%s %s", BlackJackMenu::pCasino->GetFullName(), pEarningsText);
        for (int32_t i = 0; i < 23 - iLength; ++i) {
            acEarnings[iLength - 1 + i] = 32;
            acEarnings[i + iLength] = 0;
        }
        BSsprintf(acEarnings, 260u, "%s%i", acEarnings, g_blackJackCasinoStats->iTotalWinnings);
    }
    pTiles[2]->Set(g_blackJackMenuTrait_Value, acEarnings, true);
}

void BlackJackMenu::Prepare3DElements() {
    pHandGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Hand1.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    pHandSplitGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Hand2.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    pHandDealerGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Dealer.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    BlackJackMenu::spCamera = static_cast<NiCamera*>(BSUtilities::GetObjectByName(this->pTableGeometry, "object1"));
    NiNode* pSceneRoot = new NiNode(0);
    BlackJackMenu::spSceneRoot = pSceneRoot;
    BlackJackMenu::spSceneRoot->AttachChild(this->pTableGeometry, 1);
    BlackJackMenu::spSceneRoot->AttachChild(pHandGeometry, 1);
    BlackJackMenu::spSceneRoot->AttachChild(pHandSplitGeometry, 1);
    BlackJackMenu::spSceneRoot->AttachChild(pHandDealerGeometry, 1);
    for (int32_t i = 0; i < 6; ++i) {
        BlackJackMenu::spSceneRoot->AttachChild(pBetChipGeometry[i], 1);
        BlackJackMenu::spSceneRoot->AttachChild(pBetShadowGeometry[i], 1);
    }
    SetCorrectChipMeshesVisibleForCurrentBet();
    SetSideHandVisibility(false);
    ClearDeckHighlight();
    NiUpdateData UpdateData(0.0f, false, false);
    BlackJackMenu::spSceneRoot->Update(UpdateData);
    NiControllerManager* pController = static_cast<NiControllerManager*>(
        this->pTableGeometry->GetController(&NiControllerManager::ms_RTTI)
    );
    if (pController) {
        pTableAnimManager = pController;
        pTableAnimations[0] = pTableAnimManager->GetSequenceByName("Players_Discard");
        pTableAnimations[1] = pTableAnimManager->GetSequenceByName("Dealers_Discard");
        pTableAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(this->pTableGeometry);
    }
    pController = static_cast<NiControllerManager*>(pHandGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (pController) {
        pPlayerAnimManager = pController;
        pPlayerAnimations[0] = pPlayerAnimManager->GetSequenceByName("Hand1_Discard");
        pPlayerAnimations[1] = pPlayerAnimManager->GetSequenceByName("Hand1_01");
        pPlayerAnimations[2] = pPlayerAnimManager->GetSequenceByName("Hand1_02");
        pPlayerAnimations[3] = pPlayerAnimManager->GetSequenceByName("Hand1_03");
        pPlayerAnimations[4] = pPlayerAnimManager->GetSequenceByName("Hand1_04");
        pPlayerAnimations[5] = pPlayerAnimManager->GetSequenceByName("Hand1_05");
        pPlayerAnimations[6] = pPlayerAnimManager->GetSequenceByName("Hand1_06");
        pPlayerAnimations[7] = pPlayerAnimManager->GetSequenceByName("Hand1_07");
        pPlayerAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pHandGeometry);
    }
    pController = static_cast<NiControllerManager*>(pHandSplitGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (pController) {
        pPlayerSplitAnimManager = pController;
        pPlayerSplitAnimations[0] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_Discard");
        pPlayerSplitAnimations[1] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_01");
        pPlayerSplitAnimations[2] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_02");
        pPlayerSplitAnimations[3] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_03");
        pPlayerSplitAnimations[4] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_04");
        pPlayerSplitAnimations[5] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_05");
        pPlayerSplitAnimations[6] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_06");
        pPlayerSplitAnimations[7] = pPlayerSplitAnimManager->GetSequenceByName("Hand2_07");
        pPlayerSplitAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pHandSplitGeometry);
    }
    pController = static_cast<NiControllerManager*>(pHandDealerGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (pController) {
        pDealerAnimManager = pController;
        pDealerAnimations[0] = pDealerAnimManager->GetSequenceByName("Dealer_Discard");
        pDealerAnimations[1] = pDealerAnimManager->GetSequenceByName("Dealer_01");
        pDealerAnimations[2] = pDealerAnimManager->GetSequenceByName("Dealer_02");
        pDealerAnimations[3] = pDealerAnimManager->GetSequenceByName("Dealer_03");
        pDealerAnimations[4] = pDealerAnimManager->GetSequenceByName("Dealer_04");
        pDealerAnimations[5] = pDealerAnimManager->GetSequenceByName("Dealer_05");
        pDealerAnimations[6] = pDealerAnimManager->GetSequenceByName("Dealer_06");
        pDealerAnimations[7] = pDealerAnimManager->GetSequenceByName("Dealer_07");
        pDealerReveal = pDealerAnimManager->GetSequenceByName("Dealer_Reveal");
        pDealerAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pHandDealerGeometry);
    }
    NiUpdateData arData(0.0f, false, false);
    BlackJackMenu::spSceneRoot->Update(arData);
    if (!BlackJackMenu::spCamera.m_pObject) {
        BlackJackMenu::spCamera = SetupCamera();
    }
    NiNode* pDeck = static_cast<NiNode*>(BSUtilities::GetObjectByName(this->pTableGeometry, "Deck:0"));
    // Shader property types 8 to 12 are the per-pixel lighting properties; the type check fetches the property again
    // for each test.
    int32_t iPPLighting = 0;
    if (pDeck->GetProperty(NiProperty::SHADE)) {
        NiProperty* pProperty = pDeck->GetProperty(NiProperty::SHADE);
        if (static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() >= 8) {
            pProperty = pDeck->GetProperty(NiProperty::SHADE);
            if (static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() <= 12) {
                iPPLighting = true;
            }
        }
    }
    NiProperty* pLightingProperty = nullptr;
    if (iPPLighting) {
        pLightingProperty = pDeck->GetProperty(NiProperty::SHADE);
    }
    if (pLightingProperty) {
        NiSourceTexture* pBackTexture = pDeckOfCards[0].spBackTexture.m_pObject;
        static_cast<BSShaderPPLightingProperty*>(pLightingProperty)
            ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pBackTexture);
    }
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(BlackJackMenu::spSceneRoot.m_pObject, 1);
    float fRadius = BlackJackMenu::spSceneRoot->GetWorldBound().m_fRadius * 3.0;
    pSceneRoot = BlackJackMenu::spSceneRoot.m_pObject;
    BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pSceneRoot, fRadius);
}

void BlackJackMenu::UpdateHandCardVisibility() {
    char acBackName[32];
    NiAVObject* pObject;
    char blockName[32];
    char acShadowName[32];
    for (int32_t i = 1; i <= 7; ++i) {
        BSsprintf(blockName, 30, "Hand1_0%i:%i", i, i - 1);
        BSsprintf(acBackName, 30, "Hand1_0%i_Back:0", i);
        BSsprintf(acShadowName, 30, "Hand1_0%i_Shadow:0", i);
        pObject = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(blockName));
        if (i < iAnimIndex[0]) {
            pObject->SetAppCulled(false);
            pObject = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(acBackName));
            pObject->SetAppCulled(false);
            pObject = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(acShadowName));
            pObject->SetAppCulled(false);
        }
        else {
            pObject->SetAppCulled(true);
            pObject = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(acBackName));
            pObject->SetAppCulled(true);
            pObject = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(acShadowName));
            pObject->SetAppCulled(true);
        }
    }
    for (int32_t j = 1; j <= 7; ++j) {
        BSsprintf(blockName, 30, "Dealer_0%i:0", j);
        BSsprintf(acBackName, 30, "Dealer_0%i_Back:0", j);
        BSsprintf(acShadowName, 30, "Dealer_0%i_Shadow:0", j);
        pObject = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(blockName));
        if (j < iAnimIndex[2]) {
            pObject->SetAppCulled(false);
            pObject = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(acBackName));
            pObject->SetAppCulled(false);
            pObject = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(acShadowName));
            pObject->SetAppCulled(false);
        }
        else {
            pObject->SetAppCulled(true);
            pObject = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(acBackName));
            pObject->SetAppCulled(true);
            pObject = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(acShadowName));
            pObject->SetAppCulled(true);
        }
    }
    for (int32_t k = 1; k <= 7; ++k) {
        BSsprintf(blockName, 30, "Hand2_0%i:%i", k, k - 1);
        BSsprintf(acBackName, 30, "Hand2_0%i_Back:0", k);
        BSsprintf(acShadowName, 30, "Hand2_0%i_Shadow:0", k);
        pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(blockName));
        if (k < iAnimIndex[1]) {
            pObject->SetAppCulled(false);
            pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(acBackName));
            pObject->SetAppCulled(false);
            pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(acShadowName));
            pObject->SetAppCulled(false);
        }
        else {
            pObject->SetAppCulled(true);
            pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(acBackName));
            pObject->SetAppCulled(true);
            pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(acShadowName));
            pObject->SetAppCulled(true);
        }
    }
    NiUpdateData UpdateData(0.0f, 0, 0);
    BlackJackMenu::spSceneRoot->Update(UpdateData);
}

void BlackJackMenu::ShowAllHandCards() {
    char acBackName[32];
    NiAVObject* pObject;
    char blockName[32];
    char acShadowName[32];
    for (int32_t i = 1; i <= 7; ++i) {
        BSsprintf(blockName, 30u, "Hand1_0%i:%i", i, i - 1);
        BSsprintf(acBackName, 30u, "Hand1_0%i_Back:0", i);
        BSsprintf(acShadowName, 30u, "Hand1_0%i_Shadow:0", i);
        pObject = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandGeometry), blockName);
        pObject->SetAppCulled(false);
        pObject = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandGeometry), acBackName);
        pObject->SetAppCulled(false);
        pObject = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandGeometry), acShadowName);
        pObject->SetAppCulled(false);
    }
    for (int32_t j = 1; j <= 7; ++j) {
        BSsprintf(blockName, 30u, "Dealer_0%i:0", j);
        BSsprintf(acBackName, 30u, "Dealer_0%i_Back:0", j);
        BSsprintf(acShadowName, 30u, "Dealer_0%i_Shadow:0", j);
        pObject = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandDealerGeometry), blockName);
        pObject->SetAppCulled(false);
        pObject = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandDealerGeometry), acBackName);
        pObject->SetAppCulled(false);
        pObject = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandDealerGeometry), acShadowName);
        pObject->SetAppCulled(false);
    }
    for (int32_t k = 1; k <= 7; ++k) {
        BSsprintf(blockName, 30u, "Hand2_0%i:%i", k, k - 1);
        BSsprintf(acBackName, 30u, "Hand2_0%i_Back:0", k);
        BSsprintf(acShadowName, 30u, "Hand2_0%i_Shadow:0", k);
        pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, blockName);
        pObject->SetAppCulled(false);
        pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, acBackName);
        pObject->SetAppCulled(false);
        pObject = BSUtilities::GetObjectByName(pHandSplitGeometry, acShadowName);
        pObject->SetAppCulled(false);
    }
    NiUpdateData UpdateData(0.0f, 0, 0);
    BlackJackMenu::spSceneRoot->Update(UpdateData);
}

void* BlackJackMenu::SetTileAlphasForStage(int32_t aiStage) {
    void* pResult = this;
    if (eBlackJackState != 10) {
        switch (aiStage) {
            case 3: {
                if (IsFlagSet(BLF_SPLITHANDACTIVE)) {
                    if (IsFlagSet(BLF_DISABLEHAND_SPLIT)) {
                        return BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                    }
                    return BlackJackMenu::SetNthTileAlpha(3, 255.0f);
                }
                else if (IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                    return BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                }
                return BlackJackMenu::SetNthTileAlpha(3, 255.0f);
            }
            case 4: {
                if (IsFlagSet(BLF_SPLITHANDACTIVE)) {
                    if (IsFlagSet(BLF_CANDOUBLEDOWN_SPLIT)) {
                        return BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                    }
                    return BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                }
                else if (IsFlagSet(BLF_CANDOUBLEDOWN_MAIN)) {
                    return BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                }
                return BlackJackMenu::SetNthTileAlpha(4, 128.0f);
            }
            case 5: {
                if (IsFlagSet(BLF_SPLITHANDACTIVE) || !IsFlagSet(BLF_CANSPLIT)) {
                    return BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                }

                return BlackJackMenu::SetNthTileAlpha(5, 255.0f);
            }
            case 6: {
                if (IsFlagSet(BLF_SPLITHANDACTIVE)) {
                    if (IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                        return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    }
                    return BlackJackMenu::SetNthTileAlpha(6, 255.0f);
                }
                if (!IsFlagSet(BLF_DISABLEHAND_SPLIT) && IsFlagSet(BLF_SPLIT)) {
                    return BlackJackMenu::SetNthTileAlpha(6, 255.0f);
                }
                return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
            }
            case 7: {
                if (IsFlagSet(BLF_DISABLESURRENDER)) {
                    return BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                }

                return BlackJackMenu::SetNthTileAlpha(7, 255.0f);
            }
            case 8: {
                return BlackJackMenu::SetNthTileAlpha(8, 255.0f);
            }
            case 10: {
                if (IsFlagSet(BLF_SPLITHANDACTIVE)) {
                    if (IsFlagSet(BLF_DISABLEHAND_SPLIT)) {
                        BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                    }
                    else {
                        BlackJackMenu::SetNthTileAlpha(3, 255.0f);
                        if (IsFlagSet(BLF_CANDOUBLEDOWN_SPLIT)) {
                            BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                        }
                    }
                    BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(8, 255.0f);
                    if (IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                        return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    }
                    return BlackJackMenu::SetNthTileAlpha(6, 255.0f);
                }
                else {
                    if (IsFlagSet(BLF_DISABLEHAND_MAIN) || IsFlagSet(BLF_BLACKJACK)) {
                        BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                    }
                    else {
                        BlackJackMenu::SetNthTileAlpha(3, 255.0f);
                        if (IsFlagSet(BLF_CANDOUBLEDOWN_MAIN)) {
                            BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                        }
                        if (IsFlagSet(BLF_CANSPLIT)) {
                            BlackJackMenu::SetNthTileAlpha(5, 255.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                        }
                        if (IsFlagSet(BLF_DISABLESURRENDER)) {
                            BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(7, 255.0f);
                        }
                    }
                    if (IsFlagSet(BLF_BLACKJACK) && (!IsFlagSet(BLF_SPLIT) || IsFlagSet(BLF_DISABLEHAND_SPLIT))) {
                        BlackJackMenu::SetNthTileAlpha(8, 128.0f);
                    }
                    else {
                        BlackJackMenu::SetNthTileAlpha(8, 255.0f);
                    }
                    if (IsFlagSet(BLF_DISABLEHAND_SPLIT) || !IsFlagSet(BLF_SPLIT)) {
                        return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    }
                    return BlackJackMenu::SetNthTileAlpha(6, 255.0f);
                }
            }
            default: {
                return pResult;
            }
        }
    }
    return pResult;
}

char BlackJackMenu::CanSplitHand() {
    BSSimpleList<BlackJackMenu::BlackJackCard*>* pNode = pPlayerHand;
    BlackJackMenu::BlackJackCard* pFirst = nullptr;
    int32_t i = 0;
    char bCanSplit = 0;
    while (pNode && pNode->m_item) {
        if (i) {
            // A third card ends the walk with no split.
            if (i != 1) {
                bCanSplit = 0;
                break;
            }
            if (pFirst->uiValue == pNode->m_item->uiValue && iChipCount >= 2 * iCurrentBet) {
                bCanSplit = 1;
            }
        }
        else {
            pFirst = pNode->m_item;
        }
        pNode = pNode->m_pkNext;
        ++i;
    }
    SetFlag(BlackJackMenu::BLF_CANSPLIT, bCanSplit);
    return bCanSplit;
}

void BlackJackMenu::HighlightSelectedDeck() {
    if (IsFlagSet(BLF_SPLITHANDACTIVE)) {
        BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0")->SetAppCulled(true);
        BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0@#2")->SetAppCulled(false);
    }
    else {
        BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0")->SetAppCulled(false);
        BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0@#2")->SetAppCulled(true);
    }
    NiUpdateData UpdateData(0.0, false, false);
    BlackJackMenu::spSceneRoot->GetAt(1u)->Update(UpdateData);
}

void BlackJackMenu::SetCorrectChipMeshesVisibleForCurrentBet() {
    if (iCurrentBet >= 7) {
        if (iCurrentBet >= 40) {
            if (iCurrentBet > 100) {
                if (iCurrentBet > 300) {
                    if (iCurrentBet > 500) {
                        if (iCurrentBet >= 1000) {
                            if (iCurrentBet >= 1500) {
                                int32_t iStackCount = iCurrentBet / 500 - 3 <= 10 ? iCurrentBet / 500 - 3 : 10;
                                pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 1.8f);
                                float fStackHeight = static_cast<double>(iStackCount) * 0.18000000000000002;
                                pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, fStackHeight);
                                pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                                if (iCurrentBet == 1500) {
                                    pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
                                }
                                else {
                                    pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
                                }
                            }
                            else {
                                pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 1.62f);
                                pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.18f);
                                float fStackHeight = static_cast<double>(iCurrentBet / 100 - 9) * 0.18000000000000002;
                                pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, fStackHeight);
                                pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                                pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
                            }
                        }
                        else {
                            pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 1.8f);
                            pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 1.8f);
                            pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 1.62f);
                            float fStackHeight = static_cast<double>(iCurrentBet % 100 / 25 + 6) * 0.18000000000000002;
                            pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, fStackHeight);
                            fStackHeight = static_cast<double>(iCurrentBet / 100 - 3) * 0.18000000000000002;
                            pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, fStackHeight);
                            pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
                            pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
                            pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
                            pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
                            pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
                            if (iCurrentBet / 100 == 3) {
                                pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, -5.0f);
                            }
                            else {
                                pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                            }
                            pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                            pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
                        }
                    }
                    else {
                        pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 1.8f);
                        pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 1.44f);
                        pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 0.9f);
                        pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 0.36f);
                        float fStackHeight = static_cast<double>(iCurrentBet / 25 - 12) * 0.18000000000000002;
                        pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, fStackHeight);
                        pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
                        pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
                        pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
                        pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
                        pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
                        pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                        pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
                    }
                }
                else {
                    pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 1.8f);
                    pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 1.44f);
                    float fStackHeight = static_cast<double>(iCurrentBet % 25 / 10 + 5) * 0.18000000000000002;
                    pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, fStackHeight);
                    fStackHeight = static_cast<double>(iCurrentBet / 25 - 4) * 0.18000000000000002;
                    pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, fStackHeight);
                    pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                    pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
                    pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
                    pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
                    pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
                    if (iCurrentBet / 25 == 4) {
                        pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, -5.0f);
                    }
                    else {
                        pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
                    }
                    pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, -5.0f);
                    pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
                }
            }
            else {
                pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 0.9f);
                pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 0.9f);
                float fStackHeight = static_cast<double>(iCurrentBet / 10 - 3) * 0.18000000000000002;
                pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, fStackHeight);
                pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
                pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
                pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
                pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
                pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
                pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
                pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, -5.0f);
                pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, -5.0f);
                pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
            }
        }
        else {
            float fStackHeight = static_cast<double>(iCurrentBet % 5 + 5) * 0.18000000000000002;
            pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, fStackHeight);
            fStackHeight = static_cast<double>(iCurrentBet / 5 - 1) * 0.18000000000000002;
            pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, fStackHeight);
            pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
            pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
            pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
            pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
            pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
            if (iCurrentBet <= 10) {
                pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, -5.0f);
            }
            else {
                pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
            }
            pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, -5.0f);
            pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, -5.0f);
            pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, -5.0f);
            pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
        }
    }
    else {
        float fStackHeight = static_cast<double>(iCurrentBet) * 0.18000000000000002;
        pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, fStackHeight);
        pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 0.0f);
        pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 0.0f);
        pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, 0.0f);
        pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 0.0f);
        pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, 0.0f);
        pBetShadowGeometry[0]->SetTranslate(0.0f, 0.0f, 0.0f);
        pBetShadowGeometry[1]->SetTranslate(0.0f, 0.0f, -5.0f);
        pBetShadowGeometry[2]->SetTranslate(0.0f, 0.0f, -5.0f);
        pBetShadowGeometry[3]->SetTranslate(0.0f, 0.0f, -5.0f);
        pBetShadowGeometry[4]->SetTranslate(0.0f, 0.0f, -5.0f);
        pBetShadowGeometry[5]->SetTranslate(0.0f, 0.0f, -5.0f);
    }
    NiUpdateData arData(0.0f, false, false);
    BlackJackMenu::spSceneRoot->Update(arData);
}

NiCamera* BlackJackMenu::SetupCamera() {
    int32_t iHeight;
    NiRect<float> kPort;
    NiCamera* pCamera = new NiCamera;
    ::new (&kPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pCamera->SetViewPort(kPort);
    NiFrustum kFrustum(false);
    kFrustum.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    float fMaxFarNearRatio = kFrustum.m_fFar / kFrustum.m_fNear;
    pCamera->SetMaxFarNearRatio(fMaxFarNearRatio);
    if (BSShaderManager::bLetterbox) {
        iHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        iHeight = Renderer::GetScreenHeight();
    }
    float aspectRatio = static_cast<double>(iHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fAngle = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    kFrustum.m_fLeft = -std::tan(fAngle) * 0.75f;
    kFrustum.m_fRight = std::tan(fAngle) * 0.75f;
    double dTan = std::tan(fAngle);
    kFrustum.m_fBottom = -dTan * aspectRatio * 0.75f;
    dTan = std::tan(fAngle);
    kFrustum.m_fTop = dTan * aspectRatio * 0.75f;
    kFrustum.m_bOrtho = false;
    pCamera->SetViewFrustum(kFrustum);
    pCamera->LookAtWorldPoint(NiPoint3(0.0f, 0.0f, 1.0f), NiPoint3(0.0f, 0.0f, 1.0f));
    pCamera->SetMinNearPlaneDist(1.0f);
    NiUpdateData arData(0.0f, false, false);
    pCamera->Update(arData);
    NiFixedString kName("Surgery3DCamera");
    pCamera->SetName(kName);
    return pCamera;
}

void BlackJackMenu::SwapCardTextures(int32_t aiHandIndex, int32_t aiCardIndex, BlackJackMenu::BlackJackCard* cd) {
    uint32_t uiNodeIndex = aiHandIndex + 1;
    if (aiCardIndex <= 7 && aiCardIndex >= 1 && cd) {
        char acBackName[32];
        char blockName[32];
        switch (uiNodeIndex) {
            case 1u: {
                BSsprintf(blockName, 30, "Hand1_0%i:%i", aiCardIndex, aiCardIndex - 1);
                BSsprintf(acBackName, 30, "Hand1_0%i_Back:0", aiCardIndex);
                break;
            }
            case 2u: {
                BSsprintf(blockName, 30, "Hand2_0%i:%i", aiCardIndex, aiCardIndex - 1);
                BSsprintf(acBackName, 30, "Hand2_0%i_Back:0", aiCardIndex);
                break;
            }
            case 3u: {
                BSsprintf(blockName, 30, "Dealer_0%i:0", aiCardIndex);
                BSsprintf(acBackName, 30, "Dealer_0%i_Back:0", aiCardIndex);
                break;
            }
            default: {
                return;
            }
        }
        NiAVObject* pHandNode = BlackJackMenu::spSceneRoot->GetAt(uiNodeIndex);
        NiAVObject* pavobj = BSUtilities::GetObjectByName(pHandNode, static_cast<const char*>(blockName));
        // The shade property's type tells a BSShaderPPLightingProperty (8 to 12).
        int32_t iPPLighting = 0;
        if (pavobj->GetProperty(3) &&
            static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() >= 8 &&
            static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() <= 12) {
            iPPLighting = true;
        }
        BSShaderPPLightingProperty* pFaceShader = nullptr;
        if (iPPLighting) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pavobj->GetProperty(3));
        }
        if (pFaceShader) {
            pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, cd->spFrontTexture.m_pObject);
        }
        pHandNode = BlackJackMenu::spSceneRoot->GetAt(uiNodeIndex);
        pavobj = BSUtilities::GetObjectByName(pHandNode, static_cast<const char*>(acBackName));
        iPPLighting = 0;
        if (pavobj->GetProperty(3) &&
            static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() >= 8 &&
            static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() <= 12) {
            iPPLighting = true;
        }
        pFaceShader = nullptr;
        if (iPPLighting) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pavobj->GetProperty(3));
        }
        if (pFaceShader) {
            pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, cd->spBackTexture.m_pObject);
        }
    }
}

void BlackJackMenu::DrawScene() {
    NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCullingProcess = pSceneGraph->pCuller;
    pCullingProcess->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* pCamera = BlackJackMenu::spCamera.m_pObject;
    HandleEyeFinityFOVScale(pCamera, 0.15f);
    NiPointer<NiRefObject> spAccumulator(static_cast<NiRefObject*>(InterfaceManager::Instance()->Get3dAccumulator()));
    NiAccumulator* pAccumulator = static_cast<NiAccumulator*>(spAccumulator.m_pObject);
    pCullingProcess->SetAccumulator(pAccumulator);
    NiCamera* pDrawCamera;
    if (pCamera) {
        pDrawCamera = pCamera;
    }
    else {
        pDrawCamera = pSceneGraph->GetCamera();
    }
    ShadowSceneNode* pSceneNode = BSShaderManager::GetShadowSceneNode(1);
    BSShaderUtil::AccumulateScene(pDrawCamera, pSceneNode, *pCullingProcess);
    BSShaderAccumulator* pShaderAccumulator = static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject);
    BSShaderUtil::RenderScene(pDrawCamera, pShaderAccumulator, false);
    pCullingProcess->SetAccumulator(nullptr);
    spAccumulator = static_cast<NiRefObject*>(nullptr);
    pCullingProcess->PopCullMode();
}

void* BlackJackMenu::SetNthTileAlpha(int32_t aiIndex, float afAlpha) {
    NiTListItem<Tile*>* pPos = BlackJackMenu::pMe->pTiles[aiIndex]->xChildren.m_pkHead;
    // Returns whatever the last step left: pMe, the head slot or the last position.
    void* pResult = BlackJackMenu::pMe;
    NiTPointerList<Tile*>* pChildren = &BlackJackMenu::pMe->pTiles[aiIndex]->xChildren;
    while (pPos) {
        if (!pChildren) {
            break;
        }
        Tile** ppHead = &pChildren->GetHead();
        pResult = ppHead;
        if (!*ppHead) {
            break;
        }
        pChildren->Get(pPos)->Set(4009, afAlpha, true);
        pPos = pChildren->GetNextPos(pPos);
        pResult = pPos;
    }
    return pResult;
}

void BlackJackMenu::SetShouldExit() {
    if (BlackJackMenu::pMe) {
        BlackJackMenu::pMe->eBlackJackState = BL_CLOSING_MENU;
        Tile* pCursor = static_cast<Tile*>(InterfaceManager::Instance()->GetCursorTile());
        pCursor->Set(4009, 255.0f, true);
    }
}

void BlackJackMenu::SetDealNotSwitchHands(char abDeal) {
    const char* pText;
    BlackJackMenu::pMe->pTiles[4]->Poke(4036, 1.0);
    if (abDeal) {
        pText = sDealText.String();
        BlackJackMenu::pMe->pTiles[4]->Set(4036, pText, true);
        BlackJackMenu::SetNthTileAlpha(4, 255.0f);
        BlackJackMenu::pMe->pTiles[5]->Poke(4036, 1.0);
        pText = sIncreaseBetText.String();
        BlackJackMenu::pMe->pTiles[5]->Set(4036, pText, true);
        BlackJackMenu::SetNthTileAlpha(5, 255.0f);
        BlackJackMenu::pMe->pTiles[6]->Poke(4036, 1.0);
        pText = sDecreaseBetText.String();
        BlackJackMenu::pMe->pTiles[6]->Set(4036, pText, true);
        BlackJackMenu::SetNthTileAlpha(6, 255.0f);
        BlackJackMenu::pMe->pTiles[7]->Poke(4036, 1.0);
        pText = sBetMaxText.String();
        BlackJackMenu::pMe->pTiles[7]->Set(4036, pText, true);
        BlackJackMenu::SetNthTileAlpha(7, 255.0f);
        BlackJackMenu::pMe->pTiles[8]->Poke(4036, 1.0);
        pText = sExit.String();
        BlackJackMenu::pMe->pTiles[8]->Set(4036, pText, true);
        BlackJackMenu::SetNthTileAlpha(8, 255.0f);
        BlackJackMenu::pMe->pTiles[3]->Set(4003, 0);
    }
    else {
        pText = sDoubleDownText.String();
        BlackJackMenu::pMe->pTiles[4]->Set(4036, pText, true);
        BlackJackMenu::pMe->pTiles[5]->Poke(4036, 1.0);
        pText = sSplitText.String();
        BlackJackMenu::pMe->pTiles[5]->Set(4036, pText, true);
        BlackJackMenu::pMe->pTiles[6]->Poke(4036, 1.0);
        pText = sSwitchHandsText.String();
        BlackJackMenu::pMe->pTiles[6]->Set(4036, pText, true);
        BlackJackMenu::pMe->pTiles[7]->Poke(4036, 1.0);
        pText = sSurrenderText.String();
        BlackJackMenu::pMe->pTiles[7]->Set(4036, pText, true);
        BlackJackMenu::pMe->pTiles[8]->Poke(4036, 1.0);
        pText = sStayText.String();
        BlackJackMenu::pMe->pTiles[8]->Set(4036, pText, true);
        BlackJackMenu::pMe->pTiles[3]->Set(4003, 1);
    }
}

Tile* BlackJackMenu::Create(
    TESCasino* apCasino,
    int32_t aiMinBet,
    int32_t aiMaxBet,
    int32_t aiMinWinnings,
    bool abDebugMode
) {
    BSSimpleList<CasinoData*>* cl;
    Tile* pold_menu_tile = Menu::GetMenuByClass(1081);
    InterfaceManager* pin = InterfaceManager::Instance();
    BlackJackMenu::pCasino = apCasino;
    g_blackJackMinChipsToPlay = aiMinBet;
    g_blackJackMaxBet = aiMaxBet;
    g_blackJackCasinoStats = nullptr;
    for (cl = pPlayer->GetCasinoDataList(); cl && cl->m_item; cl = cl->m_pkNext) {
        if (BlackJackMenu::pCasino->iFormID == cl->m_item->casinoID) {
            g_blackJackCasinoStats = cl->m_item;
            break;
        }
        if (!cl->m_pkNext) {
            break;
        }
    }
    if (!g_blackJackCasinoStats) {
        CasinoData* cd;
        cd = new CasinoData;
        cd->casinoID = BlackJackMenu::pCasino->iFormID;
        cd->iTotalWinnings = 0;
        cl = pPlayer->GetCasinoDataList();
        if (cl && !cl->IsEmpty()) {
            cl->AddHead(cd);
        }
        else {
            cl->SetItem(cd);
        }
        g_blackJackCasinoStats = cd;
    }
    if (pPlayer->CasinoAntiCheatCheck()) {
        char msg[264];
        int32_t iTimeRemaining = pPlayer->GetCasinoCheatTimeRemaining();
        BSsprintf(msg, 260, "%s\nTime Remaining: %i", sBlackjackAntiCheatText.String(), iTimeRemaining);
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

    if (g_blackJackCasinoStats->iTotalWinnings < BlackJackMenu::pCasino->data.iMaxWinnings) {
        TESCasinoChips* pChips = BlackJackMenu::pCasino->GetChipType();
        if (pPlayer->GetInventoryObjectCount(pChips) >= g_blackJackMinChipsToPlay) {
            if (g_blackJackCasinoStats->iTotalWinnings < aiMinWinnings && aiMinWinnings) {
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
            Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\black_jack_menu.xml");
            BlackJackMenu::pMe = static_cast<BlackJackMenu*>(ptile->GetMenu());
            if (BlackJackMenu::pMe && BlackJackMenu::pMe->GetClass() == BlackJackMenu::GetClassStatic()) {
                BlackJackMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
                if (BlackJackMenu::pMe->HasTiles()) {
                    char acEarnings[64];
                    if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                        float fDepth = Tile::GetMaximumDepth();
                        ptile->Set(4013, fDepth, true);
                    }
                    BSAudio::QInstance().GetSoundHandleByName("GAMEBlackJackCardShuffle", 289u).Play(false);
                    BlackJackMenu::pMe->iLuckSkill = pPlayer->GetClampedActorValue(ActorValue::LUCK);
                    BlackJackMenu::pMe->iCurrentBet = g_blackJackMinChipsToPlay;
                    const char* pText = sCurrentBetText.String();
                    BlackJackMenu::pMe->pTiles[0]->Set(g_blackJackMenuTrait_Value, pText, true);
                    BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, g_blackJackMinChipsToPlay);
                    pText = sChipCountText.String();
                    BlackJackMenu::pMe->pTiles[1]->Set(g_blackJackMenuTrait_Value, pText, true);
                    BlackJackMenu::pMe->pTiles[1]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    pChips = BlackJackMenu::pCasino->GetChipType();
                    int32_t iChips = pPlayer->GetInventoryObjectCount(pChips);
                    BlackJackMenu::pMe->pTiles[1]->Set(BlackJackMenu::iUT_X, iChips);
                    if (BSstrlen(BlackJackMenu::pCasino->GetFullName())) {
                        const char* pEarningsText = sEarningsText.String();
                        BSsprintf(acEarnings, 64, "%s %s", BlackJackMenu::pCasino->GetFullName(), pEarningsText);
                    }
                    else {
                        const char* pEarningsText = sEarningsText.String();
                        BSsprintf(acEarnings, 64, "%s %s", BlackJackMenu::pCasino->GetFormEditorID(), pEarningsText);
                    }
                    BlackJackMenu::pMe->pTiles[2]->Set(g_blackJackMenuTrait_Value, acEarnings, true);
                    BlackJackMenu::pMe->pTiles[2]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[2]->Set(BlackJackMenu::iUT_X, g_blackJackCasinoStats->iTotalWinnings);
                    Tile::GetTileByName(BlackJackMenu::pMe->pTiles[2], "BJM_TotalEarningsValue")->Set(4003, 0);
                    BlackJackMenu::pMe->SetTileCasinoEarnings();
                    BlackJackMenu::pMe->pTiles[3]->Poke(4036, 1.0f);
                    pText = sHitText.String();
                    BlackJackMenu::pMe->pTiles[3]->Set(4036, pText, true);
                    if (BlackJackMenu::pMe->InitCasinoData()) {
                        BlackJackMenu::SetDealNotSwitchHands(1);
                        BlackJackMenu::pMe->Prepare3DElements();
                        BlackJackMenu::pMe->PrepForVisibility(false);
                        BlackJackMenu::pMe->pTiles[3]->Set(4003, 0);
                        BlackJackMenu::pMe->ZeroFlags();
                        return ptile;
                    }

                    return nullptr;
                }

                Warning(
                    "MENUS: BlackJack Menu Creation Failed... Are your menu and art resources up to "
                    "date?"
                );
                return nullptr;
            }

            if (BlackJackMenu::pMe && BlackJackMenu::pMe->pMenu && BlackJackMenu::pMe) {
                delete BlackJackMenu::pMe;
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

int32_t BlackJackMenu::GetClassStatic() {
    return 1081;
}

CasinoData::CasinoData() : casinoID(0), iTotalWinnings(0), iWinningLevel(0) {}

TESCasinoChips* TESCasino::GetChipType() {
    return static_cast<TESCasinoChips*>(TESForm::GetFormByNumericID(data.casinoChipID));
}
