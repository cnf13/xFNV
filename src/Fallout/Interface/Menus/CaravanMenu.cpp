#include "Fallout/Interface/Menus/CaravanMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCaravan.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/TutorialMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <limits>

NiPointer<NiNode> CaravanMenu::spModel;
NiPointer<NiCamera> CaravanMenu::sp3DCamera;

CaravanMenu::CaravanMenu()
  : Menu(), eGameState(CA_IDLE), uiPrevIdleTick(0), iPlayerAnte(0), iNPCAnte(0), iTotalFunds(100), iCardsInDeck(25),
    iTotalCards(100), iNumSteps(0), iSelectedDeckCard(0), iSelectedTrack(0), iSelectedRow(0), iSelectedColumn(0),
    bTutorial(false), bClickedScrollbar(false), bScrolling(false), bContinueProcessing(true) {
    CaravanMenu::iUT_CurrentValue = Tile::AddUserTrait("_current_value", -1);
    CaravanMenu::iUT_Value = Tile::AddUserTrait("_Value", -1);
    CaravanMenu::iUT_X = Tile::AddUserTrait("_x", -1);
    ZeroFlags();
    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
    pListOfBillAnimations = new BSSimpleList<NiControllerSequence*>();
    pListOfCoinAnimations = new BSSimpleList<NiControllerSequence*>();
}

CaravanMenu::~CaravanMenu() {
    char acPath[268];
    CaravanMenu::sp3DCamera = nullptr;
    pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Table.NIF");
    BSShaderManager::GetShadowSceneNode(1)->DetachChild(CaravanMenu::spModel.m_pObject);
    BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
    if (CaravanMenu::spModel.m_pObject && CaravanMenu::spModel->GetChildCount()) {
        CaravanMenu::spModel->DetachAllChildren();
    }
    pTableGeometry = nullptr;
    pCameraAnimManager = nullptr;
    for (NiControllerSequence*& pAnimation : pCameraAnimations) {
        pAnimation = nullptr;
    }
    CaravanMenu::spModel = nullptr;
    CaravanMenu::pMe = nullptr;
    pChallenger = nullptr;
    pDeckParam = nullptr;
    BSsprintf(acPath, 260, "%sCurrency-Bill_Grid.NIF", "Meshes\\Terminals\\NV_Caravan\\");
    pModelLoader->ReleaseFile(acPath);
    BSsprintf(acPath, 260, "%sCurrency-Coin_Grid.NIF", "Meshes\\Terminals\\NV_Caravan\\");
    pModelLoader->ReleaseFile(acPath);
    pBillsGrid = nullptr;
    pCoinsGrid = nullptr;
    for (int32_t i = 0; i < 3; ++i) {
        BSsprintf(acPath, 260, "%sCurrency-Bill_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
        pModelLoader->ReleaseFile(acPath);
        BSsprintf(acPath, 260, "%sCurrency-Coin_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
        pModelLoader->ReleaseFile(acPath);
        pBillGeometry[i] = nullptr;
        pCoinGeometry[i] = nullptr;
    }
    while (pListOfBillAnimations && pListOfBillAnimations->m_item) {
        pListOfBillAnimations->RemoveHead();
    }
    if (pListOfBillAnimations) {
        delete pListOfBillAnimations;
    }
    pListOfBillAnimations = nullptr;
    while (pListOfCoinAnimations && pListOfCoinAnimations->m_item) {
        pListOfCoinAnimations->RemoveHead();
    }
    if (pListOfCoinAnimations) {
        delete pListOfCoinAnimations;
    }
    pListOfCoinAnimations = nullptr;
    // The card, money and joker arrays free their buffers in their own destructors.
    if (pDataHandler) {
        pDataHandler->ClearBarterContainer();
    }
    if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
        pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Deck.NIF");
        pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Available.NIF");
        pInactiveDeckGeometry = nullptr;
        pActiveDeckGeometry = nullptr;
        pActiveDeckAnimManager = nullptr;
        pInactiveDeckAnimManager = nullptr;
        for (int32_t i = 0; i < 6; ++i) {
            pInactiveDeckAnimations[i] = nullptr;
            pActiveDeckAnimations[i] = nullptr;
        }
        for (BSShaderPPLightingProperty*& pShader : deckFaceShaders) {
            pShader = nullptr;
        }
    }
    else if (eActiveMenu == CaravanMenu::CA_GAME_MENU || eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
        char acFile[264];
        for (NiControllerManager*& pManager : pHandAnimManager) {
            pManager = nullptr;
        }
        for (int32_t i = 0; i < 6; ++i) {
            for (int32_t j = 0; j < 7; ++j) {
                pTrackAnimManager[i][j] = nullptr;
            }
        }
        // The loop deliberately clears only the first animation pair of each track's first four rows.
        for (int32_t i = 0; i < 6; ++i) {
            for (int32_t j = 0; j < 4; ++j) {
                for (int32_t k = 0; k < 2; ++k) {
                    pTrackAnimations[i][j][0][k] = nullptr;
                }
            }
        }
        BSstrcpy(acFile, 260, "PlayerGrid_1.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "PlayerGrid_2.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "PlayerGrid_3.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "OppGrid_1.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "OppGrid_2.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "OppGrid_3.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "Select_Add.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSstrcpy(acFile, 260, "Select_Remove.NIF");
        pModelLoader->ReleaseFile(acFile);
        pCursors[0] = nullptr;
        pCursors[1] = nullptr;
        BSsprintf(acFile, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Player_Deck-Start.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSsprintf(acFile, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Opponent_Deck-Start.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSsprintf(acFile, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Player_Deck.NIF");
        pModelLoader->ReleaseFile(acFile);
        BSsprintf(acFile, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Opponent_Deck.NIF");
        pModelLoader->ReleaseFile(acFile);
        pHandGeometry[0] = nullptr;
        pHandGeometry[1] = nullptr;
        for (NiAVObject*& pGrid : pTrackGrids) {
            pGrid = nullptr;
        }
        for (int32_t i = 0; i < 3; ++i) {
            for (int32_t j = 0; j < 7; ++j) {
                BSsprintf(acFile, 260, "%sPlayer_%i_0%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1, j + 1);
                pModelLoader->ReleaseFile(acFile);
                pTrackGeometry[i][j] = nullptr;
                BSsprintf(acFile, 260, "%sOpponent_%i_0%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1, j + 1);
                pModelLoader->ReleaseFile(acFile);
                pTrackGeometry[i + 3][j + 3] = nullptr;
            }
        }
    }
}

void CaravanMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 50) {
        pTiles[aiID] = apTile;
    }
}

void CaravanMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
    bScrolling = false;
    if (eActiveMenu == 1 && aiID == 15 && !eGameState) {
        iPrevValue = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
        bScrolling = true;
        iPrevXLocation = g_caravanMenu_InterfaceManager->GetCursorX();
    }
}

void CaravanMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!eGameState && !IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
        switch (aiID) {
            case 7:
            case 14: {
                DoGamepad(12, 0.0);
                break;
            }
            case 10: {
                DoGamepad(16, 0.0);
                break;
            }
            case 12:
            case 19:
            case 37: {
                DoGamepad(9, 0.0);
                break;
            }
            case 13:
            case 36: {
                DoGamepad(11, 0.0);
                break;
            }
            case 20: {
                DoGamepad(13, 0.0);
                break;
            }
            case 21: {
                DoGamepad(14, 0.0);
                break;
            }
            case 22:
            case 38: {
                DoGamepadUpEvent(10, 0.0);
                break;
            }
            default: {
                return;
            }
        }
    }
}

void CaravanMenu::DoIdle() {
    CaravanAIPackage aiPackage;
    int32_t iElaspedMillis = 0;
    Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
    bool bIsMenuOnTop = Interface::IsTopMenuID(eMenu);
    if (bTutorial) {
        if (!bIsMenuOnTop) {
            return;
        }
        int32_t iTutorialID = iTutorial;
        if (!InterfaceManager::Instance()->xTutorialManager.HasBeenShown(
                static_cast<Interface::TutorialMessageID>(iTutorialID)
            )) {
            return;
        }
        bTutorial = false;
    }
    if (uiPrevIdleTick) {
        if (!bIsMenuOnTop) {
            SetFlag(CaravanMenu::CAF_ISONTOP, false);
            return;
        }
        SetFlag(CaravanMenu::CAF_ISONTOP, true);
        iElaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
        if (ePrevGameState == eGameState) {
            CaravanMenu::fTimeDelta = static_cast<double>(iElaspedMillis) / 1000.0 + CaravanMenu::fTimeDelta;
        }
        else {
            CaravanMenu::fTimeDelta = 0.0f;
        }
    }
    else {
        iElaspedMillis = 0;
        CaravanMenu::fTimeDelta = 0.0f;
        ePrevGameState = 0;
        fSelectionDelay = 0.0f;
    }
    int32_t iXAxis = 0;
    int32_t iYAxis = 0;
    float fXAxis = 0.0f;
    float fYAxis = 0.0f;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        iXAxis = appMain->GetControls()->GetControllerValue(0, 7);
        iYAxis = appMain->GetControls()->GetControllerValue(0, 8);
        if (NiAbs(static_cast<float>(iXAxis)) <= 7849.0) {
            iXAxis = 0;
        }
        fXAxis = static_cast<double>(iXAxis) / 32767.0;
        if (NiAbs(static_cast<float>(iYAxis)) <= 7849.0) {
            iYAxis = 0;
        }
        fYAxis = static_cast<double>(-iYAxis) / 32767.0;
    }
    uiPrevIdleTick = appTimer.iLastTime;
    ePrevGameState = eGameState;
    if (!bIsMenuOnTop || eActiveMenu == CaravanMenu::CA_DECK_MENU) {
        InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
    }
    if (this->IsFlagSet(CaravanMenu::CAF_MARKFORCLOSE)) {
        if (bIsMenuOnTop) {
            SetFlag(CaravanMenu::CAF_MARKFORCLOSE, false);
            eGameState = CaravanMenu::CA_GAME_GAMEOVER;
        }
        return;
    }
    if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU &&
        eGameState == CaravanMenu::CA_IDLE &&
        CaravanMenu::fTimeDelta > 1.0) {
        SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
    }
    switch (eGameState) {
        case CaravanMenu::CA_IDLE: {
            switch (eActiveMenu) {
                case CaravanMenu::CA_DECK_MENU: {
                    if (iXAxis) {
                        double dCurrentValue = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
                        int32_t iValue = static_cast<int32_t>(fXAxis / std::fabs(fXAxis) + dCurrentValue);
                        if (iValue >= 0 && iValue < iTotalCards) {
                            iNumSteps = 1;
                            pTiles[15]->Set(CaravanMenu::iUT_CurrentValue, static_cast<float>(iValue), false);
                            int32_t iSetInCode = Tile::TextToTrait("_SetInCode");
                            pTiles[15]->Poke(iSetInCode, 1.0f);
                            eGameState = fXAxis > 0.0 ? CaravanMenu::CA_DECK_FORWARD_ANIM
                                                      : CaravanMenu::CA_DECK_BACK_ANIM;
                        }
                    }
                    if (!InterfaceManager::Has360Controller()) {
                        if (g_caravanMenu_InterfaceManager->fMouseHeldTime == 0.0 &&
                            bScrolling &&
                            eActiveMenu == CaravanMenu::CA_DECK_MENU) {
                            iNumSteps = 0;
                            bScrolling = false;
                            int32_t iCurrentValue = static_cast<int32_t>(
                                pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue)
                            );
                            int32_t iSteps = iCurrentValue - iPrevValue;
                            if (iSteps > -1) {
                                if (iSteps >= 1) {
                                    if (iSteps >= 5) {
                                        eGameState = CaravanMenu::CA_DECK_FASTFORWARD_ANIM;
                                    }
                                    else {
                                        iNumSteps = iSteps;
                                        eGameState = CaravanMenu::CA_DECK_FORWARD_ANIM;
                                    }
                                }
                            }
                            else if (iSteps <= -5) {
                                eGameState = CaravanMenu::CA_DECK_REWIND_ANIM;
                            }
                            else {
                                iNumSteps = -iSteps;
                                eGameState = CaravanMenu::CA_DECK_BACK_ANIM;
                            }
                            iPrevValue = iCurrentValue;
                        }
                        else if (!bScrolling && g_caravanMenu_InterfaceManager->fMouseHeldTime > 0.0) {
                            iPrevValue = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
                            bScrolling = true;
                            iPrevXLocation = g_caravanMenu_InterfaceManager->GetCursorX();
                        }
                        if (eGameState == CaravanMenu::CA_IDLE) {
                            switch (uiPendingArrowKey) {
                                case 0x80000003u: {
                                    uiPendingArrowKey = 0;
                                    iYAxis = -1;
                                    break;
                                }
                                case 0x80000004u: {
                                    uiPendingArrowKey = 0;
                                    iYAxis = 1;
                                    break;
                                }
                                case 0x80000002u: {
                                    uiPendingArrowKey = 0;
                                    iXAxis = 1;
                                    break;
                                }
                                case 0x80000001u: {
                                    uiPendingArrowKey = 0;
                                    iXAxis = -1;
                                    break;
                                }
                            }
                            if ((iXAxis || iYAxis) && static_cast<double>(uiPrevIdleTick) - fSelectionDelay > 0.25) {
                                fSelectionDelay = static_cast<float>(uiPrevIdleTick);
                                if (eActiveMenu == CaravanMenu::CA_DECK_MENU && iXAxis) {
                                    int32_t iValue = static_cast<int32_t>(
                                        pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue) +
                                        static_cast<double>(iXAxis)
                                    );
                                    if (iValue >= 0 && iValue < iTotalCards) {
                                        pTiles[15]
                                            ->Set(CaravanMenu::iUT_CurrentValue, static_cast<float>(iValue), false);
                                        int32_t iSetInCode = Tile::TextToTrait("_SetInCode");
                                        pTiles[15]->Poke(iSetInCode, 1.0f);
                                        eGameState = static_cast<CaravanMenu::CaravanState>((iXAxis <= 0) + 5);
                                        iNumSteps = 1;
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                case CaravanMenu::CA_GAME_MENU: {
                    if (eGameState == CaravanMenu::CA_IDLE) {
                        switch (uiPendingArrowKey) {
                            case 0x80000003u: {
                                uiPendingArrowKey = 0;
                                iYAxis = -1;
                                fYAxis = -1.0f;
                                break;
                            }
                            case 0x80000004u: {
                                uiPendingArrowKey = 0;
                                iYAxis = 1;
                                fYAxis = 1.0f;
                                break;
                            }
                            case 0x80000002u: {
                                uiPendingArrowKey = 0;
                                iXAxis = 1;
                                fXAxis = 1.0f;
                                break;
                            }
                            case 0x80000001u: {
                                uiPendingArrowKey = 0;
                                iXAxis = -1;
                                fXAxis = -1.0f;
                                break;
                            }
                        }
                    }
                    if ((iXAxis || iYAxis) && CaravanMenu::fTimeDelta - fSelectionDelay > 0.25) {
                        fSelectionDelay = CaravanMenu::fTimeDelta;
                        if (this->IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
                            if (!this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
                                double dYStep;
                                double dXStep;
                                if (std::fabs(fXAxis) < 0.01) {
                                    dXStep = 0.0;
                                }
                                else {
                                    dXStep = fXAxis / std::fabs(fXAxis);
                                }
                                int32_t iXParam = static_cast<int32_t>(dXStep);
                                if (std::fabs(fYAxis) < 0.01) {
                                    dYStep = 0.0;
                                }
                                else {
                                    dYStep = fYAxis / std::fabs(fYAxis);
                                }
                                int32_t iYParam = static_cast<int32_t>(dYStep);
                                if (this->IsFlagSet(CaravanMenu::CAF_DESTROY_TRACK)) {
                                    DeselectWholeTrack(iSelectedTrack);
                                    iSelectedTrack += iXParam;
                                    int32_t iTrack = BSMAX(0, iSelectedTrack);
                                    iSelectedTrack = BSMIN(iTrack, 2);
                                    BSAudio::QInstance()
                                        .GetSoundHandleByName("GAMECaravanToggleTrack", 289u)
                                        .Play(false);
                                    SelectWholeTrack(iSelectedTrack);
                                }
                                else {
                                    HandleTrackSelection(iXParam, iYParam);
                                }
                            }
                        }
                        else {
                            iSelectedDeckCard = static_cast<int32_t>(
                                fXAxis / std::fabs(fXAxis) + static_cast<double>(iSelectedDeckCard)
                            );
                            int32_t iHandSize = pPlayerHand.iSize;
                            if (iSelectedDeckCard <= iHandSize - 1) {
                                if (iSelectedDeckCard < 0) {
                                    iSelectedDeckCard = iHandSize - 1;
                                }
                            }
                            else {
                                iSelectedDeckCard = 0;
                            }
                            BSAudio::QInstance().GetSoundHandleByName("GAMECaravanSwitchCard", 289u).Play(false);
                            SelectDeckCard(false);
                            if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
                                pPlayerHand[iSelectedDeckCard]->data.iFaceValue > 10) {
                                SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                            }
                            else {
                                SetFlag(CaravanMenu::CAF_DISABLE_A, false);
                            }
                            UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                        }
                    }
                    break;
                }
                case CaravanMenu::CA_RESULTS_MENU: {
                    if (appMain->GetControls()->GetMouseButton(0, 1) || appMain->GetControls()->GetMouseButton(1, 1)) {
                        DoGamepad(9, 0.0f);
                    }
                    break;
                }
            }
            UpdateCaravanFlags();
            UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
            return;
        }
        case CaravanMenu::CA_CAMERA_TO_DECK_ANIM: {
            if (pCameraAnimations[0]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pCameraAnimManager->ActivateSequence(pCameraAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pCameraAnimations[0]->m_fEndKeyTime) {
                    eGameState = CaravanMenu::CA_IDLE;
                    pCameraAnimManager->DeactivateSequence(pCameraAnimations[0], 0.0f);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    if (!InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_DECK)) {
                        TESForm* pForm = TESForm::GetFormByEditorID("HelpCaravanDeckBuilding");
                        BGSMessage* pHelpForm = dynamic_cast<BGSMessage*>(pForm);
                        if (pHelpForm) {
                            bTutorial = TutorialMenu::Create(pHelpForm, false);
                            InterfaceManager::Instance()->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_DECK);
                            iTutorial = 31;
                        }
                    }
                }
            }
            return;
        }
        case CaravanMenu::CA_CAMERA_TO_GAME_ANIM: {
            if (pCameraAnimations[1] && pHandAnimations[0][0] && pHandAnimations[1][0]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pCameraAnimManager->ActivateSequence(pCameraAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    ZeroFlags();
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    pHandAnimManager[0]->ActivateSequence(pHandAnimations[0][0], 0, false, 1.0f, 0.0f, nullptr);
                    pHandAnimManager[1]->ActivateSequence(pHandAnimations[1][0], 0, false, 1.0f, 0.0f, nullptr);
                    fSelectionDelay = 0.0f;
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanStartMatch", 289u).Play(false);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                pHandGeometry[0]->Update(UpdateData2);
                pHandGeometry[1]->Update(UpdateData3);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pCameraAnimations[1]->m_fEndKeyTime &&
                    static_cast<double>(CaravanMenu::fTimeDelta) >= pHandAnimations[0][0]->m_fEndKeyTime &&
                    static_cast<double>(CaravanMenu::fTimeDelta) >= pHandAnimations[1][0]->m_fEndKeyTime) {
                    pCameraAnimManager->DeactivateSequence(pCameraAnimations[1], 0.0f);
                    pHandAnimManager[0]->DeactivateSequence(pHandAnimations[0][0], 0.0f);
                    pHandAnimManager[1]->DeactivateSequence(pHandAnimations[1][0], 0.0f);
                    SetFlag(CaravanMenu::CAF_TRACK_SETUP, true);
                    float fX = 0.0f;
                    float fY = 0.0f;
                    int32_t iSafeZoneWidth = static_cast<int32_t>(pTiles[2]->GetFloat(4017));
                    NiAVObject* pSelectNode = BSUtilities::GetObjectByName(pTrackGrids[0], "Select1_01:0");
                    CaravanMenu::sp3DCamera->WorldPtToScreenPt(pSelectNode->GetWorldTranslate(), fX, fY, 1e-05f);
                    fX = static_cast<double>(iSafeZoneWidth) * fX - 120.0;
                    pTiles[23]->Set(4001, fX, true);
                    pTiles[28]->Set(4001, fX, true);
                    fX = fX + 265.0;
                    pTiles[24]->Set(4001, fX, true);
                    pTiles[27]->Set(4001, fX, true);
                    fX = fX + 265.0;
                    pTiles[25]->Set(4001, fX, true);
                    pTiles[26]->Set(4001, fX, true);
                    Tile::GetTileByName(pTiles[2], "CGM_NPCTrackRect")->Set(4003, 1);
                    Tile::GetTileByName(pTiles[2], "CGM_PlayerTrackRect")->Set(4003, 1);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                    if (!InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_TRACK)) {
                        TESForm* pForm = TESForm::GetFormByEditorID("HelpCaravanStartingCaravans");
                        BGSMessage* pHelpForm = dynamic_cast<BGSMessage*>(pForm);
                        if (pHelpForm) {
                            bTutorial = TutorialMenu::Create(pHelpForm, false);
                            InterfaceManager::Instance()->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_TRACK);
                            iTutorial = 32;
                        }
                    }
                }
            }

            return;
        }
        case CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM: {
            if (pCameraAnimations[2]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pCameraAnimManager->ActivateSequence(pCameraAnimations[2], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pCameraAnimations[2]->m_fEndKeyTime) {
                    eGameState = CaravanMenu::CA_IDLE;
                    eActiveMenu = CaravanMenu::CA_RESULTS_MENU;
                    pCameraAnimManager->DeactivateSequence(pCameraAnimations[2], 0.0f);
                    SetFlag(CaravanMenu::CAF_AI, false);
                    const char* pPressAnyKeyText = sCaravanPressAnyKeyText.String();
                    pTiles[35]->Set(4036, pPressAnyKeyText, true);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                }
            }
            return;
        }
        case CaravanMenu::CA_ANTE_MONEYIN_ANIM: {
            if (CaravanMenu::fTimeDelta == 0.0) {
                SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                BSAudio::QInstance().GetSoundHandleByName("GAMECaravanFundsDrop", 289u).Play(false);
            }
            for (int32_t i = iBillIndexToAnim; i < pListOfBills.iSize; ++i) {
                NiUpdateData UpdateData(CaravanMenu::fTimeDelta, 1, 0);
                pListOfBills[i]->Update(UpdateData);
            }
            for (int32_t i = iCoinIndexToAnim; i < pListOfCoins.iSize; ++i) {
                NiUpdateData UpdateData(CaravanMenu::fTimeDelta, 1, 0);
                pListOfCoins[i]->Update(UpdateData);
            }
            if (CaravanMenu::fTimeDelta > 1.0) {
                SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                iBillIndexToAnim = pListOfBills.iSize;
                iCoinIndexToAnim = pListOfCoins.iSize;
                eGameState = CaravanMenu::CA_IDLE;
            }
            return;
        }
        case CaravanMenu::CA_DECK_FORWARD_ANIM: {
            if (pActiveDeckAnimations[0] || pInactiveDeckAnimations[0]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    pInactiveDeckAnimManager
                        ->ActivateSequence(pInactiveDeckAnimations[0], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    --iNumSteps;
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanNavigate", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                pActiveDeckGeometry->Update(UpdateData2);
                pInactiveDeckGeometry->Update(UpdateData3);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[0]->m_fEndKeyTime) {
                    double dTarget = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue) -
                        static_cast<double>(iNumSteps);
                    SwapDeckBuildingTextures(static_cast<int32_t>(dTarget), false);
                    NiUpdateData ResetData1(0.0f, true, false);
                    NiUpdateData ResetData2(0.0f, true, false);
                    NiUpdateData ResetData3(0.0f, true, false);
                    CaravanMenu::spModel->GetAt(0)->Update(ResetData1);
                    pActiveDeckGeometry->Update(ResetData2);
                    pInactiveDeckGeometry->Update(ResetData3);
                    if (iNumSteps > 0) {
                        CaravanMenu::fTimeDelta = 0.0f;
                        --iNumSteps;
                    }
                    else {
                        eGameState = CaravanMenu::CA_IDLE;
                        pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[0], 0.0f);
                        pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[0], 0.0f);
                        SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                        UpdateCaravanFlags();
                        UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    }
                    uiPendingArrowKey = 0;
                }
            }
            return;
        }
        case CaravanMenu::CA_DECK_BACK_ANIM: {
            if (pActiveDeckAnimations[1] || pInactiveDeckAnimations[1]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    pInactiveDeckAnimManager
                        ->ActivateSequence(pInactiveDeckAnimations[1], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    --iNumSteps;
                    double dTarget = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue) +
                        static_cast<double>(iNumSteps);
                    SwapDeckBuildingTextures(static_cast<int32_t>(dTarget), false);
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanNavigate", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                pActiveDeckGeometry->Update(UpdateData2);
                pInactiveDeckGeometry->Update(UpdateData3);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[1]->m_fEndKeyTime) {
                    if (iNumSteps > 0) {
                        CaravanMenu::fTimeDelta = 0.0f;
                        if (iNumSteps > 0) {
                            CaravanMenu::fTimeDelta = 0.0f;
                            --iNumSteps;
                        }
                        else {
                            eGameState = CaravanMenu::CA_IDLE;
                            pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[0], 0.0f);
                            pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[0], 0.0f);
                            SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                            UpdateCaravanFlags();
                            UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                        }
                    }
                    else {
                        eGameState = CaravanMenu::CA_IDLE;
                        pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[1], 0.0f);
                        pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[1], 0.0f);
                        SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                        UpdateCaravanFlags();
                        UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    }
                    uiPendingArrowKey = 0;
                }
            }
            return;
        }
        case CaravanMenu::CA_DECK_FLIP_ANIM: {
            if (pActiveDeckAnimations[2] || pInactiveDeckAnimations[2]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[2], 0, false, 1.0f, 0.0f, nullptr);
                    pInactiveDeckAnimManager
                        ->ActivateSequence(pInactiveDeckAnimations[2], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanAddRemove", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                pActiveDeckGeometry->Update(UpdateData2);
                pInactiveDeckGeometry->Update(UpdateData3);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[2]->m_fEndKeyTime) {
                    eGameState = CaravanMenu::CA_IDLE;
                    FlipSelectedCardTextures(11);
                    NiUpdateData ResetData1(0.0f, true, false);
                    NiUpdateData ResetData2(0.0f, true, false);
                    NiUpdateData ResetData3(0.0f, true, false);
                    CaravanMenu::spModel->GetAt(0)->Update(ResetData1);
                    pActiveDeckGeometry->Update(ResetData2);
                    pInactiveDeckGeometry->Update(ResetData3);
                    pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[2], 0.0f);
                    pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[2], 0.0f);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                }
            }
            return;
        }
        case CaravanMenu::CA_DECK_FASTFORWARD_ANIM: {
            if (pActiveDeckAnimations[3] || pInactiveDeckAnimations[3]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    double dCurrentValue = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
                    SwapDeckBuildingTextures(static_cast<int32_t>(dCurrentValue), false);
                    pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[3], 0, false, 1.0f, 0.0f, nullptr);
                    pInactiveDeckAnimManager
                        ->ActivateSequence(pInactiveDeckAnimations[3], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanChangeDeck", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                pActiveDeckGeometry->Update(UpdateData2);
                pInactiveDeckGeometry->Update(UpdateData3);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[3]->m_fEndKeyTime) {
                    eGameState = CaravanMenu::CA_IDLE;
                    pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[3], 0.0f);
                    pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[3], 0.0f);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    uiPendingArrowKey = 0;
                }
            }
            return;
        }
        case CaravanMenu::CA_DECK_REWIND_ANIM: {
            if (pActiveDeckAnimations[4] || pInactiveDeckAnimations[4]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[4], 0, false, 1.0f, 0.0f, nullptr);
                    pInactiveDeckAnimManager
                        ->ActivateSequence(pInactiveDeckAnimations[4], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanChangeDeck", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                pActiveDeckGeometry->Update(UpdateData2);
                pInactiveDeckGeometry->Update(UpdateData3);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[4]->m_fEndKeyTime) {
                    eGameState = CaravanMenu::CA_IDLE;
                    double dCurrentValue = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
                    SwapDeckBuildingTextures(static_cast<int32_t>(dCurrentValue), false);
                    pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[4], 0.0f);
                    pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[4], 0.0f);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    uiPendingArrowKey = 0;
                }
            }
            return;
        }
        case CaravanMenu::CA_DECK_WIPE_ANIM: {
            if (pActiveDeckAnimations[5] || pInactiveDeckAnimations[5]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[5], 0, false, 1.0f, 0.0f, nullptr);
                    pInactiveDeckAnimManager
                        ->ActivateSequence(pInactiveDeckAnimations[5], 0, false, 1.0f, 0.0f, nullptr);
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanChangeDeck", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                NiUpdateData UpdateData3(CaravanMenu::fTimeDelta, true, false);
                CaravanMenu::spModel->GetAt(0)->Update(UpdateData1);
                pActiveDeckGeometry->Update(UpdateData2);
                pInactiveDeckGeometry->Update(UpdateData3);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[5]->m_fEndKeyTime) {
                    eGameState = CaravanMenu::CA_IDLE;
                    FlipSelectedCardTextures(11);
                    NiUpdateData FinalData1(CaravanMenu::fTimeDelta, true, false);
                    NiUpdateData FinalData2(CaravanMenu::fTimeDelta, true, false);
                    NiUpdateData FinalData3(CaravanMenu::fTimeDelta, true, false);
                    CaravanMenu::spModel->GetAt(0)->Update(FinalData1);
                    pActiveDeckGeometry->Update(FinalData2);
                    pInactiveDeckGeometry->Update(FinalData3);
                    pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[5], 0.0f);
                    pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[5], 0.0f);
                    bContinueProcessing = true;
                    SetFlag(CaravanMenu::CAF_AI, true);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    PrepareGameMenu();
                }
            }
            return;
        }
        case CaravanMenu::CA_GAME_INTRO_DEAL_ANIM: {
            uint32_t uiIsAI = this->IsFlagSet(CaravanMenu::CAF_AI);
            if (pStartAnimations[uiIsAI][iSelectedDeckCard] &&
                pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    UpdateSelectedTrackCardTexture();
                    pHandAnimManager[uiIsAI]
                        ->ActivateSequence(pStartAnimations[uiIsAI][iSelectedDeckCard], 0, false, 1.0f, 0.0f, nullptr);
                    pTrackAnimManager[iSelectedTrack][iSelectedRow]->ActivateSequence(
                        pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0],
                        0,
                        false,
                        1.0f,
                        0.0f,
                        nullptr
                    );
                    if (!uiIsAI) {
                        SelectDeckCard(true);
                    }
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    fSelectionDelay = 0.0f;
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanConfirmAddtoDeck", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                pHandGeometry[uiIsAI]->Update(UpdateData1);
                NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
                pTrackGeometry[iSelectedTrack][iSelectedRow]->Update(UpdateData2);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >=
                        pStartAnimations[uiIsAI][iSelectedDeckCard]->m_fEndKeyTime &&
                    static_cast<double>(CaravanMenu::fTimeDelta) >=
                        pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]->m_fEndKeyTime) {
                    pHandAnimManager[uiIsAI]->DeactivateSequence(pStartAnimations[uiIsAI][iSelectedDeckCard], 0.0f);
                    pTrackAnimManager[iSelectedTrack][iSelectedRow]->DeactivateSequence(
                        pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0],
                        0.0f
                    );
                    if ((pCardTracks[2]->iSize && !this->IsFlagSet(CaravanMenu::CAF_AI)) ||
                        (pCardTracks[3]->iSize && this->IsFlagSet(CaravanMenu::CAF_AI))) {
                        bool bTrackSetup = this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP);
                        bool bAI = this->IsFlagSet(CaravanMenu::CAF_AI);
                        UpdateGameHandTextures(!bAI, bTrackSetup);
                        pHandAnimManager[uiIsAI]
                            ->ActivateSequence(pHandAnimations[uiIsAI][1], 0, false, 1.0f, 0.0f, nullptr);
                        NiUpdateData ResetData1(0.0f, true, false);
                        pHandGeometry[uiIsAI]->Update(ResetData1);
                        pHandAnimManager[uiIsAI]->DeactivateSequence(pHandAnimations[uiIsAI][1], 0.0f);
                        if (!this->IsFlagSet(CaravanMenu::CAF_AI) && pCardTracks[2]->iSize && pCardTracks[3]->iSize) {
                            SetFlag(CaravanMenu::CAF_TRACK_SETUP, false);
                            if (!InterfaceManager::Instance()->xTutorialManager.HasBeenShown(
                                    Interface::TUT_CARAVAN_GAME
                                )) {
                                TESForm* pForm = TESForm::GetFormByEditorID("HelpCaravanContractWar");
                                BGSMessage* pHelpForm = dynamic_cast<BGSMessage*>(pForm);
                                if (pHelpForm) {
                                    bTutorial = TutorialMenu::Create(pHelpForm, false);
                                    InterfaceManager::Instance()->xTutorialManager.FlagAsShown(
                                        Interface::TUT_CARAVAN_GAME
                                    );
                                    iTutorial = 33;
                                }
                            }
                        }
                    }
                    else {
                        UpdateGameHandTextures(!this->IsFlagSet(CaravanMenu::CAF_AI), true);
                        pHandAnimManager[uiIsAI]
                            ->ActivateSequence(pStartAnimations[uiIsAI][0], 0, false, 1.0f, 0.0f, nullptr);
                        NiUpdateData ResetData1(0.0f, true, false);
                        pHandGeometry[uiIsAI]->Update(ResetData1);
                        pHandAnimManager[uiIsAI]->DeactivateSequence(pStartAnimations[uiIsAI][0], 0.0f);
                    }
                    pHandAnimManager[uiIsAI]->DeactivateAll(0.0f);
                    if (uiIsAI == 1) {
                        iSelectedDeckCard = pPlayerHand.iSize - 1;
                        SelectDeckCard(false);
                        if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
                            pPlayerHand[iSelectedDeckCard]->data.iFaceValue > 10) {
                            SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                        }
                    }
                    pTiles[uiIsAI + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
                    uint32_t uiDeckSize = uiIsAI ? pNPCDeck.iSize : pPlayerDeck.iSize;
                    pTiles[uiIsAI + 29]->Set(CaravanMenu::iUT_Value, uiDeckSize);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                }
            }

            return;
        }
        case CaravanMenu::CA_GAME_INTRO_DISCARD_ANIM: {
            uint32_t uiIsAI = this->IsFlagSet(CaravanMenu::CAF_AI);
            if (pStartAnimations[uiIsAI][iSelectedDeckCard]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    pHandAnimManager[uiIsAI]
                        ->ActivateSequence(pStartAnimations[uiIsAI][iSelectedDeckCard], 0, false, 1.0f, 0.0f, nullptr);
                    if (!uiIsAI) {
                        SelectDeckCard(true);
                    }
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    fSelectionDelay = 0.0f;
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanRemoveCard", 289u).Play(false);
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                pHandGeometry[uiIsAI]->Update(UpdateData1);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >=
                    pStartAnimations[uiIsAI][iSelectedDeckCard]->m_fEndKeyTime) {
                    pHandAnimManager[uiIsAI]->DeactivateSequence(pStartAnimations[uiIsAI][iSelectedDeckCard], 0.0f);
                    if ((pCardTracks[2]->iSize && !this->IsFlagSet(CaravanMenu::CAF_AI)) ||
                        (pCardTracks[3]->iSize && this->IsFlagSet(CaravanMenu::CAF_AI))) {
                        bool bTrackSetup = this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP);
                        bool bAI = this->IsFlagSet(CaravanMenu::CAF_AI);
                        UpdateGameHandTextures(!bAI, bTrackSetup);
                        pHandAnimManager[uiIsAI]
                            ->ActivateSequence(pHandAnimations[uiIsAI][1], 0, false, 1.0f, 0.0f, nullptr);
                        NiUpdateData ResetData1(0.0f, true, false);
                        pHandGeometry[uiIsAI]->Update(ResetData1);
                        pHandAnimManager[uiIsAI]->DeactivateSequence(pHandAnimations[uiIsAI][1], 0.0f);
                    }
                    else {
                        UpdateGameHandTextures(!this->IsFlagSet(CaravanMenu::CAF_AI), true);
                        pHandAnimManager[uiIsAI]
                            ->ActivateSequence(pStartAnimations[uiIsAI][0], 0, false, 1.0f, 0.0f, nullptr);
                        NiUpdateData ResetData1(0.0f, true, false);
                        pHandGeometry[uiIsAI]->Update(ResetData1);
                        pHandAnimManager[uiIsAI]->DeactivateSequence(pStartAnimations[uiIsAI][0], 0.0f);
                    }
                    pHandAnimManager[uiIsAI]->DeactivateAll(0.0f);
                    if (!uiIsAI) {
                        iSelectedDeckCard = pPlayerHand.iSize - 1;
                        SelectDeckCard(false);
                        if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
                            pPlayerHand[iSelectedDeckCard]->data.iFaceValue > 10) {
                            SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                        }
                    }
                    pTiles[uiIsAI + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
                    uint32_t uiDeckSize = uiIsAI ? pNPCDeck.iSize : pPlayerDeck.iSize;
                    pTiles[uiIsAI + 29]->Set(CaravanMenu::iUT_Value, uiDeckSize);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    if (this->IsFlagSet(CaravanMenu::CAF_AI)) {
                        eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                    }
                    else {
                        eGameState = CaravanMenu::CA_IDLE;
                        SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                    }
                }
            }
            return;
        }
        case CaravanMenu::CA_GAME_PLACECARD_ANIM: {
            uint32_t uiIsAI = this->IsFlagSet(CaravanMenu::CAF_AI);
            if (!pHandAnimations[uiIsAI][iSelectedDeckCard + 1]) {
                return;
            }
            if (!pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]) {
                return;
            }
            if (CaravanMenu::fTimeDelta == 0.0) {
                UpdateSelectedTrackCardTexture();
                int32_t iNextCard = uiIsAI ? iNextNPCCard : iNextPlayerCard;
                if (iNextCard == -1) {
                    pHandAnimManager[uiIsAI]
                        ->ActivateSequence(pStartAnimations[uiIsAI][iSelectedDeckCard], 0, false, 1.0f, 0.0f, nullptr);
                }
                else {
                    pHandAnimManager[uiIsAI]->ActivateSequence(
                        pHandAnimations[uiIsAI][iSelectedDeckCard + 1],
                        0,
                        false,
                        1.0f,
                        0.0f,
                        nullptr
                    );
                }
                pTrackAnimManager[iSelectedTrack][iSelectedRow]->ActivateSequence(
                    pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0],
                    0,
                    false,
                    1.0f,
                    0.0f,
                    nullptr
                );
                if (!uiIsAI) {
                    SelectDeckCard(true);
                }
                SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                fSelectionDelay = 0.0f;
                BSAudio::QInstance().GetSoundHandleByName("GAMECaravanConfirmAddToTrack", 289u).Play(false);
            }
            NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
            pHandGeometry[uiIsAI]->Update(UpdateData1);
            NiUpdateData UpdateData2(CaravanMenu::fTimeDelta, true, false);
            pTrackGeometry[iSelectedTrack][iSelectedRow]->Update(UpdateData2);
            if (!(static_cast<double>(CaravanMenu::fTimeDelta) >=
                  pHandAnimations[uiIsAI][iSelectedDeckCard]->m_fEndKeyTime) ||
                !(static_cast<double>(CaravanMenu::fTimeDelta) >=
                  pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]->m_fEndKeyTime)) {
                return;
            }
            pHandAnimManager[uiIsAI]->DeactivateSequence(pHandAnimations[uiIsAI][iSelectedDeckCard + 1], 0.0f);
            pTrackAnimManager[iSelectedTrack][iSelectedRow]->DeactivateSequence(
                pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0],
                0.0f
            );
            bool bTrackSetup = this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP);
            bool bAI = this->IsFlagSet(CaravanMenu::CAF_AI);
            UpdateGameHandTextures(!bAI, bTrackSetup);
            pHandAnimManager[uiIsAI]->ActivateSequence(pHandAnimations[uiIsAI][1], 0, false, 1.0f, 0.0f, nullptr);
            NiUpdateData ResetData1(0.0f, true, false);
            pHandGeometry[uiIsAI]->Update(ResetData1);
            pHandAnimManager[uiIsAI]->DeactivateSequence(pHandAnimations[uiIsAI][1], 0.0f);
            if (uiIsAI == 1 && pSelectedCard->data.iFaceValue != 12 && pSelectedCard->data.iFaceValue != 15) {
                iSelectedDeckCard = pPlayerHand.iSize - 1;
                SelectDeckCard(false);
                if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
                    pPlayerHand[iSelectedDeckCard]->data.iFaceValue > 10) {
                    SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                }
            }
            else if (pSelectedCard->data.iFaceValue == 12 || pSelectedCard->data.iFaceValue == 15) {
                pTiles[uiIsAI + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
                uint32_t uiDeckSize = uiIsAI ? pNPCDeck.iSize : pPlayerDeck.iSize;
                pTiles[uiIsAI + 29]->Set(CaravanMenu::iUT_Value, uiDeckSize);
                if (pSelectedCard->data.iFaceValue == 12) {
                    eGameState = CaravanMenu::CA_GAME_JACK_CLEAR_ANIM;
                }
                else {
                    eGameState = CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM;
                }
                if (eGameState == CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM) {
                    int32_t iFaceValue = pCardTracks[iSelectedTrack][iSelectedRow][0]->data.iFaceValue;
                    int32_t iSuitValue = pCardTracks[iSelectedTrack][iSelectedRow][0]->data.iCardSuit;
                    for (int32_t iTrack = 0; iTrack < 6; ++iTrack) {
                        for (int32_t iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
                            if (iFaceValue == 1) {
                                int32_t iCardSuit = pCardTracks[iTrack][iRow][0]->data.iCardSuit;
                                if (iCardSuit == iSuitValue && (iTrack != iSelectedTrack || iRow != iSelectedRow)) {
                                    jokerLocations[iTrack].Add(iRow);
                                }
                            }
                            else {
                                int32_t iCardFaceValue = pCardTracks[iTrack][iRow][0]->data.iFaceValue;
                                if (iCardFaceValue == iFaceValue &&
                                    (iTrack != iSelectedTrack || iRow != iSelectedRow)) {
                                    jokerLocations[iTrack].Add(iRow);
                                }
                            }
                        }
                    }
                    iSelectedTrack = 0;
                }
                break;
            }
            pTiles[uiIsAI + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
            uint32_t uiDeckSize = uiIsAI ? pNPCDeck.iSize : pPlayerDeck.iSize;
            pTiles[uiIsAI + 29]->Set(CaravanMenu::iUT_Value, uiDeckSize);
            UpdateCaravanFlags();
            UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
            eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
            return;
        }
        case CaravanMenu::CA_GAME_DISCARDCARD_ANIM: {
            uint32_t uiIsAI = this->IsFlagSet(CaravanMenu::CAF_AI);
            if (pHandAnimations[uiIsAI][iSelectedDeckCard + 1]) {
                if (CaravanMenu::fTimeDelta == 0.0) {
                    int32_t iNextCard = uiIsAI ? iNextNPCCard : iNextPlayerCard;
                    if (iNextCard == -1) {
                        pHandAnimManager[uiIsAI]->ActivateSequence(
                            pStartAnimations[uiIsAI][iSelectedDeckCard],
                            0,
                            false,
                            1.0f,
                            0.0f,
                            nullptr
                        );
                        BSAudio::QInstance().GetSoundHandleByName("GAMECaravanRemoveCard", 289u).Play(false);
                    }
                    else {
                        pHandAnimManager[uiIsAI]->ActivateSequence(
                            pHandAnimations[uiIsAI][iSelectedDeckCard + 1],
                            0,
                            false,
                            1.0f,
                            0.0f,
                            nullptr
                        );
                        BSAudio::QInstance().GetSoundHandleByName("GAMECaravanAddRemoveCard", 289u).Play(false);
                    }
                    if (!uiIsAI) {
                        SelectDeckCard(true);
                    }
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    fSelectionDelay = 0.0f;
                }
                NiUpdateData UpdateData1(CaravanMenu::fTimeDelta, true, false);
                pHandGeometry[uiIsAI]->Update(UpdateData1);
                if (static_cast<double>(CaravanMenu::fTimeDelta) >=
                    pHandAnimations[uiIsAI][iSelectedDeckCard]->m_fEndKeyTime) {
                    UpdateGameHandTextures(uiIsAI == 0, false);
                    pHandAnimManager[uiIsAI]->DeactivateSequence(pHandAnimations[uiIsAI][iSelectedDeckCard + 1], 0.0f);
                    pHandAnimManager[uiIsAI]
                        ->ActivateSequence(pHandAnimations[uiIsAI][1], 0, false, 1.0f, 0.0f, nullptr);
                    NiUpdateData ResetData1(0.0f, true, false);
                    pHandGeometry[uiIsAI]->Update(ResetData1);
                    pHandAnimManager[uiIsAI]->DeactivateSequence(pHandAnimations[uiIsAI][1], 0.0f);
                    UpdateCaravanFlags();
                    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                    pTiles[uiIsAI + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
                    uint32_t uiDeckSize = uiIsAI ? pNPCDeck.iSize : pPlayerDeck.iSize;
                    pTiles[uiIsAI + 29]->Set(CaravanMenu::iUT_Value, uiDeckSize);
                    eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                    iSelectedDeckCard = pPlayerHand.iSize - 1;
                    if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
                        pPlayerHand[iSelectedDeckCard]->data.iFaceValue > 10) {
                        SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                    }
                    if ((uiIsAI == 1 && !this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) ||
                        (!uiIsAI && this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP))) {
                        SelectDeckCard(false);
                    }
                }
            }
            return;
        }
        case CaravanMenu::CA_GAME_DISCARDTRACK_ANIM: {
            if (pTrackAnimations[iSelectedTrack][0][0][1]) {
                int32_t iCnt = 0;
                if (CaravanMenu::fTimeDelta == 0.0) {
                    iSelectedColumn = 4;
                    while (!iCnt && iSelectedColumn > 0) {
                        --iSelectedColumn;
                        for (int32_t i = 0; i < 7; ++i) {
                            int32_t iSize = pCardTracks[iSelectedTrack][i].iSize;
                            if (iSize > iSelectedColumn) {
                                pTrackAnimManager[iSelectedTrack][i]->ActivateSequence(
                                    pTrackAnimations[iSelectedTrack][i][iSelectedColumn][1],
                                    0,
                                    false,
                                    1.0f,
                                    0.0f,
                                    nullptr
                                );
                                ++iCnt;
                            }
                        }
                    }
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                    fSelectionDelay = 0.0f;
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanRemoveCard", 289u).Play(false);
                }
                for (int32_t i = 0; i < 7 && pCardTracks[iSelectedTrack][i].iSize; ++i) {
                    NiUpdateData UpdateData(CaravanMenu::fTimeDelta, 1, 0);
                    pTrackGeometry[iSelectedTrack][i]->Update(UpdateData);
                }
                if (static_cast<double>(CaravanMenu::fTimeDelta) >
                    pTrackAnimations[iSelectedTrack][0][iSelectedColumn][1]->m_fEndKeyTime) {
                    for (int32_t i = 0; i < 7; ++i) {
                        pTrackAnimManager[iSelectedTrack][i]->DeactivateSequence(
                            pTrackAnimations[iSelectedTrack][i][iSelectedColumn][1],
                            0.0f
                        );
                    }
                    while (!iCnt && iSelectedColumn > 0) {
                        --iSelectedColumn;
                        for (int32_t i = 0; i < 7; ++i) {
                            int32_t iSize = pCardTracks[iSelectedTrack][i].iSize;
                            if (iSize > iSelectedColumn) {
                                pTrackAnimManager[iSelectedTrack][i]->ActivateSequence(
                                    pTrackAnimations[iSelectedTrack][i][iSelectedColumn][1],
                                    0,
                                    false,
                                    1.0f,
                                    0.0f,
                                    nullptr
                                );
                                ++iCnt;
                            }
                        }
                    }
                    if (iCnt) {
                        CaravanMenu::fTimeDelta = 0.0f;
                    }
                    else {
                        for (int32_t i = 0; i < 7; ++i) {
                            pCardTracks[iSelectedTrack][i].Clear(true);
                        }
                        eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                        if (this->IsFlagSet(CaravanMenu::CAF_AI)) {
                            iSelectedDeckCard = pPlayerHand.iSize - 1;
                            SelectDeckCard(false);
                        }
                        UpdateTrackValue(iSelectedTrack);
                        SetFlag(CaravanMenu::CAF_DESTROY_TRACK, false);
                        SetFlag(CaravanMenu::CAF_SELECT_TRACK, false);
                    }
                }
            }
            return;
        }
        case CaravanMenu::CA_GAME_JACK_CLEAR_ANIM: {
            if (pTrackAnimations[iSelectedTrack][iSelectedRow][0][1]) {
                int32_t iCnt = 0;
                if (CaravanMenu::fTimeDelta == 0.0) {
                    iSelectedColumn = 4;
                    while (!iCnt && iSelectedColumn > 0) {
                        --iSelectedColumn;
                        for (int32_t i = iSelectedRow; i < 7; ++i) {
                            int32_t iSize = pCardTracks[iSelectedTrack][i].iSize;
                            if (iSize > iSelectedColumn) {
                                pTrackAnimManager[iSelectedTrack][i]->ActivateSequence(
                                    pTrackAnimations[iSelectedTrack][i][iSelectedColumn][1],
                                    0,
                                    false,
                                    1.0f,
                                    0.0f,
                                    nullptr
                                );
                                ++iCnt;
                            }
                        }
                    }
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                }
                for (int32_t i = iSelectedRow; i < 7 && pCardTracks[iSelectedTrack][i].iSize; ++i) {
                    NiUpdateData UpdateData(CaravanMenu::fTimeDelta, 1, 0);
                    pTrackGeometry[iSelectedTrack][i]->Update(UpdateData);
                }
                if (static_cast<double>(CaravanMenu::fTimeDelta) >
                    pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][1]->m_fEndKeyTime) {
                    for (int32_t i = iSelectedRow; i < 7; ++i) {
                        pTrackAnimManager[iSelectedTrack][i]->DeactivateSequence(
                            pTrackAnimations[iSelectedTrack][i][iSelectedColumn][1],
                            0.0f
                        );
                    }
                    while (!iCnt && iSelectedColumn > 0) {
                        --iSelectedColumn;
                        for (int32_t i = iSelectedRow; i < 7; ++i) {
                            int32_t iSize = pCardTracks[iSelectedTrack][i].iSize;
                            if (iSize > iSelectedColumn) {
                                pTrackAnimManager[iSelectedTrack][i]->ActivateSequence(
                                    pTrackAnimations[iSelectedTrack][i][iSelectedColumn][1],
                                    0,
                                    false,
                                    1.0f,
                                    0.0f,
                                    nullptr
                                );
                                ++iCnt;
                            }
                        }
                    }
                    if (iCnt) {
                        CaravanMenu::fTimeDelta = 0.0f;
                    }
                    else {
                        if (this->IsFlagSet(CaravanMenu::CAF_PROCESS_JOKER)) {
                            bool bContinue = true;
                            int32_t iSelRowTmp = iSelectedRow;
                            while (bContinue) {
                                for (int32_t i = iSelRowTmp; i < 6; ++i) {
                                    pCardTracks[iSelectedTrack][i].Clear(true);
                                    BSSimpleArray<TESCaravanCard*, 1024>* pCardArray = &pCardTracks[iSelectedTrack]
                                                                                                   [i + 1];
                                    for (int32_t j = 0; j < pCardArray->iSize; ++j) {
                                        pCardTracks[iSelectedTrack][i].Add((*pCardArray)[j]);
                                        UpdateTrackCardTexture(iSelectedTrack, i, j);
                                    }
                                }
                                jokerLocations[iSelectedTrack].Remove(0, true);
                                if (jokerLocations[iSelectedTrack].iSize) {
                                    int32_t iTmp = iSelRowTmp;
                                    iSelRowTmp = jokerLocations[iSelectedTrack][0];
                                    if (iSelRowTmp > iTmp) {
                                        --iSelRowTmp;
                                    }
                                }
                                else {
                                    bContinue = false;
                                }
                            }
                        }
                        else {
                            for (int32_t i = iSelectedRow; i < 6; ++i) {
                                pCardTracks[iSelectedTrack][i].Clear(true);
                                BSSimpleArray<TESCaravanCard*, 1024>* pCardArray = &pCardTracks[iSelectedTrack][i + 1];
                                for (int32_t j = 0; j < pCardArray->iSize; ++j) {
                                    pCardTracks[iSelectedTrack][i].Add((*pCardArray)[j]);
                                    UpdateTrackCardTexture(iSelectedTrack, i, j);
                                }
                            }
                            pCardTracks[iSelectedTrack][6].Clear(true);
                        }
                        eGameState = CaravanMenu::CA_GAME_JACK_FINISH_ANIM;
                    }
                }
            }
            return;
        }
        case CaravanMenu::CA_GAME_JACK_FINISH_ANIM: {
            if (pTrackAnimations[iSelectedTrack][iSelectedRow][0][0]) {
                int32_t iCnt = 0;
                bool bExit = false;
                if (CaravanMenu::fTimeDelta == 0.0) {
                    iSelectedColumn = -1;
                    while (!iCnt && iSelectedColumn < 4) {
                        ++iSelectedColumn;
                        for (int32_t i = iSelectedRow; i < 7; ++i) {
                            int32_t iSize = pCardTracks[iSelectedTrack][i].iSize;
                            if (iSize > iSelectedColumn) {
                                pTrackAnimManager[iSelectedTrack][i]->ActivateSequence(
                                    pTrackAnimations[iSelectedTrack][i][iSelectedColumn][0],
                                    0,
                                    false,
                                    1.0f,
                                    0.0f,
                                    nullptr
                                );
                                ++iCnt;
                            }
                        }
                    }
                    if (!iCnt) {
                        bExit = true;
                    }
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                }
                if (!bExit) {
                    for (int32_t i = iSelectedRow; i < 7 && pCardTracks[iSelectedTrack][i].iSize; ++i) {
                        NiUpdateData UpdateData(CaravanMenu::fTimeDelta, 1, 0);
                        pTrackGeometry[iSelectedTrack][i]->Update(UpdateData);
                    }
                }
                if (bExit ||
                    static_cast<double>(CaravanMenu::fTimeDelta) >
                        pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]->m_fEndKeyTime) {
                    if (!bExit) {
                        for (int32_t i = iSelectedRow; i < 7; ++i) {
                            pTrackAnimManager[iSelectedTrack][i]->DeactivateSequence(
                                pTrackAnimations[iSelectedTrack][i][iSelectedColumn][0],
                                0.0f
                            );
                        }
                        while (!iCnt && iSelectedColumn < 4) {
                            ++iSelectedColumn;
                            for (int32_t i = iSelectedRow; i < 7; ++i) {
                                int32_t iSize = pCardTracks[iSelectedTrack][i].iSize;
                                if (iSize > iSelectedColumn) {
                                    pTrackAnimManager[iSelectedTrack][i]->ActivateSequence(
                                        pTrackAnimations[iSelectedTrack][i][iSelectedColumn][0],
                                        0,
                                        false,
                                        1.0f,
                                        0.0f,
                                        nullptr
                                    );
                                    ++iCnt;
                                }
                            }
                        }
                    }
                    if (iCnt) {
                        CaravanMenu::fTimeDelta = 0.0f;
                    }
                    else {
                        UpdateTrackValue(iSelectedTrack);
                        if (this->IsFlagSet(CaravanMenu::CAF_PROCESS_JOKER)) {
                            eGameState = CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM;
                        }
                        else {
                            eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                        }
                    }
                }
                UpdateCaravanFlags();
            }
            return;
        }
        case CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM: {
            SetFlag(CaravanMenu::CAF_PROCESS_JOKER, true);
            while (iSelectedTrack < 6 && !jokerLocations[iSelectedTrack].iSize) {
                ++iSelectedTrack;
            }
            if (iSelectedTrack < 6) {
                iSelectedRow = jokerLocations[iSelectedTrack][0];
                eGameState = CaravanMenu::CA_GAME_JACK_CLEAR_ANIM;
            }
            else {
                eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
                SetFlag(CaravanMenu::CAF_PROCESS_JOKER, false);
            }
            return;
        }
        case CaravanMenu::CA_GAME_PROCESS_AI: {
            SetFlag(CaravanMenu::CAF_AI, true);
            if (IsGameOver()) {
                eGameState = CaravanMenu::CA_GAME_GAMEOVER;
            }
            else if (bContinueProcessing) {
                bContinueProcessing = false;
                ProcessAI(&aiPackage);
                if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
                    if (aiPackage.iActionType == 6) {
                        iSelectedDeckCard = aiPackage.iDeckCard;
                        pSelectedCard = pNPCHand[iSelectedDeckCard];
                        TESCaravanCard* pDeckCard = pNPCDeck[this->iNextNPCCard];
                        pNPCHand.Remove(iSelectedDeckCard, true);
                        pNPCHand.Add(pDeckCard);
                        pNPCDeck.Remove(this->iNextNPCCard, true);
                        this->iNextNPCCard = RandomInt(0, pNPCDeck.iSize);
                        UpdateDealCardTexture(false);
                        bContinueProcessing = true;
                        eGameState = CaravanMenu::CA_GAME_INTRO_DISCARD_ANIM;
                    }
                    else {
                        int32_t iTrack;
                        for (iTrack = 5; iTrack > 2 && pCardTracks[iTrack]->iSize; --iTrack) {
                            ;
                        }
                        iSelectedTrack = iTrack;
                        iSelectedDeckCard = aiPackage.iDeckCard;
                        iSelectedRow = 0;
                        iSelectedColumn = pCardTracks[iTrack][iSelectedRow].iSize;
                        pSelectedCard = pNPCHand[iSelectedDeckCard];
                        aiPackage.iTrack = iTrack;
                        aiPackage.iRow = 0;
                        if (CaravanMenu::bDebugAI) {
                            aiPackage.Print_WhatIsHeThinking(&pNPCHand);
                        }
                        TESCaravanCard* pTrackCard = pNPCHand[iSelectedDeckCard];
                        pNPCHand.Remove(iSelectedDeckCard, true);
                        pCardTracks[iTrack][iSelectedRow].Add(pTrackCard);
                        eGameState = CaravanMenu::CA_GAME_INTRO_DEAL_ANIM;
                    }
                }
                else {
                    TESCaravanCard* pHandCard;
                    if (aiPackage.iActionType == 6) {
                        if (this->iNextNPCCard == -1) {
                            iSelectedDeckCard = aiPackage.iDeckCard;
                            // The discarded card is kept in a local that is never read again.
                            pHandCard = pNPCHand[iSelectedDeckCard];
                            pNPCHand.Remove(iSelectedDeckCard, true);
                            UpdateDealCardTexture(false);
                        }
                        else {
                            iSelectedDeckCard = aiPackage.iDeckCard;
                            TESCaravanCard* pDeckCard = pNPCDeck[this->iNextNPCCard];
                            pNPCHand.Remove(iSelectedDeckCard, true);
                            pNPCHand.Add(pDeckCard);
                            pNPCDeck.Remove(this->iNextNPCCard, true);
                            if (pNPCDeck.iSize) {
                                this->iNextNPCCard = RandomInt(0, pNPCDeck.iSize);
                            }
                            else {
                                this->iNextNPCCard = -1;
                            }
                            UpdateDealCardTexture(false);
                        }
                        eGameState = CaravanMenu::CA_GAME_DISCARDCARD_ANIM;
                    }
                    else if (aiPackage.iActionType == 5) {
                        iSelectedTrack = aiPackage.iTrack;
                        trackInfo[iSelectedTrack].iValue = 0;
                        eGameState = CaravanMenu::CA_GAME_DISCARDTRACK_ANIM;
                    }
                    else {
                        if (this->iNextNPCCard == -1) {
                            NiAVObject* pDeck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_06:5");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_Back:0");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_06_Shadow:0");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_Back_Shadow:0");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            iSelectedTrack = aiPackage.iTrack;
                            iSelectedRow = aiPackage.iRow;
                            iSelectedColumn = pCardTracks[iSelectedTrack][iSelectedRow].iSize;
                            iSelectedDeckCard = aiPackage.iDeckCard;
                            pSelectedCard = pNPCHand[iSelectedDeckCard];
                            TESCaravanCard* pTrackCard = pNPCHand[iSelectedDeckCard];
                            pNPCHand.Remove(iSelectedDeckCard, true);
                            if (CaravanMenu::bDebugAI) {
                                aiPackage.Print_WhatIsHeThinking(&pNPCHand);
                            }
                            pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard);
                        }
                        else {
                            iSelectedTrack = aiPackage.iTrack;
                            iSelectedRow = aiPackage.iRow;
                            iSelectedColumn = pCardTracks[iSelectedTrack][iSelectedRow].iSize;
                            iSelectedDeckCard = aiPackage.iDeckCard;
                            pSelectedCard = pNPCHand[iSelectedDeckCard];
                            TESCaravanCard* pDeckCard = pNPCDeck[this->iNextNPCCard];
                            pHandCard = pNPCHand[iSelectedDeckCard];
                            pNPCHand.Remove(iSelectedDeckCard, true);
                            pNPCHand.Add(pDeckCard);
                            if (CaravanMenu::bDebugAI) {
                                aiPackage.Print_WhatIsHeThinking(&pNPCHand);
                            }
                            pNPCDeck.Remove(this->iNextNPCCard, true);
                            if (pNPCDeck.iSize) {
                                this->iNextNPCCard = RandomInt(0, pNPCDeck.iSize);
                            }
                            else {
                                this->iNextNPCCard = -1;
                            }
                            pCardTracks[iSelectedTrack][iSelectedRow].Add(pHandCard);
                            UpdateDealCardTexture(false);
                        }
                        eGameState = CaravanMenu::CA_GAME_PLACECARD_ANIM;
                    }
                }
            }
            else {
                iSelectedDeckCard = pPlayerHand.iSize - 1;
                iSelectedTrack = 0;
                pSelectedCard = nullptr;
                pSelectedObject = nullptr;
                SelectDeckCard(false);
                bContinueProcessing = !bContinueProcessing;
                eGameState = CaravanMenu::CA_IDLE;
                SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                UpdateCaravanFlags();
                UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                SetFlag(CaravanMenu::CAF_AI, false);
            }
            return;
        }
        case CaravanMenu::CA_GAME_GAMEOVER: {
            SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
            PrepareResultsMenu();
            eGameState = CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM;
            return;
        }
        case CaravanMenu::CA_CLOSING_MENU: {
            if (xFadeState != Interface::FADING_OUT) {
                InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
                pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
                StartFadeOut();
                CaravanMenu::pMe = nullptr;
                if (bStarted) {
                    InstantFadeOut();
                }
                bStarted = 1;
            }
            return;
        }
        default: {
            return;
        }
    }
}

bool CaravanMenu::DoKey(int32_t aiKey) {
    uiPendingArrowKey = 0;
    bool bHandled = false;
    if (!InterfaceManager::Has360Controller()) {
        if (eActiveMenu == 3) {
            Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
            if (Interface::IsTopMenuID(eMenu)) {
                CaravanMenu::Close();
                return true;
            }
        }
        if (aiKey > 65) {
            switch (aiKey) {
                case 69:
                case 101: {
                    bHandled = DoGamepad(14, 0.0f);
                    break;
                }
                case 70:
                case 102: {
                    bHandled = DoGamepad(12, 0.0f);
                    break;
                }
                case 81:
                case 113: {
                    bHandled = DoGamepad(13, 0.0f);
                    break;
                }
                case 82:
                case 114: {
                    bHandled = DoGamepadUpEvent(10, 0.0f);
                    break;
                }
                case 83:
                case 115: {
                    bHandled = DoGamepad(16, 0.0f);
                    break;
                }
                case 87:
                case 119: {
                    bHandled = DoGamepad(9, 0.0f);
                    break;
                }
                case 97: {
                    return DoGamepad(11, 0.0f);
                }
                default: {
                    break;
                }
            }
        }
        else {
            if (aiKey == 65) {
                return DoGamepad(11, 0.0f);
            }

            if (aiKey != 0x80000000u && aiKey <= -0x7FFFFFFC && !eGameState) {
                uiPendingArrowKey = aiKey;
            }
        }
    }
    return bHandled;
}

int32_t CaravanMenu::GetClass() {
    return 1083;
}

bool CaravanMenu::DoGamepad(int32_t aiButton, float afValue) {
    int32_t i;
    using randomints_t = BSSimpleArray<int32_t, 1024>;
    bool bHandled = false;
    if (IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS) || bTutorial || !IsFlagSet(CaravanMenu::CAF_ISONTOP)) {
        return eGameState == CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM;
    }
    if (IsFlagSet(CaravanMenu::CAF_AI) || eGameState) {
        return false;
    }
    switch (aiButton) {
        case 5:
        case 6:
        case 17:
        case 18: {
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            break;
        }
        case 9: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_A)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (eActiveMenu == CaravanMenu::CA_ANTE_MENU) {
                int32_t iPreviousAnte = iPlayerAnte;
                iPlayerAnte = BSMIN(iNPCAnte, iTotalFunds);
                pTiles[5]->Poke(CaravanMenu::iUT_Value, 1.0f);
                pTiles[5]->Set(CaravanMenu::iUT_Value, CaravanMenu::pMe->iPlayerAnte);
                bHandled = true;
                double dBaseBet = static_cast<double>(CaravanMenu::pMe->iNPCTotalFunds) *
                    CaravanMenu::pMe->fBetPercentage;
                int32_t iMaxBet = static_cast<int32_t>(
                    pPlayer->GetClampedActorFloatValue(ActorValue::BARTER) / 100.0 * dBaseBet + dBaseBet
                );
                UpdateBettingUI(iMaxBet, iPlayerAnte - iPreviousAnte);
                UpdateCaravanFlags();
                UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                break;
            }
            if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
                ++iCardsInDeck;
                pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
                pTiles[8]->Set(CaravanMenu::iUT_Value, iCardsInDeck);
                int32_t iIndex = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
                pDeckArray[iIndex]->bIsActive = true;
                bHandled = true;
                eGameState = CaravanMenu::CA_DECK_FLIP_ANIM;
                break;
            }
            if (eActiveMenu != CaravanMenu::CA_GAME_MENU) {
                break;
            }
            if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) || IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
                if (!IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
                    BSAudio::QInstance().GetSoundHandleByName("GAMECaravanAddToTrack", 289u).Play(false);
                    SetFlag(CaravanMenu::CAF_SELECT_TRACK, true);
                    iSelectedTrack = 0;
                    iSelectedRow = 0;
                    iSelectedColumn = 0;
                    HandleTrackSelection(0, 0);
                    bHandled = true;
                    break;
                }
                if (IsFlagSet(CaravanMenu::CAF_DESTROY_TRACK)) {
                    DeselectWholeTrack(iSelectedTrack);
                    trackInfo[iSelectedTrack].iValue = 0;
                    eGameState = CaravanMenu::CA_GAME_DISCARDTRACK_ANIM;
                    return true;
                }
                if (IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
                    if (IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
                        SetFlag(CaravanMenu::CAF_SELECT_TRACK, false);
                        DeselectTrackCard();
                        eGameState = CaravanMenu::CA_GAME_INTRO_DEAL_ANIM;
                        TESCaravanCard* pTrackCard = pPlayerHand[iSelectedDeckCard];
                        pPlayerHand.Remove(iSelectedDeckCard, true);
                        pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard);
                    }
                    else {
                        SetFlag(CaravanMenu::CAF_SELECT_TRACK, false);
                        DeselectTrackCard();
                        eGameState = CaravanMenu::CA_GAME_PLACECARD_ANIM;
                        if (iNextPlayerCard == -1) {
                            NiAVObject* pDeck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_06_Back:0");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_06:5");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_06:5");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_Back:0");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                            pDeck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_Back_Shadow:0");
                            if (pDeck) {
                                pDeck->SetAppCulled(true);
                                NiUpdateData UpdateData(0.0f, false, false);
                                pDeck->Update(UpdateData);
                            }
                        }
                        if (iNextPlayerCard == -1) {
                            TESCaravanCard* pTrackCard = pPlayerHand[iSelectedDeckCard];
                            pPlayerHand.Remove(iSelectedDeckCard, true);
                            pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard);
                        }
                        else {
                            TESCaravanCard* pDeckCard = pPlayerDeck[iNextPlayerCard];
                            TESCaravanCard* pTrackCard = pPlayerHand[iSelectedDeckCard];
                            pPlayerHand.Remove(iSelectedDeckCard, true);
                            pPlayerHand.Add(pDeckCard);
                            pPlayerDeck.Remove(iNextPlayerCard, true);
                            pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard);
                            if (pPlayerDeck.iSize) {
                                iNextPlayerCard = RandomInt(0, pPlayerDeck.iSize);
                            }
                            else {
                                iNextPlayerCard = -1;
                            }
                            UpdateDealCardTexture(true);
                        }
                    }
                    bHandled = true;
                }
            }
            else {
                iSelectedTrack = 0;
                iSelectedRow = 0;
                iSelectedColumn = 0;
                while (pCardTracks[iSelectedTrack]->iSize) {
                    ++iSelectedTrack;
                }
                if (iSelectedTrack < 3) {
                    SelectTrackCard(iSelectedTrack, iSelectedRow);
                }
                SelectDeckCard(true);
                BSAudio::QInstance().GetSoundHandleByName("GAMECaravanAddToTrack", 289u).Play(false);
                UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                SetFlag(CaravanMenu::CAF_SELECT_TRACK, true);
                bHandled = true;
            }
            break;
        }
        case 11: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_X)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (eActiveMenu) {
                if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
                    --iCardsInDeck;
                    pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
                    pTiles[8]->Set(CaravanMenu::iUT_Value, iCardsInDeck);
                    uint32_t uiIndex = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
                    pDeckArray[uiIndex]->bIsActive = false;
                    bHandled = true;
                    eGameState = CaravanMenu::CA_DECK_FLIP_ANIM;
                }
            }
            else {
                SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                UpdateCaravanFlags();
                UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                if (iTotalFunds - iPlayerAnte > 0) {
                    QuantityMenu::Create(
                        iTotalFunds - iPlayerAnte,
                        &CaravanMenu::ItemSelectCallback,
                        std::numeric_limits<int32_t>::max()
                    );
                }
                bHandled = true;
            }
            break;
        }
        case 12: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (eActiveMenu) {
                if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
                    if (iCardsInDeck >= 30) {
                        eGameState = CaravanMenu::CA_DECK_WIPE_ANIM;
                    }
                    bHandled = true;
                }
            }
            else {
                if (iPlayerAnte > 0 || !iNPCAnte) {
                    LoadNifs();
                    BSAudio::QInstance().GetSoundHandleByName("UIMenuOK", 289u).Play(false);
                }
                bHandled = true;
            }
            break;
        }
        case 13: {
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_LT)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_GAME_MENU &&
                !IsFlagSet(CaravanMenu::CAF_SELECT_TRACK) &&
                pPlayerDeck.iSize) {
                eGameState = static_cast<CaravanMenu::CaravanState>(
                    IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) ? CaravanMenu::CA_GAME_INTRO_DISCARD_ANIM
                                                            : CaravanMenu::CA_GAME_DISCARDCARD_ANIM
                );
                if (iNextPlayerCard == -1) {
                    pPlayerHand.Remove(iSelectedDeckCard, true);
                }
                else {
                    TESCaravanCard* pDeckCard = pPlayerDeck[iNextPlayerCard];
                    pPlayerHand.Remove(iSelectedDeckCard, true);
                    pPlayerHand.Add(pDeckCard);
                    pPlayerDeck.Remove(iNextPlayerCard, true);
                    if (pPlayerDeck.iSize) {
                        iNextPlayerCard = RandomInt(0, pPlayerDeck.iSize);
                    }
                    else {
                        iNextPlayerCard = -1;
                    }
                    UpdateDealCardTexture(true);
                }
            }

            break;
        }
        case 14: {
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_RT)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_GAME_MENU &&
                !IsFlagSet(CaravanMenu::CAF_SELECT_TRACK) &&
                !IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
                SetFlag(CaravanMenu::CAF_DESTROY_TRACK, true);
                SetFlag(CaravanMenu::CAF_SELECT_TRACK, true);
                iSelectedTrack = 0;
                SelectWholeTrack(iSelectedTrack);
            }
            break;
        }
        case 15: {
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_LB)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
                bHandled = true;
            }
            break;
        }
        case 16: {
            if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
                bHandled = true;
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    CaravanMenu::Close();
                }
            }
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_RB)) {
                return false;
            }
            if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
                int32_t iNumCards;
                randomints_t kRandomInts;
                int32_t iSize = pDeckArray.iSize;
                for (i = 0; i < iSize; ++i) {
                    kRandomInts.Add(i);
                    pDeckArray[i]->bIsActive = false;
                    iCardsInDeck = 0;
                }
                if (iSize <= 30) {
                    iNumCards = 30;
                }
                else {
                    iNumCards = RandomInt(30, iSize);
                }
                for (i = 0; i < iNumCards; ++i) {
                    int32_t j = RandomInt(0, kRandomInts.iSize);
                    int32_t k = kRandomInts[j];
                    kRandomInts.Remove(j, true);
                    if (!pDeckArray[k]->bIsActive) {
                        ++iCardsInDeck;
                        pDeckArray[k]->bIsActive = true;
                    }
                }
                kRandomInts.Clear(true);
                pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
                pTiles[8]->Set(CaravanMenu::iUT_Value, iCardsInDeck);
                UpdateCaravanFlags();
                UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
                double dCurrentValue = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
                SwapDeckBuildingTextures(static_cast<int32_t>(dCurrentValue), false);
                bHandled = true;
            }
            break;
        }
        default: {
            break;
        }
    }
    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
    return bHandled;
}

bool CaravanMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS) || aiButton != 10) {
        return false;
    }
    if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
        Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
        if (Interface::IsTopMenuID(eMenu)) {
            CaravanMenu::Close();
        }
    }
    switch (eActiveMenu) {
        case CaravanMenu::CA_ANTE_MENU: {
            Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
            if (Interface::IsTopMenuID(eMenu)) {
                CaravanMenu::Close();
            }
            appMain->GetControls()->ClearUserActions();
            break;
        }
        case CaravanMenu::CA_DECK_MENU: {
            Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
            if (Interface::IsTopMenuID(eMenu)) {
                CaravanMenu::Close();
            }
            appMain->GetControls()->ClearUserActions();
            break;
        }
        case CaravanMenu::CA_GAME_MENU: {
            if (IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
                if (IsFlagSet(CaravanMenu::CAF_DESTROY_TRACK)) {
                    DeselectWholeTrack(iSelectedTrack);
                }
                else {
                    DeselectTrackCard();
                }
                SetFlag(CaravanMenu::CAF_DESTROY_TRACK, false);
                SetFlag(CaravanMenu::CAF_SELECT_TRACK, false);
                SetFlag(CaravanMenu::CAF_DISABLE_A, false);
                SetFlag(CaravanMenu::CAF_DISABLE_X, true);
                UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
            }
            else {
                Interface::enumMenus eMenu = static_cast<Interface::enumMenus>(GetClass());
                if (Interface::IsTopMenuID(eMenu)) {
                    const char* pNoText = sNo.String();
                    const char* pYesText = sYes.String();
                    Interface::CreateMessageMenu(
                        sQuitCaravanText.String(),
                        static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr),
                        &CaravanMenu::CancelConfirmFunc,
                        0,
                        23,
                        0.0f,
                        0.0f,
                        pYesText,
                        pNoText,
                        nullptr
                    );
                    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    InterfaceManager::Instance();
    InterfaceManager::ClearMenuButton(Interface::eButtonB);
    return true;
}

bool CaravanMenu::IsFlagSet(enumFlags aeFlag) {
    return (aeFlag & iFlags) != 0;
}

// A track pair is decided once one side busts or the other side's value is higher; the game is over when all three
// pairs are decided with a majority, or when a side has no cards left in its deck and hand.
bool CaravanMenu::IsGameOver() {
    int32_t iPlayerTracks = 0;
    int32_t iDecidedTracks = 0;
    int32_t iNPCTracks = 0;
    bool bPlayerWon = false;
    bool bNPCWon = false;
    for (int32_t i = 0; i < 6; ++i) {
        UpdateTrackValue(i);
    }
    for (int32_t j = 0; j < 3; ++j) {
        if (!CalculateWinningState(trackInfo[j].iValue) &&
            (CalculateWinningState(trackInfo[5 - j].iValue) || trackInfo[j].iValue > trackInfo[5 - j].iValue)) {
            ++iPlayerTracks;
            ++iDecidedTracks;
        }
        if (!CalculateWinningState(trackInfo[5 - j].iValue) &&
            (CalculateWinningState(trackInfo[j].iValue) || trackInfo[j].iValue < trackInfo[5 - j].iValue)) {
            ++iNPCTracks;
            ++iDecidedTracks;
        }
        if (iPlayerTracks > iNPCTracks && iDecidedTracks == 3) {
            bPlayerWon = true;
        }
        if (iPlayerTracks < iNPCTracks && iDecidedTracks == 3) {
            bNPCWon = true;
        }
    }
    if (bPlayerWon) {
        SetFlag(CAF_PLAYERWIN, true);
        return true;
    }
    if (bNPCWon) {
        SetFlag(CAF_PLAYERWIN, false);
        return true;
    }
    if (!pPlayerDeck.GetSize() && !pPlayerHand.GetSize()) {
        SetFlag(CAF_PLAYERWIN, false);
        return true;
    }
    if (!pNPCDeck.GetSize() && !pNPCHand.GetSize()) {
        SetFlag(CAF_PLAYERWIN, true);
        return true;
    }
    return false;
}

void CaravanMenu::SetFlag(enumFlags aeFlag, bool abValue) {
    if (abValue) {
        this->iFlags |= aeFlag;
    }
    else {
        this->iFlags &= ~aeFlag;
    }
}

// Calls itself for the last tile of each tileset and then returns; it does not tail-jump.
[[clang::disable_tail_calls]] void CaravanMenu::UpdateTileAlpha(CaravanMenu::TileIndex eTile) {
    if (eTile == CaravanMenu::TI_NO_TILE) {
        // The active menu's value is its tileset's index (CA_ANTE_MENU is TI_ANTE_MENU_TILESET, ...).
        eTile = static_cast<CaravanMenu::TileIndex>(eActiveMenu);
    }
    switch (eTile) {
        case CaravanMenu::TI_ANTE_MENU_TILESET: {
            UpdateTileAlpha(CaravanMenu::TI_ANTE_ACCEPT_BUTTON);
            UpdateTileAlpha(CaravanMenu::TI_ANTE_AUTOMATCH_BUTTON);
            UpdateTileAlpha(CaravanMenu::TI_ANTE_RAISE_BUTTON);
            return;
        }
        case CaravanMenu::TI_DECK_MENU_TILESET: {
            UpdateTileAlpha(CaravanMenu::TI_DECK_CARDS);
            UpdateTileAlpha(CaravanMenu::TI_DECK_TOTAL_CARDS);
            UpdateTileAlpha(CaravanMenu::TI_DECK_ALLCARDS_BUTTON);
            UpdateTileAlpha(CaravanMenu::TI_DECK_CARAVANDECK_BUTTON);
            UpdateTileAlpha(CaravanMenu::TI_DECK_ADD_BUTTON);
            UpdateTileAlpha(CaravanMenu::TI_DECK_REMOVE_BUTTON);
            UpdateTileAlpha(CaravanMenu::TI_DECK_PLAY_BUTTON);
            return;
        }
        case CaravanMenu::TI_GAME_MENU_TILESET: {
            UpdateTileAlpha(CaravanMenu::TI_GAME_DISCARD_CARD);
            UpdateTileAlpha(CaravanMenu::TI_GAME_DISCARD_TRACK);
            UpdateTileAlpha(CaravanMenu::TI_GAME_SELECT_CARD);
            UpdateTileAlpha(CaravanMenu::TI_GAME_CANCEL);
            return;
        }
        case CaravanMenu::TI_ANTE_ACCEPT_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_DECK_CARDS: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
                if (iCardsInDeck >= 30) {
                    pTiles[eTile]->xChildren.GetHead()->Set(4009, 255.0f, true);
                }
                else {
                    pTiles[eTile]->xChildren.GetHead()->Set(4009, 128.0f, true);
                }
            }
            break;
        }
        case CaravanMenu::TI_DECK_TOTAL_CARDS:
        case CaravanMenu::TI_DECK_ALLCARDS_BUTTON: {
            SetTileTextAlpha(eTile, 255.0f);
            break;
        }
        case CaravanMenu::TI_DECK_CARAVANDECK_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_RB)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_DECK_ADD_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_A) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_DECK_REMOVE_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_X) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_DECK_PLAY_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_GAME_SELECT_CARD: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_A) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_GAME_DISCARD_CARD: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_LT) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_GAME_DISCARD_TRACK: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_RT) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_GAME_CANCEL: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_B) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_ANTE_RAISE_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_X) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        case CaravanMenu::TI_ANTE_AUTOMATCH_BUTTON: {
            if (IsFlagSet(CaravanMenu::CAF_DISABLE_A) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
                SetTileTextAlpha(eTile, 128.0f);
            }
            else {
                SetTileTextAlpha(eTile, 255.0f);
            }
            break;
        }
        default: {
            break;
        }
    }
}

// Sets the alpha trait of the child tiles of pTiles[aeTile].
void CaravanMenu::SetTileTextAlpha(TileIndex TI, float alpha) {
    Tile* pTile = CaravanMenu::pMe->pTiles[TI];
    NiTPointerListBase<Tile*>* pChildren = &pTile->xChildren;
    NiTListItem<Tile*>* pos = pTile->xChildren.m_pkHead;
    while (pos && pChildren && pChildren->GetHead() != nullptr) {
        pChildren->Get(pos)->Set(Tile::kTileValue_Alpha, alpha, true);
        pos = pChildren->GetNextPos(pos);
    }
}

void CaravanMenu::UpdateCaravanFlags() {
    CaravanMenu::ActiveMenu eActiveMenu = this->eActiveMenu;
    if (eActiveMenu) {
        if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
            uint32_t uiIndex = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
            // bIsActive is tested as a whole byte: read it as a byte, or clang merges the branches and passes a non-0/1
            // bool on to SetFlag.
            if (pDeckArray[uiIndex]->bIsActive != 0) {
                SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                SetFlag(CaravanMenu::CAF_DISABLE_X, false);
            }
            else {
                SetFlag(CaravanMenu::CAF_DISABLE_A, false);
                SetFlag(CaravanMenu::CAF_DISABLE_X, true);
            }
            if (iCardsInDeck >= 30) {
                SetFlag(CaravanMenu::CAF_DISABLE_Y, false);
                pTiles[8]->Set(4009, 255.0f, true);
            }
            else {
                SetFlag(CaravanMenu::CAF_DISABLE_Y, true);
                pTiles[8]->Set(4009, 128.0f, true);
            }
            if (!iCardsInDeck) {
                SetFlag(CaravanMenu::CAF_DISABLE_X, true);
            }
            if (iCardsInDeck >= 108) {
                SetFlag(CaravanMenu::CAF_DISABLE_A, true);
            }
            SetFlag(CaravanMenu::CAF_DISABLE_LB, true);
        }
        else if (eActiveMenu == CaravanMenu::CA_GAME_MENU) {
            if (!pPlayerDeck.iSize) {
                iNextPlayerCard = -1;
                SetFlag(CaravanMenu::CAF_DISABLE_LT, true);
            }
            if (!pNPCDeck.iSize) {
                iNextNPCCard = -1;
            }
            if ((pCardTracks[0][0].iSize || pCardTracks[1][0].iSize || pCardTracks[2][0].iSize) &&
                !IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
                SetFlag(CaravanMenu::CAF_DISABLE_RT, false);
            }
            else {
                SetFlag(CaravanMenu::CAF_DISABLE_RT, true);
            }
        }
    }
    else {
        if (iPlayerAnte <= 0 && iNPCAnte) {
            SetFlag(CaravanMenu::CAF_DISABLE_Y, true);
        }
        else {
            SetFlag(CaravanMenu::CAF_DISABLE_Y, false);
        }
        if (iTotalFunds) {
            if (iPlayerAnte <= 0) {
                SetFlag(CaravanMenu::CAF_DISABLE_A, false);
                SetFlag(CaravanMenu::CAF_DISABLE_X, false);
            }
            else {
                SetFlag(CaravanMenu::CAF_DISABLE_A, false);
                SetFlag(CaravanMenu::CAF_DISABLE_X, false);
                if (iPlayerAnte >= iNPCAnte || (iPlayerAnte >= iTotalFunds && iPlayerAnte < iNPCAnte)) {
                    SetFlag(CaravanMenu::CAF_DISABLE_A, true);
                }
                if (iPlayerAnte >= iTotalFunds) {
                    SetFlag(CaravanMenu::CAF_DISABLE_X, true);
                }
            }
        }
        else {
            SetFlag(CaravanMenu::CAF_DISABLE_A, true);
            SetFlag(CaravanMenu::CAF_DISABLE_X, true);
        }
    }
}

void CaravanMenu::UpdateBettingUI(int32_t maxraise, int32_t raise) {
    NiMatrix3 kCoinRotate;
    NiMatrix3 kBillRotate;
    char blockName[264];
    int32_t iBills = 0;
    int32_t iCoins = 0;
    if (maxraise >= 120) {
        if (maxraise >= 400) {
            if (maxraise >= 1000) {
                if (maxraise >= 2000) {
                    while (raise > 0 && iBills < 9 && iCoins < 42) {
                        int32_t iBillChance = RandomInt(0, 9 - iBills);
                        int32_t iCoinChance = RandomInt(0, 42 - iCoins);
                        int32_t iIndex = 0;
                        if (iBillChance <= iCoinChance) {
                            int32_t iAmount;
                            int32_t iCoinIndex;
                            iIndex = RandomInt(0, 100);
                            if (iIndex >= 10) {
                                iCoinIndex = 2;
                            }
                            else {
                                iCoinIndex = iIndex >= 5;
                            }
                            iIndex = iCoinIndex;
                            LoadNewCoin(iCoinIndex);
                            if (iCoinIndex >= 2) {
                                iAmount = 100;
                            }
                            else {
                                iAmount = iIndex != 0 ? 4 : 1;
                            }
                            raise -= iAmount;
                            ++iCoins;
                        }
                        else {
                            iIndex = 2;
                            LoadNewBill(2);
                            raise -= 40;
                            ++iBills;
                        }
                    }
                }
                else {
                    while (raise > 0 && iBills < 9 && iCoins < 42) {
                        int32_t iBillChance = RandomInt(0, 9 - iBills);
                        int32_t iCoinChance = RandomInt(0, 42 - iCoins);
                        int32_t iIndex = 0;
                        if (static_cast<double>(iCoinChance) >= static_cast<double>(iBillChance) * 1.5) {
                            int32_t iAmount;
                            int32_t iCoinIndex;
                            iIndex = RandomInt(0, 100);
                            if (iIndex >= 20) {
                                iCoinIndex = 2;
                            }
                            else {
                                iCoinIndex = iIndex >= 5;
                            }
                            iIndex = iCoinIndex;
                            LoadNewCoin(iCoinIndex);
                            if (iCoinIndex >= 2) {
                                iAmount = 100;
                            }
                            else {
                                iAmount = iIndex != 0 ? 4 : 1;
                            }
                            raise -= iAmount;
                            ++iCoins;
                        }
                        else {
                            int32_t iAmount;
                            iIndex = RandomInt(1, 3);
                            LoadNewBill(iIndex);
                            if (iIndex >= 2) {
                                iAmount = 40;
                            }
                            else {
                                iAmount = iIndex != 0 ? 8 : 2;
                            }
                            raise -= iAmount;
                            ++iBills;
                        }
                    }
                }
            }
            else {
                while (raise > 0 && iBills < 9 && iCoins < 42) {
                    int32_t iBillChance = RandomInt(0, 9 - iBills);
                    int32_t iCoinChance = RandomInt(0, 42 - iCoins);
                    int32_t iIndex = 0;
                    if (static_cast<double>(iCoinChance) >= static_cast<double>(iBillChance) * 1.5) {
                        int32_t iAmount;
                        int32_t iCoinIndex;
                        iIndex = RandomInt(0, 100);
                        if (iIndex >= 66) {
                            iCoinIndex = 2;
                        }
                        else {
                            iCoinIndex = iIndex >= 33;
                        }
                        iIndex = iCoinIndex;
                        LoadNewCoin(iCoinIndex);
                        if (iCoinIndex >= 2) {
                            iAmount = 100;
                        }
                        else {
                            iAmount = iIndex != 0 ? 4 : 1;
                        }
                        raise -= iAmount;
                        ++iCoins;
                    }
                    else {
                        int32_t iAmount;
                        iIndex = RandomInt(1, 3);
                        LoadNewBill(iIndex);
                        if (iIndex >= 2) {
                            iAmount = 40;
                        }
                        else {
                            iAmount = iIndex != 0 ? 8 : 2;
                        }
                        raise -= iAmount;
                        ++iBills;
                    }
                }
            }
        }
        else {
            while (raise > 0 && iBills < 9 && iCoins < 42) {
                int32_t iBillChance = RandomInt(0, 9 - iBills);
                int32_t iCoinChance = RandomInt(0, 42 - iCoins);
                int32_t iIndex = 0;
                if (static_cast<double>(iCoinChance) >= static_cast<double>(iBillChance) * 1.5) {
                    int32_t iAmount;
                    int32_t iCoinIndex;
                    iIndex = RandomInt(0, 100);
                    if (iIndex >= 75) {
                        iCoinIndex = 2;
                    }
                    else {
                        iCoinIndex = iIndex >= 33;
                    }
                    iIndex = iCoinIndex;
                    CaravanMenu::pMe->LoadNewCoin(iCoinIndex);
                    if (iCoinIndex >= 2) {
                        iAmount = 100;
                    }
                    else {
                        iAmount = iIndex != 0 ? 4 : 1;
                    }
                    raise -= iAmount;
                    ++iCoins;
                }
                else {
                    int32_t iAmount;
                    iIndex = RandomInt(0, 3);
                    CaravanMenu::pMe->LoadNewBill(iIndex);
                    if (iIndex >= 2) {
                        iAmount = 40;
                    }
                    else {
                        iAmount = iIndex != 0 ? 8 : 2;
                    }
                    raise -= iAmount;
                    ++iBills;
                }
            }
        }
    }
    else {
        while (raise > 0 && iBills < 9 && iCoins < 42) {
            int32_t iBillChance = RandomInt(0, 9 - iBills);
            int32_t iCoinChance = RandomInt(0, 42 - iCoins);
            int32_t iIndex = 0;
            if (static_cast<double>(iCoinChance) >= static_cast<double>(iBillChance) * 1.5) {
                int32_t iAmount;
                RandomInt(0, 100);
                iIndex = RandomInt(0, 2);
                CaravanMenu::pMe->LoadNewCoin(iIndex);
                if (iIndex >= 2) {
                    iAmount = 100;
                }
                else {
                    iAmount = iIndex != 0 ? 4 : 1;
                }
                raise -= iAmount;
                ++iCoins;
            }
            else {
                int32_t iAmount;
                iIndex = RandomInt(0, 3);
                CaravanMenu::pMe->LoadNewBill(iIndex);
                if (iIndex >= 2) {
                    iAmount = 40;
                }
                else {
                    iAmount = iIndex != 0 ? 8 : 2;
                }
                raise -= iAmount;
                ++iBills;
            }
        }
    }
    for (int32_t i = iBillIndexToAnim; i < pListOfBills.iSize && intlistbills.iSize; ++i) {
        int32_t iListIndex = RandomInt(0, intlistbills.iSize);
        int32_t iLocIndex = intlistbills[iListIndex];
        intlistbills.RemoveFast(iListIndex, 1u);
        BSsprintf(blockName, 260, "Bill-Placement_0%i:0", iLocIndex + 1);
        NiAVObject* pPlacement = BSUtilities::GetObjectByName(pBillsGrid, static_cast<const char*>(blockName));
        const NiBound* pWorldBound = &pPlacement->GetWorldBound();
        float fX = pWorldBound->m_kCenter.x;
        float fY = pWorldBound->m_kCenter.y;
        float fZ = pWorldBound->m_kCenter.z;
        kBillRotate = pPlacement->m_kLocal.m_Rotate;
        pListOfBills[i]->SetRotate(kBillRotate);
        pListOfBills[i]->SetTranslate(fX, fY, fZ);
        NiUpdateData UpdateData(0.0f, 0, 0);
        pListOfBills[i]->Update(UpdateData);
    }
    for (int32_t i = iCoinIndexToAnim; i < pListOfCoins.iSize && intlistcoins.iSize; ++i) {
        int32_t iListIndex = RandomInt(0, intlistcoins.iSize);
        int32_t iLocIndex = intlistcoins[iListIndex];
        intlistcoins.RemoveFast(iListIndex, 1u);
        if (iLocIndex + 1 >= 10) {
            BSsprintf(blockName, 260, "Coin-Placement_%i:0", iLocIndex + 1);
        }
        else {
            BSsprintf(blockName, 260, "Coin-Placement_0%i:0", iLocIndex + 1);
        }
        NiAVObject* pPlacement = BSUtilities::GetObjectByName(pCoinsGrid, static_cast<const char*>(blockName));
        const NiBound* pWorldBound = &pPlacement->GetWorldBound();
        float fX = pWorldBound->m_kCenter.x;
        float fY = pWorldBound->m_kCenter.y;
        float fZ = pWorldBound->m_kCenter.z;
        kCoinRotate = pPlacement->m_kLocal.m_Rotate;
        pListOfCoins[i]->SetRotate(kCoinRotate);
        pListOfCoins[i]->SetTranslate(fX, fY, fZ);
        NiUpdateData UpdateData(0.0f, 0, 0);
        pListOfCoins[i]->Update(UpdateData);
    }
    if (BSMAX(iCoins, iBills) > 0) {
        eGameState = CaravanMenu::CA_ANTE_MONEYIN_ANIM;
    }
    NiUpdateData UpdateData(0.0f, false, false);
    CaravanMenu::spModel->Update(UpdateData);
    const NiBound* pWorldBound = &CaravanMenu::spModel->GetWorldBound();
    float fRadius = pWorldBound->m_fRadius * 2.5;
    NiNode* pModel = CaravanMenu::spModel.m_pObject;
    BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pModel, fRadius);
}

void CaravanMenu::UpdateTrackValue(int32_t aiTrack) {
    int32_t i;
    int32_t iTrackValue = 0;
    for (i = 0; i < 7; ++i) {
        int32_t iFaceValue = 0;
        if (!pCardTracks[aiTrack][i].iSize) {
            break;
        }
        for (uint32_t auiIndex = 0; auiIndex < pCardTracks[aiTrack][i].iSize; ++auiIndex) {
            CARAVANCARDDATA* pCardData = &pCardTracks[aiTrack][i][auiIndex]->data;
            if (auiIndex) {
                if (pCardData->iFaceValue == 14) {
                    iFaceValue *= 2;
                }
            }
            else {
                iFaceValue = pCardData->iFaceValue;
            }
        }
        iTrackValue += iFaceValue;
    }
    trackInfo[aiTrack].iValue = iTrackValue;
    if (i) {
        // The track's last card slot and the one before it.
        BSSimpleArray<TESCaravanCard*>& rLastSlot = pCardTracks[aiTrack][i - 1];
        if (i == 1) {
            trackInfo[aiTrack].eCardSuit = (*pCardTracks[aiTrack])[0]->data.iCardSuit;
            for (uint32_t j = 1; j < rLastSlot.iSize; ++j) {
                if (rLastSlot[j]->data.iFaceValue == 13) {
                    trackInfo[aiTrack].eCardSuit = rLastSlot[j]->data.iCardSuit;
                }
            }
            trackInfo[aiTrack].eDirection = 0;
        }
        else {
            int32_t iLastFaceValue = rLastSlot[0]->data.iFaceValue;
            if (iLastFaceValue <= pCardTracks[aiTrack][i - 2][0]->data.iFaceValue) {
                trackInfo[aiTrack].eDirection = -1;
            }
            else {
                trackInfo[aiTrack].eDirection = 1;
            }
            trackInfo[aiTrack].eCardSuit = rLastSlot[0]->data.iCardSuit;
            for (uint32_t k = 1; k < rLastSlot.iSize; ++k) {
                if (rLastSlot[k]->data.iFaceValue == 13) {
                    trackInfo[aiTrack].eCardSuit = rLastSlot[k]->data.iCardSuit;
                    trackInfo[aiTrack].eDirection = -trackInfo[aiTrack].eDirection;
                }
            }
        }
    }
    else {
        trackInfo[aiTrack].eCardSuit = 5;
        trackInfo[aiTrack].eDirection = 0;
    }
    pTiles[aiTrack + 23]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[aiTrack + 23]->Set(CaravanMenu::iUT_Value, trackInfo[aiTrack].iValue);
    if (trackInfo[aiTrack].iValue < 21) {
        pTiles[aiTrack + 23]->Set(4084, 4084);
        pTiles[aiTrack + 23]->Set(4009, 126.0f, true);
    }
    if (trackInfo[aiTrack].iValue <= 26) {
        pTiles[aiTrack + 23]->Set(4084, 1);
    }
    else {
        pTiles[aiTrack + 23]->Set(4084, 2);
    }
    pTiles[aiTrack + 23]->Set(4009, 255.0f, true);
}

bool CaravanMenu::IsWellFormedMenu() {
    int32_t i;
    for (i = 0; i < 51; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void CaravanMenu::LoadNifs() {
    char acButtonText[268];
    pActiveDeckGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Deck.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    pInactiveDeckGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Available.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    CaravanMenu::spModel->AttachChild(pInactiveDeckGeometry, 1);
    CaravanMenu::spModel->AttachChild(pActiveDeckGeometry, 1);
    intlistbills.Clear(true);
    intlistcoins.Clear(true);
    NiUpdateData UpdateData(0.0f, 0, 0);
    CaravanMenu::spModel->Update(UpdateData);
    iAnte = BSMIN(iPlayerAnte, iNPCAnte);
    NiControllerManager* pController = static_cast<NiControllerManager*>(
        pActiveDeckGeometry->GetController(&NiControllerManager::ms_RTTI)
    );
    if (pController) {
        pActiveDeckAnimManager = pController;
        pActiveDeckAnimations[0] = pActiveDeckAnimManager->GetSequenceByName("Deck_Step-Forward");
        pActiveDeckAnimations[1] = pActiveDeckAnimManager->GetSequenceByName("Deck_Step-Reverse");
        pActiveDeckAnimations[2] = pActiveDeckAnimManager->GetSequenceByName("Deck_Modify");
        pActiveDeckAnimations[3] = pActiveDeckAnimManager->GetSequenceByName("Deck_Fast-Forward");
        pActiveDeckAnimations[4] = pActiveDeckAnimManager->GetSequenceByName("Deck_Fast-Reverse");
        pActiveDeckAnimations[5] = pActiveDeckAnimManager->GetSequenceByName("Deck_Wipe");
        pActiveDeckAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pActiveDeckGeometry);
    }
    pController = static_cast<NiControllerManager*>(
        pInactiveDeckGeometry->GetController(&NiControllerManager::ms_RTTI)
    );
    if (pController) {
        pInactiveDeckAnimManager = pController;
        pInactiveDeckAnimations[0] = pInactiveDeckAnimManager->GetSequenceByName("Available_Step-Forward");
        pInactiveDeckAnimations[1] = pInactiveDeckAnimManager->GetSequenceByName("Available_Step-Reverse");
        pInactiveDeckAnimations[2] = pInactiveDeckAnimManager->GetSequenceByName("Available_Modify");
        pInactiveDeckAnimations[3] = pInactiveDeckAnimManager->GetSequenceByName("Available_Fast-Forward");
        pInactiveDeckAnimations[4] = pInactiveDeckAnimManager->GetSequenceByName("Available_Fast-Reverse");
        pInactiveDeckAnimations[5] = pInactiveDeckAnimManager->GetSequenceByName("Available_Wipe");
        pInactiveDeckAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pInactiveDeckGeometry);
    }
    for (BSShaderPPLightingProperty*& pShader : deckFaceShaders) {
        pShader = nullptr;
    }
    for (int32_t j = 0; j < 24; ++j) {
        if (j >= 9) {
            BSShaderPPLightingProperty* pShader;
            int32_t iIsPPLighting;
            char acDeckName[25];
            char acDeckBackName[25];
            char acAvailableName[25];
            char acAvailableBackName[33];
            BSsprintf(acDeckName, 25, "Deck_%i:%i", j + 1, j);
            BSsprintf(acDeckBackName, 25, "Deck_%i_Back:0", j + 1);
            BSsprintf(acAvailableName, 25, "Available_%i:%i", j + 1, j);
            BSsprintf(acAvailableBackName, 25, "Available_%i_Back:0", j + 1);
            NiNode* pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acDeckName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j] = pShader;
            pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acDeckBackName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j + 24] = pShader;
            pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acAvailableName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j + 48] = pShader;
            pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acAvailableBackName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j + 72] = pShader;
        }
        else {
            BSShaderPPLightingProperty* pShader;
            int32_t iIsPPLighting;
            char acDeckName[25];
            char acDeckBackName[25];
            char acAvailableName[25];
            char acAvailableBackName[33];
            BSsprintf(acDeckName, 25, "Deck_0%i:%i", j + 1, j);
            BSsprintf(acDeckBackName, 25, "Deck_0%i_Back:0", j + 1);
            BSsprintf(acAvailableName, 25, "Available_0%i:%i", j + 1, j);
            BSsprintf(acAvailableBackName, 25, "Available_0%i_Back:0", j + 1);
            NiNode* pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acDeckName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j] = pShader;
            pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acDeckBackName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j + 24] = pShader;
            pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acAvailableName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j + 48] = pShader;
            pNode = static_cast<NiNode*>(
                BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acAvailableBackName))
            );
            if (pNode->GetProperty(3)) {
                NiProperty* pProperty = pNode->GetProperty(3);
                int32_t iShaderInRange = false;
                if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >= 8) {
                    pProperty = pNode->GetProperty(3);
                    if (static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pShader = static_cast<BSShaderPPLightingProperty*>(pNode->GetProperty(3));
            }
            else {
                pShader = nullptr;
            }
            deckFaceShaders[j + 72] = pShader;
        }
    }
    SwapDeckBuildingTextures(12, false);
    eActiveMenu = CaravanMenu::CA_DECK_MENU;
    eGameState = CaravanMenu::CA_CAMERA_TO_DECK_ANIM;
    pTiles[9]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[9]->Set(CaravanMenu::iUT_Value, iTotalCards);
    pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
    uint32_t uiCardCount = pPlayer->GetCaravanCards2()->ItemsInList();
    pTiles[8]->Set(CaravanMenu::iUT_Value, uiCardCount);
    const char* pDeckText = sCaravanDeckText.String();
    const char* pRandomizeText = sRSMRandomize.String();
    BSsprintf(acButtonText, 260, "%s %s", pRandomizeText, pDeckText);
    pTiles[10]->Set(4036, acButtonText, true);
    float fX = pTiles[10]->GetFloat(4001) - 150.0;
    pTiles[10]->Set(4001, fX, true);
    pTiles[10]->AddNeedsUpdate(1);
    if (!InterfaceManager::Has360Controller()) {
        int32_t iPCButtonText = Tile::TextToTrait("_PCButtonText");
        pTiles[10]->Set(iPCButtonText, "S)", true);
    }
    fX = pTiles[11]->GetFloat(4001) - 150.0;
    pTiles[11]->Set(4001, fX, true);
    pTiles[11]->AddNeedsUpdate(1);
    pTiles[0]->Set(4003, 0);
    pTiles[1]->Set(4003, 1);
    pTiles[2]->Set(4003, 0);
    pTiles[3]->Set(4003, 0);
    const char* pCaravanDeckText = sCaravanDeckText.String();
    pTiles[42]->Set(4036, pCaravanDeckText, true);
    const char* pTotalCardsText = sTotalCardsText.String();
    pTiles[43]->Set(4036, pTotalCardsText, true);
    const char* pAllCardsText = sAllCardsText.String();
    pTiles[11]->Set(4036, pAllCardsText, true);
    const char* pNavigateText = sNavigateText.String();
    pTiles[39]->Set(4036, pNavigateText, true);
    const char* pAddText = sAddText.String();
    pTiles[12]->Set(4036, pAddText, true);
    const char* pRemoveText = sRemoveText.String();
    pTiles[13]->Set(4036, pRemoveText, true);
    const char* pPlayCaravanText = sPlayCaravanText.String();
    pTiles[14]->Set(4036, pPlayCaravanText, true);
    pTiles[39]->Set(4003, 0);
    pTiles[39]->AddNeedsUpdate(4);
    UpdateCaravanFlags();
}

void CaravanMenu::SwapDeckBuildingTextures(int32_t aiTarget, bool abFastScrolling) {
    // Each buffer deliberately keeps its text from the loop's previous passes: the language-swapped
    // path, the source path and one name buffer per branch.
    char acPath[268];
    char acInPath[268];
    char acShownName[28];
    char acShownPaddedName[28];
    char acHiddenName[28];
    char acHiddenPaddedName[28];
    for (int32_t i = 0; i < 24; ++i) {
        if (deckFaceShaders[i] && deckFaceShaders[i + 24] && deckFaceShaders[i + 48] && deckFaceShaders[i + 72]) {
            uint32_t auiIndex = aiTarget - (11 - i);
            // Tests the difference's sign, then compares it unsigned.
            if (static_cast<int32_t>(auiIndex) >= 0 && auiIndex < iTotalCards) {
                NiAVObject* pFace;
                if (i >= 9) {
                    BSsprintf(acShownName, 25, "Deck_%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acShownName));
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownName, 25, "Deck_%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acShownName));
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownName, 25, "Available_%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acShownName));
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownName, 25, "Available_%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acShownName));
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    if (i == 12 || i == 11) {
                        BSsprintf(acShownName, 25, "Deck_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pActiveDeckGeometry,
                            static_cast<const char*>(acShownName)
                        );
                        NiPoint3 kTranslate = pFace->GetTranslate();
                        pFace->SetTranslate(kTranslate.x, kTranslate.y, 0.0f);
                        BSsprintf(acShownName, 25, "Available_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pInactiveDeckGeometry,
                            static_cast<const char*>(acShownName)
                        );
                        kTranslate = pFace->GetTranslate();
                        pFace->SetTranslate(kTranslate.x, kTranslate.y, 0.0f);
                    }
                    else {
                        BSsprintf(acShownName, 25, "Deck_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pActiveDeckGeometry,
                            static_cast<const char*>(acShownName)
                        );
                        pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                        BSsprintf(acShownName, 25, "Available_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pInactiveDeckGeometry,
                            static_cast<const char*>(acShownName)
                        );
                        pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    }
                }
                else {
                    BSsprintf(acShownPaddedName, 25, "Deck_0%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(
                        pActiveDeckGeometry,
                        static_cast<const char*>(acShownPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownPaddedName, 25, "Deck_0%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pActiveDeckGeometry,
                        static_cast<const char*>(acShownPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownPaddedName, 25, "Deck_0%i_Shadow:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pActiveDeckGeometry,
                        static_cast<const char*>(acShownPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownPaddedName, 25, "Available_0%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(
                        pInactiveDeckGeometry,
                        static_cast<const char*>(acShownPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownPaddedName, 25, "Available_0%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pInactiveDeckGeometry,
                        static_cast<const char*>(acShownPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                    BSsprintf(acShownPaddedName, 25, "Available_0%i_Shadow:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pInactiveDeckGeometry,
                        static_cast<const char*>(acShownPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, 0.0f);
                }
                NiUpdateData UpdateData1(0.0f, false, false);
                pInactiveDeckGeometry->Update(UpdateData1);
                NiUpdateData UpdateData2(0.0f, false, false);
                pActiveDeckGeometry->Update(UpdateData2);
                if (pDeckArray[auiIndex]->bIsActive) {
                    NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
                    BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[auiIndex]->faceTexture.GetTextureName());
                    SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                    TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                    if (spTexture.m_pObject) {
                        deckFaceShaders[i]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                    }
                    BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[auiIndex]->backTexture.GetTextureName());
                    SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                    TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                    if (spTexture.m_pObject) {
                        deckFaceShaders[i + 24]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                    }
                    BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[auiIndex]->backTexture.GetTextureName());
                    SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                    TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                    if (spTexture.m_pObject) {
                        deckFaceShaders[i + 48]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                    }
                    BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[auiIndex]->faceTexture.GetTextureName());
                    SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                    TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                    if (spTexture.m_pObject) {
                        deckFaceShaders[i + 72]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                    }
                }
                else {
                    // Only the call is kept: the card's faceTexture address tested after it is never null.
                    pDeckArray[auiIndex];
                    {
                        NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
                        BSsprintf(
                            acInPath,
                            260,
                            "%s%s",
                            "Textures\\",
                            pDeckArray[auiIndex]->faceTexture.GetTextureName()
                        );
                        SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                        if (spTexture.m_pObject) {
                            deckFaceShaders[i + 48]
                                ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                        }
                        BSsprintf(
                            acInPath,
                            260,
                            "%s%s",
                            "Textures\\",
                            pDeckArray[auiIndex]->backTexture.GetTextureName()
                        );
                        SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                        if (spTexture.m_pObject) {
                            deckFaceShaders[i + 72]
                                ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                        }
                        BSsprintf(
                            acInPath,
                            260,
                            "%s%s",
                            "Textures\\",
                            pDeckArray[auiIndex]->backTexture.GetTextureName()
                        );
                        SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                        if (spTexture.m_pObject) {
                            deckFaceShaders[i]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                        }
                        BSsprintf(
                            acInPath,
                            260,
                            "%s%s",
                            "Textures\\",
                            pDeckArray[auiIndex]->faceTexture.GetTextureName()
                        );
                        SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                        if (spTexture.m_pObject) {
                            deckFaceShaders[i + 24]
                                ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                        }
                    }
                }
            }
            else {
                NiAVObject* pFace;
                // The z offset is kept as an int and converted at each use.
                int32_t iZ = -50;
                if (abFastScrolling) {
                    iZ = 0;
                }
                if (i >= 9) {
                    BSsprintf(acHiddenName, 25, "Deck_%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acHiddenName));
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenName, 25, "Deck_%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(acHiddenName));
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenName, 25, "Available_%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acHiddenName));
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenName, 25, "Available_%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry, static_cast<const char*>(acHiddenName));
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    if (i == 12 || i == 11) {
                        BSsprintf(acHiddenName, 25, "Deck_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pActiveDeckGeometry,
                            static_cast<const char*>(acHiddenName)
                        );
                        NiPoint3 kTranslate = pFace->GetTranslate();
                        pFace->SetTranslate(kTranslate.x, kTranslate.y, static_cast<float>(iZ));
                        BSsprintf(acHiddenName, 25, "Available_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pInactiveDeckGeometry,
                            static_cast<const char*>(acHiddenName)
                        );
                        kTranslate = pFace->GetTranslate();
                        pFace->SetTranslate(kTranslate.x, kTranslate.y, static_cast<float>(iZ));
                    }
                    else {
                        BSsprintf(acHiddenName, 25, "Deck_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pActiveDeckGeometry,
                            static_cast<const char*>(acHiddenName)
                        );
                        pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                        BSsprintf(acHiddenName, 25, "Available_%i_Shadow:0", i + 1);
                        pFace = BSUtilities::GetObjectByName(
                            pInactiveDeckGeometry,
                            static_cast<const char*>(acHiddenName)
                        );
                        pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    }
                }
                else {
                    BSsprintf(acHiddenPaddedName, 25, "Deck_0%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(
                        pActiveDeckGeometry,
                        static_cast<const char*>(acHiddenPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenPaddedName, 25, "Deck_0%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pActiveDeckGeometry,
                        static_cast<const char*>(acHiddenPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenPaddedName, 25, "Available_0%i:%i", i + 1, i);
                    pFace = BSUtilities::GetObjectByName(
                        pInactiveDeckGeometry,
                        static_cast<const char*>(acHiddenPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenPaddedName, 25, "Available_0%i_Back:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pInactiveDeckGeometry,
                        static_cast<const char*>(acHiddenPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenPaddedName, 25, "Deck_0%i_Shadow:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pActiveDeckGeometry,
                        static_cast<const char*>(acHiddenPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                    BSsprintf(acHiddenPaddedName, 25, "Available_0%i_Shadow:0", i + 1);
                    pFace = BSUtilities::GetObjectByName(
                        pInactiveDeckGeometry,
                        static_cast<const char*>(acHiddenPaddedName)
                    );
                    pFace->SetTranslate(0.0f, 0.0f, static_cast<float>(iZ));
                }
                NiUpdateData UpdateData1(0.0f, false, false);
                pInactiveDeckGeometry->Update(UpdateData1);
                NiUpdateData UpdateData2(0.0f, false, false);
                pActiveDeckGeometry->Update(UpdateData2);
            }
        }
    }
}

void CaravanMenu::UpdateDealCardTexture(bool abPlayer) {
    BSShaderPPLightingProperty* pFaceShader;
    int32_t iIsPPLighting;
    char acPath[264];
    char blockName[264];
    NiAVObject* pHand = abPlayer ? pHandGeometry[0] : pHandGeometry[1];
    if (abPlayer) {
        BSstrcpy(blockName, 260, "Player-Deck_06:5");
    }
    else {
        BSstrcpy(blockName, 260, "Opponent-Deck_06:5");
    }
    NiAVObject* pFace = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
    if (pFace->GetProperty(3)) {
        BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
        int32_t iShaderInRange = false;
        if (pProperty->iShaderPropertyType >= 8) {
            pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
            if (pProperty->iShaderPropertyType <= 12) {
                iShaderInRange = true;
            }
        }
        iIsPPLighting = iShaderInRange;
    }
    else {
        iIsPPLighting = 0;
    }
    if (iIsPPLighting) {
        pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
    }
    else {
        pFaceShader = nullptr;
    }
    NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
    if (pFaceShader) {
        uint32_t uiHandSize = abPlayer ? pPlayerHand.iSize : pNPCHand.iSize;
        if (uiHandSize > 4) {
            const char* pTextureName;
            if (abPlayer) {
                pTextureName = pPlayerHand[4u]->faceTexture.GetTextureName();
            }
            else {
                pTextureName = pNPCHand[4u]->backTexture.GetTextureName();
            }
            BSsprintf(blockName, 260, "%s%s", "Textures\\", pTextureName);
            SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
            if (spTexture.m_pObject) {
                pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
            }
        }
    }
    if (abPlayer) {
        BSstrcpy(blockName, 260, "Player-Deck_Back:0");
    }
    else {
        BSstrcpy(blockName, 260, "Opponent-Deck_Back:0");
    }
    pFace = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
    if (pFace->GetProperty(3)) {
        BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
        int32_t iShaderInRange = false;
        if (pProperty->iShaderPropertyType >= 8) {
            pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
            if (pProperty->iShaderPropertyType <= 12) {
                iShaderInRange = true;
            }
        }
        iIsPPLighting = iShaderInRange;
    }
    else {
        iIsPPLighting = 0;
    }
    if (iIsPPLighting) {
        pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
    }
    else {
        pFaceShader = nullptr;
    }
    spTexture = static_cast<NiSourceTexture*>(nullptr);
    if (pFaceShader && (abPlayer ? pPlayerDeck.iSize != 0 : pNPCDeck.iSize != 0)) {
        if (abPlayer) {
            BSsprintf(blockName, 260, "Textures\\%s", pPlayerDeck[iNextPlayerCard]->backTexture.GetTextureName());
        }
        else {
            BSsprintf(blockName, 260, "Textures\\%s", pNPCDeck[iNextNPCCard]->backTexture.GetTextureName());
        }
        SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
        if (spTexture.m_pObject) {
            pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
        }
    }
    else {
        pFace->SetAppCulled(true);
        NiUpdateData UpdateData(0.0f, false, false);
        pFace->Update(UpdateData);
    }
}

void CaravanMenu::SelectDeckCard(bool abUnselectOnly) {
    if (pSelectedObject && pSelectedObject->GetTranslate().z > 0.0) {
        pSelectedObject->SetTranslate(0.0f, pSelectedObject->GetTranslate().y, 0.0f);
        NiUpdateData kUpdate1(0.0f, false, false);
        pSelectedObject->Update(kUpdate1);
        pSelectedShadow->SetTranslate(0.0f, pSelectedObject->GetTranslate().y, 0.0f);
        NiUpdateData kUpdate2(0.0f, false, false);
        pSelectedShadow->Update(kUpdate2);
    }

    if (!abUnselectOnly) {
        char blockName[32];
        BSsprintf(blockName, 30, "Player-Deck_0%i:%i", iSelectedDeckCard + 1, iSelectedDeckCard);
        pSelectedObject = BSUtilities::GetObjectByName(pHandGeometry[0], static_cast<const char*>(blockName));
        BSsprintf(blockName, 30, "Player-Deck_0%i_Shadow:0", iSelectedDeckCard + 1);
        pSelectedShadow = BSUtilities::GetObjectByName(pHandGeometry[0], static_cast<const char*>(blockName));
        pSelectedCard = pPlayerHand[iSelectedDeckCard];
        pSelectedObject->SetTranslate(0.0f, pSelectedObject->GetTranslate().y, 1.0f);
        NiUpdateData kUpdate3(0.0f, false, false);
        pSelectedObject->Update(kUpdate3);
        pSelectedShadow->SetTranslate(0.0f, pSelectedObject->GetTranslate().y, 1.0f);
        NiUpdateData kUpdate4(0.0f, false, false);
        pSelectedShadow->Update(kUpdate4);
    }
}

void CaravanMenu::SelectTrackCard(int32_t aiTrack, int32_t aiRow) {
    NiMatrix3 kInvalidRotate;
    NiMatrix3 kValidRotate;
    char acTextureName[260];
    char blockName[268];
    NiMatrix3 kCardRot;
    bool bIsValidPlacement = IsValidCardPlacement(aiTrack, aiRow, iSelectedDeckCard, false);
    int32_t iColumn = pCardTracks[aiTrack][aiRow].iSize;
    if (iColumn > 3) {
        iColumn = 3;
    }
    BSsprintf(blockName, 260, "Select%i_0%i:0", aiRow + 1, iColumn + 1);
    NiAVObject* pCursorPoint = BSUtilities::GetObjectByName(pTrackGrids[aiTrack], static_cast<const char*>(blockName));
    if (pCursorPoint) {
        BSShaderPPLightingProperty* pFaceShader;
        int32_t iIsPPLighting;
        NiPoint3 kCardLoc;
        NiAVObject* pCursorFace;
        if (bIsValidPlacement) {
            NiMatrix3 matrot_90;
            BSAudio::QInstance().GetSoundHandleByName("GAMECaravanToggleTrack", 289u).Play(false);
            pCursorFace = pCursors[0];
            pCursors[1]->SetTranslate(0.0f, 0.0f, -50.0f);
            NiUpdateData UpdateData1(0.0f, 0, 0);
            pCursors[1]->Update(UpdateData1);
            matrot_90.MakeZRotation(1.5707964f);
            kCardRot = pCursorPoint->m_kLocal.m_Rotate;
            kCardRot = *::new (&kValidRotate) NiMatrix3(kCardRot * matrot_90);
            kCardLoc = pCursorPoint->GetWorldBound().m_kCenter;
            pCursorFace->SetRotate(kCardRot);
            pCursorFace->SetTranslate(kCardLoc);
            NiUpdateData UpdateData2(0.0f, 0, 0);
            pCursorFace->Update(UpdateData2);
            pCursorFace = BSUtilities::GetObjectByName(pCursorFace, "Add:3");
        }
        else {
            NiMatrix3 matrot_90;
            BSAudio::QInstance().GetSoundHandleByName("GAMECaravanInvalidStackLocation", 289u).Play(false);
            pCursorFace = pCursors[1];
            pCursors[0]->SetTranslate(0.0f, 0.0f, -50.0f);
            NiUpdateData UpdateData1(0.0f, 0, 0);
            pCursors[0]->Update(UpdateData1);
            matrot_90.MakeZRotation(1.5707964f);
            kCardRot = pCursorPoint->m_kLocal.m_Rotate;
            kCardRot = *::new (&kInvalidRotate) NiMatrix3(kCardRot * matrot_90);
            kCardLoc = pCursorPoint->GetWorldBound().m_kCenter;
            pCursorFace->SetRotate(kCardRot);
            pCursorFace->SetTranslate(kCardLoc);
            NiUpdateData UpdateData2(0.0f, 0, 0);
            pCursorFace->Update(UpdateData2);
            pCursorFace = BSUtilities::GetObjectByName(pCursorFace, "Remove:3");
        }
        if (pCursorFace->GetProperty(3)) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pCursorFace->GetProperty(3));
            int32_t iShaderInRange = false;
            if (pFaceShader->iShaderPropertyType >= 8) {
                pFaceShader = static_cast<BSShaderPPLightingProperty*>(pCursorFace->GetProperty(3));
                if (pFaceShader->iShaderPropertyType <= 12) {
                    iShaderInRange = true;
                }
            }
            iIsPPLighting = iShaderInRange;
        }
        else {
            iIsPPLighting = 0;
        }
        if (iIsPPLighting) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pCursorFace->GetProperty(3));
        }
        else {
            pFaceShader = nullptr;
        }
        NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
        if (pFaceShader) {
            BSsprintf(blockName, 260, "%s%s", "Textures\\", pSelectedCard->faceTexture.GetTextureName());
            SwapPlatformLanguageTexturePath(blockName, acTextureName, 260u);
            TES::pInstance->CreateTextureImage(acTextureName, spTexture, false, false);
            if (spTexture.m_pObject) {
                pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
            }
        }
    }
}

void CaravanMenu::SelectWholeTrack(int32_t aiTrack) {
    for (int32_t i = 0; i < 7; ++i) {
        pTrackGeometry[aiTrack][i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), 3.0f);
        NiUpdateData UpdateData(0.0, 0, 0);
        pTrackGeometry[aiTrack][i]->Update(UpdateData);
    }
}

void CaravanMenu::DeselectWholeTrack(int32_t aiTrack) {
    for (int32_t i = 0; i < 7; ++i) {
        pTrackGeometry[aiTrack][i]
            ->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(0.0));
        NiUpdateData UpdateData(0.0, 0, 0);
        pTrackGeometry[aiTrack][i]->Update(UpdateData);
    }
}

void CaravanMenu::HandleTrackSelection(int32_t aiXShift, int32_t aiYShift) {
    int32_t iSelectedTrack = this->iSelectedTrack;
    int32_t iSelectedRow = this->iSelectedRow;
    if (aiXShift) {
        if (iSelectedTrack >= 3) {
            this->iSelectedTrack -= aiXShift;
            int32_t iClampedTrack = BSMAX(3, this->iSelectedTrack);
            this->iSelectedTrack = BSMIN(iClampedTrack, 5);
        }
        else {
            this->iSelectedTrack += aiXShift;
            int32_t iClampedTrack = BSMAX(0, this->iSelectedTrack);
            this->iSelectedTrack = BSMIN(iClampedTrack, 2);
        }
    }
    if (aiYShift && iSelectedRow == this->iSelectedRow) {
        if (this->iSelectedTrack >= 3) {
            this->iSelectedRow -= aiYShift;
        }
        else {
            this->iSelectedRow += aiYShift;
        }
        if (this->iSelectedRow == -1) {
            if (this->iSelectedTrack >= 3) {
                this->iSelectedTrack = 5 - iSelectedTrack;
            }
            else {
                this->iSelectedTrack = 5 - iSelectedTrack;
            }
            this->iSelectedRow = 0;
        }
        else if (this->iSelectedRow == 7) {
            this->iSelectedRow = 6;
        }
    }

    if (aiXShift == aiYShift && !aiXShift) {
        this->iSelectedTrack = 0;
        this->iSelectedRow = 0;
        iSelectedColumn = 0;
    }
    iSelectedColumn = pCardTracks[this->iSelectedTrack][this->iSelectedRow].iSize;
    SelectTrackCard(this->iSelectedTrack, this->iSelectedRow);
    if (IsValidCardPlacement(this->iSelectedTrack, this->iSelectedRow, iSelectedDeckCard, false)) {
        SetFlag(CaravanMenu::CAF_DISABLE_A, false);
    }
    else {
        SetFlag(CaravanMenu::CAF_DISABLE_A, true);
    }
    UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
}

void CaravanMenu::LoadNewBill(int32_t aiBill) {
    NiControllerSequence* pSequence;
    int32_t iBill = BSMIN(aiBill, 2);
    NiAVObject* pBill = static_cast<NiAVObject*>(pBillGeometry[iBill]->Clone());
    int32_t iPlacement = RandomInt(0, 3);
    NiControllerManager* pController = static_cast<NiControllerManager*>(
        pBill->GetController(&NiControllerManager::ms_RTTI)
    );
    if (iPlacement) {
        if (iPlacement == 1) {
            if (pController) {
                pSequence = pController->GetSequenceByName("Bottom_Place");
                pController->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
                NiTimeController::StartAnimations(pBill);
            }
        }
        else if (iPlacement == 2 && pController) {
            pSequence = pController->GetSequenceByName("Alt_Place");
            pController->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
            NiTimeController::StartAnimations(pBill);
        }
    }
    else if (pController) {
        pSequence = pController->GetSequenceByName("Top_Place");
        pController->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
        NiTimeController::StartAnimations(pBill);
    }
    pListOfBills.Add(pBill);
    pListOfBillAnimations->AddHead(pSequence);
    CaravanMenu::spModel->AttachChild(pBill, 1);
}

void CaravanMenu::LoadNewCoin(int32_t aiCoin) {
    NiControllerSequence* pSequence;
    int32_t iCoin = BSMIN(aiCoin, 2);
    NiAVObject* pCoin = static_cast<NiAVObject*>(pCoinGeometry[iCoin]->Clone());
    int32_t iPlacement = RandomInt(0, 2);
    NiControllerManager* pController = static_cast<NiControllerManager*>(
        pCoin->GetController(&NiControllerManager::ms_RTTI)
    );
    if (iPlacement) {
        if (iPlacement == 1 && pController) {
            pSequence = pController->GetSequenceByName("Bottom_Place");
            pController->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
            NiTimeController::StartAnimations(pCoin);
        }
    }
    else if (pController) {
        pSequence = pController->GetSequenceByName("Top_Place");
        pController->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
        NiTimeController::StartAnimations(pCoin);
    }
    pListOfCoins.Add(pCoin);
    pListOfCoinAnimations->AddHead(pSequence);
    CaravanMenu::spModel->AttachChild(pCoin, 1);
}

void CaravanMenu::ZeroFlags() {
    iFlags = 0;
}

void CaravanMenu::DeselectTrackCard() {
    pCursors[0]->SetTranslate(0.0f, 0.0f, -100.0f);
    pCursors[1]->SetTranslate(0.0f, 0.0f, -100.0f);
    NiUpdateData UpdateData1(0.0, 0, 0);
    pCursors[0]->Update(UpdateData1);
    NiUpdateData UpdateData2(0.0, 0, 0);
    pCursors[1]->Update(UpdateData2);
}

int32_t CaravanMenu::CalculateWinningState(int32_t aivalue) {
    if (aivalue >= 21) {
        return aivalue > 26;
    }

    return -1;
}

uint32_t CaravanMenu::LoadNifs2() {
    pTableGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Table.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    CaravanMenu::spModel = new NiNode(0);
    CaravanMenu::spModel->AttachChild(pTableGeometry, 1);
    NiUpdateData UpdateData(0.0f, false, false);
    CaravanMenu::spModel->Update(UpdateData);
    NiControllerManager* pController = static_cast<NiControllerManager*>(
        pTableGeometry->GetController(&NiControllerManager::ms_RTTI)
    );
    if (pController) {
        pCameraAnimManager = pController;
        pCameraAnimations[0] = pCameraAnimManager->GetSequenceByName("Bet_to_Deck");
        pCameraAnimations[1] = pCameraAnimManager->GetSequenceByName("Deck_to_Play");
        pCameraAnimations[2] = pCameraAnimManager->GetSequenceByName("Play_to_Bet");
        pCameraAnimManager->DeactivateAll(0.0f);
        NiTimeController::StartAnimations(pTableGeometry);
    }
    NiNode* pTable = static_cast<NiNode*>(pTableGeometry);
    CaravanMenu::sp3DCamera = static_cast<NiCamera*>(BSUtilities::GetObjectByName(pTable, "object0"));
    if (!CaravanMenu::sp3DCamera.m_pObject) {
        CaravanMenu::sp3DCamera = Create3DCamera();
    }
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(CaravanMenu::spModel.m_pObject, 1);
    const NiBound* pWorldBound = &CaravanMenu::spModel->GetWorldBound();
    float fRadius = pWorldBound->m_fRadius * 2.5;
    NiNode* pModel = CaravanMenu::spModel.m_pObject;
    return BSShaderManager::GetShadowSceneNode(1)->AddAllLightsFromNode_Recurse(pModel, fRadius);
}

NiCamera* CaravanMenu::Create3DCamera() {
    int32_t iHeight;
    NiCamera* pCamera;
    NiPoint3 kFocus;
    NiPoint3 kWorldUp;
    NiRect<float> kViewPort;
    pCamera = new NiCamera;
    ::new (&kViewPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    pCamera->SetViewPort(kViewPort);
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
    float fAspectRatio = static_cast<double>(iHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fFOV = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    kFrustum.m_fLeft = -std::tan(fFOV) * 0.75f;
    kFrustum.m_fRight = std::tan(fFOV) * 0.75f;
    double dTanFOV = std::tan(fFOV);
    kFrustum.m_fBottom = -dTanFOV * fAspectRatio * 0.75f;
    dTanFOV = std::tan(fFOV);
    kFrustum.m_fTop = dTanFOV * fAspectRatio * 0.75f;
    kFrustum.m_bOrtho = false;
    pCamera->SetViewFrustum(kFrustum);
    NiPoint3* pWorldUp = ::new (&kWorldUp) NiPoint3(0.0f, 0.0f, 1.0f);
    NiPoint3* pFocus = ::new (&kFocus) NiPoint3(0.0f, 0.0f, 1.0f);
    pCamera->LookAtWorldPoint(*pFocus, *pWorldUp);
    pCamera->SetMinNearPlaneDist(1.0f);
    NiUpdateData UpdateData(0.0f, false, false);
    pCamera->Update(UpdateData);
    NiFixedString kName("Surgery3DCamera");
    pCamera->SetName(kName);
    return pCamera;
}

char CaravanMenu::PrepareCaravanData() {
    BSSimpleList<TESCaravanCard*>* pInactiveList = pPlayer->GetCaravanCards1();
    BSSimpleList<TESCaravanCard*>* pActiveList = pPlayer->GetCaravanCards2();
    iCardsInDeck = pActiveList->ItemsInList();
    int32_t iInactiveCount = pInactiveList->ItemsInList();
    iTotalCards = iInactiveCount + pActiveList->ItemsInList();
    for (BSSimpleList<TESCaravanCard*>* pNode = pInactiveList; pNode && pNode->m_item; pNode = pNode->m_pkNext) {
        TESCaravanCard* pCard = pNode->m_item;
        pCard->bIsActive = false;
        pDeckArray.Add(pCard);
    }
    for (BSSimpleList<TESCaravanCard*>* pNode = pActiveList; pNode && pNode->m_item; pNode = pNode->m_pkNext) {
        TESCaravanCard* pCard = pNode->m_item;
        pCard->bIsActive = true;
        pDeckArray.Add(pCard);
    }
    pDeckArray.Sort(&TESCaravanCard::SortCardsFunc);
    uint32_t uiJumpAmount = iTotalCards / 10 <= 10 ? 10 : iTotalCards / 10;
    iJumpAmount = uiJumpAmount;
    return 1;
}

void CaravanMenu::SetupCurrency() {
    char modelPath[264];
    BSsprintf(modelPath, 260, "%sCurrency-Bill_Grid.NIF", "Meshes\\Terminals\\NV_Caravan\\");
    pBillsGrid = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    BSsprintf(modelPath, 260, "%sCurrency-Coin_Grid.NIF", "Meshes\\Terminals\\NV_Caravan\\");
    pCoinsGrid = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    for (int32_t i = 0; i < 3; ++i) {
        BSsprintf(modelPath, 260, "%sCurrency-Bill_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
        pBillGeometry[i] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
        BSsprintf(modelPath, 260, "%sCurrency-Coin_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
        pCoinGeometry[i] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    }
    eActiveMenu = CaravanMenu::CA_ANTE_MENU;
    eGameState = CaravanMenu::CA_IDLE;
    iBillIndexToAnim = 0;
    iCoinIndexToAnim = 0;
    pTiles[0]->Set(4003, 1);
    pTiles[1]->Set(4003, 0);
    pTiles[2]->Set(4003, 0);
    pTiles[3]->Set(4003, 0);
    const char* pCurrentAnteText = sCurrentAnteText.String();
    pTiles[40]->Set(4036, pCurrentAnteText, true);
    const char* pTotalFundsText = sTotalFundsText.String();
    pTiles[41]->Set(4036, pTotalFundsText, true);
    const char* pAutoMatchText = sAutoMatchText.String();
    pTiles[37]->Set(4036, pAutoMatchText, true);
    const char* pRaiseText = sRaiseText.String();
    pTiles[36]->Set(4036, pRaiseText, true);
    const char* pAcceptText = sAcceptText.String();
    pTiles[7]->Set(4036, pAcceptText, true);
    const char* pExitText = sExit.String();
    pTiles[38]->Set(4036, pExitText, true);
    PrepareBettingCurrency();
    iNPCAnte = static_cast<int32_t>(static_cast<double>(iNPCTotalFunds) * fBetPercentage);
    const char* pFullName = pChallenger->GetFullName();
    if (BSstrlen(pFullName) > 17) {
        pFullName = pChallenger->GetFullName();
        int32_t iOffset = 15 * (BSstrlen(pFullName) - 17);
        Tile* pBetValueTile = Tile::GetTileByName(pTiles[4], "CBM_NPCBetValue");
        float fX = pBetValueTile->GetFloat(4001) + static_cast<double>(iOffset);
        pBetValueTile->Set(4001, fX, true);
    }
    Tile* pInfoRect = pTiles[0]->GetChildByName("CBM_InfoRect");
    if (pInfoRect) {
        float fWidth = pInfoRect->GetFloat(4017) + 150.0;
        pInfoRect->Set(4017, fWidth, true);
        pInfoRect->AddNeedsUpdate(16);
    }
    pFullName = pChallenger->GetFullName();
    pTiles[4]->Set(4036, pFullName, true);
    pTiles[4]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[4]->Set(CaravanMenu::iUT_Value, iNPCAnte);
    pTiles[5]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[5]->Set(CaravanMenu::iUT_Value, iPlayerAnte);
    pTiles[6]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[6]->Set(CaravanMenu::iUT_Value, iTotalFunds);
    UpdateCaravanFlags();
    UpdateTileAlpha(CaravanMenu::TI_ANTE_MENU_TILESET);
    for (int32_t i = 0; i < 9; ++i) {
        intlistbills.Add(i);
    }
    for (int32_t i = 0; i < 42; ++i) {
        intlistcoins.Add(i);
    }
    // The NPC's bet is computed twice.
    double dNPCBet = static_cast<double>(CaravanMenu::pMe->iNPCTotalFunds) * CaravanMenu::pMe->fBetPercentage;
    double dNPCBetScale = static_cast<double>(CaravanMenu::pMe->iNPCTotalFunds) * CaravanMenu::pMe->fBetPercentage;
    UpdateBettingUI(
        static_cast<int32_t>(pPlayer->GetClampedActorFloatValue(ActorValue::BARTER) / 100.0 * dNPCBetScale + dNPCBet),
        iNPCAnte
    );
}

void CaravanMenu::PrepareBettingCurrency() {
    iTotalFunds = 0;
    pDataHandler->CreateBarterContainer(pPlayer);
    InventoryChanges* pPlayerContainer = pDataHandler->GetBarterContainer();
    iTotalFunds = pPlayerContainer->GetGoldAmount();
    for (BSSimpleList<ItemChange*>* pNode = pPlayerContainer->pListofChanges; pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        TESBoundObject* pObject = pNode->m_item->pContainerObj;
        if (pObject && pObject->GetFormType() == CMNY_ID && pNode->m_item->iNumber > 0) {
            int32_t iCount = pNode->m_item->iNumber;
            iTotalFunds += iCount * TESValueForm::GetFormValue(pObject);
        }
    }
    iNPCTotalFunds = 0;
    pDataHandler->CreateBarterContainer(pChallenger);
    InventoryChanges* pNPCContainer = pDataHandler->GetBarterContainer();
    iNPCTotalFunds = pNPCContainer->GetGoldAmount();
    for (BSSimpleList<ItemChange*>* pNode = pNPCContainer->pListofChanges; pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        TESBoundObject* pObject = pNode->m_item->pContainerObj;
        if (pObject && pObject->GetFormType() == CMNY_ID && pNode->m_item->iNumber > 0) {
            int32_t iCount = pNode->m_item->iNumber;
            iNPCTotalFunds += iCount * TESValueForm::GetFormValue(pObject);
        }
    }
}

void CaravanMenu::PrepareGameMenu() {
    NiTimeController* pTimeController;
    char modelPath[268];
    char acWinLossText[268];
    BSSimpleList<TESCaravanCard*>* pActiveList = pPlayer->GetCaravanCards2();
    BSSimpleList<TESCaravanCard*>* pInactiveList = pPlayer->GetCaravanCards1();
    pActiveList->RemoveAll();
    pInactiveList->RemoveAll();
    for (TESCaravanCard* pCard : pDeckArray) {
        if (pCard->bIsActive) {
            pActiveList->AddHead(pCard);
            pPlayerDeck.Add(pCard);
        }
        else {
            pInactiveList->AddHead(pCard);
        }
    }
    while (pDeckParam && pDeckParam->m_item) {
        pNPCDeck.Add(pDeckParam->m_item);
        pDeckParam = pDeckParam->m_pkNext;
    }
    for (int32_t i = 0; i < 8; ++i) {
        int32_t iIndex = RandomInt(0, pPlayerDeck.iSize);
        pPlayerHand.Add(pPlayerDeck[iIndex]);
        pPlayerDeck.RemoveFast(iIndex, 1);
        iIndex = RandomInt(0, pNPCDeck.iSize);
        pNPCHand.Add(pNPCDeck[iIndex]);
        pNPCDeck.RemoveFast(iIndex, 1);
    }
    iNextPlayerCard = RandomInt(0, pPlayerDeck.iSize);
    iNextNPCCard = RandomInt(0, pNPCDeck.iSize);
    pDeckParam = nullptr;
    pActiveList = nullptr;
    pInactiveList = nullptr;
    if (pDeckArray.iSize) {
        pDeckArray.Clear(true);
    }
    for (int32_t i = 0; i < 96; ++i) {
        deckFaceShaders[i] = nullptr;
    }
    pDeckArray.Clear(true);
    pInactiveDeckAnimManager = nullptr;
    pActiveDeckAnimManager = nullptr;
    for (int32_t i = 0; i < 6; ++i) {
        pInactiveDeckAnimations[i] = nullptr;
        pActiveDeckAnimations[i] = nullptr;
    }
    for (int32_t i = 0; i < 96; ++i) {
        deckFaceShaders[i] = nullptr;
    }
    CaravanMenu::spModel->DetachChildAt(CaravanMenu::spModel->GetChildCount() - 1);
    CaravanMenu::spModel->DetachChildAt(CaravanMenu::spModel->GetChildCount() - 1);
    pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Deck.NIF");
    pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Available.NIF");
    pInactiveDeckGeometry = nullptr;
    pActiveDeckGeometry = nullptr;
    for (int32_t i = 0; i < 3; ++i) {
        for (int32_t j = 0; j < 7; ++j) {
            BSsprintf(modelPath, 260, "%sPlayer_%i_0%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1, j + 1);
            pTrackGeometry[i][j] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
            CaravanMenu::spModel->AttachChild(pTrackGeometry[i][j], 1);
            BSsprintf(modelPath, 260, "%sOpponent_%i_0%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1, j + 1);
            pTrackGeometry[i + 3][j] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
            CaravanMenu::spModel->AttachChild(pTrackGeometry[i + 3][j], 1);
        }
    }
    for (int32_t i = 0; i < 3; ++i) {
        BSsprintf(modelPath, 260, "%sPlayerGrid_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
        pTrackGrids[i] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
        BSsprintf(modelPath, 260, "%sOppGrid_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
        pTrackGrids[i + 3] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    }
    BSsprintf(modelPath, 260, "%sSelect_Add.NIF", "Meshes\\Terminals\\NV_Caravan\\");
    pCursors[0] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    CaravanMenu::spModel->AttachChild(pCursors[0], 1);
    BSsprintf(modelPath, 260, "%sSelect_Remove.NIF", "Meshes\\Terminals\\NV_Caravan\\");
    pCursors[1] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    CaravanMenu::spModel->AttachChild(pCursors[1], 1);
    BSsprintf(modelPath, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Player_Deck.NIF");
    pHandGeometry[0] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    BSsprintf(modelPath, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Opponent_Deck.NIF");
    pHandGeometry[1] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
    CaravanMenu::spModel->AttachChild(pHandGeometry[0], 1);
    CaravanMenu::spModel->AttachChild(pHandGeometry[1], 1);
    for (int32_t i = 0; i < 6; ++i) {
        pTrackGrids[i]->SetAppCulled(true);
        NiUpdateData UpdateData(0.0f, 0, 0);
        pTrackGrids[i]->Update(UpdateData);
    }
    NiUpdateData UpdateData1(0.0f, 0, 0);
    pCursors[0]->Update(UpdateData1);
    NiUpdateData UpdateData2(0.0f, 0, 0);
    pCursors[1]->Update(UpdateData2);
    pCursors[0]->SetTranslate(0.0f, 0.0f, -5.0f);
    pCursors[1]->SetTranslate(0.0f, 0.0f, -5.0f);
    NiUpdateData UpdateData3(0.0f, 0, 0);
    pCursors[0]->Update(UpdateData3);
    NiUpdateData UpdateData4(0.0f, 0, 0);
    pCursors[1]->Update(UpdateData4);
    for (int32_t i = 0; i < 2; ++i) {
        pTimeController = pHandGeometry[i]->GetController(&NiControllerManager::ms_RTTI);
        if (pTimeController) {
            pHandAnimManager[i] = static_cast<NiControllerManager*>(pTimeController);
            pHandAnimations[i][0] = pHandAnimManager[i]->GetSequenceByName("Deck-Cards_In");
            pHandAnimations[i][1] = pHandAnimManager[i]->GetSequenceByName("Deck-Card1_Out");
            pHandAnimations[i][2] = pHandAnimManager[i]->GetSequenceByName("Deck-Card2_Out");
            pHandAnimations[i][3] = pHandAnimManager[i]->GetSequenceByName("Deck-Card3_Out");
            pHandAnimations[i][4] = pHandAnimManager[i]->GetSequenceByName("Deck-Card4_Out");
            pHandAnimations[i][5] = pHandAnimManager[i]->GetSequenceByName("Deck-Card5_Out");
            pStartAnimations[i][0] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card1_Out");
            pStartAnimations[i][1] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card2_Out");
            pStartAnimations[i][2] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card3_Out");
            pStartAnimations[i][3] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card4_Out");
            pStartAnimations[i][4] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card5_Out");
            pStartAnimations[i][5] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card6_Out");
            pStartAnimations[i][6] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card7_Out");
            pStartAnimations[i][7] = pHandAnimManager[i]->GetSequenceByName("Deck8-Card8_Out");
            pHandAnimManager[i]->DeactivateAll(0.0f);
            NiTimeController::StartAnimations(pHandGeometry[i]);
        }
    }
    for (int32_t i = 0; i < 6; ++i) {
        for (int32_t j = 0; j < 7; ++j) {
            pTimeController = pTrackGeometry[i][j]->GetController(&NiControllerManager::ms_RTTI);
            if (pTimeController) {
                pTrackAnimManager[i][j] = static_cast<NiControllerManager*>(pTimeController);
                pTrackAnimations[i][j][0][0] = pTrackAnimManager[i][j]->GetSequenceByName("Card1_In");
                pTrackAnimations[i][j][1][0] = pTrackAnimManager[i][j]->GetSequenceByName("Card2_In");
                pTrackAnimations[i][j][2][0] = pTrackAnimManager[i][j]->GetSequenceByName("Card3_In");
                pTrackAnimations[i][j][3][0] = pTrackAnimManager[i][j]->GetSequenceByName("Card4_In");
                pTrackAnimations[i][j][0][1] = pTrackAnimManager[i][j]->GetSequenceByName("Card1_Out");
                pTrackAnimations[i][j][1][1] = pTrackAnimManager[i][j]->GetSequenceByName("Card2_Out");
                pTrackAnimations[i][j][2][1] = pTrackAnimManager[i][j]->GetSequenceByName("Card3_Out");
                pTrackAnimations[i][j][3][1] = pTrackAnimManager[i][j]->GetSequenceByName("Card4_Out");
                pTrackAnimManager[i][j]->DeactivateAll(0.0f);
                for (int32_t k = 0; k < 4; ++k) {
                    pTrackAnimations[i][j][k][0]->SetCycleType(NiTimeController::CLAMP);
                    pTrackAnimations[i][j][k][1]->SetCycleType(NiTimeController::CLAMP);
                }
                NiTimeController::StartAnimations(pTrackGeometry[i][j]);
                pCardTracks[i][j].Clear(true);
            }
        }
    }
    NiUpdateData UpdateData(0.0f, 0, 0);
    CaravanMenu::spModel->Update(UpdateData);
    for (int32_t i = 0; i < 6; ++i) {
        trackInfo[i].iValue = 0;
        trackInfo[i].eCardSuit = 5;
        trackInfo[i].eDirection = 0;
        UpdateTrackValue(i);
    }
    SetFlag(CaravanMenu::CAF_TRACK_SETUP, true);
    UpdateGameHandTextures(true, true);
    UpdateGameHandTextures(false, true);
    pSelectedCard = nullptr;
    pSelectedObject = nullptr;
    pSelectedShadow = nullptr;
    ZeroFlags();
    SetFlag(CaravanMenu::CAF_AI, true);
    SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, true);
    uint32_t uiLosses = pPlayer->GetCaravanLosses();
    uint32_t uiWinnings = pPlayer->GetCaravanWinnings();
    BSsprintf(acWinLossText, 260, "%i/%i", uiWinnings, uiLosses);
    pTiles[16]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[16]->Set(CaravanMenu::iUT_Value, 2 * iAnte);
    pTiles[17]->Poke(CaravanMenu::iUT_Value, 1.0f);
    uint32_t uiCapWinnings = pPlayer->GetCaravanCapWinnings();
    int32_t iValue = uiCapWinnings - pPlayer->GetCaravanCapLosses();
    pTiles[17]->Set(CaravanMenu::iUT_Value, iValue);
    pTiles[18]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[18]->Set(CaravanMenu::iUT_Value, acWinLossText, true);
    pTiles[26]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[26]->Set(CaravanMenu::iUT_Value, 0);
    pTiles[27]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[27]->Set(CaravanMenu::iUT_Value, 0);
    pTiles[28]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[28]->Set(CaravanMenu::iUT_Value, 0);
    pTiles[23]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[23]->Set(CaravanMenu::iUT_Value, 0);
    pTiles[24]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[24]->Set(CaravanMenu::iUT_Value, 0);
    pTiles[25]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[25]->Set(CaravanMenu::iUT_Value, 0);
    pTiles[29]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[29]->Set(CaravanMenu::iUT_Value, pPlayerDeck.iSize);
    pTiles[30]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[30]->Set(CaravanMenu::iUT_Value, pNPCDeck.iSize);
    eActiveMenu = CaravanMenu::CA_GAME_MENU;
    eGameState = CaravanMenu::CA_CAMERA_TO_GAME_ANIM;
    Tile* pInfoRect = pTiles[2]->GetChildByName("CGM_InfoRect");
    if (pInfoRect) {
        float fWidth = pInfoRect->GetFloat(4017) + 150.0;
        pInfoRect->Set(4017, fWidth, true);
        float fX = pInfoRect->GetFloat(4001) - 150.0;
        pInfoRect->Set(4001, fX, true);
        pInfoRect->AddNeedsUpdate(17);
    }
    const char* pBackText = sBack.String();
    pTiles[22]->Set(4036, pBackText, true);
    pTiles[0]->Set(4003, 0);
    pTiles[1]->Set(4003, 0);
    pTiles[2]->Set(4003, 1);
    pTiles[3]->Set(4003, 0);
    const char* pAntePotText = sAntePotText.String();
    pTiles[44]->Set(4036, pAntePotText, true);
    const char* pNetTotalText = sNetTotalText.String();
    pTiles[45]->Set(4036, pNetTotalText, true);
    const char* pWinLossText = sWinLossText.String();
    pTiles[46]->Set(4036, pWinLossText, true);
    const char* pSelectCardText = sSelectCardText.String();
    pTiles[19]->Set(4036, pSelectCardText, true);
    const char* pDiscardCardText = sDiscardCardText.String();
    pTiles[20]->Set(4036, pDiscardCardText, true);
    const char* pDiscardTrackText = sDiscardTrackText.String();
    pTiles[21]->Set(4036, pDiscardTrackText, true);
    const char* pCancelText = sCancel.String();
    pTiles[22]->Set(4036, pCancelText, true);
    UpdateCaravanFlags();
}

void CaravanMenu::PrepareResultsMenu() {
    const char* pResultText;
    char acWinLossText[268];
    eGameState = CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM;
    if (this->IsFlagSet(CaravanMenu::CAF_PLAYERWIN)) {
        uint32_t uiCapWinnings = pPlayer->GetCaravanCapWinnings();
        pPlayer->SetCaravanCapWinnings(static_cast<int32_t>(iAnte + uiCapWinnings));
        uint32_t uiWinnings = pPlayer->GetCaravanWinnings();
        pPlayer->SetCaravanWinnings(uiWinnings + 1);
        if (iAnte > pPlayer->GetCaravanLargestWinning()) {
            pPlayer->SetCaravanLargestWinning(iAnte);
        }
        MiscStatManager::Increment(MiscStatManager::MS_CARAVAN_GAMES_WON);
        BSAudio::QInstance().GetSoundHandleByName("GAMECaravanWin", 289u).Play(false);
    }
    else {
        uint32_t uiCapLosses = pPlayer->GetCaravanCapLosses();
        pPlayer->SetCaravanCapLosses(static_cast<int32_t>(iAnte + uiCapLosses));
        uint32_t uiLosses = pPlayer->GetCaravanLosses();
        pPlayer->SetCaravanLosses(uiLosses + 1);
        MiscStatManager::Increment(MiscStatManager::MS_CARAVAN_GAMES_LOST);
        BSAudio::QInstance().GetSoundHandleByName("GAMECaravanLose", 289u).Play(false);
    }
    Tile* pInfoRect = pTiles[3]->GetChildByName("CRM_InfoRect");
    if (pInfoRect) {
        float fWidth = pInfoRect->GetFloat(4017) + 150.0;
        pInfoRect->Set(4017, fWidth, true);
        pInfoRect->AddNeedsUpdate(16);
    }
    uint32_t uiLosses = pPlayer->GetCaravanLosses();
    BSsprintf(acWinLossText, 260, "%i/%i", pPlayer->GetCaravanWinnings(), uiLosses);
    pTiles[31]->Poke(CaravanMenu::iUT_Value, 1.0f);
    uint32_t uiCapWinnings = pPlayer->GetCaravanCapWinnings();
    pTiles[31]->Set(CaravanMenu::iUT_Value, uiCapWinnings);
    pTiles[32]->Poke(CaravanMenu::iUT_Value, 1.0f);
    uint32_t uiCapLosses = pPlayer->GetCaravanCapLosses();
    pTiles[32]->Set(CaravanMenu::iUT_Value, uiCapLosses);
    pTiles[33]->Poke(CaravanMenu::iUT_Value, 1.0f);
    pTiles[33]->Set(CaravanMenu::iUT_Value, acWinLossText, true);
    pTiles[34]->Poke(CaravanMenu::iUT_Value, 1.0f);
    uint32_t uiLargestWinning = pPlayer->GetCaravanLargestWinning();
    pTiles[34]->Set(CaravanMenu::iUT_Value, uiLargestWinning);
    if (this->IsFlagSet(CaravanMenu::CAF_PLAYERWIN)) {
        pResultText = sCaravanYouWinText.String();
    }
    else {
        pResultText = sCaravanYouLoseText.String();
    }
    pTiles[35]->Set(4036, pResultText, true);
    pTiles[0]->Set(4003, 0);
    pTiles[1]->Set(4003, 0);
    pTiles[2]->Set(4003, 0);
    pTiles[3]->Set(4003, 1);
    const char* pWinningsText = sCaravanWinningsText.String();
    pTiles[47]->Set(4036, pWinningsText, true);
    const char* pLossesText = sCaravanLossesText.String();
    pTiles[48]->Set(4036, pLossesText, true);
    const char* pWinLossText = sCaravanWinLossText.String();
    pTiles[49]->Set(4036, pWinLossText, true);
    const char* pBiggestAnteText = sCaravanBiggestAnteText.String();
    pTiles[50]->Set(4036, pBiggestAnteText, true);
    RewardOrChargePlayer(this->IsFlagSet(CaravanMenu::CAF_PLAYERWIN));
}

void CaravanMenu::FlipSelectedCardTextures(int32_t aiTarget) {
    if (deckFaceShaders[12] && deckFaceShaders[36] && deckFaceShaders[60] && deckFaceShaders[84]) {
        char acInPath[268];
        // Language-swapped path buffer.
        char acPath[268];
        int32_t iIndex = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
        if (pDeckArray[iIndex]->bIsActive) {
            NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
            BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->faceTexture.GetTextureName());
            SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
            if (spTexture.m_pObject) {
                deckFaceShaders[aiTarget]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
            }
            BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->backTexture.GetTextureName());
            SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
            if (spTexture.m_pObject) {
                deckFaceShaders[aiTarget + 24]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
            }
            BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->backTexture.GetTextureName());
            SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
            if (spTexture.m_pObject) {
                deckFaceShaders[aiTarget + 48]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
            }
            BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->faceTexture.GetTextureName());
            SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
            TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
            if (spTexture.m_pObject) {
                deckFaceShaders[aiTarget + 72]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
            }
        }
        else {
            // Only the call is kept: the card's faceTexture address tested after it is never null.
            pDeckArray[iIndex];
            {
                NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
                BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->faceTexture.GetTextureName());
                SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                if (spTexture.m_pObject) {
                    deckFaceShaders[aiTarget + 48]
                        ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
                BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->backTexture.GetTextureName());
                SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                if (spTexture.m_pObject) {
                    deckFaceShaders[aiTarget + 72]
                        ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
                BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->backTexture.GetTextureName());
                SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                if (spTexture.m_pObject) {
                    deckFaceShaders[aiTarget]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
                BSsprintf(acInPath, 260, "%s%s", "Textures\\", pDeckArray[iIndex]->faceTexture.GetTextureName());
                SwapPlatformLanguageTexturePath(acInPath, acPath, 260u);
                TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                if (spTexture.m_pObject) {
                    deckFaceShaders[aiTarget + 24]
                        ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
            }
        }
    }
}

// Pays the ante: with abReward the challenger's caps in the barter container go to the player, else the player's go to
// the challenger; what the money items do not cover is paid in gold (form 0xF). The challenger's base form is passed as
// the vendor reference.
void CaravanMenu::RewardOrChargePlayer(char abReward) {
    int32_t iNumber = iAnte;
    if (abReward) {
        pDataHandler->CreateBarterContainer(pChallenger);
        InventoryChanges* pChanges = pDataHandler->GetBarterContainer();
        TESBoundObject* pObject = nullptr;
        for (BSSimpleList<ItemChange*>* pNode = pChanges->pListofChanges; pNode && pNode->GetItem();
             pNode = pNode->GetNext()) {
            int32_t iCount = 0;
            pObject = pNode->GetItem()->pContainerObj;
            if (pObject && pObject->GetFormType() == CMNY_ID) {
                if (pNode->GetItem()->iNumber > 0) {
                    int32_t iValue = TESValueForm::GetFormValue(pObject);
                    iCount = pNode->GetItem()->iNumber;
                    if (iValue * iCount > iNumber) {
                        iCount = iNumber / iValue;
                    }
                    if (iCount > 0) {
                        TESContainer kContainer;
                        kContainer.AddObject(pObject, iCount, nullptr);
                        kContainer.AddContainerToReference(pPlayer, false);
                        TESObjectREFR* pVendor = static_cast<TESObjectREFR*>(
                            static_cast<TESForm*>(pChallenger->GetObjectReference())
                        );
                        pDataHandler->RemoveItemBarterContainer(pNode->GetItem(), iCount, pVendor);
                        iNumber -= iCount * iValue;
                    }
                }
            }
            else {
                pObject = nullptr;
            }
        }
        if (iNumber) {
            TESContainer kGoldContainer;
            kGoldContainer.AddObject(static_cast<TESObject*>(TESForm::GetFormByNumericID(15u)), iNumber, nullptr);
            kGoldContainer.AddContainerToReference(pPlayer, false);
            ItemChange kGold(static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u)), 0);
            if (pChanges->GetGoldAmount() > iNumber) {
                TESObjectREFR* pVendor = static_cast<TESObjectREFR*>(
                    static_cast<TESForm*>(pChallenger->GetObjectReference())
                );
                pDataHandler->RemoveItemBarterContainer(&kGold, iNumber, pVendor);
            }
            else {
                TESObjectREFR* pVendor = static_cast<TESObjectREFR*>(
                    static_cast<TESForm*>(pChallenger->GetObjectReference())
                );
                int32_t iGold = pChanges->GetGoldAmount();
                pDataHandler->RemoveItemBarterContainer(&kGold, iGold, pVendor);
                int32_t iRest = iNumber - pChanges->GetGoldAmount();
                TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
                pChallenger->RemoveItem(pGold, nullptr, iRest, false, false, nullptr, nullptr, nullptr, true, false);
            }
        }
    }
    else {
        pDataHandler->CreateBarterContainer(pPlayer);
        InventoryChanges* pChanges = pDataHandler->GetBarterContainer();
        TESBoundObject* pObject = nullptr;
        for (BSSimpleList<ItemChange*>* pNode = pChanges->pListofChanges; pNode && pNode->GetItem();
             pNode = pNode->GetNext()) {
            int32_t iCount = 0;
            pObject = pNode->GetItem()->pContainerObj;
            if (pObject && pObject->GetFormType() == CMNY_ID) {
                if (pNode->GetItem()->iNumber > 0) {
                    int32_t iValue = TESValueForm::GetFormValue(pObject);
                    iCount = pNode->GetItem()->iNumber;
                    if (iValue * iCount > iNumber) {
                        iCount = iNumber / iValue;
                    }
                    if (iCount > 0) {
                        TESContainer kContainer;
                        kContainer.AddObject(pObject, iCount, nullptr);
                        kContainer.AddContainerToReference(pChallenger, false);
                        pPlayer->RemoveItem(
                            pObject,
                            nullptr,
                            iCount,
                            false,
                            false,
                            nullptr,
                            nullptr,
                            nullptr,
                            true,
                            false
                        );
                        iNumber -= iCount * iValue;
                    }
                }
            }
            else {
                pObject = nullptr;
            }
        }
        if (iNumber) {
            TESContainer kGoldContainer;
            kGoldContainer.AddObject(static_cast<TESObject*>(TESForm::GetFormByNumericID(15u)), iNumber, nullptr);
            kGoldContainer.AddContainerToReference(pChallenger, false);
            if (pPlayer->GetGoldAmount() > iNumber) {
                TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
                pPlayer->RemoveItem(pGold, nullptr, iNumber, false, false, nullptr, nullptr, nullptr, true, false);
            }
            else {
                int32_t iGold = pPlayer->GetGoldAmount();
                TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
                pPlayer->RemoveItem(pGold, nullptr, iGold, false, false, nullptr, nullptr, nullptr, true, false);
            }
        }
    }
}

void CaravanMenu::UpdateGameHandTextures(bool abPlayer, bool bStartup) {
    BSShaderPPLightingProperty* pFaceShader;
    NiAVObject* pFace;
    char acPath[268];
    char blockName[268];
    int32_t iHandSize = abPlayer ? pPlayerHand.iSize : pNPCHand.iSize;
    NiAVObject* pHand = abPlayer ? pHandGeometry[0] : pHandGeometry[1];
    for (int32_t i = 0; i < 8; ++i) {
        if (abPlayer) {
            BSsprintf(blockName, 260, "Player-Deck_0%i:%i", i + 1, i);
        }
        else {
            BSsprintf(blockName, 260, "Opponent-Deck_0%i:%i", i + 1, i);
        }
        pFace = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
        if (i >= iHandSize) {
            if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) || i == 5) {
                if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) && iHandSize < 5) {
                    pFace->SetAppCulled(true);
                    NiUpdateData UpdateData1(0.0f, false, false);
                    pFace->Update(UpdateData1);
                    if (abPlayer) {
                        BSsprintf(blockName, 260, "Player-Deck_0%i_Shadow:0", i + 1);
                    }
                    else {
                        BSsprintf(blockName, 260, "Opponent-Deck_0%i_Shadow:0", i + 1);
                    }
                    pFace = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
                    pFace->SetAppCulled(true);
                    NiUpdateData UpdateData2(0.0f, false, false);
                    pFace->Update(UpdateData2);
                }
            }
            else {
                pFace->SetAppCulled(true);
                NiUpdateData UpdateData1(0.0f, false, false);
                pFace->Update(UpdateData1);
                if (abPlayer) {
                    BSsprintf(blockName, 260, "Player-Deck_0%i_Shadow:0", i + 1);
                }
                else {
                    BSsprintf(blockName, 260, "Opponent-Deck_0%i_Shadow:0", i + 1);
                }
                pFace = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
                pFace->SetAppCulled(true);
                NiUpdateData UpdateData2(0.0f, false, false);
                pFace->Update(UpdateData2);
            }
        }
        else {
            int32_t iIsPPLighting;
            if (pFace->GetProperty(3)) {
                BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
                int32_t iShaderInRange = false;
                if (pProperty->iShaderPropertyType >= 8) {
                    pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
                    if (pProperty->iShaderPropertyType <= 12) {
                        iShaderInRange = true;
                    }
                }
                iIsPPLighting = iShaderInRange;
            }
            else {
                iIsPPLighting = 0;
            }
            if (iIsPPLighting) {
                pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
            }
            else {
                pFaceShader = nullptr;
            }
            NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
            if (pFaceShader) {
                if (abPlayer) {
                    BSsprintf(blockName, 260, "Textures\\%s", pPlayerHand[i]->faceTexture.GetTextureName());
                }
                else if (CaravanMenu::bDebugAI) {
                    BSsprintf(blockName, 260, "Textures\\%s", pNPCHand[i]->faceTexture.GetTextureName());
                }
                else {
                    BSsprintf(blockName, 260, "Textures\\%s", pNPCHand[i]->backTexture.GetTextureName());
                }
                SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
                TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                if (spTexture.m_pObject) {
                    pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
            }
        }
    }
    if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
        int32_t iIsPPLighting;
        if (abPlayer) {
            BSstrcpy(blockName, 260, "Player-Deck_06:5");
        }
        else {
            BSstrcpy(blockName, 260, "Opponent-Deck_06:5");
        }
        pFace = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
        if (pFace->GetProperty(3)) {
            BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
            int32_t iShaderInRange = false;
            if (pProperty->iShaderPropertyType >= 8) {
                pProperty = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
                if (pProperty->iShaderPropertyType <= 12) {
                    iShaderInRange = true;
                }
            }
            iIsPPLighting = iShaderInRange;
        }
        else {
            iIsPPLighting = 0;
        }
        if (iIsPPLighting) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
        }
        else {
            pFaceShader = nullptr;
        }
        NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
        if (pFaceShader) {
            int32_t iDeckHasCards = abPlayer ? pPlayerDeck.iSize != 0 : pNPCDeck.iSize != 0;
            if (iDeckHasCards) {
                if (abPlayer) {
                    BSsprintf(
                        blockName,
                        260,
                        "Textures\\%s",
                        pPlayerDeck[iNextPlayerCard]->backTexture.GetTextureName()
                    );
                }
                else {
                    BSsprintf(blockName, 260, "Textures\\%s", pNPCDeck[iNextNPCCard]->backTexture.GetTextureName());
                }
                SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
                TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
                if (spTexture.m_pObject) {
                    pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
                }
            }
        }
    }
}

void CaravanMenu::UpdateSelectedTrackCardTexture() {
    BSShaderPPLightingProperty* pFaceShader;
    int32_t iIsPPLighting;
    char acPath[264];
    char blockName[264];
    NiAVObject* pTrack = pTrackGeometry[iSelectedTrack][iSelectedRow];
    int32_t iCardNumber = iSelectedTrack < 3 ? iSelectedColumn + 1 : 4 - iSelectedColumn;
    BSsprintf(blockName, 260, "Card_0%i:%i", iCardNumber, iSelectedColumn);
    NiAVObject* pFace = BSUtilities::GetObjectByName(pTrack, static_cast<const char*>(blockName));
    if (pFace->GetProperty(3)) {
        pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
        int32_t iShaderInRange = false;
        if (pFaceShader->iShaderPropertyType >= 8) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
            if (pFaceShader->iShaderPropertyType <= 12) {
                iShaderInRange = true;
            }
        }
        iIsPPLighting = iShaderInRange;
    }
    else {
        iIsPPLighting = 0;
    }
    if (iIsPPLighting) {
        pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
    }
    else {
        pFaceShader = nullptr;
    }
    NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
    if (pFaceShader) {
        BSsprintf(blockName, 260, "%s%s", "Textures\\", pSelectedCard->faceTexture.GetTextureName());
        SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
        if (spTexture.m_pObject) {
            pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
        }
    }
}

void CaravanMenu::UpdateTrackCardTexture(int32_t aiTrack, int32_t aiRow, int32_t aiColumn) {
    BSShaderPPLightingProperty* pFaceShader;
    int32_t iIsPPLighting;
    char acPath[264];
    char blockName[264];
    NiAVObject* pTrack = pTrackGeometry[aiTrack][aiRow];
    if (aiTrack < 3) {
        BSsprintf(blockName, 260, "Card_0%i:%i", aiColumn + 1, aiColumn);
    }
    else {
        BSsprintf(blockName, 260, "Card_0%i:%i", 4 - aiColumn, aiColumn);
    }
    NiAVObject* pFace = BSUtilities::GetObjectByName(pTrack, static_cast<const char*>(blockName));
    if (pFace->GetProperty(3)) {
        pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
        int32_t iShaderInRange = false;
        if (pFaceShader->iShaderPropertyType >= 8) {
            pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
            if (pFaceShader->iShaderPropertyType <= 12) {
                iShaderInRange = true;
            }
        }
        iIsPPLighting = iShaderInRange;
    }
    else {
        iIsPPLighting = 0;
    }
    if (iIsPPLighting) {
        pFaceShader = static_cast<BSShaderPPLightingProperty*>(pFace->GetProperty(3));
    }
    else {
        pFaceShader = nullptr;
    }
    NiPointer<NiSourceTexture> spTexture(static_cast<NiSourceTexture*>(nullptr));
    if (pFaceShader) {
        BSsprintf(
            blockName,
            260,
            "%s%s",
            "Textures\\",
            pCardTracks[aiTrack][aiRow][aiColumn]->faceTexture.GetTextureName()
        );
        SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spTexture, false, false);
        if (spTexture.m_pObject) {
            pFaceShader->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexture.m_pObject);
        }
    }
}

int32_t CaravanMenu::GetRowValue(int32_t aiTrack, int32_t aiRow) {
    int32_t iValue = 0;
    int32_t iSize = pCardTracks[aiTrack][aiRow].iSize;
    if (iSize > 0) {
        iValue = pCardTracks[aiTrack][aiRow][0]->data.iFaceValue;
        for (int32_t i = 1; i < iSize; ++i) {
            if (pCardTracks[aiTrack][aiRow][i]->data.iFaceValue == 14) {
                iValue *= 2;
            }
        }
    }
    return iValue;
}

// Each bound is tested signed, and the deck card index is tested against the hand's last index unsigned.
bool CaravanMenu::IsValidCardPlacement(int32_t aiTrack, int32_t aiRow, int32_t aiDeckCard, bool abIsNPC) {
    if (aiTrack < 0 || aiTrack > 5 || aiRow < 0 || aiRow > 6 || aiDeckCard < 0) {
        return false;
    }
    uint32_t uiLastCard = abIsNPC ? pNPCHand.iSize - 1 : pPlayerHand.iSize - 1;
    if (static_cast<uint32_t>(aiDeckCard) > uiLastCard) {
        return false;
    }
    int32_t iFaceValue;
    if (abIsNPC) {
        iFaceValue = pNPCHand[aiDeckCard]->data.iFaceValue;
    }
    else {
        iFaceValue = pPlayerHand[aiDeckCard]->data.iFaceValue;
    }
    int32_t iCardSuit;
    if (abIsNPC) {
        iCardSuit = pNPCHand[aiDeckCard]->data.iCardSuit;
    }
    else {
        iCardSuit = pPlayerHand[aiDeckCard]->data.iCardSuit;
    }
    if (IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
        return !pCardTracks[aiTrack][aiRow].iSize && iFaceValue <= 10;
    }
    if (iFaceValue <= 10) {
        if (pCardTracks[aiTrack][aiRow].iSize) {
            return false;
        }
        if (abIsNPC && aiTrack < 3) {
            return false;
        }
        if (!abIsNPC && aiTrack > 2) {
            return false;
        }
        if (aiRow) {
            if (!pCardTracks[aiTrack][aiRow - 1].iSize) {
                return false;
            }
            int32_t iPrevValue = pCardTracks[aiTrack][aiRow - 1][0]->data.iFaceValue;
            if (iPrevValue == iFaceValue) {
                return false;
            }
            if (trackInfo[aiTrack].eDirection == 1 &&
                iPrevValue >= iFaceValue &&
                trackInfo[aiTrack].eCardSuit != iCardSuit) {
                return false;
            }
            if (trackInfo[aiTrack].eDirection == -1 &&
                iPrevValue <= iFaceValue &&
                trackInfo[aiTrack].eCardSuit != iCardSuit) {
                return false;
            }
        }
    }
    else {
        int32_t iSize = pCardTracks[aiTrack][aiRow].iSize;
        if (iSize < 1 || iSize > 3) {
            return false;
        }
        if (aiRow < 6 && pCardTracks[aiTrack][aiRow + 1].iSize && iFaceValue == 13) {
            return false;
        }
    }
    return true;
}

// The one-field setters and getters of CaravanAIPackage, BSSimpleArray and TESCaravanCard are trivial accessors,
// written as the fields.
// The exe's cleanup-only C++ EH frame is left out.
CaravanAIPackage* CaravanMenu::ProcessAI(CaravanAIPackage* apResult) {
    for (int32_t iTrack = 0; iTrack < 6; ++iTrack) {
        UpdateTrackValue(iTrack);
    }
    if (IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
        // Starts zeroed (a departure), as the packages the AI weighs below do: it is copied whole to apResult with
        // only iActionType and (on most paths) iDeckCard written; the exe copies the other fields unset.
        CaravanAIPackage kSetupPackage{};
        kSetupPackage.iActionType = 4;
        if (iDiffLevel >= 1) {
            if (iDiffLevel >= 3) {
                int32_t iNumCards = 0;
                for (TESCaravanCard* pCard : pNPCHand) {
                    if (pCard->data.iFaceValue <= 10) {
                        ++iNumCards;
                    }
                }
                if (pNPCHand.iSize - iNumCards <= 5) {
                    int32_t iDistance = std::abs(pNPCHand[0]->data.iFaceValue - 6);
                    kSetupPackage.iDeckCard = 0;
                    for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
                        if (std::abs(pNPCHand[i]->data.iFaceValue - 6) < iDistance) {
                            kSetupPackage.iDeckCard = i;
                            iDistance = std::abs(pNPCHand[i]->data.iFaceValue - 6);
                        }
                    }
                }
                else {
                    int32_t iValue = 0;
                    kSetupPackage.iActionType = 6;
                    for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
                        if (pNPCHand[i]->data.iFaceValue > 10) {
                            if (pNPCHand[i]->data.iFaceValue == 15) {
                                kSetupPackage.iDeckCard = i;
                                break;
                            }
                            if (!iValue || iValue == 12) {
                                kSetupPackage.iDeckCard = i;
                                iValue = pNPCHand[i]->data.iFaceValue;
                            }
                            else if (iValue == 14 && pNPCHand[i]->data.iFaceValue != 12) {
                                kSetupPackage.iDeckCard = i;
                                iValue = pNPCHand[i]->data.iFaceValue;
                            }
                        }
                    }
                }
            }
            else {
                int32_t iHiCards = 0;
                int32_t iLowCards = 0;
                for (TESCaravanCard* pCard : pNPCHand) {
                    if (pCard->data.iFaceValue > 5 && pCard->data.iFaceValue <= 10) {
                        ++iHiCards;
                    }
                    else if (pCard->data.iFaceValue < 4) {
                        ++iLowCards;
                    }
                }
                if (pNPCHand.iSize - iLowCards - iHiCards <= 5) {
                    for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
                        if (iHiCards > iLowCards) {
                            if (pNPCHand[i]->data.iFaceValue > 5 && pNPCHand[i]->data.iFaceValue <= 10) {
                                kSetupPackage.iDeckCard = i;
                                break;
                            }
                        }
                        else if (iLowCards && pNPCHand[i]->data.iFaceValue < 4) {
                            kSetupPackage.iDeckCard = i;
                            break;
                        }
                    }
                }
                else {
                    int32_t iValue = 0;
                    kSetupPackage.iActionType = 6;
                    for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
                        if (pNPCHand[i]->data.iFaceValue > 10) {
                            if (pNPCHand[i]->data.iFaceValue == 15) {
                                kSetupPackage.iDeckCard = i;
                                break;
                            }
                            if (!iValue || iValue == 12) {
                                kSetupPackage.iDeckCard = i;
                                iValue = pNPCHand[i]->data.iFaceValue;
                            }
                            else if (iValue == 14 && pNPCHand[i]->data.iFaceValue != 12) {
                                kSetupPackage.iDeckCard = i;
                                iValue = pNPCHand[i]->data.iFaceValue;
                            }
                        }
                    }
                }
            }
        }
        else {
            int32_t iNumCards = 0;
            int32_t iFaceCardIndex = 0;
            int32_t iNumCardIndex = 0;
            for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
                if (pNPCHand[i]->data.iFaceValue > 10) {
                    iFaceCardIndex = i;
                }
                else {
                    ++iNumCards;
                    iNumCardIndex = i;
                }
            }
            if (pNPCHand.iSize - iNumCards <= 5) {
                kSetupPackage.iDeckCard = iNumCardIndex;
            }
            else {
                kSetupPackage.iActionType = 6;
                kSetupPackage.iDeckCard = iFaceCardIndex;
            }
        }
        *apResult = kSetupPackage;
        return apResult;
    }

    BSSimpleArray<CaravanAIPackage*>* pPackageList;
    pPackageList = new BSSimpleArray<CaravanAIPackage*>;
    // Each package the AI weighs is added to the list, and a fresh zeroed one takes its place.
    CaravanAIPackage* pPackage = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
    if (pPackage) {
        std::memset(pPackage, 0, sizeof(CaravanAIPackage));
    }
    CaravanAIPackage* pNew;
    for (int32_t iTrack = 3; iTrack < 6; ++iTrack) {
        pPackage->iTrack = iTrack;
        int32_t iTotal = trackInfo[iTrack].iValue;
        if (CalculateWinningState(iTotal) >= 0) {
            if (CalculateWinningState(iTotal)) {
                if (CalculateWinningState(iTotal) > 0) {
                    for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
                        pPackage->iDeckCard = iDeckCard;
                        if (pNPCHand[iDeckCard]->data.iFaceValue != 12) {
                            continue;
                        }
                        for (int32_t iRow = 0; iRow < 7; ++iRow) {
                            pPackage->iRow = iRow;
                            int32_t iRowVal = GetRowValue(iTrack, iRow);
                            if (iTotal - iRowVal < 27 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                                pPackage->iValueGoal = iTotal - iRowVal;
                                pPackage->iCalcResults = CalculateWinningState(iTotal - iRowVal);
                                pPackage->iPriority = 100 - (iRowVal + (26 - iTotal));
                                pPackage->iActionType = 2;
                                pPackageList->Add(pPackage);
                                pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                if (pNew) {
                                    std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                }
                                pPackage = pNew;
                                pPackage->iRow = iRow;
                                pPackage->iDeckCard = iDeckCard;
                                pPackage->iTrack = iTrack;
                            }
                        }
                    }
                }
            }
            else if (iTotal >= trackInfo[5 - iTrack].iValue || trackInfo[5 - iTrack].iValue >= 27) {
                for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
                    pPackage->iDeckCard = iDeckCard;
                    int32_t iFaceValue = pNPCHand[iDeckCard]->data.iFaceValue;
                    if (iFaceValue <= 10) {
                        int32_t iRow;
                        for (iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {}
                        pPackage->iRow = iRow;
                        if (IsValidCardPlacement(iTrack, iRow, iDeckCard, true) &&
                            CalculateWinningState(trackInfo[iTrack].iValue + iFaceValue) < 1) {
                            pPackage->iValueGoal = trackInfo[iTrack].iValue + iFaceValue;
                            pPackage->iCalcResults = CalculateWinningState(trackInfo[iTrack].iValue + iFaceValue);
                            pPackage->iPriority = trackInfo[iTrack].iValue + iFaceValue;
                            pPackage->iActionType = 0;
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                    }
                    else if (pNPCHand[iDeckCard]->data.iFaceValue == 14) {
                        for (int32_t iRow = 0; iRow < 7; ++iRow) {
                            pPackage->iRow = iRow;
                            int32_t iRowVal = GetRowValue(iTrack, iRow);
                            if (CalculateWinningState(iTotal + 2 * iRowVal) < 1 &&
                                IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                                pPackage->iValueGoal = iTotal + 2 * iRowVal;
                                pPackage->iCalcResults = CalculateWinningState(iTotal + 2 * iRowVal);
                                pPackage->iPriority = iTotal + 2 * iRowVal;
                                pPackage->iActionType = 0;
                                pPackageList->Add(pPackage);
                                pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                if (pNew) {
                                    std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                }
                                pPackage = pNew;
                                pPackage->iRow = iRow;
                                pPackage->iDeckCard = iDeckCard;
                                pPackage->iTrack = iTrack;
                            }
                        }
                    }
                }
            }
            else {
                for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
                    pPackage->iDeckCard = iDeckCard;
                    int32_t iFaceValue = pNPCHand[iDeckCard]->data.iFaceValue;
                    if (iFaceValue <= 10) {
                        int32_t iRow;
                        for (iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {}
                        pPackage->iRow = iRow;
                        int32_t iNewValue = trackInfo[iTrack].iValue + iFaceValue;
                        if (iNewValue < 27 && iNewValue > trackInfo[5 - iTrack].iValue) {
                            pPackage->iPriority = 100 - (iNewValue - trackInfo[5 - iTrack].iValue);
                            pPackage->iActionType = 0;
                        }
                        else if (iNewValue < 27 && iNewValue == trackInfo[5 - iTrack].iValue) {
                            pPackage->iPriority = 50;
                            pPackage->iActionType = 0;
                        }
                        else if (iNewValue < 27) {
                            pPackage->iPriority = 25 - (26 - iNewValue);
                            pPackage->iActionType = 0;
                        }
                        if (IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                            pPackage->iValueGoal = iNewValue;
                            pPackage->iCalcResults = CalculateWinningState(iNewValue);
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                    }
                    else if (iFaceValue == 12) {
                        int32_t diff = trackInfo[5 - iTrack].iValue - iTotal;
                        for (int32_t iRow = 0; iRow < 7; ++iRow) {
                            pPackage->iRow = iRow;
                            if (!pCardTracks[5 - iTrack][iRow].iSize) {
                                break;
                            }
                            int32_t iValue = GetRowValue(5 - iTrack, iRow);
                            if (iValue > diff) {
                                int32_t iRemaining = trackInfo[5 - iTrack].iValue - iValue;
                                if (iRemaining <= 21) {
                                    pPackage->iPriority = iTotal - iRemaining + 75;
                                }
                                else {
                                    pPackage->iPriority = 100 - (iTotal - iRemaining);
                                }
                                if (IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                                    pPackage->iValueGoal = iTotal;
                                    pPackage->iCalcResults = CalculateWinningState(iTotal);
                                    pPackage->iActionType = 7;
                                    pPackageList->Add(pPackage);
                                    pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                    if (pNew) {
                                        std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                    }
                                    pPackage = pNew;
                                    pPackage->iRow = iRow;
                                    pPackage->iDeckCard = iDeckCard;
                                    pPackage->iTrack = iTrack;
                                }
                            }
                        }
                    }
                    else if (iDiffLevel >= 1 && iFaceValue == 15) {
                        for (int32_t iRow = 0; iRow < 7; ++iRow) {
                            pPackage->iRow = iRow;
                            if (!pCardTracks[iTrack][iRow].iSize) {
                                break;
                            }
                            if (!IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                                continue;
                            }
                            int32_t decreaseby[6];
                            int32_t iCardValue = pCardTracks[iTrack][iRow][0]->data.iFaceValue;
                            if (iCardValue == 1) {
                                for (int32_t i = 0; i < 6; ++i) {
                                    decreaseby[i] = GetValueOfCardValueInTrack(i, iCardValue);
                                }
                            }
                            else {
                                for (int32_t i = 0; i < 6; ++i) {
                                    decreaseby[i] = GetSumCardsB44WithSuit(
                                        i,
                                        pCardTracks[iTrack][iRow][0]->data.iFaceValue
                                    );
                                }
                            }
                            decreaseby[iTrack] += iCardValue;
                            if (iDiffLevel >= 3) {
                                int32_t diff[3];
                                for (int32_t i = 0; i < 3; ++i) {
                                    diff[i] = trackInfo[i + 3].iValue - decreaseby[i + 3] - trackInfo[i].iValue -
                                        decreaseby[i];
                                }
                                int32_t iPriority = 0;
                                if (diff[5 - iTrack] > 0) {
                                    iPriority += 75;
                                }
                                for (int32_t i = 0; i < 3; ++i) {
                                    if (trackInfo[i + 3].iValue > 20 &&
                                        trackInfo[i + 3].iValue < 27 &&
                                        trackInfo[i + 3].iValue - decreaseby[i + 3] < 21) {
                                        iPriority -= 25 - (21 - (trackInfo[i + 3].iValue - decreaseby[i + 3]));
                                    }
                                    if (CalculateWinningState(trackInfo[i + 3].iValue) >= 0) {
                                        if (trackInfo[i + 3].iValue < 27 &&
                                            trackInfo[i + 3].iValue - decreaseby[i + 3] < 21) {
                                            iPriority -= 21 - trackInfo[i + 3].iValue - decreaseby[i + 3];
                                        }
                                        if (trackInfo[i + 3].iValue < 27 &&
                                            diff[i] < 0 &&
                                            !CalculateWinningState(trackInfo[i].iValue - decreaseby[i])) {
                                            iPriority -= diff[i] + 25;
                                        }
                                        if (CalculateWinningState(trackInfo[i + 3].iValue) > 0 &&
                                            CalculateWinningState(trackInfo[i + 3].iValue - decreaseby[i + 3]) <= 0) {
                                            iPriority += trackInfo[i + 3].iValue - decreaseby[i + 3];
                                        }
                                    }
                                    if (CalculateWinningState(trackInfo[i + 3].iValue) > 0 && decreaseby[i + 3] > 0) {
                                        int32_t iNewValue = trackInfo[i + 3].iValue - decreaseby[i + 3];
                                        if (!CalculateWinningState(iNewValue)) {
                                            iPriority += iNewValue + 25;
                                        }
                                        else if (CalculateWinningState(iNewValue) < 0) {
                                            iPriority += iNewValue;
                                        }
                                        else {
                                            iPriority += decreaseby[i + 3];
                                        }
                                    }
                                    if (!CalculateWinningState(trackInfo[i].iValue) &&
                                        CalculateWinningState(trackInfo[i].iValue - decreaseby[i]) < 0) {
                                        iPriority += 46 - trackInfo[i].iValue + decreaseby[i];
                                    }
                                    if (iPriority > 0 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                                        pPackage->iPriority = iPriority;
                                        pPackage->iActionType = 3;
                                        pPackageList->Add(pPackage);
                                        pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                        if (pNew) {
                                            std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                        }
                                        pPackage = pNew;
                                        pPackage->iRow = iRow;
                                        pPackage->iDeckCard = iDeckCard;
                                        pPackage->iTrack = iTrack;
                                    }
                                }
                            }
                            else {
                                int32_t diff = trackInfo[iTrack].iValue - decreaseby[iTrack] -
                                    trackInfo[5 - iTrack].iValue - decreaseby[5 - iTrack];
                                if (diff > 0 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                                    pPackage->iPriority = diff + 50;
                                    pPackage->iActionType = 3;
                                    pPackageList->Add(pPackage);
                                    pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                    if (pNew) {
                                        std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                    }
                                    pPackage = pNew;
                                    pPackage->iRow = iRow;
                                    pPackage->iDeckCard = iDeckCard;
                                    pPackage->iTrack = iTrack;
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
                pPackage->iDeckCard = iDeckCard;
                if (pNPCHand[iDeckCard]->data.iFaceValue <= 10) {
                    int32_t iRow;
                    for (iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {}
                    pPackage->iRow = iRow;
                    if (iRow != 7) {
                        int32_t iVal = trackInfo[iTrack].iValue + pNPCHand[iDeckCard]->data.iFaceValue;
                        if (CalculateWinningState(iVal) < 1 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                            pPackage->iValueGoal = iVal;
                            pPackage->iCalcResults = CalculateWinningState(iVal);
                            pPackage->iPriority = iVal;
                            pPackage->iActionType = 0;
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                    }
                }
                else if (pNPCHand[iDeckCard]->data.iFaceValue == 14) {
                    for (int32_t iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
                        pPackage->iRow = iRow;
                        if (pCardTracks[iTrack][iRow].iSize > 3) {
                            continue;
                        }
                        int32_t iVal = trackInfo[iTrack].iValue + GetRowValue(iTrack, iRow);
                        if (CalculateWinningState(iVal) < 1 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
                            pPackage->iValueGoal = iVal;
                            pPackage->iCalcResults = CalculateWinningState(iVal);
                            pPackage->iPriority = iVal + (iDiffLevel != 2 ? 1 : -5);
                            pPackage->iActionType = 0;
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                    }
                }
            }
        }
    }
    for (int32_t iTrack = 0; iTrack < 3; ++iTrack) {
        pPackage->iTrack = iTrack;
        for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
            pPackage->iDeckCard = iDeckCard;
            if (pNPCHand[iDeckCard]->data.iFaceValue <= 10) {
                continue;
            }
            for (int32_t iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
                pPackage->iRow = iRow;
                if (pCardTracks[iTrack][iRow].iSize > 3) {
                    continue;
                }
                switch (pNPCHand[iDeckCard]->data.iFaceValue) {
                    case 12: {
                        int32_t iVal = trackInfo[iTrack].iValue - GetRowValue(iTrack, iRow);
                        if (!CalculateWinningState(trackInfo[iTrack].iValue)) {
                            pPackage->iPriority = 26 - iVal + 25;
                            pPackage->iActionType = 7;
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                        else if (CalculateWinningState(trackInfo[iTrack].iValue) < 0) {
                            pPackage->iPriority = 26 - iVal;
                            pPackage->iActionType = 7;
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                        break;
                    }
                    case 13: {
                        if (iRow > 1 && !pCardTracks[iTrack][iRow].iSize) {
                            int32_t iSum = pCardTracks[iTrack][iRow - 1].GetAt(0)->data.iFaceValue +
                                pCardTracks[iTrack][iRow - 2].GetAt(0)->data.iFaceValue;
                            if (iSum < 8) {
                                if (pCardTracks[iTrack][iRow - 1].GetAt(0)->data.iFaceValue >
                                    pCardTracks[iTrack][iRow - 2].GetAt(0)->data.iFaceValue) {
                                    pPackage->iRow = iRow - 1;
                                    pPackage->iPriority = 35 - iSum;
                                    pPackage->iActionType = 1;
                                    pPackageList->Add(pPackage);
                                    pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                    if (pNew) {
                                        std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                    }
                                    pPackage = pNew;
                                    pPackage->iRow = iRow;
                                    pPackage->iDeckCard = iDeckCard;
                                    pPackage->iTrack = iTrack;
                                }
                            }
                            else if (
                                iSum > 16 &&
                                pCardTracks[iTrack][iRow - 1].GetAt(0)->data.iFaceValue <
                                    pCardTracks[iTrack][iRow - 2].GetAt(0)->data.iFaceValue
                            ) {
                                pPackage->iRow = iRow - 1;
                                pPackage->iPriority = iSum + 25;
                                pPackage->iActionType = 1;
                                pPackageList->Add(pPackage);
                                pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                                if (pNew) {
                                    std::memset(pNew, 0, sizeof(CaravanAIPackage));
                                }
                                pPackage = pNew;
                                pPackage->iRow = iRow;
                                pPackage->iDeckCard = iDeckCard;
                                pPackage->iTrack = iTrack;
                            }
                        }
                        break;
                    }
                    case 14: {
                        int32_t iVal = trackInfo[iTrack].iValue + GetRowValue(iTrack, iRow);
                        if (CalculateWinningState(iVal) == 1) {
                            pPackage->iPriority = iVal - 1;
                            pPackage->iActionType = 7;
                            pPackageList->Add(pPackage);
                            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
                            if (pNew) {
                                std::memset(pNew, 0, sizeof(CaravanAIPackage));
                            }
                            pPackage = pNew;
                            pPackage->iRow = iRow;
                            pPackage->iDeckCard = iDeckCard;
                            pPackage->iTrack = iTrack;
                        }
                        break;
                    }
                }
            }
        }
    }
    if (!pPackageList->iSize) {
        int32_t iHighestCard = 0;
        for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
            if (pNPCHand[i]->data.iFaceValue > pNPCHand[iHighestCard]->data.iFaceValue) {
                iHighestCard = i;
            }
        }
        pPackage->iDeckCard = iHighestCard;
        pPackage->iActionType = 6;
        pPackage->iPriority = iHighestCard;
        pPackageList->Add(pPackage);
        pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
        if (pNew) {
            std::memset(pNew, 0, sizeof(CaravanAIPackage));
        }
        pPackage = pNew;
        for (int32_t iTrack = 3; iTrack < 6; ++iTrack) {
            if (!CalculateWinningState(trackInfo[iTrack].iValue)) {
                continue;
            }
            pPackage->iTrack = iTrack;
            pPackage->iActionType = 5;
            if (iDiffLevel < 1) {
                pPackage->iPriority = trackInfo[iTrack].iValue - 26;
            }
            else if (iDiffLevel < 3) {
                pPackage->iPriority = 2 * trackInfo[iTrack].iValue - 52;
            }
            else {
                int32_t iNumKings = 0;
                for (TESCaravanCard* pCard : pNPCDeck) {
                    if (pCard->data.iFaceValue == 12) {
                        ++iNumKings;
                    }
                }
                pPackage->iPriority = 2 * trackInfo[iTrack].iValue - 52 - 5 * iNumKings;
            }
            pPackageList->Add(pPackage);
            pNew = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
            if (pNew) {
                std::memset(pNew, 0, sizeof(CaravanAIPackage));
            }
            pPackage = pNew;
            pPackage->iTrack = iTrack;
        }
    }
    pPackageList->Sort(&CaravanAIPackage::SortPackagesFunc);
    CaravanAIPackage kReturnPackage = *(*pPackageList)[0];
    while (!pPackageList->IsEmpty()) {
        pPackage = *static_cast<CaravanAIPackage**>(pPackageList->GetTail());
        pPackageList->RemoveLast();
        ourFree(pPackage);
    }
    if (pPackageList) {
        delete pPackageList;
    }
    *apResult = kReturnPackage;
    return apResult;
}

int32_t CaravanMenu::GetValueOfCardValueInTrack(int32_t aiTrack, int32_t aiFaceValue) {
    int32_t iRow = 0;
    int32_t iCount = 0;
    while (iRow < 7 && pCardTracks[aiTrack][iRow].iSize) {
        if (pCardTracks[aiTrack][iRow][0]->data.iFaceValue == aiFaceValue) {
            ++iCount;
        }
        ++iRow;
    }
    return iCount * aiFaceValue;
}

int32_t CaravanMenu::GetSumCardsB44WithSuit(int32_t aiTrack, uint32_t auiSuit) {
    int32_t iRow = 0;
    int32_t iSum = 0;
    while (iRow < 7 && pCardTracks[aiTrack][iRow].iSize) {
        if (pCardTracks[aiTrack][iRow][0]->data.iCardSuit == auiSuit) {
            iSum += pCardTracks[aiTrack][iRow][0]->data.iFaceValue;
        }
        ++iRow;
    }
    return iSum;
}

int32_t CaravanMenu::GetMenuID_() {
    return 1083;
}

// The NPC matches the player's ante up to its limit: its funds times fBetPercentage, raised by the player's barter
// skill in percent, capped at its funds.
void CaravanMenu::ItemSelectCallback(int32_t aiValue) {
    int32_t iRaise = aiValue - pMe->iPlayerAnte;
    pMe->iPlayerAnte += aiValue;
    pMe->pTiles[TI_ANTE_PLAYER_BET]->Poke(iUT_Value, 1.0f);
    pMe->pTiles[TI_ANTE_PLAYER_BET]->Set(iUT_Value, pMe->iPlayerAnte);
    pMe->SetFlag(CAF_DISABLE_ALLCONTROLS, false);
    double dBaseRaise = static_cast<double>(pMe->iNPCTotalFunds) * pMe->fBetPercentage;
    double dBaseRaise2 = static_cast<double>(pMe->iNPCTotalFunds) * pMe->fBetPercentage;
    // pPlayer is adjusted to its ActorValueOwner without a null check.
    int32_t iMaxRaise = static_cast<int32_t>(
        pPlayer->ActorValueOwner::GetClampedActorFloatValue(ActorValue::BARTER) / 100.0 * dBaseRaise2 + dBaseRaise
    );
    iMaxRaise = BSMIN(iMaxRaise, pMe->iNPCTotalFunds);
    if (pMe->iPlayerAnte > pMe->iNPCAnte) {
        pMe->iNPCAnte = BSMIN(iMaxRaise, pMe->iPlayerAnte);
        pMe->pTiles[TI_ANTE_NPC_BET]->Poke(iUT_Value, 1.0f);
        pMe->pTiles[TI_ANTE_NPC_BET]->Set(iUT_Value, pMe->iNPCAnte);
    }
    iRaise += BSMAX(BSMIN(pMe->iPlayerAnte, iMaxRaise) - pMe->iNPCAnte, 0);
    pMe->UpdateCaravanFlags();
    pMe->UpdateTileAlpha(TI_NO_TILE);
    pMe->UpdateBettingUI(iMaxRaise, iRaise);
}

void CaravanMenu::Close() {
    if (CaravanMenu::pMe) {
        CaravanMenu::pMe->eGameState = CaravanMenu::CA_CLOSING_MENU;
        if (Menu::GetMenuByClass(CaravanMenu::GetMenuID_()) &&
            CaravanMenu::pMe->eActiveMenu == CaravanMenu::CA_DECK_MENU) {
            BSSimpleList<TESCaravanCard*>* pAct = pPlayer->GetCaravanCards2();
            BSSimpleList<TESCaravanCard*>* pInact = pPlayer->GetCaravanCards1();
            pAct->RemoveAll();
            pInact->RemoveAll();
            for (TESCaravanCard* pCard : CaravanMenu::pMe->pDeckArray) {
                if (pCard->bIsActive) {
                    pAct->AddHead(pCard);
                }
                else {
                    pInact->AddHead(pCard);
                }
            }
        }
    }
}

void CaravanMenu::CancelConfirmFunc() {
    if (Interface::GetMessageMenuResult()) {
        CaravanMenu::pMe->SetFlag(CAF_DISABLE_ALLCONTROLS, false);
    }
    else {
        CaravanMenu::pMe->SetFlag(CAF_MARKFORCLOSE, true);
        appMain->GetControls()->ClearUserActions();
    }
}

void CaravanMenu::Draw3DElements() {
    NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
    SceneGraph* pWorldSceneGraph = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = pWorldSceneGraph->pCuller;
    pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* p3DCamera = CaravanMenu::sp3DCamera.m_pObject;
    NiCamera::HandleEyeFinityFOVScale(p3DCamera, 0.15f);
    NiPointer<NiRefObject> spAccumulator(static_cast<NiRefObject*>(InterfaceManager::Instance()->Get3dAccumulator()));
    pCuller->SetAccumulator(static_cast<NiAccumulator*>(spAccumulator.m_pObject));
    NiCamera* pCamera = p3DCamera ? p3DCamera : pWorldSceneGraph->GetCamera();
    BSShaderUtil::AccumulateScene(pCamera, BSShaderManager::GetShadowSceneNode(1), *pCuller);
    BSShaderUtil::RenderScene(pCamera, static_cast<BSShaderAccumulator*>(spAccumulator.m_pObject), false);
    pCuller->SetAccumulator(nullptr);
    spAccumulator = static_cast<NiRefObject*>(nullptr);
    pCuller->PopCullMode();
}

Tile* CaravanMenu::Create(
    Actor* apNPCChallenger,
    TESCaravanDeck* apNPCDeck,
    int32_t aiDiffLevel,
    float afBetPercentage,
    bool abDebugMode
) {
    Tile* pOldMenuTile = Menu::GetMenuByClass(1083);
    g_caravanMenu_InterfaceManager = InterfaceManager::Instance();
    int32_t iInactiveCount = pPlayer->GetCaravanCards1()->ItemsInList();
    int32_t iCardTotal = pPlayer->GetCaravanCards2()->ItemsInList() + iInactiveCount;
    if (iCardTotal >= 30) {
        if (pOldMenuTile) {
            delete pOldMenuTile;
        }
        Tile* pTile = g_caravanMenu_InterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\caravan_menu.xml");
        CaravanMenu::pMe = static_cast<CaravanMenu*>(pTile->GetMenu());
        if (CaravanMenu::pMe) {
            int32_t iClass = CaravanMenu::pMe->GetClass();
            if (iClass == CaravanMenu::GetMenuID_()) {
                CaravanMenu::pMe->pChallenger = apNPCChallenger;
                CaravanMenu::pMe->pDeckParam = apNPCDeck->pDeck;
                CaravanMenu::pMe->iDiffLevel = aiDiffLevel;
                CaravanMenu::pMe->fBetPercentage = afBetPercentage;
                CaravanMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
                if (CaravanMenu::pMe->IsWellFormedMenu()) {
                    if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                        float fDepth = Tile::GetMaximumDepth();
                        pTile->Set(4013, fDepth, true);
                    }
                    CaravanMenu::pMe->PrepareCaravanData();
                    CaravanMenu::pMe->pTiles[15]
                        ->Set(Tile::kTileValue_User0, static_cast<float>(CaravanMenu::pMe->iTotalCards), false);
                    CaravanMenu::pMe->pTiles[15]->Set(CaravanMenu::iUT_CurrentValue, 12.0f, false);
                    int32_t iSetInCode = Tile::TextToTrait("_SetInCode");
                    CaravanMenu::pMe->pTiles[15]->Poke(iSetInCode, 1.0f);
                    CaravanMenu::bDebugAI = false;
                    CaravanMenu::pMe->LoadNifs2();
                    CaravanMenu::pMe->SetupCurrency();
                    CaravanMenu::pMe->SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, false);
                    CaravanMenu::pMe->PrepForVisibility(false);
                    if (!InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_BET)) {
                        TESForm* pForm = TESForm::GetFormByEditorID("HelpCaravanBetting");
                        BGSMessage* pHelpForm = dynamic_cast<BGSMessage*>(pForm);
                        if (pHelpForm) {
                            CaravanMenu::pMe->bTutorial = TutorialMenu::Create(pHelpForm, false);
                            InterfaceManager::Instance()->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_BET);
                            CaravanMenu::pMe->iTutorial = 30;
                        }
                    }
                    return pTile;
                }

                Warning("MENUS: Caravan Menu Creation Failed... Are your menu and art resources up to date?");
                CaravanMenu::Close();
                return nullptr;
            }
        }

        if (CaravanMenu::pMe && CaravanMenu::pMe->pMenu && CaravanMenu::pMe) {
            delete CaravanMenu::pMe;
        }

        return nullptr;
    }

    char msg[264];
    BSsprintf(msg, 260, static_cast<const char*>(sCardCountText));
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

void CaravanAIPackage::Print_WhatIsHeThinking(BSSimpleArray<TESCaravanCard*, 1024>* pNPCHandArray) {
    if (pNPCHandArray) {
        TESCaravanCard* pCard = (*pNPCHandArray)[iDeckCard];
        InitializationFlow(
            "track:%i row:%i dcard:%i goal:%i calcresult:%i priority:%i card:%s",
            iTrack,
            iRow,
            iDeckCard,
            iValueGoal,
            iCalcResults,
            iPriority,
            pCard->GetFormEditorID()
        );
    }
    else {
        InitializationFlow(
            "track:%i row:%i dcard:%i goal:%i calcresult:%i priority:%i",
            iTrack,
            iRow,
            iDeckCard,
            iValueGoal,
            iCalcResults,
            iPriority
        );
    }
}

int32_t CaravanAIPackage::SortPackagesFunc(const void* t1, const void* t2) {
    int32_t iPriority1 = (*static_cast<CaravanAIPackage* const*>(t1))->iPriority;
    int32_t iPriority2 = (*static_cast<CaravanAIPackage* const*>(t2))->iPriority;
    if (iPriority1 < iPriority2) {
        return 1;
    }
    if (iPriority1 <= iPriority2) {
        return 0;
    }
    return -1;
}
