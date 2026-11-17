#include "Fallout/AI/Combat/VATS.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSCameraPath.h"
#include "Fallout Shared/BGSCameraPathManager.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/ImageSpaceModifierInstanceRB.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"

VATS pVATS;
SettingT<INISettingCollection> gs_fVATSLightLevelMin_VATS("fVATSLightLevelMin:VATS", 20.0f);
SettingT<INISettingCollection> gs_fVATSLightLevelMax_VATS("fVATSLightLevelMax:VATS", 40.0f);
SettingT<INISettingCollection> gs_fVATSLightAngle_VATS("fVATSLightAngle:VATS", 0.0f);
SettingT<INISettingCollection> gs_fVATSLightDistance_VATS("fVATSLightDistance:VATS", 100.0f);
SettingT<INISettingCollection> gs_fVATSLightElevation_VATS("fVATSLightElevation:VATS", 100.0f);
SettingT<INISettingCollection> gs_fVatsLightColorR_VATS("fVatsLightColorR:VATS", 1.0f);
SettingT<INISettingCollection> gs_fVatsLightColorG_VATS("fVatsLightColorG:VATS", 1.0f);
SettingT<INISettingCollection> gs_fVatsLightColorB_VATS("fVatsLightColorB:VATS", 1.0f);

bool VATSQueuedAction::IsActionGrenadeOrMine(ActionPoints::Action aeAction) {
	return aeAction >= ActionPoints::ACTION_ATTACK_GRENADE && aeAction <= ActionPoints::ACTION_ATTACK_MINE;
}

VATS::VATS()
	: eMode(VATS::VATS_MODE_NONE), pCameraList(nullptr), pCamera(nullptr), fCameraTimer(0.0f), fCameraMinTimer(0.0f),
	  pFirstProjectile(nullptr), pLastProjectile(nullptr), pMeleeAttack(nullptr), pImagespaceMod(nullptr),
	  pTransitionImagespaceMod(nullptr), spVATSLight(nullptr), bFirstVatsAttack(true), iKillCount(0),
	  pUnEquipWeapon(nullptr), pUnEquipExtra(nullptr) {
	eVATSCameraAction = 0;
}

VATS::~VATS() {
	ClearData();
}

void VATS::QueueAction(VATS_COMMAND* apCommand) {
	commandList.AddTail(apCommand);
}

NiPoint3* VATS::GetTargetPercentagePos(NiPoint3* apResult, float afPercentage) {
	NiNode* v3 = pVATSAttacker->TESObjectREFR::Get3D();
	NiPoint3 startingPos = v3->GetWorldBound().m_kCenter;
	VATS_COMMAND* pcommand = commandList.GetItem();
	if ((pcommand && pcommand->pTarget) || pVATSTarget) {
		NiPoint3 endingPos = NiPoint3_ZERO;
		if (pcommand && pcommand->pTarget) {
			if (pcommand->pTarget->Get3D()) {
				NiNode* v4 = pcommand->pTarget->Get3D();
				endingPos = v4->GetWorldBound().m_kCenter;
			} else {
				endingPos = pcommand->pTarget->GetLocationOnReference();
			}
		} else {
			if (pVATSTarget) {
				if (pVATSTarget->Get3D()) {
					NiNode* v6 = pVATSTarget->Get3D();
					endingPos = v6->GetWorldBound().m_kCenter;
				} else {
					endingPos = pVATSTarget->GetLocationOnReference();
				}
			}
		}
		NiPoint3 delta = endingPos - startingPos;
		float flength = delta.UnitizeGetLength();
		startingPos += delta * flength * afPercentage / 100.0f;
	}
	*apResult = startingPos;
	return apResult;
}

VATS_COMMAND* VATS::GetCurrentAction() {
	bool bEmpty;
	bEmpty = commandList.IsEmpty();
	if (bEmpty)
		return nullptr;
	VATS_COMMAND* pCommand;
	pCommand = commandList.GetItem();
	return pCommand;
}

// The last queued command: walks the list with IsEmpty/GetNext and returns the node's item when GetNext reports the
// end. The exe calls GetNext twice per node, as written, and tests the entry node (this) for null before the loop.
VATS_COMMAND* VATS::GetLastAction() {
	for (BSSimpleList<VATS_COMMAND*>* pIter = &commandList; pIter;) {
		bool bEmpty;
		bEmpty = pIter->IsEmpty();
		if (bEmpty)
			break;
		BSSimpleList<VATS_COMMAND*>* pNext;
		pNext = pIter->GetNext();
		if (!pNext) {
			VATS_COMMAND* pCommand;
			pCommand = pIter->GetItem();
			return pCommand;
		}
		pIter = pIter->GetNext();
	}
	return nullptr;
}

bool VATS::IsMeleeCommand(VATS_COMMAND* apCommand) {
	VATS_COMMAND* pCommand = apCommand;
	if (!apCommand)
		pCommand = commandList.GetItem();
	if (!pCommand)
		return false;
	switch (pCommand->eAction) {
	case ActionPoints::ACTION_ATTACK_UNARMED:
	case ActionPoints::ACTION_ATTACK_ONE_HAND_MELEE:
	case ActionPoints::ACTION_ATTACK_TWO_HAND_MELEE:
	case ActionPoints::ACTION_PLAYER_SPECIAL_WEAPON_ATTACK:
	case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK:
	case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_2:
	case ActionPoints::ACTION_PLAYER_SPECIAL_UNARMED_ATTACK_3:
		return true;
	default:
		return false;
	}
}

bool VATS::IsInVats() {
	return this->eMode != VATS::VATS_MODE_NONE;
}

ShadowSceneLight* VATS::GetVATSLight() {
	// The engine routes this through NiPointer::operator->; both return the light.
	return this->spVATSLight;
}

VATS::VATS_MODE_ENUM VATS::GetMode() {
	return this->eMode;
}

void VATS::SetFirstProjectile(Projectile* apProjectile) {
	if (pCamera) {
		if (pCamera->Data.eLocation == BGSCameraShot::CO_PROJECTILE) {
			if (apProjectile) {
				NiNode* v3 = apProjectile->Get3D();
				pCamera->SetCameraLocation(v3);
			} else {
				pCamera->SetCameraLocation(0);
			}
		}
		if (pCamera->Data.eTarget == BGSCameraShot::CO_PROJECTILE) {
			if (apProjectile) {
				NiNode* v2 = apProjectile->Get3D();
				pCamera->SetCameraTarget(v2);
			} else {
				pCamera->SetCameraTarget(0);
			}
		}
	}
	pFirstProjectile = apProjectile;
}

// Strict (b41b) leaves the edge where UseNextCamera clears pCamera uncovered, and
// MissileProjectile's registered body calls this; the draft is in BP71/left.md.
void VATS::SetLastProjectile(Projectile* apProjectile) {
	VATS_COMMAND* currentAction;
	uint8_t i;
	pLastProjectile = apProjectile;
	if (pCamera) {
		currentAction = GetCurrentAction();
		UpdateCurrentCameraAction(currentAction);
		if (fCameraMinTimer == 0.0) {
			for (i = 1; pCamera && (i && (eVATSCameraAction > ((int32_t)pCamera->GetCameraAction())));
				i = UseNextCamera()) {
				;
			}
		}
	}
}

void VATS::UpdateCurrentCameraAction(VATS_COMMAND* apCommand) {
	if (IsMeleeCommand(apCommand)) {
		Animation* panim = pVATSAttacker->Actor::GetAnimation();
		if (!eVATSCameraAction) {
			TESAnimGroup* AnimGroup;
			TESAnimGroup* v3;
			BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_WEAPON);
			if (pseq &&
				(((AnimGroup = pseq->GetAnimGroup(), !AnimGroup->IsAimAnim()) || apCommand->cFireShots) &&
					((v3 = pseq->GetAnimGroup(), !v3->IsAttackAction()) ||
						panim->GetGroupAction(AGS_WEAPON) < AGA_STOP))) {
				if (apCommand->pTarget) {
					if (apCommand->pTarget->IsDead(0)) {
						eVATSCameraAction = 2;
					}
				}
			} else {
				eVATSCameraAction = 2;
			}
			if (eVATSCameraAction == 2 && fCameraMinTimer == 0.0) {
				for (bool bnewCamera = 1; bnewCamera; bnewCamera = UseNextCamera()) {
					int32_t CameraAction = pCamera->GetCameraAction();
					if (eVATSCameraAction <= CameraAction) {
						break;
					}
				}
			}
		}
	} else {
		if (apCommand && apCommand->eAction == ActionPoints::ACTION_TARGET_KILL_SHOT) {
			eVATSCameraAction = 2;
			pCamera->bCameraIsKillCam = 1;
		} else {
			if (pFirstProjectile) {
				if (!eVATSCameraAction) {
					eVATSCameraAction = 1;
				}
			}
			if (pFirstProjectile || pLastProjectile) {
				if (pFirstProjectile) {
					if (pLastProjectile && (!pCamera || pCamera->GetCameraAction() == BGSCameraShot::CA_HIT)) {
						eVATSCameraAction = 2;
					} else {
						const NiPoint3* v9 = &pFirstProjectile->GetLocationOnReference();
						const NiPoint3* v5 = &apCommand->pTarget->GetLocationOnReference();
						NiPoint3 deltaPos = *v5 - *v9;
						double v14 = deltaPos.Length();
						double v13 = static_cast<double>(iVATSCameraHitDist.Int());
						if (!(apCommand->pTarget->GetBoundMax().y + v13 > v14)) {
							if (pLastProjectile) {
								const NiPoint3* v10 = &pLastProjectile->GetLocationOnReference();
								const NiPoint3* v6 = &apCommand->pTarget->GetLocationOnReference();
								deltaPos = *v6 - *v10;
								double v12 = deltaPos.Length();
								double v11 = static_cast<double>(iVATSCameraHitDist.Int());
								if (apCommand->pTarget->GetBoundMax().y + v11 > v12) {
									eVATSCameraAction = 2;
								}
							}
						} else {
							eVATSCameraAction = 2;
						}
					}
				} else {
					eVATSCameraAction = 2;
				}
			} else {
				if (eVATSCameraAction == 1) {
					eVATSCameraAction = 2;
				}
			}
		}
	}
}

bool VATS::UseNextCamera() {
	BSSimpleList<BGSCameraShot*>* plist = pCameraList;
	bool bretVal = 0;
	while (plist && plist->m_item != pCamera) {
		plist = plist->m_pkNext;
	}
	if (plist) {
		plist = plist->m_pkNext;
	}
	if (plist) {
		if (pCamera) {
			pCamera->FreeCamera();
			pCamera = 0;
		}
		pCamera = plist->m_item;
		if (pCamera) {
			if (static_cast<double>(fCameraMinTimer) < pCamera->Data.fMinTime) {
				fCameraMinTimer = pCamera->Data.fMinTime;
			}
		}
		if (pCamera && pCamera->GetIsFirstPersonCamera()) {
			pPlayer->SetFirstPerson(1);
		} else {
			bSwitchTo3rdPerson = 1;
		}
		return 1;
	} else {
		if (pCamera) {
			if (!pCamera->IsInitialized()) {
				pCamera->FreeCamera();
				pCamera = 0;
			}
		}
	}
	return bretVal;
}

// The exe's cleanup-only C++ EH frame is left out.
void VATS::SetMode(VATS::VATS_MODE_ENUM aeMode, bool bForce) {
	NiPoint2 v17;
	if (aeMode != eMode || bForce) {
		pVATSAttacker = pPlayer;
		switch (aeMode) {
		case VATS::VATS_MODE_NONE: {
			if (VATS::pStranger) {
				delete VATS::pStranger;
				VATS::pStranger = 0;
			}
			if (VATS::pMissFortune) {
				delete VATS::pMissFortune;
				VATS::pMissFortune = 0;
			}
			appTimer.SetGlobalTimeMultiplier(1.0f, 1);
			pForceCameraPath = 0;
			pForceDefaultCameraPath = 0;
			eCameraZoom = 0;
			if (pCamera) {
				pCamera->FreeCamera();
			}
			eVATSCameraAction = 0;
			pCameraList = 0;
			pCamera = 0;
			fCameraTimer = 0.0f;
			fCameraMinTimer = 0.0f;
			pFirstProjectile = 0;
			pLastProjectile = 0;
			pCameraPath = 0;
			bSwitchTo3rdPerson = 0;
			if (pImagespaceMod) {
				ImageSpaceModifierInstance::Stop(pImagespaceMod);
				pImagespaceMod = 0;
			}
			if (pTransitionImagespaceMod) {
				ImageSpaceModifierInstance::Stop(pTransitionImagespaceMod);
				pTransitionImagespaceMod = 0;
			}
			bFirstVatsAttack = 1;
			pPlayer->SetIronSights(0, 0, 0);
			pPlayer->SetBlock(0);
			Controls* Controls_ = appMain->GetControls();
			Controls_->ResetControlState(6);
			if (pUnEquipWeapon) {
				pPlayer->UnEquipObject(pUnEquipWeapon, 1, pUnEquipExtra, 0, 1, 1);
				pUnEquipWeapon = 0;
				pUnEquipExtra = 0;
			}
			break;
		}
		case VATS::VATS_MENU_MODE: {
			pUnEquipWeapon = 0;
			pUnEquipExtra = 0;
			FOVATSEffectManager* VATSEffect = Interface::GetVATSEffect();
			VATSEffect->DisableScan();
			if (!pVATS.GetVATSLight()) {
				NiLight* pnewlight_2;
				NiPointLight* v18 = static_cast<NiPointLight*>(NiMemObject::operator new(sizeof(NiPointLight)));
				if (v18) {
					pnewlight_2 = ::new (v18) NiPointLight;
				} else {
					pnewlight_2 = 0;
				}
				pVATS.SetVATSLight(pnewlight_2, 1);
			}
			break;
		}
		case VATS::VATS_SCAN_MODE: {
			pUnEquipWeapon = 0;
			pUnEquipExtra = 0;
			if (!pVATS.GetVATSLight()) {
				NiLight* pnewlight;
				void* apMemory = NiMemObject::operator new(sizeof(NiPointLight));
				if (apMemory) {
					pnewlight = ::new (apMemory) NiPointLight;
				} else {
					pnewlight = 0;
				}
				pVATS.SetVATSLight(pnewlight, 1);
			}
			break;
		}
		case VATS::VATS_PLAYBACK: {
			pUnEquipWeapon = 0;
			pUnEquipExtra = 0;
			iKillCount = 0;
			fVATSPlaybackTimer = 0.0f;
			fVATSCommandTimer = 0.0f;
			bVATSFlushCommandWhenAnimActionIsDone = 0;
			bLastCommandBlock = 0;
			if (pImagespaceMod) {
				ImageSpaceModifierInstance::Stop(pImagespaceMod);
				pImagespaceMod = 0;
			}
			if (pTransitionImagespaceMod) {
				ImageSpaceModifierInstance::Stop(pTransitionImagespaceMod);
				pTransitionImagespaceMod = 0;
			}
			NiPoint2* positon = ::new (&v17) NiPoint2(0.5f, 0.5f);
			float fVATSCamTransRBDownStart = ::fVATSCamTransRBDownStart.Float();
			float fVATSCamTransRBRampDown = ::fVATSCamTransRBRampDown.Float();
			float fVATSCamTransRBStart = ::fVATSCamTransRBStart.Float();
			float* v4 = &fVATSCamTransRBRampup.Float();
			pTransitionImagespaceMod = ImageSpaceModifierInstanceRB::Trigger_0052A440(0.0f, *v4, fVATSCamTransRBStart,
				fVATSCamTransRBRampDown, fVATSCamTransRBDownStart, *positon, 0.0f);
			if (pTransitionImagespaceMod) {
				pTransitionImagespaceMod->SetPermanent(1);
			}
			break;
		}
		default: {
			break;
		}
		}
		BSShaderManager::SetFullScreenMotionBlurVATS(aeMode == VATS::VATS_MENU_MODE);
		eMode = aeMode;
		if (eMode == VATS::VATS_MODE_NONE) {
			HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
			bVATSForce1stPerson = 1;
		}
	}
	if (aeMode) {
		if (aeMode > VATS::VATS_TARGET_SELECT && aeMode <= VATS::VATS_SCAN_MODE) {
			if (pVATSTarget) {
				pVATS.UpdateVATSLight(pVATSTarget);
			}
		}
	} else {
		if (pVATS.GetVATSLight()) {
			NiLight* afDuration = pVATS.GetVATSLight()->GetLight();
			ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
			ShadowSceneNode_->RemoveLight(afDuration);
			ShadowSceneNode* v13 = BSShaderManager::GetShadowSceneNode(0);
			NiLight* Light = pVATS.GetVATSLight()->GetLight();
			v13->DetachChild(Light);
			pVATS.SetVATSLight(0, 1);
		}
	}
}

float VATS::GetPlayerUpdateMult() {
	float offset = (float)1.0;
	if (((int32_t)eMode) == 4) {
		if ((!pCamera) || pCamera->IsInitialized()) {
			// The exe reads the float at +0x48 through a folded getter (NiControllerSequence::GetOffset, 0x639AA0).
			if (pCamera && (pCamera->Data.fPlayerTimeMult > 0.0)) {
				offset = pCamera->Data.fPlayerTimeMult;
			} else {
				offset = (&fVATSPlayerTimeUpdateMult)->Float();
			}
		} else {
			offset = (float)1.0;
		}
	}
	double globalTimeMultiplierTarget = BSTimer::GetGlobalTimeMultiplierTarget();
	return (float)((globalTimeMultiplierTarget / BSTimer::QGlobalTimeMultiplier()) * offset);
}

float VATS::GetTargetUpdateMult() {
	float targetTimeMult = (float)1.0;
	if (((int32_t)eMode) == 4) {
		if ((!pCamera) || pCamera->IsInitialized()) {
			if (pCamera && (pCamera->GetTargetTimeMult() > 0.0)) {
				targetTimeMult = pCamera->GetTargetTimeMult();
			} else {
				targetTimeMult = (&fVATSTargetTimeUpdateMult)->Float();
			}
		} else {
			targetTimeMult = (float)1.0;
		}
	}
	double globalTimeMultiplierTarget = BSTimer::GetGlobalTimeMultiplierTarget();
	return (float)((globalTimeMultiplierTarget / BSTimer::QGlobalTimeMultiplier()) * targetTimeMult);
}

void VATS::ClearTarget(TESObjectREFR* apRef) {
	if (apRef == pVATSTarget) {
		pVATSTarget = 0;
	}
	BSSimpleList<VATS_COMMAND*>* plist = &commandList;
	BSSimpleList<VATS_COMMAND*>* plastlist = 0;
	while (plist && !plist->IsEmpty()) {
		VATS_COMMAND* pcommand = plist->m_item;
		if (pcommand->pTarget == apRef) {
			if (pcommand) {
				delete pcommand;
			}
			if (plastlist) {
				plastlist->Remove(pcommand);
				plist = plastlist->m_pkNext;
			} else {
				plist->RemoveHead();
			}
		} else {
			plastlist = plist;
			plist = plist->m_pkNext;
		}
	}
}

void VATS::QuitVATSPlayback(bool abForceQuit, bool abFinishCommandPlayback) {
	if (eMode) {
		if ((fEndVATSDelay == 0.0 || !commandList.IsEmpty()) &&
			(abForceQuit ||
				(commandList.IsEmpty() || commandList.m_item->eAction != ActionPoints::ACTION_PLAYER_VATS_DEATH))) {
			if (abFinishCommandPlayback) {
				VATS_COMMAND* pcommand = GetCurrentAction();
				if (pcommand) {
					if (pcommand->cFireShots > 1u) {
						pcommand->cFireShots = 1;
						pcommand->cFakeFireShots = 0;
					}
				}
				if (!commandList.IsEmpty()) {
					for (BSSimpleList<VATS_COMMAND*>* plist = commandList.m_pkNext; plist;
						plist = commandList.m_pkNext) {
						VATS_COMMAND* pdelete = plist->m_item;
						if (pdelete) {
							delete pdelete;
						}
						commandList.Remove(pdelete);
					}
				}
			} else {
				ClearCommandList();
				fEndVATSDelay = fVATSPlaybackDelay.Float();
				appTimer.SetGlobalTimeMultiplier(1.0f, 0);
			}
		}
	}
}

BGSCameraShot* VATS::GetCurrentCamera() {
	return pCamera;
}

float VATS::GetCurrentCameraMinTimer() {
	return fCameraMinTimer;
}

void VATS::ClearCommandList() {
	if (pMeleeAttack) {
		Animation* Animation_ = pVATSAttacker->Actor::GetAnimation();
		Animation_->SpecialIdleFree(1, 0);
		pMeleeAttack = 0;
	}
	while (!commandList.IsEmpty()) {
		VATS_COMMAND* pcommand = commandList.m_item;
		if (pcommand) {
			delete pcommand;
		}
		commandList.RemoveHead();
	}
}

void VATS::ClearData() {
	ClearCommandList();
	if (pCamera) {
		pCamera->FreeCamera();
	}
	pCameraList = nullptr;
	pCamera = nullptr;
	fCameraTimer = 0.0f;
	fCameraMinTimer = 0.0f;
	if (pImagespaceMod) {
		ImageSpaceModifierInstance::Stop(pImagespaceMod);
		pImagespaceMod = nullptr;
	}
	if (pTransitionImagespaceMod) {
		ImageSpaceModifierInstance::Stop(pTransitionImagespaceMod);
		pTransitionImagespaceMod = nullptr;
	}
}

void VATS::IncrementKillCount() {
	++iKillCount;
}

void VATS::EvaluateCommand(VATS_COMMAND* apCommand) {
	if (apCommand) {
		if ((uint8_t)ActionPoints::IsAttackAction(apCommand->eAction)) {
			if (apCommand->bSuccess) {
				if (apCommand->pTarget) {
					if ((!bhkWorld::QPickTimeOverMax()) &&
						(!pVATSAttacker->LineOfSight(1, apCommand->pTarget, 1, nullptr, 0))) {
						apCommand->bSuccess = (bool)0;
					}
				}
			}
			if (apCommand->pHitData) {
				apCommand->pHitData->ClearData();
			}
			HitData::InitializeHitData(apCommand->pHitData, apCommand);
		}
	}
}

void VATS::InitPlayerDeath() {
	VATS_COMMAND* v1;
	QuitVATSPlayback(0, 0);
	void* v3 = ourMalloc(sizeof(VATS_COMMAND));
	if (v3) {
		v1 = ::new (v3) VATS_COMMAND;
	} else {
		v1 = nullptr;
	}
	v1->eAction = ActionPoints::ACTION_PLAYER_VATS_DEATH;
	v1->bSuccess = (bool)1;
	v1->pTarget = pPlayer;
	QueueAction(v1);
	SetMode(VATS::VATS_PLAYBACK, 0);
	HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_START);
}

void VATS::Update() {
	float fapReward;
	bhkWorld::SetVatsReplay(eMode == VATS::VATS_PLAYBACK);
	if (eMode == VATS::VATS_PLAYBACK) {
		int32_t v3;
		if (commandList.IsEmpty()) {
		LABEL_217:;
			if (commandList.IsEmpty()) {
				if (fEndVATSDelay == 0.0) {
					fEndVATSDelay = fVATSPlaybackDelay.Float();
				} else {
					double Seconds = appTimer.fDelta;
					double v27 = BSTimer::GetGlobalTimeMultiplierTarget() * Seconds;
					fEndVATSDelay = fEndVATSDelay - v27 / BSTimer::QGlobalTimeMultiplier();
					if (!(fEndVATSDelay <= 0.0)) {
						double v23 = appTimer.fDelta;
						fVATSPlaybackTimer = v23 / BSTimer::QGlobalTimeMultiplier() + fVATSPlaybackTimer;
						if (pCamera) {
							if (pCamera->IsInitialized()) {
								pCamera->Update(fVATSPlaybackTimer);
							}
						}
						if (bDollyToNewPos) {
							appTimer.SetGlobalTimeMultiplier(0.0001f, 0);
						} else {
							if (pCamera && pCamera->IsInitialized()) {
								float afStrengthb = pCamera->Data.fGlobalTimeMult;
								appTimer.SetGlobalTimeMultiplier(afStrengthb, 0);
							} else {
								appTimer.SetGlobalTimeMultiplier(1.0f, 0);
							}
						}
						if (bLastCommandBlock) {
							if (pPlayer->GetAnimAction() != ANIM_ACTION_RELOAD &&
								pPlayer->GetAnimAction() != ANIM_ACTION_RELOAD_LOOP) {
								Controls* pcontrols_2 = appMain->GetControls();
								pcontrols_2->ForceUserAction(Controls::UACTN_BLOCK);
							}
						}
					} else {
						appTimer.SetGlobalTimeMultiplier(1.0f, 0);
						if (pVATSAttacker == VATS::pStranger ||
							(pVATSAttacker == VATS::pMissFortune ||
								(pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE ||
									(pVATSAttacker->GetAnimAction() == ANIM_ACTION_BLOCK ||
										(pVATSAttacker->GetAnimAction() == ANIM_ACTION_RELOAD ||
											(pVATSAttacker->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP_START ||
												pVATSAttacker->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP)))))) {
							SetMode(VATS::VATS_MODE_NONE, 0);
							if (iKillCount > 0) {
								TESObjectWEAP* pplayerWeapon_2;
								fapReward = 0.0f;
								TESObjectWEAP* pplayerWeapon = pPlayer->GetCurrentWeapon();
								if (pplayerWeapon) {
									pplayerWeapon_2 = pplayerWeapon;
								} else {
									pplayerWeapon_2 = pFists;
								}
								BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::PLAYER_KILL_AP_REWARD, pPlayer,
									pplayerWeapon_2, pPlayer, &fapReward);
								if (fapReward > 0.0) {
									pPlayer->RestoreActorValue(ActorValue::ACTION_POINTS, fapReward);
								}
								iKillCount = 0;
							}
							fEndVATSDelay = 0.0f;
						}
					}
				}
			}
			if (pVATSTarget) {
				if (eMode) {
					pVATS.UpdateVATSLight(pVATSTarget);
				}
			}
			return;
		}
		VATS_COMMAND* pcommand = commandList.m_item;
		Controls* pcontrols = appMain->GetControls();
		HighProcess* pcurrentProcess = static_cast<HighProcess*>(pVATSAttacker->pCurrentProcess);
		bool bgetNextCamera = 0;
		pVATSTarget = pcommand->pTarget;
		if (pCamera && (pCamera->IsInitialized() && fCameraMinTimer > 0.0)) {
			double v38 = appTimer.fDelta;
			fCameraMinTimer = fCameraMinTimer - v38 / BSTimer::QGlobalTimeMultiplier();
			if (fCameraMinTimer < 0.0) {
				fCameraMinTimer = 0.0f;
				for (bool bnewCamera = 1; bnewCamera; bnewCamera = UseNextCamera()) {
					int32_t CameraAction = pCamera->GetCameraAction();
					if (eVATSCameraAction <= CameraAction) {
						break;
					}
				}
			}
		} else {
			if (pCamera) {
				if (pcommand != pcameraCommand) {
					pCamera->FreeCamera();
					eVATSCameraAction = 0;
					pCamera = 0;
					pCameraList = 0;
					fCameraTimer = 0.0f;
					fCameraMinTimer = 0.0f;
				}
			}
		}
		if (pCamera) {
			if (fCameraTimer > 0.0) {
				double v37 = appTimer.fDelta;
				fCameraTimer = fCameraTimer - v37 / BSTimer::QGlobalTimeMultiplier();
				if (fCameraTimer <= 0.0) {
					bgetNextCamera = 1;
					fCameraTimer = 0.0f;
					fCameraMinTimer = 0.0f;
				}
			}
		}
		if (!pCameraList || bgetNextCamera) {
			bool bnewCamera_2 = 0;
			if (!pCameraList) {
				bnewCamera_2 = 1;
				EvaluateCommand(pcommand);
				if (pForceCameraPath) {
					pCameraPath = pForceCameraPath;
				} else {
					if (pForceDefaultCameraPath) {
						pCameraPath = pForceDefaultCameraPath;
						pForceDefaultCameraPath = 0;
					} else {
						pCameraPath = pCameraPathManager->GetCameraPathToPlay(pVATSAttacker, pcommand->pTarget);
					}
				}
				if (pCameraPath) {
					if (pCameraPath->GetZoomDefault()) {
						eCameraZoom = 0;
					} else {
						if (pCameraPath->GetZoomDisable()) {
							eCameraZoom = 1;
						} else {
							if (pCameraPath->GetZoomShotList()) {
								eCameraZoom = 2;
							}
						}
					}
				} else {
					eCameraZoom = 0;
				}
				if (pCameraPath) {
					pCameraList = &pCameraPath->ShotList;
				} else {
					pCameraList = 0;
				}
			}
			if (pCameraList) {
				if (bgetNextCamera) {
					if (!UseNextCamera()) {
						if ((pVATSAttacker == VATS::pStranger && pcommand->bStranger) ||
							(pVATSAttacker == VATS::pMissFortune && pcommand->bMissFortune)) {
							pcommand->cFireShots = 0;
						} else {
							SetLastProjectile(0);
						}
					}
				} else {
					pCamera = pCameraList->m_item;
					if (pCamera) {
						if (static_cast<double>(fCameraMinTimer) < pCamera->Data.fMinTime) {
							fCameraMinTimer = pCamera->Data.fMinTime;
						}
					}
					if (pCamera && pCamera->GetIsFirstPersonCamera()) {
						pPlayer->SetFirstPerson(1);
					} else {
						bSwitchTo3rdPerson = 1;
					}
				}
			}
			if (bnewCamera_2) {
				UpdateMeleeAttack(pcommand);
			}
			pcameraCommand = pcommand;
			if (icameraIndex > 1) {
				icameraIndex = 0;
			}
		}
		AnimIdle* panimIdle = pVATSAttacker->Actor::GetAnimation()->spAnimIdle.m_pObject;
		if (pMeleeAttack) {
			if (!panimIdle ||
				(panimIdle->pIdleForm != pMeleeAttack ||
					(panimIdle->eFlags >= AnimIdle::ANIM_IDLE_PLAYING && !panimIdle->GetSequence()))) {
				ANIM_GROUP_SECTION a2 = pMeleeAttack->GetIdleAnimGroupSection();
				TESIdleForm* afStrength = pMeleeAttack;
				Animation* Animation_ = pVATSAttacker->Actor::GetAnimation();
				Animation_->SpecialIdleQueue(afStrength, a2);
			}
		}
		if (!pCamera) {
		LABEL_169:;
			float fuseActionPoints = 0.0f;
			if (ActionPoints::IsAttackAction(pcommand->eAction)) {
				if (IsMeleeCommand(pcommand) ||
					(VATSQueuedAction::IsActionGrenadeOrMine(pcommand->eAction) ||
						(pcommand->eAction == ActionPoints::ACTION_ATTACK_LAUNCHER ||
							(pPlayer->GetAnimAction() == ANIM_ACTION_RELOAD ||
								pPlayer->GetAnimAction() == ANIM_ACTION_RELOAD_LOOP)))) {
					bLastCommandBlock = 0;
				} else {
					pcontrols->ForceUserAction(Controls::UACTN_BLOCK);
					bLastCommandBlock = 1;
				}
				if (!pcommand->cFireShots) {
					bool bremoveCommand = 0;
					if (!pCamera || fCameraMinTimer == 0.0) {
						if (IsMeleeCommand(pcommand)) {
							Animation* v11;
							uint16_t Group;
							UpdateCurrentCameraAction(pcommand);
							if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE ||
								(v11 = pVATSAttacker->Actor::GetAnimation(),
									(Group = v11->GetGroup(AGS_WEAPON), TESAnimGroup::IsAttackSpinAction(Group)))) {
								bremoveCommand = 1;
							}
						} else {
							if (!pLastProjectile) {
								bremoveCommand = 1;
							}
						}
					}
					if (bremoveCommand) {
						fuseActionPoints = pcommand->fActionPoints;
						FreeHeadAction();
						pcommand = 0;
					}
				}
				TESObjectWEAP* pWeap = 0;
				if (pcurrentProcess->GetCurrentWeapon()) {
					pWeap = static_cast<TESObjectWEAP*>(pcurrentProcess->GetCurrentWeapon()->GetItemChangeObject());
				}
			} else {
				if (bVATSFlushCommandWhenAnimActionIsDone) {
					if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
						fuseActionPoints = pcommand->fActionPoints;
						FreeHeadAction();
						pcommand = 0;
					}
				} else {
					switch (pcommand->eAction) {
					case ActionPoints::ACTION_RELOAD: {
						if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
							if (pcurrentProcess->GetCurrentWeapon()) {
								ItemChange* v14 = pcurrentProcess->GetCurrentWeapon();
								bool bclipMod = v14->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
								TESObjectWEAP* a2a = static_cast<TESObjectWEAP*>(pcurrentProcess->GetCurrentWeapon()
										->GetItemChangeObject());
								pVATSAttacker->ReloadWeaponNV(a2a, 2, bclipMod, false);
								pcurrentProcess->SetSustainedFire(0);
							}
							bVATSFlushCommandWhenAnimActionIsDone = 1;
						}
						break;
					}
					case ActionPoints::ACTION_CROUCH:
					case ActionPoints::ACTION_STAND: {
						if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
							if (!pVATSAttacker->IsParalyzed() && !pVATSAttacker->IsDead(0) &&
								!pVATSAttacker->IsKnockedOut() && !pVATSAttacker->IsRestrained() &&
								!pVATSAttacker->IsUnConscious()) {
								if (pVATSAttacker == pPlayer) {
									pPlayer->SetMoveMode(1024u, pcommand->eAction == ActionPoints::ACTION_CROUCH);
								} else {
									pVATSAttacker->SetForceSneak(pcommand->eAction == ActionPoints::ACTION_CROUCH);
								}
							}
							fuseActionPoints = pcommand->fActionPoints;
							FreeHeadAction();
							pcommand = 0;
						}
						break;
					}
					case ActionPoints::ACTION_TOGGLE_WEAPON_DRAWN: {
						if (pVATSAttacker->GetAnimAction() == ANIM_ACTION_NONE) {
							bool WantWeaponDrawn = pVATSAttacker->GetWantWeaponDrawn();
							pVATSAttacker->SetWantWeaponDrawn(!WantWeaponDrawn);
							bVATSFlushCommandWhenAnimActionIsDone = 1;
						}
						break;
					}
					default: {
						break;
					}
					}
				}
			}
			if (fuseActionPoints > 0.0) {
				pPlayer->UseActionPoints(fuseActionPoints);
				fuseActionPoints = 0.0f;
			}
			goto LABEL_217;
		}
		if (pCamera->IsInitialized() ||
			((UpdateCurrentCameraAction(pcommand), !pCamera) ||
				((v3 = pCamera->GetCameraAction(), eVATSCameraAction < v3) &&
					pCamera->GetCameraAction() != BGSCameraShot::CA_ZOOM))) {
		LABEL_150:;
			if (bDollyToNewPos && fDollyPercentage == 0.0) {
				fDollyStartTime = fVATSPlaybackTimer;
			}
			double v31 = appTimer.fDelta;
			fVATSPlaybackTimer = v31 / BSTimer::QGlobalTimeMultiplier() + fVATSPlaybackTimer;
			double v30 = appTimer.fDelta;
			fVATSCommandTimer = v30 / BSTimer::QGlobalTimeMultiplier() + fVATSCommandTimer;
			if (fVATSCameraMaxTime.Float() < static_cast<double>(fVATSCommandTimer)) {
				QuitVATSPlayback(0, 0);
			}
			if (pCamera && pCamera->IsInitialized()) {
				if (bSwitchTo3rdPerson) {
					pPlayer->SetFirstPerson(0);
					bSwitchTo3rdPerson = 0;
				}
				pCamera->Update(fVATSPlaybackTimer);
				if (bDollyToNewPos) {
					appTimer.SetGlobalTimeMultiplier(0.0001f, 0);
				} else {
					if (pcameraCommand && pcameraCommand->eAction == ActionPoints::ACTION_TARGET_KILL_SHOT) {
						appTimer.SetGlobalTimeMultiplier(0.3f, 0);
					} else {
						float afStrengtha = pCamera->Data.fGlobalTimeMult;
						appTimer.SetGlobalTimeMultiplier(afStrengtha, 0);
					}
				}
				float ftransstrength = fDollyPercentage;
				if (fDollyPercentage > 0.5) {
					ftransstrength = 1.0 - ftransstrength;
				}
				float v29 = ftransstrength * fVATSCamTransRBStrengthCap.Float();
				pTransitionImagespaceMod->SetStrength(v29);
			} else {
				appTimer.SetGlobalTimeMultiplier(1.0f, 0);
			}
			goto LABEL_169;
		}
		fCameraTimer = pCamera->Data.fMaxTime;
		if (fCameraTimer <= 0.0010000000474974513) {
			fCameraTimer = fVATSCameraMaxTime.Float();
			fVATSCommandTimer = 0.0f;
		}
		if (fCameraMinTimer < static_cast<double>(fVATSCameraMinTime.Float())) {
			fCameraMinTimer = fVATSCameraMinTime.Float();
		}
		if (pcommand->pTarget &&
			(pCamera->Data.eLocation || pcommand->eAction == ActionPoints::ACTION_TARGET_KILL_SHOT)) {
			pCamera->InitCamera(pcommand->pTarget, eCameraZoom);
		} else {
			pCamera->InitCamera(pVATSAttacker, eCameraZoom);
		}
		Animation* v4 = pVATSAttacker->Actor::GetAnimation();
		NiAVObject* pattacker3d = v4->pAccumRoot->GetAtChecked(0);
		NiAVObject* pattackerWeapon = 0;
		NiAVObject* ptarget3d = 0;
		NiAVObject* ptargetTorso = 0;
		if (pCamera->GetFollowsBone()) {
			BaseProcess* CurrentAIProcess = pVATSAttacker->pCurrentProcess;
			BipedAnim* AsForm = pVATSAttacker->GetVATSBiped();
			pattackerWeapon = CurrentAIProcess->GetWeaponBone(AsForm);
		}
		if (pcommand->pTarget) {
			ptarget3d = (NiAVObject*)pcommand->pTarget->Get3D();
			if (pcommand->pTarget->IsActor()) {
				Actor* pactor = static_cast<Actor*>(pcommand->pTarget);
				TESActorBase* ActorBase = pactor->GetActorBase();
				BGSBodyPartData* pbodyPartData = ActorBase->GetBodyPartData();
				if (pbodyPartData) {
					BGSBodyPart* pbodyPart = 0;
					if (pcommand->pHitData->eDamageLimb != BGSBodyPart::LIMB_NONE) {
						pbodyPart = pbodyPartData->GetBodyPart_005E5130(pcommand->eLimb);
						if (pbodyPart) {
							if (pactor->GetDismembered(pcommand->pHitData->eDamageLimb)) {
								BaseProcess* v34 = pactor->pCurrentProcess;
								NiNode* pdismemberedRoot = v34->GetDamageNode(pcommand->pHitData->eDamageLimb);
								if (pdismemberedRoot) {
									ptargetTorso =
										BSUtilities::GetObjectByName(pdismemberedRoot, pbodyPart->GetTargetName());
								}
							} else {
								ptargetTorso = BSUtilities::GetObjectByName(ptarget3d, pbodyPart->GetTargetName());
							}
						}
					}
					if (!ptargetTorso) {
						pbodyPart = pbodyPartData->GetBodyPart(BGSBodyPart::LIMB_TORSO);
						if (pbodyPart) {
							ptargetTorso = BSUtilities::GetObjectByName(ptarget3d, pbodyPart->GetTargetName());
						}
					}
				}
			}
		}
		BGSCameraShot::CAM_OBJECT EnchantmentSource = pCamera->Data.eLocation;
		if (EnchantmentSource) {
			if (EnchantmentSource == BGSCameraShot::CO_PROJECTILE) {
				if (!pCamera->GetLocationNode()) {
					if (pFirstProjectile) {
						NiNode* v6 = pFirstProjectile->Get3D();
						pCamera->SetCameraLocation(v6);
					} else {
						if (pLastProjectile) {
							NiNode* v7 = pLastProjectile->Get3D();
							pCamera->SetCameraLocation(v7);
						} else {
							if (pattackerWeapon) {
								pCamera->SetCameraTarget(pattackerWeapon);
							} else {
								pCamera->SetCameraTarget(pattacker3d);
							}
						}
					}
				}
			LABEL_116:;
				BGSCameraShot::CAM_OBJECT ParentCell = pCamera->Data.eTarget;
				if (ParentCell) {
					if (ParentCell == BGSCameraShot::CO_PROJECTILE) {
						if (pCamera->GetTargetNode()) {
							goto LABEL_143;
						}
						if (pFirstProjectile) {
							if (pCamera->GetNoTracer() && pFirstProjectile->IsFlagSet(Projectile::TRACER)) {
								pFirstProjectile->SetFlag(Projectile::TRACER, 0);
								pFirstProjectile->Process3D();
							}
							NiNode* v8 = pFirstProjectile->Get3D();
							pCamera->SetCameraTarget(v8);
							goto LABEL_143;
						}
						if (pLastProjectile) {
							if (pCamera->GetNoTracer() && pFirstProjectile->IsFlagSet(Projectile::TRACER)) {
								pLastProjectile->SetFlag(Projectile::TRACER, 0);
								pLastProjectile->Process3D();
							}
							NiNode* v9 = pLastProjectile->Get3D();
							pCamera->SetCameraTarget(v9);
							goto LABEL_143;
						}
						if (pattackerWeapon) {
							goto LABEL_127;
						}
					} else {
						if (ParentCell != BGSCameraShot::CO_TARGET) {
							goto LABEL_143;
						}
						if (ptargetTorso) {
							pCamera->SetCameraTarget(ptargetTorso);
						LABEL_143:;
							NiAVObject* ptarget = pCamera->GetImagespaceTarget();
							if (pImagespaceMod) {
								float* v10 = &fVATSImageSpaceTransitionTime.Float();
								pImagespaceMod->StartTransition(pCamera->pFormImageSpaceModifying, *v10, 1.0f, ptarget);
							} else {
								pImagespaceMod =
									ImageSpaceModifierInstanceForm::Trigger(pCamera->pFormImageSpaceModifying, 1.0f,
										ptarget);
								if (pImagespaceMod) {
									pImagespaceMod->SetPermanent(1);
								}
							}
							if (pCamera) {
								if (!pCamera->IsInitialized()) {
									UseNextCamera();
								}
							}
							goto LABEL_150;
						}
						if (ptarget3d) {
							pCamera->SetCameraTarget(ptarget3d);
							goto LABEL_143;
						}
						if (pattackerWeapon) {
						LABEL_127:;
							pCamera->SetCameraTarget(pattackerWeapon);
							goto LABEL_143;
						}
					}
				} else {
					if (pattackerWeapon) {
						goto LABEL_127;
					}
				}
				pCamera->SetCameraTarget(pattacker3d);
				goto LABEL_143;
			}
			if (EnchantmentSource != BGSCameraShot::CO_TARGET) {
				goto LABEL_116;
			}
			if (ptargetTorso) {
				pCamera->SetCameraLocation(ptargetTorso);
				goto LABEL_116;
			}
			if (ptarget3d) {
				pCamera->SetCameraLocation(ptarget3d);
				goto LABEL_116;
			}
			if (!pattackerWeapon) {
			LABEL_107:;
				pCamera->SetCameraLocation(pattacker3d);
				goto LABEL_116;
			}
		} else {
			if (!pattackerWeapon) {
				goto LABEL_107;
			}
		}
		pCamera->SetCameraLocation(pattackerWeapon);
		goto LABEL_116;
	}
}

void VATS::FreeHeadAction() {
	if (!commandList.IsEmpty()) {
		VATS_COMMAND* pcommand = commandList.m_item;
		if (pcommand) {
			delete pcommand;
			VATS_COMMAND* pcommand_2 = 0;
			commandList.RemoveHead();
			if (pMeleeAttack) {
				Animation* Animation_ = pVATSAttacker->Actor::GetAnimation();
				Animation_->SpecialIdleFree(1, 0);
				pMeleeAttack = 0;
			}
			while (!commandList.IsEmpty()) {
				pcommand_2 = commandList.m_item;
				bool v4 = 0;
				if (pcommand_2) {
					if (pcommand_2->pTarget) {
						if (pcommand_2->pTarget->IsDead(0)) {
							v4 = 1;
						} else {
							if (pcommand_2->bStranger) {
								double Health = pcommand_2->pTarget->GetHealth();
								v4 = static_cast<double>(iVATSStrangerMaxHP.Int()) < Health;
							}
						}
					}
				}
				if (!v4) {
					break;
				}
				if (pcommand_2) {
					delete pcommand_2;
				}
				pcommand_2 = 0;
				commandList.RemoveHead();
			}
			if (pcommand_2) {
				if (pcommand_2->bStranger) {
					pVATSAttacker = VATS::pStranger;
					MiscStatManager::Increment(MiscStatManager::MS_STRANGER_ASSISTS);
				}
			}
			if (pcommand_2) {
				if (pcommand_2->bMissFortune) {
					pVATSAttacker = VATS::pMissFortune;
					MiscStatManager::Increment(MiscStatManager::MS_MISS_FORTUNE);
				}
			}
		}
	}
}

void VATS::UpdateMeleeAttack(VATS_COMMAND* apCommand) {
	NiPoint3 ecx0;
	if (apCommand->cFireShots) {
		if (pVATSAttacker == VATS::pStranger || (pVATSAttacker == VATS::pMissFortune || IsMeleeCommand(apCommand))) {
			if (apCommand->pTarget) {
				float a3 = 0.0;
				if (apCommand->bStranger) {
					a3 = fVATSStrangerDistance.Float();
					if (VATS::pStranger->GetAlphaMult() != 1.0) {
						VATS::pStranger->SetAlphaMult(1.0);
						VATS::pStranger->UpdateAlpha();
						PlayMysteriousStrangerMusic();
					}
				} else {
					if (apCommand->bMissFortune) {
						a3 = fVATSStrangerDistance.Float();
						if (VATS::pMissFortune->GetAlphaMult() != 1.0) {
							VATS::pMissFortune->SetAlphaMult(1.0);
							VATS::pMissFortune->UpdateAlpha();
							PlayMysteriousStrangerMusic();
						}
					} else {
						TESObjectWEAP* CurrentWeapon = pVATSAttacker->GetCurrentWeapon();
						a3 = CombatUtilities::GetWeaponReach(pVATSAttacker, CurrentWeapon);
						if (pVATSAttacker->GetCurrentWeapon()) {
							TESObjectWEAP* v3 = pVATSAttacker->GetCurrentWeapon();
							if (v3->IsMeleeWeapon()) {
								a3 = a3 * fVATSMeleeWarpDistanceMult.Float();
							}
						} else {
							a3 = a3 * fVATSH2HWarpDistanceMult.Float();
						}
					}
				}
				if (pVATSAttacker == VATS::pStranger || pVATSAttacker == VATS::pMissFortune) {
					const NiPoint3* v4 = &pPlayer->GetLocationOnReference();
					pVATSAttacker->SetLocationOnReference(*v4);
				}
				if (Pathing::FindSafeStraightLinePathPoint(pVATSAttacker, apCommand->pTarget, a3, &ecx0)) {
					pVATSAttacker->SetPosition(ecx0);
					pVATSAttacker->SetHeading(apCommand->pTarget);
					if (pPlayer == pVATSAttacker && !GetLineOfSight(apCommand)) {
						const NiPoint3* v5 = &apCommand->pTarget->GetLocationOnReference();
						pVATSAttacker->SetPosition(*v5);
						pVATSAttacker->SetHeading(apCommand->pTarget);
					}
				} else {
					const NiPoint3* v6 = &apCommand->pTarget->GetLocationOnReference();
					pVATSAttacker->SetPosition(*v6);
					pVATSAttacker->SetHeading(apCommand->pTarget);
				}
				Animation* v7 = pVATSAttacker->GetAnimation();
				v7->UpdateSceneGraphNoController();
			}
		}
		pMeleeAttack = pIdleManager->GetIdleToPlay(pVATSAttacker, apCommand->pTarget);
		if (pMeleeAttack) {
			if (pMeleeAttack->GetIdleAnimGroupSection() != AGS_WEAPON) {
				const char* v8 = pMeleeAttack->GetFormEditorID();
				Warning("COMBAT: Bad VATS Melee Attack '%s' in the idle manager. Attacks should be tagged for 'Weapon' "
						"anim group section.",
					v8);
				pMeleeAttack = 0;
			}
		}
	}
}

void VATS::UpdateVATSLight(TESObjectREFR* apVictim) {
	NiPoint3 v33;
	if (apVictim->IsActor()) {
		if (apVictim->Get3D()) {
			if (spVATSLight.m_pObject) {
				Actor* pactor = static_cast<Actor*>(apVictim);
				if (pactor->GetCurrentAIProcess()) {
					NiMatrix3 rotmat;
					NiNode* v2 = apVictim->Get3D();
					NiPoint3 pos = v2->GetWorldBound().m_kCenter;
					const NiPoint3* a2 = &apVictim->GetLocationOnReference();
					const NiPoint3* v3 = &pVATSAttacker->GetLocationOnReference();
					NiPoint3 vec_2 = *v3 - *a2;
					float a2a = gs_fVATSLightAngle_VATS.operator float();
					rotmat.MakeZRotation(a2a);
					vec_2.Unitize();
					vec_2 = vec_2 * rotmat;
					float a2b = gs_fVATSLightDistance_VATS.operator float();
					vec_2.operator*=(a2b);
					pos = pos + vec_2;
					double v4 = gs_fVATSLightElevation_VATS.operator float();
					pos.z = v4 + pos.z;
					float a2c = gs_fVatsLightColorB_VATS.Float();
					float a1 = gs_fVatsLightColorG_VATS.Float();
					float* v5 = &gs_fVatsLightColorR_VATS.Float();
					// The exe builds the colour with NiPoint3's folded three-float constructor (0x416870).
					FNVD_CONSTRUCTED_LOCAL(NiColor, lightcolor, nullptr, *v5, a1, a2c);
					int32_t inumberlights = 0;
					float fambient = 0.0f;
					float fdirectional = 0.0f;
					float a2d = pactor->GetHeight();
					NiPoint3* a2e = ::new (&v33) NiPoint3(0.0f, 0.0f, a2d);
					const NiPoint3* v6 = &pactor->GetLocationOnReference();
					NiPoint3 ktarget = *v6 + *a2e;
					NiLight* a2f = spVATSLight.m_pObject->GetLight();
					NiPoint3 x2 = pVATSAttacker->GetLocationOnReference();
					NiPoint3 v48 = ktarget;
					ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
					float ftargetluminance =
						ShadowSceneNode_->GetViewLuminanceAtPoint(v48, inumberlights, fambient, fdirectional, x2, a2f) *
						100.0;
					float a1a = gs_fVATSLightLevelMax_VATS.operator float();
					float x = gs_fVATSLightLevelMin_VATS.operator float();
					float a2h = FLerp(1.0f, 0.0f, x, a1a, ftargetluminance);
					float a2i = BSMAX(0.0f, a2h);
					float intensity = BSMIN(1.0f, a2i);
					lightcolor *= intensity;
					NiNode* v8 = apVictim->Get3D();
					double v11 = (pos - v8->GetWorldBound().m_kCenter).Length();
					float radius = v11 + v11;
					NiLight* Light = spVATSLight.m_pObject->GetLight();
					Light->SetWorldTranslate(pos);
					// The exe builds the specular colour with NiPoint3's folded three-float constructor (0x416870).
					FNVD_CONSTRUCTED_LOCAL(NiColor, specularcolor, nullptr, radius, 0.0f, 0.0f);
					NiLight* v13 = spVATSLight.m_pObject->GetLight();
					v13->SetSpecularColor(specularcolor);
					NiLight* v14 = spVATSLight.m_pObject->GetLight();
					v14->SetDiffuseColor(lightcolor);
					NiLight* v15 = spVATSLight.m_pObject->GetLight();
					v15->SetAppCulled(0);
				}
			}
		}
	}
}

void VATS::RemoveLastAction(ActionPoints::Action& aeAction, bool& abIronSights) {
	for (BSSimpleList<VATS_COMMAND*>* iter = &commandList; iter && !iter->IsEmpty(); iter = iter->m_pkNext) {
		if (!iter->m_pkNext) {
			VATS_COMMAND* pcommand = iter->m_item;
			aeAction = pcommand->eAction;
			abIronSights = pcommand->bIronSights;
			if (pcommand) {
				delete pcommand;
			}
			commandList.Remove(pcommand);
			return;
		}
	}
}

void VATS::QuitVATSPlayback_0(TESBoundObject* apObject, ExtraDataList* apExtra) {
	pUnEquipWeapon = apObject;
	pUnEquipExtra = apExtra;
	QuitVATSPlayback(1, 1);
}

NiPoint3* VATS::GetCamshotTargetPos(NiPoint3* a2) {
	NiPoint3 dst;
	NiPoint3 apResult;
	NiPoint3 v10 = NiPoint3_ZERO;
	if (pCamera) {
		v10 = *pCamera->GetTargetPos(&apResult);
	}
	if (v10.operator==(NiPoint3_ZERO)) {
		NiPoint3 result;
		float z = pPlayer->GetEyeHeight();
		NiPoint3* za = ::new (&dst) NiPoint3(static_cast<float>(0.0), static_cast<float>(0.0), z);
		NiNode* v2 = pVATSAttacker->TESObjectREFR::Get3D();
		const NiPoint3* WorldTranslate = &v2->GetWorldTranslate();
		v10 = (result = *WorldTranslate + *za);
	}
	*a2 = v10;
	return a2;
}

// The exe's cleanup-only C++ EH frame is left out.
bool VATS::GetLineOfSight(VATS_COMMAND* apCommand) {
	FNVD_INLINED_LOCAL(hkpAllRayHitCollector, collector, std::nullopt, std::nullopt);
	CFilter fltr;
	bhkWorld* pw;
	int32_t v30;
	void* retaddr;
	int32_t* v25 = &v30;
	Actor* apTarget = 0;
	if (apCommand->pTarget->IsActor()) {
		apTarget = static_cast<Actor*>(apCommand->pTarget);
	}
	if (!apTarget) {
		return 0;
	}
	if (apTarget->GetParentCell()) {
		pw = apTarget->GetParentCell()->GetbhkWorld();
	} else {
		pw = 0;
	}
	if (!pw) {
		return 0;
	}
	int32_t v20 = 0x26206;
	bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK1, 0x26206u);
	NiPoint3 aLocation = pVATSAttacker->GetLocationOnReference();
	alignas(16) NiPoint3 targetLocation = apTarget->GetLocationOnReference();
	aLocation.z = aLocation.z + 60.0;
	targetLocation.z = targetLocation.z + 60.0;
	FNVD_CONSTRUCTED_LOCAL(bhkPickData, pd, nullptr);
	fltr.iFilter = 0;
	fltr.SetLayer(L_CUSTOMPICK1);
	bool v3 = apTarget->IsActor();
	Actor* pactorTarget = v3 ? apTarget : 0;
	if (pactorTarget && pactorTarget->GetCharController()) {
		bhkCharacterController* CharController = pactorTarget->GetCharController();
		uint32_t FilterGroup = CharController->GetFilterGroup();
		fltr.SetGroup(FilterGroup);
	} else {
		uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
		fltr.SetGroup(NewSystemGroup);
	}
	pd.SetFilter(fltr);
	pd.NiSetFrom(aLocation);
	pd.NiSetTo(targetLocation);
	::new (&collector) hkpAllRayHitCollector;
	pd.SetAllCollector(&collector);
	char v13 = 0;
	pw->PickObject(pd);
	for (int32_t i = 0; !v13; ++i) {
		int32_t FormHealth_0 = (&pd.pAllCollector->getHits())->m_size;
		if (i >= FormHealth_0) {
			break;
		}
		const hkpWorldRayCastOutput* v8 = &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i);
		FNVD_CONSTRUCTED_LOCAL(hkpWorldRayCastOutput, point, nullptr, *v8);
		NiAVObject* pav = GetAVObjectForCollidable(point.m_rootCollidable);
		TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
		if (pav && (pref != apTarget && pref != pVATSAttacker)) {
			collector.~hkpAllRayHitCollector();
			return 0;
		}
	}
	collector.~hkpAllRayHitCollector();
	return 1;
}

void VATS::SetVATSLight(NiLight* apLight, bool abRemoveOld) {
	if (abRemoveOld && spVATSLight.operator!=(static_cast<ShadowSceneLight*>(0))) {
		spVATSLight.operator=(static_cast<ShadowSceneLight*>(0));
	}
	if (apLight) {
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		spVATSLight.operator=(ShadowSceneNode_->AddLight(apLight, 1));
		ShadowSceneNode* v4 = BSShaderManager::GetShadowSceneNode(0);
		v4->AttachChild(apLight, 1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void VATS::AppendCamTarget(TESObjectREFR* apTarget, int32_t a3) {
	VATS_COMMAND* v5;
	QuitVATSPlayback(0, 0);
	VATS_COMMAND* apMemory = static_cast<VATS_COMMAND*>(ourMalloc(sizeof(VATS_COMMAND)));
	if (apMemory) {
		v5 = ::new (apMemory) VATS_COMMAND;
	} else {
		v5 = 0;
	}
	v5->eAction = ActionPoints::ACTION_TARGET_KILL_SHOT;
	v5->bSuccess = 1;
	v5->pTarget = apTarget;
	if (a3 == -1) {
		a3 = 0;
	}
	v5->pHitData->eDamageLimb = static_cast<BGSBodyPart::LIMB_ENUM>(a3);
	if (pPlayer->pCurrentProcess) {
		BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
		if (CurrentAIProcess->GetCurrentWeapon()) {
			BaseProcess* v3 = pPlayer->pCurrentProcess;
			v5->pHitData->pWeapon = static_cast<TESObjectWEAP*>(v3->GetCurrentWeapon()->pContainerObj);
		}
	}
	v5->pHitData->SetFlag(HitData::HD_FATAL, 1);
	pVATSAttacker = pPlayer;
	QueueAction(v5);
	SetMode(VATS::VATS_PLAYBACK, 0);
}

uint32_t VATS::GetCount() {
	// The queued commands counted in place: the head node is this list's inline first node.
	uint32_t uiCount = 0;
	for (const BSSimpleList<VATS_COMMAND*>* pNode = &commandList; pNode; pNode = pNode->GetNext()) {
		if (pNode->GetItem()) {
			++uiCount;
		}
	}
	return uiCount;
}

VATS_COMMAND::VATS_COMMAND() {
	Initialize();
}

VATS_COMMAND::~VATS_COMMAND() {
	if (pHitData) {
		ourFree(pHitData);
		pHitData = nullptr;
	}
}

void VATS_COMMAND::Initialize() {
	HitData* v1;
	eAction = ActionPoints::ACTION_COUNT;
	bSuccess = (bool)0;
	bIronSights = (bool)0;
	bStranger = (bool)0;
	bParalyzingPalm = (bool)0;
	cFireShots = (uint8_t)0;
	cFakeFireShots = (uint8_t)0;
	pTarget = nullptr;
	eLimb = ActorValue::ACTOR_VALUE_COUNT;
	void* v3 = ourMalloc(sizeof(HitData));
	if (v3) {
		v1 = ::new (v3) HitData;
	} else {
		v1 = nullptr;
	}
	pHitData = v1;
	fTotalDamage = (float)0.0;
	fActionPoints = (float)0.0;
	fCumulativeDamage = (float)0.0;
	bMissFortune = (bool)0;
}

void VATS_COMMAND::operator delete(void* apMemory) {
	ourFree(apMemory);
}

float bhkWorld::QMaxPickTime() {
	if (bhkWorld::bVatsReplay) {
		return bhkWorld::fMaxPickTimeVATS;
	} else {
		return bhkWorld::fMaxPickTime;
	}
}
