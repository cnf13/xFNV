#include "Fallout/Interface/Menus/LockPickMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/ExperiencePoints.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/stdlib.h"

#include <cmath>

g_iSweetSpotLengthMinArray_t g_iSweetSpotLengthMinArray{&iSweetSpotLengthMinVeryEasy, &iSweetSpotLengthMinEasy,
	&iSweetSpotLengthMinAverage, &iSweetSpotLengthMinHard, &iSweetSpotLengthMinVeryHard};
g_iSweetSpotLengthMaxArray_t g_iSweetSpotLengthMaxArray{&iSweetSpotLengthMaxVeryEasy, &iSweetSpotLengthMaxEasy,
	&iSweetSpotLengthMaxAverage, &iSweetSpotLengthMaxHard, &iSweetSpotLengthMaxVeryHard};
g_iConcentricLengthMinArray_t g_iConcentricLengthMinArray{&iConcentricLengthMinVeryEasy, &iConcentricLengthMinEasy,
	&iConcentricLengthMinAverage, &iConcentricLengthMinHard, &iConcentricLengthMinVeryHard};
g_iConcentricLengthMaxArray_t g_iConcentricLengthMaxArray{&iConcentricLengthMaxVeryEasy, &iConcentricLengthMaxEasy,
	&iConcentricLengthMaxAverage, &iConcentricLengthMaxHard, &iConcentricLengthMaxVeryHard};
NiPointer<NiNode> LockPickMenu::spCameraRoot;
NiPointer<NiCamera> LockPickMenu::spCamera;
LockLevelMaxVeryEasy_t LockLevelMaxVeryEasy = {static_cast<int32_t>(getLockLevelName(LL_VERYEASY)),
	static_cast<int32_t>(getLockLevelName(LL_EASY)), static_cast<int32_t>(getLockLevelName(LL_AVERAGE)),
	static_cast<int32_t>(getLockLevelName(LL_HARD)), static_cast<int32_t>(getLockLevelName(LL_VERYHARD)), 101};

LockPickMenu::LockPickMenu()
	: eLockState(LockPickMenu::LS_INTRO_ANIM), iLockpickSkill(pPlayer->GetClampedActorValue(ActorValue::LOCKPICK)),
	  fLockpickSkill(static_cast<float>(iLockpickSkill)), uiPrevIdleTick(0), fLockAngle(0.0f),
	  fPickHealth(fLockpickSkill * fLockpickBonusHealth.Float() + 1.0), bForceLockSuccess(0), bShowingTutorial(0),
	  pIntroAnim(0), pLockRotateAnim(0), pPickIntroAnim(0), pPickBendAnim(0), pPickBreakAnim(0) {
	LockPickMenu::iUT_Value = Tile::AddUserTrait("_Value", -1);
	LockPickMenu::iUT_X = Tile::AddUserTrait("_x", -1);
}

LockPickMenu::~LockPickMenu() {
	BSShaderManager::GetShadowSceneNode(1)->DetachChild(LockPickMenu::spCameraRoot.m_pObject);
	LockPickMenu::spCameraRoot.m_pObject->DetachAllChildren();
	LockPickMenu::spCameraRoot = static_cast<NiNode*>(0);
	LockPickMenu::spCamera = static_cast<NiCamera*>(0);
	LockPickMenu::pMe = 0;
	BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
}

void LockPickMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (((uint32_t)aiID) < 16) {
		pTiles[aiID] = apTile;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void LockPickMenu::DoClick(int32_t a1, Tile* a2) {
	FNVD_INLINED_LOCAL(BSStringT<char>, strtemp_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStringT<char>, strtemp, std::nullopt, std::nullopt);
	int32_t fdoPunishment;
	int32_t itweakIncrAmount = 5;
	switch (a1) {
	case 3:
	case 5: {
		int32_t iskillLevelRange = LockLevelMaxVeryEasy[eLockLevel + 1] - LockLevelMaxVeryEasy[eLockLevel];
		float afA = static_cast<double>(iLockpickSkill - LockLevelMaxVeryEasy[eLockLevel]) /
			(static_cast<double>(iskillLevelRange) + static_cast<double>(iskillLevelRange));
		float a12 = BSMIN(afA, 1.0f);
		float flockLevelRatio = BSMAX(a12, 0.0f);
		int32_t idelta = itweakIncrAmount * (2 * (a1 != 5) - 1);
		::new (&strtemp) BSStringT<char>;
		g_iconcentricLength += idelta;
		iTotalSweetSpotRadius += idelta / 2;
		strtemp.SPrintF("%d", g_iconcentricLength);
		pTiles[4]->Set(4036, strtemp.pString, 1);
		g_iSweetSpotLengthMinArray[eLockLevel]->operator=(g_iSweetSpotLengthMinArray[eLockLevel]->Int() +
			static_cast<int32_t>(static_cast<double>(idelta) * (1.0 - flockLevelRatio)));
		g_iSweetSpotLengthMaxArray[eLockLevel]->operator=(g_iSweetSpotLengthMaxArray[eLockLevel]->Int() +
			static_cast<int32_t>(static_cast<double>(idelta) * flockLevelRatio));
		for (int32_t imeterCt = 0; imeterCt < 6; ++imeterCt) {
			Tile* pcurrTile = pTiles[14]->GetChildByID(imeterCt + 17);
			float v14 = pcurrTile->GetFloat(4017) + static_cast<double>(idelta);
			pcurrTile->Set(4017, v14, 1);
			float v13 = pcurrTile->GetFloat(4001) - static_cast<double>(idelta / 2);
			pcurrTile->Set(4001, v13, 1);
		}
		strtemp.~BSStringT<char>();
		break;
	}
	case 6:
	case 8: {
		int32_t iskillLevelRange_2 = LockLevelMaxVeryEasy[eLockLevel + 1] - LockLevelMaxVeryEasy[eLockLevel];
		float v12 = static_cast<double>(iLockpickSkill - LockLevelMaxVeryEasy[eLockLevel]) /
			(static_cast<double>(iskillLevelRange_2) + static_cast<double>(iskillLevelRange_2));
		float a1a = BSMIN(v12, 1.0f);
		float flockLevelRatio_2 = BSMAX(a1a, 0.0f);
		int32_t idelta_2 = itweakIncrAmount * (2 * (a1 != 8) - 1);
		::new (&strtemp_2) BSStringT<char>;
		g_isweetSpotLength += idelta_2;
		iTotalSweetSpotRadius += idelta_2;
		strtemp_2.SPrintF("%d", g_isweetSpotLength);
		pTiles[7]->Set(4036, strtemp_2.pString, 1);
		g_iConcentricLengthMinArray[eLockLevel]->operator=(g_iConcentricLengthMinArray[eLockLevel]->Int() +
			static_cast<int32_t>(static_cast<double>(idelta_2) * (1.0 - flockLevelRatio_2)));
		g_iConcentricLengthMaxArray[eLockLevel]->operator=(g_iConcentricLengthMaxArray[eLockLevel]->Int() +
			static_cast<int32_t>(static_cast<double>(idelta_2) * flockLevelRatio_2));
		for (int32_t imeterCt_2 = 1; imeterCt_2 < 6; ++imeterCt_2) {
			Tile* pcurrTile_2 = pTiles[14]->GetChildByID(imeterCt_2 + 17);
			float v11 = pcurrTile_2->GetFloat(4017) +
				static_cast<double>(imeterCt_2) * 2.0 * static_cast<double>(idelta_2) / 5.0;
			pcurrTile_2->Set(4017, v11, 1);
			float v10 = pcurrTile_2->GetFloat(4001) - static_cast<double>(idelta_2 * imeterCt_2) / 5.0;
			pcurrTile_2->Set(4001, v10, 1);
		}
		strtemp_2.~BSStringT<char>();
		break;
	}
	case 9: {
		*reinterpret_cast<float*>(&fdoPunishment) = 0.0f;
		BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &fdoPunishment);
		BSRandom* Singleton = BSRandom::GetSingleton();
		uint32_t v4 = Singleton->UnsignedInt(100u);
		int32_t a22 = LockLevelMaxVeryEasy[eLockLevel];
		int32_t ClampedActorValue = pPlayer->GetClampedActorValue(ActorValue::LOCKPICK);
		if (v4 >= GamePlayFormulas::GetForceLockChance(ClampedActorValue, a22)) {
			if (pLockTarget || *reinterpret_cast<float*>(&fdoPunishment) != 0.0) {
				uint32_t* p_iNumTries = &pLockTarget->GetLock()->iNumTries;
				++*p_iNumTries;
				pLockTarget->AddLockChange();
				const char* v6 = sLockBroken.String();
				Interface::ShowMessage(v6, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
					"UILockpickingForceFail", 2.0f, 0);
				LockPickMenu::Close();
			} else {
				pTiles[11]->Set(4036, "LOCK BROKEN!  :(", 1);
				pTiles[11]->Set(4003, 1);
				eLockState = LockPickMenu::LS_RESETTING_LOCK;
			}
		} else {
			bForceLockSuccess = 1;
			fMaxLockAngle = 90.0f;
			eLockState = LockPickMenu::LS_TURNING_LOCK;
		}
		break;
	}
	case 10: {
		LockPickMenu::Close();
		break;
	}
	default: {
		return;
	}
	}
}

void LockPickMenu::DoEnter(int32_t aiID, Tile* apTarget) {
	InterfaceManager* singleton;
	Tile* processLevel;
	if (aiID == 13) {
		singleton = InterfaceManager::Instance();
		processLevel = singleton->GetCursorTile();
		processLevel->Set(4009, (float)0.0, (bool)1);
	}
}

void LockPickMenu::DoLeave(int32_t aiID, Tile* apTarget) {
	InterfaceManager* singleton;
	Tile* processLevel;
	if (aiID == 13) {
		singleton = InterfaceManager::Instance();
		processLevel = singleton->GetCursorTile();
		processLevel->Set(4009, 255.0f, (bool)1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void LockPickMenu::DoIdle() {
	static int32_t iLockShakeDirection = 1;
	FNVD_INLINED_LOCAL(BSSoundHandle, v72, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v73, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v74, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v75, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v76, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v77, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v78, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v79, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	int32_t ielaspedMillis = 0;
	float fxAxisRatio = 1.0f;
	// The function static lockpickSound (the PDB's name; IDA's LockPickMenu::kCurrentSound), guarded by bit 1 of
	// lockpickSound_guard and destroyed by the exe's atexit thunk 0xFD8000.
	static BSSoundHandle lockpickSound;
	Interface::enumMenus v1 = static_cast<Interface::enumMenus>(GetClass());
	bool bisMenuOnTop = Interface::IsTopMenuID(v1);
	if (uiPrevIdleTick) {
		ielaspedMillis = appTimer.iLastTime - uiPrevIdleTick;
		if (eprevLockState == eLockState) {
			ftotalStateSecs_1 = static_cast<double>(ielaspedMillis) / 1000.0 + ftotalStateSecs_1;
		} else {
			ftotalStateSecs_1 = 0.0f;
		}
	} else {
		ielaspedMillis = 0;
		ftotalStateSecs_1 = 0.0f;
		eprevLockState = 0;
	}
	uiPrevIdleTick = appTimer.iLastTime;
	eprevLockState = eLockState;
	if (!bShowingTutorial) {
		goto LABEL_13;
	}
	if (bisMenuOnTop) {
		InterfaceManager* v2;
		Interface::TutorialMessageID X;
		X = LockPickMenu::GetTutorialMessageID();
		v2 = InterfaceManager::Instance();
		if (v2->xTutorialManager.HasBeenShown(X)) {
			bShowingTutorial = 0;
		LABEL_13:;
			if (!bisMenuOnTop) {
				InterfaceManager* v3 = InterfaceManager::Instance();
				v3->pCursor->Set(4009, 255.0f, 1);
			}
			int32_t ixAxis = 0;
			const int32_t cimaxAxisValue = 0x7FFF;
			InterfaceManager::Instance();
			if (InterfaceManager::Has360Controller()) {
				Controls* Controls_ = appMain->GetControls();
				ixAxis = Controls_->GetControllerValue(0, 9);
				if (NiAbs(static_cast<float>(ixAxis)) <= 8689.0) {
					ixAxis = 0;
				}
				fxAxisRatio = static_cast<double>(-ixAxis) / static_cast<double>(cimaxAxisValue);
			}
			switch (eLockState) {
			case LockPickMenu::LS_INTRO_ANIM: {
				if (pIntroAnim) {
					if (pPickIntroAnim) {
						FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1, nullptr, ftotalStateSecs_1, 1, 0);
						FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2, nullptr, ftotalStateSecs_1, 1, 0);
						NiAVObject* v5 = LockPickMenu::spCameraRoot.m_pObject->GetAt(0);
						v5->Update(UpdateData1);
						if (ftotalStateSecs_1 == 0.0 ||
							(!pLockTarget || pTiles[1]->GetFloat(LockPickMenu::iUT_Value) > 0.0)) {
							NiAVObject* v6 = LockPickMenu::spCameraRoot.m_pObject->GetAt(1u);
							v6->Update(UpdateData2);
						}
						if (ftotalStateSecs_1 == 0.0) {
							if (pLockTarget) {
								if (!(pTiles[1]->GetFloat(LockPickMenu::iUT_Value) <= 0.0)) {
									BSAudio* v8 = &BSAudio::QInstance();
									BSSoundHandle* SoundHandleByName =
										v8->GetSoundHandleByName(&arHandle, "UILockpickingEnter", 289u);
									SoundHandleByName->Play(0);
								} else {
									const char* v7 = sOutOfLockpicks.String();
									Interface::ShowMessage(v7, 0,
										"Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", 0, 2.0f,
										0);
								}
							}
						}
						if (static_cast<double>(ftotalStateSecs_1) >= pIntroAnim->m_fEndKeyTime) {
							NiAVObject* v9 = LockPickMenu::spCameraRoot.m_pObject->GetAt(0);
							NiControllerManager* plockAnimManager =
								static_cast<NiControllerManager*>(v9->GetController(&NiControllerManager::ms_RTTI));
							NiNode* v10 = static_cast<NiNode*>(LockPickMenu::spCameraRoot.m_pObject->GetAt(0));
							NiNode* v11 = static_cast<NiNode*>(v10->GetAt(0));
							NiAVObject* pscrewdriver = v11->GetAt(2u);
							NiPoint3 origtrans = pscrewdriver->GetTranslate();
							eLockState = LockPickMenu::LS_IDLE;
							plockAnimManager->DeactivateSequence(pIntroAnim, 0.0f);
							plockAnimManager->ActivateSequence(pLockRotateAnim, 0, 1, 1.0f, 0.0f, 0);
							ppickAnimManager->DeactivateSequence(pPickIntroAnim, 0.0f);
							ppickAnimManager->ActivateSequence(pPickBendAnim, 0, 1, 1.0f, 0.0f, 0);
						}
					}
				}
				break;
			}
			case LockPickMenu::LS_PICK_INTRO_ANIM: {
				if (pPickIntroAnim) {
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData, nullptr, ftotalStateSecs_1, 1, 0);
					NiAVObject* v12 = LockPickMenu::spCameraRoot.m_pObject->GetAt(1u);
					v12->Update(UpdateData);
					if (static_cast<double>(ftotalStateSecs_1) >= pPickIntroAnim->m_fEndKeyTime) {
						eLockState = LockPickMenu::LS_IDLE;
						ppickAnimManager->DeactivateSequence(pPickIntroAnim, 0.0f);
						ppickAnimManager->ActivateSequence(pPickBendAnim, 0, 1, 1.0f, 0.0f, 0);
					}
				}
				break;
			}
			case LockPickMenu::LS_IDLE: {
				InterfaceManager* v13;
				Interface::TutorialMessageID Xa;
				// The exe tests the shared `return true` stub 0x5D4A40 here.
				if (!ReturnTrue() ||
					(Xa = LockPickMenu::GetTutorialMessageID(),
						(v13 = InterfaceManager::Instance(), v13->xTutorialManager.HasBeenShown(Xa)))) {
					if (bisMenuOnTop) {
						if (!pLockTarget || pTiles[1]->GetFloat(LockPickMenu::iUT_Value) > 0.0) {
							if ((InterfaceManager::Has360Controller() && fxAxisRatio > 0.0) ||
								(!InterfaceManager::Has360Controller() && GetPCInput(1))) {
								BSAudio* v15 = &BSAudio::QInstance();
								BSSoundHandle* v69 =
									v15->GetSoundHandleByName(&apResult, "UILockpickingCylinderTurn", 289u);
								v69->Play(0);
								eLockState = LockPickMenu::LS_TURNING_LOCK;
							}
							RotateLockpick();
						}
						if (fLockAngle > 0.0) {
							float fprevLockAngle = fLockAngle;
							fLockAngle = fLockAngle - static_cast<double>(90 * ielaspedMillis) / 1000.0;
							fLockAngle = BSMAX(fLockAngle, 0.0f);
							RotateLock(0.0f);
							if (fLockAngle < 30.0 && fprevLockAngle > 30.0) {
								BSAudio* v16 = &BSAudio::QInstance();
								BSSoundHandle* v68 =
									v16->GetSoundHandleByName(&v79, "UILockpickingCylinderSqueakA", 257u);
								v68->Play(0);
							}
							if (fLockAngle < 60.0 && fprevLockAngle > 60.0) {
								BSAudio* v17 = &BSAudio::QInstance();
								BSSoundHandle* v67 =
									v17->GetSoundHandleByName(&v78, "UILockpickingCylinderSqueakB", 257u);
								v67->Play(0);
							}
						}
					}
				} else {
					int32_t a1 = LockPickMenu::GetClassStatic();
					Interface::TutorialMessageID a4 = LockPickMenu::GetTutorialMessageID();
					InterfaceManager* v14 = InterfaceManager::Instance();
					bShowingTutorial = v14->xTutorialManager.ShowMessage(a4, a1, 0);
				}
				break;
			}
			case LockPickMenu::LS_TURNING_LOCK: {
				if (bForceLockSuccess ||
					(bisMenuOnTop &&
						((!InterfaceManager::Has360Controller() || !(fxAxisRatio <= 0.0)) &&
							(InterfaceManager::Has360Controller() || GetPCInput(0))))) {
					float fprevLockAngle_2 = fLockAngle;
					if (bForceLockSuccess) {
						fxAxisRatio = 1.0f;
					}
					if (fMaxLockAngle >= static_cast<double>(fLockAngle)) {
						fLockAngle = static_cast<double>(90 * ielaspedMillis) * fxAxisRatio / 1000.0 + fLockAngle;
						fLockAngle = BSMIN(fLockAngle, fMaxLockAngle);
					}
					RotateLock(0.0f);
					if (InterfaceManager::Has360Controller()) {
						Controls* v19 = appMain->GetControls();
						v19->Rumble(0.1f, 0.0f, 250, 125, PERIODIC_SAWTOOTH, 1, 0);
					}
					if (fprevLockAngle_2 < 30.0 && fLockAngle > 30.0) {
						BSAudio* v20 = &BSAudio::QInstance();
						BSSoundHandle* v66 = v20->GetSoundHandleByName(&v77, "UILockpickingCylinderSqueakA", 257u);
						v66->Play(0);
					}
					if (fprevLockAngle_2 < 60.0 && fLockAngle > 60.0) {
						BSAudio* v21 = &BSAudio::QInstance();
						BSSoundHandle* v65 = v21->GetSoundHandleByName(&v76, "UILockpickingCylinderSqueakB", 257u);
						v65->Play(0);
					}
					if (fMaxLockAngle <= static_cast<double>(fLockAngle)) {
						if (!(fLockAngle >= 90.0)) {
							BSAudio* v25 = &BSAudio::QInstance();
							BSSoundHandle* v62 = v25->GetSoundHandleByName(&v74, "UILockpickingCylinderStop", 257u);
							v62->Play(0);
							BSAudio* v26 = &BSAudio::QInstance();
							BSSoundHandle* v61 = v26->GetSoundHandleByName(&v73, "UILockpickingPickTensionLPM", 1u);
							lockpickSound.operator=(*v61);
							lockpickSound.Play(1);
							eLockState = LockPickMenu::LS_DAMAGING_PICK;
						} else {
							if (pLockTarget) {
								MiscStatManager::Increment(MiscStatManager::MS_LOCKS_PICKED);
								if (!pLockTarget->GetLock()->iTimesUnlocked) {
									int32_t ExperiencePoints_ =
										ExperiencePoints::GetExperiencePoints(ExperiencePoints::ACTIVITY_PICK_LOCK,
											eLockLevel);
									pPlayer->RewardExperience(ExperiencePoints_);
									uint32_t* p_iTimesUnlocked = &pLockTarget->GetLock()->iTimesUnlocked;
									++*p_iTimesUnlocked;
									pLockTarget->AddLockChange();
								}
								if (pLockTarget->GetOwner()) {
									TESForm* pform = pLockTarget->GetOwner();
									Actor* pvictim = 0;
									if (pform->GetFormType() == FACT_ID) {
										TESFaction* pfaction = static_cast<TESFaction*>(pform);
										pvictim = ReferenceProcessLists.GetActorRefInHigh(pfaction, 1, 1);
									} else {
										TESNPC* pnpc = static_cast<TESNPC*>(pform);
										pvictim = ReferenceProcessLists.GetActorRefInHigh(pnpc, 1);
									}
									if (pvictim) {
										pPlayer->bAttackOnNextTheft = 1;
										pPlayer->StealAlarm(pvictim, 0, 0, 0, pform);
									} else {
										bool bkarma = 0;
										if (pform && pform->GetFormType() == FACT_ID) {
											TESFaction* pfaction = static_cast<TESFaction*>(pform);
											if (!pfaction->TESFaction::GetIsEvil()) {
												bkarma = 1;
											}
										} else if (pform) {
											TESNPC* pnpc = static_cast<TESNPC*>(pform);
											if (pnpc) {
												// Slot 3 of the ActorValueOwner subobject: GetActorFloatValue.
												float Xb = static_cast<ActorValueOwner*>(pnpc)
															   ->GetActorFloatValue(ActorValue::KARMA);
												TESActorBaseData::ALIGNMENT ealign =
													TESActorBaseData::GetAlignmentForKarma(Xb);
												if (ealign != TESActorBaseData::ALIGNMENT_EVIL &&
													ealign != TESActorBaseData::ALIGNMENT_VERY_EVIL) {
													bkarma = 1;
												}
											}
										}
										if (bkarma) {
											double v23 = fKarmaModStealing.Float();
											pPlayer->RewardKarma(static_cast<int32_t>(v23));
										}
									}
								}
								BSAudio* v24 = &BSAudio::QInstance();
								BSSoundHandle* v63 = v24->GetSoundHandleByName(&v75, "UILockpickingUnlock", 257u);
								v63->Play(0);
								pLockTarget->UnLock();
								if (pLockTarget->GetFormType() == DOOR_ID) {
									pPlayer->btransporting = 1;
								}
								if (pLockTarget->GetObjectReference()) {
									if (pLockTarget->GetObjectReference()->GetFormType() == CONT_ID) {
										InterfaceManager::SetContainerMenuQueued(1);
									}
								}
								pLockTarget->Activate(pPlayer, 0, 0, 1);
								if (pLockTarget->GetFormType() == DOOR_ID) {
									pPlayer->btransporting = 0;
								}
								LockPickMenu::Close();
							} else {
								pTiles[11]->Set(4036, "LOCK OPENED!  :D", 1);
								pTiles[11]->Set(4003, 1);
								if (!pLockTarget) {
									pTiles[1]->Set(LockPickMenu::iUT_Value, 0);
								}
								bCreateNewSweetSpot = 1;
								eLockState = LockPickMenu::LS_RESETTING_LOCK;
							}
						}
						Controls* v27 = appMain->GetControls();
						v27->KillRumble();
					}
				} else {
					Controls* v18 = appMain->GetControls();
					v18->KillRumble();
					eLockState = LockPickMenu::LS_IDLE;
				}
				break;
			}
			case LockPickMenu::LS_DAMAGING_PICK: {
				if (bisMenuOnTop &&
					(((InterfaceManager::Instance(), !InterfaceManager::Has360Controller()) || !(fxAxisRatio <= 0.0)) &&
						((InterfaceManager::Instance(), InterfaceManager::Has360Controller()) || GetPCInput(0)))) {
					char strtemp[32];
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData_2, nullptr, 0.0, 1, 0);
					float felapsedRatio = static_cast<double>(ielaspedMillis) / 1000.0;
					if (fPickBreakSecs.Float() != 0.0) {
						float* v29 = &fPickBreakSecs.Float();
						fPickHealth = fPickHealth - felapsedRatio / *v29;
					}
					fPickHealth = BSMAX(fPickHealth, 0.0f);
					float v60 = static_cast<double>(iLockShakeDirection) * 0.25;
					RotateLock(v60);
					if (InterfaceManager::Has360Controller()) {
						Controls* v30 = appMain->GetControls();
						v30->Rumble(0.5f, 0.0f, 250, 0, CONSTANT, 0, 0);
					}
					if (pPickBendAnim) {
						double BeginKeyTime = pPickBendAnim->GetBeginKeyTime();
						double v58 = 1.0 - fPickHealth;
						double EndKeyTime = pPickBendAnim->m_fEndKeyTime;
						float v56 = (EndKeyTime - pPickBendAnim->GetBeginKeyTime()) * v58 + BeginKeyTime;
						pPickBendAnim->SetPhase(v56, 0);
						LockPickMenu::spCameraRoot.m_pObject->Update(UpdateData_2);
					}
					float v55 =
						pTiles[15]->GetFloat(LockPickMenu::iUT_X) + static_cast<double>(iLockShakeDirection) * 2.5;
					RotateLockpickModel(v55);
					iLockShakeDirection = -iLockShakeDirection;
					float v54 = fPickHealth * 100.0;
					BSsprintf(strtemp, 32, "PICK HEALTH: %d%s", static_cast<int32_t>(BSRoundToNearest(v54, 1.0f)), "%");
					pTiles[12]->Set(4036, strtemp, 1);
					if (fPickHealth <= 0.0) {
						lockpickSound.FadeOutAndRelease(100u);
						BSAudio* v31 = &BSAudio::QInstance();
						BSSoundHandle* v52 = v31->GetSoundHandleByName(&v72, "UILockpickingPickBreak", 257u);
						v52->Play(0);
						if (pLockTarget) {
							pPlayer->RemoveItem(pLockpick, 0, 1, 0, 0, 0, 0, 0, 1, 0);
						}
						float v50 = pTiles[1]->GetFloat(LockPickMenu::iUT_Value) - 1.0;
						pTiles[1]->Set(LockPickMenu::iUT_Value, v50, 1);
						ppickAnimManager->DeactivateSequence(pPickBendAnim, 0.0f);
						ppickAnimManager->ActivateSequence(pPickBreakAnim, 0, 1, 1.0, 0.0f, 0);
						Controls* v32 = appMain->GetControls();
						v32->KillRumble();
						eLockState = LockPickMenu::LS_BREAKING_PICK;
					}
				} else {
					lockpickSound.FadeOutAndRelease(500u);
					Controls* v28 = appMain->GetControls();
					v28->KillRumble();
					eLockState = LockPickMenu::LS_IDLE;
				}
				break;
			}
			case LockPickMenu::LS_BREAKING_PICK: {
				if (pPickBreakAnim) {
					if (bisMenuOnTop) {
						FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData_3, nullptr, ftotalStateSecs_1, 1, 0);
						NiAVObject* v33 = LockPickMenu::spCameraRoot.m_pObject->GetAt(1u);
						v33->Update(UpdateData_3);
						if (static_cast<double>(ftotalStateSecs_1) >= pPickBreakAnim->m_fEndKeyTime) {
							eLockState = LockPickMenu::LS_RESETTING_LOCK;
						}
					}
				}
				break;
			}
			case LockPickMenu::LS_RESETTING_LOCK: {
				if (ftotalStateSecs_1 >= 1.0 && bisMenuOnTop) {
					pTiles[11]->Set(4003, 0);
					pTiles[12]->Set(4036, "PICK HEALTH: 100%", 1);
					fPickHealth = fLockpickSkill * fLockpickBonusHealth.Float() + 1.0;
					fLockAngle = 0.0f;
					RotateLock(0.0f);
					float v48 = pTiles[14]->GetFloat(4017) / 2.0;
					pTiles[15]->Set(LockPickMenu::iUT_X, v48, 1);
					NiAVObject* v34 = LockPickMenu::spCameraRoot.m_pObject->GetAt(1u);
					v34->SetTranslate(origPickTranslate);
					NiAVObject* v35 = LockPickMenu::spCameraRoot.m_pObject->GetAt(1u);
					v35->SetRotate(origPickRotate);
					ppickAnimManager->DeactivateAll(0.0f);
					ppickAnimManager->ActivateSequence(pPickIntroAnim, 0, 1, 1.0f, 0.0f, 0);
					if (bCreateNewSweetSpot) {
						SetupLockLevelSweetSpots();
						bCreateNewSweetSpot = 0;
					}
					if (pLockTarget && pTiles[1]->GetFloat(LockPickMenu::iUT_Value) == 0.0) {
						const char* v36 = sOutOfLockpicks.String();
						Interface::ShowMessage(v36, 0,
							"Interface\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", 0, 2.0f, 0);
						pTiles[11]->Set(4003, 0);
						FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData_4, nullptr, 0.0f, 1, 0);
						NiAVObject* v37 = LockPickMenu::spCameraRoot.m_pObject->GetAt(1u);
						v37->Update(UpdateData_4);
						eLockState = LockPickMenu::LS_IDLE;
					} else {
						eLockState = LockPickMenu::LS_PICK_INTRO_ANIM;
					}
				}
				break;
			}
			case LockPickMenu::LS_CLOSING_MENU: {
				if (xFadeState != Interface::FADING_OUT) {
					InterfaceManager* v38 = InterfaceManager::Instance();
					v38->pCursor->Set(4009, 255.0f, 1);
					InterfaceManager* v39 = InterfaceManager::Instance();
					v39->SetLockpickControlsDisable(1);
					pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
					StartFadeOut();
				}
				break;
			}
			default: {
				return;
			}
			}
		}
	}
}

int32_t LockPickMenu::GetClass() {
	return 1014;
}

bool LockPickMenu::IsWellFormedMenu() {
	int32_t i;
	for (i = 0; i < 16; ++i) {
		if (!pTiles[i]) {
			return (bool)0;
		}
	}
	return (bool)1;
}

bool LockPickMenu::GetPCInput(int32_t state) {
	Controls* v3;
	Controls* v4;
	Controls* v5;
	Controls* controls = appMain->GetControls();
	uint8_t v7 = 1;
	if (!controls->GetUserAction(static_cast<Controls::USERACTIONS>(2),
			static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
		v3 = appMain->GetControls();
		if (!v3->GetUserAction(static_cast<Controls::USERACTIONS>(3),
				static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
			v4 = appMain->GetControls();
			if (!v4->GetUserAction(static_cast<Controls::USERACTIONS>(0),
					static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
				v5 = appMain->GetControls();
				if (!v5->GetUserAction(static_cast<Controls::USERACTIONS>(1),
						static_cast<Controls::CONTROL_TOGGLE_FLAGS>(state))) {
					return (bool)0;
				}
			}
		}
	}
	return (bool)v7;
}

void LockPickMenu::RotateLock(float afAngleOffset) {
	float fPhase = (fLockAngle + afAngleOffset) / 90.0;
	NiUpdateData kUpdateData;
	kUpdateData.~NiUpdateData();
	::new (&kUpdateData) NiUpdateData(0.0f, true, false);
	if (pLockRotateAnim) {
		double fBeginKeyTime = pLockRotateAnim->GetBeginKeyTime();
		double fEndKeyTime = pLockRotateAnim->GetEndKeyTime();
		float fTime = (fEndKeyTime - pLockRotateAnim->GetBeginKeyTime()) * fPhase + fBeginKeyTime;
		pLockRotateAnim->SetPhase(fTime, false);
		LockPickMenu::spCameraRoot->GetAt(0)->Update(kUpdateData);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void LockPickMenu::Prepare3DElements() {
	NiNode* v14;
	FNVD_INLINED_LOCAL(NiFixedString, v16, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v17, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v18, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, v19, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
	NiPoint3 dst;
	NiMatrix3 matZ;
	NiMatrix3 matX;
	NiAVObject* plockGeometry =
		pModelLoader->LoadFile("Meshes\\Terminals\\LockInterface01.NIF", LOD_MULT_NONE, 1, 0, 0, 0);
	NiAVObject* ppickGeometry = pModelLoader->LoadFile("Meshes\\Terminals\\BobbyPin01.NIF", LOD_MULT_NONE, 1, 0, 0, 0);
	origPickRotate = NiMatrix3_IDENTITY;
	origPickTranslate = *::new (&dst) NiPoint3(0.0f, 0.0f, -1.6f);
	ppickGeometry->SetTranslate(origPickTranslate);
	ppickGeometry->SetRotate(origPickRotate);
	void* apMemory = NiMemObject::operator new(sizeof(NiNode));
	if (apMemory) {
		v14 = ::new (apMemory) NiNode(0);
	} else {
		v14 = 0;
	}
	NiRefObject* v23 = v14;
	LockPickMenu::spCameraRoot.operator=(v14);
	NiNode* v13 = LockPickMenu::spCameraRoot.m_pObject;
	v13->AttachChild(plockGeometry, 1);
	NiNode* v12 = LockPickMenu::spCameraRoot.m_pObject;
	v12->AttachChild(ppickGeometry, 1);
	matX.MakeXRotation(1.5707964f);
	matZ.MakeZRotation(1.5707964f);
	LockPickMenu::spCameraRoot.m_pObject->SetRotate(matX * matZ);
	LockPickMenu::spCameraRoot.m_pObject->SetTranslate(125.0f, 0.0f, 0.0f);
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1, nullptr, 0.0f, 0, 0);
	LockPickMenu::spCameraRoot.m_pObject->Update(UpdateData1);
	NiTimeController* plockAnimController = plockGeometry->GetController(&NiControllerManager::ms_RTTI);
	if (plockAnimController) {
		NiControllerManager* plockAnimManager = static_cast<NiControllerManager*>(plockAnimController);
		NiFixedString* kName = ::new (static_cast<void*>(&apString)) NiFixedString("Forward");
		pIntroAnim = plockAnimManager->GetSequenceByName(*kName);
		apString.~NiFixedString();
		NiFixedString* v10 = ::new (static_cast<void*>(&v19)) NiFixedString("Backward");
		pLockRotateAnim = plockAnimManager->GetSequenceByName(*v10);
		v19.~NiFixedString();
		plockAnimManager->DeactivateAll(0.0f);
		if (pIntroAnim) {
			plockAnimManager->ActivateSequence(pIntroAnim, 0, 1, 1.0f, 0.0f, 0);
		}
		if (pLockRotateAnim) {
			pLockRotateAnim->SetCycleType(NiTimeController::CLAMP);
		}
		NiTimeController::StartAnimations(plockGeometry);
	}
	NiTimeController* ppickAnimController = ppickGeometry->GetController(&NiControllerManager::ms_RTTI);
	if (ppickAnimController) {
		ppickAnimManager = static_cast<NiControllerManager*>(ppickAnimController);
		NiFixedString* v9 = ::new (static_cast<void*>(&v18)) NiFixedString("Forward");
		pPickIntroAnim = ppickAnimManager->GetSequenceByName(*v9);
		v18.~NiFixedString();
		NiFixedString* v8 = ::new (static_cast<void*>(&v17)) NiFixedString("Backward");
		pPickBendAnim = ppickAnimManager->GetSequenceByName(*v8);
		v17.~NiFixedString();
		NiFixedString* v5 = ::new (static_cast<void*>(&v16)) NiFixedString("Left");
		pPickBreakAnim = ppickAnimManager->GetSequenceByName(*v5);
		v16.~NiFixedString();
		ppickAnimManager->DeactivateAll(0.0f);
		if (pPickIntroAnim) {
			ppickAnimManager->ActivateSequence(pPickIntroAnim, 0, 1, 1.0f, 0.0f, 0);
		}
		if (pPickBreakAnim) {
			pPickBreakAnim->SetCycleType(NiTimeController::CLAMP);
		}
		NiTimeController::StartAnimations(ppickGeometry);
	}
	LockPickMenu::spCamera.operator=(Create3DCamera());
	ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
	ShadowSceneNode_->AttachChild(LockPickMenu::spCameraRoot.m_pObject, 1);
	const NiBound* WorldBound = &LockPickMenu::spCameraRoot.m_pObject->GetWorldBound();
	float fradius = WorldBound->m_fRadius * 3.0;
	NiNode* v6 = LockPickMenu::spCameraRoot.m_pObject;
	ShadowSceneNode* v4 = BSShaderManager::GetShadowSceneNode(1);
	v4->AddAllLightsFromNode_Recurse(v6, fradius);
}

// The exe's cleanup-only C++ EH frame is left out.
NiCamera* LockPickMenu::Create3DCamera() {
	int32_t ScreenHeight;
	NiCamera* pcam;
	NiPoint3 v10;
	NiPoint3 dst;
	NiRect<float> port;
	void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
	if (apMemory) {
		pcam = ::new (apMemory) NiCamera;
	} else {
		pcam = 0;
	}
	::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
	pcam->SetViewPort(port);
	FNVD_CONSTRUCTED_LOCAL(NiFrustum, fr, nullptr, 0);
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	fr.m_fFar = WorldSceneGraph->GetFarDistance();
	fr.m_fNear = BSSceneGraph::fNearDistance.Float();
	float v7 = fr.m_fFar / fr.m_fNear;
	pcam->SetMaxFarNearRatio(v7);
	if (BSShaderManager::bLetterbox) {
		ScreenHeight = BSShaderManager::iLetterboxHeight;
	} else {
		ScreenHeight = Renderer::GetScreenHeight();
	}
	float fHeightByWidth = static_cast<double>(ScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
	float faspectDelta = 0.75f;
	float theta = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
	fr.m_fLeft = -std::tan(theta) * 0.75f;
	fr.m_fRight = std::tan(theta) * 0.75f;
	double v1 = std::tan(theta);
	fr.m_fBottom = -v1 * fHeightByWidth * 0.75f;
	double v2 = std::tan(theta);
	fr.m_fTop = v2 * fHeightByWidth * 0.75f;
	fr.m_bOrtho = 0;
	pcam->SetViewFrustum(fr);
	NiPoint3* z = ::new (&dst) NiPoint3(0.0f, 0.0f, 1.0f);
	NiPoint3* v3 = ::new (&v10) NiPoint3(0.0f, 0.0f, 1.0f);
	pcam->LookAtWorldPoint(*v3, *z);
	pcam->SetMinNearPlaneDist(1.0f);
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData2, nullptr, 0.0f, 0, 0);
	pcam->Update(UpdateData2);
	return pcam;
}

// The exe's cleanup-only C++ EH frame is left out.
void LockPickMenu::RotateLockpick() {
	BSRandom* Singleton;
	FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	float fxDelta = 0.0f;
	int32_t ixAxis = 0;
	float cfmaxAxisValue = 32767.0f;
	if (InterfaceManager::Has360Controller()) {
		Controls* Controls_ = appMain->GetControls();
		ixAxis = Controls_->GetControllerValue(0, 7);
		if (NiAbs(static_cast<float>(ixAxis)) <= 7849.0) {
			ixAxis = 0;
		}
		float v22 = static_cast<double>(ixAxis) / cfmaxAxisValue;
		fxDelta = hypotf_3(v22, 3.0f) * 80.0;
	} else {
		Controls* v2 = appMain->GetControls();
		double xDelta = static_cast<double>(v2->GetMouseAxis(1));
		double ScreenWidth = InterfaceManager::GetScreenWidth();
		fxDelta = ScreenWidth / InterfaceManager::GetRealScreenWidth() * xDelta;
	}
	float fnewX = pTiles[15]->GetFloat(LockPickMenu::iUT_X) + fxDelta;
	int32_t iprevXMod = static_cast<int32_t>(pTiles[15]->GetFloat(LockPickMenu::iUT_X)) % 100;
	int32_t inewXMod = static_cast<int32_t>(fnewX) % 100;
	fnewX = BSMAX(fnewX, 0.0);
	float Xa = pTiles[14]->GetFloat(4017);
	fnewX = BSMIN(fnewX, Xa);
	if (((!(fxDelta > 0.0) || iprevXMod <= inewXMod) && (!(fxDelta < 0.0) || inewXMod <= iprevXMod)) ||
		(Singleton = BSRandom::GetSingleton(), Singleton->UnsignedInt(10u))) {
		if (fxDelta != 0.0 && LockPickMenu::PreviousAngleDelta == 0.0) {
			BSAudio* v5 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v5->GetSoundHandleByName(&th_s, "UILockpickingPickMovement", 257u);
			SoundHandleByName->Play(0);
		}
	} else {
		BSAudio* v4 = &BSAudio::QInstance();
		BSSoundHandle* v19 = v4->GetSoundHandleByName(&arHandle, "UILockpickingPickMovement", 257u);
		v19->Play(0);
	}
	pTiles[15]->Set(LockPickMenu::iUT_X, fnewX, 1);
	LockPickMenu::PreviousAngleDelta = fxDelta;
	RotateLockpickModel(fnewX);
	float v17 = fnewX - fSweetSpot;
	float fdistFromCenter = NiAbs(v17);
	if (!(fdistFromCenter < static_cast<double>(iTotalSweetSpotRadius))) {
		double v16 = static_cast<double>(iTotalSweetSpotRadius) + fdistFromCenter;
		double v15 = v16 - pTiles[14]->GetFloat(4017);
		double v14 = static_cast<double>(iTotalSweetSpotRadius) * 2.0;
		float fdistRatio = v15 / (v14 - pTiles[14]->GetFloat(4017));
		fMaxLockAngle = fdistRatio * 15.0;
		float a1c = pTiles[14]->GetFloat(4018);
		pTiles[15]->Set(4018, a1c, 1);
		float a1d = pTiles[14]->GetFloat(4019);
		pTiles[15]->Set(4019, a1d, 1);
		float a1e = pTiles[14]->GetFloat(4020);
		pTiles[15]->Set(4020, a1e, 1);
	} else {
		// The head position comes from the exe's shared one-field getter 0x559450 (a trivial accessor); GetNext is the
		// list's inline template, which the exe calls out of line (0x57CBE0).
		NiTListItem<Tile*>* pos = pTiles[14]->xChildren.m_pkHead;
		float fminHalfLength = pTiles[14]->GetFloat(4017);
		Tile* pclosestChild = nullptr;
		while (pos) {
			Tile* pcurrChild = pTiles[14]->xChildren.GetNext(pos);
			float fcurrHalfLength = pcurrChild->GetFloat(4017) / 2.0;
			if (fminHalfLength > static_cast<double>(fcurrHalfLength) &&
				fcurrHalfLength > static_cast<double>(fdistFromCenter)) {
				fminHalfLength = fcurrHalfLength;
				pclosestChild = pcurrChild;
			}
		}
		if (pclosestChild) {
			fMaxLockAngle = (1.0 - (pclosestChild->GetFloat(4010) - 17.0) / 6.0) * 90.0;
			float a1 = pclosestChild->GetFloat(4018);
			pTiles[15]->Set(4018, a1, 1);
			float a1a = pclosestChild->GetFloat(4019);
			pTiles[15]->Set(4019, a1a, 1);
			float a1b = pclosestChild->GetFloat(4020);
			pTiles[15]->Set(4020, a1b, 1);
		}
	}
}

void LockPickMenu::RotateLockpickModel(float a2) {
	NiMatrix3 v10;
	NiPoint3 v11;
	NiPoint3 v12;
	NiPoint3 v13;
	NiPoint3 dst;
	NiPoint3 result;
	NiMatrix3 ecx0a;
	NiPoint3 pt;
	NiNode* v2 = LockPickMenu::spCameraRoot.m_pObject;
	NiNode* v19 = static_cast<NiNode*>(v2->GetAt(1u));
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, a2a, nullptr, 0.0, 0, 0);
	NiPoint3* z = ::new (&dst) NiPoint3(static_cast<float>(0.0), static_cast<float>(0.0), 0.6f);
	NiPoint3* v3 = &(result = -origPickTranslate);
	pt = *v3 + *z;
	float v8 = (a2 / pTiles[14]->GetFloat(4017) * -180.0 + 90.0) * 0.01745329238474369;
	ecx0a.MakeYRotation(v8);
	NiPoint3* za = &(v12 = ecx0a * pt);
	NiPoint3* v4 = &(v13 = origPickTranslate + pt);
	NiPoint3* v5 = &(v11 = *v4 - *za);
	v19->SetTranslate(*v5);
	ecx0a = (v10 = ecx0a * origPickRotate);
	v19->SetRotate(ecx0a);
	v19->Update(a2a);
}

// The exe's cleanup-only C++ EH frame is left out.
void LockPickMenu::SetupLockLevelSweetSpots() {
	FNVD_INLINED_LOCAL(NiColorA, v32, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStringT<char>, v33, std::nullopt, std::nullopt);
	int32_t v28 = LockLevelMaxVeryEasy[eLockLevel + 1] - LockLevelMaxVeryEasy[eLockLevel];
	float v19 = static_cast<double>(iLockpickSkill - LockLevelMaxVeryEasy[eLockLevel]) /
		(static_cast<double>(v28) + static_cast<double>(v28));
	float a1 = BSMIN(v19, 1.0f);
	float v27 = BSMAX(a1, 0.0f);
	int32_t a3 = 0;
	int32_t aiValue = 6;
	FNVD_CONSTRUCTED_LOCAL(NiColorA, apMemory, nullptr, 0.0f, 1.0f, 0.0f, 1.0f);
	FNVD_CONSTRUCTED_LOCAL(NiColorA, v31, nullptr, 1.0f, 0.0f, 0.0f, 1.0f);
	v32 = apMemory;
	pTiles[14]->DeleteChildren();
	::new (&v33) BSStringT<char>;
	double v18 = static_cast<double>(g_iSweetSpotLengthMinArray[eLockLevel]->Int());
	int32_t* v1 = &g_iSweetSpotLengthMaxArray[eLockLevel]->Int();
	g_iconcentricLength =
		static_cast<int32_t>(static_cast<double>(*v1 - g_iSweetSpotLengthMinArray[eLockLevel]->Int()) * v27 + v18);
	double v17 = static_cast<double>(g_iConcentricLengthMinArray[eLockLevel]->Int());
	int32_t* v2 = &g_iConcentricLengthMaxArray[eLockLevel]->Int();
	g_isweetSpotLength =
		static_cast<int32_t>(static_cast<double>(*v2 - g_iConcentricLengthMinArray[eLockLevel]->Int()) * v27 + v17);
	v33.SPrintF("%d", g_iconcentricLength);
	pTiles[4]->Set(4036, v33.pString, 1);
	v33.SPrintF("%d", g_isweetSpotLength);
	pTiles[7]->Set(4036, v33.pString, 1);
	iTotalSweetSpotRadius = g_isweetSpotLength + g_iconcentricLength / 2;
	double Float = pTiles[14]->GetFloat(4017);
	BSRandom* Singleton = BSRandom::GetSingleton();
	fSweetSpot = Singleton->Float0To1() * Float;
	float v15 = pTiles[14]->GetFloat(4017) - static_cast<double>(iTotalSweetSpotRadius);
	float a1b = BSMAX(fSweetSpot, static_cast<float>(iTotalSweetSpotRadius));
	fSweetSpot = BSMIN(a1b, v15);
	a3 = static_cast<int32_t>(fSweetSpot - static_cast<double>(g_iconcentricLength / 2));
	for (int32_t i = 0; i < 6; ++i) {
		Tile* v24 = pMenu->pMenu->RenderTemplate(pTiles[14], "LPM_SweetSpotTemplate", 0);
		v24->Set(4010, i + 17);
		double v4 = fSweetSpot - static_cast<double>(a3);
		float v14 = v4 + v4;
		v24->Set(4017, v14, 1);
		v24->Set(4001, a3);
		v24->Set(4013, aiValue);
		float v13 = v32.r * 255.0;
		v24->Set(4018, v13, 1);
		float mult = v32.g * 255.0;
		v24->Set(4019, mult, 1);
		float v11 = v32.b * 255.0;
		v24->Set(4020, v11, 1);
		a3 -= g_isweetSpotLength / 5;
		--aiValue;
		float v10 = static_cast<double>(i + 1) / 5.0;
		v32 = apMemory + (v31 - apMemory) * v10;
	}
	v33.~BSStringT<char>();
}

// The exe's cleanup-only C++ EH frame is left out.
Tile* LockPickMenu::Create(TESObjectREFR* apTarget, int32_t aiLockLevel, bool abDebugMode) {
	int32_t ilockLevel;
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	Tile* pold_menu_tile = Menu::GetMenuByClass(1014);
	InterfaceManager* pin = InterfaceManager::Instance();
	if (apTarget) {
		int32_t LockLevel = apTarget->GetLockLevel();
		ilockLevel = GetLockPickLevel(LockLevel);
	} else {
		ilockLevel = aiLockLevel;
	}
	int32_t v4 = BSMAX(ilockLevel, 0);
	int32_t ilockLevel_2 = BSMIN(v4, 4);
	if (apTarget && pPlayer->GetClampedActorValue(ActorValue::LOCKPICK) < LockLevelMaxVeryEasy[ilockLevel_2]) {
		char msg[268];
		BSsprintf(msg, 260, sLockpickSkillTooLow.String(), LockLevelMaxVeryEasy[ilockLevel_2]);
		Interface::ShowMessage(msg, 0, "Interface\\Icons\\Message Icons\\glow_message_padlock.dds", 0, 2.0f, 0);
		BSAudio* v5 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v5->GetSoundHandleByName(&a1, "UIPopUpMessageGeneral", 289u);
		SoundHandleByName->Play(0);
		return 0;
	} else {
		int32_t v7;
		if (pold_menu_tile) {
			delete pold_menu_tile;
		}
		Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\lockpick_menu.xml");
		LockPickMenu::pMe = static_cast<LockPickMenu*>(ptile->GetMenu());
		if (LockPickMenu::pMe && (v7 = LockPickMenu::pMe->GetClass(), v7 == LockPickMenu::GetClassStatic())) {
			LockPickMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
			if (LockPickMenu::pMe->IsWellFormedMenu()) {
				char Src[64];
				if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
					float time = Tile::GetMaximumDepth();
					ptile->Set(4013, time, 1);
				}
				LockPickMenu::pMe->pLockTarget = apTarget;
				LockPickMenu::pMe->eLockLevel = static_cast<LOCK_LEVEL>(ilockLevel_2);
				int32_t v8 = Tile::TextToTrait("_DebugMode");
				LockPickMenu::pMe->pMenu->Set(v8, static_cast<int32_t>(abDebugMode));
				BSsprintf(Src, 64, "%d", LockPickMenu::pMe->iLockpickSkill);
				LockPickMenu::pMe->pTiles[0]->Set(LockPickMenu::iUT_Value, Src, 1);
				int32_t afValue = LockLevelMaxVeryEasy[LockPickMenu::pMe->eLockLevel];
				int32_t ClampedActorValue = pPlayer->GetClampedActorValue(ActorValue::LOCKPICK);
				int32_t afValuea = GamePlayFormulas::GetForceLockChance(ClampedActorValue, afValue);
				BSsprintf(Src, 64, "%s [%d", sForceLock.String(), afValuea);
				BSstrcat(Src, 64, "%]");
				LockPickMenu::pMe->pTiles[9]->Set(4036, Src, 1);
				const char* v10 = pLockLevelNames[LockPickMenu::pMe->eLockLevel]->String();
				LockPickMenu::pMe->pTiles[2]->Set(LockPickMenu::iUT_Value, v10, 1);
				LockPickMenu::pMe->pTiles[12]->Set(4036, "PICK HEALTH: 100%", 1);
				LockPickMenu::pMe->pTiles[1]->Poke(LockPickMenu::iUT_Value, 1.0f);
				if (apTarget) {
					int32_t a3 = pPlayer->GetInventoryObjectCount(pLockpick);
					LockPickMenu::pMe->pTiles[1]->Set(LockPickMenu::iUT_Value, a3);
				} else {
					LockPickMenu::pMe->pTiles[1]->Set(LockPickMenu::iUT_Value, 0);
				}
				const char* v11 = sLanguage.String();
				if (BSstrcmp(v11, "ENGLISH")) {
					const char* timea = sLockpickSkillText.String();
					Tile* ChildByName = ptile->GetChildByName("LPM_SkillTitle");
					ChildByName->Set(4036, timea, 1);
					const char* timeb = sPicksRemainingText.String();
					Tile* v13 = ptile->GetChildByName("LPM_PickCountTitle");
					v13->Set(4036, timeb, 1);
					const char* timec = sLockLevelText.String();
					Tile* v14 = ptile->GetChildByName("LPM_LevelTitle");
					v14->Set(4036, timec, 1);
					const char* v15 = sExit.String();
					LockPickMenu::pMe->pTiles[10]->Set(4036, v15, 1);
				}
				LockPickMenu::pMe->Prepare3DElements();
				LockPickMenu::pMe->SetupLockLevelSweetSpots();
				LockPickMenu::pMe->PrepForVisibility(0);
				return ptile;
			} else {
				Warning("MENUS: Lockpicking Menu Creation Failed... Are your menu and art resources up to date?");
				return 0;
			}
		} else {
			if (LockPickMenu::pMe) {
				if (LockPickMenu::pMe->pMenu) {
					if (LockPickMenu::pMe) {
						delete LockPickMenu::pMe;
					}
				}
			}
			return 0;
		}
	}
}

void LockPickMenu::Close() {
	if (LockPickMenu::pMe) {
		LockPickMenu::pMe->eLockState = LS_CLOSING_MENU;
	}
}

int32_t LockPickMenu::GetClassStatic() {
	return 1014;
}

Interface::TutorialMessageID LockPickMenu::GetTutorialMessageID() {
	return static_cast<Interface::TutorialMessageID>(Interface::Has360Controller() ? 28 : 20);
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe's cleanup-only C++ EH frame is left out.
void LockPickMenu::Draw3DElements() {
	if (LockPickMenu::spCameraRoot.m_pObject) {
		NiCamera* v6;
		NiRenderer* Renderer_ = NiRenderer::GetRenderer();
		Renderer_->ClearBuffer(0, 4u);
		SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
		BSCullingProcess* a1 = WorldSceneGraph->pCuller;
		a1->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
		NiCamera* apCamera = LockPickMenu::spCamera.m_pObject;
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
}
