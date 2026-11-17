#include "Fallout/AI/Movement/DetailedActorPathHandler.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Sky/Precipitation.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/Movement/ActorPathHandler.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <new>
#include <stdint.h>

// The exe's vtable address for the actor-avoidance array's instantiation.
inline constexpr fnvd::Vtable BSSimpleArray_ActorAvoidanceData_1024_vftable{};

// The avoidance settings, under IDA's names (gs_..._Pathfinding) without the prefix; the PDB names them
// fINIAvoidanceDefaultWaitTime, fINIAvoidanceIgnoreTime, fINIAvoidanceIgnoreMinTime, fINIAvoidanceDetectionTime and
// fINIAvoidanceAvoidNodeCost.
static SettingT<INISettingCollection> fAvoidanceDefaultWaitTime("fAvoidanceDefaultWaitTime:Pathfinding", 5.0f);
static SettingT<INISettingCollection> fAvoidanceIgnoreTime("fAvoidanceIgnoreTime:Pathfinding", 5.0f);
static SettingT<INISettingCollection> fAvoidanceIgnoreMinTime("fAvoidanceIgnoreMinTime:Pathfinding", 2.5f);
static SettingT<INISettingCollection> fAvoidanceDetectionTime("fAvoidanceDetectionTime:Pathfinding", 0.5f);
static SettingT<INISettingCollection> fAvoidanceAvoidNodeCost("fAvoidanceAvoidNodeCost", 2.0f);

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> gs_bUseActorAvoidance_Pathfinding("bUseActorAvoidance:Pathfinding", true);
static SettingT<INISettingCollection> bINIUseFartherCheckForAvoidance("bUseFartherCheckForAvoidance:Pathfinding",
	false);
SettingT<INISettingCollection> fAvoidanceMinWaitTime("fAvoidanceMinWaitTime:Pathfinding", 0.25f);
SettingT<INISettingCollection> gs_fAvoidanceTimeCheck_Pathfinding("fAvoidanceTimeCheck:Pathfinding", 0.75f);
SettingT<INISettingCollection> gs_fRepathingWaitDistance_Pathfinding("fRepathingWaitDistance:Pathfinding", 20.0f);
SettingT<INISettingCollection> gs_fAvoidanceAvoidAllRadius_Pathfinding("fAvoidanceAvoidAllRadius:Pathfinding", 100.0f);
SettingT<INISettingCollection> gs_fAITurnSpeedScale_Pathfinding("fAITurnSpeedScale:Pathfinding", 1.5f);
SettingT<INISettingCollection> gs_fAICombatTurnSpeedScale_Pathfinding("fAICombatTurnSpeedScale:Pathfinding", 2.5f);
SettingT<INISettingCollection> gs_fCreatureTurnSpeedScale_Pathfinding("fCreatureTurnSpeedScale:Pathfinding", 1.25f);
SettingT<INISettingCollection> gs_fCreatureCombatTurnSpeedScale_Pathfinding("fCreatureCombatTurnSpeedScale:Pathfinding",
	1.25f);
SettingT<INISettingCollection> fLODActorAvoidanceDistance("fLODActorAvoidanceDistance:General", 500.0f);
SettingT<INISettingCollection> fRotationSpeedIncrease("fRotationSpeedIncrease:Pathfinding", 0.43633232f);
NiTPrimitiveArray<Actor*> DetailedActorPathHandler::AvoidanceCandidates(0, 1);

// The registration macro cannot take the template's comma, so the instantiation is named once.
using ActorAvoidanceArray = BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>;

DetailedActorPathHandler::ActorAvoidanceData::ActorAvoidanceData()
	: pActor(nullptr), fDetection(-0x1.fffffe0000000p+127f) {}

DetailedActorPathHandler::ActorAvoidanceData::ActorAvoidanceData(Actor* a1, float a2) : pActor(a1), fDetection(a2) {}

DetailedActorPathHandler::ActorAvoidanceData*
DetailedActorPathHandler::ActorAvoidanceData::Copy(const DetailedActorPathHandler::ActorAvoidanceData* a2) {
	*this = *a2;
	return this;
}

// The exe's cleanup-only C++ EH frame is left out.
// BlockedTargetPoint and the timers the exe's constructor leaves alone stay unset.
DetailedActorPathHandler::DetailedActorPathHandler(Actor* apActor, ActorMover* apMover)
	: ActorPathHandler(apActor, apMover), fTetherMinMove(10.0f), fTetherDistFromActor(20.0f), MoveDelta(NiPoint3_ZERO),
	  TetherTargetPoint(NiPoint3_ZERO), LastLocation(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f),
	  LastTetherPoint(NiPoint3_ZERO), ActorAvoidList(), MoveMode(0), iCurrentNodeIndex(0), fPathLength(0.0f),
	  fCurrentSplineT(0.0f), fAvoidanceDetectionValue(0.0f), fTimeInAnim(1.0f), fRunWalkTime(0.0f),
	  fMinRotateTime(-1.0f), fGoalRadiusSqr(0.0f), fDistanceTraveled(0.0f), fIgnoreLookAtPointTimer(0.0f),
	  fLastRotationApplied(0.0f), bIgnoreInitialRotation(false), bIgnoreInitialRotationKeepActorHeading(false),
	  fIgnoreInitialRotationTetherParam(0.0f), eState(DAP_GOAL_REACHED), eBlendRunning(PATH_RUNNING_BLEND_INVALID),
	  eAvoidState(PATH_AVOID_INVALID), pCurrentDoor(nullptr), bEndUpdatePath(false), bWaitingForUpdate(false),
	  bIgnoreLookAtPoint(false), bDeltaComputedThisFrame(false) {}

// BlockedTargetPoint and the timers the exe's constructor leaves alone stay unset; eState is UNK_0 with a path or a
// request, else DAP_GOAL_REACHED.
DetailedActorPathHandler::DetailedActorPathHandler(Actor* apActor, ActorMover* apMover,
	NiPointer<PathingSolution>& apPath, PathingRequest* apRequest)
	: ActorPathHandler(apActor, apMover, apPath, apRequest), fTetherMinMove(10.0f), fTetherDistFromActor(20.0f),
	  MoveDelta(NiPoint3_ZERO), TetherTargetPoint(NiPoint3_ZERO),
	  LastLocation(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f), LastTetherPoint(NiPoint3_ZERO), ActorAvoidList(),
	  MoveMode(0), iCurrentNodeIndex(0), fPathLength(0.0f), fCurrentSplineT(0.0f), fAvoidanceDetectionValue(0.0f),
	  fTimeInAnim(1.0f), fRunWalkTime(0.0f), fMinRotateTime(-1.0f), fGoalRadiusSqr(0.0f), fDistanceTraveled(0.0f),
	  fIgnoreLookAtPointTimer(0.0f), fLastRotationApplied(0.0f), bIgnoreInitialRotation(false),
	  bIgnoreInitialRotationKeepActorHeading(false), fIgnoreInitialRotationTetherParam(0.0f),
	  eState((!(apPath == static_cast<PathingSolution*>(nullptr)) || apRequest) ? UNK_0 : DAP_GOAL_REACHED),
	  eBlendRunning(PATH_RUNNING_BLEND_INVALID), eAvoidState(PATH_AVOID_INVALID), pCurrentDoor(nullptr),
	  bEndUpdatePath(false), bWaitingForUpdate(false), bIgnoreLookAtPoint(false), bDeltaComputedThisFrame(false) {}

DetailedActorPathHandler::~DetailedActorPathHandler() {
	spPath = nullptr;
	spRequest = nullptr;
	ActorAvoidList.RemoveAll();
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::Init(float afPercentTraveled, float afDeltaT) {
	eState = DetailedActorPathHandler::DAP_INITIALIZING;
	if (eBlendRunning == DetailedActorPathHandler::PATH_RUNNING_BLEND_INVALID) {
		eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
	}
	DetailedActorPathHandler::PATH_AVOID_STATE eAvoidState = this->eAvoidState;
	if (eAvoidState != DetailedActorPathHandler::PATH_AVOID_WAITING &&
		eAvoidState != DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE) {
		this->eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
		fActorAvoidanceTimer = 0.0f;
		fWaitTime = 0.0f;
	}
	MoveDelta = NiPoint3_ZERO;
	bRotateMode = 0;
	bDeltaComputedThisFrame = 0;
	bMovementBlocked = 0;
	fBlockTime = 0.0f;
	iUnblockDirection = 0;
	fAlternateFacingAngle = 0.0f;
	fBlockedStartAngle = 0.0f;
	fLastFrameMinMoveDelta = 0.0f;
	LastFrameMinMoveDelta = NiPoint3_ZERO;
	LastLocation = pActor->GetLocationOnReference();
	fFallTime = 0.0f;
	fMinRotateTime = -1.0f;
	if (spPath.m_pObject->GetNodeCount() >= 2) {
		if (spPath.m_pObject->IsIncomplete()) {
			fGoalRadiusSqr = 1600.0f;
		} else {
			double MaximumLeafAngle = spRequest.m_pObject->fTargetRadius;
			float v8 = spRequest.m_pObject->fTargetRadius * MaximumLeafAngle;
			fGoalRadiusSqr = BSMAX(v8, 0.1f);
			PathingNode* LastNode = spPath.m_pObject->GetLastNode();
			if (LastNode->IsTeleportDoorNode()) {
				fGoalRadiusSqr = 6400.0f;
			}
		}
		PathingNode* v5 = spPath.m_pObject->GetLastNode();
		if (v5->IsTransitionNode()) {
			bEndUpdatePath = 1;
		}
		ComputeMinTetherDistance();
		bool bpathTransition = bWaitingForUpdate;
		FindPathStartingLocation(afPercentTraveled);
		bIgnoreInitialRotation = 0;
		bIgnoreInitialRotationKeepActorHeading = 0;
		fIgnoreInitialRotationTetherParam = 0.0f;
		if (!pActor->IsInCombat() &&
			static_cast<double>(fTetherDistFromActor) > spPath.m_pObject->ComputeDetailedPathLength() &&
			!pActorMover->HasLookAtTarget()) {
			bIgnoreInitialRotation = 1;
			bIgnoreInitialRotationKeepActorHeading = 1;
			fIgnoreInitialRotationTetherParam = static_cast<float>(spPath.m_pObject->GetNodeCount());
		}
		if (CheckGoalReached() == ActorPathHandler::PATH_HANDLER_RESULT_OK) {
			float fgoalAngle;
			NiPoint3 dirVec;
			if (bIgnoreLookAtPoint || !pActorMover->GetLookAtPoint(dirVec)) {
				dirVec = TetherTargetPoint;
			}
			const NiPoint3* v6 = &pActor->GetLocationOnReference();
			dirVec.operator-=(*v6);
			double v7 = dirVec.Length() * 0.949999988079071;
			if (!(std::fabs(dirVec.z) >= v7)) {
				fgoalAngle = GetZAngleFromVector(dirVec);
			} else {
				fgoalAngle = pActor->GetHeading(0);
			}
			if (!bpathTransition) {
				if (afPercentTraveled == 0.0) {
					InitRotationMode(fgoalAngle, 0, bpathTransition);
				} else {
					pActor->SetHeading(fgoalAngle);
				}
			}
		}
		eState = DetailedActorPathHandler::DAP_MOVING;
		if (!(afDeltaT > 0.0)) {
			return ActorPathHandler::PATH_HANDLER_RESULT_OK;
		} else {
			return Update(afDeltaT);
		}
	} else {
		ResetMoveModeAndDelta();
		bWaitingForUpdate = 0;
		TetherTargetPoint = pActor->GetLocationOnReference();
		LastTetherPoint.x = TetherTargetPoint.x;
		LastTetherPoint.y = TetherTargetPoint.y;
		LastTetherPoint.z = TetherTargetPoint.z;
		eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
		return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
	}
}

bool DetailedActorPathHandler::InitRotationOnly(float afTargetAngle, bool abForceTurn) {
	bWaitingForUpdate = 0;
	eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
	eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
	eAvoidState = PATH_AVOID_OK;
	fAvoidanceDetectionValue = 0.0;
	fDistanceTraveled = 0.0;
	fPathLength = 0.0;
	spPath.operator=(static_cast<PathingSolution*>(0));
	spRequest.operator=(static_cast<PathingRequest*>(0));
	bIgnoreInitialRotation = 0;
	bIgnoreInitialRotationKeepActorHeading = 0;
	fIgnoreInitialRotationTetherParam = 0.0;
	if (InitRotationMode(afTargetAngle, abForceTurn, 0)) {
		eState = DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH;
		return 1;
	} else {
		eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
		return 0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::Update(float afDeltaT) {
	NiPoint3 v17;
	FNVD_INLINED_LOCAL(NiPointer<PathingRequest>, spnewRequest, std::nullopt, std::nullopt);
	NiMatrix3 MoveModeRot;
	if (eState == DetailedActorPathHandler::DAP_GOAL_REACHED ||
		eState == DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH) {
		return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
	}
	if (!pActor->CanMove()) {
		if (!spPath.m_pObject) {
			return ActorPathHandler::PATH_HANDLER_RESULT_OK;
		}
		ActorPathHandler::PATH_HANDLER_RESULT eresult = CheckGoalReached();
		if (eresult != ActorPathHandler::PATH_HANDLER_RESULT_DONE) {
			return ActorPathHandler::PATH_HANDLER_RESULT_OK;
		}
		ResetMoveModeAndDelta();
		eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
		return eresult;
	}
	bDeltaComputedThisFrame = 1;
	bhkCharacterController* pcon = pActor->GetCharController();
	if (afDeltaT == 0.0 || !pcon) {
	LABEL_52:;
		ResetMoveModeAndDelta();
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	if (pPlayer->IsSleepingorResting()) {
		return UpdateWhilePlayerWaits(afDeltaT);
	}
	if (pcon->GetState() == HK_CHARACTER_IN_AIR) {
		fFallTime = pcon->QFallTime();
		if (fFallTime > 0.4000000059604645) {
			goto LABEL_52;
		}
	} else {
		if (fFallTime > 0.6499999761581421) {
			ResetMoveModeAndDelta();
			return ActorPathHandler::PATH_HANDLER_RESULT_FELL_OFF_PATH;
		}
		fFallTime = 0.0f;
	}
	if (!bRotateMode && !bWaitingForUpdate) {
		ActorPathHandler::PATH_HANDLER_RESULT eresult_2 = CheckGoalReached();
		if (eresult_2) {
			if (eresult_2 != ActorPathHandler::PATH_HANDLER_RESULT_DONE || !CheckForEndRotate()) {
				ResetMoveModeAndDelta();
				eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
				return eresult_2;
			}
		}
	}
	if (!bWaitingForUpdate) {
		UpdateTether(afDeltaT);
	}
	if (bIgnoreInitialRotation && fIgnoreInitialRotationTetherParam <= static_cast<double>(fCurrentSplineT)) {
		bIgnoreInitialRotation = 0;
		bIgnoreInitialRotationKeepActorHeading = 0;
		fIgnoreInitialRotationTetherParam = 0.0f;
	}
	if (bRotateMode) {
		float fnewHeading = UpdateRotation(afDeltaT);
		pActor->SetHeading(fnewHeading);
		float fheading = 0.0f;
		float flooking = 0.0f;
		ComputeHeadingAndLooking(fheading, flooking);
		UpdateLooking(afDeltaT, flooking);
		MoveDelta = NiPoint3_ZERO;
		fLastFrameMinMoveDelta = 0.0f;
		LastFrameMinMoveDelta = NiPoint3_ZERO;
		pActorMover->DebugOnEvent();
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	if (eState == DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH) {
		goto LABEL_52;
	}
	if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_WAITING) {
		DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN avoidRet;
		fWaitTime = fWaitTime - afDeltaT;
		if (!(fWaitTime > 0.0)) {
			fWaitTime = 0.0f;
			eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
			goto LABEL_52;
		}
		avoidRet = UpdateActorAvoidance(afDeltaT);
		if (avoidRet == DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_OK) {
			fActorAvoidanceTimer = fActorAvoidanceTimer - afDeltaT;
			if (fActorAvoidanceTimer <= 0.0) {
				fWaitTime = 0.0f;
				eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
			}
			goto LABEL_52;
		}
		if (avoidRet != DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_FAIL) {
			fActorAvoidanceTimer = fAvoidanceMinWaitTime.Float();
			goto LABEL_52;
		}
	LABEL_42:;
		ResetMoveModeAndDelta();
		return ActorPathHandler::PATH_HANDLER_RESULT_CANT_MOVE;
	}
	DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE v14 = HandleDoorOpening();
	if (v14 == DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE_WAITING_FOR_DOOR_TO_OPEN) {
		goto LABEL_52;
	}
	if (v14 == DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE_FAILED) {
		ResetMoveModeAndDelta();
		return ActorPathHandler::PATH_HANDLER_RESULT_CANT_USE_DOOR;
	}
	ComputeMoveMode(afDeltaT);
	if (!UpdateStaticAvoidance(afDeltaT)) {
		if (eState != DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH) {
			eAvoidState = DetailedActorPathHandler::PATH_AVOID_FAILED;
			return ActorPathHandler::PATH_HANDLER_RESULT_CANT_MOVE;
		}
		goto LABEL_52;
	}
	ComputeSimpleMoveDelta(afDeltaT);
	if (gs_bUseActorAvoidance_Pathfinding.Bool()) {
		if (!bMovementBlocked) {
			DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN eval = UpdateActorAvoidance(afDeltaT);
			if (eval) {
				fAvoidanceDetectionValue = 0.0f;
				switch (eval) {
				case DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_WAIT: {
					eAvoidState = DetailedActorPathHandler::PATH_AVOID_WAITING;
					ResetMoveModeAndDelta();
					return ActorPathHandler::PATH_HANDLER_RESULT_OK;
				}
				case DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_REPATH: {
					void* v13;
					void* apMemory = NiMemObject::operator new(sizeof(PathingRequest));
					if (apMemory) {
						v13 = (void*)::new (apMemory) PathingRequest;
					} else {
						v13 = 0;
					}
					NiRefObject* v20 = static_cast<NiRefObject*>(v13);
					::new (&spnewRequest) NiPointer<PathingRequest>(static_cast<PathingRequest*>(v13));
					spRequest.m_pObject->PathingRequest::CopyTo(spnewRequest.m_pObject);
					if (!spPath.m_pObject) {
						eAvoidState = DetailedActorPathHandler::PATH_AVOID_WAITING;
						ResetMoveModeAndDelta();
						spnewRequest.~NiPointer<PathingRequest>();
						return ActorPathHandler::PATH_HANDLER_RESULT_OK;
					}
					if (!spPath.m_pObject->IsIncomplete() || (&spRequest.m_pObject->Destination)->QUnused()) {
						VirtualPathingNode* LastVirtualNode = spPath.m_pObject->GetLastVirtualNode();
						spnewRequest.m_pObject->SetDestination(*&LastVirtualNode->Location);
					} else {
						spnewRequest.m_pObject->SetDestination(*&spRequest.m_pObject->Destination);
					}
					spnewRequest.m_pObject->ClearInitialPathHeading();
					eAvoidState = DetailedActorPathHandler::PATH_AVOID_REPATH;
					pActorMover->BackGroundBuildPath(spnewRequest.m_pObject);
					if (pActorMover->eState == ACTOR_MOVER_STATE_FINISHED) {
						ResetMoveModeAndDelta();
						bWaitingForUpdate = 0;
						eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
					}
					if (bWaitingForUpdate) {
						spPath.operator=(static_cast<PathingSolution*>(0));
						if (eState == DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH) {
							ResetMoveModeAndDelta();
							spnewRequest.~NiPointer<PathingRequest>();
							return ActorPathHandler::PATH_HANDLER_RESULT_OK;
						}
					} else {
						eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
					}
					spnewRequest.~NiPointer<PathingRequest>();
					break;
				}
				case DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_FAIL: {
					goto LABEL_42;
				}
				}
			}
		}
		if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE) {
			fActorAvoidanceTimer = fActorAvoidanceTimer - afDeltaT;
			if (fActorAvoidanceTimer < 0.0) {
				fActorAvoidanceTimer = 0.0f;
				eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
			}
		}
	}
	float fslowDown = 0.0;
	float fnewHeading_2 = UpdateHeading(afDeltaT, fslowDown);
	pActor->SetHeading(fnewHeading_2);
	pActorMover->DebugOnEvent();
	if (fslowDown > 0.0) {
		MoveDelta.operator*=(fslowDown);
	}
	RotateMoveDelta(afDeltaT);
	fLastFrameMinMoveDelta = MoveDelta.XYLength();
	MoveModeRot.MakeZRotationQ(pActor->GetHeading(0));
	LastFrameMinMoveDelta = MoveModeRot * MoveDelta;
	if (eState == DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING) {
		NiPoint3 tetherDir = TetherTargetPoint - LastTetherPoint;
		tetherDir.z = 0.0;
		const NiPoint3* v6 = &pActor->GetLocationOnReference();
		NiPoint3 actorToTetherDir = TetherTargetPoint - *v6;
		actorToTetherDir.z = 0.0;
		float fdot = tetherDir.Dot(actorToTetherDir);
		if (fdot <= 0.0) {
			eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
		}
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	} else {
		if (spPath.m_pObject) {
			uint32_t v7 = iCurrentNodeIndex + 1;
			if (v7 >= spPath.m_pObject->GetNodeCount()) {
				PathingNode* LastNode = spPath.m_pObject->GetLastNode();
				NiPoint3* a2a = ::new (&v17) NiPoint3((&LastNode->Location)->GetLocation());
				const NiPoint3* v9 = &pActor->GetLocationOnReference();
				float fdistToGoal = (*v9 - *a2a).XYLength();
				if (fdistToGoal < static_cast<double>(fLastFrameMinMoveDelta)) {
					float fmoveDeltaZ = MoveDelta.z;
					MoveDelta.Unitize();
					MoveDelta.x = MoveDelta.x * fdistToGoal;
					MoveDelta.y = MoveDelta.y * fdistToGoal;
					MoveDelta.z = fmoveDeltaZ;
					fLastFrameMinMoveDelta = MoveDelta.XYLength();
					LastFrameMinMoveDelta.x = MoveDelta.x;
					LastFrameMinMoveDelta.y = MoveDelta.y;
					LastFrameMinMoveDelta.z = MoveDelta.z;
				}
			}
		}
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::NoPathUpdate(float afDeltaT) {
	NiPoint3 rot;
	ActorPathHandler::PATH_HANDLER_RESULT eresult = ActorPathHandler::PATH_HANDLER_RESULT_NONE;
	bool bimmobile = pActor->GetActorBase()->IsImmobile();
	if (!pActor->GetTurretBehavior() && !pActorMover->DidPathingFail() && !pActorMover->bForceMoveMode) {
		switch (eState) {
		case DetailedActorPathHandler::UNK_0:
		case DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH: {
			goto LABEL_11;
		}
		case DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING: {
			Update(afDeltaT);
			eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
			break;
		}
		case DetailedActorPathHandler::DAP_GOAL_REACHED: {
			spPath.operator=(static_cast<PathingSolution*>(0));
			spRequest.operator=(static_cast<PathingRequest*>(0));
		LABEL_11:;
			if (!pActorMover->HasLookAtTarget()) {
				MoveMode = pActorMover->GetPreferredMoveModeType();
				eresult = ActorPathHandler::PATH_HANDLER_RESULT_DONE;
			}
			break;
		}
		case DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH: {
			float a1a = UpdateRotation(afDeltaT);
			pActor->SetHeading(a1a);
			MoveDelta = NiPoint3_ZERO;
			fLastFrameMinMoveDelta = 0.0f;
			LastFrameMinMoveDelta = NiPoint3_ZERO;
			bDeltaComputedThisFrame = 1;
			pActorMover->DebugOnEvent();
			if (!bRotateMode) {
				eState = DetailedActorPathHandler::UNK_7;
			}
			eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
			break;
		}
		case DetailedActorPathHandler::UNK_7: {
			ResetMoveModeAndDelta();
			eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
			eresult = ActorPathHandler::PATH_HANDLER_RESULT_DONE;
			break;
		}
		default: {
			break;
		}
		}
	}
	float flookingAngle = 0.0f;
	if (eresult == ActorPathHandler::PATH_HANDLER_RESULT_NONE) {
		if (bIgnoreLookAtPoint) {
			fIgnoreLookAtPointTimer = fIgnoreLookAtPointTimer - afDeltaT;
			if (fIgnoreLookAtPointTimer < 0.0) {
				bIgnoreLookAtPoint = 0;
				eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
			}
		}
		if (eresult == ActorPathHandler::PATH_HANDLER_RESULT_NONE) {
			NiPoint3 lookatPoint;
			if (pActorMover->GetLookAtPoint(lookatPoint) && !pActor->IsInDialoguewithPlayer()) {
				float a1b = pActor->GetHeading(0);
				float factangle = ClampAngle(a1b);
				NiPoint3 firingLocation = CombatUtilities::GetAttackerFiringLocation(pActor);
				// The exe keeps the squared length at full x87 precision (fstp qword) for the compare below.
				double v12 = (firingLocation - lookatPoint).SqrXYLength();
				double Radius = pActor->GetRadius();
				if (pActor->GetRadius() * Radius * 0.25 > v12) {
					eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
				}
				if (eresult == ActorPathHandler::PATH_HANDLER_RESULT_NONE) {
					float a1;
					float fdir;
					CombatUtilities::GetAngleToProjectedPoint(&rot, pActor, lookatPoint);
					float fgoalAngle = ClampAngle(rot.z);
					float fgoalLookAngle = rot.x;
					if (!(pActor->GetRadius() >= 100.0)) {
						a1 = GetAngleDelta(factangle, fgoalAngle, fdir);
					} else {
						const NiPoint3* v3 = &pActor->GetLocationOnReference();
						NiPoint3 delta = lookatPoint - *v3;
						delta.z = 0.0f;
						float fZAngle = GetZAngleFromVector(delta);
						a1 = GetAngleDelta(factangle, fZAngle, fdir);
					}
					float fangleDelta = std::fabs(a1);
					float frotateMax = pActor->GetTurningSpeed() * 0.01745329238474369 * afDeltaT;
					if (pActor->IsCreature()) {
						float fCreatureTurnSpeedScale;
						if (pActor->IsInCombat()) {
							fCreatureTurnSpeedScale = gs_fCreatureCombatTurnSpeedScale_Pathfinding.Float();
						} else {
							fCreatureTurnSpeedScale = gs_fCreatureTurnSpeedScale_Pathfinding.Float();
						}
						frotateMax = frotateMax * fCreatureTurnSpeedScale;
					} else {
						float fTurnSpeedScale;
						if (pActor->IsInCombat()) {
							fTurnSpeedScale = gs_fAICombatTurnSpeedScale_Pathfinding.Float();
						} else {
							fTurnSpeedScale = gs_fAITurnSpeedScale_Pathfinding.Float();
						}
						frotateMax = frotateMax * fTurnSpeedScale;
					}
					float fclampedAngleDelta = fangleDelta;
					if (frotateMax < static_cast<double>(fangleDelta)) {
						fclampedAngleDelta = frotateMax;
					}
					uint32_t inewMoveMode = MoveMode;
					if (bimmobile || !(fMinRotateTime > 0.0)) {
						if (!(fangleDelta > 0.017453292f)) {
							inewMoveMode = pActorMover->GetPreferredMoveModeType();
							factangle = fclampedAngleDelta * fdir + factangle;
						} else {
							if (!bimmobile) {
								fMinRotateTime = fActorTurnAnimMinTime.Float() - afDeltaT;
							}
							if (fdir == 1.0) {
								inewMoveMode = pActorMover->GetPreferredMoveModeType() | 32;
								factangle = factangle + fclampedAngleDelta;
							} else {
								if (fdir == -1.0) {
									inewMoveMode = pActorMover->GetPreferredMoveModeType() | 16;
									factangle = factangle - fclampedAngleDelta;
								}
							}
						}
					} else {
						fMinRotateTime = fMinRotateTime - afDeltaT;
						factangle = fclampedAngleDelta * fdir + factangle;
					}
					if (!pActorMover->bForceMoveMode) {
						MoveMode = inewMoveMode;
					}
					if (!bimmobile || std::fabs(fgoalLookAngle) <= 1.535889744758606) {
						pActor->SetHeading(factangle);
					}
					flookingAngle = fgoalLookAngle;
					if (bimmobile) {
						UpdateLooking(afDeltaT, flookingAngle);
					}
					pActorMover->DebugOnEvent();
				}
			}
		}
		eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	if (!bimmobile) {
		UpdateLooking(afDeltaT, flookingAngle);
	}
	return eresult;
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::UpdateWhilePlayerWaits(float afDeltaT) {
	NiPoint3 rot;
	ResetStaticAvoidance();
	ResetActorAvoidance();
	bRotateMode = 0;
	eState = DetailedActorPathHandler::DAP_MOVING;
	eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
	fFallTime = 0.0f;
	if (spRequest.m_pObject && spRequest.m_pObject->GetWaitIfPlayerWaits()) {
		pActor->SetHeading(pPlayer);
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	} else {
		float fdistToGoal = fPathLength - GetDistanceTraveled();
		PathingNode* plastNode = spPath.m_pObject->GetLastNode();
		const PathingLocation* loc = &plastNode->Location;
		NiPoint3 goalPoint = loc->GetLocation();
		NiPoint3* FrustumPlanes = &plastNode->Tangent;
		float fgoalAngle = GetZAngleFromVector(*FrustumPlanes);
		TESObjectCELL* pgoalCell = loc->GetCell();
		TESWorldSpace* pgoalWS = loc->pWorldSpace;
		float fdelta = 3600.0 / appCalendar.GetTimeScale();
		float fdistanceCanTravel = pActor->GetCurrentSpeed() * fdelta;
		int32_t v4 = pActorMover->iNextNode + 1;
		bool bfinishedPath = v4 == spPath.m_pObject->GetVirtualNodeCount();
		if (pActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING || pActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
			if (spRequest.m_pObject && (spRequest.m_pObject->bFaceTargetAtGoal && bfinishedPath)) {
				fgoalAngle = spRequest.m_pObject->fGoalAngle;
			} else {
				if (pActorMover->HasLookAtTarget()) {
					NiPoint3 lookatPoint;
					pActorMover->GetLookAtPoint(lookatPoint);
					CombatUtilities::GetAngleToProjectedPoint(&rot, pActor, lookatPoint);
					fgoalAngle = ClampAngle(rot.z);
				} else {
					uint32_t inodeCount = spPath.m_pObject->GetNodeCount();
					if (inodeCount <= 1) {
						fgoalAngle = pActor->GetHeading(0);
					} else {
						PathingNode* Node = spPath.m_pObject->GetNode(inodeCount - 2);
						NiPoint3 nextToLastNode = (&Node->Location)->GetLocation();
						PathingNode* v6 = spPath.m_pObject->GetNode(inodeCount - 1);
						NiPoint3 lastNode = (&v6->Location)->GetLocation();
						NiPoint3 movedirDelta = lastNode - nextToLastNode;
						fgoalAngle = GetZAngleFromVector(movedirDelta);
					}
				}
			}
		}
		pActor->MoveActorAndFollowers(goalPoint, fgoalAngle, pgoalCell, pgoalWS, 1, 0);
		ActorPathHandler::PATH_HANDLER_RESULT eresult = ActorPathHandler::PATH_HANDLER_RESULT_DONE;
		ResetMoveModeAndDelta();
		eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
		if (!bfinishedPath) {
			return ActorPathHandler::PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL;
		}
		return eresult;
	}
}

float DetailedActorPathHandler::GetDistanceTraveled() {
	if ((&spPath)->operator->()) {
		return fDistanceTraveled;
	} else {
		return 0.0;
	}
}

// The share of the path walked, clamped to [0, 1]; 1 when the path has no length. A NaN length takes the division.
float DetailedActorPathHandler::GetPercentTraveled() {
	if (fPathLength <= 0.0) {
		return 1.0f;
	}
	float fPercent = GetDistanceTraveled() / fPathLength;
	BSClamp(fPercent, 0.0f, 1.0f);
	return fPercent;
}

NiPoint3 DetailedActorPathHandler::GetNextNodeLocation() {
	NiPoint3 result;
	if (spPath.m_pObject && spPath.m_pObject->GetNodeCount()) {
		PathingNode* LastNode;
		uint32_t v2 = iCurrentNodeIndex + 1;
		if (v2 >= spPath.m_pObject->GetNodeCount()) {
			LastNode = spPath.m_pObject->GetLastNode();
		} else {
			LastNode = spPath.m_pObject->GetNode(iCurrentNodeIndex + 1);
		}
		result = (&LastNode->Location)->GetLocation();
		return result;
	} else {
		result = pActor->GetLocationOnReference();
		return result;
	}
}

bool DetailedActorPathHandler::GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) {
	NiPoint3 v9;
	if (!(&spPath)->operator->()) {
		return (bool)0;
	}
	PathingSolution* v3 = (&spPath)->operator->();
	if (aiIndex >= v3->GetNodeCount()) {
		return (bool)0;
	}
	PathingSolution* v4 = (&spPath)->operator->();
	PathingNode* node = v4->GetNode(aiIndex);
	PathingLocation* v6 = &node->PathingNode::GetLocation();
	arLocation = *&(v9 = v6->GetLocation());
	return (bool)1;
}

// The path length left to walk, never below 0. The exe calls BSMAX (0x404010) out of line.
float DetailedActorPathHandler::GetDistanceRemaining() {
	float fRemaining = fPathLength - GetDistanceTraveled();
	return BSMAX(fRemaining, 0.0f);
}

// The exe's cleanup-only C++ EH frame is left out.
NiPoint3 DetailedActorPathHandler::ComputeApproximateLocation(float afDeltaT) {
	NiPoint3 result;
	FNVD_INLINED_LOCAL(NiPointer<PathingSolution>, sppath, std::nullopt, std::nullopt);
	if (pActorMover->DidPathingFail()) {
		result = pActor->GetLocationOnReference();
		return result;
	} else {
		sppath.SetPointer(&spPath);
		if (sppath.m_pObject && sppath.m_pObject->GetNodeCount()) {
			float fspeed;
			if ((pActorMover->GetPreferredMoveMode() & 512) != 0) {
				fspeed = pActor->GetRunSpeed();
			} else {
				fspeed = pActor->GetWalkSpeed();
			}
			float fdistance = fspeed * afDeltaT;
			::new (&result) NiPoint3(sppath.m_pObject->ComputeApproxLocationForDistance(fCurrentSplineT, fdistance));
			sppath.~NiPointer<PathingSolution>();
			return result;
		} else {
			result = pActor->GetLocationOnReference();
			sppath.~NiPointer<PathingSolution>();
			return result;
		}
	}
}

// The exe's shared one-field getter (thiscall, ret 0, 0x70EC90).
uint32_t DetailedActorPathHandler::GetCurrentNodeIndex() {
	return iCurrentNodeIndex;
}

// The exe's shared "return true" stub (0x8D0360).
bool DetailedActorPathHandler::IsDetailedPathHandler() {
	return true;
}

// The exe's cleanup-only C++ EH frame is left out.
void DetailedActorPathHandler::SetWaitForUpdatedPath(PathingRequest* apRequest) {
	NiPoint3 result;
	NiPoint3 newPathOrigin;
	if (spPath.m_pObject && (spPath.m_pObject->GetNodeCount() && !pPlayer->IsSleepingorResting())) {
		fDistanceTraveled = 0.0f;
		if (bWaitingForUpdate) {
			FNVD_CONSTRUCTED_LOCAL(PathingLocation, apMemory, &PathingLocation::Destruct, TetherTargetPoint, pActor);
			apRequest->SetOrigin(apMemory);
		} else {
			if (eState == DetailedActorPathHandler::DAP_MOVING) {
				bWaitingForUpdate = 1;
				NiPoint3 actorCurrent = pActor->GetLocationOnReference();
				if (bMovementBlocked || (IsWaitingForAvoidance() || bRotateMode)) {
					TetherTargetPoint = actorCurrent;
					LastTetherPoint = actorCurrent;
					eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
					FNVD_CONSTRUCTED_LOCAL(PathingLocation, apLocation, &PathingLocation::Destruct, pActor);
					apRequest->SetOrigin(apLocation);
				} else {
					NiPoint3* a2 = ::new (&result) NiPoint3((&apRequest->Destination)->GetLocation());
					float fnewLen = ((&apRequest->Origin)->GetLocation() - *a2).XYLength();
					float fwaitDist = gs_fRepathingWaitDistance_Pathfinding.Float();
					if ((MoveMode & ActorMover::MF_RUNNING) != 0) {
						float* v4 = &gs_fRepathingWaitDistance_Pathfinding.Float();
						fwaitDist = fwaitDist + *v4;
					}
					fwaitDist = fwaitDist + fwaitDist;
					if (fwaitDist > static_cast<double>(fnewLen)) {
						fwaitDist = fnewLen * 0.5;
					}
					float fdiff = (actorCurrent - TetherTargetPoint).XYLength();
					if (!(fwaitDist > static_cast<double>(fdiff))) {
						float fupdateLen_2 = fdiff - fwaitDist;
						UpdateTetherPosition(-fupdateLen_2);
					} else {
						float fupdateLen = fwaitDist - fdiff;
						UpdateTetherPosition(fupdateLen);
					}
					bool bfixupTether = 1;
					if (pActor->GetParentCell() && pActor->GetParentCell()->GetInterior()) {
						bfixupTether = 0;
					} else {
						if (pActor->GetWorldSpace()) {
							TESWorldSpace* WorldSpace = pActor->GetWorldSpace();
							if (WorldSpace->GetCellFromWorldCoord(TetherTargetPoint)) {
								bfixupTether = 0;
							}
						}
					}
					if (bfixupTether) {
						TetherTargetPoint = actorCurrent;
						LastTetherPoint = actorCurrent;
						eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
					} else {
						NiPoint3 tetherTangent;
						LastTetherPoint = actorCurrent;
						eState = DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING;
						newPathOrigin = TetherTargetPoint;
						bool bSetInitialTangent = 1;
						float v13 = fCurrentSplineT + 0.10000000149011612;
						spPath.m_pObject->ComputeLocationFromParameter(v13, tetherTangent);
						tetherTangent.operator-=(TetherTargetPoint);
						float finitialAngle = GetZAngleFromVector(tetherTangent);
						if (!ActorAvoidList.IsEmpty()) {
							BSSimpleList<Actor*>* plist = &ActorAvoidList;
							while (plist) {
								Actor* pavoidActor = plist->m_item;
								plist = plist->m_pkNext;
								float a2b = pavoidActor->GetRadius();
								const NiPoint3* afCurrentSplineParameter = &pavoidActor->GetLocationOnReference();
								const NiPoint3* v7 = &pActor->GetLocationOnReference();
								if (NavMeshMath::SegmentCircleIntersection(*v7, TetherTargetPoint,
										*afCurrentSplineParameter, a2b)) {
									newPathOrigin = pActor->GetLocationOnReference();
									bSetInitialTangent = 0;
									break;
								}
							}
						}
						{
							FNVD_CONSTRUCTED_LOCAL(PathingLocation, v17, &PathingLocation::Destruct, newPathOrigin,
								pActor);
							apRequest->SetOrigin(v17);
						}
						if (bSetInitialTangent) {
							spRequest.m_pObject->SetInitialPathHeading(finitialAngle);
						} else {
							spRequest.m_pObject->ClearInitialPathHeading();
						}
					}
				}
			}
		}
	} else {
		eState = DetailedActorPathHandler::UNK_0;
	}
}

bool DetailedActorPathHandler::IsWaitingForAvoidance() {
	return (bool)((uint8_t)(eAvoidState == 2));
}

bool DetailedActorPathHandler::IsMovementBlocked() {
	return (bool)((uint8_t)bMovementBlocked);
}

bool DetailedActorPathHandler::ComputePathPassesThroughPlayersArea() {
	if (!spPath.operator->()) {
		return 0;
	}
	PathingSolution* v1 = spPath.operator->();
	if (!v1->IsValid()) {
		return 0;
	}
	PathingSolution* v3 = spPath.operator->();
	uint32_t NodeCount = v3->GetNodeCount();
	uint32_t aiIndex = iCurrentNodeIndex + 1;
	bool v12 = 1;
	TESObjectCELL* pParentCell = pPlayer->GetParentCell();
	TESWorldSpace* pParentWorldspace = pPlayer->GetWorldSpace();
	if (!pParentCell) {
		pParentCell = TES::pInstance->GetInterior();
		v12 = pParentCell != 0;
	}
	if (!v12 && !pParentWorldspace) {
		pParentWorldspace = TES::pInstance->GetWorldSpace();
		if (!pParentWorldspace) {
			return 0;
		}
	}
	while (aiIndex < NodeCount) {
		PathingSolution* v4 = spPath.operator->();
		PathingNode* Node = v4->GetNode(aiIndex);
		if (Node) {
			PathingLocation* ActorPackageThatIsRunning = static_cast<PathingLocation*>(&Node->GetLocation());
			if (v12) {
				if (pParentCell == ActorPackageThatIsRunning->GetCell()) {
					return 1;
				}
			} else {
				if (pParentWorldspace == ActorPackageThatIsRunning->GetWorldspaceSimple()) {
					return 1;
				}
			}
			if (Node->IsTeleportDoorNode()) {
				TESObjectREFR* ConversationData = Node->pDoorRef;
				if (ConversationData) {
					DoorTeleportData* Teleport = ConversationData->GetTeleport();
					if (Teleport) {
						TESObjectREFR* v7 = Teleport->GetLinkedDoor();
						if (v7) {
							if (v12) {
								if (pParentCell == v7->GetParentCell()) {
									return 1;
								}
							} else {
								if (pParentWorldspace == v7->GetWorldSpace()) {
									return 1;
								}
							}
						}
					}
				}
			}
		}
		++aiIndex;
	}
	return 0;
}

// The exe's shared one-field getter of +0x60 (thiscall, ret 0, 0x5F5F80): EffectSetting's associated item and the
// first sequence's accumulation root read the same offset through it.
uint32_t DetailedActorPathHandler::GetMoveMode() {
	return MoveMode;
}

bool DetailedActorPathHandler::GetMoveDelta(NiPoint3& aDelta) {
	if (bDeltaComputedThisFrame) {
		bDeltaComputedThisFrame = 0;
		aDelta = MoveDelta;
		return 1;
	} else {
		aDelta = NiPoint3_ZERO;
		return 0;
	}
}

bool DetailedActorPathHandler::GetNextTetherPoint(NiPoint3& arPoint) {
	arPoint = TetherTargetPoint;
	return (bool)1;
}

void DetailedActorPathHandler::ForceMoveMode(uint32_t aiMoveMode) {
	MoveMode = (uint32_t)((int32_t)aiMoveMode);
}

// Takes over the old handler's move mode, blends, avoidance state and list, look-at timer and tether points.
void DetailedActorPathHandler::HandlePreviousPathTransition(ActorPathHandler* apOldHander) {
	if (!apOldHander || !apOldHander->IsDetailedPathHandler()) {
		return;
	}
	DetailedActorPathHandler* pold = static_cast<DetailedActorPathHandler*>(apOldHander);
	MoveMode = pold->MoveMode;
	eBlendRunning = pold->eBlendRunning;
	fRunWalkTime = pold->fRunWalkTime;
	fTimeInAnim = pold->fTimeInAnim;
	fAvoidanceDetectionValue = pold->fAvoidanceDetectionValue;
	if (pold->eAvoidState == DetailedActorPathHandler::PATH_AVOID_WAITING) {
		eAvoidState = DetailedActorPathHandler::PATH_AVOID_WAITING;
		fActorAvoidanceTimer = pold->fActorAvoidanceTimer;
		fWaitTime = pold->fWaitTime;
	} else if (pold->eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH) {
		eAvoidState = DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE;
		fActorAvoidanceTimer = pold->fActorAvoidanceTimer;
		fWaitTime = 0.0f;
		// The exe tests only the head's item, then passes the head item's address to AddHead (0x5AE3D0).
		while (pold->ActorAvoidList.GetItem()) {
			ActorAvoidList.AddHead(pold->ActorAvoidList.GetItem());
			pold->ActorAvoidList.RemoveHead();
		}
	}
	bIgnoreLookAtPoint = pold->bIgnoreLookAtPoint;
	fIgnoreLookAtPointTimer = pold->fIgnoreLookAtPointTimer;
	if (pold->bWaitingForUpdate) {
		bWaitingForUpdate = 1;
		LastTetherPoint = pold->LastTetherPoint;
		TetherTargetPoint = pold->TetherTargetPoint;
	}
}

void DetailedActorPathHandler::HandleDeletedReference(TESObjectREFR* apRef) {
	if (apRef == pCurrentDoor) {
		eDoorState = DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE_NONE;
		pCurrentDoor = 0;
	}
	if (apRef->IsActor()) {
		for (uint32_t i = 0; i < DetailedActorPathHandler::uiAvoidanceCandidatesSize; ++i) {
			if (DetailedActorPathHandler::AvoidanceCandidates.operator[](i) == apRef) {
				DetailedActorPathHandler::AvoidanceCandidates.RemoveAt(i);
				DetailedActorPathHandler::AvoidanceCandidates.Compact();
				--DetailedActorPathHandler::uiAvoidanceCandidatesSize;
			}
		}
		Actor* item = static_cast<Actor*>(apRef);
		if (ActorAvoidList.IsInList(item)) {
			Actor* v3 = static_cast<Actor*>(apRef);
			ActorAvoidList.Remove(v3);
			if (ActorAvoidList.IsEmpty()) {
				ResetActorAvoidance();
			}
		}
	}
}

void DetailedActorPathHandler::PrintCurrentStatus(IDebugText* apDebugText, DEBUGTEXT_LOCATION aLoc,
	DEBUGTEXT_JUSTIFY aJust) {
	char Src[264];
	char Dst[268];
	const NiPoint3* v4 = &pActor->GetLocationOnReference();
	apDebugText->PrintLine(aLoc, aJust, "DetailedPathHandler: Actor's Position %1.f %1.f %1.f", v4->x, v4->y, v4->z);
	BSstrcpy(Dst, 260, "Pathing State: ");
	switch (eState) {
	case DetailedActorPathHandler::DAP_INITIALIZING: {
		BSstrcat(Dst, 260, "Initializing");
		break;
	}
	case DetailedActorPathHandler::DAP_MOVING: {
		if (bRotateMode) {
			BSstrcat(Dst, 260, "Rotating");
		} else {
			if (bMovementBlocked) {
				BSstrcat(Dst, 260, "Static Avoidance");
			} else {
				BSstrcat(Dst, 260, "Moving");
			}
		}
		break;
	}
	case DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING: {
		BSstrcat(Dst, 260, "Moving While Waiting");
		break;
	}
	case DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH: {
		BSstrcat(Dst, 260, "Waiting For Updated Path");
		break;
	}
	case DetailedActorPathHandler::DAP_GOAL_REACHED: {
		BSstrcat(Dst, 260, "Goal Reached");
		break;
	}
	case DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH:
	case DetailedActorPathHandler::UNK_7: {
		BSstrcat(Dst, 260, "Rotating without path");
		break;
	}
	default: {
		BSstrcat(Dst, 260, "Invalid");
		break;
	}
	}
	apDebugText->PrintLine(aLoc, aJust, Dst);
	BSstrcpy(Dst, 260, "Actor Avoid State: ");
	switch (eAvoidState) {
	case DetailedActorPathHandler::PATH_AVOID_OK: {
		BSstrcat(Dst, 260, "OK");
		if (fAvoidanceDetectionValue > 0.0) {
			BSsprintf(Src, 260, " :detect %f", fAvoidanceDetectionValue);
			BSstrcat(Dst, 260, Src);
		}
		break;
	}
	case DetailedActorPathHandler::PATH_AVOID_WAITING: {
		BSstrcat(Dst, 260, "Waiting");
		break;
	}
	case DetailedActorPathHandler::PATH_AVOID_REPATH: {
		BSstrcat(Dst, 260, "Repathing (building)");
		break;
	}
	case DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE: {
		BSstrcat(Dst, 260, "Repath ignore");
		if (fAvoidanceDetectionValue > 0.0) {
			BSsprintf(Src, 260, " :detect %f", fAvoidanceDetectionValue);
			BSstrcat(Dst, 260, Src);
		}
		break;
	}
	case DetailedActorPathHandler::PATH_AVOID_FAILED: {
		BSstrcat(Dst, 260, "Failed");
		break;
	}
	default: {
		BSstrcat(Dst, 260, "Invalid");
		break;
	}
	}
	apDebugText->PrintLine(aLoc, aJust, Dst);
}

void DetailedActorPathHandler::ClearMovement() {
	MoveDelta = NiPoint3_ZERO;
	MoveMode = pActorMover->GetPreferredMoveModeType();
	fDistanceTraveled = (float)0.0;
	(&spPath)->operator=(nullptr);
	(&spRequest)->operator=(nullptr);
	bWaitingForUpdate = (bool)0;
	eDoorState = PATH_HANDLER_DOOR_STATE_NONE;
	pCurrentDoor = nullptr;
	eAvoidState = PATH_AVOID_OK;
	(&ActorAvoidList)->RemoveAll();
	eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
}

// The exe's cleanup-only C++ EH frame is left out.
bool DetailedActorPathHandler::WarpToNextNode() {
	// The exe constructs the filter by a call and never destroys it.
	FNVD_INLINED_LOCAL(DoorInPathFilter, filter, std::nullopt, std::nullopt);
	uint16_t ustri;
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	if (spRequest.m_pObject && spRequest.m_pObject->fActorRadius > 100.0) {
		return 0;
	}
	float fMinPathLength = 64.0f;
	float fMinPathLengthSqr = 64.0f * 64.0f;
	float fMaxWarpDistance = 128.0f;
	float fMaxWarpDistanceSqr = 128.0f * 128.0f;
	const NiPoint3* v3 = &pActor->GetLocationOnReference();
	float fdistToTether = (TetherTargetPoint - *v3).XYLength();
	float fdistanceFromGoalThresold = 0.0f;
	if (spPath.m_pObject->IsIncomplete()) {
		fdistanceFromGoalThresold = pActor->GetLength() * 0.6000000238418579;
	}
	if (!spPath.m_pObject->GetNodeCount()) {
		return 0;
	}
	if (fMaxWarpDistance > static_cast<double>(fdistToTether)) {
		float v8 = fMaxWarpDistance - fdistToTether;
		UpdateTetherPosition(v8);
	}
	const NiPoint3* v5 = &pActor->GetLocationOnReference();
	fdistToTether = (TetherTargetPoint - *v5).XYLength();
	PathingNode* LastNode = spPath.m_pObject->GetLastNode();
	float fdistFromGoal = ((&LastNode->Location)->GetLocation() - TetherTargetPoint).XYLength();
	if (fMinPathLength > static_cast<double>(fdistToTether) ||
		fdistanceFromGoalThresold > static_cast<double>(fdistFromGoal)) {
		return 0;
	}
	::new (&filter) DoorInPathFilter(0);
	FNVD_CONSTRUCTED_LOCAL(PathingLocation, actorLoc, &PathingLocation::Destruct, pActor);
	actorLoc.ResolveNavMeshInfo(0);
	::new (&spmesh) NavMeshPtr;
	if (actorLoc.GetNavMeshAndTriangle(spmesh, ustri)) {
		NavMeshTriangle* ptri = spmesh.m_pObject->GetTriangle(ustri);
		if (ptri) {
			filter.bEntirePathInDoor = ptri->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
		}
	}
	Pathing::CheckLineOfSight(actorLoc, TetherTargetPoint, 0, &filter);
	if (filter.bEnterTriWithDoor || (filter.bLeaveTriWithDoor || filter.bEntirePathInDoor)) {
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	} else {
		pActorMover->MoveActorToLocation(TetherTargetPoint);
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 1;
	}
}

void DetailedActorPathHandler::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(&MoveDelta, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&BlockedTargetPoint, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&TetherTargetPoint, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&LastLocation, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&LastTetherPoint, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&MoveMode, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCurrentNodeIndex, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fRotateGoal, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fPathLength, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fCurrentSplineT, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fActorAvoidanceTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fAvoidanceDetectionValue, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iUnblockDirection, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fAlternateFacingAngle, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fBlockedStartAngle, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fBlockTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTimeInAnim, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fRunWalkTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fMinRotateTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fWaitTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fLastFrameMinMoveDelta, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fGoalRadiusSqr, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fFallTime, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fDistanceTraveled, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fIgnoreLookAtPointTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTetherMinMove, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTetherDistFromActor, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&eBlendRunning, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&eAvoidState, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&eDoorState, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bRotateMode, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bRotateCCW, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bMovementBlocked, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bEndUpdatePath, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bWaitingForUpdate, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bDeltaComputedThisFrame, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIgnoreLookAtPoint, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIgnoreInitialRotation, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bIgnoreInitialRotationKeepActorHeading, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&fIgnoreInitialRotationTetherParam, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&LastFrameMinMoveDelta, 12u, 0);
	apSaveGameBuffer->SaveDataEndian(&fLastRotationApplied, 4u, 0);
	apSaveGameBuffer->SaveFormID(pCurrentDoor, 0);
	uint32_t isaveGameCount = 0;
	uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
	for (Actor* pForm : ActorAvoidList) {
		if (pForm) {
			apSaveGameBuffer->SaveFormID(pForm, 0);
			++isaveGameCount;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void DetailedActorPathHandler::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor* pForm;
	apLoadGameBuffer->LoadData(&MoveDelta, 12u);
	apLoadGameBuffer->LoadData(&BlockedTargetPoint, 12u);
	apLoadGameBuffer->LoadData(&TetherTargetPoint, 12u);
	apLoadGameBuffer->LoadData(&LastLocation, 12u);
	apLoadGameBuffer->LoadData(&LastTetherPoint, 12u);
	apLoadGameBuffer->LoadData(&MoveMode, 4u);
	apLoadGameBuffer->LoadData(&iCurrentNodeIndex, 4u);
	apLoadGameBuffer->LoadData(&fRotateGoal, 4u);
	apLoadGameBuffer->LoadData(&fPathLength, 4u);
	apLoadGameBuffer->LoadData(&fCurrentSplineT, 4u);
	apLoadGameBuffer->LoadData(&fActorAvoidanceTimer, 4u);
	apLoadGameBuffer->LoadData(&fAvoidanceDetectionValue, 4u);
	apLoadGameBuffer->LoadData(&iUnblockDirection, 4u);
	apLoadGameBuffer->LoadData(&fAlternateFacingAngle, 4u);
	apLoadGameBuffer->LoadData(&fBlockedStartAngle, 4u);
	apLoadGameBuffer->LoadData(&fBlockTime, 4u);
	apLoadGameBuffer->LoadData(&fTimeInAnim, 4u);
	apLoadGameBuffer->LoadData(&fRunWalkTime, 4u);
	apLoadGameBuffer->LoadData(&fMinRotateTime, 4u);
	apLoadGameBuffer->LoadData(&fWaitTime, 4u);
	apLoadGameBuffer->LoadData(&fLastFrameMinMoveDelta, 4u);
	apLoadGameBuffer->LoadData(&fGoalRadiusSqr, 4u);
	apLoadGameBuffer->LoadData(&fFallTime, 4u);
	apLoadGameBuffer->LoadData(&fDistanceTraveled, 4u);
	apLoadGameBuffer->LoadData(&fIgnoreLookAtPointTimer, 4u);
	if (apLoadGameBuffer->GetVersion() >= 4u) {
		apLoadGameBuffer->LoadData(&fTetherMinMove, 4u);
		apLoadGameBuffer->LoadData(&fTetherDistFromActor, 4u);
	}
	apLoadGameBuffer->LoadData(&eState, 4u);
	apLoadGameBuffer->LoadData(&eBlendRunning, 4u);
	apLoadGameBuffer->LoadData(&eAvoidState, 4u);
	apLoadGameBuffer->LoadData(&eDoorState, 4u);
	apLoadGameBuffer->LoadData(&bRotateMode, 1u);
	apLoadGameBuffer->LoadData(&bRotateCCW, 1u);
	apLoadGameBuffer->LoadData(&bMovementBlocked, 1u);
	apLoadGameBuffer->LoadData(&bEndUpdatePath, 1u);
	apLoadGameBuffer->LoadData(&bWaitingForUpdate, 1u);
	apLoadGameBuffer->LoadData(&bDeltaComputedThisFrame, 1u);
	apLoadGameBuffer->LoadData(&bIgnoreLookAtPoint, 1u);
	if (apLoadGameBuffer->GetVersion() >= 12u) {
		apLoadGameBuffer->LoadData(&bIgnoreInitialRotation, 1u);
		apLoadGameBuffer->LoadData(&bIgnoreInitialRotationKeepActorHeading, 1u);
		apLoadGameBuffer->LoadData(&fIgnoreInitialRotationTetherParam, 4u);
	}
	if (apLoadGameBuffer->GetVersion() >= 18u) {
		apLoadGameBuffer->LoadData(&LastFrameMinMoveDelta, 12u);
		apLoadGameBuffer->LoadData(&fLastRotationApplied, 4u);
	}
	apLoadGameBuffer->LoadFormID(pCurrentDoor);
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
		pForm = 0;
		apLoadGameBuffer->LoadFormID(pForm);
		ActorAvoidList.AddHead(pForm);
	}
}

void DetailedActorPathHandler::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR* v5;
	if (pCurrentDoor) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentDoor)));
		v5 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v5 = 0;
	}
	pCurrentDoor = v5;
	BSSimpleList<Actor*>* plist = &ActorAvoidList;
	BSSimpleList<Actor*>* plastList = 0;
	while (plist && !plist->IsEmpty()) {
		Actor* pForm = plist->m_item;
		bool bremove = 0;
		if (pForm) {
			TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
			pForm = dynamic_cast<Actor*>(v3);
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
				BSSimpleList<Actor*>* v4 = plist;
				plastList->Remove(v4->m_item);
				plist = plastList->m_pkNext;
			} else {
				plist->RemoveHead();
			}
		} else {
			plastList = plist;
			plist = plist->m_pkNext;
		}
	}
	if (pCurrentDoor) {
		pCurrentDoor->SetTargeted(1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool DetailedActorPathHandler::IsDoorInPath(uint32_t aDoorID) {
	NiPoint3 thisPoint;
	// The exe constructs the filters by a call and never destroys them.
	FNVD_INLINED_LOCAL(DoorInPathFilter, filter_2, std::nullopt, std::nullopt);
	uint16_t ustri_2;
	NiPoint3 nextPoint;
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh_2, std::nullopt, std::nullopt);
	uint16_t ustri;
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(DoorInPathFilter, filter, std::nullopt, std::nullopt);
	if (spPath.m_pObject) {
		if (spPath.m_pObject->IsDoorInPath(aDoorID)) {
			return 1;
		} else {
			for (float fparam = 0.0f;; fparam = fparam + 0.5) {
				double v3 = static_cast<double>(spPath.m_pObject->GetNodeCount()) - 1.0;
				if (!(fparam < v3)) {
					break;
				}
				float fnextParam = fparam + 0.5;
				spPath.m_pObject->ComputeLocationFromParameter(fparam, thisPoint);
				spPath.m_pObject->ComputeLocationFromParameter(fnextParam, nextPoint);
				::new (&filter_2) DoorInPathFilter(aDoorID);
				FNVD_CONSTRUCTED_LOCAL(PathingLocation, start, &PathingLocation::Destruct, thisPoint, pActor);
				start.ResolveNavMeshInfo(0);
				::new (&spmesh_2) NavMeshPtr;
				if (start.GetNavMeshAndTriangle(spmesh_2, ustri_2)) {
					NavMeshTriangle* ptri_2 = spmesh_2.m_pObject->GetTriangle(ustri_2);
					if (ptri_2) {
						filter_2.bEntirePathInDoor = ptri_2->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
					}
				}
				Pathing::CheckLineOfSight(start, nextPoint, static_cast<PathingLocation*>(0), &filter_2);
				if (filter_2.bEnterTriWithDoor || (filter_2.bLeaveTriWithDoor || filter_2.bEntirePathInDoor)) {
					FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh_2);
					return 1;
				}
				FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh_2);
			}
			return 0;
		}
	} else {
		::new (&filter) DoorInPathFilter(aDoorID);
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, actorLoc, &PathingLocation::Destruct, pActor);
		actorLoc.ResolveNavMeshInfo(0);
		::new (&spmesh) NavMeshPtr;
		if (actorLoc.GetNavMeshAndTriangle(spmesh, ustri)) {
			NavMeshTriangle* ptri = spmesh.m_pObject->GetTriangle(ustri);
			if (ptri) {
				filter.bEntirePathInDoor = ptri->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
			}
		}
		Pathing::CheckLineOfSight(actorLoc, TetherTargetPoint, static_cast<PathingLocation*>(0), &filter);
		if (filter.bEnterTriWithDoor || (filter.bLeaveTriWithDoor || filter.bEntirePathInDoor)) {
			FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
			return 1;
		} else {
			FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
			return 0;
		}
	}
}

bool DetailedActorPathHandler::InitRotationMode(float afGoalAngle, bool abForceTurn, bool abPathTransition) {
	float fdir;
	if (bIgnoreInitialRotation) {
		return 0;
	}
	if (!(afGoalAngle < 0.0)) {
		if (afGoalAngle >= 6.2831854820251465) {
			afGoalAngle = std::fmod(afGoalAngle, 6.2831855f);
		}
	} else {
		float v6 = afGoalAngle + 6.2831854820251465;
		afGoalAngle = std::fmod(v6, 6.2831855f);
	}
	float afStartAngle = pActor->GetHeading(0);
	float fangledelta = GetAngleDelta(afStartAngle, afGoalAngle, fdir);
	float fabsoluteDelta = std::fabs(fangledelta);
	float fepsilon = 0.017453292f;
	if (abForceTurn) {
		fepsilon = 0.017453292f;
	} else {
		if (abPathTransition && (pActorMover->GetPreferredMoveModeType() & ActorMover::MF_RUNNING) != 0) {
			fepsilon = 0.5235988f;
		}
	}
	if (!(fepsilon < static_cast<double>(fabsoluteDelta))) {
		return 0;
	}
	bRotateCCW = fdir > 0.0;
	bRotateMode = 1;
	fRotateGoal = afGoalAngle;
	BSWrap(fRotateGoal, -3.1415927f, 3.1415927f);
	if (!pActor->GetTurretBehavior()) {
		fMinRotateTime = fActorTurnAnimMinTime.Float();
	}
	pActorMover->DebugOnEvent();
	return 1;
}

// The turn is limited to afDeltaT times 1.3089969158172607 (a double constant, 75 degrees in radians). The exe calls
// the float fabs (0x408840) out of line and keeps the smaller turn only when the limit is below it.
void DetailedActorPathHandler::UpdateLooking(float afDeltaT, float afLookingAngle) {
	float fLooking = pActor->GetLooking();
	float fMaxTurn = afDeltaT * 1.3089969158172607;
	float fDir = 0.0f;
	float fTurn = std::fabs(GetAngleDelta(fLooking, afLookingAngle, fDir));
	float fApplied = fTurn;
	if (fMaxTurn < fApplied) {
		fApplied = fMaxTurn;
	}
	fLooking = fApplied * fDir + fLooking;
	pActor->SetLooking(fLooking);
}

void DetailedActorPathHandler::ComputeSimpleMoveDelta(float afDeltaT) {
	if (!pActor->GetAnimationMoveDelta(MoveDelta)) {
		MoveDelta.y = (float)(pActor->GetCurrentSpeed() * afDeltaT);
	}
}

void DetailedActorPathHandler::ResetStaticAvoidance() {
	bMovementBlocked = (bool)0;
	fBlockTime = (float)0.0;
}

void DetailedActorPathHandler::ResetActorAvoidance() {
	eAvoidState = PATH_AVOID_OK;
	ActorAvoidList.RemoveAll();
	fAvoidanceDetectionValue = 0.0f;
	fActorAvoidanceTimer = 0.0f;
}

bool DetailedActorPathHandler::CheckForEndRotate() {
	if (!spRequest->GetFaceTargetAtGoal()) {
		return false;
	}
	if (pActorMover->iNextNode + 1 != (int32_t)spPath->GetVirtualNodeCount() ||
		pActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING || pActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
		return false;
	}
	InitRotationMode(spRequest->GetGoalAngleToFace(), false, false);
	return bRotateMode;
}

// The exe calls ActorMover::GetPreferredMoveModeType (0x9DD130) out of line; src's is the inline header getter, a
// transparent function.
void DetailedActorPathHandler::ResetMoveModeAndDelta() {
	MoveMode = pActorMover->GetPreferredMoveModeType();
	MoveDelta = NiPoint3_ZERO;
	fLastFrameMinMoveDelta = 0.0f;
	LastFrameMinMoveDelta = NiPoint3_ZERO;
}

// Collects the actors in the way (BuildAvoidanceList) and weighs each one's detection value into
// fAvoidanceDetectionValue. A moving actor makes a waiting handler stop waiting (1); once the detection value reaches
// fAvoidanceDetectionTime the handler waits (1) when it should wait for one of them, or else repaths around them with
// an avoid node array (2). An actor standing on the path's goal fails a request that may not end on an occupied goal
// (3) and otherwise softens its goal radius. Without anything to avoid the detection value decays. The exe's
// cleanup-only EH frame and its stack cookie are left out; the locals are constructed and destroyed where the exe does
// it, on typed storage for which src runs no C++ constructor or destructor.
DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN DetailedActorPathHandler::UpdateActorAvoidance(float afDeltaT) {
	NiAVObject* p3D = pActor->Get3D();
	if (!p3D || !p3D->QFadedIn())
		return PATH_ACTOR_AVOID_RETURN_OK;
	if (p3D->GetNotVisible()) {
		// The exe keeps the distance as a double across the setting read.
		double dLODDistance = static_cast<bhkCharacterController*>(pActor->GetCharController())->GetLODDistance();
		if (fLODActorAvoidanceDistance.Float() < dLODDistance)
			return PATH_ACTOR_AVOID_RETURN_OK;
	}
	// The exe reads the move delta's x and y as a NiPoint2 (NiPoint2::SqrLength on the NiPoint3's address).
	if (eAvoidState != PATH_AVOID_WAITING && reinterpret_cast<NiPoint2*>(&MoveDelta)->SqrLength() < 0.25)
		return PATH_ACTOR_AVOID_RETURN_OK;
	if (eAvoidState != PATH_AVOID_REPATH_IGNORE && eAvoidState != PATH_AVOID_REPATH) {
		ActorAvoidList.RemoveAll();
	}
	Actor* pFollowLeader = GetFollowLeader(pActor);
	FNVD_INLINED_LOCAL(ActorAvoidanceArray, kCandidates, std::nullopt, std::nullopt);
	::new (&kCandidates) ActorAvoidanceArray;
	NiPoint3 kMoveDelta = MoveDelta;
	if (eAvoidState == PATH_AVOID_WAITING) {
		NiPoint3 kForward;
		kMoveDelta = *::new (&kForward) NiPoint3(0.0f, 1.0f, 0.0f);
	}
	BuildAvoidanceList(pActor, spRequest.operator->(), MoveMode, kMoveDelta, this, kCandidates);
	FNVD_INLINED_LOCAL(BSSimpleArray<Actor*>, kActors, std::nullopt, std::nullopt);
	::new (&kActors) BSSimpleArray<Actor*>;
	// Whether to wait for an actor rather than repath, and whether the player was added.
	uint8_t bWait = 0;
	uint8_t bPlayerAdded = 0;
	for (uint32_t i = 0; i < kCandidates.GetSize(); ++i) {
		Actor* pOther = kCandidates.GetAt(i).pActor;
		float fDetection = kCandidates.GetAt(i).fDetection;
		if (pOther == pPlayer) {
			if (pPlayer == pFollowLeader)
				continue;
			if (spPath.operator->()) {
				FNVD_INLINED_LOCAL(PathingLocation, kGoal, std::nullopt, std::nullopt);
				::new (&kGoal) PathingLocation(spPath->GetLastNode()->GetLocation());
				float fRadius = pPlayer->GetRadius();
				bool bClose;
				{
					FNVD_CONSTRUCTED_LOCAL(PathingLocation, kPlayerLocation, &PathingLocation::Destruct, pPlayer);
					bClose = kGoal.IsCloseTo(kPlayerLocation, fRadius);
				}
				if (bClose) {
					if (spRequest->GetFailIfGoalOccupied()) {
						kGoal.~PathingLocation();
						kActors.~BSSimpleArray<Actor*>();
						kCandidates.~ActorAvoidanceArray();
						return PATH_ACTOR_AVOID_RETURN_FAIL;
					}
					spRequest->SetSoftGoalRadius(false);
				}
				kGoal.~PathingLocation();
			}
			fAvoidanceDetectionValue = fDetection * afDeltaT + fAvoidanceDetectionValue;
			Actor* pPlayerActor = pPlayer;
			kActors.Add(pPlayerActor);
			bPlayerAdded = 1;
			continue;
		}
		uint8_t bMoving = 0;
		if (pOther->IsMoving() && !pOther->IsWaitingForAvoidance())
			bMoving = 1;
		FNVD_INLINED_LOCAL(PathingLocation, kOtherGoal, std::nullopt, std::nullopt);
		::new (&kOtherGoal) PathingLocation;
		if (pOther->GetCurrentPathfindingGoal(&kOtherGoal)) {
			// An actor already heading to this actor's location is not in the way.
			float fRadius = spRequest->GetActorRadius();
			bool bClose;
			{
				FNVD_CONSTRUCTED_LOCAL(PathingLocation, kLocation, &PathingLocation::Destruct, pActor);
				bClose = kOtherGoal.IsCloseTo(kLocation, fRadius);
			}
			if (bClose)
				bMoving = 0;
		}
		if (bMoving) {
			if (eAvoidState == PATH_AVOID_WAITING) {
				kOtherGoal.~PathingLocation();
				kActors.~BSSimpleArray<Actor*>();
				kCandidates.~ActorAvoidanceArray();
				return PATH_ACTOR_AVOID_RETURN_WAIT;
			}
			fAvoidanceDetectionValue = fDetection * afDeltaT + fAvoidanceDetectionValue;
			if ((MoveMode & 0x200) != 0) {
				if ((pOther->GetMoveMode() & 0x200) != 0)
					bWait = 1;
				else
					kActors.Add(pOther);
			} else if (!pActor->HasFollower(pOther)) {
				bWait = 1;
				if (fAvoidanceDetectionTime.Float() <= fAvoidanceDetectionValue) {
					kOtherGoal.~PathingLocation();
					break;
				}
			}
		} else {
			if (eAvoidState == PATH_AVOID_WAITING) {
				kOtherGoal.~PathingLocation();
				continue;
			}
			fAvoidanceDetectionValue = fDetection * 2.0 * afDeltaT + fAvoidanceDetectionValue;
			if (pFollowLeader && pOther == pFollowLeader)
				bWait = 1;
			if (spPath.operator->()) {
				FNVD_INLINED_LOCAL(PathingLocation, kGoal, std::nullopt, std::nullopt);
				::new (&kGoal) PathingLocation(spPath->GetLastNode()->GetLocation());
				float fRadius = pOther->GetRadius();
				bool bClose;
				{
					FNVD_CONSTRUCTED_LOCAL(PathingLocation, kOtherLocation, &PathingLocation::Destruct, pOther);
					bClose = kGoal.IsCloseTo(kOtherLocation, fRadius);
				}
				if (bClose) {
					if (spRequest->GetFailIfGoalOccupied()) {
						kGoal.~PathingLocation();
						kOtherGoal.~PathingLocation();
						kActors.~BSSimpleArray<Actor*>();
						kCandidates.~ActorAvoidanceArray();
						return PATH_ACTOR_AVOID_RETURN_FAIL;
					}
					spRequest->SetSoftGoalRadius(false);
				}
				kGoal.~PathingLocation();
			}
			kActors.Add(pOther);
		}
		kOtherGoal.~PathingLocation();
	}
	if (eAvoidState == PATH_AVOID_WAITING) {
		kActors.~BSSimpleArray<Actor*>();
		kCandidates.~ActorAvoidanceArray();
		return PATH_ACTOR_AVOID_RETURN_OK;
	}
	if (fAvoidanceDetectionValue == 0.0) {
		kActors.~BSSimpleArray<Actor*>();
		kCandidates.~ActorAvoidanceArray();
		return PATH_ACTOR_AVOID_RETURN_OK;
	}
	if (fAvoidanceDetectionTime.Float() <= fAvoidanceDetectionValue) {
		if (bWait) {
			fWaitTime = fAvoidanceDefaultWaitTime.Float();
			fActorAvoidanceTimer = fAvoidanceMinWaitTime.Float();
			pActorMover->DebugOnEvent();
			kActors.~BSSimpleArray<Actor*>();
			kCandidates.~ActorAvoidanceArray();
			return PATH_ACTOR_AVOID_RETURN_WAIT;
		}
		if (!kActors.IsEmpty()) {
			if (eAvoidState == PATH_AVOID_REPATH_IGNORE || bPlayerAdded || (MoveMode & 0x200) != 0) {
				float fIgnoreMinTime = fAvoidanceIgnoreMinTime.Float();
				fActorAvoidanceTimer = BSMAX(fActorAvoidanceTimer, fIgnoreMinTime);
			} else {
				fActorAvoidanceTimer = fAvoidanceIgnoreTime.Float();
			}
			void* pMemory = NiMemObject::operator new(sizeof(PathingAvoidNodeArray));
			PathingAvoidNodeArray* pAvoidNodes = pMemory ? ::new (pMemory) PathingAvoidNodeArray : nullptr;
			uint32_t uiCount = kActors.GetSize();
			for (uint32_t j = 0; j < uiCount; ++j) {
				ActorAvoidList.AddHead(kActors.GetAt(j));
			}
			float fRadiusScale = (MoveMode & 0x200) != 0 ? 1.75f : 1.5f;
			Actor* pAvoidActor = nullptr;
			BSSimpleList<Actor*>* pNode = &ActorAvoidList;
			float fActorRadius = spRequest->GetActorRadius();
			while (pNode) {
				pAvoidActor = pNode->GetItem();
				pNode = pNode->GetNext();
				// An actor within reach gets a wider node.
				float fNearScale = 1.0f;
				const NiPoint3* pLocation = &pActor->GetLocationOnReference();
				float fDistance = (pAvoidActor->GetLocationOnReference() - *pLocation).Length();
				if (fDistance < pAvoidActor->GetRadius() + fActorRadius)
					fNearScale = 1.5f;
				float fAvoidRadius;
				PathingAvoidNode::AVOID_NODE_TYPE eType;
				if (pAvoidActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
					pAvoidActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
					fAvoidRadius = static_cast<float>(pAvoidActor->GetRadius() * 0.5 * fRadiusScale);
					eType = PathingAvoidNode::AVOID_NODE_SPHERE;
				} else {
					fAvoidRadius = pAvoidActor->GetRadius() * fRadiusScale * fNearScale;
					eType = PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR;
				}
				// A moving actor also gets a node where its move takes it within a second.
				NiMatrix3 kRotation;
				kRotation.~NiMatrix3();
				::new (&kRotation) NiMatrix3;
				kRotation.MakeZRotationQ(pAvoidActor->GetHeading(false));
				NiPoint3 kMove = kRotation * *pAvoidActor->GetActorMoverOverrideMovement();
				NiPoint3 kVelocity = kMove / appTimer.GetSeconds();
				double dSpeedSqr = kVelocity.SqrLength();
				if (pAvoidActor->GetRadius() < dSpeedSqr) {
					NiPoint3 kAhead = pAvoidActor->GetLocationOnReference() + kVelocity;
					PathingAvoidNode kAheadNode;
					pAvoidNodes->Add(*FNVD_RECONSTRUCT(PathingAvoidNode)(&kAheadNode, kAhead, fAvoidRadius, 2.0f,
						eType));
				}
				// The node sits at the character proxy's position when the actor collides with characters.
				NiPoint3 kPosition = pAvoidActor->GetLocationOnReference();
				bhkCharacterController* pController =
					static_cast<bhkCharacterController*>(pAvoidActor->GetCharController());
				if (pController &&
					!pController->bhkCharacterListener::GetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS)) {
					// SWEEP-INLINE 0x4A3C90
					alignas(16) hkVector4 kProxyPosition = pController->GethkPosition();
					NiPoint3 kNiPosition;
					kNiPosition.~NiPoint3();
					::new (&kNiPosition) NiPoint3;
					HK2NI(kNiPosition, kProxyPosition);
					kPosition.x = kNiPosition.x;
					kPosition.y = kNiPosition.y;
				}
				PathingAvoidNode kNode;
				pAvoidNodes->Add(*FNVD_RECONSTRUCT(PathingAvoidNode)(&kNode, kPosition, fAvoidRadius,
					fAvoidanceAvoidNodeCost.Float(), eType));
			}
			{
				FNVD_CONSTRUCTED_LOCAL(PathingLocation, kOrigin, &PathingLocation::Destruct, pActor);
				spRequest->SetOrigin(kOrigin);
			}
			spRequest->SetAvoidNodeArray(pAvoidNodes);
			pActorMover->DebugOnEvent();
			kActors.~BSSimpleArray<Actor*>();
			kCandidates.~ActorAvoidanceArray();
			return PATH_ACTOR_AVOID_RETURN_REPATH;
		}
	}
	if (!bWait && kActors.IsEmpty()) {
		fAvoidanceDetectionValue = fAvoidanceDetectionValue - afDeltaT;
		if (fAvoidanceDetectionValue < 0.0)
			fAvoidanceDetectionValue = 0.0f;
	}
	kActors.~BSSimpleArray<Actor*>();
	kCandidates.~ActorAvoidanceArray();
	return PATH_ACTOR_AVOID_RETURN_OK;
}

bool DetailedActorPathHandler::ComputeMinTetherDistance() {
	fTetherMinMove = 10.0f;
	fTetherDistFromActor = 20.0f;
	fTetherDistFromActor = spRequest->GetActorRadius();
	fTetherMinMove = float(fTetherDistFromActor * 0.5);
	return false;
}

DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE DetailedActorPathHandler::HandleDoorOpening() {
	switch (eDoorState) {
	case PATH_HANDLER_DOOR_STATE_NONE:
		if (pActorMover->DetectionDoorID) {
			TESForm* pForm = TESForm::GetFormByNumericID(pActorMover->DetectionDoorID);
			if (pForm && pForm->IsReference()) {
				TESObjectREFR* pDoor = static_cast<TESObjectREFR*>(pForm);
				if (BGSOpenCloseForm::GetOpenState(pDoor) != BGSOpenCloseForm::OBJECT_CLOSED) {
					return PATH_HANDLER_DOOR_STATE_NONE;
				}
				if (!spRequest->GetCantOpenDoors() && IsDoorInPath(pDoor->GetFormID())) {
					if (pDoor->Activate(pActor, false, nullptr, 1)) {
						pCurrentDoor = pDoor;
						pCurrentDoor->SetTargeted(true);
						eDoorState = PATH_HANDLER_DOOR_STATE_WAITING_FOR_DOOR_TO_OPEN;
					} else {
						eDoorState = PATH_HANDLER_DOOR_STATE_FAILED;
					}
				}
				return eDoorState;
			}
		}
		eDoorState = PATH_HANDLER_DOOR_STATE_NONE;
		return eDoorState;
	case PATH_HANDLER_DOOR_STATE_WAITING_FOR_DOOR_TO_OPEN: {
		if (!pCurrentDoor) {
			eDoorState = PATH_HANDLER_DOOR_STATE_FAILED;
			return eDoorState;
		}
		BGSOpenCloseForm::OPEN_STATE eState = BGSOpenCloseForm::GetOpenState(pCurrentDoor);
		if (eState == BGSOpenCloseForm::OBJECT_OPEN) {
			pCurrentDoor = nullptr;
			eDoorState = PATH_HANDLER_DOOR_STATE_NONE;
			return eDoorState;
		}
		if (eState == BGSOpenCloseForm::OBJECT_OPENING) {
			return PATH_HANDLER_DOOR_STATE_WAITING_FOR_DOOR_TO_OPEN;
		}
		return PATH_HANDLER_DOOR_STATE_NONE;
	}
	case PATH_HANDLER_DOOR_STATE_FAILED:
		return PATH_HANDLER_DOOR_STATE_FAILED;
	default:
		eDoorState = PATH_HANDLER_DOOR_STATE_NONE;
		return eDoorState;
	}
}

float DetailedActorPathHandler::UpdateRotation(float afDeltaT) {
	float factangle = pActor->GetHeading(0);
	if (pActor->CanMove()) {
		BSWrap(factangle, -3.1415927f, 3.1415927f);
		float fangleDelta = fRotateGoal - factangle;
		BSWrap(fangleDelta, -3.1415927f, 3.1415927f);
		if (!(std::fabs(fangleDelta) > 0.01745329238474369)) {
			factangle = fRotateGoal;
			if (fMinRotateTime <= 0.0) {
				bRotateMode = 0;
				pActorMover->DebugOnEvent();
			}
		} else {
			float v8;
			float v7 = pActor->GetTurningSpeed() * 0.01745329238474369 * afDeltaT;
			if (pActor->IsCreature()) {
				float v4;
				if (pActor->IsInCombat()) {
					v4 = gs_fCreatureCombatTurnSpeedScale_Pathfinding.Float();
				} else {
					v4 = gs_fCreatureTurnSpeedScale_Pathfinding.Float();
				}
				v8 = v7 * v4;
			} else {
				float v5;
				if (pActor->IsInCombat()) {
					v5 = gs_fAICombatTurnSpeedScale_Pathfinding.Float();
				} else {
					v5 = gs_fAITurnSpeedScale_Pathfinding.Float();
				}
				v8 = v7 * v5;
			}
			if (!(static_cast<double>(v8) > std::fabs(fangleDelta))) {
				if (bRotateCCW) {
					MoveMode = pActorMover->GetPreferredMoveModeType() | 32;
					factangle = factangle + v8;
				} else {
					MoveMode = pActorMover->GetPreferredMoveModeType() | 16;
					factangle = factangle - v8;
				}
			} else {
				factangle = fRotateGoal;
				if (fMinRotateTime <= 0.0) {
					bRotateMode = 0;
					pActorMover->DebugOnEvent();
				}
			}
		}
		fMinRotateTime = fMinRotateTime - afDeltaT;
		return factangle;
	} else {
		MoveMode = pActorMover->GetPreferredMoveModeType();
		return factangle;
	}
}

bool DetailedActorPathHandler::ComputeMoveMode(float afDeltaT) {
	NiPoint3 ecx0;
	if (pActor->IsRestrained() || pActor->IsUnConscious()) {
		this->MoveMode = 0;
		return 1;
	}
	uint32_t MoveMode = this->MoveMode;
	if (bIgnoreLookAtPoint) {
		fIgnoreLookAtPointTimer = fIgnoreLookAtPointTimer - afDeltaT;
		if (fIgnoreLookAtPointTimer < 0.0) {
			bIgnoreLookAtPoint = 0;
			fTimeInAnim = 1.0;
		}
	}
	uint32_t PreferredMoveModeType = pActorMover->GetPreferredMoveModeType();
	uint32_t v23 = this->MoveMode & 63;
	if (pActor->IsSwimming() && pActor->IsCreature()) {
		this->MoveMode = PreferredMoveModeType | 1;
		return 1;
	}
	if (!bIgnoreLookAtPoint && (pActorMover->HasLookAtTarget() || bIgnoreInitialRotation)) {
		double v6;
		float z;
		NiPoint3 result;
		float ZAngleFromVector;
		if (pActorMover->HasLookAtTarget()) {
			pActorMover->GetLookAtPoint(ecx0);
			const NiPoint3* v3 = &pActor->GetLocationOnReference();
			ecx0.operator-=(*v3);
			ZAngleFromVector = GetZAngleFromVector(ecx0);
		} else {
			if (bIgnoreInitialRotationKeepActorHeading || !spPath.operator->()) {
				ecx0 = NiPoint3_ZERO;
				ZAngleFromVector = pActor->GetHeading(0);
			} else {
				float X_4 = fIgnoreInitialRotationTetherParam;
				PathingSolution* v4 = spPath.operator->();
				v4->ComputeLocationFromParameter(X_4, ecx0);
				const NiPoint3* v5 = &pActor->GetLocationOnReference();
				ecx0.operator-=(*v5);
				ZAngleFromVector = GetZAngleFromVector(ecx0);
			}
		}
		NiPoint3 X = TetherTargetPoint;
		NiPoint3 v25 = pActor->GetLocationOnReference();
		result = CombatUtilities::GetAngleToProjectedPoint(v25, X);
		if (pActor->IsSwimming() &&
			(v6 = std::fabs(result.x), std::fabs(v6 - 1.5707963267948966) < 0.2617993950843811)) {
			z = ZAngleFromVector;
		} else {
			z = result.z;
		}
		float afOutDir = 1.0;
		float v16 = ClampAngle(z);
		float AngleDelta = GetAngleDelta(v16, ZAngleFromVector, afOutDir);
		if (!(fTimeInAnim > 0.25)) {
			fTimeInAnim = fTimeInAnim + afDeltaT;
		} else {
			if (!(std::fabs(AngleDelta) > 0.01745329238474369)) {
				v23 = pActorMover->GetPreferredMoveMode() & 63;
			} else {
				Animation* v7;
				double v8;
				float v14 = AngleDelta * 57.2957763671875;
				if (!(std::fabs(v14) > 135.0) ||
					((v7 = pActor->GetAnimation(), !v7->GroupLoaded(4u)) ||
						((v8 = std::fabs(result.x), !(std::fabs(v8 - 1.5707963267948966) >= 0.2617993950843811)) &&
							pActor->IsSwimming()))) {
					if (!(std::fabs(v14) > 45.0)) {
						v23 = 1;
					} else {
						if (!(afOutDir > 0.0)) {
							Animation* v10;
							if (!pActor->IsCreature() || (v10 = pActor->GetAnimation(), v10->GroupLoaded(6u))) {
								v23 = 8;
							} else {
								v23 = 1;
								bIgnoreLookAtPoint = 1;
								fIgnoreLookAtPointTimer = 1.0;
							}
						} else {
							Animation* v9;
							if (!pActor->IsCreature() || (v9 = pActor->GetAnimation(), v9->GroupLoaded(5u))) {
								v23 = 4;
							} else {
								v23 = 1;
								bIgnoreLookAtPoint = 1;
								fIgnoreLookAtPointTimer = 1.0;
							}
						}
					}
				} else {
					v23 = 2;
				}
			}
		}
	} else {
		v23 = pActorMover->GetPreferredMoveMode() & 63;
	}
	DetailedActorPathHandler::PATH_RUNNING_BLEND eBlendRunning = this->eBlendRunning;
	if (eBlendRunning == DetailedActorPathHandler::PATH_RUNNING_BLEND_WALK_TO_RUN) {
	LABEL_53:;
		PreferredMoveModeType &= ~512u;
		PreferredMoveModeType |= 256u;
		fRunWalkTime = fRunWalkTime - afDeltaT;
		if (fRunWalkTime < 0.0) {
			this->eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_FINISH_BLEND;
			fRunWalkTime = 0.0;
		}
		goto LABEL_57;
	}
	if (eBlendRunning != DetailedActorPathHandler::PATH_RUNNING_BLEND_FINISH_BLEND) {
		if (eBlendRunning != DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE ||
			((PreferredMoveModeType & 512) == 0 ||
				((MoveMode & 512) != 0 && ((MoveMode & 48) == 0 || (v23 & 48) != 0)))) {
			goto LABEL_57;
		}
		this->eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_WALK_TO_RUN;
		fRunWalkTime = pActor->GetAnimActionTime(ANIM_GROUP_MOVE_FORWARD) * 0.5;
		goto LABEL_53;
	}
	this->eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
LABEL_57:;
	this->MoveMode = PreferredMoveModeType | v23;
	return 1;
}

void DetailedActorPathHandler::RotateMoveDelta(float afDeltaT) {
	if (pActor->IsSwimming()) {
		NiPoint3 v13;
		NiMatrix3 ecx0;
		float v19;
		NiPoint3 aVal = TetherTargetPoint;
		float RelevantWaterHeight = pActor->GetRelevantWaterHeight();
		if (RelevantWaterHeight != -3.4028234663852886e+38 &&
			RelevantWaterHeight < static_cast<double>(TetherTargetPoint.z)) {
			TetherTargetPoint.z = pActor->GetRelevantWaterHeight() - 5.0;
		}
		bhkCharacterController* CharController = pActor->GetCharController();
		NiPoint3 pt = *pActor->QLocationOnReference();
		if (CharController) {
			double v9 = pActor->GetHeight() * 0.5;
			pt.z = CharController->QSwimFloatHeight() * v9 + pt.z;
			aVal.operator-=(pt);
		} else {
			const NiPoint3* v2 = &pActor->GetLocationOnReference();
			aVal.operator-=(*v2);
		}
		double Height = pActor->GetHeight();
		if (TetherTargetPoint.z >= pt.z - Height && pt.z > static_cast<double>(TetherTargetPoint.z)) {
			aVal.z = 0.0;
		}
		aVal.Unitize();
		BSClamp(aVal.z, -1.0f, 1.0);
		if (!(aVal.z >= 0.0)) {
			float X = -aVal.z;
			v19 = std::acosf(X);
			v19 = v19 - 1.5707963267948966;
		} else {
			v19 = std::acosf(aVal.z);
			v19 = 1.5707963267948966 - v19;
		}
		if (MoveDelta.y < 0.0 && (MoveMode & 2) != 0) {
			float Xa = -v19;
			ecx0.MakeXRotation(Xa);
		} else {
			ecx0.MakeXRotation(v19);
		}
		MoveDelta = (v13 = MoveDelta * ecx0);
	} else {
		NiPoint3* p_BlockedTargetPoint;
		NiPoint3 result;
		float v24;
		NiMatrix3 v25;
		if (bMovementBlocked) {
			p_BlockedTargetPoint = &BlockedTargetPoint;
		} else {
			p_BlockedTargetPoint = &TetherTargetPoint;
		}
		NiPoint3 a1 = *p_BlockedTargetPoint;
		const NiPoint3* v4 = &pActor->GetLocationOnReference();
		a1.operator-=(*v4);
		a1.z = 0.0;
		float ZAngleFromVector = GetZAngleFromVector(a1);
		switch (GetMoveMode() & 15) {
		case 0u:
		case 1u: {
			v24 = 0.0;
			break;
		}
		case 2u: {
			v24 = 3.1415927f;
			break;
		}
		case 4u: {
			v24 = -1.5707964f;
			break;
		}
		case 8u: {
			v24 = 1.5707964f;
			break;
		}
		default: {
			v24 = 0.0;
			break;
		}
		}
		double v5 = pActor->GetHeading(0);
		float v21 = v5 + v24;
		float v22 = ZAngleFromVector - v21;
		v25.MakeZRotationQ(v22);
		MoveDelta = (result = v25 * MoveDelta);
	}
}

void DetailedActorPathHandler::UpdateTetherPosition(float afDist) {
	NiPoint3 ecx0;
	float arfNewSplineT;
	PathingSolution* v2 = spPath.operator->();
	if (v2->GetNodeCount()) {
		uint32_t v3 = iCurrentNodeIndex + 1;
		PathingSolution* v4 = spPath.operator->();
		if (v3 < v4->GetNodeCount()) {
			float afParameter = fCurrentSplineT;
			PathingSolution* v5 = spPath.operator->();
			fDistanceTraveled = v5->ComputeDistanceToParameter(afParameter);
			LastTetherPoint.x = TetherTargetPoint.x;
			LastTetherPoint.y = TetherTargetPoint.y;
			LastTetherPoint.z = TetherTargetPoint.z;
			ResetStaticAvoidance();
			if (!(afDist >= 0.0)) {
				float afDista = -afDist;
				float afCurrentParametera = fCurrentSplineT;
				PathingSolution* v7 = spPath.operator->();
				v7->FindPointOnPath(afCurrentParametera, afDista, 0, &arfNewSplineT, &ecx0);
			} else {
				float afCurrentParameter = fCurrentSplineT;
				PathingSolution* v6 = spPath.operator->();
				v6->FindPointOnPath(afCurrentParameter, afDist, 1, &arfNewSplineT, &ecx0);
			}
			fCurrentSplineT = arfNewSplineT;
			TetherTargetPoint = ecx0;
			iCurrentNodeIndex = static_cast<int32_t>(fCurrentSplineT);
			pActorMover->DebugOnEvent();
		}
	}
}

void DetailedActorPathHandler::UpdateTether(float afDeltaT) {
	NiPoint3 v21;
	NiPoint3 v22;
	if (spPath.m_pObject->GetNodeCount()) {
		uint32_t v2;
		if (spPath.m_pObject->GetNodeCount() && (v2 = iCurrentNodeIndex + 1, v2 >= spPath.m_pObject->GetNodeCount())) {
			PathingNode* LastNode = spPath.m_pObject->GetLastNode();
			TetherTargetPoint = (&LastNode->Location)->GetLocation();
		} else {
			if (((MoveMode & ActorMover::MF_FORWARD) == 0 || !(MoveDelta.y < 0.0)) &&
				((MoveMode & ActorMover::MF_BACKWARD) == 0 || !(MoveDelta.y > 0.0)) &&
				((MoveMode & ActorMover::MF_LEFT) == 0 || !(MoveDelta.x > 0.0)) &&
				((MoveMode & ActorMover::MF_RIGHT) == 0 || !(MoveDelta.x < 0.0))) {
				NiPoint3 nextTetherDelta;
				NiMatrix3 rot;
				float a2 = pActor->GetHeading(0);
				rot.MakeZRotationQ(a2);
				NiPoint3 worldSpaceMoveDir = rot * MoveDelta;
				const NiPoint3* v4 = &pActor->GetLocationOnReference();
				NiPoint3 tetherDelta = TetherTargetPoint - *v4;
				float v19 = fCurrentSplineT + 0.10000000149011612;
				spPath.m_pObject->ComputeLocationFromParameter(v19, nextTetherDelta);
				nextTetherDelta.operator-=(TetherTargetPoint);
				tetherDelta.z = 0.0;
				float fdot = worldSpaceMoveDir.Dot(tetherDelta);
				float ftetherDot = nextTetherDelta.Dot(tetherDelta);
				float fsqrDist = tetherDelta.SqrXYLength();
				float fsqrTetherMinMoveThisFrame = MoveDelta.SqrLength() * 4.0;
				float v18 = fTetherMinMove * fTetherMinMove;
				fsqrTetherMinMoveThisFrame = BSMAX(fsqrTetherMinMoveThisFrame, v18);
				if (fdot < 0.0 || (ftetherDot < 0.0 || fsqrTetherMinMoveThisFrame > static_cast<double>(fsqrDist))) {
					fDistanceTraveled = spPath.m_pObject->ComputeDistanceToParameter(fCurrentSplineT);
					LastTetherPoint.x = TetherTargetPoint.x;
					LastTetherPoint.y = TetherTargetPoint.y;
					LastTetherPoint.z = TetherTargetPoint.z;
					if (!(fCurrentSplineT >= static_cast<double>(spPath.m_pObject->GetNodeCount() - 1))) {
						float flastDot = fdot;
						fCurrentSplineT = fCurrentSplineT + 0.10000000149011612;
						spPath.m_pObject->ComputeLocationFromParameter(fCurrentSplineT, TetherTargetPoint);
						float v17 = fCurrentSplineT + 0.10000000149011612;
						spPath.m_pObject->ComputeLocationFromParameter(v17, nextTetherDelta);
						nextTetherDelta.operator-=(TetherTargetPoint);
						const NiPoint3* v7 = &pActor->GetLocationOnReference();
						NiPoint3* v8 = ::new (&v22) NiPoint3(TetherTargetPoint - *v7);
						tetherDelta.x = v8->x;
						tetherDelta.y = v8->y;
						float z = v8->z;
						tetherDelta.z = 0.0f;
						fdot = worldSpaceMoveDir.Dot(tetherDelta);
						ftetherDot = nextTetherDelta.Dot(tetherDelta);
						fsqrDist = tetherDelta.SqrXYLength();
						while (!(fCurrentSplineT >= static_cast<double>(spPath.m_pObject->GetNodeCount() - 1))) {
							if (!(fdot < 0.0) && !(ftetherDot < 0.0)) {
								if (fsqrTetherMinMoveThisFrame <= static_cast<double>(fsqrDist)) {
									goto LABEL_27;
								}
							} else {
								if (flastDot > static_cast<double>(fdot)) {
									goto LABEL_27;
								}
							}
							flastDot = fdot;
							fCurrentSplineT = fCurrentSplineT + 0.10000000149011612;
							spPath.m_pObject->ComputeLocationFromParameter(fCurrentSplineT, TetherTargetPoint);
							float v16 = fCurrentSplineT + 0.10000000149011612;
							spPath.m_pObject->ComputeLocationFromParameter(v16, nextTetherDelta);
							nextTetherDelta.operator-=(TetherTargetPoint);
							const NiPoint3* v11 = &pActor->GetLocationOnReference();
							NiPoint3* v12 = ::new (&v21) NiPoint3(TetherTargetPoint - *v11);
							tetherDelta.x = v12->x;
							tetherDelta.y = v12->y;
							float v13 = v12->z;
							tetherDelta.z = 0.0f;
							fdot = worldSpaceMoveDir.Dot(tetherDelta);
							ftetherDot = nextTetherDelta.Dot(tetherDelta);
							fsqrDist = tetherDelta.SqrXYLength();
						}
						fCurrentSplineT = static_cast<float>(spPath.m_pObject->GetNodeCount() - 1);
					LABEL_27:;
						iCurrentNodeIndex = static_cast<int32_t>(fCurrentSplineT);
						pActorMover->DebugOnEvent();
					} else {
						fCurrentSplineT = static_cast<float>(spPath.m_pObject->GetNodeCount() - 1);
						iCurrentNodeIndex = static_cast<int32_t>(fCurrentSplineT);
						PathingNode* v6 = spPath.m_pObject->GetLastNode();
						TetherTargetPoint = (&v6->Location)->GetLocation();
					}
				}
			}
		}
	}
}

void DetailedActorPathHandler::ComputeHeadingAndLooking(float& arfHeading, float& arfLooking) {
	NiPoint3 rot;
	float fangle = 0.0f;
	float flookingAngle = 0.0f;
	if (bMovementBlocked) {
		fangle = fAlternateFacingAngle;
	} else {
		if (bIgnoreInitialRotation) {
			if (bIgnoreInitialRotationKeepActorHeading || !spPath.m_pObject) {
				fangle = pActor->GetHeading(0);
			} else {
				NiPoint3 targetPoint;
				spPath.m_pObject->ComputeLocationFromParameter(fIgnoreInitialRotationTetherParam, targetPoint);
				const NiPoint3* v4 = &pActor->GetLocationOnReference();
				NiPoint3 delta = targetPoint - *v4;
				delta.z = 0.0f;
				fangle = GetZAngleFromVector(delta);
			}
		} else {
			if (bIgnoreLookAtPoint || !pActorMover->HasLookAtTarget()) {
				double v7;
				NiPoint3 lookatPoint_2;
				if (spPath.m_pObject && spPath.m_pObject->GetNodeCount()) {
					float afCurrentSplineParameter = fCurrentSplineT + 0.10000000149011612;
					spPath.m_pObject->ComputeLocationFromParameter(afCurrentSplineParameter, lookatPoint_2);
				} else {
					lookatPoint_2 = TetherTargetPoint;
				}
				const NiPoint3* v6 = &pActor->GetLocationOnReference();
				NiPoint3 deltaToTargetPoint = lookatPoint_2 - *v6;
				if (pActor->IsSwimming() &&
					(v7 = deltaToTargetPoint.Length() * 0.949999988079071, std::fabs(deltaToTargetPoint.z) >= v7)) {
					fangle = pActor->GetHeading(0);
				} else {
					fangle = GetZAngleFromVector(deltaToTargetPoint);
				}
				if (pActor->IsSwimming()) {
					flookingAngle = 0.0f;
				}
			} else {
				NiPoint3 lookatPoint;
				pActorMover->GetLookAtPoint(lookatPoint);
				NiPoint3 firingLocation = CombatUtilities::GetAttackerFiringLocation(pActor);
				// The exe keeps the squared length at full x87 precision (fstp qword) for the compare below.
				double v10 = (firingLocation - lookatPoint).SqrXYLength();
				double Radius = pActor->GetRadius();
				if (!(pActor->GetRadius() * Radius * 0.25 > v10)) {
					CombatUtilities::GetAngleToProjectedPoint(&rot, pActor, lookatPoint);
					if (!pActor->IsSwimming()) {
						flookingAngle = rot.x;
					}
					if (!(pActor->GetRadius() >= 100.0)) {
						fangle = rot.z;
					} else {
						const NiPoint3* v5 = &pActor->GetLocationOnReference();
						NiPoint3 delta_2 = lookatPoint - *v5;
						delta_2.z = 0.0f;
						fangle = GetZAngleFromVector(delta_2);
					}
				} else {
					fangle = pActor->GetHeading(0);
				}
			}
		}
	}
	arfHeading = fangle;
	arfLooking = flookingAngle;
}

float DetailedActorPathHandler::UpdateHeading(float fdeltaT, float& afSlowDown) {
	double v8;
	NiPoint3 v21;
	float fnewT;
	NiPoint3 furtherTetherLoc;
	float fratio;
	float fangledelta;
	float fangle = 0.0f;
	float flookingAngle = 0.0f;
	ComputeHeadingAndLooking(fangle, flookingAngle);
	float factangle = pActor->GetHeading(0);
	BSWrap(factangle, -3.1415927f, 3.1415927f);
	BSWrap(fangle, -3.1415927f, 3.1415927f);
	fangledelta = fangle - factangle;
	BSWrap(fangledelta, -3.1415927f, 3.1415927f);
	if (std::fabs(fangledelta) >= 0.01745329238474369) {
		fratio = 1.0f;
		if (spPath.m_pObject) {
			if (pActor->GetTurningSpeed() > 45.0) {
				float fspeed;
				const NiPoint3* v3 = &pActor->GetLocationOnReference();
				float fdistToTether = (TetherTargetPoint - *v3).Length();
				if ((MoveMode & ActorMover::MF_RUNNING) != 0) {
					fspeed = pActor->GetRunSpeed();
				} else {
					fspeed = pActor->GetWalkSpeed();
				}
				furtherTetherLoc = TetherTargetPoint;
				if (fspeed > static_cast<double>(fdistToTether)) {
					float afDist = fspeed - fdistToTether;
					spPath.m_pObject->FindPointOnPath(fCurrentSplineT, afDist, 1, &fnewT, &furtherTetherLoc);
				}
				float factorAngle = pActor->GetHeading(0);
				const NiPoint3* v5 = &pActor->GetLocationOnReference();
				NiPoint3 deltaToFurtherTether = furtherTetherLoc - *v5;
				float ffurtherTetherAngle = GetZAngleFromVector(deltaToFurtherTether);
				float fangleDiff = ffurtherTetherAngle - factorAngle;
				BSWrap(fangleDiff, -3.1415927f, 3.1415927f);
				fratio = std::fabs(fangleDiff) / 1.5707963267948966;
				BSClamp(fratio, 0.025f, 1.0f);
			}
		}
		double v6 = pActor->GetTurningSpeed();
		float frotate = v6 * 0.01745329238474369 * fdeltaT * 3.0 * fratio;
		double v7 = std::fabs(fangledelta);
		if (frotate < v7) {
			if (!(std::fabs(fangledelta) > 2.700000047683716)) {
				if (!(std::fabs(fangledelta) > 2.0)) {
					v8 = std::fabs(fangledelta);
					if (!(v8 > 1.600000023841858)) {
						if (spPath.m_pObject) {
							if (spRequest.m_pObject) {
								PathingNode* LastNode = spPath.m_pObject->GetLastNode();
								NiPoint3* a1 = ::new (&v21) NiPoint3((&LastNode->Location)->GetLocation());
								const NiPoint3* v10 = &pActor->GetLocationOnReference();
								// The exe keeps the squared length at full x87 precision (fstp qword).
								double v16 = (*v10 - *a1).SqrXYLength();
								double Radius = pActor->GetRadius();
								if (pActor->GetRadius() * Radius > v16) {
									afSlowDown = 0.5f;
								}
							}
						}
					} else {
						afSlowDown = 0.2f;
					}
				} else {
					afSlowDown = 0.1f;
				}
			} else {
				afSlowDown = 0.05f;
			}
		}
		BSClamp(fangledelta, -frotate, frotate);
		fangle = factangle + fangledelta;
	}
	fLastRotationApplied = std::fabs(fangledelta);
	UpdateLooking(fdeltaT, flookingAngle);
	return fangle;
}

// The exe's cleanup-only C++ EH frame is left out.
bool DetailedActorPathHandler::UpdateStaticAvoidance(float afDeltaT) {
	FNVD_INLINED_LOCAL(NavMeshPtr, apMemory_16, std::nullopt, std::nullopt);
	bool breturnBlocked = 0;
	if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH ||
		eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE) {
		return 1;
	}
	NiPoint3 currentLoc = pActor->GetLocationOnReference();
	NiPoint3 lastDeltaToTether = TetherTargetPoint - LastLocation;
	NiPoint3 deltaToTether = TetherTargetPoint - currentLoc;
	NiPoint3 lastFrameDelta = currentLoc - LastLocation;
	bool bblockedThisFrame = 0;
	bool bblockedbyActor = 0;
	if (pActor->IsSwimming()) {
		float fFudge_2 = 20.0f;
		float lastSqr = lastDeltaToTether.SqrLength();
		float lastSqrFudge = lastDeltaToTether.Length();
		lastSqrFudge = lastSqrFudge + 20.0f;
		lastSqrFudge = lastSqrFudge * lastSqrFudge;
		float thisSqr = deltaToTether.SqrLength();
		double v18 = lastFrameDelta.Dot(lastFrameDelta);
		if (LastFrameMinMoveDelta.Dot(LastFrameMinMoveDelta) * 0.125 > v18) {
			bhkCharacterController* pcon_2 = pActor->GetCharController();
			if (pcon_2) {
				TESObjectREFR* pref_4;
				NiAVObject* pav_2;
				TESObjectREFR* pact_2 = 0;
				bhkCharacterController* pcc_2 = pcon_2->GetBumpedCharacterController();
				if (pcc_2) {
					pav_2 = pcc_2->GetNiObject();
				} else {
					pav_2 = 0;
				}
				NiAVObject* pbumped3D = pav_2;
				TESObjectREFR* pref_3 = TESObjectREFR::FindReferenceFor3D(pav_2);
				if (pref_3 && pref_3->IsActor()) {
					pref_4 = pref_3;
				} else {
					pref_4 = 0;
				}
				pact_2 = pref_4;
				if (pref_4) {
					bblockedbyActor = 1;
				}
			}
			bblockedThisFrame = 1;
		}
	} else {
		float fFudge = 20.0f;
		float lastXYSqr = lastDeltaToTether.SqrXYLength();
		float lastXYSqrFudge = lastDeltaToTether.XYLength();
		lastXYSqrFudge = lastXYSqrFudge + 20.0f;
		lastXYSqrFudge = lastXYSqrFudge * lastXYSqrFudge;
		float thisXYSqr = deltaToTether.SqrXYLength();
		if (fLastFrameMinMoveDelta * fLastFrameMinMoveDelta * 0.125 > lastFrameDelta.Dot(lastFrameDelta)) {
			bhkCharacterController* pcon = pActor->GetCharController();
			if (pcon) {
				TESObjectREFR* pref_2;
				NiAVObject* pav;
				TESObjectREFR* pact = 0;
				bhkCharacterController* pcc = pcon->GetBumpedCharacterController();
				if (pcc) {
					pav = pcc->GetNiObject();
				} else {
					pav = 0;
				}
				NiNode* a1 = static_cast<NiNode*>(pav);
				TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
				if (pref && pref->IsActor()) {
					pref_2 = pref;
				} else {
					pref_2 = 0;
				}
				pact = pref_2;
				if (pref_2) {
					bblockedbyActor = 1;
				}
			}
			bblockedThisFrame = 1;
		}
	}
	LastLocation = currentLoc;
	if (bblockedThisFrame) {
		double v5;
		if (bblockedbyActor) {
			v5 = afDeltaT * 0.20000000298023224 + fBlockTime;
		} else {
			v5 = fBlockTime + afDeltaT;
		}
		fBlockTime = v5;
		if (eState == DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING) {
			eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
			return 0;
		}
	} else {
		if (!bMovementBlocked) {
			fBlockTime = 0.0f;
			return 1;
		}
	}
	if (fBlockTime > 1.5) {
		if (pActor->GetAnimation()) {
			Animation* v6 = pActor->GetAnimation();
			if (v6->ShouldBeMoving()) {
				bMovementBlocked = 1;
			}
		}
	}
	if (bMovementBlocked) {
		breturnBlocked = 1;
		// The exe tests the three states for equality (0x9E519C), not a range.
		if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_INVALID ||
			eAvoidState == DetailedActorPathHandler::PATH_AVOID_OK ||
			eAvoidState == DetailedActorPathHandler::PATH_AVOID_FAILED) {
			TESWorldSpace* WorldSpace = pActor->GetWorldSpace();
			FNVD_CONSTRUCTED_LOCAL(PathingLocation, apMemory_24, &PathingLocation::Destruct, &TetherTargetPoint,
				pActor->GetParentCell(), WorldSpace);
			if (apMemory_24.ResolveNavMeshInfo(0)) {
				uint16_t apMemory_20;
				::new (&apMemory_16) NavMeshPtr;
				if (apMemory_24.GetNavMeshAndTriangle(apMemory_16, apMemory_20)) {
					using v26_t = BSSimpleArray<NavMeshStaticAvoidNode*, 1024>;
					FNVD_CONSTRUCTED_LOCAL(v26_t, v26, &v26_t::Destruct);
					apMemory_16.m_pObject->GetStaticAvoidNodesForTri(apMemory_20, v26);
					char v25 = 1;
					if (!v26.IsEmpty()) {
						for (uint32_t uiIndex = 0;; ++uiIndex) {
							uint32_t Spell = v26.iSize;
							if (uiIndex >= Spell) {
								break;
							}
							NavMeshStaticAvoidNode* pnode = v26.GetAt(static_cast<int32_t>(uiIndex));
							double v15 = (pnode->Point1 - TetherTargetPoint).SqrLength();
							double v14 = pnode->fRadius;
							if (pnode->fRadius * v14 > v15) {
								if (!(pnode->fRadius < 128.0)) {
									float v12 = pnode->fCost * 10.0;
									pnode->SetCost(v12);
								} else {
									double v10 = pnode->fRadius;
									float v13 = v10 + v10;
									pnode->SetRadius(v13);
								}
								v25 = 0;
							}
						}
					}
					if (v25) {
						float afRadius = spRequest.m_pObject->fActorRadius;
						apMemory_16.m_pObject->AddStaticAvoidNode(TetherTargetPoint, apMemory_20, afRadius, 1.0f);
					}
				}
				FNVD_DESTRUCT(NiPointer<NavMesh>)(&apMemory_16);
			}
		}
	}
	if (breturnBlocked) {
		pActorMover->DebugOnEvent();
	}
	return !breturnBlocked;
}

// The exe's cleanup-only C++ EH frame is left out.
void DetailedActorPathHandler::FindPathStartingLocation(float afPercentTraveled) {
	NiPoint3 v9;
	fPathLength = spPath.m_pObject->ComputeDetailedPathLength();
	fCurrentSplineT =
		spPath.m_pObject->GetSplineParametersFromPercentage(afPercentTraveled, fPathLength, iCurrentNodeIndex);
	if (bWaitingForUpdate) {
		bWaitingForUpdate = 0;
		if (fPathLength == 0.0) {
			iCurrentNodeIndex = spPath.m_pObject->GetNodeCount() - 1;
			fCurrentSplineT = static_cast<float>(iCurrentNodeIndex);
			PathingNode* LastNode = spPath.m_pObject->GetLastNode();
			TetherTargetPoint = (&LastNode->Location)->GetLocation();
			LastTetherPoint = pActor->GetLocationOnReference();
			bWaitingForUpdate = 0;
		} else {
			NiPoint3 oldDirVec = TetherTargetPoint - LastTetherPoint;
			oldDirVec.z = 0.0;
			NiPoint3 actorLoc = pActor->GetLocationOnReference();
			NiPoint3 actorDirVec = TetherTargetPoint - actorLoc;
			float factDotCheck = actorDirVec.Dot(oldDirVec);
			bool bpastGoal = factDotCheck < 0.0;
			float fcheck5 = 5.0 / fPathLength;
			float fcheck20 = fcheck5 * 4.0;
			float fbestSqrTetherDist = 3.4028235e+38f;
			float fbestSplineT = 0.0f;
			uint32_t ibestNode = 0;
			bool botherSide = 0;
			for (int32_t i = 0; i < 4; ++i) {
				NiPoint3 actorToTetherDir;
				NiPoint3 newTetherLoc;
				uint32_t itempNode;
				float v7 = afPercentTraveled + fcheck5 + static_cast<double>(i) * fcheck20;
				float ftempSplineT = spPath.m_pObject->GetSplineParametersFromPercentage(v7, fPathLength, itempNode);
				spPath.m_pObject->ComputeLocationFromParameter(ftempSplineT, newTetherLoc);
				NiPoint3* v3 = ::new (&v9) NiPoint3(newTetherLoc - actorLoc);
				actorToTetherDir.x = v3->x;
				actorToTetherDir.y = v3->y;
				float z = v3->z;
				actorToTetherDir.z = 0.0f;
				float fdot = actorToTetherDir.Dot(oldDirVec);
				float fsqrDist = actorToTetherDir.SqrXYLength();
				if ((bpastGoal || !(fdot < 0.0)) && (!bpastGoal || !(fdot > 0.0))) {
					if (fbestSqrTetherDist > static_cast<double>(fsqrDist)) {
						ibestNode = itempNode;
						fbestSplineT = ftempSplineT;
						fbestSqrTetherDist = fsqrDist;
					}
				} else {
					if (botherSide) {
						if (fbestSqrTetherDist > static_cast<double>(fsqrDist)) {
							ibestNode = itempNode;
							fbestSplineT = ftempSplineT;
							fbestSqrTetherDist = fsqrDist;
						}
					} else {
						botherSide = 1;
						ibestNode = itempNode;
						fbestSplineT = ftempSplineT;
						fbestSqrTetherDist = fsqrDist;
					}
				}
			}
			spPath.m_pObject->ComputeLocationFromParameter(fbestSplineT, TetherTargetPoint);
			iCurrentNodeIndex = ibestNode;
			fCurrentSplineT = fbestSplineT;
			LastTetherPoint = actorLoc;
		}
	} else {
		spPath.m_pObject->ComputeLocationFromParameter(fCurrentSplineT, TetherTargetPoint);
		LastTetherPoint.x = TetherTargetPoint.x;
		LastTetherPoint.y = TetherTargetPoint.y;
		LastTetherPoint.z = TetherTargetPoint.z;
		if (afPercentTraveled > 0.0) {
			FNVD_CONSTRUCTED_LOCAL(PathingLocation, tetherNavMeshLoc, &PathingLocation::Destruct, TetherTargetPoint,
				pActor);
			NiPoint3 snapLocation = TetherTargetPoint;
			Pathing::GetNavMeshZForLocation(tetherNavMeshLoc, snapLocation.z);
			float fheading = pActor->GetHeading(0);
			TESWorldSpace* WorldSpace = pActor->GetWorldSpace();
			pActor->MoveActorAndFollowers(snapLocation, fheading, pActor->GetParentCell(), WorldSpace, 1, 1);
		}
		UpdateTetherPosition(0.0f);
	}
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::CheckGoalReached() {
	int32_t v10;
	const char* FullName;
	NiPoint3 goal;
	if (bEndUpdatePath) {
		uint32_t v2 = iCurrentNodeIndex + 2;
		if (v2 >= spPath.m_pObject->GetNodeCount()) {
			NiPoint3 diff;
			char v44 = 0;
			uint32_t v3 = iCurrentNodeIndex + 2;
			if (v3 == spPath.m_pObject->GetNodeCount()) {
				const NiPoint3* a1_4 = &pActor->GetLocationOnReference();
				PathingNode* Node = spPath.m_pObject->GetNode(iCurrentNodeIndex);
				diff = (&Node->Location)->GetLocation() - *a1_4;
			} else {
				v44 = 1;
			}
			if (v44 || diff.SqrXYLength() <= 25600.0) {
				uint32_t iupdateNode;
				uint32_t icurrentSize = spPath.m_pObject->GetNodeCount();
				if (pActorMover->iNextNode) {
					iupdateNode = pActorMover->iNextNode - 1;
				} else {
					iupdateNode = pActorMover->iNextNode;
				}
				uint32_t aiCurrentVirtualNodeIndex = iupdateNode;
				const NiPoint3* v6 = &pActor->GetLocationOnReference();
				Pathing::UPDATE_DETAILED_PATH_RESULT updated =
					Pathing::UpdateSolution(spRequest.m_pObject, spPath.m_pObject, aiCurrentVirtualNodeIndex, *v6);
				if (updated == Pathing::CLEARED_DETAILED_PATH) {
					return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
				}
				if (updated > Pathing::CLEARED_DETAILED_PATH && updated <= Pathing::UPDATED_DETAILED_PATH) {
					PathingNode* LastNode = spPath.m_pObject->GetLastNode();
					bEndUpdatePath = LastNode->IsTransitionNode();
					pActorMover->iNextNode = spPath.m_pObject->iLastLoadedVirtualNodeIndex;
					return ActorPathHandler::PATH_HANDLER_RESULT_OK;
				}
				bEndUpdatePath = 0;
			}
		}
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	bool bgoalReached = 0;
	float fdistanceSqr = 3.4028235e+38f;
	if (spPath.m_pObject->IsIncomplete() ||
		(v10 = spPath.m_pObject->iLastLoadedVirtualNodeIndex, v10 != spPath.m_pObject->GetVirtualNodeCount() - 1)) {
		PathingNode* v13 = spPath.m_pObject->GetLastNode();
		goal = (&v13->Location)->GetLocation();
	} else {
		uint32_t ShaderPropertyType = spPath.m_pObject->iLastLoadedVirtualNodeIndex;
		VirtualPathingNode* VirtualNode = spPath.m_pObject->GetVirtualNode(ShaderPropertyType);
		goal = (&VirtualNode->Location)->GetLocation();
	}
	const NiPoint3* v14 = &pActor->GetLocationOnReference();
	NiPoint3 diff_2 = goal - *v14;
	fdistanceSqr = diff_2.SqrXYLength();
	float fgoalRadiusTestSqr = fGoalRadiusSqr;
	bgoalReached = fGoalRadiusSqr >= static_cast<double>(fdistanceSqr);
	if (!bgoalReached) {
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	bool bvalidCheck = 1;
	if (spRequest.m_pObject->GetSoftGoalRadius() && fInnerGoalRadiusSqr < static_cast<double>(fGoalRadiusSqr) &&
		fInnerGoalRadiusSqr < static_cast<double>(fdistanceSqr)) {
		NiPoint3 tetherDir = TetherTargetPoint - LastTetherPoint;
		tetherDir.z = 0.0;
		const NiPoint3* v15 = &pActor->GetLocationOnReference();
		NiPoint3 actorToTetherDir = TetherTargetPoint - *v15;
		actorToTetherDir.z = 0.0;
		float fdot = tetherDir.Dot(actorToTetherDir);
		if (fdot >= 0.0 && fAvoidanceDetectionValue == 0.0 && fBlockTime == 0.0) {
			bvalidCheck = 0;
		}
	}
	if (!bvalidCheck) {
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	if (pActor->IsSwimming()) {
		float a1_4a = pActor->GetHeight();
		float a1 = spRequest.m_pObject->fGoalZDelta;
		float fzdeltaThresold = BSMIN(a1, a1_4a);
		double v17 = std::fabs(diff_2.z);
		if (fzdeltaThresold >= v17) {
			return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
		}
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	double v22 = std::fabs(diff_2.z);
	if (spRequest.m_pObject->fGoalZDelta >= v22) {
		return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
	}
	uint32_t v16 = iCurrentNodeIndex + 1;
	if (v16 < spPath.m_pObject->GetNodeCount() || pActor->IsSwimming()) {
		return ActorPathHandler::PATH_HANDLER_RESULT_OK;
	}
	if (pActor->GetFormEditorIDLength()) {
		FullName = pActor->GetFormEditorID();
	} else {
		FullName = pActor->GetFullName();
	}
	Warning("PATHFINDING: Actor %s failed pathing due to goal Z difference %f, being too large", FullName,
		std::fabs(diff_2.z));
	return ActorPathHandler::PATH_HANDLER_RESULT_CANT_REACH_Z;
}

void DetailedActorPathHandler::BuildAvoidanceList(Actor* apActor, PathingRequest* aspRequest, uint32_t aMoveMode,
	const NiPoint3& aMoveDelta, DetailedActorPathHandler* apHandler,
	BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>& aRetCandidates) {
	DetailedActorPathHandler::ActorAvoidanceData v15;
	DetailedActorPathHandler::ActorAvoidanceData apMemory;
	float fdetection;
	NiMatrix3 rot;
	float fAngle = apActor->GetHeading(0);
	rot.MakeZRotationQ(fAngle);
	NiPoint3 worldSpaceMoveDir = rot * aMoveDelta;
	NiPoint3 unitWorldSpaceMoveDir = worldSpaceMoveDir;
	float fmoveTime = appTimer.fDelta;
	float favoidTime = gs_fAvoidanceTimeCheck_Pathfinding.Float();
	float fabsoluteMoveLength = unitWorldSpaceMoveDir.UnitizeGetLength();
	float fabsoluteMoveSpeed = fabsoluteMoveLength / fmoveTime;
	float fabsoluteAvoidDistance = favoidTime * fabsoluteMoveSpeed;
	NiPoint3 currentPos = apActor->GetLocationOnReference();
	float fourRadius = aspRequest->fActorRadius;
	fdetection = 0.0f;
	float favoidConeAngle = DetailedActorPathHandler::fActorAvoidConeAngle;
	float fabsoluteCheckDistance = fourRadius + fabsoluteAvoidDistance;
	// The exe stores the follow leader in a local it never reads.
	Actor* pfollowLeader = DetailedActorPathHandler::GetFollowLeader(apActor);
	if ((aMoveMode & ActorMover::MF_RUNNING) != 0) {
		favoidConeAngle = DetailedActorPathHandler::fRunningActorAvoidConeAngle;
	}
	DetailedActorPathHandler::UpdateAvoidanceCandidateList();
	for (uint32_t i = 0; i < DetailedActorPathHandler::uiAvoidanceCandidatesSize; ++i) {
		Actor* pact = DetailedActorPathHandler::AvoidanceCandidates.operator[](i);
		if (pact != apActor) {
			NiPoint3 toOtherActor = pact->GetLocationOnReference();
			toOtherActor.operator-=(currentPos);
			float fflatDist2 = toOtherActor.SqrXYLength();
			if (!(fflatDist2 > 1e+06)) {
				float ftheirRadius = pact->GetRadius();
				if (ftheirRadius >= fourRadius * 0.33000001311302185) {
					NiPoint3 toTarget;
					NiPoint3 unitToTarget;
					NiPoint3 relativeMoveDir = worldSpaceMoveDir;
					if (pact->IsMoving()) {
						NiMatrix3 targetRot;
						float fAnglea = pact->GetHeading(0);
						targetRot.MakeZRotationQ(fAnglea);
						NiPoint3* ActorMoverOverrideMovement = pact->GetActorMoverOverrideMovement();
						NiPoint3 targetWorldSpaceMoveDir = targetRot * *ActorMoverOverrideMovement;
						relativeMoveDir.operator-=(targetWorldSpaceMoveDir);
					}
					float fmoveLength = relativeMoveDir.UnitizeGetLength();
					float* v8 = &gs_fAvoidanceAvoidAllRadius_Pathfinding.Float();
					float* v9 = &gs_fAvoidanceAvoidAllRadius_Pathfinding.Float();
					if (!(fflatDist2 > *v8 * *v9) ||
						(toTarget = pact->GetLocationOnReference(),
							(&toTarget.operator-=(currentPos),
								(unitToTarget = toTarget,
									(unitToTarget.Unitize(), !(unitToTarget.Dot(unitWorldSpaceMoveDir) < 0.0)))))) {
						float fmoveSpeed = fmoveLength / fmoveTime;
						float favoidDistance = favoidTime * fmoveSpeed;
						float fcheckDistance = fourRadius + favoidDistance;
						float* v10 = &gs_fAvoidanceAvoidAllRadius_Pathfinding.Float();
						if (*v10 > static_cast<double>(fcheckDistance)) {
							fcheckDistance = gs_fAvoidanceAvoidAllRadius_Pathfinding.Float();
						}
						if (DetailedActorPathHandler::CheckDetection(apActor, apHandler, pact, currentPos,
								relativeMoveDir, fcheckDistance, favoidConeAngle, fdetection, 0)) {
							DetailedActorPathHandler::ActorAvoidanceData* v11 = FNVD_RECONSTRUCT_AS(Construct_009E5AB0,
								DetailedActorPathHandler::ActorAvoidanceData)(&apMemory, pact, fdetection);
							aRetCandidates.Add(*v11);
						}
					}
				}
			}
		}
	}
	if (DetailedActorPathHandler::CheckDetection(apActor, apHandler, pPlayer, currentPos, worldSpaceMoveDir,
			fabsoluteAvoidDistance, favoidConeAngle, fdetection, 0)) {
		DetailedActorPathHandler::ActorAvoidanceData* v12 = FNVD_RECONSTRUCT_AS(Construct_009E5AB0,
			DetailedActorPathHandler::ActorAvoidanceData)(&v15, pPlayer, fdetection);
		aRetCandidates.Add(*v12);
	}
}

Actor* DetailedActorPathHandler::GetFollowLeader(Actor* apActor) {
	int32_t v5 = 0;
	BaseProcess* baseProcess = apActor->MobileObject::GetCurrentAIProcess();
	if (!baseProcess) {
		return reinterpret_cast<Actor*>(static_cast<uintptr_t>(v5));
	}
	TESPackage* v4 = baseProcess->GetPackageThatIsRunning();
	if (!v4) {
		return reinterpret_cast<Actor*>(static_cast<uintptr_t>(v5));
	}
	PACKAGE_PROCEDURE_TYPE parent = v4->TESPackage::GetProcedureType();
	if (Procedure_Action[(uint32_t)parent][(int32_t)baseProcess->GetProcedureIndexRunning()] != 6) {
		return reinterpret_cast<Actor*>(static_cast<uintptr_t>(v5));
	}
	Actor* v3 = static_cast<Actor*>(baseProcess->GetTarget());
	if (!v3) {
		return reinterpret_cast<Actor*>(static_cast<uintptr_t>(v5));
	}
	if (v3->IsActor()) {
		return v3;
	}
	return reinterpret_cast<Actor*>(static_cast<uintptr_t>(v5));
}

// The exe's cleanup-only C++ EH frame is left out.
void DetailedActorPathHandler::UpdateAvoidNodesForPath(Actor* apActor, PathingRequest* apRequest,
	DetailedActorPathHandler* apHandler) {
	alignas(16) PathingAvoidNode v16;
	hkVector4 pconPos;
	NiPoint3 avoidCenter;
	PathingAvoidNode v21;
	NiPoint3 v22;
	using avoidActors_t = BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>;
	FNVD_INLINED_LOCAL(avoidActors_t, avoidActors, std::nullopt, std::nullopt);
	if (!apHandler || apHandler->eAvoidState != DetailedActorPathHandler::PATH_AVOID_REPATH) {
		float v38 = 1.5f;
		::new (&avoidActors) BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>;
		DetailedActorPathHandler::BuildAvoidanceList(apActor, apRequest, 257u, NiPoint3_UNIT_Y, apHandler, avoidActors);
		if (!avoidActors.IsEmpty()) {
			float fourRadius = apRequest->fActorRadius;
			PathingAvoidNodeArray* pavoidNodes = apRequest->GetAvoidNodeArray();
			if (!pavoidNodes) {
				PathingAvoidNodeArray* pavoidNodes_2;
				PathingAvoidNodeArray* v33 =
					static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
				if (v33) {
					pavoidNodes_2 = ::new (v33) PathingAvoidNodeArray;
				} else {
					pavoidNodes_2 = 0;
				}
				pavoidNodes = pavoidNodes_2;
				apRequest->SetAvoidNodeArray(pavoidNodes_2);
			}
			for (uint32_t i = 0;; ++i) {
				NiMatrix3 targetRot;
				PathingAvoidNode::AVOID_NODE_TYPE nodeType;
				float fnodeRadius_2;
				uint32_t FormFlags = avoidActors.iSize;
				if (i >= FormFlags) {
					break;
				}
				Actor* pavoidActor = avoidActors.GetAt(static_cast<int32_t>(i)).pActor;
				if (apHandler) {
					apHandler->ActorAvoidList.AddHead(pavoidActor);
				}
				if (pavoidActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
					pavoidActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
					double Radius = pavoidActor->GetRadius();
					fnodeRadius_2 = Radius * 0.5 * v38;
					nodeType = PathingAvoidNode::AVOID_NODE_SPHERE;
				} else {
					double v5 = pavoidActor->GetRadius();
					fnodeRadius_2 = v5 * v38;
					nodeType = PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR;
				}
				float fAngle = pavoidActor->GetHeading(0);
				targetRot.MakeZRotationQ(fAngle);
				NiPoint3* ActorMoverOverrideMovement = pavoidActor->GetActorMoverOverrideMovement();
				alignas(16) NiPoint3 targetWorldSpaceMoveDir = targetRot * *ActorMoverOverrideMovement;
				float fAnglea = appTimer.fDelta;
				NiPoint3 OneSecondDelta = targetWorldSpaceMoveDir / fAnglea;
				double v23 = OneSecondDelta.SqrLength();
				double v7 = pavoidActor->GetRadius();
				if (v7 < v23) {
					PathingAvoidNode::AVOID_NODE_TYPE fAngleb = nodeType;
					float afRadius = fnodeRadius_2;
					const NiPoint3* v8 = &pavoidActor->GetLocationOnReference();
					NiPoint3* v9 = ::new (&v22) NiPoint3(*v8 + OneSecondDelta);
					PathingAvoidNode* v10 = FNVD_RECONSTRUCT(PathingAvoidNode)(&v21, *v9, afRadius, 2.0f, fAngleb);
					pavoidNodes->Add(*v10);
				}
				avoidCenter = pavoidActor->GetLocationOnReference();
				bhkCharacterController* pavoidCon = pavoidActor->GetCharController();
				if (pavoidCon) {
					if (!pavoidCon->GetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS)) {
						NiPoint3 pconPosNi;
						FNVD_RECONSTRUCT(hkVector4)(&pconPos, pavoidCon->GethkPosition());
						HK2NI(pconPosNi, pconPos);
						avoidCenter.x = pconPosNi.x;
						avoidCenter.y = pconPosNi.y;
					}
				}
				PathingAvoidNode* v11 =
					FNVD_RECONSTRUCT(PathingAvoidNode)(&v16, avoidCenter, fnodeRadius_2, 2.0f, nodeType);
				pavoidNodes->Add(*v11);
				if (apHandler) {
					apHandler->eAvoidState = DetailedActorPathHandler::PATH_AVOID_REPATH;
				}
			}
		}
		avoidActors.~BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>();
	}
}

bool DetailedActorPathHandler::UpdateAvoidanceCandidateList() {
	uint32_t inow = appTimer.iLastTime;
	if (DetailedActorPathHandler::uiAvoidanceCandidatesTimestamp == inow) {
		return 0;
	}
	const BSSimpleList<Actor*>* plist = ReferenceProcessLists.GetAliveActorList();
	DetailedActorPathHandler::uiAvoidanceCandidatesTimestamp = inow;
	DetailedActorPathHandler::AvoidanceCandidates.RemoveAll();
	DetailedActorPathHandler::uiAvoidanceCandidatesMaxSize = plist->ItemsInList();
	DetailedActorPathHandler::AvoidanceCandidates.SetSize(DetailedActorPathHandler::uiAvoidanceCandidatesMaxSize);
	DetailedActorPathHandler::uiAvoidanceCandidatesSize = 0;
	while (plist) {
		Actor* pobj = plist->m_item;
		if (pobj) {
			NiNode* ptheir3d = pobj->Get3D();
			if (ptheir3d) {
				if (static_cast<uint8_t>(ptheir3d->QFadedIn())) {
					bhkCharacterController* CharController;
					double LODDistance;
					if (!pobj->GetCharController() ||
						(!static_cast<uint8_t>(ptheir3d->GetNotVisible()) ||
							(CharController = pobj->GetCharController(),
								(LODDistance = CharController->GetLODDistance(),
									!(fLODActorAvoidanceDistance.Float() < LODDistance))))) {
						++DetailedActorPathHandler::uiAvoidanceCandidatesSize;
						DetailedActorPathHandler::AvoidanceCandidates.Add(pobj);
					}
				}
			}
		}
		plist = plist->m_pkNext;
	}
	return 1;
}

bool DetailedActorPathHandler::CheckDetection(Actor* apActor, DetailedActorPathHandler* apHandler, Actor* apOtherActor,
	const NiPoint3& aLocation, const NiPoint3& aVelocityDir, float afDistanceCheck, float afAvoidAngleCone,
	float& afOutDetection, bool abCheckOnly) {
	ActorPackage* pactorPackage;
	float flen;
	afOutDetection = 0.0f;
	if (apHandler &&
		((apHandler->eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE ||
			 apHandler->eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH) &&
			apHandler->ActorAvoidList.IsInList(apOtherActor))) {
		return 0;
	}
	const NiPoint3* a2 = &aLocation;
	const NiPoint3* v10 = &apOtherActor->GetLocationOnReference();
	NiPoint3 actorToOtherDir = *v10 - *a2;
	float factorForwardLength = apActor->GetForwardLength();
	double Radius = apOtherActor->GetRadius();
	float fdistanceCheck = Radius + afDistanceCheck + factorForwardLength;
	float fdistanceCheckSqr = fdistanceCheck * fdistanceCheck;
	float fsqrXYLength = actorToOtherDir.SqrXYLength();
	if (!(fdistanceCheckSqr > fsqrXYLength)) {
		return 0;
	}
	if (!(std::fabs(actorToOtherDir.z) < 180.0)) {
		return 0;
	}
	if (apActor->IsInCombat()) {
		Actor* v13 = apActor->GetCombatTarget();
		if (apOtherActor == v13) {
			return 0;
		}
	}
	if (apActor->pCurrentProcess) {
		BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
		pactorPackage = CurrentAIProcess->GetActorPackageThatIsRunning();
	} else {
		pactorPackage = 0;
	}
	if (pactorPackage) {
		Actor* Target = static_cast<Actor*>(pactorPackage->pTarget);
		if (Target == apOtherActor && pactorPackage->pPackage->GetPackType() != PACKAGE_TYPE_FOLLOW) {
			return 0;
		}
	}
	actorToOtherDir.z = 0.0f;
	float fdist = actorToOtherDir.UnitizeGetLength();
	bool bignoreCone = 0;
	float* v15 = &gs_fAvoidanceAvoidAllRadius_Pathfinding.Float();
	if (*v15 > static_cast<double>(fdist)) {
		bignoreCone = 1;
	}
	fdist = fdist - factorForwardLength;
	fdist = BSMAX(fdist, 0.0f);
	float fdot = actorToOtherDir.Dot(aVelocityDir);
	if (!(afAvoidAngleCone <= static_cast<double>(fdot)) && !bignoreCone) {
		return 0;
	}
	if (abCheckOnly) {
		return 1;
	}
	flen = fdist / fdistanceCheck;
	BSClamp(flen, 0.0f, 1.0f);
	int32_t idist = static_cast<int32_t>(flen * 4.0);
	float fdetection = fdot;
	switch (idist) {
	case 1: {
		goto LABEL_26;
	}
	case 2: {
		goto LABEL_25;
	}
	case 3:
	case 4: {
		fdetection = fdetection * fdetection;
	LABEL_25:;
		fdetection = fdetection * fdetection;
	LABEL_26:;
		fdetection = fdetection * fdetection;
		break;
	}
	default: {
		break;
	}
	}
	afOutDetection = fdetection;
	return 1;
}

// Runs the base filter's constructor first, then stores the vtable, the door ID and the three flags: both triangle
// flags clear and the whole-path flag set.
DoorInPathFilter::DoorInPathFilter(uint32_t aiDoorID)
	: Pathing::CheckLineOfSightFilter(), DoorID(aiDoorID), bEnterTriWithDoor(false), bLeaveTriWithDoor(false),
	  bEntirePathInDoor(true) {}

bool DoorInPathFilter::CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) {
	NavMeshClosedDoorInfo* v7;
	NavMeshClosedDoorInfo* closedDoorInfo;
	int8_t v10 = 0;
	NavMeshTriangle* triangle = hEdge.pNavMesh->GetTriangle(hEdge.usTriangle);
	if (triangle->GetFlag(4096)) {
		closedDoorInfo = hEdge.pNavMesh->GetClosedDoorInfoForTriangle(hEdge.usTriangle);
		if (closedDoorInfo) {
			if ((closedDoorInfo->DoorFormID == DoorID) || (!DoorID)) {
				v10 = 1;
			}
		}
	}
	int8_t v9 = 0;
	NavMeshTriangle* v4 = hMatchingEdge.pNavMesh->GetTriangle(hMatchingEdge.usTriangle);
	if (v4->GetFlag(4096)) {
		v7 = hMatchingEdge.pNavMesh->GetClosedDoorInfoForTriangle(hMatchingEdge.usTriangle);
		if (v7) {
			if ((v7->DoorFormID == DoorID) || (!DoorID)) {
				v9 = 1;
			}
		}
	}
	if (!v10) {
		bEntirePathInDoor = (bool)0;
	}
	if (!v9) {
		bEntirePathInDoor = (bool)0;
	}
	if (v10) {
		if (!v9) {
			bLeaveTriWithDoor = (bool)1;
		}
	}
	if (!v10) {
		if (v9) {
			bEnterTriWithDoor = (bool)1;
		}
	}
	return (bool)1;
}

bool PathingRequest::GetFailIfGoalOccupied() {
	return bFailIfGoalOccupied;
}

bool PathingSolution::IsDoorInPath(uint32_t aDoorID) {
	return DoorIDs.Find(aDoorID, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) != -1;
}

void NiMatrix3::MakeZRotationQ(float fAngle) {
	float _cs_1;
	float sn;
	NiSinCos_Fast(fAngle, &sn, &_cs_1);
	m_pEntry[0][0] = _cs_1;
	m_pEntry[0][1] = sn;
	m_pEntry[0][2] = 0.0;
	m_pEntry[1][0] = -sn;
	m_pEntry[1][1] = _cs_1;
	m_pEntry[1][2] = 0.0;
	m_pEntry[2][0] = 0.0;
	m_pEntry[2][1] = 0.0;
	m_pEntry[2][2] = 1.0;
}
