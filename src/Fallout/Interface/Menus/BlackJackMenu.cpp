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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Reconstruction.hpp"
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
    BlackJackMenu::spSceneRoot.m_pObject->DetachAllChildren();
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
    // The hand pointers are cleared above before they are tested, so these deletes never run, as the game does.
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

void BlackJackMenu::AttachTileByID(int32_t a1, Tile* a2) {
    if (static_cast<uint32_t>(a1) <= 9) {
        pTiles[a1] = a2;
    }
}

void BlackJackMenu::DoClick(int32_t a1, Tile* a2) {
    if (eBlackJackState == BlackJackMenu::BL_IDLE || eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
        switch (a1) {
            case 3: {
                if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
                    int32_t v39 = 0;
                    int32_t v36 = 0;
                    eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                    SetFlag(BlackJackMenu::BLF_DISABLESURRENDER, true);
                    SetTileAlphasForStage(7);
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                    SetFlag(BlackJackMenu::BLF_CANSPLIT, false);
                    if (!IsFlagSet(BLF_SPLITHANDACTIVE) || IsFlagSet(BLF_DISABLEHAND_SPLIT)) {
                        if (!IsFlagSet(BLF_SPLITHANDACTIVE) && !IsFlagSet(BLF_DISABLEHAND_MAIN)) {
                            int32_t cardindex_2 = DealCard(pPlayerHand, 1, 0);
                            BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[cardindex_2];
                            pPlayerHand->AddTail(pCard);
                            int32_t hValue_2 = GetHandValue(pPlayerHand);
                            if (hValue_2 >= 21) {
                                SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                                if (hValue_2 > 21) {
                                    eHandResults[0] = 4;
                                }
                            }
                            SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                            BSSimpleList<BlackJackMenu::BlackJackCard*>* cl_2 = pPlayerHand;
                            int32_t cardCount_2 = 0;
                            while (cl_2 && cl_2->GetItem()) {
                                ++cardCount_2;
                                cl_2 = cl_2->GetNext();
                            }
                            if (cardCount_2 >= 7) {
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
                else {
                    if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
                        if ((!IsFlagSet(BLF_SPLITHANDACTIVE) || !IsFlagSet(BLF_CANDOUBLEDOWN_SPLIT)) &&
                            IsFlagSet(BLF_CANDOUBLEDOWN_MAIN)) {
                            iCurrentBet *= 2;
                            SetFlag(BlackJackMenu::BLF_DOUBLEDOWN_MAIN, true);
                            SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                            int32_t v27 = 0;
                            int32_t cardindex_3 = DealCard(pPlayerHand, 1, 0);
                            BlackJackMenu::BlackJackCard* pCard = &pDeckOfCards[cardindex_3];
                            pPlayerHand->AddTail(pCard);
                            if (!IsFlagSet(BLF_DISABLEHAND_MAIN) && IsFlagSet(BLF_CANDOUBLEDOWN_MAIN)) {
                                CanDoubleDown(false);
                            }
                            eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                            SetFlag(BlackJackMenu::BLF_DISABLESURRENDER, true);
                            BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                            BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet + iCurrentSplitBet);
                            int32_t hValue_3 = GetHandValue(pPlayerHand);
                            if (hValue_3 > 21) {
                                eHandResults[0] = 4;
                            }
                            SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                        }
                        SetTileAlphasForStage(10);
                    }
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
                                    int32_t v3;
                                    if (iCurrentBet >= 1500) {
                                        v3 = iCurrentBet + 500;
                                    }
                                    else {
                                        v3 = iCurrentBet + 100;
                                    }
                                    iCurrentBet = v3;
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
                        BSAudio* v4 = &BSAudio::QInstance();
                        v4->GetSoundHandleByName(
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
                else {
                    if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN && IsFlagSet(BLF_CANSPLIT)) {
                        SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                        SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                        eBlackJackState = BlackJackMenu::BL_SPLIT_ANIM;
                        SetFlag(BlackJackMenu::BLF_DISABLESURRENDER, true);
                        BSSimpleList<BlackJackMenu::BlackJackCard*>* clist = pPlayerHand->GetNext();
                        BlackJackMenu::BlackJackCard* cd = clist->GetItem();
                        int32_t v20 = 0;
                        iCurrentSplitBet = iCurrentBet;
                        if (pPlayerSplitHand && !pPlayerSplitHand->IsEmpty()) {
                            pPlayerSplitHand->AddHead(cd);
                        }
                        else {
                            pPlayerSplitHand->SetItem(cd);
                        }
                        SwapCardTextures(1, 1, cd);
                        int32_t cardindex_4 = DealCard(pPlayerHand, 0, 0);
                        BlackJackMenu::BlackJackCard* pHandCard = &pDeckOfCards[cardindex_4];
                        clist->SetItem(pHandCard);
                        int32_t cardindex_5 = DealCard(pPlayerSplitHand, 0, 0);
                        BlackJackMenu::BlackJackCard* pSplitCard = &pDeckOfCards[cardindex_5];
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
                                        int32_t v7;
                                        if (iCurrentBet % 500) {
                                            v7 = 500 * (iCurrentBet / 500);
                                        }
                                        else {
                                            v7 = iCurrentBet - 500;
                                        }
                                        iCurrentBet = v7;
                                    }
                                    else {
                                        int32_t v8;
                                        if (iCurrentBet % 100) {
                                            v8 = 100 * (iCurrentBet / 100);
                                        }
                                        else {
                                            v8 = iCurrentBet - 100;
                                        }
                                        iCurrentBet = v8;
                                    }
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
                    if (iCurrentBet < g_blackJackMinChipsToPlay) {
                        iCurrentBet = g_blackJackMinChipsToPlay;
                    }
                    if (iPreviousBet > iCurrentBet) {
                        BSAudio* v5 = &BSAudio::QInstance();
                        v5->GetSoundHandleByName(
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
                else {
                    if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN && IsFlagSet(BLF_SPLIT)) {
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
                else {
                    if (eBlackJackState == BlackJackMenu::BL_PLAYER_TURN) {
                        eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                        SetTileAlphasForStage(10);
                    }
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
    int32_t v173 = 0;
    float v176 = 1.0f;
    [[maybe_unused]] static BSSoundHandle spinningReelSound;
    Interface::enumMenus v1 = static_cast<Interface::enumMenus>(GetClass());
    bool bisMenuOnTop = Interface::IsTopMenuID(v1);
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
        BSSimpleList<BlackJackMenu::BlackJackCard*>* v89;
        BSSimpleList<BlackJackMenu::BlackJackCard*>* v90;
        BSSimpleList<BlackJackMenu::BlackJackCard*>* v91;
        BSSimpleList<BlackJackMenu::BlackJackCard*>* v109 = static_cast<BSSimpleList<BlackJackMenu::BlackJackCard*>*>(
            ourMalloc(sizeof(BSSimpleList<BlackJackMenu::BlackJackCard*>))
        );
        if (v109) {
            v91 = ::new (v109) BSSimpleList<BlackJackMenu::BlackJackCard*>;
        }
        else {
            v91 = nullptr;
        }
        pPlayerHand = v91;
        BSSimpleList<BlackJackMenu::BlackJackCard*>* v107 = static_cast<BSSimpleList<BlackJackMenu::BlackJackCard*>*>(
            ourMalloc(sizeof(BSSimpleList<BlackJackMenu::BlackJackCard*>))
        );
        if (v107) {
            v90 = ::new (v107) BSSimpleList<BlackJackMenu::BlackJackCard*>;
        }
        else {
            v90 = nullptr;
        }
        pPlayerSplitHand = v90;
        BSSimpleList<BlackJackMenu::BlackJackCard*>* v105 = static_cast<BSSimpleList<BlackJackMenu::BlackJackCard*>*>(
            ourMalloc(sizeof(BSSimpleList<BlackJackMenu::BlackJackCard*>))
        );
        if (v105) {
            v89 = ::new (v105) BSSimpleList<BlackJackMenu::BlackJackCard*>;
        }
        else {
            v89 = nullptr;
        }
        pDealerHand = v89;
        v173 = 0;
        fStateSecs = 0.0f;
        eprevBlackJackState = 0;
    }
    uiPrevIdleTick = appTimer.iLastTime;
    eprevBlackJackState = eBlackJackState;
    if (!bisMenuOnTop) {
        InterfaceManager* v2 = InterfaceManager::Instance();
        v2->pCursor->Set(4009, 255.0f, true);
    }
    switch (eBlackJackState) {
        case BlackJackMenu::BL_DEALING_ANIM: {
            int32_t i_3 = iAnimIndex[2];
            float animOffset = pPlayerAnimations[i_3]->m_fEndKeyTime / 2.0;
            static float animDelay = pPlayerAnimations[1]->m_fEndKeyTime;
            if ((pDealerAnimations[i_3]) && (pPlayerAnimations[i_3])) {
                if (fStateSecs > 1.0) {
                    pTiles[9]->Set(4003, 0);
                }
                NiUpdateData UpdateData1(fStateSecs, true, false);
                NiAVObject* v15 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
                v15->Update(UpdateData1);
                if (fStateSecs == 0.0) {
                    NiUpdateData UpdateData0(0.0f, true, false);
                    BlackJackMenu::spSceneRoot.m_pObject->Update(UpdateData0);
                    BSAudio* v16 = &BSAudio::QInstance();
                    v16->GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
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
                    NiAVObject* v19 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(1u);
                    v19->Update(UpdateData2);
                }
                if (iAnimIndex[2] == 1 && animOffset <= static_cast<double>(fStateSecs)) {
                    if ((bCleanDealerHand) && (static_cast<double>(fStateSecs) < pPlayerAnimations[1]->m_fEndKeyTime)) {
                        bCleanDealerHand = 0;
                        BSAudio* v20 = &BSAudio::QInstance();
                        v20->GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                    }

                    float v86 = fStateSecs - animOffset;
                    NiUpdateData UpdateData3(v86, true, false);
                    NiAVObject* v21 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                    v21->Update(UpdateData3);
                }
                if (iAnimIndex[0] == 2 && static_cast<double>(fStateSecs) >= pPlayerAnimations[1]->m_fEndKeyTime) {
                    if (!bCleanDealerHand) {
                        double EndKeyTime = pPlayerAnimations[1]->m_fEndKeyTime;
                        if (fStateSecs < EndKeyTime + animOffset) {
                            bCleanDealerHand = 1;
                            BSAudio* v23 = &BSAudio::QInstance();
                            v23->GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                        }
                    }
                    float v84 = fStateSecs - pPlayerAnimations[i_3]->m_fEndKeyTime;
                    NiUpdateData UpdateData2_2(v84, true, false);
                    NiAVObject* v24 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(1u);
                    v24->Update(UpdateData2_2);
                }
                if (iAnimIndex[2] == 2) {
                    double v25 = pPlayerAnimations[1]->m_fEndKeyTime;
                    if (fStateSecs >= v25 + animOffset) {
                        if (bCleanDealerHand) {
                            bCleanDealerHand = 0;
                            BSAudio* v26 = &BSAudio::QInstance();
                            v26->GetSoundHandleByName("GAMEBlackJackCardFlip", 289u).Play(false);
                        }
                        double v27 = pPlayerAnimations[1]->m_fEndKeyTime;
                        float v82 = fStateSecs - (v27 + animOffset);
                        NiUpdateData UpdateData3_2(v82, true, false);
                        NiAVObject* v28 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                        v28->Update(UpdateData3_2);
                    }
                }
                if (animDelay <= static_cast<double>(fStateSecs)) {
                    if (iAnimIndex[0] == 1) {
                        ++iAnimIndex[0];
                        pPlayerAnimManager->DeactivateSequence(pPlayerAnimations[i_3], 0.0f);
                        pPlayerAnimManager->ActivateSequence(
                            pPlayerAnimations[iAnimIndex[0]], 0, false, 1.0f, 0.0f, nullptr
                        );
                        double v29 = pDealerAnimations[1]->m_fEndKeyTime;
                        animDelay = v29 + animOffset;
                    }
                    else {
                        if (iAnimIndex[2] == 1) {
                            ++iAnimIndex[2];
                            pDealerAnimManager->DeactivateSequence(pDealerAnimations[i_3], 0.0f);
                            pDealerAnimManager->ActivateSequence(
                                pDealerAnimations[iAnimIndex[2]], 0, false, 1.0f, 0.0f, nullptr
                            );
                            double v81 = pPlayerAnimations[1]->m_fEndKeyTime;
                            animDelay = pDealerAnimations[1]->m_fEndKeyTime + v81;
                        }
                        else {
                            if (iAnimIndex[0] == 2) {
                                ++iAnimIndex[0];
                                pPlayerAnimManager->DeactivateSequence(pPlayerAnimations[i_3], 0.0f);
                                double v30 = pDealerAnimations[1]->m_fEndKeyTime;
                                double v80 = v30 + animOffset;
                                animDelay = pDealerAnimations[2]->m_fEndKeyTime + v80;
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
                                if (!IsFlagSet(32)) {
                                    BlackJackMenu::SetDealNotSwitchHands(0);
                                }
                                if (iChipCount < 2 * iCurrentBet) {
                                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, false);
                                    SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_SPLIT, false);
                                    SetFlag(BlackJackMenu::BLF_CANSPLIT, false);
                                }
                                else {
                                    if (hValue == 11) {
                                        SetFlag(BlackJackMenu::BLF_CANDOUBLEDOWN_MAIN, true);
                                    }
                                    else {
                                        CanSplitHand();
                                    }
                                }
                                SetTileAlphasForStage(10);
                                animDelay = 0.0f;
                                ++iAnimIndex[2];
                                pDealerAnimManager->DeactivateSequence(pDealerAnimations[i_3], 0.0f);
                                if (IsFlagSet(32)) {
                                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                                }
                                else {
                                    eBlackJackState = BlackJackMenu::BL_PLAYER_TURN;
                                }
                            }
                        }
                    }
                }
            }

            return;
        }
        case BlackJackMenu::BL_HIT_ANIM: {
            NiAVObject* v33;
            bool useSplit = IsFlagSet(16);
            if (useSplit) {
                if (!pPlayerSplitAnimations[iAnimIndex[1]]) {
                    return;
                }
            }
            else {
                if (!pPlayerAnimations[iAnimIndex[0]]) {
                    return;
                }
            }
            if (fStateSecs > 1.0) {
                pTiles[9]->Set(4003, 0);
            }
            NiUpdateData UpdateData1_2(fStateSecs, true, false);
            NiAVObject* v31 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
            v31->Update(UpdateData1_2);
            if (fStateSecs == 0.0) {
                BSAudio* v32 = &BSAudio::QInstance();
                v32->GetSoundHandleByName("GAMEBlackJackCardDeal", 289u).Play(false);
                BSSimpleList<BlackJackMenu::BlackJackCard*>* pPl_2 = nullptr;
                BlackJackMenu::BlackJackCard* cd = nullptr;
                int32_t counter = 0;
                int32_t ihand = 0;
                if (useSplit) {
                    ihand = 1;
                    pPlayerSplitAnimManager->ActivateSequence(
                        pPlayerSplitAnimations[iAnimIndex[1]], 0, false, 1.0f, 0.0f, nullptr
                    );
                    pPl_2 = pPlayerSplitHand;
                    while (pPl_2 && pPl_2->m_item && counter != iAnimIndex[1]) {
                        cd = pPl_2->m_item;
                        pPl_2 = pPl_2->m_pkNext;
                        ++counter;
                    }
                }
                else {
                    pPlayerAnimManager->ActivateSequence(
                        pPlayerAnimations[iAnimIndex[0]], 0, false, 1.0f, 0.0f, nullptr
                    );
                    pPl_2 = pPlayerHand;
                    while (pPl_2 && pPl_2->m_item && counter != iAnimIndex[0]) {
                        cd = pPl_2->m_item;
                        pPl_2 = pPl_2->m_pkNext;
                        ++counter;
                    }
                }
                // Walks on to the hand's last card once any card was counted.
                while (pPl_2 && pPl_2->m_item && counter) {
                    cd = pPl_2->m_item;
                    pPl_2 = pPl_2->m_pkNext;
                    ++counter;
                }
                SwapCardTextures(ihand, counter, cd);
            }
            NiUpdateData UpdateData2_3(fStateSecs, true, false);
            if (useSplit) {
                v33 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(2u);
            }
            else {
                v33 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(1u);
            }
            v33->Update(UpdateData2_3);
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
                else {
                    if (IsFlagSet(BlackJackMenu::BLF_DEALERS_TURN)) {
                        eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                    }
                    else {
                        if (IsFlagSet(8) && (IsFlagSet(1024) && IsFlagSet(2048))) {
                            eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                        }
                        else {
                            if (IsFlagSet(8) || !IsFlagSet(1024)) {
                                eBlackJackState = BlackJackMenu::BL_PLAYER_TURN;
                            }
                            else {
                                eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                            }
                        }
                    }
                }
            }
            return;
        }
        case BlackJackMenu::BL_SPLIT_ANIM: {
            if ((pPlayerSplitAnimations[1]) && (pPlayerSplitAnimations[2]) && (pPlayerAnimations[2])) {
                if (iAnimIndex[1] == 1) {
                    SetFlag(BlackJackMenu::BLF_PLAYER_SPLIT_ANIM, true);
                    SetSideHandVisibility(true);
                    pPlayerAnimManager->ActivateSequence(pPlayerAnimations[2], 0, false, 1.0f, 0.0f, nullptr);
                    NiUpdateData update0(0.0f, true, false);
                    NiUpdateData update1(0.0f, true, false);
                    NiAVObject* v34 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                    v34->Update(update1);
                    NiAVObject* v35 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(1u);
                    v35->Update(update0);
                    SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, true);
                    --iAnimIndex[0];
                    eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                }
                else {
                    if (iAnimIndex[0] == 2) {
                        SetFlag(BlackJackMenu::BLF_SPLITHANDACTIVE, false);
                        eBlackJackState = BlackJackMenu::BL_HIT_ANIM;
                    }
                    else {
                        if (iAnimIndex[1] == 2) {
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
                }
            }

            return;
        }
        case BlackJackMenu::BL_PLAYER_TURN: {
            if (eHandResults[0]) {
                if (IsFlagSet(8)) {
                    if (eHandResults[1]) {
                        eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                    }
                }
                else {
                    eBlackJackState = BlackJackMenu::BL_DEALER_FLIP_ANIM;
                }
            }
            if (IsFlagSet(1024)) {
                if (IsFlagSet(8)) {
                    if (IsFlagSet(2048)) {
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
                    BSAudio* v36 = &BSAudio::QInstance();
                    v36->GetSoundHandleByName("GAMEBlackJackCardCollect", 289u).Play(false);
                    NiUpdateData UpdateData0_2(0.0f, true, false);
                    BlackJackMenu::spSceneRoot.m_pObject->Update(UpdateData0_2);
                    pTableAnimManager->ActivateSequence(pTableAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    pPlayerAnimManager->ActivateSequence(pPlayerAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    pPlayerSplitAnimManager->ActivateSequence(pPlayerSplitAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    UpdateHandCardVisibility();
                }
                if (bCleanDealerHand || !(static_cast<double>(fStateSecs) < pPlayerAnimations[0]->m_fEndKeyTime)) {
                    if (bCleanDealerHand) {
                        double v77 = fStateSecs - fStartKeyTime;
                        if (pDealerAnimations[0]->m_fEndKeyTime > v77) {
                            float v76 = fStateSecs - fStartKeyTime;
                            NiUpdateData UpdateData1_4(v76, true, false);
                            float v75 = fStateSecs - fStartKeyTime;
                            NiUpdateData UpdateData2_5(v75, true, false);
                            NiAVObject* v40 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
                            v40->Update(UpdateData1_4);
                            NiAVObject* v41 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                            v41->Update(UpdateData2_5);
                        }
                    }
                }
                else {
                    NiUpdateData UpdateData1_3(fStateSecs, true, false);
                    NiUpdateData UpdateData2_4(fStateSecs, true, false);
                    NiUpdateData UpdateData3_3(fStateSecs, true, false);
                    NiAVObject* v37 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
                    v37->Update(UpdateData1_3);
                    NiAVObject* v38 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(1u);
                    v38->Update(UpdateData2_4);
                    NiAVObject* v39 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(2u);
                    v39->Update(UpdateData3_3);
                }
                if (!(static_cast<double>(fStateSecs) >= pPlayerAnimations[0]->m_fEndKeyTime) || bCleanDealerHand) {
                    double v73 = fStateSecs - fStartKeyTime;
                    if ((pDealerAnimations[0]->m_fEndKeyTime <= v73) && (bCleanDealerHand)) {
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
                        int32_t iMax = (&BlackJackMenu::pCasino->data)->iMaxWinnings;
                        int32_t iCurrentWinnings = g_blackJackCasinoStats->iTotalWinnings;
                        int32_t iWinningLevel = g_blackJackCasinoStats->iWinningLevel;
                        for (int32_t i_4 = 1; i_4 < 5; ++i_4) {
                            if (i_4 > iWinningLevel) {
                                if (static_cast<double>(iCurrentWinnings) >=
                                    static_cast<double>(iMax) * (static_cast<double>(i_4) * 0.25)) {
                                    g_blackJackCasinoStats->iWinningLevel = i_4;
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
                        for (int32_t i_5 = 0; i_5 < 3; ++i_5) {
                            iAnimIndex[i_5] = 1;
                        }
                    }
                }
                else {
                    pTableAnimManager->DeactivateSequence(pTableAnimations[0], 0.0f);
                    pTableAnimManager->ActivateSequence(pTableAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    pDealerAnimManager->ActivateSequence(pDealerAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    BSAudio* v42 = &BSAudio::QInstance();
                    v42->GetSoundHandleByName("GAMEBlackJackCardCollectDealer", 289u).Play(false);
                    bCleanDealerHand = 1;
                    fStartKeyTime = fStateSecs;
                    NiUpdateData UpdateData1_5(0.0f, true, false);
                    NiUpdateData UpdateData2_6(0.0f, true, false);
                    NiAVObject* v43 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
                    v43->Update(UpdateData1_5);
                    NiAVObject* v44 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                    v44->Update(UpdateData2_6);
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
                NiAVObject* v45 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
                v45->Update(UpdateData1_6);
                if (fStateSecs == 0.0) {
                    SetFlag(BlackJackMenu::BLF_DISABLEHAND_MAIN, true);
                    SetFlag(BlackJackMenu::BLF_DISABLEHAND_SPLIT, true);
                    ClearDeckHighlight();
                    BSAudio* v46 = &BSAudio::QInstance();
                    v46->GetSoundHandleByName("GAMEBlackJackCardFlip", 289u).Play(false);
                    pDealerAnimManager->ActivateSequence(pDealerReveal, 0, false, 1.0f, 0.0f, nullptr);
                }
                if (static_cast<double>(fStateSecs) < pDealerReveal->m_fEndKeyTime) {
                    NiUpdateData UpdateData2_7(fStateSecs, true, false);
                    NiAVObject* v47 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                    v47->Update(UpdateData2_7);
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
                NiAVObject* v48 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(0);
                v48->Update(UpdateData1_7);
                if (fStateSecs == 0.0) {
                    BSAudio* v49 = &BSAudio::QInstance();
                    v49->GetSoundHandleByName("GAMEBlackJackCardFlip", 289u).Play(false);
                    ClearDeckHighlight();
                    BSSimpleList<BlackJackMenu::BlackJackCard*>* pPl_3 = pDealerHand;
                    BlackJackMenu::BlackJackCard* cd_2 = nullptr;
                    int32_t counter_2 = 0;
                    while (pPl_3 && pPl_3->m_item) {
                        cd_2 = pPl_3->m_item;
                        pPl_3 = pPl_3->m_pkNext;
                        ++counter_2;
                    }
                    if (cd_2) {
                        SwapCardTextures(2, counter_2, cd_2);
                    }
                    pDealerAnimManager->ActivateSequence(
                        pDealerAnimations[iAnimIndex[2]], 0, false, 1.0f, 0.0f, nullptr
                    );
                }
                if (static_cast<double>(fStateSecs) < pDealerAnimations[iAnimIndex[2]]->m_fEndKeyTime) {
                    NiUpdateData UpdateData2_8(fStateSecs, true, false);
                    NiAVObject* v50 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(3u);
                    v50->Update(UpdateData2_8);
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
            else {
                if (GetHandValue(pDealerHand) > 21 || IsFlagSet(BlackJackMenu::BLF_PLAYER_BUST)) {
                    eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
                }
                else {
                    eBlackJackState = BlackJackMenu::BL_DEALER_TURN;
                }
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
                    msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
                );
                BSAudio* v53 = &BSAudio::QInstance();
                v53->GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
                BlackJackMenu::SetShouldExit();
            }
            return;
        }
        case BlackJackMenu::BL_DEALER_TURN: {
            SetFlag(BlackJackMenu::BLF_DEALERS_TURN, true);
            if (IsFlagSet(8)) {
                if (eHandResults[0] == 4 && eHandResults[1] == 4) {
                LABEL_233:;
                    SetFlag(BlackJackMenu::BLF_PLAYER_BUST, true);
                    eBlackJackState = BlackJackMenu::BL_DETERMINE_WINNER;
                    return;
                }
            }
            else {
                if (eHandResults[0] == 4 || eHandResults[0] == 6) {
                    goto LABEL_233;
                }
            }
            if (IsFlagSet(32)) {
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
            char Src[264];
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
            if (IsFlagSet(8)) {
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
                    BSsprintf(Src, 260, "%s", sYouBreakEvenText.String());
                    BlackJackMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                    BlackJackMenu::pMe->pTiles[9]->Set(4036, Src, true);
                    BlackJackMenu::pMe->pTiles[9]->Set(4003, 1);
                }
                else {
                    if (bBadLuck) {
                        const char* fEaseOutTimed = sPlural.String();
                        const char* timed = sCasinoChipText.String();
                        BSsprintf(Src, 260, "%s %d %s%s", sUnluckyLoseText.String(), -winnings, timed, fEaseOutTimed);
                    }
                    else {
                        const char* fEaseOutTimee = sPlural.String();
                        const char* timee = sCasinoChipText.String();
                        BSsprintf(Src, 260, "%s %d %s%s", sYouLoseText.String(), -winnings, timee, fEaseOutTimee);
                    }
                    BSAudio* v52 = &BSAudio::QInstance();
                    v52->GetSoundHandleByName("GAMEBlackJackLose", 289u).Play(false);
                    BlackJackMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                    BlackJackMenu::pMe->pTiles[9]->Set(4036, Src, true);
                    BlackJackMenu::pMe->pTiles[9]->Set(4003, 1);
                }
            }
            else {
                if (bGoodLuck) {
                    const char* fEaseOutTimeb = sPlural.String();
                    const char* timeb = sCasinoChipText.String();
                    BSsprintf(Src, 260, "%s %d %s%s", sLuckyWinText.String(), winnings, timeb, fEaseOutTimeb);
                }
                else {
                    const char* fEaseOutTimec = sPlural.String();
                    const char* timec = sCasinoChipText.String();
                    BSsprintf(Src, 260, "%s %d %s%s", sYouWinText.String(), winnings, timec, fEaseOutTimec);
                }
                BSAudio* v51 = &BSAudio::QInstance();
                v51->GetSoundHandleByName("GAMEBlackJackWin", 289u).Play(false);
                BlackJackMenu::pMe->pTiles[9]->Poke(4036, 1.0f);
                BlackJackMenu::pMe->pTiles[9]->Set(4036, Src, true);
                BlackJackMenu::pMe->pTiles[9]->Set(4003, 1);
            }
            pPlayerHand->RemoveAll();
            pDealerHand->RemoveAll();
            if (IsFlagSet(8)) {
                pPlayerSplitHand->RemoveAll();
                BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, iCurrentBet);
                iCurrentSplitBet = 0;
            }
            if (IsFlagSet(128)) {
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
                InterfaceManager* v3 = InterfaceManager::Instance();
                v3->pCursor->Set(4009, 255.0f, true);
                pMenu->Set(6002, 1);
                StartFadeOut();
                TESCasinoChips* ChipType = BlackJackMenu::pCasino->GetChipType();
                int32_t i = iChipCount - pPlayer->GetInventoryObjectCount(ChipType);
                if (i >= 0) {
                    if (i > 0) {
                        TESContainer tempcont;
                        TESCasinoChips* v10 = BlackJackMenu::pCasino->GetChipType();
                        tempcont.AddObject(v10, i, nullptr);
                        if (g_blackJackCasinoStats->iTotalWinnings >= BlackJackMenu::pCasino->GetMaxWinnings()) {
                            tempcont.AddContainerToReference(pPlayer, true);
                            BSStringT<char> pstring_2;
                            const char* fEaseOutTimea = sGamblingBannedText.String();
                            const char* timea = sAddItemtoInventory.String();
                            const char* a4 = sPlural.String();
                            TESCasinoChips* v11 = BlackJackMenu::pCasino->GetChipType();
                            const char* FullName = TESFullName::GetFullName(v11);
                            pstring_2.SPrintF("%i %s%s %s\n%s", i, FullName, a4, timea, fEaseOutTimea);
                            TESCasinoChips* v13 = BlackJackMenu::pCasino->GetChipType();
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
                    TESCasinoChips* v5 = BlackJackMenu::pCasino->GetChipType();
                    pPlayer->RemoveItem(v5, nullptr, -i, false, false, nullptr, nullptr, nullptr, true, false);
                    BSStringT<char> pstring;
                    const char* fEaseOutTime = sRemoveItemfromInventory.String();
                    const char* time = sPlural.String();
                    TESCasinoChips* v6 = BlackJackMenu::pCasino->GetChipType();
                    const char* v7 = TESFullName::GetFullName(v6);
                    pstring.SPrintF("%i %s%s %s", -i, v7, time, fEaseOutTime);
                    TESCasinoChips* v8 = BlackJackMenu::pCasino->GetChipType();
                    const char* v9 = pPlayer->GetPickUpSoundName(v8, true, false);
                    Interface::ShowMessage(
                        pstring.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", v9, 2.0f,
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
    return (((int32_t)(aiFlag & iFlags)) != 0);
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
    for (int32_t n = 0; n < 3; ++n) {
        iAnimIndex[n] = 1;
    }
    bRunScriptOnExit = false;
    pCard->~BlackJackCard();
    return true;
}

void BlackJackMenu::SetFlag(enumFlags aeFlag, bool abValue) {
    BlackJackMenu* result;
    int32_t v4;
    int32_t v5;
    if (abValue) {
        result = this;
        iFlags |= ((int32_t)aeFlag);
        v4 = (int32_t)iFlags;
    }
    else {
        iFlags &= (~((int32_t)aeFlag));
        result = this;
        v5 = (int32_t)iFlags;
    }
    int32_t v6 = (int32_t)iFlags;
}

// The chip compares are signed; isSplitHand is tested again before each step, as the game does.
bool BlackJackMenu::CanDoubleDown(bool isSplitHand) {
    bool bCanDoubleDown = true;
    if (isSplitHand && iChipCount < static_cast<int32_t>(iCurrentBet + iCurrentSplitBet * 2)) {
        bCanDoubleDown = false;
    }
    if (!isSplitHand && iChipCount < static_cast<int32_t>(iCurrentSplitBet + iCurrentBet * 2)) {
        bCanDoubleDown = false;
    }
    // Sets BLF_DOUBLEDOWN_SPLIT for the split hand rather than BLF_CANDOUBLEDOWN_SPLIT, which DoClick reads and clears;
    // this mismatch is deliberate, as the game does.
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

// KEEP: luckPool holds 5 entries ([ebp-0x50]), but the exe fills abs(iLuckSkill - 5) of them (0x739193-0x7391c8)
// with no bound: a luck outside 0..10 writes on into its own frame (bestLowIndex at [ebp-0x3C], the count at
// [ebp-0x38], ...), which changes how long the loop runs. src cannot express those writes into its own locals.
uint32_t BlackJackMenu::DealCard(BSSimpleList<BlackJackMenu::BlackJackCard*>* pHand, char a2, char a3) {
    BlackJackMenu::BlackJackCard tCard;
    BlackJackMenu::BlackJackCard card;
    int32_t cardindex = 0;
    if (this->iCardsRemaining < 5) {
        Shuffle();
    }
    if (pHand && (a2 && ((!a3 || iLuckSkill <= 4) && (a3 || iLuckSkill >= 6)))) {
        int32_t luckPool[5];
        int32_t pointTotal = GetHandValue(pHand);
        int32_t bestHighIndex = 0;
        int32_t bestLowIndex = 0;
        int32_t pointTotal_2 = pointTotal;
        int32_t pointTotal_3 = pointTotal;
        int32_t num_lc = std::abs(iLuckSkill - 5);
        for (int32_t i = 0; i < num_lc; ++i) {
            luckPool[i] = RandomInt(0, this->iCardsRemaining);
        }
        for (int32_t i_2 = 0; i_2 < num_lc; ++i_2) {
            int32_t tCardValue = GetNumericCardValue(pDeckOfCards[luckPool[i_2]].uiValue);
            if (tCardValue == 11) {
                tCardValue = pointTotal + 11 <= 21 ? 11 : 1;
            }
            if (pointTotal <= 9) {
                int32_t v8 = pointTotal + tCardValue;
                if (21 - (pointTotal + tCardValue) < pointTotal_2 && v8 < 22) {
                    pointTotal_2 = 21 - v8;
                    bestHighIndex = i_2;
                }
                if (11 - v8 < pointTotal_3 && v8 < 12) {
                    pointTotal_3 = 11 - v8;
                    bestLowIndex = i_2;
                }
            }
            else {
                if (pointTotal + tCardValue > pointTotal_2 && pointTotal + tCardValue < 22) {
                    pointTotal_2 = pointTotal + tCardValue;
                    bestHighIndex = i_2;
                }
            }
            if (pointTotal_2 >= 4) {
                if (pointTotal_3 >= pointTotal_2 || pointTotal_3 >= 2) {
                    cardindex = luckPool[bestHighIndex];
                }
                else {
                    cardindex = luckPool[bestLowIndex];
                    if (bestLowIndex) {
                        if (a3) {
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
                    if (a3) {
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
        int32_t aiMax = this->iCardsRemaining;
        this->iCardsRemaining = aiMax - 1;
        cardindex = RandomInt(0, aiMax);
    }
    card.operator=(pDeckOfCards[cardindex]);
    tCard.operator=(pDeckOfCards[this->iCardsRemaining]);
    pDeckOfCards[this->iCardsRemaining].operator=(card);
    pDeckOfCards[cardindex].operator=(tCard);
    uint32_t iCardsRemaining = this->iCardsRemaining;
    this->iCardsRemaining = iCardsRemaining - 1;
    return iCardsRemaining;
}

int32_t BlackJackMenu::DealHand() {
    eHandResults[0] = 0;
    eHandResults[1] = 0;
    uint32_t v9 = DealCard(nullptr, 0, 0);
    if (pDealerHand && !pDealerHand->IsEmpty()) {
        BlackJackMenu::BlackJackCard* a2 = &pDeckOfCards[v9];
        pDealerHand->AddHead(a2);
    }
    else {
        BlackJackMenu::BlackJackCard* v8 = &pDeckOfCards[v9];
        pDealerHand->SetItem(v8);
    }
    v9 = DealCard(nullptr, 0, 0);
    if (pPlayerHand && !pPlayerHand->IsEmpty()) {
        BlackJackMenu::BlackJackCard* item = &pDeckOfCards[v9];
        pDealerHand->AddHead(item);
    }
    else {
        BlackJackMenu::BlackJackCard* v6 = &pDeckOfCards[v9];
        pPlayerHand->SetItem(v6);
    }
    v9 = DealCard(pDealerHand, 1, 1);
    BlackJackMenu::BlackJackCard* apItem_4 = &pDeckOfCards[v9];
    pDealerHand->AddTail(apItem_4);
    v9 = DealCard(pPlayerHand, 1, 0);
    BlackJackMenu::BlackJackCard* apItem = &pDeckOfCards[v9];
    pPlayerHand->AddTail(apItem);
    return GetHandValue(pPlayerHand);
}

void BlackJackMenu::SetTileCasinoEarnings() {
    char Src[264];
    int32_t iTotalWinnings = g_blackJackCasinoStats->iTotalWinnings;
    const char* v1 = sCasinoEarningsText.String();
    BSsprintf(Src, 260u, "%s %s%i", BlackJackMenu::pCasino->GetFullName(), v1, iTotalWinnings);
    int32_t v7 = BSstrlen(Src);
    if (v7 < 23) {
        const char* v2 = sCasinoEarningsText.String();
        BSsprintf(Src, 260u, "%s %s", BlackJackMenu::pCasino->GetFullName(), v2);
        for (int32_t i = 0; i < 23 - v7; ++i) {
            Src[v7 - 1 + i] = 32;
            Src[i + v7] = 0;
        }
        BSsprintf(Src, 260u, "%s%i", Src, g_blackJackCasinoStats->iTotalWinnings);
    }
    pTiles[2]->Set(g_blackJackMenuTrait_Value, Src, true);
}

void BlackJackMenu::LoadNifs() {
    NiProperty* v9;
    int32_t v11;
    NiNode* v45;
    pHandGeometry = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Hand1.NIF", LOD_MULT_NONE, true, false, false, false
    );
    pHandSplitGeometry = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Hand2.NIF", LOD_MULT_NONE, true, false, false, false
    );
    pHandDealerGeometry = (NiAVObject*)pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_BlackJack\\NV_Blackjack-Dealer.NIF", LOD_MULT_NONE, true, false, false, false
    );
    NiAVObject* pTableGeometry = this->pTableGeometry;
    BlackJackMenu::spCamera.operator=(
        static_cast<NiCamera*>(BSUtilities::GetObjectByName(this->pTableGeometry, "object1"))
    );
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        v45 = ::new (apMemory) NiNode(0);
    }
    else {
        v45 = nullptr;
    }
    NiRefObject* v75 = (NiRefObject*)v45;
    BlackJackMenu::spSceneRoot.operator=(v45);
    NiNode* v44 = BlackJackMenu::spSceneRoot.m_pObject;
    v44->AttachChild(this->pTableGeometry, 1);
    NiNode* v43 = BlackJackMenu::spSceneRoot.m_pObject;
    v43->AttachChild(pHandGeometry, 1);
    NiNode* v42 = BlackJackMenu::spSceneRoot.m_pObject;
    v42->AttachChild(pHandSplitGeometry, 1);
    NiNode* v41 = BlackJackMenu::spSceneRoot.m_pObject;
    v41->AttachChild(pHandDealerGeometry, 1);
    for (int32_t i = 0; i < 6; ++i) {
        NiNode* v40 = BlackJackMenu::spSceneRoot.m_pObject;
        v40->AttachChild(pBetChipGeometry[i], 1);
        NiNode* v39 = BlackJackMenu::spSceneRoot.m_pObject;
        v39->AttachChild(pBetShadowGeometry[i], 1);
    }
    SetCorrectChipMeshesVisibleForCurrentBet();
    SetSideHandVisibility(false);
    ClearDeckHighlight();
    NiUpdateData a2(0.0f, false, false);
    BlackJackMenu::spSceneRoot.m_pObject->Update(a2);
    NiControllerManager* Controller =
        static_cast<NiControllerManager*>(this->pTableGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (Controller) {
        pTableAnimManager = Controller;
        pTableAnimations[0] = pTableAnimManager->GetSequenceByName("Players_Discard");
        pTableAnimations[1] = pTableAnimManager->GetSequenceByName("Dealers_Discard");
        pTableAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(this->pTableGeometry);
    }
    NiControllerManager* v81 =
        static_cast<NiControllerManager*>(pHandGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (v81) {
        pPlayerAnimManager = v81;
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
    NiControllerManager* v82 =
        static_cast<NiControllerManager*>(pHandSplitGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (v82) {
        pPlayerSplitAnimManager = v82;
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
    NiControllerManager* v84 =
        static_cast<NiControllerManager*>(pHandDealerGeometry->GetController(&NiControllerManager::ms_RTTI));
    if (v84) {
        pDealerAnimManager = v84;
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
    BlackJackMenu::spSceneRoot.m_pObject->Update(arData);
    if (!BlackJackMenu::spCamera.m_pObject) {
        BlackJackMenu::spCamera.operator=(SetupCamera());
    }
    NiNode* ObjectByName = static_cast<NiNode*>(BSUtilities::GetObjectByName(this->pTableGeometry, "Deck:0"));
    // Shader property types 8 to 12 are the per-pixel lighting properties; the type check fetches the property again
    // for each test.
    if (ObjectByName->GetProperty(NiProperty::SHADE)) {
        NiProperty* Property = ObjectByName->GetProperty(NiProperty::SHADE);
        int32_t v10 = false;
        if (static_cast<NiShadeProperty*>(Property)->GetShaderPropertyType() >= 8) {
            NiProperty* v2 = ObjectByName->GetProperty(NiProperty::SHADE);
            if (static_cast<NiShadeProperty*>(v2)->GetShaderPropertyType() <= 12) {
                v10 = true;
            }
        }
        v11 = v10;
    }
    else {
        v11 = 0;
    }
    if (v11) {
        v9 = ObjectByName->GetProperty(NiProperty::SHADE);
    }
    else {
        v9 = nullptr;
    }
    if (v9) {
        NiSourceTexture* pBackTexture = pDeckOfCards[0].spBackTexture.m_pObject;
        static_cast<BSShaderPPLightingProperty*>(v9)->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pBackTexture);
    }
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    ShadowSceneNode_->AttachChild(BlackJackMenu::spSceneRoot.m_pObject, 1);
    const NiBound* WorldBound = &BlackJackMenu::spSceneRoot.m_pObject->GetWorldBound();
    float v79 = WorldBound->m_fRadius * 3.0;
    NiNode* afTime = BlackJackMenu::spSceneRoot.m_pObject;
    ShadowSceneNode* v5 = BSShaderManager::GetShadowSceneNode(1);
    v5->AddAllLightsFromNode_Recurse(afTime, v79);
}

void BlackJackMenu::UpdateHandCardVisibility() {
    char apDest[32];
    NiAVObject* ObjectByName;
    char blockName[32];
    char apName[32];
    for (int32_t i = 1; i <= 7; ++i) {
        BSsprintf(blockName, 30, "Hand1_0%i:%i", i, i - 1);
        BSsprintf(apDest, 30, "Hand1_0%i_Back:0", i);
        BSsprintf(apName, 30, "Hand1_0%i_Shadow:0", i);
        ObjectByName = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(blockName));
        if (i < iAnimIndex[0]) {
            ObjectByName->SetAppCulled(false);
            ObjectByName = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(apDest));
            ObjectByName->SetAppCulled(false);
            ObjectByName = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(apName));
            ObjectByName->SetAppCulled(false);
        }
        else {
            ObjectByName->SetAppCulled(true);
            ObjectByName = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(apDest));
            ObjectByName->SetAppCulled(true);
            ObjectByName = BSUtilities::GetObjectByName(pHandGeometry, static_cast<const char*>(apName));
            ObjectByName->SetAppCulled(true);
        }
    }
    for (int32_t j = 1; j <= 7; ++j) {
        BSsprintf(blockName, 30, "Dealer_0%i:0", j);
        BSsprintf(apDest, 30, "Dealer_0%i_Back:0", j);
        BSsprintf(apName, 30, "Dealer_0%i_Shadow:0", j);
        ObjectByName = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(blockName));
        if (j < iAnimIndex[2]) {
            ObjectByName->SetAppCulled(false);
            ObjectByName = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(apDest));
            ObjectByName->SetAppCulled(false);
            ObjectByName = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(apName));
            ObjectByName->SetAppCulled(false);
        }
        else {
            ObjectByName->SetAppCulled(true);
            ObjectByName = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(apDest));
            ObjectByName->SetAppCulled(true);
            ObjectByName = BSUtilities::GetObjectByName(pHandDealerGeometry, static_cast<const char*>(apName));
            ObjectByName->SetAppCulled(true);
        }
    }
    for (int32_t k = 1; k <= 7; ++k) {
        BSsprintf(blockName, 30, "Hand2_0%i:%i", k, k - 1);
        BSsprintf(apDest, 30, "Hand2_0%i_Back:0", k);
        BSsprintf(apName, 30, "Hand2_0%i_Shadow:0", k);
        ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(blockName));
        if (k < iAnimIndex[1]) {
            ObjectByName->SetAppCulled(false);
            ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(apDest));
            ObjectByName->SetAppCulled(false);
            ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(apName));
            ObjectByName->SetAppCulled(false);
        }
        else {
            ObjectByName->SetAppCulled(true);
            ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(apDest));
            ObjectByName->SetAppCulled(true);
            ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, static_cast<const char*>(apName));
            ObjectByName->SetAppCulled(true);
        }
    }
    NiUpdateData apMemory(0.0f, 0, 0);
    BlackJackMenu::spSceneRoot.m_pObject->Update(apMemory);
}

void BlackJackMenu::ShowAllHandCards() {
    char apDest[32];
    NiAVObject* ObjectByName;
    char blockName[32];
    char apName[32];
    for (int32_t i = 1; i <= 7; ++i) {
        BSsprintf(blockName, 30u, "Hand1_0%i:%i", i, i - 1);
        BSsprintf(apDest, 30u, "Hand1_0%i_Back:0", i);
        BSsprintf(apName, 30u, "Hand1_0%i_Shadow:0", i);
        ObjectByName = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandGeometry), blockName);
        ObjectByName->SetAppCulled(false);
        ObjectByName = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandGeometry), apDest);
        ObjectByName->SetAppCulled(false);
        ObjectByName = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandGeometry), apName);
        ObjectByName->SetAppCulled(false);
    }
    for (int32_t j = 1; j <= 7; ++j) {
        BSsprintf(blockName, 30u, "Dealer_0%i:0", j);
        BSsprintf(apDest, 30u, "Dealer_0%i_Back:0", j);
        BSsprintf(apName, 30u, "Dealer_0%i_Shadow:0", j);
        ObjectByName = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandDealerGeometry), blockName);
        ObjectByName->SetAppCulled(false);
        ObjectByName = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandDealerGeometry), apDest);
        ObjectByName->SetAppCulled(false);
        ObjectByName = BSUtilities::GetObjectByName(static_cast<NiAVObject*>(pHandDealerGeometry), apName);
        ObjectByName->SetAppCulled(false);
    }
    for (int32_t k = 1; k <= 7; ++k) {
        BSsprintf(blockName, 30u, "Hand2_0%i:%i", k, k - 1);
        BSsprintf(apDest, 30u, "Hand2_0%i_Back:0", k);
        BSsprintf(apName, 30u, "Hand2_0%i_Shadow:0", k);
        ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, blockName);
        ObjectByName->SetAppCulled(false);
        ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, apDest);
        ObjectByName->SetAppCulled(false);
        ObjectByName = BSUtilities::GetObjectByName(pHandSplitGeometry, apName);
        ObjectByName->SetAppCulled(false);
    }
    NiUpdateData apMemory(0.0f, 0, 0);
    NiAVObject* v1 = BlackJackMenu::spSceneRoot.m_pObject;
    v1->Update(apMemory);
}

void* BlackJackMenu::SetTileAlphasForStage(int32_t a1) {
    void* result = this;
    if (eBlackJackState != 10) {
        switch (a1) {
            case 3: {
                if (IsFlagSet(16)) {
                    if (IsFlagSet(2048)) {
                        return BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                    }
                }
                else {
                    if (IsFlagSet(1024)) {
                        return BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                    }
                }
                return BlackJackMenu::SetNthTileAlpha(3, 255.0f);
            }
            case 4: {
                if (IsFlagSet(16)) {
                    if (IsFlagSet(256)) {
                        return BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                    }
                }
                else {
                    if (IsFlagSet(64)) {
                        return BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                    }
                }
                return BlackJackMenu::SetNthTileAlpha(4, 128.0f);
            }
            case 5: {
                if (IsFlagSet(16) || !IsFlagSet(4)) {
                    return BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                }

                return BlackJackMenu::SetNthTileAlpha(5, 255.0f);
            }
            case 6: {
                if (IsFlagSet(16)) {
                    if (IsFlagSet(1024)) {
                        return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    }
                    goto LABEL_31;
                }
                if (!IsFlagSet(2048) && IsFlagSet(8)) {
                    goto LABEL_31;
                }
                return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
            }
            case 7: {
                if (IsFlagSet(1)) {
                    return BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                }

                return BlackJackMenu::SetNthTileAlpha(7, 255.0f);
            }
            case 8: {
                return BlackJackMenu::SetNthTileAlpha(8, 255.0f);
            }
            case 10: {
                if (IsFlagSet(16)) {
                    if (IsFlagSet(2048)) {
                        BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                    }
                    else {
                        BlackJackMenu::SetNthTileAlpha(3, 255.0f);
                        if (IsFlagSet(256)) {
                            BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                        }
                    }
                    BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                    BlackJackMenu::SetNthTileAlpha(8, 255.0f);
                    if (IsFlagSet(1024)) {
                        return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    }
                }
                else {
                    if (IsFlagSet(1024) || IsFlagSet(32)) {
                        BlackJackMenu::SetNthTileAlpha(3, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                        BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                    }
                    else {
                        BlackJackMenu::SetNthTileAlpha(3, 255.0f);
                        if (IsFlagSet(64)) {
                            BlackJackMenu::SetNthTileAlpha(4, 255.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(4, 128.0f);
                        }
                        if (IsFlagSet(4)) {
                            BlackJackMenu::SetNthTileAlpha(5, 255.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(5, 128.0f);
                        }
                        if (IsFlagSet(1)) {
                            BlackJackMenu::SetNthTileAlpha(7, 128.0f);
                        }
                        else {
                            BlackJackMenu::SetNthTileAlpha(7, 255.0f);
                        }
                    }
                    if (IsFlagSet(32) && (!IsFlagSet(8) || IsFlagSet(2048))) {
                        BlackJackMenu::SetNthTileAlpha(8, 128.0f);
                    }
                    else {
                        BlackJackMenu::SetNthTileAlpha(8, 255.0f);
                    }
                    if (IsFlagSet(2048) || !IsFlagSet(8)) {
                        return BlackJackMenu::SetNthTileAlpha(6, 128.0f);
                    }
                }
            LABEL_31:;
                result = BlackJackMenu::SetNthTileAlpha(6, 255.0f);
                break;
            }
            default: {
                return result;
            }
        }
    }
    return result;
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
    if (IsFlagSet(16)) {
        NiAVObject* ObjectByName = BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0");
        ObjectByName->SetAppCulled(true);
        NiAVObject* v6 = BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0@#2");
        v6->SetAppCulled(false);
    }
    else {
        NiAVObject* v2 = BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0");
        v2->SetAppCulled(false);
        NiAVObject* v7 = BSUtilities::GetObjectByName(pTableGeometry, "Hand1_Arrow:0@#2");
        v7->SetAppCulled(true);
    }
    NiUpdateData a2(0.0, false, false);
    NiNode* v3 = BlackJackMenu::spSceneRoot.m_pObject;
    NiAVObject* v4 = v3->GetAt(1u);
    v4->Update(a2);
}

void BlackJackMenu::SetCorrectChipMeshesVisibleForCurrentBet() {
    if (iCurrentBet >= 7) {
        if (iCurrentBet >= 40) {
            if (iCurrentBet > 100) {
                if (iCurrentBet > 300) {
                    if (iCurrentBet > 500) {
                        if (iCurrentBet >= 1000) {
                            if (iCurrentBet >= 1500) {
                                int32_t v2;
                                if (iCurrentBet / 500 - 3 <= 10) {
                                    v2 = iCurrentBet / 500 - 3;
                                }
                                else {
                                    v2 = 10;
                                }
                                pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, 1.8f);
                                pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, 1.8f);
                                float v1 = static_cast<double>(v2) * 0.18000000000000002;
                                pBetChipGeometry[5]->SetTranslate(0.0f, 0.0f, v1);
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
                                float v3 = static_cast<double>(iCurrentBet / 100 - 9) * 0.18000000000000002;
                                pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, v3);
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
                            float v5 = static_cast<double>(iCurrentBet % 100 / 25 + 6) * 0.18000000000000002;
                            pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, v5);
                            float v4 = static_cast<double>(iCurrentBet / 100 - 3) * 0.18000000000000002;
                            pBetChipGeometry[4]->SetTranslate(0.0f, 0.0f, v4);
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
                        float v6 = static_cast<double>(iCurrentBet / 25 - 12) * 0.18000000000000002;
                        pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, v6);
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
                    float v8 = static_cast<double>(iCurrentBet % 25 / 10 + 5) * 0.18000000000000002;
                    pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, v8);
                    float v7 = static_cast<double>(iCurrentBet / 25 - 4) * 0.18000000000000002;
                    pBetChipGeometry[3]->SetTranslate(0.0f, 0.0f, v7);
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
                float v9 = static_cast<double>(iCurrentBet / 10 - 3) * 0.18000000000000002;
                pBetChipGeometry[2]->SetTranslate(0.0f, 0.0f, v9);
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
            float v11 = static_cast<double>(iCurrentBet % 5 + 5) * 0.18000000000000002;
            pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, v11);
            float v10 = static_cast<double>(iCurrentBet / 5 - 1) * 0.18000000000000002;
            pBetChipGeometry[1]->SetTranslate(0.0f, 0.0f, v10);
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
        float v12 = static_cast<double>(iCurrentBet) * 0.18000000000000002;
        pBetChipGeometry[0]->SetTranslate(0.0f, 0.0f, v12);
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
    BlackJackMenu::spSceneRoot.m_pObject->Update(arData);
}

NiCamera* BlackJackMenu::SetupCamera() {
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
    float v17 = 0.75f;
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

void BlackJackMenu::SwapCardTextures(int32_t aiHandIndex, int32_t aiCardIndex, BlackJackMenu::BlackJackCard* cd) {
    uint32_t a1 = aiHandIndex + 1;
    if (aiCardIndex <= 7 && (aiCardIndex >= 1 && cd)) {
        BSShaderPPLightingProperty* pFaceShader_3;
        int32_t v14;
        BSShaderPPLightingProperty* pFaceShader;
        int32_t v17;
        char apDest[32];
        char blockName[32];
        switch (a1) {
            case 1u: {
                BSsprintf(blockName, 30, "Hand1_0%i:%i", aiCardIndex, aiCardIndex - 1);
                BSsprintf(apDest, 30, "Hand1_0%i_Back:0", aiCardIndex);
                break;
            }
            case 2u: {
                BSsprintf(blockName, 30, "Hand2_0%i:%i", aiCardIndex, aiCardIndex - 1);
                BSsprintf(apDest, 30, "Hand2_0%i_Back:0", aiCardIndex);
                break;
            }
            case 3u: {
                BSsprintf(blockName, 30, "Dealer_0%i:0", aiCardIndex);
                BSsprintf(apDest, 30, "Dealer_0%i_Back:0", aiCardIndex);
                break;
            }
            default: {
                return;
            }
        }
        NiAVObject* v4 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(a1);
        NiAVObject* pavobj = BSUtilities::GetObjectByName(v4, static_cast<const char*>(blockName));
        // The shade property's type tells a BSShaderPPLightingProperty (8 to 12).
        if (pavobj->GetProperty(3)) {
            int32_t v16 = false;
            if ((static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() >= 8) &&
                (static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() <= 12)) {
                v16 = true;
            }

            v17 = v16;
        }
        else {
            v17 = 0;
        }
        if (v17) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pavobj->GetProperty(3));
        }
        else {
            pFaceShader = nullptr;
        }
        BSShaderPPLightingProperty* pFaceShader_2 = pFaceShader;
        if (pFaceShader) {
            NiSourceTexture* v10 = cd->spFrontTexture.m_pObject;
            pFaceShader_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v10);
        }
        NiAVObject* v7 = BlackJackMenu::spSceneRoot.m_pObject->GetAt(a1);
        pavobj = BSUtilities::GetObjectByName(v7, static_cast<const char*>(apDest));
        if (pavobj->GetProperty(3)) {
            int32_t v13 = false;
            if ((static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() >= 8) &&
                (static_cast<NiShadeProperty*>(pavobj->GetProperty(3))->GetShaderPropertyType() <= 12)) {
                v13 = true;
            }

            v14 = v13;
        }
        else {
            v14 = 0;
        }
        if (v14) {
            pFaceShader_3 = static_cast<BSShaderPPLightingProperty*>(pavobj->GetProperty(3));
        }
        else {
            pFaceShader_3 = nullptr;
        }
        pFaceShader_2 = pFaceShader_3;
        if (pFaceShader_3) {
            NiSourceTexture* v11 = cd->spBackTexture.m_pObject;
            pFaceShader_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v11);
        }
    }
}

void BlackJackMenu::DrawScene() {
    NiCamera* v6;
    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
    Renderer_->ClearBuffer(nullptr, 4u);
    SceneGraph* pSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCullingProcess = pSceneGraph->pCuller;
    pCullingProcess->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    void* pCamera = BlackJackMenu::spCamera.m_pObject;
    NiCamera::HandleEyeFinityFOVScale(static_cast<NiCamera*>(pCamera), 0.15f);
    InterfaceManager* v1 = InterfaceManager::Instance();
    NiPointer<NiRefObject> apMemory(static_cast<NiRefObject*>(v1->Get3dAccumulator()));
    NiAccumulator* afScale = static_cast<NiAccumulator*>(apMemory.m_pObject);
    pCullingProcess->SetAccumulator(afScale);
    if (pCamera) {
        v6 = static_cast<NiCamera*>(pCamera);
    }
    else {
        v6 = pSceneGraph->GetCamera();
    }
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
    BSShaderUtil::AccumulateScene(v6, ShadowSceneNode_, *pCullingProcess);
    BSShaderAccumulator* v3 = static_cast<BSShaderAccumulator*>(apMemory.m_pObject);
    BSShaderUtil::RenderScene(v6, v3, false);
    pCullingProcess->SetAccumulator(nullptr);
    apMemory.operator=(static_cast<NiRefObject*>(nullptr));
    pCullingProcess->PopCullMode();
}

void* BlackJackMenu::SetNthTileAlpha(int32_t aiIndex, float afAlpha) {
    NiTListItem<Tile*>* pPos = BlackJackMenu::pMe->pTiles[aiIndex]->xChildren.m_pkHead;
    // Returns whatever the last step left: pMe, the head slot or the last position, as the game does.
    void* result = BlackJackMenu::pMe;
    NiTPointerList<Tile*>* pChildren = &BlackJackMenu::pMe->pTiles[aiIndex]->xChildren;
    while (pPos) {
        if (!pChildren) {
            break;
        }
        Tile** ppHead = &pChildren->GetHead();
        result = ppHead;
        if (!*ppHead) {
            break;
        }
        pChildren->Get(pPos)->Set(4009, afAlpha, true);
        pPos = pChildren->GetNextPos(pPos);
        result = pPos;
    }
    return result;
}

void BlackJackMenu::SetShouldExit() {
    InterfaceManager* singleton;
    Tile* processLevel;
    if (BlackJackMenu::pMe) {
        BlackJackMenu::pMe->eBlackJackState = BL_CLOSING_MENU;
        singleton = InterfaceManager::Instance();
        processLevel = static_cast<Tile*>(singleton->GetCursorTile());
        processLevel->Set(4009, 255.0f, true);
    }
}

void BlackJackMenu::SetDealNotSwitchHands(char a1) {
    const char* v1;
    const char* v2;
    const char* v3;
    const char* v4;
    const char* v5;
    const char* v6;
    const char* v7;
    const char* v8;
    const char* v9;
    const char* v10;
    BlackJackMenu::pMe->pTiles[4]->Poke(4036, 1.0);
    if (a1) {
        v6 = (&sDealText)->String();
        BlackJackMenu::pMe->pTiles[4]->Set(4036, v6, true);
        BlackJackMenu::SetNthTileAlpha(4, 255.0f);
        BlackJackMenu::pMe->pTiles[5]->Poke(4036, 1.0);
        v7 = (&sIncreaseBetText)->String();
        BlackJackMenu::pMe->pTiles[5]->Set(4036, v7, true);
        BlackJackMenu::SetNthTileAlpha(5, 255.0f);
        BlackJackMenu::pMe->pTiles[6]->Poke(4036, 1.0);
        v8 = (&sDecreaseBetText)->String();
        BlackJackMenu::pMe->pTiles[6]->Set(4036, v8, true);
        BlackJackMenu::SetNthTileAlpha(6, 255.0f);
        BlackJackMenu::pMe->pTiles[7]->Poke(4036, 1.0);
        v9 = (&gs_sBetMax)->String();
        BlackJackMenu::pMe->pTiles[7]->Set(4036, v9, true);
        BlackJackMenu::SetNthTileAlpha(7, 255.0f);
        BlackJackMenu::pMe->pTiles[8]->Poke(4036, 1.0);
        v10 = (&sExit)->String();
        BlackJackMenu::pMe->pTiles[8]->Set(4036, v10, true);
        BlackJackMenu::SetNthTileAlpha(8, 255.0f);
        BlackJackMenu::pMe->pTiles[3]->Set(4003, 0);
    }
    else {
        v1 = (&sDoubleDownText)->String();
        BlackJackMenu::pMe->pTiles[4]->Set(4036, v1, true);
        BlackJackMenu::pMe->pTiles[5]->Poke(4036, 1.0);
        v2 = (&sSplitText)->String();
        BlackJackMenu::pMe->pTiles[5]->Set(4036, v2, true);
        BlackJackMenu::pMe->pTiles[6]->Poke(4036, 1.0);
        v3 = (&sSwitchHandsText)->String();
        BlackJackMenu::pMe->pTiles[6]->Set(4036, v3, true);
        BlackJackMenu::pMe->pTiles[7]->Poke(4036, 1.0);
        v4 = (&sSurrenderText)->String();
        BlackJackMenu::pMe->pTiles[7]->Set(4036, v4, true);
        BlackJackMenu::pMe->pTiles[8]->Poke(4036, 1.0);
        v5 = (&sStayText)->String();
        BlackJackMenu::pMe->pTiles[8]->Set(4036, v5, true);
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
        int32_t CastingType = BlackJackMenu::pCasino->iFormID;
        if (CastingType == cl->m_item->casinoID) {
            g_blackJackCasinoStats = cl->m_item;
            break;
        }
        if (!cl->m_pkNext) {
            break;
        }
    }
    if (!g_blackJackCasinoStats) {
        CasinoData* cd;
        void* apMemory = ourMalloc(sizeof(CasinoData));
        if (apMemory) {
            cd = ::new (apMemory) CasinoData;
        }
        else {
            cd = nullptr;
        }
        CasinoData* cd_2 = cd;
        cd->casinoID = BlackJackMenu::pCasino->iFormID;
        cd_2->iTotalWinnings = 0;
        cl = pPlayer->GetCasinoDataList();
        if (cl && !cl->IsEmpty()) {
            cl->AddHead(cd_2);
        }
        else {
            cl->SetItem(cd_2);
        }
        g_blackJackCasinoStats = cd_2;
    }
    if (pPlayer->CasinoAntiCheatCheck()) {
        char msg[264];
        int32_t afValue = pPlayer->GetCasinoCheatTimeRemaining();
        BSsprintf(msg, 260, "%s\nTime Remaining: %i", sBlackjackAntiCheatText.String(), afValue);
        Interface::ShowMessage(
            msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio* v6 = &BSAudio::QInstance();
        v6->GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
        return nullptr;
    }

    CASINO_DATA* Data = &BlackJackMenu::pCasino->data;
    if (g_blackJackCasinoStats->iTotalWinnings < Data->iMaxWinnings) {
        TESCasinoChips* ChipType = BlackJackMenu::pCasino->GetChipType();
        if (pPlayer->GetInventoryObjectCount(ChipType) >= g_blackJackMinChipsToPlay) {
            if (g_blackJackCasinoStats->iTotalWinnings < aiMinWinnings && aiMinWinnings) {
                char apString[264];
                BSsprintf(apString, 260, sGamblingMinWinText.String());
                Interface::ShowMessage(
                    apString, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f,
                    false
                );
                BSAudio* v12 = &BSAudio::QInstance();
                v12->GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
                return nullptr;
            }

            int32_t v13;
            if (pold_menu_tile) {
                delete pold_menu_tile;
            }
            Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\black_jack_menu.xml");
            BlackJackMenu::pMe = static_cast<BlackJackMenu*>(ptile->GetMenu());
            if (BlackJackMenu::pMe && (v13 = BlackJackMenu::pMe->GetClass(), v13 == BlackJackMenu::GetClassStatic())) {
                BlackJackMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
                if (BlackJackMenu::pMe->HasTiles()) {
                    char Src[64];
                    if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                        float time = Tile::GetMaximumDepth();
                        ptile->Set(4013, time, true);
                    }
                    BSAudio* v14 = &BSAudio::QInstance();
                    v14->GetSoundHandleByName("GAMEBlackJackCardShuffle", 289u).Play(false);
                    BlackJackMenu::pMe->iLuckSkill = pPlayer->GetClampedActorValue(ActorValue::LUCK);
                    BlackJackMenu::pMe->iCurrentBet = g_blackJackMinChipsToPlay;
                    const char* v15 = sCurrentBetText.String();
                    BlackJackMenu::pMe->pTiles[0]->Set(g_blackJackMenuTrait_Value, v15, true);
                    BlackJackMenu::pMe->pTiles[0]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[0]->Set(BlackJackMenu::iUT_X, g_blackJackMinChipsToPlay);
                    const char* v16 = sChipCountText.String();
                    BlackJackMenu::pMe->pTiles[1]->Set(g_blackJackMenuTrait_Value, v16, true);
                    BlackJackMenu::pMe->pTiles[1]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    TESCasinoChips* v17 = BlackJackMenu::pCasino->GetChipType();
                    int32_t ItemCountinContainer = pPlayer->GetInventoryObjectCount(v17);
                    BlackJackMenu::pMe->pTiles[1]->Set(BlackJackMenu::iUT_X, ItemCountinContainer);
                    if (BSstrlen(BlackJackMenu::pCasino->GetFullName())) {
                        const char* v19 = sCasinoEarningsText.String();
                        BSsprintf(Src, 64, "%s %s", BlackJackMenu::pCasino->GetFullName(), v19);
                    }
                    else {
                        const char* afValuea = sCasinoEarningsText.String();
                        BSsprintf(Src, 64, "%s %s", BlackJackMenu::pCasino->GetFormEditorID(), afValuea);
                    }
                    BlackJackMenu::pMe->pTiles[2]->Set(g_blackJackMenuTrait_Value, Src, true);
                    BlackJackMenu::pMe->pTiles[2]->Poke(BlackJackMenu::iUT_X, 1.0f);
                    BlackJackMenu::pMe->pTiles[2]->Set(BlackJackMenu::iUT_X, g_blackJackCasinoStats->iTotalWinnings);
                    Tile* TileByName = Tile::GetTileByName(BlackJackMenu::pMe->pTiles[2], "BJM_TotalEarningsValue");
                    TileByName->Set(4003, 0);
                    BlackJackMenu::pMe->SetTileCasinoEarnings();
                    BlackJackMenu::pMe->pTiles[3]->Poke(4036, 1.0f);
                    const char* v21 = sHitText.String();
                    BlackJackMenu::pMe->pTiles[3]->Set(4036, v21, true);
                    if (BlackJackMenu::pMe->InitCasinoData()) {
                        BlackJackMenu::SetDealNotSwitchHands(1);
                        BlackJackMenu::pMe->LoadNifs();
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

            if ((BlackJackMenu::pMe) && (BlackJackMenu::pMe->pMenu) && (BlackJackMenu::pMe)) {
                delete BlackJackMenu::pMe;
            }

            return nullptr;
        }

        char apText[264];
        BSsprintf(apText, 260, sGamblingBrokeText.String());
        Interface::ShowMessage(
            apText, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
        );
        BSAudio* v11 = &BSAudio::QInstance();
        v11->GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
        return nullptr;
    }

    char apDest[264];
    BSsprintf(apDest, 260, sGamblingBannedText.String());
    Interface::ShowMessage(
        apDest, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
    );
    BSAudio* v9 = &BSAudio::QInstance();
    v9->GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
    return nullptr;
}

int32_t BlackJackMenu::GetClassStatic() {
    return 1081;
}

CasinoData::CasinoData() : casinoID(0), iTotalWinnings(0), iWinningLevel(0) {}

TESCasinoChips* TESCasino::GetChipType() {
    return static_cast<TESCasinoChips*>(TESForm::GetFormByNumericID(data.casinoChipID));
}
