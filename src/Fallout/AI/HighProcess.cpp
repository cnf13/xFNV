#include "Fallout/AI/HighProcess.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkBoxShape.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSPortal.h"
#include "BSMain/NiBSBoneLODController.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSSystem/BSTMessageQueue.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSIdleMarker.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/KFModel.h"
#include "Fallout Shared/LipSynchAnim.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageDataUseWeapon.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout Shared/Pathfinding/PathingRequestHide.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESLevSpell.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/ActorValueCache.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/AvoidAreaStruct.h"
#include "Fallout/AI/BackUpPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatDetectionCycle.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/MuzzleFlash.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DetectionEvent.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/EscortPackage.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/GuardPackage.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MiddleLowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SandBoxPackage.h"
#include "Fallout/AI/SearchPackage.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/AI/StartCombatStates.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Dialogue/Conversation.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/DetectLifeEffect.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>

// The perf timers RunDetection and ReconcileDetectionData are bracketed with (IDA:
// BSPerfTimer::Detection::RunDetection and ::ReconcileDetectionData; the PDB's RunDetectionTimer and
// EvaluateDetectionTimer).
static BSPerformanceTimer BSPerfTimer_Detection_RunDetection("Detection", "RunDetection", true, true);
static BSPerformanceTimer EvaluateDetectionTimer("Detection", "ReconcileDetectionData", true, true);

// The PDB's file-local NiPoint3 start, which the exe's initialiser builds with the empty constructor; no src body
// reads it.
static NiPoint3 start;

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
BSSimpleList<TESObjectREFR*> DoorRefListCrime;
BSSimpleList<TESObjectREFR*> TempRefList;
BSPerformanceTimer HighUpdateTimer("AIProcessing", "HighUpdate", true, true);
SettingT<INISettingCollection> fHealthBarWidth("fHealthBarWidth:GamePlay", 40.0f);
SettingT<INISettingCollection> fHealthBarHeight("fHealthBarHeight:GamePlay", 4.0f);
SettingT<INISettingCollection> fHealthBarSpeed("fHealthBarSpeed:GamePlay", 80.0f);
SettingT<INISettingCollection> fHealthBarFadeOutSpeed("fHealthBarFadeOutSpeed:GamePlay", 1.0f);
SettingT<INISettingCollection> fHealthBarEmittanceTime("fHealthBarEmittanceTime:GamePlay", 1.5f);
SettingT<INISettingCollection> fHealthBarEmittanceFadeTime("fHealthBarEmittanceFadeTime:GamePlay", 0.5f);
SettingT<INISettingCollection> fINIDetectDoorsForPathingTime("fINIDetectDoorsForPathingTime:Pathing", 0.5f);

ObjectstoAcquire::ObjectstoAcquire()
	: pRef(nullptr), pObject(nullptr), bSteal(false), bKill(false), iamount(0), iWantValue(2147483647u),
	  pExtra(nullptr), eType(ACQUIRE_TYPE_NONE) {}

void* ObjectstoAcquire::Delete(uint32_t aiFlags) {
	// The object's destructor is empty: only the free is left.
	if (aiFlags & 1) {
		ourFree(this);
	}
	return this;
}

DetectionState::DetectionState()
	: pActor(nullptr), eDetectionLevel(DETECTION_LOST), iLevel(-100), LastPositionDetected(NiPoint3_ZERO),
	  b360view(false), bcombat(false), bLineofSight(false), bEvaluated(true), iDetectedEventLevel(-100) {
	fLastTimeDetected = CombatTimeStamp(-3.4028235e+38f);
}

void DetectionState::CopyNewData(DetectionState* apState) {
	pActor = apState->pActor;
	eDetectionLevel = apState->eDetectionLevel;
	bLineofSight = apState->bLineofSight;
	iLevel = apState->iLevel;
	b360view = apState->b360view;
	bcombat = apState->bcombat;
	iDetectedEventLevel = apState->iDetectedEventLevel;
	bEvaluated = apState->bEvaluated;
	// The exe skips the copy when the time is -FLT_MAX or NaN (fcompp; test ah, 5; jp).
	if (-3.40282347e+38f < (float)apState->fLastTimeDetected) {
		LastPositionDetected = apState->LastPositionDetected;
		fLastTimeDetected = apState->fLastTimeDetected;
	}
}

void DetectionState::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveFormID(pActor, (uint32_t)0);
	int8_t apData = (int8_t)eDetectionLevel;
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&apData)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iLevel)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&LastPositionDetected)), 0xCu, 0);
	(&fLastTimeDetected)->SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bLineofSight)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&b360view)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bcombat)), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iDetectedEventLevel)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bEvaluated)), 1u, 0);
}

void DetectionState::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	char cdetectionLevel;
	// The save holds pActor's form ID, which the exe reads into the pointer field itself; InitLoadGame resolves it.
	apLoadGameBuffer->LoadFormID(pActor);
	cdetectionLevel = 0;
	apLoadGameBuffer->LoadData(&cdetectionLevel, 1u);
	eDetectionLevel = static_cast<DETECTION_LEVEL>(cdetectionLevel);
	apLoadGameBuffer->LoadData(&iLevel, 4u);
	apLoadGameBuffer->LoadData(&LastPositionDetected, 12u);
	fLastTimeDetected.LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(&bLineofSight, 1u);
	apLoadGameBuffer->LoadData(&b360view, 1u);
	apLoadGameBuffer->LoadData(&bcombat, 1u);
	apLoadGameBuffer->LoadData(&iDetectedEventLevel, 4u);
	if (apLoadGameBuffer->GetVersion() >= 8u) {
		apLoadGameBuffer->LoadData(&bEvaluated, 1u);
	}
}

void DetectionState::InitLoadGame() {
	Actor* v2;
	if (pActor) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
		v2 = dynamic_cast<Actor*>(FormByNumericID);
	} else {
		v2 = 0;
	}
	pActor = v2;
}

HighProcess::HighProcess()
	: bEvaluateDetection(0), fDetectListTimer(0.0f), bSayGoodByePlayer(0), bProcessGreetSayTo(0), plastDetected(0),
	  fTalkTimer(0.0f), pPathLookAtTarget(0), fMaxAlpha(1.0f), fPackageEvalTimer(0.0f), fUseItemTimer(0.0f),
	  fHoldAttackTimer(0.0f), sShotsFired(0), sShotsToFire(-1), sBurstsFired(0), bCheckDeadTalk(0), bSkippedUpdate(0),
	  pNode(0), fDelayTimer(0.0f), fDistanceMoved(0.0f), fTurnTime(0.0f), cLastTurnDir(0), fEvaluateAcquireTimer(0.0f),
	  pBoneLOD(0), iLastBoneLOD(-2), sAnimAction(-1), pAnimSeq(0), bAutomaticFireAtLeastOne(0), fDetectionTimer(0.0f),
	  iLastDetection(-1), WeaponLastPos(NiPoint3_ZERO), pGreetActor(0), fSoundDelay(0.0f), bGreetingFlag(0),
	  fGreetingTimer(0.0f), fIdleTimer(1.0f), fDetectGreetTimer(0.0f), fBreathTimer(20.0f), bHeadTrack(1),
	  fVoiceTimer(0.0f), bLipQuequed(0), bWeaponAlertDrawn(0), fAwarePlayerTimer(0.0f), pIdleToPlay(0),
	  bDialoguewithPlayer(0), pGreetTopic(0), bContinuingPackageforPC(0), bActivateAnim(0), fScriptPackageEndTime(0.0f),
	  fHealthBarAlphaValue(0.0f), fActorHealthPercentage(1.0f), fHealthBarEmittanceValue(0.0f),
	  iNumberGuardsPersuing(0), bStop(0), fReEquipArmorTimer(0.0f), bUnequippedArmorToSwim(0), iHasHealingSpell(-1),
	  iHasHealingPotion(-1), pLeveledSpellList(0), cLastTurn(0), bCurrentlyReanimating(0), fDetectionModifer(0.0f),
	  fDetectionModifierTimer(0.0f), fLightLevel(0.0f), fLightLevelTimer(0.0f), pLipSynicAnim(0), bWaitingForLipFile(0),
	  bLipFileFailed(0), pCurrentMuzzleFlash(0), pActorsGeneratedDetectionEvent(0), bFinishingCombatPackage(0),
	  m_pSayToDialogueTopic(0), eFadeState(BaseProcess::FADE_IN), fFadeAlpha(0.0f), pTeleportFadeRef(0),
	  pMoveToFadeStruct(0), fHeadTrackTargetTimer(-1.0f), pLastTarget(0), bForceRotate(0), ePostAnimActon(0),
	  pActorValueCache(0), fCachedActorHeight(0.0f), eSpecialIdleType(AnimIdle::PLAY_TYPE_AUTO), fRadiationTimer(0.0f),
	  fHighestRadiation(0.0f), bPlantedExplosive(0), bNeedTalkPlayer(0), fTakeBackTimer(-1.0f), pListOfAvoidAreas(0),
	  fAvoidWaitTimer(0.0f), pSubtitleVoice(0), bHiding(0), bIsDoingSayTo(0) {
	iHourPackageEvaluated = -1;
	pDetectedActorList = new BSSimpleList<DetectionState*>;
	pLastSpokeToList = new BSSimpleList<Actor*>;
	pThreadDetectList = new BSSimpleList<DetectionState*>;
	pActorsWhoDetectMeList = new BSSimpleList<DetectionState*>;
	pTempActorsWhoDetectMeList = new BSSimpleList<DetectionState*>;
	fIdleChatterTimer =
		CombatFormulas::GetRandomBetween(fIdleChatterCommentTimer.Float(), fIdleChatterCommentTimerMax.Float());
	fCheckToTalkTimer = CombatFormulas::GetRandomBetween(fAISocialTimerForConversationsMin.Float(),
		fAISocialTimerForConversationsMax.Float());
	for (int32_t i = 0; i < 6; ++i) {
		HeadTrackingTargets[i] = nullptr;
		HeadTrackingTargetFlags[i] = false;
	}
	pDialogTarget = nullptr;
	iDetectionCounter = iStaticDetectionCounter++;
	iDetectionCounter %= 12u;
	eLevel = PROCESS_TYPE_HIGH;
	fClearTalkToListTimer = fAItalktosameNPCtimer.Float();
}

HighProcess::~HighProcess() {
	if (pPlayer->pCurrentProcess == this && !pDataHandler->GetClearingData()) {
		Warning("AI: Don't kill the PlayerCharacter HighProcess until we exit the game. The game will now crash.");
	}
	if (spHealthBarNode.m_pObject) {
		if (spHealthBarNode.m_pObject->m_pkParent) {
			spHealthBarNode.m_pObject->m_pkParent->DetachChild(spHealthBarNode.m_pObject);
		}
		spHealthBarNode = nullptr;
	}
	if (pLipSynicAnim) {
		delete pLipSynicAnim;
	}
	pLipSynicAnim = nullptr;
	pNode = nullptr;
	pLastSpokeToList->RemoveAll();
	delete pLastSpokeToList;
	for (DetectionState*& rpItem : pDetectedActorList->ItemsUntilNull()) {
		ourFree(rpItem);
	}
	pDetectedActorList->RemoveAll();
	delete pDetectedActorList;
	for (DetectionState*& rpItem : pThreadDetectList->ItemsUntilNull()) {
		ourFree(rpItem);
	}
	pThreadDetectList->RemoveAll();
	delete pThreadDetectList;
	for (DetectionState*& rpItem : pTempActorsWhoDetectMeList->ItemsUntilNull()) {
		ourFree(rpItem);
	}
	pTempActorsWhoDetectMeList->RemoveAll();
	delete pTempActorsWhoDetectMeList;
	for (DetectionState*& rpItem : pActorsWhoDetectMeList->ItemsUntilNull()) {
		ourFree(rpItem);
	}
	pActorsWhoDetectMeList->RemoveAll();
	delete pActorsWhoDetectMeList;
	FreeAvoidNodes();
	for (ACTOR_SOUND_HANDLE n = ACTOR_SOUND_HANDLE_VOICE; n < ACTOR_SOUND_HANDLE_COUNT;
		n = static_cast<ACTOR_SOUND_HANDLE>(n + 1)) {
		HighProcess::StopSoundHandle(n);
	}
	if (pGreetTopic) {
		delete pGreetTopic;
	}
	if (pLeveledSpellList) {
		pLeveledSpellList->RemoveAll();
		delete pLeveledSpellList;
		pLeveledSpellList = nullptr;
	}
	if (pCurrentMuzzleFlash) {
		delete pCurrentMuzzleFlash;
	}
	pCurrentMuzzleFlash = nullptr;
	if (pActorValueCache) {
		ourFree(pActorValueCache);
		pActorValueCache = nullptr;
	}
	spAnimIdleToPlay[0] = nullptr;
	spAnimIdleToPlay[1] = nullptr;
	spAnimIdleKF[0] = nullptr;
	spAnimIdleKF[1] = nullptr;
	spShapePhantom = nullptr;
	if (pActorsGeneratedDetectionEvent) {
		ourFree(pActorsGeneratedDetectionEvent);
	}
}

void HighProcess::Update(MobileObject* apRef, float fTimeDelta) {
	if (apRef) {
		TESPackage* pPack = 0;
		bGreetingFlag = 0;
		fGreetingTimer = 0.0f;
		BSAudio* v3 = &BSAudio::QInstance();
		v3->EndAllSoundsOfType(4u);
		fDelayTimer = 0.0f;
		pPack = GetPackageThatIsRunning();
		Actor* apActor = 0;
		if (apRef->IsActor()) {
			apActor = static_cast<Actor*>(apRef);
		}
		if (apActor->IsDying()) {
			apActor->DoDeathStuff();
			FinishDying(apActor);
		} else {
			SetGreetingTimer(0.0f);
		}
		pPlayer->ResetPlayerGreetFlag();
		bool bevalflag = 1;
		UpdateHeadTrackTargets();
		if (apActor) {
			if (apActor->GetRefPersists()) {
				PackageLocation* ploc = 0;
				if (GetCurrentPackage()) {
					GetCurrentPackage()->GetLocation();
				}
				TESObjectREFR* pRef = 0;
				bool bMustChange = 0;
				if (fScriptPackageEndTime != 0.0) {
					float fhour = appCalendar.GetHour();
					float fPackageLife = fhour - this->CurrentPackage.GetPackageStartTime();
					float ftime = std::fabs(fPackageLife) * 100.0;
					if (fScriptPackageEndTime <= static_cast<double>(ftime)) {
						fPackageEvalTimer = 0.0f;
						bMustChange = 1;
						AddToProcedureIndexRunning(apActor, 3);
						if (pPack) {
							if (pPack->GetOncePerDay()) {
								char Day = appCalendar.GetDay();
								apActor->SetRunOnceExtraPackage(pPack, Day);
							}
						}
					}
				}
				bool abnew = CheckforNewPackage(apActor, bMustChange);
				ResetTarget();
				pPack = GetPackageThatIsRunning();
				if (pPack) {
					if (abnew) {
						iNumberItemsActivate = pPack->GetInitialTargetCount();
					}
				}
				if (pPack && pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
					while (bevalflag) {
						pPack = GetPackageThatIsRunning();
						if (!pPack) {
							break;
						}
						uint32_t Count = pPack->ePROCEDURE_TYPE;
						switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
						case PROCEDURE_TRAVEL: {
							TESObjectREFR* LocReference;
							if (ploc) {
								LocReference = ploc->GetLocReference();
							}
							if (!LocReference) {
								LocReference = GetGenericLocation();
							}
							if (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
								TESBoundObject* ObjectReference;
								AddToProcedureIndexRunning(apActor, 1);
								bevalflag = 0;
								if ((LocReference &&
										(ObjectReference = LocReference->GetObjectReference(),
											ObjectReference == pXMarkerHeading)) ||
									(pPack->pPackLoc &&
										(pPack->pPackLoc->GetLocType() == PLT_EDITORLOC && !DidPathingFail(apActor)))) {
									float ftargetangle = 0.0f;
									if (LocReference) {
										ftargetangle = (&LocReference->GetAngleOnReference())->z;
									} else {
										ftargetangle = apActor->GetStartingAngle().z;
									}
									apActor->SetHeading(ftargetangle);
								}
							} else {
								if (apActor->IsPathingComplete()) {
									TESObjectREFR* pRef_2 = 0;
									if (pPack->pPackLoc) {
										pRef_2 = pPack->pPackLoc->GetLocReference();
									}
									if (pGenericLocation) {
										if (!GetRunOncePackage()) {
											pRef_2 = pGenericLocation;
										}
									}
									if (apActor->GetSitSleepState()) {
										TESPackage* CurrentPackage = apActor->GetCurrentPackage();
										if (!CurrentPackage->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
											ProcessGetUpOneHour(apActor);
										}
									}
									float a5 = pPack->GetPackageRadiusActorToLocation(apActor, 0);
									TESWorldSpace* afRadius2 = pPack->GetLocationWorld(apActor);
									TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
									apActor->SetPathfindingGoal(pPack->GetLocationCoord(apActor), LocationCell,
										afRadius2, a5, 0);
									bevalflag = 0;
									if (!apActor->pCurrentProcess->eLevel &&
										pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
										TESBoundObject* v8;
										if (!GetLockedLocation()) {
											if (pPack) {
												if (pPack->GetLockDoorsAtLocation()) {
													const NiPoint3* v22 = &apActor->GetLocationOnReference();
													const NiPoint3* v19 = &apActor->GetLocationOnReference();
													TESObjectCELL* ParentCell = apActor->GetParentCell();
													pDataHandler->EnumReferencesCloseToPoint(ParentCell, *v19, 5000.0f,
														*v22, 5000.0f, &LowProcess::LockDoorsatLocation, apActor);
												}
											}
											SetLockedLocation(1);
										}
										if (pPack) {
											if (pPack->GetUnlockDoorsAtLocation()) {
												const NiPoint3* v23 = &apActor->GetLocationOnReference();
												const NiPoint3* v20 = &apActor->GetLocationOnReference();
												TESObjectCELL* v18 = apActor->GetParentCell();
												pDataHandler->EnumReferencesCloseToPoint(v18, *v20, 5000.0f, *v23,
													5000.0f, &LowProcess::UnLockDoorsatLocation, apActor);
											}
										}
										AddToProcedureIndexRunning(apActor, 1);
										if (((pRef_2 && (v8 = pRef_2->GetObjectReference(), v8 == pXMarkerHeading)) ||
												(pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC)) &&
											!DidPathingFail(apActor)) {
											float ftargetangle_2 = 0.0f;
											if (pRef_2) {
												ftargetangle_2 = (&pRef_2->GetAngleOnReference())->z;
											} else {
												ftargetangle_2 = apActor->GetStartingAngle().z;
											}
											apActor->SetHeading(ftargetangle_2);
										} else {
											if (pPack->GetLocationRadius(apActor)) {
												NiPoint3 nearcoord;
												float a5a = static_cast<float>(pPack->GetLocationRadius(apActor));
												NiPoint3 afRadius1 = pPack->GetLocationCoord(apActor);
												if (HighProcess::ComputeRandomPointOnPath(apActor, afRadius1, a5a,
														nearcoord)) {
													apActor->SetPosition(nearcoord);
												}
											}
										}
									}
								} else {
									bevalflag = 0;
								}
							}
							continue;
						}
						case PROCEDURE_WANDER: {
							ProcessWanderOneHour(apActor, fTimeDelta);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_ACTIVATE: {
							bevalflag = ProcessActivateOneHour(apActor, fTimeDelta);
							continue;
						}
						case PROCEDURE_ACQUIRE: {
							if (apActor->GetSitSleepState()) {
								ProcessGetUpOneHour(apActor);
							}
							ProcessAcquireOneHour(apActor, fTimeDelta);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_SLEEP: {
							bevalflag = ProcessSleepOneHour(apActor, fTimeDelta);
							continue;
						}
						case PROCEDURE_EAT: {
							bevalflag = ProcessEatOneHour(apActor, fTimeDelta);
							continue;
						}
						case PROCEDURE_FOLLOW: {
							if (apActor->GetSitSleepState()) {
								ProcessGetUpOneHour(apActor);
							}
							bevalflag = ProcessFollowOneHour(apActor, fTimeDelta, 1);
							continue;
						}
						case PROCEDURE_ESCORT: {
							if (apActor->GetSitSleepState()) {
								ProcessGetUpOneHour(apActor);
							}
							bevalflag = ProcessEscortOneHour(apActor, fTimeDelta);
							continue;
						}
						case PROCEDURE_ALARM: {
							if (apActor->GetSitSleepState()) {
								ProcessGetUpOneHour(apActor);
							}
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_COMBAT: {
							if (apActor->GetSitSleepState()) {
								ProcessGetUpOneHour(apActor);
							}
							bevalflag = ProcessCombatOneHour(apActor, fTimeDelta);
							continue;
						}
						case PROCEDURE_DIALOGUE: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_WAIT: {
							if (!pTarget) {
								SetTargetForPackage(apActor);
							}
							if (!pTarget) {
								goto LABEL_105;
							}
							if (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
								int32_t NumberOfFollowers = pPlayer->GetNumberOfFollowers();
								if (NumberOfFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
									goto LABEL_105;
								}
								bevalflag = 0;
							} else {
								int32_t ipadding;
								if (pTarget->GetDelete() || pTarget->GetDisabled()) {
									if (pTarget->GetDelete()) {
										pPack->SetNeverRun(pTarget, 1);
									}
									AddToProcedureIndexRunning(apActor, 1);
									return;
								}
								if (pTarget->IsDead(1)) {
									pPack->SetNeverRun(pTarget, 1);
									return;
								}
								ipadding = pPack->GetLocationRadius(apActor) == 0 ? 400 : 0;
								if (pPack->IsActorAtRefTarget(apActor, ipadding)) {
								LABEL_105:;
									AddToProcedureIndexRunning(apActor, 1);
								} else {
									bevalflag = 0;
								}
							}
							break;
						}
						case PROCEDURE_TRAVEL_TARGET: {
							if (!pTarget) {
								SetTargetForPackage(apActor);
							}
							if (!pTarget) {
								AddToProcedureIndexRunning(apActor, 1);
								if (!apActor->IsPathingComplete()) {
									EndMoveMessage(apActor);
								}
							}
							ProcessActivateOneHour(apActor, fTimeDelta);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_PURSUE: {
							LowProcess::ProcessPursue(apActor, 0, 1, 257);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_CREATE_FOLLOW: {
							ProcessCreateFollow(apActor);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_GET_UP: {
							ProcessGetUpOneHour(apActor);
							AddToProcedureIndexRunning(apActor, 1);
							continue;
						}
						case PROCEDURE_DO_NOTHING: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_NOTIFY: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_ACCOMPANY: {
							ProcessAccompanyOneHour(apActor, fTimeDelta);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_USE_ITEM_AT: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_FEED: {
							ProcessSandman(apActor);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_AMBUSH_WAIT: {
							ProcessAmbushWait(apActor);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_SURFACE: {
							ProcessSurfaceOneHour(apActor);
							continue;
						}
						case PROCEDURE_CHOOSE_CAST: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_REMOVE_WORN_ITEMS: {
							ProcessRemoveWorn(apActor, 1, 0);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_AVOID_RADIATION: {
							ProcessAvoidArea(apActor);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_PATROL: {
							if (apActor->GetSitSleepState()) {
								ProcessGetUpOneHour(apActor);
							}
							ProcessPatrolOneHour(apActor, fTimeDelta);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_GUARD: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_USE_WEAPON: {
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_CANNIBAL_FEED: {
							ProcessCannibal(apActor);
							bevalflag = 0;
							continue;
						}
						case PROCEDURE_DONE: {
							bevalflag = 0;
							continue;
						}
						default: {
							bevalflag = 0;
							continue;
						}
						}
					}
				}
			}
		}
		if (apActor->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
			TESPackage* pPack_2 = GetPackageThatIsRunning();
			if (pPack_2) {
				uint32_t v10 = pPack_2->ePROCEDURE_TYPE;
				if (Procedure_Action[v10][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
					if (!GetRunOncePackage()) {
						fScriptPackageEndTime = 0.0f;
					}
					if (pPack_2->ePROCEDURE_TYPE) {
						if (pPack_2->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE ||
							(pPack_2->GetPackType() == PACKAGE_TYPE_EAT ||
								pPack_2->GetPackType() == PACKAGE_TYPE_SLEEP)) {
							goto LABEL_144;
						}
					} else {
						if (!pPack_2->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
						LABEL_144:;
							AddToProcedureIndexRunning(apActor, -1);
							return;
						}
					}
					EndMoveMessage(apActor);
					if (bContinuingPackageforPC) {
						AddToProcedureIndexRunning(apActor, -1);
						SetCurrentActionComplete(0);
						return;
					}
					if (pPack_2) {
						if (pPack_2->GetOncePerDay()) {
							char v11 = appCalendar.GetDay();
							apActor->SetRunOnceExtraPackage(pPack_2, v11);
						}
					}
					if (!pPack_2->GetPackageSchedule()->PSData.iDuration) {
						SetTarget(0);
						if (!GetRunOncePackage() || ContinuingPackageforPC()) {
							TESPackage* v12 = GetCurrentPackage();
							if (v12->GetIsCreated()) {
								TESPackage* v13 = GetCurrentPackage();
								if (v13->IsInterruptPackage()) {
									LoadPackageFromExtraData(apActor);
								} else {
									ClearCurrentPackage();
								}
								if (!apActor->IsPathingComplete()) {
									EndMoveMessage(apActor);
								}
							}
						} else {
							ClearRunOncePackage();
						}
						if (GetCurrentPackage()) {
							TESPackage* v15;
							TESPackage* v16;
							TESPackage* v14 = GetCurrentPackage();
							if (v14->GetOncePerDay() ||
								((v15 = GetCurrentPackage(), v15->GetMustReachLocation()) ||
									(v16 = GetCurrentPackage(), v16->GetMustComplete()))) {
								fPackageEvalTimer = 0.0f;
							}
						}
						ObjectstoAcquire* apThis = pAcquireObject;
						if (apThis) {
							apThis->Delete(1u);
						}
						pAcquireObject = 0;
						pItemBeingUsed = 0;
						while (!ObjectList.IsEmpty()) {
							ObjectstoAcquire* pobj = ObjectList.m_item;
							if (pobj) {
								pobj->Delete(1u);
							}
							ObjectList.Remove(pobj);
						}
						fDetectListTimer = 0.0f;
						pGenericLocation = 0;
						GenericLocationsList.RemoveAll();
					}
				}
			}
		}
		bool bskippedpathing = 0;
		BaseProcess* CurrentAIProcess = apRef->pCurrentProcess;
		CurrentAIProcess->ComputeLastTimeProcessed();
	}
}

// The exe's cleanup-only C++ EH frame is left out. The voice handle the exe tests is a temporary: its destructor
// (0x483710) is empty.
void HighProcess::Update(MobileObject* apRef) {
	NiPoint3 result;
	bool bsearch;
	TESObjectREFR* ptarget;
	Actor* ptargetActor;
	bool bfleeflag;
	TESPackage* pPack = 0;
	if (!apRef) {
		return;
	}
	if (!apRef->IsTalkingActivator() && (!apRef->Get3D() || (apRef->GetDelete() || apRef->GetDisabled()))) {
		return;
	}
	TESObjectCELL* pparent = apRef->GetParentCell();
	if (!pparent) {
		return;
	}
	if (!pparent->GetAttached()) {
		return;
	}
	if (apRef->IsActor() && eFadeState == BaseProcess::FADE_TELEPORT_OUT) {
		return;
	}
	if (eFadeState == BaseProcess::FADE_TELEPORT_IN) {
		return;
	}
	if (!apRef->IsTalkingActivator()) {
		if (!GetAnimation()) {
			return;
		}
		Animation* v2 = GetAnimation();
		if (v2->GetAnimsLoading()) {
			return;
		}
	}
	if (!apRef->IsTalkingActivator()) {
		apRef->UpdateTopicInfoSayOnce();
	}
	Actor* apActor = 0;
	if (apRef->IsActor()) {
		apActor = static_cast<Actor*>(apRef);
	}
	fGreetingTimer = fGreetingTimer - appTimer.fDelta;
	bool bfixactorpos = GetSkippedLastUpdate();
	SetSkippedLastUpdate(0);
	UpdateHeadTrackTargets();
	if (!apActor) {
		if (apRef->IsTalkingActivator()) {
			TESPackage* pPack_2 = GetPackageThatIsRunning();
			if (pPack_2) {
				uint32_t Count = pPack_2->ePROCEDURE_TYPE;
				if (Procedure_Action[Count][GetProcedureIndexRunning()] == PROCEDURE_DIALOGUE) {
					ProcessDialogue(apRef);
				}
			}
		}
	LABEL_477:;
		bool bskippedpathing = 0;
		BaseProcess* CurrentAIProcess = apRef->pCurrentProcess;
		CurrentAIProcess->ComputeLastTimeProcessed();
		return;
	}
	fCheckToTalkTimer = fCheckToTalkTimer - appTimer.fDelta;
	if (apActor->IsDying()) {
		TESPackage* ppack = GetRunOncePackage();
		if (GetGreetingTimer() != 3.4028234663852886e+38) {
			if (!ppack || ppack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
				if (GetGreetingTimer() <= 0.0) {
					apActor->DoDeathStuff();
				}
				return;
			}
			uint32_t v3 = ppack->ePROCEDURE_TYPE;
			if (Procedure_Action[v3][GetProcedureIndexRunning()] != PROCEDURE_DONE) {
				ProcessDialogue(apActor);
				return;
			}
		}
		apActor->DoDeathStuff();
		return;
	}
	bool btorsoUnderWater = apActor->IsPointDeepUnderWater(*apActor->QLocationOnReference(), pparent, 0.5f);
	if (bfixactorpos) {
		apActor->MoveToSafePoint(0);
	}
	if (this->GetBoneLOD()) {
		if (apActor->Get3D()) {
			EvaluateBoneLod(apActor);
		}
	}
	if (bDetectLifeDetected || DetectLifeEffect::GetPlayerDetectingLife()) {
		bDetectLifeDetected = DetectLifeEffect::CheckActorDetected(apActor, bDetectLifeDetected);
	}
	if (apActor->IsUnConscious()) {
		PickPackageIdles(apActor, 0);
		RandomlyPlaySpecialIdles(apActor);
		apActor->PickAnimations(1.0f, 1.0f);
		BaseProcess* v53 = apRef->pCurrentProcess;
		v53->ComputeLastTimeProcessed();
		return;
	}
	if (pSaveLoadGameOLD->GetLoading()) {
		goto LABEL_480;
	}
	if (!apActor->IsInCombat()) {
		goto LABEL_480;
	}
	if (apActor->GetCombatController()) {
		goto LABEL_480;
	}
	ptarget = apActor->GetCurrentPackageTarget();
	bfleeflag = apActor->IsFleeing(0);
	ptargetActor = 0;
	if (ptarget) {
		if (ptarget->IsActor()) {
			ptargetActor = static_cast<Actor*>(ptarget);
		}
	}
	apActor->EndInterruptPackage(0);
	if (!EnterCombat(apActor, ptargetActor, 1, bfleeflag, 0, bfleeflag, 0, 0, 0, 0, 0, 1, 0)) {
	LABEL_480:;
		if (bSayGoodByePlayer && !Interface::IsMenuIDVisible(Interface::eDialogMenu, 0)) {
			SayGoodBye(apActor);
			bSayGoodByePlayer = 0;
			return;
		}
		if (!bEvaluateDetection || !EvaluateDetection(apActor)) {
			TESPackage* ppack_2;
			TESPackage* v106;
			if (!(fDetectionModifierTimer > 0.0)) {
				fDetectionModifer = 0.0f;
			} else {
				fDetectionModifierTimer = fDetectionModifierTimer - appTimer.fDelta;
			}
			int32_t idetection = 0;
			if (this->GetPathLookAtTarget()) {
				NiPoint3 v118 = this->GetPathLookAtTarget()->GetLocationOnReference();
				apActor->SetLookAtTarget(v118);
			}
			bool bincombat = apActor->IsInCombat();
			if (!bincombat) {
				SetIsAggressor(0);
			}
			pPack = GetPackageThatIsRunning();
			TESPackage* pcurpack = GetCurrentPackage();
			if ((!pPack || pPack->GetPackType() != PACKAGE_TYPE_COMBAT) &&
				(!pcurpack || pcurpack->GetPackType() != PACKAGE_TYPE_COMBAT) && apActor->IsInCombat()) {
				apActor->ClearInCombat(1);
			}
			if (GetNeedTalkPlayer() && (!pPack || pPack->GetPackType() != PACKAGE_TYPE_GET_UP)) {
				SetNeedTalkPlayer(0);
			}
			bool bDoGreetIdleChatter = false;
			if (!pPlayer->IsImportantConversationRunning() && !apActor->IsInCombat() && !apActor->IsFleeing(0) &&
				!apActor->IsUnConscious() && !apActor->IsKnockedOut()) {
				if (!GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE).IsValid() &&
					(!GetGreetingFlag() && !pPlayer->GetPlayerGreetFlag())) {
					bDoGreetIdleChatter = true;
				}
			}
			if (bDoGreetIdleChatter) {
				double DistanceFromReference;
				bool blinesight;
				idetection = apActor->GetDetectionLevelAgainstActor(0, pPlayer, blinesight, bincombat, 0, 0, 0);
				if (idetection <= 0 ||
					(apActor->IsSleeping() ||
						(pPlayer->IsSneaking() ||
							(pPlayer->IsTresPassing() ||
								((apActor->GetCurrentTarget() == pPlayer && !apActor->IsRunningRunOnce()) ||
									(!CanSetActionHeadTrackTarget() ||
										(DistanceFromReference = pPlayer->GetDistanceFromReference(apActor, 0, 0),
											!(fAIMinGreetingDistance.Float() >= DistanceFromReference)))))))) {
					if (!(fIdleChatterTimer <= 0.0) || apActor->IsinDialogue()) {
						fIdleChatterTimer = fIdleChatterTimer - appTimer.fDelta;
					} else {
						if (!pPack ||
							(pPack->GetPackType() != PACKAGE_TYPE_DIALOGUE &&
								pPack->GetFOEnableIdleChatterBehavior())) {
							TESTopic* ptopic = TESTopic::GetTopic(DT_CONVERSATION, 3);
							if (ptopic) {
								ProcessGreet(apActor, ptopic, 0, 0, 1, 0);
							}
							float _Xx = fIdleChatterCommentTimerMax.Float();
							float* v4 = &fIdleChatterCommentTimer.Float();
							fIdleChatterTimer = CombatFormulas::GetRandomBetween(*v4, _Xx);
						}
					}
				} else {
					bsearch = 0;
					if (!pPlayer->IsPlayerCharacterInCombat(&bsearch) &&
						!apActor->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_HELLOS_TO_PLAYER) &&
						!apActor->IsContinuingPackagePCNear() && !bincombat && !apActor->IsAlarmed() &&
						(!apActor->IsinDialogue() || apActor->IsMoving()) && GetTarget() != pPlayer &&
						GetGreetingTimer() <= 0.0) {
						TESTopic* pTopic = TESTopic::GetTopic(DT_CONVERSATION, 0);
						apActor->StartGreetingPlayer(pTopic);
					}
				}
			} else {
				if (GetGreetingFlag()) {
					bool bstopped = GetMovementStoped();
					bool bque = GetLipQuequed();
					ProcessGreet(apActor, 0, 0, bstopped, bque, 0);
				}
			}
			if (!pPlayer->IsImportantConversationRunning() && !GetGreetingFlag() && !pPlayer->GetPlayerGreetFlag()) {
				CheckIfThereSomeoneTalkWith(apActor);
			}
			bool bNewPackage = 0;
			bool bMustChange = 0;
			pPack = GetPackageThatIsRunning();
			if (fScriptPackageEndTime != 0.0) {
				float fhour = appCalendar.GetHour();
				double PackageStartTime = this->CurrentPackage.GetPackageStartTime();
				float v49 = fhour - PackageStartTime;
				float ftime = std::fabs(v49) * 100.0;
				if (fScriptPackageEndTime <= static_cast<double>(ftime)) {
					fPackageEvalTimer = 0.0f;
					bMustChange = 1;
					AddToProcedureIndexRunning(apActor, 3);
					if (pPack) {
						if (pPack->GetOncePerDay()) {
							char Day = appCalendar.GetDay();
							apActor->SetRunOnceExtraPackage(pPack, Day);
						}
					}
				}
			}
			if (GetCurrentPackage()) {
				TESPackage* v7 = GetCurrentPackage();
				bMustChange = v7->IsNeverToRun();
			}
			if (apActor != pPlayer && !GetRunOncePackage()) {
				TESPackage* v8;
				if (!GetCurrentPackage() || (v8 = GetCurrentPackage(), !v8->IsInterruptPackage())) {
					bNewPackage = CheckforNewPackage(apActor, bMustChange);
				}
			}
			pPack = GetPackageThatIsRunning();
			if (apRef != pPlayer && (bNewPackage || (!pPack && !apActor->IsPathingComplete()))) {
				ClearCurrentDataforProcess(apActor);
				pPack = GetPackageThatIsRunning();
				if (pPack) {
					PackageTarget* ptarg = pPack->GetPackageTarget();
					if (ptarg) {
						if (ptarg->GetTargType()) {
							if (ptarg->GetTargType() != PTT_LINKEDREF) {
								iNumberItemsActivate = pPack->GetInitialTargetCount();
							}
						}
					}
				}
			}
			if (!GetRunOncePackage()) {
				SIT_SLEEP_STATE v48 = apActor->GetSitSleepState();
				if (v48 == SIT_SLEEP_WANT_TO_STAND || v48 == SIT_SLEEP_WANT_TO_WAKE) {
					apActor->InitiateGetUpPackage();
				}
			}
			pPack = GetPackageThatIsRunning();
			PickPackageIdles(apActor, 0);
			RandomlyPlaySpecialIdles(apActor);
			if (bNewPackage) {
				apActor->AlertAccompanyToPackageChange();
				SetLocationForPackage(apActor, 0);
			}
			if (!GetRunOncePackage()) {
				if (pPack) {
					if (!pPack->IsInterruptPackage()) {
						if (GetCurrentPackage()) {
							TESPackage* v9 = GetCurrentPackage();
							uint32_t v10 = v9->ePROCEDURE_TYPE;
							if (Procedure_Action[v10][GetProcedureIndexRunning()]) {
								if (ContinuingPackageforPC() ||
									(pPack->GetPCContinue() &&
										(pPack->GetServicesEnabled() &&
											(pparent && pparent->IsActorCellOwner(apActor))))) {
									CreateFollownoEscort(apActor, 0);
									pPack = GetPackageThatIsRunning();
								}
							}
						}
					}
				}
			}
			if (fAwarePlayerTimer >= 0.0 ||
				(GetRunOncePackage() &&
					(ppack_2 = GetRunOncePackage(), ppack_2->GetPackType() == PACKAGE_TYPE_FOLLOW))) {
				TESPackage* ppack_3;
				if (fAwarePlayerTimer <= 0.0 &&
					(GetRunOncePackage() &&
						(ppack_3 = GetRunOncePackage(), ppack_3->GetPackType() == PACKAGE_TYPE_FOLLOW))) {
					fAwarePlayerTimer = 0.0f;
					EndMoveMessage(apActor);
					ClearRunOncePackage();
					ClearActionHeadTrackTarget(1);
					SetTarget(0);
					pPack = GetPackageThatIsRunning();
				} else {
					fAwarePlayerTimer = fAwarePlayerTimer - appTimer.fDelta;
				}
			}
			ResetTarget();
			TESPackage* peditorpack = (&apActor->m_Extra)->GetPackageExtra();
			TESPackage* pcurrent = pPack;
			if (IsRunningRunOnce()) {
				v106 = GetCurrentPackage();
			}
			if (!apActor->IsInCombat() &&
				((v106 && v106->GetDrawWeaponFlag()) ||
					((peditorpack && peditorpack->GetDrawWeaponFlag()) || (GetAlert() || bWeaponAlertDrawn)))) {
				if (GetSitSleepState() == SIT_SLEEP_NORMAL && GetAnimAction() == ANIM_ACTION_NONE) {
					if (v106 && (v106->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE && apActor == pTarget)) {
						if (peditorpack && peditorpack->GetDrawWeaponFlag()) {
							if (!GetCurrentWeapon()) {
								ItemChange* pweap = apActor->GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
								if (pweap) {
									BaseExtraList* pextra = 0;
									if (pweap->pExtraObjectList) {
										pextra = (BaseExtraList*)pweap->pExtraObjectList->m_item;
									}
									ExtraDataList* pextra_3 = static_cast<ExtraDataList*>(pextra);
									TESBoundObject* FormFlags = pweap->pContainerObj;
									apActor->EquipObject(FormFlags, 1, pextra_3, 1, 0, 1);
								}
							}
							apActor->SetWantWeaponDrawn(1);
							SetWantWeaponDrawn(1);
							goto LABEL_204;
						}
					LABEL_203:;
						apActor->SetWantWeaponDrawn(0);
						SetWantWeaponDrawn(0);
						goto LABEL_204;
					}
					if (!GetWeaponDrawn() && !GetWantWeaponDrawn()) {
						if (!GetCurrentWeapon()) {
							ItemChange* pweap_2 = apActor->GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
							if (pweap_2) {
								BaseExtraList* pextra_2 = 0;
								if (pweap_2->pExtraObjectList) {
									pextra_2 = (BaseExtraList*)pweap_2->pExtraObjectList->m_item;
								}
								ExtraDataList* pextra_4 = static_cast<ExtraDataList*>(pextra_2);
								TESBoundObject* v33 = pweap_2->pContainerObj;
								apActor->EquipObject(v33, 1, pextra_4, 1, 0, 1);
							}
						}
						apActor->SetWantWeaponDrawn(1);
						SetWantWeaponDrawn(1);
					}
				}
			} else {
				if (apActor != pPlayer && (!apActor->IsInCombat() && (GetWeaponDrawn() && GetWantWeaponDrawn()))) {
					goto LABEL_203;
				}
			}
		LABEL_204:;
			if (apActor->GetSitSleepState()) {
				if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING &&
					apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
					uint32_t v13;
					if (pPack &&
						(v13 = pPack->ePROCEDURE_TYPE,
							Procedure_Action[v13][GetProcedureIndexRunning()] == PROCEDURE_GET_UP)) {
						AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_GET_UP);
					} else {
						if (iAnimActionSuccess) {
							AlignToFurnitureAndAddPostAnimSitSleep(apActor);
						} else {
							if (pCurrentFurniture) {
								ProcessGetUpOneHour(apActor);
							} else {
								SetSitSleepState(apActor, SIT_SLEEP_NORMAL, 0, 127);
							}
						}
					}
				}
			}
			if (pPack && pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE) {
				if (pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE && GetProcedureIndexRunning() >= 3) {
					DialoguePackage* pdialogpack = static_cast<DialoguePackage*>(pPack);
					Actor* pdialogueact = static_cast<Actor*>(pTarget);
					if (pdialogueact) {
						if (!pdialogpack->pTalkingActivator) {
							PROCESS_TYPE CurrentProcessType = pdialogueact->GetCurrentProcessType();
							if (CurrentProcessType != eLevel) {
								apActor->EndDialogue();
								pPack = GetPackageThatIsRunning();
								ResetTarget();
							}
						}
					}
				}
				Actor* pactarget = 0;
				DetectionState* pdetect = 0;
				DETECTION_LEVEL elevel = DETECTION_SEEN;
				if (pPack->pPackLoc) {
					if (pPack->pPackLoc->IsLocationGeneric()) {
						uint32_t v15 = pPack->ePROCEDURE_TYPE;
						if (Procedure_Action[v15][GetProcedureIndexRunning()] != PROCEDURE_DONE) {
							if (pGenericLocation) {
								if (pGenericLocation->IsFurniture() &&
									apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
									SetProcedureIndexRunning(0);
								}
							} else {
								SetLocationForPackage(apActor, 0);
								SetProcedureIndexRunning(0);
							}
						}
					}
				}
				if (!apActor->IsInCombat() && apActor->IsPathingComplete()) {
					if (pPack) {
						if (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) &&
							pPack->GetPackType() != PACKAGE_TYPE_SANDBOX &&
							pPack->GetPackType() != PACKAGE_TYPE_GUARD &&
							pPack->GetPackType() != PACKAGE_TYPE_USE_WEAPON && !GetGreetingFlag()) {
							TESBoundObject* ObjectReference;
							TESObjectREFR* pRef = 0;
							if (pPack->pPackLoc) {
								pRef = pPack->pPackLoc->GetLocReference();
							}
							if (pGenericLocation) {
								pRef = pGenericLocation;
							}
							if ((pRef &&
									(ObjectReference = pRef->GetObjectReference(),
										ObjectReference == pXMarkerHeading)) ||
								(pRef && (pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC))) {
								if (!DidPathingFail(apActor) && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
									const NiPoint3* AngleOnReference = &pRef->GetAngleOnReference();
									float v47 = AngleOnReference->z - (&apActor->GetAngleOnReference())->z;
									if (std::fabs(v47) > 0.01745329238474369) {
										const NiPoint3* v18 = &pRef->GetAngleOnReference();
										apActor->RequestRotate(v18->z, 1);
									}
								}
							}
						}
					}
				}
				uint32_t v19 = pPack->ePROCEDURE_TYPE;
				switch (Procedure_Action[v19][GetProcedureIndexRunning()]) {
				case PROCEDURE_TRAVEL: {
					ProcessTravel(apActor, bNewPackage, 1, -1, 0);
					goto LABEL_477;
				}
				case PROCEDURE_WANDER: {
					ProcessWander(apActor, 0, -1.0f);
					goto LABEL_477;
				}
				case PROCEDURE_ACTIVATE: {
					ProcessActivate(apActor, 1);
					goto LABEL_477;
				}
				case PROCEDURE_ACQUIRE: {
					ProcessAcquire(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_SLEEP: {
					ProcessSleep(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_EAT: {
					ProcessEat(apActor, 0);
					goto LABEL_477;
				}
				case PROCEDURE_FOLLOW: {
					ProcessFollow(apActor, 1, 257, 0);
					goto LABEL_477;
				}
				case PROCEDURE_ESCORT: {
					ProcessEscort(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_ALARM: {
					ProcessAlarm(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_FLEE: {
					ProcessFlee(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_DIALOGUE: {
					ProcessDialogue(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_WAIT: {
					if (apActor->IsInCombat()) {
						goto LABEL_344;
					}
					if (!pTarget) {
						SetTargetForPackage(apActor);
					}
					if (!pTarget) {
						goto LABEL_285;
					}
					if (pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
						bool bcontinueMoving;
						Actor* v76;
						DialoguePackage* pdial = static_cast<DialoguePackage*>(pPack);
						// The exe keeps the dialogue's pActor (the one-field getter 0x9EE040) as an actor.
						Actor* pact = static_cast<Actor*>(pdial->pActor);
						if (pTarget == pPlayer) {
							goto LABEL_288;
						}
						if (pact == apActor) {
							bcontinueMoving = pdial->GetContinueMoving();
						} else {
							bcontinueMoving = pdial->GetTargetContinueMoving();
						}
						if (bcontinueMoving) {
							AddToProcedureIndexRunning(apActor, 1);
							return;
						}
						if (!pact ||
							(!pact->pCurrentProcess || (pact->IsActor() && (v76 = pact, pact->DidPathingFail())))) {
						LABEL_288:;
							apActor->EndInterruptPackage(0);
							return;
						}
						BaseProcess* v45 = pact->pCurrentProcess;
						if (v45->GetProcedureIndexRunning() == 2) {
							BaseProcess* v44 = pact->pCurrentProcess;
							v44->AddToProcedureIndexRunning(pact, 1);
						}
					} else {
						if (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
							const TESFollowPackageData* pfollowData = pPack->GetFollowPackageData();
							if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_AMBUSH_FOLLOW &&
								pPack->GetPackageSecondLocation()) {
								if (pPack->IsTargetAtSecondLocation(apActor, pfollowData->fTriggerRadius)) {
									AddToProcedureIndexRunning(apActor, 1);
								}
							} else {
								int32_t NumberOfFollowers = pPlayer->GetNumberOfFollowers();
								if (NumberOfFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
									AddToProcedureIndexRunning(apActor, 1);
								}
							}
						} else {
							if (pTarget->GetDelete() || pTarget->GetDisabled()) {
								if (pTarget->GetDelete()) {
									pPack->SetNeverRun(pTarget, 1);
								}
							LABEL_285:;
								AddToProcedureIndexRunning(apActor, 1);
							} else {
								if (pTarget->IsDead(1)) {
									pPack->SetNeverRun(pTarget, 1);
									return;
								}
								if (pPack->GetPackType() == PACKAGE_TYPE_DIALOGUE) {
								LABEL_438:;
									AddToProcedureIndexRunning(apActor, -1);
									return;
								}
								if (pPack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE && !pPack->GetWaiting()) {
									TESPackage* CurrentPackage = apActor->GetCurrentPackage();
									if (CurrentPackage->GetPackType() == PACKAGE_TYPE_FOLLOW) {
										float fwaitdistance = static_cast<float>(pPack->GetPackageTarget()->iValue);
										double v22 = apActor->GetDistanceFromReference(pTarget, 0, 0);
										if (fwaitdistance < v22) {
											AddToProcedureIndexRunning(apActor, 1);
										}
									} else {
										if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_NPC) {
											float fcurrenttime = appCalendar.GetHour();
											float* v23 = &fAIConversationExploreTime.Float();
											if (fcurrenttime >= *v23 + fDetectListTimer) {
												fDetectListTimer = fcurrenttime;
												AddToProcedureIndexRunning(apActor, -1);
											}
										} else {
											TESPackage* v24 = apActor->GetCurrentPackage();
											if (v24->GetPackType() == PACKAGE_TYPE_ESCORT) {
												Actor* pwait = 0;
												if (pTarget) {
													if (pTarget->IsActor()) {
														pwait = static_cast<Actor*>(pTarget);
														if (GetLineSightonActor(apActor, pwait, 0, 1)) {
															SetActionHeadTrackTarget(pTarget);
														}
													}
												}
												if (pwait) {
													if (pwait == pPlayer) {
														TESWorldSpace* _Xxa = pPack->GetLocationWorld(apActor);
														TESObjectCELL* afTargetAngle = pPack->GetLocationCell(apActor);
														NiPoint3* LocationCoord =
															::new (&result) NiPoint3(pPack->GetLocationCoord(apActor));
														FNVD_CONSTRUCTED_LOCAL(PathingLocation, goalLoc,
															&PathingLocation::Destruct, LocationCoord, afTargetAngle,
															_Xxa);
														if (!ShouldWaitForEscortTarget(apActor, pwait, goalLoc, 0)) {
															apActor->ClearLookAtTarget();
															ClearAllHeadTrackTargets();
															AddToProcedureIndexRunning(apActor, 1);
														}
													} else {
														TESPackage* pwaitpack = pwait->GetCurrentPackage();
														if (pwaitpack &&
															pwaitpack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
															BaseProcess* v43 = pwait->pCurrentProcess;
															v43->SetProcedureIndexRunning(1);
															AddToProcedureIndexRunning(apActor, 1);
														} else {
															apActor->ClearLookAtTarget();
															ClearAllHeadTrackTargets();
															AddToProcedureIndexRunning(apActor, -2);
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
					apActor->StopMoving();
					goto LABEL_477;
				}
				case PROCEDURE_TRAVEL_TARGET: {
					if (apActor->IsPlayingLowerBodySpecialIdle()) {
						Animation* pactorAnim = apActor->GetAnimation();
						if (pactorAnim) {
							pactorAnim->SpecialIdleFree(1, 0);
						}
					}
					ProcessActivate(apActor, 0);
					goto LABEL_477;
				}
				case PROCEDURE_PURSUE: {
					pactarget = static_cast<Actor*>(pTarget);
					pdetect = GetDetectionState(pactarget, 0);
					if (pactarget) {
						if (pdetect) {
							pdetect->eDetectionLevel = elevel;
						} else {
							InsertIntoDetectionArray(pactarget, elevel);
						}
					}
					ProcessPursue(apActor, 0, 1, 257);
					goto LABEL_477;
				}
				case PROCEDURE_GREET: {
					ProcessGreet(apActor, 0, 0, 0, 1, 0);
					goto LABEL_477;
				}
				case PROCEDURE_CREATE_FOLLOW: {
					ProcessCreateFollow(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_OBSERVE_COMBAT: {
					ProcessObserveCombat(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_WARN: {
					ProcessWarning(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_GET_UP: {
					AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_GET_UP);
					goto LABEL_477;
				}
				case PROCEDURE_NOTIFY: {
					ProcessNotify(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_ACCOMPANY: {
					ProcessAccompany(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_USE_ITEM_AT: {
					ProcessUseItemAt(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_FEED: {
					ProcessSandman(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_AMBUSH_WAIT: {
					ProcessAmbushWait(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_SURFACE: {
					ProcessSurface(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_WAIT_FOR_SPELL: {
					if (!GetCurrentSpell()) {
						AddToProcedureIndexRunning(apActor, 1);
					}
					goto LABEL_477;
				}
				case PROCEDURE_CHOOSE_CAST: {
					ProcessChooseSpell(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_FLEE_NON_COMBAT: {
					ProcessFleeNonCombat(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_REMOVE_WORN_ITEMS: {
					ProcessRemoveWorn(apActor, 1, 0);
					goto LABEL_477;
				}
				case PROCEDURE_SEARCH: {
					ProcessSearch(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_CLEAR_MOUNT_POSITION: {
					ProcessClearMountPosition(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_SUMMON_CREATURE_DEFEND: {
					ProcessSummonCreatureDefend(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_AVOID_RADIATION: {
					ProcessAvoidArea(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_UNEQUIP_ARMOR: {
					goto LABEL_477;
				}
				case PROCEDURE_TAKE_BACK_ITEM: {
					ProcessTakeBackItem(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_SANDBOX: {
					ProcessSandBox(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_USE_IDLE_MARKER: {
					ProcessUseIdleMarker(apActor, pTarget);
					goto LABEL_477;
				}
				case PROCEDURE_PATROL: {
					ProcessPatrol(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_EXPLOSION_REACTION: {
					LowProcess::ProcessAvoidArea(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_GRENADE_MINE_PICKUP_THROW: {
				LABEL_344:;
					AddToProcedureIndexRunning(apActor, 1);
					goto LABEL_477;
				}
				case PROCEDURE_GUARD: {
					ProcessGuard(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_ALERT_SEARCH: {
					ProcessAlertSearch(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_DIALOGUE_ACTIVATE: {
					ProcessDialogueActivate(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_USE_WEAPON: {
					ProcessUseWeapon(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_MOVEMENT_BLOCKED: {
					ProcessMovementBlocked(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_CANNIBAL_FEED: {
					ProcessCannibal(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_BACK_UP: {
					ProcessBackUp(apActor);
					goto LABEL_477;
				}
				case PROCEDURE_DONE: {
					if (pPack->GetDrawWeaponFlag()) {
						apActor->SetAlert(0);
					}
					if (!GetRunOncePackage()) {
						fScriptPackageEndTime = 0.0f;
					}
					if (!IsPackageDoneOnce() && !pPack->GetIsCreated() && pPack->IsInterruptPackage()) {
						ExtraDataList* afTargetAnglea = &apActor->m_Extra;
						Script::SetActionFlag(pPack, afTargetAnglea, 1024u);
						PackageDone(apActor, pPack);
						SetPackageDoneOnce(1);
					}
					switch (pPack->ePROCEDURE_TYPE) {
					case PACKAGE_PROCEDURE_EXPLORE_WANDER:
					case PACKAGE_PROCEDURE_SLEEP:
					case PACKAGE_PROCEDURE_EAT:
					case PACKAGE_PROCEDURE_GUARD:
					case PACKAGE_PROCEDURE_FOLLOW_WITHOUT_ESCORT: {
						goto LABEL_438;
					}
					default: {
						if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_USEWEAPON) {
							const TESUseWeaponPackageData* puseWeaponData = pPack->GetUseWeaponPackageData();
							if (puseWeaponData) {
								int32_t sBurstsFired;
								if (puseWeaponData->bRepeatFire ||
									(sBurstsFired = this->sBurstsFired, sBurstsFired < puseWeaponData->sBurstCount)) {
									if (fPackageIdleTimer <= 0.0) {
										AddToProcedureIndexRunning(apActor, -1);
									}
								}
							}
						}
						if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_PATROL) {
							const TESPatrolPackageData* ppatrolData = pPack->GetPatrolPackageData();
							bool v42 = ppatrolData && ppatrolData->bRepeatable;
							bool bnewPackage = 0;
							PackageSchedule* psched = pPack->GetPackageSchedule();
							if ((!psched || static_cast<double>(psched->PSData.iDuration) <= 0.0) && !v42) {
								SetCurrentActionComplete(1);
								ClearCurrentDataforProcess(apActor);
								ClearCurrentPackage();
								bnewPackage = CheckforNewPackage(apActor, 1);
							}
							if (v42 && !bnewPackage) {
								AddToProcedureIndexRunning(apActor, -1);
							}
							return;
						}
						if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_SANDBOX) {
							SandBoxActorPackageData* pdata =
								static_cast<SandBoxActorPackageData*>(GetInstanceDataThatIsRunning());
							if (pdata) {
								pdata->HandleSubprocessFinished();
								SetProcedureIndexRunning(0);
							}
							return;
						}
						if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_USEITEMAT &&
							(pPack->GetPackageTarget() && !pPack->GetInitialTargetCount())) {
							AddToProcedureIndexRunning(apActor, -3);
							return;
						}
						if (pPack->ePROCEDURE_TYPE) {
							if (pPack->GetPackType() == PACKAGE_TYPE_EAT ||
								pPack->GetPackType() == PACKAGE_TYPE_SLEEP) {
								AddToProcedureIndexRunning(apActor, -1);
								return;
							}
						} else {
							TESBoundObject* v27;
							if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 1) && apActor != pPlayer) {
								goto LABEL_438;
							}
							if (!apActor->IsPathingComplete() || apActor->IsRotating()) {
								return;
							}
							TESObjectREFR* pRef_2 = 0;
							if (pPack->pPackLoc) {
								pRef_2 = pPack->pPackLoc->GetLocReference();
							}
							if (pGenericLocation) {
								pRef_2 = pGenericLocation;
							}
							if ((pRef_2 && (v27 = pRef_2->GetObjectReference(), v27 == pXMarkerHeading)) ||
								(pRef_2 && (pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC))) {
								if (!DidPathingFail(apActor) && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
									const NiPoint3* v28 = &pRef_2->GetAngleOnReference();
									float v41 = v28->z - (&apActor->GetAngleOnReference())->z;
									if (std::fabs(v41) > 0.01745329238474369) {
										const NiPoint3* v29 = &pRef_2->GetAngleOnReference();
										apActor->RequestRotate(v29->z, 1);
									}
								}
							}
						}
						if (bContinuingPackageforPC) {
							AddToProcedureIndexRunning(apActor, -1);
							SetCurrentActionComplete(0);
							return;
						}
						if (pPack) {
							if (pPack->GetOncePerDay()) {
								char v30 = appCalendar.GetDay();
								apActor->SetRunOnceExtraPackage(pPack, v30);
							}
						}
						if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACTIVATE &&
							pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
							if (!pPack->GetPackageSchedule()->PSData.iDuration &&
								pPack->GetPackType() != PACKAGE_TYPE_COMBAT) {
								SetTarget(0);
								if (!ContinuingPackageforPC()) {
									if (GetRunOncePackage()) {
										ClearRunOncePackage();
										pPack = GetPackageThatIsRunning();
									} else {
										if (pPack) {
											if (pPack->GetIsCreated()) {
												if (pPack->IsInterruptPackage()) {
													ClearCurrentPackage();
													LoadPackageFromExtraData(apActor);
													pPack = GetPackageThatIsRunning();
												}
												if (!apActor->IsPathingComplete()) {
													EndMoveMessage(apActor);
												}
											}
										}
									}
								}
								if (pPack) {
									if (pPack->GetOncePerDay() ||
										(pPack->GetMustReachLocation() || pPack->GetMustComplete())) {
										fPackageEvalTimer = 0.0f;
									}
								}
								ObjectstoAcquire* apThis = pAcquireObject;
								if (apThis) {
									apThis->Delete(1u);
								}
								pAcquireObject = 0;
								pItemBeingUsed = 0;
								while (!ObjectList.IsEmpty()) {
									ObjectstoAcquire* pobj = (&ObjectList)->m_item;
									if (pobj) {
										pobj->Delete(1u);
									}
									ObjectList.Remove(pobj);
								}
								fDetectListTimer = 0.0f;
								pGenericLocation = 0;
								GenericLocationsList.RemoveAll();
							}
							if (GetFinishingCombatPackage()) {
								SetFinishingCombatPackage(0);
							}
							goto LABEL_477;
						}
						if (!GetCurrentActionComplete()) {
							goto LABEL_438;
						}
						if (!pTarget) {
							SetTargetForPackage(apActor);
						}
						if (pTarget) {
							if (pTarget->IsFurniture() && GetSitSleepState() == SIT_SLEEP_NORMAL) {
								AddToProcedureIndexRunning(apActor, -1);
							}
						}
						break;
					}
					}
					return;
				}
				default: {
					goto LABEL_477;
				}
				}
			}
			goto LABEL_477;
		}
	}
}

NiPoint3 HighProcess::GetLastPositionDetected(Actor* apActor, int32_t abWhichList) {
	NiPoint3 a2;
	NiPoint3 LastPositionDetected = NiPoint3_ZERO;
	DetectionState* v5 = GetDetectionState(apActor, abWhichList);
	if (v5) {
		LastPositionDetected = v5->LastPositionDetected;
	}
	a2 = LastPositionDetected;
	return a2;
}

float HighProcess::GetLastTimeDetected(Actor* apActor, int32_t abWhichList) {
	float v4 = -0x1.fffffe0000000p+127f;
	DetectionState* v5 = GetDetectionState(apActor, abWhichList);
	if (v5) {
		return (float)(&v5->fLastTimeDetected)->operator float();
	}
	return v4;
}

bool HighProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
	TESPackage* v4;
	PACKAGE_PROCEDURE_TYPE parent;
	TESPackage* pRunOnce;
	if (apActor->GetSitSleepState() &&
		((((int32_t)apActor->GetSitSleepState()) != 4) && (((int32_t)apActor->GetSitSleepState()) != 9))) {
		return (bool)0;
	}
	if ((GetRunOncePackage() && (!abFlag)) || GetEndIdlesPlayed()) {
		return (bool)0;
	}
	if (GetRunOncePackage()) {
		v4 = GetRunOncePackage();
		if (((int32_t)v4->GetPackType()) != 26) {
			ClearRunOncePackage();
		}
	}
	apActor->GetCurrentPackage();
	float hour = (float)(&appCalendar)->GetHour();
	uint8_t v10 = 0;
	TESPackage* activeStablePackage = apActor->GetPackageSetAsPcurrent();
	if (activeStablePackage) {
		if (activeStablePackage->IsScriptPackage()) {
			parent = activeStablePackage->TESPackage::GetProcedureType();
			if (Procedure_Action[(uint32_t)parent][GetCurrentProcedureIndex()] != 54) {
				return (bool)0;
			}
		}
	}
	if (abFlag ||
		((!GetCurrentPackage()) ||
			((fPackageEvalTimer <= 0.0) || (iHourPackageEvaluated != ((uint32_t)((int32_t)hour)))))) {
		v10 = (uint8_t)LowProcess::CheckforNewPackage(apActor, abFlag);
		iHourPackageEvaluated = (uint32_t)((int32_t)hour);
		fPackageEvalTimer = 20.0f;
	}
	fPackageEvalTimer = (float)(fPackageEvalTimer - (&appTimer)->GetSeconds());
	if (v10) {
		pRunOnce = GetRunOncePackage();
		GetCurrentPackage();
		if (pRunOnce) {
			if (((int32_t)pRunOnce->GetPackType()) != 26) {
				ClearRunOncePackage();
			}
		}
		ClearShootingAction(apActor);
	}
	return (bool)v10;
}

bool HighProcess::SetupSpecialIdle(MobileObject* apMobile, TESIdleForm* apIdle, AnimIdle::PLAY_TYPE_ENUM aeType,
	bool abForce3rd, bool abChooseNewOne, bool abTestConditions) {
	Actor* pactor = 0;
	Animation* panim = apMobile->GetAnimation();
	if (apMobile->IsActor()) {
		pactor = static_cast<Actor*>(apMobile);
	}
	if (!pactor || !panim) {
		return 0;
	}
	if (pactor == pPlayer && pVATS.eMode == VATS::VATS_PLAYBACK) {
		return 0;
	}
	SIT_SLEEP_STATE v11 = pactor->GetSitSleepState();
	if (v11 && (v11 != SIT_SLEEP_IS_SITTING && v11 != SIT_SLEEP_IS_SLEEPING)) {
		return 0;
	}
	if (pactor->IsRestrained() && !apIdle) {
		return 0;
	}
	if (pactor->IsParalyzed() || pactor->IsKnockedOut()) {
		return 0;
	}
	if (pactor->GetCurrentPackage()) {
		TESPackage* CurrentPackage = pactor->GetCurrentPackage();
		if (CurrentPackage->GetNoIdlesFlag()) {
			return 0;
		}
	}
	if (!panim->SpecialIdleDonePlaying() && (!apIdle && (pPlayer != pactor && !abChooseNewOne))) {
		return 0;
	}
	if (abTestConditions) {
		if (apIdle) {
			// The exe tests this address (&apIdle->Conditions), which is never null.
			TESObjectREFR* v9 = GetTarget();
			if (!(&apIdle->Conditions)->IsTrue(pactor, v9)) {
				apIdle = 0;
			}
		}
	}
	if (apIdle) {
		if (!pactor->CanUseIdle(apIdle)) {
			apIdle = 0;
		}
	}
	if (!apIdle && !panim->SpecialIdleWorking() &&
		(pactor->GetAnimAction() == ANIM_ACTION_NONE || aeType == AnimIdle::PLAY_TYPE_AUTO)) {
		TESObjectREFR* v10 = GetTarget();
		apIdle = pIdleManager->GetIdleToPlay(apMobile, v10);
	}
	if (!apIdle || panim->IsIdleNotLoaded(apIdle)) {
		return 0;
	}
	pIdleToPlay = apIdle;
	eSpecialIdleType = aeType;
	SetEndIdlesPlayed(0);
	if (pactor == pPlayer) {
		if (abForce3rd) {
			FinishSetupSpecialIdle(pPlayer);
		}
		pIdleToPlay = 0;
	} else {
		AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_PLAY_IDLE);
	}
	return 1;
}

void HighProcess::SetLastSeenLocation() {
	if (pTarget) {
		lastSeenPostion = pTarget->GetLocationOnReference();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool HighProcess::EvaluateDetection(Actor* apActor) {
	uint32_t uiAggroRadius;
	bool bsearch;
	if (apActor->IsDead(0)) {
		return 0;
	}
	if (!ReferenceProcessLists.bRunDetection) {
		return 0;
	}
	bCheckDeadTalk = 0;
	TESPackage* pPack = GetPackageThatIsRunning();
	TESPackage* pEditorPack = apActor->GetPackageSetAsPcurrent();
	bool bspectatorstart = 0;
	TempRefList.RemoveAll();
	Actor* pactor = (Actor*)pPlayer;
	int32_t v66 = -100;
	bool bcombat = 0;
	int32_t iCombatLevel = 0;
	Actor* pcombattarget = 0;
	bool bcrime = 0;
	float fdistance = fSneakMaxDistance.Float();
	bool bPlayerCriminal = 0;
	int32_t pCrimeTarget = 0;
	bool bcombattargetIsGuard = 0;
	int32_t phelpgroup = 0;
	bool bAddAvoidActor = 0;
	float fCombatStrength = apActor->CalculateCombatStrength(-1.0f);
	CombatGroup* pcurrentGroup = apActor->GetCombatGroup();
	TESActorBase* pbase = apActor->GetActorBase();
	if (pbase && pbase->AIData.bAggroRadius) {
		uiAggroRadius = pbase->AIData.uiAggroRadius;
	} else {
		uiAggroRadius = 0;
	}
	int32_t iAggroRadius = uiAggroRadius;
	bool bAddAggroActor = 0;
	FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
	FIGHT_REACTION ereaction2 = FIGHT_REACTION_NEUTRAL;
	ACTOR_AGGRESSION eaggression = static_cast<ACTOR_AGGRESSION>(apActor->GetActorValue(ActorValue::AGGRESSION));
	float fConfidence = apActor->GetActorFloatValue(ActorValue::CONFIDENCE);
	float fThierCombatStrength = pactor->CalculateCombatStrength(-1.0f);
	if (fThierCombatStrength == 0.0) {
		fThierCombatStrength = 1.0f;
	}
	if (apActor->GetShouldAttackActor(pPlayer, 0, ereaction, 0)) {
		iCombatLevel = 100;
	} else {
		if (iAggroRadius > 0 && (ereaction == FIGHT_REACTION_NEUTRAL || ereaction == FIGHT_REACTION_ENEMY)) {
			NiPoint3 vec = *apActor->QLocationOnReference() - *pactor->QLocationOnReference();
			double v3 = vec.SqrLength();
			if (static_cast<double>(iAggroRadius * iAggroRadius) >= v3) {
				bAddAggroActor = 1;
			}
		}
	}
	if (pactor != apActor) {
		int32_t idetection = -100;
		DetectionState* pstate = GetDetectionState(pactor, 0);
		if (pstate) {
			idetection = pstate->iLevel;
		}
		if (pPlayer->IsDead(0) && !pPlayer->IsDying()) {
			BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
			if (!CurrentAIProcess->GetGreetingFlag() && pPlayer->fCheckMyDeadBodyTimer <= 0.0) {
				bCheckDeadTalk = 1;
			}
		}
		float fdetectvalue = fSneakNoticedMin.Float();
		if (pEditorPack) {
			if (!pEditorPack->GetCombatAlert()) {
				fdetectvalue = 0.0f;
			}
		}
		if (fdetectvalue < static_cast<double>(idetection) && (!pstate || !pstate->bEvaluated)) {
			if (pstate) {
				pstate->bEvaluated = 1;
			}
			if (!pactor->IsDead(0)) {
				if (bAddAvoidActor) {
					Actor* a2 = (Actor*)pPlayer;
					AvoidActorList.AddHead(a2);
				} else {
					if (iCombatLevel <= 0) {
						if (bAddAggroActor && idetection > 0 &&
							!apActor->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_AGGRO_RADIUS_BEHAVIOR)) {
							Actor* v19 = (Actor*)pPlayer;
							AggroRadiusList.AddHead(v19);
						}
					} else {
						if (!pactor->IsInCombatantFaction() ||
							(!pcombattarget || !pcombattarget->IsInCombatantFaction())) {
							bool baggro = 1;
							int32_t NumberActorsInCombat = pPlayer->GetNumberActorsInCombat();
							if (NumberActorsInCombat >= iCombatTargetPlayerSoftCap.Int()) {
								double v13 = static_cast<double>(idetection);
								// `test ah, 1; jne` keeps baggro on less or unordered (0x8F58E8).
								baggro = !(fSneakSeenMin.Float() >= v13);
							}
							if (baggro) {
								StartCombatStates* psta;
								bool abflag = !apActor->IsActoraGuard();
								if (apActor->GetActorBase()->IsInEvilFactionsOnly()) {
									abflag = 1;
								}
								StartCombatStates* HighProcess__UnkCombatTarget =
									static_cast<StartCombatStates*>(ourMalloc(sizeof(StartCombatStates)));
								if (HighProcess__UnkCombatTarget) {
									psta = ::new (HighProcess__UnkCombatTarget) StartCombatStates;
								} else {
									psta = 0;
								}
								StartCombatStates* psta_2 = psta;
								psta->pTarget = pactor;
								psta_2->bSendAlarm = abflag;
								AggroList.AddHead(psta_2);
								plastDetected = (TESObjectREFR*)pPlayer;
							}
						}
					}
				}
				CombatGroup* pgroup = pactor->GetCombatGroup();
				double v11 = static_cast<double>(idetection);
				if (fSneakSeenMin.Float() < v11) {
					if (pgroup) {
						if (pgroup != pcurrentGroup) {
							if (pgroup->GetTargetCount()) {
								if (!IsGroupAlreadyInList(pgroup) &&
									pCombatManager->GetWanttoHelpGroup(apActor, pactor)) {
									StartCombatStates* pstat;
									StartCombatStates* v18 =
										static_cast<StartCombatStates*>(ourMalloc(sizeof(StartCombatStates)));
									if (v18) {
										pstat = ::new (v18) StartCombatStates;
									} else {
										pstat = 0;
									}
									StartCombatStates* pstat_2 = pstat;
									pstat->pGroup = pgroup;
									GroupsToHelpList.AddHead(pstat_2);
								}
							}
						}
					}
				}
			}
		}
		if (idetection > 0 && (pactor == pPlayer || pactor->GetActorValue(ActorValue::SNEAK) < 100) &&
			pactor->IsTresPassing() && !apActor->IsProcessingTresPassAlarm()) {
			TESObjectCELL* pcell = pactor->GetParentCell();
			TESForm* powner = pcell->GetOwner();
			bool bevil = 0;
			if (powner) {
				if (powner->GetFormType() == NPC__ID) {
					if (static_cast<TESNPC*>(powner)->TESActorBaseData::IsInEvilFactionsOnly()) {
						bevil = 1;
					}
				} else {
					if (powner->GetFormType() == FACT_ID) {
						if (static_cast<TESFaction*>(powner)->TESFaction::GetIsEvil()) {
							bevil = 1;
						}
					}
				}
			}
			TESActorBase* ptestactorbase = apActor->GetActorBase();
			if (!bevil || ptestactorbase->IsInEvilFactionsOnly()) {
				TESObjectCELL* pcell_2 = pactor->GetParentCell();
				if (pactor->IsTresPassing()) {
					TrespassPackage* ptrespack = (&pactor->m_Extra)->GetTrespassPackage();
					TESForm* v30 = 0;
					if (ptrespack && ptrespack->pObjectOwned) {
						TESForm* pownedcell = ptrespack->pObjectOwned;
						if (pownedcell->GetFormType() == CELL_ID && pcell_2 != pownedcell) {
							(&pactor->m_Extra)->RemoveTrespassPackage();
							ptrespack = 0;
						}
					} else {
						if (!ptrespack) {
							TrespassPackage* ptrespack_2;
							int32_t iwarningvalue = 3;
							if (pcell_2->IsOffLimits()) {
								iwarningvalue = 0;
							}
							TrespassPackage* v17 = static_cast<TrespassPackage*>(ourMalloc(sizeof(TrespassPackage)));
							if (v17) {
								TESForm* Owner = pcell_2->GetOwner();
								ptrespack_2 = ::new (v17) TrespassPackage(pactor, Owner, pcell_2, iwarningvalue);
							} else {
								ptrespack_2 = 0;
							}
							ptrespack = ptrespack_2;
							(&pactor->m_Extra)->SetTrespassPackageExtra(ptrespack_2);
							pactor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
						}
					}
					if (!ReferenceProcessLists.GetCrimeinGlobalCrimeList(pactor, 0, PackageNS::CRIME_TYPE_TRESPASS)) {
						if (!apActor->IsTresPassing()) {
							BSSimpleList<StartCombatStates*>* plist;
							for (plist = &AggroList; plist && plist->m_item; plist = plist->m_pkNext) {
								StartCombatStates* psta_3 = plist->m_item;
								ourFree(psta_3);
							}
							AggroList.RemoveAll();
							for (plist = &GroupsToHelpList; plist && plist->m_item; plist = plist->m_pkNext) {
								StartCombatStates* psta_4 = plist->m_item;
								ourFree(psta_4);
							}
							GroupsToHelpList.RemoveAll();
							AvoidActorList.RemoveAll();
							return apActor->InitiateTresPassPackage(ptrespack);
						}
						if (!pactor->IsTresPassing()) {
							(&pactor->m_Extra)->RemoveTrespassPackage();
						}
					}
				} else {
					if (!pactor->IsTresPassing()) {
						(&pactor->m_Extra)->RemoveTrespassPackage();
					}
				}
			}
		}
		bsearch = 0;
		if (pPlayer->IsPlayerCharacterInCombat(&bsearch)) {
			ObserveCombat(apActor, pactor);
		}
	}
	plastDetected = 0;
	if (!apActor->GetParentCell() || !apActor->GetParentCell()->GetInterior()) {
		fdistance = fdistance + fdistance;
	}
	for (uint32_t i = 0;; ++i) {
		uint32_t ProcessLevelTail = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
		if (i >= ProcessLevelTail) {
			break;
		}
		TESObjectREFR* pref = (TESObjectREFR*)(&ReferenceProcessLists.MobProcessArray)->GetItem(i);
		DetectActor(pref, apActor, bspectatorstart, 0, eaggression, fConfidence, fCombatStrength);
	}
	EvaluateCombatDetectionLists(apActor);
	bEvaluateDetection = 0;
	bool bflag = plastDetected != 0;
	plastDetected = 0;
	return bflag;
}

void HighProcess::EvaluateCombatDetectionLists(Actor* apActor) {
	BSSimpleList<StartCombatStates*>* plist = &GroupsToHelpList;
	CombatGroup* pgroup = apActor->GetCombatGroup();
	while (plist) {
		if (!plist->m_item) {
			break;
		}
		StartCombatStates* psta = plist->m_item;
		if (pgroup != psta->pGroup) {
			bool baggresor = 0;
			if (psta->pGroup) {
				Actor* pact = psta->pGroup->GetMember(0);
				if (pact) {
					BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
					baggresor = CurrentAIProcess->GetIsAggressor();
				}
			}
			EnterCombat(apActor, 0, 0, psta->bSendAlarm, psta->pGroup, psta->bFleeing, 0, baggresor, 0, 0, 0, 1, 0);
			ourFree(psta);
			if (!apActor->GetCombatController() || pgroup != apActor->GetCombatGroup()) {
				break;
			}
		}
		plist = plist->m_pkNext;
	}
	plist = &AggroList;
	// The exe also tests the list's address, which is never null.
	if (!plist->m_item) {
		SetDetectAlert(0);
	}
	while (plist && plist->m_item) {
		StartCombatStates* psta_2 = plist->m_item;
		bool baggressor = 0;
		bool bcrimecaused = 0;
		FIGHT_REACTION eFactionFight = apActor->GetFactionFightReaction(psta_2->pTarget, bcrimecaused);
		if ((!apActor->IsAngryWithPlayer() || psta_2->pTarget != pPlayer) && !bcrimecaused &&
			apActor->GetShouldAttackActor(psta_2->pTarget, 0, eFactionFight, 0)) {
			baggressor = 1;
		}
		EnterCombat(apActor, psta_2->pTarget, 0, psta_2->bSendAlarm, 0, 0, 0, baggressor, 0, 0, 0, 1, 0);
		ourFree(psta_2);
		plist = plist->m_pkNext;
	}
	if (apActor->IsInCombat() || AggroRadiusList.IsEmpty()) {
		if (apActor->IsInCombat() || AvoidActorList.IsEmpty()) {
			if (!apActor->IsInCombat()) {
				for (plist = &SpectatorList; plist && plist->m_item; plist = plist->m_pkNext) {
					StartCombatStates* m_item = plist->m_item;
					pCombatManager->AddSpectatorForCombatGroup(apActor, m_item->pTarget);
					ourFree(m_item);
				}
			}
		} else {
			for (Actor* pact_2 : AvoidActorList.ItemsUntilNull()) {
				if (!apActor->IsFleeing(0)) {
					apActor->InitiateFlee(pact_2, 0, 1, 1, 0, 0, -1.0f, -1.0f);
				}
				FleePackage* pfleePack = static_cast<FleePackage*>(GetPackageThatIsRunning());
				if (pfleePack) {
					if (pfleePack->GetPackType() == PACKAGE_TYPE_FLEE) {
						pfleePack->AddAvoidedRef(pact_2);
					}
				}
			}
		}
	} else {
		TESPackage* v2;
		if (!GetRunOncePackage() || (v2 = GetRunOncePackage(), v2->GetPackType() != PACKAGE_TYPE_GUARD)) {
			apActor->InitiateAggroRadiusGuardPackage(apActor);
		}
	}
	GroupsToHelpList.RemoveAll();
	AggroList.RemoveAll();
	TargetToAddList.RemoveAll();
	SpectatorList.RemoveAll();
}

// True outside combat, and in combat on the detection passes that run player detection. The exe tests the flag's
// whole byte and returns 1, so the byte is read as an integer.
bool HighProcess::ShouldRunPlayerDetection(Actor* apActor) {
	return !apActor->Actor::IsInCombat() ||
		reinterpret_cast<const uint8_t&>(CombatDetectionCycles[iDetectionCounter].bRunPlayerDetection) != 0;
}

// Walks apActor to the package's target and activates it once there; an acquire object is taken (or its owner
// attacked) instead. Returns true when the target was activated this hour.
bool HighProcess::ProcessActivateOneHour(Actor* apActor, float afTimeDelta) {
	bool bAtTarget = false;
	TESPackage* pPackage = GetPackageThatIsRunning();
	if (!pPackage) {
		return false;
	}
	if (!apActor->IsPathingComplete()) {
		EndMoveMessage(apActor);
	}
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	if (!pTarget) {
		AddToProcedureIndexRunning(apActor, 1);
		return false;
	}
	if (pTarget->IsDead(true) && !pAcquireObject) {
		pPackage->SetNeverRun(pTarget, true);
		return false;
	}
	if (pTarget->GetDelete() || pTarget->GetDisabled()) {
		if (pTarget->GetDelete()) {
			pPackage->SetNeverRun(pTarget, true);
		}
		AddToProcedureIndexRunning(apActor, 1);
		return false;
	}
	if (pPackage->GetPackType() == PACKAGE_TYPE_AMBUSH && !pPackage->IsTargetAtLocation(apActor, 300)) {
		AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-1));
	}
	if (pTarget->GetTeleport()) {
		const NiPoint3* pActorLoc = &apActor->GetLocationOnReference();
		NiPoint3 kDiff = pTarget->GetLinkedDoorTeleportPosition() - *pActorLoc;
		double fDist = kDiff.Length();
		if (static_cast<int32_t>(iAIDistanceRadiusMinLocation.UInt()) >= fDist) {
			bAtTarget = true;
		}
	} else {
		bAtTarget = pPackage->IsActorAtRefTarget(apActor, 0);
	}
	if (!bAtTarget && !DidPathingFail(apActor)) {
		if (apActor->GetSitSleepState()) {
			ProcessGetUpOneHour(apActor);
		}
		NiPoint3 kGoal;
		kGoal.~NiPoint3();
		::new (&kGoal) NiPoint3;
		if (!pTarget->GetTeleport()) {
			kGoal = pTarget->GetLocationOnReference();
		} else {
			kGoal = pTarget->GetLinkedDoorTeleportPosition();
		}
		float fRadius = static_cast<float>(pPackage->GetPackageRadiusActorToRefTarget(apActor, false));
		TESWorldSpace* pWorld = pTarget->GetWorldSpace();
		TESObjectCELL* pCell = pTarget->GetParentCell();
		apActor->SetPathfindingGoal(kGoal, pCell, pWorld, fRadius, nullptr);
		if (apActor->GetCurrentProcessType()) {
			return false;
		}
		if (pTarget->GetTeleport()) {
			const NiPoint3* pActorLoc = &apActor->GetLocationOnReference();
			NiPoint3 kDiff = pTarget->GetLinkedDoorTeleportPosition() - *pActorLoc;
			double fDist = kDiff.Length();
			if (static_cast<int32_t>(iAIDistanceRadiusMinLocation.UInt()) >= fDist) {
				bAtTarget = true;
			}
		} else {
			bAtTarget = pPackage->IsActorAtRefTarget(apActor, 0);
		}
	}
	if (!bAtTarget) {
		return false;
	}
	if (DidPathingFail(apActor) && pPackage->GetMustComplete()) {
		return false;
	}
	if (!pTarget->IsActor()) {
		SetTargetActivated(1);
		if (pAcquireObject) {
			pTarget->Activate(apActor, true, static_cast<TESBoundObject*>(pAcquireObject->pObject),
				pAcquireObject->iamount);
			--iNumberItemsActivate;
			ObjectstoAcquire* pOld = pAcquireObject;
			if (pOld) {
				pOld->Delete(1);
			}
			pAcquireObject = nullptr;
			SetTarget(nullptr);
			if (iNumberItemsActivate >= 1) {
				return false;
			}
			if (pPackage->GetProcedureType() == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC &&
				!pPackage->GetPackageSchedule()->GetDuration()) {
				AddToProcedureIndexRunning(apActor, 2);
			} else {
				AddToProcedureIndexRunning(apActor, 1);
			}
			return true;
		}
		pTarget->Activate(apActor, false, nullptr, 1);
		AddToProcedureIndexRunning(apActor, 1);
		return true;
	}
	if (pTarget == pPlayer && pPlayer->IsSleepingorResting() && !pPlayer->btransporting) {
		pPlayer->SetSleeping(0, true);
		AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-2));
		return false;
	}
	if (pAcquireObject) {
		if (pTarget->IsDead(false) || pAcquireObject->bSteal) {
			pTarget->Activate(apActor, true, static_cast<TESBoundObject*>(pAcquireObject->pObject),
				pAcquireObject->iamount);
			--iNumberItemsActivate;
			ObjectstoAcquire* pOld = pAcquireObject;
			if (pOld) {
				pOld->Delete(1);
			}
			pAcquireObject = nullptr;
			SetTarget(nullptr);
		} else if (pAcquireObject->bKill) {
			EnterCombat(apActor, static_cast<Actor*>(pTarget), false, false, nullptr, false, true, false, true, false,
				false, true, nullptr);
		}
		if (iNumberItemsActivate <= 0) {
			AddToProcedureIndexRunning(apActor, 2);
		}
		return false;
	}
	if (pTarget != pPlayer) {
		AddToProcedureIndexRunning(apActor, 2);
		return false;
	}
	return false;
}

bool HighProcess::IsDoingSayTo() {
	return bIsDoingSayTo;
}

void HighProcess::ProcessSandman(Actor* apActor) {
	TESPackage* v9 = GetPackageThatIsRunning();
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	Actor* a1 = 0;
	if (pTarget) {
		if (pTarget->IsActor()) {
			a1 = static_cast<Actor*>(pTarget);
		}
	}
	if (static_cast<double>(static_cast<int32_t>(apActor->GetDistanceFromPoint(FurnitureMarker.Position))) <= 40.0) {
		if (GetWeaponDrawn()) {
			SetWantWeaponDrawn(0);
		}
		if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
			apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			AddToProcedureIndexRunning(apActor, 1);
			BaseProcess* CurrentAIProcess = a1->GetCurrentAIProcess();
			CurrentAIProcess->SetRunOncePackage(0, a1);
			SetTargetActivated(1);
			pPlayer->SetAIControlledPackage(0);
			a1->AttackAlarm(apActor, 0, 1);
		} else {
			if (!GetCurrentFurniture() || !SitOrSleep(apActor)) {
				if (pPlayer->GetSandManDetectionValue() <= 0) {
					a1->Kill(0, 0.0);
				} else {
					a1->Kill(apActor, 0.0);
				}
				AddToProcedureIndexRunning(apActor, 1);
				BaseProcess* v7 = a1->GetCurrentAIProcess();
				v7->SetRunOncePackage(0, a1);
				BaseProcess* v6 = apActor->GetCurrentAIProcess();
				v6->SetRunOncePackage(0, apActor);
				pPlayer->SetAIControlledPackage(0);
				pPlayer->Sneak();
			}
		}
	} else {
		if (apActor == pPlayer) {
			apActor->SetPosition(FurnitureMarker.Position);
			float zero = FurnitureMarker.GetHeading();
			apActor->SetHeading(zero);
		} else {
			TESWorldSpace* LocationWorld = v9->GetLocationWorld(apActor);
			TESObjectCELL* LocationCell = v9->GetLocationCell(apActor);
			apActor->SetPathfindingGoal(FurnitureMarker.Position, LocationCell, LocationWorld, 40.0f,
				static_cast<PathingAvoidNodeArray*>(0));
		}
	}
}

void HighProcess::ProcessCannibal(Actor* apActor) {
	GetPackageThatIsRunning();
	if (!this->pTarget) {
		SetTargetForPackage(apActor);
	}
	if (GetWeaponDrawn()) {
		SetWantWeaponDrawn(0);
	}
	Actor* pTarget = 0;
	if (this->pTarget) {
		if (this->pTarget->IsActor()) {
			pTarget = static_cast<Actor*>(this->pTarget);
		}
	}
	Animation* v6 = apActor->GetAnimation();
	if (GetCurrentActionComplete()) {
		if (v6->SpecialIdleDonePlaying()) {
			AddToProcedureIndexRunning(apActor, 1);
			if (pTarget) {
				if (pTarget->pCurrentProcess) {
					pTarget->AttackAlarm(apActor, 0, 1);
					BaseProcess* CurrentAIProcess = pTarget->pCurrentProcess;
					CurrentAIProcess->SetRunOncePackage(0, pTarget);
				}
			}
			BaseProcess* v2 = apActor->pCurrentProcess;
			v2->SetRunOncePackage(0, pPlayer);
			pPlayer->SetAIControlledPackage(0);
		}
	} else {
		SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 1, 1);
		if (pTarget) {
			pTarget->SetHasBeenEaten(1);
		}
		SetCurrentActionComplete(1);
	}
}

void HighProcess::SetDoingSayTo(bool abFlag) {
	bIsDoingSayTo = abFlag;
}

void HighProcess::ClearDialogTarget() {
	pDialogTarget = nullptr;
}

float HighProcess::GetSocialTalkTimer() {
	return fCheckToTalkTimer;
}

void HighProcess::SetSocialTalkTimer(float afTime) {
	fCheckToTalkTimer = afTime;
}

void HighProcess::RemoveSpokenToActor(Actor* apActor) {
	pLastSpokeToList->Remove(apActor);
}

bool HighProcess::GetDetectAlert() {
	return bWeaponAlertDrawn;
}

void HighProcess::SetDetectAlert(bool abflag) {
	bWeaponAlertDrawn = abflag;
}

void HighProcess::RandomlyPlaySpecialIdles(Actor* apActor) {
	if (apActor->Get3D()) {
		TESPackage* pPackage = GetPackageThatIsRunning();
		Animation* pAnim = apActor->GetAnimation();
		if (pPackage) {
			if (pPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
				pPackage = (&apActor->m_Extra)->GetPackageExtra();
			}
		}
		if (GetEndIdlesPlayed() && !pAnim->SpecialIdleWorking() && pAnim->SpecialIdleDonePlaying()) {
			SetEndIdlesPlayed(0);
		}
		NiNode* v2 = apActor->Get3D();
		const NiBound* WorldBound = &v2->GetWorldBound();
		float Seconds = WorldBound->m_fRadius;
		if (!pIdleToPlay) {
			double DistanceFromReference = apActor->GetDistanceFromReference(pPlayer, 0, 1);
			float v11 = Seconds / 64.0;
			if (!(fAIIdleAnimationDistance.Float() * v11 < DistanceFromReference) &&
				(!pPackage || (!pPackage->pIdleCollection && pPackage->GetPackType() != PACKAGE_TYPE_GET_UP))) {
				if (pAnim) {
					if (!pAnim->SpecialIdleWorking() && pAnim->SpecialIdleDonePlaying() &&
						GetAnimAction() == ANIM_ACTION_NONE && !apActor->IsEating() && !apActor->IsParalyzed()) {
						float v8 = GetIdleTimer();
						if (!(v8 < 0.0) || fAIIdleWaitTime.Float() == 0.0) {
							float v5 = v8 - appTimer.fDelta;
							SetIdleTimer(v5);
						} else {
							SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
							float* v4 = &fAIIdleWaitTime.Float();
							SetIdleTimer(*v4);
						}
					}
				}
			}
		}
	}
}

void HighProcess::PostAnimFreeUpSpecialIdle(Actor* apActor) {
	BSFaceGenAnimationData* faceAnimationData;
	Animation* v3;
	if ((((int32_t)apActor->GetSitSleepState()) == 0) ||
		((((int32_t)apActor->GetSitSleepState()) == 4) || (((int32_t)apActor->GetSitSleepState()) == 9))) {
		v3 = apActor->GetAnimation();
		if (v3) {
			v3->SpecialIdleFree(1, 0);
		}
		faceAnimationData = apActor->GetFaceAnimationData();
		if (faceAnimationData) {
			faceAnimationData->ResetAnimCurrentMorphs(0.75f, (bool)1, (bool)1, (bool)1, (bool)1);
		}
	}
}

void HighProcess::ResetSearchTimer() {
	fEvaluateAcquireTimer = 0.0f;
}

void HighProcess::ResetSearchChatterTimer() {
	fIdleChatterTimer = 0.0f;
}

DetectionState* HighProcess::InsertIntoDetectionList(Actor* apActor, DETECTION_LEVEL aedetect, bool abLineSight,
	int32_t aiLevel, int32_t abWhichList, bool ab360view, bool abcombat) {
	void* pMemory = ourMalloc(sizeof(DetectionState));
	DetectionState* pState = pMemory ? ::new (pMemory) DetectionState : nullptr;
	if (!abWhichList) {
		pState->bEvaluated = false;
		pThreadDetectList->AddHead(pState);
	} else {
		pTempActorsWhoDetectMeList->AddHead(pState);
	}
	pState->eDetectionLevel = aedetect;
	pState->pActor = apActor;
	pState->bLineofSight = abLineSight;
	pState->iLevel = aiLevel;
	pState->b360view = ab360view;
	pState->bcombat = abcombat;
	if (aiLevel > 0) {
		TESObjectREFR* pRef = apActor;
		const NiPoint3* pLocation = &pRef->GetLocationOnReference();
		pState->LastPositionDetected.x = pLocation->x;
		pState->LastPositionDetected.y = pLocation->y;
		pState->LastPositionDetected.z = pLocation->z;
		CombatTimeStamp TimeStamp;
		FNVD_RECONSTRUCT(CombatTimeStamp)(&TimeStamp, CombatTimer::GetTimer());
		pState->fLastTimeDetected = TimeStamp;
	}
	return pState;
}

// The exe's cleanup-only C++ EH frame is left out.
BSSimpleList<DetectionState*>* HighProcess::GetWhoDetectsMe(Actor* apActor) {
	BSSimpleList<DetectionState*>* pNewList;
	BSSimpleList<DetectionState*>* plist = pActorsWhoDetectMeList;
	DetectionState* v9 = 0;
	BSSimpleList<DetectionState*>* v6 =
		static_cast<BSSimpleList<DetectionState*>*>(ourMalloc(sizeof(BSSimpleList<DetectionState*>)));
	if (v6) {
		pNewList = ::new (v6) BSSimpleList<DetectionState*>;
	} else {
		pNewList = 0;
	}
	while (plist && plist->m_item) {
		DetectionState* pstate;
		DetectionState* pcurrentstate = plist->m_item;
		plist = plist->m_pkNext;
		DetectionState* apMemory = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
		if (apMemory) {
			pstate = ::new (apMemory) DetectionState;
		} else {
			pstate = 0;
		}
		DetectionState* pstate_2 = pstate;
		*pstate = *pcurrentstate;
		pNewList->AddHead(pstate_2);
	}
	return pNewList;
}

DetectionEvent* HighProcess::GetActorsDetectionEvent(Actor*) {
	return pActorsGeneratedDetectionEvent;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::SetActorsDetectionEvent(Actor* apActor, NiPoint3 aLocation, int32_t aiSoundLevel,
	int32_t aiDetectionEventType, TESObjectREFR* apRef) {
	DetectionEvent* v7;
	if (pActorsGeneratedDetectionEvent) {
		void* Block = pActorsGeneratedDetectionEvent;
		ourFree(Block);
	}
	void* apMemory = ourMalloc(sizeof(DetectionEvent));
	if (apMemory) {
		v7 = ::new (apMemory) DetectionEvent;
	} else {
		v7 = 0;
	}
	pActorsGeneratedDetectionEvent = v7;
	pActorsGeneratedDetectionEvent->iActionValue = aiSoundLevel;
	pActorsGeneratedDetectionEvent->Location = aLocation;
	float a2 = CombatTimer::GetTimer();
	FNVD_CONSTRUCTED_LOCAL(CombatTimeStamp, v9, nullptr, a2);
	pActorsGeneratedDetectionEvent->fTimeStamp = v9;
	pActorsGeneratedDetectionEvent->pRef = apRef;
	if (apRef) {
		apRef->SetTargeted(1);
	}
}

void HighProcess::RemoveDetectionEvent() {
	if (pActorsGeneratedDetectionEvent) {
		ourFree(pActorsGeneratedDetectionEvent);
	}
	pActorsGeneratedDetectionEvent = nullptr;
}

void HighProcess::CheckForExpiredDetectionEvent() {
	double elapsedTime;
	if (pActorsGeneratedDetectionEvent) {
		elapsedTime = (double)(&pActorsGeneratedDetectionEvent->fTimeStamp)->ElapsedTime();
		if (((&fDetectionEventExpireTime)->Float()) < elapsedTime) {
			ourFree(pActorsGeneratedDetectionEvent);
			pActorsGeneratedDetectionEvent = nullptr;
		}
	}
}

bool HighProcess::GetLipQuequed() {
	return bLipQuequed;
}

void HighProcess::SetLipQuequed(bool abFlag) {
	bLipQuequed = abFlag;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::CreateFollownoEscort(Actor* apActor, bool abflag) {
	BaseProcess* CurrentAIProcess;
	bool bCreate = 0;
	TESPackage* pPack = GetPackageThatIsRunning();
	if (!pPack || (CurrentAIProcess = apActor->pCurrentProcess, CurrentAIProcess->GetGreetingFlag())) {
		bCreate = 1;
	} else {
		if (pPack->GetPCContinue() && pPack->GetServicesEnabled()) {
			bCreate = 1;
		} else {
			if (pPack->GetPackType() == PACKAGE_TYPE_WANDER) {
				uint32_t Count = pPack->ePROCEDURE_TYPE;
				if (Procedure_Action[Count][GetProcedureIndexRunning()] == PROCEDURE_WANDER) {
					bCreate = 1;
				}
			} else {
				if (pPack->GetPackType() == PACKAGE_TYPE_TRAVEL || pPack->GetPackType() == PACKAGE_TYPE_EXPLORE) {
					uint32_t v4 = pPack->ePROCEDURE_TYPE;
					if (Procedure_Action[v4][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
						bCreate = 1;
					}
				}
			}
		}
	}
	if (bCreate) {
		PackageTarget* pnew;
		if (apActor->GetSitSleepState()) {
			apActor->InitiateGetUpPackage();
		}
		TESPackage* ppack__pPack = TESPackage::CreatePackage(PACKAGE_TYPE_FOLLOW);
		ppack__pPack->SetPackType(PACKAGE_TYPE_FOLLOW);
		ppack__pPack->SetMustReachLocation(1);
		ppack__pPack->SetMustComplete(1);
		PackageTarget* v9 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
		if (v9) {
			pnew = ::new (v9) PackageTarget;
		} else {
			pnew = 0;
		}
		ppack__pPack->SetPackageTarget(pnew);
		if (pnew) {
			delete pnew;
		}
		ppack__pPack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
		ppack__pPack->GetPackageTarget()->SetTargReference(pPlayer);
		if (abflag) {
			ppack__pPack->GetPackageTarget()->iValue = 3000u;
		} else {
			ppack__pPack->GetPackageTarget()->iValue = 80u;
		}
		if (pPack) {
			ppack__pPack->pCombatStyle = pPack->pCombatStyle;
			bool DrawWeaponFlag = pPack->GetDrawWeaponFlag();
			ppack__pPack->SetDrawWeaponFlag(DrawWeaponFlag);
		}
		ppack__pPack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FOLLOW_WITHOUT_ESCORT;
		apActor->PutCreatedPackage(ppack__pPack, 1, 1);
		SetActionHeadTrackTarget(pPlayer);
		if (abflag) {
			fAwarePlayerTimer = fAIAwareofPlayerTimer.Float();
		}
	}
}

float HighProcess::GetAwarePlayerTimer() {
	return fAwarePlayerTimer;
}

void HighProcess::ModAwarePlayerTimer(float aftime) {
	fAwarePlayerTimer += aftime;
}

void HighProcess::AddTalkedtoActor(Actor* apActor) {
	pLastSpokeToList->AddHead(apActor);
}

bool HighProcess::ComputeCanInitiateDialogueWithNPC(Actor* apActor, Actor* apTalkToActor) {
	if (pPlayer->IsImportantConversationRunning() || pPlayer->GetPlayerGreetFlag()) {
		return 0;
	}
	if (!GetPackageThatIsRunning()) {
		return 0;
	}
	if (!apActor->CanSpeak() || apActor->IsSleeping()) {
		return 0;
	}
	if (fTalkTimer > 0.0 ||
		(apActor->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_RANDOM_CONVERSATIONS) ||
			(apTalkToActor == apActor ||
				(apTalkToActor == pPlayer ||
					(!apTalkToActor->CanSpeak() ||
						(!apTalkToActor->Get3D() || !apTalkToActor->ShouldTalkTo(apActor))))))) {
		return 0;
	}
	apActor->GetPackageSetAsPcurrent();
	apTalkToActor->GetPackageSetAsPcurrent();
	BaseProcess* CurrentAIProcess = apTalkToActor->pCurrentProcess;
	if (!CurrentAIProcess || !CurrentAIProcess->ComputeCanRespondToDialogueFromNPC(apTalkToActor, apActor)) {
		return 0;
	}
	if (!ReferenceProcessLists.bRunDetection) {
		return 1;
	}
	DetectionState* v5 = GetDetectionState(apTalkToActor, 0);
	return v5 && v5->fLastTimeDetected.ElapsedTime() < 30.0;
}

bool HighProcess::ComputeCanRespondToDialogueFromNPC(Actor* apActor, Actor* apInitiatingActor) {
	uint8_t result = 0;
	if ((GetCommandingActor() || GetGreetingFlag()) ||
		apActor->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_RANDOM_CONVERSATIONS)) {
		return (bool)result;
	}
	if (!GetCurrentPackage()) {
		return (bool)1;
	}
	TESPackage* v3 = GetCurrentPackage();
	if (!v3->IsInterruptPackage()) {
		return (bool)1;
	}
	return (bool)result;
}

// The exe tests the whole byte the package's virtual returns (movzx; test) and returns 0 or 1; the result goes
// through its byte in memory so that clang does not hand the callee's al on as an already normalised bool.
bool HighProcess::IsActorInsideAmbushArea(Actor* apActor, Actor* apTarget) {
	BaseProcess* pProcess = this;
	TESPackage* pPackage = pProcess->GetPackageThatIsRunning();
	if (!pPackage) {
		return false;
	}
	bool bInside = pPackage->IsActorAtSecondLocation(apTarget, apActor, false, -1.0f, false);
	return static_cast<uint8_t>(bInside) != 0;
}

bool HighProcess::CheckIfThereSomeoneTalkWith(Actor* apActor) {
	TESPackage* ppack = GetPackageThatIsRunning();
	bool bReturnFlag = 0;
	bool bmoving = 0;
	Actor* ptalkactor = 0;
	if (ppack) {
		if (!ppack->IsInterruptPackage() &&
			!apActor->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_RANDOM_CONVERSATIONS)) {
			bool bcontinue = 1;
			switch (ppack->GetPackType()) {
			case PACKAGE_TYPE_FOLLOW:
			case PACKAGE_TYPE_ESCORT:
			case PACKAGE_TYPE_ACCOMPANY: {
				bmoving = 1;
				ptalkactor = static_cast<Actor*>(pTarget);
				break;
			}
			case PACKAGE_TYPE_SLEEP:
			case PACKAGE_TYPE_USE_ITEM_AT:
			case PACKAGE_TYPE_AMBUSH:
			case PACKAGE_TYPE_GUARD:
			case PACKAGE_TYPE_DIALOGUE:
			case PACKAGE_TYPE_USE_WEAPON: {
				bcontinue = 0;
				break;
			}
			case PACKAGE_TYPE_SANDBOX: {
				bcontinue = ppack->SandboxGetAllowConversation();
				break;
			}
			default: {
				break;
			}
			}
			if (fCheckToTalkTimer <= 0.0 && bcontinue) {
				if (!ptalkactor) {
					bCheckDeadTalk = 0;
					for (DetectionState* pstate : pDetectedActorList->ItemsUntilNull()) {
						BaseProcess* v6;
						if (!pstate) {
							break;
						}
						ptalkactor = pstate->pActor;
						if (ptalkactor &&
							(!ptalkactor->GetDisabledOrDeleted() &&
								(pstate->eDetectionLevel == 3 &&
									(ptalkactor != apActor &&
										(ptalkactor != pPlayer &&
											((v6 = ptalkactor->pCurrentProcess,
												 v6->ComputeCanInitiateDialogueWithNPC(ptalkactor, apActor)) &&
												ComputeCanInitiateDialogueWithNPC(apActor, ptalkactor))))))) {
							TESPackage* ptargPack = ptalkactor->GetCurrentPackage();
							if (ptargPack &&
								(ptargPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_SANDBOX &&
									!ptargPack->SandboxGetAllowConversation())) {
								ptalkactor = 0;
							} else {
								float fsocialdistance = 200.0f;
								float fmaxsocialdistance = fAISocialRadiusToTriggerConversation.Float();
								if (apActor->GetParentCell()->GetInterior()) {
									fmaxsocialdistance = fAISocialRadiusToTriggerConversationInterior.Float();
								}
								if (fmaxsocialdistance < static_cast<double>(fsocialdistance)) {
									fsocialdistance = fAISocialRadiusToTriggerConversation.Float();
								}
								if (!(static_cast<double>(fsocialdistance) >=
										ptalkactor->GetDistanceFromReference(apActor, 0, 0))) {
									ptalkactor = 0;
								} else {
									if (pLastSpokeToList->IsEmpty()) {
										fClearTalkToListTimer = fAItalktosameNPCtimer.Float();
									} else {
										fClearTalkToListTimer = fClearTalkToListTimer - appTimer.fDelta;
									}
									if (fClearTalkToListTimer <= 0.0) {
										pLastSpokeToList->RemoveAll();
									}
									int32_t rnone = Random() % 100;
									if (pLastSpokeToList->IsInList(ptalkactor)) {
										ptalkactor = 0;
									} else {
										float fchancetalk = fAISocialchanceForConversation.Float();
										if (apActor->GetParentCell()->GetInterior()) {
											fchancetalk = fAISocialchanceForConversationInterior.Float();
										}
										if (fchancetalk > static_cast<double>(rnone)) {
											AddTalkedtoActor(ptalkactor);
											BaseProcess* CurrentAIProcess = ptalkactor->pCurrentProcess;
											CurrentAIProcess->AddTalkedtoActor(apActor);
											break;
										}
										ptalkactor = 0;
									}
								}
							}
						} else {
							ptalkactor = 0;
						}
					}
				}
				if (ptalkactor) {
					if (ptalkactor != pPlayer) {
						apActor->InitiateDialogue(ptalkactor, 0, 0, 1, bmoving, 0, 0, bmoving, 0);
						float afMax = fAISocialTimerForConversationsMax.Float();
						float* v2 = &fAISocialTimerForConversationsMin.Float();
						fCheckToTalkTimer = CombatFormulas::GetRandomBetween(*v2, afMax);
						return 1;
					}
				}
			}
		}
	}
	return bReturnFlag;
}

bool HighProcess::ComputeAllowSandboxConversation(Actor* apActor, Actor* apTalkToActor) {
	if (!apTalkToActor ||
		(apTalkToActor->GetDisabledOrDeleted() ||
			(apTalkToActor == apActor ||
				(apTalkToActor == pPlayer || !ComputeCanInitiateDialogueWithNPC(apActor, apTalkToActor))))) {
		return 0;
	}
	TESPackage* ptargPack = apTalkToActor->GetCurrentPackage();
	if (ptargPack &&
		(ptargPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_SANDBOX && !ptargPack->SandboxGetAllowConversation())) {
		return 0;
	}
	if (pLastSpokeToList->IsEmpty()) {
		fClearTalkToListTimer = fAItalktosameNPCtimer.Float();
	} else {
		fClearTalkToListTimer = fClearTalkToListTimer - appTimer.fDelta;
	}
	if (fClearTalkToListTimer <= 0.0) {
		pLastSpokeToList->RemoveAll();
	}
	return !pLastSpokeToList->IsInList(apTalkToActor);
}

void HighProcess::ClearGreetingInfoData() {
	// `delete pGreetTopic`: the exe calls DialogueItem's scalar deleting destructor out of line.
	DialogueItem* pTopic = pGreetTopic;
	if (pTopic)
		delete pTopic;
	pGreetTopic = nullptr;
	pSubtitleVoice = nullptr;
	m_pSayToDialogueTopic = nullptr;
}

void HighProcess::SetupNewPackage() {
	MiddleHighProcess::SetupNewPackage();
	bActivateAnim = false;
	sShotsFired = 0;
	sShotsToFire = -1;
	sBurstsFired = 0;
	fHoldAttackTimer = 0.0f;
	fScriptPackageEndTime = 0.0f;
	fUseItemTimer = 0.0f;
	bStop = false;
	fEvaluateAcquireTimer = 0.0f;
	bWeaponAlertDrawn = false;
	pCurrentIdle = nullptr;
	bFinishingCombatPackage = false;
}

DialogueItem* HighProcess::GetSubtitleItem() {
	return pSubtitleVoice;
}

void HighProcess::SetSubtitleItem(DialogueItem* apItem) {
	pSubtitleVoice = apItem;
}

// The exe's cleanup-only C++ EH frame is left out.
PathingAvoidNodeArray* HighProcess::CreateAvoidArray(Actor* apActor) {
	PathingAvoidNodeArray* parray = 0;
	if (apActor->ShouldActorAvoidRadiation()) {
		BSSimpleList<AvoidAreaStruct*>* plist = pListOfAvoidAreas;
		if (plist) {
			if (!plist->IsEmpty()) {
				PathingAvoidNodeArray* parray_2;
				PathingAvoidNodeArray* apMemory =
					static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
				if (apMemory) {
					parray_2 = ::new (apMemory) PathingAvoidNodeArray;
				} else {
					parray_2 = 0;
				}
				parray = parray_2;
				parray_2->Add(pListOfAvoidAreas->m_item->pAvoidNode);
			}
		}
	}
	return parray;
}

// The exe calls BSSimpleList's GetItem (0x6815C0) twice per node and GetNext (0x726070) out of line, and keeps
// walking until the found flag stops the loop.
bool HighProcess::IsAvoidAreaInAvoidPathingList(TESObjectREFR* apRef) {
	BSSimpleList<AvoidAreaStruct*>* pNode = pListOfAvoidAreas;
	bool bFound = false;
	while (pNode) {
		AvoidAreaStruct* pItem;
		pItem = pNode->GetItem();
		if (!pItem || bFound)
			break;
		pItem = pNode->GetItem();
		pNode = pNode->GetNext();
		if (pItem->pRefObj == apRef)
			bFound = true;
	}
	return bFound;
}

void HighProcess::SetAvoidWaitTimer(float aftime) {
	fAvoidWaitTimer = aftime;
}

// The exe calls BSSimpleList's GetItem (0x6815C0) twice per node, GetNext (0x726070) and the pointer list's Remove
// (0x905330) out of line.
void HighProcess::RemoveAvoidPathingNode(TESObjectREFR* apRef) {
	for (BSSimpleList<AvoidAreaStruct*>* pNode = pListOfAvoidAreas; pNode;) {
		AvoidAreaStruct* pItem;
		pItem = pNode->GetItem();
		if (!pItem)
			return;
		pItem = pNode->GetItem();
		if (pItem->pRefObj == apRef) {
			pListOfAvoidAreas->Remove(pItem);
			ourFree(pItem);
			return;
		}
		pNode = pNode->GetNext();
	}
}

float HighProcess::GetHeadTrackChangeTimer() {
	return fHeadTrackTargetTimer;
}

void HighProcess::SetHeadTrackChangeTimer(float afTimer) {
	fHeadTrackTargetTimer = afTimer;
}

void HighProcess::ClearLastHeadTrackTarget() {
	pLastTarget = nullptr;
}

void HighProcess::EndMoveMessage(Actor* apActor) {
	SetCurrentMovementComplete(true);
	if (apActor)
		apActor->StopMoving();
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessFollow(Actor* apActor, bool abflag, int32_t aemovemode, bool abpursuit) {
	double v12;
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	NiPoint3 delta;
	// The combat controller is a package: the exe takes it, else the running package.
	TESPackage* v96 = apActor->GetCombatController();
	if (!v96) {
		v96 = GetPackageThatIsRunning();
	}
	if (!v96 || !v96->GetWaiting()) {
		float* v11;
		float v42;
		float ffollowRadius;
		float fradiusRestartMovement;
		float fdistancemoved;
		TESObjectREFR* ptarg;
		bool btargetSwimming;
		Actor* ptargactor;
		bool bmustmove;
		NiPoint3 targetloc;
		if (pPlayer->IsActorFollowingPlayer(apActor)) {
			pPlayer->AddFollower(apActor);
		}
		if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
			apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			goto LABEL_9;
		}
		ptarg = 0;
		if (apActor->IsInCombat() && apActor->GetCombatController()) {
			ptarg = (TESObjectREFR*)apActor->GetCombatController()->GetTarget();
		} else {
			if (!pTarget) {
				SetTargetForPackage(apActor);
			}
			ptarg = pTarget;
		}
		if (!ptarg || ptarg->GetDisabled()) {
			if (abflag) {
				AddToProcedureIndexRunning(apActor, 1);
			}
			return;
		}
		if ((ptarg->GetDelete() && !(&ptarg->m_Extra)->GetReferencePointer()) || ptarg->GetDisabled()) {
			if (ptarg->GetDelete()) {
				v96->SetNeverRun(ptarg, 1);
			}
			if (abflag) {
				AddToProcedureIndexRunning(apActor, 1);
			}
			return;
		}
		if (ptarg->IsDead(1)) {
			if (v96->GetIsCreated()) {
				apActor->EndInterruptPackage(0);
				apActor->EvaluatePackage(0, 0);
			} else {
				if (v96->GetPackageTarget()->GetTargType() && v96->GetPackageTarget()->GetTargType() != PTT_LINKEDREF) {
					SetTarget(0);
					SetProcedureIndexRunning(0);
				} else {
					v96->SetNeverRun(pTarget, 1);
				}
			}
			return;
		}
		targetloc = ptarg->GetLocationOnReference();
		lastSeenPostion = targetloc;
		btargetSwimming = 0;
		ptargactor = 0;
		if (ptarg->IsActor()) {
			ptargactor = static_cast<Actor*>(ptarg);
		}
		if (ptargactor) {
			btargetSwimming = ptargactor->IsSwimming();
			if (btargetSwimming) {
				if (!apActor->DoesSwim() || !apActor->IsAllowedToSwim()) {
					goto LABEL_45;
				}
			} else {
				if (apActor->SwimsOnly()) {
				LABEL_45:;
					EndMoveMessage(apActor);
					return;
				}
			}
			if (ptargactor->GetCurrentProcessType() == PROCESS_TYPE_LOW &&
				(!ptargactor->IsDead(0) &&
					(!ptargactor->GetDisabled() &&
						(ptargactor != apActor &&
							(ptargactor != pPlayer &&
								((!ptargactor->Get3D() || !ptargactor->IsVisible(7)) &&
									(!apActor->Get3D() || !apActor->IsVisible(7)))))))) {
				TESWorldSpace* WorldSpace = ptargactor->GetWorldSpace();
				TESObjectCELL* ParentCell = ptargactor->GetParentCell();
				float a3 = (&ptargactor->GetAngleOnReference())->z;
				const NiPoint3* v5 = &ptargactor->GetLocationOnReference();
				apActor->MoveActorAndFollowers(*v5, a3, ParentCell, WorldSpace, 1, 1);
				return;
			}
		}
		bmustmove = 0;
		if (v96->GetIsCreated()) {
			TESPackage* pEditorPack;
			pEditorPack = GetCurrentPackage();
			if (pEditorPack) {
				if (pEditorPack->GetPCContinue() &&
					(pEditorPack->GetServicesEnabled() &&
						(apActor->GetParentCell() && apActor->GetParentCell()->IsActorCellOwner(apActor)))) {
					SetActionHeadTrackTarget(pPlayer);
					if (apActor->LineOfSight(0, ptarg, 1, 0, 0) || !GetDetectionState(static_cast<Actor*>(ptarg), 0)) {
						if (!apActor->IsPathingComplete()) {
							EndMoveMessage(apActor);
						}
						return;
					}
					if (apActor->IsMoving() || apActor->IsWaitingOnPath()) {
						if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
							apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
						LABEL_9:;
							apActor->InitiateGetUpPackage();
							return;
						}
					} else {
						bmustmove = 1;
					}
				}
			}
		}
		CombatController* pcombatController = apActor->GetCombatController();
		const NiPoint3* v6 = &apActor->GetLocationOnReference();
		float fdistFromTarget = (*v6 - targetloc).Length();
		if (apActor->pActorMover) {
			if (apActor->pActorMover->IsPathing()) {
				float fdistFromTarget_2;
				float fdistanceAlongPath = apActor->pActorMover->GetDistanceRemaining();
				if (std::isfinite(fdistanceAlongPath)) {
					fdistFromTarget_2 = BSMAX(fdistFromTarget, fdistanceAlongPath);
				} else {
					fdistFromTarget_2 = fdistFromTarget;
				}
				fdistFromTarget = fdistFromTarget_2;
			}
		}
		bool bTargetMoving = ptargactor && ptargactor->IsMoving();
		float fbaseRadius = 0.0f;
		if (ptarg == pPlayer || v96->GetPackageTarget()) {
			fbaseRadius = v96->GetPackageRadiusActorToRefTarget(apActor, 0);
		}
		if (fbaseRadius < 1.0) {
			fbaseRadius = static_cast<float>(iAIDistanceRadiusMinLocation.Int());
		}
		double FollowRadiusFudgeForMultipleFollowers =
			AiFormulas::GetFollowRadiusFudgeForMultipleFollowers(apActor, ptargactor);
		float v93 = FollowRadiusFudgeForMultipleFollowers + fbaseRadius;
		float fstopRadius = AiFormulas::GetFollowRadiusStop(v93);
		if (bTargetMoving) {
			ffollowRadius = fstopRadius;
		} else {
			ffollowRadius = AiFormulas::GetFollowRadiusSlow(v93);
		}
		float frecalcDist = ffollowRadius * 0.75;
		bool bpathingToBreadcrumbs = 0;
		if (!ptarg ||
			(v96->GetPackType() == PACKAGE_TYPE_FOLLOW &&
				(v96->pPackLoc &&
					((v96->pPackLoc->GetLocType() < PLT_CURRLOC || v96->pPackLoc->GetLocType() == PLT_LINKEDREF) &&
						v96->IsActorAtLocation(apActor, 0, -1.0f, 0))))) {
			if (abflag) {
				AddToProcedureIndexRunning(apActor, 1);
				if (v96->GetIsCreated()) {
					if (GetRunOncePackage()) {
						ClearRunOncePackage();
					} else {
						ClearCurrentPackage();
					}
					CheckforNewPackage(apActor, 0);
					return;
				}
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
			}
			goto LABEL_219;
		}
		fdistancemoved = (lastSeenPostion - targetloc).Length();
		if (pcombatController) {
			float fangle = pcombatController->GetCombatPosition();
			if (fangle > 0.0) {
				ptarg->GetDeltaForAngle(&delta, fangle);
				delta.operator*=(250.0f);
				targetloc.operator+=(delta);
			}
		}
		if (ptargactor) {
			if (btargetSwimming || ptargactor->IsInWater()) {
				double Height = ptargactor->GetHeight();
				targetloc.z = Height + targetloc.z;
				float a1_4 = ptargactor->GetRelevantWaterHeight();
				targetloc.z = BSMIN(targetloc.z, a1_4);
			}
		}
		v11 = &fAIMoveDistanceToRecalcFollowPath.Float();
		if (frecalcDist > static_cast<double>(*v11)) {
			frecalcDist = fAIMoveDistanceToRecalcFollowPath.Float();
		}
		if (pcombatController) {
			v42 = 0.0f;
		} else {
			v42 = 50.0f;
		}
		fradiusRestartMovement = AiFormulas::GetFollowRadiusStartMoving(v93);
		if (ptargactor) {
			if (ptargactor == pPlayer) {
				float fbreadcrumbGoalRadius = 100.0f;
				ExtraFollowerSwimBreadcrumbs* pextraCrumbs =
					(&apActor->m_Extra)->GetOrCreateExtraFollowerSwimBreadcrumbs();
				if (pextraCrumbs) {
					apActor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
					FNVD_CONSTRUCTED_LOCAL(PathingLocation, leaderLoc, &PathingLocation::Destruct, ptargactor);
					bool bvalid = leaderLoc.ResolveNavMeshInfo(0);
					if (!bvalid) {
						bvalid = leaderLoc.ResolveToClosestNavmeshAndTriangle();
					}
					if (bvalid) {
						uint32_t leaderNavMeshID;
						BREADCRUMB_STATE eleaderState = BREADCRUMB_STATE_INVALID;
						if (btargetSwimming || ptargactor->IsInWater()) {
							eleaderState = BREADCRUMB_STATE_IN_WATER;
						} else {
							bhkCharacterController* pcc = ptargactor->GetCharController();
							if (pcc) {
								if (pcc->GetState() == HK_CHARACTER_ON_GROUND) {
									eleaderState = BREADCRUMB_STATE_ON_GROUND;
								}
							}
						}
						const NavMeshInfo* pinfo = leaderLoc.GetBestNavMeshInfo();
						if (pinfo) {
							leaderNavMeshID = pinfo->NavMeshID;
						} else {
							leaderNavMeshID = 0;
						}
						NiPoint3 leaderPos = leaderLoc.GetLocation();
						if (pextraCrumbs->UpdateLeaderInfo(eleaderState, leaderPos, leaderNavMeshID)) {
							EndMoveMessage(apActor);
						}
						if (pextraCrumbs->GetFirstCrumb()) {
							bhkCharacterController* pcc_2;
							if (apActor->DoesFly() ||
								((eleaderState == BREADCRUMB_STATE_IN_WATER &&
									 (apActor->IsSwimming() || apActor->IsInWater())) ||
									(eleaderState == BREADCRUMB_STATE_ON_GROUND &&
										(apActor->GetCharController() &&
											(pcc_2 = apActor->GetCharController(),
												pcc_2->GetState() == HK_CHARACTER_ON_GROUND))))) {
								FNVD_CONSTRUCTED_LOCAL(PathingLocation, myLoc, &PathingLocation::Destruct, apActor);
								if (myLoc.ResolveNavMeshInfo(0) || myLoc.ResolveToClosestNavmeshAndTriangle()) {
									uint32_t myNavMeshID;
									const NavMeshInfo* pmyInfo = myLoc.GetBestNavMeshInfo();
									if (pmyInfo) {
										myNavMeshID = pmyInfo->NavMeshID;
									} else {
										myNavMeshID = 0;
									}
									if (myNavMeshID == leaderNavMeshID) {
										EndMoveMessage(apActor);
										bpathingToBreadcrumbs = 0;
										while (pextraCrumbs->GetFirstCrumb()) {
											pextraCrumbs->RemoveFirstCrumb();
										}
									}
								}
							}
						}
					}
					const ExtraFollowerSwimBreadcrumb* pcrumb = pextraCrumbs->GetFirstCrumb();
					if (pcrumb) {
						bpathingToBreadcrumbs = 1;
						if (apActor->IsPathingComplete()) {
							TESObjectCELL* Target;
							const NiPoint3* v16;
							const NiPoint3* v17;
							double v18;
							const ExtraFollowerSwimBreadcrumb* pcrumb_2;
							float v38;
							float v39;
							::new (&spmesh) NavMeshPtr;
							uint32_t a1_4a = pcrumb->StartNavMeshID;
							NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
							NavMeshInfo* pinfo_2 = NavMeshInfoMap_->GetNavMeshInfo(a1_4a);
							if (pinfo_2) {
								pinfo_2->GetNavMesh(spmesh);
							}
							// NavMesh::GetParentCell is the exe's shared one-field getter (0x59BB30), a trivial accessor.
							if ((spmesh.operator NavMesh*() &&
									(Target = spmesh.m_pObject->pParentCell, Target != apActor->GetParentCell())) ||
								((pcrumb_2 = pcrumb,
									 (v16 = &apActor->GetLocationOnReference(),
										 (v12 = (*v16 - pcrumb_2->StartLocation).SqrXYLength(),
											 (v39 = fbreadcrumbGoalRadius * fbreadcrumbGoalRadius, !(v39 > v12))))) ||
									(v17 = &apActor->GetLocationOnReference(),
										(v38 = v17->z - pcrumb->StartLocation.z,
											(v18 = std::fabsf(v38), !(fbreadcrumbGoalRadius > v18)))))) {
								FNVD_CONSTRUCTED_LOCAL(PathingRequest, prequest, &PathingRequest::Destruct);
								if (spmesh.operator NavMesh*()) {
									TESForm* ParentSpace = spmesh.m_pObject->GetParentSpace();
									apActor->BuildRequest_0(&prequest, pcrumb->StartLocation, ParentSpace,
										fbreadcrumbGoalRadius, 0);
								} else {
									TESWorldSpace* v26 = apActor->GetWorldSpace();
									TESObjectCELL* v24 = apActor->GetParentCell();
									apActor->BuildRequest(&prequest, pcrumb->StartLocation, v24, v26,
										fbreadcrumbGoalRadius, 0);
								}
								apActor->SetPathfindingGoal(&prequest);
								targetloc = pcrumb->StartLocation;
							} else {
								apActor->ForceStopMoving();
								apActor->pActorMover->MoveActorToLocation(pcrumb->EndLocation);
								if (!apActor->DoesFly()) {
									bhkCharacterController* pcon = GetCharController();
									if (pcon) {
										pcon->SetTrueState(pcrumb->bEnteringWater ? HK_CHARACTER_IN_AIR
																				  : HK_CHARACTER_ON_GROUND);
									}
								}
								pextraCrumbs->RemoveFirstCrumb();
								bpathingToBreadcrumbs = pextraCrumbs->GetFirstCrumb() != 0;
							}
							FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
						}
					}
				}
			}
		}
		if (!bpathingToBreadcrumbs &&
			(apActor->IsMoving() || fradiusRestartMovement < static_cast<double>(fdistFromTarget))) {
			float v37;
			if (bmustmove ||
				(frecalcDist < static_cast<double>(fdistancemoved) ||
					(((ptargactor && ptargactor->IsMoving()) ||
						 (v37 = ffollowRadius + v42, !apActor->IsAtPoint(targetloc, v37, 0, 0))) &&
						(apActor->IsPathingComplete() ||
							(!apActor->IsWaitingOnPath() && apActor->pActorMover->GetCurrentPercent() > 0.5))))) {
				if (abpursuit) {
					float* v20 = &fAIPursueDistanceLineOfSight.Float();
					if (!(*v20 < static_cast<double>(fdistFromTarget))) {
						fPursueTimer = 0.0f;
					} else {
						if (apActor->LineOfSight(0, ptarg, 1, 0, 0)) {
							fPursueTimer = 0.0f;
						} else {
							fPursueTimer = fPursueTimer + 1.0;
							if (fPursueTimer >= 30.0) {
								if (apActor->IsInCombat()) {
									Actor* ptargetActor = 0;
									if (ptarg) {
										if (ptarg->IsActor()) {
											ptargetActor = static_cast<Actor*>(ptarg);
										}
									}
									if (ptargetActor) {
										apActor->StopCombat(ptargetActor);
									}
								} else {
									AddToProcedureIndexRunning(apActor, 2);
								}
								fPursueTimer = 0.0f;
								return;
							}
						}
					}
				}
				TESWorldSpace* v21 = ptarg->GetWorldSpace();
				if (!apActor->SetPathfindingGoal(targetloc, ptarg->GetParentCell(), v21, ffollowRadius, 0)) {
					return;
				}
				lastSeenPostion = ptarg->GetLocationOnReference();
			}
		}
		if (!apActor->IsPathingComplete()) {
			bool considerAlwaysRun = 1;
			if (ptargactor) {
				considerAlwaysRun = (ptargactor->GetMoveMode() & ActorMover::MF_RUNNING) != 0;
			}
			if (aemovemode == -1) {
				int32_t emove = 0;
				bool bflag = 0;
				if (v96) {
					if (v96->GetPackType() == PACKAGE_TYPE_ALARM || v96->GetPackType() == PACKAGE_TYPE_COMBAT) {
						bflag = 1;
					}
				}
				bool bcalc = 1;
				if (v96->GetIsCreated()) {
					TESPackage* pEditorPack_2 = GetCurrentPackage();
					if (pEditorPack_2) {
						if (pEditorPack_2->GetPCContinue() && pEditorPack_2->GetServicesEnabled()) {
							if (apActor->GetParentCell()) {
								if (apActor->GetParentCell()->IsActorCellOwner(apActor)) {
									emove = 257;
									bcalc = 0;
								}
							}
						}
					}
				}
				if (bcalc) {
					float fLongDistSq = ffollowRadius + ffollowRadius;
					emove = CalculateMoveMode(apActor, fdistFromTarget, ffollowRadius, fLongDistSq, bflag, 0);
				}
				SetActorsAnimation(apActor, emove, considerAlwaysRun);
			} else {
				SetActorsAnimation(apActor, aemovemode, considerAlwaysRun);
			}
		}
	LABEL_219:;
		uint32_t emoveMode = apActor->pActorMover->GetPreferredMoveMode();
		emoveMode = apActor->ModifyMoveModeForFollow(static_cast<int32_t>(emoveMode), v93, fdistFromTarget);
		apActor->SetMoveMode(emoveMode);
		bool bDirectionalRun = (emoveMode & ActorMover::MF_RUNNING) != 0 &&
			(emoveMode &
				(ActorMover::MF_FORWARD | ActorMover::MF_BACKWARD | ActorMover::MF_LEFT | ActorMover::MF_RIGHT)) != 0;
		bool bIsCreature = apActor->GetActorType() == 0;
		bool bRun = bDirectionalRun || bIsCreature;
		bool v33 = !bDirectionalRun || bIsCreature;
		if (bRun) {
			float frunSpeed = apActor->CalculateRunSpeed();
			apActor->AdjustSpeedForFollowing(frunSpeed, v93, fdistFromTarget);
			apActor->SetRunSpeed(frunSpeed);
		}
		if (v33) {
			float fwalkSpeed = apActor->CalculateWalkSpeed();
			apActor->AdjustSpeedForFollowing(fwalkSpeed, v93, fdistFromTarget);
			apActor->SetWalkSpeed(fwalkSpeed);
		}
		if (!bpathingToBreadcrumbs && fstopRadius > static_cast<double>(fdistFromTarget) &&
			(ptarg != pPlayer || apActor->LineOfSight(0, ptarg, 1, 0, 0)) && !apActor->IsPathingComplete() &&
			!apActor->IsRotating()) {
			HighProcess* pProcess = 0;
			if (apActor->pCurrentProcess) {
				if (!apActor->pCurrentProcess->eLevel) {
					pProcess = static_cast<HighProcess*>(apActor->pCurrentProcess);
				}
			}
			TESTopic* pTopic = TESTopic::GetTopic(DT_MISCELLANEOUS, 2);
			if (apActor->IsContinuingPackagePCNear()) {
				if (pProcess) {
					apActor->SetDialogueItemTarget(pPlayer);
					if (pTopic) {
						pProcess->ProcessGreet(apActor, pTopic, 0, 0, 1, 0);
					} else {
						if (GetGreetingFlag()) {
							pProcess->ProcessGreet(apActor, 0, 0, 0, 1, 0);
						}
					}
				}
			}
			apActor->EndMovement();
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessFlee(Actor* apActor) {
	float fFleeDoneDist;
	float fFleeStartDist;
	FNVD_INLINED_LOCAL(PathingRequestFlee, fleeRequest_2, std::nullopt, std::nullopt);
	float FormFlags;
	FleePackage* ppackFlee = 0;
	TESPackage* pbasepack = GetPackageThatIsRunning();
	if (pbasepack) {
		if (pbasepack->GetPackType() == PACKAGE_TYPE_FLEE) {
			ppackFlee = static_cast<FleePackage*>(pbasepack);
		}
	}
	if (ppackFlee) {
		ppackFlee->UpdateAvoidedRefList(this);
	}
	TESObjectREFR* pRef = 0;
	bool bswimflee = 0;
	if (apActor->GetCombatController()) {
		bswimflee = apActor->IsSwimming();
	}
	if (apActor->IsOverEncumbered()) {
		ppackFlee->SetFleeSucceeded(0);
		float v36 = ppackFlee->fFleeTimer * -1.0;
		ppackFlee->ModFleeTimer(v36);
		return;
	}
	bool bcombatMode = ppackFlee->bCombatMode;
	if (!bcombatMode) {
		if (ppackFlee) {
			if (ppackFlee->GetPackageTarget()) {
				if (ppackFlee->GetPackageTarget()->GetTargReference()) {
					pRef = ppackFlee->GetPackageTarget()->GetTargReference();
				}
			}
		}
	}
	FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
	if (((!pRef || pRef->IsActor()) && (!bswimflee && !ppackFlee)) ||
		((ppackFlee->ShouldShutDown() && !apActor->GetShouldAttackActor(pPlayer, 0, ereaction, 0)) ||
			(pRef && (!pRef->IsActor() && (pRef->GetDestroyed() || ppackFlee->FleeSucceeded()))))) {
		AddToProcedureIndexRunning(apActor, 1);
		return;
	}
	MobileObject* CriticalQueuedRefCount = static_cast<MobileObject*>(ppackFlee->pTeleportDoor);
	TESObjectCELL* pcell = apActor->GetParentCell();
	TESWorldSpace* pworld = apActor->GetWorldSpace();
	float FadeDuration = ppackFlee->fFleeTimer;
	TESObjectREFR* pavoidRef = ppackFlee->GetClosestAvoidedRef(apActor);
	TESObjectREFR* pfleeToRef = ppackFlee->GetRefFleeTo();
	bool bknows = ppackFlee->bknowstarget;
	bool bdoor = 0;
	int32_t emove = 513;
	if (!bcombatMode && !pavoidRef) {
		pavoidRef = pRef;
	}
	if (pcell && pcell->GetInterior()) {
		fFleeStartDist = fFleeDistanceInterior.Float();
	} else {
		fFleeStartDist = fFleeDistanceExterior.Float();
	}
	if (pcell && pcell->GetInterior()) {
		fFleeDoneDist = fFleeDoneDistanceInterior.Float();
	} else {
		fFleeDoneDist = fFleeDoneDistanceExterior.Float();
	}
	if (ppackFlee->GetPackageTarget()) {
		if (static_cast<double>(ppackFlee->GetPackageTarget()->iValue) > 0.0) {
			FormFlags = static_cast<float>(ppackFlee->GetPackageTarget()->iValue);
		}
	}
	if ((pfleeToRef || !pavoidRef) && (ppackFlee->GetMustComplete() || ppackFlee->GetMustReachLocation())) {
		bool v32 = true;
		if (pfleeToRef) {
			double DistanceFromReference = apActor->GetDistanceFromReference(pfleeToRef, 0, 0);
			if (!(static_cast<double>(ppackFlee->GetLocationRadius(apActor)) > DistanceFromReference)) {
				v32 = false;
			}
		}
		if (v32) {
			if (ppackFlee->GetMustReachLocation()) {
				fPackageEvalTimer = 0.0f;
			}
			apActor->StopMoving();
			ppackFlee->SetFleeSucceeded(1);
			EndMoveMessage(apActor);
			Animation* panim = apActor->GetAnimation();
			if (panim) {
				if (panim->SpecialIdleDonePlaying()) {
					SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
				}
			}
			if (!(fIdleChatterTimer <= 0.0) ||
				(pPlayer->IsImportantConversationRunning() || pPlayer->GetPlayerGreetFlag())) {
				fIdleChatterTimer = fIdleChatterTimer - appTimer.fDelta;
			} else {
				BSRandom* Singleton = BSRandom::GetSingleton();
				int32_t ichance = Singleton->UnsignedInt(0, 100u);
				if (ichance <= iChanceFleeDialog.Int()) {
					TESTopic* ptopic = TESTopic::GetTopic(DT_COMBAT, 3);
					if (ptopic) {
						ProcessGreet(apActor, ptopic, 0, 0, 1, 0);
					}
				}
				float a2 = fIdleChatterCommentTimerMax.Float();
				float* v3 = &fIdleChatterCommentTimer.Float();
				fIdleChatterTimer = CombatFormulas::GetRandomBetween(*v3, a2);
			}
			float a2a = appTimer.fDelta;
			ppackFlee->ModFleeTimer(a2a);
			if (ppackFlee->fFleeTimer >= 5.0) {
				AddToProcedureIndexRunning(apActor, 1);
			}
			return;
		}
		fIdleChatterTimer = 0.0f;
	}
	if (apActor->DidPathingFail()) {
		apActor->SetMovementComplete(1);
		ppackFlee->pRefFleeTo = 0;
		ppackFlee->pTeleportDoor = 0;
		FNVD_CONSTRUCTED_LOCAL(PathingRequestFlee, fleeRequest, &PathingRequestFlee::Destruct);
		fleeRequest.Initialize(apActor);
		if (apActor->IsCreature()) {
			TESActorBaseData* v31 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
			fleeRequest.SetCantOpenDoors(v31->GetCantOpenDoors());
		}
		float a2b = static_cast<float>(ppackFlee->GetLocationRadius(apActor));
		fleeRequest.SetTargetRadius(a2b);
		PathingAvoidNodeArray* AvoidNodeArray = ppackFlee->CreateAvoidNodeArray();
		fleeRequest.SetAvoidNodeArray(AvoidNodeArray);
		fleeRequest.SetMinFleeDistance(FormFlags);
		float a2c = apActor->GetHeading(0);
		fleeRequest.SetInitialPathHeading(a2c);
		if (ppackFlee->GetRefFleeTo()) {
			FNVD_CONSTRUCTED_LOCAL(PathingLocation, kDestination, &PathingLocation::Destruct,
				ppackFlee->GetRefFleeTo());
			fleeRequest.SetDestination(kDestination);
		} else {
			fleeRequest.SetNoDestination();
		}
		apActor->SetPathfindingHide(&fleeRequest);
	} else {
		if (pcell) {
			Actor* v29;
			float fDistance = 3.4028235e+38f;
			if (pavoidRef) {
				if (!pavoidRef->IsDead(0) && !pavoidRef->GetDisabled()) {
					fDistance = apActor->GetDistanceFromReference(pavoidRef, 0, 0);
				}
			}
			TESObjectREFR* prefFleeTo = ppackFlee->GetRefFleeTo();
			if (prefFleeTo && prefFleeTo->IsActor()) {
				v29 = static_cast<Actor*>(prefFleeTo);
			} else {
				v29 = 0;
			}
			if ((prefFleeTo && (prefFleeTo->IsDead(0) || prefFleeTo->GetDisabled())) ||
				(v29 && (v29->IsInCombat() || !v29->CanMove()))) {
				prefFleeTo = 0;
				ppackFlee->pRefFleeTo = 0;
			}
			CriticalQueuedRefCount = static_cast<MobileObject*>(ppackFlee->pTeleportDoor);
			if ((!(fFleeStartDist > static_cast<double>(fDistance)) || CriticalQueuedRefCount) &&
				(bknows || (FadeDuration != 0.0 || !ppackFlee->bEvaluatePoint))) {
				if (ppackFlee->bEvaluatePoint) {
					if (prefFleeTo) {
						pcell = prefFleeTo->GetParentCell();
						pworld = prefFleeTo->GetWorldSpace();
					}
				} else {
					PackageLocation* plocation = ppackFlee->pPackLoc;
					if (plocation->GetLocType() == PLT_CELL) {
						pcell = plocation->GetLocCell();
					} else {
						if (plocation->GetLocType() == PLT_REFERENCE || plocation->GetLocType() == PLT_LINKEDREF) {
							TESObjectREFR* pref = GetGenericLocation();
							if (pref) {
								pcell = pref->GetParentCell();
								pworld = pref->GetWorldSpace();
							}
						}
					}
				}
			} else {
				if (!(fEssentialDownTimer <= 0.0)) {
					fEssentialDownTimer = fEssentialDownTimer - appTimer.fDelta;
				} else {
					bdoor = 1;
					fEssentialDownTimer = 20.0f;
				}
				if (ppackFlee->bEvaluatePoint) {
					TESObjectREFR* pfleeDest = ppackFlee->GetRefFleeTo();
					if (pfleeDest == pPlayer) {
						pfleeDest = 0;
					}
					if (pfleeDest) {
						pcell = pfleeDest->GetParentCell();
						pworld = pfleeDest->GetWorldSpace();
					} else {
						pcell = 0;
						pworld = 0;
						if (bdoor) {
							ppackFlee->FindTeleportDoor(apActor, pavoidRef);
						} else {
							ppackFlee->pTeleportDoor = 0;
						}
					}
				}
				if (apActor->IsPathingComplete() ||
					(apActor->IsPathValid() &&
						(apActor->pActorMover->GetDistanceTraveled() > 128.0 &&
							apActor->pActorMover->GetDistanceRemaining() < 128.0))) {
					::new (&fleeRequest_2) PathingRequestFlee;
					fleeRequest_2.Initialize(apActor);
					float a2d = apActor->GetHeading(0);
					fleeRequest_2.SetInitialPathHeading(a2d);
					if (apActor->IsCreature()) {
						TESActorBaseData* v28 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
						fleeRequest_2.SetCantOpenDoors(v28->GetCantOpenDoors());
					}
					float a2e = static_cast<float>(ppackFlee->GetLocationRadius(apActor));
					fleeRequest_2.SetTargetRadius(a2e);
					fleeRequest_2.SetMinFleeDistance(FormFlags);
					PathingAvoidNodeArray* v5 = ppackFlee->CreateAvoidNodeArray();
					fleeRequest_2.SetAvoidNodeArray(v5);
					float a2f = apActor->GetHeading(0);
					fleeRequest_2.SetInitialPathHeading(a2f);
					if (ppackFlee->GetRefFleeTo()) {
						FNVD_CONSTRUCTED_LOCAL(PathingLocation, kDestination, &PathingLocation::Destruct,
							ppackFlee->GetRefFleeTo());
						fleeRequest_2.SetDestination(kDestination);
					} else {
						fleeRequest_2.SetNoDestination();
					}
					if (!apActor->SetPathfindingHide(&fleeRequest_2)) {
						fleeRequest_2.~PathingRequestFlee();
						return;
					}
					fleeRequest_2.~PathingRequestFlee();
				}
				ppackFlee->SetFleeSucceeded(0);
				float v26 = ppackFlee->fFleeTimer * -1.0;
				ppackFlee->ModFleeTimer(v26);
			}
		}
		TESObjectREFR* pteleportDoor = ppackFlee->pTeleportDoor;
		if (apActor->IsPathingComplete() ||
			(!pteleportDoor &&
				(apActor->IsPathValid() &&
					(apActor->pActorMover->GetDistanceTraveled() > 128.0 &&
						apActor->pActorMover->GetDistanceRemaining() < 128.0)))) {
			TESObjectREFR* pfleeDest_2 = ppackFlee->GetRefFleeTo();
			if (pfleeDest_2 == pPlayer) {
				pfleeDest_2 = 0;
				ppackFlee->pRefFleeTo = 0;
			}
			if (pteleportDoor) {
				float fdistToTeleportMarker = 10000.0f;
				DoorTeleportData* pteleportData = pteleportDoor->GetTeleport();
				if (pteleportData) {
					const NiPoint3* a2g = &apActor->GetLocationOnReference();
					DoorTeleportData* Teleport = pteleportData->pLinkedDoor->GetTeleport();
					fdistToTeleportMarker = (*&Teleport->position - *a2g).Length();
				}
				float a2h = apActor->GetDistanceFromReference(pteleportDoor, 0, 0);
				float fsmallestDist = BSMIN(fdistToTeleportMarker, a2h);
				if (!(fsmallestDist < 160.0)) {
					if (pteleportData) {
						FNVD_CONSTRUCTED_LOCAL(PathingRequestFlee, kRequest, &PathingRequestFlee::Destruct);
						kRequest.Initialize(apActor);
						float a2i = apActor->GetHeading(0);
						kRequest.SetInitialPathHeading(a2i);
						if (apActor->IsCreature()) {
							TESActorBaseData* v25 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
							kRequest.SetCantOpenDoors(v25->GetCantOpenDoors());
						}
						float a2j = static_cast<float>(ppackFlee->GetLocationRadius(apActor));
						kRequest.SetTargetRadius(a2j);
						kRequest.SetMinFleeDistance(FormFlags);
						PathingAvoidNodeArray* v8 = ppackFlee->CreateAvoidNodeArray();
						kRequest.SetAvoidNodeArray(v8);
						DoorTeleportData* v9 = pteleportData->pLinkedDoor->GetTeleport();
						{
							FNVD_CONSTRUCTED_LOCAL(PathingLocation, kDestination, &PathingLocation::Destruct,
								v9->position, pteleportDoor);
							kRequest.SetDestination(kDestination);
						}
						float a2k = apActor->GetHeading(0);
						kRequest.SetInitialPathHeading(a2k);
						if (apActor->SetPathfindingHide(&kRequest)) {
						} else {
						}
					}
				} else {
					FreeUpSpecialIdle(apActor);
					ppackFlee->pTeleportDoor = 0;
					pteleportDoor->Activate(apActor, 0, 0, 1);
					fEssentialDownTimer = 20.0f;
				}
			} else {
				if (pfleeDest_2 && apActor->GetDistanceFromReference(pfleeDest_2, 0, 0) > 160.0) {
					const NiPoint3* pactorPoint = &pfleeDest_2->GetLocationOnReference();
					pcell = pfleeDest_2->GetParentCell();
					pworld = pfleeDest_2->GetWorldSpace();
					if (!apActor->SetPathfindingGoal(*pactorPoint, pcell, pworld, 0.0f, 0)) {
						ppackFlee->pRefFleeTo = 0;
					}
				} else {
					Animation* panim_2 = apActor->GetAnimation();
					if (panim_2) {
						if (panim_2->SpecialIdleDonePlaying()) {
							SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
						}
					}
					ppackFlee->SetFleeSucceeded(1);
					float a2l = appTimer.fDelta;
					ppackFlee->ModFleeTimer(a2l);
				}
			}
		} else {
			Animation* panim_3 = apActor->GetAnimation();
			if (panim_3) {
				if (!panim_3->SpecialIdleDonePlaying()) {
					FreeUpSpecialIdle(apActor);
				}
			}
			SetActorsAnimation(apActor, emove, 1);
		}
	}
}

// Runs apActor's greeting: says aptopic to the dialogue item's target (or the last dialogue target) one response at a
// time, starting each response's voice sound, idles and head tracking, and once the last response has been said
// clears the greeting and the head tracking. A greeting waits while the lip file loads.
// The exe's cleanup-only EH frame for the voice sound handles is left out: the handles' destructor is called where the
// exe calls it.
void HighProcess::ProcessGreet(Actor* apActor, TESTopic* apGreetTopic, bool abForceSub, bool abStop, bool abQue,
	bool abSayCallBack) {
	bool bNextResponse = false;
	bStop = abStop;
	bool bUseIdleManager = true;
	bool bInCombat = false;
	if (apActor->IsInCombat())
		bInCombat = true;
	if (abSayCallBack)
		bIsDoingSayTo = true;
	TESPackage* pCurrentPackage = GetCurrentPackage();
	if (pCurrentPackage && pCurrentPackage->pIdleCollection)
		bUseIdleManager = false;
	MobileObject* pGreetTarget = apActor->GetDialogueItemTarget();
	if (GetRunOncePackage() && GetRunOncePackage()->GetPackType() == PACKAGE_TYPE_GET_UP)
		bUseIdleManager = false;
	if (abSayCallBack)
		bProcessGreetSayTo = true;
	if (!pGreetTarget)
		pGreetTarget = pDialogTarget;
	if (pGreetTarget)
		pPlayer->SetPlayerGreetFlag();
	if (bWaitingForLipFile)
		return;

	if (apGreetTopic || (!bWaitingForLipFile && pLipSynicAnim) || bLipFileFailed) {
		if (pGreetTopic && (!abQue || pLipSynicAnim || bLipFileFailed)) {
			apActor->StopCurrentDialogue();
			if (abQue) {
				m_pSayToDialogueTopic = pGreetTopic;
			} else {
				if (pGreetTopic) {
					// `delete pGreetTopic`: the exe calls DialogueItem's scalar deleting destructor out of line.
					DialogueItem* pOldTopic = pGreetTopic;
					if (pOldTopic)
						delete pOldTopic;
					pGreetTopic = nullptr;
					pSubtitleVoice = nullptr;
				}
				m_pSayToDialogueTopic = apGreetTopic->CreateDialogueItem(apActor, pGreetTarget, nullptr, nullptr, true);
				if (m_pSayToDialogueTopic) {
					m_pSayToDialogueTopic->FirstResponse();
					pGreetTopic = m_pSayToDialogueTopic;
					pSubtitleVoice = m_pSayToDialogueTopic;
				} else {
					// IDA's kEvent_SayToDone; src has no name for action flag 0x40000.
					if (abSayCallBack)
						Script::SetActionFlag(apGreetTopic, apActor->GetExtra(), 0x40000);
					TESPackage* pPackage = GetCurrentPackage();
					if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_DIALOGUE &&
						pPackage->GetDialogueSayToFlag()) {
						SetCurrentActionComplete(true);
						AddToProcedureIndexRunning(apActor, 3);
					}
					fGreetingTimer = fAIGreetingTimer.Float();
					SetGreetingFlag(false);
				}
			}
			if (pGreetTopic)
				pGreetTopic->RunResult(TIRS_BEGIN);
			if (m_pSayToDialogueTopic) {
				if (pGreetTarget && !pGreetTarget->GetDisabled())
					pGreetTarget->GetCurrentAIProcess()->SetLastGreeted(apActor);
				DialogueResponse* pResponse = m_pSayToDialogueTopic->GetCurrentResponse();
				if (pResponse) {
					bool bSpeak = true;
					if (pResponse->GetVoiceSound() && apActor->bSoundFileDone) {
						uint32_t uiSoundFormID = pResponse->GetVoiceSound()->GetFormID();
						// PlaySound constructs the returned handle in place: untyped stack memory until then. This
						// path keeps the handle local (the voice handle is not assigned). The callbacks' context is
						// the actor's form ID, which the exe passes as the untyped context pointer.
						alignas(BSSoundHandle) uint8_t aucVoiceSound[sizeof(BSSoundHandle)];
						BSSoundHandle* pVoiceSound = reinterpret_cast<BSSoundHandle*>(aucVoiceSound);
						apActor->PlaySound(pVoiceSound, uiSoundFormID, false,
							BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
								BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
							true);
						// The exe fetches the audio manager and leaves it unused.
						BSAudioManager::QInstance();
						if (abSayCallBack) {
							apActor->SetSayToTopic(apGreetTopic);
							apActor->SetSayToTopicInfo(m_pSayToDialogueTopic->GetInfo());
							apActor->SetSayToResponseID(pResponse->GetResponseID());
							SoundHandle[0].SetCompletionCallback(&MobileObject::SayToCallBack,
								reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
							bProcessGreetSayTo = false;
						} else {
							SoundHandle[0].SetCompletionCallback(&MobileObject::SoundFileCallBack,
								reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
						}
						apActor->bSoundFileDone = false;
						bSpeak = false;
						Script::SetActionFlag(apGreetTopic, apActor->GetExtra(), 0x40000);
						apActor->SetSayToTopic(nullptr);
						apActor->GetExtra()->RemoveSayToInfoExtra();
						SetDoingSayTo(false);
						pVoiceSound->~BSSoundHandle();
					}
					apActor->SetShouldUseEmotionAnimation(pResponse->GetUseEmotion());
					if (bSpeak && apActor->bSoundFileDone) {
						// The exe evaluates the arguments right to left.
						TESIdleForm* pListenerIdle = pResponse->GetListenerIdle();
						TESIdleForm* pSpeakerIdle = pResponse->GetSpeakerIdle();
						int32_t iLength = pResponse->GetText()->GetLength();
						uint32_t uiEmotionValue = pResponse->GetEmotionValue();
						DIALOGUE_EMOTION eEmotion = pResponse->GetEmotion();
						const char* pVoiceFile = pResponse->GetVoice()->String();
						fSoundDelay =
							apActor->SpeakSoundFunction(pVoiceFile, SoundHandle[0], eEmotion, uiEmotionValue, iLength,
								pSpeakerIdle, pListenerIdle, pGreetTarget, true, false, false, true, bUseIdleManager);
						if (bProcessGreetSayTo) {
							// The exe reads the voice handle's sound ID and leaves it unused.
							SoundHandle[0].GetID();
							SoundHandle[0].SetCompletionCallback(&MobileObject::SayToCallBack,
								reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
							apActor->SetSayToTopic(m_pSayToDialogueTopic->GetTopic());
							apActor->SetSayToTopicInfo(m_pSayToDialogueTopic->GetInfo());
							apActor->SetSayToResponseID(pResponse->GetResponseID());
							bProcessGreetSayTo = false;
							apActor->bVoiceFileDone = false;
						}
					}
					if (bUseIdleManager)
						SetupSpecialIdle(apActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, true, false, true);
					if (fSoundDelay <= 0.0) {
						bNextResponse = true;
					} else {
						if (bStop && !bInCombat)
							EndMoveMessage(apActor);
						bGreetingFlag = 1;
					}
					TESPackage* pRunning = GetPackageThatIsRunning();
					if (!bInCombat && pRunning &&
						Procedure_Action[pRunning->GetProcedureType()][GetProcedureIndexRunning()] == PROCEDURE_WANDER)
						apActor->EndMovement();
					SetActionHeadTrackTarget(pGreetTarget);
					if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && pGreetTarget &&
						apActor->ShouldRotateToTrack(pGreetTarget))
						SetForceRotate(true);
					float fSeconds = appTimer.GetSeconds();
					fSoundDelay = fSoundDelay - fSeconds;
					if (pGreetTarget)
						fGreetingTimer = fAIGreetingTimer.Float();
				}
			} else if (abSayCallBack) {
				Script::SetActionFlag(apGreetTopic, apActor->GetExtra(), 0x40000);
				apActor->SetSayToTopic(nullptr);
				apActor->GetExtra()->RemoveSayToInfoExtra();
				SetDoingSayTo(false);
			}
		} else {
			m_pSayToDialogueTopic = apGreetTopic->CreateDialogueItem(apActor, pGreetTarget, nullptr, nullptr, true);
			if (pGreetTopic) {
				// `delete pGreetTopic`: the exe calls DialogueItem's scalar deleting destructor out of line.
				DialogueItem* pOldTopic = pGreetTopic;
				if (pOldTopic)
					delete pOldTopic;
				pSubtitleVoice = nullptr;
				pGreetTopic = nullptr;
			}
			pGreetTopic = m_pSayToDialogueTopic;
			pSubtitleVoice = m_pSayToDialogueTopic;
			// The exe stores the mobile object as the dialogue target unchecked.
			pDialogTarget = static_cast<Actor*>(pGreetTarget);
			if (m_pSayToDialogueTopic) {
				if (pGreetTarget && !pGreetTarget->GetDisabled())
					pGreetTarget->GetCurrentAIProcess()->SetLastGreeted(apActor);
				m_pSayToDialogueTopic->FirstResponse();
				DialogueResponse* pResponse = m_pSayToDialogueTopic->GetCurrentResponse();
				if (pResponse) {
					bool bSpeak = true;
					if (pResponse->GetVoiceSound() && apActor->bSoundFileDone) {
						if (apActor->GetAnimation()) {
							TESIdleForm* pSpeakerIdle = pResponse->GetSpeakerIdle();
							if (pSpeakerIdle)
								SetupSpecialIdle(apActor, pSpeakerIdle,
									static_cast<AnimIdle::PLAY_TYPE_ENUM>((pSpeakerIdle != nullptr) + 2), false, true,
									true);
						}
						if (pGreetTarget && pGreetTarget != apActor &&
							(pGreetTarget->IsMobileObject() || pGreetTarget->IsActor())) {
							MobileObject* pListener = pGreetTarget;
							TESIdleForm* pListenerIdle = pResponse->GetListenerIdle();
							if (pListenerIdle) {
								pListener->GetCurrentAIProcess()->SetupSpecialIdle(pListener, pListenerIdle,
									static_cast<AnimIdle::PLAY_TYPE_ENUM>((pListenerIdle != nullptr) + 2), false, true,
									true);
							}
						}
						uint32_t uiSoundFormID = pResponse->GetVoiceSound()->GetFormID();
						// PlaySound constructs the returned handle in place: untyped stack memory until then. The
						// callbacks' context is the actor's form ID, which the exe passes as the untyped context
						// pointer.
						alignas(BSSoundHandle) uint8_t aucVoiceSound[sizeof(BSSoundHandle)];
						BSSoundHandle* pVoiceSound = reinterpret_cast<BSSoundHandle*>(aucVoiceSound);
						SoundHandle[0] = *apActor->PlaySound(pVoiceSound, uiSoundFormID, false,
							BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
								BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
							true);
						pVoiceSound->~BSSoundHandle();
						if (abSayCallBack) {
							apActor->SetSayToTopic(apGreetTopic);
							apActor->SetSayToTopicInfo(m_pSayToDialogueTopic->GetInfo());
							apActor->SetSayToResponseID(1);
							SoundHandle[0].SetCompletionCallback(&MobileObject::SayToCallBack,
								reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
							bProcessGreetSayTo = false;
						} else {
							SoundHandle[0].SetCompletionCallback(&MobileObject::SoundFileCallBack,
								reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
						}
						apActor->bSoundFileDone = false;
						bSpeak = false;
					}
					if (bStop && !bInCombat)
						EndMoveMessage(apActor);
					if (bSpeak && apActor->bSoundFileDone) {
						apActor->SetShouldUseEmotionAnimation(pResponse->GetUseEmotion());
						// The exe evaluates the arguments right to left.
						TESIdleForm* pListenerIdle = pResponse->GetListenerIdle();
						TESIdleForm* pSpeakerIdle = pResponse->GetSpeakerIdle();
						int32_t iLength = pResponse->GetText()->GetLength();
						uint32_t uiEmotionValue = pResponse->GetEmotionValue();
						DIALOGUE_EMOTION eEmotion = pResponse->GetEmotion();
						const char* pVoiceFile = pResponse->GetVoice()->String();
						fSoundDelay =
							apActor->SpeakSoundFunction(pVoiceFile, SoundHandle[0], eEmotion, uiEmotionValue, iLength,
								pSpeakerIdle, pListenerIdle, pGreetTarget, true, false, true, true, bUseIdleManager);
						bGreetingFlag = 1;
						if (bProcessGreetSayTo && !bWaitingForLipFile) {
							SoundHandle[0].SetCompletionCallback(&MobileObject::SayToCallBack,
								reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
							apActor->SetSayToTopic(apGreetTopic);
							apActor->SetSayToTopicInfo(m_pSayToDialogueTopic->GetInfo());
							apActor->SetSayToResponseID(1);
							bProcessGreetSayTo = false;
							apActor->bVoiceFileDone = false;
						}
					}
					if (pGreetTopic && !bWaitingForLipFile)
						pGreetTopic->RunResult(TIRS_BEGIN);
				}
				if (pGreetTarget)
					fGreetingTimer = fAIGreetingTimer.Float();
				SetActionHeadTrackTarget(pGreetTarget);
				if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && pGreetTarget &&
					apActor->ShouldRotateToTrack(pGreetTarget))
					SetForceRotate(true);
			} else {
				// IDA's kEvent_SayToDone; src has no name for action flag 0x40000.
				if (abSayCallBack)
					Script::SetActionFlag(apGreetTopic, apActor->GetExtra(), 0x40000);
				TESPackage* pPackage = GetCurrentPackage();
				if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_DIALOGUE && pPackage->GetDialogueSayToFlag()) {
					SetCurrentActionComplete(true);
					AddToProcedureIndexRunning(apActor, 3);
				}
				fGreetingTimer = fAIGreetingTimer.Float();
				SetGreetingFlag(false);
			}
		}
	} else {
		// A response is being said: count its delay down; it is done once the delay has run out.
		if (fSoundDelay > 0.0) {
			float fSeconds = appTimer.GetSeconds();
			fSoundDelay = fSoundDelay - fSeconds;
		} else {
			bNextResponse = true;
		}
		if (pGreetTarget) {
			TESPackage* pRunning = GetPackageThatIsRunning();
			if (!bInCombat && pRunning &&
				Procedure_Action[pRunning->GetProcedureType()][GetProcedureIndexRunning()] == PROCEDURE_WANDER)
				apActor->EndMovement();
			bool bFaceTarget = true;
			if (!GetForceRotate()) {
				if (!abStop)
					bFaceTarget = false;
				else if (!apActor->ShouldRotateToTrack(nullptr))
					bFaceTarget = false;
				else if (bInCombat)
					bFaceTarget = false;
				else if (apActor->GetSitSleepState())
					bFaceTarget = false;
				else if (apActor->IsTurning())
					bFaceTarget = false;
				else if (apActor->IsPathing())
					bFaceTarget = false;
				else if (pRunning) {
					if (pRunning->GetPackType() == PACKAGE_TYPE_TRAVEL)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_ESCORT)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_FOLLOW)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_ACCOMPANY)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_PATROL)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_USE_WEAPON)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_EAT)
						bFaceTarget = false;
					else if (pRunning->GetPackType() == PACKAGE_TYPE_SANDBOX)
						bFaceTarget = false;
				}
			}
			if (bFaceTarget && apActor->ShouldRotateToTrack(nullptr)) {
				SetActionHeadTrackTarget(pGreetTarget);
				apActor->RequestRotate(pGreetTarget->GetLocationOnReference(), false);
			}
		}
	}

	if (bNextResponse && !bWaitingForLipFile && !SoundHandle[0].IsPlaying()) {
		if (m_pSayToDialogueTopic && m_pSayToDialogueTopic->NextResponse()) {
			DialogueResponse* pResponse = m_pSayToDialogueTopic->GetCurrentResponse();
			if (pResponse) {
				pGreetTopic = m_pSayToDialogueTopic;
				pSubtitleVoice = m_pSayToDialogueTopic;
				// The exe stores the mobile object as the dialogue target unchecked.
				pDialogTarget = static_cast<Actor*>(pGreetTarget);
				bProcessGreetSayTo = true;
				// The exe reuses the topic parameter for the item's own topic.
				apGreetTopic = m_pSayToDialogueTopic->GetTopic();
				if (m_pSayToDialogueTopic) {
					if (pGreetTarget && !pGreetTarget->GetDisabled())
						pGreetTarget->GetCurrentAIProcess()->SetLastGreeted(apActor);
					if (pResponse) {
						bool bSpeak = true;
						if (pResponse->GetVoiceSound() && apActor->bSoundFileDone) {
							if (apActor->GetAnimation()) {
								TESIdleForm* pSpeakerIdle = pResponse->GetSpeakerIdle();
								if (pSpeakerIdle) {
									SetupSpecialIdle(apActor, pSpeakerIdle,
										static_cast<AnimIdle::PLAY_TYPE_ENUM>((pSpeakerIdle != nullptr) + 2), false,
										true, true);
								}
							}
							if (pGreetTarget && pGreetTarget != apActor &&
								(pGreetTarget->IsMobileObject() || pGreetTarget->IsActor())) {
								MobileObject* pListener = pGreetTarget;
								TESIdleForm* pListenerIdle = pResponse->GetListenerIdle();
								if (pListenerIdle) {
									pListener->GetCurrentAIProcess()->SetupSpecialIdle(pListener, pListenerIdle,
										static_cast<AnimIdle::PLAY_TYPE_ENUM>((pListenerIdle != nullptr) + 2), false,
										true, true);
								}
							}
							uint32_t uiSoundFormID = pResponse->GetVoiceSound()->GetFormID();
							// PlaySound constructs the returned handle in place: untyped stack memory until then.
							// The callbacks' context is the actor's form ID, which the exe passes as the untyped
							// context pointer.
							alignas(BSSoundHandle) uint8_t aucVoiceSound[sizeof(BSSoundHandle)];
							BSSoundHandle* pVoiceSound = reinterpret_cast<BSSoundHandle*>(aucVoiceSound);
							SoundHandle[0] = *apActor->PlaySound(pVoiceSound, uiSoundFormID, false,
								BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
									BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
								true);
							pVoiceSound->~BSSoundHandle();
							if (abSayCallBack) {
								apActor->SetSayToTopic(apGreetTopic);
								apActor->SetSayToTopicInfo(m_pSayToDialogueTopic->GetInfo());
								apActor->SetSayToResponseID(apActor->GetSayToResponseID() + 1);
								SoundHandle[0].SetCompletionCallback(&MobileObject::SayToCallBack,
									reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
								bProcessGreetSayTo = false;
							} else {
								SoundHandle[0].SetCompletionCallback(&MobileObject::SoundFileCallBack,
									reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
							}
							apActor->bSoundFileDone = false;
							bSpeak = false;
						}
						if (bStop && !bInCombat)
							EndMoveMessage(apActor);
						if (bSpeak && apActor->bSoundFileDone) {
							apActor->SetShouldUseEmotionAnimation(pResponse->GetUseEmotion());
							// The exe evaluates the arguments right to left.
							TESIdleForm* pListenerIdle = pResponse->GetListenerIdle();
							TESIdleForm* pSpeakerIdle = pResponse->GetSpeakerIdle();
							int32_t iLength = pResponse->GetText()->GetLength();
							uint32_t uiEmotionValue = pResponse->GetEmotionValue();
							DIALOGUE_EMOTION eEmotion = pResponse->GetEmotion();
							const char* pVoiceFile = pResponse->GetVoice()->String();
							fSoundDelay = apActor->SpeakSoundFunction(pVoiceFile, SoundHandle[0], eEmotion,
								uiEmotionValue, iLength, pSpeakerIdle, pListenerIdle, pGreetTarget, true, false, true,
								true, bUseIdleManager);
							bGreetingFlag = 1;
							if (bProcessGreetSayTo && !bWaitingForLipFile) {
								SoundHandle[0].SetCompletionCallback(&MobileObject::SayToCallBack,
									reinterpret_cast<void*>(static_cast<uintptr_t>(apActor->GetFormID())));
								apActor->SetSayToTopic(apGreetTopic);
								apActor->SetSayToTopicInfo(m_pSayToDialogueTopic->GetInfo());
								apActor->SetSayToResponseID(apActor->GetSayToResponseID() + 1);
								bProcessGreetSayTo = false;
								apActor->bVoiceFileDone = false;
							}
						}
						if (pGreetTopic && !bWaitingForLipFile)
							pGreetTopic->RunResult(TIRS_BEGIN);
					}
					if (pGreetTarget)
						fGreetingTimer = fAIGreetingTimer.Float();
					SetActionHeadTrackTarget(pGreetTarget);
					if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && pGreetTarget &&
						apActor->ShouldRotateToTrack(pGreetTarget))
						SetForceRotate(true);
				} else {
					// IDA's kEvent_SayToDone; src has no name for action flag 0x40000.
					if (abSayCallBack)
						Script::SetActionFlag(apGreetTopic, apActor->GetExtra(), 0x40000);
					TESPackage* pPackage = GetCurrentPackage();
					if (pPackage && pPackage->GetPackType() == PACKAGE_TYPE_DIALOGUE &&
						pPackage->GetDialogueSayToFlag()) {
						SetCurrentActionComplete(true);
						AddToProcedureIndexRunning(apActor, 3);
					}
					fGreetingTimer = fAIGreetingTimer.Float();
					SetGreetingFlag(false);
				}
			}
		} else {
			// The last response has been said: drop the greeting and the head tracking on both sides.
			m_pSayToDialogueTopic = nullptr;
			if (pGreetTopic) {
				// `delete pGreetTopic`: the exe calls DialogueItem's scalar deleting destructor out of line.
				DialogueItem* pOldTopic = pGreetTopic;
				if (pOldTopic)
					delete pOldTopic;
			}
			pGreetTopic = nullptr;
			pDialogTarget = nullptr;
			StopSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
			bStop = false;
			bGreetingFlag = 0;
			TESPackage* pRunning = GetPackageThatIsRunning();
			if (pRunning && pRunning->GetPackType() == PACKAGE_TYPE_DIALOGUE && pTarget == pPlayer &&
				pRunning->GetDialogueSayToFlag())
				AddToProcedureIndexRunning(apActor, 3);
			pPlayer->ResetPlayerGreetFlag();
			TESObjectREFR* pHeadingTarget = apActor->GetHeadingTarget();
			Actor* pHeadingActor = nullptr;
			if (pHeadingTarget && pHeadingTarget->IsActor())
				pHeadingActor = static_cast<Actor*>(pHeadingTarget);
			if (pHeadingActor && pHeadingActor->GetCurrentAIProcess() && pHeadingActor != pPlayer) {
				pHeadingActor->GetCurrentAIProcess()->SetLastGreeted(nullptr);
				pHeadingActor->GetCurrentAIProcess()->ClearActionHeadTrackTarget(true);
			}
			ClearActionHeadTrackTarget(true);
		}
	}
	apActor->SetDialogueItemTarget(nullptr);
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessObserveCombat(Actor* apActor) {
	NiPoint3 result;
	FNVD_INLINED_LOCAL(PathingLocation, apMemory, std::nullopt, std::nullopt);
	NiPoint3 goalPoint;
	FNVD_INLINED_LOCAL(PathingRequestHide, hideRequest, std::nullopt, std::nullopt);
	using v18_t = BSSimpleArray<Actor*, 1024>;
	FNVD_INLINED_LOCAL(v18_t, v18, std::nullopt, std::nullopt);
	TESPackage* pbasepack = GetPackageThatIsRunning();
	SpectatorPackage* pspecPack = 0;
	if (pbasepack) {
		if (pbasepack->GetPackType() == PACKAGE_TYPE_SPECTATOR) {
			pspecPack = static_cast<SpectatorPackage*>(pbasepack);
		}
	}
	if (pspecPack) {
		if (pCombatManager->CheckShouldFlee(apActor, 0) || apActor->GetActorValue(ActorValue::CONFIDENCE) <= 0) {
			::new (&v18) BSSimpleArray<Actor*, 1024>;
			pspecPack->CreateThreatArray(&v18);
			int32_t FormFlags = v18.iSize;
			if (FormFlags > 0) {
				apActor->EndInterruptPackage(0);
				apActor->InitiateFlee(v18.operator[](0), 1, 1, 1, 0, 0, -1.0f, -1.0f);
				TESPackage* CurrentPackage = apActor->GetCurrentPackage();
				FleePackage* pflee = dynamic_cast<FleePackage*>(CurrentPackage);
				if (pflee) {
					for (int32_t i = 0; i < FormFlags; ++i) {
						pflee->AddAvoidedRef(v18.operator[](i));
					}
				}
				v18.~BSSimpleArray<Actor*, 1024>();
				return;
			}
			v18.~BSSimpleArray<Actor*, 1024>();
		}
		if (pspecPack->IsComplete()) {
			apActor->EndInterruptPackage(0);
			return;
		}
		float fdistance = 0.0f;
		float fendDist = 0.0f;
		int32_t v20 = 513;
		NiPoint3 eventCenter = pspecPack->GetEventCenter();
		float fdistFromEventCenter = apActor->GetDistanceFromPoint(eventCenter);
		TESObjectCELL* pcell = apActor->GetParentCell();
		bool bWithinSpectatorDist = 0;
		if (pcell && pcell->GetInterior()) {
			fdistance = fAIInteriorSpectatorDistance.Float();
			fendDist = fAIInteriorSpectatorDetection.Float();
			if (fdistance > static_cast<double>(fdistFromEventCenter)) {
				bWithinSpectatorDist = 1;
			}
		} else {
			fdistance = fAIExteriorSpectatorDistance.Float();
			fendDist = fAIExteriorSpectatorDetection.Float();
			if (fendDist > static_cast<double>(fdistFromEventCenter)) {
				bWithinSpectatorDist = 1;
			}
		}
		pspecPack->Update(apActor);
		if (apActor->IsPathingComplete()) {
			pspecPack->LookAtCombatants(apActor, 1);
			pspecPack->MakeCombatComment(apActor, 1);
			uint32_t itime = appTimer.iLastTime;
			if (itime > pspecPack->iLastPathRequestTime + 1500) {
				pspecPack->iLastPathRequestTime = itime;
				NiPoint3 v12 = apActor->GetLocationOnReference();
				::new (&hideRequest) PathingRequestHide;
				hideRequest.Initialize(apActor);
				if (apActor->IsCreature()) {
					TESActorBaseData* v6 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
					hideRequest.SetCantOpenDoors(v6->GetCantOpenDoors());
				}
				NiPoint3* v3 = ::new (&result) NiPoint3(pspecPack->GetEventCenter());
				PathingLocation* aThreatLocation = ::new (&apMemory) PathingLocation(*v3, apActor);
				hideRequest.SetThreatLocation(*aThreatLocation);
				apMemory.~PathingLocation();
				float afDistance = pspecPack->fEventRadius + 128.0;
				hideRequest.SetMinHideDistance(afDistance);
				hideRequest.SetTargetRadius(64.0f);
				PathingAvoidNodeArray* pavoidNodeArray = pspecPack->CreateAvoidNodeArray();
				hideRequest.SetAvoidNodeArray(pavoidNodeArray);
				if (!apActor->SetPathfindingHide(&hideRequest)) {
					hideRequest.~PathingRequestHide();
					return;
				}
				if (apActor->GetBlocked()) {
					apActor->SetBlock(0);
				}
				apActor->SetMoveMode(512u);
				if (apActor->pActorMover->GetCurrentGoal_009DE2B0(&goalPoint)) {
					float fdistancedest = apActor->GetDistanceFromPoint(goalPoint);
					if (fdistancedest <= 64.0) {
						v20 = 257;
					}
					SetActorsAnimation(apActor, v20, 1);
				}
				hideRequest.~PathingRequestHide();
			}
		} else {
			if (apActor->IsNpc()) {
				pspecPack->LookAtCombatants(apActor, 1);
				pspecPack->MakeCombatComment(apActor, 1);
			}
		}
		apActor->DidPathingFail();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessTravel(Actor* apActor, bool abNewpackage, bool abInterateindex, int32_t aemovemode,
	bool abSecondLoc) {
	NiPoint3 v64;
	NiPoint3 a1;
	FNVD_INLINED_LOCAL(PathingRequest, request_6, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequest, request_5, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequest, request_4, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequest, request_3, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequest, request, std::nullopt, std::nullopt);
	NiPoint3 point;
	TESPackage* pPack = 0;
	pPack = GetPackageThatIsRunning();
	bool bIgnoreDistance = 0;
	if (!pPack) {
		return;
	}
	PACKAGE_TYPE epackType = pPack->GetPackType();
	Animation* panim = apActor->GetAnimation();
	if (GetPackageIdleDone() && (panim && (panim->SpecialIdleWorking() || panim->SpecialIdlePlaying()))) {
		return;
	}
	bool v60 = epackType != PACKAGE_TYPE_SANDBOX && epackType != PACKAGE_TYPE_WANDER;
	if (epackType == PACKAGE_TYPE_FOLLOW) {
		const TESFollowPackageData* pfollowData = pPack->GetFollowPackageData();
		if (pfollowData->pStartLocation) {
			abSecondLoc = 1;
			SetTarget(apActor);
		} else {
			AddToProcedureIndexRunning(apActor, 1);
		}
	} else {
		if ((epackType == PACKAGE_TYPE_DIALOGUE || epackType == PACKAGE_TYPE_IN_GAME_DIALOGUE) &&
			(!pPack->pPackLoc ||
				(pPack->GetLocationReference(apActor) == apActor && pPack->pPackLoc->GetLocType() != PLT_EDITORLOC))) {
			AddToProcedureIndexRunning(apActor, 1);
			return;
		}
	}
	float fcurrentdistance = 0.0f;
	if (abSecondLoc) {
		NiPoint3* SecondLocationCoord = &(a1 = pPack->GetSecondLocationCoord(apActor));
		fcurrentdistance = apActor->GetDistanceFromPoint(*SecondLocationCoord);
	} else {
		fcurrentdistance = apActor->GetDistanceFromPoint(pPack->GetLocationCoord(apActor));
	}
	TESObjectREFR* pRef = 0;
	if (abSecondLoc && pPack->GetPackageSecondLocation()) {
		PackageLocation* PackageSecondLocation = pPack->GetPackageSecondLocation();
		pRef = PackageSecondLocation->GetLocReference();
	} else {
		if (pPack->pPackLoc) {
			pRef = pPack->pPackLoc->GetLocReference();
		}
	}
	if (pRef || (!pGenericLocation || GetRunOncePackage())) {
		pGenericLocation = 0;
	} else {
		pRef = pGenericLocation;
	}
	bool batRef = 0;
	if (v60 && (pRef && pRef->IsFurniture())) {
		NiPoint3 v98 = apActor->GetLocationOnReference();
		uint32_t inum = GetCurrentFurnitureIndex();
		if ((pCurrentFurniture == pRef && apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) ||
			apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			if (pPack->GetMustReachLocation() && !pPack->GetPackageTarget()) {
				fPackageEvalTimer = 0.0f;
			}
			if (abInterateindex) {
				AddToProcedureIndexRunning(apActor, 1);
			}
			goto LABEL_45;
		}
		if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
			(apActor->IsPathingComplete() &&
				(apActor != pPlayer && (pCurrentFurniture && pCurrentFurniture->GetMarkerUsed(inum, 1))))) {
			if (abInterateindex) {
				AddToProcedureIndexRunning(apActor, 1);
			}
			ClearFurnitureMarker();
			EndMoveMessage(apActor);
			return;
		}
		if (pCurrentFurniture &&
			(pRef != pCurrentFurniture &&
				(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_IS_SLEEPING))) {
			apActor->InitiateGetUpPackage();
			return;
		}
	}
	if (pRef) {
		if (pRef->IsFurniture() && !apActor->IsPathingComplete() && !pCurrentFurniture &&
			cCurrentFurnitureIndex != 127) {
			cCurrentFurnitureIndex = 127;
		}
	}
	if (apActor->IsPathingComplete() ||
		(apActor == pPlayer || (pRef && (pCurrentFurniture != pRef && pRef->IsFurniture())))) {
		if (pRef && pRef->IsFurniture()) {
			if (!pCurrentFurniture) {
				pCurrentFurniture = pRef;
				ClearFurnitureMarker();
			}
			if (cCurrentFurnitureIndex == 127) {
				if (apActor->GetParentCell()) {
					TESObjectCELL* v9;
					if (!apActor->GetParentCell()->GetInterior() ||
						(v9 = apActor->GetParentCell(), v9 == pCurrentFurniture->GetParentCell())) {
						uint32_t inum_2;
						point = apActor->GetLocationOnReference();
						inum_2 = 0;
						if (!pCurrentFurniture->GetClosestFreeMarker(point, 1, 1, FurnitureMarker, inum_2, 1)) {
							if (pCurrentFurniture->GetDistanceFromReference(apActor, 0, 0) > 200.0) {
								FNVD_CONSTRUCTED_LOCAL(PathingRequest, request_2, &PathingRequest::Destruct);
								TESWorldSpace* afRadius2a = pCurrentFurniture->GetWorldSpace();
								TESObjectCELL* ParentCell = pCurrentFurniture->GetParentCell();
								const NiPoint3* v11 = &pCurrentFurniture->GetLocationOnReference();
								apActor->BuildRequest(&request_2, *v11, ParentCell, afRadius2a, 200.0f, 0);
								request_2.SetSoftGoalRadius(0);
								if (apActor->SetPathfindingGoal(&request_2)) {
									if (pCurrentFurniture->Get3D()) {
										if (abInterateindex) {
											AddToProcedureIndexRunning(apActor, 1);
										}
										if (GetSitSleepState() == SIT_SLEEP_NORMAL) {
											ClearFurniture(apActor);
										}
										EndMoveMessage(apActor);
									}
								}
								return;
							}
							if (abInterateindex) {
								AddToProcedureIndexRunning(apActor, 1);
							}
							if (GetSitSleepState() == SIT_SLEEP_NORMAL) {
								ClearFurniture(apActor);
							}
						LABEL_45:;
							EndMoveMessage(apActor);
							return;
						}
						if (apActor == pPlayer) {
							NiPoint3* a2 = &(&FurnitureMarker)->Position;
							pPlayer->SetPosition(*a2);
							float a2a = FurnitureMarker.GetHeading();
							pPlayer->SetHeading(a2a);
							pPlayer->StopMoving();
							cCurrentFurnitureIndex = inum_2;
						} else {
							float afOutDir[2];
							float ftargetangle = FurnitureMarker.GetHeading();
							afOutDir[0] = 0.0f;
							float afRadius2 = apActor->GetHeading(0);
							float fangledif = GetAngleDelta(afRadius2, ftargetangle, *afOutDir);
							afOutDir[1] = static_cast<double>(iActorKeepTurnDegree.Int()) * 0.01745329238474369;
							::new (&request) PathingRequest;
							float afRadius = fAIFurnitureDestinationRadius.Float();
							TESWorldSpace* WorldSpace = pCurrentFurniture->GetWorldSpace();
							apActor->BuildRequest(&request, FurnitureMarker.Position,
								pCurrentFurniture->GetParentCell(), WorldSpace, afRadius, 0);
							request.SetGoalAngleToFace(ftargetangle);
							request.SetSoftGoalRadius(1);
							if (!apActor->SetPathfindingGoal(&request)) {
								request.~PathingRequest();
								return;
							}
							cCurrentFurnitureIndex = inum_2;
							request.~PathingRequest();
						}
					}
				}
			}
			if (apActor->GetSitSleepState()) {
				batRef = 1;
				goto LABEL_118;
			}
			if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
				(cCurrentFurnitureIndex != 127 &&
					(pCurrentFurniture->GetMarkerUsed(cCurrentFurnitureIndex, 1) && !apActor->IsPathingComplete()))) {
				ClearFurniture(apActor);
				EndMoveMessage(apActor);
				AddToProcedureIndexRunning(apActor, 1);
				return;
			}
			MobileObject* pmob = 0;
			if (pTarget) {
				if (pTarget->IsMobileObject()) {
					pmob = static_cast<MobileObject*>(pTarget);
				}
			}
			if (abSecondLoc) {
				batRef = pPack->IsActorAtSecondLocation(pmob, apActor, bIgnoreDistance, -1.0f, 0);
				goto LABEL_118;
			}
		} else {
			MobileObject* pmob_2 = 0;
			if (pTarget) {
				if (pTarget->IsMobileObject()) {
					pmob_2 = static_cast<MobileObject*>(pTarget);
				}
			}
			if (abSecondLoc) {
				batRef = pPack->IsActorAtSecondLocation(pmob_2, apActor, bIgnoreDistance, -1.0f, 0);
				goto LABEL_118;
			}
		}
		batRef = pPack->IsActorAtLocation(apActor, bIgnoreDistance, -1.0f, 0);
	}
LABEL_118:;
	float fdistance = pPack->GetPackageRadiusActorToLocation(apActor, abSecondLoc);
	if (pPack->GetPackType() == PACKAGE_TYPE_WANDER) {
		if (pPack->GetLocationCell(apActor)) {
			TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
			if (LocationCell->GetInterior()) {
				fdistance = 20.0f;
			}
		}
	}
	if (bIgnoreDistance) {
		fdistance = 10.0f;
	}
	if ((!batRef || !apActor->IsPathingComplete()) && !DidPathingFail(apActor)) {
		goto LABEL_281;
	}
	fPackageIdleTimer = 0.0f;
	if (pPack->GetPackType() == PACKAGE_TYPE_EAT) {
		if (pRef) {
			if (pRef->IsFurniture() && batRef && !apActor->GetBestFoodItem()) {
				AddToProcedureIndexRunning(apActor, 1);
				EndMoveMessage(apActor);
				return;
			}
		}
	}
	if (sShotsFired) {
		ArrowProjectile::DeleteAttachedArrows(pRef, 1);
		sShotsFired = 0;
		sShotsToFire = -1;
	}
	if (v60 && (pRef && (pRef->IsFurniture() && batRef))) {
		if (!pCurrentFurniture) {
			pCurrentFurniture = pRef;
			ClearFurnitureMarker();
		}
		TESObjectCELL* v13 = apActor->GetParentCell();
		if (v13 == pCurrentFurniture->GetParentCell()) {
			if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
				apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
				if (pPack->GetMustReachLocation() && !pPack->GetPackageTarget()) {
					fPackageEvalTimer = 0.0f;
				}
				if (abInterateindex) {
					AddToProcedureIndexRunning(apActor, 1);
				}
				EndMoveMessage(apActor);
			} else {
				if (!iAnimActionSuccess) {
					AddToProcedureIndexRunning(apActor, 1);
					pGenericLocation = 0;
					EndMoveMessage(apActor);
					iAnimActionSuccess = 1;
					return;
				}
				AlignToFurnitureAndAddPostAnimSitSleep(apActor);
			}
		}
	} else {
		uint8_t bfail = DidPathingFail(apActor);
		EndMoveMessage(apActor);
		if (!bfail) {
			TESBoundObject* ObjectReference;
			if ((pRef && (ObjectReference = pRef->GetObjectReference(), ObjectReference == pXMarkerHeading)) ||
				(pPack->pPackLoc && pPack->pPackLoc->GetLocType() == PLT_EDITORLOC)) {
				if (!DidPathingFail(apActor) && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && !GetForceRotate()) {
					float ftargetangle_3 = 0.0f;
					if (pRef) {
						ftargetangle_3 = (&pRef->GetAngleOnReference())->z;
					} else {
						ftargetangle_3 = apActor->GetStartingAngle().z;
					}
					apActor->RequestRotate(ftargetangle_3, 0);
				}
			}
		}
		if (bfail &&
			(pPack->GetMustReachLocation() ||
				(pPack->GetMustComplete() && pPack->GetPackType() == PACKAGE_TYPE_TRAVEL))) {
			abNewpackage = 1;
		} else {
			if (abInterateindex) {
				AddToProcedureIndexRunning(apActor, 1);
			}
		}
		if (pPack->GetMustReachLocation() && !pPack->GetOncePerDay() && !pPack->GetPackageTarget()) {
			fPackageEvalTimer = 0.0f;
		}
		if (pPack->GetOncePerDay() &&
			(!pPack->GetPackageSchedule() || !pPack->GetPackageSchedule()->PSData.iDuration) &&
			pPack->GetPackType() == PACKAGE_TYPE_TRAVEL) {
			char Day = appCalendar.GetDay();
			apActor->SetRunOnceExtraPackage(pPack, Day);
		}
		if (!GetLockedLocation()) {
			if (pPack->GetLockDoorsAtLocation()) {
				const NiPoint3* v36 = &apActor->GetLocationOnReference();
				const NiPoint3* v33 = &apActor->GetLocationOnReference();
				TESObjectCELL* v31 = apActor->GetParentCell();
				pDataHandler->EnumReferencesCloseToPoint(v31, *v33, 5000.0f, *v36, 5000.0f,
					&LowProcess::LockDoorsatLocation, apActor);
			}
			SetLockedLocation(1);
		}
		if (pPack->GetUnlockDoorsAtLocation()) {
			const NiPoint3* v37 = &apActor->GetLocationOnReference();
			const NiPoint3* v34 = &apActor->GetLocationOnReference();
			TESObjectCELL* v32 = apActor->GetParentCell();
			pDataHandler->EnumReferencesCloseToPoint(v32, *v34, 5000.0f, *v37, 5000.0f,
				&LowProcess::UnLockDoorsatLocation, apActor);
		}
		if (pPack) {
			if (!bDoneClothesChange && pPack->GetForceUnequipWeapon()) {
				bDoneClothesChange = 1;
				ItemChange* pweapon = GetCurrentWeapon();
				if (pweapon) {
					apActor->QueueUnEquipObject(pweapon->pContainerObj, 1, 0, 0, 0, 1);
				}
				ItemChange* pammo = GetCurrentAmmo();
				if (pammo) {
					uint32_t FormHealth_0 = pammo->iNumber;
					apActor->QueueUnEquipObject(pammo->pContainerObj, FormHealth_0, 0, 0, 0, 1);
				}
			}
		}
	}
	if (DidPathingFail(apActor) && abNewpackage) {
	LABEL_281:;
		if (batRef && apActor->IsPathingComplete()) {
			AddToProcedureIndexRunning(apActor, 1);
			return;
		}
		if (!batRef && apActor->IsPathingComplete()) {
			int32_t v59 = abSecondLoc ? !pPack->IsActorAtSecondLocation(apActor, apActor, bIgnoreDistance, -1.0f, 0)
									  : !pPack->IsActorAtLocation(apActor, bIgnoreDistance, -1.0f, 0);
			if (v59 && !DidPathingFail(apActor)) {
				if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
					GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
					apActor->InitiateGetUpPackage();
					return;
				}
				if (pRef && pRef->IsFurniture()) {
					NiPoint3 point_2 = apActor->GetLocationOnReference();
					uint32_t inum_3 = 0;
					if (!pCurrentFurniture || pCurrentFurniture->Get3D()) {
						if (!pCurrentFurniture ||
							(!pCurrentFurniture->Get3D() ||
								!pCurrentFurniture->GetClosestFreeMarker(point_2, 1, 1, FurnitureMarker, inum_3, 1))) {
							EndMoveMessage(apActor);
							PACKAGE_PROCEDURE_TYPE Count = pPack->ePROCEDURE_TYPE;
							int32_t ProcedureCount = Procedure::GetProcedureCount(Count);
							SetProcedureIndexRunning(ProcedureCount - 1);
							return;
						}
						::new (&request_4) PathingRequest;
						float afRadiusb = fAIFurnitureDestinationRadius.Float();
						TESWorldSpace* afRadius2c = pPack->GetLocationWorld(apActor);
						TESObjectCELL* v18 = pPack->GetLocationCell(apActor);
						apActor->BuildRequest(&request_4, FurnitureMarker.Position, v18, afRadius2c, afRadiusb, 0);
						float ftargetangle_4 = FurnitureMarker.GetHeading();
						request_4.SetGoalAngleToFace(ftargetangle_4);
						request_4.SetSoftGoalRadius(1);
						if (!apActor->SetPathfindingGoal(&request_4)) {
							request_4.~PathingRequest();
							return;
						}
						pCurrentFurniture = pRef;
						cCurrentFurnitureIndex = inum_3;
						request_4.~PathingRequest();
					} else {
						::new (&request_3) PathingRequest;
						float afRadiusa = fAIFurnitureDestinationRadius.Float();
						TESWorldSpace* afRadius2b = pPack->GetLocationWorld(apActor);
						TESObjectCELL* v38 = pPack->GetLocationCell(apActor);
						const NiPoint3* v17 = &pCurrentFurniture->GetLocationOnReference();
						apActor->BuildRequest(&request_3, *v17, v38, afRadius2b, afRadiusa, 0);
						request_3.SetSoftGoalRadius(1);
						if (!apActor->SetPathfindingGoal(&request_3)) {
							request_3.~PathingRequest();
							return;
						}
						request_3.~PathingRequest();
					}
				} else {
					if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
						::new (&request_5) PathingRequest;
						if (abSecondLoc) {
							TESWorldSpace* afRadius2d = pPack->GetSecondLocationWorld(apActor);
							TESObjectCELL* SecondLocationCell = pPack->GetSecondLocationCell(apActor);
							NiPoint3* v21 = &(v64 = pPack->GetSecondLocationCoord(apActor));
							apActor->BuildRequest(&request_5, *v21, SecondLocationCell, afRadius2d, fdistance, 0);
						} else {
							TESWorldSpace* afRadius2e = pPack->GetLocationWorld(apActor);
							TESObjectCELL* v40 = pPack->GetLocationCell(apActor);
							apActor->BuildRequest(&request_5, pPack->GetLocationCoord(apActor), v40, afRadius2e,
								fdistance, 0);
						}
						if (pRef) {
							TESBoundObject* pobject = pRef->GetObjectReference();
							if (pobject) {
								if (pobject == pXMarkerHeading || pobject->GetFormType() == IDLM_ID) {
									request_5.SetGoalAngleToFace((&pRef->GetAngleOnReference())->z);
								}
							}
						}
						request_5.SetSoftGoalRadius(0);
						if (!apActor->SetPathfindingGoal(&request_5)) {
							request_5.~PathingRequest();
							return;
						}
						request_5.~PathingRequest();
					}
				}
			}
		}
		if (DidPathingFail(apActor) && (!pPack->GetMustReachLocation() && !pPack->GetMustComplete())) {
			PACKAGE_PROCEDURE_TYPE v23 = pPack->ePROCEDURE_TYPE;
			int32_t v24 = Procedure::GetProcedureCount(v23);
			SetProcedureIndexRunning(v24 - 1);
			return;
		}
		if (!batRef && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
			float v74 = 0.0f;
			Actor* plocactor = 0;
			if (pPack->pPackLoc) {
				TESObjectREFR* plocref = pPack->pPackLoc->GetLocReference();
				if (!plocref) {
					plocref = GetGenericLocation();
				}
				if (plocref) {
					if (plocref->IsActor()) {
						plocactor = static_cast<Actor*>(plocref);
					}
				}
			}
			if (plocactor) {
				const NiPoint3* v25 = &plocactor->GetLocationOnReference();
				float fdistancemoved = (lastSeenPostion - *v25).Length();
				float* v27 = &fAIMoveDistanceToRecalcFollowPath.Float();
				if (*v27 < static_cast<double>(fdistancemoved) ||
					(fdistance < static_cast<double>(fcurrentdistance) && apActor->IsPathingComplete())) {
					::new (&request_6) PathingRequest;
					TESWorldSpace* afRadius2f = plocactor->GetWorldSpace();
					TESObjectCELL* v41 = plocactor->GetParentCell();
					const NiPoint3* v28 = &plocactor->GetLocationOnReference();
					apActor->BuildRequest(&request_6, *v28, v41, afRadius2f, fdistance, 0);
					request_6.SetSoftGoalRadius(0);
					if (!apActor->SetPathfindingGoal(&request_6)) {
						request_6.~PathingRequest();
						return;
					}
					lastSeenPostion = plocactor->GetLocationOnReference();
					request_6.~PathingRequest();
				}
			}
			if (apActor == pPlayer) {
				if (!pRef || !pRef->IsFurniture()) {
					apActor->InitiateGetUpPackage();
				}
			} else {
				if (pPack->GetAlwaysRun()) {
					aemovemode = 513;
				} else {
					if (aemovemode == -1) {
						int32_t v68 = 0;
						bool bflag = 0;
						if (GetCurrentPackage()) {
							TESPackage* v30;
							TESPackage* v29 = GetCurrentPackage();
							if (v29->GetPackType() == PACKAGE_TYPE_ALARM ||
								(v30 = GetCurrentPackage(), v30->GetPackType() == PACKAGE_TYPE_COMBAT)) {
								bflag = 1;
							}
						}
						float v58 = fdistance + fdistance;
						int32_t emove = CalculateMoveMode(apActor, fcurrentdistance, fdistance, v58, bflag, 1);
						aemovemode = emove;
					}
				}
				SetActorsAnimation(apActor, aemovemode, 1);
				NiPoint3 entryPoint = pPack->GetLocationCoord(apActor);
				if (pRef) {
					if (pRef->IsFurniture()) {
						entryPoint = FurnitureMarker.Position;
					}
				}
			}
		}
		if (!batRef || apActor->IsPathing()) {
			if (!batRef &&
				(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
				apActor->InitiateGetUpPackage();
			}
		} else {
			AddToProcedureIndexRunning(apActor, 1);
		}
	}
}

void HighProcess::RemoveDetectionActor(Actor* apActor, int32_t abWhichList) {
	BSSimpleList<DetectionState*>* plist = 0;
	BSSimpleList<DetectionState*>* plastlist = 0;
	bool bnext = 0;
	if (abWhichList && abWhichList != 3) {
		if (abWhichList == 1) {
			plist = pActorsWhoDetectMeList;
		}
	} else {
		plist = pDetectedActorList;
	}
	if (abWhichList == 3) {
		if (plist && plist->m_item) {
			bnext = 1;
		} else {
			plastlist = 0;
			plist = pActorsWhoDetectMeList;
		}
	}
	while (plist && plist->m_item) {
		DetectionState* ptemp = plist->m_item;
		if (ptemp->pActor == apActor) {
			if (plastlist) {
				plastlist->Remove(ptemp);
				plist = plastlist->m_pkNext;
			} else {
				plist->RemoveHead();
			}
			ourFree(ptemp);
		} else {
			plastlist = plist;
			plist = plist->m_pkNext;
		}
		if ((!plist || !plist->m_item) && bnext) {
			plastlist = 0;
			plist = pActorsWhoDetectMeList;
			bnext = 0;
		}
	}
}

void HighProcess::SetDetectionTimer(float aftime) {
	fDetectionTimer = aftime;
}

bool HighProcess::GetLineSightonActor(Actor* apActor, Actor* apTarget, int32_t abWhichList, bool ab360) {
	DetectionState* pData = GetDetectionState(apTarget, abWhichList);
	uint8_t v6 = 0;
	if (pData) {
		if (ab360) {
			return (bool)((uint8_t)pData->b360view);
		} else {
			return (bool)((uint8_t)pData->bLineofSight);
		}
	}
	return (bool)v6;
}

bool HighProcess::Get360LineSightonActor(Actor* apActor, Actor* apTarget, int32_t abWhichList) {
	DetectionState* v6 = GetDetectionState(apTarget, abWhichList);
	int8_t v5 = 0;
	if (v6) {
		return (bool)((uint8_t)v6->b360view);
	}
	return (bool)v5;
}

int32_t HighProcess::GetDetectionActor(Actor* apActor, int32_t abWhichList) {
	DetectionState* v5 = GetDetectionState(apActor, abWhichList);
	if (v5) {
		return v5->iLevel;
	} else {
		return 2147483647;
	}
}

bool HighProcess::GetTalkingtoPC() {
	return bDialoguewithPlayer;
}

void HighProcess::SetTalkingtoPC(bool abflag) {
	bDialoguewithPlayer = abflag;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::UpdateFollowers(Actor* apActor, float afDelta) {
	ExtraFollower* pfollow;
	Actor* ptarget;
	// The exe tests this address (&apActor->m_Extra), which is never null.
	pfollow = (&apActor->m_Extra)->GetFollowerExtra();
	if (pfollow) {
		BSSimpleList<Actor*>* pHead;
		BSSimpleList<Actor*>* pTempList;
		BSSimpleList<Actor*>* v10 = static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
		if (v10) {
			pTempList = ::new (v10) BSSimpleList<Actor*>;
		} else {
			pTempList = 0;
		}
		BSSimpleList<Actor*>* v9 = static_cast<BSSimpleList<Actor*>*>(ourMalloc(sizeof(BSSimpleList<Actor*>)));
		if (v9) {
			pHead = ::new (v9) BSSimpleList<Actor*>;
		} else {
			pHead = 0;
		}
		BSSimpleList<Actor*>* plist = pHead;
		BSSimpleList<Actor*>* ptemp = pfollow->pActorlist;
		float fnumber = 0.0f;
		while (ptemp && ptemp->m_item) {
			pHead->AddTail(ptemp->m_item);
			ptemp = ptemp->m_pkNext;
		}
		while (plist && plist->m_item) {
			uint32_t ihighActorCount = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
			ptarget = plist->m_item;
			if (ptarget) {
				if (ptarget != pPlayer) {
					if (ptarget->IsFollowing(apActor)) {
						PROCESS_TYPE etype = ptarget->GetCurrentProcessType();
						if (etype) {
							float time = afDelta;
							if (afDelta <= 0.0) {
								time = 3600.0f;
							}
							if (etype == PROCESS_TYPE_LOW) {
								float v5 = appCalendar.GetHour() - 1.0;
								ptarget->pCurrentProcess->SetLastTimeProcessed(v5);
							}
							ptarget->Process(time);
						} else {
							ptarget->Process(0.0f);
							if (ptarget->IsActor() && ptarget->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
								if (!(ptarget->GetDetectionTimer() < 0.0) ||
									(ptarget->IsUnConscious() || ptarget->IsDead(0))) {
									ptarget->ReduceDetectionTimer();
								} else {
									ptarget->EvaluateDetection(fnumber);
									double v4 = static_cast<double>(ihighActorCount) * 0.25;
									double Seconds = appTimer.fDelta;
									fnumber = Seconds * v4 + fnumber;
								}
							}
						}
					} else {
						pTempList->AddHead(ptarget);
					}
				}
			}
			plist = plist->m_pkNext;
		}
		for (plist = pTempList; plist && plist->m_item; plist = plist->m_pkNext) {
			(&apActor->m_Extra)->RemoveFollower(plist->m_item);
		}
		pTempList->RemoveAll();
		if (pTempList) {
			delete pTempList;
		}
		pHead->RemoveAll();
		if (pHead) {
			delete pHead;
		}
		if (pfollow->pActorlist->IsEmpty()) {
			(&apActor->m_Extra)->RemoveFollowerExtra();
		}
	}
}

int32_t HighProcess::GetEmotionsDispostion(Actor* apActor, TESObjectREFR* pRef) {
	int32_t v5 = 50;
	if (apActor->IsNpc() && ((!pRef) && (pTarget && (pTarget->IsActor() && (!pTarget->GetDisabled()))))) {
		return apActor->GetDisposition(static_cast<Actor*>(pTarget), nullptr);
	}
	if (pRef) {
		if (pRef->IsActor()) {
			return apActor->GetDisposition(static_cast<Actor*>(pRef), nullptr);
		}
	}
	return v5;
}

DIALOGUE_EMOTION HighProcess::GetLastSpeakingEmotion() {
	return eLastSpeakingEmotion;
}

void HighProcess::SetLastSpeakingEmotion(DIALOGUE_EMOTION aeEmotion) {
	eLastSpeakingEmotion = aeEmotion;
}

// Runs apActor's detection: against the player first, then against the actors of the high process list until one
// is the last detected; actors out of range get the minimum combat detection. Then the detection timer restarts,
// plus fnumber out of combat.
void HighProcess::RunDetection(Actor* apActor, float fnumber) {
	BaseProcess* pProcess = this;
	AggroRadiusList.RemoveAll();
	AvoidActorList.RemoveAll();
	// test ah, 0x41; jne: a NaN alpha runs detection.
	if (pFaderManager->GetFaderAlpha(FADER_TO_BLACK) > 0.0) {
		return;
	}
	if (apActor->IsKnockedOut()) {
		return;
	}
	if (apActor->IsReanimated() && apActor->GetGhost()) {
		return;
	}
	if (apActor->GetDistanceFromReference(pPlayer, false, false) >= 8192.0) {
		return;
	}
	if (!ReferenceProcessLists.bRunDetection) {
		return;
	}
	BSPerfTimer_Detection_RunDetection.Start();
	bCheckDeadTalk = false;
	TempRefList.RemoveAll();
	int32_t iLevel = 0;
	bool bInCombat = apActor->IsInCombat();
	float fMaxDistance = fSneakMaxDistance.Float();
	Actor* pTarget = pPlayer;
	if (!pTarget->IsDead(false)) {
		// The exe moves the player's commander into pTarget and this process's commander into apActor (its own
		// argument slot), then restores both: only the calls have an effect.
		Actor* pSavedActor = apActor;
		Actor* pSavedTarget = pTarget;
		if (pTarget->GetCurrentAIProcess() && pTarget->GetCurrentAIProcess()->GetCommandingActor()) {
			pTarget = pTarget->GetCurrentAIProcess()->GetCommandingActor();
		}
		if (pProcess->GetCommandingActor()) {
			apActor = pProcess->GetCommandingActor();
			if (apActor == pPlayer) {
				apActor = pTarget;
				pTarget = pPlayer;
			}
		}
		apActor = pSavedActor;
		pTarget = pSavedTarget;
	}
	// The exe doubles the distance outdoors here too, then reloads it before its only use.
	if (!apActor->GetParentCell() || !apActor->GetParentCell()->GetInterior()) {
		fMaxDistance = fMaxDistance + fMaxDistance;
	}
	if (pTarget != apActor) {
		bool bTargetInCombat = pTarget->IsInCombat();
		bool b360LineOfSight = false;
		if (bInCombat) {
			b360LineOfSight = true;
		}
		FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
		bool bAttack = false;
		if (bInCombat && apActor->IsInCombatWithActor(pTarget)) {
			bAttack = true;
		} else if (apActor->GetShouldAttackActor(pTarget, false, eReaction, true)) {
			bAttack = true;
		}
		if (!bInCombat || ShouldRunCombatDetection(apActor, pTarget)) {
			bool bLineOfSight = false;
			iLevel = apActor->GetDetectionLevelAgainstActor(true, pTarget, bLineOfSight, bTargetInCombat, false,
				bInCombat, &b360LineOfSight);
			int32_t iDetectedEventLevel = -100;
			// test ah, 1; jne: the event check runs when the setting is >= the level (not NaN).
			if (fSneakSeenMin.Float() >= (double)iLevel &&
				pTarget->GetCurrentAIProcess()->GetActorsDetectionEvent(pTarget) && bAttack &&
				(!bInCombat || ShouldRunCombatDetectionEventCheck(apActor, pTarget))) {
				DetectionEvent* pEvent = pTarget->GetCurrentAIProcess()->GetActorsDetectionEvent(pTarget);
				int32_t iLightLevel = (int32_t)apActor->GetCurrentAIProcess()->GetActorLightLevel();
				bool bExterior = true;
				if (pEvent && pEvent->pRef && pEvent->pRef->GetParentCell()) {
					bExterior = !pEvent->pRef->GetParentCell()->GetInterior();
				}
				bool bLOS = apActor->CalculateLOS(&pEvent->Location, true, true);
				bool bInView = false;
				if (bLOS && apActor->IsPointInViewCone(pEvent->Location, fDetectionViewCone.Float())) {
					bInView = true;
				}
				// The exe evaluates the arguments right to left.
				int32_t iActionValue = pEvent->iActionValue;
				bool bActorInCombat = apActor->IsInCombat();
				bool bAlert = apActor->GetAlert();
				bool bSleeping = apActor->IsSleeping();
				float fDistance = apActor->GetDistanceFromPoint(pEvent->Location);
				int32_t iSneak = static_cast<ActorValueOwner*>(apActor)->GetClampedActorValue(ActorValue::SNEAK);
				iDetectedEventLevel = AiFormulas::GetDetectedEvent(iSneak, bLOS, fDistance, 0, iLightLevel, bSleeping,
					bAlert, bActorInCombat, iActionValue, bInView, bExterior);
				// test ah, 5; jp: raised only when the setting is below the event level (not NaN).
				if (iDetectedEventLevel > iLevel && fSneakNoticedMin.Float() < (double)iDetectedEventLevel) {
					iLevel = (int32_t)fSneakNoticedMin.Float() + 1;
				}
			}
			DETECTION_LEVEL eDetect = DETECTION_LOST;
			const NiPoint3* pLocation = &pTarget->GetLocationOnReference();
			NiPoint3 kLastPosition;
			kLastPosition.x = pLocation->x;
			kLastPosition.y = pLocation->y;
			kLastPosition.z = pLocation->z;
			// The exe tests a state pointer it has just set to null before this insert.
			DetectionState* pState = pProcess->InsertIntoDetectionList(pTarget, eDetect, bLineOfSight, iLevel, 0,
				b360LineOfSight, bInCombat);
			if (iLevel > 0) {
				pTarget->GetCurrentAIProcess()->InsertIntoDetectionList(apActor, eDetect, bLineOfSight, iLevel, 1,
					b360LineOfSight, bTargetInCombat);
			} else {
				pTarget->GetCurrentAIProcess()->RemoveDetectionActor(apActor, 1);
			}
			pState->iLevel = iLevel;
			pState->bLineofSight = bLineOfSight;
			pState->iDetectedEventLevel = iDetectedEventLevel;
			if (iLevel > 0) {
				pState->LastPositionDetected.x = kLastPosition.x;
				pState->LastPositionDetected.y = kLastPosition.y;
				pState->LastPositionDetected.z = kLastPosition.z;
				CombatTimeStamp kTimeStamp;
				FNVD_RECONSTRUCT(CombatTimeStamp)(&kTimeStamp, CombatTimer::GetTimer());
				pState->fLastTimeDetected.fTimeStamp = kTimeStamp.fTimeStamp;
				if (bAttack) {
					pPlayer->SetHostileDetection(true);
				}
			}
		} else {
			pProcess->RemoveDetectionActor(pPlayer, 0);
			pTarget->GetCurrentAIProcess()->RemoveDetectionActor(apActor, 1);
		}
	}
	plastDetected = nullptr;
	fMaxDistance = fSneakMaxDistance.Float();
	if (!apActor->GetParentCell() || !apActor->GetParentCell()->GetInterior()) {
		fMaxDistance = fMaxDistance + fMaxDistance;
	}
	ProcessArray* pArray = ReferenceProcessLists.GetProcessArray();
	for (uint32_t i = 0; !plastDetected && i < pArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
		MobileObject* pMobile = pArray->GetItem(i);
		if (!pMobile || pMobile->GetDisabled() || !pMobile->IsActor() || pMobile == apActor) {
			continue;
		}
		pTarget = static_cast<Actor*>(pMobile);
		float fDistance = apActor->GetDistanceFromReference(pMobile, false, false);
		if (bInCombat && apActor != pPlayer && !ShouldRunCombatDetection(apActor, pTarget)) {
			continue;
		}
		// fcompp; test ah, 1: in range when the max distance is >= the distance; test ah, 0x41; jp: and the target
		// is within 8192 of the player (NaN is out of range in both).
		if (fMaxDistance >= fDistance && pTarget->GetDistanceFromReference(pPlayer, false, false) <= 8192.0) {
			bool bLineOfSight = false;
			bool bTargetInCombat = false;
			bool bAllTargetsSearching = false;
			if (pTarget == pPlayer) {
				bTargetInCombat = pPlayer->IsPlayerCharacterInCombat(&bAllTargetsSearching);
			} else {
				bTargetInCombat = pTarget->IsInCombat(true);
			}
			bool b360LineOfSight = bInCombat;
			int32_t iDetectedEventLevel = -100;
			// test ah, 0x41; jp: within 2 (not NaN) counts as detected at 100.
			if (fDistance <= 2.0) {
				iLevel = 100;
				bLineOfSight = true;
				b360LineOfSight = true;
			} else {
				iLevel = apActor->GetDetectionLevelAgainstActor(true, pTarget, bLineOfSight, bTargetInCombat, false,
					false, &b360LineOfSight);
			}
			FIGHT_REACTION eReaction = FIGHT_REACTION_NEUTRAL;
			if (fSneakSeenMin.Float() >= (double)iLevel &&
				pTarget->GetCurrentAIProcess()->GetActorsDetectionEvent(pTarget)) {
				bool bAttack = false;
				if (bInCombat && apActor->IsInCombatWithActor(pTarget)) {
					bAttack = true;
				} else if (apActor->GetShouldAttackActor(pTarget, false, eReaction, true)) {
					bAttack = true;
				}
				if (bAttack && (!bInCombat || ShouldRunCombatDetectionEventCheck(apActor, pTarget))) {
					DetectionEvent* pEvent = pTarget->GetCurrentAIProcess()->GetActorsDetectionEvent(pTarget);
					int32_t iLightLevel = (int32_t)apActor->GetCurrentAIProcess()->GetActorLightLevel();
					bool bExterior = true;
					if (pEvent && pEvent->pRef && pEvent->pRef->GetParentCell()) {
						bExterior = !pEvent->pRef->GetParentCell()->GetInterior();
					}
					bool bLOS = apActor->CalculateLOS(&pEvent->Location, true, true);
					bool bInView = false;
					if (bLOS && apActor->IsPointInViewCone(pEvent->Location, fDetectionViewCone.Float())) {
						bInView = true;
					}
					// The exe evaluates the arguments right to left.
					int32_t iActionValue = pEvent->iActionValue;
					bool bActorInCombat = apActor->IsInCombat();
					bool bAlert = apActor->GetAlert();
					bool bSleeping = apActor->IsSleeping();
					float fEventDistance = apActor->GetDistanceFromPoint(pEvent->Location);
					int32_t iSneak = static_cast<ActorValueOwner*>(apActor)->GetClampedActorValue(ActorValue::SNEAK);
					iDetectedEventLevel = AiFormulas::GetDetectedEvent(iSneak, bLOS, fEventDistance, 0, iLightLevel,
						bSleeping, bAlert, bActorInCombat, iActionValue, bInView, bExterior);
					if (iDetectedEventLevel > iLevel && fSneakNoticedMin.Float() < (double)iDetectedEventLevel) {
						iLevel = (int32_t)fSneakNoticedMin.Float() + 1;
					}
				}
			}
			DETECTION_LEVEL eDetect = DETECTION_LOST;
			// test ah, 5; jp: seen when the setting is below the level (not NaN).
			if (fSneakSeenMin.Float() < (double)iLevel) {
				eDetect = DETECTION_SEEN;
			}
			// The exe tests a state pointer it has just set to null before this insert.
			DetectionState* pState = pProcess->InsertIntoDetectionList(pTarget, eDetect, bLineOfSight, iLevel, 0,
				b360LineOfSight, bInCombat);
			if (iLevel > 0) {
				pTarget->GetCurrentAIProcess()->InsertIntoDetectionList(apActor, eDetect, bLineOfSight, iLevel, 1,
					b360LineOfSight, bTargetInCombat);
			} else {
				pTarget->GetCurrentAIProcess()->RemoveDetectionActor(apActor, 1);
			}
			if (pState) {
				pState->iLevel = iLevel;
				pState->bLineofSight = bLineOfSight;
				pState->iDetectedEventLevel = iDetectedEventLevel;
			}
		} else {
			DetectionState* pState = pProcess->GetDetectionState(pTarget, 0);
			if (!pState) {
				pState = pProcess->InsertIntoDetectionList(pTarget, DETECTION_LOST, false, iAICombatMinDetection.UInt(),
					0, false, bInCombat);
			} else {
				pState->iLevel = iAICombatMinDetection.UInt();
			}
			pTarget->GetCurrentAIProcess()->RemoveDetectionActor(apActor, 1);
		}
	}
	TempRefList.RemoveAll();
	if (!bInCombat) {
		fDetectionTimer = fDetectionTimerSetting.Float() + fnumber;
	} else {
		fDetectionTimer = fDetectionTimerSetting.Float();
	}
	bEvaluateDetection = 1;
	++iDetectionCounter;
	iDetectionCounter %= 12;
	BSPerfTimer_Detection_RunDetection.Stop();
}

float HighProcess::GetDetectionTimer() {
	return fDetectionTimer;
}

// Subtracts the current frame time from the detection timer.
void HighProcess::ReduceDetectionTimer() {
	float fSeconds = appTimer.GetSeconds();
	fDetectionTimer = fDetectionTimer - fSeconds;
}

void HighProcess::SetBreathTimer(float afTimer) {
	fBreathTimer = afTimer;
}

float HighProcess::GetBreathTimer() {
	return fBreathTimer;
}

bool HighProcess::GetGreetingFlag() {
	return bGreetingFlag;
}

void HighProcess::SetGreetingFlag(bool abFlag) {
	bGreetingFlag = abFlag;
}

float HighProcess::GetSoundDelay() {
	return fSoundDelay;
}

void HighProcess::SetSoundDelay(float afDelay) {
	fSoundDelay = afDelay;
}

bool HighProcess::GetFinishingCombatPackage() {
	return bFinishingCombatPackage;
}

void HighProcess::SetFinishingCombatPackage(bool abFlag) {
	bFinishingCombatPackage = abFlag;
}

// The exe's cleanup-only EH frame around the ItemChange construction is left out.
ItemChange* HighProcess::CreateWeaponAcquireList(Actor* apActor, PACKAGE_OBJECT_TYPE aeType) {
	ItemChange* pResult = nullptr;
	ObjectList.RemoveAll();
	TESObjectCELL* pCell = apActor->GetParentCell();
	NiPoint3 kPoint = apActor->GetLocationOnReference();
	pObjecttoAcquire = nullptr;
	eFormType = aeType;
	float fRadius = 0.0f;
	if (!GetCurrentPackage() || !GetCurrentPackage()->GetAcquireRadius(apActor, fRadius)) {
		fRadius = fAIAcquireObjectDistance.Float();
	}
	const NiPoint3* pLocation = &apActor->GetLocationOnReference();
	// The exe passes the actor itself as the enumeration data.
	pDataHandler->EnumReferencesCloseToPoint(pCell, kPoint, fRadius, *pLocation, fRadius,
		&LowProcess_EvaluateReferenceforAcquire, apActor);
	eFormType = POTYPE_NONE;
	pObjecttoAcquire = nullptr;
	EvaluateOrderAcquireList(apActor);
	if (ObjectList.IsEmpty()) {
		return pResult;
	}
	ObjectstoAcquire* pAcquire = ObjectList.GetItem();
	void* pMemory = ourMalloc(sizeof(ItemChange));
	ItemChange* pChange;
	if (pMemory) {
		pChange = ::new (pMemory) ItemChange(static_cast<TESBoundObject*>(pAcquire->pObject), 1);
	} else {
		pChange = nullptr;
	}
	pResult = pChange;
	pResult->GetExtraDataList()->AddHead(pAcquire->pExtra);
	return pResult;
}

// The exe calls BSSimpleList's IsEmpty (0x8256D0), GetItem (0x6815C0) and Remove (0x905330) out of line.
// The exe tests the copied item for null again (0x8DA637) before it deletes it.
void HighProcess::ClearAcquireList() {
	for (;;) {
		bool bEmpty;
		bEmpty = ObjectList.IsEmpty();
		if (bEmpty)
			break;
		ObjectstoAcquire* pObject;
		pObject = ObjectList.GetItem();
		ObjectstoAcquire* pDelete = pObject;
		if (pDelete)
			pDelete->Delete();
		ObjectList.Remove(pObject);
	}
}

float HighProcess::GetIdleTimer() {
	return fIdleTimer;
}

void HighProcess::SetIdleTimer(float aftime) {
	fIdleTimer = aftime;
}

bool HighProcess::EnterCombat(Actor* apActor, Actor* apTarget, bool abCheckForAmbush, bool absendalarm,
	CombatGroup* apGroup, bool abfleeing, bool abForceFlee, bool abIsAggressor, char acTeam, bool bforcecombat,
	bool bIgnoreDefensivePackage, bool abCastSpell, TESPackage* apPackage) {
	TESPackage* v16 = GetRunOncePackage();
	GetCurrentPackage();
	if (GetFinishingCombatPackage() || (v16 && v16->GetPackType() == PACKAGE_TYPE_USE_WEAPON)) {
		return 0;
	}
	if (bUnequippedArmorToSwim) {
		ProcessRemoveWorn(apActor, 0, 0);
		bUnequippedArmorToSwim = 0;
		fReEquipArmorTimer = fReEquipArmorTime.Float();
	}
	return MiddleHighProcess::EnterCombat(apActor, apTarget, abCheckForAmbush, absendalarm, apGroup, abfleeing,
		abForceFlee, abIsAggressor, acTeam, bforcecombat, bIgnoreDefensivePackage, abCastSpell, apPackage);
}

void HighProcess::GuardCallforHelp(Actor* apActor, Actor* apCriminal) {
	float afRadius2 = static_cast<float>(iCrimeAlarmRecDistance.Int());
	const NiPoint3* v7 = &apActor->GetLocationOnReference();
	float afRadius1 = static_cast<float>(iCrimeAlarmRecDistance.Int());
	const NiPoint3* v5 = &apActor->GetLocationOnReference();
	TESObjectCELL* ParentCell = apActor->GetParentCell();
	pDataHandler->EnumReferencesCloseToPoint(ParentCell, *v5, afRadius1, *v7, afRadius2,
		&HighProcess::FindDoorsToSendYellAlarm, apActor);
	BSSimpleList<TESObjectREFR*>* pListRef =
		ReferenceProcessLists.SendResistArresttoGuards(apCriminal, &DoorRefListCrime);
	BSSimpleList<TESObjectREFR*>* ptemplist = pListRef;
	while (pListRef && pListRef->m_item) {
		Actor* pSendactor = 0;
		TESObjectREFR* m_item = pListRef->m_item;
		if (m_item->IsActor()) {
			pSendactor = static_cast<Actor*>(pListRef->m_item);
		}
		pListRef = pListRef->m_pkNext;
		if (pSendactor) {
			if (pSendactor->GetCurrentProcessType()) {
				float v10 = appCalendar.GetHour() - 1.0;
				pSendactor->pCurrentProcess->SetLastTimeProcessed(v10);
				PROCESS_TYPE a2 = pSendactor->GetCurrentProcessType();
				ReferenceProcessLists.RemoveReference(pSendactor, a2);
				PROCESS_TYPE CurrentProcessType = pSendactor->GetCurrentProcessType();
				ReferenceProcessLists.AddReference(pSendactor, CurrentProcessType, 0, 0, 0);
			}
		}
	}
	if (ptemplist) {
		ptemplist->RemoveAll();
		delete ptemplist;
	}
	DoorRefListCrime.RemoveAll();
}

// The exe's alarm flag is a byte local set to 0 before it is tested, so the alarm block is never run; the exe still
// reads the actor's process first.
void HighProcess::CallforHelp(Actor* apActor, Actor* pCriminal, Crime* pcrime) {
	TESNPC* pnpc = 0;
	if (apActor->GetObjectReference()->GetFormType() == NPC__ID) {
		pnpc = static_cast<TESNPC*>(apActor->GetObjectReference());
	}
	TESObjectREFR* pcrimeAct = 0;
	TESObjectREFR* pcrimeRef = pcrime->pcrimetarget;
	if (pcrimeRef) {
		if (pcrimeRef->IsActor()) {
			pcrimeAct = pcrimeRef;
		}
	}
	if (apActor->GetInterior()) {
		float apfnEnumFunc = static_cast<float>(iCrimeAlarmRecDistance.Int());
		const NiPoint3* v9 = &apActor->GetLocationOnReference();
		float aPoint2 = static_cast<float>(iCrimeAlarmRecDistance.Int());
		const NiPoint3* v4 = &apActor->GetLocationOnReference();
		TESObjectCELL::EnumReferencesCloseToPointFromInterior(apActor->GetParentCell(), *v4, aPoint2, *v9, apfnEnumFunc,
			&HighProcess::FindDoorsToSendYellAlarm, pcrime);
	} else {
		float apfnEnumFunca = static_cast<float>(iCrimeAlarmRecDistance.Int());
		const NiPoint3* v10 = &apActor->GetLocationOnReference();
		float aPoint2a = static_cast<float>(iCrimeAlarmRecDistance.Int());
		const NiPoint3* v6 = &apActor->GetLocationOnReference();
		TESObjectCELL* ParentCell = apActor->GetParentCell();
		pDataHandler->EnumReferencesCloseToPoint(ParentCell, *v6, aPoint2a, *v10, apfnEnumFunca,
			&HighProcess::FindDoorsToSendYellAlarm, pcrime);
	}
	BSSimpleList<TESObjectREFR*>* pRefList = ReferenceProcessLists.SendActorsYellAlarm(pcrime, &DoorRefListCrime);
	DoorRefListCrime.RemoveAll();
	BSSimpleList<TESObjectREFR*>* pListRef = pRefList;
	while (pListRef && pListRef->m_item) {
		Actor* pSendactor = 0;
		TESObjectREFR* m_item = pListRef->m_item;
		if (m_item->IsActor()) {
			pSendactor = static_cast<Actor*>(pListRef->m_item);
		}
		pListRef = pListRef->m_pkNext;
		FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
		if (pSendactor) {
			if (!pSendactor->IsAlarmed() && !pSendactor->IsSleeping() && !pSendactor->IsInCombat() &&
				(!pSendactor->IsActoraGuard() || !pcrime->bguardrespond) &&
				((pSendactor->IsActoraGuard() && !pnpc->IsInEvilFactionsOnly()) ||
					(!pSendactor->IsActoraGuard() && pSendactor->GetShouldAttackActor(pCriminal, 0, ereaction, 0)))) {
				bool bSendAlarm = false;
				if (pSendactor->pCurrentProcess && bSendAlarm) {
					pSendactor->InitiateAlarm(pcrime, 0, true, nullptr);
					if (pSendactor->GetCurrentProcessType()) {
						// fsub qword [0x1016FF0], which holds 1.5.
						float fTime = appCalendar.GetHour() - 1.5;
						pSendactor->pCurrentProcess->SetLastTimeProcessed(fTime);
						PROCESS_TYPE eOldType = pSendactor->GetCurrentProcessType();
						ReferenceProcessLists.RemoveReference(pSendactor, eOldType);
						PROCESS_TYPE eType = pSendactor->GetCurrentProcessType();
						ReferenceProcessLists.AddReference(pSendactor, eType, 0, 0, 0);
					}
				}
			}
		}
	}
	if (pRefList) {
		pRefList->RemoveAll();
	}
	if (pRefList) {
		delete pRefList;
	}
}

void HighProcess::SetNumberGuardsArresting(int32_t ivalue) {
	iNumberGuardsPersuing = ivalue;
}

int32_t HighProcess::CalculateMoveMode(Actor* apActor, float afdistance, float afshortdist, float aflongdist,
	bool balarmcombat, bool abpuretravel) {
	apActor->GetMoveMode();
	TESPackage* pPackage = GetPackageThatIsRunning();
	if (apActor->IsRestrained() || apActor->IsUnConscious()) {
		return (uint32_t)0;
	}
	if (apActor->GetForceRun() || ((pPackage && pPackage->GetAlwaysRun()) || apActor->IsInCombat())) {
		return (uint32_t)513;
	}
	if (balarmcombat) {
		return (uint32_t)513;
	}
	if (abpuretravel) {
		return (uint32_t)257;
	}
	if ((apActor->GetMoveMode() & 512) != 0) {
		if (!(afshortdist > ((double)afdistance))) {
			return (uint32_t)513;
		} else {
			return (uint32_t)257;
		}
	} else {
		if (!(aflongdist < ((double)afdistance))) {
			return (uint32_t)257;
		} else {
			return (uint32_t)513;
		}
	}
}

void HighProcess::SetActorsAnimation(Actor* apActor, int32_t emovemode, bool abCheckAlwaysRun) {
	int16_t moveMode;
	TESPackage* v6;
	if (!GetMovementStoped()) {
		v6 = GetPackageThatIsRunning();
		if (((int32_t)emovemode) == ((-1))) {
			emovemode = static_cast<uint32_t>((static_cast<uint32_t>(emovemode) & 0xffff0000u) |
				static_cast<uint32_t>((uint16_t)256));
		}
		if (abCheckAlwaysRun) {
			if (v6) {
				if (v6->GetAlwaysRun()) {
					emovemode = static_cast<uint32_t>((static_cast<uint32_t>(emovemode) & 0xffff0000u) |
						static_cast<uint32_t>((uint16_t)513));
				}
			}
		}
		moveMode = (int16_t)apActor->GetMoveMode();
		apActor->SetMoveMode((int32_t)((int16_t)(((int32_t)emovemode) | (moveMode & 3072))));
	}
}

void HighProcess::FinishDying(Actor* apActor) {
	double v2;
	TESAnimGroup* animGroup;
	ExtraDataList* extra;
	float fSecondsPassed;
	Animation* v7;
	BSAnimGroupSequence* currentSequence;
	float lastProcessHour;
	float v10;
	float hour;
	NiNode* v12 = apActor->TESObjectREFR::Get3D();
	uint8_t isActive = (uint8_t)bhkWorld::IsActive(v12, 1, 0);
	if (isActive) {
		lastProcessHour = (float)BaseProcess::GetLastTimeProcessed();
		hour = (float)(&appCalendar)->GetHour();
		if (!(lastProcessHour > ((double)hour))) {
			v2 = (double)(hour - lastProcessHour);
		} else {
			v2 = (lastProcessHour + 23.0) - hour;
		}
		v10 = (float)v2;
		isActive = (uint8_t)(v10 < ((&appCalendar)->GetTimeScale() * 0.0005000000237487257));
	}
	if (GetAnimation()) {
		v7 = GetAnimation();
		currentSequence = v7->GetCurrentSequence(AGS_MOVEMENT);
		if (currentSequence) {
			animGroup = currentSequence->GetAnimGroup();
			if (animGroup->GetType() == 224) {
				fSecondsPassed = (float)Animation::GetDuration(currentSequence);
				v7->Update(apActor, fSecondsPassed, (-0x1p+0f));
			}
		}
	}
	if (!isActive) {
		apActor->DoDeathStuff();
		apActor->DetachHavok();
		apActor->SetHavokDeath((bool)1);
		extra = apActor->TESObjectREFR::GetExtra();
		extra->SetRagDollData(apActor);
	}
}

bool HighProcess::GetWaitingforLipFile() {
	return bWaitingForLipFile;
}

void HighProcess::SetWaitingforLipFile(bool abflag) {
	bWaitingForLipFile = abflag;
}

void HighProcess::SetLipFileFailed(bool abflag) {
	bLipFileFailed = abflag;
}

bool HighProcess::GetLipFileFailed() {
	return bLipFileFailed;
}

LipSynchAnim* HighProcess::GetLipAnim() {
	return pLipSynicAnim;
}

void HighProcess::SetLipAnim(LipSynchAnim* apAnim) {
	pLipSynicAnim = apAnim;
}

void HighProcess::SetPlantedExplosive(bool abFlag) {
	bPlantedExplosive = abFlag;
}

bool HighProcess::GetPlantedExplosive() {
	return bPlantedExplosive;
}

void HighProcess::FreeUpSpecialIdle(Actor* apActor) {
	AddPostAnimationAction(POSTANIM_FREEUP_SPECIAL_IDLE);
}

// Calls GetActorFloatValue through the process vtable, then floors the returned float to an integer.
int32_t HighProcess::GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) {
	MiddleLowProcess* pProcess = this;
	float fValue = pProcess->GetActorFloatValue(apActorBase, aiWhich, apActor);
	return static_cast<int32_t>(FloatFloor(fValue));
}

float HighProcess::GetActorFloatValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) {
	ActorValueCache* v6;
	void* v8;
	if (!pActorValueCache) {
		v8 = ourMalloc(sizeof(ActorValueCache));
		if (v8) {
			v6 = ::new (v8) ActorValueCache;
		} else {
			v6 = nullptr;
		}
		pActorValueCache = v6;
	}
	if (!pActorValueCache->IsCachedActorValueOutOfDate(aiWhich)) {
		return pActorValueCache->GetCachedActorValue(aiWhich);
	}
	float afValue = MiddleLowProcess::GetActorFloatValue(apActorBase, aiWhich, apActor);
	pActorValueCache->SetCachedActorValue(aiWhich, afValue);
	return pActorValueCache->GetCachedActorValue(aiWhich);
}

void HighProcess::TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) {
	ActorValueCache* v4;
	void* v6;
	MiddleLowProcess::TempModActorValue(apActor, aiWhich, afValue);
	if (!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) {
		if (!pActorValueCache) {
			v6 = ourMalloc(sizeof(ActorValueCache));
			if (v6) {
				v4 = ::new (v6) ActorValueCache;
			} else {
				v4 = nullptr;
			}
			pActorValueCache = v4;
		}
		pActorValueCache->SetCachedActorValueOutOfDate(aiWhich);
	}
}

void HighProcess::TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) {
	ActorValueCache* v4;
	void* v6;
	MiddleLowProcess::TempModActorValue(apActor, aiWhich, aiValue);
	if (!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) {
		if (!pActorValueCache) {
			v6 = ourMalloc(sizeof(ActorValueCache));
			if (v6) {
				v4 = ::new (v6) ActorValueCache;
			} else {
				v4 = nullptr;
			}
			pActorValueCache = v4;
		}
		pActorValueCache->SetCachedActorValueOutOfDate(aiWhich);
	}
}

void HighProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) {
	ActorValueCache* v4;
	void* v6;
	MiddleLowProcess::DamageModActorValue(apActor, aiWhich, afValue);
	if (!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) {
		if (!pActorValueCache) {
			v6 = ourMalloc(sizeof(ActorValueCache));
			if (v6) {
				v4 = ::new (v6) ActorValueCache;
			} else {
				v4 = nullptr;
			}
			pActorValueCache = v4;
		}
		pActorValueCache->SetCachedActorValueOutOfDate(aiWhich);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) {
	LowProcess::DamageModActorValue(apActor, aiWhich, aiValue);
	if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
		if (!pActorValueCache) {
			ActorValueCache* v4;
			ActorValueCache* apMemory = static_cast<ActorValueCache*>(ourMalloc(sizeof(ActorValueCache)));
			if (apMemory) {
				v4 = ::new (apMemory) ActorValueCache;
			} else {
				v4 = 0;
			}
			pActorValueCache = v4;
		}
		pActorValueCache->SetCachedActorValueOutOfDate(aiWhich);
	}
}

void HighProcess::SetCachedActorValueOutOfDate(ActorValue::Index aiWhich) {
	ActorValueCache* v2;
	void* v4;
	if (!pActorValueCache) {
		v4 = ourMalloc(sizeof(ActorValueCache));
		if (v4) {
			v2 = ::new (v4) ActorValueCache;
		} else {
			v2 = nullptr;
		}
		pActorValueCache = v2;
	}
	if (pActorValueCache) {
		pActorValueCache->SetCachedActorValueOutOfDate(aiWhich);
	}
}

ANIMATION_ACTION HighProcess::GetAnimAction() {
	return static_cast<ANIMATION_ACTION>(sAnimAction);
}

BSAnimGroupSequence* HighProcess::GetAnimActionAnimSeq() {
	return pAnimSeq;
}

void HighProcess::SetAnimAction(ANIMATION_ACTION aeAction, BSAnimGroupSequence* apAnimSeq) {
	sAnimAction = static_cast<int16_t>(aeAction);
	pAnimSeq = apAnimSeq;
}

bool HighProcess::GetAutomaticFireAtLeastOnce() {
	return bAutomaticFireAtLeastOne;
}

void HighProcess::SetAutomaticFireAtLeastOnce(bool abVal) {
	bAutomaticFireAtLeastOne = abVal;
}

bool HighProcess::CanAttack() {
	Animation* pAnimation = GetAnimation();
	switch (GetAnimAction()) {
	case ANIM_ACTION_NONE:
	case ANIM_ACTION_ATTACK_WAIT_FOR_EJECT:
	case ANIM_ACTION_ATTACK_FOLLOW_THROUGH:
	case ANIM_ACTION_ATTACK_THROW_ATTACH:
	case ANIM_ACTION_BLOCK:
	case ANIM_ACTION_RELOAD_LOOP:
		break;
	case ANIM_ACTION_ATTACK:
	case ANIM_ACTION_ATTACK_THROW:
		if (pAnimation) {
			BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(AGS_LEFT_ARM);
			if (pSequence) {
				int32_t eType = pSequence->GetAnimGroup()->GetType();
				if (eType == ANIM_GROUP_PIPBOY || eType == ANIM_GROUP_PIPBOY_CHILD)
					return false;
			}
		}
		break;
	default:
		return false;
	}
	if (pAnimation && pAnimation->ReloadAnimQueued())
		return false;
	if (this == pPlayer->GetCurrentAIProcess()) {
		if (pPlayer->IsPipboyActive())
			return false;
	} else if (pAnimation) {
		TESIdleForm* pIdle = pAnimation->GetSpecialIdleForm();
		if (pIdle && pIdle->GetNoAttacking())
			return false;
	}
	return true;
}

bool HighProcess::CanForceGreet(Actor* apActor) {
	uint8_t result;
	TESPackage* currentPackage;
	uint8_t eSitSleepState;
	if ((&pVATS)->IsInVats()) {
		return (bool)0;
	}
	if (!apActor->Has360LOS(pPlayer)) {
		return (bool)0;
	}
	if (pPlayer->GetCurrentPackage()) {
		currentPackage = pPlayer->GetCurrentPackage();
		if (((int32_t)currentPackage->GetPackType()) == 6) {
			return (bool)0;
		}
	}
	if (pPlayer->GetSitSleepState() && (((int32_t)pPlayer->GetSitSleepState()) != 4)) {
		return (bool)0;
	}
	TESObjectCELL* parentCell = pPlayer->TESObjectREFR::GetParentCell();
	NiPoint3* mBRoomAccumList = pPlayer->TESObjectREFR::QLocationOnReference();
	float v8 = (float)pPlayer->IsPointDeepUnderWater(mBRoomAccumList->z, parentCell);
	if (v8 >= 0.875) {
		return (bool)0;
	}
	switch (sAnimAction) {
	case -1:
	case 7: {
		eSitSleepState = cSitSleepState;
		result = (uint8_t)((!eSitSleepState) || ((eSitSleepState == 4) || (eSitSleepState == 9)));
		break;
	}
	case 2:
	case 4:
	case 5: {
		if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
			PlayerCharacter::bReleaseTheArrow = (bool)1;
		}
		result = 0;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return (bool)result;
}

void HighProcess::AttackCallback(int32_t aiNumAttacks) {
	sShotsFired = static_cast<int16_t>(sShotsFired + aiNumAttacks);
}

void HighProcess::SaveWeaponLastPos(NiPoint3 aPos) {
	WeaponLastPos = aPos;
}

const NiPoint3* HighProcess::GetWeaponLastPos() {
	return &WeaponLastPos;
}

void HighProcess::Update3dModel(Actor* apActor) {
	uint8_t cFlags = GetAll3DUpdateFlags();
	if (!apActor->IsCreature() && cFlags) {
		NiNode* pRoot = apActor->Get3D();
		if (pRoot) {
			if ((cFlags & 4) || (cFlags & 8)) {
				if (pRoot->IsNode()) {
					Animation* pAnimation = GetAnimation();
					NiDefaultAVObjectPalette* pPalette = nullptr;
					if (pAnimation && pAnimation->GetControllerManager()) {
						pPalette = static_cast<NiDefaultAVObjectPalette*>(pAnimation->GetControllerManager()
								->GetObjectPalette());
					}
					BSFaceGenNiNode* pFaceNode = apActor->GetFaceNodeSkinned(nullptr);
					if (pFaceNode && pFaceNode->GetParent()) {
						SetFaceSkinnedNode(nullptr);
						RecurseAndRemoveObjectsFromPalette(pFaceNode, pPalette);
						pFaceNode->GetParent()->DetachChild(pFaceNode);
					}
					pFaceNode = apActor->GetFaceNodeBiped(nullptr);
					if (pFaceNode && pFaceNode->GetParent()) {
						SetFaceNode(nullptr);
						SetHeadAnims(nullptr);
						RecurseAndRemoveObjectsFromPalette(pFaceNode, pPalette);
						pFaceNode->GetParent()->DetachChild(pFaceNode);
					}
				}
				TESNPC* pNPC = static_cast<TESNPC*>(apActor->GetActorBase());
				if (cFlags & 4) {
					pNPC->ClearHead();
				}
				ClearFaceAnimationData();
				// The exe calls Character's biped getter (0x5D9F90) directly on the actor.
				BipedAnim* pBiped = static_cast<Character*>(apActor)->Character::GetBiped();
				BSFaceGenNiNode* pSkinnedNode = nullptr;
				BSFaceGenNiNode* pBipedNode = pSkinnedNode;
				pNPC->InitHead(pBipedNode, pSkinnedNode);
				pNPC->AttachHead(apActor, pBiped, pBipedNode, pSkinnedNode);
				// The player's base NPC (form ID 7).
				if (pNPC->GetFormID() == 7) {
					pNPC->TESRaceForm::GetFormRace()->KillEGTData();
				}
			}
			if (cFlags & 1) {
				apActor->SetFireNode(nullptr);
				// The exe calls Get3D before GetShadowSceneNode, at both scene-graph calls.
				NiAVObject* pOld3D = apActor->Get3D();
				BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveObject(pOld3D);
				bool bResetUpdateSkins = false;
				if ((cFlags & 2) && !bBipedAnimUpdateSkins) {
					bBipedAnimUpdateSkins = true;
					bResetUpdateSkins = true;
				}
				apActor->ReplaceModel();
				if (bResetUpdateSkins) {
					bBipedAnimUpdateSkins = false;
				}
				NiAVObject* pNew3D = apActor->Get3D();
				BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddObject(pNew3D);
				TESNPC* pNPCObject = nullptr;
				if (apActor->IsNpc()) {
					pNPCObject = static_cast<TESNPC*>(apActor->TESObjectREFR::GetObjectReference());
				}
				BSFaceGenAnimationData* pFaceAnim = apActor->Actor::GetFaceAnimationData();
				if (pFaceAnim) {
					pFaceAnim->SetDead(apActor->IsDead(false), true);
				}
				if (pNPCObject) {
					pNPCObject->FixNPCNormals(apActor);
				}
				SetShouldRefreshMagicShadersEffects(true, true, false);
				if (BSShaderManager::IsRefractionEnabled() && apActor->IsRefractive()) {
					apActor->SetRefraction(true, apActor->GetExtra()->GetRefractionPropertyExtra()->fRefractionPower);
				} else {
					apActor->UpdateAlpha();
				}
				if (apActor == pPlayer && Interface::GetInventoryMenuVisible()) {
					pPlayer->UpdateCurrentDebugGeometry();
				} else {
					NiNode* pRoot3D = apActor->Get3D();
					NiAVObject* pObject = pRoot3D;
					if (apActor == pPlayer && !pPlayer->Is3rdPerson()) {
						pObject = pRoot3D->GetAtChecked(0);
					}
					apActor->InitLightingPropertyPtr(pObject);
					if (apActor != pPlayer) {
						uint32_t uiFormID = apActor->GetFormID();
						SetReferenceIDOnScenegraph(apActor->Get3D(), uiFormID);
					}
				}
			}
		}
	}
	if (cFlags & 0x10) {
		apActor->SetScale(apActor->fRefScale);
	}
	ClearAll3DUpdateFlags();
	if (apActor != pPlayer) {
		apActor->HandleTextureUpgrade(false);
	}
}

TESObjectREFR* HighProcess::GetLastGreeted() {
	return pGreetActor;
}

void HighProcess::SetLastGreeted(TESObjectREFR* apRef) {
	pGreetActor = apRef;
}

// KEEP: the exe calls BSSoundHandle::operator= (0x418900) on the caller's hidden result buffer without constructing
// it first; a by-value return in src either constructs a local (0x41A250) or copies the fields inline, and giving
// the virtual an explicit result pointer would change the whole BaseProcess family.
BSSoundHandle HighProcess::GetSoundHandle(ACTOR_SOUND_HANDLE aeType) {
	return SoundHandle[aeType];
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::SetSoundHandle(ACTOR_SOUND_HANDLE aeType, BSSoundHandle aSound) {
	SoundHandle[aeType].operator=(aSound);
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::StopSoundHandle(ACTOR_SOUND_HANDLE aeType) {
	FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
	if (SoundHandle[aeType].IsValid()) {
		if (SoundHandle[aeType].IsPlaying()) {
			SoundHandle[aeType].Stop();
		}
		SoundHandle[aeType].Release();
		BSSoundHandle* arHandle = ::new (&th_s) BSSoundHandle;
		SoundHandle[aeType].operator=(*arHandle);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::StartTorchSound(Actor* apActor) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
	if (!SoundHandle[1].IsValid()) {
		TESSound* psound = pDataHandler->GetSound("ITMTorchHeldLP");
		if (apActor) {
			if (psound) {
				uint32_t CastingType = psound->iFormID;
				BSSoundHandle* arHandle = apActor->PlaySound(&a2, CastingType, 1, 2u, 1);
				SoundHandle[1].operator=(*arHandle);
			}
		}
	}
}

bool HighProcess::GetMovementStoped() {
	return bStop;
}

void HighProcess::ClearMovementStoped() {
	bStop = false;
}

void HighProcess::SetDetectionModifier(float afvalue) {
	SetDetectionModifierTimer();
	fDetectionModifer = afvalue;
}

void HighProcess::SetDetectionModifierTimer() {
	fDetectionModifierTimer = (float)((&fDetectionActionTimer)->Float());
}

float HighProcess::GetDetectionModifier() {
	return fDetectionModifer;
}

float HighProcess::GetGreetingTimer() {
	return fGreetingTimer;
}

void HighProcess::SetGreetingTimer(float ftime) {
	fGreetingTimer = ftime;
}

bool HighProcess::IsTalking(MobileObject* apActor) {
	bool bResult = false;
	if (apActor && apActor->IsinDialogue()) {
		TESPackage* pRunningPackage = GetPackageThatIsRunning();
		DialoguePackage* pDialoguePackage = nullptr;
		if (pRunningPackage && pRunningPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)
			pDialoguePackage = static_cast<DialoguePackage*>(pRunningPackage);
		if (pDialoguePackage && pDialoguePackage->pTalkingActor == apActor)
			bResult = true;
	} else if (SoundHandle[0].IsValid() || GetGreetingFlag()) {
		bResult = true;
	}
	return bResult;
}

bool HighProcess::ContinuingPackageforPC() {
	return bContinuingPackageforPC;
}

void HighProcess::SetContinuingPackage(bool abflag) {
	bContinuingPackageforPC = abflag;
}

// The detection of apActor in the detected-actor list (abWhichList 0) or the list of those who detect this actor.
// The exe calls BSSimpleList's GetItem (0x6815C0) twice per node and GetNext (0x726070) out of line, and keeps
// walking to the end of the list's items only while nothing matched.
DetectionState* HighProcess::GetDetectionState(Actor* apActor, int32_t abWhichList) {
	DetectionState* pFound = nullptr;
	BSSimpleList<DetectionState*>* pNode = abWhichList ? pActorsWhoDetectMeList : pDetectedActorList;
	while (pNode) {
		DetectionState* pItem;
		pItem = pNode->GetItem();
		if (!pItem || pFound) {
			break;
		}
		DetectionState* pState;
		pState = pNode->GetItem();
		if (pState->pActor == apActor) {
			pFound = pState;
		}
		pNode = pNode->GetNext();
	}
	return pFound;
}

float HighProcess::GetLightLevelTimer() {
	return fLightLevelTimer;
}

void HighProcess::SetLightLevelTimer(float afTimer) {
	fLightLevelTimer = afTimer;
}

void HighProcess::SetPackageEndTimerValue(float fTime) {
	fScriptPackageEndTime = fTime;
}

float HighProcess::GetPackageEndTimerValue() {
	return fScriptPackageEndTime;
}

uint16_t HighProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
	uint16_t SaveSize = MiddleHighProcess::GetSaveSize(aiFlags, pmob);
	uint16_t v7 = SaveSize;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		SaveSize += 6;
	}
	int16_t v9 = SaveSize + 56;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
		v9 += 4;
	}
	int16_t v10 = v9 + 43;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
		v10 += 5;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
		v10 += 5;
	}
	int16_t v11 = 13 * pDetectedActorList->ItemsInList() + (v10 + 14);
	if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
		++v11;
	}
	uint16_t isize = GetCharControllerSaveSize(pmob) + (v11 + 2);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 90u) {
		isize += 5;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 93u) {
		isize += 8;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
		isize += 4;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
		isize += 24;
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize - v7, pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 0x3152,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize - v7, 0x3152,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp");
		}
	}
	return isize;
}

void HighProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	bool bhasDetectionEvent;
	bool bhasGreetTopic;
	bool bhasLipSyncAnim;
	MiddleHighProcess::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(&bGreetingFlag, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bHeadTrack, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bContinuingPackageforPC, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bActivateAnim, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iLastDetection, 2u, 0);
	apSaveGameBuffer->SaveDataEndian(&fPackageEvalTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDetectionTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fSoundDelay, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fGreetingTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fIdleTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDetectGreetTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fAwarePlayerTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDetectListTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fUseItemTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fHoldAttackTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fIdleChatterTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&sShotsFired, 2u, 0);
	apSaveGameBuffer->SaveDataEndian(&sShotsToFire, 2u, 0);
	apSaveGameBuffer->SaveDataEndian(&sBurstsFired, 2u, 0);
	apSaveGameBuffer->SaveDataEndian(&bWeaponAlertDrawn, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&WeaponLastPos, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&eLastSpeakingEmotion, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&eFadeState, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fFadeAlpha, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fBreathTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTalkTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fScriptPackageEndTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bStop, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iNumberGuardsPersuing, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bUnequippedArmorToSwim, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fReEquipArmorTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bForceRotate, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDetectionModifer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDetectionModifierTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bCheckDeadTalk, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDelayTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDistanceMoved, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTurnTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&cLastTurn, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&cLastTurnDir, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fEvaluateAcquireTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fVoiceTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&cLastTurnDir, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&cLastTurnDir, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iDetectionCounter, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTakeBackTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bProcessGreetSayTo, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fMaxAlpha, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fCheckToTalkTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fHeadTrackTargetTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fRadiationTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fHighestRadiation, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bPlantedExplosive, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bNeedTalkPlayer, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fAvoidWaitTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bHiding, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&eSpecialIdleType, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bFinishingCombatPackage, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIsDoingSayTo, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fClearTalkToListTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bWaitingForLipFile, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bLipFileFailed, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bLipQuequed, 1u, 0);
	bhasLipSyncAnim = pLipSynicAnim != 0;
	apSaveGameBuffer->SaveDataEndian(&bhasLipSyncAnim, 1u, 0);
	apSaveGameBuffer->SaveFormID(pGreetActor, 0);
	apSaveGameBuffer->SaveFormID(plastDetected, 0);
	apSaveGameBuffer->SaveFormID(pTeleportFadeRef, 0);
	apSaveGameBuffer->SaveFormID(pLastTarget, 0);
	apSaveGameBuffer->SaveFormID(pDialogTarget, 0);
	apSaveGameBuffer->SaveFormID(pIdleToPlay, 0);
	apSaveGameBuffer->SaveFormID(this->GetPathLookAtTarget(), 0);
	for (uint32_t i = 0; i < 6; ++i) {
		apSaveGameBuffer->SaveFormID(HeadTrackingTargets[i], 0);
		apSaveGameBuffer->SaveDataEndian(&HeadTrackingTargetFlags[i], 1u, 0);
	}
	uint32_t isaveGameCount = 0;
	uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
	for (Actor* pForm : AggroRadiusList) {
		if (pForm) {
			apSaveGameBuffer->SaveFormID(pForm, 0);
			++isaveGameCount;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
	uint32_t isaveGameCount_2 = 0;
	uint32_t isaveGameBufferPos_2 = apSaveGameBuffer->StartVariableSizedValue();
	for (Actor* pForm_2 : AvoidActorList) {
		if (pForm_2) {
			apSaveGameBuffer->SaveFormID(pForm_2, 0);
			++isaveGameCount_2;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_2, isaveGameBufferPos_2);
	uint32_t isaveGameCount_3 = 0;
	uint32_t isaveGameBufferPos_3 = apSaveGameBuffer->StartVariableSizedValue();
	for (Actor* pForm_3 : *pLastSpokeToList) {
		if (pForm_3) {
			apSaveGameBuffer->SaveFormID(pForm_3, 0);
			++isaveGameCount_3;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_3, isaveGameBufferPos_3);
	bhasGreetTopic = pGreetTopic != 0;
	apSaveGameBuffer->SaveDataEndian(&bhasGreetTopic, 1u, 0);
	if (bhasGreetTopic) {
		pGreetTopic->SaveGame(apSaveGameBuffer);
	}
	uint32_t isaveGameCount_4 = 0;
	uint32_t isaveGameBufferPos_4 = apSaveGameBuffer->StartVariableSizedValue();
	for (AvoidAreaStruct* pavoidArea : *pListOfAvoidAreas) {
		if (pavoidArea) {
			pavoidArea->SaveGame(apSaveGameBuffer);
			++isaveGameCount_4;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_4, isaveGameBufferPos_4);
	uint32_t isaveGameCount_5 = 0;
	uint32_t isaveGameBufferPos_5 = apSaveGameBuffer->StartVariableSizedValue();
	for (DetectionState* pdetectionState : *pDetectedActorList) {
		if (pdetectionState) {
			pdetectionState->SaveGame(apSaveGameBuffer);
			++isaveGameCount_5;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_5, isaveGameBufferPos_5);
	uint32_t isaveGameCount_6 = 0;
	uint32_t isaveGameBufferPos_6 = apSaveGameBuffer->StartVariableSizedValue();
	for (DetectionState* pdetectionState_2 : *pActorsWhoDetectMeList) {
		if (pdetectionState_2) {
			pdetectionState_2->SaveGame(apSaveGameBuffer);
			++isaveGameCount_6;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount_6, isaveGameBufferPos_6);
	bhasDetectionEvent = pActorsGeneratedDetectionEvent != 0;
	apSaveGameBuffer->SaveDataEndian(&bhasDetectionEvent, 1u, 0);
	if (bhasDetectionEvent) {
		pActorsGeneratedDetectionEvent->SaveGame(apSaveGameBuffer);
	}
	uint32_t isizePosition = apSaveGameBuffer->StartVariableSizedValue();
	uint32_t iposition1 = apSaveGameBuffer->iBufferPosition;
	SaveGame_00928880(apSaveGameBuffer);
	uint32_t iposition2 = apSaveGameBuffer->iBufferPosition;
	apSaveGameBuffer->SaveVariableSizedValue(iposition2 - iposition1, isizePosition);
}

// KEEP: the exe saves `dummy` (4 bytes at [ebp-0xC]) without ever writing it, an uninitialised read on every path.
void HighProcess::SaveGame(uint32_t aiFlags, MobileObject* pmob) {
	uint32_t idialogTargetID;
	uint32_t ilastTargetID;
	char csequenceIndex;
	uint32_t iactorID;
	uint32_t iblockHeader;
	uint32_t igreetActorID;
	uint16_t icount;
	uint32_t iteleportFadeRefID;
	uint16_t iblockSize;
	uint16_t isize;
	uint32_t dummy;
	uint32_t ilastDetectedID;
	MiddleHighProcess::SaveGame(aiFlags, pmob);
	iblockSize = 0;
	uint16_t* pblockSize = 0;
	uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		iblockHeader = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
		pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
	}
	pSaveLoadGameOLD->SaveGameDataOLD(&bGreetingFlag, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bHeadTrack, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bContinuingPackageforPC, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bActivateAnim, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&dummy, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&sAnimAction, 2u);
	pSaveLoadGameOLD->SaveGameDataOLD(&iLastDetection, 2u);
	pSaveLoadGameOLD->SaveGameDataOLD(&iLastBoneLOD, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fPackageEvalTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fDetectionTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fSoundDelay, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fGreetingTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fIdleTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fDetectGreetTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fAwarePlayerTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fDetectListTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fUseItemTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fHoldAttackTimer, 4u);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&fIdleChatterTimer, 4u);
	}
	pSaveLoadGameOLD->SaveGameDataOLD(&sShotsFired, 2u);
	pSaveLoadGameOLD->SaveGameDataOLD(&sShotsToFire, 2u);
	pSaveLoadGameOLD->SaveGameDataOLD(&sBurstsFired, 2u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bWeaponAlertDrawn, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&WeaponLastPos, 12u);
	pSaveLoadGameOLD->SaveGameDataOLD(&eLastSpeakingEmotion, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&eFadeState, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fFadeAlpha, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fBreathTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fTalkTimer, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fScriptPackageEndTime, 4u);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&bStop, 1u);
		pSaveLoadGameOLD->SaveGameDataOLD(&iNumberGuardsPersuing, 4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&bUnequippedArmorToSwim, 1u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fReEquipArmorTimer, 4u);
	}
	igreetActorID = 0;
	if (pGreetActor) {
		igreetActorID = pGreetActor->iFormID;
	}
	pSaveLoadGameOLD->SaveNumericID(&igreetActorID, 4u);
	ilastDetectedID = 0;
	if (plastDetected) {
		ilastDetectedID = plastDetected->iFormID;
	}
	pSaveLoadGameOLD->SaveNumericID(&ilastDetectedID, 4u);
	iteleportFadeRefID = 0;
	if (pTeleportFadeRef) {
		iteleportFadeRefID = pTeleportFadeRef->iFormID;
	}
	pSaveLoadGameOLD->SaveNumericID(&iteleportFadeRefID, 4u);
	icount = 0;
	uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
	pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
	for (BSSimpleList<DetectionState*>* plist = pDetectedActorList; plist && !plist->IsEmpty();
		plist = plist->m_pkNext) {
		DetectionState* pdetectionState = plist->m_item;
		iactorID = 0;
		if (pdetectionState->pActor) {
			iactorID = pdetectionState->pActor->iFormID;
		}
		pSaveLoadGameOLD->SaveNumericID(&iactorID, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&pdetectionState->eDetectionLevel, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&pdetectionState->bLineofSight, 1u);
		pSaveLoadGameOLD->SaveGameDataOLD(&pdetectionState->iLevel, 4u);
		++icount;
	}
	*pcount = icount;
	if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
		csequenceIndex = -1;
		if (GetAnimation()) {
			if (pAnimSeq) {
				for (uint32_t i = 0; i < 8; ++i) {
					Animation* v3 = GetAnimation();
					if (v3->GetCurrentSequence(static_cast<ANIM_GROUP_SECTION>(i)) == pAnimSeq) {
						csequenceIndex = i;
					}
				}
			}
		}
		pSaveLoadGameOLD->SaveGameDataOLD(&csequenceIndex, 1u);
	}
	isize = GetCharControllerSaveSize(pmob);
	pSaveLoadGameOLD->SaveGameDataOLD(&isize, 2u);
	if (isize) {
		SaveCharController(pmob);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 90u) {
		ilastTargetID = 0;
		if (pLastTarget) {
			ilastTargetID = pLastTarget->iFormID;
		}
		pSaveLoadGameOLD->SaveNumericID(&ilastTargetID, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&bForceRotate, 1u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 93u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&fDetectionModifer, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fDetectionModifierTimer, 4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
		idialogTargetID = 0;
		if (pDialogTarget) {
			idialogTargetID = pDialogTarget->iFormID;
		}
		pSaveLoadGameOLD->SaveNumericID(&idialogTargetID, 4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
		pSaveLoadGameOLD->SaveGameDataOLD(&bCheckDeadTalk, 1u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fDelayTimer, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fDistanceMoved, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fTurnTime, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&cLastTurn, 1u);
		pSaveLoadGameOLD->SaveGameDataOLD(&cLastTurnDir, 1u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fEvaluateAcquireTimer, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fVoiceTimer, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&cLastTurn, 1u);
	}
	if (bDebugSaveBuffer.Bool()) {
		uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			uint32_t iFlags = pheader->iFlags;
			const char* v4 = pform->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				BufferPointer - itempStartPosition, pheader->iFormID, v4, iFlags, 0x3200,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition,
				0x3200, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uintptr_t icurrentBufferPosition = reinterpret_cast<uintptr_t>(pSaveLoadGameOLD->m_pBuffer);
		uintptr_t istartBufferPosition = reinterpret_cast<uintptr_t>(pblockSize);
		if (icurrentBufferPosition > istartBufferPosition + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x3200);
		}
		*pblockSize = static_cast<uint16_t>(icurrentBufferPosition - istartBufferPosition);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor* pForm_3;
	Actor* pForm_2;
	Actor* pForm;
	bool bhasLipSyncAnim;
	int32_t ptempData;
	bool bhasDetectionEvent;
	bool bhasGreetTopic;
	MiddleHighProcess::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(&bGreetingFlag, 1u);
	apLoadGameBuffer->LoadData(&bHeadTrack, 1u);
	apLoadGameBuffer->LoadData(&bContinuingPackageforPC, 1u);
	apLoadGameBuffer->LoadData(&bActivateAnim, 1u);
	apLoadGameBuffer->LoadData(&iLastDetection, 2u);
	if (apLoadGameBuffer->GetVersion() < 8u) {
		apLoadGameBuffer->LoadData(&ptempData, 4u);
	}
	apLoadGameBuffer->LoadData(&fPackageEvalTimer, 4u);
	apLoadGameBuffer->LoadData(&fDetectionTimer, 4u);
	apLoadGameBuffer->LoadData(&fSoundDelay, 4u);
	apLoadGameBuffer->LoadData(&fGreetingTimer, 4u);
	apLoadGameBuffer->LoadData(&fIdleTimer, 4u);
	apLoadGameBuffer->LoadData(&fDetectGreetTimer, 4u);
	apLoadGameBuffer->LoadData(&fAwarePlayerTimer, 4u);
	apLoadGameBuffer->LoadData(&fDetectListTimer, 4u);
	apLoadGameBuffer->LoadData(&fUseItemTimer, 4u);
	apLoadGameBuffer->LoadData(&fHoldAttackTimer, 4u);
	apLoadGameBuffer->LoadData(&fIdleChatterTimer, 4u);
	apLoadGameBuffer->LoadData(&sShotsFired, 2u);
	apLoadGameBuffer->LoadData(&sShotsToFire, 2u);
	apLoadGameBuffer->LoadData(&sBurstsFired, 2u);
	apLoadGameBuffer->LoadData(&bWeaponAlertDrawn, 1u);
	apLoadGameBuffer->LoadData(&WeaponLastPos, 12u);
	apLoadGameBuffer->LoadData(&eLastSpeakingEmotion, 4u);
	apLoadGameBuffer->LoadData(&eFadeState, 4u);
	apLoadGameBuffer->LoadData(&fFadeAlpha, 4u);
	apLoadGameBuffer->LoadData(&fBreathTimer, 4u);
	apLoadGameBuffer->LoadData(&fTalkTimer, 4u);
	apLoadGameBuffer->LoadData(&fScriptPackageEndTime, 4u);
	apLoadGameBuffer->LoadData(&bStop, 1u);
	apLoadGameBuffer->LoadData(&iNumberGuardsPersuing, 4u);
	apLoadGameBuffer->LoadData(&bUnequippedArmorToSwim, 1u);
	apLoadGameBuffer->LoadData(&fReEquipArmorTimer, 4u);
	apLoadGameBuffer->LoadData(&bForceRotate, 1u);
	apLoadGameBuffer->LoadData(&fDetectionModifer, 4u);
	apLoadGameBuffer->LoadData(&fDetectionModifierTimer, 4u);
	apLoadGameBuffer->LoadData(&bCheckDeadTalk, 1u);
	apLoadGameBuffer->LoadData(&fDelayTimer, 4u);
	apLoadGameBuffer->LoadData(&fDistanceMoved, 4u);
	apLoadGameBuffer->LoadData(&fTurnTime, 4u);
	apLoadGameBuffer->LoadData(&cLastTurn, 1u);
	apLoadGameBuffer->LoadData(&cLastTurnDir, 1u);
	apLoadGameBuffer->LoadData(&fEvaluateAcquireTimer, 4u);
	apLoadGameBuffer->LoadData(&fVoiceTimer, 4u);
	apLoadGameBuffer->LoadData(&cLastTurnDir, 1u);
	apLoadGameBuffer->LoadData(&cLastTurnDir, 1u);
	apLoadGameBuffer->LoadData(&iDetectionCounter, 4u);
	iDetectionCounter %= 12u;
	apLoadGameBuffer->LoadData(&fTakeBackTimer, 4u);
	apLoadGameBuffer->LoadData(&bProcessGreetSayTo, 1u);
	apLoadGameBuffer->LoadData(&fMaxAlpha, 4u);
	apLoadGameBuffer->LoadData(&fCheckToTalkTimer, 4u);
	apLoadGameBuffer->LoadData(&fHeadTrackTargetTimer, 4u);
	apLoadGameBuffer->LoadData(&fRadiationTimer, 4u);
	apLoadGameBuffer->LoadData(&fHighestRadiation, 4u);
	apLoadGameBuffer->LoadData(&bPlantedExplosive, 1u);
	apLoadGameBuffer->LoadData(&bNeedTalkPlayer, 1u);
	apLoadGameBuffer->LoadData(&fAvoidWaitTimer, 4u);
	apLoadGameBuffer->LoadData(&bHiding, 1u);
	apLoadGameBuffer->LoadData(&eSpecialIdleType, 4u);
	if (apLoadGameBuffer->GetVersion()) {
		apLoadGameBuffer->LoadData(&bFinishingCombatPackage, 1u);
	}
	if (apLoadGameBuffer->GetVersion() >= 10u) {
		apLoadGameBuffer->LoadData(&bIsDoingSayTo, 1u);
	}
	if (apLoadGameBuffer->GetVersion() >= 18u) {
		apLoadGameBuffer->LoadData(&fClearTalkToListTimer, 4u);
	}
	if (apLoadGameBuffer->GetVersion() >= 19u) {
		apLoadGameBuffer->LoadData(&bWaitingForLipFile, 1u);
		apLoadGameBuffer->LoadData(&bLipFileFailed, 1u);
		apLoadGameBuffer->LoadData(&bLipQuequed, 1u);
		bhasLipSyncAnim = 0;
		apLoadGameBuffer->LoadData(&bhasLipSyncAnim, 1u);
		if (bWaitingForLipFile || bhasLipSyncAnim) {
			bWaitingForLipFile = 0;
			bLipFileFailed = 1;
		}
	}
	apLoadGameBuffer->LoadFormID(pGreetActor);
	apLoadGameBuffer->LoadFormID(plastDetected);
	apLoadGameBuffer->LoadFormID(pTeleportFadeRef);
	apLoadGameBuffer->LoadFormID(pLastTarget);
	apLoadGameBuffer->LoadFormID(pDialogTarget);
	uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
	pIdleToPlay = dynamic_cast<TESIdleForm*>(FormByNumericID);
	if (apLoadGameBuffer->GetVersion() >= 14u) {
		apLoadGameBuffer->LoadFormID(pPathLookAtTarget);
	}
	uint32_t iheadTrackTargetCount = 6;
	if (apLoadGameBuffer->GetVersion() < 4u) {
		iheadTrackTargetCount = 5;
	}
	for (uint32_t i = 0; i < iheadTrackTargetCount; ++i) {
		apLoadGameBuffer->LoadFormID(HeadTrackingTargets[i]);
		apLoadGameBuffer->LoadData(&HeadTrackingTargetFlags[i], 1u);
	}
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
		pForm = 0;
		apLoadGameBuffer->LoadFormID(pForm);
		AggroRadiusList.AddHead(pForm);
	}
	uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop_2 = 0; iloop_2 < isaveGameCount_2; ++iloop_2) {
		pForm_2 = 0;
		apLoadGameBuffer->LoadFormID(pForm_2);
		AvoidActorList.AddHead(pForm_2);
	}
	if (apLoadGameBuffer->GetVersion() >= 18u) {
		uint32_t isaveGameCount_3 = apLoadGameBuffer->LoadVariableSizedValue();
		for (uint32_t iloop_3 = 0; iloop_3 < isaveGameCount_3; ++iloop_3) {
			pForm_3 = 0;
			apLoadGameBuffer->LoadFormID(pForm_3);
			pLastSpokeToList->AddHead(pForm_3);
		}
	}
	bhasGreetTopic = 0;
	apLoadGameBuffer->LoadData(&bhasGreetTopic, 1u);
	if (bhasGreetTopic) {
		DialogueItem* v9;
		DialogueItem* v16 = static_cast<DialogueItem*>(ourMalloc(sizeof(DialogueItem)));
		if (v16) {
			v9 = ::new (v16) DialogueItem;
		} else {
			v9 = 0;
		}
		pGreetTopic = v9;
		pGreetTopic->LoadGameBGS(apLoadGameBuffer);
	}
	uint32_t isaveGameCount_4 = apLoadGameBuffer->LoadVariableSizedValue();
	if (isaveGameCount_4) {
		if (pListOfAvoidAreas) {
			pListOfAvoidAreas->RemoveAll();
		} else {
			BSSimpleList<AvoidAreaStruct*>* v8;
			BSSimpleList<AvoidAreaStruct*>* v15 =
				static_cast<BSSimpleList<AvoidAreaStruct*>*>(ourMalloc(sizeof(BSSimpleList<AvoidAreaStruct*>)));
			if (v15) {
				v8 = ::new (v15) BSSimpleList<AvoidAreaStruct*>;
			} else {
				v8 = 0;
			}
			pListOfAvoidAreas = v8;
		}
		for (uint32_t iloop_4 = 0; iloop_4 < isaveGameCount_4; ++iloop_4) {
			AvoidAreaStruct* pavoidArea;
			AvoidAreaStruct* v14 = static_cast<AvoidAreaStruct*>(ourMalloc(sizeof(AvoidAreaStruct)));
			if (v14) {
				pavoidArea = ::new (v14) AvoidAreaStruct;
			} else {
				pavoidArea = 0;
			}
			AvoidAreaStruct* pavoidArea_2 = pavoidArea;
			pavoidArea->LoadGameBGS(apLoadGameBuffer);
			pListOfAvoidAreas->AddHead(pavoidArea_2);
		}
	}
	uint32_t isaveGameCount_5 = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop_5 = 0; iloop_5 < isaveGameCount_5; ++iloop_5) {
		DetectionState* pdetectionState;
		DetectionState* apMemory = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
		if (apMemory) {
			pdetectionState = ::new (apMemory) DetectionState;
		} else {
			pdetectionState = 0;
		}
		DetectionState* pdetectionState_2 = pdetectionState;
		pdetectionState->LoadGame(apLoadGameBuffer);
		pDetectedActorList->AddHead(pdetectionState_2);
	}
	uint32_t isaveGameCount_6 = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop_6 = 0; iloop_6 < isaveGameCount_6; ++iloop_6) {
		DetectionState* pdetectionState_3;
		DetectionState* v12 = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
		if (v12) {
			pdetectionState_3 = ::new (v12) DetectionState;
		} else {
			pdetectionState_3 = 0;
		}
		DetectionState* pdetectionState_4 = pdetectionState_3;
		pdetectionState_3->LoadGame(apLoadGameBuffer);
		pActorsWhoDetectMeList->AddHead(pdetectionState_4);
	}
	bhasDetectionEvent = 0;
	apLoadGameBuffer->LoadData(&bhasDetectionEvent, 1u);
	if (bhasDetectionEvent) {
		DetectionEvent* v4;
		DetectionEvent* v11 = static_cast<DetectionEvent*>(ourMalloc(sizeof(DetectionEvent)));
		if (v11) {
			v4 = ::new (v11) DetectionEvent;
		} else {
			v4 = 0;
		}
		pActorsGeneratedDetectionEvent = v4;
		pActorsGeneratedDetectionEvent->LoadGame(apLoadGameBuffer);
	}
	pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_CHAR_CONTROLLER, apLoadGameBuffer, 0);
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) {
	uint32_t ilastDefaultTargetID;
	uint32_t idialogTargetID;
	uint32_t ilastTargetID;
	char csequenceIndex;
	uint32_t iactorID;
	uint8_t ccount;
	uint32_t iitemBeingUsedID;
	uint32_t itrackObjectID;
	bool btemp_2;
	bool btemp;
	uint32_t iblockHeader;
	uint32_t igreetActorID;
	uint16_t icount;
	uint32_t iteleportFadeRefID;
	uint16_t iblockSize;
	uint16_t isize;
	uint32_t dummy;
	uint32_t ilastDetectedID;
	MiddleHighProcess::LoadGame(aiFlags, aiCurrentFlags, pmob);
	iblockSize = 0;
	uint8_t* istartBufferPosition = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
		if (iblockHeader != 0x424C4F4B) {
			LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pheader) {
				TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
						"loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x320D, pheader->iFormID,
					pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
						"version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x320D,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() < 62u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&btemp, 1u);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&bGreetingFlag, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bHeadTrack, 1u);
	if (pSaveLoadGameOLD->GetCurrentVersion() < 25u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&btemp_2, 1u);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&bContinuingPackageforPC, 1u);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 27u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&bActivateAnim, 1u);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&dummy, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&sAnimAction, 2u);
	pSaveLoadGameOLD->LoadGameDataOLD(&iLastDetection, 2u);
	pSaveLoadGameOLD->LoadGameDataOLD(&iLastBoneLOD, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fPackageEvalTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fDetectionTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fSoundDelay, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fGreetingTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fIdleTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fDetectGreetTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fAwarePlayerTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fDetectListTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fUseItemTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fHoldAttackTimer, 4u);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 50u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&fIdleChatterTimer, 4u);
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&sShotsFired, 2u);
	pSaveLoadGameOLD->LoadGameDataOLD(&sShotsToFire, 2u);
	pSaveLoadGameOLD->LoadGameDataOLD(&sBurstsFired, 2u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bWeaponAlertDrawn, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&WeaponLastPos, 12u);
	pSaveLoadGameOLD->LoadGameDataOLD(&eLastSpeakingEmotion, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&eFadeState, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fFadeAlpha, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fBreathTimer, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fTalkTimer, 4u);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 20u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&fScriptPackageEndTime, 4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 63u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&bStop, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&iNumberGuardsPersuing, 4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 66u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&bUnequippedArmorToSwim, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fReEquipArmorTimer, 4u);
	}
	pSaveLoadGameOLD->LoadNumericID(&igreetActorID, 4u);
	pGreetActor = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(igreetActorID));
	if (pSaveLoadGameOLD->GetCurrentVersion() < 113u) {
		pSaveLoadGameOLD->LoadNumericID(&itrackObjectID, 4u);
	}
	pSaveLoadGameOLD->LoadNumericID(&ilastDetectedID, 4u);
	plastDetected = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(ilastDetectedID));
	if (pSaveLoadGameOLD->GetCurrentVersion() < 79u) {
		pSaveLoadGameOLD->LoadNumericID(&iitemBeingUsedID, 4u);
		TESForm* pFormCheck = TESForm::GetFormByNumericID(iitemBeingUsedID);
		pItemBeingUsed = pFormCheck->IsBoundObject() ? static_cast<TESBoundObject*>(pFormCheck) : 0;
	}
	pSaveLoadGameOLD->LoadNumericID(&iteleportFadeRefID, 4u);
	pTeleportFadeRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iteleportFadeRefID));
	for (DetectionState*& rpItem : pDetectedActorList->ItemsUntilNull()) {
		ourFree(rpItem);
	}
	pDetectedActorList->RemoveAll();
	icount = 0;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 78u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() < 78u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&ccount, 1u);
		icount = ccount;
	}
	for (uint32_t i = 0; i < icount; ++i) {
		DetectionState* pdetectionState;
		DetectionState* v14 = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
		if (v14) {
			pdetectionState = ::new (v14) DetectionState;
		} else {
			pdetectionState = 0;
		}
		DetectionState* pdetectionState_2 = pdetectionState;
		pSaveLoadGameOLD->LoadNumericID(&iactorID, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&pdetectionState_2->eDetectionLevel, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&pdetectionState_2->bLineofSight, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&pdetectionState_2->iLevel, 4u);
		pdetectionState_2->pActor = reinterpret_cast<Actor*>(static_cast<uintptr_t>(iactorID));
		pDetectedActorList->AddHead(pdetectionState_2);
	}
	if (pmob == pPlayer) {
		for (DetectionState*& rpItem : pDetectedActorList->ItemsUntilNull()) {
			ourFree(rpItem);
		}
		pDetectedActorList->RemoveAll();
	}
	pAnimSeq = 0;
	if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
		pSaveLoadGameOLD->LoadGameDataOLD(&csequenceIndex, 1u);
		if (csequenceIndex == -1) {
			pAnimSeq = 0;
		} else {
			// The exe keeps the sequence index plus 8 in the pointer field, not a pointer.
			pAnimSeq = reinterpret_cast<BSAnimGroupSequence*>(static_cast<intptr_t>(csequenceIndex + 8));
		}
	} else {
		if (pSaveLoadGameOLD->GetCurrentVersion() < 28u && pmob->IsActor()) {
			pSaveLoadGameOLD->AdvanceBuffer(1u);
		}
	}
	pSaveLoadGameOLD->LoadGameDataOLD(&isize, 2u);
	if (isize) {
		pSaveLoadGameOLD->QueueCharControllerLoad(pmob, isize);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 90u) {
		pSaveLoadGameOLD->LoadNumericID(&ilastTargetID, 4u);
		pLastTarget = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(ilastTargetID));
		pSaveLoadGameOLD->LoadGameDataOLD(&bForceRotate, 1u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 93u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&fDetectionModifer, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fDetectionModifierTimer, 4u);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
		pSaveLoadGameOLD->LoadNumericID(&idialogTargetID, 4u);
		pDialogTarget = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(idialogTargetID));
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&bCheckDeadTalk, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fDelayTimer, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fDistanceMoved, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fTurnTime, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&cLastTurn, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&cLastTurnDir, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fEvaluateAcquireTimer, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&fVoiceTimer, 4u);
		pSaveLoadGameOLD->LoadGameDataOLD(&cLastTurn, 1u);
		if (pSaveLoadGameOLD->GetCurrentVersion() < 125u) {
			for (uint32_t i_2 = 0; i_2 < 5; ++i_2) {
				pSaveLoadGameOLD->LoadNumericID(&ilastDefaultTargetID, 4u);
			}
		}
	}
	MobileObject* pactor = 0;
	if (pmob) {
		if (pmob->IsActor()) {
			pactor = pmob;
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
		if (pheader_2) {
			TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
			if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
				if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
					uint32_t iFlags = pheader_2->iFlags;
					int32_t cVersion = pheader_2->cVersion;
					const char* v5 = pform_2->GetFormEditorID();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
							"form is %08X %s with version %i and flags %08X",
						&istartBufferPosition[iblockSize] - icurrentBufferPosition,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x32FF,
						pheader_2->iFormID, v5, cVersion, iFlags);
				}
			} else {
				uint32_t v8 = pheader_2->iFlags;
				int32_t v6 = pheader_2->cVersion;
				const char* v4 = pform_2->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
						"is %08X %s with version %i and flags %08X",
					icurrentBufferPosition - &istartBufferPosition[iblockSize],
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x32FF, pheader_2->iFormID,
					v4, v6, v8);
			}
		} else {
			if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
				if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
					int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
							"%i",
						&istartBufferPosition[iblockSize] - icurrentBufferPosition,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x32FF, CurrentVersion);
				}
			} else {
				int32_t v10 = pSaveLoadGameOLD->GetCurrentVersion();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
					icurrentBufferPosition - &istartBufferPosition[iblockSize],
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\HighProcess.cpp", 0x32FF, v10);
			}
		}
	}
}

void HighProcess::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	MobileObject* v17;
	TESObjectREFR* v18;
	TESObjectREFR* v19;
	TESObjectREFR* v20;
	TESObjectREFR* v21;
	Actor* pactor = apLoadGameBuffer->GetActor();
	MiddleHighProcess::InitLoadGame(apLoadGameBuffer);
	if (pGreetActor) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGreetActor)));
		v21 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v21 = 0;
	}
	pGreetActor = v21;
	if (plastDetected) {
		TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plastDetected)));
		v20 = dynamic_cast<TESObjectREFR*>(v3);
	} else {
		v20 = 0;
	}
	plastDetected = v20;
	if (pTeleportFadeRef) {
		TESForm* v4 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTeleportFadeRef)));
		v19 = dynamic_cast<TESObjectREFR*>(v4);
	} else {
		v19 = 0;
	}
	pTeleportFadeRef = v19;
	if (pLastTarget) {
		TESForm* v5 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLastTarget)));
		v18 = dynamic_cast<TESObjectREFR*>(v5);
	} else {
		v18 = 0;
	}
	pLastTarget = v18;
	if (pDialogTarget) {
		TESForm* v6 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDialogTarget)));
		v17 = dynamic_cast<MobileObject*>(v6);
	} else {
		v17 = 0;
	}
	pDialogTarget = v17;
	if (apLoadGameBuffer->GetVersion() >= 14u) {
		TESObjectREFR* v16;
		if (this->GetPathLookAtTarget()) {
			TESForm* v7 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this
					->GetPathLookAtTarget())));
			v16 = dynamic_cast<TESObjectREFR*>(v7);
		} else {
			v16 = 0;
		}
		pPathLookAtTarget = v16;
		if (this->GetPathLookAtTarget()) {
			this->GetPathLookAtTarget()->SetTargeted(1);
		}
	}
	for (uint32_t i = 0; i < 6; ++i) {
		TESObjectREFR* v15;
		if (HeadTrackingTargets[i]) {
			TESForm* v8 =
				TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(HeadTrackingTargets[i])));
			v15 = dynamic_cast<TESObjectREFR*>(v8);
		} else {
			v15 = 0;
		}
		HeadTrackingTargets[i] = v15;
		if (HeadTrackingTargets[i]) {
			HeadTrackingTargets[i]->SetTargeted(1);
		}
	}
	BSSimpleList<Actor*>* plist = &AggroRadiusList;
	BSSimpleList<Actor*>* plastList = 0;
	while (plist && !plist->IsEmpty()) {
		Actor* pForm = plist->m_item;
		bool bremove = 0;
		if (pForm) {
			TESForm* v9 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
			pForm = dynamic_cast<Actor*>(v9);
			if (pForm) {
				plist->SetItem(pForm);
			} else {
				bremove = 1;
			}
		} else {
			bremove = 1;
		}
		if (bremove) {
			if (plastList) {
				BSSimpleList<Actor*>* v12 = plist;
				plastList->Remove(v12->m_item);
				plist = plastList->m_pkNext;
			} else {
				plist->RemoveHead();
			}
		} else {
			plastList = plist;
			plist = plist->m_pkNext;
		}
	}
	BSSimpleList<Actor*>* plist_2 = &AvoidActorList;
	BSSimpleList<Actor*>* plastList_2 = 0;
	while (plist_2 && !plist_2->IsEmpty()) {
		Actor* pForm_2 = plist_2->m_item;
		bool bremove_2 = 0;
		if (pForm_2) {
			TESForm* v10 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm_2)));
			pForm_2 = dynamic_cast<Actor*>(v10);
			if (pForm_2) {
				plist_2->SetItem(pForm_2);
			} else {
				bremove_2 = 1;
			}
		} else {
			bremove_2 = 1;
		}
		if (bremove_2) {
			if (plastList_2) {
				BSSimpleList<Actor*>* v13 = plist_2;
				plastList_2->Remove(v13->m_item);
				plist_2 = plastList_2->m_pkNext;
			} else {
				plist_2->RemoveHead();
			}
		} else {
			plastList_2 = plist_2;
			plist_2 = plist_2->m_pkNext;
		}
	}
	if (apLoadGameBuffer->GetVersion() >= 18u) {
		BSSimpleList<Actor*>* plist_3 = pLastSpokeToList;
		BSSimpleList<Actor*>* plastList_3 = 0;
		while (plist_3 && !plist_3->IsEmpty()) {
			Actor* pForm_3 = plist_3->m_item;
			bool bremove_3 = 0;
			if (pForm_3) {
				TESForm* v11 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm_3)));
				pForm_3 = dynamic_cast<Actor*>(v11);
				if (pForm_3) {
					plist_3->SetItem(pForm_3);
				} else {
					bremove_3 = 1;
				}
			} else {
				bremove_3 = 1;
			}
			if (bremove_3) {
				if (plastList_3) {
					BSSimpleList<Actor*>* v14 = plist_3;
					plastList_3->Remove(v14->m_item);
					plist_3 = plastList_3->m_pkNext;
				} else {
					plist_3->RemoveHead();
				}
			} else {
				plastList_3 = plist_3;
				plist_3 = plist_3->m_pkNext;
			}
		}
	}
	if (pGreetTopic) {
		pGreetTopic->InitItem(apLoadGameBuffer);
	}
	for (AvoidAreaStruct* pavoidArea : *pListOfAvoidAreas) {
		if (pavoidArea) {
			pavoidArea->InitLoadGame(apLoadGameBuffer);
		}
	}
	BSSimpleList<DetectionState*>* plist_5 = pDetectedActorList;
	BSSimpleList<DetectionState*>* plastList_4 = 0;
	while (plist_5 && !plist_5->IsEmpty()) {
		DetectionState* pdetectionState = plist_5->m_item;
		bool bremove_4 = 0;
		if (pdetectionState) {
			pdetectionState->InitLoadGame();
			if (!pdetectionState->pActor) {
				ourFree(pdetectionState);
				bremove_4 = 1;
			}
		} else {
			bremove_4 = 1;
		}
		if (bremove_4) {
			if (plastList_4) {
				plastList_4->Remove(plist_5->m_item);
				plist_5 = plastList_4->m_pkNext;
			} else {
				plist_5->RemoveHead();
			}
		} else {
			plastList_4 = plist_5;
			plist_5 = plist_5->m_pkNext;
		}
	}
	BSSimpleList<DetectionState*>* plist_6 = pActorsWhoDetectMeList;
	BSSimpleList<DetectionState*>* plastList_5 = 0;
	while (plist_6 && !plist_6->IsEmpty()) {
		DetectionState* pdetectionState_2 = plist_6->m_item;
		bool bremove_5 = 0;
		if (pdetectionState_2) {
			pdetectionState_2->InitLoadGame();
			if (!pdetectionState_2->pActor) {
				ourFree(pdetectionState_2);
				bremove_5 = 1;
			}
		} else {
			bremove_5 = 1;
		}
		if (bremove_5) {
			if (plastList_5) {
				plastList_5->Remove(plist_6->m_item);
				plist_6 = plastList_5->m_pkNext;
			} else {
				plist_6->RemoveHead();
			}
		} else {
			plastList_5 = plist_6;
			plist_6 = plist_6->m_pkNext;
		}
	}
	if (pActorsGeneratedDetectionEvent) {
		pActorsGeneratedDetectionEvent->InitLoadGame(apLoadGameBuffer);
	}
	if (this->GetBoneLOD()) {
		if (pactor) {
			if (pactor != pPlayer) {
				if (pactor->Get3D()) {
					NiNode* proot = pactor->Get3D();
					proot->SetNotVisible(0);
					EvaluateBoneLod(pactor);
				}
			}
		}
	}
	if (pActorValueCache) {
		pActorValueCache->ResetCachedActorValues();
	}
}

void HighProcess::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) {
	MiddleHighProcess::InitLoadGame(aiFlags, aiOldFlags, pmob);
	uint32_t igreetActorID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGreetActor));
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(igreetActorID);
	pGreetActor = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	uint32_t ilastDetectedID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plastDetected));
	TESForm* v5 = TESForm::GetFormByNumericID(ilastDetectedID);
	plastDetected = dynamic_cast<TESObjectREFR*>(v5);
	uint32_t iteleportFadeRefID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTeleportFadeRef));
	TESForm* v6 = TESForm::GetFormByNumericID(iteleportFadeRefID);
	pTeleportFadeRef = dynamic_cast<TESObjectREFR*>(v6);
	BSSimpleList<DetectionState*>* plist = pDetectedActorList;
	BSSimpleList<DetectionState*>* plastList = 0;
	while (plist && !plist->IsEmpty()) {
		DetectionState* pdectectionState = plist->m_item;
		if (pdectectionState->pActor) {
			TESForm* v7 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pdectectionState
					->pActor)));
			Actor* v8 = dynamic_cast<Actor*>(v7);
			pdectectionState->pActor = v8;
		}
		if (pdectectionState->pActor) {
			plastList = plist;
			plist = plist->m_pkNext;
		} else {
			if (plastList) {
				plastList->Remove(pdectectionState);
				plist = plastList->m_pkNext;
			} else {
				plist->RemoveHead();
			}
			ourFree(pdectectionState);
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 90u) {
		uint32_t ilastTargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLastTarget));
		if (ilastTargetID) {
			TESForm* v9 = TESForm::GetFormByNumericID(ilastTargetID);
			pLastTarget = dynamic_cast<TESObjectREFR*>(v9);
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
		uint32_t idialogTargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDialogTarget));
		if (idialogTargetID) {
			TESForm* v10 = TESForm::GetFormByNumericID(idialogTargetID);
			pDialogTarget = dynamic_cast<MobileObject*>(v10);
		}
	}
}

void HighProcess::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	MiddleHighProcess::Revert(apLoadGameBuffer);
	if (pActorValueCache) {
		pActorValueCache->ResetCachedActorValues();
	}
	bDialoguewithPlayer = 0;
	iHasHealingSpell = -1;
	iHasHealingPotion = -1;
	fDelayTimer = 0.0f;
	bCheckDeadTalk = 0;
	iHourPackageEvaluated = -1;
	fSoundDelay = 0.0f;
	bGreetingFlag = 0;
	fGreetingTimer = 0.0f;
	bHeadTrack = 1;
	bWeaponAlertDrawn = 0;
	bContinuingPackageforPC = 0;
	fAwarePlayerTimer = 0.0f;
	fDetectGreetTimer = 0.0f;
	fEvaluateAcquireTimer = 0.0f;
	fHoldAttackTimer = 0.0f;
	fUseItemTimer = 0.0f;
	sShotsFired = 0;
	sShotsToFire = -1;
	sBurstsFired = 0;
	fBreathTimer = 20.0f;
	fTalkTimer = 0.0f;
	fTurnTime = 0.0f;
	fVoiceTimer = 0.0f;
	cLastTurnDir = 0;
	bActivateAnim = 0;
	fScriptPackageEndTime = 0.0f;
	fHealthBarAlphaValue = 0.0f;
	fActorHealthPercentage = 1.0f;
	fHealthBarEmittanceValue = 0.0f;
	iNumberGuardsPersuing = 0;
	bStop = 0;
	fReEquipArmorTimer = 0.0f;
	bUnequippedArmorToSwim = 0;
	cLastTurn = 0;
	bCurrentlyReanimating = 0;
	bWaitingForLipFile = 0;
	bLipQuequed = 0;
	LipSynchAnim* apThis = pLipSynicAnim;
	if (apThis) {
		delete apThis;
	}
	pLipSynicAnim = 0;
	bLipFileFailed = 0;
	pAnimSeq = 0;
	sAnimAction = -1;
	fTakeBackTimer = -1.0f;
	pIdleToPlay = 0;
	bProcessGreetSayTo = 0;
	fMaxAlpha = 1.0f;
	fCheckToTalkTimer = 0.0f;
	fHeadTrackTargetTimer = -1.0f;
	fRadiationTimer = 0.0f;
	fHighestRadiation = 0.0f;
	bPlantedExplosive = 0;
	bNeedTalkPlayer = 0;
	fAvoidWaitTimer = 0.0f;
	bHiding = 0;
	eSpecialIdleType = AnimIdle::PLAY_TYPE_AUTO;
	bFinishingCombatPackage = 0;
	fCachedActorHeight = 0.0f;
	fClearTalkToListTimer = fAItalktosameNPCtimer.Float();
	pPathLookAtTarget = 0;
	DialogueItem* pGreetTopic = this->pGreetTopic;
	if (pGreetTopic) {
		delete pGreetTopic;
	}
	this->pGreetTopic = 0;
	AggroRadiusList.RemoveAll();
	AvoidActorList.RemoveAll();
	pLastSpokeToList->RemoveAll();
	ClearMuzzleFlash();
	pSubtitleVoice = 0;
	FreeAvoidNodes();
	for (int32_t i = 0; i < 6; ++i) {
		HeadTrackingTargets[i] = 0;
		HeadTrackingTargetFlags[i] = 0;
	}
	pLastTarget = 0;
	bForceRotate = 0;
	fDetectionModifer = 0.0f;
	fDetectionModifierTimer = 0.0f;
	for (BSSimpleList<DetectionState*>* ecx0 = pDetectedActorList; ecx0; ecx0 = ecx0->m_pkNext) {
		void* v7 = ecx0->m_item;
		if (v7) {
			ourFree(v7);
		}
	}
	pDetectedActorList->RemoveAll();
	for (BSSimpleList<DetectionState*>* j = pActorsWhoDetectMeList; j; j = j->m_pkNext) {
		void* v5 = j->m_item;
		if (v5) {
			ourFree(v5);
		}
	}
	pActorsWhoDetectMeList->RemoveAll();
	ourFree(pActorsGeneratedDetectionEvent);
	pActorsGeneratedDetectionEvent = 0;
	m_pSayToDialogueTopic = 0;
}

void HighProcess::Revert(uint32_t aiFlags, MobileObject* pmob) {
	Actor* v5 = 0;
	if (pmob) {
		if (pmob->IsActor()) {
			v5 = static_cast<Actor*>(pmob);
		}
	}
	MiddleHighProcess::Revert(aiFlags, pmob);
	if (pSaveLoadGameOLD->IsUnk048Zero()) {
		if (pActorValueCache) {
			pActorValueCache->ResetCachedActorValues();
		}
		bDialoguewithPlayer = 0;
		iHasHealingSpell = -1;
		iHasHealingPotion = -1;
		fDelayTimer = 0.0;
		bCheckDeadTalk = 0;
		iHourPackageEvaluated = -1;
		fSoundDelay = 0.0;
		bGreetingFlag = 0;
		fGreetingTimer = 0.0;
		bHeadTrack = 1;
		bWeaponAlertDrawn = 0;
		pGreetTopic = 0;
		bContinuingPackageforPC = 0;
		fAwarePlayerTimer = 0.0;
		fDetectGreetTimer = 0.0;
		fEvaluateAcquireTimer = 0.0;
		fHoldAttackTimer = 0.0;
		fUseItemTimer = 0.0;
		sShotsFired = 0;
		sShotsToFire = -1;
		sBurstsFired = 0;
		fBreathTimer = 20.0f;
		fTalkTimer = 0.0;
		fTurnTime = 0.0;
		fVoiceTimer = 0.0;
		cLastTurnDir = 0;
		bActivateAnim = 0;
		fScriptPackageEndTime = 0.0;
		fHealthBarAlphaValue = 0.0;
		fActorHealthPercentage = 1.0;
		fHealthBarEmittanceValue = 0.0;
		iNumberGuardsPersuing = 0;
		bStop = 0;
		fReEquipArmorTimer = 0.0;
		bUnequippedArmorToSwim = 0;
		cLastTurn = 0;
		bCurrentlyReanimating = 0;
		bWaitingForLipFile = 0;
		pLipSynicAnim = 0;
		bLipFileFailed = 0;
		for (int32_t i = 0; i < 6; ++i) {
			HeadTrackingTargets[i] = 0;
			HeadTrackingTargetFlags[i] = 0;
		}
		pLastTarget = 0;
		bForceRotate = 0;
		fDetectionModifer = 0.0;
		fDetectionModifierTimer = 0.0;
		if (pLeveledSpellList) {
			pLeveledSpellList->RemoveAll();
			pLeveledSpellList = 0;
		}
		if (v5) {
			FreeUpSpecialIdle(v5);
		}
	}
}

void HighProcess::StartNewPackage(Actor* apActor, TESPackage* apPack) {
	if (apPack) {
		Script* pscript = apPack->GetOnBeginScript();
		if (pscript) {
			ScriptLocals* ScriptLocals_ = (&apActor->m_Extra)->GetScriptLocals();
			pscript->Run(apActor, ScriptLocals_, 0, 1);
		}
		TESIdleForm* pstartidle = apPack->GetOnBeginIdle();
		if (pstartidle) {
			SetupSpecialIdle(apActor, pstartidle, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
			SetBeginIdlesPlayed(1);
		} else {
			bPickPackIdle = 1;
			if (apActor->GetAnimation()) {
				Animation* v3 = apActor->GetAnimation();
				if (!v3->SpecialIdleDonePlaying()) {
					SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 0, 1, 1);
				}
			}
		}
		TESTopic* pTopic = apPack->GetOnBeginTopicInfo();
		if (pTopic) {
			ProcessGreet(apActor, pTopic, 0, 0, 1, 1);
		}
	}
	if (apActor) {
		apActor->SetWalkRunFlagDirty();
	}
}

void HighProcess::ChangePackage(Actor* apActor, TESPackage* apPack) {
	ExtraDataList* extra;
	ScriptLocals* scriptLocals;
	TESIdleForm* v6;
	TESTopic* v7;
	Script* v8;
	if (apPack) {
		v8 = apPack->GetOnChangeScript();
		if (v8) {
			if (v8->GetCompiledData() || v8->GetText()) {
				extra = apActor->TESObjectREFR::GetExtra();
				scriptLocals = extra->GetScriptLocals();
				v8->Run(apActor, scriptLocals, nullptr, 1);
			}
		}
		v6 = apPack->GetOnChangeIdle();
		if (v6) {
			SetupSpecialIdle(apActor, v6, static_cast<AnimIdle::PLAY_TYPE_ENUM>(2), (bool)1, (bool)0, (bool)1);
		}
		v7 = apPack->GetOnChangeTopicInfo();
		if (v7) {
			ProcessGreet(apActor, v7, (bool)0, (bool)0, (bool)1, (bool)1);
		}
	}
	GetCurrentPackage();
}

void HighProcess::PackageDone(Actor* apActor, TESPackage* apPack) {
	if (apPack) {
		Script* v8 = apPack->GetOnEndScript();
		if (v8) {
			ExtraDataList* Extra = apActor->GetExtra();
			ScriptLocals* ScriptLocals_ = Extra->GetScriptLocals();
			v8->Run(apActor, ScriptLocals_, 0, 1);
		}
		TESIdleForm* v6 = apPack->GetOnEndIdle();
		if (v6) {
			SetupSpecialIdle(apActor, v6, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
			if (apPack->GetMustComplete()) {
				SetEndIdlesPlayed(1);
			}
		}
		TESTopic* v7 = apPack->GetOnEndTopic();
		if (v7) {
			ProcessGreet(apActor, v7, 0, 0, 1, 1);
		}
	}
}

int32_t HighProcess::GetNumberGuardsArresting() {
	return iNumberGuardsPersuing;
}

void HighProcess::ModNumberGuardsArresting(int32_t ivalue) {
	iNumberGuardsPersuing += ivalue;
}

// The exe's cleanup-only C++ EH frame is left out.
BSSimpleList<SpellItem*>* HighProcess::GetLeveledSpells(Actor* apActor, bool abIncludeDiseases) {
	if (!apActor || !apActor->GetActorBase()) {
		return 0;
	}
	if (pLeveledSpellList) {
		return pLeveledSpellList;
	}
	BSSimpleList<TESLevSpell*>* pleveledSpells = &apActor->GetActorBase()->leveledSpellList;
	if (pleveledSpells) {
		while (pleveledSpells && !pleveledSpells->IsEmpty()) {
			TESLevSpell* pleveledSpell = pleveledSpells->m_item;
			if (pleveledSpell) {
				BSSimpleList<SpellItem*>* pspellList = pleveledSpell->GetLeveledSpells_(apActor);
				for (BSSimpleList<SpellItem*>* pspells = pspellList; pspells && !pspells->IsEmpty();
					pspells = pspells->m_pkNext) {
					SpellItem* pspell = pspells->m_item;
					if (pspell) {
						if (pspell->GetSpellType() != MagicSystem::ABILITY) {
							bool baddIt = 1;
							if (pspell->GetSpellType() == MagicSystem::DISEASE) {
								baddIt = abIncludeDiseases;
							}
							if (baddIt) {
								if (!pLeveledSpellList) {
									BSSimpleList<SpellItem*>* v4;
									BSSimpleList<SpellItem*>* v6 = static_cast<
										BSSimpleList<SpellItem*>*>(ourMalloc(sizeof(BSSimpleList<SpellItem*>)));
									if (v6) {
										v4 = ::new (v6) BSSimpleList<SpellItem*>;
									} else {
										v4 = 0;
									}
									pLeveledSpellList = v4;
								}
								pLeveledSpellList->AddHead(pspell);
							}
						}
					}
				}
				if (pspellList) {
					pspellList->RemoveAll();
					if (pspellList) {
						delete pspellList;
					}
				}
			}
			pleveledSpells = pleveledSpells->m_pkNext;
		}
	}
	return pLeveledSpellList;
}

void HighProcess::SetHasHealingSpells(int32_t aiHasHealingSpell) {
	iHasHealingSpell = aiHasHealingSpell;
}

int32_t HighProcess::GetHasHealingSpells() {
	return iHasHealingSpell;
}

void HighProcess::SetHasHealingPotions(int32_t aiHasHealingPotion) {
	iHasHealingPotion = aiHasHealingPotion;
}

int32_t HighProcess::GetHasHealingPotions() {
	return iHasHealingPotion;
}

void HighProcess::SetNeedTalkPlayer(bool abFlag) {
	bNeedTalkPlayer = abFlag;
}

bool HighProcess::GetNeedTalkPlayer() {
	return bNeedTalkPlayer;
}

BSSimpleList<Actor*>* HighProcess::GetAggroRadiusActorList() {
	return &AggroRadiusList;
}

void HighProcess::NotifyGuardAboutActivation(Actor* apActor, TESObjectREFR* apActionRef, TESObjectREFR* apActivated) {
	if (apActionRef->IsActor() && apActor != apActivated) {
		ActorPackage* pPackages[2];
		pPackages[0] = &RunOncePackage;
		pPackages[1] = &CurrentPackage;
		for (int32_t i = 0; i < 2; ++i) {
			ActorPackage* pPackage = pPackages[i];
			TESPackage* pActorPackage = pPackage->pPackage;
			if (pActorPackage &&
				(pActorPackage->GetProcedureType() == PACKAGE_PROCEDURE_GUARD &&
					(pPackage->GetTarget() == apActivated &&
						apActor->CanAttackActor(static_cast<Actor*>(apActionRef))))) {
				EnterCombat(apActor, static_cast<Actor*>(apActionRef), 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
				return;
			}
		}
	}
}

HighProcess::FADE_STATE HighProcess::GetFadeState() {
	return eFadeState;
}

void HighProcess::AddPostAnimationAction(BaseProcess::POSTANIM_ACTION aeAction) {
	ePostAnimActon |= aeAction;
}

void HighProcess::ClearPostAnimationActions() {
	ePostAnimActon = 0;
}

void HighProcess::RemovePostAnimationAction(BaseProcess::POSTANIM_ACTION aeAction) {
	ePostAnimActon &= ~aeAction;
}

void HighProcess::SetDefaultHeadTrackTarget(TESObjectREFR* apRef) {
	HeadTrackingTargets[0] = apRef;
	HeadTrackingTargetFlags[0] = (uint8_t)1;
	OnNewHeadTrackTarget();
	if (apRef) {
		apRef->SetTargeted(1);
	}
}

void HighProcess::SetActionHeadTrackTarget(TESObjectREFR* apRef) {
	HeadTrackingTargets[1] = apRef;
	if (apRef) {
		HeadTrackingTargetFlags[1] = (uint8_t)1;
		OnNewHeadTrackTarget();
		apRef->SetTargeted(1);
	} else {
		HeadTrackingTargetFlags[1] = (uint8_t)0;
	}
}

void HighProcess::SetScriptHeadTrackTarget(TESObjectREFR* apRef) {
	HeadTrackingTargets[2] = apRef;
	if (apRef) {
		HeadTrackingTargetFlags[2] = (uint8_t)1;
		OnNewHeadTrackTarget();
		apRef->SetTargeted(1);
	} else {
		HeadTrackingTargetFlags[2] = (uint8_t)0;
	}
}

void HighProcess::SetCombatHeadTrackTarget(TESObjectREFR* apRef) {
	HeadTrackingTargets[3] = apRef;
	if (apRef) {
		HeadTrackingTargetFlags[3] = (uint8_t)1;
		OnNewHeadTrackTarget();
		apRef->SetTargeted(1);
	} else {
		HeadTrackingTargetFlags[3] = (uint8_t)0;
	}
}

void HighProcess::SetDialogHeadTrackTarget(TESObjectREFR* apRef) {
	HeadTrackingTargets[4] = apRef;
	if (apRef) {
		HeadTrackingTargetFlags[4] = (uint8_t)1;
		OnNewHeadTrackTarget();
		apRef->SetTargeted(1);
	} else {
		HeadTrackingTargetFlags[4] = (uint8_t)0;
	}
}

void HighProcess::SetUseWeaponHeadTrackTarget(TESObjectREFR* apRef) {
	HeadTrackingTargets[5] = apRef;
	if (apRef) {
		HeadTrackingTargetFlags[5] = (uint8_t)1;
		OnNewHeadTrackTarget();
		apRef->SetTargeted(1);
	} else {
		HeadTrackingTargetFlags[5] = (uint8_t)0;
	}
}

void HighProcess::SetHeadTrackTarget(HEAD_TRACK_TYPE aType, TESObjectREFR* apRef) {
	HeadTrackingTargets[aType] = apRef;
	if (apRef) {
		HeadTrackingTargetFlags[aType] = (uint8_t)1;
		OnNewHeadTrackTarget();
		apRef->SetTargeted(1);
	} else {
		HeadTrackingTargetFlags[aType] = (uint8_t)0;
	}
}

void HighProcess::ClearDefaultHeadTrackTarget() {
	HeadTrackingTargets[0] = nullptr;
	HeadTrackingTargetFlags[0] = (uint8_t)0;
	OnNewHeadTrackTarget();
}

void HighProcess::ClearActionHeadTrackTarget(bool abHoldDefault) {
	TESObjectREFR* pTarget = HeadTrackingTargets[1];
	HeadTrackingTargets[1] = nullptr;
	HeadTrackingTargetFlags[1] = 0;
	if (abHoldDefault) {
		HeadTrackingTargets[0] = pTarget;
		SetHeadTrackChangeTimer(fAIHoldDefaultHeadTrackTimer.Float());
	} else {
		OnNewHeadTrackTarget();
	}
}

void HighProcess::ClearScriptHeadTrackTarget(bool abHoldDefault) {
	TESObjectREFR* pTarget = HeadTrackingTargets[2];
	HeadTrackingTargets[2] = nullptr;
	HeadTrackingTargetFlags[2] = 0;
	if (abHoldDefault) {
		HeadTrackingTargets[0] = pTarget;
		SetHeadTrackChangeTimer(fAIHoldDefaultHeadTrackTimer.Float());
	} else {
		OnNewHeadTrackTarget();
	}
}

void HighProcess::ClearCombatHeadTrackTarget() {
	HeadTrackingTargets[3] = nullptr;
	HeadTrackingTargetFlags[3] = (uint8_t)0;
	OnNewHeadTrackTarget();
}

void HighProcess::ClearDialogHeadTrackTarget(bool abHoldDefault) {
	TESObjectREFR* pTarget = HeadTrackingTargets[4];
	HeadTrackingTargets[4] = nullptr;
	HeadTrackingTargetFlags[4] = 0;
	if (abHoldDefault) {
		HeadTrackingTargets[0] = pTarget;
		SetHeadTrackChangeTimer(fAIHoldDefaultHeadTrackTimer.Float());
	} else {
		OnNewHeadTrackTarget();
	}
}

void HighProcess::ClearUseWeaponHeadTrackTarget(bool abHoldDefault) {
	TESObjectREFR* v4 = HeadTrackingTargets[5];
	HeadTrackingTargets[5] = 0;
	HeadTrackingTargetFlags[5] = 0;
	if (abHoldDefault) {
		HeadTrackingTargets[0] = v4;
		float* v2 = &fAIHoldDefaultHeadTrackTimer.Float();
		SetHeadTrackChangeTimer(*v2);
	} else {
		OnNewHeadTrackTarget();
	}
}

void HighProcess::ModDetectionModifierTimer() {
	fDetectionModifierTimer = (float)(fDetectionModifierTimer - (&appTimer)->GetSeconds());
}

float HighProcess::GetDetectionModifierTimer() {
	return fDetectionModifierTimer;
}

void HighProcess::ClearAllHeadTrackTargets() {
	int32_t i;
	for (i = 0; i < 6; ++i) {
		HeadTrackingTargets[i] = nullptr;
		HeadTrackingTargetFlags[i] = (uint8_t)0;
	}
	OnNewHeadTrackTarget();
}

void HighProcess::RemoveHeadTrackTarget(TESObjectREFR* apRef) {
	int32_t i;
	for (i = 0; i < 6; ++i) {
		if (HeadTrackingTargets[i] == apRef) {
			HeadTrackingTargets[i] = nullptr;
			HeadTrackingTargetFlags[i] = (uint8_t)0;
		}
	}
	OnNewHeadTrackTarget();
}

bool HighProcess::CanSetDefaultHeadTrackTarget() {
	int32_t i;
	for (i = 5; i > 0; --i) {
		if (HeadTrackingTargetFlags[i]) {
			return (bool)0;
		}
	}
	return (bool)((uint8_t)(GetHeadTrackChangeTimer() < 0.0));
}

bool HighProcess::CanSetActionHeadTrackTarget() {
	int32_t i;
	for (i = 5; i >= 1; --i) {
		if (HeadTrackingTargetFlags[i]) {
			return (bool)0;
		}
	}
	return (bool)1;
}

bool HighProcess::CanSetCombatHeadTrackTarget() {
	return HeadTrackingTargetFlags[4] == 0;
}

TESObjectREFR* HighProcess::GetCurrentHeadTrackTarget(HEAD_TRACK_TYPE aType) {
	return HeadTrackingTargets[aType];
}

TESObjectREFR* HighProcess::GetCurrentHeadTrackTarget() {
	for (int32_t iSlot = 5; iSlot >= 0; --iSlot) {
		if (HeadTrackingTargetFlags[iSlot])
			return HeadTrackingTargets[iSlot];
	}
	return nullptr;
}

HighProcess::HEAD_TRACK_TYPE HighProcess::GetCurrentHeadTrackType() {
	for (int32_t i = HEAD_TRACK_TYPE_USE_WEAPON; i >= HEAD_TRACK_TYPE_DEFAULT; --i) {
		if (HeadTrackingTargetFlags[i]) {
			return static_cast<HEAD_TRACK_TYPE>(i);
		}
	}
	return HEAD_TRACK_TYPE_DEFAULT;
}

const char* HighProcess::GetCurrentHeadTrackTypeString() {
	const char* result;
	switch (GetCurrentHeadTrackType()) {
	case 0: {
		result = "DEFAULT";
		break;
	}
	case 1: {
		result = "ACTION";
		break;
	}
	case 2: {
		result = "SCRIPT";
		break;
	}
	case 3: {
		result = "COMBAT";
		break;
	}
	case 4: {
		result = "DIALOG";
		break;
	}
	default: {
		result = "NONE";
		break;
	}
	}
	return result;
}

void HighProcess::SetForceRotate(bool abForceRotate) {
	bForceRotate = abForceRotate;
}

bool HighProcess::GetForceRotate() {
	return bForceRotate;
}

TESObjectREFR* HighProcess::GetLastHeadTrackTarget() {
	return pLastTarget;
}

void HighProcess::CreateMuzzleFlash(Projectile* apProjectile, TESObjectREFR* apParentRef) {
	MuzzleFlash* v3;
	void* v5;
	if (!pCurrentMuzzleFlash) {
		v5 = ourMalloc(sizeof(MuzzleFlash));
		if (v5) {
			v3 = ::new (v5) MuzzleFlash(apProjectile, static_cast<PlayerCharacter*>(apParentRef));
		} else {
			v3 = nullptr;
		}
		pCurrentMuzzleFlash = v3;
	}
}

MuzzleFlash* HighProcess::GetCurrentMuzzleFlash() {
	return pCurrentMuzzleFlash;
}

// The exe tests the copied pointer for null again (0x902257) inside the test of the field.
void HighProcess::ClearMuzzleFlash() {
	if (pCurrentMuzzleFlash) {
		MuzzleFlash* pMuzzleFlash = pCurrentMuzzleFlash;
		if (pMuzzleFlash)
			delete pMuzzleFlash;
	}
	pCurrentMuzzleFlash = nullptr;
}

void HighProcess::SetCurrentlyReanimating(bool abCurrentlyReanimating) {
	bCurrentlyReanimating = abCurrentlyReanimating;
}

bool HighProcess::GetCurrentlyReanimating() {
	return bCurrentlyReanimating;
}

void HighProcess::UpdateRadiation(Actor* apActor, float afDelta) {
	if (apActor->IsPc() || apActor->IsCreature()) {
		fRadiationTimer = fRadiationTimer + afDelta;
		if (apActor == pPlayer) {
			pPlayer->ReportRadiationChange();
		}
		float v6 = fRadiationReevaluationRate.Float() + 1.0;
		fRadiationTimer = BSMIN(fRadiationTimer, v6);
		if (fHighestRadiation != 0.0) {
			double v5 = fRadiationTimer * -1.0;
			float* v3 = &fRadiationDecayRate.Float();
			float fradDelta = NiAbs(*v3) * v5;
			if (fHighestRadiation != 0.0) {
				double X_4 = fHighestRadiation * fRadiationTimer;
				fradDelta = fRadiationAccumulationRate.Float() * X_4 + fradDelta;
			}
			float fcurrentRads = apActor->GetActorFloatValue(ActorValue::RADIATION_RADS);
			if (fcurrentRads + fradDelta < 0.0) {
				fradDelta = -fcurrentRads;
			}
			if (std::fabsf(fradDelta) != 0.0) {
				apActor->DamageModActorValue(ActorValue::RADIATION_RADS, fradDelta, static_cast<Actor*>(0));
			}
			fHighestRadiation = 0.0f;
			fRadiationTimer = 0.0f;
		}
	}
}

void HighProcess::FinishSetupSpecialIdle(Actor* apActor) {
	int32_t idleAnimGroupSection;
	AnimIdle::PLAY_TYPE_ENUM eSpecialIdleType;
	Animation* pAnimation = apActor->GetAnimation();
	if (pAnimation) {
		if (pIdleToPlay) {
			if (pAnimation->SpecialIdleWorking()) {
				AddPostAnimationAction(POSTANIM_ACTION_PLAY_IDLE);
			} else {
				eSpecialIdleType = this->eSpecialIdleType;
				idleAnimGroupSection = (int32_t)pIdleToPlay->GetIdleAnimGroupSection();
				pAnimation->SpecialIdleAuto(pIdleToPlay, apActor, static_cast<ANIM_GROUP_SECTION>(idleAnimGroupSection),
					eSpecialIdleType);
				bDoneOnce = (bool)0;
				pIdleToPlay = nullptr;
				this->eSpecialIdleType = AnimIdle::PLAY_TYPE_AUTO;
				if (GetWeaponDrawn() && GetPlantedExplosive()) {
					apActor->DropAddOnObjects();
				}
			}
		}
	}
}

TESIdleForm* HighProcess::GetCurrentProcessIdle() {
	return pIdleToPlay;
}

void HighProcess::SetCurrentProcessIdle(TESIdleForm* apIdle) {
	pIdleToPlay = apIdle;
}

void HighProcess::SetCurrentProcessAnimIdleLoaded(AnimIdle* apAnimIdle, KFModel* apAnimIdleKF) {
	if (apAnimIdle || apAnimIdleKF) {
		if (spAnimIdleToPlay->m_pObject && apAnimIdle) {
			spAnimIdleToPlay[1].operator=(apAnimIdle);
			spAnimIdleKF[1].operator=(apAnimIdleKF);
		} else {
			spAnimIdleToPlay->operator=(apAnimIdle);
			spAnimIdleKF->operator=(apAnimIdleKF);
		}
	} else {
		spAnimIdleToPlay->operator=(spAnimIdleToPlay[1]);
		spAnimIdleKF->operator=(spAnimIdleKF[1]);
		spAnimIdleToPlay[1].operator=(static_cast<AnimIdle*>(0));
		spAnimIdleKF[1].operator=(static_cast<KFModel*>(0));
	}
}

AnimIdle* HighProcess::GetCurrentProcessAnimIdle() {
	return spAnimIdleToPlay[0];
}

KFModel* HighProcess::GetCurrentProcessAnimIdleKF() {
	return spAnimIdleKF[0];
}

float HighProcess::GetActorLightLevel() {
	return fLightLevel;
}

void HighProcess::SetActorLightLevel(float aflight) {
	fLightLevel = aflight;
}

float HighProcess::GetRadiationDelta() {
	if (!(fRadiationDelta < ((double)fHighestRadiation))) {
		return fRadiationDelta;
	} else {
		return fHighestRadiation;
	}
}

void HighProcess::SetPathLookAtTarget(TESObjectREFR* apRef) {
	pPathLookAtTarget = apRef;
	if (apRef) {
		apRef->SetTargeted(1);
	}
}

void HighProcess::ClearPathLookTarget(TESObjectREFR* apRef) {
	if (pPathLookAtTarget == apRef)
		pPathLookAtTarget = nullptr;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessEscort(Actor* apActor) {
	NiPoint3 result;
	bool bforceWalkForEscortedActor[2];
	TESPackage* pPack = GetPackageThatIsRunning();
	ItemChange* pitem = apActor->GetItemBeingEscorted();
	bforceWalkForEscortedActor[1] = 0;
	if (!pitem) {
		uint32_t v2;
		if (!pTarget || (!pTarget->IsActor() && (v2 = pTarget->iFormID, !apActor->HasObjectId(v2)))) {
			uint32_t CastingType;
			SetTargetForPackage(apActor);
			if (!pTarget ||
				(!pTarget->IsActor() && (CastingType = pTarget->iFormID, !apActor->HasObjectId(CastingType)))) {
				AddToProcedureIndexRunning(apActor, 1);
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
				return;
			}
		}
	}
	if (!pitem) {
		if (this->pAcquireObject) {
			if (this->pAcquireObject->pRef == apActor) {
				pitem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
				bforceWalkForEscortedActor[1] = 1;
			}
		}
	}
	bool bIsActor = 0;
	Actor* pescortedActor = 0;
	if (pTarget && (pTarget->IsActor() && pTarget != apActor)) {
		bIsActor = 1;
		pescortedActor = static_cast<Actor*>(pTarget);
		Actor* pact = static_cast<Actor*>(pTarget);
		BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
		TESPackage* prun = CurrentAIProcess->GetCurrentPackage();
		if (pTarget != pPlayer && (!prun || (prun->GetPackType() != PACKAGE_TYPE_FOLLOW && !prun->GetIsCreated()))) {
			SetProcedureIndexRunning(0);
			ItemChange* v47 = pitem;
			ItemChange* pitem_2 = pitem;
			if (pitem) {
				delete pitem_2;
			}
			return;
		}
		ClearAllHeadTrackTargets();
		apActor->ClearLookAtTarget();
	}
	PackageTarget* ppackTarget = pPack->GetPackageTarget();
	if (ppackTarget) {
		PACK_TARG_TYPE etype = ppackTarget->GetTargType();
		bool v30 = etype == PTT_REFERENCE || etype == PTT_LINKEDREF;
		if (bIsActor && v30) {
			if (!pTarget || pTarget->GetDisabled()) {
				AddToProcedureIndexRunning(apActor, 1);
				return;
			}
			if (pTarget->GetDelete() && !(&pTarget->m_Extra)->GetReferencePointer()) {
				pPack->SetNeverRun(pTarget, 1);
				AddToProcedureIndexRunning(apActor, 1);
				return;
			}
		}
	}
	Animation* pactorAnim = apActor->GetAnimation();
	if (pactorAnim) {
		if (apActor->IsPlayingLowerBodySpecialIdle()) {
			pactorAnim->SpecialIdleFree(1, 0);
		}
	}
	bool bwaitForEscortedActor = 0;
	bforceWalkForEscortedActor[0] = 0;
	if (bIsActor) {
		TESWorldSpace* LocationWorld = pPack->GetLocationWorld(apActor);
		TESObjectCELL* afRadius = pPack->GetLocationCell(apActor);
		NiPoint3* LocationCoord = &(result = pPack->GetLocationCoord(apActor));
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, goalLoc, &PathingLocation::Destruct, LocationCoord, afRadius,
			LocationWorld);
		bwaitForEscortedActor = ShouldWaitForEscortTarget(apActor, pescortedActor, goalLoc, bforceWalkForEscortedActor);
	}
	if (pPack->pPackLoc && pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
		if (bIsActor) {
			TESPackage* v13;
			TESPackage* v14;
			BaseProcess* v26;
			BaseProcess* v27;
			BaseProcess* v28;
			BaseProcess* v29;
			Actor* ptarg;
			if (bwaitForEscortedActor) {
				goto LABEL_129;
			}
			ptarg = static_cast<Actor*>(pTarget);
			if (!ptarg ||
				(!ptarg->pCurrentProcess ||
					((v29 = ptarg->pCurrentProcess, !v29->GetPackageThatIsRunning()) ||
						((v28 = ptarg->pCurrentProcess,
							 (v13 = v28->GetPackageThatIsRunning(),
								 v13->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FOLLOW_WITH_ESCORT)) ||
							(v27 = ptarg->pCurrentProcess,
								(v27->AddToProcedureIndexRunning(ptarg, 1),
									(v26 = ptarg->pCurrentProcess,
										(v14 = v26->GetPackageThatIsRunning(), !v14->GetIsCreated())))))))) {
			LABEL_128:;
				AddToProcedureIndexRunning(apActor, 1);
			LABEL_129:;
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
				return;
			}
			BaseProcess* v25 = ptarg->pCurrentProcess;
			v25->ClearCurrentPackage();
			if (ptarg->IsPathingComplete()) {
				ptarg->EndMovement();
			}
			SetCurrentActionComplete(1);
			CheckforNewPackage(apActor, 0);
			AddToProcedureIndexRunning(apActor, 1);
		} else {
			int32_t inumber;
			const NiPoint3* ppoint;
			Animation* panim = apActor->GetAnimation();
			if (panim && !panim->SpecialIdleDonePlaying()) {
				if (bActivateAnim) {
					ItemChange* v44 = pitem;
					ItemChange* pitem_3 = pitem;
					if (pitem) {
						delete pitem_3;
					}
					return;
				}
				panim->SpecialIdleFree(1, 0);
			}
			PackageLocation* ppackloc = pPack->pPackLoc;
			PackageTarget* ppacktarg = pPack->GetPackageTarget();
			if (!pitem) {
				goto LABEL_128;
			}
			ExtraDataList* plist = 0;
			if (pitem->pExtraObjectList) {
				plist = pitem->pExtraObjectList->m_item;
			}
			uint32_t id = 0;
			if (plist) {
				if (plist->GetReferencePointer()) {
					TESObjectREFR* ReferencePointer = plist->GetReferencePointer();
					id = ReferencePointer->iFormID;
				}
			}
			TESObjectREFR* pcont = pPack->pPackLoc->GetLocReference();
			if (!pcont) {
				pcont = pGenericLocation;
			}
			if (pcont && pcont->HasContainer()) {
				if (!bActivateAnim) {
					bActivateAnim = 1;
					FindSpecialIdletoPlay(apActor, pcont->GetObjectReference(), pcont);
					ItemChange* v42 = pitem;
					ItemChange* pitem_4 = pitem;
					if (pitem) {
						delete pitem_4;
					}
					return;
				}
				int32_t InitialTargetCount = pPack->GetInitialTargetCount();
				apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, InitialTargetCount, id);
				bActivateAnim = 0;
				goto LABEL_83;
			}
			if (pitem->pExtraObjectList) {
				plist = pitem->pExtraObjectList->m_item;
			}
			ppoint = 0;
			if (ppackloc) {
				TESObjectREFR* pref = ppackloc->GetLocReference();
				if (!pref) {
					pref = pGenericLocation;
				}
				if (pref) {
					TESBoundObject* v8;
					TESBoundObject* ObjectReference = pref->GetObjectReference();
					if (ObjectReference == pXMarker || (v8 = pref->GetObjectReference(), v8 == pXMarkerHeading)) {
						ppoint = &pref->GetLocationOnReference();
					}
				}
			}
			inumber = pPack->GetInitialTargetCount();
			if (bActivateAnim) {
				apActor->DropObject(pitem->pContainerObj, plist, inumber, ppoint, 0);
				bActivateAnim = 0;
			LABEL_83:;
				ItemChange* v38 = pitem;
				ItemChange* pitem_6 = pitem;
				if (pitem) {
					delete pitem_6;
				}
				if (ppacktarg->GetTargObjectType() == POTYPE_WEAP ||
					(ppacktarg->GetTargObjectType() >= POTYPE_COMBATWEAR &&
						ppacktarg->GetTargObjectType() <= POTYPE_WEAP_RANGED)) {
					while (!ObjectList.IsEmpty()) {
						this->pAcquireObject = ObjectList.m_item;
						ObjectList.Remove(this->pAcquireObject);
						SetTarget(this->pAcquireObject->pRef);
						pitem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
						ObjectstoAcquire* pAcquireObject = this->pAcquireObject;
						void* v37 = pAcquireObject;
						if (pAcquireObject) {
							delete static_cast<Date*>(v37);
						}
						this->pAcquireObject = 0;
						if (pcont && pcont->HasContainer()) {
							int32_t v9 = pPack->GetInitialTargetCount();
							apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, v9, id);
						} else {
							if (pitem->pExtraObjectList) {
								plist = pitem->pExtraObjectList->m_item;
							}
							const NiPoint3* ppoint_2 = 0;
							if (ppackloc) {
								TESObjectREFR* pref_2 = ppackloc->GetLocReference();
								if (!pref_2) {
									pref_2 = pGenericLocation;
								}
								if (pref_2) {
									TESBoundObject* v11;
									TESBoundObject* v10 = pref_2->GetObjectReference();
									if (v10 == pXMarker ||
										(v11 = pref_2->GetObjectReference(), v11 == pXMarkerHeading)) {
										ppoint_2 = &pref_2->GetLocationOnReference();
									}
								}
							}
							int32_t inumber_3 = pPack->GetInitialTargetCount();
							apActor->DropObject(pitem->pContainerObj, plist, inumber_3, ppoint_2, 0);
						}
						ItemChange* v34 = pitem;
						ItemChange* pitem_7 = pitem;
						if (pitem) {
							delete pitem_7;
						}
						pitem = 0;
					}
					bool abweararmor = 1;
					bool abwearweapon = 1;
					if (GetCurrentPackage()) {
						TESPackage* v12 = GetCurrentPackage();
						if (v12->GetForceUnequipWeapon()) {
							abwearweapon = 0;
						}
					}
					if (apActor->IsNpc()) {
						TESNPC* pnpc = static_cast<TESNPC*>(apActor->GetObjectReference());
						if (pnpc) {
							pnpc->InitDefaultWorn(apActor, abweararmor, abwearweapon, 0, 1);
						}
					} else {
						if (apActor->IsCreature()) {
							TESCreature* pcrea = static_cast<TESCreature*>(apActor->GetObjectReference());
							if (pcrea) {
								pcrea->InitDefaultWorn(apActor, abweararmor, abwearweapon, 1);
							}
						}
					}
				}
				SetCurrentActionComplete(1);
				goto LABEL_128;
			}
			bActivateAnim = 1;
			FindSpecialIdletoPlay(apActor, pitem->pContainerObj, 0);
			if (pitem) {
				delete pitem;
			}
		}
	} else {
		if (bIsActor) {
			TESPackage* CurrentPackage;
			if (bwaitForEscortedActor ||
				(pescortedActor != pPlayer &&
					(pescortedActor->GetCurrentPackage() &&
						(CurrentPackage = pescortedActor->GetCurrentPackage(),
							CurrentPackage->IsInterruptPackage())))) {
				if (!apActor->IsPathingComplete()) {
					SetCurrentMovementComplete(1);
					AddToProcedureIndexRunning(apActor, -1);
					return;
				}
			}
			float fcurrentdistance = 0.0f;
			float fpackdist = pPack->GetPackageRadiusActorToLocation(apActor, 0);
			float v24 = fpackdist + fpackdist;
			int32_t emove = CalculateMoveMode(apActor, fcurrentdistance, fpackdist, v24, 0, 0);
			if (bforceWalkForEscortedActor[0]) {
				emove = (emove & 0xFFFFFCFFu) | 256;
			} else {
				if (apActor->bForceRun || pPack->GetAlwaysRun()) {
					emove = (emove & 0xFFFFFCFFu) | 512;
				}
			}
			SetActorsAnimation(apActor, emove, 0);
		}
		if (!apActor->IsPathing()) {
			if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
				apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
				apActor->InitiateGetUpPackage();
			} else {
				if (!bIsActor || !bwaitForEscortedActor) {
					float fpackdist_2 = pPack->GetPackageRadiusActorToLocation(apActor, 0);
					FNVD_CONSTRUCTED_LOCAL(PathingRequest, request, &PathingRequest::Destruct);
					TESWorldSpace* v18 = pPack->GetLocationWorld(apActor);
					TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
					apActor->BuildRequest(&request, pPack->GetLocationCoord(apActor), LocationCell, v18, fpackdist_2,
						0);
					request.SetWaitIfPlayerWaits(pescortedActor == pPlayer);
					apActor->SetPathfindingGoal(&request);
				}
			}
		}
	}
}

void HighProcess::ProcessSleep(Actor* apActor) {
	GetPackageThatIsRunning();
	if (!pCurrentFurniture) {
		if (apActor->GetSitSleepState()) {
			apActor->GetOutofFurnitureQuick();
		}
		ClearFurnitureMarker();
	}
	if (apActor->IsNpc() && GetSitSleepState() == SIT_SLEEP_NORMAL && !pCurrentFurniture &&
		(RefListChairBed.IsEmpty() || nFurnitureListDirtied >= nFurnitureListTimer)) {
		FindBedChairs(apActor, 0, 0);
	}
	if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING && apActor->IsNpc()) {
		if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
			apActor->InitiateGetUpPackage();
			return;
		}
		if (!pCurrentFurniture) {
			if (RefListChairBed.ItemsInList()) {
				if (TESObjectREFR::kFurnitureLock.TryLock()) {
					pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
					ClearFurnitureMarker();
					while (pCurrentFurniture && !pCurrentFurniture->HasFreeMarker(0)) {
						RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
						pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
						ClearFurnitureMarker();
					}
					if (pCurrentFurniture) {
						if (!pCurrentFurniture->GetOwner()) {
							int32_t inumber = RefListChairBed.ItemsInList();
							int32_t irandom = static_cast<int32_t>(Random() % static_cast<uint32_t>(inumber));
							if (irandom >= inumber) {
								irandom = inumber;
							}
							BSSimpleList<void*>* plist = &RefListChairBed;
							for (int32_t i = 0; i < irandom; ++i) {
								plist = plist->m_pkNext;
							}
							pCurrentFurniture = static_cast<TESObjectREFR*>(plist->m_item);
							ClearFurnitureMarker();
						}
					}
					TESObjectREFR::kFurnitureLock.Unlock();
				}
			}
		}
		if (pCurrentFurniture) {
			if (!pTarget) {
				SetTarget(pCurrentFurniture);
			}
			ProcessActivate(apActor, 0);
		}
	}
	if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
		SetCurrentActionComplete(1);
		RefListChairBed.RemoveAll();
	} else {
		if (!pCurrentFurniture && (RefListChairBed.IsEmpty() && apActor->IsNpc())) {
			EndMoveMessage(apActor);
			fEvaluateAcquireTimer = 20.0f;
			AddToProcedureIndexRunning(apActor, 1);
			return;
		}
		if (DidPathingFail(apActor)) {
			ClearFurniture(apActor);
			EndMoveMessage(apActor);
			AddToProcedureIndexRunning(apActor, 1);
			return;
		}
	}
	if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
		BSFaceGenAnimationData* panimData = apActor->GetFaceAnimationData();
		if (panimData) {
			if (!panimData->GetDead()) {
				panimData->SetDead(1, 0);
			}
		}
	}
}

void HighProcess::ProcessEat(Actor* apActor, TESObjectREFR* apLocRef) {
	NiPoint3 pointB;
	TESPackage* v47 = 0;
	TESPackage* pPack = GetPackageThatIsRunning();
	if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
		apActor->InitiateGetUpPackage();
		return;
	}
	if (!GetCurrentActionComplete() && apActor->IsNpc()) {
		ItemChange* pfood = apActor->GetBestFoodItem();
		if (pfood) {
			pItemBeingUsed = pfood->pContainerObj;
		}
		if (pfood || pItemBeingUsed) {
			if (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) || pCurrentFurniture) {
				TESObjectREFR* plocRef;
				if (apLocRef) {
					plocRef = apLocRef;
				} else {
					plocRef = pPack->GetLocationReference(apActor);
				}
				if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && !pCurrentFurniture &&
					(RefListChairBed.IsEmpty() || nFurnitureListDirtied >= nFurnitureListTimer)) {
					FindBedChairs(apActor, 1, plocRef);
				}
				if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
					Animation* panim_2 = apActor->GetAnimation();
					ExtraDataList* pextra = 0;
					if (pfood) {
						pItemBeingUsed = pfood->pContainerObj;
						if (pfood->pExtraObjectList) {
							pextra = pfood->pExtraObjectList->m_item;
						}
					}
					if (!panim_2 || panim_2->SpecialIdleDonePlaying()) {
						apActor->Eat(pItemBeingUsed, pextra, 1);
					}
					SetCurrentActionComplete(1);
					RefListChairBed.RemoveAll();
					SetIdleTimer(0.0f);
				} else {
					if (!pCurrentFurniture) {
						if (RefListChairBed.ItemsInList()) {
							if (TESObjectREFR::kFurnitureLock.TryLock()) {
								pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
								ClearFurnitureMarker();
								while (pCurrentFurniture && !pCurrentFurniture->HasFreeMarker(0)) {
									RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
									pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
									ClearFurnitureMarker();
								}
								if (pCurrentFurniture) {
									if (!pCurrentFurniture->GetOwner()) {
										int32_t inumber = RefListChairBed.ItemsInList();
										int32_t irandom =
											static_cast<int32_t>(Random() % static_cast<uint32_t>(inumber));
										if (irandom >= inumber) {
											irandom = inumber;
										}
										BSSimpleList<void*>* plist = &RefListChairBed;
										for (int32_t i = 0; i < irandom; ++i) {
											plist = plist->m_pkNext;
										}
										pCurrentFurniture = static_cast<TESObjectREFR*>(plist->m_item);
										ClearFurnitureMarker();
									}
								}
								TESObjectREFR::kFurnitureLock.Unlock();
							}
						}
					}
					SetTarget(pCurrentFurniture);
					Animation* panim = apActor->GetAnimation();
					if (pCurrentFurniture &&
						(!panim ||
							(panim->SpecialIdleDonePlaying() || pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_SANDBOX))) {
						ProcessActivate(apActor, 0);
					} else {
						if (panim) {
							if (!panim->SpecialIdleDonePlaying()) {
								FreeUpSpecialIdle(apActor);
							}
						}
					}
					if (!pCurrentFurniture && RefListChairBed.IsEmpty()) {
						if (pfood) {
							pItemBeingUsed = pfood->pContainerObj;
							apActor->QueueEquipObject(pfood->pContainerObj, 1, 0, 1, 0, 1);
						}
						SetCurrentActionComplete(1);
						RefListChairBed.RemoveAll();
						SetIdleTimer(0.0f);
					}
					if (DidPathingFail(apActor)) {
						ClearFurniture(apActor);
						EndMoveMessage(apActor);
						SetIdleTimer(0.0f);
					}
				}
				if ((apActor->GetMoveMode() & 1024) != 0) {
					apActor->SetWalkRunFlagDirty();
					apActor->ClearMoveMode(1024u);
				}
			} else {
				SetProcedureIndexRunning(0);
			}
		} else {
			if (pTarget) {
				ProcessAcquire(apActor);
				return;
			}
			if (ObjectList.IsEmpty()) {
				if (pPack->GetPackageSearchLocation()) {
					float facquireDist;
					uint32_t iradius = pPack->GetSearchLocationRadius(apActor);
					TESObjectCELL* pCell = pPack->GetSearchLocationCell(apActor);
					pPack->GetSearchLocationCoord(&pointB, apActor);
					if (!iradius && !pCell) {
						const char* afMaxa = pPack->GetFormEditorID();
						const char* afMin = apActor->GetFormEditorID();
						Warning("ProcessEat has an actor %s (%08X) that has no radius or cell specified for package "
								"(%08X). Forcing radius to 64, have a nice day instead of CRASHING.\n",
							apActor->GetFullName(), afMin, afMaxa);
						iradius = 64;
					}
					if (iradius) {
						facquireDist = static_cast<float>(iradius);
					} else {
						float v12;
						if (pCell->GetInterior()) {
							v12 = 3.4028235e+38f;
						} else {
							v12 = fAIAcquireObjectDistance.Float();
						}
						facquireDist = v12;
					}
					HighProcess_bEating = 1;
					eFormType = POTYPE_FOOD;
					pObjecttoAcquire = 0;
					pDataHandler->EnumReferencesCloseToPoint(pCell, pointB, facquireDist, pointB, facquireDist,
						&LowProcess_EvaluateReferenceforAcquire, apActor);
					HighProcess_bEating = 0;
					eFormType = POTYPE_NONE;
					pFactiontoAquire = 0;
					EvaluateOrderAcquireList(apActor);
				}
				if (ObjectList.IsEmpty()) {
					int32_t FormType;
					BGSListForm* pfoodListWrapper_2;
					TESForm* pform_4;
					PackageTarget* ppackTarg_2 = pPack->GetPackageTarget();
					if (ppackTarg_2 && ppackTarg_2->GetTargType() == PTT_OBJECT) {
						pform_4 = ppackTarg_2->GetTargObject();
					} else {
						pform_4 = 0;
					}
					if (pform_4) {
						pfoodListWrapper_2 = static_cast<BGSListForm*>(pform_4);
					} else {
						pfoodListWrapper_2 =
							static_cast<BGSListForm*>(BGSDefaultObjectManager::GetDefaultObject(DO_RANDOM_FOOD));
					}
					TESForm* pform_5 = (TESForm*)pfoodListWrapper_2;
					if (!pfoodListWrapper_2) {
						goto LABEL_141;
					}
					if (pfoodListWrapper_2->GetFormType() == FLST_ID) {
						BSSimpleList<TESForm*>* pfoodList_2 = &pfoodListWrapper_2->ListOfForms;
						if (pfoodList_2) {
							for (int32_t ientry_2 = RandomInt(0, pfoodList_2->ItemsInList() - 1);
								pfoodList_2 && ientry_2 > 0; --ientry_2) {
								pfoodList_2 = pfoodList_2->m_pkNext;
							}
							pform_5 = pfoodList_2->m_item;
						}
					}
					FormType = pform_5->GetFormType();
					if (FormType == 29) {
						pItemBeingUsed = static_cast<TESBoundObject*>(pform_5);
						return;
					}
					if (FormType == 47) {
						pItemBeingUsed = static_cast<TESBoundObject*>(pform_5);
					} else {
					LABEL_141:;
						EndMoveMessage(apActor);
						fEvaluateAcquireTimer = 20.0f;
						AddToProcedureIndexRunning(apActor, 1);
					}
				}
			} else {
				pAcquireObject = (&ObjectList)->m_item;
				pAcquireObject->iamount = 1;
				ObjectList.Remove(pAcquireObject);
				if (pAcquireObject->eType == ACQUIRE_TYPE_BUY_NPC && !pAcquireObject->pRef->IsActor()) {
					TESNPC* pnpc = 0;
					TESForm* pownerform = pAcquireObject->pRef->GetOwner();
					if (pownerform) {
						if (pownerform->GetFormType() == NPC__ID) {
							pnpc = static_cast<TESNPC*>(pownerform);
						}
					}
					if (pnpc) {
						Actor* ActorRefInHigh_0 = ReferenceProcessLists.GetActorRefInHigh(pnpc, 0);
						SetTarget(ActorRefInHigh_0);
					}
				} else {
					SetTarget(pAcquireObject->pRef);
				}
			}
		}
	} else {
		TESObjectREFR* pitemref = pPack->GetLocationReference(apActor);
		if (apActor->IsNpc() ||
			(!pitemref || (pitemref->GetDelete() || pitemref->GetObjectReference()->GetFormType() != INGR_ID))) {
			if (!pItemBeingUsed) {
				TESForm* pform_2;
				PackageTarget* ppackTarg = pPack->GetPackageTarget();
				if (ppackTarg && ppackTarg->GetTargType() == PTT_OBJECT) {
					pform_2 = ppackTarg->GetTargObject();
				} else {
					pform_2 = 0;
				}
				TESForm* pform = pform_2 ? pform_2 : BGSDefaultObjectManager::GetDefaultObject(DO_RANDOM_FOOD);
				TESForm* pform_3 = pform;
				if (pform) {
					if (pform_3->GetFormType() == FLST_ID) {
						BGSListForm* pfoodListWrapper = static_cast<BGSListForm*>(pform_3);
						if (pform_3) {
							BSSimpleList<TESForm*>* pfoodList = &pfoodListWrapper->ListOfForms;
							if (pfoodList) {
								for (int32_t ientry = RandomInt(0, pfoodList->ItemsInList() - 1);
									pfoodList && ientry > 0; --ientry) {
									pfoodList = pfoodList->m_pkNext;
								}
								pform_3 = pfoodList->m_item;
							}
						}
					}
					int32_t v16 = pform_3->GetFormType();
					if (v16 == 29) {
						pItemBeingUsed = static_cast<TESBoundObject*>(pform_3);
						return;
					}
					if (v16 == 47) {
						pItemBeingUsed = static_cast<TESBoundObject*>(pform_3);
						return;
					}
				}
			}
		} else {
			apActor->PickUpObject(pitemref, 1, 0);
			pItemBeingUsed = pitemref->GetObjectReference();
			apActor->Eat(pitemref->GetObjectReference(), &pitemref->m_Extra, 1);
			FreeUpSpecialIdle(apActor);
		}
		if (apActor->GetAnimation()) {
			Animation* v3 = apActor->GetAnimation();
			if (v3->SpecialIdleDonePlaying()) {
				float ftime = GetIdleTimer();
				if (!(ftime <= 0.0)) {
					float v15 = ftime - appTimer.fDelta;
					SetIdleTimer(v15);
				} else {
					TESIdleManager::SetUsedItem(pItemBeingUsed);
					SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
					float afMax = RandomFloat(1.0f, 6.0f);
					SetIdleTimer(afMax);
					TESIdleManager::SetUsedItem(0);
				}
			}
		}
	}
}

void HighProcess::ProcessAlarm(Actor* apActor) {
	AlarmPackage* ppack = static_cast<AlarmPackage*>(GetCurrentPackage());
	bool bstealing = 0;
	Actor* pCriminal = static_cast<Actor*>(ppack->GetPackageTarget()->GetTargReference());
	TESPackage* pPack = (&apActor->m_Extra)->GetPackageExtra();
	if (pCriminal == pPlayer && pPlayer->GetControlsDisabled(PlayerCharacter::DPC_FIGHTING)) {
		EndMoveMessage(apActor);
	} else {
		bool bcombatflag = 0;
		if (apActor->GetSitSleepState() &&
			(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
				apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
			apActor->InitiateGetUpPackage();
		} else {
			if (ppack) {
				TESClass* Class = apActor->GetClass();
				if (Class->IsGuard()) {
					AddToProcedureIndexRunning(apActor, 1);
				} else {
					BSSimpleList<Crime*>* plist = &ppack->Crimes;
					bool bflee = 1;
					while (plist && plist->m_item) {
						Crime* pcrime = plist->m_item;
						TESObjectREFR* v11 = 0;
						TESObjectREFR* v10 = 0;
						TESObjectREFR* pcrimeRef = pcrime->pcrimetarget;
						if (pcrimeRef) {
							if (pcrimeRef->IsActor()) {
								v10 = pcrimeRef;
							}
						}
						if (!pcrime->bguardrespond) {
							CallforHelp(apActor, pCriminal, pcrime);
						}
						if (pcrime->bguardrespond) {
							bflee = 0;
						} else {
							if (!pcrime->ecrime || pcrime->ecrime == PackageNS::CRIME_TYPE_PICKPOCKET) {
								PlayerCharacter* CastingType = static_cast<PlayerCharacter*>(pcrime->pcriminal);
								if (CastingType == pPlayer) {
									TESBoundObject* ObjectReference;
									TESForm* pownerform = pcrime->pOwnership;
									TESFaction* apFaction = 0;
									if (pownerform) {
										if (pownerform->GetFormType() == FACT_ID) {
											apFaction = static_cast<TESFaction*>(pownerform);
										}
									}
									if (pcrime->ecrime == PackageNS::CRIME_TYPE_PICKPOCKET ||
										((ObjectReference = apActor->GetObjectReference(),
											 ObjectReference == pcrime->pOwnership) ||
											(apFaction &&
												static_cast<double>(apActor->GetActorBase()->GetFactionRank(apFaction,
													apActor == pPlayer)) > -1.0))) {
										bstealing = 1;
									}
								}
							}
						}
						if (!bcombatflag) {
							FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
							if (pcrime->ecrime <= PackageNS::CRIME_TYPE_TRESPASS) {
								if (apActor->GetShouldAttackActor(pCriminal, 0, ereaction, 0)) {
									bcombatflag = 1;
								}
							} else {
								if (v10) {
									if (apActor->GetShouldAttackActor(pCriminal, 0, ereaction, 0)) {
										bcombatflag = 1;
									}
								}
							}
						}
						plist = plist->m_pkNext;
					}
					if (bcombatflag) {
						apActor->EndInterruptPackage(0);
						EnterCombat(apActor, pCriminal, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
					} else {
						if (bflee) {
							if (bstealing) {
								AddToProcedureIndexRunning(apActor, 3);
							} else {
								if (pCriminal->IsTresPassing()) {
									apActor->EndInterruptPackage(0);
									TrespassPackage* ptrespack = (&pCriminal->m_Extra)->GetTrespassPackage();
									if (ptrespack) {
										ptrespack->ModWarning(0);
										ptrespack->ModWarningTimer(0.0f);
									}
									apActor->InitiateTresPassPackage(ptrespack);
								} else {
									AddToProcedureIndexRunning(apActor, 2);
								}
							}
						} else {
							TESPackage* ppack_2 = (&apActor->m_Extra)->GetPackageExtra();
							if ((pCriminal == pPlayer ||
									(ppack_2 &&
										(ppack_2->GetPackType() == PACKAGE_TYPE_SLEEP &&
											ppack_2->IsActorAtLocation(apActor, 0, -1.0f, 0)))) &&
								(pCriminal == pPlayer && pCriminal->IsTresPassing())) {
								SetTarget(pPlayer);
								ProcessFollow(apActor, 0, 257, 0);
							} else {
								AddToProcedureIndexRunning(apActor, 4);
							}
						}
					}
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out. It compares iNumberItemsActivate and ObjectstoAcquire::iamount
// signed (jg at 0x8EA81F, 0x8EAFF4, 0x8EB0CC, 0x8EB228, 0x8EB64D): both are the PDB's int.
bool HighProcess::ProcessActivate(Actor* apActor, bool abInterateindex) {
	TESPackage* v12;
	FNVD_INLINED_LOCAL(PathingRequest, request, std::nullopt, std::nullopt);
	if (pFaderManager->GetFaderAlpha(FADER_TO_BLACK) > 0.0 ||
		(GetSitSleepState() != SIT_SLEEP_IS_SITTING && GetSitSleepState())) {
		return 0;
	}
	TESPackage* v113 = 0;
	TESPackage* pPack = GetPackageThatIsRunning();
	TESPackage* peditorpack = (&apActor->m_Extra)->GetPackageExtra();
	if (!pTarget || pTarget->GetDelete()) {
		SetTargetForPackage(apActor);
	}
	if (!pTarget) {
		if (abInterateindex) {
			AddToProcedureIndexRunning(apActor, 1);
		}
		return 0;
	}
	if (pTarget->GetDelete() || pTarget->GetDisabled()) {
		if (pTarget->GetDelete()) {
			pPack->SetNeverRun(pTarget, 1);
		}
		if (abInterateindex) {
			AddToProcedureIndexRunning(apActor, 1);
		}
		return 0;
	}
	if (pTarget->IsDead(1) && !this->pAcquireObject) {
		if (pPack == GetRunOncePackage()) {
			apActor->EndInterruptPackage(0);
		} else {
			pPack->SetNeverRun(pTarget, 1);
		}
		return 0;
	}
	TESObjectREFR* pstortarget = pTarget;
	if (pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
		if (pTarget == pPlayer &&
			(GetTalkingtoPC() &&
				(pPlayer->GetDistanceFromReference(apActor, 0, 0) > 800.0 && pPlayer->IsPathingComplete()))) {
			if (peditorpack) {
				float fpackdist = apActor->GetDistanceFromPoint(peditorpack->GetLocationCoord(apActor));
				if (fpackdist > 600.0) {
					apActor->EndInterruptPackage(0);
					return 0;
				}
			}
		} else {
			// A dialogue package: 0x4036B0 is the one-field getter of its pTalkingActivator.
			if (static_cast<DialoguePackage*>(pPack)->pTalkingActivator) {
				TESObjectREFR* pstortarget_2 = static_cast<DialoguePackage*>(pPack)->pTalkingActivator;
				SetTarget(pstortarget_2);
			}
		}
	}
	TESObjectREFR* pref = pPack->GetLocationReference(apActor);
	if ((pref && pref->IsFurniture()) || apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
		TESBoundObject* v7;
		TESBoundObject* ObjectReference = pTarget->GetObjectReference();
		if (ObjectReference == pXMarkerHeading || (v7 = pTarget->GetObjectReference(), v7 == pXMarker)) {
			SetActionHeadTrackTarget(pref);
			return 1;
		}
	}
	DoorTeleportData* pTel = pTarget->GetTeleport();
	bool batRef = 0;
	float fdistance = pPack->GetPackageRadiusActorToRefTarget(apActor, 0);
	if (pTarget->IsFurniture()) {
		if (!pCurrentFurniture || pCurrentFurniture != pTarget) {
			if (pCurrentFurniture &&
				(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
					apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
				apActor->InitiateGetUpPackage();
			} else {
				pCurrentFurniture = pTarget;
				ClearFurnitureMarker();
			}
		}
		if (apActor->GetSitSleepState()) {
			batRef = 1;
		}
		if (pCurrentFurniture &&
			(apActor->GetSitSleepState() == SIT_SLEEP_NORMAL &&
				(pTarget->GetMarkerUsed(cCurrentFurnitureIndex, 1) && !apActor->IsPathingComplete()))) {
			pCurrentFurniture = 0;
			EndMoveMessage(apActor);
			SetTarget(0);
			return 0;
		}
		NiPoint3 point = apActor->GetLocationOnReference();
		uint32_t iindex = 0;
		if (cCurrentFurnitureIndex == 127) {
			TESObjectCELL* ParentCell = apActor->GetParentCell();
			if (ParentCell == pCurrentFurniture->GetParentCell()) {
				if (!pTarget->GetClosestFreeMarker(point, 1, 1, FurnitureMarker, iindex, 1)) {
					pCurrentFurniture = 0;
					SetTarget(0);
					if (abInterateindex) {
						if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACTIVATE) {
							AddToProcedureIndexRunning(apActor, -1);
						} else {
							AddToProcedureIndexRunning(apActor, 1);
						}
					}
					return 0;
				}
				pCurrentFurniture = pTarget;
				float afRadius = FurnitureMarker.GetHeading();
				TESWorldSpace* WorldSpace = pTarget->GetWorldSpace();
				if (!apActor->SetPathfindingGoalAndAngle_0(FurnitureMarker.Position, pTarget->GetParentCell(),
						WorldSpace, 20.0f, afRadius, 0)) {
					return 0;
				}
				cCurrentFurnitureIndex = iindex;
			}
		}
	}
	if (pTel) {
		const NiPoint3* afB = &apActor->GetLocationOnReference();
		const NiPoint3* LinkedDoorTeleportPosition = &pTarget->GetLinkedDoorTeleportPosition();
		NiPoint3 dist = *LinkedDoorTeleportPosition - *afB;
		double v11 = dist.Length();
		if (static_cast<double>(iAIDistanceRadiusMinLocation.Int()) >= v11) {
			batRef = 1;
		}
	} else {
		if (!batRef) {
			batRef = pPack->IsActorAtRefTarget(apActor, 0);
		}
	}
	uint8_t bpathingfailed = DidPathingFail(apActor);
	if (batRef || bpathingfailed) {
		SetTarget(pstortarget);
		if (!apActor->DidPathingFail()) {
			TESBoundObject* v22 = pTarget->GetObjectReference();
			if (v22 == pXMarkerHeading && !DidPathingFail(apActor)) {
				apActor->RequestRotate((&pTarget->GetAngleOnReference())->z, 0);
			}
			if (pTarget != apActor && (pTarget->IsActor() && apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING)) {
				if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
					apActor->InitiateGetUpPackage();
					return 1;
				}
				NiPoint3 v117 = pTarget->GetLocationOnReference();
				apActor->RequestRotate(v117, 0);
			}
		}
		bool bdoneMoving = 1;
		if (abInterateindex) {
			if (pTarget) {
				if (pTarget == pPlayer && !bpathingfailed) {
					Actor* pactor = static_cast<Actor*>(pTarget);
					BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
					if (!CurrentAIProcess->CanForceGreet(apActor)) {
						return 0;
					}
				}
			}
			if (!bpathingfailed) {
				if (this->pAcquireObject && (!pTarget->IsFurniture() && !pTarget->IsMobileObject())) {
					Animation* panim = apActor->GetAnimation();
					SetActionHeadTrackTarget(this->pAcquireObject->pRef);
					bool v64 = !apActor->IsMoving() && !apActor->IsRotating();
					bdoneMoving = v64;
					if (v64) {
						if (!bActivateAnim) {
							bActivateAnim = 1;
							FreeUpSpecialIdle(apActor);
							SetActionHeadTrackTarget(pTarget);
							if (FindSpecialIdletoPlay(apActor, pTarget->GetObjectReference(), pTarget)) {
								fIdleTimer = 1.0f;
								return 0;
							}
							fIdleTimer = 0.0f;
						}
						if (!(fIdleTimer <= 0.0)) {
							fIdleTimer = fIdleTimer - appTimer.fDelta;
						} else {
							this->pAcquireObject->pRef->Activate(apActor, 1, this->pAcquireObject->pObject,
								this->pAcquireObject->iamount);
							--iNumberItemsActivate;
							bActivateAnim = 0;
							ClearActionHeadTrackTarget(1);
							if (iNumberItemsActivate <= 0) {
								AddToProcedureIndexRunning(apActor, 1);
								SetTargetActivated(1);
								SetCurrentActionComplete(1);
								bActivateAnim = 0;
							}
						}
					}
					goto LABEL_300;
				}
				if (!pTarget->IsMobileObject() && !pTarget->IsFurniture()) {
					Animation* panim_2 = apActor->GetAnimation();
					if (!bActivateAnim) {
						bActivateAnim = 1;
						FreeUpSpecialIdle(apActor);
						SetActionHeadTrackTarget(pTarget);
						FindSpecialIdletoPlay(apActor, pTarget->GetObjectReference(), pTarget);
						return 0;
					}
					if (!panim_2 || panim_2->SpecialIdleDonePlaying()) {
						AddToProcedureIndexRunning(apActor, 1);
						if (this->pAcquireObject) {
							pTarget->Activate(apActor, 1, this->pAcquireObject->pObject, this->pAcquireObject->iamount);
						} else {
							pTarget->Activate(apActor, 1, 0, 1);
						}
						SetTargetActivated(1);
						uint32_t Count = pPack->ePROCEDURE_TYPE;
						if (Procedure_Action[Count][GetCurrentProcedureIndex()] == PROCEDURE_DONE) {
							SetCurrentActionComplete(1);
						}
						ClearActionHeadTrackTarget(1);
						bActivateAnim = 0;
					}
					goto LABEL_300;
				}
				if (!pTarget->IsFurniture()) {
					float fcurrenttime = appCalendar.GetHour();
					fDetectListTimer = fcurrenttime;
					if (!apActor->IsNpc() && pTarget->IsNpc()) {
						pTarget->Activate(apActor, 0, 0, 1);
					} else {
						Actor* pact_2 = 0;
						if (pTarget->IsActor()) {
							pact_2 = static_cast<Actor*>(pTarget);
						}
						if (pact_2 && pact_2->IsTryingToEnterFurniture()) {
							Actor* pact_3 = static_cast<Actor*>(pTarget);
							BaseProcess* v63 = pact_3->pCurrentProcess;
							v63->ProcessGetUpOneHour(pact_3);
							return 0;
						}
						if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_NPC && pPack->GetOncePerDay()) {
							if (pPack->GetPackageSchedule()) {
								if (!pPack->GetPackageSchedule()->PSData.iDuration) {
									AddToProcedureIndexRunning(apActor, 2);
									char Day = appCalendar.GetDay();
									apActor->SetRunOnceExtraPackage(pPack, Day);
								}
							}
						}
						if (!pPack->GetIsCreated() && pTarget->IsDead(1) && !this->pAcquireObject) {
							pPack->SetNeverRun(pTarget, 1);
						}
						pTarget->Activate(apActor, 1, 0, 1);
					}
					return 1;
				}
				if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
					apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
					AddToProcedureIndexRunning(apActor, 1);
					SetTargetActivated(1);
					uint32_t v24 = pPack->ePROCEDURE_TYPE;
					if (Procedure_Action[v24][GetCurrentProcedureIndex()] == PROCEDURE_DONE) {
						SetCurrentActionComplete(1);
					}
					return 1;
				}
				if (!iAnimActionSuccess) {
				LABEL_181:;
					AddToProcedureIndexRunning(apActor, 1);
					pGenericLocation = 0;
					EndMoveMessage(apActor);
					iAnimActionSuccess = 1;
					return 1;
				}
				goto LABEL_182;
			}
			if (pPack->GetMustComplete()) {
				if (pTarget->IsFurniture()) {
					BaseProcess* v60 = apActor->pCurrentProcess;
					FurnitureMark* pfurn_3 = v60->GetFurnitureMarker();
					TESWorldSpace* afAngleToFacef = pTarget->GetWorldSpace();
					TESObjectCELL* v40 = pTarget->GetParentCell();
					apActor->SetPathfindingGoal(pfurn_3->Position, v40, afAngleToFacef, 0.0f, 0);
				} else {
					float afRadiusa = pPack->GetPackageRadiusActorToRefTarget(apActor, 0);
					TESWorldSpace* afAngleToFaceg = pTarget->GetWorldSpace();
					TESObjectCELL* v41 = pTarget->GetParentCell();
					const NiPoint3* v31 = &pTarget->GetLocationOnReference();
					apActor->SetPathfindingGoal(*v31, v41, afAngleToFaceg, afRadiusa, 0);
				}
				return 0;
			}
			if (pPack->GetPackType() == PACKAGE_TYPE_ESCORT) {
				Actor* ptarg = 0;
				if (pTarget) {
					if (pTarget->IsActor()) {
						ptarg = static_cast<Actor*>(pTarget);
					}
				}
				if (ptarg) {
					BaseProcess* v62 = ptarg->pCurrentProcess;
					v62->AddToProcedureIndexRunning(ptarg, 1);
				}
				return 1;
			}
			if (bpathingfailed) {
				bActivateAnim = 0;
				ClearActionHeadTrackTarget(1);
				if (pTarget->IsFurniture()) {
					BaseProcess* v61 = apActor->pCurrentProcess;
					FurnitureMark* pfurn_2 = v61->GetFurnitureMarker();
					TESWorldSpace* afAngleToFaced = pTarget->GetWorldSpace();
					TESObjectCELL* v38 = pTarget->GetParentCell();
					apActor->SetPathfindingGoal(pfurn_2->Position, v38, afAngleToFaced, 0.0f, 0);
				} else {
					if (this->pAcquireObject && iNumberItemsActivate <= 0) {
						PACKAGE_PROCEDURE_TYPE v28 = pPack->ePROCEDURE_TYPE;
						int32_t ProcedureCount = Procedure::GetProcedureCount(v28);
						SetProcedureIndexRunning(ProcedureCount - 1);
					} else {
						if (this->pAcquireObject) {
							SetTarget(0);
							ObjectstoAcquire* pAcquireObject = this->pAcquireObject;
							if (pAcquireObject) {
								pAcquireObject->Delete(1u);
							}
							this->pAcquireObject = 0;
						}
					}
					if (pTarget) {
						TESWorldSpace* afAngleToFacee = pTarget->GetWorldSpace();
						TESObjectCELL* v39 = pTarget->GetParentCell();
						const NiPoint3* v30 = &pTarget->GetLocationOnReference();
						apActor->SetPathfindingGoal(*v30, v39, afAngleToFacee, 0.0f, 0);
						return 0;
					}
				}
				return 1;
			}
			Animation* panim_3 = apActor->GetAnimation();
			if (bActivateAnim) {
				if (!panim_3 || panim_3->SpecialIdleDonePlaying()) {
					if (this->pAcquireObject) {
						if (pTarget->Activate(apActor, 0, this->pAcquireObject->pObject,
								this->pAcquireObject->iamount)) {
							if (this->pAcquireObject) {
								if (iNumberItemsActivate <= this->pAcquireObject->iamount) {
									AddToProcedureIndexRunning(apActor, 1);
								}
								ObjectstoAcquire* apThis = this->pAcquireObject;
								if (apThis) {
									apThis->Delete(1u);
								}
							} else {
								--iNumberItemsActivate;
							}
							this->pAcquireObject = 0;
							SetTarget(0);
						} else {
							SetTarget(0);
						}
						bActivateAnim = 0;
						ClearActionHeadTrackTarget(1);
						if (this->pAcquireObject && iNumberItemsActivate <= 0) {
							PACKAGE_PROCEDURE_TYPE v26 = pPack->ePROCEDURE_TYPE;
							int32_t v27 = Procedure::GetProcedureCount(v26);
							SetProcedureIndexRunning(v27 - 1);
						} else {
							if (this->pAcquireObject) {
								SetTarget(0);
								ObjectstoAcquire* v72 = this->pAcquireObject;
								if (v72) {
									v72->Delete(1u);
								}
								this->pAcquireObject = 0;
							}
						}
					}
				}
				return 1;
			}
		} else {
			if (bpathingfailed) {
				if (pPack->GetMustComplete()) {
					float afRadiusb = pPack->GetPackageRadiusActorToRefTarget(apActor, 0);
					TESWorldSpace* afAngleToFaceh = pTarget->GetWorldSpace();
					TESObjectCELL* v42 = pTarget->GetParentCell();
					const NiPoint3* v32 = &pTarget->GetLocationOnReference();
					apActor->SetPathfindingGoal(*v32, v42, afAngleToFaceh, afRadiusb, 0);
					return 0;
				}
				if (pPack->GetPackType() == PACKAGE_TYPE_ESCORT) {
					Actor* ptarg_2 = 0;
					if (pTarget) {
						if (pTarget->IsActor()) {
							ptarg_2 = static_cast<Actor*>(pTarget);
						}
					}
					if (ptarg_2) {
						TESPackage* CurrentPackage;
						if (!ptarg_2->GetCurrentPackage() ||
							(CurrentPackage = ptarg_2->GetCurrentPackage(),
								CurrentPackage->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FOLLOW_WITH_ESCORT)) {
							AddToProcedureIndexRunning(ptarg_2, -1);
							return 0;
						}
						BaseProcess* v59 = ptarg_2->pCurrentProcess;
						v59->AddToProcedureIndexRunning(ptarg_2, 1);
					}
				}
				goto LABEL_300;
			}
			if (pTarget->IsFurniture()) {
				if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
					apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
					SetTargetActivated(1);
					return 1;
				}
				if (!iAnimActionSuccess) {
					goto LABEL_181;
				}
			LABEL_182:;
				AlignToFurnitureAndAddPostAnimSitSleep(apActor);
			LABEL_300:;
				Animation* panim_6 = apActor->GetAnimation();
				if ((panim_6 && !panim_6->SpecialIdleDonePlaying()) ||
					(!this->pAcquireObject ||
						((pTarget && pTarget->IsFurniture()) || (!bdoneMoving && apActor->IsPathing())))) {
					return 0;
				}
				ObjectstoAcquire* v69 = this->pAcquireObject;
				if (v69) {
					v69->Delete(1u);
				}
				this->pAcquireObject = 0;
				SetTarget(0);
				pSavedAcquireObject = 0;
				return 1;
			}
			if (this->pAcquireObject) {
				Animation* panim_4 = apActor->GetAnimation();
				if (bActivateAnim) {
					if (!panim_4 || panim_4->SpecialIdleDonePlaying()) {
						if (pTarget->Activate(apActor, 0, this->pAcquireObject->pObject,
								this->pAcquireObject->iamount)) {
							if (this->pAcquireObject) {
								if (iNumberItemsActivate <= this->pAcquireObject->iamount) {
									AddToProcedureIndexRunning(apActor, 1);
								}
							}
							ObjectstoAcquire* v70 = this->pAcquireObject;
							if (v70) {
								v70->Delete(1u);
							}
							this->pAcquireObject = 0;
							SetTarget(0);
						} else {
							SetTarget(0);
						}
						bActivateAnim = 0;
						ClearActionHeadTrackTarget(1);
					}
					goto LABEL_300;
				}
			} else {
				if (pPack->GetPackType() == PACKAGE_TYPE_ACCOMPANY) {
					AddToProcedureIndexRunning(apActor, 1);
					EndMoveMessage(apActor);
					return 1;
				}
				if (pTarget->IsNpc()) {
					if (pTarget->GetSitSleepState() == SIT_SLEEP_NORMAL ||
						(pTarget->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
							pTarget->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
						pTarget->Activate(apActor, 0, 0, 1);
					}
					goto LABEL_300;
				}
				Animation* panim_5 = apActor->GetAnimation();
				if (bActivateAnim) {
					if (panim_5) {
						if (panim_5->SpecialIdleDonePlaying()) {
							ClearActionHeadTrackTarget(1);
							pTarget->Activate(apActor, 0, 0, 1);
							bActivateAnim = 0;
						}
					}
					goto LABEL_300;
				}
			}
		}
		bActivateAnim = 1;
		FreeUpSpecialIdle(apActor);
		FindSpecialIdletoPlay(apActor, pTarget->GetObjectReference(), pTarget);
		SetActionHeadTrackTarget(pTarget);
		return 0;
	}
	if (!GetRunOncePackage() || (v12 = GetRunOncePackage(), v12->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
		if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			if (pPack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
				DialoguePackage* pdia = static_cast<DialoguePackage*>(pPack);
				if (pPack->pPackLoc) {
					if (pdia->pPackLoc->GetLocReference()) {
						TESObjectREFR* pref_2 = pdia->pPackLoc->GetLocReference();
						if (pref_2->IsFurniture() && pdia->GetReachedAmbushLocation()) {
							return 0;
						}
					}
				}
			}
			apActor->InitiateGetUpPackage();
			return 0;
		}
	}
	const NiPoint3* v13 = &pTarget->GetLocationOnReference();
	float fdistancemoved = (lastSeenPostion - *v13).Length();
	if (!apActor->IsPathingComplete()) {
		float* v15;
		double v16;
		float v67;
		v67 = fdistance * 0.75;
		v15 = &fAIMoveDistanceToRecalcFollowPath.Float();
		v16 = BSMIN(*v15, v67);
		if (!(fdistancemoved > v16)) {
		LABEL_113:;
			if (!apActor->IsPathingComplete()) {
				TESPackage* v21;
				int32_t aemovemode = 256;
				if (apActor->bForceRun ||
					(pPack->GetAlwaysRun() ||
						(apActor->IsInCombat() ||
							(pPack->GetPackType() == PACKAGE_TYPE_ALARM ||
								(GetCurrentPackage() &&
									(v21 = GetCurrentPackage(), v21->GetPackType() == PACKAGE_TYPE_COMBAT)))))) {
					apActor->SetMoveMode(512u);
				}
				if (apActor->bForceSneak || (apActor->bPlayerTeammate && pPlayer->IsSneaking())) {
					if (!apActor->IsSneaking()) {
						apActor->SetWalkRunFlagDirty();
					}
					apActor->SetMoveMode(1024u);
				}
				NiPoint3 v92 = pTarget->GetLocationOnReference();
			}
			return 0;
		}
	}
	if (pTarget->GetTeleport()) {
		const NiPoint3* v17;
		TESObjectCELL* v34;
		TESWorldSpace* afAngleToFace;
		afAngleToFace = pTarget->GetWorldSpace();
		v34 = pTarget->GetParentCell();
		v17 = &pTarget->GetLinkedDoorTeleportPosition();
		if (!apActor->SetPathfindingGoal(*v17, v34, afAngleToFace, fdistance, 0)) {
			return 0;
		}
	LABEL_112:;
		lastSeenPostion = pTarget->GetLocationOnReference();
		goto LABEL_113;
	}
	if (!pTarget->IsFurniture()) {
		if (pTarget->IsActor() && pTarget->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
			Actor* pact = static_cast<Actor*>(pTarget);
			BaseProcess* v66 = pact->pCurrentProcess;
			FurnitureMark* pfurn = v66->GetFurnitureMarker();
			if (pfurn) {
				TESWorldSpace* afAngleToFacea = pTarget->GetWorldSpace();
				TESObjectCELL* v35 = pTarget->GetParentCell();
				if (!apActor->SetPathfindingGoal(pfurn->Position, v35, afAngleToFacea, 5.0f, 0)) {
					return 0;
				}
			} else {
				TESWorldSpace* afAngleToFaceb = pTarget->GetWorldSpace();
				TESObjectCELL* v36 = pTarget->GetParentCell();
				const NiPoint3* v19 = &pTarget->GetLocationOnReference();
				if (!apActor->SetPathfindingGoal(*v19, v36, afAngleToFaceb, fdistance, 0)) {
					return 0;
				}
			}
		} else {
			TESWorldSpace* afAngleToFacec = pTarget->GetWorldSpace();
			TESObjectCELL* v37 = pTarget->GetParentCell();
			const NiPoint3* v20 = &pTarget->GetLocationOnReference();
			if (!apActor->SetPathfindingGoal(*v20, v37, afAngleToFacec, fdistance, 0)) {
				return 0;
			}
		}
		goto LABEL_112;
	}
	NiPoint3 point_2 = apActor->GetLocationOnReference();
	uint32_t iindex_2 = 0;
	if (pTarget->GetClosestFreeMarker(point_2, 1, 1, FurnitureMarker, iindex_2, 1)) {
		::new (&request) PathingRequest;
		TESWorldSpace* v18 = pTarget->GetWorldSpace();
		apActor->BuildRequest(&request, FurnitureMarker.Position, pTarget->GetParentCell(), v18, 5.0f, 0);
		request.SetSoftGoalRadius(1);
		if (!apActor->SetPathfindingGoal(&request)) {
			request.~PathingRequest();
			return 0;
		}
		cCurrentFurnitureIndex = iindex_2;
		request.~PathingRequest();
		goto LABEL_112;
	}
	pCurrentFurniture = 0;
	EndMoveMessage(apActor);
	SetTarget(0);
	if (abInterateindex) {
		AddToProcedureIndexRunning(apActor, 1);
	}
	return 0;
}

void HighProcess::ProcessAccompany(Actor* apActor) {
	NiPoint3 apResult;
	NiPoint3 v29;
	TESPackage* v17 = GetPackageThatIsRunning();
	if (v17) {
		if (!v17->GetWaiting()) {
			uint32_t CastingType;
			if ((this->pTarget && this->pTarget->IsActor()) ||
				((SetTargetForPackage(apActor), this->pTarget) &&
					(this->pTarget->IsActor() ||
						(CastingType = this->pTarget->iFormID, apActor->HasObjectId(CastingType))))) {
				float v18 = 0.0f;
				Actor* pTarget = static_cast<Actor*>(this->pTarget);
				if (pTarget) {
					BaseProcess* CurrentAIProcess = pTarget->pCurrentProcess;
					if (pTarget->pCurrentProcess) {
						if (CurrentAIProcess->GetPackageThatIsRunning()) {
							NiPoint3 v8;
							v8 = CurrentAIProcess->GetCurrentDestinationCoordinate(pTarget, 1);
							TESObjectCELL* apCell = CurrentAIProcess->GetCurrentDestinationCell(pTarget);
							TESWorldSpace* apWorld = CurrentAIProcess->GetCurrentDestinationWorldSpace(pTarget);
							if (pTarget->IsDead(0) || pTarget->GetDisabled()) {
								AddToProcedureIndexRunning(apActor, 1);
								if (!apActor->IsPathingComplete()) {
									EndMoveMessage(apActor);
								}
							} else {
								NiPoint3 result;
								NiPoint3 v20;
								NiPoint3 v28;
								NiPoint3 v30;
								const NiPoint3* pt = &apActor->GetLocationOnReference();
								const NiPoint3* v23 = &pTarget->GetLocationOnReference();
								apActor->pActorMover->GetNextNodeLocation(&apResult);
								NiPoint3* p_apResult = &apResult;
								pTarget->pActorMover->GetNextNodeLocation(&v29);
								NiPoint3* arPos = &v29;
								result = v8 - *pt;
								v20 = v8 - *v23;
								v28 = *v23 - *pt;
								v30 = *arPos - *p_apResult;
								float EscortFollowDistance = static_cast<float>(v17->GetEscortFollowDistance());
								float v16 = EscortFollowDistance * EscortFollowDistance;
								float v11 = result.SqrLength();
								float v26 = v20.SqrLength();
								float v25 = v28.SqrLength();
								float v13 = v30.SqrLength();
								if (apActor->IsPathingComplete() &&
									(v26 > static_cast<double>(v11) || v16 > static_cast<double>(v25))) {
									EndMoveMessage(apActor);
									double DistanceFromReference = apActor->GetDistanceFromReference(pTarget, 0, 0);
									if (EscortFollowDistance + 300.0 < DistanceFromReference) {
										AddToProcedureIndexRunning(apActor, -1);
									}
								} else {
									if (!apActor->IsPathing() && v16 < static_cast<double>(v13)) {
										float v7 = std::sqrtf(v13);
										float v5 = EscortFollowDistance * 3.0;
										CalculateMoveMode(apActor, v7, EscortFollowDistance, v5, 0, 0);
										uint32_t MoveMode = pTarget->GetMoveMode();
										apActor->SetMoveMode(MoveMode);
										apActor->SetPathfindingGoal(*arPos, apCell, apWorld, EscortFollowDistance,
											static_cast<PathingAvoidNodeArray*>(0));
									}
								}
							}
						}
					}
				}
			} else {
				AddToProcedureIndexRunning(apActor, 1);
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
			}
		}
	}
}

void HighProcess::ProcessAcquire(Actor* apActor) {
	TESPackage* pPack = GetPackageThatIsRunning();
	if (pPack) {
		if (GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			apActor->InitiateGetUpPackage();
		}
		int32_t ainumb = pPack->GetInitialTargetCount();
		if (ainumb <= 0) {
			ainumb = 1;
		}
		PackageTarget* ptarg = pPack->GetPackageTarget();
		if (CheckIfHasObject(apActor, ainumb)) {
			if (GetCurrentPackage() &&
				GetCurrentPackage()->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
				AddToProcedureIndexRunning(apActor, 2);
				return;
			}
			goto LABEL_91;
		}
		if (!pTarget || (pTarget->GetDelete() || (pTarget->GetDisabled() || !pAcquireObject))) {
			SetTargetForPackage(apActor);
		}
		if (pPack->GetPackType() == PACKAGE_TYPE_EXPLORE &&
			(ptarg->GetTargObjectType() == POTYPE_WEAP ||
				(ptarg->GetTargObjectType() >= POTYPE_COMBATWEAR &&
					ptarg->GetTargObjectType() <= POTYPE_WEAP_RANGED))) {
			ProcessActivate(apActor, 1);
			if (GetProcedureIndexRunning() > 1) {
				if (ObjectList.IsEmpty()) {
					AddToProcedureIndexRunning(apActor, 1);
					if (ptarg->GetTargObjectType() == POTYPE_COMBATWEAR || ptarg->GetTargObjectType() == POTYPE_WEAR) {
						bool abwearweapon = 1;
						if (GetCurrentPackage()) {
							TESPackage* v2 = GetCurrentPackage();
							abwearweapon = !v2->GetForceUnequipWeapon();
						}
						if (apActor->IsNpc()) {
							TESNPC* pnpc = static_cast<TESNPC*>(apActor->GetObjectReference());
							if (pnpc) {
								pnpc->InitDefaultWorn(apActor, 1, abwearweapon, 0, 1);
							}
						} else {
							if (apActor->IsCreature()) {
								TESCreature* pcrea = static_cast<TESCreature*>(apActor->GetObjectReference());
								if (pcrea) {
									pcrea->InitDefaultWorn(apActor, 1, abwearweapon, 1);
								}
							}
						}
					}
				} else {
					AddToProcedureIndexRunning(apActor, -1);
					pAcquireObject = ObjectList.m_item;
					ObjectList.Remove(pAcquireObject);
					SetTarget(pAcquireObject->pRef);
				}
			}
			return;
		}
		if (CheckIfHasObject(apActor, ainumb)) {
			EndMoveMessage(apActor);
			if (GetRunOncePackage()) {
				goto LABEL_45;
			}
			if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) && pPack->GetPackType() == PACKAGE_TYPE_EAT) {
				SetProcedureIndexRunning(0);
				goto LABEL_46;
			}
			if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC ||
				pPack->GetPackageSchedule()->PSData.iDuration) {
			LABEL_45:;
				AddToProcedureIndexRunning(apActor, 1);
			} else {
				AddToProcedureIndexRunning(apActor, 2);
			}
		LABEL_46:;
			if (apActor->GetMoveMode() == 1024) {
				apActor->SetWalkRunFlagDirty();
				apActor->ClearMoveMode(1024u);
			}
			return;
		}
		if (!pTarget || (pTarget->GetDelete() || pTarget->GetDisabled())) {
			SetTargetForPackage(apActor);
		}
		if (pPack->GetPackType() == PACKAGE_TYPE_USE_ITEM_AT || pPack->GetPackType() == PACKAGE_TYPE_USE_WEAPON) {
			if (!pPack->GetPackageTarget() ||
				(pPack->GetPackageTarget()->GetTargType() == PTT_REFERENCE ||
					(pPack->GetPackageTarget()->GetTargType() == PTT_LINKEDREF || pPack->GetPackageSearchLocation()))) {
				PACKAGE_OBJECT_TYPE eptype;
				if (pTarget) {
					eptype = TESPackage::GetObjectTypeFromForm(pTarget->GetObjectReference());
				} else {
					eptype = pPack->GetPackageTarget()->GetTargObjectType();
				}
				switch (eptype) {
				case POTYPE_ACTI:
				case POTYPE_CONT:
				case POTYPE_DOOR:
				case POTYPE_FLOR:
				case POTYPE_FURN:
				case POTYPE_NPC:
				case POTYPE_CREA:
				case POTYPE_WEAP_NONE:
				case POTYPE_SPELL: {
					eUseItem = eptype;
					AddToProcedureIndexRunning(apActor, 1);
					break;
				}
				case POTYPE_LIGH: {
					TESObjectLIGH* plight = 0;
					if (pTarget->GetObjectReference()->GetFormType() == LIGH_ID) {
						plight = static_cast<TESObjectLIGH*>(pTarget->GetObjectReference());
					}
					if (plight) {
						if (!plight->GetCanCarry()) {
							AddToProcedureIndexRunning(apActor, 1);
						}
					}
					eUseItem = eptype;
					break;
				}
				default: {
					goto LABEL_71;
				}
				}
			} else {
				SetTarget(0);
				AddToProcedureIndexRunning(apActor, 1);
			}
			return;
		}
	LABEL_71:;
		if (pTarget) {
			if (!pTarget->IsActor()) {
				if (pAcquireObject) {
					SetActionHeadTrackTarget(pAcquireObject->pRef);
					if (pAcquireObject->eType == ACQUIRE_TYPE_STEALGROUND) {
						ProcessSteal(apActor);
						return;
					}
					if (GetCurrentPackage()->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EAT) {
						goto LABEL_86;
					}
				} else {
					if (!pPack->GetPackageTarget() || pPack->GetPackageTarget()->GetTargReference() != pTarget) {
						goto LABEL_91;
					}
				}
				ProcessActivate(apActor, 1);
				return;
			}
			if (pAcquireObject) {
				switch (pAcquireObject->eType) {
				case ACQUIRE_TYPE_PICKPOCKET: {
					ProcessPickPocket(apActor);
					return;
				}
				case ACQUIRE_TYPE_KILL: {
					ProcessKill(apActor);
					return;
				}
				case ACQUIRE_TYPE_BUY_NPC: {
					ProcessBuyObject(apActor);
					return;
				}
				}
			LABEL_86:;
				ProcessActivate(apActor, 0);
				return;
			}
		}
	LABEL_91:;
		AddToProcedureIndexRunning(apActor, 1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessAmbushWait(Actor* apActor) {
	NiPoint3 v16;
	// The exe builds it with PathingLocation's constructor (0x6DCEA0) and destroys it with 0x4FF7E0 on the cover
	// path only: no C++ constructor storing src's vtable.
	FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
	using apReturnArray_t = BSSimpleArray<PathingCoverLocation, 1024>;
	FNVD_INLINED_LOCAL(apReturnArray_t, apReturnArray, std::nullopt, std::nullopt);
	NiPoint3 a1;
	bool bLineOfSight;
	TESPackage* pPack = 0;
	pPack = GetPackageThatIsRunning();
	NiPoint3 pLoc = pPack->GetLocationCoord(apActor);
	a1 = pPack->GetSecondLocationCoord(apActor);
	float fMaxHeightChange = 1024.0f;
	float fMaxDist = 0.0f;
	fMaxDist = pPack->GetPackageRadiusActorToLocation(apActor, 0);
	int32_t iMaxCoverLocations = 200;
	FIGHT_REACTION a4 = FIGHT_REACTION_NEUTRAL;
	bLineOfSight = false;
	if (apActor->GetShouldAttackActor(pPlayer, 0, a4, 0) &&
		apActor->GetDetectionLevelAgainstActor(0, pPlayer, bLineOfSight, 0, 0, 0, 0) > 0) {
		double DistanceFromReference = pPlayer->GetDistanceFromReference(apActor, 0, 0);
		if (fAmbushOverRideRadiusforPlayerDetection.Float() >= DistanceFromReference) {
			apActor->StartCombat(pPlayer, 0, 1, 1, 0, 0, 1, 0);
			apActor->SetForceSneak(0);
			bHiding = 0;
		}
	}
	if (!pPack->GetPackageTarget()) {
		goto LABEL_20;
	}
	if (!pTarget || pTarget->GetDisabled()) {
		if (!(fEvaluateAcquireTimer <= 0.0)) {
			fEvaluateAcquireTimer = fEvaluateAcquireTimer - appTimer.fDelta;
		} else {
			SetTargetForPackage(apActor);
			if (!pTarget) {
				fEvaluateAcquireTimer = 10.0f;
			}
		}
	LABEL_20:;
		NiPoint3* LocationCoord = ::new (&v16) NiPoint3(pPack->GetLocationCoord(apActor));
		float fdistance = apActor->GetDistanceFromPoint(*LocationCoord);
		double fMaxDist_2 = pPack->GetPackageRadiusActorToLocation(apActor, 0);
		if (!(fdistance > fMaxDist_2 + 50.0)) {
			if (pPack->AmbushGetHideWhileAmbushing()) {
				if (bHiding) {
					apActor->SetForceSneak(1);
				} else {
					FNVD_CONSTRUCTED_LOCAL(PathingRequestCover, v20, &PathingRequestCover::Destruct);
					TESWorldSpace* SecondLocationWorld = pPack->GetSecondLocationWorld(apActor);
					::new (&apLocation) PathingLocation(&a1, SecondLocationWorld);
					v20.SetOrigin(apLocation);
					v20.SetMaxHeightChange(fMaxHeightChange);
					v20.SetMaxCoverDistance(fMaxDist);
					v20.SetMaxResults(200u);
					float a2 = apActor->GetHeight();
					v20.SetActorHeight(a2);
					float a2a = apActor->GetSneakHeight(0);
					v20.SetCrouchHeight(a2a);
					// 0x8EEAA0 (IDA: Animation::SkipUpdate) is the one-byte store of bCanCrouch.
					v20.bCanCrouch = apActor->CanSneak();
					::new (&apReturnArray) BSSimpleArray<PathingCoverLocation, 1024>;
					if (Pathing::GetCoverPoints(&v20, apReturnArray)) {
						if (apReturnArray.iSize) {
							PathingCoverLocation* coverLoc = &apReturnArray.GetAt(0);
							TESWorldSpace* WorldspaceSimple = coverLoc->pWorldSpace;
							TESObjectCELL* Cell = coverLoc->GetCell();
							apActor->SetPathfindingGoal(coverLoc->GetLocation(), Cell, WorldspaceSimple, -1.0f, 0);
							bHiding = 1;
						}
					}
					apReturnArray.~BSSimpleArray<PathingCoverLocation, 1024>();
					apLocation.~PathingLocation();
				}
			} else {
				if (pPack->GetPackageRadiusActorToLocation(apActor, 0) > 400.0) {
					float fRadius = pPack->GetPackageRadiusActorToLocation(apActor, 0);
					const NiPoint3* v7 = &apActor->GetLocationOnReference();
					ProcessWander(apActor, v7, fRadius);
				}
			}
		} else {
			apActor->SetForceSneak(0);
			bHiding = 0;
			AddToProcedureIndexRunning(apActor, -1);
		}
		if (pTarget) {
			bool batRef = 0;
			batRef = pPack->IsTargetAtSecondLocation(apActor, -1.0f);
			if (batRef && pTarget->IsActor() && (pTarget != pPlayer || !pPlayer->btransporting)) {
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
				FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
				if (!apActor->GetShouldAttackActor(static_cast<Actor*>(pTarget), 0, ereaction, 0)) {
					apActor->StartCombat(static_cast<Actor*>(pTarget), 0, 1, 1, 0, 0, 1, 0);
					bHiding = 0;
					apActor->SetForceSneak(0);
				}
			}
		}
		return;
	}
	if (!pTarget->GetDelete()) {
		if (pTarget->IsDead(1) && !pAcquireObject) {
			pPack->SetNeverRun(pTarget, 1);
			apActor->SetForceSneak(0);
			bHiding = 0;
			return;
		}
		goto LABEL_20;
	}
	if (pTarget->GetDelete()) {
		pPack->SetNeverRun(pTarget, 1);
	}
	apActor->SetForceSneak(0);
	bHiding = 0;
	AddToProcedureIndexRunning(apActor, 2);
}

bool HighProcess::ProcessUseItemAt(Actor* apActor) {
	bool result;
	TESPackage* ppack = 0;
	ppack = GetPackageThatIsRunning();
	double DistanceFromReference = apActor->GetDistanceFromReference(pPlayer, 0, 0);
	if (fAIIdleAnimationDistance.Float() < DistanceFromReference) {
		return 1;
	}
	TESForm* pFormObject = 0;
	TESObjectREFR* plocRef = 0;
	int32_t itimesdoperform = 1;
	PackageLocation* ploc = ppack->pPackLoc;
	InventoryChanges* pchangecont = InventoryChanges::GetInventoryChanges(apActor);
	PackageTarget* ptarg = ppack->GetPackageTarget();
	if (ploc) {
		if (ploc->GetLocReference()) {
			plocRef = ploc->GetLocReference();
		}
	}
	if (!plocRef && !GetRunOncePackage()) {
		plocRef = pGenericLocation;
	}
	if (!ppack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
		AddToProcedureIndexRunning(apActor, -1);
		return 0;
	}
	if (plocRef) {
		TESBoundObject* ObjectReference = plocRef->GetObjectReference();
		if (ObjectReference == pXMarkerHeading && !bAimingTarget) {
			float v51 = 0.0f;
			float ftargetangle = (&plocRef->GetAngleOnReference())->z;
			float fignore = 0.0f;
			float afStartAngle = apActor->GetHeading(0);
			float fangledif = GetAngleDelta(afStartAngle, ftargetangle, fignore);
			float fdiff = static_cast<double>(iActorKeepTurnDegree.Int()) * 0.01745329238474369;
			double v4 = std::fabs(fangledif);
			if (fdiff < v4) {
				apActor->RequestRotate(ftargetangle, 1);
				SetActionHeadTrackTarget(0);
				return 0;
			}
			apActor->StopMoving();
			ClearActionHeadTrackTarget(1);
			const NiPoint3* v5 = &plocRef->GetLocationOnReference();
			apActor->SetLocationOnReference(*v5);
			apActor->SetHeading((&plocRef->GetAngleOnReference())->z);
			EndMoveMessage(apActor);
		}
	}
	if (ptarg) {
		if (ploc) {
			if (ploc->GetLocReference()) {
				plocRef = ploc->GetLocReference();
			}
		}
		if (!plocRef && !GetRunOncePackage()) {
			plocRef = pGenericLocation;
		}
		if (!pTarget) {
			SetTarget(plocRef);
		}
		if (pTarget && (ptarg->GetTargType() == PTT_REFERENCE || ptarg->GetTargType() == PTT_LINKEDREF)) {
			pFormObject = (TESForm*)pTarget->GetObjectReference();
			eUseItem = TESPackage::GetObjectTypeFromForm(pTarget->GetObjectReference());
		} else {
			if (ptarg->GetTargType() == PTT_OBJECT) {
				pFormObject = ptarg->GetTargObject();
				eUseItem = TESPackage::GetObjectTypeFromForm(pFormObject);
			} else {
				if (ptarg->GetTargType() == PTT_TYPE) {
					eUseItem = ptarg->GetTargObjectType();
				}
			}
		}
		if (pTarget) {
			if (eUseItem == POTYPE_NONE) {
				pFormObject = (TESForm*)pTarget->GetObjectReference();
				eUseItem = TESPackage::GetObjectTypeFromForm(pTarget->GetObjectReference());
				if (pGenericLocation) {
					SetTarget(pGenericLocation);
					pFormObject = (TESForm*)pGenericLocation->GetObjectReference();
					eUseItem = TESPackage::GetObjectTypeFromForm(pGenericLocation->GetObjectReference());
				} else {
					eUseItem = POTYPE_NONE;
				}
			}
		}
	} else {
		eUseItem = POTYPE_NONE;
	}
	if (!pTarget && !CheckIfHasObject(apActor, 1)) {
		AddToProcedureIndexRunning(apActor, 1);
		return 0;
	}
	TESObjectREFR* poriginalTarget = pTarget;
	if (ppack->GuardGetTargetIsLinkedRef() && apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
		bool bfindChair = 1;
		if (pCurrentFurniture) {
			TESFurniture* pfurnBase = static_cast<TESFurniture*>(pCurrentFurniture->GetObjectReference());
			bool v23 = !pfurnBase->CanSitOn() || !pCurrentFurniture->HasFreeMarker(0);
			bfindChair = v23;
		}
		if (bfindChair) {
			FindBedChairs(apActor, 1, 0);
			if (RefListChairBed.ItemsInList()) {
				if (TESObjectREFR::kFurnitureLock.TryLock()) {
					pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
					ClearFurnitureMarker();
					while (pCurrentFurniture && !pCurrentFurniture->HasFreeMarker(0)) {
						RefListChairBed.Remove(*reinterpret_cast<void**>(&pCurrentFurniture));
						pCurrentFurniture = static_cast<TESObjectREFR*>((&RefListChairBed)->m_item);
						ClearFurnitureMarker();
					}
					if (pCurrentFurniture) {
						if (!pCurrentFurniture->GetOwner()) {
							int32_t inumber = RefListChairBed.ItemsInList();
							uint32_t v6 = Random();
							int32_t irandom = v6 % inumber;
							if (irandom >= inumber) {
								irandom = inumber;
							}
							BSSimpleList<void*>* plist = &RefListChairBed;
							for (int32_t i = 0; i < irandom; ++i) {
								plist = plist->m_pkNext;
							}
							pCurrentFurniture = static_cast<TESObjectREFR*>(plist->m_item);
							ClearFurnitureMarker();
						}
					}
					TESObjectREFR::kFurnitureLock.Unlock();
				}
			}
			RefListChairBed.RemoveAll();
		}
		if (pCurrentFurniture) {
			SetTarget(pCurrentFurniture);
			ProcessActivate(apActor, 0);
			SetTarget(0);
		}
		if (DidPathingFail(apActor)) {
			pCurrentFurniture = 0;
			ClearFurnitureMarker();
			EndMoveMessage(apActor);
			SetIdleTimer(0.0f);
		}
		if (pCurrentFurniture) {
			return 0;
		}
		SetTarget(poriginalTarget);
	}
	switch (eUseItem) {
	case POTYPE_ACTI: {
		goto LABEL_90;
	}
	case POTYPE_CLOT:
	case POTYPE_COMBATWEAR: {
		TESBoundObject* v9;
		PackageTarget* ptarg_2;
		TESForm* pobj;
		ptarg_2 = ppack->GetPackageTarget();
		pobj = 0;
		if (!CheckIfHasObject(apActor, 1)) {
			AddToProcedureIndexRunning(apActor, 2);
			FreeUpSpecialIdle(apActor);
			EndMoveMessage(apActor);
			return 0;
		}
		if (ptarg_2->GetTargType() == PTT_OBJECT) {
			pobj = ptarg_2->GetTargObject();
		} else {
			if (ptarg_2->GetTargType()) {
				if (ptarg_2->GetTargType() == PTT_LINKEDREF) {
					TESBoundObject* pobj_2;
					TESObjectREFR* pref = apActor->GetLinkedRef();
					if (pref) {
						pobj_2 = pref->GetObjectReference();
					} else {
						pobj_2 = 0;
					}
					pobj = (TESForm*)pobj_2;
				}
			} else {
				pobj = (TESForm*)ptarg_2->GetTargReference()->GetObjectReference();
			}
		}
		if (pobj) {
			TESForm* parmor = pobj->GetFormType() == ARMO_ID ? pobj : 0;
		}
	LABEL_90:;
		if (itimesdoperform > 0 && sShotsFired >= itimesdoperform && apActor->GetAnimAction() == ANIM_ACTION_NONE) {
			Animation* v8 = apActor->GetAnimation();
			if (v8->SpecialIdleDonePlaying()) {
				AddToProcedureIndexRunning(apActor, 2);
				FreeUpSpecialIdle(apActor);
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
				return 1;
			}
		}
		if ((plocRef && (v9 = plocRef->GetObjectReference(), v9 == pXMarkerHeading)) ||
			(ppack->pPackLoc && ppack->pPackLoc->GetLocType() == PLT_EDITORLOC)) {
			if (!DidPathingFail(apActor) && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL && !GetForceRotate()) {
				float ftargetangle_2 = 0.0f;
				if (plocRef) {
					ftargetangle_2 = (&plocRef->GetAngleOnReference())->z;
				} else {
					ftargetangle_2 = apActor->GetStartingAngle().z;
				}
				apActor->RequestRotate(ftargetangle_2, 0);
			}
		}
		float ftargetangle_3 = 0.0f;
		if (plocRef) {
			ftargetangle_3 = (&plocRef->GetAngleOnReference())->z;
		} else {
			ftargetangle_3 = apActor->GetStartingAngle().z;
		}
		float fignore_2 = 0.0f;
		float afStartAnglea = apActor->GetHeading(0);
		float fangledif_2 = GetAngleDelta(afStartAnglea, ftargetangle_3, fignore_2);
		float fdiff_2 = static_cast<double>(iActorKeepTurnDegree.Int()) * 0.01745329238474369;
		double v10 = std::fabs(fangledif_2);
		if (fdiff_2 < v10) {
			SetActionHeadTrackTarget(0);
			apActor->RequestRotate(ftargetangle_3, 0);
			return 0;
		}
		apActor->StopMoving();
		if (ProcessActivate(apActor, 1)) {
			++sShotsFired;
		}
		if (ppack->GetOncePerDay() && !ppack->GetPackageSchedule()->PSData.iDuration) {
			Animation* v11 = apActor->GetAnimation();
			if (v11->SpecialIdleDonePlaying()) {
				ClearActionHeadTrackTarget(1);
				AddToProcedureIndexRunning(apActor, 2);
			}
		}
		if (itimesdoperform > 0 && (sShotsFired >= itimesdoperform && apActor->GetAnimAction() == ANIM_ACTION_NONE)) {
			AddToProcedureIndexRunning(apActor, 2);
			ClearShootingAction(apActor);
			FreeUpSpecialIdle(apActor);
			if (!apActor->IsPathingComplete()) {
				EndMoveMessage(apActor);
			}
			return 1;
		}
		fUseItemTimer = fUseItemTimer - appTimer.fDelta;
		if (ppack->GetOncePerDay() && !ppack->GetPackageSchedule()->PSData.iDuration) {
			Animation* v12 = apActor->GetAnimation();
			if (v12->SpecialIdleDonePlaying()) {
				AddToProcedureIndexRunning(apActor, 2);
				ClearShootingAction(apActor);
			}
		}
		goto LABEL_165;
	}
	case POTYPE_WEAP:
	case POTYPE_AMMO:
	case POTYPE_WEAP_NONE:
	case POTYPE_WEAP_MELEE:
	case POTYPE_WEAP_RANGED: {
		char buffer[256];
		const char* v7 = ppack->GetFormEditorID();
		BSsprintf(buffer, 256, "UseItem package %s has a weapon target; should be UseWeapon.", v7);
		Warning(buffer);
		return 0;
	}
	case POTYPE_NPC:
	case POTYPE_CREA: {
		goto LABEL_165;
	}
	default: {
		Animation* v13;
		if (pFormObject) {
			SetItemBeingUsed(static_cast<TESBoundObject*>(pFormObject));
		}
		if (pItemBeingUsed &&
			(itimesdoperform > 0 &&
				(sShotsFired >= itimesdoperform &&
					(apActor->GetAnimAction() == ANIM_ACTION_NONE &&
						(v13 = apActor->GetAnimation(), v13->SpecialIdleDonePlaying()))))) {
			apActor->QueueEquipObject(pItemBeingUsed, 1, 0, 1, 0, 1);
			AddToProcedureIndexRunning(apActor, 2);
			ClearShootingAction(apActor);
			FreeUpSpecialIdle(apActor);
			if (!apActor->IsPathingComplete()) {
				EndMoveMessage(apActor);
			}
			result = 1;
		} else {
			TESBoundObject* v14;
			Animation* panim;
			if ((plocRef && (v14 = plocRef->GetObjectReference(), v14 == pXMarkerHeading)) ||
				(ppack->pPackLoc && ppack->pPackLoc->GetLocType() == PLT_EDITORLOC)) {
				float ftargetangle_5 = 0.0f;
				if (plocRef) {
					ftargetangle_5 = (&plocRef->GetAngleOnReference())->z;
				} else {
					ftargetangle_5 = apActor->GetStartingAngle().z;
				}
				float fignore_3 = 0.0f;
				float afStartAngleb = apActor->GetHeading(0);
				float fangledif_3 = GetAngleDelta(afStartAngleb, ftargetangle_5, fignore_3);
				float fdiff_3 = static_cast<double>(iActorKeepTurnDegree.Int()) * 0.01745329238474369;
				double v15 = std::fabs(fangledif_3);
				if (fdiff_3 < v15) {
					apActor->RequestRotate(ftargetangle_5, 0);
					SetActionHeadTrackTarget(0);
					return 0;
				}
				apActor->StopMoving();
				ClearActionHeadTrackTarget(1);
			}
			panim = apActor->GetAnimation();
			if (pItemBeingUsed || eUseItem == POTYPE_NONE) {
				if (pItemBeingUsed || eUseItem == POTYPE_NONE) {
					if (panim) {
						if (panim->SpecialIdleDonePlaying()) {
							TESIdleManager::SetUsedItem(pItemBeingUsed);
							SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
							float v21 = static_cast<double>(Random() % 0x1388) * 0.0010000000474974513 + 5.0;
							SetIdleTimer(v21);
							TESIdleManager::SetUsedItem(0);
							++sShotsFired;
						}
					}
				}
				if (panim->SpecialIdleDonePlaying()) {
					if (pItemBeingUsed) {
						apActor->QueueEquipObject(pItemBeingUsed, 1, 0, 1, 0, 1);
					}
					if (ppack->GetOncePerDay() && !ppack->GetPackageSchedule()->PSData.iDuration &&
						panim->SpecialIdleDonePlaying()) {
						AddToProcedureIndexRunning(apActor, 2);
						ClearShootingAction(apActor);
					}
				}
			LABEL_165:;
				result = 0;
			} else {
				AddToProcedureIndexRunning(apActor, -2);
				result = 0;
			}
		}
		break;
	}
	}
	return result;
}

bool HighProcess::ProcessUseWeapon(Actor* apActor) {
	TESBoundObject* ObjectReference;
	int32_t icount;
	int32_t numobjs;
	TESPackage* pPack = GetPackageThatIsRunning();
	double DistanceFromReference = apActor->GetDistanceFromReference(pPlayer, 0, 0);
	if (fAIUseWeaponDistance.Float() < DistanceFromReference) {
		return 1;
	}
	int32_t irand = Random() % 100;
	TESObjectWEAP* pweap = 0;
	TESForm* pFormObject = 0;
	TESObjectREFR* pRef = 0;
	PackageLocation* ploc = pPack->pPackLoc;
	InventoryChanges* pchangecont = InventoryChanges::GetInventoryChanges(apActor);
	PackageTarget* ptarg = pPack->GetPackageTarget();
	const TESUseWeaponPackageData* ppackData = pPack->GetUseWeaponPackageData();
	SetTargetForPackage(apActor);
	if (ploc) {
		if (ploc->GetLocReference()) {
			pRef = ploc->GetLocReference();
		}
	}
	if (!pRef) {
		pRef = pGenericLocation;
	}
	if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
		CleanUpAfterProcessUseWeapon(apActor);
		AddToProcedureIndexRunning(apActor, -1);
		return 0;
	}
	if (pRef &&
		((ObjectReference = pRef->GetObjectReference(), ObjectReference == pXMarkerHeading) && !bAimingTarget)) {
		float v87 = 0.0f;
		float ftargetangle = (&pRef->GetAngleOnReference())->z;
		float fignore = 0.0f;
		float afStartAngle = apActor->GetHeading(0);
		float fangledif = GetAngleDelta(afStartAngle, ftargetangle, fignore);
		float fdiff = static_cast<double>(iActorKeepTurnDegree.Int()) * 0.01745329238474369;
		double v4 = std::fabs(fangledif);
		if (fdiff < v4) {
			apActor->RequestRotate(ftargetangle, 0);
			SetActionHeadTrackTarget(0);
			return 0;
		}
		apActor->StopMoving();
		ClearActionHeadTrackTarget(1);
		apActor->SetHeading((&pRef->GetAngleOnReference())->z);
		EndMoveMessage(apActor);
	} else {
		if (apActor->IsMoving()) {
			apActor->StopMoving();
			EndMoveMessage(apActor);
		}
	}
	Actor* pattacktarg = 0;
	UseWeaponActorPackageData* pdata = static_cast<UseWeaponActorPackageData*>(GetInstanceDataThatIsRunning());
	if (pdata) {
		pattacktarg = static_cast<Actor*>(pdata->pAttackTarget);
	}
	if (!pattacktarg) {
		pattacktarg = static_cast<Actor*>(pRef);
		PackageTarget* ppackTarget = ppackData->pAttackTarget;
		if (ppackTarget) {
			switch (ppackTarget->GetTargType()) {
			case PTT_REFERENCE: {
				pattacktarg = static_cast<Actor*>(ppackTarget->GetTargReference());
				break;
			}
			case PTT_OBJECT:
			case PTT_TYPE: {
				char buffer[256];
				BSsprintf(buffer, 256,
					"UseWeapon package %s should have a referenceas its attack target; object/type targets are not "
					"supported (yet).",
					pPack->GetFormEditorID());
				Warning(buffer);
				break;
			}
			case PTT_LINKEDREF: {
				pattacktarg = static_cast<Actor*>(apActor->GetLinkedRef());
				break;
			}
			default: {
				break;
			}
			}
		}
		if (pdata) {
			pdata->pAttackTarget = pattacktarg;
		}
	}
	if (ptarg) {
		if (pTarget && (ptarg->GetTargType() == PTT_REFERENCE || ptarg->GetTargType() == PTT_LINKEDREF)) {
			pFormObject = (TESForm*)pTarget->GetObjectReference();
			eUseItem = TESPackage::GetObjectTypeFromForm(pTarget->GetObjectReference());
		} else {
			if (ptarg->GetTargType() == PTT_OBJECT) {
				pFormObject = ptarg->GetTargObject();
				eUseItem = TESPackage::GetObjectTypeFromForm(pFormObject);
			} else {
				if (ptarg->GetTargType() == PTT_TYPE) {
					eUseItem = ptarg->GetTargObjectType();
				}
			}
		}
		if (pTarget) {
			if (eUseItem == POTYPE_NONE) {
				pFormObject = (TESForm*)pTarget->GetObjectReference();
				eUseItem = TESPackage::GetObjectTypeFromForm(pTarget->GetObjectReference());
				if (pGenericLocation) {
					SetTarget(pGenericLocation);
					pFormObject = (TESForm*)pGenericLocation->GetObjectReference();
					eUseItem = TESPackage::GetObjectTypeFromForm(pGenericLocation->GetObjectReference());
				} else {
					eUseItem = POTYPE_NONE;
				}
			}
		}
	} else {
		eUseItem = POTYPE_NONE;
	}
	if (GetCurrentWeapon()) {
		if (GetCurrentWeapon()->pContainerObj) {
			ItemChange* v5 = GetCurrentWeapon();
			if (v5->pContainerObj->GetFormType() == WEAP_ID) {
				pweap = static_cast<TESObjectWEAP*>(GetCurrentWeapon()->pContainerObj);
			}
		}
	}
	switch (eUseItem) {
	case POTYPE_WEAP:
	case POTYPE_AMMO:
	case POTYPE_WEAP_NONE:
	case POTYPE_WEAP_MELEE:
	case POTYPE_WEAP_RANGED: {
		bool result;
		ptarg = pPack->GetPackageTarget();
		if (!CheckIfHasObject(apActor, 1) && eUseItem != POTYPE_WEAP_NONE) {
			CleanUpAfterProcessUseWeapon(apActor);
			AddToProcedureIndexRunning(apActor, 2);
			return 0;
		}
		if (pweap) {
			switch (ptarg->GetTargType()) {
			case PTT_REFERENCE:
			case PTT_LINKEDREF: {
				TESObjectREFR* pref = GetActorPackageThatIsRunning()->pTarget;
				if (!pref) {
					goto LABEL_64;
				}
				TESObjectWEAP* v8 = static_cast<TESObjectWEAP*>(pref->GetObjectReference());
				if (pweap == v8) {
					goto LABEL_64;
				}
				apActor->QueueEquipObject(pref->GetObjectReference(), 1, 0, 1, 0, 1);
				result = 1;
				break;
			}
			case PTT_OBJECT: {
				TESForm* pFormObject_5 = ptarg->GetTargObject();
				if (pweap == pFormObject_5) {
					goto LABEL_64;
				}
				TESForm* pFormObject_6 = ptarg->GetTargObject();
				apActor->QueueEquipObject(static_cast<TESBoundObject*>(pFormObject_6), 1, 0, 1, 0, 1);
				result = 1;
				break;
			}
			case PTT_TYPE: {
				PACKAGE_OBJECT_TYPE TargObjectType = ptarg->GetTargObjectType();
				if (TESPackage::FormMatchesPackageObjectType(pweap, TargObjectType)) {
					goto LABEL_64;
				}
				numobjs = 0;
				TESBoundObject* pobject = pchangecont->GetObjectbyPackObjType(eUseItem, numobjs);
				if (!pobject) {
					goto LABEL_64;
				}
				apActor->QueueEquipObject(pobject, 1, 0, 1, 0, 1);
				result = 1;
				break;
			}
			default: {
				goto LABEL_64;
			}
			}
		} else {
		LABEL_64:;
			if (this->sShotsToFire <= 0) {
				if (pweap) {
					int32_t FormClipRounds = pweap->GetFormClipRounds(0);
					this->sShotsToFire = BSMAX(1, FormClipRounds);
					if (ppackData) {
						if (ppackData->bVolleyFire) {
							int16_t sShotsToFire;
							int32_t a1 = ppackData->GetVolleyShotsMax();
							uint16_t VolleyShotsMin = ppackData->GetVolleyShotsMin();
							int32_t ipotentialShotsToFire = RandomInt(VolleyShotsMin, a1);
							if (ipotentialShotsToFire <= 0) {
								sShotsToFire = this->sShotsToFire;
							} else {
								sShotsToFire = ipotentialShotsToFire;
							}
							this->sShotsToFire = sShotsToFire;
						}
					}
				}
			}
			if (this->sShotsToFire > 0 && sShotsFired >= this->sShotsToFire) {
				++this->sBurstsFired;
				this->sShotsToFire = -1;
				sShotsFired = 0;
				bool v38 = false;
				if (pweap) {
					if (pweap->IsThrownWeapon() && !ppackData->bRepeatFire) {
						int32_t sBurstsFired = this->sBurstsFired;
						if (sBurstsFired >= ppackData->sBurstCount) {
							v38 = true;
						}
					}
				}
				if (v38) {
					apActor->SetWantWeaponDrawn(0);
					apActor->QueueUnEquipObject(pweap, 1, 0, 0, 0, 1);
					fUseItemTimer = 0.0f;
				} else {
					if (ppackData->bVolleyFire) {
						float a1a = ppackData->fVolleyWaitMax;
						float afTargetAngle = ppackData->fVolleyWaitMin;
						fUseItemTimer = RandomFloat(afTargetAngle, a1a);
					}
				}
			}
			if (ppackData && (ppackData->IsCrouch() && !pPack->GetAlwaysSneak())) {
				Animation* panim = apActor->GetAnimation();
				ItemChange* pammoItem = GetCurrentAmmo();
				bool v37 = pammoItem && (pammoItem->iNumber <= 1 && (pweap && pweap->GetAmmoPerShot()));
				bool v36 = apActor->IsReloading() || (panim && panim->ReloadAnimQueued());
				uint16_t Group = panim->GetGroup(AGS_WEAPON);
				ANIM_GROUP_ENUM eweapAnim = TESAnimGroup::GetType(Group);
				bool v35 = eweapAnim >= ANIM_GROUP_EQUIP && eweapAnim <= ANIM_GROUP_COUNTER;
				bool bshouldCrouch = !v35 && (v37 || v36);
				uint16_t v14 = panim->GetGroup(AGS_IDLE);
				bool bisCrouching = TESAnimGroup::GetMove(v14) == AGM_WALK;
				apActor->SetForceSneak(bshouldCrouch);
				if (v37) {
					if (v35) {
						panim->BlendOut(AGS_WEAPON, 0);
					} else {
						if (bisCrouching && !v36) {
							BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_IDLE);
							bool v33 = pseq && pseq->GetState() == NiControllerSequence::kAnimating;
							if (v33) {
								apActor->QueueReload();
							}
						}
					}
				}
				if (bshouldCrouch || (bisCrouching || (v37 && v35))) {
					if (pattacktarg) {
						AimAtTarget(apActor, pattacktarg, 0);
					}
					if (pattacktarg) {
						SetUseWeaponHeadTrackTarget(pattacktarg);
					}
					return 0;
				}
			} else {
				if (pweap) {
					if (!pweap->IsThrownWeapon()) {
						Animation* panim_2 = apActor->GetAnimation();
						ItemChange* pammoItem_2 = GetCurrentAmmo();
						bool v32 = pammoItem_2 && (pammoItem_2->iNumber <= 1 && (pweap && pweap->GetAmmoPerShot()));
						BaseProcess::POSTANIM_ACTION eactions =
							static_cast<BaseProcess::POSTANIM_ACTION>(GetPostAnimationActions());
						bool v31 = apActor->IsReloading() ||
							((panim_2 && panim_2->ReloadAnimQueued()) ||
								(eactions & BaseProcess::POSTANIM_ACTION_RELOAD_WEAPON) != 0);
						if (v32 && !v31) {
							apActor->QueueReload();
							return 0;
						}
					}
				}
			}
			if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
				ClearShootingAction(apActor);
				AddToProcedureIndexRunning(apActor, -1);
				return 0;
			}
			if (GetCurrentWeapon()) {
				pweap = 0;
				if (GetCurrentWeapon()->pContainerObj) {
					ItemChange* v15 = GetCurrentWeapon();
					if (v15->pContainerObj->GetFormType() == WEAP_ID) {
						pweap = static_cast<TESObjectWEAP*>(GetCurrentWeapon()->pContainerObj);
					}
				}
			}
			if (eUseItem == POTYPE_WEAP_NONE) {
				if (pweap) {
					apActor->QueueUnEquipObject(pweap, 1, 0, 0, 0, 1);
					return 1;
				}
			} else {
				if (!pweap) {
					TESBoundObject* pObject = 0;
					icount = 0;
					ItemChange* pchange = 0;
					if (pTarget) {
						TESBoundObject* afTargetAnglea = pTarget->GetObjectReference();
						pchange = pchangecont->GetInventoryItem(afTargetAnglea, 0);
					} else {
						if (pFormObject) {
							pchange = pchangecont->GetInventoryItem(static_cast<TESBoundObject*>(pFormObject), 0);
						} else {
							pObject = pchangecont->GetObjectbyPackObjType(eUseItem, icount);
						}
					}
					if (!pObject) {
						if (pchange) {
							pObject = pchange->pContainerObj;
						}
					}
					if (pObject) {
						apActor->QueueEquipObject(pObject, 1, 0, 1, 0, 1);
					} else {
						CleanUpAfterProcessUseWeapon(apActor);
						AddToProcedureIndexRunning(apActor, 2);
					}
					if (pchange) {
						apActor->QueueEquipObject(pchange->pContainerObj, 1, 0, 1, 0, 1);
					}
					if (pchange) {
						delete pchange;
					}
					return 1;
				}
			}
			bool breadyToFire = fUseItemTimer <= 0.0;
			if (breadyToFire && !ppackData->bRepeatFire) {
				int32_t v16 = this->sBurstsFired;
				if (v16 >= ppackData->sBurstCount) {
					CleanUpAfterProcessUseWeapon(apActor);
					AddToProcedureIndexRunning(apActor, 2);
					return 1;
				}
			}
			if (breadyToFire) {
				if (pattacktarg) {
					if (pRef != pattacktarg) {
						breadyToFire = AimAtTarget(apActor, pattacktarg, 0);
					}
				}
			}
			if (breadyToFire) {
				if (pPack->GetPackageSecondLocation()) {
					if (pattacktarg) {
						if (pattacktarg->IsMobileObject()) {
							MobileObject* ptargetMob = (MobileObject*)pattacktarg;
							breadyToFire = pPack->IsActorAtSecondLocation(pattacktarg, apActor, 0, -1.0f, 0);
						} else {
							breadyToFire = 0;
						}
					}
				}
			}
			if (breadyToFire && (ppackData && ppackData->IsGetHoldFire())) {
				float fdistSquaredToTarget_2 = apActor->GetDistanceFromReference(pattacktarg, 0, 0);
				fdistSquaredToTarget_2 = fdistSquaredToTarget_2 * fdistSquaredToTarget_2;
				BSSimpleList<DetectionState*>* pdetected = pDetectedActorList;
				while (pdetected && !pdetected->IsEmpty()) {
					DetectionState* pdetectState = pdetected->m_item;
					pdetected = pdetected->m_pkNext;
					if (pdetectState->pActor != pattacktarg && !pdetectState->pActor->IsDead(0) &&
						pdetectState->bLineofSight && pdetectState->iLevel > 0) {
						NiPoint3 toDetectedActor = pdetectState->LastPositionDetected;
						const NiPoint3* v17 = &apActor->GetLocationOnReference();
						toDetectedActor.operator-=(*v17);
						double v18 = toDetectedActor.SqrLength();
						if (!(fdistSquaredToTarget_2 <= v18)) {
							float fignore_2;
							float ftargetangle_2 = GetZAngleFromVector(toDetectedActor);
							float ftargetangle_3 = ftargetangle_2;
							float afStartAnglea = apActor->GetHeading(0);
							float ftargetangledelta = GetAngleDelta(afStartAnglea, ftargetangle_3, fignore_2);
							if (std::fabs(ftargetangledelta) < 0.5235987901687622) {
								breadyToFire = 0;
								break;
							}
						}
					}
				}
			}
			if (GetWeaponDrawn() || GetWantWeaponDrawn()) {
				if (breadyToFire) {
					int32_t irand_2 = Random() % 100;
					ANIM_GROUP_ENUM eattack = ANIM_GROUP_NONE;
					TESObjectWEAP* pweap_2 = 0;
					if (GetCurrentWeapon()) {
						pweap_2 = 0;
						if (GetCurrentWeapon()->pContainerObj) {
							ItemChange* v19 = GetCurrentWeapon();
							if (v19->pContainerObj->GetFormType() == WEAP_ID) {
								pweap_2 = static_cast<TESObjectWEAP*>(GetCurrentWeapon()->pContainerObj);
							}
						}
					}
					if (pweap_2 && pweap_2->GetWeaponType() >= WEAPON_ONE_HAND_PISTOL) {
						FireAtObject(apActor, pweap_2);
					} else {
						if (apActor->IsPathingComplete()) {
							if (GetAnimAction() == ANIM_ACTION_NONE) {
								// The exe's table of attack groups and their chances (PDB local animProbData).
								struct AnimProbData {
									ANIM_GROUP_ENUM eGroup;
									int32_t iProb;
								};
								AnimProbData animProbData[6];
								bool canDoAttack[6];
								NiPoint3 attackDelta;
								int32_t totalProbRange;
								int32_t i;
								if (pattacktarg) {
									if (pattacktarg->IsActor()) {
										if (irand_2 >= 40 &&
											(pattacktarg->GetAnimAction() == ANIM_ACTION_ATTACK ||
												pattacktarg->GetAnimAction() == ANIM_ACTION_ATTACK_FOLLOW_THROUGH)) {
											apActor->SetBlock(1);
											fUseItemTimer = 1.2f;
											return 0;
										}
									}
								}
								animProbData[0].eGroup = ANIM_GROUP_ATTACK_BACK_POWER;
								animProbData[0].iProb = 10;
								animProbData[1].eGroup = ANIM_GROUP_ATTACK_RIGHT_POWER;
								animProbData[1].iProb = 10;
								animProbData[2].eGroup = ANIM_GROUP_ATTACK_NORMAL_POWER;
								animProbData[2].iProb = 10;
								animProbData[3].eGroup = ANIM_GROUP_ATTACK_LEFT_POWER;
								animProbData[3].iProb = 10;
								animProbData[4].eGroup = ANIM_GROUP_ATTACK_LEFT;
								animProbData[4].iProb = 30;
								animProbData[5].eGroup = ANIM_GROUP_ATTACK_RIGHT;
								animProbData[5].iProb = 30;
								totalProbRange = 0;
								for (i = 0; i < 6; ++i) {
									uint16_t AnimGroup = apActor->GetAnimGroup(animProbData[i].eGroup, 0, 0, 0);
									bool IsAttackAction = TESAnimGroup::IsAttackAction(AnimGroup);
									canDoAttack[i] = IsAttackAction;
									totalProbRange += IsAttackAction ? animProbData[i].iProb : 0;
								}
								i = 0;
								if (totalProbRange > 0) {
									int32_t irand_3 = Random() % static_cast<uint32_t>(totalProbRange);
									while (irand_3 >= 0) {
										if (canDoAttack[i]) {
											irand_3 -= animProbData[i].iProb;
											if (irand_3 <= 0) {
												eattack = animProbData[i].eGroup;
											}
										}
										++i;
									}
								}
								if (eattack != ANIM_GROUP_INVALID &&
									Actor::GetDistanceForAnimGroup(apActor, eattack, &attackDelta) &&
									attackDelta.y != 0.0) {
									eattack = static_cast<ANIM_GROUP_ENUM>(irand_2 > 20 ? ANIM_GROUP_ATTACK_RIGHT
																						: ANIM_GROUP_ATTACK_LEFT);
								}
								if (apActor->IsSneaking() && !IsCurrentWeaponGrenadeOrMine() &&
									!apActor->IsSwimming()) {
									eattack = ANIM_GROUP_ATTACK_NORMAL_POWER;
								}
								if (apActor->GetBlocked()) {
									fUseItemTimer = 0.8f;
								} else {
									if (eattack != ANIM_GROUP_INVALID && apActor->QueueAttack(eattack)) {
										fUseItemTimer = 0.8f;
									}
								}
							} else {
								if (apActor->GetBlocked()) {
									apActor->SetBlock(0);
								}
							}
						}
					}
				} else {
					FreeUpSpecialIdle(apActor);
					ClearAutomaticShotsToFire(0);
					if (pattacktarg) {
						AimAtTarget(apActor, pattacktarg, 0);
					}
					if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0) &&
						apActor->GetAnimAction() == ANIM_ACTION_NONE) {
						Animation* v22 = apActor->GetAnimation();
						if (v22->SpecialIdleDonePlaying()) {
							AddToProcedureIndexRunning(apActor, -1);
						}
					}
					if (fHoldAttackTimer <= 0.0) {
						fUseItemTimer = fUseItemTimer - appTimer.fDelta;
					}
				}
			} else {
				apActor->SetWantWeaponDrawn(1);
				bWeaponAlertDrawn = 1;
			}
			if (pattacktarg) {
				SetUseWeaponHeadTrackTarget(pattacktarg);
			}
			return 0;
		}
		return result;
	}
	default: {
		char buffer_2[256];
		BSsprintf(buffer_2, 256, "UseWeapon package %s has a non-weapon target; should be UseItem.",
			pPack->GetFormEditorID());
		Warning(buffer_2);
		return 0;
	}
	}
}

void HighProcess::ProcessFleeNonCombat(Actor* apActor) {
	TESObjectREFR* v10;
	PackageTarget* PackageTarget_;
	TESObjectREFR* apRef;
	TESPackage* v9 = GetPackageThatIsRunning();
	if (apActor->IsOverEncumbered()) {
		goto LABEL_2;
	}
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	SetLocationForPackage(apActor, 0);
	PackageTarget_ = v9->GetPackageTarget();
	if (!pTarget) {
		if (PackageTarget_) {
			TESObjectREFR* TargReference = PackageTarget_->GetTargReference();
			SetTarget(TargReference);
		}
	}
	v10 = GetTarget();
	apRef = GetGenericLocation();
	if (v10 || apRef) {
		bool v3;
		float FormFlags;
		float LocationRadius = static_cast<float>(v9->GetLocationRadius(apActor));
		if (PackageTarget_) {
			FormFlags = static_cast<float>(PackageTarget_->iValue);
		} else {
			FormFlags = -1.0f;
		}
		if (apRef) {
			v3 = static_cast<double>(LocationRadius) < apActor->GetDistanceFromReference(apRef, 0, 0);
		} else {
			v3 = static_cast<double>(FormFlags) > apActor->GetDistanceFromReference(v10, 0, 0);
		}
		if (v3) {
			apActor->InitiateFlee(v10, 1, 1, 0, 0, apRef, FormFlags, LocationRadius);
			TESPackage* v7 = GetPackageThatIsRunning();
			FleePackage* v8 = 0;
			if (v7) {
				if (v7->GetPackType() == PACKAGE_TYPE_FLEE) {
					v8 = static_cast<FleePackage*>(v7);
				}
			}
			if (v8) {
				if (v10) {
					v8->AddAvoidedRef(v10);
				}
			}
		} else {
			if (apActor->IsPathingComplete()) {
				if (v9->GetMustComplete() || v9->GetMustReachLocation()) {
					EndMoveMessage(apActor);
					SetCurrentActionComplete(1);
					AddToProcedureIndexRunning(apActor, 1);
					CheckforNewPackage(apActor, 0);
				}
			} else {
				EndMoveMessage(apActor);
				Animation* v6 = apActor->GetAnimation();
				if (v6) {
					if (v6->SpecialIdleDonePlaying()) {
						SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
					}
				}
			}
		}
	} else {
	LABEL_2:;
		AddToProcedureIndexRunning(apActor, 1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessAvoidArea(Actor* apActor) {
	if (pTarget) {
		if (!apActor->IsPathingComplete() || !(fAvoidWaitTimer <= 0.0)) {
			goto LABEL_11;
		}
		goto LABEL_10;
	}
	SetTargetForPackage(apActor);
	if (!pTarget) {
		if (!(fAvoidWaitTimer <= 0.0)) {
		LABEL_11:;
			fAvoidWaitTimer = fAvoidWaitTimer - appTimer.fDelta;
			return;
		}
	LABEL_10:;
		AddToProcedureIndexRunning(apActor, 1);
		return;
	}
	NiPoint3 v9 = pTarget->GetLocationOnReference();
	NiPoint3 v10 = apActor->GetLocationOnReference();
	float fdistance = fFleeDistanceInterior.Float();
	FNVD_CONSTRUCTED_LOCAL(PathingRequestFlee, fleeRequest, &PathingRequestFlee::Destruct);
	fleeRequest.Initialize(apActor);
	float a2 = apActor->GetHeading(0);
	fleeRequest.SetInitialPathHeading(a2);
	if (apActor->IsCreature()) {
		TESActorBaseData* v4 = static_cast<TESActorBaseData*>(apActor->GetActorBase());
		fleeRequest.SetCantOpenDoors(v4->GetCantOpenDoors());
	}
	fleeRequest.SetTargetRadius(20.0f);
	PathingAvoidNodeArray* v2 = CreateAvoidArray(apActor);
	fleeRequest.SetAvoidNodeArray(v2);
	fleeRequest.SetMinFleeDistance(300.0f);
	fleeRequest.SetNoDestination();
	apActor->SetPathfindingHide(&fleeRequest);
}

void HighProcess::ProcessGuard(Actor* apActor) {
	GuardActorPackageData* pdata = static_cast<GuardActorPackageData*>(BaseProcess::GetInstanceDataThatIsRunning());
	if (GetRunOncePackage()) {
		TESPackage* v2 = GetRunOncePackage();
		if (v2->GetPackType() == PACKAGE_TYPE_GUARD) {
			pdata = static_cast<GuardActorPackageData*>(GetRunOncePackageInstanceData());
		}
	}
	if (pdata) {
		TESObjectREFR* pguardedRef_3;
		TESObjectREFR* pguardedRef_2;
		bWeaponAlertDrawn = 0;
		TESPackage* ppack = GetPackageThatIsRunning();
		TESObjectREFR* pguardedRef = ppack->ComputeInitialTarget(apActor);
		if (pguardedRef) {
			pguardedRef_2 = pguardedRef;
		} else {
			pguardedRef_2 = apActor;
		}
		TESObjectREFR* pholder = (&pguardedRef_2->m_Extra)->GetReferencePointer();
		if (pholder) {
			pguardedRef_3 = pholder;
		} else {
			pguardedRef_3 = pguardedRef_2;
		}
		if (GetTarget() != pguardedRef_3) {
			SetTarget(pguardedRef_3);
			(&pguardedRef_3->m_Extra)->AddGuard(apActor);
			pguardedRef_3->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
		}
		if (!IsRunningRunOnce() &&
			(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
				apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
			pdata->eState = GuardActorPackageData::GUARD_STATE_INIT;
			apActor->InitiateGetUpPackage();
		} else {
			bool atLoc = 1;
			if (!apActor->IsImmobile()) {
				float fdistRemaining;
				float ftargRadius;
				if (ppack->pPackLoc) {
					atLoc = ppack->IsActorAtLocation(apActor, 0, -1.0f, 0);
					ftargRadius = static_cast<float>(ppack->GetLocationRadius(apActor));
				} else {
					float ftargRadius_2;
					PackageTarget* ppackTarg = ppack->GetPackageTarget();
					if (ppackTarg) {
						ftargRadius_2 = static_cast<float>(ppackTarg->iValue);
					} else {
						ftargRadius_2 = 30.0f;
					}
					ftargRadius = ftargRadius_2;
					atLoc = static_cast<double>(ftargRadius_2) > apActor->GetDistanceFromReference(pguardedRef_3, 0, 0);
				}
				if (pdata->eState) {
					if ((apActor->IsPathingComplete() && !atLoc) || apActor->DidPathingFail()) {
						ProcessGuardStartTravelToLocation(apActor, pdata);
					}
				}
				ActorMover* pmover = apActor->pActorMover;
				if (atLoc || !pmover) {
					fdistRemaining = 0.0f;
				} else {
					fdistRemaining = pmover->GetDistanceRemaining();
				}
				float v4 = ftargRadius + ftargRadius;
				int32_t emove = CalculateMoveMode(apActor, fdistRemaining, ftargRadius, v4, 0, 0);
				ppack->GetPackageRadiusActorToLocation(apActor, 0);
				SetActorsAnimation(apActor, emove, 1);
			}
			uint32_t eState = pdata->eState;
			if (eState) {
				if (eState == 1) {
					ProcessGuardIdle(apActor, pdata, atLoc);
				} else {
					if (eState == 2) {
						ProcessGuardWaitToAttack(apActor, pdata);
					}
				}
			} else {
				ProcessGuardStartTravelAndReset(apActor, pdata);
			}
		}
	} else {
		SetProcedureIndexRunning(-1);
	}
}

void HighProcess::ProcessSummonCreatureDefend(Actor* apActor) {
	LowProcess* pProcess = this;
	if (!pCommandingActor) {
		pProcess->AddToProcedureIndexRunning(apActor, 1);
		return;
	}
	bool bSearching = false;
	if (pCommandingActor->Actor::IsInCombat() ||
		(pCommandingActor == pPlayer && pPlayer->IsPlayerCharacterInCombat(&bSearching))) {
		if (pCommandingActor == pPlayer) {
			BSSimpleList<TESObjectREFR*>* pList =
				ReferenceProcessLists.IsActorTargetingREFinPackage(pPlayer, PACKAGE_TYPE_COMBAT, true);
			while (pList) {
				if (!pList->GetItem())
					break;
				TESObjectREFR* pRef = static_cast<TESObjectREFR*>(pList->GetItem());
				Actor* pTarget = nullptr;
				if (pRef->IsActor()) {
					pTarget = static_cast<Actor*>(pList->GetItem());
				}
				if (pTarget)
					pProcess->EnterCombat(apActor, pTarget, false, true, nullptr, false, false, true, false, false,
						false, true, nullptr);
				pList = pList->GetNext();
			}
		}
	} else if (pCommandingActor != pPlayer) {
		pProcess->ProcessAccompany(apActor);
	} else {
		pProcess->ProcessFollow(apActor, false, 0x101, false);
	}
}

// Whether apActor holds what the running package targets: the package's target reference's base object (a reference
// or linked-reference target falls back to the package target's reference or apActor's linked reference when the
// running package has none), ainumb of the target object, or ainumb of the target object type. The match type goes
// into eUseItem.
bool HighProcess::CheckIfHasObject(Actor* apActor, int32_t ainumb) {
	BaseProcess* pProcess = this;
	TESPackage* pPackage = pProcess->GetPackageThatIsRunning();
	PackageTarget* pTarget = pPackage ? pPackage->GetPackageTarget() : nullptr;
	if (!pTarget) {
		return false;
	}
	PACK_TARG_TYPE eTargType = pTarget->GetTargType();
	bool bHasObjects = false;
	PACKAGE_OBJECT_TYPE& reMatchId = this->eUseItem;
	switch (eTargType) {
	case PTT_REFERENCE:
	case PTT_LINKEDREF: {
		TESObjectREFR* pRef = pProcess->GetActorPackageThatIsRunning()->GetTarget();
		if (!pRef) {
			pRef = eTargType != PTT_REFERENCE ? apActor->GetLinkedRef() : pTarget->GetTargReference();
		}
		if (pRef) {
			uint32_t uiID = pRef->GetFormID();
			bHasObjects = apActor->HasObjects(pRef->GetObjectReference(), POTYPE_NONE, 1, uiID, reMatchId);
		}
		break;
	}
	case PTT_OBJECT:
		bHasObjects = apActor->HasObjects(static_cast<TESBoundObject*>(pTarget->GetTargObject()), POTYPE_NONE, ainumb,
			0, reMatchId);
		break;
	case PTT_TYPE:
		bHasObjects = apActor->HasObjects(nullptr, pTarget->GetTargObjectType(), ainumb, 0, reMatchId);
		break;
	}
	return bHasObjects;
}

void HighProcess::EvaluateOrderAcquireList(Actor* apActor) {
	bool ballowKilling;
	bool ballowStealing;
	bool ballowBuying;
	BSSimpleList<ObjectstoAcquire*>* pRefItem = &TempObjectList;
	TESPackage* ppack = GetPackageThatIsRunning();
	if (ppack) {
		ballowBuying = ppack->SearchGetAllowBuying();
	} else {
		ballowBuying = 0;
	}
	if (ppack) {
		ballowStealing = ppack->SearchGetAllowStealing();
	} else {
		ballowStealing = 0;
	}
	if (ppack) {
		ballowKilling = ppack->SearchGetAllowKilling();
	} else {
		ballowKilling = 0;
	}
	while (pRefItem && pRefItem->m_item) {
		ObjectstoAcquire* pObject = pRefItem->m_item;
		TESObjectREFR* pRef = pObject->pRef;
		TESObjectREFR* pactor = 0;
		TESBoundObject* pObj = pObject->pObject;
		bool bbuyit = 0;
		bool badd = 1;
		if (pObj->GetFormType() == NPC__ID || pObj->GetFormType() == CREA_ID) {
			badd = 1;
		} else {
			if ((pRef->GetOwner() && !pRef->IsAnOwner(apActor, 1)) || pRef->IsActor()) {
				bool bsteal = 0;
				bool bkill = 0;
				TESNPC* pnpc = 0;
				Actor* pbuyactor = 0;
				bool v2 = pRef->IsActor();
				Actor* pTargetActor = v2 ? static_cast<Actor*>(pRef) : nullptr;
				if (pTargetActor) {
					if (pTargetActor->IsPlayerTeammate()) {
						badd = 0;
					}
				}
				bool bstop = 0;
				if (!pRef->IsNpc() || pRef->IsDead(0)) {
					TESForm* pownerform = pRef->GetOwner();
					if (pownerform) {
						if (pownerform->GetFormType() == NPC__ID) {
							pnpc = static_cast<TESNPC*>(pownerform);
						}
					}
					if (pnpc) {
						pbuyactor = ReferenceProcessLists.GetActorRefInHigh(pnpc, 0);
					}
				} else {
					pbuyactor = static_cast<Actor*>(pRef);
				}
				if (!pbuyactor || pbuyactor->IsDead(0)) {
					if (pRef->IsNpc()) {
						badd = 1;
						pObject->eType = ACQUIRE_TYPE_NOOWNED_GROUND;
						bstop = 1;
					}
				} else {
					if (ballowBuying && pbuyactor->GetActorBase()->SellBuysItem(pObj)) {
						int32_t GoldAmount;
						if (TESPackage::GetObjectTypeFromForm(pObj) == POTYPE_FOOD ||
							(GoldAmount = apActor->GetGoldAmount(),
								GoldAmount >= TESValueForm::GetFormValue(pObject->pObject))) {
							if (apActor->IsInCombat()) {
								badd = 0;
							} else {
								pObject->eType = ACQUIRE_TYPE_BUY_NPC;
								double DistanceFromReference = apActor->GetDistanceFromReference(pRef, 0, 0);
								pObject->iWantValue = static_cast<int32_t>(DistanceFromReference);
								bbuyit = 1;
							}
						}
					}
				}
				if (!bbuyit && !bstop && (!pTargetActor || !pTargetActor->IsPlayerTeammate())) {
					if (pRef->IsActor()) {
						if (pRef->IsDead(0)) {
							badd = 1;
						} else {
							if (pRef->IsNpc()) {
								pactor = pRef;
								if (ballowStealing) {
									bsteal = 1;
								}
								if (ballowKilling) {
									bkill = 1;
								}
							}
						}
					} else {
						if (ballowStealing) {
							bsteal = 1;
						}
					}
					if ((!bsteal && !bkill) || (pTargetActor && pTargetActor->IsPlayerTeammate())) {
						if (apActor->IsCreature()) {
							badd = 0;
						} else {
							TESBoundObject* Owner;
							if (!pRef->IsActor() ||
								(pRef->IsNpc() ||
									((Owner = static_cast<TESBoundObject*>(pRef->GetOwner()),
										 Owner == apActor->GetObjectReference()) ||
										apActor->bPlayerTeammate))) {
								badd = 0;
							} else {
								if (pTargetActor) {
									if (!pTargetActor->IsPlayerTeammate()) {
										pObject->eType = ACQUIRE_TYPE_KILL;
										badd = 1;
									}
								}
							}
						}
					} else {
						if (pRef->IsActor()) {
							if (bsteal) {
								if (apActor->IsInCombat()) {
									badd = 0;
								} else {
									pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
								}
							} else {
								if (bkill) {
									if (apActor->IsInCombat() || apActor->bPlayerTeammate) {
										badd = 0;
									} else {
										if (!pTargetActor || !pTargetActor->IsPlayerTeammate()) {
											pObject->eType = ACQUIRE_TYPE_KILL;
										}
									}
								}
							}
						} else {
							if (pObject->eType == ACQUIRE_TYPE_NOOWNED_CONTAINER) {
								PACKAGE_OBJECT_TYPE eacquireItem = POTYPE_NONE;
								pObject->eType = ACQUIRE_TYPE_PICKPOCKET;
								if (!apActor->HasObjects(pLockpick, POTYPE_NONE, 1, 0, eacquireItem)) {
									if (pRef->GetLock()) {
										badd = 0;
									}
								}
							} else {
								pObject->eType = ACQUIRE_TYPE_STEALGROUND;
							}
						}
						pObject->bSteal = bsteal;
						pObject->bKill = bkill;
					}
				}
			} else {
				double v6 = apActor->GetDistanceFromReference(pRef, 0, 0);
				pObject->iWantValue = static_cast<int32_t>(v6);
			}
		}
		if (badd) {
			pObject->pRef->SetTargeted(1);
			ObjectList.AddTail(pObject);
		} else {
			if (pObject) {
				pObject->Delete(1u);
			}
		}
		pRefItem = pRefItem->m_pkNext;
	}
	SortObjectsListByType();
	SortObjectsListByValue();
	TempObjectList.RemoveAll();
}

void HighProcess::SetAnimation(Animation* apAnimation) {
	// without a new animation, a running one is told to stop
	if (pAnimation && !apAnimation)
		SetAnimAction(ANIM_ACTION_NONE, nullptr);
	MiddleHighProcess::SetAnimation(apAnimation);
}

void HighProcess::SetAnimationActiveFlag(bool abflag) {
	bActivateAnim = abflag;
}

void HighProcess::ProcessBuyObject(Actor* apActor) {
	TESPackage* pPack = GetPackageThatIsRunning();
	int32_t aiAmount = 0;
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	if (!pTarget || (pTarget->GetDelete() || pTarget->GetDisabled())) {
		AddToProcedureIndexRunning(apActor, 1);
		return;
	}
	if (this->pAcquireObject) {
		int32_t ivalue = this->pAcquireObject->iamount * TESValueForm::GetFormValue(this->pAcquireObject->pObject);
		if (!pTarget) {
			ObjectstoAcquire* pAcquireObject = this->pAcquireObject;
			if (pAcquireObject->pObject == pAcquireObject->pRef->GetObjectReference()) {
				SetTarget(this->pAcquireObject->pRef);
				return;
			}
		}
		DoorTeleportData* pTel = pTarget->GetTeleport();
		bool batRef = 0;
		if (pTel) {
			const NiPoint3* v11 = &apActor->GetLocationOnReference();
			const NiPoint3* LinkedDoorTeleportPosition = &pTarget->GetLinkedDoorTeleportPosition();
			NiPoint3 dist = *LinkedDoorTeleportPosition - *v11;
			double v4 = dist.Length();
			if (static_cast<double>(iAIDistanceRadiusMinLocation.Int()) >= v4) {
				batRef = 1;
			}
		} else {
			batRef = pPack->IsActorAtRefTarget(apActor, 0);
		}
		if (batRef || DidPathingFail(apActor)) {
			if (!apActor->IsPathingComplete()) {
				EndMoveMessage(apActor);
			}
			if (DidPathingFail(apActor) && !batRef) {
				SetTarget(0);
			} else {
				Actor* pact = 0;
				if (this->pAcquireObject->pRef->IsActor()) {
					pact = static_cast<Actor*>(pTarget);
					Script::SetActionFlag(pact, this->pAcquireObject->pExtra, 0x4000u);
					pact->RemoveItem(this->pAcquireObject->pObject, 0, 1, 0, 0, apActor, 0, 0, 1, 0);
				} else {
					TESForm* pownerform = this->pAcquireObject->pRef->GetOwner();
					TESNPC* npc = 0;
					if (pownerform) {
						if (pownerform->GetFormType() == NPC__ID) {
							npc = static_cast<TESNPC*>(this->pAcquireObject->pRef->GetOwner());
						}
					}
					if (npc) {
						pact = ReferenceProcessLists.GetActorRefInHigh(npc, 0);
					}
					TESBoundObject* pcompobj = (&apActor->m_Extra)->GetLevCreaOriginalBase();
					if (!pcompobj) {
						pcompobj = apActor->GetObjectReference();
					}
					(&this->pAcquireObject->pRef->m_Extra)->SetOwnership(pcompobj);
					Script::SetActionFlag(pact, this->pAcquireObject->pExtra, 0x4000u);
					this->pAcquireObject->pRef->Activate(apActor, 0, this->pAcquireObject->pObject, 1);
				}
				if (pact) {
					if (pact != apActor) {
						apActor->PayGoldToActor(pact, ivalue);
					}
				}
			}
			if (this->pAcquireObject) {
				ObjectstoAcquire* apThis = this->pAcquireObject;
				if (apThis) {
					apThis->Delete(1u);
				}
			}
			this->pAcquireObject = 0;
		} else {
			if (apActor->GetSitSleepState()) {
				apActor->InitiateGetUpPackage();
				return;
			}
			if (apActor->IsPathingComplete()) {
				if (pTarget->GetTeleport()) {
					TESWorldSpace* WorldSpace = pTarget->GetWorldSpace();
					TESObjectCELL* ParentCell = pTarget->GetParentCell();
					const NiPoint3* v6 = &pTarget->GetLinkedDoorTeleportPosition();
					if (!apActor->SetPathfindingGoal(*v6, ParentCell, WorldSpace, 0.0f, 0)) {
						return;
					}
				} else {
					TESWorldSpace* v9 = pTarget->GetWorldSpace();
					TESObjectCELL* v7 = pTarget->GetParentCell();
					const NiPoint3* v5 = &pTarget->GetLocationOnReference();
					if (!apActor->SetPathfindingGoal(*v5, v7, v9, 0.0f, 0)) {
						return;
					}
				}
			}
			if (!apActor->IsPathingComplete()) {
				SetActorsAnimation(apActor, 257, 1);
			}
		}
	}
}

void HighProcess::ProcessSteal(Actor* apActor) {
	apActor->SetForceSneak(1);
	if (!apActor->IsSneaking()) {
		apActor->SetWalkRunFlagDirty();
	}
	apActor->SetMoveMode(1024);
	if (ProcessActivate(apActor, (bool)0)) {
		if (apActor->IsSneaking()) {
			apActor->SetWalkRunFlagDirty();
		}
		apActor->SetForceSneak(0);
		apActor->ClearMoveMode(0x400u);
	}
}

void HighProcess::ProcessPickPocket(Actor* apActor) {
	apActor->SetForceSneak(1);
	if (!apActor->IsSneaking()) {
		apActor->SetWalkRunFlagDirty();
	}
	apActor->SetMoveMode(1024);
	if (ProcessActivate(apActor, (bool)0)) {
		if (apActor->IsSneaking()) {
			apActor->SetWalkRunFlagDirty();
		}
		apActor->SetForceSneak(0);
		apActor->ClearMoveMode(0x400u);
	}
}

// Runs the in-game dialogue package: ends the dialogue when a speaker is gone, dying or off the package, starts the
// conversation, turns both speakers towards each other, counts the listen timer down and, once the current line is
// done, ends the dialogue or says the next line. The exe's cleanup-only EH frame (around the Conversation
// constructor and the sound handle temporary) is left out. The first turn block computes the angle delta and the
// turn limit without using them; the calls are kept.
void HighProcess::ProcessDialogue(MobileObject* apMobile) {
	// The PDB's parameter is a MobileObject; the dialogue package only runs on actors.
	Actor* apActor = static_cast<Actor*>(apMobile);
	TESPackage* pRunning = GetPackageThatIsRunning();
	if (pRunning->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
		AddToProcedureIndexRunning(apActor, static_cast<uint32_t>(-2));
		return;
	}
	TESPackage* pExtraPackage = apActor->GetExtra()->GetPackageExtra();
	if (pExtraPackage && pExtraPackage->GetPackType() == PACKAGE_TYPE_DIALOGUE) {
		apActor->GetExtra()->SetPackageExtraActionComplete(true);
	}
	DialoguePackage* pPackage = static_cast<DialoguePackage*>(pRunning);
	MobileObject* pTarget = pPackage->GetTargetOfConversation();
	MobileObject* pStarter = pPackage->GetActorStartedConversation();
	if (!pTarget || !pStarter || !pTarget->GetCurrentAIProcess() || !pStarter->GetCurrentAIProcess() ||
		(pStarter == pPlayer && pTarget != pPlayer &&
			(pStarter->GetCurrentPackage() != pPackage || pTarget->GetCurrentPackage() != pPackage))) {
		apActor->EndDialogue();
		return;
	}
	// The exe's `test ah, 0x41; jp` skips on > or unordered: the dialogue ends when the timer is <= 0.
	if (apActor && apActor->IsDying() && GetGreetingTimer() <= 0.0) {
		apActor->EndDialogue();
		return;
	}
	if (pTarget && pTarget->IsTalkingActivator()) {
		BaseProcess* pTargetProcess = pTarget->GetCurrentAIProcess();
		pTargetProcess->AddToProcedureIndexRunning(static_cast<Actor*>(pTarget), 2);
	}
	if (!pPackage->GetPackConversation()) {
		TESTopic* pTopic = pPackage->GetDialogueTopic();
		Conversation* pConversation = nullptr;
		if (pTarget != pPlayer && pStarter != pPlayer) {
			TESObjectREFR* pListener = pTarget;
			if (pTarget->IsTalkingActivator()) {
				pListener = pTarget->GetTalkingActivator();
				BGSTalkingActivator* pActivator = static_cast<BGSTalkingActivator*>(pListener->GetObjectReference());
				if (pActivator->GetTempRef()) {
					pListener = pActivator->GetTempRef();
				}
			}
			void* pMemory = ourMalloc(sizeof(Conversation));
			Conversation* pNew;
			if (pMemory) {
				pNew = ::new (pMemory) Conversation(pStarter, pListener, pTopic);
			} else {
				pNew = nullptr;
			}
			pConversation = pNew;
			pPackage->SetPackConversation(pConversation);
		}
	}
	if (!pPackage->GetNoHeadTrack()) {
		if (pTarget->IsActor() && !pPackage->GetDialogueDoNotControlTarget() &&
			pTarget->GetSitSleepState() == SIT_SLEEP_NORMAL && pTarget != apActor) {
			Actor* pTargetActor = static_cast<Actor*>(pTarget);
			// The exe reads the target's position first, then the starter's.
			const NiPoint3* pTargetPos = &pTarget->GetLocationOnReference();
			NiPoint3 kDirection = pStarter->GetLocationOnReference().operator-(*pTargetPos);
			float fZAngle = GetZAngleFromVector(kDirection);
			float fTurnDir = 0.0f;
			float fHeading = pStarter->GetHeading(false) + 3.1415927410125732;
			[[maybe_unused]] float fAngleDelta = GetAngleDelta(fHeading, fZAngle, fTurnDir);
			[[maybe_unused]] float fTurnLimit = static_cast<int32_t>(iActorTurnDegree.UInt()) * 0.01745329238474369;
			if (pTargetActor->IsTurning()) {
				fTurnLimit = static_cast<int32_t>(iActorKeepTurnDegree.UInt()) * 0.01745329238474369;
			}
			pTargetActor->RequestRotate(pStarter->GetLocationOnReference(), false);
		}
		if (pStarter->IsActor() && pStarter->GetSitSleepState() == SIT_SLEEP_NORMAL && pStarter != pTarget) {
			Actor* pStarterActor = static_cast<Actor*>(pStarter);
			if (!pStarterActor->IsTurning()) {
				// The exe reads the starter's position first, then the target's.
				const NiPoint3* pStarterPos = &pStarter->GetLocationOnReference();
				NiPoint3 kDirection = pTarget->GetLocationOnReference().operator-(*pStarterPos);
				float fZAngle = GetZAngleFromVector(kDirection);
				float fTurnDir = 0.0f;
				float fHeading = pTarget->GetHeading(false) + 3.1415927410125732;
				float fAngleDelta = GetAngleDelta(fHeading, fZAngle, fTurnDir);
				// The exe calls the float fabs (0x408840) and keeps its result as a double.
				double dAbsDelta = std::fabs(fAngleDelta);
				// `test ah, 5; jp` skips on >=, == or unordered: the starter turns only when the limit is below.
				if (static_cast<int32_t>(iActorTurnDegree.UInt()) * 0.01745329238474369 < dAbsDelta) {
					pStarterActor->RequestRotate(pTarget->GetLocationOnReference(), false);
				}
			}
		}
	}
	// `test ah, 0x41; jne` takes the else branch on <, == or unordered.
	if (pPackage->flistentimer > 0.0) {
		pPackage->flistentimer = pPackage->flistentimer - appTimer.GetSeconds();
		return;
	}
	bool bLineDone = false;
	if (GetCurrentActionComplete()) {
		BSSoundHandle kHandle = GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
		if (!kHandle.IsValid()) {
			bLineDone = true;
		}
	}
	if (!bLineDone) {
		pPackage->Speak(true);
		return;
	}
	bool bSamePackage = false;
	TESPackage* pTargetPackage = pTarget->GetCurrentPackage();
	if (pTargetPackage == pStarter->GetCurrentPackage()) {
		bSamePackage = true;
	}
	if (!pPackage->GetTargetContinueMoving() && !pPackage->GetDialogueDoNotControlTarget() && pTarget->IsActor()) {
		static_cast<Actor*>(pTarget)->StopMoving();
	}
	if (bSamePackage) {
		MobileObject* pOther = nullptr;
		if (pStarter == apActor) {
			pOther = pTarget;
		} else {
			pOther = pStarter;
		}
		HighProcess* pOtherProcess = nullptr;
		if (pOther->GetCurrentAIProcess() && pOther->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH) {
			pOtherProcess = static_cast<HighProcess*>(pOther->GetCurrentAIProcess());
		}
		if (pOtherProcess) {
			pOtherProcess->fPackageEvalTimer = 0.0f;
		}
	}
	fPackageEvalTimer = 0.0f;
	apActor->EndDialogue();
}

bool HighProcess::ProcessDialogueActivate(Actor* apActor) {
	NiPoint3 delta;
	TESPackage* pCurrent = GetPackageThatIsRunning();
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	if (pCurrent->GetDialogueSayToFlag() && pTarget == pPlayer) {
		if (IsTalking(apActor)) {
			return 1;
		}
		MobileObject* ptarget = 0;
		if (pTarget->IsMobileObject()) {
			ptarget = static_cast<MobileObject*>(pTarget);
		}
		if (pCurrent->GetPackageSecondLocation() && !pCurrent->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
			goto LABEL_11;
		}
		if (!pCurrent->GetPackageSecondLocation() || pCurrent->IsActorAtSecondLocation(ptarget, apActor, 0, -1.0f, 1)) {
			// One local in the exe ([ebp-0x1C], 0x8E8741): the package target's radius, 120 when not positive.
			float a2 = 0.0f;
			if (pCurrent->GetPackageTarget()) {
				a2 = static_cast<float>(pCurrent->GetPackageTarget()->iValue);
			}
			if (a2 <= 0.0) {
				a2 = 120.0f;
			}
			if (static_cast<double>(a2) < apActor->GetDistanceFromReference(pTarget, 0, 0)) {
				if (apActor->GetSitSleepState()) {
					apActor->InitiateGetUpPackage();
					return 1;
				} else {
					if (apActor->IsPathingComplete()) {
						if (pCurrent->GetAlwaysRun()) {
							apActor->SetMoveMode(512u);
						}
						if (pCurrent->GetAlwaysSneak()) {
							if (!apActor->IsSneaking()) {
								apActor->SetWalkRunFlagDirty();
							}
							apActor->SetMoveMode(1024u);
						}
						apActor->SetPathfindingGoal(pTarget, a2, 0);
					}
					return 1;
				}
			}
			EndMoveMessage(apActor);
			if ((apActor->IsPathing() && !apActor->IsRotating()) || !apActor->LineOfSight(0, pPlayer, 1, 0, 0)) {
				return 1;
			}
			TESTopic* ptopic = pCurrent->GetDialogueTopic();
			if (!ptopic) {
				ptopic = TESTopic::GetTopic(DT_CONVERSATION, 0);
			}
			if (ptarget) {
				if (pTarget != pPlayer && !pCurrent->GetDialogueNoHeadtrack()) {
					BaseProcess* CurrentAIProcess = ptarget->pCurrentProcess;
					CurrentAIProcess->ClearScriptHeadTrackTarget(0);
					BaseProcess* v22 = ptarget->pCurrentProcess;
					v22->SetActionHeadTrackTarget(apActor);
					ptarget->SetDialogueItemTarget(apActor);
				}
			}
			BaseProcess* v21 = apActor->pCurrentProcess;
			v21->ClearScriptHeadTrackTarget(0);
			BaseProcess* v20 = apActor->pCurrentProcess;
			v20->SetActionHeadTrackTarget(pTarget);
			apActor->SetDialogueItemTarget(ptarget);
			BaseProcess* v19 = apActor->pCurrentProcess;
			v19->ProcessGreet(apActor, ptopic, 0, 0, 1, 1);
		}
		return 1;
	}
	if (pCurrent->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
		MobileObject* pStartedConversation;
		DialoguePackage* pPack;
		MobileObject* pTargetTalker;
		if (pTarget) {
			if (pTarget->IsDead(0)) {
				TESPackage* ppack = apActor->GetPackageSetAsPcurrent();
				if (ppack) {
					apActor->EndDialogue();
					ppack->SetNeverRun(pTarget, 1);
					return 0;
				}
			}
		}
		pPack = static_cast<DialoguePackage*>(GetPackageThatIsRunning());
		pTargetTalker = pPack->pTargetActor;
		pStartedConversation = pPack->GetActorStartedConversation();
		if ((!pPack->GetReachedAmbushLocation() || !apActor->IsPathing()) &&
			(pPack->pPackLoc &&
				(pPack->GetPackageSecondLocation() &&
					(!pPack->IsActorAtLocation(pStartedConversation, 0, -1.0f, 0) &&
						!pPack->IsActorAtSecondLocation(pTargetTalker, pStartedConversation, 0, -1.0f, 0))))) {
		LABEL_11:;
			AddToProcedureIndexRunning(apActor, -1);
			return 0;
		}
		if (pPack == GetRunOncePackage()) {
			if (GetCurrentPackage()) {
				TESPackage* v3 = GetCurrentPackage();
				if (v3->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_SANDBOX &&
					(pPack->GetPackageSecondLocation() &&
						(pPack->GetTargetContinueMoving() &&
							(!pPack->GetTargetMoveTowardStarter() &&
								!pPack->IsActorAtSecondLocation(pTargetTalker, pStartedConversation, 0, -1.0f, 0))))) {
					apActor->EndInterruptPackage(0);
					return 0;
				}
			}
		}
		if (!pPack->IsCreated()) {
			if (pTargetTalker == pPlayer) {
				TESPackage* CurrentPackage;
				if ((apActor == pPlayer &&
						(pPlayer->GetCurrentPackage() &&
							(CurrentPackage = pPlayer->GetCurrentPackage(),
								CurrentPackage->GetPackType() == PACKAGE_TYPE_TRAVEL))) ||
					(apActor->GetSitSleepState() && apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING)) {
					if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
						apActor->InitiateGetUpPackage();
					}
					return 1;
				}
				if (pPack->GetPackageSecondLocation() &&
					(!pPack->IsActorAtSecondLocation(pTargetTalker, apActor, 0, -1.0f, 1) ||
						pPack->IsActorAtRefTarget(apActor, 0))) {
					if (pPack->GetPackageSecondLocation()) {
						if (pPack->IsActorAtSecondLocation(pTargetTalker, apActor, 0, -1.0f, 1)) {
							apActor->StopMoving();
							pPack->SetReachedAmbushLocation(1);
							BaseProcess* v14 = apActor->pCurrentProcess;
							v14->SetHeadTrackTarget(BaseProcess::HEAD_TRACK_TYPE_DIALOG, pPlayer);
							TESBoundObject* ObjectReference = pPlayer->GetObjectReference();
							ObjectReference->Activate(pPlayer, apActor, 0, 0, 1);
						}
					}
				} else {
					TESObjectREFR* LocationReference;
					TESObjectREFR* v7;
					if (pPack->GetReachedAmbushLocation() ||
						(!pPack->GetLocationReference(apActor) ||
							((LocationReference = pPack->GetLocationReference(apActor),
								 !LocationReference->IsFurniture()) ||
								((v7 = pPack->GetLocationReference(apActor), v7->IsFurniture()) &&
									(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
										apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING))))) {
						pPack->SetReachedAmbushLocation(1);
						if (ProcessActivate(apActor, 1)) {
							pCurrent = GetPackageThatIsRunning();
							if (pCurrent->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
								AddToProcedureIndexRunning(apActor, 1);
							}
						}
					}
				}
			} else {
				if (pTargetTalker == apActor) {
					Actor* ptargactor;
					if (!pTargetTalker->IsActor() ||
						((ptargactor = static_cast<Actor*>(pTargetTalker), pPack->GetDialogueDoNotControlTarget()) ||
							(!pPack->GetPackageSecondLocation() ||
								pPack->IsActorAtSecondLocation(ptargactor, apActor, 0, -1.0f, 0)))) {
						AddToProcedureIndexRunning(apActor, 1);
						pPack->SetReachedAmbushLocation(1);
					} else {
						BaseProcess* v18 = pTargetTalker->pCurrentProcess;
						v18->ProcessTravel(ptargactor, 0, 0, -1, 1);
					}
				} else {
					if (!pPack->GetPackageSecondLocation() ||
						pPack->IsActorAtSecondLocation(pTargetTalker, apActor, 0, -1.0f, 1)) {
						if (pPack->GetPackageSecondLocation()) {
							if (!pPack->GetDialogueDoNotControlTarget() &&
								pTargetTalker->GetCurrentPackage() != pPack) {
								BaseProcess* v17 = pTargetTalker->pCurrentProcess;
								v17->SavePackageToExtraData(pTargetTalker);
								TESPackage* v4 = apActor->GetCurrentPackage();
								pTargetTalker->PutCreatedPackage(v4, 0, 1);
								BaseProcess* v16 = pTargetTalker->pCurrentProcess;
								v16->SetTarget(pTargetTalker);
								BaseProcess* v15 = pTargetTalker->pCurrentProcess;
								v15->AddToProcedureIndexRunning(pTargetTalker, 1);
								pTargetTalker->SetDialogueItemTarget(apActor);
							}
						}
						if (ProcessActivate(apActor, 1)) {
							pCurrent = GetPackageThatIsRunning();
							if (pCurrent->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
								if (GetCurrentProcedureIndex() == 1) {
									AddToProcedureIndexRunning(apActor, 1);
								}
								pPack->SetReachedAmbushLocation(1);
							}
						}
					}
				}
			}
			return 1;
		}
		if (apActor == pTargetTalker) {
			if (pPack->GetTargetContinueMoving()) {
				float fradius;
				float FormFlags;
				if (pCurrent->GetPackageTarget()) {
					FormFlags = static_cast<float>(pCurrent->GetPackageTarget()->iValue);
				} else {
					FormFlags = 0.0f;
				}
				if (!(FormFlags <= 0.0)) {
					fradius = FormFlags;
				} else {
					fradius = 400.0f;
				}
				bool bpath = 0;
				if (apActor->IsPathingComplete()) {
					bpath =
						static_cast<double>(fradius) < apActor->GetDistanceFromReference(pStartedConversation, 0, 0);
				} else {
					apActor->GetCurrentPathfindingGoal_008B3840(&delta);
					const NiPoint3* v8 = &pStartedConversation->GetLocationOnReference();
					delta.operator-=(*v8);
					bpath = delta.SqrLength() > 10000.0;
				}
				if (bpath) {
					apActor->SetPathfindingGoal(pStartedConversation, fradius, 0);
				}
			} else {
				AddToProcedureIndexRunning(apActor, 1);
			}
		} else {
			if (pTargetTalker == pPlayer) {
				if (apActor == pPlayer) {
					if (pPlayer->GetCurrentPackage()) {
						TESPackage* v9 = pPlayer->GetCurrentPackage();
						if (v9->GetPackType() == PACKAGE_TYPE_TRAVEL) {
							return 1;
						}
					}
				}
				if (apActor->GetSitSleepState() && apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
					return 1;
				}
			}
			pCurrent = GetPackageThatIsRunning();
			if (ProcessActivate(apActor, 1)) {
				TESPackage* pnewPack = GetPackageThatIsRunning();
				if (!pnewPack || (pnewPack != pCurrent || pnewPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_DIALOGUE)) {
					return 0;
				}
				BaseProcess* pproc = pTargetTalker->pCurrentProcess;
				if (pproc) {
					TESPackage* ptheirpack = pproc->GetRunOncePackage();
					if (!ptheirpack || ptheirpack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_DIALOGUE) {
						pTargetTalker->PutCreatedPackage(pPack, 1, 1);
						TESPackage* ptheirpackNew = pproc->GetRunOncePackage();
						if (ptheirpackNew->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_DIALOGUE) {
							pproc->AddToProcedureIndexRunning(pTargetTalker, 2);
						}
					}
				}
				pCurrent = GetPackageThatIsRunning();
				if (pCurrent->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
					AddToProcedureIndexRunning(apActor, 1);
				}
			} else {
				if (apActor->GetSitSleepState()) {
					if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
						if (pPack->GetLocationReference(apActor)) {
							TESObjectREFR* v10 = pPack->GetLocationReference(apActor);
							if (v10->IsFurniture()) {
								if (pPack == GetRunOncePackage()) {
									AddToProcedureIndexRunning(apActor, 2);
								}
								return 0;
							}
						}
					}
					if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
						(apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING || apActor->IsTryingToEnterFurniture())) {
						if (pPack == GetRunOncePackage()) {
							AddToProcedureIndexRunning(apActor, 2);
						} else {
							apActor->EndDialogue();
							apActor->InitiateGetUpPackage();
						}
						return 0;
					}
				}
			}
		}
		return 1;
	}
	if (pTarget) {
		if (pTarget->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			// The exe calls Actor's slot 262 on the sleeping target: it is an Actor.
			static_cast<Actor*>(pTarget)->InitiateGetUpPackage();
			return 1;
		}
		if (pTarget->GetSitSleepState() != SIT_SLEEP_IS_SITTING && pTarget->GetSitSleepState()) {
			return 0;
		}
	} else {
		if (!SetTargetForPackage(apActor)) {
			AddToProcedureIndexRunning(apActor, 3);
			return 0;
		}
	}
	apActor->InitiateDialogue(pCurrent);
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessWander(Actor* apActor, const NiPoint3* apCenter, float afRadius) {
	float forigradius;
	// The exe builds these with PathingLocation's constructor (0x6DCE60) and destroys them with 0x4FF7E0 where it
	// uses them: no C++ constructor storing src's vtable.
	FNVD_INLINED_LOCAL(PathingLocation, v21, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, v22, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequestClosePoint, v26, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequestClosePoint, apThis_208, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequestClosePoint, v31, std::nullopt, std::nullopt);
	NiPoint3 apMemory_252;
	NiPoint3 pPackPoint;
	if (bGreetingFlag) {
		EndMoveMessage(apActor);
		return;
	}
	bool ballowChangingProcIndex = 1;
	bool bcellwander = 0;
	TESPackage* pPack = 0;
	pPack = GetPackageThatIsRunning();
	if (!(afRadius > 0.0)) {
		forigradius = static_cast<float>(pPack->GetLocationRadius(apActor));
	} else {
		forigradius = afRadius;
	}
	float forigradius_2 = forigradius;
	bool bIsWander = 1;
	if (pPack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_WANDER) {
		bIsWander = 0;
		switch (pPack->GetPackType()) {
		case PACKAGE_TYPE_EXPLORE:
		case PACKAGE_TYPE_FIND: {
			if (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
				break;
			}
			goto LABEL_20;
		}
		case PACKAGE_TYPE_EAT: {
			ItemChange* apMemory_264 = apActor->GetBestFoodItem();
			if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
				goto LABEL_20;
			}
			if (apMemory_264 && apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
				pItemBeingUsed = apMemory_264->pContainerObj;
				SetCurrentActionComplete(1);
				AddToProcedureIndexRunning(apActor, -1);
				return;
			}
			if (apMemory_264) {
				goto LABEL_16;
			}
			if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING) {
				break;
			}
			return;
		}
		case PACKAGE_TYPE_SLEEP: {
			if (!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
			LABEL_20:;
				EndMoveMessage(apActor);
				AddToProcedureIndexRunning(apActor, -2);
				return;
			}
			if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			LABEL_16:;
				AddToProcedureIndexRunning(apActor, -1);
				return;
			}
			break;
		}
		case PACKAGE_TYPE_SANDBOX:
		case PACKAGE_TYPE_GUARD: {
			bIsWander = 1;
			ballowChangingProcIndex = 0;
			break;
		}
		default: {
			break;
		}
		}
	}
	TESObjectREFR* pRef = 0;
	::new (&pPackPoint) NiPoint3(pPack->GetLocationCoord(apActor));
	PackageLocation* ploc = pPack->pPackLoc;
	if (ploc) {
		pRef = ploc->GetLocReference();
		if (!pRef && ploc->GetLocType() == PLT_CELL) {
			bcellwander = 1;
			pPackPoint = apActor->GetLocationOnReference();
		}
		if (!pRef) {
			pRef = GetGenericLocation();
		}
	}
	if (pGenericLocation) {
		if (!GetRunOncePackage()) {
			pRef = pGenericLocation;
		}
	}
	if (apCenter) {
		pPackPoint = *apCenter;
	}
	if (forigradius_2 < 60.0 && (bIsWander && (!bcellwander && ballowChangingProcIndex))) {
		if (pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
			if (bIsWander) {
				if (pRef) {
					TESBoundObject* ObjectReference = pRef->GetObjectReference();
					if (ObjectReference == pXMarkerHeading) {
						apActor->RequestRotate((&pRef->GetAngleOnReference())->z, 0);
					}
				}
			}
		} else {
			AddToProcedureIndexRunning(apActor, -1);
		}
		return;
	}
	if (!bIsWander && ballowChangingProcIndex) {
		if (fEvaluateAcquireTimer <= 0.0 && apActor->IsPathingComplete()) {
			EndMoveMessage(apActor);
			AddToProcedureIndexRunning(apActor, -1);
			fEvaluateAcquireTimer = 20.0f;
			return;
		}
		fEvaluateAcquireTimer = fEvaluateAcquireTimer - appTimer.fDelta;
	}
	if (!apActor->IsPathingComplete()) {
		SetActorsAnimation(apActor, 257, 1);
		return;
	}
	if (apActor->pActorMover->IsPathingComplete() &&
		(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
			(apActor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
				apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING))) {
		apMemory_252 = apActor->GetLocationOnReference();
		TESObjectCELL* apMemory_248 = apActor->GetParentCell();
		NiPoint3 apMemory_232 = pPackPoint;
		NiPoint3 apMemory_220 = apMemory_232 - apMemory_252;
		float apMemory_216 = apMemory_220.UnitizeGetLength();
		if (apMemory_216 > forigradius_2 + 250.0 && ballowChangingProcIndex) {
			SetProcedureIndexRunning(0);
			return;
		}
		bool apMemory_247 = !bcellwander;
		if (apActor->GetParentCell()->GetInterior()) {
			NiPoint3 apMemory_204 = pPackPoint;
			if (bcellwander) {
				forigradius_2 = 800.0f;
			}
			NiPoint3 apMemory_192 = apMemory_204 - apMemory_252;
			float apMemory_188 = apMemory_192.UnitizeGetLength();
			if (apActor->GetDistanceFromPoint(apMemory_252) <= 20.0) {
				::new (&v31) PathingRequestClosePoint;
				PathingLocation* apOrigin = ::new (&apLocation) PathingLocation(&pPackPoint, apMemory_248);
				v31.SetOrigin(*apOrigin);
				apLocation.~PathingLocation();
				float v16 = forigradius_2 * 0.75;
				float a2_4 = BSMIN(32.0f, v16);
				v31.SetMinDistance(a2_4);
				v31.SetMaxDistance(forigradius_2);
				Pathing::FindClosePoint(&v31, &apMemory_252);
				v31.~PathingRequestClosePoint();
			}
		} else {
			TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
			apMemory_248 = pDataHandler->GetCellFromWorldCoord(pPackPoint.x, pPackPoint.y, WorldSpace, 0);
			if (apMemory_248) {
				::new (&apThis_208) PathingRequestClosePoint;
				PathingLocation* aOrigin = ::new (&v22) PathingLocation(&pPackPoint, apMemory_248);
				apThis_208.SetOrigin(*aOrigin);
				v22.~PathingLocation();
				float v14 = forigradius_2 * 0.75;
				float a2_4a = BSMIN(32.0f, v14);
				apThis_208.SetMinDistance(a2_4a);
				apThis_208.SetMaxDistance(forigradius_2);
				Pathing::FindClosePoint(&apThis_208, &apMemory_252);
				apThis_208.~PathingRequestClosePoint();
			}
		}
		if (apActor->GetDistanceFromPoint(apMemory_252) <= 5.0) {
			fDelayTimer = 0.0f;
		}
		if (fDelayTimer > 10.0) {
			fDelayTimer = fDelayTimer - appTimer.fDelta;
			return;
		}
		if (apActor->GetParentCell()->GetInterior()) {
			if (apActor->GetSitSleepState()) {
				if (apActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
					apActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
					return;
				}
				apActor->InitiateGetUpPackage();
			}
			TESWorldSpace* v8 = apActor->GetWorldSpace();
			if (apActor->SetPathfindingGoal(apMemory_252, apActor->GetParentCell(), v8, 50.0f, 0)) {
				if (!(apActor->GetDistanceFromPoint(apMemory_252) < 50.0)) {
					apActor->GetFatiguePercentage();
					int32_t ActorBaseEnergyLevel = apActor->GetActorBaseEnergyLevel();
					fDelayTimer = AiFormulas::ComputeTimeTillNextWander(ActorBaseEnergyLevel);
				} else {
					fDelayTimer = 0.0f;
				}
			}
			return;
		}
		NiPoint3 apThis_192 = apMemory_252 - pPackPoint;
		float apThis_204 = apThis_192.Length();
		char apThis_191 = 0;
		if (forigradius_2 < static_cast<double>(apThis_204)) {
			apThis_191 = 1;
			::new (&v26) PathingRequestClosePoint;
			PathingLocation* v13 = ::new (&v21) PathingLocation(&pPackPoint, apActor->GetParentCell());
			v26.SetOrigin(*v13);
			v21.~PathingLocation();
			float v12 = forigradius_2 / 2.0;
			v26.SetMinDistance(v12);
			v26.SetMaxDistance(forigradius_2);
			Pathing::FindClosePoint(&v26, &apMemory_252);
			apThis_192 = apMemory_252 - pPackPoint;
			apThis_204 = apThis_192.Length();
			v26.~PathingRequestClosePoint();
		}
		if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
			goto LABEL_81;
		}
		if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
			apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
			apActor->InitiateGetUpPackage();
		LABEL_81:;
			TESWorldSpace* v6 = apActor->GetWorldSpace();
			if (apActor->SetPathfindingGoal(apMemory_252, apActor->GetParentCell(), v6, 50.0f, 0)) {
				if (!(apActor->GetDistanceFromPoint(apMemory_252) < 50.0)) {
					apActor->GetFatiguePercentage();
					int32_t v7 = apActor->GetActorBaseEnergyLevel();
					fDelayTimer = AiFormulas::ComputeTimeTillNextWander(v7);
				} else {
					fDelayTimer = 0.0f;
				}
				if (Interface::GetConsoleSelection() == apActor) {
					char Format[308];
					std::sprintf(Format, "%s is wandering to point x %.02f and y %.02f", apActor->GetFullName(),
						apMemory_252.x, apMemory_252.y);
					Interface::PrintLine(static_cast<const char*>(Format));
				}
			}
		}
	} else {
		if (!apActor->IsPathingComplete()) {
			if (pCurrentFurniture && pCurrentFurniture->GetMarkerUsed(cCurrentFurnitureIndex, 1)) {
				EndMoveMessage(apActor);
				fDelayTimer = 0.0f;
				pCurrentFurniture = 0;
				ClearFurnitureMarker();
			} else {
				SetActorsAnimation(apActor, 257, 1);
				float v24 = 0.0f;
				pCurrentFurniture;
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool HighProcess::ProcessSandBoxDialogue(Actor* apActor, TESObjectREFR* aptalkTarget) {
	PackageLocation* ploc;
	bool v8;
	if (!aptalkTarget || !aptalkTarget->IsActor()) {
		return 0;
	}
	if (!ComputeCanInitiateDialogueWithNPC(apActor, static_cast<Actor*>(aptalkTarget))) {
		return 0;
	}
	TESPackage* ptargPack = static_cast<MobileObject*>(aptalkTarget)->MobileObject::GetCurrentPackage();
	if (ptargPack) {
		bool v9 = ptargPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_SANDBOX && ptargPack->SandboxGetAllowConversation();
		v8 = v9;
	} else {
		v8 = 0;
	}
	TESPackage* pourPack = GetPackageThatIsRunning();
	if (pourPack) {
		ploc = pourPack->pPackLoc;
	} else {
		ploc = 0;
	}
	PackageLocation* ploc_2 = ploc;
	FNVD_CONSTRUCTED_LOCAL(PackageLocation, tempLoc, &PackageLocation::Destruct);
	ActorPackageData* pdata = GetInstanceDataThatIsRunning();
	if (pdata) {
		if (pdata->GetPackageType() == PACKAGE_TYPE_SANDBOX) {
			// 0x44EDB0, the exe's shared one-field getter of +0x10: SandBoxActorPackageData::GetLocation.
			TESObjectREFR* psandboxLoc = static_cast<SandBoxActorPackageData*>(pdata)->pLocation;
			if (psandboxLoc) {
				tempLoc.SetLocReference(psandboxLoc);
				ploc_2 = &tempLoc;
			}
		}
	}
	float afMax = fAISocialTimerForConversationsMax.Float();
	float afMin = fAISocialTimerForConversationsMin.Float();
	fCheckToTalkTimer = CombatFormulas::GetRandomBetween(afMin, afMax);
	AddTalkedtoActor(static_cast<Actor*>(aptalkTarget));
	if (static_cast<MobileObject*>(aptalkTarget)->GetCurrentAIProcess()) {
		BaseProcess* pTargetProcess = static_cast<MobileObject*>(aptalkTarget)->GetCurrentAIProcess();
		pTargetProcess->AddTalkedtoActor(apActor);
	}
	bool v11 = apActor->InitiateDialogue(static_cast<MobileObject*>(aptalkTarget), ploc_2, ploc_2, 1, 0, 0, 0, 1, v8);
	return v11;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessUseIdleMarker(Actor* apActor, TESObjectREFR* apRef) {
	using idlearray_t = BSSimpleArray<TESIdleForm*, 1024>;
	FNVD_INLINED_LOCAL(idlearray_t, idlearray, std::nullopt, std::nullopt);
	BGSIdleMarker* pIdleMark = static_cast<BGSIdleMarker*>(apRef->GetObjectReference());
	BGSIdleCollection::PICK_TYPE etype = pIdleMark->GetPickType();
	if (apRef) {
		SetTarget(apRef);
	LABEL_8:;
		if (DidPathingFail(apActor)) {
			EndMoveMessage(apActor);
			AddToProcedureIndexRunning(apActor, -1);
		} else {
			if (apActor->IsPathing()) {
				TESPackage* pPack = GetPackageThatIsRunning();
				float fcurrentDistance = apActor->GetDistanceFromReference(apRef, 0, 0);
				int32_t emoveMode = 257;
				float ftooCloseToRun = 64.0f;
				bool bcouldRun = 64.0f < static_cast<double>(fcurrentDistance);
				if (64.0f < static_cast<double>(fcurrentDistance)) {
					float fdistance = pPack->GetPackageRadiusActorToLocation(apActor, 0);
					float v6 = fdistance + fdistance;
					emoveMode = CalculateMoveMode(apActor, fcurrentDistance, fdistance, v6, 0, 0);
				}
				SetActorsAnimation(apActor, emoveMode, bcouldRun);
			} else {
				float fheightForIdleMarkers = 100.0f;
				float fradiusForIdleMarkers = 10.0f;
				float fradiusSquaredForIdleMarkers = fradiusForIdleMarkers * fradiusForIdleMarkers;
				const NiPoint3* locGoal = (const NiPoint3*)&pTarget->GetLocationOnReference();
				const NiPoint3* locCurrent = (const NiPoint3*)&apActor->GetLocationOnReference();
				NiPoint3 goalDelta = *locGoal - *locCurrent;
				// The exe compares the squared XY length and fabsf's result straight from ST0, unstored.
				if (!(fradiusSquaredForIdleMarkers < goalDelta.SqrXYLength()) &&
					!(fheightForIdleMarkers < std::fabsf(goalDelta.z))) {
					float fcurrentAngle;
					float funusedOutput;
					float fdeltaAngle;
					float fdesiredAngle;
					if (!pTarget ||
						(DidPathingFail(apActor) ||
							(apActor->GetSitSleepState() ||
								(fdesiredAngle = (&pTarget->GetAngleOnReference())->z,
									(fcurrentAngle = (&apActor->GetAngleOnReference())->z,
										(funusedOutput = 0.0f,
											(fdeltaAngle = GetAngleDelta(fcurrentAngle, fdesiredAngle, funusedOutput),
												!(std::fabs(fdeltaAngle) > 0.10000000149011612)))))))) {
						ClearAllHeadTrackTargets();
						::new (&idlearray) BSSimpleArray<TESIdleForm*, 1024>(10u);
						uint32_t icount = pIdleMark->GetIdlesForActor(apActor, pTarget, &idlearray);
						if (icount) {
							Animation* panim = apActor->GetAnimation();
							if (panim) {
								if ((!pCurrentIdle || panim->SpecialIdleDonePlaying()) && !apActor->IsParalyzed()) {
									TESIdleForm* pPrevIdle = pCurrentIdle;
									pCurrentIdle = 0;
									if (etype) {
										if (etype == BGSIdleCollection::SEQUENCE) {
											if (iPackageIdleNumber == -1 || iPackageIdleNumber + 1 >= icount) {
												iPackageIdleNumber = 0;
											} else {
												++iPackageIdleNumber;
											}
											pCurrentIdle = idlearray.GetAt(iPackageIdleNumber);
											fPackageIdleTimer = -1.0f;
										}
									} else {
										// The exe divides rand's result unsigned (xor edx, edx; div).
										uint32_t a1 = static_cast<uint32_t>(rt::rand()) % icount;
										pCurrentIdle = idlearray.GetAt(a1);
										iPackageIdleNumber = a1;
										if (pCurrentIdle) {
											fPackageIdleTimer = 0.8f;
										}
									}
									if (pCurrentIdle) {
										SetupSpecialIdle(apActor, pCurrentIdle, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 0);
									}
								}
							}
						}
						idlearray.~BSSimpleArray<TESIdleForm*, 1024>();
					} else {
						apActor->RequestRotate(fdesiredAngle, 1);
					}
				} else {
					apActor->SetPathfindingGoalAndAngle(pTarget, fradiusForIdleMarkers, 0);
				}
			}
		}
		return;
	}
	if (pTarget) {
		goto LABEL_8;
	}
	SetTargetForPackage(apActor);
	if (pTarget) {
		if (pTarget->GetObjectReference() && pTarget->GetObjectReference()->GetFormType() == IDLM_ID) {
			goto LABEL_8;
		}
	}
}

BSSimpleList<Actor*>* HighProcess::GetAvoidActorList() {
	return &AvoidActorList;
}

void HighProcess::FadeOut(Actor* apActor, TESObjectREFR* apRef, bool abTeleport) {
	if ((((int32_t)eFadeState) != 6) && (((int32_t)eFadeState) != 5)) {
		eFadeState = static_cast<HighProcess::FADE_STATE>(abTeleport ? 4 : 2);
		if (apRef) {
			pTeleportFadeRef = apRef;
		}
		if (fFadeAlpha <= 0.0) {
			fFadeAlpha = (float)1.0;
		}
	}
}

bool HighProcess::ProcessFollowOneHour(Actor* apActor, float afTimeDelta, bool abInterateIndex) {
	TESPackage* pPack = GetPackageThatIsRunning();
	if (!pPack) {
		return 0;
	}
	if (pPack->GetWaiting()) {
		if (pPlayer->IsPlayerFollower(apActor)) {
			return 0;
		}
		int32_t NumberOfFollowers = pPlayer->GetNumberOfFollowers();
		if (NumberOfFollowers >= iNumberActorsAllowedToFollowPlayer.Int()) {
			return 0;
		}
		pPack->SetWaiting(0);
	}
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	if (pTarget) {
		if (pTarget->GetDelete() || pTarget->GetDisabled()) {
			if (pTarget->GetDelete()) {
				pPack->SetNeverRun(pTarget, 1);
			}
			if (abInterateIndex) {
				AddToProcedureIndexRunning(apActor, 1);
			}
			return 0;
		} else {
			if (pTarget->IsDead(1)) {
				pPack->SetNeverRun(pTarget, 1);
				return 0;
			} else {
				if (pTarget &&
					(!pPack->pPackLoc ||
						(pPack->pPackLoc->GetLocType() >= PLT_CURRLOC ||
							!pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)))) {
					goto LABEL_48;
				}
				if (abInterateIndex) {
					AddToProcedureIndexRunning(apActor, 1);
				}
				if (pPack->GetIsCreated()) {
					ClearCurrentPackage();
					CheckforNewPackage(apActor, 0);
					return 0;
				} else {
					TESObjectCELL* pparent;
					TESPackage* pEditorPack;
				LABEL_48:;
					if (pPack->GetIsCreated() &&
						((pEditorPack = GetCurrentPackage()) != 0 &&
							((pparent = apActor->GetParentCell(), pEditorPack->GetPCContinue()) &&
								(pEditorPack->GetServicesEnabled() &&
									(pparent &&
										(pparent->IsActorCellOwner(apActor) &&
											apActor->LineOfSight(0, pTarget, 1, 0, 0))))))) {
						EndMoveMessage(apActor);
						return 0;
					} else {
						float fdistance = pPack->GetPackageRadiusActorToRefTarget(apActor, 0);
						if (pTarget) {
							if (static_cast<double>(fdistance) < apActor->GetDistanceFromReference(pTarget, 0, 0) &&
								(pTarget != pPlayer || !apActor->IsPathing())) {
								TESWorldSpace* LocationWorld = pPack->GetLocationWorld(apActor);
								TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
								apActor->SetPathfindingGoal(pPack->GetLocationCoord(apActor), LocationCell,
									LocationWorld, fdistance, 0);
							}
						}
						return 0;
					}
				}
			}
		}
	} else {
		if (abInterateIndex) {
			AddToProcedureIndexRunning(apActor, 1);
		}
		return 0;
	}
}

void HighProcess::FadeOutAndMove(Actor* apActor, TESObjectREFR* apTarget, float afX, float afY, float afZ) {
	if (eFadeState != HighProcess::FADE_OUT_DELETE && eFadeState != HighProcess::FADE_OUT_DISABLE) {
		eFadeState = HighProcess::FADE_MOVE_TO_MARKER;
		apActor->StopMoving();
		apActor->PickAnimations(1.0f, 1.0f);
		pMoveToFadeStruct = static_cast<MoveToFadeStruct*>(ourMalloc(sizeof(MoveToFadeStruct)));
		pMoveToFadeStruct->pRef = apTarget;
		pMoveToFadeStruct->x = afX;
		pMoveToFadeStruct->y = afY;
		pMoveToFadeStruct->z = afZ;
		if (fFadeAlpha <= 0.0) {
			fFadeAlpha = 1.0f;
		}
	}
}

void HighProcess::ProcessTakeBackItem(Actor* apActor) {
	TESPackage* pPackage = GetPackageThatIsRunning();
	if (apActor->GetSitSleepState()) {
		apActor->InitiateGetUpPackage();
	}
	if (!pTarget) {
		SetTarget(pPlayer);
	}
	if (fTakeBackTimer == -1.0) {
		fTakeBackTimer = fTakeBackTimerSetting.Float();
	} else {
		fTakeBackTimer = fTakeBackTimer - appTimer.GetSeconds();
	}
	if (fTakeBackTimer <= 0.0) {
		apActor->StartCombat(pPlayer, nullptr, true, false, false, 0, true, nullptr);
		return;
	}
	if (pPackage && !pPackage->IsActorAtRefTarget(apActor, 0)) {
		// The move modes are IDA's MF_FORWARD | MF_RUNNING (0x201) and MF_FORWARD | MF_WALKING (0x101); the PDB has
		// no enum for them.
		if (apActor->GetDistanceFromReference(pPlayer, false, false) > 1000.0) {
			ProcessTravel(apActor, false, false, 0x201u, false);
		} else {
			ProcessTravel(apActor, false, false, 0x101u, false);
		}
		return;
	}
	if (GetCurrentActionComplete()) {
		TESPackage* pExtraPackage = apActor->GetExtra()->GetPackageExtra();
		if (pExtraPackage && pExtraPackage->GetPackType() == PACKAGE_TYPE_SLEEP) {
			SetTarget(pPlayer);
			// IDA's MF_FORWARD | MF_WALKING.
			ProcessFollow(apActor, false, 0x101u, false);
		} else {
			AddToProcedureIndexRunning(apActor, 1);
		}
		return;
	}
	SetCurrentActionComplete(true);
	apActor->RequestRotate(pPlayer->GetLocationOnReference(), false);
	TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 9);
	apActor->SetDialogueItemTarget(pPlayer);
	BaseProcess* pProcess = apActor->GetCurrentAIProcess();
	pProcess->ProcessGreet(apActor, pTopic, false, false, true, false);
	bActivateAnim = true;
	FindSpecialIdletoPlay(apActor, pPlayer->TESObjectREFR::GetObjectReference(), pPlayer);
	BSSimpleList<Crime*>* pCrimeList = apActor->GetExtra()->GetPlayerCrimeList();
	while (pCrimeList && pCrimeList->GetItem()) {
		Crime* pCrime = pCrimeList->GetItem();
		if (pCrime && (!pCrime->GetCrimeType() || pCrime->GetCrimeType() == 1)) {
			TESObjectREFR* pStolenContainer = apActor;
			if (apActor->GetExtra()->GetMerchantContainer()) {
				pStolenContainer = apActor->GetExtra()->GetMerchantContainer();
			}
			TESForm* pOwner = pCrime->GetOwnershipCrime();
			pPlayer->GetExtra()->GetContainerChanges()->RemoveStolenItems(pPlayer, pStolenContainer, pOwner);
			pCrimeList->Remove(pCrime);
			ReferenceProcessLists.RemoveCrimeGlobalCrimeList(pCrime);
			if (pCrime) {
				delete pCrime;
			}
			pCrimeList = apActor->GetExtra()->GetPlayerCrimeList();
		} else {
			pCrimeList = pCrimeList->GetNext();
		}
	}
}

bool HighProcess::FindSpecialIdletoPlay(Actor* apActor, TESBoundObject* apObject, TESObjectREFR* apRef) {
	TESIdleManager::SetUsedItem(apObject);
	TESIdleManager::SetUsedItemActivate(1);
	int32_t iItemLevel = 0;
	if (apRef) {
		double fEyeLevel = apActor->GetEyeLevel();
		double fWorldEyeLevel = apActor->GetLocationOnReference().z + fEyeLevel;
		float fReachHeight = fWorldEyeLevel - apActor->GetReach();
		if (fReachHeight < static_cast<double>(apRef->GetLocationOnReference().z) || apRef->IsNpc()) {
			iItemLevel = 1;
		}
	}
	TESIdleManager::SetUsedItemLevel(iItemLevel);
	bool bIdleFound = SetupSpecialIdle(apActor, 0, AnimIdle::PLAY_TYPE_AUTO, 1, 0, 1);
	TESIdleManager::SetUsedItem(0);
	TESIdleManager::SetUsedItemActivate(0);
	TESIdleManager::SetUsedItemLevel(-1);
	return bIdleFound;
}

bool HighProcess::CheckTargetCombatDetectionState(Actor* apActor, Actor* apTarget) {
	uint32_t eState;
	eState = CombatDetectionCycles[iDetectionCounter].eState;
	if (eState) {
		if (eState == 1) {
			if (apTarget == apActor->GetCurrentCombatTarget()) {
				return (bool)1;
			}
			if (apActor->IsInCombatWithActor(apTarget)) {
				return (bool)1;
			}
		} else {
			if (eState == 2) {
				if (apTarget == apActor->GetCurrentCombatTarget()) {
					return (bool)1;
				}
				if ((!apActor->IsInCombatWithActor(apTarget)) && (!apActor->IsCombatGroupMember(apTarget))) {
					return (bool)1;
				}
			} else {
				Warning("COMBAT: Unknown combat detection state %i", CombatDetectionCycles[iDetectionCounter].eState);
			}
		}
	} else {
		if (apTarget == apActor->GetCurrentCombatTarget()) {
			return (bool)1;
		}
	}
	return (bool)0;
}

bool HighProcess::ShouldRunCombatDetection(Actor* apActor, Actor* apTarget) {
	uint8_t bCheckCone;
	if (apTarget->IsDead((bool)0) || apTarget->IsUnConscious()) {
		return (bool)0;
	}
	bCheckCone = (uint8_t)CombatDetectionCycles[iDetectionCounter].bCheckCone;
	if (static_cast<const void*>(apTarget) == static_cast<const void*>(pPlayer)) {
		bCheckCone = 0;
	}
	if (!CheckTargetCombatDetectionState(apActor, apTarget)) {
		return (bool)0;
	}
	if (!bCheckCone) {
		return (bool)1;
	}
	NiPoint3* pTargetPos = apTarget->TESObjectREFR::QLocationOnReference();
	return (bool)((uint8_t)apActor->IsPointInViewCone(*pTargetPos, 1.5707963705062866f));
}

bool HighProcess::ShouldRunCombatDetectionEventCheck(Actor* apActor, Actor* apTarget) {
	return CombatDetectionCycles[iDetectionCounter].bRunDetectionEventCheck &&
		CheckTargetCombatDetectionState(apActor, apTarget);
}

// The exe calls LowProcess's body directly, not through the vtable.
bool HighProcess::ProcessAcquireOneHour(Actor* apActor, float afTimeDelta) {
	LowProcess::ProcessAcquire(apActor);
	return true;
}

// The exe calls LowProcess's body directly, not through the vtable.
bool HighProcess::ProcessPatrolOneHour(Actor* apActor, float afTimeDelta) {
	LowProcess::ProcessPatrol(apActor);
	return true;
}

bhkShapePhantom* HighProcess::QDoorDetectionPhantom() {
	return spShapePhantom.m_pObject;
}

bool HighProcess::CheckPhantom() {
	return spShapePhantom.m_pObject != nullptr;
}

void HighProcess::RemovePhantom() {
	if (spShapePhantom.m_pObject) {
		spShapePhantom.m_pObject->Remove();
		spShapePhantom = nullptr;
	}
}

// Returns whether the previous update was skipped.
bool HighProcess::GetSkippedLastUpdate() {
	return bSkippedUpdate;
}

// Removes deleted or missing targets and refreshes the active head target when the list changes.
void HighProcess::UpdateHeadTrackTargets() {
	bool bChanged = false;
	for (int32_t iTarget = 5; iTarget >= 0; --iTarget) {
		if (HeadTrackingTargetFlags[iTarget] &&
			(!HeadTrackingTargets[iTarget] || HeadTrackingTargets[iTarget]->GetDelete())) {
			HeadTrackingTargetFlags[iTarget] = 0;
			HeadTrackingTargets[iTarget] = nullptr;
			bChanged = true;
		}
	}
	if (bChanged)
		OnNewHeadTrackTarget();
}

void HighProcess::OnNewHeadTrackTarget() {
	int32_t i;
	TESObjectREFR* v2 = nullptr;
	for (i = 5; i >= 0; --i) {
		if (HeadTrackingTargetFlags[i]) {
			v2 = HeadTrackingTargets[i];
			break;
		}
	}
	if (pLastTarget != v2) {
		bForceRotate = (bool)0;
		pLastTarget = v2;
	}
}

// Selects the actor's bone LOD controller, if its skeleton exposes one.
void HighProcess::InitBoneLOD(Actor* apActor) {
	pBoneLOD = nullptr;
	NiNode* pNode = apActor->TESObjectREFR::Get3D();
	if (!pNode)
		return;

	NiAVObject* pBone = apActor->GetActorBone(pNode, FixedStrings::QBip01());
	if (!pBone)
		return;

	for (NiTimeController* pController = pBone->GetControllers(); pController; pController = pController->GetNext()) {
		if (NiObject::IsExactKindOf(&NiBSBoneLODController::ms_RTTI, pController)) {
			pBoneLOD = static_cast<NiBSBoneLODController*>(pController);
			return;
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::SetupPhantom(Actor* apActor) {
	if (apActor->pCurrentProcess->GetBSBound()) {
		if (!this->spShapePhantom.m_pObject) {
			alignas(16) CFilter kCharFilter;
			alignas(16) NiPoint3 playerextents;
			double v2;
			CFilter ifilter;
			bhkBoxShape* pplayerShape;
			playerextents = apActor->pCurrentProcess->GetBSBound()->Extents;
			float actorScale = apActor->GetScale();
			if (!(playerextents.z < static_cast<double>(playerextents.y))) {
				v2 = playerextents.y * 2.5 * actorScale;
			} else {
				v2 = playerextents.y * actorScale;
			}
			playerextents.y = v2;
			playerextents.x = playerextents.x * 0.5 * actorScale;
			playerextents.z = playerextents.z * actorScale;
			bhkBoxShape* v18 = static_cast<bhkBoxShape*>(NiMemObject::operator new(sizeof(bhkBoxShape)));
			if (v18) {
				pplayerShape = ::new (v18) bhkBoxShape(playerextents);
			} else {
				pplayerShape = 0;
			}
			ifilter.iFilter = 0;
			ifilter.SetLayer(L_DOORDETECTION);
			bhkCharacterController* pcon = this->GetCharController();
			if (pcon) {
				CFilter* CollisionFilter = pcon->GetCollisionFilter(&kCharFilter);
				uint32_t Group = CollisionFilter->GetGroup();
				ifilter.SetGroup(Group);
			}
			// The exe constructs it with hkCachingShapePhantomCinfo's constructor (0x56E090), the same function.
			FNVD_CONSTRUCTED_LOCAL(hkSimpleShapePhantomCinfo, csi, &hkSimpleShapePhantomCinfo::Destruct);
			csi.m_collisionFilterInfo = ifilter.iFilter;
			csi.m_shape = pplayerShape->operator*();
			const NiPoint3* v5 = &apActor->GetLocationOnReference();
			csi.m_transform.Trans.NI2HK(*v5);
			bhkSimpleShapePhantom* v9;
			bhkSimpleShapePhantom* v10 =
				static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
			if (v10) {
				v9 = ::new (v10) bhkSimpleShapePhantom(csi);
			} else {
				v9 = 0;
			}
			this->spShapePhantom.operator=(static_cast<bhkShapePhantom*>(v9));
			// The exe copies the setting with fld/fstp.
			float fTime = fINIDetectDoorsForPathingTime.Float();
			this->fDetectDoorForPathingTimer = fTime;
		}
		bhkWorld* pw = apActor->GetParentCell()->GetbhkWorld();
		bhkShapePhantom* v7 = this->spShapePhantom.m_pObject;
		v7->Add(pw);
	}
}

float HighProcess::GetActorHeight() {
	return fCachedActorHeight;
}

void HighProcess::SetActorHeight(float afHeight) {
	fCachedActorHeight = afHeight;
}

float HighProcess::GetFadeAlpha() {
	return fFadeAlpha;
}

void HighProcess::SetMaxAlpha(float afAlpha) {
	fMaxAlpha = afAlpha;
}

void HighProcess::SkipFadeIn(Actor* apActor) {
	if (eFadeState == FADE_IN || eFadeState == FADE_TELEPORT_IN) {
		fFadeAlpha = 1.0f;
		eFadeState = FADE_NORMAL;
		apActor->UpdateAlpha();
	}
}

void HighProcess::FadeIn(Actor* apActor, bool abTeleport) {
	if ((((int32_t)eFadeState) != 6) && (((int32_t)eFadeState) != 5)) {
		eFadeState = static_cast<HighProcess::FADE_STATE>(abTeleport ? 3 : 1);
		pTeleportFadeRef = nullptr;
		if (fFadeAlpha >= 1.0) {
			fFadeAlpha = (float)0.0;
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ReconcileDetectionData(Actor* apActor) {
	EvaluateDetectionTimer.Start();
	bool bupdated = false;
	BSSimpleList<DetectionState*>* plist = pThreadDetectList;
	while (plist && plist->m_item) {
		DetectionState* pstate = plist->m_item;
		DetectionState* ptemp = GetDetectionState(pstate->pActor, 0);
		if (!ptemp) {
			DetectionState* pMemory = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
			if (pMemory) {
				ptemp = ::new (pMemory) DetectionState;
			} else {
				ptemp = 0;
			}
			pDetectedActorList->AddHead(ptemp);
		}
		ptemp->CopyNewData(pstate);
		bupdated = true;
		ourFree(pstate);
		plist = plist->GetNext();
	}
	pThreadDetectList->RemoveAll();
	if (bupdated) {
		CombatController* pcombatController = apActor->GetCombatController();
		if (pcombatController) {
			pcombatController->SetUpdatedDetection(1);
		}
	}
	for (plist = pTempActorsWhoDetectMeList; plist && plist->m_item; plist = plist->GetNext()) {
		DetectionState* pstate = plist->m_item;
		DetectionState* ptemp = GetDetectionState(pstate->pActor, 1);
		if (!ptemp) {
			DetectionState* pMemory = static_cast<DetectionState*>(ourMalloc(sizeof(DetectionState)));
			if (pMemory) {
				ptemp = ::new (pMemory) DetectionState;
			} else {
				ptemp = 0;
			}
			pActorsWhoDetectMeList->AddHead(ptemp);
		}
		ptemp->CopyNewData(pstate);
		ourFree(pstate);
	}
	pTempActorsWhoDetectMeList->RemoveAll();
	EvaluateDetectionTimer.Stop();
}

float HighProcess::QMaxAlpha() const {
	return fMaxAlpha;
}

float HighProcess::GetEvalTimer() const {
	return fPackageEvalTimer;
}

float HighProcess::GetCurrentRadiation() const {
	return fHighestRadiation;
}

float HighProcess::GetRadReEvaluationTimer() const {
	return fRadiationTimer;
}

void HighProcess::SetVoiceTimer(float afTime) {
	fVoiceTimer = afTime;
}

void HighProcess::SetCurrentRadiation(float a2) {
	fHighestRadiation = a2;
}

void HighProcess::ProcessAccompanyOneHour(Actor* apActor, float fTimeDelta) {
	ProcessFollowOneHour(apActor, fTimeDelta, 1);
}

void HighProcess::FadeAndDelete(Actor* apActor) {
	eFadeState = HighProcess::FADE_OUT_DELETE;
	pTeleportFadeRef = nullptr;
	if (fFadeAlpha <= 0.0) {
		fFadeAlpha = (float)1.0;
	}
	uint8_t v2 = 1;
	if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
		v2 = (uint8_t)pPlayer->Is3rdPerson();
		if (!v2) {
			pPlayer->SetFirstPerson(0);
		}
	}
	apActor->StopMoving();
	apActor->PickAnimations(1.0, 1.0);
	if (!v2) {
		pPlayer->SetFirstPerson(1);
	}
}

void HighProcess::FadeAndDisable(Actor* apActor) {
	eFadeState = HighProcess::FADE_OUT_DISABLE;
	pTeleportFadeRef = nullptr;
	if (fFadeAlpha <= 0.0) {
		fFadeAlpha = (float)1.0;
	}
	uint8_t v2 = 1;
	if (static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
		v2 = (uint8_t)pPlayer->Is3rdPerson();
		if (!v2) {
			pPlayer->SetFirstPerson(0);
		}
	}
	apActor->StopMoving();
	apActor->PickAnimations(1.0, 1.0);
	if (!v2) {
		pPlayer->SetFirstPerson(1);
	}
}

bool HighProcess::DoesFadeProhibitAction() {
	int32_t eFadeState;
	eFadeState = (int32_t)this->eFadeState;
	return (bool)((uint8_t)((eFadeState >= 3) && (eFadeState <= 6)));
}

void HighProcess::HighCopy(HighProcess* apProcess) {
	fPackageEvalTimer = apProcess->GetEvalTimer();
	fUseItemTimer = apProcess->fUseItemTimer;
	fHoldAttackTimer = apProcess->fHoldAttackTimer;
	bActivateAnim = apProcess->bActivateAnim;
	bIsDoingSayTo = apProcess->IsDoingSayTo();
	for (BSSimpleList<DetectionState*>* plist = apProcess->pDetectedActorList; plist && plist->m_item;
		plist = plist->m_pkNext) {
		pDetectedActorList->AddTail(plist->m_item);
	}
	for (BSSimpleList<DetectionState*>* plist_2 = apProcess->pActorsWhoDetectMeList; plist_2 && plist_2->m_item;
		plist_2 = plist_2->m_pkNext) {
		pActorsWhoDetectMeList->AddTail(plist_2->m_item);
	}
	pBoneLOD = apProcess->GetBoneLOD();
	iLastBoneLOD = apProcess->iLastBoneLOD;
	iHasHealingSpell = apProcess->iHasHealingSpell;
	iHasHealingPotion = apProcess->iHasHealingPotion;
	bCurrentlyReanimating = apProcess->bCurrentlyReanimating;
	pLipSynicAnim = apProcess->pLipSynicAnim;
	bWaitingForLipFile = apProcess->bWaitingForLipFile;
	pDialogTarget = apProcess->pDialogTarget;
	bLipQuequed = apProcess->bLipQuequed;
	bProcessGreetSayTo = apProcess->bProcessGreetSayTo;
	bEvaluateDetection = apProcess->bEvaluateDetection;
	fLightLevel = apProcess->fLightLevel;
	pPathLookAtTarget = apProcess->GetPathLookAtTarget();
}

void HighProcess::ClearShootingAction(Actor* a1) {
	a1->ClearLookAtTarget();
	ClearAutomaticShotsToFire(0);
	bAimingTarget = 0;
}

void HighProcess::ProcessSearch(Actor* apActor) {
	TESObjectCELL* parentCell;
	TESWorldSpace* worldSpace;
	NiPoint3 kTargetPos;
	SearchPackage* pSearchPackage = nullptr;
	TESPackage* pPackage = GetPackageThatIsRunning();
	if (pPackage && (((int32_t)pPackage->GetPackType()) == 30)) {
		pSearchPackage = static_cast<SearchPackage*>(GetPackageThatIsRunning());
	} else {
		if (!pPackage) {
			return;
		}
	}
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	apActor->SetAlert(1);
	fEvaluateAcquireTimer = (float)((&appTimer)->GetSeconds() + fEvaluateAcquireTimer);
	SetActorsAnimation(apActor, 0x201u, (bool)1);
	if (apActor->IsPathingComplete()) {
		SetCurrentActionComplete((uint8_t)1u);
		pSearchPackage->LookForTarget(&kTargetPos, apActor);
		worldSpace = pTarget->GetWorldSpace();
		parentCell = pTarget->TESObjectREFR::GetParentCell();
		if (apActor->SetPathfindingGoal(kTargetPos, parentCell, worldSpace, (float)0.0, nullptr)) {
			lastSeenPostion = kTargetPos;
		}
	}
}

void HighProcess::SayGoodBye(Actor* apActor) {
	TESPackage* currentPackage;
	BaseProcess* v3;
	BaseProcess* currentAIProcess;
	TESTopic* topic = TESTopic::GetTopic(DT_CONVERSATION, 2);
	apActor->SetDialogueItemTarget(pPlayer);
	if (topic) {
		currentAIProcess = apActor->MobileObject::GetCurrentAIProcess();
		currentAIProcess->ProcessGreet(apActor, topic, (bool)0, (bool)1, (bool)0, (bool)0);
	}
	if (((int32_t)apActor->GetSitSleepState()) == 0) {
		if ((!apActor->GetCurrentPackage()) ||
			((currentPackage = apActor->GetCurrentPackage()), (((int32_t)currentPackage->GetPackType()) != 6))) {
			v3 = apActor->MobileObject::GetCurrentAIProcess();
			v3->CreateFollownoEscort(apActor, (bool)1);
		}
	}
}

void HighProcess::ProcessGuardStartTravelAndReset(Actor* a2, GuardActorPackageData* a3) {
	ProcessGuardStartTravelToLocation(a2, a3);
	a3->ClearWarningCount();
	a3->ClearWarningTimer();
}

bool HighProcess::ProcessEatOneHour(Actor* apActor, float afTimeDelta) {
	TESPackage* v5 = GetPackageThatIsRunning();
	if (!v5) {
		return (bool)0;
	}
	EndMoveMessage(apActor);
	if (v5->IsActorAtLocation(apActor, (bool)0, (-0x1p+0f), (bool)0) || (((int32_t)apActor->GetSitSleepState()) == 4)) {
		SetCurrentActionComplete((uint8_t)1u);
		return (bool)0;
	} else {
		SetProcedureIndexRunning((uint32_t)0);
		return (bool)1;
	}
}

bool HighProcess::FadeUpdate(Actor* apActor) {
	float ffadeDelta;
	if (eFadeState == HighProcess::FADE_NORMAL) {
		if (!(fFadeAlpha >= 1.0)) {
			fFadeAlpha = 1.0f;
			apActor->UpdateAlpha();
		}
		return 0;
	}
	if (apActor == pPlayer) {
		double Seconds = appTimer.fDelta;
		ffadeDelta = Seconds / fPlayerTeleportFadeSeconds.Float();
	} else {
		double v3 = appTimer.fDelta;
		ffadeDelta = v3 / fActorTeleportFadeSeconds.Float();
	}
	switch (eFadeState) {
	case HighProcess::FADE_IN:
	case HighProcess::FADE_TELEPORT_IN: {
		fFadeAlpha = fFadeAlpha + ffadeDelta;
		if (fFadeAlpha > 1.0) {
			fFadeAlpha = 1.0f;
			eFadeState = HighProcess::FADE_NORMAL;
		}
	LABEL_45:;
		apActor->UpdateAlpha();
		return DoesFadeProhibitAction();
	}
	case HighProcess::FADE_OUT: {
		if (fFadeAlpha > 0.0) {
			fFadeAlpha = fFadeAlpha - ffadeDelta;
			if (fFadeAlpha < 0.0) {
				fFadeAlpha = 0.0f;
				pTeleportFadeRef = 0;
			}
		}
		goto LABEL_45;
	}
	case HighProcess::FADE_MOVE_TO_MARKER: {
		if (fFadeAlpha > 0.0) {
			fFadeAlpha = fFadeAlpha - ffadeDelta;
			if (fFadeAlpha <= 0.0) {
				fFadeAlpha = 0.0f;
				if (pMoveToFadeStruct) {
					Script::MoveToFunctionBase(apActor, pMoveToFadeStruct->pRef, pMoveToFadeStruct->x,
						pMoveToFadeStruct->y, pMoveToFadeStruct->z);
					ourFree(pMoveToFadeStruct);
				}
				pMoveToFadeStruct = 0;
				pTeleportFadeRef = 0;
			}
		}
		goto LABEL_45;
	}
	}
	if (eFadeState != HighProcess::FADE_TELEPORT_OUT) {
		if (eFadeState == HighProcess::FADE_OUT_DELETE) {
			fFadeAlpha = fFadeAlpha - ffadeDelta;
			if (fFadeAlpha < 0.0) {
				fFadeAlpha = 0.0f;
				apActor->SetDelete(1);
			}
		} else {
			if (eFadeState == HighProcess::FADE_OUT_DISABLE) {
				fFadeAlpha = fFadeAlpha - ffadeDelta;
				if (fFadeAlpha < 0.0) {
					fFadeAlpha = 0.0f;
					apActor->Disable();
				}
			}
		}
		goto LABEL_45;
	}
	if (apActor != pPlayer) {
		fFadeAlpha = fFadeAlpha - ffadeDelta;
		if (fFadeAlpha < 0.0) {
			fFadeAlpha = 0.0f;
			TESObjectREFR* ptempref_2 = pTeleportFadeRef;
			pTeleportFadeRef = 0;
			if (ptempref_2) {
				ptempref_2->Activate(apActor, 0, 0, 1);
			}
			if (apActor != pPlayer && apActor->GetDesiredProcessLevel()) {
				eFadeState = HighProcess::FADE_NORMAL;
				return 1;
			}
			FadeIn(apActor, 1);
		}
		goto LABEL_45;
	}
	fFadeAlpha = 1.0f;
	TESObjectREFR* ptempref = pTeleportFadeRef;
	pTeleportFadeRef = 0;
	if (ptempref) {
		ptempref->Activate(apActor, 0, 0, 1);
	}
	eFadeState = HighProcess::FADE_NORMAL;
	return 1;
}

void HighProcess::ProcessAlertSearch(Actor* pActor) {
	Animation* pAnim;
	if (GetPackageThatIsRunning()) {
		pActor->SetAlert(1);
		if (!pTarget) {
			SetTargetForPackage(pActor);
		}
		pAnim = pActor->GetAnimation();
		if (pAnim) {
			if (pAnim->SpecialIdleDonePlaying()) {
				FindSpecialIdletoPlay(pActor, nullptr, nullptr);
			}
		}
		AddToProcedureIndexRunning(pActor, 1u);
	}
}

bool HighProcess::ProcessSleepOneHour(Actor* apActor, float afTimeDelta) {
	TESPackage* v5 = GetPackageThatIsRunning();
	if (!v5) {
		return (bool)0;
	}
	EndMoveMessage(apActor);
	if (v5->IsActorAtLocation(apActor, (bool)0, (-0x1p+0f), (bool)0) || (((int32_t)apActor->GetSitSleepState()) == 9)) {
		SetCurrentActionComplete((uint8_t)1u);
		return (bool)0;
	} else {
		SetProcedureIndexRunning((uint32_t)0);
		return (bool)0;
	}
}

void HighProcess::ProcessGuardIdle(Actor* a1, GuardActorPackageData* a2, bool a3) {
	bool v18;
	TESPackage* ppack = GetPackageThatIsRunning();
	bool ballowWarnAndAttack = ppack->pPackLoc != 0;
	TESObjectREFR* pguardedRef = GetTarget();
	if (ballowWarnAndAttack) {
		v18 = ppack->IsActorAtLocation(a1, 0, -1.0f, 0);
	} else {
		double DistanceFromReference = a1->GetDistanceFromReference(pguardedRef, 0, 0);
		v18 = static_cast<double>(ppack->GetPackageTarget()->iValue) > DistanceFromReference;
	}
	TESPackage* ppack_2 = GetPackageThatIsRunning();
	float FormFlags = static_cast<float>(ppack_2->GetPackageTarget()->iValue);
	float fguardSqrRadius_2 = FormFlags * FormFlags;
	Actor* pwarnedActor = 0;
	BSSimpleList<DetectionState*>* plist = pDetectedActorList;
	TESPackage* v5 = GetPackageThatIsRunning();
	if (!v5->GetGuardWarnBehavior()) {
		plist = 0;
	}
	while (plist && !plist->IsEmpty()) {
		DetectionState* pstate = plist->m_item;
		plist = plist->m_pkNext;
		Actor* ptargActor = pstate->pActor;
		if (ptargActor) {
			if (ptargActor != pguardedRef) {
				if (ptargActor->pCurrentProcess) {
					if (!ptargActor->pCurrentProcess->eLevel &&
						(!ptargActor ||
							(!ptargActor->IsDead(0) &&
								(!ptargActor->IsDying() &&
									(!ptargActor->IsKnockedOut() &&
										(!ptargActor->IsParalyzed() && !ptargActor->IsUnConscious()))))) &&
						pstate->iLevel > 0) {
						Actor* v16 = static_cast<Actor*>(GetTarget());
						const NiPoint3* X = &v16->GetLocationOnReference();
						const NiPoint3* v6 = &ptargActor->GetLocationOnReference();
						float fsqrDist = (*v6 - *X).SqrLength();
						if (fguardSqrRadius_2 > static_cast<double>(fsqrDist)) {
							FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
							if (!pguardedRef ||
								((!pguardedRef->GetOwner() || !pguardedRef->IsAnOwner(ptargActor, 1)) &&
									a1->GetShouldAttackActor(ptargActor, 1, ereaction, 0))) {
								pwarnedActor = ptargActor;
								break;
							}
						}
					}
				}
			}
		}
	}
	if (pwarnedActor) {
		if (a3) {
			EndMoveMessage(a1);
			// The exe pushes 0 for abForceTurn, then copies the location into the argument slot.
			a1->RequestRotate(pwarnedActor->GetLocationOnReference(), 0);
		}
		a2->SetTarget(pwarnedActor);
		SayGuardWarning(a1, a2);
		a2->eState = GuardActorPackageData::GUARD_STATE_WAIT_TO_ATTACK;
	} else {
		ClearActionHeadTrackTarget(0);
		ClearUseWeaponHeadTrackTarget(0);
		a1->ClearLookAtTarget();
		if (a3) {
			bool bwandering = 0;
			if (ballowWarnAndAttack) {
				TESPackage* v8 = GetPackageThatIsRunning();
				if (v8->GetLocationRadius(a1)) {
					ProcessWander(a1, 0, -1.0f);
					bwandering = 1;
				}
			} else {
				if (fguardSqrRadius_2 > 0.0) {
					// sqrtf's result goes straight into ProcessWander's radius slot (fstp [esp]).
					float fradius = std::sqrtf(fguardSqrRadius_2);
					const NiPoint3* v9 = &pguardedRef->GetLocationOnReference();
					ProcessWander(a1, v9, fradius);
					bwandering = 1;
				}
			}
			if (!bwandering) {
				TESObjectREFR* plocRef = 0;
				PackageLocation* ppackLoc = ppack->pPackLoc;
				if (ppackLoc) {
					plocRef = ppackLoc->GetLocReference();
				}
				if (pGenericLocation) {
					plocRef = pGenericLocation;
				}
				if (!a1->IsPathing() && a1->GetSitSleepState() == SIT_SLEEP_NORMAL) {
					TESBoundObject* ObjectReference;
					if ((plocRef &&
							(ObjectReference = plocRef->GetObjectReference(), ObjectReference == pXMarkerHeading)) ||
						(plocRef && (ppackLoc && ppackLoc->GetLocType() == PLT_EDITORLOC))) {
						float fgoalAngle = (&plocRef->GetAngleOnReference())->z;
						const NiPoint3* AngleOnReference = &a1->GetAngleOnReference();
						float v15 = fgoalAngle - AngleOnReference->z;
						if (std::fabs(v15) > 0.01745329238474369) {
							a1->RequestRotate(fgoalAngle, 1);
						}
					}
				}
			}
		}
	}
}

void HighProcess::SayGuardWarning(Actor* a1, GuardActorPackageData* a2) {
	TESObjectREFR* v5 = GetTarget();
	if (v5) {
		v5->GetOwner();
	}
	TESTopic* Topic = TESTopic::GetTopic(DT_COMBAT, 20);
	a2->IncWarningCount();
	a2->ClearWarningTimer();
	ProcessGreet(a1, Topic, 0, 0, 1, 0);
}

// IDA name; not in the PDB. Bubble-sorts the objects-to-acquire list by eType, highest first, swapping items
// through the folded pointer-list SetItem. The return value is the exe's leftover eax: the last list node the outer
// loop reached, or 0 when the list is empty.
BSSimpleList<ObjectstoAcquire*>* HighProcess::SortObjectsListByType() {
	BSSimpleList<ObjectstoAcquire*>* pList = &ObjectList;
	if (pList != nullptr) {
		int32_t iCount = pList->ItemsInList();
		if (!iCount) {
			return nullptr;
		}
		bool bSwapped = 1;
		while (iCount && bSwapped) {
			bSwapped = 0;
			for (BSSimpleList<ObjectstoAcquire*>* pNode = pList; pNode; pNode = pNode->GetNext()) {
				ObjectstoAcquire* pItem = pList->m_item;
				ObjectstoAcquire* pOther = pNode->m_item;
				// The exe compares signed (jle).
				if (static_cast<int32_t>(pItem->eType) > static_cast<int32_t>(pOther->eType)) {
					pList->SetItem(pOther);
					pNode->SetItem(pItem);
					bSwapped = 1;
				}
			}
			pList = pList->GetNext();
			--iCount;
		}
	}
	return pList;
}

// IDA name; not in the PDB. Bubble-sorts the objects-to-acquire list by iWantValue, highest first, as
// SortObjectsListByType does; the return value is the exe's leftover eax likewise.
BSSimpleList<ObjectstoAcquire*>* HighProcess::SortObjectsListByValue() {
	BSSimpleList<ObjectstoAcquire*>* pList = &ObjectList;
	if (pList != nullptr) {
		int32_t iCount = pList->ItemsInList();
		if (!iCount) {
			return nullptr;
		}
		bool bSwapped = 1;
		while (iCount && bSwapped) {
			bSwapped = 0;
			for (BSSimpleList<ObjectstoAcquire*>* pNode = pList; pNode; pNode = pNode->GetNext()) {
				ObjectstoAcquire* pItem = pList->m_item;
				ObjectstoAcquire* pOther = pNode->m_item;
				// The exe compares the unsigned field signed (jle).
				if (static_cast<int32_t>(pItem->iWantValue) > static_cast<int32_t>(pOther->iWantValue)) {
					pList->SetItem(pOther);
					pNode->SetItem(pItem);
					bSwapped = 1;
				}
			}
			pList = pList->GetNext();
			--iCount;
		}
	}
	return pList;
}

void HighProcess::ProcessWarning(Actor* apActor) {
	TESPackage* pbasepack = GetPackageThatIsRunning();
	TrespassPackage* ppack = 0;
	Actor* ptargetactor = 0;
	if (!pTarget) {
		SetTargetForPackage(apActor);
	}
	if (pTarget) {
		if (pTarget->IsActor()) {
			ptargetactor = static_cast<Actor*>(pTarget);
		}
	}
	if (pbasepack) {
		if (pbasepack->GetPackType() == PACKAGE_TYPE_TRESPASS) {
			ppack = static_cast<TrespassPackage*>(pbasepack);
		}
	}
	bool bflag = 0;
	if (ppack) {
		if (ptargetactor) {
			if (ptargetactor->IsTresPassing()) {
				FIGHT_REACTION ereaction;
				if ((ppack->GetPackType() == PACKAGE_TYPE_STEAL_WARNING ||
						ppack->GetPackType() == PACKAGE_TYPE_PICKPOCKET_WARNING) &&
					(ereaction = FIGHT_REACTION_NEUTRAL,
						apActor->GetShouldAttackActor(ptargetactor, 0, ereaction, 0))) {
					apActor->EndInterruptPackage(0);
					apActor->StartCombat(ptargetactor, 0, 0, 0, 0, 0, 1, 0);
				} else {
					if (ppack->fwarningtimer <= 0.0) {
						TESForm* Owner = pPlayer->GetParentCell()->GetOwner();
						Script::SetCrimeVictim(Owner);
						TESTopic* pTopic = TESTopic::GetTopic(DT_COMBAT, 20);
						Script::SetCrimeVictim(0);
						if (pTopic) {
							ProcessGreet(apActor, pTopic, 0, 0, 0, 0);
							float* v3 = &fAITrespassWarningTimer.Float();
							ppack->ModWarningTimer(*v3);
							ppack->ModWarning(1);
							bflag = 1;
						}
					}
					if (bflag) {
						AddToProcedureIndexRunning(apActor, -1);
					}
				}
			} else {
				AddToProcedureIndexRunning(apActor, 1);
			}
		}
	} else {
		if (ptargetactor) {
			TESTopic* pTopic_2;
			if (pbasepack->GetPackType() == PACKAGE_TYPE_STEAL_WARNING) {
				pTopic_2 = TESTopic::GetTopic(DT_COMBAT, 9);
			} else {
				pTopic_2 = TESTopic::GetTopic(DT_COMBAT, 12);
			}
			BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
			CurrentAIProcess->ProcessGreet(ptargetactor, pTopic_2, 0, 0, 1, 0);
			AddToProcedureIndexRunning(apActor, 1);
		}
	}
}

void HighProcess::ProcessCreateFollow(Actor* apActor) {
	TESPackage* ppack = GetPackageThatIsRunning();
	if (ppack) {
		PackageTarget* ptarg = ppack->GetPackageTarget();
		if (ptarg) {
			ActorPackage* pactorPack;
			int32_t itotalFollowersNeeded;
			PACK_TARG_TYPE etype = ptarg->GetTargType();
			bool v9 = etype == PTT_REFERENCE || etype == PTT_LINKEDREF;
			bool bobjTarget = etype == PTT_OBJECT;
			EscortActorPackageData* pescortData = static_cast<EscortActorPackageData*>(GetInstanceDataThatIsRunning());
			TESBoundObject* ptype = static_cast<TESBoundObject*>(ptarg->GetTargObject());
			Actor* item = pPlayer;
			bool bescortIncludesPlayer = pescortData->EscortedActors.IsInList(item);
			if (v9) {
				itotalFollowersNeeded = 1;
			} else {
				itotalFollowersNeeded = ptarg->iValue;
			}
			int32_t v26 = itotalFollowersNeeded;
			int32_t inewFollowersNeeded = itotalFollowersNeeded;
			if (bobjTarget) {
				int32_t NumEscortedActorsOfType = pescortData->GetNumEscortedActorsOfType(apActor, ptype);
				inewFollowersNeeded -= NumEscortedActorsOfType;
			}
			for (int32_t i = 0;; ++i) {
				if (i >= inewFollowersNeeded) {
					goto LABEL_28;
				}
				TESObjectREFR* EscortTargetForPackage = ppack->GetEscortTargetForPackage(apActor, 1);
				SetTarget(EscortTargetForPackage);
				if (!pTarget) {
					break;
				}
				if (pTarget->IsActor()) {
					if (pTarget == pPlayer) {
						bescortIncludesPlayer = 1;
						--v26;
					} else {
						CreateFollowforEscort(apActor, ppack, 0);
						EscortActorPackageData* pescortData_3 =
							static_cast<EscortActorPackageData*>(GetInstanceDataThatIsRunning());
						ActorPackageData* pdata = pescortData_3;
						Actor* pescortedActor = static_cast<Actor*>(pTarget);
						pescortData_3->AddEscortedActor(pescortedActor);
					}
				}
			}
			if (v9) {
				if (bescortIncludesPlayer) {
					SetTarget(pPlayer);
				}
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
			LABEL_38:;
				AddToProcedureIndexRunning(apActor, 1);
				return;
			}
		LABEL_28:;
			if (bescortIncludesPlayer) {
				SetTarget(pPlayer);
			}
			if (v26 > 0) {
				int32_t v4 = pescortData->GetNumEscortedActorsOfType(apActor, ptype);
				v26 -= v4;
			}
			if (GetRunOncePackage()) {
				pactorPack = &RunOncePackage;
			} else {
				pactorPack = &CurrentPackage;
			}
			double Hour = appCalendar.GetHour();
			float v5 = Hour - pactorPack->GetPackageStartTime();
			float ftime = std::fabs(v5);
			if (ftime >= 0.15000000596046448 || (pTarget && v26 <= 0)) {
				goto LABEL_38;
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessSurface(Actor* apActor) {
	if (apActor) {
		if (apActor->IsNotOutOfBreath()) {
			ClearRunOncePackage();
			apActor->StopMoving();
		} else {
			if (apActor->IsPathingComplete()) {
				float fsurfaceGoalRadius = 25.0f;
				NiPoint3 surfacepos = apActor->GetLocationOnReference();
				surfacepos.z = apActor->GetRelevantWaterHeight() + 25.0f;
				FNVD_CONSTRUCTED_LOCAL(PathingRequest, request, &PathingRequest::Destruct);
				float afRadius = fsurfaceGoalRadius;
				TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
				apActor->BuildRequest(&request, surfacepos, apActor->GetParentCell(), WorldSpace, afRadius, 0);
				request.SetGoalZDelta(fsurfaceGoalRadius);
				request.SetGoDirectIfPathingFails(1);
				apActor->SetPathfindingGoal(&request);
			}
		}
	}
}

// KEEP: the exe copies both smart pointers into BuildPath's argument slots (their IncRefCount calls, 0x6EBF30 and
// 0x559A40) before it calls PathManager::QInstance (0x47D0B0, 0x8DF71C); C++17 evaluates a member call's object
// expression before its arguments, so src calls QInstance first. The exe's cleanup-only C++ EH frame is left out.
bool HighProcess::ShouldWaitForEscortTarget(Actor* apActor, Actor* apEscortedActor, PathingLocation& goalLoc,
	bool* abSlowDownOut) {
	FNVD_INLINED_LOCAL(ActorPathingMessage, v46, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(ActorPathingMessage, apThis_8, std::nullopt, std::nullopt);
	PathingLocation apThis_16;
	PathingLocation escortedActorLoc;
	float fwaitdistance;
	TESPackage* pPack = GetPackageThatIsRunning();
	// The exe takes the escorted actor's location first, then apActor's, before the subtraction.
	const NiPoint3& rEscortedLoc = apEscortedActor->GetLocationOnReference();
	NiPoint3 deltameToEscortedActor = apActor->GetLocationOnReference() - rEscortedLoc;
	int32_t ipackdist = pPack->GetEscortFollowDistance();
	if (ipackdist <= 200) {
		ipackdist = 200;
	}
	if (this->pTarget == pPlayer) {
		fwaitdistance = static_cast<float>(ipackdist);
	} else {
		if (apActor->GetParentCell()->GetInterior()) {
			fwaitdistance = fAIEscortWaitDistanceInterior.Float();
		} else {
			double v31 = static_cast<double>(ipackdist);
			fwaitdistance = fAIEscortWaitDistanceExterior.Float() * v31;
		}
	}
	if (apActor->IsPathing()) {
		float* v6 = &fAIEscortHysteresisWidth.Float();
		fwaitdistance = fwaitdistance + *v6;
	}
	float fdistSqrMeToEscortedActor_2 = deltameToEscortedActor.SqrLength();
	float fdistSqrWait = fwaitdistance * fwaitdistance;
	if (abSlowDownOut) {
		float fdistSqrSlowDown = fdistSqrWait * 0.44444000720977783;
		*abSlowDownOut = fdistSqrSlowDown < static_cast<double>(fdistSqrMeToEscortedActor_2);
	}
	if (!(fdistSqrWait < static_cast<double>(fdistSqrMeToEscortedActor_2))) {
		return 0;
	}
	if (abSlowDownOut) {
		*abSlowDownOut = 0;
	}
	TESObjectCELL* pgoalCell = goalLoc.GetCell();
	TESWorldSpace* pgoalSpace = goalLoc.pWorldSpace;
	apThis_16.~PathingLocation();
	::new (&apThis_16) PathingLocation(apActor);
	escortedActorLoc.~PathingLocation();
	::new (&escortedActorLoc) PathingLocation(apEscortedActor);
	if (this->spMeToGoalRequest.m_pObject && this->spEscortedToGoalRequest.m_pObject) {
		::new (&apThis_8) ActorPathingMessage;
		::new (&v46) ActorPathingMessage;
		ActorPathingMessageQueue* v29 = this->spMeToGoalMessageQueue.m_pObject;
		if (v29->TryPop(&apThis_8)) {
			ActorPathingMessageQueue* v28 = this->spEscortedToGoalMessageQueue.m_pObject;
			if (v28->TryPop(&v46)) {
				this->spMeToGoalRequest.operator=(static_cast<PathingRequest*>(0));
				this->spEscortedToGoalRequest.operator=(static_cast<PathingRequest*>(0));
				this->spMeToGoalMessageQueue.operator=(static_cast<ActorPathingMessageQueue*>(0));
				this->spEscortedToGoalMessageQueue.operator=(static_cast<ActorPathingMessageQueue*>(0));
				if (apThis_8.spSolution.m_pObject) {
					if (v46.spSolution.m_pObject) {
						float distMeToGoal = apThis_8.spSolution.m_pObject->ComputeVirtualPathLength();
						float distEscortedActorToGoal = v46.spSolution.m_pObject->ComputeVirtualPathLength();
						if (distEscortedActorToGoal > static_cast<double>(distMeToGoal)) {
							v46.~ActorPathingMessage();
							apThis_8.~ActorPathingMessage();
							return 1;
						}
					}
				}
			} else {
				ActorPathingMessageQueue* v27 = this->spMeToGoalMessageQueue.m_pObject;
				v27->Push(&apThis_8);
			}
		}
		v46.~ActorPathingMessage();
		apThis_8.~ActorPathingMessage();
	} else {
		PathingRequest* v44 = static_cast<PathingRequest*>(NiMemObject::operator new(sizeof(PathingRequest)));
		PathingRequest* v26;
		if (v44) {
			v26 = ::new (v44) PathingRequest;
		} else {
			v26 = 0;
		}
		this->spMeToGoalRequest.operator=(v26);
		this->spMeToGoalRequest.m_pObject->Initialize(apActor);
		this->spMeToGoalRequest.m_pObject->SetDestination(goalLoc);
		void* apMemory = NiMemObject::operator new(sizeof(ActorPathingMessageQueue));
		ActorPathingMessageQueue* v25;
		if (apMemory) {
			v25 = ::new (apMemory) ActorPathingMessageQueue;
		} else {
			v25 = 0;
		}
		this->spMeToGoalMessageQueue.operator=(v25);
		// Both smart pointers go by value: the exe copies them into the argument slots (queue first) and BuildPath
		// releases them.
		PathManager::QInstance()->BuildPath(this->spMeToGoalRequest, this->spMeToGoalMessageQueue, true);
		PathingRequest* v38 = static_cast<PathingRequest*>(NiMemObject::operator new(sizeof(PathingRequest)));
		PathingRequest* v20;
		if (v38) {
			v20 = ::new (v38) PathingRequest;
		} else {
			v20 = 0;
		}
		this->spEscortedToGoalRequest.operator=(v20);
		this->spEscortedToGoalRequest.m_pObject->Initialize(apEscortedActor);
		this->spEscortedToGoalRequest.m_pObject->SetDestination(goalLoc);
		ActorPathingMessageQueue* v36 =
			static_cast<ActorPathingMessageQueue*>(NiMemObject::operator new(sizeof(ActorPathingMessageQueue)));
		ActorPathingMessageQueue* v19;
		if (v36) {
			v19 = ::new (v36) ActorPathingMessageQueue;
		} else {
			v19 = 0;
		}
		this->spEscortedToGoalMessageQueue.operator=(v19);
		PathManager::QInstance()->BuildPath(this->spEscortedToGoalRequest, this->spEscortedToGoalMessageQueue, true);
	}
	bool v33 = !apActor->IsPathing();
	return v33;
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::DetectDoorsForPathing(Actor* apActor) {
	NiPoint3 v7;
	NiPoint3 v8;
	NiPoint3 v9;
	NiPoint3 dst;
	NiPoint3 apResult;
	FNVD_INLINED_LOCAL(hkpAllCdBodyPairCollector, collector, std::nullopt, std::nullopt);
	NiTransform actorToWorld;
	NiTransform pextenttemp;
	NiTransform boxToActor;
	fDetectDoorForPathingTimer = fDetectDoorForPathingTimer - appTimer.fDelta;
	if (!(fDetectDoorForPathingTimer > 0.0)) {
		bhkShapePhantom* v5;
		NiTransform boxToWorld__boxToActor;
		NiPoint3 playerextents;
		float* v2 = &fINIDetectDoorsForPathingTime.Float();
		fDetectDoorForPathingTimer = RandomFloat(0.8f, 1.2f) * *v2;
		TESObjectREFR* pfoundDoor = 0;
		if (!apActor->pCurrentProcess->GetBSBound() || !apActor->Get3D()) {
			goto LABEL_21;
		}
		if (!spShapePhantom.m_pObject || (v5 = spShapePhantom.m_pObject, v5->GethkWorld())) {
			bhkBoxShape* pplayerShape;
			uint32_t imoveMode;
			int32_t ihits;
			pextenttemp.~NiTransform();
			::new (&pextenttemp) NiTransform;
			boxToActor.~NiTransform();
			::new (&boxToActor) NiTransform;
			pplayerShape = static_cast<bhkBoxShape*>(spShapePhantom.m_pObject->GetShape());
			playerextents = *pplayerShape->GetHalfExtents(&apResult);
			imoveMode = apActor->GetMoveMode();
			if ((imoveMode & 2) != 0) {
				boxToActor.m_Translate = *::new (&dst) NiPoint3(0.0f, -playerextents.y, playerextents.z);
				boxToActor.m_Rotate.MakeZRotation(3.1415927f);
			} else {
				if ((imoveMode & 4) != 0) {
					boxToActor.m_Translate = *::new (&v9) NiPoint3(-playerextents.y, 0.0f, playerextents.z);
					boxToActor.m_Rotate.MakeZRotation(1.5707964f);
				} else {
					if ((imoveMode & 8) != 0) {
						boxToActor.m_Translate = *::new (&v8) NiPoint3(playerextents.y, 0.0f, playerextents.z);
						boxToActor.m_Rotate.MakeZRotation(-1.5707964f);
					} else {
						boxToActor.m_Translate = *::new (&v7) NiPoint3(0.0f, playerextents.y, playerextents.z);
						boxToActor.m_Rotate = NiMatrix3_IDENTITY;
					}
				}
			}
			actorToWorld = apActor->Get3D()->GetWorldTransform();
			::new (&boxToWorld__boxToActor) NiTransform(actorToWorld * boxToActor);
			spShapePhantom.m_pObject->SetTransform(boxToWorld__boxToActor);
			::new (&collector) hkpAllCdBodyPairCollector;
			collector.hkpAllCdBodyPairCollector::reset();
			spShapePhantom.m_pObject->GetPenetrations(&collector);
			ihits = (&collector.m_hits)->m_size;
			for (int32_t I = 0; I < ihits; ++I) {
				const hkpCollidable* pcol = (&(&collector.m_hits)->operator[]<hkpRootCdBodyPair>(I))->m_rootCollidableB;
				NiAVObject* pavobj = bhkUtilFunctions::GetNiAVObject(pcol);
				TESObjectREFR* pTemp = TESObjectREFR::FindReferenceFor3D(pavobj);
				if (pTemp) {
					if (pTemp->GetObjectReference()->GetFormType() == DOOR_ID && !pTemp->GetTeleport()) {
						pfoundDoor = pTemp;
					}
				}
			}
			collector.~hkpAllCdBodyPairCollector();
		LABEL_21:;
			apActor->SetPathingDoorReference(pfoundDoor);
		}
	}
}

// The parent room's multibound shape is visible when the culler's verdict for it is BS_CULL_VISIBLE.
void HighProcess::EvaluateBoneLod(MobileObject* apRef) {
	float actorHeight = apRef->GetScale();
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	const NiPoint3* Translate = &WorldSceneGraph->GetCameraRoot()->GetTranslate();
	const NiPoint3* v3 = &apRef->GetLocationOnReference();
	NiPoint3 delta = (*v3 - *Translate) / actorHeight;
	float actorHeighta = delta.Length();
	double distance = std::fabs(actorHeighta) * 12.0;
	SceneGraph* v5 = Main::GetWorldSceneGraph();
	double fScaledLODAdjust = v5->GetCamera()->m_fLODAdjust * distance;
	double iBoneLODDistMult = static_cast<double>(::iBoneLODDistMult.Int());
	int32_t ilodIndex =
		static_cast<int32_t>(fScaledLODAdjust / (BSFadeNode::QFadeOutMultiplier(LOD_MULT_ACTORS) * iBoneLODDistMult));
	int32_t ConversationData = this->GetBoneLOD()->m_uiNumLODs;
	if (ilodIndex < ConversationData) {
		NiNode* v7;
		if (apRef->Get3D() &&
			((v7 = apRef->Get3D(), static_cast<uint8_t>(v7->GetNotVisible())) && !bDialoguewithPlayer)) {
			ilodIndex = -1;
		} else {
			if (apRef != pPlayer) {
				bool bvisible = 1;
				NiNode* pparent = apRef->Get3D()->m_pkParent;
				if (pparent && NiObject::IsExactKindOf(&BSMultiBoundRoom::ms_RTTI, pparent)) {
					BSMultiBound* pMultiBound = static_cast<BSMultiBoundRoom*>(pparent)->QMultiBound();
					bvisible = pMultiBound->QShape()->kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE;
				} else {
					if (pparent) {
						if (NiObject::IsExactKindOf(&BSPortalSharedNode::ms_RTTI, pparent)) {
							BSPortal* pPortal = static_cast<BSPortalSharedNode*>(pparent)->pPortal;
							bvisible = pPortal->AnyRoomsVisible() == 1;
						}
					}
				}
				if (!bvisible && !bDialoguewithPlayer) {
					ilodIndex = -1;
				}
			}
		}
	} else {
		ilodIndex = -1;
	}
	if (ilodIndex != iLastBoneLOD) {
		this->GetBoneLOD()->SetBoneLOD(ilodIndex);
		BSFaceGenNiNode* pfacenode = GetFaceNode();
		BSFaceGenNiNode* pfacenodeSkinned = GetFaceSkinnedNode();
		NiTriShape* panimFace = static_cast<NiTriShape*>(GetHeadAnims());
		if (panimFace) {
			if (pfacenode) {
				if (pfacenodeSkinned) {
					bool bfacegenLOD = panimFace->m_pkParent->GetSelectiveUpdate();
					pfacenode->SetSelectiveUpdate(bfacegenLOD);
					pfacenodeSkinned->SetSelectiveUpdate(bfacegenLOD);
				}
			}
		}
		iLastBoneLOD = ilodIndex;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessBackUp(Actor* apActor) {
	FNVD_INLINED_LOCAL(PathingRequest, request_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequest, request, std::nullopt, std::nullopt);
	NiPoint3 vDestination;
	TESPackage* pPack = GetPackageThatIsRunning();
	BackUpPackage* pBackUpPack = dynamic_cast<BackUpPackage*>(pPack);
	bool bIgnoreDistance = 0;
	if (pPack) {
		if (pBackUpPack) {
			PACKAGE_TYPE epackType = pPack->GetPackType();
			Animation* panim = apActor->GetAnimation();
			if (!GetPackageIdleDone() || (!panim || (!panim->SpecialIdleWorking() && !panim->SpecialIdlePlaying()))) {
				uint8_t bfail;
				bool v22 = epackType != PACKAGE_TYPE_SANDBOX && epackType != PACKAGE_TYPE_WANDER;
				pBackUpPack->GetBackUpPoint(&vDestination);
				float fcurrentdistance = apActor->GetDistanceFromPoint(vDestination);
				bool batRef = 0;
				if (apActor->IsPathingComplete() || apActor == pPlayer) {
					batRef = apActor->IsAtPoint(vDestination, 75.0f, 1, 0);
				}
				float fdistance = pPack->GetPackageRadiusActorToLocation(apActor, 0);
				if ((!batRef || !apActor->IsPathingComplete()) && !DidPathingFail(apActor)) {
					goto LABEL_87;
				}
				fPackageIdleTimer = 0.0f;
				bfail = DidPathingFail(apActor);
				EndMoveMessage(apActor);
				AddToProcedureIndexRunning(apActor, 1);
				if (pPack->GetMustReachLocation() && !pPack->GetOncePerDay() && !pPack->GetPackageTarget()) {
					fPackageEvalTimer = 0.0f;
				}
				if (pPack->GetOncePerDay() &&
					(!pPack->GetPackageSchedule() || !pPack->GetPackageSchedule()->PSData.iDuration) &&
					pPack->GetPackType() == PACKAGE_TYPE_TRAVEL) {
					char Day = appCalendar.GetDay();
					apActor->SetRunOnceExtraPackage(pPack, Day);
				}
				if (!GetLockedLocation()) {
					if (pPack->GetLockDoorsAtLocation()) {
						const NiPoint3* v16 = &apActor->GetLocationOnReference();
						const NiPoint3* v14 = &apActor->GetLocationOnReference();
						TESObjectCELL* ParentCell = apActor->GetParentCell();
						pDataHandler->EnumReferencesCloseToPoint(ParentCell, *v14, 5000.0f, *v16, 5000.0f,
							&LowProcess::LockDoorsatLocation, apActor);
					}
					SetLockedLocation(1);
				}
				if (pPack->GetUnlockDoorsAtLocation()) {
					const NiPoint3* v17 = &apActor->GetLocationOnReference();
					const NiPoint3* v15 = &apActor->GetLocationOnReference();
					TESObjectCELL* v13 = apActor->GetParentCell();
					pDataHandler->EnumReferencesCloseToPoint(v13, *v15, 5000.0f, *v17, 5000.0f,
						&LowProcess::UnLockDoorsatLocation, apActor);
				}
				if (DidPathingFail(apActor)) {
				LABEL_87:;
					if (batRef && apActor->IsPathingComplete()) {
						AddToProcedureIndexRunning(apActor, 1);
						return;
					}
					if (!batRef &&
						(apActor->IsPathingComplete() &&
							(!pPack->IsActorAtLocation(apActor, bIgnoreDistance, -1.0f, 0) &&
								!DidPathingFail(apActor)))) {
						if (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
							GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
							apActor->InitiateGetUpPackage();
							return;
						}
						if (apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
							::new (&request) PathingRequest;
							TESWorldSpace* afRadius2 = pPack->GetLocationWorld(apActor);
							TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
							apActor->BuildRequest(&request, vDestination, LocationCell, afRadius2, fdistance, 0);
							apActor->ForceMoveMode(514u);
							request.SetSoftGoalRadius(0);
							if (!apActor->SetPathfindingGoal(&request)) {
								request.~PathingRequest();
								return;
							}
							request.~PathingRequest();
						}
					}
					if (DidPathingFail(apActor) && (!pPack->GetMustReachLocation() && !pPack->GetMustComplete())) {
						PACKAGE_PROCEDURE_TYPE Count = pPack->ePROCEDURE_TYPE;
						int32_t ProcedureCount = Procedure::GetProcedureCount(Count);
						SetProcedureIndexRunning(ProcedureCount - 1);
						return;
					}
					if (!batRef && apActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
						float v30 = 0.0f;
						Actor* plocactor = 0;
						if (pPack->pPackLoc) {
							TESObjectREFR* plocref = pPack->pPackLoc->GetLocReference();
							if (!plocref) {
								plocref = GetGenericLocation();
							}
							if (plocref) {
								if (plocref->IsActor()) {
									plocactor = static_cast<Actor*>(plocref);
								}
							}
						}
						if (plocactor) {
							const NiPoint3* v6 = &plocactor->GetLocationOnReference();
							float fdistancemoved = (lastSeenPostion - *v6).Length();
							float* v8 = &fAIMoveDistanceToRecalcFollowPath.Float();
							if (*v8 < static_cast<double>(fdistancemoved) ||
								(fdistance < static_cast<double>(fcurrentdistance) && apActor->IsPathingComplete())) {
								::new (&request_2) PathingRequest;
								TESWorldSpace* WorldSpace = plocactor->GetWorldSpace();
								apActor->BuildRequest(&request_2, vDestination, plocactor->GetParentCell(), WorldSpace,
									fdistance, 0);
								request_2.SetGoalAngleToFace(0.0f);
								request_2.SetSoftGoalRadius(0);
								apActor->ForceMoveMode(514u);
								if (!apActor->SetPathfindingGoal(&request_2)) {
									request_2.~PathingRequest();
									return;
								}
								lastSeenPostion = plocactor->GetLocationOnReference();
								request_2.~PathingRequest();
							}
						}
						if (apActor != pPlayer) {
							int32_t emovemode = -1;
							if (pPack->GetAlwaysRun()) {
								emovemode = 513;
							} else {
								if (emovemode == -1) {
									bool bflag = 0;
									if (GetCurrentPackage()) {
										TESPackage* v11;
										TESPackage* v10 = GetCurrentPackage();
										if (v10->GetPackType() == PACKAGE_TYPE_ALARM ||
											(v11 = GetCurrentPackage(), v11->GetPackType() == PACKAGE_TYPE_COMBAT)) {
											bflag = 1;
										}
									}
									float v21 = fdistance + fdistance;
									emovemode = CalculateMoveMode(apActor, fcurrentdistance, fdistance, v21, bflag, 1);
								}
							}
							SetActorsAnimation(apActor, emovemode, 1);
							NiPoint3 entryPoint = pPack->GetLocationCoord(apActor);
						}
					}
					if (!batRef || apActor->IsPathing()) {
						if (!batRef &&
							(apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
								GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
							apActor->InitiateGetUpPackage();
						}
					} else {
						AddToProcedureIndexRunning(apActor, 1);
					}
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::ProcessSurfaceOneHour(Actor* apActor) {
	if (apActor) {
		float fsurfaceGoalRadius = 25.0f;
		NiPoint3 surfacepos = apActor->GetLocationOnReference();
		surfacepos.z = apActor->GetRelevantWaterHeight() + 25.0f;
		FNVD_CONSTRUCTED_LOCAL(PathingRequest, request, &PathingRequest::Destruct);
		float afRadius = fsurfaceGoalRadius;
		TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
		apActor->BuildRequest(&request, surfacepos, apActor->GetParentCell(), WorldSpace, afRadius, 0);
		request.SetGoalZDelta(fsurfaceGoalRadius);
		request.SetGoDirectIfPathingFails(1);
		apActor->SetPathfindingGoal(&request);
		ClearRunOncePackage();
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool HighProcess::ProcessWanderOneHour(Actor* apActor, float afTimeDelta) {
	// The exe builds it with PathingLocation's constructor (0x6DCE60) and destroys it with 0x4FF7E0: no C++
	// constructor storing src's vtable.
	FNVD_INLINED_LOCAL(PathingLocation, result_1, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingRequestClosePoint, v11, std::nullopt, std::nullopt);
	NiPoint3 apMemory_192;
	if (!apActor->GetParentCell()) {
		return 0;
	}
	TESPackage* apMemory_188 = 0;
	apMemory_188 = GetPackageThatIsRunning();
	if (!apMemory_188) {
		return 0;
	}
	if (apMemory_188->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_WANDER) {
		AddToProcedureIndexRunning(apActor, -1);
	}
	::new (&apMemory_192) NiPoint3(apMemory_188->GetLocationCoord(apActor));
	TESObjectCELL* apMemory_204 = apActor->GetParentCell();
	if (!apMemory_204) {
		TESWorldSpace* WorldSpace = apActor->GetWorldSpace();
		apMemory_204 = pDataHandler->GetCellFromWorldCoord(apMemory_192.x, apMemory_192.y, WorldSpace, 0);
	}
	::new (&v11) PathingRequestClosePoint;
	PathingLocation* apOrigin = ::new (&result_1) PathingLocation(&apMemory_192, apMemory_204);
	v11.SetOrigin(*apOrigin);
	result_1.~PathingLocation();
	v11.SetMinDistance(32.0f);
	float a2_4 = static_cast<float>(apMemory_188->GetLocationRadius(apActor));
	v11.SetMaxDistance(a2_4);
	Pathing::FindClosePoint(&v11, &apMemory_192);
	if (apActor->GetSitSleepState()) {
		ProcessGetUpOneHour(apActor);
	}
	TESWorldSpace* v5 = apActor->GetWorldSpace();
	apActor->SetPathfindingGoal(apMemory_192, apActor->GetParentCell(), v5, 100.0f, 0);
	if (Interface::GetConsoleSelection() == apActor) {
		char Format[308];
		std::sprintf(Format, "%s is wandering to point x %.02f and y %.02f", apActor->GetFullName(), apMemory_192.x,
			apMemory_192.y);
		Interface::PrintLine(static_cast<const char*>(Format));
	}
	v11.~PathingRequestClosePoint();
	return 0;
}

bool HighProcess::IsGroupAlreadyInList(CombatGroup* pgroup) {
	bool bflag = 0;
	for (BSSimpleList<StartCombatStates*>* plist = &GroupsToHelpList; plist && (plist->m_item && !bflag);
		plist = plist->m_pkNext) {
		bflag = pgroup == plist->m_item->pGroup;
	}
	return bflag;
}

bool HighProcess::AimAtTarget(Actor* apActor, TESObjectREFR* apTarget, bool abHighArc) {
	NiPoint3* AttackerFiringLocation;
	BGSProjectile* pProjectile;
	NiPoint3 v14;
	NiPoint3 result;
	NiPoint3 difference;
	if (!apTarget) {
		return 0;
	}
	NiPoint3 a2 = NiPoint3_ZERO;
	NiPoint3 v24 = NiPoint3_ZERO;
	TESObjectWEAP* weapon = apActor->GetCurrentWeapon();
	if (weapon) {
		pProjectile = weapon->GetProjectile(0);
	} else {
		pProjectile = 0;
	}
	if (weapon) {
		AttackerFiringLocation = &(result = weapon->CalculateReleasePoint(apActor));
	} else {
		AttackerFiringLocation = &(v14 = CombatUtilities::GetAttackerFiringLocation(apActor));
	}
	v24 = *AttackerFiringLocation;
	if (pProjectile) {
		float v8;
		NiPoint3 arResult;
		if (abHighArc) {
			v8 = gs_fGrenadeHighArcSpeedPercentage.Float();
		} else {
			v8 = 1.0;
		}
		float fProjectileSpeed = pProjectile->GetSpeed() * v8;
		a2 = (arResult = CombatUtilities::GetProjectedPointForTarget(apActor, apTarget, nullptr, pProjectile,
				  fProjectileSpeed, abHighArc));
	} else {
		NiPoint3 akPos;
		a2 = (akPos = CombatUtilities::GetTargetFiringLocation(apTarget, ACTOR_SEGMENT_INVIEW_MIDDLE));
	}
	apActor->SetLookAtTarget(a2);
	bAimingTarget = 1;
	if (!weapon || weapon->IsMeleeWeapon()) {
		return CombatUtilities::IsTargetWithinStrikeAngle(apActor, apTarget, nullptr);
	}
	float* v5 = &fCombatIronSightsDistance.Float();
	float sightUsage = weapon->GetIronSightUseMult() * *v5;
	const NiPoint3* v6 = &apActor->GetLocationOnReference();
	difference = *v6 - a2;
	double v7 = difference.SqrLength();
	bool v19 = sightUsage * sightUsage < v7;
	apActor->SetIronSights(v19, 0, 0);
	return CombatUtilities::IsTargetWithinFiringArc(apActor, a2, 0.08726646f, 0.08726646f);
}

void HighProcess::CleanUpAfterProcessUseWeapon(Actor* apActor) {
	TESPackage* pPack = GetPackageThatIsRunning();
	const TESUseWeaponPackageData* ppackData = pPack->GetUseWeaponPackageData();
	if (!ppackData->bRepeatFire) {
		int32_t sBurstsFired = this->sBurstsFired;
		if (sBurstsFired >= ppackData->sBurstCount) {
			if (GetCurrentWeapon()) {
				TESObjectWEAP* pweap = 0;
				TESBoundObject* pweapBound = GetCurrentWeapon()->pContainerObj;
				if (pweapBound) {
					if (pweapBound->GetFormType() == WEAP_ID) {
						pweap = static_cast<TESObjectWEAP*>(GetCurrentWeapon()->pContainerObj);
					}
				}
				if (pweap) {
					if (pweap->IsThrownWeapon()) {
						apActor->SetWantWeaponDrawn(0);
						apActor->QueueUnEquipObject(pweapBound, 1, 0, 0, 0, 1);
					}
				}
			}
		}
	}
	apActor->ClearLookAtTarget();
	apActor->SetLooking(0.0f);
	apActor->SetIronSights(0, 0, 0);
	apActor->SetForceSneak(0);
	FreeUpSpecialIdle(apActor);
	ClearShootingAction(apActor);
	ClearUseWeaponHeadTrackTarget(1);
	if (!apActor->IsPathingComplete()) {
		EndMoveMessage(apActor);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool HighProcess::ObserveCombat(Actor* apActor, Actor* apTarget) {
	StartCombatStates* a2;
	PlayerCharacter* v10 = static_cast<PlayerCharacter*>(apTarget->GetCombatTarget());
	FIGHT_REACTION a4 = FIGHT_REACTION_NEUTRAL;
	if (apActor->GetShouldHelp(apTarget) || (v10 && apActor->GetShouldAttackActor(v10, 0, a4, 0))) {
		return 0;
	}
	if ((apTarget == pPlayer || (apTarget->GetCombatController() && apTarget->GetCombatTarget())) &&
		!apActor->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_OBSERVE_COMBAT_BEHAVIOR) && !apActor->IsAlarmed() &&
		!apActor->IsInCombat()) {
		double DistanceFromReference = apTarget->GetDistanceFromReference(apActor, 0, 1);
		if (fAIExteriorSpectatorDetection.Float() >= DistanceFromReference) {
			StartCombatStates* started;
			StartCombatStates* HighProcess__UnkCombatTarget =
				static_cast<StartCombatStates*>(ourMalloc(sizeof(StartCombatStates)));
			if (HighProcess__UnkCombatTarget) {
				started = ::new (HighProcess__UnkCombatTarget) StartCombatStates;
			} else {
				started = 0;
			}
			a2 = started;
			started->pTarget = apTarget;
			a2->pGroup = apTarget->GetCombatGroup();
			a2->bSpectatorOfCombat = 1;
			SpectatorList.AddHead(a2);
		}
	}
	return 0;
}

void HighProcess::ProcessGuardWaitToAttack(Actor* apActor, GuardActorPackageData* apData) {
	char v27 = 0;
	Actor* pwarnedActor = apData->pGuardWarnedActor;
	if (pGreetTopic) {
		v27 = 1;
		if (apActor->IsImmobile()) {
			if (pwarnedActor) {
				ProcessGreet(pwarnedActor, 0, 0, 0, 0, 0);
			}
		}
	}
	TESObjectREFR* pguardedRef = GetTarget();
	if (!pguardedRef ||
		(!pwarnedActor ||
			(pwarnedActor->IsDead(0) ||
				(pwarnedActor->IsDying() ||
					(pwarnedActor->IsKnockedOut() ||
						(pwarnedActor->IsParalyzed() || pwarnedActor->IsUnConscious())))))) {
		apData->eState = GuardActorPackageData::GUARD_STATE_INIT;
		ProcessGuardStartTravelAndReset(apActor, apData);
	} else {
		TESObjectREFR* plookAt;
		TESPackage* v3 = GetPackageThatIsRunning();
		float FormFlags = static_cast<float>(v3->GetPackageTarget()->iValue);
		float v32 = FormFlags * fGuardPackageAttackRadiusMult.Float();
		float v31 = FormFlags * FormFlags;
		v32 = v32 * v32;
		char v29 = 0;
		bool v33 = 0;
		const NiPoint3* v9 = &pguardedRef->GetLocationOnReference();
		const NiPoint3* v4 = &pwarnedActor->GetLocationOnReference();
		float v34 = (*v4 - *v9).SqrLength();
		if (v31 > static_cast<double>(v34)) {
			v29 = 1;
			v33 = v32 > static_cast<double>(v34);
		}
		if (v29) {
			plookAt = (TESObjectREFR*)pwarnedActor;
		} else {
			plookAt = pguardedRef;
		}
		SetActionHeadTrackTarget(plookAt);
		if (v29) {
			AimAtTarget(apActor, pwarnedActor, 0);
		} else {
			apActor->ClearLookAtTarget();
		}
		if (v29) {
			Animation* panim = GetAnimation();
			float* v6 = &fAIIdleWaitTime.Float();
			SetIdleTimer(*v6);
			if (!panim->SpecialIdleDonePlaying()) {
				panim->SpecialIdleFree(1, 0);
			}
			bWeaponAlertDrawn = 1;
			if (!apActor->IsWeaponDrawn()) {
				apActor->SetWantWeaponDrawn(1);
			}
			if (apActor->GetCurrentWeapon()) {
				if (apActor->IsWeaponDrawn()) {
					TESObjectWEAP* CurrentWeapon = apActor->GetCurrentWeapon();
					if (CurrentWeapon->IsGunWeapon()) {
						SetIronSights(1);
					}
				}
			}
			SetUseWeaponHeadTrackTarget(plookAt);
			if (!v27) {
				float Seconds = appTimer.fDelta;
				apData->UpdateWarningTimer(Seconds);
				bool battack = v33;
				bool bwarn = 0;
				if (!v33) {
					double PackageStartTime = apData->fGuardTimer;
					if (fWarningTimer.Float() <= PackageStartTime) {
						TESPackage* ppack = GetPackageThatIsRunning();
						const GuardPackageData* pdata;
						if (ppack) {
							pdata = ppack->GetGuardPackageData();
						} else {
							pdata = 0;
						}
						int32_t imaxWarningCount;
						if (pdata) {
							// 0x44DDC0, the exe's shared one-field getter: GuardPackageData::GetWarningMaxCount.
							imaxWarningCount = pdata->iWarningMaxCount;
						} else {
							imaxWarningCount = 2;
						}
						int32_t CastingType = apData->iWarningCount;
						battack = CastingType > imaxWarningCount;
						bwarn = CastingType <= imaxWarningCount;
					}
				}
				if (battack) {
					if (EnterCombat(apActor, apData->pGuardWarnedActor, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0)) {
						ActorPackageData* pactorPackageData = CurrentPackage.pData;
						if (pactorPackageData == apData) {
							apData->eState = GuardActorPackageData::GUARD_STATE_INIT;
						}
					}
				} else {
					if (bwarn) {
						SayGuardWarning(apActor, apData);
					}
				}
			}
		} else {
			SetIronSights(0);
			TESPackage* ppack_2 = GetPackageThatIsRunning();
			if (!ppack_2 || !ppack_2->GetDrawWeaponFlag()) {
				apActor->SetWantWeaponDrawn(0);
			}
			apData->eState = GuardActorPackageData::GUARD_STATE_IDLE;
			apData->ClearWarningTimer();
			apData->ClearWarningCount();
			ClearUseWeaponHeadTrackTarget(1);
		}
	}
}

void HighProcess::FireAtObject(Actor* apActor, TESObjectWEAP* apWeapon) {
	if (CanAttack()) {
		bool battack = 0;
		Animation* panim = apActor->GetAnimation();
		if (apWeapon->IsAutomaticWeapon()) {
			bool v7 = 1;
			uint8_t cshotsToFire = GetAutomaticShotsToFire(apActor);
			uint16_t Group = panim->GetGroup(AGS_WEAPON);
			if (TESAnimGroup::IsAttackSpinAction(Group)) {
				v7 = panim->GetGroupAction(AGS_WEAPON) == AGA_STOP;
			}
			if (v7) {
				if (cshotsToFire) {
					SetShotsToFire(cshotsToFire);
					battack = 1;
				}
			}
		} else {
			ANIMATION_ACTION v5 = GetAnimAction();
			if (v5 == ANIM_ACTION_NONE || v5 == ANIM_ACTION_ATTACK_FOLLOW_THROUGH) {
				battack = 1;
			}
		}
		if (battack) {
			ANIM_GROUP_ENUM eattack_2;
			if (IsCurrentWeaponGrenade() || IsCurrentWeaponThrown()) {
				eattack_2 = ANIM_GROUP_ATTACK_THROW;
			} else {
				if (IsCurrentWeaponMine()) {
					eattack_2 = ANIM_GROUP_POWER_ATTACK_STOP;
				} else {
					switch (panim->FindSwitchKey(0)) {
					case 51: {
						eattack_2 = ANIM_GROUP_ATTACK_3;
						break;
					}
					case 52: {
						eattack_2 = ANIM_GROUP_ATTACK_4;
						break;
					}
					case 53: {
						eattack_2 = ANIM_GROUP_ATTACK_5;
						break;
					}
					case 54: {
						eattack_2 = ANIM_GROUP_ATTACK_6;
						break;
					}
					case 55: {
						eattack_2 = ANIM_GROUP_ATTACK_7;
						break;
					}
					case 56: {
						eattack_2 = ANIM_GROUP_ATTACK_8;
						break;
					}
					case 108: {
						eattack_2 = ANIM_GROUP_ATTACK_LEFT;
						break;
					}
					default: {
						if (!apWeapon || apWeapon->GetAttackAnim() == ANIM_GROUP_NONE) {
							eattack_2 = ANIM_GROUP_ATTACK_RIGHT;
						} else {
							eattack_2 = apWeapon->GetAttackAnim();
						}
						break;
					}
					}
				}
			}
			if (apActor->QueueAttack(eattack_2)) {
				bool IsAutomaticWeapon = apWeapon->IsAutomaticWeapon();
				SetSustainedFire(IsAutomaticWeapon);
			}
		}
	}
}

void HighProcess::FreeAvoidNodes() {
	if (this->pListOfAvoidAreas) {
		BSSimpleList<AvoidAreaStruct*>* ecx0 = this->pListOfAvoidAreas;
		while (ecx0 && ecx0->m_item) {
			AvoidAreaStruct* m_item = ecx0->m_item;
			ecx0 = ecx0->m_pkNext;
			ourFree(m_item);
		}
		this->pListOfAvoidAreas->RemoveAll();
		void* pListOfAvoidAreas = this->pListOfAvoidAreas;
		if (pListOfAvoidAreas) {
			delete static_cast<BSSimpleList<ActiveEffect*>*>(pListOfAvoidAreas);
		}
		this->pListOfAvoidAreas = 0;
	}
}

// KEEP: when the target is an actor and the wait distance test passes, the exe compares the lengths of two points
// it constructs at [ebp-0x14] and [ebp-0x34] with the empty NiPoint3 constructor (0x6815C0 at 0x8F2900 and
// 0x8F2908) and never writes (NiPoint3::Length at 0x8F2F76 and 0x8F2F84): an uninitialised read.
bool HighProcess::ProcessEscortOneHour(Actor* apActor, float afTimeDelta) {
	TESPackage* pPack = GetPackageThatIsRunning();
	if (!pPack) {
		return 0;
	}
	ItemChange* pitem = apActor->GetItemBeingEscorted();
	bool bManyflag = 0;
	if (!pitem) {
		uint32_t v4;
		if (!pTarget || (!pTarget->IsActor() && (v4 = pTarget->iFormID, !apActor->HasObjectId(v4)))) {
			uint32_t CastingType;
			SetTargetForPackage(apActor);
			if (!pTarget ||
				(!pTarget->IsActor() && (CastingType = pTarget->iFormID, !apActor->HasObjectId(CastingType)))) {
				AddToProcedureIndexRunning(apActor, 1);
				if (!apActor->IsPathingComplete()) {
					EndMoveMessage(apActor);
				}
				return 0;
			}
		}
	}
	if (!pitem) {
		if (this->pAcquireObject) {
			if (this->pAcquireObject->pRef == apActor) {
				pitem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
				bManyflag = 1;
			}
		}
	}
	bool bIsActor = 0;
	if (pTarget) {
		if (pTarget->IsActor() && pTarget != apActor) {
			bIsActor = 1;
			Actor* pact = static_cast<Actor*>(pTarget);
			BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
			TESPackage* prun = CurrentAIProcess->GetCurrentPackage();
			if (pTarget != pPlayer &&
				(!prun || (prun->GetPackType() != PACKAGE_TYPE_FOLLOW && !prun->GetIsCreated()))) {
				SetProcedureIndexRunning(0);
				return 0;
			}
		}
	}
	NiPoint3 distance = apActor->GetLocationOnReference();
	if (bIsActor) {
		const NiPoint3* v22 = &pTarget->GetLocationOnReference();
		const NiPoint3* v6 = &apActor->GetLocationOnReference();
		distance = *v6 - *v22;
	}
	float fwaitdistance = 0.0f;
	TESObjectREFR* ptarg = 0;
	if (bIsActor) {
		ptarg = pTarget;
		fwaitdistance = 0.0f;
		int32_t ipackdist = pPack->GetPackageTarget()->iValue;
		if (ipackdist <= 0) {
			ipackdist = 200;
		}
		if (pTarget == pPlayer && fwaitdistance == 0.0) {
			fwaitdistance = static_cast<float>(ipackdist);
		} else {
			if (fwaitdistance == 0.0 && (apActor->GetParentCell() && apActor->GetParentCell()->GetInterior())) {
				fwaitdistance = fAIEscortWaitDistanceInterior.Float();
			} else {
				if (fwaitdistance == 0.0) {
					double v30 = static_cast<double>(ipackdist);
					fwaitdistance = fAIEscortWaitDistanceExterior.Float() * v30;
				}
			}
		}
	}
	if (pPack->pPackLoc && pPack->IsActorAtLocation(apActor, 0, -1.0f, 0)) {
		NiPoint3 fMydistance;
		NiPoint3 ftargdistance;
		if (!bIsActor) {
			PackageLocation* ppackloc;
			PackageTarget* ppacktarg_2;
			ppackloc = pPack->pPackLoc;
			ppacktarg_2 = pPack->GetPackageTarget();
			if (pitem) {
				ExtraDataList* plist = 0;
				if (pitem->pExtraObjectList) {
					plist = pitem->pExtraObjectList->m_item;
				}
				uint32_t id = 0;
				if (plist) {
					if (plist->GetReferencePointer()) {
						TESObjectREFR* ReferencePointer = plist->GetReferencePointer();
						id = ReferencePointer->iFormID;
					}
				}
				TESObjectREFR* pcont = pPack->pPackLoc->GetLocReference();
				if (!pcont) {
					pcont = pGenericLocation;
				}
				if (pcont && pcont->HasContainer()) {
					int32_t InitialTargetCount = pPack->GetInitialTargetCount();
					apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, InitialTargetCount, id);
					bActivateAnim = 0;
				} else {
					if (pitem->pExtraObjectList) {
						plist = pitem->pExtraObjectList->m_item;
					}
					const NiPoint3* ppoint = 0;
					if (ppackloc) {
						TESObjectREFR* pref = ppackloc->GetLocReference();
						if (!pref) {
							pref = pGenericLocation;
						}
						if (pref) {
							TESBoundObject* v10;
							TESBoundObject* ObjectReference = pref->GetObjectReference();
							if (ObjectReference == pXMarker ||
								(v10 = pref->GetObjectReference(), v10 == pXMarkerHeading)) {
								ppoint = &pref->GetLocationOnReference();
							}
						}
					}
					int32_t inumber = pPack->GetInitialTargetCount();
					apActor->DropObject(pitem->pContainerObj, plist, inumber, ppoint, 0);
					bActivateAnim = 0;
				}
				ItemChange* v38 = pitem;
				ItemChange* pitem_2 = pitem;
				if (pitem) {
					delete pitem_2;
				}
				if (ppacktarg_2->GetTargObjectType() == POTYPE_WEAP ||
					(ppacktarg_2->GetTargObjectType() >= POTYPE_COMBATWEAR &&
						ppacktarg_2->GetTargObjectType() <= POTYPE_WEAP_RANGED)) {
					while (!ObjectList.IsEmpty()) {
						this->pAcquireObject = (&ObjectList)->m_item;
						ObjectList.Remove(this->pAcquireObject);
						SetTarget(this->pAcquireObject->pRef);
						pitem = apActor->GetInventoryItem(this->pAcquireObject->pObject, 0);
						ObjectstoAcquire* pAcquireObject = this->pAcquireObject;
						void* v37 = (void*)pAcquireObject;
						if (pAcquireObject) {
							delete static_cast<Date*>(v37);
						}
						this->pAcquireObject = 0;
						if (pcont && pcont->HasContainer()) {
							int32_t v11 = pPack->GetInitialTargetCount();
							apActor->PutItemInReference(pitem->pContainerObj, plist, pcont, v11, id);
						} else {
							if (pitem->pExtraObjectList) {
								plist = pitem->pExtraObjectList->m_item;
							}
							const NiPoint3* ppoint_2 = 0;
							if (ppackloc) {
								TESObjectREFR* pref_2 = ppackloc->GetLocReference();
								if (!pref_2) {
									pref_2 = pGenericLocation;
								}
								if (pref_2) {
									TESBoundObject* v13;
									TESBoundObject* v12 = pref_2->GetObjectReference();
									if (v12 == pXMarker ||
										(v13 = pref_2->GetObjectReference(), v13 == pXMarkerHeading)) {
										ppoint_2 = &pref_2->GetLocationOnReference();
									}
								}
							}
							int32_t inumber_2 = pPack->GetInitialTargetCount();
							apActor->DropObject(pitem->pContainerObj, plist, inumber_2, ppoint_2, 0);
						}
						ItemChange* v34 = pitem;
						ItemChange* pitem_3 = pitem;
						if (pitem) {
							delete pitem_3;
						}
						pitem = 0;
					}
					bool abweararmor = 1;
					bool abwearweapon = 1;
					if (GetCurrentPackage()) {
						TESPackage* v14 = GetCurrentPackage();
						if (v14->GetForceUnequipWeapon()) {
							abwearweapon = 0;
						}
					}
					if (apActor->IsNpc()) {
						TESNPC* pnpc = static_cast<TESNPC*>(apActor->GetObjectReference());
						if (pnpc) {
							pnpc->InitDefaultWorn(apActor, abweararmor, abwearweapon, 0, 1);
						}
					} else {
						if (apActor->IsCreature()) {
							TESCreature* pcrea = static_cast<TESCreature*>(apActor->GetObjectReference());
							if (pcrea) {
								pcrea->InitDefaultWorn(apActor, abweararmor, abwearweapon, 1);
							}
						}
					}
				}
				SetCurrentActionComplete(1);
			}
			AddToProcedureIndexRunning(apActor, 1);
		LABEL_106:;
			if (!apActor->IsPathingComplete()) {
				EndMoveMessage(apActor);
			}
			return 0;
		}
		double v15 = distance.Length();
		if (!(fwaitdistance >= v15)) {
			goto LABEL_106;
		}
		double v29 = fMydistance.Length();
		if (!(ftargdistance.Length() >= v29)) {
			goto LABEL_106;
		}
		AddToProcedureIndexRunning(apActor, 1);
		Actor* ptarg_2 = static_cast<Actor*>(pTarget);
		BaseProcess* v28 = ptarg_2->pCurrentProcess;
		if (!v28->GetPackageThatIsRunning()) {
			goto LABEL_106;
		}
		BaseProcess* v27 = ptarg_2->pCurrentProcess;
		v27->AddToProcedureIndexRunning(apActor, 1);
		BaseProcess* v26 = ptarg_2->pCurrentProcess;
		TESPackage* v16 = v26->GetPackageThatIsRunning();
		if (!v16->GetIsCreated()) {
			goto LABEL_106;
		}
		BaseProcess* v25 = ptarg_2->pCurrentProcess;
		v25->ClearCurrentPackage();
		if (ptarg_2->IsPathingComplete()) {
			ptarg_2->EndMovement();
		}
		SetCurrentActionComplete(1);
		CheckforNewPackage(apActor, 0);
		return 0;
	} else {
		float fdistance = pPack->GetPackageRadiusActorToRefTarget(apActor, 0);
		if (pTarget == pPlayer) {
			apActor->ForceStopMoving();
		} else {
			double DistanceFromReference;
			if (!pTarget ||
				(!pTarget->IsActor() ||
					(DistanceFromReference = apActor->GetDistanceFromReference(pTarget, 0, 0),
						fdistance < DistanceFromReference))) {
				TESWorldSpace* LocationWorld = pPack->GetLocationWorld(apActor);
				TESObjectCELL* LocationCell = pPack->GetLocationCell(apActor);
				apActor->SetPathfindingGoal(pPack->GetLocationCoord(apActor), LocationCell, LocationWorld, fdistance,
					0);
			}
		}
		return 0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::AddAvoidPathingArea(Actor* apActor, NiPoint3 aPoint, float afRadius, float afTimeToExpire,
	float afRadiationLevel, TESObjectREFR* apRef, TESObjectSTAT* apMarker) {
	AvoidAreaStruct* pavoidstruct;
	PathingAvoidNode v13;
	void* apMemory = ourMalloc(sizeof(AvoidAreaStruct));
	if (apMemory) {
		pavoidstruct = ::new (apMemory) AvoidAreaStruct;
	} else {
		pavoidstruct = 0;
	}
	AvoidAreaStruct* pavoidstruct_2 = pavoidstruct;
	PathingAvoidNode* v8 = FNVD_RECONSTRUCT(PathingAvoidNode)(&v13, aPoint, afRadius, afRadiationLevel);
	(*pavoidstruct_2).pAvoidNode = *v8;
	pavoidstruct_2->afRadiationLevel = afRadiationLevel;
	pavoidstruct_2->fTimeExpire = afTimeToExpire;
	pavoidstruct_2->pRadiationMarker = apMarker;
	pavoidstruct_2->pRefObj = apRef;
	apRef->SetTargeted(1);
	if (!pListOfAvoidAreas) {
		BSSimpleList<AvoidAreaStruct*>* v9;
		void* v12 = ourMalloc(sizeof(BSSimpleList<AvoidAreaStruct*>));
		if (v12) {
			v9 = ::new (v12) BSSimpleList<AvoidAreaStruct*>;
		} else {
			v9 = 0;
		}
		pListOfAvoidAreas = v9;
	}
	pListOfAvoidAreas->AddHead(pavoidstruct_2);
	if (GetCurrentRadiation() <= 0.0) {
		float aftime = 0.0f;
		if (afTimeToExpire != 3.4028234663852886e+38 && !apActor->IsPathing()) {
			aftime = afTimeToExpire;
		}
		apActor->InitiateAvoidPackage(apRef, aftime);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HighProcess::DetectActor(TESObjectREFR* pref, Actor* apActor, bool& bspectatorstart,
	DispositionParamData* pdispdata, ACTOR_AGGRESSION aeAggressionvalue, float afConfidence, float afCombatStrength) {
	uint32_t uiAggroRadius;
	int32_t idetection = -100;
	Actor* pactor = static_cast<Actor*>(pref);
	TESActorBase* pbase = apActor->GetActorBase();
	if (pbase && pbase->AIData.bAggroRadius) {
		uiAggroRadius = pbase->AIData.uiAggroRadius;
	} else {
		uiAggroRadius = 0;
	}
	int32_t iAggroRadius = uiAggroRadius;
	bool bAddAggroActor = 0;
	bool bmeInCombat = apActor->IsInCombat();
	CombatGroup* pcurrentGroup = apActor->GetCombatGroup();
	if (pref) {
		if (!pref->GetDisabled() && pref->IsActor() && pactor != apActor) {
			BGSDefaultObjectManager::Instance();
			TESFaction* DefaultObject =
				static_cast<TESFaction*>(BGSDefaultObjectManager::GetDefaultObject(DO_PERK_STRANGER_FACTION));
			if (!pactor->IsInFaction(DefaultObject)) {
				DetectionState* pstate = GetDetectionState(pactor, 0);
				if (pstate) {
					idetection = pstate->iLevel;
				}
				TESPackage* peditorPack = apActor->GetPackageSetAsPcurrent();
				float fdetectvalue = fSneakNoticedMin.Float();
				if (peditorPack) {
					if (!peditorPack->GetCombatAlert()) {
						fdetectvalue = 0.0f;
					}
				}
				if (fdetectvalue < static_cast<double>(idetection) && (!pstate || !pstate->bEvaluated)) {
					if (pstate) {
						pstate->bEvaluated = 1;
					}
					if (!pactor->IsDead(0) && !pactor->IsUnConscious()) {
						TESRace* v11;
						// The exe zeroes four locals it never reads again (bytes at [ebp-0x51] and [ebp-0x45], dwords at
						// [ebp-0x50] and [ebp-0x4C]); named here after the PDB's unused locals of those types.
						FIGHT_REACTION ereaction;
						Actor* pcombattarget = 0;
						int32_t idispcombat = 0;
						int32_t iCombatLevel = 0;
						bool bcombat = 0;
						if (idetection > 0 && pactor->IsInCombat(!bmeInCombat)) {
							Actor* ptarget = pactor->GetCombatTarget();
							if (ptarget) {
								BaseProcess* pTargetsProcess = pactor->pCurrentProcess;
								DetectionState* pdetect = pTargetsProcess->GetDetectionState(ptarget, 0);
								bcombat = pdetect && pdetect->iLevel > 0;
							}
						}
						bool bcrime = 0;
						int32_t pCriminal = 0;
						bool bcrimeagainstplayer = 0;
						if (bcombat && fSneakSeenMin.Float() < static_cast<double>(idetection) &&
							!pactor->IsInCombatantFaction()) {
							CombatGroup* pgroup = pactor->GetCombatGroup();
							if (pgroup) {
								if (pcurrentGroup != pgroup) {
									if (pgroup->GetTargetCount()) {
										if (!IsGroupAlreadyInList(pgroup) &&
											pCombatManager->GetWanttoHelpGroup(apActor, pactor)) {
											bool bhelpGroup = 1;
											int32_t NumberActorsInCombat = pPlayer->GetNumberActorsInCombat();
											if (NumberActorsInCombat >= iCombatTargetPlayerSoftCap.Int() &&
												pgroup->IsTarget(pPlayer) &&
												(!pcurrentGroup || !pcurrentGroup->IsTarget(pPlayer))) {
												bhelpGroup = 0;
											}
											if (bhelpGroup) {
												StartCombatStates* pstat;
												StartCombatStates* HighProcess__UnkCombatTarget = static_cast<
													StartCombatStates*>(ourMalloc(sizeof(StartCombatStates)));
												if (HighProcess__UnkCombatTarget) {
													pstat = ::new (HighProcess__UnkCombatTarget) StartCombatStates;
												} else {
													pstat = 0;
												}
												StartCombatStates* pstat_2 = pstat;
												pstat->pGroup = pgroup;
												GroupsToHelpList.AddHead(pstat_2);
											}
										}
									}
								}
							}
						}
						Actor* ptemp2 = pactor;
						int32_t ireaction = 0;
						float fThierCombatStrength = pactor->CalculateCombatStrength(-1.0f);
						if (fThierCombatStrength == 0.0) {
							fThierCombatStrength = 1.0f;
						}
						ereaction = FIGHT_REACTION_NEUTRAL;
						FIGHT_REACTION ereaction2 = FIGHT_REACTION_NEUTRAL;
						if (apActor->GetGhost() || (pactor->GetGhost() || apActor == pactor)) {
							iCombatLevel = 0;
						} else {
							double fTargetsConfidence;
							if (apActor->GetActorValue(ActorValue::AGGRESSION) ||
								(!pactor->GetShouldAttackActor(apActor, 0, ereaction2, 0) ||
									(fTargetsConfidence = afCombatStrength / fThierCombatStrength,
										!(TESAIForm::GetConfidenceLevel(static_cast<int32_t>(afConfidence)) >
											fTargetsConfidence)))) {
								if (apActor->GetShouldAttackActor(pactor, 0, ereaction, 0)) {
									iCombatLevel = 100;
								} else {
									if (iAggroRadius > 0 &&
										(ereaction == FIGHT_REACTION_NEUTRAL || ereaction == FIGHT_REACTION_ENEMY)) {
										NiPoint3 vec =
											*apActor->QLocationOnReference() - *pactor->QLocationOnReference();
										double v10 = vec.SqrLength();
										if (static_cast<double>(iAggroRadius * iAggroRadius) >= v10) {
											bAddAggroActor = 1;
										}
									}
								}
							} else {
								AvoidActorList.AddHead(pactor);
							}
						}
						Actor* apActora = apActor;
						pactor = ptemp2;
						if ((!apActor->IsCreature() &&
								(pactor->IsNpc() &&
									((v11 = pactor->GetRace(), v11->GetPlayable()) &&
										!pactor->GetActorBase()->IsInEvilFactionsOnly()))) ||
							iCombatLevel <= 0) {
							if (iCombatLevel <= 0) {
								if (bAddAggroActor && idetection > 0 &&
									!apActora->ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_AGGRO_RADIUS_BEHAVIOR)) {
									AggroRadiusList.AddHead(pactor);
								}
							} else {
								if (!pcombattarget) {
									if (pactor->GetCombatController()) {
										pcombattarget = pactor->GetCombatController()->GetTarget();
									}
								}
								if (!pactor->IsInCombatantFaction() ||
									(!pcombattarget || !pcombattarget->IsInCombatantFaction())) {
									StartCombatStates* psta_3;
									bool abflag_2 = !apActora->IsActoraGuard();
									if (!apActora->GetActorBase()->IsInEvilFactionsOnly()) {
										pactor->GetActorBase()->IsInEvilFactionsOnly();
									}
									StartCombatStates* v19 =
										static_cast<StartCombatStates*>(ourMalloc(sizeof(StartCombatStates)));
									if (v19) {
										psta_3 = ::new (v19) StartCombatStates;
									} else {
										psta_3 = 0;
									}
									StartCombatStates* psta_4 = psta_3;
									psta_3->pTarget = pactor;
									AggroList.AddHead(psta_4);
									plastDetected = pref;
								}
							}
						} else {
							StartCombatStates* psta;
							apActora->IsActoraGuard();
							bool abflag = apActora->GetActorBase()->IsInEvilFactionsOnly();
							StartCombatStates* apMemory =
								static_cast<StartCombatStates*>(ourMalloc(sizeof(StartCombatStates)));
							if (apMemory) {
								psta = ::new (apMemory) StartCombatStates;
							} else {
								psta = 0;
							}
							StartCombatStates* psta_2 = psta;
							psta->pTarget = pactor;
							psta_2->bSendAlarm = abflag;
							AggroList.AddHead(psta_2);
						}
						ObserveCombat(apActora, pactor);
					}
				}
			}
		}
	}
}

void HighProcess::SaveGame_00928880(BGSSaveFormBuffer* a2) {
	NiPoint3 kPosition;
	char cWantState;
	uint32_t Flag;
	NiPoint3 kVelocity;
	char cState;
	Actor* v7 = a2->GetActor();
	bhkCharacterController* v8 = GetCharController();
	if (v8) {
		if (!v7 || !v7->IsDead(0)) {
			cState = static_cast<char>(v8->getState());
			a2->SaveDataEndian(&cState, 1u, 0);
			cWantState = v8->eWantState;
			a2->SaveDataEndian(&cWantState, 1u, 0);
			Flag = v8->GetFlag(0xFFFFFFFFu);
			a2->SaveDataEndian(&Flag, 4u, 0);
			if ((Flag & 2048) != 0) {
				v8->GetPosition(kPosition);
				a2->SaveDataEndian(&kPosition, 12u, 0);
			}
			a2->SaveDataEndian(&v8->OutVelocity, 16u, 0);
			a2->SaveDataEndian(&v8->VelocityMod, 16u, 0);
			v8->GetLinearVelocity(kVelocity);
			a2->SaveDataEndian(&kVelocity, 12u, 0);
			if (v7) {
				a2->SaveDataEndian(&v8->fJumpHeight, 4u, 0);
				a2->SaveDataEndian(&v8->fFallStartHeight, 4u, 0);
			} else {
				a2->SaveDataEndian(&v8->fGravity, 4u, 0);
			}
			a2->SaveDataEndian(&v8->fFallTime, 4u, 0);
			a2->SaveDataEndian(&v8->fPitchAngle, 4u, 0);
			a2->SaveDataEndian(&v8->fPitchMult, 4u, 0);
		}
	}
}

// The exe's body is empty: it returns false without touching its arguments.
bool HighProcess::ComputeRandomPointOnPath(Actor* apActor, NiPoint3 aStartPoint, float afRadius, NiPoint3& aOutCoord) {
	return false;
}

NiPoint3* HighProcess::ComputeRandomPoint(NiPoint3* apResult, NiPoint3 aStartPoint, float afMaxRadius,
	float afMinRadius, bool bRandZ) {
	NiPoint3 v7;
	NiPoint3 result;
	float x = RandomFloat0To1();
	float y = RandomFloat0To1();
	float z = 0.0;
	if (bRandZ) {
		z = RandomFloat0To1();
	}
	NiPoint3 dst(x, y, z);
	dst.Unitize();
	float v9 = RandomFloat0To1();
	float v12 = (afMaxRadius + afMinRadius) * v9;
	if (afMinRadius > static_cast<double>(v12)) {
		v12 = afMinRadius;
	}
	NiPoint3* v5 = &(result = dst * v12);
	dst = (v7 = aStartPoint + *v5);
	*apResult = dst;
	return apResult;
}

bool HighProcess::FindDoorsToSendYellAlarm(TESObjectREFR* apRef, void* aiFilterData) {
	if (!apRef || (apRef->GetDelete() || apRef->GetTemporary())) {
		return 0;
	}
	if (apRef->GetObjectReference()->GetFormType() == DOOR_ID) {
		if (apRef->GetTeleport()) {
			DoorRefListCrime.AddHead(apRef);
		}
	}
	return 0;
}

NiPoint3* bhkBoxShape::GetHalfExtents(NiPoint3* apResult) {
	NiPoint3 ecx0;
	const hkVector4* v2 = &GethkHalfExtents();
	*apResult = HK2NI(ecx0, *v2);
	return apResult;
}

const hkVector4& bhkBoxShape::GethkHalfExtents() {
	hkpBoxShape* pb = static_cast<hkpBoxShape*>(operator*());
	if (pb) {
		return *&pb->m_halfExtents;
	} else {
		return hkVector4::getZero();
	}
}

// The PDB places TESPackage::GetOnChangeScript and GetOnEndScript in this file.
Script* TESPackage::GetOnChangeScript() {
	return OnChange.QScript();
}

Script* TESPackage::GetOnEndScript() {
	return OnEnd.QScript();
}

// Whether the package forbids idles; src's PACKAGE_FLAGS has no name for flag 0x1000000.
bool TESPackage::GetNoIdlesFlag() {
	return (PackData.iPackFlags & 0x1000000) != 0;
}

TESTopic* TESPackage::GetOnEndTopic() {
	return OnEnd.pTopic;
}

void PathingRequestFlee::SetMinFleeDistance(float afDistance) {
	fMinFleeDistance = afDistance;
}

float ActorValueCache::GetCachedActorValue(ActorValue::Index aiWhich) {
	return Cache[aiWhich].fValue;
}

bool ActorValueCache::IsCachedActorValueOutOfDate(ActorValue::Index aiWhich) {
	return Cache[aiWhich].bOutOfDate;
}

AvoidAreaStruct::AvoidAreaStruct()
	: fTimeExpire(3.4028235e+38f), pRadiationMarker(nullptr), afRadiationLevel(0.0f), pRefObj(nullptr) {}

void AvoidAreaStruct::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR* v3;
	if (pRefObj) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefObj)));
		v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v3 = 0;
	}
	pRefObj = v3;
	if (pRefObj) {
		pRefObj->SetTargeted(1);
	}
}

void AvoidAreaStruct::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	pAvoidNode.SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(&fTimeExpire, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&afRadiationLevel, 4u, 0);
	apSaveGameBuffer->SaveFormID(pRadiationMarker, 0);
	apSaveGameBuffer->SaveFormID(pRefObj, 0);
}

DetectionEvent::DetectionEvent()
	: iActionValue(0), Location(NiPoint3_ZERO), eEventType(DETECTION_EVENT_NONE), pRef(nullptr) {
	CombatTimeStamp kStamp(-3.4028235e+38f);
	fTimeStamp.fTimeStamp = kStamp.fTimeStamp;
}

void DetectionEvent::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iActionValue), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Location), 0xC, 0);
	fTimeStamp.SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eEventType), 4, 0);
	apSaveGameBuffer->SaveFormID(pRef, 0);
}

void DetectionEvent::LoadGame(BGSLoadGameBuffer* arg0) {
	arg0->LoadData(this, 4u);
	arg0->LoadData(&Location, 12u);
	fTimeStamp.LoadGame(arg0);
	arg0->LoadData(&eEventType, 4u);
	arg0->LoadFormID(pRef);
}

void DetectionEvent::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR* v3;
	if (pRef) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRef)));
		v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v3 = 0;
	}
	pRef = v3;
	if (pRef) {
		pRef->SetTargeted(1);
	}
}

Conversation* DialoguePackage::GetPackConversation() {
	return pConversation;
}

void DialoguePackage::SetPackConversation(Conversation* apConversation) {
	pConversation = apConversation;
}

void MiddleHighProcess::SetCharController(NiPointer<bhkCharacterController> apControl) {
	spCharController = apControl;
}

bool MiddleHighProcess::IsRunningRunOnce() {
	BaseProcess* pProcess = this;
	return pProcess->GetRunOncePackage() != nullptr;
}

StartCombatStates::StartCombatStates()
	: pTarget(nullptr), pGroup(nullptr), bMustEntercombat(false), bSendAlarm(false), bForceFlee(false), bFleeing(false),
	  bIgnoreDefensivePackage(false), bSpectatorOfCombat(false), iPriority(0) {}

uint32_t DialogueResponse::GetResponseID() {
	return nResponseID;
}
