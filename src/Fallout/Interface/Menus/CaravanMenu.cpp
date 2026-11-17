#include "Fallout/Interface/Menus/CaravanMenu.h"

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
	SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
	pListOfBillAnimations = new BSSimpleList<NiControllerSequence*>();
	pListOfCoinAnimations = new BSSimpleList<NiControllerSequence*>();
}

CaravanMenu::~CaravanMenu() {
	char acPath[268];
	CaravanMenu::sp3DCamera = nullptr;
	pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Table.NIF");
	BSShaderManager::GetShadowSceneNode(1)->DetachChild(CaravanMenu::spModel.m_pObject);
	BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
	if (CaravanMenu::spModel.m_pObject && CaravanMenu::spModel.m_pObject->GetChildCount()) {
		CaravanMenu::spModel.m_pObject->DetachAllChildren();
	}
	pTableGeometry = nullptr;
	pCameraAnimManager = nullptr;
	for (int32_t i = 0; i < 4; ++i) {
		pCameraAnimations[i] = nullptr;
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
		for (int32_t i = 0; i < 96; ++i) {
			deckFaceShaders[i] = nullptr;
		}
	} else if (eActiveMenu == CaravanMenu::CA_GAME_MENU || eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
		char acFile[264];
		for (int32_t i = 0; i < 2; ++i) {
			pHandAnimManager[i] = nullptr;
		}
		for (int32_t i = 0; i < 6; ++i) {
			for (int32_t j = 0; j < 7; ++j) {
				pTrackAnimManager[i][j] = nullptr;
			}
		}
		// The exe's loop clears only the first animation pair of each track's first four rows.
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
		for (int32_t i = 0; i < 6; ++i) {
			pTrackGrids[i] = nullptr;
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
	if (((uint32_t)aiID) <= 50) {
		pTiles[aiID] = apTile;
	}
}

void CaravanMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
	bScrolling = (bool)0;
	if (((eActiveMenu == 1) && (aiID == 15)) && (!eGameState)) {
		iPrevValue = (int32_t)pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
		bScrolling = (bool)1;
		iPrevXLocation = g_caravanMenu_InterfaceManager->GetCursorX();
	}
}

void CaravanMenu::DoClick(int32_t a1, Tile* a2) {
	if (!eGameState && !IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
		switch (a1) {
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

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::DoIdle() {
	FNVD_INLINED_LOCAL(BSSoundHandle, v113, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v114, std::nullopt, std::nullopt);
	NiUpdateData v115;
	NiUpdateData v116;
	NiUpdateData v117;
	FNVD_INLINED_LOCAL(BSSoundHandle, v118, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v119, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v120, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v121, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v122, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v123, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v124, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v125, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v126, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v127, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
	NiUpdateData v130;
	NiUpdateData v131;
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	CaravanAIPackage aiPackage;
	int32_t ielaspedMillis = 0;
	float fxAxisRatio = 1.0f;
	Interface::enumMenus v1 = static_cast<Interface::enumMenus>(GetClass());
	bool bisMenuOnTop = Interface::IsTopMenuID(v1);
	if (bTutorial) {
		if (!bisMenuOnTop) {
			return;
		}
		int32_t X = iTutorial;
		InterfaceManager* v2 = InterfaceManager::Instance();
		if (!v2->xTutorialManager.HasBeenShown(static_cast<Interface::TutorialMessageID>(X))) {
			return;
		}
		bTutorial = 0;
	}
	if (uiPrevIdleTick) {
		if (!bisMenuOnTop) {
			SetFlag(CaravanMenu::CAF_ISONTOP, 0);
			return;
		}
		SetFlag(CaravanMenu::CAF_ISONTOP, 1);
		ielaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
		if (ePrevGameState == eGameState) {
			CaravanMenu::fTimeDelta = static_cast<double>(ielaspedMillis) / 1000.0 + CaravanMenu::fTimeDelta;
		} else {
			CaravanMenu::fTimeDelta = 0.0f;
		}
	} else {
		ielaspedMillis = 0;
		CaravanMenu::fTimeDelta = 0.0f;
		ePrevGameState = 0;
		fSelectionDelay = 0.0f;
	}
	int32_t ixAxis = 0;
	int32_t iyAxis = 0;
	int32_t cimaxAxisValue = 0x7FFF;
	float fxAxis = 0.0f;
	float fyAxis = 0.0f;
	InterfaceManager::Instance();
	if (InterfaceManager::Has360Controller()) {
		Controls* Controls_ = appMain->GetControls();
		ixAxis = Controls_->GetControllerValue(0, 7);
		Controls* v4 = appMain->GetControls();
		iyAxis = v4->GetControllerValue(0, 8);
		if (NiAbs(static_cast<float>(ixAxis)) <= 7849.0) {
			ixAxis = 0;
		}
		fxAxis = static_cast<double>(ixAxis) / 32767.0;
		if (NiAbs(static_cast<float>(iyAxis)) <= 7849.0) {
			iyAxis = 0;
		}
		fyAxis = static_cast<double>(-iyAxis) / 32767.0;
	}
	uiPrevIdleTick = appTimer.iLastTime;
	ePrevGameState = eGameState;
	if (!bisMenuOnTop || eActiveMenu == CaravanMenu::CA_DECK_MENU) {
		InterfaceManager* v5 = InterfaceManager::Instance();
		v5->pCursor->Set(4009, 255.0f, 1);
	}
	if (this->IsFlagSet(CaravanMenu::CAF_MARKFORCLOSE)) {
		if (bisMenuOnTop) {
			SetFlag(CaravanMenu::CAF_MARKFORCLOSE, 0);
			eGameState = CaravanMenu::CA_GAME_GAMEOVER;
		}
		return;
	}
	if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU && eGameState == CaravanMenu::CA_IDLE &&
		CaravanMenu::fTimeDelta > 1.0) {
		SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
	}
	switch (eGameState) {
	case CaravanMenu::CA_IDLE: {
		switch (eActiveMenu) {
		case CaravanMenu::CA_DECK_MENU: {
			if (ixAxis) {
				double Float = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
				int32_t iValue = static_cast<int32_t>(fxAxis / std::fabs(fxAxis) + Float);
				if (iValue >= 0 && iValue < iTotalCards) {
					CaravanMenu::CaravanState v89;
					iNumSteps = 1;
					pTiles[15]->Set(CaravanMenu::iUT_CurrentValue, static_cast<float>(iValue), 0);
					int32_t v68 = Tile::TextToTrait("_SetInCode");
					pTiles[15]->Poke(v68, 1.0f);
					if (!(fxAxis > 0.0)) {
						v89 = CaravanMenu::CA_DECK_BACK_ANIM;
					} else {
						v89 = CaravanMenu::CA_DECK_FORWARD_ANIM;
					}
					eGameState = v89;
				}
			}
			if (!InterfaceManager::Has360Controller()) {
				if (g_caravanMenu_InterfaceManager->fMouseHeldTime == 0.0 &&
					(bScrolling && eActiveMenu == CaravanMenu::CA_DECK_MENU)) {
					iNumSteps = 0;
					bScrolling = 0;
					int32_t v139 = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
					int32_t Number = v139 - iPrevValue;
					if (Number > -1) {
						if (Number >= 1) {
							if (Number >= 5) {
								eGameState = CaravanMenu::CA_DECK_FASTFORWARD_ANIM;
							} else {
								iNumSteps = Number;
								eGameState = CaravanMenu::CA_DECK_FORWARD_ANIM;
							}
						}
					} else {
						if (Number <= -5) {
							eGameState = CaravanMenu::CA_DECK_REWIND_ANIM;
						} else {
							iNumSteps = -Number;
							eGameState = CaravanMenu::CA_DECK_BACK_ANIM;
						}
					}
					iPrevValue = v139;
				} else {
					if (!bScrolling && g_caravanMenu_InterfaceManager->fMouseHeldTime > 0.0) {
						iPrevValue = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
						bScrolling = 1;
						iPrevXLocation = g_caravanMenu_InterfaceManager->GetCursorX();
					}
				}
				if (eGameState == CaravanMenu::CA_IDLE) {
					switch (unkE80) {
					case 0x80000003u: {
						unkE80 = 0;
						iyAxis = -1;
						break;
					}
					case 0x80000004u: {
						unkE80 = 0;
						iyAxis = 1;
						break;
					}
					case 0x80000002u: {
						unkE80 = 0;
						ixAxis = 1;
						break;
					}
					case 0x80000001u: {
						unkE80 = 0;
						ixAxis = -1;
						break;
					}
					}
					if ((ixAxis || iyAxis) && static_cast<double>(uiPrevIdleTick) - fSelectionDelay > 0.25) {
						fSelectionDelay = static_cast<float>(uiPrevIdleTick);
						if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
							if (ixAxis) {
								int32_t iValue_2 =
									static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue) +
										static_cast<double>(ixAxis));
								if (iValue_2 >= 0 && iValue_2 < iTotalCards) {
									pTiles[15]->Set(CaravanMenu::iUT_CurrentValue, static_cast<float>(iValue_2), 0);
									int32_t v69 = Tile::TextToTrait("_SetInCode");
									pTiles[15]->Poke(v69, 1.0f);
									eGameState = static_cast<CaravanMenu::CaravanState>((ixAxis <= 0) + 5);
									iNumSteps = 1;
								}
							}
						}
					}
				}
			}
			break;
		}
		case CaravanMenu::CA_GAME_MENU: {
			if (eGameState == CaravanMenu::CA_IDLE) {
				switch (unkE80) {
				case 0x80000003u: {
					unkE80 = 0;
					iyAxis = -1;
					fyAxis = -1.0f;
					break;
				}
				case 0x80000004u: {
					unkE80 = 0;
					iyAxis = 1;
					fyAxis = 1.0f;
					break;
				}
				case 0x80000002u: {
					unkE80 = 0;
					ixAxis = 1;
					fxAxis = 1.0f;
					break;
				}
				case 0x80000001u: {
					unkE80 = 0;
					ixAxis = -1;
					fxAxis = -1.0f;
					break;
				}
				}
			}
			if ((ixAxis || iyAxis) && CaravanMenu::fTimeDelta - fSelectionDelay > 0.25) {
				fSelectionDelay = CaravanMenu::fTimeDelta;
				if (this->IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
					if (!this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
						double v86;
						double v87;
						if (!(std::fabs(fxAxis) < 0.01)) {
							v87 = fxAxis / std::fabs(fxAxis);
						} else {
							v87 = 0.0;
						}
						int32_t xParam = static_cast<int32_t>(v87);
						if (!(std::fabs(fyAxis) < 0.01)) {
							v86 = fyAxis / std::fabs(fyAxis);
						} else {
							v86 = 0.0;
						}
						int32_t yParam = static_cast<int32_t>(v86);
						if (this->IsFlagSet(CaravanMenu::CAF_DESTROY_TRACK)) {
							DeselectWholeTrack(iSelectedTrack);
							iSelectedTrack += xParam;
							int32_t v71 = BSMAX(0, iSelectedTrack);
							iSelectedTrack = BSMIN(v71, 2);
							BSAudio* v72 = &BSAudio::QInstance();
							BSSoundHandle* SoundHandleByName =
								v72->GetSoundHandleByName(&v113, "GAMECaravanToggleTrack", 289u);
							SoundHandleByName->Play(0);
							SelectWholeTrack(iSelectedTrack);
						} else {
							HandleTrackSelection(xParam, yParam);
						}
					}
				} else {
					iSelectedDeckCard =
						static_cast<int32_t>(fxAxis / std::fabs(fxAxis) + static_cast<double>(iSelectedDeckCard));
					int32_t hsize = pPlayerHand.iSize;
					if (iSelectedDeckCard <= hsize - 1) {
						if (iSelectedDeckCard < 0) {
							iSelectedDeckCard = hsize - 1;
						}
					} else {
						iSelectedDeckCard = 0;
					}
					BSAudio* v70 = &BSAudio::QInstance();
					BSSoundHandle* v88 = v70->GetSoundHandleByName(&v114, "GAMECaravanSwitchCard", 289u);
					v88->Play(0);
					SelectDeckCard(0);
					if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
						pPlayerHand.operator[](iSelectedDeckCard)->data.iFaceValue > 10) {
						SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
					} else {
						SetFlag(CaravanMenu::CAF_DISABLE_A, 0);
					}
					UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				}
			}
			break;
		}
		case CaravanMenu::CA_RESULTS_MENU: {
			Controls* v74;
			Controls* v73 = appMain->GetControls();
			if (v73->GetMouseButton(0, 1) || (v74 = appMain->GetControls(), v74->GetMouseButton(1, 1))) {
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
				pCameraAnimManager->ActivateSequence(pCameraAnimations[0], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v7 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v7->Update(UpdateData1);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pCameraAnimations[0]->m_fEndKeyTime) {
				eGameState = CaravanMenu::CA_IDLE;
				pCameraAnimManager->DeactivateSequence(pCameraAnimations[0], 0.0f);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				InterfaceManager* v8 = InterfaceManager::Instance();
				if (!v8->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_DECK)) {
					TESForm* FormByEditorID = TESForm::GetFormByEditorID("HelpCaravanDeckBuilding");
					BGSMessage* helpForm = dynamic_cast<BGSMessage*>(FormByEditorID);
					if (helpForm) {
						bTutorial = TutorialMenu::Create(helpForm, 0);
						InterfaceManager* v10 = InterfaceManager::Instance();
						v10->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_DECK);
						iTutorial = 31;
					}
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_CAMERA_TO_GAME_ANIM: {
		if (pCameraAnimations[1]) {
			if (pHandAnimations[0][0]) {
				if (pHandAnimations[1][0]) {
					if (CaravanMenu::fTimeDelta == 0.0) {
						pCameraAnimManager->ActivateSequence(pCameraAnimations[1], 0, 0, 1.0f, 0.0f, 0);
						ZeroFlags();
						SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
						pHandAnimManager[0]->ActivateSequence(pHandAnimations[0][0], 0, 0, 1.0f, 0.0f, 0);
						pHandAnimManager[1]->ActivateSequence(pHandAnimations[1][0], 0, 0, 1.0f, 0.0f, 0);
						fSelectionDelay = 0.0f;
						BSAudio* v11 = &BSAudio::QInstance();
						BSSoundHandle* v111 = v11->GetSoundHandleByName(&a1, "GAMECaravanStartMatch", 289u);
						v111->Play(0);
						UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
					}
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_2, nullptr, CaravanMenu::fTimeDelta, 1, 0);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2, nullptr, CaravanMenu::fTimeDelta, 1, 0);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3, nullptr, CaravanMenu::fTimeDelta, 1, 0);
					pHandGeometry[0]->Update(UpdateData2);
					pHandGeometry[1]->Update(UpdateData3);
					NiAVObject* v12 = CaravanMenu::spModel.m_pObject->GetAt(0);
					v12->Update(UpdateData1_2);
					if (static_cast<double>(CaravanMenu::fTimeDelta) >= pCameraAnimations[1]->m_fEndKeyTime &&
						static_cast<double>(CaravanMenu::fTimeDelta) >= pHandAnimations[0][0]->m_fEndKeyTime &&
						static_cast<double>(CaravanMenu::fTimeDelta) >= pHandAnimations[1][0]->m_fEndKeyTime) {
						pCameraAnimManager->DeactivateSequence(pCameraAnimations[1], 0.0f);
						pHandAnimManager[0]->DeactivateSequence(pHandAnimations[0][0], 0.0f);
						pHandAnimManager[1]->DeactivateSequence(pHandAnimations[1][0], 0.0f);
						SetFlag(CaravanMenu::CAF_TRACK_SETUP, 1);
						float x = 0.0f;
						float y = 0.0f;
						int32_t iSafeZoneWidth = static_cast<int32_t>(pTiles[2]->GetFloat(4017));
						NiAVObject* obj = BSUtilities::GetObjectByName(pTrackGrids[0], "Select1_01:0");
						CaravanMenu::sp3DCamera.m_pObject->WorldPtToScreenPt(obj->GetWorldTranslate(), x, y, 1e-05f);
						x = static_cast<double>(iSafeZoneWidth) * x - 120.0;
						pTiles[23]->Set(4001, x, 1);
						pTiles[28]->Set(4001, x, 1);
						x = x + 265.0;
						pTiles[24]->Set(4001, x, 1);
						pTiles[27]->Set(4001, x, 1);
						x = x + 265.0;
						pTiles[25]->Set(4001, x, 1);
						pTiles[26]->Set(4001, x, 1);
						Tile* TileByName = Tile::GetTileByName(pTiles[2], "CGM_NPCTrackRect");
						TileByName->Set(4003, 1);
						Tile* v14 = Tile::GetTileByName(pTiles[2], "CGM_PlayerTrackRect");
						v14->Set(4003, 1);
						UpdateCaravanFlags();
						UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
						eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
						InterfaceManager* v15 = InterfaceManager::Instance();
						if (!v15->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_TRACK)) {
							TESForm* v16 = TESForm::GetFormByEditorID("HelpCaravanStartingCaravans");
							BGSMessage* helpForm_2 = dynamic_cast<BGSMessage*>(v16);
							if (helpForm_2) {
								bTutorial = TutorialMenu::Create(helpForm_2, 0);
								InterfaceManager* v17 = InterfaceManager::Instance();
								v17->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_TRACK);
								iTutorial = 32;
							}
						}
					}
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM: {
		if (pCameraAnimations[2]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pCameraAnimManager->ActivateSequence(pCameraAnimations[2], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_3, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v18 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v18->Update(UpdateData1_3);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pCameraAnimations[2]->m_fEndKeyTime) {
				eGameState = CaravanMenu::CA_IDLE;
				eActiveMenu = CaravanMenu::CA_RESULTS_MENU;
				pCameraAnimManager->DeactivateSequence(pCameraAnimations[2], 0.0f);
				SetFlag(CaravanMenu::CAF_AI, 0);
				const char* v19 = sCaravanPressAnyKeyText.String();
				pTiles[35]->Set(4036, v19, 1);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			}
		}
		return;
	}
	case CaravanMenu::CA_ANTE_MONEYIN_ANIM: {
		if (CaravanMenu::fTimeDelta == 0.0) {
			SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
			BSAudio* v20 = &BSAudio::QInstance();
			BSSoundHandle* v110 = v20->GetSoundHandleByName(&arHandle, "GAMECaravanFundsDrop", 289u);
			v110->Play(0);
		}
		for (int32_t i = iBillIndexToAnim;; ++i) {
			uint32_t FormFlags = pListOfBills.iSize;
			if (i >= FormFlags) {
				break;
			}
			NiUpdateData* updated = FNVD_RECONSTRUCT(NiUpdateData)(&v131, CaravanMenu::fTimeDelta, 1, 0);
			pListOfBills.operator[](i)->Update(*updated);
		}
		for (int32_t i_2 = iCoinIndexToAnim;; ++i_2) {
			uint32_t v23 = pListOfCoins.iSize;
			if (i_2 >= v23) {
				break;
			}
			NiUpdateData* v24 = FNVD_RECONSTRUCT(NiUpdateData)(&v130, CaravanMenu::fTimeDelta, 1, 0);
			pListOfCoins.operator[](i_2)->Update(*v24);
		}
		if (CaravanMenu::fTimeDelta > 1.0) {
			SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
			iBillIndexToAnim = pListOfBills.iSize;
			iCoinIndexToAnim = pListOfCoins.iSize;
			eGameState = CaravanMenu::CA_IDLE;
		}
		return;
	}
	case CaravanMenu::CA_DECK_FORWARD_ANIM: {
		if (pActiveDeckAnimations[0] || pInactiveDeckAnimations[0]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[0], 0, 0, 1.0f, 0.0f, 0);
				pInactiveDeckAnimManager->ActivateSequence(pInactiveDeckAnimations[0], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				--iNumSteps;
				BSAudio* v25 = &BSAudio::QInstance();
				BSSoundHandle* v109 = v25->GetSoundHandleByName(&th_s, "GAMECaravanNavigate", 289u);
				v109->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_4, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_2, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_2, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v26 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v26->Update(UpdateData1_4);
			pActiveDeckGeometry->Update(UpdateData2_2);
			pInactiveDeckGeometry->Update(UpdateData3_2);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[0]->m_fEndKeyTime) {
				double v27 = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue) - static_cast<double>(iNumSteps);
				SwapDeckBuildingTextures(static_cast<int32_t>(v27), 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_5, nullptr, 0.0f, 1, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_3, nullptr, 0.0f, 1, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_3, nullptr, 0.0f, 1, 0);
				NiAVObject* v28 = CaravanMenu::spModel.m_pObject->GetAt(0);
				v28->Update(UpdateData1_5);
				pActiveDeckGeometry->Update(UpdateData2_3);
				pInactiveDeckGeometry->Update(UpdateData3_3);
				if (iNumSteps > 0) {
					CaravanMenu::fTimeDelta = 0.0f;
					--iNumSteps;
				} else {
					eGameState = CaravanMenu::CA_IDLE;
					pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[0], 0.0f);
					pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[0], 0.0f);
					SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
					UpdateCaravanFlags();
					UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				}
				unkE80 = 0;
			}
		}
		return;
	}
	case CaravanMenu::CA_DECK_BACK_ANIM: {
		if (pActiveDeckAnimations[1] || pInactiveDeckAnimations[1]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[1], 0, 0, 1.0f, 0.0f, 0);
				pInactiveDeckAnimManager->ActivateSequence(pInactiveDeckAnimations[1], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				--iNumSteps;
				double v29 = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue) + static_cast<double>(iNumSteps);
				SwapDeckBuildingTextures(static_cast<int32_t>(v29), 0);
				BSAudio* v30 = &BSAudio::QInstance();
				BSSoundHandle* v108 = v30->GetSoundHandleByName(&apResult, "GAMECaravanNavigate", 289u);
				v108->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_6, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_4, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_4, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v31 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v31->Update(UpdateData1_6);
			pActiveDeckGeometry->Update(UpdateData2_4);
			pInactiveDeckGeometry->Update(UpdateData3_4);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[1]->m_fEndKeyTime) {
				if (iNumSteps > 0) {
					CaravanMenu::fTimeDelta = 0.0f;
					if (iNumSteps > 0) {
						CaravanMenu::fTimeDelta = 0.0f;
						--iNumSteps;
					} else {
						eGameState = CaravanMenu::CA_IDLE;
						pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[0], 0.0f);
						pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[0], 0.0f);
						SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
						UpdateCaravanFlags();
						UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
					}
				} else {
					eGameState = CaravanMenu::CA_IDLE;
					pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[1], 0.0f);
					pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[1], 0.0f);
					SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
					UpdateCaravanFlags();
					UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				}
				unkE80 = 0;
			}
		}
		return;
	}
	case CaravanMenu::CA_DECK_FLIP_ANIM: {
		if (pActiveDeckAnimations[2] || pInactiveDeckAnimations[2]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[2], 0, 0, 1.0f, 0.0f, 0);
				pInactiveDeckAnimManager->ActivateSequence(pInactiveDeckAnimations[2], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				BSAudio* v38 = &BSAudio::QInstance();
				BSSoundHandle* v105 = v38->GetSoundHandleByName(&v125, "GAMECaravanAddRemove", 289u);
				v105->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_9, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_7, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_7, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v39 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v39->Update(UpdateData1_9);
			pActiveDeckGeometry->Update(UpdateData2_7);
			pInactiveDeckGeometry->Update(UpdateData3_7);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[2]->m_fEndKeyTime) {
				eGameState = CaravanMenu::CA_IDLE;
				FlipSelectedCardTextures(11);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_10, nullptr, 0.0f, 1, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_8, nullptr, 0.0f, 1, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_8, nullptr, 0.0f, 1, 0);
				NiAVObject* v40 = CaravanMenu::spModel.m_pObject->GetAt(0);
				v40->Update(UpdateData1_10);
				pActiveDeckGeometry->Update(UpdateData2_8);
				pInactiveDeckGeometry->Update(UpdateData3_8);
				pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[2], 0.0f);
				pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[2], 0.0f);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			}
		}
		return;
	}
	case CaravanMenu::CA_DECK_FASTFORWARD_ANIM: {
		if (pActiveDeckAnimations[3] || pInactiveDeckAnimations[3]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				double v32 = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
				SwapDeckBuildingTextures(static_cast<int32_t>(v32), 0);
				pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[3], 0, 0, 1.0f, 0.0f, 0);
				pInactiveDeckAnimManager->ActivateSequence(pInactiveDeckAnimations[3], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				BSAudio* v33 = &BSAudio::QInstance();
				BSSoundHandle* v107 = v33->GetSoundHandleByName(&v127, "GAMECaravanChangeDeck", 289u);
				v107->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_7, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_5, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_5, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v34 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v34->Update(UpdateData1_7);
			pActiveDeckGeometry->Update(UpdateData2_5);
			pInactiveDeckGeometry->Update(UpdateData3_5);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[3]->m_fEndKeyTime) {
				eGameState = CaravanMenu::CA_IDLE;
				pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[3], 0.0f);
				pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[3], 0.0f);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				unkE80 = 0;
			}
		}
		return;
	}
	case CaravanMenu::CA_DECK_REWIND_ANIM: {
		if (pActiveDeckAnimations[4] || pInactiveDeckAnimations[4]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[4], 0, 0, 1.0f, 0.0f, 0);
				pInactiveDeckAnimManager->ActivateSequence(pInactiveDeckAnimations[4], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				BSAudio* v35 = &BSAudio::QInstance();
				BSSoundHandle* v106 = v35->GetSoundHandleByName(&v126, "GAMECaravanChangeDeck", 289u);
				v106->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_8, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_6, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_6, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v36 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v36->Update(UpdateData1_8);
			pActiveDeckGeometry->Update(UpdateData2_6);
			pInactiveDeckGeometry->Update(UpdateData3_6);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[4]->m_fEndKeyTime) {
				eGameState = CaravanMenu::CA_IDLE;
				double v37 = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
				SwapDeckBuildingTextures(static_cast<int32_t>(v37), 0);
				pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[4], 0.0f);
				pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[4], 0.0f);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				unkE80 = 0;
			}
		}
		return;
	}
	case CaravanMenu::CA_DECK_WIPE_ANIM: {
		if (pActiveDeckAnimations[5] || pInactiveDeckAnimations[5]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pActiveDeckAnimManager->ActivateSequence(pActiveDeckAnimations[5], 0, 0, 1.0f, 0.0f, 0);
				pInactiveDeckAnimManager->ActivateSequence(pInactiveDeckAnimations[5], 0, 0, 1.0f, 0.0f, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				BSAudio* v41 = &BSAudio::QInstance();
				BSSoundHandle* v104 = v41->GetSoundHandleByName(&v124, "GAMECaravanChangeDeck", 289u);
				v104->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_11, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_9, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_9, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			NiAVObject* v42 = CaravanMenu::spModel.m_pObject->GetAt(0);
			v42->Update(UpdateData1_11);
			pActiveDeckGeometry->Update(UpdateData2_9);
			pInactiveDeckGeometry->Update(UpdateData3_9);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >= pActiveDeckAnimations[5]->m_fEndKeyTime) {
				eGameState = CaravanMenu::CA_IDLE;
				FlipSelectedCardTextures(11);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_12, nullptr, CaravanMenu::fTimeDelta, 1, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_10, nullptr, CaravanMenu::fTimeDelta, 1, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData3_10, nullptr, CaravanMenu::fTimeDelta, 1, 0);
				NiAVObject* v43 = CaravanMenu::spModel.m_pObject->GetAt(0);
				v43->Update(UpdateData1_12);
				pActiveDeckGeometry->Update(UpdateData2_10);
				pInactiveDeckGeometry->Update(UpdateData3_10);
				pActiveDeckAnimManager->DeactivateSequence(pActiveDeckAnimations[5], 0.0f);
				pInactiveDeckAnimManager->DeactivateSequence(pInactiveDeckAnimations[5], 0.0f);
				bContinueProcessing = 1;
				SetFlag(CaravanMenu::CAF_AI, 1);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				PrepareGameMenu();
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_INTRO_DEAL_ANIM: {
		uint32_t isAI = this->IsFlagSet(CaravanMenu::CAF_AI);
		if (pStartAnimations[isAI][iSelectedDeckCard]) {
			if (pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]) {
				if (CaravanMenu::fTimeDelta == 0.0) {
					UpdateSelectedTrackCardTexture();
					pHandAnimManager[isAI]->ActivateSequence(pStartAnimations[isAI][iSelectedDeckCard], 0, 0, 1.0f,
						0.0f, 0);
					pTrackAnimManager[iSelectedTrack][iSelectedRow]
						->ActivateSequence(pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0], 0, 0,
							1.0f, 0.0f, 0);
					if (!isAI) {
						SelectDeckCard(1);
					}
					SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
					fSelectionDelay = 0.0f;
					BSAudio* v44 = &BSAudio::QInstance();
					BSSoundHandle* v103 = v44->GetSoundHandleByName(&v123, "GAMECaravanConfirmAddtoDeck", 289u);
					v103->Play(0);
				}
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_13, nullptr, CaravanMenu::fTimeDelta, 1, 0);
				pHandGeometry[isAI]->Update(UpdateData1_13);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_11, nullptr, CaravanMenu::fTimeDelta, 1, 0);
				pTrackGeometry[iSelectedTrack][iSelectedRow]->Update(UpdateData2_11);
				if (static_cast<double>(CaravanMenu::fTimeDelta) >=
						pStartAnimations[isAI][iSelectedDeckCard]->m_fEndKeyTime &&
					static_cast<double>(CaravanMenu::fTimeDelta) >=
						pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]->m_fEndKeyTime) {
					uint32_t auiValue;
					pHandAnimManager[isAI]->DeactivateSequence(pStartAnimations[isAI][iSelectedDeckCard], 0.0f);
					pTrackAnimManager[iSelectedTrack][iSelectedRow]
						->DeactivateSequence(pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0], 0.0f);
					if ((pCardTracks[2]->iSize && !this->IsFlagSet(CaravanMenu::CAF_AI)) ||
						(pCardTracks[3]->iSize && this->IsFlagSet(CaravanMenu::CAF_AI))) {
						bool Xa = this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP);
						bool IsFlagSet = this->IsFlagSet(CaravanMenu::CAF_AI);
						UpdateGameHandTextures(!IsFlagSet, Xa);
						pHandAnimManager[isAI]->ActivateSequence(pHandAnimations[isAI][1], 0, 0, 1.0f, 0.0f, 0);
						FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_14, nullptr, 0.0f, 1, 0);
						pHandGeometry[isAI]->Update(UpdateData1_14);
						pHandAnimManager[isAI]->DeactivateSequence(pHandAnimations[isAI][1], 0.0f);
						if (!this->IsFlagSet(CaravanMenu::CAF_AI)) {
							if (pCardTracks[2]->iSize) {
								if (pCardTracks[3]->iSize) {
									SetFlag(CaravanMenu::CAF_TRACK_SETUP, 0);
									InterfaceManager* v46 = InterfaceManager::Instance();
									if (!v46->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_GAME)) {
										TESForm* v47 = TESForm::GetFormByEditorID("HelpCaravanContractWar");
										BGSMessage* helpForm_3 = dynamic_cast<BGSMessage*>(v47);
										if (helpForm_3) {
											bTutorial = TutorialMenu::Create(helpForm_3, 0);
											InterfaceManager* v48 = InterfaceManager::Instance();
											v48->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_GAME);
											iTutorial = 33;
										}
									}
								}
							}
						}
					} else {
						bool v49 = this->IsFlagSet(CaravanMenu::CAF_AI);
						UpdateGameHandTextures(!v49, 1);
						pHandAnimManager[isAI]->ActivateSequence(pStartAnimations[isAI][0], 0, 0, 1.0f, 0.0f, 0);
						FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_15, nullptr, 0.0f, 1, 0);
						pHandGeometry[isAI]->Update(UpdateData1_15);
						pHandAnimManager[isAI]->DeactivateSequence(pStartAnimations[isAI][0], 0.0f);
					}
					pHandAnimManager[isAI]->DeactivateAll(0.0f);
					if (isAI == 1) {
						iSelectedDeckCard = pPlayerHand.iSize - 1;
						SelectDeckCard(0);
						if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
							pPlayerHand.operator[](iSelectedDeckCard)->data.iFaceValue > 10) {
							SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
						}
					}
					pTiles[isAI + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
					if (isAI) {
						auiValue = pNPCDeck.iSize;
					} else {
						auiValue = pPlayerDeck.iSize;
					}
					pTiles[isAI + 29]->Set(CaravanMenu::iUT_Value, auiValue);
					UpdateCaravanFlags();
					UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
					eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_INTRO_DISCARD_ANIM: {
		uint32_t isAI_2 = this->IsFlagSet(CaravanMenu::CAF_AI);
		if (pStartAnimations[isAI_2][iSelectedDeckCard]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				pHandAnimManager[isAI_2]->ActivateSequence(pStartAnimations[isAI_2][iSelectedDeckCard], 0, 0, 1.0f,
					0.0f, 0);
				if (!isAI_2) {
					SelectDeckCard(1);
				}
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				fSelectionDelay = 0.0f;
				BSAudio* v50 = &BSAudio::QInstance();
				BSSoundHandle* v101 = v50->GetSoundHandleByName(&v122, "GAMECaravanRemoveCard", 289u);
				v101->Play(0);
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_16, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			pHandGeometry[isAI_2]->Update(UpdateData1_16);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >=
				pStartAnimations[isAI_2][iSelectedDeckCard]->m_fEndKeyTime) {
				uint32_t v100;
				pHandAnimManager[isAI_2]->DeactivateSequence(pStartAnimations[isAI_2][iSelectedDeckCard], 0.0f);
				if ((pCardTracks[2]->iSize && !this->IsFlagSet(CaravanMenu::CAF_AI)) ||
					(pCardTracks[3]->iSize && this->IsFlagSet(CaravanMenu::CAF_AI))) {
					bool Xb = this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP);
					bool v51 = this->IsFlagSet(CaravanMenu::CAF_AI);
					UpdateGameHandTextures(!v51, Xb);
					pHandAnimManager[isAI_2]->ActivateSequence(pHandAnimations[isAI_2][1], 0, 0, 1.0f, 0.0f, 0);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_17, nullptr, 0.0f, 1, 0);
					pHandGeometry[isAI_2]->Update(UpdateData1_17);
					pHandAnimManager[isAI_2]->DeactivateSequence(pHandAnimations[isAI_2][1], 0.0f);
				} else {
					bool v52 = this->IsFlagSet(CaravanMenu::CAF_AI);
					UpdateGameHandTextures(!v52, 1);
					pHandAnimManager[isAI_2]->ActivateSequence(pStartAnimations[isAI_2][0], 0, 0, 1.0f, 0.0f, 0);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_18, nullptr, 0.0f, 1, 0);
					pHandGeometry[isAI_2]->Update(UpdateData1_18);
					pHandAnimManager[isAI_2]->DeactivateSequence(pStartAnimations[isAI_2][0], 0.0f);
				}
				pHandAnimManager[isAI_2]->DeactivateAll(0.0f);
				if (!isAI_2) {
					iSelectedDeckCard = pPlayerHand.iSize - 1;
					SelectDeckCard(0);
					if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
						pPlayerHand.operator[](iSelectedDeckCard)->data.iFaceValue > 10) {
						SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
					}
				}
				pTiles[isAI_2 + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
				if (isAI_2) {
					v100 = pNPCDeck.iSize;
				} else {
					v100 = pPlayerDeck.iSize;
				}
				pTiles[isAI_2 + 29]->Set(CaravanMenu::iUT_Value, v100);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				if (this->IsFlagSet(CaravanMenu::CAF_AI)) {
					eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
				} else {
					eGameState = CaravanMenu::CA_IDLE;
					SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_PLACECARD_ANIM: {
		uint32_t v97;
		uint32_t isAI_3 = this->IsFlagSet(CaravanMenu::CAF_AI);
		if (!pHandAnimations[isAI_3][iSelectedDeckCard + 1]) {
			return;
		}
		if (!pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]) {
			return;
		}
		if (CaravanMenu::fTimeDelta == 0.0) {
			int32_t iNextNPCCard;
			UpdateSelectedTrackCardTexture();
			if (isAI_3) {
				iNextNPCCard = this->iNextNPCCard;
			} else {
				iNextNPCCard = this->iNextPlayerCard;
			}
			if (iNextNPCCard == -1) {
				pHandAnimManager[isAI_3]->ActivateSequence(pStartAnimations[isAI_3][iSelectedDeckCard], 0, 0, 1.0f,
					0.0f, 0);
			} else {
				pHandAnimManager[isAI_3]->ActivateSequence(pHandAnimations[isAI_3][iSelectedDeckCard + 1], 0, 0, 1.0f,
					0.0f, 0);
			}
			pTrackAnimManager[iSelectedTrack][iSelectedRow]
				->ActivateSequence(pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0], 0, 0, 1.0f, 0.0f,
					0);
			if (!isAI_3) {
				SelectDeckCard(1);
			}
			SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
			fSelectionDelay = 0.0f;
			BSAudio* v53 = &BSAudio::QInstance();
			BSSoundHandle* v98 = v53->GetSoundHandleByName(&v121, "GAMECaravanConfirmAddToTrack", 289u);
			v98->Play(0);
		}
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_19, nullptr, CaravanMenu::fTimeDelta, 1, 0);
		pHandGeometry[isAI_3]->Update(UpdateData1_19);
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2_12, nullptr, CaravanMenu::fTimeDelta, 1, 0);
		pTrackGeometry[iSelectedTrack][iSelectedRow]->Update(UpdateData2_12);
		if (!(static_cast<double>(CaravanMenu::fTimeDelta) >=
				pHandAnimations[isAI_3][iSelectedDeckCard]->m_fEndKeyTime) ||
			!(static_cast<double>(CaravanMenu::fTimeDelta) >=
				pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]->m_fEndKeyTime)) {
			return;
		}
		pHandAnimManager[isAI_3]->DeactivateSequence(pHandAnimations[isAI_3][iSelectedDeckCard + 1], 0.0f);
		pTrackAnimManager[iSelectedTrack][iSelectedRow]
			->DeactivateSequence(pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0], 0.0f);
		bool Xc = this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP);
		bool v54 = this->IsFlagSet(CaravanMenu::CAF_AI);
		UpdateGameHandTextures(!v54, Xc);
		pHandAnimManager[isAI_3]->ActivateSequence(pHandAnimations[isAI_3][1], 0, 0, 1.0f, 0.0f, 0);
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_20, nullptr, 0.0f, 1, 0);
		pHandGeometry[isAI_3]->Update(UpdateData1_20);
		pHandAnimManager[isAI_3]->DeactivateSequence(pHandAnimations[isAI_3][1], 0.0f);
		if (isAI_3 == 1 && (pSelectedCard->data.iFaceValue != 12 && pSelectedCard->data.iFaceValue != 15)) {
			uint32_t v96;
			iSelectedDeckCard = pPlayerHand.iSize - 1;
			SelectDeckCard(0);
			if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
				pPlayerHand.operator[](iSelectedDeckCard)->data.iFaceValue > 10) {
				SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
			}
		LABEL_225:;
			pTiles[isAI_3 + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
			if (isAI_3) {
				v96 = pNPCDeck.iSize;
			} else {
				v96 = pPlayerDeck.iSize;
			}
			pTiles[isAI_3 + 29]->Set(CaravanMenu::iUT_Value, v96);
			UpdateCaravanFlags();
			UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
			return;
		}
		if (pSelectedCard->data.iFaceValue != 12 && pSelectedCard->data.iFaceValue != 15) {
			goto LABEL_225;
		}
		pTiles[isAI_3 + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
		if (isAI_3) {
			v97 = pNPCDeck.iSize;
		} else {
			v97 = pPlayerDeck.iSize;
		}
		pTiles[isAI_3 + 29]->Set(CaravanMenu::iUT_Value, v97);
		eGameState = static_cast<CaravanMenu::CaravanState>(2 * (pSelectedCard->data.iFaceValue != 12) + 17);
		if (eGameState == CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM) {
			int32_t faceValue = pCardTracks[iSelectedTrack][iSelectedRow].operator[](0)->data.iFaceValue;
			int32_t suitValue = pCardTracks[iSelectedTrack][iSelectedRow].operator[](0)->data.iCardSuit;
			for (int32_t iTrack = 0; iTrack < 6; ++iTrack) {
				for (int32_t iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
					if (faceValue == 1) {
						int32_t CostOverride = pCardTracks[iTrack][iRow].operator[](0)->data.iCardSuit;
						if (CostOverride == suitValue && (iTrack != iSelectedTrack || iRow != iSelectedRow)) {
							jokerLocations[iTrack].Add(iRow);
						}
					} else {
						int32_t PackageEvaluateHour = pCardTracks[iTrack][iRow].operator[](0)->data.iFaceValue;
						if (PackageEvaluateHour == faceValue && (iTrack != iSelectedTrack || iRow != iSelectedRow)) {
							jokerLocations[iTrack].Add(iRow);
						}
					}
				}
			}
			iSelectedTrack = 0;
		}
		break;
	}
	case CaravanMenu::CA_GAME_DISCARDCARD_ANIM: {
		uint32_t isAI_4 = this->IsFlagSet(CaravanMenu::CAF_AI);
		if (pHandAnimations[isAI_4][iSelectedDeckCard + 1]) {
			if (CaravanMenu::fTimeDelta == 0.0) {
				int32_t iNextPlayerCard;
				if (isAI_4) {
					iNextPlayerCard = this->iNextNPCCard;
				} else {
					iNextPlayerCard = this->iNextPlayerCard;
				}
				if (iNextPlayerCard == -1) {
					pHandAnimManager[isAI_4]->ActivateSequence(pStartAnimations[isAI_4][iSelectedDeckCard], 0, 0, 1.0f,
						0.0f, 0);
					BSAudio* v57 = &BSAudio::QInstance();
					BSSoundHandle* v94 = v57->GetSoundHandleByName(&v120, "GAMECaravanRemoveCard", 289u);
					v94->Play(0);
				} else {
					pHandAnimManager[isAI_4]->ActivateSequence(pHandAnimations[isAI_4][iSelectedDeckCard + 1], 0, 0,
						1.0f, 0.0f, 0);
					BSAudio* v58 = &BSAudio::QInstance();
					BSSoundHandle* v93 = v58->GetSoundHandleByName(&v119, "GAMECaravanAddRemoveCard", 289u);
					v93->Play(0);
				}
				if (!isAI_4) {
					SelectDeckCard(1);
				}
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				fSelectionDelay = 0.0f;
			}
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_21, nullptr, CaravanMenu::fTimeDelta, 1, 0);
			pHandGeometry[isAI_4]->Update(UpdateData1_21);
			if (static_cast<double>(CaravanMenu::fTimeDelta) >=
				pHandAnimations[isAI_4][iSelectedDeckCard]->m_fEndKeyTime) {
				uint32_t v92;
				UpdateGameHandTextures(isAI_4 == 0, 0);
				pHandAnimManager[isAI_4]->DeactivateSequence(pHandAnimations[isAI_4][iSelectedDeckCard + 1], 0.0f);
				pHandAnimManager[isAI_4]->ActivateSequence(pHandAnimations[isAI_4][1], 0, 0, 1.0f, 0.0f, 0);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1_22, nullptr, 0.0f, 1, 0);
				pHandGeometry[isAI_4]->Update(UpdateData1_22);
				pHandAnimManager[isAI_4]->DeactivateSequence(pHandAnimations[isAI_4][1], 0.0f);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				pTiles[isAI_4 + 29]->Poke(CaravanMenu::iUT_Value, 1.0f);
				if (isAI_4) {
					v92 = pNPCDeck.iSize;
				} else {
					v92 = pPlayerDeck.iSize;
				}
				pTiles[isAI_4 + 29]->Set(CaravanMenu::iUT_Value, v92);
				eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
				iSelectedDeckCard = pPlayerHand.iSize - 1;
				if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) &&
					pPlayerHand.operator[](iSelectedDeckCard)->data.iFaceValue > 10) {
					SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
				}
				if ((isAI_4 == 1 && !this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) ||
					(!isAI_4 && this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP))) {
					SelectDeckCard(0);
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_DISCARDTRACK_ANIM: {
		if (pTrackAnimations[iSelectedTrack][0][0][1]) {
			int32_t cnt = 0;
			if (CaravanMenu::fTimeDelta == 0.0) {
				iSelectedColumn = 4;
				while (!cnt && iSelectedColumn > 0) {
					--iSelectedColumn;
					for (int32_t i_3 = 0; i_3 < 7; ++i_3) {
						int32_t isize = pCardTracks[iSelectedTrack][i_3].iSize;
						if (isize > iSelectedColumn) {
							pTrackAnimManager[iSelectedTrack][i_3]
								->ActivateSequence(pTrackAnimations[iSelectedTrack][i_3][iSelectedColumn][1], 0, 0,
									1.0f, 0.0f, 0);
							++cnt;
						}
					}
				}
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
				fSelectionDelay = 0.0f;
				BSAudio* v59 = &BSAudio::QInstance();
				BSSoundHandle* v91 = v59->GetSoundHandleByName(&v118, "GAMECaravanRemoveCard", 289u);
				v91->Play(0);
			}
			for (int32_t i_4 = 0; i_4 < 7 && pCardTracks[iSelectedTrack][i_4].iSize; ++i_4) {
				NiUpdateData* v60 = FNVD_RECONSTRUCT(NiUpdateData)(&v117, CaravanMenu::fTimeDelta, 1, 0);
				pTrackGeometry[iSelectedTrack][i_4]->Update(*v60);
			}
			if (static_cast<double>(CaravanMenu::fTimeDelta) >
				pTrackAnimations[iSelectedTrack][0][iSelectedColumn][1]->m_fEndKeyTime) {
				for (int32_t i_5 = 0; i_5 < 7; ++i_5) {
					pTrackAnimManager[iSelectedTrack][i_5]
						->DeactivateSequence(pTrackAnimations[iSelectedTrack][i_5][iSelectedColumn][1], 0.0f);
				}
				while (!cnt && iSelectedColumn > 0) {
					--iSelectedColumn;
					for (int32_t i_6 = 0; i_6 < 7; ++i_6) {
						int32_t isize_2 = pCardTracks[iSelectedTrack][i_6].iSize;
						if (isize_2 > iSelectedColumn) {
							pTrackAnimManager[iSelectedTrack][i_6]
								->ActivateSequence(pTrackAnimations[iSelectedTrack][i_6][iSelectedColumn][1], 0, 0,
									1.0f, 0.0f, 0);
							++cnt;
						}
					}
				}
				if (cnt) {
					CaravanMenu::fTimeDelta = 0.0f;
				} else {
					for (int32_t i_7 = 0; i_7 < 7; ++i_7) {
						pCardTracks[iSelectedTrack][i_7].Clear(1);
					}
					eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
					if (this->IsFlagSet(CaravanMenu::CAF_AI)) {
						iSelectedDeckCard = pPlayerHand.iSize - 1;
						SelectDeckCard(0);
					}
					UpdateTrackValue(iSelectedTrack);
					SetFlag(CaravanMenu::CAF_DESTROY_TRACK, 0);
					SetFlag(CaravanMenu::CAF_SELECT_TRACK, 0);
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_JACK_CLEAR_ANIM: {
		if (pTrackAnimations[iSelectedTrack][iSelectedRow][0][1]) {
			int32_t cnt_2 = 0;
			if (CaravanMenu::fTimeDelta == 0.0) {
				iSelectedColumn = 4;
				while (!cnt_2 && iSelectedColumn > 0) {
					--iSelectedColumn;
					for (int32_t i_8 = iSelectedRow; i_8 < 7; ++i_8) {
						int32_t isize_3 = pCardTracks[iSelectedTrack][i_8].iSize;
						if (isize_3 > iSelectedColumn) {
							pTrackAnimManager[iSelectedTrack][i_8]
								->ActivateSequence(pTrackAnimations[iSelectedTrack][i_8][iSelectedColumn][1], 0, 0,
									1.0f, 0.0f, 0);
							++cnt_2;
						}
					}
				}
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
			}
			for (int32_t i_9 = iSelectedRow; i_9 < 7 && pCardTracks[iSelectedTrack][i_9].iSize; ++i_9) {
				NiUpdateData* v61 = FNVD_RECONSTRUCT(NiUpdateData)(&v116, CaravanMenu::fTimeDelta, 1, 0);
				pTrackGeometry[iSelectedTrack][i_9]->Update(*v61);
			}
			if (static_cast<double>(CaravanMenu::fTimeDelta) >
				pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][1]->m_fEndKeyTime) {
				for (int32_t i_10 = iSelectedRow; i_10 < 7; ++i_10) {
					pTrackAnimManager[iSelectedTrack][i_10]
						->DeactivateSequence(pTrackAnimations[iSelectedTrack][i_10][iSelectedColumn][1], 0.0f);
				}
				while (!cnt_2 && iSelectedColumn > 0) {
					--iSelectedColumn;
					for (int32_t i_11 = iSelectedRow; i_11 < 7; ++i_11) {
						int32_t isize_4 = pCardTracks[iSelectedTrack][i_11].iSize;
						if (isize_4 > iSelectedColumn) {
							pTrackAnimManager[iSelectedTrack][i_11]
								->ActivateSequence(pTrackAnimations[iSelectedTrack][i_11][iSelectedColumn][1], 0, 0,
									1.0f, 0.0f, 0);
							++cnt_2;
						}
					}
				}
				if (cnt_2) {
					CaravanMenu::fTimeDelta = 0.0f;
				} else {
					if (this->IsFlagSet(CaravanMenu::CAF_PROCESS_JOKER)) {
						bool bContinue = 1;
						int32_t iSelRowTmp = iSelectedRow;
						while (bContinue) {
							for (int32_t i_13 = iSelRowTmp; i_13 < 6; ++i_13) {
								pCardTracks[iSelectedTrack][i_13].Clear(1);
								BSSimpleArray<TESCaravanCard*, 1024>* pCardArray_2 =
									&pCardTracks[iSelectedTrack][i_13 + 1];
								for (int32_t j_2 = 0;; ++j_2) {
									uint32_t v63 = pCardArray_2->iSize;
									if (j_2 >= v63) {
										break;
									}
									TESCaravanCard** Xe = &pCardArray_2->operator[](j_2);
									pCardTracks[iSelectedTrack][i_13].Add(*Xe);
									UpdateTrackCardTexture(iSelectedTrack, i_13, j_2);
								}
							}
							jokerLocations[iSelectedTrack].Remove(0, 1);
							if (jokerLocations[iSelectedTrack].iSize) {
								int32_t iTmp = iSelRowTmp;
								iSelRowTmp = jokerLocations[iSelectedTrack].operator[](0);
								if (iSelRowTmp > iTmp) {
									--iSelRowTmp;
								}
							} else {
								bContinue = 0;
							}
						}
					} else {
						for (int32_t i_12 = iSelectedRow; i_12 < 6; ++i_12) {
							pCardTracks[iSelectedTrack][i_12].Clear(1);
							BSSimpleArray<TESCaravanCard*, 1024>* pCardArray = &pCardTracks[iSelectedTrack][i_12 + 1];
							for (int32_t j = 0;; ++j) {
								uint32_t v62 = pCardArray->iSize;
								if (j >= v62) {
									break;
								}
								TESCaravanCard** Xd = &pCardArray->operator[](j);
								pCardTracks[iSelectedTrack][i_12].Add(*Xd);
								UpdateTrackCardTexture(iSelectedTrack, i_12, j);
							}
						}
						pCardTracks[iSelectedTrack][6].Clear(1);
					}
					eGameState = CaravanMenu::CA_GAME_JACK_FINISH_ANIM;
				}
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_JACK_FINISH_ANIM: {
		if (pTrackAnimations[iSelectedTrack][iSelectedRow][0][0]) {
			int32_t cnt_3 = 0;
			bool bexit = 0;
			if (CaravanMenu::fTimeDelta == 0.0) {
				iSelectedColumn = -1;
				while (!cnt_3 && iSelectedColumn < 4) {
					++iSelectedColumn;
					for (int32_t i_14 = iSelectedRow; i_14 < 7; ++i_14) {
						int32_t isize_5 = pCardTracks[iSelectedTrack][i_14].iSize;
						if (isize_5 > iSelectedColumn) {
							pTrackAnimManager[iSelectedTrack][i_14]
								->ActivateSequence(pTrackAnimations[iSelectedTrack][i_14][iSelectedColumn][0], 0, 0,
									1.0f, 0.0f, 0);
							++cnt_3;
						}
					}
				}
				if (!cnt_3) {
					bexit = 1;
				}
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
			}
			if (!bexit) {
				for (int32_t i_15 = iSelectedRow; i_15 < 7 && pCardTracks[iSelectedTrack][i_15].iSize; ++i_15) {
					NiUpdateData* v64 = FNVD_RECONSTRUCT(NiUpdateData)(&v115, CaravanMenu::fTimeDelta, 1, 0);
					pTrackGeometry[iSelectedTrack][i_15]->Update(*v64);
				}
			}
			if (bexit ||
				static_cast<double>(CaravanMenu::fTimeDelta) >
					pTrackAnimations[iSelectedTrack][iSelectedRow][iSelectedColumn][0]->m_fEndKeyTime) {
				if (!bexit) {
					for (int32_t i_16 = iSelectedRow; i_16 < 7; ++i_16) {
						pTrackAnimManager[iSelectedTrack][i_16]
							->DeactivateSequence(pTrackAnimations[iSelectedTrack][i_16][iSelectedColumn][0], 0.0f);
					}
					while (!cnt_3 && iSelectedColumn < 4) {
						++iSelectedColumn;
						for (int32_t i_17 = iSelectedRow; i_17 < 7; ++i_17) {
							int32_t isize_6 = pCardTracks[iSelectedTrack][i_17].iSize;
							if (isize_6 > iSelectedColumn) {
								pTrackAnimManager[iSelectedTrack][i_17]
									->ActivateSequence(pTrackAnimations[iSelectedTrack][i_17][iSelectedColumn][0], 0, 0,
										1.0f, 0.0f, 0);
								++cnt_3;
							}
						}
					}
				}
				if (cnt_3) {
					CaravanMenu::fTimeDelta = 0.0f;
				} else {
					UpdateTrackValue(iSelectedTrack);
					if (this->IsFlagSet(CaravanMenu::CAF_PROCESS_JOKER)) {
						eGameState = CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM;
					} else {
						eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
					}
				}
			}
			UpdateCaravanFlags();
		}
		return;
	}
	case CaravanMenu::CA_GAME_JOKER_CLEAR_ANIM: {
		SetFlag(CaravanMenu::CAF_PROCESS_JOKER, 1);
		while (iSelectedTrack < 6 && !jokerLocations[iSelectedTrack].iSize) {
			++iSelectedTrack;
		}
		if (iSelectedTrack < 6) {
			iSelectedRow = jokerLocations[iSelectedTrack].operator[](0);
			eGameState = CaravanMenu::CA_GAME_JACK_CLEAR_ANIM;
		} else {
			eGameState = CaravanMenu::CA_GAME_PROCESS_AI;
			SetFlag(CaravanMenu::CAF_PROCESS_JOKER, 0);
		}
		return;
	}
	case CaravanMenu::CA_GAME_PROCESS_AI: {
		SetFlag(CaravanMenu::CAF_AI, 1);
		if (IsGameOver()) {
			eGameState = CaravanMenu::CA_GAME_GAMEOVER;
		} else {
			if (bContinueProcessing) {
				bContinueProcessing = 0;
				ProcessAI(&aiPackage);
				if (this->IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
					if (aiPackage.iActionType == 6) {
						iSelectedDeckCard = aiPackage.iDeckCard;
						pSelectedCard = pNPCHand.operator[](iSelectedDeckCard);
						// The exe keeps both cards in locals; the hand card's is never read again.
						TESCaravanCard* pDeckCard = pNPCDeck.operator[](this->iNextNPCCard);
						TESCaravanCard* pHandCard = pNPCHand.operator[](iSelectedDeckCard);
						(void)pHandCard;
						pNPCHand.Remove(iSelectedDeckCard, 1);
						pNPCHand.Add(pDeckCard);
						pNPCDeck.Remove(this->iNextNPCCard, 1);
						uint32_t v65 = pNPCDeck.iSize;
						this->iNextNPCCard = RandomInt(0, v65);
						UpdateDealCardTexture(0);
						bContinueProcessing = 1;
						eGameState = CaravanMenu::CA_GAME_INTRO_DISCARD_ANIM;
					} else {
						int32_t iTrack_2;
						for (iTrack_2 = 5; iTrack_2 > 2 && pCardTracks[iTrack_2]->iSize; --iTrack_2) {
							;
						}
						iSelectedTrack = iTrack_2;
						iSelectedDeckCard = aiPackage.iDeckCard;
						iSelectedRow = 0;
						iSelectedColumn = pCardTracks[iTrack_2][iSelectedRow].iSize;
						pSelectedCard = pNPCHand.operator[](iSelectedDeckCard);
						aiPackage.iTrack = iTrack_2;
						aiPackage.iRow = 0;
						if (CaravanMenu::bDebugAI) {
							aiPackage.Print_WhatIsHeThinking(&pNPCHand);
						}
						TESCaravanCard* pTrackCard = pNPCHand.operator[](iSelectedDeckCard);
						pNPCHand.Remove(iSelectedDeckCard, 1);
						pCardTracks[iTrack_2][iSelectedRow].Add(pTrackCard);
						eGameState = CaravanMenu::CA_GAME_INTRO_DEAL_ANIM;
					}
				} else {
					TESCaravanCard* pHandCard;
					if (aiPackage.iActionType == 6) {
						if (this->iNextNPCCard == -1) {
							iSelectedDeckCard = aiPackage.iDeckCard;
							// The exe keeps the discarded card in a local it never reads again.
							pHandCard = pNPCHand.operator[](iSelectedDeckCard);
							(void)pHandCard;
							pNPCHand.Remove(iSelectedDeckCard, 1);
							UpdateDealCardTexture(0);
						} else {
							iSelectedDeckCard = aiPackage.iDeckCard;
							TESCaravanCard* pDeckCard = pNPCDeck.operator[](this->iNextNPCCard);
							TESCaravanCard* pDiscardCard = pNPCHand.operator[](iSelectedDeckCard);
							(void)pDiscardCard;
							pNPCHand.Remove(iSelectedDeckCard, 1);
							pNPCHand.Add(pDeckCard);
							pNPCDeck.Remove(this->iNextNPCCard, 1);
							if (pNPCDeck.iSize) {
								uint32_t v66 = pNPCDeck.iSize;
								this->iNextNPCCard = RandomInt(0, v66);
							} else {
								this->iNextNPCCard = -1;
							}
							UpdateDealCardTexture(0);
						}
						eGameState = CaravanMenu::CA_GAME_DISCARDCARD_ANIM;
					} else {
						if (aiPackage.iActionType == 5) {
							iSelectedTrack = aiPackage.iTrack;
							trackInfo[iSelectedTrack].iValue = 0;
							eGameState = CaravanMenu::CA_GAME_DISCARDTRACK_ANIM;
						} else {
							if (this->iNextNPCCard == -1) {
								NiAVObject* deck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_06:5");
								if (deck) {
									deck->SetAppCulled(1);
									FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1, nullptr, 0.0f, 0, 0);
									deck->Update(update1);
								}
								deck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_Back:0");
								if (deck) {
									deck->SetAppCulled(1);
									FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update2, nullptr, 0.0f, 0, 0);
									deck->Update(update2);
								}
								deck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_06_Shadow:0");
								if (deck) {
									deck->SetAppCulled(1);
									FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update2_2, nullptr, 0.0f, 0, 0);
									deck->Update(update2_2);
								}
								deck = BSUtilities::GetObjectByName(pHandGeometry[1], "Opponent-Deck_Back_Shadow:0");
								if (deck) {
									deck->SetAppCulled(1);
									FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update2_3, nullptr, 0.0f, 0, 0);
									deck->Update(update2_3);
								}
								iSelectedTrack = aiPackage.iTrack;
								iSelectedRow = aiPackage.iRow;
								iSelectedColumn = pCardTracks[iSelectedTrack][iSelectedRow].iSize;
								iSelectedDeckCard = aiPackage.iDeckCard;
								pSelectedCard = pNPCHand.operator[](iSelectedDeckCard);
								TESCaravanCard* pTrackCard_2 = pNPCHand.operator[](iSelectedDeckCard);
								pNPCHand.Remove(iSelectedDeckCard, 1);
								if (CaravanMenu::bDebugAI) {
									aiPackage.Print_WhatIsHeThinking(&pNPCHand);
								}
								pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard_2);
							} else {
								iSelectedTrack = aiPackage.iTrack;
								iSelectedRow = aiPackage.iRow;
								iSelectedColumn = pCardTracks[iSelectedTrack][iSelectedRow].iSize;
								iSelectedDeckCard = aiPackage.iDeckCard;
								pSelectedCard = pNPCHand.operator[](iSelectedDeckCard);
								TESCaravanCard* pDeckCard = pNPCDeck.operator[](this->iNextNPCCard);
								pHandCard = pNPCHand.operator[](iSelectedDeckCard);
								pNPCHand.Remove(iSelectedDeckCard, 1);
								pNPCHand.Add(pDeckCard);
								if (CaravanMenu::bDebugAI) {
									aiPackage.Print_WhatIsHeThinking(&pNPCHand);
								}
								pNPCDeck.Remove(this->iNextNPCCard, 1);
								if (pNPCDeck.iSize) {
									uint32_t v67 = pNPCDeck.iSize;
									this->iNextNPCCard = RandomInt(0, v67);
								} else {
									this->iNextNPCCard = -1;
								}
								pCardTracks[iSelectedTrack][iSelectedRow].Add(pHandCard);
								UpdateDealCardTexture(0);
							}
							eGameState = CaravanMenu::CA_GAME_PLACECARD_ANIM;
						}
					}
				}
			} else {
				iSelectedDeckCard = pPlayerHand.iSize - 1;
				iSelectedTrack = 0;
				pSelectedCard = 0;
				pSelectedObject = 0;
				SelectDeckCard(0);
				bContinueProcessing = !bContinueProcessing;
				eGameState = CaravanMenu::CA_IDLE;
				SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				SetFlag(CaravanMenu::CAF_AI, 0);
			}
		}
		return;
	}
	case CaravanMenu::CA_GAME_GAMEOVER: {
		SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
		PrepareResultsMenu();
		eGameState = CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM;
		return;
	}
	case CaravanMenu::CA_CLOSING_MENU: {
		if (xFadeState != Interface::FADING_OUT) {
			InterfaceManager* v6 = InterfaceManager::Instance();
			v6->pCursor->Set(4009, 255.0f, 1);
			pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
			StartFadeOut();
			CaravanMenu::pMe = 0;
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

bool CaravanMenu::DoKey(int32_t a1) {
	unkE80 = 0;
	bool v5 = 0;
	if (!InterfaceManager::Has360Controller()) {
		if (eActiveMenu == 3) {
			Interface::enumMenus v2 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v2)) {
				CaravanMenu::Close();
				return 1;
			}
		}
		if (a1 > 65) {
			switch (a1) {
			case 69:
			case 101: {
				v5 = DoGamepad(14, 0.0f);
				break;
			}
			case 70:
			case 102: {
				v5 = DoGamepad(12, 0.0f);
				break;
			}
			case 81:
			case 113: {
				v5 = DoGamepad(13, 0.0f);
				break;
			}
			case 82:
			case 114: {
				v5 = DoGamepadUpEvent(10, 0.0f);
				break;
			}
			case 83:
			case 115: {
				v5 = DoGamepad(16, 0.0f);
				break;
			}
			case 87:
			case 119: {
				v5 = DoGamepad(9, 0.0f);
				break;
			}
			case 97: {
				return DoGamepad(11, 0.0f);
			}
			default: {
				return v5;
			}
			}
		} else {
			if (a1 == 65) {
				return DoGamepad(11, 0.0f);
			} else {
				if (a1 != 0x80000000u && a1 <= -0x7FFFFFFC && !eGameState) {
					unkE80 = a1;
				}
			}
		}
	}
	return v5;
}

int32_t CaravanMenu::GetClass() {
	return 1083;
}

// The exe's cleanup-only C++ EH frame is left out.
bool CaravanMenu::DoGamepad(int32_t a1, float a2) {
	bool bhandled_2;
	FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a12, std::nullopt, std::nullopt);
	int32_t i;
	using randomints_t = BSSimpleArray<int32_t, 1024>;
	FNVD_INLINED_LOCAL(randomints_t, randomints, std::nullopt, std::nullopt);
	bool bhandled = 0;
	if (IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS) || (bTutorial || !IsFlagSet(CaravanMenu::CAF_ISONTOP))) {
		return eGameState == CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM;
	}
	if (IsFlagSet(CaravanMenu::CAF_AI) || eGameState) {
		return 0;
	}
	switch (a1) {
	case 5:
	case 6:
	case 17:
	case 18: {
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v19 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v19)) {
				CaravanMenu::Close();
			}
		}
		goto LABEL_135;
	}
	case 9: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_A)) {
			return 0;
		}
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v4 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v4)) {
				CaravanMenu::Close();
			}
		}
		if (eActiveMenu == CaravanMenu::CA_ANTE_MENU) {
			int32_t iPreviousAnte = iPlayerAnte;
			iPlayerAnte = BSMIN(iNPCAnte, iTotalFunds);
			pTiles[5]->Poke(CaravanMenu::iUT_Value, 1.0f);
			pTiles[5]->Set(CaravanMenu::iUT_Value, CaravanMenu::pMe->iPlayerAnte);
			bhandled = 1;
			// The exe computes the NPC's funds times the bet percentage twice (fild; fmul dword) before the call and
			// keeps both as doubles; the two values are the same.
			double fBaseBet = static_cast<double>(CaravanMenu::pMe->iNPCTotalFunds) * CaravanMenu::pMe->fBetPercentage;
			int32_t maxbet =
				static_cast<int32_t>(pPlayer->GetClampedActorFloatValue(ActorValue::BARTER) / 100.0 * fBaseBet +
					fBaseBet);
			UpdateBettingUI(maxbet, iPlayerAnte - iPreviousAnte);
			UpdateCaravanFlags();
			UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			goto LABEL_135;
		}
		if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
			++iCardsInDeck;
			pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
			pTiles[8]->Set(CaravanMenu::iUT_Value, iCardsInDeck);
			int32_t index = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
			pDeckArray.operator[](index)->bIsActive = 1;
			bhandled = 1;
			eGameState = CaravanMenu::CA_DECK_FLIP_ANIM;
			goto LABEL_135;
		}
		if (eActiveMenu != CaravanMenu::CA_GAME_MENU) {
			goto LABEL_135;
		}
		if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) || IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
			if (!IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
				BSAudio* v7 = &BSAudio::QInstance();
				BSSoundHandle* SoundHandleByName = v7->GetSoundHandleByName(&arHandle, "GAMECaravanAddToTrack", 289u);
				SoundHandleByName->Play(0);
				SetFlag(CaravanMenu::CAF_SELECT_TRACK, 1);
				iSelectedTrack = 0;
				iSelectedRow = 0;
				iSelectedColumn = 0;
				HandleTrackSelection(0, 0);
				bhandled = 1;
				goto LABEL_135;
			}
			if (IsFlagSet(CaravanMenu::CAF_DESTROY_TRACK)) {
				DeselectWholeTrack(iSelectedTrack);
				trackInfo[iSelectedTrack].iValue = 0;
				eGameState = CaravanMenu::CA_GAME_DISCARDTRACK_ANIM;
				return 1;
			}
			if (IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
				if (IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
					SetFlag(CaravanMenu::CAF_SELECT_TRACK, 0);
					DeselectTrackCard();
					eGameState = CaravanMenu::CA_GAME_INTRO_DEAL_ANIM;
					TESCaravanCard* pTrackCard = pPlayerHand.operator[](iSelectedDeckCard);
					pPlayerHand.Remove(iSelectedDeckCard, 1);
					pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard);
				} else {
					SetFlag(CaravanMenu::CAF_SELECT_TRACK, 0);
					DeselectTrackCard();
					eGameState = CaravanMenu::CA_GAME_PLACECARD_ANIM;
					if (iNextPlayerCard == -1) {
						NiAVObject* deck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_06_Back:0");
						if (deck) {
							deck->SetAppCulled(1);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1, nullptr, 0.0f, 0, 0);
							deck->Update(update1);
						}
						deck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_06:5");
						if (deck) {
							deck->SetAppCulled(1);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1_2, nullptr, 0.0f, 0, 0);
							deck->Update(update1_2);
						}
						deck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_06:5");
						if (deck) {
							deck->SetAppCulled(1);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1_3, nullptr, 0.0f, 0, 0);
							deck->Update(update1_3);
						}
						deck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_Back:0");
						if (deck) {
							deck->SetAppCulled(1);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1_4, nullptr, 0.0f, 0, 0);
							deck->Update(update1_4);
						}
						deck = BSUtilities::GetObjectByName(pHandGeometry[0], "Player-Deck_Back_Shadow:0");
						if (deck) {
							deck->SetAppCulled(1);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1_5, nullptr, 0.0f, 0, 0);
							deck->Update(update1_5);
						}
					}
					if (iNextPlayerCard == -1) {
						TESCaravanCard* pTrackCard_3 = pPlayerHand.operator[](iSelectedDeckCard);
						pPlayerHand.Remove(iSelectedDeckCard, 1);
						pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard_3);
					} else {
						TESCaravanCard* pDeckCard = pPlayerDeck.operator[](iNextPlayerCard);
						TESCaravanCard* pTrackCard_2 = pPlayerHand.operator[](iSelectedDeckCard);
						pPlayerHand.Remove(iSelectedDeckCard, 1);
						pPlayerHand.Add(pDeckCard);
						pPlayerDeck.Remove(iNextPlayerCard, 1);
						pCardTracks[iSelectedTrack][iSelectedRow].Add(pTrackCard_2);
						if (pPlayerDeck.iSize) {
							uint32_t FormFlags = pPlayerDeck.iSize;
							iNextPlayerCard = RandomInt(0, FormFlags);
						} else {
							iNextPlayerCard = -1;
						}
						UpdateDealCardTexture(1);
					}
				}
				bhandled = 1;
			}
		} else {
			iSelectedTrack = 0;
			iSelectedRow = 0;
			iSelectedColumn = 0;
			while (pCardTracks[iSelectedTrack]->iSize) {
				++iSelectedTrack;
			}
			if (iSelectedTrack < 3) {
				SelectTrackCard(iSelectedTrack, iSelectedRow);
			}
			SelectDeckCard(1);
			BSAudio* v6 = &BSAudio::QInstance();
			BSSoundHandle* v24 = v6->GetSoundHandleByName(&a12, "GAMECaravanAddToTrack", 289u);
			v24->Play(0);
			UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			SetFlag(CaravanMenu::CAF_SELECT_TRACK, 1);
			bhandled = 1;
		}
		goto LABEL_135;
	}
	case 11: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_X)) {
			return 0;
		}
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v9 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v9)) {
				CaravanMenu::Close();
			}
		}
		if (eActiveMenu) {
			if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
				--iCardsInDeck;
				pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
				pTiles[8]->Set(CaravanMenu::iUT_Value, iCardsInDeck);
				uint32_t Float = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
				pDeckArray.operator[](Float)->bIsActive = 0;
				bhandled = 1;
				eGameState = CaravanMenu::CA_DECK_FLIP_ANIM;
			}
		} else {
			SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
			UpdateCaravanFlags();
			UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			if (iTotalFunds - iPlayerAnte > 0) {
				QuantityMenu::Create(iTotalFunds - iPlayerAnte, &CaravanMenu::ItemSelectCallback, 0x7FFFFFFF);
			}
			bhandled = 1;
		}
		goto LABEL_135;
	}
	case 12: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y)) {
			return 0;
		}
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v10 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v10)) {
				CaravanMenu::Close();
			}
		}
		if (eActiveMenu) {
			if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
				if (iCardsInDeck >= 30) {
					eGameState = CaravanMenu::CA_DECK_WIPE_ANIM;
				}
				bhandled = 1;
			}
		} else {
			if (iPlayerAnte > 0 || !iNPCAnte) {
				LoadNifs();
				BSAudio* v11 = &BSAudio::QInstance();
				BSSoundHandle* v22 = v11->GetSoundHandleByName(&th_s, "UIMenuOK", 289u);
				v22->Play(0);
			}
			bhandled = 1;
		}
		goto LABEL_135;
	}
	case 13: {
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v16 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v16)) {
				CaravanMenu::Close();
			}
		}
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_LT)) {
			return 0;
		}
		if (eActiveMenu == CaravanMenu::CA_GAME_MENU && !IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
			if (pPlayerDeck.iSize) {
				eGameState = static_cast<CaravanMenu::CaravanState>(IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)
						? CaravanMenu::CA_GAME_INTRO_DISCARD_ANIM
						: CaravanMenu::CA_GAME_DISCARDCARD_ANIM);
				if (iNextPlayerCard == -1) {
					TESCaravanCard* pTrackCard_4 = pPlayerHand.operator[](iSelectedDeckCard);
					pPlayerHand.Remove(iSelectedDeckCard, 1);
				} else {
					TESCaravanCard* pDeckCard_2 = pPlayerDeck.operator[](iNextPlayerCard);
					TESCaravanCard* pTrackCard_5 = pPlayerHand.operator[](iSelectedDeckCard);
					pPlayerHand.Remove(iSelectedDeckCard, 1);
					pPlayerHand.Add(pDeckCard_2);
					pPlayerDeck.Remove(iNextPlayerCard, 1);
					if (pPlayerDeck.iSize) {
						uint32_t v17 = pPlayerDeck.iSize;
						iNextPlayerCard = RandomInt(0, v17);
					} else {
						iNextPlayerCard = -1;
					}
					UpdateDealCardTexture(1);
				}
			}
		}
		goto LABEL_135;
	}
	case 14: {
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v18 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v18)) {
				CaravanMenu::Close();
			}
		}
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_RT)) {
			return 0;
		}
		if (eActiveMenu == CaravanMenu::CA_GAME_MENU && !IsFlagSet(CaravanMenu::CAF_SELECT_TRACK) &&
			!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
			SetFlag(CaravanMenu::CAF_DESTROY_TRACK, 1);
			SetFlag(CaravanMenu::CAF_SELECT_TRACK, 1);
			iSelectedTrack = 0;
			SelectWholeTrack(iSelectedTrack);
		}
		goto LABEL_135;
	}
	case 15: {
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v12 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v12)) {
				CaravanMenu::Close();
			}
		}
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_LB)) {
			return 0;
		}
		if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
			bhandled = 1;
		}
		goto LABEL_135;
	}
	case 16: {
		if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
			bhandled = 1;
			Interface::enumMenus v13 = static_cast<Interface::enumMenus>(GetClass());
			if (Interface::IsTopMenuID(v13)) {
				CaravanMenu::Close();
			}
		}
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_RB)) {
			bhandled_2 = 0;
		} else {
			if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
				int32_t numcards;
				::new (&randomints) BSSimpleArray<int32_t, 1024>;
				int32_t isize = pDeckArray.iSize;
				for (i = 0; i < isize; ++i) {
					randomints.Add(i);
					pDeckArray.operator[](i)->bIsActive = 0;
					iCardsInDeck = 0;
				}
				if (isize <= 30) {
					numcards = 30;
				} else {
					numcards = RandomInt(30, isize);
				}
				for (int32_t i_2 = 0; i_2 < numcards; ++i_2) {
					uint32_t v14 = randomints.iSize;
					int32_t j = RandomInt(0, v14);
					int32_t k = randomints.operator[](j);
					randomints.Remove(j, 1);
					if (!pDeckArray.operator[](k)->bIsActive) {
						++iCardsInDeck;
						pDeckArray.operator[](k)->bIsActive = 1;
					}
				}
				randomints.Clear(1);
				pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
				pTiles[8]->Set(CaravanMenu::iUT_Value, iCardsInDeck);
				UpdateCaravanFlags();
				UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				double v15 = pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue);
				SwapDeckBuildingTextures(static_cast<int32_t>(v15), 0);
				bhandled = 1;
				randomints.~BSSimpleArray<int32_t, 1024>();
			}
		LABEL_135:;
			UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
			bhandled_2 = bhandled;
		}
		break;
	}
	default: {
		goto LABEL_135;
	}
	}
	return bhandled_2;
}

bool CaravanMenu::DoGamepadUpEvent(int32_t a1, float a2) {
	Controls* Controls_;
	if (IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS) || a1 != 10) {
		return 0;
	}
	if (eActiveMenu == CaravanMenu::CA_RESULTS_MENU) {
		Interface::enumMenus v4 = static_cast<Interface::enumMenus>(GetClass());
		if (Interface::IsTopMenuID(v4)) {
			CaravanMenu::Close();
		}
	}
	if (eActiveMenu) {
		if (eActiveMenu != CaravanMenu::CA_DECK_MENU) {
			if (eActiveMenu == CaravanMenu::CA_GAME_MENU) {
				if (IsFlagSet(CaravanMenu::CAF_SELECT_TRACK)) {
					if (IsFlagSet(CaravanMenu::CAF_DESTROY_TRACK)) {
						DeselectWholeTrack(iSelectedTrack);
					} else {
						DeselectTrackCard();
					}
					SetFlag(CaravanMenu::CAF_DESTROY_TRACK, 0);
					SetFlag(CaravanMenu::CAF_SELECT_TRACK, 0);
					SetFlag(CaravanMenu::CAF_DISABLE_A, 0);
					SetFlag(CaravanMenu::CAF_DISABLE_X, 1);
					UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
				} else {
					Interface::enumMenus v8 = static_cast<Interface::enumMenus>(GetClass());
					if (Interface::IsTopMenuID(v8)) {
						const char* v10 = sNo.String();
						const char* v9 = sYes.String();
						Interface::CreateMessageMenu(sQuitCaravanText.String(), static_cast<const char*>(0),
							static_cast<const char*>(0), &CaravanMenu::CancelConfirmFunc, 0, 23, 0.0f, 0.0f, v9, v10,
							nullptr);
						SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
					}
				}
			}
			goto LABEL_23;
		}
		Interface::enumMenus v7 = static_cast<Interface::enumMenus>(GetClass());
		if (Interface::IsTopMenuID(v7)) {
			CaravanMenu::Close();
		}
	} else {
		Interface::enumMenus v5 = static_cast<Interface::enumMenus>(GetClass());
		if (Interface::IsTopMenuID(v5)) {
			CaravanMenu::Close();
		}
	}
	Controls_ = appMain->GetControls();
	Controls_->ClearUserActions();
LABEL_23:;
	InterfaceManager::Instance();
	InterfaceManager::ClearMenuButton(Interface::eButtonB);
	return 1;
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
	for (int32_t i = 0; i < 6; ++i)
		UpdateTrackValue(i);
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
		if (iPlayerTracks > iNPCTracks && iDecidedTracks == 3)
			bPlayerWon = true;
		if (iPlayerTracks < iNPCTracks && iDecidedTracks == 3)
			bNPCWon = true;
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
	uint32_t iFlags;
	uint32_t v4;
	if (abValue) {
		this->iFlags |= aeFlag;
		iFlags = this->iFlags;
	} else {
		this->iFlags &= (~aeFlag);
		v4 = this->iFlags;
	}
	uint32_t v5 = this->iFlags;
}

// The exe calls itself for the last tile of each tileset and then returns; it does not tail-jump.
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
		if (!IsFlagSet(CaravanMenu::CAF_DISABLE_Y) && !IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_53;
		}
		goto LABEL_52;
	}
	case CaravanMenu::TI_DECK_CARDS: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y)) {
			goto LABEL_52;
		}
		SetTileTextAlpha(eTile, 255.0f);
		if (iCardsInDeck >= 30) {
			pTiles[eTile]->xChildren.GetHead()->Set(4009, 255.0f, 1);
		} else {
			pTiles[eTile]->xChildren.GetHead()->Set(4009, 128.0f, 1);
		}
		break;
	}
	case CaravanMenu::TI_DECK_TOTAL_CARDS:
	case CaravanMenu::TI_DECK_ALLCARDS_BUTTON: {
		goto LABEL_53;
	}
	case CaravanMenu::TI_DECK_CARAVANDECK_BUTTON: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_RB)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_DECK_ADD_BUTTON: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_A) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_DECK_REMOVE_BUTTON: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_X) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_DECK_PLAY_BUTTON: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_Y) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_GAME_SELECT_CARD: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_A) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_GAME_DISCARD_CARD: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_LT) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_GAME_DISCARD_TRACK: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_RT) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_GAME_CANCEL: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_B) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_ANTE_RAISE_BUTTON: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_X) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
			goto LABEL_52;
		}
		goto LABEL_53;
	}
	case CaravanMenu::TI_ANTE_AUTOMATCH_BUTTON: {
		if (IsFlagSet(CaravanMenu::CAF_DISABLE_A) || IsFlagSet(CaravanMenu::CAF_DISABLE_ALLCONTROLS)) {
		LABEL_52:;
			SetTileTextAlpha(eTile, 128.0f);
		} else {
		LABEL_53:;
			SetTileTextAlpha(eTile, 255.0f);
		}
		break;
	}
	default: {
		return;
	}
	}
}

// Sets the alpha trait of the child tiles of pTiles[aeTile]. The exe reads the head node through the shared one-field
// getter 0x559450 and walks the list with GetHead, Get and GetNextPos.
void CaravanMenu::SetTileTextAlpha(TileIndex TI, float alpha) {
	Tile* pTile = CaravanMenu::pMe->pTiles[TI];
	NiTPointerListBase<Tile*>* pChildren = &pTile->xChildren;
	NiTListItem<Tile*>* pos = pTile->xChildren.m_pkHead;
	while (pos && pChildren && pChildren->GetHead() != nullptr) {
		// SWEEP-INLINE 0x73AC90
		pChildren->Get(pos)->Set(Tile::kTileValue_Alpha, alpha, true);
		pos = pChildren->GetNextPos(pos);
	}
}

void CaravanMenu::UpdateCaravanFlags() {
	CaravanMenu::ActiveMenu eActiveMenu = this->eActiveMenu;
	if (eActiveMenu) {
		if (eActiveMenu == CaravanMenu::CA_DECK_MENU) {
			uint32_t Float = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
			// The exe's getter (0x73CB80) returns the field's raw byte and the caller tests all of it (movzx; test):
			// read as a byte, or clang merges the branches and passes a non-0/1 bool on to SetFlag.
			if (pDeckArray.operator[](Float)->bIsActive != 0) {
				SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
				SetFlag(CaravanMenu::CAF_DISABLE_X, 0);
			} else {
				SetFlag(CaravanMenu::CAF_DISABLE_A, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_X, 1);
			}
			if (iCardsInDeck >= 30) {
				SetFlag(CaravanMenu::CAF_DISABLE_Y, 0);
				pTiles[8]->Set(4009, 255.0f, 1);
			} else {
				SetFlag(CaravanMenu::CAF_DISABLE_Y, 1);
				pTiles[8]->Set(4009, 128.0f, 1);
			}
			if (!iCardsInDeck) {
				SetFlag(CaravanMenu::CAF_DISABLE_X, 1);
			}
			if (iCardsInDeck >= 108) {
				SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
			}
			SetFlag(CaravanMenu::CAF_DISABLE_LB, 1);
		} else {
			if (eActiveMenu == CaravanMenu::CA_GAME_MENU) {
				if (!pPlayerDeck.iSize) {
					iNextPlayerCard = -1;
					SetFlag(CaravanMenu::CAF_DISABLE_LT, 1);
				}
				if (!pNPCDeck.iSize) {
					iNextNPCCard = -1;
				}
				if ((pCardTracks[0][0].iSize || (pCardTracks[1][0].iSize || pCardTracks[2][0].iSize)) &&
					!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
					SetFlag(CaravanMenu::CAF_DISABLE_RT, 0);
				} else {
					SetFlag(CaravanMenu::CAF_DISABLE_RT, 1);
				}
			}
		}
	} else {
		if (iPlayerAnte <= 0 && iNPCAnte) {
			SetFlag(CaravanMenu::CAF_DISABLE_Y, 1);
		} else {
			SetFlag(CaravanMenu::CAF_DISABLE_Y, 0);
		}
		if (iTotalFunds) {
			if (iPlayerAnte <= 0) {
				SetFlag(CaravanMenu::CAF_DISABLE_A, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_X, 0);
			} else {
				SetFlag(CaravanMenu::CAF_DISABLE_A, 0);
				SetFlag(CaravanMenu::CAF_DISABLE_X, 0);
				if (iPlayerAnte >= iNPCAnte || (iPlayerAnte >= iTotalFunds && iPlayerAnte < iNPCAnte)) {
					SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
				}
				if (iPlayerAnte >= iTotalFunds) {
					SetFlag(CaravanMenu::CAF_DISABLE_X, 1);
				}
			}
		} else {
			SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
			SetFlag(CaravanMenu::CAF_DISABLE_X, 1);
		}
	}
}

void CaravanMenu::UpdateBettingUI(int32_t maxraise, int32_t raise) {
	NiUpdateData update2_2;
	NiUpdateData update2;
	NiMatrix3 mat_2;
	NiMatrix3 mat;
	char blockName[264];
	int32_t ibills = 0;
	int32_t icoins = 0;
	if (maxraise >= 120) {
		if (maxraise >= 400) {
			if (maxraise >= 1000) {
				if (maxraise >= 2000) {
					while (raise > 0 && (ibills < 9 && icoins < 42)) {
						int32_t iBillChance_5 = RandomInt(0, 9 - ibills);
						int32_t iCoinChance_5 = RandomInt(0, 42 - icoins);
						int32_t index_9 = 0;
						if (iBillChance_5 <= iCoinChance_5) {
							int32_t v15;
							int32_t index_4;
							index_9 = RandomInt(0, 100);
							if (index_9 >= 10) {
								index_4 = 2;
							} else {
								index_4 = index_9 >= 5;
							}
							index_9 = index_4;
							LoadNewCoin(index_4);
							if (index_4 >= 2) {
								v15 = 100;
							} else {
								v15 = index_9 != 0 ? 4 : 1;
							}
							raise -= v15;
							++icoins;
						} else {
							index_9 = 2;
							LoadNewBill(2);
							raise -= 40;
							++ibills;
						}
					}
				} else {
					while (raise > 0 && (ibills < 9 && icoins < 42)) {
						int32_t iBillChance_4 = RandomInt(0, 9 - ibills);
						int32_t iCoinChance_4 = RandomInt(0, 42 - icoins);
						int32_t index_8 = 0;
						if (!(static_cast<double>(iCoinChance_4) < static_cast<double>(iBillChance_4) * 1.5)) {
							int32_t v17;
							int32_t index_3;
							index_8 = RandomInt(0, 100);
							if (index_8 >= 20) {
								index_3 = 2;
							} else {
								index_3 = index_8 >= 5;
							}
							index_8 = index_3;
							LoadNewCoin(index_3);
							if (index_3 >= 2) {
								v17 = 100;
							} else {
								v17 = index_8 != 0 ? 4 : 1;
							}
							raise -= v17;
							++icoins;
						} else {
							int32_t v19;
							index_8 = RandomInt(1, 3);
							LoadNewBill(index_8);
							if (index_8 >= 2) {
								v19 = 40;
							} else {
								v19 = index_8 != 0 ? 8 : 2;
							}
							raise -= v19;
							++ibills;
						}
					}
				}
			} else {
				while (raise > 0 && (ibills < 9 && icoins < 42)) {
					int32_t iBillChance_3 = RandomInt(0, 9 - ibills);
					int32_t iCoinChance_3 = RandomInt(0, 42 - icoins);
					int32_t index_7 = 0;
					if (!(static_cast<double>(iCoinChance_3) < static_cast<double>(iBillChance_3) * 1.5)) {
						int32_t v20;
						int32_t index_2;
						index_7 = RandomInt(0, 100);
						if (index_7 >= 66) {
							index_2 = 2;
						} else {
							index_2 = index_7 >= 33;
						}
						index_7 = index_2;
						LoadNewCoin(index_2);
						if (index_2 >= 2) {
							v20 = 100;
						} else {
							v20 = index_7 != 0 ? 4 : 1;
						}
						raise -= v20;
						++icoins;
					} else {
						int32_t v22;
						index_7 = RandomInt(1, 3);
						LoadNewBill(index_7);
						if (index_7 >= 2) {
							v22 = 40;
						} else {
							v22 = index_7 != 0 ? 8 : 2;
						}
						raise -= v22;
						++ibills;
					}
				}
			}
		} else {
			while (raise > 0 && (ibills < 9 && icoins < 42)) {
				int32_t iBillChance_2 = RandomInt(0, 9 - ibills);
				int32_t iCoinChance_2 = RandomInt(0, 42 - icoins);
				int32_t index_6 = 0;
				if (!(static_cast<double>(iCoinChance_2) < static_cast<double>(iBillChance_2) * 1.5)) {
					int32_t v23;
					int32_t index;
					index_6 = RandomInt(0, 100);
					if (index_6 >= 75) {
						index = 2;
					} else {
						index = index_6 >= 33;
					}
					index_6 = index;
					CaravanMenu::pMe->LoadNewCoin(index);
					if (index >= 2) {
						v23 = 100;
					} else {
						v23 = index_6 != 0 ? 4 : 1;
					}
					raise -= v23;
					++icoins;
				} else {
					int32_t v25;
					index_6 = RandomInt(0, 3);
					CaravanMenu::pMe->LoadNewBill(index_6);
					if (index_6 >= 2) {
						v25 = 40;
					} else {
						v25 = index_6 != 0 ? 8 : 2;
					}
					raise -= v25;
					++ibills;
				}
			}
		}
	} else {
		while (raise > 0 && (ibills < 9 && icoins < 42)) {
			int32_t iBillChance = RandomInt(0, 9 - ibills);
			int32_t iCoinChance = RandomInt(0, 42 - icoins);
			int32_t index_5 = 0;
			if (!(static_cast<double>(iCoinChance) < static_cast<double>(iBillChance) * 1.5)) {
				int32_t v26;
				RandomInt(0, 100);
				index_5 = RandomInt(0, 2);
				CaravanMenu::pMe->LoadNewCoin(index_5);
				if (index_5 >= 2) {
					v26 = 100;
				} else {
					v26 = index_5 != 0 ? 4 : 1;
				}
				raise -= v26;
				++icoins;
			} else {
				int32_t v27;
				index_5 = RandomInt(0, 3);
				CaravanMenu::pMe->LoadNewBill(index_5);
				if (index_5 >= 2) {
					v27 = 40;
				} else {
					v27 = index_5 != 0 ? 8 : 2;
				}
				raise -= v27;
				++ibills;
			}
		}
	}
	for (int32_t i = iBillIndexToAnim;; ++i) {
		uint32_t FormFlags = pListOfBills.iSize;
		if (i >= FormFlags || !intlistbills.iSize) {
			break;
		}
		uint32_t v4 = intlistbills.iSize;
		int32_t index_10 = RandomInt(0, v4);
		int32_t locindex = intlistbills.operator[](index_10);
		intlistbills.RemoveFast(index_10, 1u);
		BSsprintf(blockName, 260, "Bill-Placement_0%i:0", locindex + 1);
		NiAVObject* avobj = BSUtilities::GetObjectByName(pBillsGrid, static_cast<const char*>(blockName));
		const NiBound* WorldBound = &avobj->GetWorldBound();
		float x = WorldBound->m_kCenter.x;
		float y = WorldBound->m_kCenter.y;
		float z = WorldBound->m_kCenter.z;
		mat = (*&avobj->m_kLocal).m_Rotate;
		pListOfBills.operator[](i)->SetRotate(mat);
		pListOfBills.operator[](i)->SetTranslate(x, y, z);
		NiUpdateData* updated = FNVD_RECONSTRUCT(NiUpdateData)(&update2, 0.0f, 0, 0);
		pListOfBills.operator[](i)->Update(*updated);
	}
	for (int32_t i_2 = iCoinIndexToAnim;; ++i_2) {
		uint32_t v7 = pListOfCoins.iSize;
		if (i_2 >= v7 || !intlistcoins.iSize) {
			break;
		}
		uint32_t v8 = intlistcoins.iSize;
		int32_t index_11 = RandomInt(0, v8);
		int32_t v32 = intlistcoins.operator[](index_11);
		intlistcoins.RemoveFast(index_11, 1u);
		if (v32 + 1 >= 10) {
			BSsprintf(blockName, 260, "Coin-Placement_%i:0", v32 + 1);
		} else {
			BSsprintf(blockName, 260, "Coin-Placement_0%i:0", v32 + 1);
		}
		NiAVObject* avobj_2 = BSUtilities::GetObjectByName(pCoinsGrid, static_cast<const char*>(blockName));
		const NiBound* v9 = &avobj_2->GetWorldBound();
		float v33 = v9->m_kCenter.x;
		float v34 = v9->m_kCenter.y;
		float v35 = v9->m_kCenter.z;
		mat_2 = (*&avobj_2->m_kLocal).m_Rotate;
		pListOfCoins.operator[](i_2)->SetRotate(mat_2);
		pListOfCoins.operator[](i_2)->SetTranslate(v33, v34, v35);
		NiUpdateData* v10 = FNVD_RECONSTRUCT(NiUpdateData)(&update2_2, 0.0f, 0, 0);
		pListOfCoins.operator[](i_2)->Update(*v10);
	}
	if (BSMAX(icoins, ibills) > 0) {
		eGameState = CaravanMenu::CA_ANTE_MONEYIN_ANIM;
	}
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update2_3, nullptr, 0.0f, 0, 0);
	CaravanMenu::spModel.m_pObject->Update(update2_3);
	const NiBound* v11 = &CaravanMenu::spModel.m_pObject->GetWorldBound();
	float v14 = v11->m_fRadius * 2.5;
	NiNode* a3 = CaravanMenu::spModel.m_pObject;
	ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
	ShadowSceneNode_->AddAllLightsFromNode_Recurse(a3, v14);
}

void CaravanMenu::UpdateTrackValue(int32_t aiTrack) {
	int32_t i;
	int32_t v9 = 0;
	for (i = 0; i < 7; ++i) {
		int32_t iFaceValue = 0;
		if (!pCardTracks[aiTrack][i].iSize) {
			break;
		}
		for (uint32_t auiIndex = 0; auiIndex < pCardTracks[aiTrack][i].iSize; ++auiIndex) {
			CARAVANCARDDATA* MuzzleFlashModel = &pCardTracks[aiTrack][i].operator[](auiIndex)->data;
			if (auiIndex) {
				if (MuzzleFlashModel->iFaceValue == 14) {
					iFaceValue *= 2;
				}
			} else {
				iFaceValue = MuzzleFlashModel->iFaceValue;
			}
		}
		v9 += iFaceValue;
	}
	trackInfo[aiTrack].iValue = v9;
	if (i) {
		// The track's last card slot and the one before it.
		BSSimpleArray<TESCaravanCard*>& rLastSlot = pCardTracks[aiTrack][i - 1];
		if (i == 1) {
			trackInfo[aiTrack].eCardSuit = pCardTracks[aiTrack]->operator[](0)->data.iCardSuit;
			for (uint32_t j = 1; j < rLastSlot.iSize; ++j) {
				if (rLastSlot.operator[](j)->data.iFaceValue == 13) {
					trackInfo[aiTrack].eCardSuit = rLastSlot.operator[](j)->data.iCardSuit;
				}
			}
			trackInfo[aiTrack].eDirection = 0;
		} else {
			int32_t PackageEvaluateHour = rLastSlot.operator[](0)->data.iFaceValue;
			if (PackageEvaluateHour <= pCardTracks[aiTrack][i - 2].operator[](0)->data.iFaceValue) {
				trackInfo[aiTrack].eDirection = -1;
			} else {
				trackInfo[aiTrack].eDirection = 1;
			}
			trackInfo[aiTrack].eCardSuit = rLastSlot.operator[](0)->data.iCardSuit;
			for (uint32_t k = 1; k < rLastSlot.iSize; ++k) {
				if (rLastSlot.operator[](k)->data.iFaceValue == 13) {
					trackInfo[aiTrack].eCardSuit = rLastSlot.operator[](k)->data.iCardSuit;
					trackInfo[aiTrack].eDirection = -trackInfo[aiTrack].eDirection;
				}
			}
		}
	} else {
		trackInfo[aiTrack].eCardSuit = 5;
		trackInfo[aiTrack].eDirection = 0;
	}
	pTiles[aiTrack + 23]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[aiTrack + 23]->Set(CaravanMenu::iUT_Value, trackInfo[aiTrack].iValue);
	if (trackInfo[aiTrack].iValue < 21) {
		pTiles[aiTrack + 23]->Set(4084, 4084);
		pTiles[aiTrack + 23]->Set(4009, 126.0f, 1);
	}
	if (trackInfo[aiTrack].iValue <= 26) {
		pTiles[aiTrack + 23]->Set(4084, 1);
	} else {
		pTiles[aiTrack + 23]->Set(4084, 2);
	}
	pTiles[aiTrack + 23]->Set(4009, 255.0f, 1);
}

bool CaravanMenu::IsWellFormedMenu() {
	int32_t i;
	for (i = 0; i < 51; ++i) {
		if (!pTiles[i]) {
			return (bool)0;
		}
	}
	return (bool)1;
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::LoadNifs() {
	FNVD_INLINED_LOCAL(NiFixedString, v70, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v71, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v72, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v73, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v74, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v75, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v76, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v77, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v78, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v79, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v80, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiUpdateData apMemory;
	char Src[268];
	pActiveDeckGeometry = (NiAVObject*)pModelLoader->LoadFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Deck.NIF",
		LOD_MULT_NONE, 1, 0, 0, 0);
	pInactiveDeckGeometry =
		(NiAVObject*)pModelLoader->LoadFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Available.NIF", LOD_MULT_NONE, 1,
			0, 0, 0);
	NiNode* v68 = CaravanMenu::spModel.m_pObject;
	v68->AttachChild(pInactiveDeckGeometry, 1);
	NiNode* v67 = CaravanMenu::spModel.m_pObject;
	v67->AttachChild(pActiveDeckGeometry, 1);
	intlistbills.Clear(1);
	intlistcoins.Clear(1);
	NiUpdateData* updated = FNVD_RECONSTRUCT(NiUpdateData)(&apMemory, 0.0f, 0, 0);
	CaravanMenu::spModel.m_pObject->Update(*updated);
	iAnte = BSMIN(iPlayerAnte, iNPCAnte);
	NiControllerManager* Controller =
		static_cast<NiControllerManager*>(pActiveDeckGeometry->GetController(&NiControllerManager::ms_RTTI));
	if (Controller) {
		pActiveDeckAnimManager = Controller;
		NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("Deck_Step-Forward");
		pActiveDeckAnimations[0] = pActiveDeckAnimManager->GetSequenceByName(*a2);
		apString.~NiFixedString();
		NiFixedString* kName = ::new (static_cast<void*>(&v80)) NiFixedString("Deck_Step-Reverse");
		pActiveDeckAnimations[1] = pActiveDeckAnimManager->GetSequenceByName(*kName);
		v80.~NiFixedString();
		NiFixedString* v64 = ::new (static_cast<void*>(&v79)) NiFixedString("Deck_Modify");
		pActiveDeckAnimations[2] = pActiveDeckAnimManager->GetSequenceByName(*v64);
		v79.~NiFixedString();
		NiFixedString* v63 = ::new (static_cast<void*>(&v78)) NiFixedString("Deck_Fast-Forward");
		pActiveDeckAnimations[3] = pActiveDeckAnimManager->GetSequenceByName(*v63);
		v78.~NiFixedString();
		NiFixedString* v62 = ::new (static_cast<void*>(&v77)) NiFixedString("Deck_Fast-Reverse");
		pActiveDeckAnimations[4] = pActiveDeckAnimManager->GetSequenceByName(*v62);
		v77.~NiFixedString();
		NiFixedString* v61 = ::new (static_cast<void*>(&v76)) NiFixedString("Deck_Wipe");
		pActiveDeckAnimations[5] = pActiveDeckAnimManager->GetSequenceByName(*v61);
		v76.~NiFixedString();
		pActiveDeckAnimManager->DeactivateAll(0.0f);
		NiTimeController::StartAnimations(pActiveDeckGeometry);
	}
	Controller = static_cast<NiControllerManager*>(pInactiveDeckGeometry->GetController(&NiControllerManager::ms_RTTI));
	if (Controller) {
		pInactiveDeckAnimManager = Controller;
		NiFixedString* v60 = ::new (static_cast<void*>(&v75)) NiFixedString("Available_Step-Forward");
		pInactiveDeckAnimations[0] = pInactiveDeckAnimManager->GetSequenceByName(*v60);
		v75.~NiFixedString();
		NiFixedString* v59 = ::new (static_cast<void*>(&v74)) NiFixedString("Available_Step-Reverse");
		pInactiveDeckAnimations[1] = pInactiveDeckAnimManager->GetSequenceByName(*v59);
		v74.~NiFixedString();
		NiFixedString* v58 = ::new (static_cast<void*>(&v73)) NiFixedString("Available_Modify");
		pInactiveDeckAnimations[2] = pInactiveDeckAnimManager->GetSequenceByName(*v58);
		v73.~NiFixedString();
		NiFixedString* v57 = ::new (static_cast<void*>(&v72)) NiFixedString("Available_Fast-Forward");
		pInactiveDeckAnimations[3] = pInactiveDeckAnimManager->GetSequenceByName(*v57);
		v72.~NiFixedString();
		NiFixedString* v56 = ::new (static_cast<void*>(&v71)) NiFixedString("Available_Fast-Reverse");
		pInactiveDeckAnimations[4] = pInactiveDeckAnimManager->GetSequenceByName(*v56);
		v71.~NiFixedString();
		NiFixedString* v55 = ::new (static_cast<void*>(&v70)) NiFixedString("Available_Wipe");
		pInactiveDeckAnimations[5] = pInactiveDeckAnimManager->GetSequenceByName(*v55);
		v70.~NiFixedString();
		pInactiveDeckAnimManager->DeactivateAll(0.0f);
		NiTimeController::StartAnimations(pInactiveDeckGeometry);
	}
	for (int32_t i = 0; i < 96; ++i) {
		deckFaceShaders[i] = 0;
	}
	for (int32_t j = 0; j < 24; ++j) {
		if (j >= 9) {
			BSShaderPPLightingProperty* v31;
			int32_t v33;
			BSShaderPPLightingProperty* v34;
			int32_t v36;
			BSShaderPPLightingProperty* v37;
			int32_t v39;
			BSShaderPPLightingProperty* v40;
			int32_t v42;
			char v84[25];
			char v85[25];
			char v86[25];
			char v87[33];
			BSsprintf(v84, 25, "Deck_%i:%i", j + 1, j);
			BSsprintf(v85, 25, "Deck_%i_Back:0", j + 1);
			BSsprintf(v86, 25, "Available_%i:%i", j + 1, j);
			BSsprintf(v87, 25, "Available_%i_Back:0", j + 1);
			NiNode* ObjectByName = 0;
			ObjectByName =
				static_cast<NiNode*>(BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(v84)));
			if (ObjectByName->GetProperty(3)) {
				NiProperty* Property = ObjectByName->GetProperty(3);
				int32_t v41 = false;
				if (static_cast<NiShadeProperty*>(Property)->iShaderPropertyType >= 8) {
					NiProperty* v11 = ObjectByName->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v11)->iShaderPropertyType <= 12) {
						v41 = true;
					}
				}
				v42 = v41;
			} else {
				v42 = 0;
			}
			if (v42) {
				v40 = static_cast<BSShaderPPLightingProperty*>(ObjectByName->GetProperty(3));
			} else {
				v40 = 0;
			}
			deckFaceShaders[j] = v40;
			ObjectByName =
				static_cast<NiNode*>(BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(v85)));
			if (ObjectByName->GetProperty(3)) {
				NiProperty* v12 = ObjectByName->GetProperty(3);
				int32_t v38 = false;
				if (static_cast<NiShadeProperty*>(v12)->iShaderPropertyType >= 8) {
					NiProperty* v13 = ObjectByName->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v13)->iShaderPropertyType <= 12) {
						v38 = true;
					}
				}
				v39 = v38;
			} else {
				v39 = 0;
			}
			if (v39) {
				v37 = static_cast<BSShaderPPLightingProperty*>(ObjectByName->GetProperty(3));
			} else {
				v37 = 0;
			}
			deckFaceShaders[j + 24] = v37;
			ObjectByName = static_cast<NiNode*>(BSUtilities::GetObjectByName(pInactiveDeckGeometry,
				static_cast<const char*>(v86)));
			if (ObjectByName->GetProperty(3)) {
				NiProperty* v14 = ObjectByName->GetProperty(3);
				int32_t v35 = false;
				if (static_cast<NiShadeProperty*>(v14)->iShaderPropertyType >= 8) {
					NiProperty* v15 = ObjectByName->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v15)->iShaderPropertyType <= 12) {
						v35 = true;
					}
				}
				v36 = v35;
			} else {
				v36 = 0;
			}
			if (v36) {
				v34 = static_cast<BSShaderPPLightingProperty*>(ObjectByName->GetProperty(3));
			} else {
				v34 = 0;
			}
			deckFaceShaders[j + 48] = v34;
			ObjectByName = static_cast<NiNode*>(BSUtilities::GetObjectByName(pInactiveDeckGeometry,
				static_cast<const char*>(v87)));
			if (ObjectByName->GetProperty(3)) {
				NiProperty* v16 = ObjectByName->GetProperty(3);
				int32_t v32 = false;
				if (static_cast<NiShadeProperty*>(v16)->iShaderPropertyType >= 8) {
					NiProperty* v17 = ObjectByName->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v17)->iShaderPropertyType <= 12) {
						v32 = true;
					}
				}
				v33 = v32;
			} else {
				v33 = 0;
			}
			if (v33) {
				v31 = static_cast<BSShaderPPLightingProperty*>(ObjectByName->GetProperty(3));
			} else {
				v31 = 0;
			}
			deckFaceShaders[j + 72] = v31;
		} else {
			BSShaderPPLightingProperty* v43;
			int32_t v45;
			BSShaderPPLightingProperty* v46;
			int32_t v48;
			BSShaderPPLightingProperty* v49;
			int32_t v51;
			BSShaderPPLightingProperty* v52;
			int32_t v54;
			char blockName[25];
			char apDest[25];
			char apName[25];
			char apString1[33];
			BSsprintf(blockName, 25, "Deck_0%i:%i", j + 1, j);
			BSsprintf(apDest, 25, "Deck_0%i_Back:0", j + 1);
			BSsprintf(apName, 25, "Available_0%i:%i", j + 1, j);
			BSsprintf(apString1, 25, "Available_0%i_Back:0", j + 1);
			NiNode* v88 = 0;
			v88 = static_cast<NiNode*>(BSUtilities::GetObjectByName(pActiveDeckGeometry,
				static_cast<const char*>(blockName)));
			if (v88->GetProperty(3)) {
				NiProperty* v2 = v88->GetProperty(3);
				int32_t v53 = false;
				if (static_cast<NiShadeProperty*>(v2)->iShaderPropertyType >= 8) {
					NiProperty* v3 = v88->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v3)->iShaderPropertyType <= 12) {
						v53 = true;
					}
				}
				v54 = v53;
			} else {
				v54 = 0;
			}
			if (v54) {
				v52 = static_cast<BSShaderPPLightingProperty*>(v88->GetProperty(3));
			} else {
				v52 = 0;
			}
			deckFaceShaders[j] = v52;
			v88 = static_cast<NiNode*>(BSUtilities::GetObjectByName(pActiveDeckGeometry,
				static_cast<const char*>(apDest)));
			if (v88->GetProperty(3)) {
				NiProperty* v4 = v88->GetProperty(3);
				int32_t v50 = false;
				if (static_cast<NiShadeProperty*>(v4)->iShaderPropertyType >= 8) {
					NiProperty* v5 = v88->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v5)->iShaderPropertyType <= 12) {
						v50 = true;
					}
				}
				v51 = v50;
			} else {
				v51 = 0;
			}
			if (v51) {
				v49 = static_cast<BSShaderPPLightingProperty*>(v88->GetProperty(3));
			} else {
				v49 = 0;
			}
			deckFaceShaders[j + 24] = v49;
			v88 = static_cast<NiNode*>(BSUtilities::GetObjectByName(pInactiveDeckGeometry,
				static_cast<const char*>(apName)));
			if (v88->GetProperty(3)) {
				NiProperty* v6 = v88->GetProperty(3);
				int32_t v47 = false;
				if (static_cast<NiShadeProperty*>(v6)->iShaderPropertyType >= 8) {
					NiProperty* v7 = v88->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v7)->iShaderPropertyType <= 12) {
						v47 = true;
					}
				}
				v48 = v47;
			} else {
				v48 = 0;
			}
			if (v48) {
				v46 = static_cast<BSShaderPPLightingProperty*>(v88->GetProperty(3));
			} else {
				v46 = 0;
			}
			deckFaceShaders[j + 48] = v46;
			v88 = static_cast<NiNode*>(BSUtilities::GetObjectByName(pInactiveDeckGeometry,
				static_cast<const char*>(apString1)));
			if (v88->GetProperty(3)) {
				NiProperty* v8 = v88->GetProperty(3);
				int32_t v44 = false;
				if (static_cast<NiShadeProperty*>(v8)->iShaderPropertyType >= 8) {
					NiProperty* v9 = v88->GetProperty(3);
					if (static_cast<NiShadeProperty*>(v9)->iShaderPropertyType <= 12) {
						v44 = true;
					}
				}
				v45 = v44;
			} else {
				v45 = 0;
			}
			if (v45) {
				v43 = static_cast<BSShaderPPLightingProperty*>(v88->GetProperty(3));
			} else {
				v43 = 0;
			}
			deckFaceShaders[j + 72] = v43;
		}
	}
	SwapDeckBuildingTextures(12, 0);
	eActiveMenu = CaravanMenu::CA_DECK_MENU;
	eGameState = CaravanMenu::CA_CAMERA_TO_DECK_ANIM;
	pTiles[9]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[9]->Set(CaravanMenu::iUT_Value, iTotalCards);
	pTiles[8]->Poke(CaravanMenu::iUT_Value, 1.0f);
	uint32_t fEaseOutTime = pPlayer->GetCaravanCards2()->ItemsInList();
	pTiles[8]->Set(CaravanMenu::iUT_Value, fEaseOutTime);
	const char* fEaseOutTimea = sCaravanDeckText.String();
	const char* v18 = sRSMRandomize.String();
	BSsprintf(Src, 260, "%s %s", v18, fEaseOutTimea);
	pTiles[10]->Set(4036, Src, 1);
	float afValue = pTiles[10]->GetFloat(4001) - 150.0;
	pTiles[10]->Set(4001, afValue, 1);
	pTiles[10]->AddNeedsUpdate(1);
	if (!InterfaceManager::Has360Controller()) {
		int32_t v19 = Tile::TextToTrait("_PCButtonText");
		pTiles[10]->Set(v19, "S)", 1);
	}
	float v29 = pTiles[11]->GetFloat(4001) - 150.0;
	pTiles[11]->Set(4001, v29, 1);
	pTiles[11]->AddNeedsUpdate(1);
	pTiles[0]->Set(4003, 0);
	pTiles[1]->Set(4003, 1);
	pTiles[2]->Set(4003, 0);
	pTiles[3]->Set(4003, 0);
	const char* v20 = sCaravanDeckText.String();
	pTiles[42]->Set(4036, v20, 1);
	const char* v21 = sTotalCardsText.String();
	pTiles[43]->Set(4036, v21, 1);
	const char* v22 = sAllCardsText.String();
	pTiles[11]->Set(4036, v22, 1);
	const char* v23 = sNavigateText.String();
	pTiles[39]->Set(4036, v23, 1);
	const char* v24 = sAddText.String();
	pTiles[12]->Set(4036, v24, 1);
	const char* v25 = sRemoveText.String();
	pTiles[13]->Set(4036, v25, 1);
	const char* v26 = sPlayCaravanText.String();
	pTiles[14]->Set(4036, v26, 1);
	pTiles[39]->Set(4003, 0);
	pTiles[39]->AddNeedsUpdate(4);
	UpdateCaravanFlags();
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::SwapDeckBuildingTextures(int32_t aiTarget, bool abFastScrolling) {
	// The exe's buffers live in the function's frame, so each keeps its text from the loop's previous passes:
	// the language-swapped path ([ebp-0x11c]), the source path ([ebp-0x22c]) and one name buffer per branch.
	char acPath[268];
	char apInPath[268];
	char apString1[28];
	char apName[28];
	char apDest[28];
	char blockName[28];
	for (int32_t i = 0; i < 24; ++i) {
		if (deckFaceShaders[i]) {
			if (deckFaceShaders[i + 24]) {
				if (deckFaceShaders[i + 48]) {
					if (deckFaceShaders[i + 72]) {
						uint32_t auiIndex = aiTarget - (11 - i);
						// The exe tests the difference's sign (js), then compares it unsigned (jb).
						if (static_cast<int32_t>(auiIndex) >= 0 && auiIndex < iTotalCards) {
							NiAVObject* pFace_2;
							if (i >= 9) {
								BSsprintf(apString1, 25, "Deck_%i:%i", i + 1, i);
								pFace_2 = BSUtilities::GetObjectByName(pActiveDeckGeometry,
									static_cast<const char*>(apString1));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apString1, 25, "Deck_%i_Back:0", i + 1);
								pFace_2 = BSUtilities::GetObjectByName(pActiveDeckGeometry,
									static_cast<const char*>(apString1));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apString1, 25, "Available_%i:%i", i + 1, i);
								pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apString1));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apString1, 25, "Available_%i_Back:0", i + 1);
								pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apString1));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								if (i == 12 || i == 11) {
									BSsprintf(apString1, 25, "Deck_%i_Shadow:0", i + 1);
									pFace_2 = BSUtilities::GetObjectByName(pActiveDeckGeometry,
										static_cast<const char*>(apString1));
									NiPoint3 v39 = pFace_2->GetTranslate();
									pFace_2->SetTranslate(v39.x, v39.y, 0.0f);
									BSsprintf(apString1, 25, "Available_%i_Shadow:0", i + 1);
									pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
										static_cast<const char*>(apString1));
									v39 = pFace_2->GetTranslate();
									pFace_2->SetTranslate(v39.x, v39.y, 0.0f);
								} else {
									BSsprintf(apString1, 25, "Deck_%i_Shadow:0", i + 1);
									pFace_2 = BSUtilities::GetObjectByName(pActiveDeckGeometry,
										static_cast<const char*>(apString1));
									pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
									BSsprintf(apString1, 25, "Available_%i_Shadow:0", i + 1);
									pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
										static_cast<const char*>(apString1));
									pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								}
							} else {
								BSsprintf(apName, 25, "Deck_0%i:%i", i + 1, i);
								pFace_2 =
									BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(apName));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apName, 25, "Deck_0%i_Back:0", i + 1);
								pFace_2 =
									BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(apName));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apName, 25, "Deck_0%i_Shadow:0", i + 1);
								pFace_2 =
									BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(apName));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apName, 25, "Available_0%i:%i", i + 1, i);
								pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apName));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apName, 25, "Available_0%i_Back:0", i + 1);
								pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apName));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
								BSsprintf(apName, 25, "Available_0%i_Shadow:0", i + 1);
								pFace_2 = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apName));
								pFace_2->SetTranslate(0.0f, 0.0f, 0.0f);
							}
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, v33, nullptr, 0.0f, 0, 0);
							pInactiveDeckGeometry->Update(v33);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, v32, nullptr, 0.0f, 0, 0);
							pActiveDeckGeometry->Update(v32);
							// The exe stores the getter's result to a slot nothing reads.
							[[maybe_unused]] float DataFogFarPlane = deckFaceShaders[i]->fAlpha;
							if (pDeckArray.operator[](auiIndex)->bIsActive) {
								FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr,
									&NiPointer<NiSourceTexture>::Destruct, static_cast<NiSourceTexture*>(0));
								BSsprintf(apInPath, 260, "%s%s", "Textures\\",
									(&pDeckArray.operator[](auiIndex)->faceTexture)->GetTextureName());
								SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
								TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
								if (textptr.m_pObject) {
									NiSourceTexture* a4n = textptr.m_pObject;
									deckFaceShaders[i]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4n);
								}
								BSsprintf(apInPath, 260, "%s%s", "Textures\\",
									(&pDeckArray.operator[](auiIndex)->backTexture)->GetTextureName());
								SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
								TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
								if (textptr.m_pObject) {
									NiSourceTexture* a4o = textptr.m_pObject;
									deckFaceShaders[i + 24]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4o);
								}
								BSsprintf(apInPath, 260, "%s%s", "Textures\\",
									(&pDeckArray.operator[](auiIndex)->backTexture)->GetTextureName());
								SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
								TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
								if (textptr.m_pObject) {
									NiSourceTexture* a4p = textptr.m_pObject;
									deckFaceShaders[i + 48]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4p);
								}
								BSsprintf(apInPath, 260, "%s%s", "Textures\\",
									(&pDeckArray.operator[](auiIndex)->faceTexture)->GetTextureName());
								SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
								TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
								if (textptr.m_pObject) {
									NiSourceTexture* a4q = textptr.m_pObject;
									deckFaceShaders[i + 72]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4q);
								}
							} else {
								// The exe tests the address of the card's faceTexture, which is never null, after
								// the call.
								pDeckArray.operator[](auiIndex);
								{
									FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr_2,
										&NiPointer<NiSourceTexture>::Destruct, static_cast<NiSourceTexture*>(0));
									BSsprintf(apInPath, 260, "%s%s", "Textures\\",
										(&pDeckArray.operator[](auiIndex)->faceTexture)->GetTextureName());
									SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
									TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
									if (textptr_2.m_pObject) {
										NiSourceTexture* a4r = textptr_2.m_pObject;
										deckFaceShaders[i + 48]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4r);
									}
									BSsprintf(apInPath, 260, "%s%s", "Textures\\",
										(&pDeckArray.operator[](auiIndex)->backTexture)->GetTextureName());
									SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
									TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
									if (textptr_2.m_pObject) {
										NiSourceTexture* a4s = textptr_2.m_pObject;
										deckFaceShaders[i + 72]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4s);
									}
									BSsprintf(apInPath, 260, "%s%s", "Textures\\",
										(&pDeckArray.operator[](auiIndex)->backTexture)->GetTextureName());
									SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
									TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
									if (textptr_2.m_pObject) {
										NiSourceTexture* a4t = textptr_2.m_pObject;
										deckFaceShaders[i]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4t);
									}
									BSsprintf(apInPath, 260, "%s%s", "Textures\\",
										(&pDeckArray.operator[](auiIndex)->faceTexture)->GetTextureName());
									SwapPlatformLanguageTexturePath(apInPath, acPath, 260u);
									TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
									if (textptr_2.m_pObject) {
										NiSourceTexture* a4u = textptr_2.m_pObject;
										deckFaceShaders[i + 24]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4u);
									}
								}
							}
						} else {
							NiAVObject* pFace;
							// The exe keeps the z offset as an int and converts it with fild at each use.
							int32_t iZ = -50;
							if (abFastScrolling) {
								iZ = 0;
							}
							if (i >= 9) {
								BSsprintf(apDest, 25, "Deck_%i:%i", i + 1, i);
								pFace =
									BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(apDest));
								float a4f = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4f);
								BSsprintf(apDest, 25, "Deck_%i_Back:0", i + 1);
								pFace =
									BSUtilities::GetObjectByName(pActiveDeckGeometry, static_cast<const char*>(apDest));
								float a4g = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4g);
								BSsprintf(apDest, 25, "Available_%i:%i", i + 1, i);
								pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apDest));
								float a4h = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4h);
								BSsprintf(apDest, 25, "Available_%i_Back:0", i + 1);
								pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(apDest));
								float a4i = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4i);
								if (i == 12 || i == 11) {
									BSsprintf(apDest, 25, "Deck_%i_Shadow:0", i + 1);
									pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry,
										static_cast<const char*>(apDest));
									NiPoint3 v44 = pFace->GetTranslate();
									float a4l = static_cast<float>(iZ);
									pFace->SetTranslate(v44.x, v44.y, a4l);
									BSsprintf(apDest, 25, "Available_%i_Shadow:0", i + 1);
									pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
										static_cast<const char*>(apDest));
									v44 = pFace->GetTranslate();
									float a4m = static_cast<float>(iZ);
									pFace->SetTranslate(v44.x, v44.y, a4m);
								} else {
									BSsprintf(apDest, 25, "Deck_%i_Shadow:0", i + 1);
									pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry,
										static_cast<const char*>(apDest));
									float a4j = static_cast<float>(iZ);
									pFace->SetTranslate(0.0f, 0.0f, a4j);
									BSsprintf(apDest, 25, "Available_%i_Shadow:0", i + 1);
									pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
										static_cast<const char*>(apDest));
									float a4k = static_cast<float>(iZ);
									pFace->SetTranslate(0.0f, 0.0f, a4k);
								}
							} else {
								BSsprintf(blockName, 25, "Deck_0%i:%i", i + 1, i);
								pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry,
									static_cast<const char*>(blockName));
								float a4 = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4);
								BSsprintf(blockName, 25, "Deck_0%i_Back:0", i + 1);
								pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry,
									static_cast<const char*>(blockName));
								float a4a = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4a);
								BSsprintf(blockName, 25, "Available_0%i:%i", i + 1, i);
								pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(blockName));
								float a4b = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4b);
								BSsprintf(blockName, 25, "Available_0%i_Back:0", i + 1);
								pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(blockName));
								float a4c = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4c);
								BSsprintf(blockName, 25, "Deck_0%i_Shadow:0", i + 1);
								pFace = BSUtilities::GetObjectByName(pActiveDeckGeometry,
									static_cast<const char*>(blockName));
								float a4d = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4d);
								BSsprintf(blockName, 25, "Available_0%i_Shadow:0", i + 1);
								pFace = BSUtilities::GetObjectByName(pInactiveDeckGeometry,
									static_cast<const char*>(blockName));
								float a4e = static_cast<float>(iZ);
								pFace->SetTranslate(0.0f, 0.0f, a4e);
							}
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, apMemory, nullptr, 0.0f, 0, 0);
							pInactiveDeckGeometry->Update(apMemory);
							FNVD_CONSTRUCTED_LOCAL(NiUpdateData, v34, nullptr, 0.0f, 0, 0);
							pActiveDeckGeometry->Update(v34);
						}
					}
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::UpdateDealCardTexture(bool abPlayer) {
	int32_t v7;
	BSShaderPPLightingProperty* faceshader_3;
	int32_t v10;
	BSShaderPPLightingProperty* faceshader;
	int32_t v14;
	NiAVObject* pHand;
	char acPath[264];
	char blockName[264];
	if (abPlayer) {
		pHand = pHandGeometry[0];
	} else {
		pHand = pHandGeometry[1];
	}
	if (abPlayer) {
		BSstrcpy(blockName, 260, "Player-Deck_06:5");
	} else {
		BSstrcpy(blockName, 260, "Opponent-Deck_06:5");
	}
	NiAVObject* face = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
	if (face->GetProperty(3)) {
		// The shader type is read through the shared one-field getter 0x441110 (a trivial accessor).
		BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
		int32_t v13 = false;
		if (pProperty->iShaderPropertyType >= 8) {
			BSShaderPPLightingProperty* v3 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
			if (v3->iShaderPropertyType <= 12) {
				v13 = true;
			}
		}
		v14 = v13;
	} else {
		v14 = 0;
	}
	if (v14) {
		faceshader = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
	} else {
		faceshader = 0;
	}
	BSShaderPPLightingProperty* faceshader_2 = faceshader;
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr, &NiPointer<NiSourceTexture>::Destruct,
		static_cast<NiSourceTexture*>(0));
	if (faceshader) {
		uint32_t FormFlags;
		if (abPlayer) {
			FormFlags = pPlayerHand.iSize;
		} else {
			FormFlags = pNPCHand.iSize;
		}
		if (FormFlags > 4) {
			const char* TextureName;
			if (abPlayer) {
				TextureName = (&pPlayerHand.operator[](4u)->faceTexture)->GetTextureName();
			} else {
				TextureName = (&pNPCHand.operator[](4u)->backTexture)->GetTextureName();
			}
			BSsprintf(blockName, 260, "%s%s", "Textures\\", TextureName);
			SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
			TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
			if (textptr.m_pObject) {
				faceshader_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, textptr.m_pObject);
			}
		}
	}
	if (abPlayer) {
		BSstrcpy(blockName, 260, "Player-Deck_Back:0");
	} else {
		BSstrcpy(blockName, 260, "Opponent-Deck_Back:0");
	}
	face = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
	if (face->GetProperty(3)) {
		BSShaderPPLightingProperty* v5 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
		int32_t v9 = false;
		if (v5->iShaderPropertyType >= 8) {
			BSShaderPPLightingProperty* v6 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
			if (v6->iShaderPropertyType <= 12) {
				v9 = true;
			}
		}
		v10 = v9;
	} else {
		v10 = 0;
	}
	if (v10) {
		faceshader_3 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
	} else {
		faceshader_3 = 0;
	}
	faceshader_2 = faceshader_3;
	textptr.operator=(static_cast<NiSourceTexture*>(0));
	if (faceshader_3 && (!abPlayer ? v7 = pNPCDeck.iSize != 0 : v7 = pPlayerDeck.iSize != 0, v7)) {
		if (abPlayer) {
			BSsprintf(blockName, 260, "Textures\\%s",
				(&pPlayerDeck.operator[](iNextPlayerCard)->backTexture)->GetTextureName());
		} else {
			BSsprintf(blockName, 260, "Textures\\%s",
				(&pNPCDeck.operator[](iNextNPCCard)->backTexture)->GetTextureName());
		}
		SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
		TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
		if (textptr.m_pObject) {
			faceshader_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, textptr.m_pObject);
		}
	} else {
		face->SetAppCulled(1);
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, updatedata, nullptr, 0.0f, 0, 0);
		face->Update(updatedata);
	}
}

void CaravanMenu::SelectDeckCard(bool abUnselectOnly) {
	if (pSelectedObject) {
		if ((&pSelectedObject->GetTranslate())->z > 0.0) {
			pSelectedObject->SetTranslate(0.0f, (&pSelectedObject->GetTranslate())->y, 0.0f);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, kUpdate1, nullptr, 0.0f, 0, 0);
			pSelectedObject->Update(kUpdate1);
			pSelectedShadow->SetTranslate(0.0f, (&pSelectedObject->GetTranslate())->y, 0.0f);
			FNVD_CONSTRUCTED_LOCAL(NiUpdateData, kUpdate2, nullptr, 0.0f, 0, 0);
			pSelectedShadow->Update(kUpdate2);
		}
	}
	if (!abUnselectOnly) {
		char blockName[32];
		BSsprintf(blockName, 30, "Player-Deck_0%i:%i", iSelectedDeckCard + 1, iSelectedDeckCard);
		pSelectedObject = BSUtilities::GetObjectByName(pHandGeometry[0], static_cast<const char*>(blockName));
		BSsprintf(blockName, 30, "Player-Deck_0%i_Shadow:0", iSelectedDeckCard + 1);
		pSelectedShadow = BSUtilities::GetObjectByName(pHandGeometry[0], static_cast<const char*>(blockName));
		pSelectedCard = pPlayerHand.operator[](iSelectedDeckCard);
		pSelectedObject->SetTranslate(0.0f, (&pSelectedObject->GetTranslate())->y, 1.0f);
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, kUpdate3, nullptr, 0.0f, 0, 0);
		pSelectedObject->Update(kUpdate3);
		pSelectedShadow->SetTranslate(0.0f, (&pSelectedObject->GetTranslate())->y, 1.0f);
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, kUpdate4, nullptr, 0.0f, 0, 0);
		pSelectedShadow->Update(kUpdate4);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::SelectTrackCard(int32_t aiTrack, int32_t aiRow) {
	NiUpdateData v18;
	NiMatrix3 matrot_90_4;
	NiUpdateData v20;
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	NiUpdateData v22;
	NiMatrix3 matrot_90_2;
	NiUpdateData apMemory;
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	char texturename[260];
	char blockName[268];
	NiMatrix3 cardrot;
	bool bIsValidPlacement = IsValidCardPlacement(aiTrack, aiRow, iSelectedDeckCard, 0);
	int32_t iColumn = pCardTracks[aiTrack][aiRow].iSize;
	if (iColumn > 3) {
		iColumn = 3;
	}
	BSsprintf(blockName, 260, "Select%i_0%i:0", aiRow + 1, iColumn + 1);
	NiAVObject* cursorPoint = BSUtilities::GetObjectByName(pTrackGrids[aiTrack], static_cast<const char*>(blockName));
	if (cursorPoint) {
		BSShaderPPLightingProperty* faceshader_3;
		int32_t v14;
		NiPoint3 cardloc;
		NiAVObject* cursorface;
		if (bIsValidPlacement) {
			NiMatrix3 matrot_90;
			BSAudio* v3 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "GAMECaravanToggleTrack", 289u);
			SoundHandleByName->Play(0);
			cursorface = pCursors[0];
			pCursors[1]->SetTranslate(0.0f, 0.0f, -50.0f);
			NiUpdateData* updated = FNVD_RECONSTRUCT(NiUpdateData)(&apMemory, 0.0f, 0, 0);
			pCursors[1]->Update(*updated);
			matrot_90.MakeZRotation(1.5707964f);
			cardrot = (*&cursorPoint->m_kLocal).m_Rotate;
			cardrot = *::new (&matrot_90_2) NiMatrix3(cardrot * matrot_90);
			cardloc = (&cursorPoint->GetWorldBound())->m_kCenter;
			cursorface->SetRotate(cardrot);
			cursorface->SetTranslate(cardloc);
			NiUpdateData* v5 = FNVD_RECONSTRUCT(NiUpdateData)(&v22, 0.0f, 0, 0);
			cursorface->Update(*v5);
			cursorface = BSUtilities::GetObjectByName(cursorface, "Add:3");
		} else {
			NiMatrix3 matrot_90_3;
			BSAudio* v6 = &BSAudio::QInstance();
			BSSoundHandle* v15 = v6->GetSoundHandleByName(&arHandle, "GAMECaravanInvalidStackLocation", 289u);
			v15->Play(0);
			cursorface = pCursors[1];
			pCursors[0]->SetTranslate(0.0f, 0.0f, -50.0f);
			NiUpdateData* v7 = FNVD_RECONSTRUCT(NiUpdateData)(&v20, 0.0f, 0, 0);
			pCursors[0]->Update(*v7);
			matrot_90_3.MakeZRotation(1.5707964f);
			cardrot = (*&cursorPoint->m_kLocal).m_Rotate;
			cardrot = *::new (&matrot_90_4) NiMatrix3(cardrot * matrot_90_3);
			cardloc = (&cursorPoint->GetWorldBound())->m_kCenter;
			cursorface->SetRotate(cardrot);
			cursorface->SetTranslate(cardloc);
			NiUpdateData* v8 = FNVD_RECONSTRUCT(NiUpdateData)(&v18, 0.0f, 0, 0);
			cursorface->Update(*v8);
			cursorface = BSUtilities::GetObjectByName(cursorface, "Remove:3");
		}
		if (cursorface->GetProperty(3)) {
			BSShaderPPLightingProperty* faceshader =
				static_cast<BSShaderPPLightingProperty*>(cursorface->GetProperty(3));
			int32_t v13 = false;
			if (faceshader->iShaderPropertyType >= 8) {
				BSShaderPPLightingProperty* faceshader_2 =
					static_cast<BSShaderPPLightingProperty*>(cursorface->GetProperty(3));
				if (faceshader_2->iShaderPropertyType <= 12) {
					v13 = true;
				}
			}
			v14 = v13;
		} else {
			v14 = 0;
		}
		if (v14) {
			faceshader_3 = static_cast<BSShaderPPLightingProperty*>(cursorface->GetProperty(3));
		} else {
			faceshader_3 = 0;
		}
		FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr, &NiPointer<NiSourceTexture>::Destruct,
			static_cast<NiSourceTexture*>(0));
		if (faceshader_3) {
			BSsprintf(blockName, 260, "%s%s", "Textures\\", (&pSelectedCard->faceTexture)->GetTextureName());
			SwapPlatformLanguageTexturePath(blockName, texturename, 260u);
			TES::pInstance->CreateTextureImage(texturename, textptr, 0, 0);
			if (textptr.m_pObject) {
				NiSourceTexture* a4 = textptr.m_pObject;
				faceshader_3->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, a4);
			}
		}
	}
}

void CaravanMenu::SelectWholeTrack(int32_t a1) {
	NiUpdateData apMemory;
	for (int32_t i = 0; i < 7; ++i) {
		pTrackGeometry[a1][i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), 3.0f);
		NiUpdateData* a4 = FNVD_RECONSTRUCT(NiUpdateData)(&apMemory, 0.0, 0, 0);
		pTrackGeometry[a1][i]->Update(*a4);
	}
}

void CaravanMenu::DeselectWholeTrack(int32_t a1) {
	NiUpdateData apMemory;
	for (int32_t i = 0; i < 7; ++i) {
		pTrackGeometry[a1][i]->SetTranslate(static_cast<float>(0.0), static_cast<float>(0.0), static_cast<float>(0.0));
		NiUpdateData* a4 = FNVD_RECONSTRUCT(NiUpdateData)(&apMemory, 0.0, 0, 0);
		pTrackGeometry[a1][i]->Update(*a4);
	}
}

void CaravanMenu::HandleTrackSelection(int32_t aiXShift, int32_t aiYShift) {
	int32_t iSelectedTrack = this->iSelectedTrack;
	int32_t iSelectedRow = this->iSelectedRow;
	bool v12 = pPlayerHand.operator[](iSelectedDeckCard)->data.iFaceValue <= 10;
	if (aiXShift) {
		if (iSelectedTrack >= 3) {
			this->iSelectedTrack -= aiXShift;
			int32_t v4 = BSMAX(3, this->iSelectedTrack);
			this->iSelectedTrack = BSMIN(v4, 5);
		} else {
			this->iSelectedTrack += aiXShift;
			int32_t v3 = BSMAX(0, this->iSelectedTrack);
			this->iSelectedTrack = BSMIN(v3, 2);
		}
	}
	if (aiYShift) {
		if (iSelectedRow == this->iSelectedRow) {
			if (this->iSelectedTrack >= 3) {
				this->iSelectedRow -= aiYShift;
				int32_t v6 = this->iSelectedRow;
			} else {
				this->iSelectedRow += aiYShift;
				int32_t v5 = this->iSelectedRow;
			}
			int32_t v10 = this->iSelectedRow;
			if (v10 == -1) {
				if (this->iSelectedTrack >= 3) {
					this->iSelectedTrack = 5 - iSelectedTrack;
					int32_t v8 = this->iSelectedTrack;
				} else {
					this->iSelectedTrack = 5 - iSelectedTrack;
					int32_t v7 = this->iSelectedTrack;
				}
				int32_t v9 = this->iSelectedTrack;
				this->iSelectedRow = 0;
			} else {
				if (this->iSelectedRow == 7) {
					this->iSelectedRow = 6;
				}
			}
		}
	}
	if (aiXShift == aiYShift && !aiXShift) {
		this->iSelectedTrack = 0;
		this->iSelectedRow = 0;
		iSelectedColumn = 0;
	}
	iSelectedColumn = pCardTracks[this->iSelectedTrack][this->iSelectedRow].iSize;
	SelectTrackCard(this->iSelectedTrack, this->iSelectedRow);
	if (IsValidCardPlacement(this->iSelectedTrack, this->iSelectedRow, iSelectedDeckCard, 0)) {
		SetFlag(CaravanMenu::CAF_DISABLE_A, 0);
	} else {
		SetFlag(CaravanMenu::CAF_DISABLE_A, 1);
	}
	UpdateTileAlpha(CaravanMenu::TI_NO_TILE);
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::LoadNewBill(int32_t a1) {
	FNVD_INLINED_LOCAL(NiFixedString, v7, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v8, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiControllerSequence* a2;
	int32_t arg0 = BSMIN(a1, 2);
	NiAVObject* a1a = static_cast<NiAVObject*>(pBillGeometry[arg0]->Clone());
	int32_t v16 = RandomInt(0, 3);
	NiControllerManager* Controller =
		static_cast<NiControllerManager*>(a1a->GetController(&NiControllerManager::ms_RTTI));
	if (v16) {
		if (v16 == 1) {
			if (Controller) {
				NiFixedString* v4 = ::new (static_cast<void*>(&v8)) NiFixedString("Bottom_Place");
				a2 = Controller->GetSequenceByName(*v4);
				v8.~NiFixedString();
				Controller->ActivateSequence(a2, 0, 0, 1.0f, 0.0f, 0);
				NiTimeController::StartAnimations(a1a);
			}
		} else {
			if (v16 == 2) {
				if (Controller) {
					NiFixedString* v3 = ::new (static_cast<void*>(&v7)) NiFixedString("Alt_Place");
					a2 = Controller->GetSequenceByName(*v3);
					v7.~NiFixedString();
					Controller->ActivateSequence(a2, 0, 0, 1.0f, 0.0f, 0);
					NiTimeController::StartAnimations(a1a);
				}
			}
		}
	} else {
		if (Controller) {
			NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Top_Place");
			a2 = Controller->GetSequenceByName(*kName);
			apString.~NiFixedString();
			Controller->ActivateSequence(a2, 0, 0, 1.0f, 0.0f, 0);
			NiTimeController::StartAnimations(a1a);
		}
	}
	pListOfBills.Add(a1a);
	pListOfBillAnimations->AddHead(a2);
	NiNode* v2 = CaravanMenu::spModel.m_pObject;
	v2->AttachChild(a1a, 1);
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::LoadNewCoin(int32_t a1) {
	FNVD_INLINED_LOCAL(NiFixedString, v6, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiControllerSequence* a2;
	int32_t arg0 = BSMIN(a1, 2);
	NiAVObject* a1a = static_cast<NiAVObject*>(pCoinGeometry[arg0]->Clone());
	int32_t v13 = RandomInt(0, 2);
	NiControllerManager* Controller =
		static_cast<NiControllerManager*>(a1a->GetController(&NiControllerManager::ms_RTTI));
	if (v13) {
		if (v13 == 1) {
			if (Controller) {
				NiFixedString* v3 = ::new (static_cast<void*>(&v6)) NiFixedString("Bottom_Place");
				a2 = Controller->GetSequenceByName(*v3);
				v6.~NiFixedString();
				Controller->ActivateSequence(a2, 0, 0, 1.0f, 0.0f, 0);
				NiTimeController::StartAnimations(a1a);
			}
		}
	} else {
		if (Controller) {
			NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Top_Place");
			a2 = Controller->GetSequenceByName(*kName);
			apString.~NiFixedString();
			Controller->ActivateSequence(a2, 0, 0, 1.0f, 0.0f, 0);
			NiTimeController::StartAnimations(a1a);
		}
	}
	pListOfCoins.Add(a1a);
	pListOfCoinAnimations->AddHead(a2);
	NiNode* v2 = CaravanMenu::spModel.m_pObject;
	v2->AttachChild(a1a, 1);
}

void CaravanMenu::ZeroFlags() {
	iFlags = (uint32_t)0;
}

void CaravanMenu::DeselectTrackCard() {
	NiUpdateData v4;
	NiUpdateData v5;
	pCursors[0]->SetTranslate((float)0.0, (float)0.0, (-100.0f));
	pCursors[1]->SetTranslate((float)0.0, (float)0.0, (-100.0f));
	NiUpdateData* updated = FNVD_RECONSTRUCT(NiUpdateData)(&v5, 0.0, 0, 0);
	pCursors[0]->Update(*updated);
	NiUpdateData* v2 = FNVD_RECONSTRUCT(NiUpdateData)(&v4, 0.0, 0, 0);
	pCursors[1]->Update(*v2);
}

int32_t CaravanMenu::CalculateWinningState(int32_t aivalue) {
	if (aivalue >= 21) {
		return (int32_t)((uint8_t)(aivalue > 26));
	} else {
		return (-1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
uint32_t CaravanMenu::LoadNifs2() {
	NiNode* v11;
	FNVD_INLINED_LOCAL(NiFixedString, a3, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v14, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	pTableGeometry = (NiAVObject*)pModelLoader->LoadFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Table.NIF",
		LOD_MULT_NONE, 1, 0, 0, 0);
	void* apMemory = NiMemObject::operator new(sizeof(NiNode));
	if (apMemory) {
		v11 = ::new (apMemory) NiNode(0);
	} else {
		v11 = 0;
	}
	NiRefObject* v17 = (NiRefObject*)v11;
	CaravanMenu::spModel.operator=(v11);
	NiNode* v10 = CaravanMenu::spModel.m_pObject;
	v10->AttachChild(pTableGeometry, 1);
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, arData, nullptr, 0.0f, 0, 0);
	CaravanMenu::spModel.m_pObject->Update(arData);
	NiControllerManager* Controller =
		static_cast<NiControllerManager*>(pTableGeometry->GetController(&NiControllerManager::ms_RTTI));
	if (Controller) {
		pCameraAnimManager = Controller;
		NiFixedString* a2 = ::new (static_cast<void*>(&apString)) NiFixedString("Bet_to_Deck");
		pCameraAnimations[0] = pCameraAnimManager->GetSequenceByName(*a2);
		apString.~NiFixedString();
		NiFixedString* kName = ::new (static_cast<void*>(&v14)) NiFixedString("Deck_to_Play");
		pCameraAnimations[1] = pCameraAnimManager->GetSequenceByName(*kName);
		v14.~NiFixedString();
		NiFixedString* v7 = ::new (static_cast<void*>(&a3)) NiFixedString("Play_to_Bet");
		pCameraAnimations[2] = pCameraAnimManager->GetSequenceByName(*v7);
		a3.~NiFixedString();
		pCameraAnimManager->DeactivateAll(0.0f);
		NiTimeController::StartAnimations(pTableGeometry);
	}
	NiNode* a1 = static_cast<NiNode*>(pTableGeometry);
	CaravanMenu::sp3DCamera.operator=(static_cast<NiCamera*>(BSUtilities::GetObjectByName(a1, "object0")));
	if (!CaravanMenu::sp3DCamera.m_pObject) {
		CaravanMenu::sp3DCamera.operator=(Create3DCamera());
	}
	ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
	ShadowSceneNode_->AttachChild(CaravanMenu::spModel.m_pObject, 1);
	const NiBound* WorldBound = &CaravanMenu::spModel.m_pObject->GetWorldBound();
	float v20 = WorldBound->m_fRadius * 2.5;
	NiNode* afTime = CaravanMenu::spModel.m_pObject;
	ShadowSceneNode* v3 = BSShaderManager::GetShadowSceneNode(1);
	return v3->AddAllLightsFromNode_Recurse(afTime, v20);
}

// The exe's cleanup-only C++ EH frame is left out.
NiCamera* CaravanMenu::Create3DCamera() {
	int32_t fHeight;
	NiCamera* v10;
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiPoint3 v12;
	NiPoint3 dst;
	NiRect<float> a2;
	void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
	if (apMemory) {
		v10 = ::new (apMemory) NiCamera;
	} else {
		v10 = 0;
	}
	::new (&a2) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
	v10->SetViewPort(a2);
	FNVD_CONSTRUCTED_LOCAL(NiFrustum, kFrustum, nullptr, 0);
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	kFrustum.m_fFar = WorldSceneGraph->GetFarDistance();
	kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
	float v8 = kFrustum.m_fFar / kFrustum.m_fNear;
	v10->SetMaxFarNearRatio(v8);
	if (BSShaderManager::bLetterbox) {
		fHeight = BSShaderManager::iLetterboxHeight;
	} else {
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
	kFrustum.m_bOrtho = 0;
	v10->SetViewFrustum(kFrustum);
	NiPoint3* z = ::new (&dst) NiPoint3(0.0f, 0.0f, 1.0f);
	NiPoint3* v3 = ::new (&v12) NiPoint3(0.0f, 0.0f, 1.0f);
	v10->LookAtWorldPoint(*v3, *z);
	v10->SetMinNearPlaneDist(1.0f);
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, arData, nullptr, 0.0f, 0, 0);
	v10->Update(arData);
	NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Surgery3DCamera");
	v10->SetName(*kName);
	apString.~NiFixedString();
	return v10;
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
	uint32_t uiJumpAmount;
	if (iTotalCards / 10 <= 10) {
		uiJumpAmount = 10;
	} else {
		uiJumpAmount = iTotalCards / 10;
	}
	iJumpAmount = uiJumpAmount;
	return 1;
}

void CaravanMenu::SetupCurrency() {
	char modelPath[264];
	BSsprintf(modelPath, 260, "%sCurrency-Bill_Grid.NIF", "Meshes\\Terminals\\NV_Caravan\\");
	pBillsGrid = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	BSsprintf(modelPath, 260, "%sCurrency-Coin_Grid.NIF", "Meshes\\Terminals\\NV_Caravan\\");
	pCoinsGrid = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	for (int32_t i = 0; i < 3; ++i) {
		BSsprintf(modelPath, 260, "%sCurrency-Bill_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
		pBillGeometry[i] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
		BSsprintf(modelPath, 260, "%sCurrency-Coin_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i + 1);
		pCoinGeometry[i] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	}
	eActiveMenu = CaravanMenu::CA_ANTE_MENU;
	eGameState = CaravanMenu::CA_IDLE;
	iBillIndexToAnim = 0;
	iCoinIndexToAnim = 0;
	pTiles[0]->Set(4003, 1);
	pTiles[1]->Set(4003, 0);
	pTiles[2]->Set(4003, 0);
	pTiles[3]->Set(4003, 0);
	const char* v1 = sCurrentAnteText.String();
	pTiles[40]->Set(4036, v1, 1);
	const char* v2 = sTotalFundsText.String();
	pTiles[41]->Set(4036, v2, 1);
	const char* v3 = sAutoMatchText.String();
	pTiles[37]->Set(4036, v3, 1);
	const char* v4 = sRaiseText.String();
	pTiles[36]->Set(4036, v4, 1);
	const char* v5 = sAcceptText.String();
	pTiles[7]->Set(4036, v5, 1);
	const char* v6 = sExit.String();
	pTiles[38]->Set(4036, v6, 1);
	PrepareBettingCurrency();
	iNPCAnte = static_cast<int32_t>(static_cast<double>(iNPCTotalFunds) * fBetPercentage);
	const char* FullName = pChallenger->GetFullName();
	if (BSstrlen(FullName) > 17) {
		const char* v8 = pChallenger->GetFullName();
		int32_t v18 = 15 * (BSstrlen(v8) - 17);
		Tile* th_s = Tile::GetTileByName(pTiles[4], "CBM_NPCBetValue");
		float v13 = th_s->GetFloat(4001) + static_cast<double>(v18);
		th_s->Set(4001, v13, 1);
	}
	Tile* ChildByName = pTiles[0]->GetChildByName("CBM_InfoRect");
	if (ChildByName) {
		float v12 = ChildByName->GetFloat(4017) + 150.0;
		ChildByName->Set(4017, v12, 1);
		ChildByName->AddNeedsUpdate(16);
	}
	const char* v9 = pChallenger->GetFullName();
	pTiles[4]->Set(4036, v9, 1);
	pTiles[4]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[4]->Set(CaravanMenu::iUT_Value, iNPCAnte);
	pTiles[5]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[5]->Set(CaravanMenu::iUT_Value, iPlayerAnte);
	pTiles[6]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[6]->Set(CaravanMenu::iUT_Value, iTotalFunds);
	UpdateCaravanFlags();
	UpdateTileAlpha(CaravanMenu::TI_ANTE_MENU_TILESET);
	for (int32_t a2 = 0; a2 < 9; ++a2) {
		intlistbills.Add(a2);
	}
	for (int32_t aItem = 0; aItem < 42; ++aItem) {
		intlistcoins.Add(aItem);
	}
	// The exe computes the NPC's bet twice (two spills) and uses the barter value straight from ST0.
	double dNPCBet = static_cast<double>(CaravanMenu::pMe->iNPCTotalFunds) * CaravanMenu::pMe->fBetPercentage;
	double dNPCBetScale = static_cast<double>(CaravanMenu::pMe->iNPCTotalFunds) * CaravanMenu::pMe->fBetPercentage;
	UpdateBettingUI(static_cast<int32_t>(pPlayer->GetClampedActorFloatValue(ActorValue::BARTER) / 100.0 * dNPCBetScale +
						dNPCBet),
		iNPCAnte);
}

void CaravanMenu::PrepareBettingCurrency() {
	iTotalFunds = 0;
	pDataHandler->CreateBarterContainer(pPlayer);
	InventoryChanges* pPlayerContainer = pDataHandler->GetBarterContainer();
	iTotalFunds = pPlayerContainer->GetGoldAmount();
	for (BSSimpleList<ItemChange*>* pNode = pPlayerContainer->pListofChanges; pNode && pNode->m_item;
		pNode = pNode->m_pkNext) {
		TESBoundObject* pObject = pNode->m_item->pContainerObj;
		if (pObject) {
			if (pObject->GetFormType() == CMNY_ID && pNode->m_item->iNumber > 0) {
				int32_t iCount = pNode->m_item->iNumber;
				iTotalFunds += iCount * TESValueForm::GetFormValue(pObject);
			}
		}
	}
	iNPCTotalFunds = 0;
	pDataHandler->CreateBarterContainer(pChallenger);
	InventoryChanges* pNPCContainer = pDataHandler->GetBarterContainer();
	iNPCTotalFunds = pNPCContainer->GetGoldAmount();
	for (BSSimpleList<ItemChange*>* pNode = pNPCContainer->pListofChanges; pNode && pNode->m_item;
		pNode = pNode->m_pkNext) {
		TESBoundObject* pObject = pNode->m_item->pContainerObj;
		if (pObject) {
			if (pObject->GetFormType() == CMNY_ID && pNode->m_item->iNumber > 0) {
				int32_t iCount = pNode->m_item->iNumber;
				iNPCTotalFunds += iCount * TESValueForm::GetFormValue(pObject);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::PrepareGameMenu() {
	NiUpdateData v81;
	FNVD_INLINED_LOCAL(NiFixedString, v82, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v83, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v84, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v85, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v86, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v87, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v88, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v89, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v90, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v91, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v92, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v93, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v94, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v95, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v96, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v97, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v98, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v99, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v100, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v101, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v102, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiUpdateData v104;
	NiUpdateData v105;
	NiUpdateData v106;
	NiUpdateData v107;
	NiUpdateData apMemory;
	NiTimeController* pTimeController;
	char modelPath[268];
	char Src[268];
	BSSimpleList<TESCaravanCard*>* activeList = pPlayer->GetCaravanCards2();
	BSSimpleList<TESCaravanCard*>* inactiveList = pPlayer->GetCaravanCards1();
	activeList->RemoveAll();
	inactiveList->RemoveAll();
	for (int32_t i = 0; i < pDeckArray.iSize; ++i) {
		if (pDeckArray.operator[](i)->bIsActive) {
			activeList->AddHead(pDeckArray.operator[](i));
			pPlayerDeck.Add(pDeckArray.operator[](i));
		} else {
			inactiveList->AddHead(pDeckArray.operator[](i));
		}
	}
	while (pDeckParam && pDeckParam->m_item) {
		pNPCDeck.Add(pDeckParam->m_item);
		pDeckParam = pDeckParam->m_pkNext;
	}
	for (int32_t i_2 = 0; i_2 < 8; ++i_2) {
		uint32_t FormFlags = pPlayerDeck.iSize;
		int32_t index = RandomInt(0, FormFlags);
		pPlayerHand.Add(pPlayerDeck.operator[](index));
		pPlayerDeck.RemoveFast(index, 1);
		uint32_t v2 = pNPCDeck.iSize;
		index = RandomInt(0, v2);
		pNPCHand.Add(pNPCDeck.operator[](index));
		pNPCDeck.RemoveFast(index, 1);
	}
	uint32_t v3 = pPlayerDeck.iSize;
	iNextPlayerCard = RandomInt(0, v3);
	uint32_t v4 = pNPCDeck.iSize;
	iNextNPCCard = RandomInt(0, v4);
	pDeckParam = 0;
	activeList = 0;
	inactiveList = 0;
	if (pDeckArray.iSize) {
		pDeckArray.Clear(1);
	}
	for (int32_t i_3 = 0; i_3 < 96; ++i_3) {
		deckFaceShaders[i_3] = 0;
	}
	pDeckArray.Clear(1);
	pInactiveDeckAnimManager = 0;
	pActiveDeckAnimManager = 0;
	for (int32_t i_4 = 0; i_4 < 6; ++i_4) {
		pInactiveDeckAnimations[i_4] = 0;
		pActiveDeckAnimations[i_4] = 0;
	}
	for (int32_t i_5 = 0; i_5 < 96; ++i_5) {
		deckFaceShaders[i_5] = 0;
	}
	NiNode* v79 = CaravanMenu::spModel.m_pObject;
	uint32_t ChildCount = CaravanMenu::spModel.m_pObject->GetChildCount();
	v79->DetachChildAt(ChildCount - 1);
	NiNode* v78 = CaravanMenu::spModel.m_pObject;
	uint32_t v6 = CaravanMenu::spModel.m_pObject->GetChildCount();
	v78->DetachChildAt(v6 - 1);
	pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Deck.NIF");
	pModelLoader->ReleaseFile("Meshes\\Terminals\\NV_Caravan\\NV_Caravan-Available.NIF");
	pInactiveDeckGeometry = 0;
	pActiveDeckGeometry = 0;
	for (int32_t i_6 = 0; i_6 < 3; ++i_6) {
		for (int32_t j = 0; j < 7; ++j) {
			BSsprintf(modelPath, 260, "%sPlayer_%i_0%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i_6 + 1, j + 1);
			pTrackGeometry[i_6][j] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
			NiNode* v77 = CaravanMenu::spModel.m_pObject;
			v77->AttachChild(pTrackGeometry[i_6][j], 1);
			BSsprintf(modelPath, 260, "%sOpponent_%i_0%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i_6 + 1, j + 1);
			pTrackGeometry[i_6 + 3][j] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
			NiNode* v76 = CaravanMenu::spModel.m_pObject;
			v76->AttachChild(pTrackGeometry[i_6 + 3][j], 1);
		}
	}
	for (int32_t i_7 = 0; i_7 < 3; ++i_7) {
		BSsprintf(modelPath, 260, "%sPlayerGrid_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i_7 + 1);
		pTrackGrids[i_7] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
		BSsprintf(modelPath, 260, "%sOppGrid_%i.NIF", "Meshes\\Terminals\\NV_Caravan\\", i_7 + 1);
		pTrackGrids[i_7 + 3] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	}
	BSsprintf(modelPath, 260, "%sSelect_Add.NIF", "Meshes\\Terminals\\NV_Caravan\\");
	pCursors[0] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	NiNode* v75 = CaravanMenu::spModel.m_pObject;
	v75->AttachChild(pCursors[0], 1);
	BSsprintf(modelPath, 260, "%sSelect_Remove.NIF", "Meshes\\Terminals\\NV_Caravan\\");
	pCursors[1] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	NiNode* v74 = CaravanMenu::spModel.m_pObject;
	v74->AttachChild(pCursors[1], 1);
	BSsprintf(modelPath, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Player_Deck.NIF");
	pHandGeometry[0] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	BSsprintf(modelPath, 260, "%s%s", "Meshes\\Terminals\\NV_Caravan\\", "Opponent_Deck.NIF");
	pHandGeometry[1] = (NiAVObject*)pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, 1, 0, 0, 0);
	NiNode* v73 = CaravanMenu::spModel.m_pObject;
	v73->AttachChild(pHandGeometry[0], 1);
	NiNode* v72 = CaravanMenu::spModel.m_pObject;
	v72->AttachChild(pHandGeometry[1], 1);
	for (int32_t i_8 = 0; i_8 < 6; ++i_8) {
		pTrackGrids[i_8]->SetAppCulled(1);
		NiUpdateData* updated = FNVD_RECONSTRUCT(NiUpdateData)(&apMemory, 0.0f, 0, 0);
		pTrackGrids[i_8]->Update(*updated);
	}
	NiUpdateData* v8 = FNVD_RECONSTRUCT(NiUpdateData)(&v107, 0.0f, 0, 0);
	pCursors[0]->Update(*v8);
	NiUpdateData* v9 = FNVD_RECONSTRUCT(NiUpdateData)(&v106, 0.0f, 0, 0);
	pCursors[1]->Update(*v9);
	pCursors[0]->SetTranslate(0.0f, 0.0f, -5.0f);
	pCursors[1]->SetTranslate(0.0f, 0.0f, -5.0f);
	NiUpdateData* v10 = FNVD_RECONSTRUCT(NiUpdateData)(&v105, 0.0f, 0, 0);
	pCursors[0]->Update(*v10);
	NiUpdateData* v11 = FNVD_RECONSTRUCT(NiUpdateData)(&v104, 0.0f, 0, 0);
	pCursors[1]->Update(*v11);
	for (int32_t i_9 = 0; i_9 < 2; ++i_9) {
		pTimeController = pHandGeometry[i_9]->GetController(&NiControllerManager::ms_RTTI);
		if (pTimeController) {
			pHandAnimManager[i_9] = static_cast<NiControllerManager*>(pTimeController);
			NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Deck-Cards_In");
			NiControllerSequence* SequenceByName = pHandAnimManager[i_9]->GetSequenceByName(*kName);
			pHandAnimations[i_9][0] = SequenceByName;
			apString.~NiFixedString();
			NiFixedString* v70 = ::new (static_cast<void*>(&v102)) NiFixedString("Deck-Card1_Out");
			NiControllerSequence* v13 = pHandAnimManager[i_9]->GetSequenceByName(*v70);
			pHandAnimations[i_9][1] = v13;
			v102.~NiFixedString();
			NiFixedString* v69 = ::new (static_cast<void*>(&v101)) NiFixedString("Deck-Card2_Out");
			NiControllerSequence* v14 = pHandAnimManager[i_9]->GetSequenceByName(*v69);
			pHandAnimations[i_9][2] = v14;
			v101.~NiFixedString();
			NiFixedString* v68 = ::new (static_cast<void*>(&v100)) NiFixedString("Deck-Card3_Out");
			NiControllerSequence* v15 = pHandAnimManager[i_9]->GetSequenceByName(*v68);
			pHandAnimations[i_9][3] = v15;
			v100.~NiFixedString();
			NiFixedString* v67 = ::new (static_cast<void*>(&v99)) NiFixedString("Deck-Card4_Out");
			NiControllerSequence* v16 = pHandAnimManager[i_9]->GetSequenceByName(*v67);
			pHandAnimations[i_9][4] = v16;
			v99.~NiFixedString();
			NiFixedString* v66 = ::new (static_cast<void*>(&v98)) NiFixedString("Deck-Card5_Out");
			NiControllerSequence* v17 = pHandAnimManager[i_9]->GetSequenceByName(*v66);
			pHandAnimations[i_9][5] = v17;
			v98.~NiFixedString();
			NiFixedString* v65 = ::new (static_cast<void*>(&v97)) NiFixedString("Deck8-Card1_Out");
			NiControllerSequence* v18 = pHandAnimManager[i_9]->GetSequenceByName(*v65);
			pStartAnimations[i_9][0] = v18;
			v97.~NiFixedString();
			NiFixedString* v64 = ::new (static_cast<void*>(&v96)) NiFixedString("Deck8-Card2_Out");
			NiControllerSequence* v19 = pHandAnimManager[i_9]->GetSequenceByName(*v64);
			pStartAnimations[i_9][1] = v19;
			v96.~NiFixedString();
			NiFixedString* v63 = ::new (static_cast<void*>(&v95)) NiFixedString("Deck8-Card3_Out");
			NiControllerSequence* v20 = pHandAnimManager[i_9]->GetSequenceByName(*v63);
			pStartAnimations[i_9][2] = v20;
			v95.~NiFixedString();
			NiFixedString* v62 = ::new (static_cast<void*>(&v94)) NiFixedString("Deck8-Card4_Out");
			NiControllerSequence* v21 = pHandAnimManager[i_9]->GetSequenceByName(*v62);
			pStartAnimations[i_9][3] = v21;
			v94.~NiFixedString();
			NiFixedString* v61 = ::new (static_cast<void*>(&v93)) NiFixedString("Deck8-Card5_Out");
			NiControllerSequence* v22 = pHandAnimManager[i_9]->GetSequenceByName(*v61);
			pStartAnimations[i_9][4] = v22;
			v93.~NiFixedString();
			NiFixedString* v60 = ::new (static_cast<void*>(&v92)) NiFixedString("Deck8-Card6_Out");
			NiControllerSequence* v23 = pHandAnimManager[i_9]->GetSequenceByName(*v60);
			pStartAnimations[i_9][5] = v23;
			v92.~NiFixedString();
			NiFixedString* v59 = ::new (static_cast<void*>(&v91)) NiFixedString("Deck8-Card7_Out");
			NiControllerSequence* v24 = pHandAnimManager[i_9]->GetSequenceByName(*v59);
			pStartAnimations[i_9][6] = v24;
			v91.~NiFixedString();
			NiFixedString* v58 = ::new (static_cast<void*>(&v90)) NiFixedString("Deck8-Card8_Out");
			NiControllerSequence* v25 = pHandAnimManager[i_9]->GetSequenceByName(*v58);
			pStartAnimations[i_9][7] = v25;
			v90.~NiFixedString();
			pHandAnimManager[i_9]->DeactivateAll(0.0f);
			NiTimeController::StartAnimations(pHandGeometry[i_9]);
		}
	}
	for (int32_t i_10 = 0; i_10 < 6; ++i_10) {
		for (int32_t j_2 = 0; j_2 < 7; ++j_2) {
			pTimeController = pTrackGeometry[i_10][j_2]->GetController(&NiControllerManager::ms_RTTI);
			if (pTimeController) {
				pTrackAnimManager[i_10][j_2] = static_cast<NiControllerManager*>(pTimeController);
				NiFixedString* v57 = ::new (static_cast<void*>(&v89)) NiFixedString("Card1_In");
				NiControllerSequence* v26 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v57);
				pTrackAnimations[i_10][j_2][0][0] = v26;
				v89.~NiFixedString();
				NiFixedString* v56 = ::new (static_cast<void*>(&v88)) NiFixedString("Card2_In");
				NiControllerSequence* v27 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v56);
				pTrackAnimations[i_10][j_2][1][0] = v27;
				v88.~NiFixedString();
				NiFixedString* v55 = ::new (static_cast<void*>(&v87)) NiFixedString("Card3_In");
				NiControllerSequence* v28 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v55);
				pTrackAnimations[i_10][j_2][2][0] = v28;
				v87.~NiFixedString();
				NiFixedString* v54 = ::new (static_cast<void*>(&v86)) NiFixedString("Card4_In");
				NiControllerSequence* v29 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v54);
				pTrackAnimations[i_10][j_2][3][0] = v29;
				v86.~NiFixedString();
				NiFixedString* v53 = ::new (static_cast<void*>(&v85)) NiFixedString("Card1_Out");
				NiControllerSequence* v30 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v53);
				pTrackAnimations[i_10][j_2][0][1] = v30;
				v85.~NiFixedString();
				NiFixedString* v52 = ::new (static_cast<void*>(&v84)) NiFixedString("Card2_Out");
				NiControllerSequence* v31 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v52);
				pTrackAnimations[i_10][j_2][1][1] = v31;
				v84.~NiFixedString();
				NiFixedString* v51 = ::new (static_cast<void*>(&v83)) NiFixedString("Card3_Out");
				NiControllerSequence* v32 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v51);
				pTrackAnimations[i_10][j_2][2][1] = v32;
				v83.~NiFixedString();
				NiFixedString* v50 = ::new (static_cast<void*>(&v82)) NiFixedString("Card4_Out");
				NiControllerSequence* v33 = pTrackAnimManager[i_10][j_2]->GetSequenceByName(*v50);
				pTrackAnimations[i_10][j_2][3][1] = v33;
				v82.~NiFixedString();
				pTrackAnimManager[i_10][j_2]->DeactivateAll(0.0f);
				for (int32_t k = 0; k < 4; ++k) {
					pTrackAnimations[i_10][j_2][k][0]->SetCycleType(NiTimeController::CLAMP);
					pTrackAnimations[i_10][j_2][k][1]->SetCycleType(NiTimeController::CLAMP);
				}
				NiTimeController::StartAnimations(pTrackGeometry[i_10][j_2]);
				pCardTracks[i_10][j_2].Clear(1);
			}
		}
	}
	NiUpdateData* v34 = FNVD_RECONSTRUCT(NiUpdateData)(&v81, 0.0f, 0, 0);
	CaravanMenu::spModel.m_pObject->Update(*v34);
	for (int32_t i_11 = 0; i_11 < 6; ++i_11) {
		trackInfo[i_11].iValue = 0;
		trackInfo[i_11].eCardSuit = 5;
		trackInfo[i_11].eDirection = 0;
		UpdateTrackValue(i_11);
	}
	SetFlag(CaravanMenu::CAF_TRACK_SETUP, 1);
	UpdateGameHandTextures(1, 1);
	UpdateGameHandTextures(0, 1);
	pSelectedCard = 0;
	pSelectedObject = 0;
	pSelectedShadow = 0;
	ZeroFlags();
	SetFlag(CaravanMenu::CAF_AI, 1);
	SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 1);
	uint32_t activeList_2 = pPlayer->GetCaravanLosses();
	uint32_t inactiveList_2 = pPlayer->GetCaravanWinnings();
	BSsprintf(Src, 260, "%i/%i", inactiveList_2, activeList_2);
	pTiles[16]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[16]->Set(CaravanMenu::iUT_Value, 2 * iAnte);
	pTiles[17]->Poke(CaravanMenu::iUT_Value, 1.0f);
	uint32_t CaravanCapWinnings = pPlayer->GetCaravanCapWinnings();
	int32_t ivalue = CaravanCapWinnings - pPlayer->GetCaravanCapLosses();
	pTiles[17]->Set(CaravanMenu::iUT_Value, ivalue);
	pTiles[18]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[18]->Set(CaravanMenu::iUT_Value, Src, 1);
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
	uint32_t v37 = pPlayerDeck.iSize;
	pTiles[29]->Set(CaravanMenu::iUT_Value, v37);
	pTiles[30]->Poke(CaravanMenu::iUT_Value, 1.0f);
	uint32_t v38 = pNPCDeck.iSize;
	pTiles[30]->Set(CaravanMenu::iUT_Value, v38);
	eActiveMenu = CaravanMenu::CA_GAME_MENU;
	eGameState = CaravanMenu::CA_CAMERA_TO_GAME_ANIM;
	Tile* th_s = pTiles[2]->GetChildByName("CGM_InfoRect");
	if (th_s) {
		float v49 = th_s->GetFloat(4017) + 150.0;
		th_s->Set(4017, v49, 1);
		float v48 = th_s->GetFloat(4001) - 150.0;
		th_s->Set(4001, v48, 1);
		th_s->AddNeedsUpdate(17);
	}
	const char* v39 = sBack.String();
	pTiles[22]->Set(4036, v39, 1);
	pTiles[0]->Set(4003, 0);
	pTiles[1]->Set(4003, 0);
	pTiles[2]->Set(4003, 1);
	pTiles[3]->Set(4003, 0);
	const char* v40 = sAntePotText.String();
	pTiles[44]->Set(4036, v40, 1);
	const char* v41 = sNetTotalText.String();
	pTiles[45]->Set(4036, v41, 1);
	const char* v42 = sWinLossText.String();
	pTiles[46]->Set(4036, v42, 1);
	const char* v43 = sSelectCardText.String();
	pTiles[19]->Set(4036, v43, 1);
	const char* v44 = sDiscardCardText.String();
	pTiles[20]->Set(4036, v44, 1);
	const char* v45 = sDiscardTrackText.String();
	pTiles[21]->Set(4036, v45, 1);
	const char* v46 = sCancel.String();
	pTiles[22]->Set(4036, v46, 1);
	UpdateCaravanFlags();
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::PrepareResultsMenu() {
	const char* apString;
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	char Src[268];
	eGameState = CaravanMenu::CA_CAMERA_TO_RESULTS_ANIM;
	if (this->IsFlagSet(CaravanMenu::CAF_PLAYERWIN)) {
		uint32_t CaravanCapWinnings = pPlayer->GetCaravanCapWinnings();
		pPlayer->SetCaravanCapWinnings(static_cast<int32_t>(iAnte + CaravanCapWinnings));
		uint32_t CaravanWinnings = pPlayer->GetCaravanWinnings();
		pPlayer->SetCaravanWinnings(CaravanWinnings + 1);
		if (iAnte > pPlayer->GetCaravanLargestWinning()) {
			pPlayer->SetCaravanLargestWinning(iAnte);
		}
		MiscStatManager::Increment(MiscStatManager::MS_CARAVAN_GAMES_WON);
		BSAudio* v3 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "GAMECaravanWin", 289u);
		SoundHandleByName->Play(0);
	} else {
		uint32_t CaravanCapLosses = pPlayer->GetCaravanCapLosses();
		pPlayer->SetCaravanCapLosses(static_cast<int32_t>(iAnte + CaravanCapLosses));
		uint32_t CaravanLosses = pPlayer->GetCaravanLosses();
		pPlayer->SetCaravanLosses(CaravanLosses + 1);
		MiscStatManager::Increment(MiscStatManager::MS_CARAVAN_GAMES_LOST);
		BSAudio* v6 = &BSAudio::QInstance();
		BSSoundHandle* v18 = v6->GetSoundHandleByName(&arHandle, "GAMECaravanLose", 289u);
		v18->Play(0);
	}
	Tile* th_s = pTiles[3]->GetChildByName("CRM_InfoRect");
	if (th_s) {
		float v17 = th_s->GetFloat(4017) + 150.0;
		th_s->Set(4017, v17, 1);
		th_s->AddNeedsUpdate(16);
	}
	uint32_t afValue = pPlayer->GetCaravanLosses();
	BSsprintf(Src, 260, "%i/%i", pPlayer->GetCaravanWinnings(), afValue);
	pTiles[31]->Poke(CaravanMenu::iUT_Value, 1.0f);
	uint32_t v7 = pPlayer->GetCaravanCapWinnings();
	pTiles[31]->Set(CaravanMenu::iUT_Value, v7);
	pTiles[32]->Poke(CaravanMenu::iUT_Value, 1.0f);
	uint32_t v8 = pPlayer->GetCaravanCapLosses();
	pTiles[32]->Set(CaravanMenu::iUT_Value, v8);
	pTiles[33]->Poke(CaravanMenu::iUT_Value, 1.0f);
	pTiles[33]->Set(CaravanMenu::iUT_Value, Src, 1);
	pTiles[34]->Poke(CaravanMenu::iUT_Value, 1.0f);
	uint32_t CaravanLargestWinning = pPlayer->GetCaravanLargestWinning();
	pTiles[34]->Set(CaravanMenu::iUT_Value, CaravanLargestWinning);
	if (this->IsFlagSet(CaravanMenu::CAF_PLAYERWIN)) {
		apString = sCaravanYouWinText.String();
	} else {
		apString = sCaravanYouLoseText.String();
	}
	pTiles[35]->Set(4036, apString, 1);
	pTiles[0]->Set(4003, 0);
	pTiles[1]->Set(4003, 0);
	pTiles[2]->Set(4003, 0);
	pTiles[3]->Set(4003, 1);
	const char* v10 = sCaravanWinningsText.String();
	pTiles[47]->Set(4036, v10, 1);
	const char* v11 = sCaravanLossesText.String();
	pTiles[48]->Set(4036, v11, 1);
	const char* v12 = sCaravanWinLossText.String();
	pTiles[49]->Set(4036, v12, 1);
	const char* v13 = sCaravanBiggestAnteText.String();
	pTiles[50]->Set(4036, v13, 1);
	char IsFlagSet = this->IsFlagSet(CaravanMenu::CAF_PLAYERWIN);
	RewardOrChargePlayer(IsFlagSet);
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::FlipSelectedCardTextures(int32_t aiTarget) {
	if (deckFaceShaders[12]) {
		if (deckFaceShaders[36]) {
			if (deckFaceShaders[60]) {
				if (deckFaceShaders[84]) {
					char apDest[268];
					// The exe's language-swapped path buffer ([ebp-0x11c], 268 bytes).
					char acPath[268];
					int32_t index = static_cast<int32_t>(pTiles[15]->GetFloat(CaravanMenu::iUT_CurrentValue));
					if (pDeckArray.operator[](index)->bIsActive) {
						FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr,
							&NiPointer<NiSourceTexture>::Destruct, static_cast<NiSourceTexture*>(0));
						BSsprintf(apDest, 260, "%s%s", "Textures\\",
							(&pDeckArray.operator[](index)->faceTexture)->GetTextureName());
						SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
						TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
						if (textptr.m_pObject) {
							deckFaceShaders[aiTarget]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
								textptr.m_pObject);
						}
						BSsprintf(apDest, 260, "%s%s", "Textures\\",
							(&pDeckArray.operator[](index)->backTexture)->GetTextureName());
						SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
						TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
						if (textptr.m_pObject) {
							deckFaceShaders[aiTarget + 24]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
								textptr.m_pObject);
						}
						BSsprintf(apDest, 260, "%s%s", "Textures\\",
							(&pDeckArray.operator[](index)->backTexture)->GetTextureName());
						SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
						TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
						if (textptr.m_pObject) {
							deckFaceShaders[aiTarget + 48]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
								textptr.m_pObject);
						}
						BSsprintf(apDest, 260, "%s%s", "Textures\\",
							(&pDeckArray.operator[](index)->faceTexture)->GetTextureName());
						SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
						TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
						if (textptr.m_pObject) {
							deckFaceShaders[aiTarget + 72]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
								textptr.m_pObject);
						}
					} else {
						// The exe tests the address of the card's faceTexture, which is never null, after the call.
						pDeckArray.operator[](index);
						{
							FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr_2,
								&NiPointer<NiSourceTexture>::Destruct, static_cast<NiSourceTexture*>(0));
							BSsprintf(apDest, 260, "%s%s", "Textures\\",
								(&pDeckArray.operator[](index)->faceTexture)->GetTextureName());
							SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
							TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
							if (textptr_2.m_pObject) {
								deckFaceShaders[aiTarget + 48]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
									textptr_2.m_pObject);
							}
							BSsprintf(apDest, 260, "%s%s", "Textures\\",
								(&pDeckArray.operator[](index)->backTexture)->GetTextureName());
							SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
							TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
							if (textptr_2.m_pObject) {
								deckFaceShaders[aiTarget + 72]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
									textptr_2.m_pObject);
							}
							BSsprintf(apDest, 260, "%s%s", "Textures\\",
								(&pDeckArray.operator[](index)->backTexture)->GetTextureName());
							SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
							TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
							if (textptr_2.m_pObject) {
								deckFaceShaders[aiTarget]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
									textptr_2.m_pObject);
							}
							BSsprintf(apDest, 260, "%s%s", "Textures\\",
								(&pDeckArray.operator[](index)->faceTexture)->GetTextureName());
							SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
							TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
							if (textptr_2.m_pObject) {
								deckFaceShaders[aiTarget + 24]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
									textptr_2.m_pObject);
							}
						}
					}
				}
			}
		}
	}
}

// Pays the ante: with abReward the challenger's caps in the barter container go to the player, else the player's go
// to the challenger; what the money items do not cover is paid in gold (form 0xF).
// The exe passes the challenger's base form (TESObjectREFR+0x20, through 0x7AF430) as the vendor reference.
// The exe's cleanup-only C++ EH frame is left out.
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
						FNVD_CONSTRUCTED_LOCAL(TESContainer, kContainer, &TESContainer::Destruct);
						kContainer.AddObject(pObject, iCount, nullptr);
						kContainer.AddContainerToReference(pPlayer, 0);
						TESObjectREFR* pVendor =
							static_cast<TESObjectREFR*>(static_cast<TESForm*>(pChallenger->GetObjectReference()));
						pDataHandler->RemoveItemBarterContainer(pNode->GetItem(), iCount, pVendor);
						iNumber -= iCount * iValue;
					}
				}
			} else {
				pObject = nullptr;
			}
		}
		if (iNumber) {
			FNVD_CONSTRUCTED_LOCAL(TESContainer, kGoldContainer, &TESContainer::Destruct);
			kGoldContainer.AddObject(static_cast<TESObject*>(TESForm::GetFormByNumericID(15u)), iNumber, nullptr);
			kGoldContainer.AddContainerToReference(pPlayer, 0);
			FNVD_CONSTRUCTED_LOCAL(ItemChange, kGold, &ItemChange::Destruct,
				static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u)), 0);
			if (pChanges->GetGoldAmount() > iNumber) {
				TESObjectREFR* pVendor =
					static_cast<TESObjectREFR*>(static_cast<TESForm*>(pChallenger->GetObjectReference()));
				pDataHandler->RemoveItemBarterContainer(&kGold, iNumber, pVendor);
			} else {
				TESObjectREFR* pVendor =
					static_cast<TESObjectREFR*>(static_cast<TESForm*>(pChallenger->GetObjectReference()));
				int32_t iGold = pChanges->GetGoldAmount();
				pDataHandler->RemoveItemBarterContainer(&kGold, iGold, pVendor);
				int32_t iRest = iNumber - pChanges->GetGoldAmount();
				TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
				pChallenger->RemoveItem(pGold, nullptr, iRest, 0, 0, nullptr, nullptr, nullptr, 1, 0);
			}
		}
	} else {
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
						FNVD_CONSTRUCTED_LOCAL(TESContainer, kContainer, &TESContainer::Destruct);
						kContainer.AddObject(pObject, iCount, nullptr);
						kContainer.AddContainerToReference(pChallenger, 0);
						pPlayer->RemoveItem(pObject, nullptr, iCount, 0, 0, nullptr, nullptr, nullptr, 1, 0);
						iNumber -= iCount * iValue;
					}
				}
			} else {
				pObject = nullptr;
			}
		}
		if (iNumber) {
			FNVD_CONSTRUCTED_LOCAL(TESContainer, kGoldContainer, &TESContainer::Destruct);
			kGoldContainer.AddObject(static_cast<TESObject*>(TESForm::GetFormByNumericID(15u)), iNumber, nullptr);
			kGoldContainer.AddContainerToReference(pChallenger, 0);
			if (pPlayer->GetGoldAmount() > iNumber) {
				TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
				pPlayer->RemoveItem(pGold, nullptr, iNumber, 0, 0, nullptr, nullptr, nullptr, 1, 0);
			} else {
				int32_t iGold = pPlayer->GetGoldAmount();
				TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
				pPlayer->RemoveItem(pGold, nullptr, iGold, 0, 0, nullptr, nullptr, nullptr, 1, 0);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::UpdateGameHandTextures(bool abPlayer, bool bStartup) {
	NiAVObject* pHand;
	int32_t max;
	BSShaderPPLightingProperty* faceshader_2;
	NiAVObject* face;
	char acPath[268];
	char blockName[268];
	if (abPlayer) {
		max = pPlayerHand.iSize;
	} else {
		max = pNPCHand.iSize;
	}
	int32_t v26 = max;
	if (abPlayer) {
		pHand = pHandGeometry[0];
	} else {
		pHand = pHandGeometry[1];
	}
	for (int32_t i = 0; i < 8; ++i) {
		if (abPlayer) {
			BSsprintf(blockName, 260, "Player-Deck_0%i:%i", i + 1, i);
		} else {
			BSsprintf(blockName, 260, "Opponent-Deck_0%i:%i", i + 1, i);
		}
		face = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
		if (i >= v26) {
			if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) || i == 5) {
				if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP) && v26 < 5) {
					face->SetAppCulled(1);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1_2, nullptr, 0.0f, 0, 0);
					face->Update(update1_2);
					if (abPlayer) {
						BSsprintf(blockName, 260, "Player-Deck_0%i_Shadow:0", i + 1);
					} else {
						BSsprintf(blockName, 260, "Opponent-Deck_0%i_Shadow:0", i + 1);
					}
					face = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
					face->SetAppCulled(1);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update2_2, nullptr, 0.0f, 0, 0);
					face->Update(update2_2);
				}
			} else {
				face->SetAppCulled(1);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update1, nullptr, 0.0f, 0, 0);
				face->Update(update1);
				if (abPlayer) {
					BSsprintf(blockName, 260, "Player-Deck_0%i_Shadow:0", i + 1);
				} else {
					BSsprintf(blockName, 260, "Opponent-Deck_0%i_Shadow:0", i + 1);
				}
				face = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
				face->SetAppCulled(1);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update2, nullptr, 0.0f, 0, 0);
				face->Update(update2);
			}
		} else {
			BSShaderPPLightingProperty* faceshader;
			int32_t v15;
			if (face->GetProperty(3)) {
				// The shader type is read through the shared one-field getter 0x441110 (a trivial accessor).
				BSShaderPPLightingProperty* pProperty = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
				int32_t v14 = false;
				if (pProperty->iShaderPropertyType >= 8) {
					BSShaderPPLightingProperty* v4 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
					if (v4->iShaderPropertyType <= 12) {
						v14 = true;
					}
				}
				v15 = v14;
			} else {
				v15 = 0;
			}
			if (v15) {
				faceshader = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
			} else {
				faceshader = 0;
			}
			faceshader_2 = faceshader;
			FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr, &NiPointer<NiSourceTexture>::Destruct,
				static_cast<NiSourceTexture*>(0));
			if (faceshader_2) {
				if (abPlayer) {
					BSsprintf(blockName, 260, "Textures\\%s",
						(&pPlayerHand.operator[](i)->faceTexture)->GetTextureName());
				} else {
					if (CaravanMenu::bDebugAI) {
						BSsprintf(blockName, 260, "Textures\\%s",
							(&pNPCHand.operator[](i)->faceTexture)->GetTextureName());
					} else {
						BSsprintf(blockName, 260, "Textures\\%s",
							(&pNPCHand.operator[](i)->backTexture)->GetTextureName());
					}
				}
				SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
				TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
				if (textptr.m_pObject) {
					NiSourceTexture* v7 = textptr.m_pObject;
					faceshader_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v7);
				}
			}
		}
	}
	if (!IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
		BSShaderPPLightingProperty* faceshader_3;
		int32_t v12;
		if (abPlayer) {
			BSstrcpy(blockName, 260, "Player-Deck_06:5");
		} else {
			BSstrcpy(blockName, 260, "Opponent-Deck_06:5");
		}
		face = BSUtilities::GetObjectByName(pHand, static_cast<const char*>(blockName));
		if (face->GetProperty(3)) {
			BSShaderPPLightingProperty* v5 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
			int32_t v11 = false;
			if (v5->iShaderPropertyType >= 8) {
				BSShaderPPLightingProperty* v6 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
				if (v6->iShaderPropertyType <= 12) {
					v11 = true;
				}
			}
			v12 = v11;
		} else {
			v12 = 0;
		}
		if (v12) {
			faceshader_3 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
		} else {
			faceshader_3 = 0;
		}
		faceshader_2 = faceshader_3;
		FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr_2, &NiPointer<NiSourceTexture>::Destruct,
			static_cast<NiSourceTexture*>(0));
		if (faceshader_2) {
			int32_t v9;
			if (abPlayer) {
				v9 = pPlayerDeck.iSize != 0;
			} else {
				v9 = pNPCDeck.iSize != 0;
			}
			if (v9) {
				if (abPlayer) {
					BSsprintf(blockName, 260, "Textures\\%s",
						(&pPlayerDeck.operator[](iNextPlayerCard)->backTexture)->GetTextureName());
				} else {
					BSsprintf(blockName, 260, "Textures\\%s",
						(&pNPCDeck.operator[](iNextNPCCard)->backTexture)->GetTextureName());
				}
				SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
				TES::pInstance->CreateTextureImage(acPath, textptr_2, 0, 0);
				if (textptr_2.m_pObject) {
					NiSourceTexture* v8 = textptr_2.m_pObject;
					faceshader_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, v8);
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::UpdateSelectedTrackCardTexture() {
	int32_t v3;
	BSShaderPPLightingProperty* faceshader_3;
	int32_t v6;
	char acPath[264];
	char blockName[264];
	NiAVObject* avobj = pTrackGeometry[iSelectedTrack][iSelectedRow];
	if (iSelectedTrack < 3) {
		v3 = iSelectedColumn + 1;
	} else {
		v3 = 4 - iSelectedColumn;
	}
	BSsprintf(blockName, 260, "Card_0%i:%i", v3, iSelectedColumn);
	NiAVObject* face = BSUtilities::GetObjectByName(avobj, static_cast<const char*>(blockName));
	if (face->GetProperty(3)) {
		BSShaderPPLightingProperty* faceshader = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
		int32_t v5 = false;
		if (faceshader->iShaderPropertyType >= 8) {
			BSShaderPPLightingProperty* faceshader_2 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
			if (faceshader_2->iShaderPropertyType <= 12) {
				v5 = true;
			}
		}
		v6 = v5;
	} else {
		v6 = 0;
	}
	if (v6) {
		faceshader_3 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
	} else {
		faceshader_3 = 0;
	}
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr, &NiPointer<NiSourceTexture>::Destruct,
		static_cast<NiSourceTexture*>(0));
	if (faceshader_3) {
		BSsprintf(blockName, 260, "%s%s", "Textures\\", (&pSelectedCard->faceTexture)->GetTextureName());
		SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
		TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
		if (textptr.m_pObject) {
			faceshader_3->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, textptr.m_pObject);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::UpdateTrackCardTexture(int32_t aiTrack, int32_t aiRow, int32_t aiColumn) {
	BSShaderPPLightingProperty* faceshader_3;
	int32_t v8;
	char acPath[264];
	char blockName[264];
	NiAVObject* avobj = pTrackGeometry[aiTrack][aiRow];
	if (aiTrack < 3) {
		BSsprintf(blockName, 260, "Card_0%i:%i", aiColumn + 1, aiColumn);
	} else {
		BSsprintf(blockName, 260, "Card_0%i:%i", 4 - aiColumn, aiColumn);
	}
	NiAVObject* face = BSUtilities::GetObjectByName(avobj, static_cast<const char*>(blockName));
	if (face->GetProperty(3)) {
		BSShaderPPLightingProperty* faceshader = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
		int32_t v7 = false;
		if (faceshader->iShaderPropertyType >= 8) {
			BSShaderPPLightingProperty* faceshader_2 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
			if (faceshader_2->iShaderPropertyType <= 12) {
				v7 = true;
			}
		}
		v8 = v7;
	} else {
		v8 = 0;
	}
	if (v8) {
		faceshader_3 = static_cast<BSShaderPPLightingProperty*>(face->GetProperty(3));
	} else {
		faceshader_3 = 0;
	}
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiSourceTexture>, textptr, &NiPointer<NiSourceTexture>::Destruct,
		static_cast<NiSourceTexture*>(0));
	if (faceshader_3) {
		BSsprintf(blockName, 260, "%s%s", "Textures\\",
			(&pCardTracks[aiTrack][aiRow].operator[](aiColumn)->faceTexture)->GetTextureName());
		SwapPlatformLanguageTexturePath(blockName, acPath, 260u);
		TES::pInstance->CreateTextureImage(acPath, textptr, 0, 0);
		if (textptr.m_pObject) {
			faceshader_3->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, textptr.m_pObject);
		}
	}
}

int32_t CaravanMenu::GetRowValue(int32_t aiTrack, int32_t aiRow) {
	int32_t iValue = 0;
	int32_t iSize = pCardTracks[aiTrack][aiRow].iSize;
	if (iSize > 0) {
		iValue = pCardTracks[aiTrack][aiRow].operator[](0)->data.iFaceValue;
		for (int32_t i = 1; i < iSize; ++i) {
			if (pCardTracks[aiTrack][aiRow].operator[](i)->data.iFaceValue == 14) {
				iValue *= 2;
			}
		}
	}
	return iValue;
}

// The exe tests each bound signed (jl/jg) and the deck card index against the hand's last index unsigned (jbe).
bool CaravanMenu::IsValidCardPlacement(int32_t aiTrack, int32_t aiRow, int32_t aiDeckCard, bool abIsNPC) {
	if (aiTrack < 0 || aiTrack > 5 || aiRow < 0 || aiRow > 6 || aiDeckCard < 0) {
		return false;
	}
	uint32_t uiLastCard;
	if (abIsNPC) {
		uiLastCard = pNPCHand.iSize - 1;
	} else {
		uiLastCard = pPlayerHand.iSize - 1;
	}
	if (static_cast<uint32_t>(aiDeckCard) > uiLastCard) {
		return false;
	}
	int32_t iFaceValue;
	if (abIsNPC) {
		iFaceValue = pNPCHand.operator[](aiDeckCard)->data.iFaceValue;
	} else {
		iFaceValue = pPlayerHand.operator[](aiDeckCard)->data.iFaceValue;
	}
	int32_t iCardSuit;
	if (abIsNPC) {
		iCardSuit = pNPCHand.operator[](aiDeckCard)->data.iCardSuit;
	} else {
		iCardSuit = pPlayerHand.operator[](aiDeckCard)->data.iCardSuit;
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
			int32_t iPrevValue = pCardTracks[aiTrack][aiRow - 1].operator[](0)->data.iFaceValue;
			if (iPrevValue == iFaceValue) {
				return false;
			}
			if (trackInfo[aiTrack].eDirection == 1 && iPrevValue >= iFaceValue &&
				trackInfo[aiTrack].eCardSuit != iCardSuit) {
				return false;
			}
			if (trackInfo[aiTrack].eDirection == -1 && iPrevValue <= iFaceValue &&
				trackInfo[aiTrack].eCardSuit != iCardSuit) {
				return false;
			}
		}
	} else {
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

// KEEP (not registered): with CAF_TRACK_SETUP set, the exe copies its local package to apResult (rep movsd at
// 0x74FF02) with only iActionType and iDeckCard written; the other five fields are uninitialised stack bytes.
// The calls to the one-field setters and getters of CaravanAIPackage, BSSimpleArray and TESCaravanCard (0x403550,
// 0x41FD00, 0x437730, 0x600AB0, 0x984F60, 0x9611E0, 0x44DDC0, 0x8D80E0) are trivial accessors, written as the fields.
// The exe's cleanup-only C++ EH frame is left out.
CaravanAIPackage* CaravanMenu::ProcessAI(CaravanAIPackage* apResult) {
	for (int32_t iTrack = 0; iTrack < 6; ++iTrack) {
		UpdateTrackValue(iTrack);
	}
	if (IsFlagSet(CaravanMenu::CAF_TRACK_SETUP)) {
		// Left uninitialised as in the exe: only iActionType and iDeckCard are written.
		CaravanAIPackage setupPackage;
		setupPackage.iActionType = 4;
		if (iDiffLevel >= 1) {
			if (iDiffLevel >= 3) {
				int32_t numcards = 0;
				for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
					if (pNPCHand[i]->data.iFaceValue <= 10) {
						++numcards;
					}
				}
				if (pNPCHand.iSize - numcards <= 5) {
					int32_t idistance = std::abs(pNPCHand[0]->data.iFaceValue - 6);
					setupPackage.iDeckCard = 0;
					for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
						if (std::abs(pNPCHand[i]->data.iFaceValue - 6) < idistance) {
							setupPackage.iDeckCard = i;
							idistance = std::abs(pNPCHand[i]->data.iFaceValue - 6);
						}
					}
				} else {
					int32_t ivalue = 0;
					setupPackage.iActionType = 6;
					for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
						if (pNPCHand[i]->data.iFaceValue > 10) {
							if (pNPCHand[i]->data.iFaceValue == 15) {
								setupPackage.iDeckCard = i;
								break;
							}
							if (!ivalue || ivalue == 12) {
								setupPackage.iDeckCard = i;
								ivalue = pNPCHand[i]->data.iFaceValue;
							} else if (ivalue == 14 && pNPCHand[i]->data.iFaceValue != 12) {
								setupPackage.iDeckCard = i;
								ivalue = pNPCHand[i]->data.iFaceValue;
							}
						}
					}
				}
			} else {
				int32_t hicards = 0;
				int32_t lowcards = 0;
				for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
					if (pNPCHand[i]->data.iFaceValue > 5 && pNPCHand[i]->data.iFaceValue <= 10) {
						++hicards;
					} else if (pNPCHand[i]->data.iFaceValue < 4) {
						++lowcards;
					}
				}
				if (pNPCHand.iSize - lowcards - hicards <= 5) {
					for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
						if (hicards > lowcards) {
							if (pNPCHand[i]->data.iFaceValue > 5 && pNPCHand[i]->data.iFaceValue <= 10) {
								setupPackage.iDeckCard = i;
								break;
							}
						} else if (lowcards && pNPCHand[i]->data.iFaceValue < 4) {
							setupPackage.iDeckCard = i;
							break;
						}
					}
				} else {
					int32_t ivalue = 0;
					setupPackage.iActionType = 6;
					for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
						if (pNPCHand[i]->data.iFaceValue > 10) {
							if (pNPCHand[i]->data.iFaceValue == 15) {
								setupPackage.iDeckCard = i;
								break;
							}
							if (!ivalue || ivalue == 12) {
								setupPackage.iDeckCard = i;
								ivalue = pNPCHand[i]->data.iFaceValue;
							} else if (ivalue == 14 && pNPCHand[i]->data.iFaceValue != 12) {
								setupPackage.iDeckCard = i;
								ivalue = pNPCHand[i]->data.iFaceValue;
							}
						}
					}
				}
			}
		} else {
			int32_t numcards = 0;
			int32_t facecardindex = 0;
			int32_t numcardindex = 0;
			for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
				if (pNPCHand[i]->data.iFaceValue > 10) {
					facecardindex = i;
				} else {
					++numcards;
					numcardindex = i;
				}
			}
			if (pNPCHand.iSize - numcards <= 5) {
				setupPackage.iDeckCard = numcardindex;
			} else {
				setupPackage.iActionType = 6;
				setupPackage.iDeckCard = facecardindex;
			}
		}
		*apResult = setupPackage;
		return apResult;
	}

	BSSimpleArray<CaravanAIPackage*>* pPackageList;
	void* pListMemory = ourMalloc(sizeof(BSSimpleArray<CaravanAIPackage*>));
	if (pListMemory) {
		pPackageList = ::new (pListMemory) BSSimpleArray<CaravanAIPackage*>;
	} else {
		pPackageList = nullptr;
	}
	// Each package the AI weighs is added to the list, and a fresh zeroed one takes its place.
	CaravanAIPackage* pPackage = static_cast<CaravanAIPackage*>(ourMalloc(sizeof(CaravanAIPackage)));
	if (pPackage) {
		std::memset(pPackage, 0, sizeof(CaravanAIPackage));
	}
	CaravanAIPackage* pNew;
	for (int32_t iTrack = 3; iTrack < 6; ++iTrack) {
		pPackage->iTrack = iTrack;
		int32_t total = trackInfo[iTrack].iValue;
		if (CalculateWinningState(total) >= 0) {
			if (CalculateWinningState(total)) {
				if (CalculateWinningState(total) > 0) {
					for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
						pPackage->iDeckCard = iDeckCard;
						if (pNPCHand[iDeckCard]->data.iFaceValue != 12) {
							continue;
						}
						for (int32_t iRow = 0; iRow < 7; ++iRow) {
							pPackage->iRow = iRow;
							int32_t rowVal = GetRowValue(iTrack, iRow);
							if (total - rowVal < 27 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
								pPackage->iValueGoal = total - rowVal;
								pPackage->iCalcResults = CalculateWinningState(total - rowVal);
								pPackage->iPriority = 100 - (rowVal + (26 - total));
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
			} else if (total >= trackInfo[5 - iTrack].iValue || trackInfo[5 - iTrack].iValue >= 27) {
				for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
					pPackage->iDeckCard = iDeckCard;
					int32_t faceValue = pNPCHand[iDeckCard]->data.iFaceValue;
					if (faceValue <= 10) {
						int32_t iRow;
						for (iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
						}
						pPackage->iRow = iRow;
						if (IsValidCardPlacement(iTrack, iRow, iDeckCard, true) &&
							CalculateWinningState(trackInfo[iTrack].iValue + faceValue) < 1) {
							pPackage->iValueGoal = trackInfo[iTrack].iValue + faceValue;
							pPackage->iCalcResults = CalculateWinningState(trackInfo[iTrack].iValue + faceValue);
							pPackage->iPriority = trackInfo[iTrack].iValue + faceValue;
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
					} else if (pNPCHand[iDeckCard]->data.iFaceValue == 14) {
						for (int32_t iRow = 0; iRow < 7; ++iRow) {
							pPackage->iRow = iRow;
							int32_t rowVal = GetRowValue(iTrack, iRow);
							if (CalculateWinningState(total + 2 * rowVal) < 1 &&
								IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
								pPackage->iValueGoal = total + 2 * rowVal;
								pPackage->iCalcResults = CalculateWinningState(total + 2 * rowVal);
								pPackage->iPriority = total + 2 * rowVal;
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
			} else {
				for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
					pPackage->iDeckCard = iDeckCard;
					int32_t faceValue = pNPCHand[iDeckCard]->data.iFaceValue;
					if (faceValue <= 10) {
						int32_t iRow;
						for (iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
						}
						pPackage->iRow = iRow;
						int32_t newValue = trackInfo[iTrack].iValue + faceValue;
						if (newValue < 27 && newValue > trackInfo[5 - iTrack].iValue) {
							pPackage->iPriority = 100 - (newValue - trackInfo[5 - iTrack].iValue);
							pPackage->iActionType = 0;
						} else if (newValue < 27 && newValue == trackInfo[5 - iTrack].iValue) {
							pPackage->iPriority = 50;
							pPackage->iActionType = 0;
						} else if (newValue < 27) {
							pPackage->iPriority = 25 - (26 - newValue);
							pPackage->iActionType = 0;
						}
						if (IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
							pPackage->iValueGoal = newValue;
							pPackage->iCalcResults = CalculateWinningState(newValue);
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
					} else if (faceValue == 12) {
						int32_t diff = trackInfo[5 - iTrack].iValue - total;
						for (int32_t iRow = 0; iRow < 7; ++iRow) {
							pPackage->iRow = iRow;
							if (!pCardTracks[5 - iTrack][iRow].iSize) {
								break;
							}
							int32_t iValue = GetRowValue(5 - iTrack, iRow);
							if (iValue > diff) {
								int32_t iRemaining = trackInfo[5 - iTrack].iValue - iValue;
								if (iRemaining <= 21) {
									pPackage->iPriority = total - iRemaining + 75;
								} else {
									pPackage->iPriority = 100 - (total - iRemaining);
								}
								if (IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
									pPackage->iValueGoal = total;
									pPackage->iCalcResults = CalculateWinningState(total);
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
					} else if (iDiffLevel >= 1 && faceValue == 15) {
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
							} else {
								for (int32_t i = 0; i < 6; ++i) {
									decreaseby[i] =
										GetSumCardsB44WithSuit(i, pCardTracks[iTrack][iRow][0]->data.iFaceValue);
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
									if (trackInfo[i + 3].iValue > 20 && trackInfo[i + 3].iValue < 27 &&
										trackInfo[i + 3].iValue - decreaseby[i + 3] < 21) {
										iPriority -= 25 - (21 - (trackInfo[i + 3].iValue - decreaseby[i + 3]));
									}
									if (CalculateWinningState(trackInfo[i + 3].iValue) >= 0) {
										if (trackInfo[i + 3].iValue < 27 &&
											trackInfo[i + 3].iValue - decreaseby[i + 3] < 21) {
											iPriority -= 21 - trackInfo[i + 3].iValue - decreaseby[i + 3];
										}
										if (trackInfo[i + 3].iValue < 27 && diff[i] < 0 &&
											!CalculateWinningState(trackInfo[i].iValue - decreaseby[i])) {
											iPriority -= diff[i] + 25;
										}
										if (CalculateWinningState(trackInfo[i + 3].iValue) > 0 &&
											CalculateWinningState(trackInfo[i + 3].iValue - decreaseby[i + 3]) <= 0) {
											iPriority += trackInfo[i + 3].iValue - decreaseby[i + 3];
										}
									}
									if (CalculateWinningState(trackInfo[i + 3].iValue) > 0 && decreaseby[i + 3] > 0) {
										int32_t newvalue = trackInfo[i + 3].iValue - decreaseby[i + 3];
										if (!CalculateWinningState(newvalue)) {
											iPriority += newvalue + 25;
										} else if (CalculateWinningState(newvalue) < 0) {
											iPriority += newvalue;
										} else {
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
							} else {
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
		} else {
			for (uint32_t iDeckCard = 0; iDeckCard < pNPCHand.iSize; ++iDeckCard) {
				pPackage->iDeckCard = iDeckCard;
				if (pNPCHand[iDeckCard]->data.iFaceValue <= 10) {
					int32_t iRow;
					for (iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
					}
					pPackage->iRow = iRow;
					if (iRow != 7) {
						int32_t ival = trackInfo[iTrack].iValue + pNPCHand[iDeckCard]->data.iFaceValue;
						if (CalculateWinningState(ival) < 1 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
							pPackage->iValueGoal = ival;
							pPackage->iCalcResults = CalculateWinningState(ival);
							pPackage->iPriority = ival;
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
				} else if (pNPCHand[iDeckCard]->data.iFaceValue == 14) {
					for (int32_t iRow = 0; iRow < 7 && pCardTracks[iTrack][iRow].iSize; ++iRow) {
						pPackage->iRow = iRow;
						if (pCardTracks[iTrack][iRow].iSize > 3) {
							continue;
						}
						int32_t ival = trackInfo[iTrack].iValue + GetRowValue(iTrack, iRow);
						if (CalculateWinningState(ival) < 1 && IsValidCardPlacement(iTrack, iRow, iDeckCard, true)) {
							pPackage->iValueGoal = ival;
							pPackage->iCalcResults = CalculateWinningState(ival);
							pPackage->iPriority = ival + (iDiffLevel != 2 ? 1 : -5);
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
					int32_t ival = trackInfo[iTrack].iValue - GetRowValue(iTrack, iRow);
					if (!CalculateWinningState(trackInfo[iTrack].iValue)) {
						pPackage->iPriority = 26 - ival + 25;
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
					} else if (CalculateWinningState(trackInfo[iTrack].iValue) < 0) {
						pPackage->iPriority = 26 - ival;
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
						int32_t sum = pCardTracks[iTrack][iRow - 1].GetAt(0)->data.iFaceValue +
							pCardTracks[iTrack][iRow - 2].GetAt(0)->data.iFaceValue;
						if (sum < 8) {
							if (pCardTracks[iTrack][iRow - 1].GetAt(0)->data.iFaceValue >
								pCardTracks[iTrack][iRow - 2].GetAt(0)->data.iFaceValue) {
								pPackage->iRow = iRow - 1;
								pPackage->iPriority = 35 - sum;
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
						} else if (sum > 16) {
							if (pCardTracks[iTrack][iRow - 1].GetAt(0)->data.iFaceValue <
								pCardTracks[iTrack][iRow - 2].GetAt(0)->data.iFaceValue) {
								pPackage->iRow = iRow - 1;
								pPackage->iPriority = sum + 25;
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
					}
					break;
				}
				case 14: {
					int32_t ival = trackInfo[iTrack].iValue + GetRowValue(iTrack, iRow);
					if (CalculateWinningState(ival) == 1) {
						pPackage->iPriority = ival - 1;
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
		int32_t highestcard = 0;
		for (uint32_t i = 0; i < pNPCHand.iSize; ++i) {
			if (pNPCHand[i]->data.iFaceValue > pNPCHand[highestcard]->data.iFaceValue) {
				highestcard = i;
			}
		}
		pPackage->iDeckCard = highestcard;
		pPackage->iActionType = 6;
		pPackage->iPriority = highestcard;
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
			} else if (iDiffLevel < 3) {
				pPackage->iPriority = 2 * trackInfo[iTrack].iValue - 52;
			} else {
				int32_t numKings = 0;
				for (uint32_t i = 0; i < pNPCDeck.iSize; ++i) {
					if (pNPCDeck[i]->data.iFaceValue == 12) {
						++numKings;
					}
				}
				pPackage->iPriority = 2 * trackInfo[iTrack].iValue - 52 - 5 * numKings;
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
	// The exe counts the packages that share the highest priority and never uses the count.
	int32_t highestpriority = (*pPackageList)[0]->iPriority;
	int32_t highestindex = 0;
	for (uint32_t i = 0; i < pPackageList->iSize; ++i) {
		if ((*pPackageList)[i]->iPriority != highestpriority) {
			break;
		}
		++highestindex;
	}
	CaravanAIPackage returnPackage = *(*pPackageList)[0];
	while (!pPackageList->IsEmpty()) {
		pPackage = *static_cast<CaravanAIPackage**>(pPackageList->GetTail());
		pPackageList->RemoveLast();
		ourFree(pPackage);
	}
	if (pPackageList) {
		delete pPackageList;
	}
	*apResult = returnPackage;
	return apResult;
}

int32_t CaravanMenu::GetValueOfCardValueInTrack(int32_t aiTrack, int32_t aiFaceValue) {
	int32_t iRow = 0;
	int32_t iCount = 0;
	while (iRow < 7 && pCardTracks[aiTrack][iRow].iSize) {
		if (pCardTracks[aiTrack][iRow].operator[](0)->data.iFaceValue == aiFaceValue) {
			++iCount;
		}
		++iRow;
	}
	return iCount * aiFaceValue;
}

int32_t CaravanMenu::GetSumCardsB44WithSuit(int32_t a1, uint32_t a2) {
	int32_t v5 = 0;
	int32_t v6 = 0;
	while (v5 < 7 && pCardTracks[a1][v5].iSize) {
		if (pCardTracks[a1][v5].operator[](0)->data.iCardSuit == a2) {
			v6 += pCardTracks[a1][v5].operator[](0)->data.iFaceValue;
		}
		++v5;
	}
	return v6;
}

int32_t CaravanMenu::GetMenuID_() {
	return 1083;
}

// The NPC matches the player's ante up to its limit: its funds times fBetPercentage, raised by the player's barter
// skill in percent, capped at its funds. The exe reads pMe again for every access.
void CaravanMenu::ItemSelectCallback(int32_t aiValue) {
	int32_t iRaise = aiValue - pMe->iPlayerAnte;
	pMe->iPlayerAnte += aiValue;
	pMe->pTiles[TI_ANTE_PLAYER_BET]->Poke(iUT_Value, 1.0f);
	pMe->pTiles[TI_ANTE_PLAYER_BET]->Set(iUT_Value, pMe->iPlayerAnte);
	pMe->SetFlag(CAF_DISABLE_ALLCONTROLS, false);
	double dBaseRaise = static_cast<double>(pMe->iNPCTotalFunds) * pMe->fBetPercentage;
	double dBaseRaise2 = static_cast<double>(pMe->iNPCTotalFunds) * pMe->fBetPercentage;
	// The exe adjusts pPlayer to its ActorValueOwner without a null check.
	int32_t iMaxRaise = static_cast<int32_t>(pPlayer->ActorValueOwner::GetClampedActorFloatValue(ActorValue::BARTER) /
			100.0 * dBaseRaise2 +
		dBaseRaise);
	iMaxRaise = BSMIN(iMaxRaise, pMe->iNPCTotalFunds);
	if (pMe->iPlayerAnte > pMe->iNPCAnte) {
		pMe->iNPCAnte = BSMIN(iMaxRaise, pMe->iPlayerAnte);
		pMe->pTiles[TI_ANTE_NPC_BET]->Poke(iUT_Value, 1.0f);
		pMe->pTiles[TI_ANTE_NPC_BET]->Set(iUT_Value, pMe->iNPCAnte);
	}
	iRaise += BSMAX(BSMIN(pMe->iPlayerAnte, iMaxRaise) - pMe->iNPCAnte, 0);
	pMe->UpdateCaravanFlags();
	pMe->UpdateTileAlpha(static_cast<TileIndex>(-1));
	pMe->UpdateBettingUI(iMaxRaise, iRaise);
}

void CaravanMenu::Close() {
	if (CaravanMenu::pMe) {
		CaravanMenu::pMe->eGameState = CaravanMenu::CA_CLOSING_MENU;
		int32_t MenuID = CaravanMenu::GetMenuID_();
		if (Menu::GetMenuByClass(MenuID)) {
			if (CaravanMenu::pMe->eActiveMenu == CaravanMenu::CA_DECK_MENU) {
				BSSimpleList<TESCaravanCard*>* pAct = pPlayer->GetCaravanCards2();
				BSSimpleList<TESCaravanCard*>* pInact = pPlayer->GetCaravanCards1();
				pAct->RemoveAll();
				pInact->RemoveAll();
				for (int32_t i = 0; i < CaravanMenu::pMe->pDeckArray.iSize; ++i) {
					if (CaravanMenu::pMe->pDeckArray.operator[](i)->bIsActive) {
						pAct->AddHead(CaravanMenu::pMe->pDeckArray.operator[](i));
					} else {
						pInact->AddHead(CaravanMenu::pMe->pDeckArray.operator[](i));
					}
				}
			}
		}
	}
}

void CaravanMenu::CancelConfirmFunc() {
	Controls* controls;
	if (Interface::GetMessageMenuResult()) {
		CaravanMenu::pMe->SetFlag(CAF_DISABLE_ALLCONTROLS, 0);
	} else {
		CaravanMenu::pMe->SetFlag(static_cast<enumFlags>(131072), 1);
		controls = appMain->GetControls();
		controls->ClearUserActions();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe's cleanup-only C++ EH frame is left out.
void CaravanMenu::Draw3DElements() {
	NiCamera* v6;
	NiRenderer* Renderer_ = NiRenderer::GetRenderer();
	Renderer_->ClearBuffer(0, 4u);
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	BSCullingProcess* a1 = WorldSceneGraph->pCuller;
	a1->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
	NiCamera* apCamera = CaravanMenu::sp3DCamera.m_pObject;
	NiCamera::HandleEyeFinityFOVScale(apCamera, 0.15f);
	InterfaceManager* v1 = InterfaceManager::Instance();
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiRefObject>, apMemory, &NiPointer<NiRefObject>::Destruct,
		static_cast<NiRefObject*>(v1->Get3dAccumulator()));
	NiAccumulator* afScale = static_cast<NiAccumulator*>(apMemory.m_pObject);
	a1->SetAccumulator(afScale);
	if (apCamera) {
		v6 = apCamera;
	} else {
		v6 = WorldSceneGraph->GetCamera();
	}
	ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
	BSShaderUtil::AccumulateScene(v6, ShadowSceneNode_, *a1);
	BSShaderAccumulator* v3 = static_cast<BSShaderAccumulator*>(apMemory.m_pObject);
	BSShaderUtil::RenderScene(v6, v3, 0);
	a1->SetAccumulator(0);
	apMemory.operator=(static_cast<NiRefObject*>(0));
	a1->PopCullMode();
}

// The exe's cleanup-only C++ EH frame is left out.
Tile* CaravanMenu::Create(Actor* apNPCChallenger, TESCaravanDeck* apNPCDeck, int32_t aiDiffLevel, float afBetPercentage,
	bool abDebugMode) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	Tile* pold_menu_tile = Menu::GetMenuByClass(1083);
	g_caravanMenu_InterfaceManager = InterfaceManager::Instance();
	int32_t v5 = pPlayer->GetCaravanCards1()->ItemsInList();
	int32_t iCardTotal = pPlayer->GetCaravanCards2()->ItemsInList() + v5;
	if (iCardTotal >= 30) {
		int32_t v8;
		if (pold_menu_tile) {
			delete pold_menu_tile;
		}
		Tile* ptile = g_caravanMenu_InterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\caravan_menu.xml");
		CaravanMenu::pMe = static_cast<CaravanMenu*>(ptile->GetMenu());
		if (CaravanMenu::pMe && (v8 = CaravanMenu::pMe->GetClass(), v8 == CaravanMenu::GetMenuID_())) {
			CaravanMenu::pMe->pChallenger = apNPCChallenger;
			BSSimpleList<TESCaravanCard*>* Target = apNPCDeck->pDeck;
			CaravanMenu::pMe->pDeckParam = Target;
			CaravanMenu::pMe->iDiffLevel = aiDiffLevel;
			CaravanMenu::pMe->fBetPercentage = afBetPercentage;
			CaravanMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
			if (CaravanMenu::pMe->IsWellFormedMenu()) {
				if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
					float time = Tile::GetMaximumDepth();
					ptile->Set(4013, time, 1);
				}
				CaravanMenu::pMe->PrepareCaravanData();
				CaravanMenu::pMe->pTiles[15]->Set(Tile::kTileValue_User0,
					static_cast<float>(CaravanMenu::pMe->iTotalCards), 0);
				CaravanMenu::pMe->pTiles[15]->Set(CaravanMenu::iUT_CurrentValue, 12.0f, 0);
				int32_t v10 = Tile::TextToTrait("_SetInCode");
				CaravanMenu::pMe->pTiles[15]->Poke(v10, 1.0f);
				CaravanMenu::bDebugAI = 0;
				CaravanMenu::pMe->LoadNifs2();
				CaravanMenu::pMe->SetupCurrency();
				CaravanMenu::pMe->SetFlag(CaravanMenu::CAF_DISABLE_ALLCONTROLS, 0);
				CaravanMenu::pMe->PrepForVisibility(0);
				InterfaceManager* v11 = InterfaceManager::Instance();
				if (!v11->xTutorialManager.HasBeenShown(Interface::TUT_CARAVAN_BET)) {
					TESForm* FormByEditorID = TESForm::GetFormByEditorID("HelpCaravanBetting");
					BGSMessage* helpForm = dynamic_cast<BGSMessage*>(FormByEditorID);
					if (helpForm) {
						CaravanMenu::pMe->bTutorial = TutorialMenu::Create(helpForm, 0);
						InterfaceManager* v13 = InterfaceManager::Instance();
						v13->xTutorialManager.FlagAsShown(Interface::TUT_CARAVAN_BET);
						CaravanMenu::pMe->iTutorial = 30;
					}
				}
				return ptile;
			} else {
				Warning("MENUS: Caravan Menu Creation Failed... Are your menu and art resources up to date?");
				CaravanMenu::Close();
				return 0;
			}
		} else {
			if (CaravanMenu::pMe) {
				if (CaravanMenu::pMe->pMenu) {
					if (CaravanMenu::pMe) {
						delete CaravanMenu::pMe;
					}
				}
			}
			return 0;
		}
	} else {
		char msg[264];
		BSsprintf(msg, 260, sCardCountText.operator const char*());
		Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", 0, 2.0f,
			0);
		BSAudio* v6 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v6->GetSoundHandleByName(&a1, "UIPopUpMessageGeneral", 289u);
		SoundHandleByName->Play(0);
		return 0;
	}
}

void CaravanAIPackage::Print_WhatIsHeThinking(BSSimpleArray<TESCaravanCard*, 1024>* pNPCHandArray) {
	if (pNPCHandArray) {
		TESCaravanCard* v2 = pNPCHandArray->operator[](iDeckCard);
		InitializationFlow("track:%i row:%i dcard:%i goal:%i calcresult:%i priority:%i card:%s", iTrack, iRow,
			iDeckCard, iValueGoal, iCalcResults, iPriority, v2->GetFormEditorID());
	} else {
		InitializationFlow("track:%i row:%i dcard:%i goal:%i calcresult:%i priority:%i", iTrack, iRow, iDeckCard,
			iValueGoal, iCalcResults, iPriority);
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
