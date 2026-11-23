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

#include <cmath>
#include <cstdint>
#include <limits>
#include <new>

static SettingT<INISettingCollection> fAvoidanceDefaultWaitTime("fAvoidanceDefaultWaitTime:Pathfinding", 5.0f);
static SettingT<INISettingCollection> fAvoidanceIgnoreTime("fAvoidanceIgnoreTime:Pathfinding", 5.0f);
static SettingT<INISettingCollection> fAvoidanceIgnoreMinTime("fAvoidanceIgnoreMinTime:Pathfinding", 2.5f);
static SettingT<INISettingCollection> fAvoidanceDetectionTime("fAvoidanceDetectionTime:Pathfinding", 0.5f);
static SettingT<INISettingCollection> fAvoidanceAvoidNodeCost("fAvoidanceAvoidNodeCost", 2.0f);

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> gs_bUseActorAvoidance_Pathfinding("bUseActorAvoidance:Pathfinding", true);
static SettingT<INISettingCollection>
    bINIUseFartherCheckForAvoidance("bUseFartherCheckForAvoidance:Pathfinding", false);
SettingT<INISettingCollection> fAvoidanceMinWaitTime("fAvoidanceMinWaitTime:Pathfinding", 0.25f);
SettingT<INISettingCollection> gs_fAvoidanceTimeCheck_Pathfinding("fAvoidanceTimeCheck:Pathfinding", 0.75f);
SettingT<INISettingCollection> gs_fRepathingWaitDistance_Pathfinding("fRepathingWaitDistance:Pathfinding", 20.0f);
SettingT<INISettingCollection> gs_fAvoidanceAvoidAllRadius_Pathfinding("fAvoidanceAvoidAllRadius:Pathfinding", 100.0f);
SettingT<INISettingCollection> gs_fAITurnSpeedScale_Pathfinding("fAITurnSpeedScale:Pathfinding", 1.5f);
SettingT<INISettingCollection> gs_fAICombatTurnSpeedScale_Pathfinding("fAICombatTurnSpeedScale:Pathfinding", 2.5f);
SettingT<INISettingCollection> gs_fCreatureTurnSpeedScale_Pathfinding("fCreatureTurnSpeedScale:Pathfinding", 1.25f);
SettingT<INISettingCollection>
    gs_fCreatureCombatTurnSpeedScale_Pathfinding("fCreatureCombatTurnSpeedScale:Pathfinding", 1.25f);
SettingT<INISettingCollection> fLODActorAvoidanceDistance("fLODActorAvoidanceDistance:General", 500.0f);
SettingT<INISettingCollection> fRotationSpeedIncrease("fRotationSpeedIncrease:Pathfinding", 0.43633232f);
NiTPrimitiveArray<Actor*> DetailedActorPathHandler::AvoidanceCandidates(0, 1);

using ActorAvoidanceArray = BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>;

DetailedActorPathHandler::ActorAvoidanceData::ActorAvoidanceData()
  : pActor(nullptr), fDetection(-0x1.fffffe0000000p+127f) {}

DetailedActorPathHandler::ActorAvoidanceData::ActorAvoidanceData(Actor* apActor, float afDetection)
  : pActor(apActor), fDetection(afDetection) {}

DetailedActorPathHandler::ActorAvoidanceData*
DetailedActorPathHandler::ActorAvoidanceData::Copy(const DetailedActorPathHandler::ActorAvoidanceData* apSource) {
    *this = *apSource;
    return this;
}

// BlockedTargetPoint and the timers are deliberately left unset by the constructor.
DetailedActorPathHandler::DetailedActorPathHandler(Actor* apActor, ActorMover* apMover)
  : ActorPathHandler(apActor, apMover), fTetherMinMove(10.0f), fTetherDistFromActor(20.0f), MoveDelta(NiPoint3::ZERO),
    TetherTargetPoint(NiPoint3::ZERO), LastLocation(
                                           std::numeric_limits<float>::max(),
                                           std::numeric_limits<float>::max(),
                                           std::numeric_limits<float>::max()
                                       ),
    LastTetherPoint(NiPoint3::ZERO), ActorAvoidList(), MoveMode(0), iCurrentNodeIndex(0), fPathLength(0.0f),
    fCurrentSplineT(0.0f), fAvoidanceDetectionValue(0.0f), fTimeInAnim(1.0f), fRunWalkTime(0.0f), fMinRotateTime(-1.0f),
    fGoalRadiusSqr(0.0f), fDistanceTraveled(0.0f), fIgnoreLookAtPointTimer(0.0f), fLastRotationApplied(0.0f),
    bIgnoreInitialRotation(false), bIgnoreInitialRotationKeepActorHeading(false),
    fIgnoreInitialRotationTetherParam(0.0f), eState(DAP_GOAL_REACHED), eBlendRunning(PATH_RUNNING_BLEND_INVALID),
    eAvoidState(PATH_AVOID_INVALID), pCurrentDoor(nullptr), bEndUpdatePath(false), bWaitingForUpdate(false),
    bIgnoreLookAtPoint(false), bDeltaComputedThisFrame(false) {}

// BlockedTargetPoint and some timers are deliberately left unset; eState is UNK_0 with a path or a
// request, else DAP_GOAL_REACHED.
DetailedActorPathHandler::DetailedActorPathHandler(
    Actor* apActor,
    ActorMover* apMover,
    NiPointer<PathingSolution>& apPath,
    PathingRequest* apRequest
)
  : ActorPathHandler(apActor, apMover, apPath, apRequest), fTetherMinMove(10.0f), fTetherDistFromActor(20.0f),
    MoveDelta(NiPoint3::ZERO), TetherTargetPoint(NiPoint3::ZERO), LastLocation(
                                                                      std::numeric_limits<float>::max(),
                                                                      std::numeric_limits<float>::max(),
                                                                      std::numeric_limits<float>::max()
                                                                  ),
    LastTetherPoint(NiPoint3::ZERO), ActorAvoidList(), MoveMode(0), iCurrentNodeIndex(0), fPathLength(0.0f),
    fCurrentSplineT(0.0f), fAvoidanceDetectionValue(0.0f), fTimeInAnim(1.0f), fRunWalkTime(0.0f), fMinRotateTime(-1.0f),
    fGoalRadiusSqr(0.0f), fDistanceTraveled(0.0f), fIgnoreLookAtPointTimer(0.0f), fLastRotationApplied(0.0f),
    bIgnoreInitialRotation(false), bIgnoreInitialRotationKeepActorHeading(false),
    fIgnoreInitialRotationTetherParam(0.0f),
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
    if (eAvoidState != DetailedActorPathHandler::PATH_AVOID_WAITING &&
        eAvoidState != DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE) {
        eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
        fActorAvoidanceTimer = 0.0f;
        fWaitTime = 0.0f;
    }
    MoveDelta = NiPoint3::ZERO;
    bRotateMode = false;
    bDeltaComputedThisFrame = false;
    bMovementBlocked = false;
    fBlockTime = 0.0f;
    iUnblockDirection = 0;
    fAlternateFacingAngle = 0.0f;
    fBlockedStartAngle = 0.0f;
    fLastFrameMinMoveDelta = 0.0f;
    LastFrameMinMoveDelta = NiPoint3::ZERO;
    LastLocation = pActor->GetLocationOnReference();
    fFallTime = 0.0f;
    fMinRotateTime = -1.0f;
    if (spPath->GetNodeCount() >= 2) {
        if (spPath->IsIncomplete()) {
            fGoalRadiusSqr = 1600.0f;
        }
        else {
            double dTargetRadius = spRequest->fTargetRadius;
            float fRadiusSqr = spRequest->fTargetRadius * dTargetRadius;
            fGoalRadiusSqr = BSMAX(fRadiusSqr, 0.1f);
            if (spPath->GetLastNode()->IsTeleportDoorNode()) {
                fGoalRadiusSqr = 6400.0f;
            }
        }
        if (spPath->GetLastNode()->IsTransitionNode()) {
            bEndUpdatePath = true;
        }
        ComputeMinTetherDistance();
        bool bpathTransition = bWaitingForUpdate;
        FindPathStartingLocation(afPercentTraveled);
        bIgnoreInitialRotation = false;
        bIgnoreInitialRotationKeepActorHeading = false;
        fIgnoreInitialRotationTetherParam = 0.0f;
        if (!pActor->IsInCombat() &&
            static_cast<double>(fTetherDistFromActor) > spPath->ComputeDetailedPathLength() &&
            !pActorMover->HasLookAtTarget()) {
            bIgnoreInitialRotation = true;
            bIgnoreInitialRotationKeepActorHeading = true;
            fIgnoreInitialRotationTetherParam = static_cast<float>(spPath->GetNodeCount());
        }
        if (CheckGoalReached() == ActorPathHandler::PATH_HANDLER_RESULT_OK) {
            float fgoalAngle;
            NiPoint3 dirVec;
            if (bIgnoreLookAtPoint || !pActorMover->GetLookAtPoint(dirVec)) {
                dirVec = TetherTargetPoint;
            }
            dirVec -= pActor->GetLocationOnReference();
            double dMaxVertical = dirVec.Length() * 0.949999988079071;
            if (std::fabs(dirVec.z) >= dMaxVertical) {
                fgoalAngle = pActor->GetHeading(false);
            }
            else {
                fgoalAngle = GetZAngleFromVector(dirVec);
            }
            if (!bpathTransition) {
                if (afPercentTraveled == 0.0) {
                    InitRotationMode(fgoalAngle, false, bpathTransition);
                }
                else {
                    pActor->SetHeading(fgoalAngle);
                }
            }
        }
        eState = DetailedActorPathHandler::DAP_MOVING;
        if (afDeltaT > 0.0) {
            return Update(afDeltaT);
        }

        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }

    ResetMoveModeAndDelta();
    bWaitingForUpdate = false;
    TetherTargetPoint = pActor->GetLocationOnReference();
    LastTetherPoint = TetherTargetPoint;
    eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
    return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
}

bool DetailedActorPathHandler::InitRotationOnly(float afTargetAngle, bool abForceTurn) {
    bWaitingForUpdate = false;
    eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
    eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
    eAvoidState = PATH_AVOID_OK;
    fAvoidanceDetectionValue = 0.0;
    fDistanceTraveled = 0.0;
    fPathLength = 0.0;
    spPath = static_cast<PathingSolution*>(nullptr);
    spRequest = static_cast<PathingRequest*>(nullptr);
    bIgnoreInitialRotation = false;
    bIgnoreInitialRotationKeepActorHeading = false;
    fIgnoreInitialRotationTetherParam = 0.0;
    if (InitRotationMode(afTargetAngle, abForceTurn, false)) {
        eState = DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH;
        return true;
    }

    eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
    return false;
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::Update(float afDeltaT) {
    NiPoint3 kGoalLocation;
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
    bDeltaComputedThisFrame = true;
    bhkCharacterController* pcon = pActor->GetCharController();
    if (afDeltaT == 0.0 || !pcon) {
        ResetMoveModeAndDelta();
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (pPlayer->IsSleepingorResting()) {
        return UpdateWhilePlayerWaits(afDeltaT);
    }
    if (pcon->GetState() == HK_CHARACTER_IN_AIR) {
        fFallTime = pcon->QFallTime();
        if (fFallTime > 0.4000000059604645) {
            ResetMoveModeAndDelta();
            return ActorPathHandler::PATH_HANDLER_RESULT_OK;
        }
    }
    else {
        if (fFallTime > 0.6499999761581421) {
            ResetMoveModeAndDelta();
            return ActorPathHandler::PATH_HANDLER_RESULT_FELL_OFF_PATH;
        }
        fFallTime = 0.0f;
    }
    if (!bRotateMode && !bWaitingForUpdate) {
        ActorPathHandler::PATH_HANDLER_RESULT eresult = CheckGoalReached();
        if (eresult && (eresult != ActorPathHandler::PATH_HANDLER_RESULT_DONE || !CheckForEndRotate())) {
            ResetMoveModeAndDelta();
            eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
            return eresult;
        }
    }
    if (!bWaitingForUpdate) {
        UpdateTether(afDeltaT);
    }
    if (bIgnoreInitialRotation && fIgnoreInitialRotationTetherParam <= static_cast<double>(fCurrentSplineT)) {
        bIgnoreInitialRotation = false;
        bIgnoreInitialRotationKeepActorHeading = false;
        fIgnoreInitialRotationTetherParam = 0.0f;
    }
    if (bRotateMode) {
        float fnewHeading = UpdateRotation(afDeltaT);
        pActor->SetHeading(fnewHeading);
        float fheading = 0.0f;
        float flooking = 0.0f;
        ComputeHeadingAndLooking(fheading, flooking);
        UpdateLooking(afDeltaT, flooking);
        MoveDelta = NiPoint3::ZERO;
        fLastFrameMinMoveDelta = 0.0f;
        LastFrameMinMoveDelta = NiPoint3::ZERO;
        pActorMover->DebugOnEvent();
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (eState == DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH) {
        ResetMoveModeAndDelta();
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_WAITING) {
        DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN avoidRet;
        fWaitTime = fWaitTime - afDeltaT;
        if (!(fWaitTime > 0.0)) {
            fWaitTime = 0.0f;
            eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
            ResetMoveModeAndDelta();
            return ActorPathHandler::PATH_HANDLER_RESULT_OK;
        }
        avoidRet = UpdateActorAvoidance(afDeltaT);
        if (avoidRet == DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_OK) {
            fActorAvoidanceTimer = fActorAvoidanceTimer - afDeltaT;
            if (fActorAvoidanceTimer <= 0.0) {
                fWaitTime = 0.0f;
                eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
            }
            ResetMoveModeAndDelta();
            return ActorPathHandler::PATH_HANDLER_RESULT_OK;
        }
        if (avoidRet != DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_FAIL) {
            fActorAvoidanceTimer = fAvoidanceMinWaitTime.Float();
            ResetMoveModeAndDelta();
            return ActorPathHandler::PATH_HANDLER_RESULT_OK;
        }
        ResetMoveModeAndDelta();
        return ActorPathHandler::PATH_HANDLER_RESULT_CANT_MOVE;
    }
    DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE eDoorState = HandleDoorOpening();
    if (eDoorState == DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE_WAITING_FOR_DOOR_TO_OPEN) {
        ResetMoveModeAndDelta();
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (eDoorState == DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE_FAILED) {
        ResetMoveModeAndDelta();
        return ActorPathHandler::PATH_HANDLER_RESULT_CANT_USE_DOOR;
    }
    ComputeMoveMode(afDeltaT);
    if (!UpdateStaticAvoidance(afDeltaT)) {
        if (eState != DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH) {
            eAvoidState = DetailedActorPathHandler::PATH_AVOID_FAILED;
            return ActorPathHandler::PATH_HANDLER_RESULT_CANT_MOVE;
        }
        ResetMoveModeAndDelta();
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
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
                        NiPointer<PathingRequest> spnewRequest(new PathingRequest);
                        spRequest->PathingRequest::CopyTo(spnewRequest.m_pObject);
                        if (!spPath.m_pObject) {
                            eAvoidState = DetailedActorPathHandler::PATH_AVOID_WAITING;
                            ResetMoveModeAndDelta();
                            return ActorPathHandler::PATH_HANDLER_RESULT_OK;
                        }
                        if (!spPath->IsIncomplete() || spRequest->Destination.QUnused()) {
                            VirtualPathingNode* pLastVirtualNode = spPath->GetLastVirtualNode();
                            spnewRequest->SetDestination(*&pLastVirtualNode->Location);
                        }
                        else {
                            spnewRequest->SetDestination(*&spRequest->Destination);
                        }
                        spnewRequest->ClearInitialPathHeading();
                        eAvoidState = DetailedActorPathHandler::PATH_AVOID_REPATH;
                        pActorMover->BackGroundBuildPath(spnewRequest.m_pObject);
                        if (pActorMover->eState == ACTOR_MOVER_STATE_FINISHED) {
                            ResetMoveModeAndDelta();
                            bWaitingForUpdate = false;
                            eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
                        }
                        if (bWaitingForUpdate) {
                            spPath = static_cast<PathingSolution*>(nullptr);
                            if (eState == DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH) {
                                ResetMoveModeAndDelta();
                                return ActorPathHandler::PATH_HANDLER_RESULT_OK;
                            }
                        }
                        else {
                            eAvoidState = DetailedActorPathHandler::PATH_AVOID_OK;
                        }
                        break;
                    }
                    case DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN_FAIL: {
                        ResetMoveModeAndDelta();
                        return ActorPathHandler::PATH_HANDLER_RESULT_CANT_MOVE;
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
    float fnewHeading = UpdateHeading(afDeltaT, fslowDown);
    pActor->SetHeading(fnewHeading);
    pActorMover->DebugOnEvent();
    if (fslowDown > 0.0) {
        MoveDelta *= fslowDown;
    }
    RotateMoveDelta(afDeltaT);
    fLastFrameMinMoveDelta = MoveDelta.XYLength();
    MoveModeRot.MakeZRotationQ(pActor->GetHeading(false));
    LastFrameMinMoveDelta = MoveModeRot * MoveDelta;
    if (eState == DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING) {
        NiPoint3 tetherDir = TetherTargetPoint - LastTetherPoint;
        tetherDir.z = 0.0;
        NiPoint3 actorToTetherDir = TetherTargetPoint - pActor->GetLocationOnReference();
        actorToTetherDir.z = 0.0;
        float fdot = tetherDir.Dot(actorToTetherDir);
        if (fdot <= 0.0) {
            eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
        }
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }

    if (spPath.m_pObject) {
        uint32_t uiNextNodeIndex = iCurrentNodeIndex + 1;
        if (uiNextNodeIndex >= spPath->GetNodeCount()) {
            PathingNode* pLastNode = spPath->GetLastNode();
            ::new (&kGoalLocation) NiPoint3(pLastNode->Location.GetLocation());
            float fdistToGoal = (pActor->GetLocationOnReference() - kGoalLocation).XYLength();
            if (fdistToGoal < static_cast<double>(fLastFrameMinMoveDelta)) {
                float fmoveDeltaZ = MoveDelta.z;
                MoveDelta.Unitize();
                MoveDelta.x = MoveDelta.x * fdistToGoal;
                MoveDelta.y = MoveDelta.y * fdistToGoal;
                MoveDelta.z = fmoveDeltaZ;
                fLastFrameMinMoveDelta = MoveDelta.XYLength();
                LastFrameMinMoveDelta = MoveDelta;
            }
        }
    }
    return ActorPathHandler::PATH_HANDLER_RESULT_OK;
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::NoPathUpdate(float afDeltaT) {
    ActorPathHandler::PATH_HANDLER_RESULT eresult = ActorPathHandler::PATH_HANDLER_RESULT_NONE;
    bool bimmobile = pActor->GetActorBase()->IsImmobile();
    if (!pActor->GetTurretBehavior() && !pActorMover->DidPathingFail() && !pActorMover->bForceMoveMode) {
        switch (eState) {
            case DetailedActorPathHandler::DAP_GOAL_REACHED: {
                spPath = static_cast<PathingSolution*>(nullptr);
                spRequest = static_cast<PathingRequest*>(nullptr);
                [[fallthrough]];
            }
            case DetailedActorPathHandler::UNK_0:
            case DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH: {
                if (!pActorMover->HasLookAtTarget()) {
                    MoveMode = pActorMover->GetPreferredMoveModeType();
                    eresult = ActorPathHandler::PATH_HANDLER_RESULT_DONE;
                }
                break;
            }
            case DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING: {
                Update(afDeltaT);
                eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
                break;
            }
            case DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH: {
                float fnewHeading = UpdateRotation(afDeltaT);
                pActor->SetHeading(fnewHeading);
                MoveDelta = NiPoint3::ZERO;
                fLastFrameMinMoveDelta = 0.0f;
                LastFrameMinMoveDelta = NiPoint3::ZERO;
                bDeltaComputedThisFrame = true;
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
                bIgnoreLookAtPoint = false;
                eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
            }
        }
        if (eresult == ActorPathHandler::PATH_HANDLER_RESULT_NONE) {
            NiPoint3 lookatPoint;
            if (pActorMover->GetLookAtPoint(lookatPoint) && !pActor->IsInDialoguewithPlayer()) {
                float factangle = ClampAngle(pActor->GetHeading(false));
                NiPoint3 firingLocation = CombatUtilities::GetAttackerFiringLocation(pActor);
                // The squared length is kept in double precision for the compare below.
                double dDistSqr = (firingLocation - lookatPoint).SqrXYLength();
                double dRadius = pActor->GetRadius();
                if (pActor->GetRadius() * dRadius * 0.25 > dDistSqr) {
                    eresult = ActorPathHandler::PATH_HANDLER_RESULT_OK;
                }
                if (eresult == ActorPathHandler::PATH_HANDLER_RESULT_NONE) {
                    float fsignedDelta;
                    float fdir;
                    NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(pActor, lookatPoint);
                    float fgoalAngle = ClampAngle(rot.z);
                    float fgoalLookAngle = rot.x;
                    if (pActor->GetRadius() >= 100.0) {
                        NiPoint3 delta = lookatPoint - pActor->GetLocationOnReference();
                        delta.z = 0.0f;
                        float fZAngle = GetZAngleFromVector(delta);
                        fsignedDelta = GetAngleDelta(factangle, fZAngle, fdir);
                    }
                    else {
                        fsignedDelta = GetAngleDelta(factangle, fgoalAngle, fdir);
                    }
                    float fangleDelta = std::fabs(fsignedDelta);
                    float frotateMax = pActor->GetTurningSpeed() * 0.01745329238474369 * afDeltaT;
                    if (pActor->IsCreature()) {
                        float fCreatureTurnSpeedScale;
                        if (pActor->IsInCombat()) {
                            fCreatureTurnSpeedScale = gs_fCreatureCombatTurnSpeedScale_Pathfinding.Float();
                        }
                        else {
                            fCreatureTurnSpeedScale = gs_fCreatureTurnSpeedScale_Pathfinding.Float();
                        }
                        frotateMax = frotateMax * fCreatureTurnSpeedScale;
                    }
                    else {
                        float fTurnSpeedScale;
                        if (pActor->IsInCombat()) {
                            fTurnSpeedScale = gs_fAICombatTurnSpeedScale_Pathfinding.Float();
                        }
                        else {
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
                        if (fangleDelta > 0.017453292f) {
                            if (!bimmobile) {
                                fMinRotateTime = fActorTurnAnimMinTime.Float() - afDeltaT;
                            }
                            if (fdir == 1.0) {
                                inewMoveMode = pActorMover->GetPreferredMoveModeType() | 32;
                                factangle = factangle + fclampedAngleDelta;
                            }
                            else if (fdir == -1.0) {
                                inewMoveMode = pActorMover->GetPreferredMoveModeType() | 16;
                                factangle = factangle - fclampedAngleDelta;
                            }
                        }
                        else {
                            inewMoveMode = pActorMover->GetPreferredMoveModeType();
                            factangle = fclampedAngleDelta * fdir + factangle;
                        }
                    }
                    else {
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
    ResetStaticAvoidance();
    ResetActorAvoidance();
    bRotateMode = false;
    eState = DetailedActorPathHandler::DAP_MOVING;
    eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
    fFallTime = 0.0f;
    if (spRequest.m_pObject && spRequest->GetWaitIfPlayerWaits()) {
        pActor->SetHeading(pPlayer);
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }

    PathingNode* plastNode = spPath->GetLastNode();
    const PathingLocation* loc = &plastNode->Location;
    NiPoint3 goalPoint = loc->GetLocation();
    float fgoalAngle = GetZAngleFromVector(plastNode->Tangent);
    TESObjectCELL* pgoalCell = loc->GetCell();
    TESWorldSpace* pgoalWS = loc->pWorldSpace;
    pActor->GetCurrentSpeed();
    int32_t iNextNodeIndex = pActorMover->iNextNode + 1;
    bool bfinishedPath = iNextNodeIndex == spPath->GetVirtualNodeCount();
    if (pActor->GetSitSleepState() != SIT_SLEEP_IS_SITTING || pActor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING) {
        if (spRequest.m_pObject && spRequest->bFaceTargetAtGoal && bfinishedPath) {
            fgoalAngle = spRequest->fGoalAngle;
        }
        else if (pActorMover->HasLookAtTarget()) {
            NiPoint3 lookatPoint;
            pActorMover->GetLookAtPoint(lookatPoint);
            NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(pActor, lookatPoint);
            fgoalAngle = ClampAngle(rot.z);
        }
        else {
            uint32_t inodeCount = spPath->GetNodeCount();
            if (inodeCount <= 1) {
                fgoalAngle = pActor->GetHeading(false);
            }
            else {
                NiPoint3 nextToLastNode = spPath->GetNode(inodeCount - 2)->Location.GetLocation();
                NiPoint3 lastNode = spPath->GetNode(inodeCount - 1)->Location.GetLocation();
                NiPoint3 movedirDelta = lastNode - nextToLastNode;
                fgoalAngle = GetZAngleFromVector(movedirDelta);
            }
        }
    }
    pActor->MoveActorAndFollowers(goalPoint, fgoalAngle, pgoalCell, pgoalWS, true, false);
    ActorPathHandler::PATH_HANDLER_RESULT eresult = ActorPathHandler::PATH_HANDLER_RESULT_DONE;
    ResetMoveModeAndDelta();
    eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
    if (!bfinishedPath) {
        return ActorPathHandler::PATH_HANDLER_RESULT_DONE_SLEEP_SPECIAL;
    }
    return eresult;
}

float DetailedActorPathHandler::GetDistanceTraveled() {
    if (spPath) {
        return fDistanceTraveled;
    }

    return 0.0;
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
    NiPoint3 kLocation;
    if (spPath.m_pObject && spPath->GetNodeCount()) {
        PathingNode* pNode;
        uint32_t uiNextNodeIndex = iCurrentNodeIndex + 1;
        if (uiNextNodeIndex >= spPath->GetNodeCount()) {
            pNode = spPath->GetLastNode();
        }
        else {
            pNode = spPath->GetNode(iCurrentNodeIndex + 1);
        }
        kLocation = pNode->Location.GetLocation();
        return kLocation;
    }

    kLocation = pActor->GetLocationOnReference();
    return kLocation;
}

bool DetailedActorPathHandler::GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) {
    if (!spPath) {
        return false;
    }
    if (aiIndex >= spPath->GetNodeCount()) {
        return false;
    }
    PathingNode* pNode = spPath->GetNode(aiIndex);
    arLocation = pNode->PathingNode::GetLocation().GetLocation();
    return true;
}

// The path length left to walk, never below 0.
float DetailedActorPathHandler::GetDistanceRemaining() {
    float fRemaining = fPathLength - GetDistanceTraveled();
    return BSMAX(fRemaining, 0.0f);
}

NiPoint3 DetailedActorPathHandler::ComputeApproximateLocation(float afDeltaT) {
    NiPoint3 kLocation;
    if (pActorMover->DidPathingFail()) {
        kLocation = pActor->GetLocationOnReference();
        return kLocation;
    }

    NiPointer<PathingSolution> sppath(spPath);
    if (sppath.m_pObject && sppath->GetNodeCount()) {
        float fspeed;
        if ((pActorMover->GetPreferredMoveMode() & 512) != 0) {
            fspeed = pActor->GetRunSpeed();
        }
        else {
            fspeed = pActor->GetWalkSpeed();
        }
        float fdistance = fspeed * afDeltaT;
        ::new (&kLocation) NiPoint3(sppath->ComputeApproxLocationForDistance(fCurrentSplineT, fdistance));
        return kLocation;
    }

    kLocation = pActor->GetLocationOnReference();
    return kLocation;
}

uint32_t DetailedActorPathHandler::GetCurrentNodeIndex() {
    return iCurrentNodeIndex;
}

bool DetailedActorPathHandler::IsDetailedPathHandler() {
    return true;
}

void DetailedActorPathHandler::SetWaitForUpdatedPath(PathingRequest* apRequest) {
    NiPoint3 kDestination;
    NiPoint3 newPathOrigin;
    if (spPath.m_pObject && spPath->GetNodeCount() && !pPlayer->IsSleepingorResting()) {
        fDistanceTraveled = 0.0f;
        if (bWaitingForUpdate) {
            PathingLocation kOrigin(TetherTargetPoint, pActor);
            apRequest->SetOrigin(kOrigin);
        }
        else if (eState == DetailedActorPathHandler::DAP_MOVING) {
            bWaitingForUpdate = true;
            NiPoint3 actorCurrent = pActor->GetLocationOnReference();
            if (bMovementBlocked || IsWaitingForAvoidance() || bRotateMode) {
                TetherTargetPoint = actorCurrent;
                LastTetherPoint = actorCurrent;
                eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
                PathingLocation kOrigin(pActor);
                apRequest->SetOrigin(kOrigin);
            }
            else {
                ::new (&kDestination) NiPoint3(apRequest->Destination.GetLocation());
                float fnewLen = (apRequest->Origin.GetLocation() - kDestination).XYLength();
                float fwaitDist = gs_fRepathingWaitDistance_Pathfinding.Float();
                if ((MoveMode & ActorMover::MF_RUNNING) != 0) {
                    fwaitDist = fwaitDist + gs_fRepathingWaitDistance_Pathfinding.Float();
                }
                fwaitDist = fwaitDist + fwaitDist;
                if (fwaitDist > static_cast<double>(fnewLen)) {
                    fwaitDist = fnewLen * 0.5;
                }
                float fdiff = (actorCurrent - TetherTargetPoint).XYLength();
                if (fwaitDist > static_cast<double>(fdiff)) {
                    float fupdateLen = fwaitDist - fdiff;
                    UpdateTetherPosition(fupdateLen);
                }
                else {
                    float fupdateLen = fdiff - fwaitDist;
                    UpdateTetherPosition(-fupdateLen);
                }
                bool bfixupTether = true;
                if (pActor->GetParentCell() && pActor->GetParentCell()->GetInterior()) {
                    bfixupTether = false;
                }
                else if (pActor->GetWorldSpace()) {
                    if (pActor->GetWorldSpace()->GetCellFromWorldCoord(TetherTargetPoint)) {
                        bfixupTether = false;
                    }
                }
                if (bfixupTether) {
                    TetherTargetPoint = actorCurrent;
                    LastTetherPoint = actorCurrent;
                    eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
                }
                else {
                    NiPoint3 tetherTangent;
                    LastTetherPoint = actorCurrent;
                    eState = DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING;
                    newPathOrigin = TetherTargetPoint;
                    bool bSetInitialTangent = true;
                    float fnextSplineT = fCurrentSplineT + 0.10000000149011612;
                    spPath->ComputeLocationFromParameter(fnextSplineT, tetherTangent);
                    tetherTangent -= TetherTargetPoint;
                    float finitialAngle = GetZAngleFromVector(tetherTangent);
                    if (!ActorAvoidList.IsEmpty()) {
                        BSSimpleList<Actor*>* plist = &ActorAvoidList;
                        while (plist) {
                            Actor* pavoidActor = plist->m_item;
                            plist = plist->m_pkNext;
                            float favoidRadius = pavoidActor->GetRadius();
                            const NiPoint3& kAvoidLocation = pavoidActor->GetLocationOnReference();
                            const NiPoint3& kActorLocation = pActor->GetLocationOnReference();
                            if (NavMeshMath::SegmentCircleIntersection(
                                    kActorLocation,
                                    TetherTargetPoint,
                                    kAvoidLocation,
                                    favoidRadius
                                )) {
                                newPathOrigin = pActor->GetLocationOnReference();
                                bSetInitialTangent = false;
                                break;
                            }
                        }
                    }
                    {
                        PathingLocation kOrigin(newPathOrigin, pActor);
                        apRequest->SetOrigin(kOrigin);
                    }
                    if (bSetInitialTangent) {
                        spRequest->SetInitialPathHeading(finitialAngle);
                    }
                    else {
                        spRequest->ClearInitialPathHeading();
                    }
                }
            }
        }
    }
    else {
        eState = DetailedActorPathHandler::UNK_0;
    }
}

bool DetailedActorPathHandler::IsWaitingForAvoidance() {
    return eAvoidState == PATH_AVOID_WAITING;
}

bool DetailedActorPathHandler::IsMovementBlocked() {
    return bMovementBlocked;
}

bool DetailedActorPathHandler::ComputePathPassesThroughPlayersArea() {
    if (!spPath) {
        return false;
    }
    if (!spPath->IsValid()) {
        return false;
    }
    uint32_t uiNodeCount = spPath->GetNodeCount();
    uint32_t uiIndex = iCurrentNodeIndex + 1;
    bool bUseCell = true;
    TESObjectCELL* pParentCell = pPlayer->GetParentCell();
    TESWorldSpace* pParentWorldspace = pPlayer->GetWorldSpace();
    if (!pParentCell) {
        pParentCell = TES::pInstance->GetInterior();
        bUseCell = pParentCell != nullptr;
    }
    if (!bUseCell && !pParentWorldspace) {
        pParentWorldspace = TES::pInstance->GetWorldSpace();
        if (!pParentWorldspace) {
            return false;
        }
    }
    while (uiIndex < uiNodeCount) {
        PathingNode* pNode = spPath->GetNode(uiIndex);
        if (pNode) {
            PathingLocation* pLocation = static_cast<PathingLocation*>(&pNode->GetLocation());
            if (bUseCell) {
                if (pParentCell == pLocation->GetCell()) {
                    return true;
                }
            }
            else if (pParentWorldspace == pLocation->GetWorldspaceSimple()) {
                return true;
            }
            if (pNode->IsTeleportDoorNode()) {
                TESObjectREFR* pDoorRef = pNode->pDoorRef;
                if (pDoorRef) {
                    DoorTeleportData* pTeleport = pDoorRef->GetTeleport();
                    if (pTeleport) {
                        TESObjectREFR* pLinkedDoor = pTeleport->GetLinkedDoor();
                        if (pLinkedDoor) {
                            if (bUseCell) {
                                if (pParentCell == pLinkedDoor->GetParentCell()) {
                                    return true;
                                }
                            }
                            else if (pParentWorldspace == pLinkedDoor->GetWorldSpace()) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        ++uiIndex;
    }
    return false;
}

uint32_t DetailedActorPathHandler::GetMoveMode() {
    return MoveMode;
}

bool DetailedActorPathHandler::GetMoveDelta(NiPoint3& aDelta) {
    if (bDeltaComputedThisFrame) {
        bDeltaComputedThisFrame = false;
        aDelta = MoveDelta;
        return true;
    }

    aDelta = NiPoint3::ZERO;
    return false;
}

bool DetailedActorPathHandler::GetNextTetherPoint(NiPoint3& arPoint) {
    arPoint = TetherTargetPoint;
    return true;
}

void DetailedActorPathHandler::ForceMoveMode(uint32_t aiMoveMode) {
    MoveMode = aiMoveMode;
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
    }
    else if (pold->eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH) {
        eAvoidState = DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE;
        fActorAvoidanceTimer = pold->fActorAvoidanceTimer;
        fWaitTime = 0.0f;
        // Tests only the head's item, then passes the head item to AddHead.
        while (pold->ActorAvoidList.GetItem()) {
            ActorAvoidList.AddHead(pold->ActorAvoidList.GetItem());
            pold->ActorAvoidList.RemoveHead();
        }
    }
    bIgnoreLookAtPoint = pold->bIgnoreLookAtPoint;
    fIgnoreLookAtPointTimer = pold->fIgnoreLookAtPointTimer;
    if (pold->bWaitingForUpdate) {
        bWaitingForUpdate = true;
        LastTetherPoint = pold->LastTetherPoint;
        TetherTargetPoint = pold->TetherTargetPoint;
    }
}

void DetailedActorPathHandler::HandleDeletedReference(TESObjectREFR* apRef) {
    if (apRef == pCurrentDoor) {
        eDoorState = DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE_NONE;
        pCurrentDoor = nullptr;
    }
    if (apRef->IsActor()) {
        for (uint32_t i = 0; i < DetailedActorPathHandler::uiAvoidanceCandidatesSize; ++i) {
            if (DetailedActorPathHandler::AvoidanceCandidates[i] == apRef) {
                DetailedActorPathHandler::AvoidanceCandidates.RemoveAt(i);
                DetailedActorPathHandler::AvoidanceCandidates.Compact();
                --DetailedActorPathHandler::uiAvoidanceCandidatesSize;
            }
        }
        Actor* pAvoidActor = static_cast<Actor*>(apRef);
        if (ActorAvoidList.IsInList(pAvoidActor)) {
            ActorAvoidList.Remove(pAvoidActor);
            if (ActorAvoidList.IsEmpty()) {
                ResetActorAvoidance();
            }
        }
    }
}

void DetailedActorPathHandler::PrintCurrentStatus(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aLoc,
    DEBUGTEXT_JUSTIFY aJust
) {
    char acDetect[264];
    char acStatus[268];
    const NiPoint3& kLocation = pActor->GetLocationOnReference();
    apDebugText->PrintLine(
        aLoc,
        aJust,
        "DetailedPathHandler: Actor's Position %1.f %1.f %1.f",
        kLocation.x,
        kLocation.y,
        kLocation.z
    );
    BSstrcpy(acStatus, 260, "Pathing State: ");
    switch (eState) {
        case DetailedActorPathHandler::DAP_INITIALIZING: {
            BSstrcat(acStatus, 260, "Initializing");
            break;
        }
        case DetailedActorPathHandler::DAP_MOVING: {
            if (bRotateMode) {
                BSstrcat(acStatus, 260, "Rotating");
            }
            else if (bMovementBlocked) {
                BSstrcat(acStatus, 260, "Static Avoidance");
            }
            else {
                BSstrcat(acStatus, 260, "Moving");
            }
            break;
        }
        case DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING: {
            BSstrcat(acStatus, 260, "Moving While Waiting");
            break;
        }
        case DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH: {
            BSstrcat(acStatus, 260, "Waiting For Updated Path");
            break;
        }
        case DetailedActorPathHandler::DAP_GOAL_REACHED: {
            BSstrcat(acStatus, 260, "Goal Reached");
            break;
        }
        case DetailedActorPathHandler::DAP_ROTATING_WITHOUT_PATH:
        case DetailedActorPathHandler::UNK_7: {
            BSstrcat(acStatus, 260, "Rotating without path");
            break;
        }
        default: {
            BSstrcat(acStatus, 260, "Invalid");
            break;
        }
    }
    apDebugText->PrintLine(aLoc, aJust, acStatus);
    BSstrcpy(acStatus, 260, "Actor Avoid State: ");
    switch (eAvoidState) {
        case DetailedActorPathHandler::PATH_AVOID_OK: {
            BSstrcat(acStatus, 260, "OK");
            if (fAvoidanceDetectionValue > 0.0) {
                BSsprintf(acDetect, 260, " :detect %f", fAvoidanceDetectionValue);
                BSstrcat(acStatus, 260, acDetect);
            }
            break;
        }
        case DetailedActorPathHandler::PATH_AVOID_WAITING: {
            BSstrcat(acStatus, 260, "Waiting");
            break;
        }
        case DetailedActorPathHandler::PATH_AVOID_REPATH: {
            BSstrcat(acStatus, 260, "Repathing (building)");
            break;
        }
        case DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE: {
            BSstrcat(acStatus, 260, "Repath ignore");
            if (fAvoidanceDetectionValue > 0.0) {
                BSsprintf(acDetect, 260, " :detect %f", fAvoidanceDetectionValue);
                BSstrcat(acStatus, 260, acDetect);
            }
            break;
        }
        case DetailedActorPathHandler::PATH_AVOID_FAILED: {
            BSstrcat(acStatus, 260, "Failed");
            break;
        }
        default: {
            BSstrcat(acStatus, 260, "Invalid");
            break;
        }
    }
    apDebugText->PrintLine(aLoc, aJust, acStatus);
}

void DetailedActorPathHandler::ClearMovement() {
    MoveDelta = NiPoint3::ZERO;
    MoveMode = pActorMover->GetPreferredMoveModeType();
    fDistanceTraveled = 0.0f;
    spPath = nullptr;
    spRequest = nullptr;
    bWaitingForUpdate = false;
    eDoorState = PATH_HANDLER_DOOR_STATE_NONE;
    pCurrentDoor = nullptr;
    eAvoidState = PATH_AVOID_OK;
    ActorAvoidList.RemoveAll();
    eState = DetailedActorPathHandler::DAP_GOAL_REACHED;
}

bool DetailedActorPathHandler::WarpToNextNode() {
    // The filter is never destroyed.
    uint16_t ustri;
    if (spRequest.m_pObject && spRequest->fActorRadius > 100.0) {
        return false;
    }
    float fMinPathLength = 64.0f;
    float fMaxWarpDistance = 128.0f;
    float fdistToTether = (TetherTargetPoint - pActor->GetLocationOnReference()).XYLength();
    float fdistanceFromGoalThresold = 0.0f;
    if (spPath->IsIncomplete()) {
        fdistanceFromGoalThresold = pActor->GetLength() * 0.6000000238418579;
    }
    if (!spPath->GetNodeCount()) {
        return false;
    }
    if (fMaxWarpDistance > static_cast<double>(fdistToTether)) {
        float fupdateLen = fMaxWarpDistance - fdistToTether;
        UpdateTetherPosition(fupdateLen);
    }
    fdistToTether = (TetherTargetPoint - pActor->GetLocationOnReference()).XYLength();
    float fdistFromGoal = (spPath->GetLastNode()->Location.GetLocation() - TetherTargetPoint).XYLength();
    if (fMinPathLength > static_cast<double>(fdistToTether) ||
        fdistanceFromGoalThresold > static_cast<double>(fdistFromGoal)) {
        return false;
    }
    DoorInPathFilter filter(0);
    PathingLocation actorLoc(pActor);
    actorLoc.ResolveNavMeshInfo(nullptr);
    NavMeshPtr spmesh;
    if (actorLoc.GetNavMeshAndTriangle(spmesh, ustri)) {
        NavMeshTriangle* ptri = spmesh->GetTriangle(ustri);
        if (ptri) {
            filter.bEntirePathInDoor = ptri->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
        }
    }
    Pathing::CheckLineOfSight(actorLoc, TetherTargetPoint, nullptr, &filter);
    if (filter.bEnterTriWithDoor || filter.bLeaveTriWithDoor || filter.bEntirePathInDoor) {
        return false;
    }

    pActorMover->MoveActorToLocation(TetherTargetPoint);
    return true;
}

void DetailedActorPathHandler::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&MoveDelta, sizeof(MoveDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&BlockedTargetPoint, sizeof(BlockedTargetPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&TetherTargetPoint, sizeof(TetherTargetPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&LastLocation, sizeof(LastLocation), 0);
    apSaveGameBuffer->SaveDataEndian(&LastTetherPoint, sizeof(LastTetherPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&MoveMode, sizeof(MoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(&iCurrentNodeIndex, sizeof(iCurrentNodeIndex), 0);
    apSaveGameBuffer->SaveDataEndian(&fRotateGoal, sizeof(fRotateGoal), 0);
    apSaveGameBuffer->SaveDataEndian(&fPathLength, sizeof(fPathLength), 0);
    apSaveGameBuffer->SaveDataEndian(&fCurrentSplineT, sizeof(fCurrentSplineT), 0);
    apSaveGameBuffer->SaveDataEndian(&fActorAvoidanceTimer, sizeof(fActorAvoidanceTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&fAvoidanceDetectionValue, sizeof(fAvoidanceDetectionValue), 0);
    apSaveGameBuffer->SaveDataEndian(&iUnblockDirection, sizeof(iUnblockDirection), 0);
    apSaveGameBuffer->SaveDataEndian(&fAlternateFacingAngle, sizeof(fAlternateFacingAngle), 0);
    apSaveGameBuffer->SaveDataEndian(&fBlockedStartAngle, sizeof(fBlockedStartAngle), 0);
    apSaveGameBuffer->SaveDataEndian(&fBlockTime, sizeof(fBlockTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fTimeInAnim, sizeof(fTimeInAnim), 0);
    apSaveGameBuffer->SaveDataEndian(&fRunWalkTime, sizeof(fRunWalkTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fMinRotateTime, sizeof(fMinRotateTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fWaitTime, sizeof(fWaitTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fLastFrameMinMoveDelta, sizeof(fLastFrameMinMoveDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&fGoalRadiusSqr, sizeof(fGoalRadiusSqr), 0);
    apSaveGameBuffer->SaveDataEndian(&fFallTime, sizeof(fFallTime), 0);
    apSaveGameBuffer->SaveDataEndian(&fDistanceTraveled, sizeof(fDistanceTraveled), 0);
    apSaveGameBuffer->SaveDataEndian(&fIgnoreLookAtPointTimer, sizeof(fIgnoreLookAtPointTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&fTetherMinMove, sizeof(fTetherMinMove), 0);
    apSaveGameBuffer->SaveDataEndian(&fTetherDistFromActor, sizeof(fTetherDistFromActor), 0);
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(&eBlendRunning, sizeof(eBlendRunning), 0);
    apSaveGameBuffer->SaveDataEndian(&eAvoidState, sizeof(eAvoidState), 0);
    apSaveGameBuffer->SaveDataEndian(&eDoorState, sizeof(eDoorState), 0);
    apSaveGameBuffer->SaveDataEndian(&bRotateMode, sizeof(bRotateMode), 0);
    apSaveGameBuffer->SaveDataEndian(&bRotateCCW, sizeof(bRotateCCW), 0);
    apSaveGameBuffer->SaveDataEndian(&bMovementBlocked, sizeof(bMovementBlocked), 0);
    apSaveGameBuffer->SaveDataEndian(&bEndUpdatePath, sizeof(bEndUpdatePath), 0);
    apSaveGameBuffer->SaveDataEndian(&bWaitingForUpdate, sizeof(bWaitingForUpdate), 0);
    apSaveGameBuffer->SaveDataEndian(&bDeltaComputedThisFrame, sizeof(bDeltaComputedThisFrame), 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreLookAtPoint, sizeof(bIgnoreLookAtPoint), 0);
    apSaveGameBuffer->SaveDataEndian(&bIgnoreInitialRotation, sizeof(bIgnoreInitialRotation), 0);
    apSaveGameBuffer
        ->SaveDataEndian(&bIgnoreInitialRotationKeepActorHeading, sizeof(bIgnoreInitialRotationKeepActorHeading), 0);
    apSaveGameBuffer->SaveDataEndian(&fIgnoreInitialRotationTetherParam, sizeof(fIgnoreInitialRotationTetherParam), 0);
    apSaveGameBuffer->SaveDataEndian(&LastFrameMinMoveDelta, sizeof(LastFrameMinMoveDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&fLastRotationApplied, sizeof(fLastRotationApplied), 0);
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
    apLoadGameBuffer->LoadData(&MoveDelta, sizeof(MoveDelta));
    apLoadGameBuffer->LoadData(&BlockedTargetPoint, sizeof(BlockedTargetPoint));
    apLoadGameBuffer->LoadData(&TetherTargetPoint, sizeof(TetherTargetPoint));
    apLoadGameBuffer->LoadData(&LastLocation, sizeof(LastLocation));
    apLoadGameBuffer->LoadData(&LastTetherPoint, sizeof(LastTetherPoint));
    apLoadGameBuffer->LoadData(&MoveMode, sizeof(MoveMode));
    apLoadGameBuffer->LoadData(&iCurrentNodeIndex, sizeof(iCurrentNodeIndex));
    apLoadGameBuffer->LoadData(&fRotateGoal, sizeof(fRotateGoal));
    apLoadGameBuffer->LoadData(&fPathLength, sizeof(fPathLength));
    apLoadGameBuffer->LoadData(&fCurrentSplineT, sizeof(fCurrentSplineT));
    apLoadGameBuffer->LoadData(&fActorAvoidanceTimer, sizeof(fActorAvoidanceTimer));
    apLoadGameBuffer->LoadData(&fAvoidanceDetectionValue, sizeof(fAvoidanceDetectionValue));
    apLoadGameBuffer->LoadData(&iUnblockDirection, sizeof(iUnblockDirection));
    apLoadGameBuffer->LoadData(&fAlternateFacingAngle, sizeof(fAlternateFacingAngle));
    apLoadGameBuffer->LoadData(&fBlockedStartAngle, sizeof(fBlockedStartAngle));
    apLoadGameBuffer->LoadData(&fBlockTime, sizeof(fBlockTime));
    apLoadGameBuffer->LoadData(&fTimeInAnim, sizeof(fTimeInAnim));
    apLoadGameBuffer->LoadData(&fRunWalkTime, sizeof(fRunWalkTime));
    apLoadGameBuffer->LoadData(&fMinRotateTime, sizeof(fMinRotateTime));
    apLoadGameBuffer->LoadData(&fWaitTime, sizeof(fWaitTime));
    apLoadGameBuffer->LoadData(&fLastFrameMinMoveDelta, sizeof(fLastFrameMinMoveDelta));
    apLoadGameBuffer->LoadData(&fGoalRadiusSqr, sizeof(fGoalRadiusSqr));
    apLoadGameBuffer->LoadData(&fFallTime, sizeof(fFallTime));
    apLoadGameBuffer->LoadData(&fDistanceTraveled, sizeof(fDistanceTraveled));
    apLoadGameBuffer->LoadData(&fIgnoreLookAtPointTimer, sizeof(fIgnoreLookAtPointTimer));
    if (apLoadGameBuffer->GetVersion() >= 4u) {
        apLoadGameBuffer->LoadData(&fTetherMinMove, sizeof(fTetherMinMove));
        apLoadGameBuffer->LoadData(&fTetherDistFromActor, sizeof(fTetherDistFromActor));
    }
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    apLoadGameBuffer->LoadData(&eBlendRunning, sizeof(eBlendRunning));
    apLoadGameBuffer->LoadData(&eAvoidState, sizeof(eAvoidState));
    apLoadGameBuffer->LoadData(&eDoorState, sizeof(eDoorState));
    apLoadGameBuffer->LoadData(&bRotateMode, sizeof(bRotateMode));
    apLoadGameBuffer->LoadData(&bRotateCCW, sizeof(bRotateCCW));
    apLoadGameBuffer->LoadData(&bMovementBlocked, sizeof(bMovementBlocked));
    apLoadGameBuffer->LoadData(&bEndUpdatePath, sizeof(bEndUpdatePath));
    apLoadGameBuffer->LoadData(&bWaitingForUpdate, sizeof(bWaitingForUpdate));
    apLoadGameBuffer->LoadData(&bDeltaComputedThisFrame, sizeof(bDeltaComputedThisFrame));
    apLoadGameBuffer->LoadData(&bIgnoreLookAtPoint, sizeof(bIgnoreLookAtPoint));
    if (apLoadGameBuffer->GetVersion() >= 12u) {
        apLoadGameBuffer->LoadData(&bIgnoreInitialRotation, sizeof(bIgnoreInitialRotation));
        apLoadGameBuffer->LoadData(
            &bIgnoreInitialRotationKeepActorHeading,
            sizeof(bIgnoreInitialRotationKeepActorHeading)
        );
        apLoadGameBuffer->LoadData(&fIgnoreInitialRotationTetherParam, sizeof(fIgnoreInitialRotationTetherParam));
    }
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        apLoadGameBuffer->LoadData(&LastFrameMinMoveDelta, sizeof(LastFrameMinMoveDelta));
        apLoadGameBuffer->LoadData(&fLastRotationApplied, sizeof(fLastRotationApplied));
    }
    apLoadGameBuffer->LoadFormID(pCurrentDoor);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        pForm = nullptr;
        apLoadGameBuffer->LoadFormID(pForm);
        ActorAvoidList.AddHead(pForm);
    }
}

void DetailedActorPathHandler::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pDoor = nullptr;
    if (pCurrentDoor) {
        TESForm* pDoorForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentDoor))
        );
        pDoor = dynamic_cast<TESObjectREFR*>(pDoorForm);
    }
    pCurrentDoor = pDoor;
    BSSimpleList<Actor*>* plist = &ActorAvoidList;
    BSSimpleList<Actor*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        Actor* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* pActorForm = TESForm::GetFormByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm))
            );
            pForm = dynamic_cast<Actor*>(pActorForm);
            if (pForm) {
                plist->SetItem(pForm);
            }
            else {
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastList) {
                plastList->Remove(plist->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
    if (pCurrentDoor) {
        pCurrentDoor->SetTargeted(true);
    }
}

bool DetailedActorPathHandler::IsDoorInPath(uint32_t aDoorID) {
    NiPoint3 thisPoint;
    // The filters are constructed and never destroyed.
    uint16_t ustri;
    NiPoint3 nextPoint;
    if (spPath.m_pObject) {
        if (spPath->IsDoorInPath(aDoorID)) {
            return true;
        }

        for (float fparam = 0.0f; fparam < static_cast<double>(spPath->GetNodeCount()) - 1.0; fparam = fparam + 0.5) {
            float fnextParam = fparam + 0.5;
            spPath->ComputeLocationFromParameter(fparam, thisPoint);
            spPath->ComputeLocationFromParameter(fnextParam, nextPoint);
            DoorInPathFilter filter(aDoorID);
            PathingLocation start(thisPoint, pActor);
            start.ResolveNavMeshInfo(nullptr);
            NavMeshPtr spmesh;
            if (start.GetNavMeshAndTriangle(spmesh, ustri)) {
                NavMeshTriangle* ptri = spmesh->GetTriangle(ustri);
                if (ptri) {
                    filter.bEntirePathInDoor = ptri->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
                }
            }
            Pathing::CheckLineOfSight(start, nextPoint, static_cast<PathingLocation*>(nullptr), &filter);
            if (filter.bEnterTriWithDoor || filter.bLeaveTriWithDoor || filter.bEntirePathInDoor) {
                return true;
            }
        }
        return false;
    }

    DoorInPathFilter filter(aDoorID);
    PathingLocation actorLoc(pActor);
    actorLoc.ResolveNavMeshInfo(nullptr);
    NavMeshPtr spmesh;
    if (actorLoc.GetNavMeshAndTriangle(spmesh, ustri)) {
        NavMeshTriangle* ptri = spmesh->GetTriangle(ustri);
        if (ptri) {
            filter.bEntirePathInDoor = ptri->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR);
        }
    }
    Pathing::CheckLineOfSight(actorLoc, TetherTargetPoint, static_cast<PathingLocation*>(nullptr), &filter);
    return filter.bEnterTriWithDoor || filter.bLeaveTriWithDoor || filter.bEntirePathInDoor;
}

bool DetailedActorPathHandler::InitRotationMode(float afGoalAngle, bool abForceTurn, bool abPathTransition) {
    float fdir;
    if (bIgnoreInitialRotation) {
        return false;
    }
    if (afGoalAngle < 0.0) {
        float fwrappedAngle = afGoalAngle + 6.2831854820251465;
        afGoalAngle = std::fmod(fwrappedAngle, 6.2831855f);
    }
    else if (afGoalAngle >= 6.2831854820251465) {
        afGoalAngle = std::fmod(afGoalAngle, 6.2831855f);
    }
    float fStartAngle = pActor->GetHeading(false);
    float fangledelta = GetAngleDelta(fStartAngle, afGoalAngle, fdir);
    float fabsoluteDelta = std::fabs(fangledelta);
    float fepsilon = 0.017453292f;
    if (abForceTurn) {
        fepsilon = 0.017453292f;
    }
    else if (abPathTransition && (pActorMover->GetPreferredMoveModeType() & ActorMover::MF_RUNNING) != 0) {
        fepsilon = 0.5235988f;
    }
    if (!(fepsilon < static_cast<double>(fabsoluteDelta))) {
        return false;
    }
    bRotateCCW = fdir > 0.0;
    bRotateMode = true;
    fRotateGoal = afGoalAngle;
    BSWrap(fRotateGoal, -3.1415927f, 3.1415927f);
    if (!pActor->GetTurretBehavior()) {
        fMinRotateTime = fActorTurnAnimMinTime.Float();
    }
    pActorMover->DebugOnEvent();
    return true;
}

// The turn is limited to afDeltaT times 1.3089969158172607 (a double constant, 75 degrees in radians). The smaller turn
// is kept only when the limit is below it.
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
        MoveDelta.y = (pActor->GetCurrentSpeed() * afDeltaT);
    }
}

void DetailedActorPathHandler::ResetStaticAvoidance() {
    bMovementBlocked = false;
    fBlockTime = 0.0f;
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
    if (pActorMover->iNextNode + 1 != static_cast<int32_t>(spPath->GetVirtualNodeCount()) ||
        pActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
        pActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
        return false;
    }
    InitRotationMode(spRequest->GetGoalAngleToFace(), false, false);
    return bRotateMode;
}

void DetailedActorPathHandler::ResetMoveModeAndDelta() {
    MoveMode = pActorMover->GetPreferredMoveModeType();
    MoveDelta = NiPoint3::ZERO;
    fLastFrameMinMoveDelta = 0.0f;
    LastFrameMinMoveDelta = NiPoint3::ZERO;
}

// Collects the actors in the way (BuildAvoidanceList) and weighs each one's detection value into
// fAvoidanceDetectionValue. A moving actor makes a waiting handler stop waiting (1); once the detection value reaches
// fAvoidanceDetectionTime the handler waits (1) when it should wait for one of them, or else repaths around them with
// an avoid node array (2). An actor standing on the path's goal fails a request that may not end on an occupied goal
// (3) and otherwise softens its goal radius. Without anything to avoid the detection value decays. The locals are
// constructed and destroyed explicitly on typed storage that runs no C++ constructor or destructor.
DetailedActorPathHandler::PATH_ACTOR_AVOID_RETURN DetailedActorPathHandler::UpdateActorAvoidance(float afDeltaT) {
    NiAVObject* p3D = pActor->Get3D();
    if (!p3D || !p3D->QFadedIn()) {
        return PATH_ACTOR_AVOID_RETURN_OK;
    }
    if (p3D->GetNotVisible()) {
        // The distance is kept as a double across the setting read.
        double dLODDistance = static_cast<bhkCharacterController*>(pActor->GetCharController())->GetLODDistance();
        if (fLODActorAvoidanceDistance.Float() < dLODDistance) {
            return PATH_ACTOR_AVOID_RETURN_OK;
        }
    }
    // Only the move delta's x and y are read, as a NiPoint2, so the length test ignores z.
    if (eAvoidState != PATH_AVOID_WAITING && reinterpret_cast<NiPoint2*>(&MoveDelta)->SqrLength() < 0.25) {
        return PATH_ACTOR_AVOID_RETURN_OK;
    }
    if (eAvoidState != PATH_AVOID_REPATH_IGNORE && eAvoidState != PATH_AVOID_REPATH) {
        ActorAvoidList.RemoveAll();
    }
    Actor* pFollowLeader = GetFollowLeader(pActor);
    ActorAvoidanceArray kCandidates;
    NiPoint3 kMoveDelta = MoveDelta;
    if (eAvoidState == PATH_AVOID_WAITING) {
        NiPoint3 kForward;
        kMoveDelta = *::new (&kForward) NiPoint3(0.0f, 1.0f, 0.0f);
    }
    BuildAvoidanceList(pActor, spRequest, MoveMode, kMoveDelta, this, kCandidates);
    BSSimpleArray<Actor*> kActors;
    // Whether to wait for an actor rather than repath, and whether the player was added.
    uint8_t bWait = 0;
    uint8_t bPlayerAdded = 0;
    for (uint32_t i = 0; i < kCandidates.GetSize(); ++i) {
        Actor* pOther = kCandidates.GetAt(i).pActor;
        float fDetection = kCandidates.GetAt(i).fDetection;
        if (pOther == pPlayer) {
            if (pPlayer == pFollowLeader) {
                continue;
            }
            if (spPath) {
                PathingLocation kGoal(spPath->GetLastNode()->GetLocation());
                float fRadius = pPlayer->GetRadius();
                bool bClose;
                {
                    PathingLocation kPlayerLocation(pPlayer);
                    bClose = kGoal.IsCloseTo(kPlayerLocation, fRadius);
                }
                if (bClose) {
                    if (spRequest->GetFailIfGoalOccupied()) {
                        return PATH_ACTOR_AVOID_RETURN_FAIL;
                    }
                    spRequest->SetSoftGoalRadius(false);
                }
            }
            fAvoidanceDetectionValue = fDetection * afDeltaT + fAvoidanceDetectionValue;
            Actor* pPlayerActor = pPlayer;
            kActors.Add(pPlayerActor);
            bPlayerAdded = 1;
            continue;
        }
        uint8_t bMoving = 0;
        if (pOther->IsMoving() && !pOther->IsWaitingForAvoidance()) {
            bMoving = 1;
        }
        PathingLocation kOtherGoal;
        if (pOther->GetCurrentPathfindingGoal(&kOtherGoal)) {
            // An actor already heading to this actor's location is not in the way.
            float fRadius = spRequest->GetActorRadius();
            bool bClose;
            {
                PathingLocation kLocation(pActor);
                bClose = kOtherGoal.IsCloseTo(kLocation, fRadius);
            }
            if (bClose) {
                bMoving = 0;
            }
        }
        if (bMoving) {
            if (eAvoidState == PATH_AVOID_WAITING) {
                return PATH_ACTOR_AVOID_RETURN_WAIT;
            }
            fAvoidanceDetectionValue = fDetection * afDeltaT + fAvoidanceDetectionValue;
            if ((MoveMode & 0x200) != 0) {
                if ((pOther->GetMoveMode() & 0x200) != 0) {
                    bWait = 1;
                }
                else {
                    kActors.Add(pOther);
                }
            }
            else if (!pActor->HasFollower(pOther)) {
                bWait = 1;
                if (fAvoidanceDetectionTime.Float() <= fAvoidanceDetectionValue) {
                    break;
                }
            }
        }
        else {
            if (eAvoidState == PATH_AVOID_WAITING) {
                continue;
            }
            fAvoidanceDetectionValue = fDetection * 2.0 * afDeltaT + fAvoidanceDetectionValue;
            if (pFollowLeader && pOther == pFollowLeader) {
                bWait = 1;
            }
            if (spPath) {
                PathingLocation kGoal(spPath->GetLastNode()->GetLocation());
                float fRadius = pOther->GetRadius();
                bool bClose;
                {
                    PathingLocation kOtherLocation(pOther);
                    bClose = kGoal.IsCloseTo(kOtherLocation, fRadius);
                }
                if (bClose) {
                    if (spRequest->GetFailIfGoalOccupied()) {
                        return PATH_ACTOR_AVOID_RETURN_FAIL;
                    }
                    spRequest->SetSoftGoalRadius(false);
                }
            }
            kActors.Add(pOther);
        }
    }
    if (eAvoidState == PATH_AVOID_WAITING) {
        return PATH_ACTOR_AVOID_RETURN_OK;
    }
    if (fAvoidanceDetectionValue == 0.0) {
        return PATH_ACTOR_AVOID_RETURN_OK;
    }
    if (fAvoidanceDetectionTime.Float() <= fAvoidanceDetectionValue) {
        if (bWait) {
            fWaitTime = fAvoidanceDefaultWaitTime.Float();
            fActorAvoidanceTimer = fAvoidanceMinWaitTime.Float();
            pActorMover->DebugOnEvent();
            return PATH_ACTOR_AVOID_RETURN_WAIT;
        }
        if (!kActors.IsEmpty()) {
            if (eAvoidState == PATH_AVOID_REPATH_IGNORE || bPlayerAdded || (MoveMode & 0x200) != 0) {
                float fIgnoreMinTime = fAvoidanceIgnoreMinTime.Float();
                fActorAvoidanceTimer = BSMAX(fActorAvoidanceTimer, fIgnoreMinTime);
            }
            else {
                fActorAvoidanceTimer = fAvoidanceIgnoreTime.Float();
            }
            PathingAvoidNodeArray* pAvoidNodes = new PathingAvoidNodeArray;
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
                if (fDistance < pAvoidActor->GetRadius() + fActorRadius) {
                    fNearScale = 1.5f;
                }
                float fAvoidRadius;
                PathingAvoidNode::AVOID_NODE_TYPE eType;
                if (pAvoidActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    pAvoidActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    fAvoidRadius = static_cast<float>(pAvoidActor->GetRadius() * 0.5 * fRadiusScale);
                    eType = PathingAvoidNode::AVOID_NODE_SPHERE;
                }
                else {
                    fAvoidRadius = pAvoidActor->GetRadius() * fRadiusScale * fNearScale;
                    eType = PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR;
                }
                // A moving actor also gets a node where its move takes it within a second.
                NiMatrix3 kRotation;
                kRotation.MakeZRotationQ(pAvoidActor->GetHeading(false));
                NiPoint3 kMove = kRotation * *pAvoidActor->GetActorMoverOverrideMovement();
                NiPoint3 kVelocity = kMove / appTimer.GetSeconds();
                double dSpeedSqr = kVelocity.SqrLength();
                if (pAvoidActor->GetRadius() < dSpeedSqr) {
                    NiPoint3 kAhead = pAvoidActor->GetLocationOnReference() + kVelocity;
                    pAvoidNodes->Add(PathingAvoidNode(kAhead, fAvoidRadius, 2.0f, eType));
                }
                // The node sits at the character proxy's position when the actor collides with characters.
                NiPoint3 kPosition = pAvoidActor->GetLocationOnReference();
                bhkCharacterController* pController = static_cast<bhkCharacterController*>(
                    pAvoidActor->GetCharController()
                );
                if (pController &&
                    !pController->bhkCharacterListener::GetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS)) {
                    alignas(16) hkVector4 kProxyPosition = pController->GethkPosition();
                    NiPoint3 kNiPosition;
                    HK2NI(kNiPosition, kProxyPosition);
                    kPosition.x = kNiPosition.x;
                    kPosition.y = kNiPosition.y;
                }
                pAvoidNodes->Add(PathingAvoidNode(kPosition, fAvoidRadius, fAvoidanceAvoidNodeCost.Float(), eType));
            }
            {
                PathingLocation kOrigin(pActor);
                spRequest->SetOrigin(kOrigin);
            }
            spRequest->SetAvoidNodeArray(pAvoidNodes);
            pActorMover->DebugOnEvent();
            return PATH_ACTOR_AVOID_RETURN_REPATH;
        }
    }
    if (!bWait && kActors.IsEmpty()) {
        fAvoidanceDetectionValue = fAvoidanceDetectionValue - afDeltaT;
        if (fAvoidanceDetectionValue < 0.0) {
            fAvoidanceDetectionValue = 0.0f;
        }
    }
    return PATH_ACTOR_AVOID_RETURN_OK;
}

bool DetailedActorPathHandler::ComputeMinTetherDistance() {
    fTetherMinMove = 10.0f;
    fTetherDistFromActor = 20.0f;
    fTetherDistFromActor = spRequest->GetActorRadius();
    fTetherMinMove = static_cast<float>(fTetherDistFromActor * 0.5);
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
                        }
                        else {
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
        case PATH_HANDLER_DOOR_STATE_FAILED: return PATH_HANDLER_DOOR_STATE_FAILED;
        default: eDoorState = PATH_HANDLER_DOOR_STATE_NONE; return eDoorState;
    }
}

float DetailedActorPathHandler::UpdateRotation(float afDeltaT) {
    float factangle = pActor->GetHeading(false);
    if (pActor->CanMove()) {
        BSWrap(factangle, -3.1415927f, 3.1415927f);
        float fangleDelta = fRotateGoal - factangle;
        BSWrap(fangleDelta, -3.1415927f, 3.1415927f);
        if (std::fabs(fangleDelta) > 0.01745329238474369) {
            float frotateMax = pActor->GetTurningSpeed() * 0.01745329238474369 * afDeltaT;
            if (pActor->IsCreature()) {
                float fCreatureTurnSpeedScale;
                if (pActor->IsInCombat()) {
                    fCreatureTurnSpeedScale = gs_fCreatureCombatTurnSpeedScale_Pathfinding.Float();
                }
                else {
                    fCreatureTurnSpeedScale = gs_fCreatureTurnSpeedScale_Pathfinding.Float();
                }
                frotateMax = frotateMax * fCreatureTurnSpeedScale;
            }
            else {
                float fTurnSpeedScale;
                if (pActor->IsInCombat()) {
                    fTurnSpeedScale = gs_fAICombatTurnSpeedScale_Pathfinding.Float();
                }
                else {
                    fTurnSpeedScale = gs_fAITurnSpeedScale_Pathfinding.Float();
                }
                frotateMax = frotateMax * fTurnSpeedScale;
            }
            if (static_cast<double>(frotateMax) > std::fabs(fangleDelta)) {
                factangle = fRotateGoal;
                if (fMinRotateTime <= 0.0) {
                    bRotateMode = false;
                    pActorMover->DebugOnEvent();
                }
            }
            else if (bRotateCCW) {
                MoveMode = pActorMover->GetPreferredMoveModeType() | 32;
                factangle = factangle + frotateMax;
            }
            else {
                MoveMode = pActorMover->GetPreferredMoveModeType() | 16;
                factangle = factangle - frotateMax;
            }
        }
        else {
            factangle = fRotateGoal;
            if (fMinRotateTime <= 0.0) {
                bRotateMode = false;
                pActorMover->DebugOnEvent();
            }
        }
        fMinRotateTime = fMinRotateTime - afDeltaT;
        return factangle;
    }

    MoveMode = pActorMover->GetPreferredMoveModeType();
    return factangle;
}

bool DetailedActorPathHandler::ComputeMoveMode(float afDeltaT) {
    NiPoint3 kLookDir;
    if (pActor->IsRestrained() || pActor->IsUnConscious()) {
        MoveMode = 0;
        return true;
    }
    uint32_t uiOldMoveMode = MoveMode;
    if (bIgnoreLookAtPoint) {
        fIgnoreLookAtPointTimer = fIgnoreLookAtPointTimer - afDeltaT;
        if (fIgnoreLookAtPointTimer < 0.0) {
            bIgnoreLookAtPoint = false;
            fTimeInAnim = 1.0;
        }
    }
    uint32_t uiPreferredMoveMode = pActorMover->GetPreferredMoveModeType();
    uint32_t uiMoveDirection = MoveMode & 63;
    if (pActor->IsSwimming() && pActor->IsCreature()) {
        MoveMode = uiPreferredMoveMode | 1;
        return true;
    }
    if (!bIgnoreLookAtPoint && (pActorMover->HasLookAtTarget() || bIgnoreInitialRotation)) {
        float fAngle;
        NiPoint3 kAngles;
        float fLookAngle;
        if (pActorMover->HasLookAtTarget()) {
            pActorMover->GetLookAtPoint(kLookDir);
            kLookDir -= pActor->GetLocationOnReference();
            fLookAngle = GetZAngleFromVector(kLookDir);
        }
        else if (bIgnoreInitialRotationKeepActorHeading || !spPath) {
            kLookDir = NiPoint3::ZERO;
            fLookAngle = pActor->GetHeading(false);
        }
        else {
            spPath->ComputeLocationFromParameter(fIgnoreInitialRotationTetherParam, kLookDir);
            kLookDir -= pActor->GetLocationOnReference();
            fLookAngle = GetZAngleFromVector(kLookDir);
        }
        NiPoint3 kTetherPoint = TetherTargetPoint;
        NiPoint3 kActorLocation = pActor->GetLocationOnReference();
        kAngles = CombatUtilities::GetAngleToProjectedPoint(kActorLocation, kTetherPoint);
        if (pActor->IsSwimming() && std::fabs(std::fabs(kAngles.x) - 1.5707963267948966) < 0.2617993950843811) {
            fAngle = fLookAngle;
        }
        else {
            fAngle = kAngles.z;
        }
        float fOutDir = 1.0;
        float fAngleDelta = GetAngleDelta(ClampAngle(fAngle), fLookAngle, fOutDir);
        if (fTimeInAnim > 0.25) {
            if (std::fabs(fAngleDelta) > 0.01745329238474369) {
                float fAngleDegrees = fAngleDelta * 57.2957763671875;
                if (!(std::fabs(fAngleDegrees) > 135.0) ||
                    !pActor->GetAnimation()->GroupLoaded(4u) ||
                    (!(std::fabs(std::fabs(kAngles.x) - 1.5707963267948966) >= 0.2617993950843811) &&
                     pActor->IsSwimming())) {
                    if (!(std::fabs(fAngleDegrees) > 45.0)) {
                        uiMoveDirection = 1;
                    }
                    else if (fOutDir > 0.0) {
                        if (!pActor->IsCreature() || pActor->GetAnimation()->GroupLoaded(5u)) {
                            uiMoveDirection = 4;
                        }
                        else {
                            uiMoveDirection = 1;
                            bIgnoreLookAtPoint = true;
                            fIgnoreLookAtPointTimer = 1.0;
                        }
                    }
                    else if (!pActor->IsCreature() || pActor->GetAnimation()->GroupLoaded(6u)) {
                        uiMoveDirection = 8;
                    }
                    else {
                        uiMoveDirection = 1;
                        bIgnoreLookAtPoint = true;
                        fIgnoreLookAtPointTimer = 1.0;
                    }
                }
                else {
                    uiMoveDirection = 2;
                }
            }
            else {
                uiMoveDirection = pActorMover->GetPreferredMoveMode() & 63;
            }
        }
        else {
            fTimeInAnim = fTimeInAnim + afDeltaT;
        }
    }
    else {
        uiMoveDirection = pActorMover->GetPreferredMoveMode() & 63;
    }
    switch (eBlendRunning) {
        case DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE:
            if ((uiPreferredMoveMode & 512) != 0 &&
                ((uiOldMoveMode & 512) == 0 || ((uiOldMoveMode & 48) != 0 && (uiMoveDirection & 48) == 0))) {
                eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_WALK_TO_RUN;
                fRunWalkTime = pActor->GetAnimActionTime(ANIM_GROUP_MOVE_FORWARD) * 0.5;
            }
            else {
                break;
            }
            [[fallthrough]];
        case DetailedActorPathHandler::PATH_RUNNING_BLEND_WALK_TO_RUN:
            uiPreferredMoveMode &= ~512u;
            uiPreferredMoveMode |= 256u;
            fRunWalkTime = fRunWalkTime - afDeltaT;
            if (fRunWalkTime < 0.0) {
                eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_FINISH_BLEND;
                fRunWalkTime = 0.0;
            }
            break;
        case DetailedActorPathHandler::PATH_RUNNING_BLEND_FINISH_BLEND:
            eBlendRunning = DetailedActorPathHandler::PATH_RUNNING_BLEND_DONE;
            break;
        default: break;
    }
    MoveMode = uiPreferredMoveMode | uiMoveDirection;
    return true;
}

void DetailedActorPathHandler::RotateMoveDelta(float afDeltaT) {
    if (pActor->IsSwimming()) {
        NiMatrix3 kRotation;
        float fPitch;
        NiPoint3 kTetherDelta = TetherTargetPoint;
        float fWaterHeight = pActor->GetRelevantWaterHeight();
        if (fWaterHeight != -3.4028234663852886e+38 && fWaterHeight < static_cast<double>(TetherTargetPoint.z)) {
            TetherTargetPoint.z = pActor->GetRelevantWaterHeight() - 5.0;
        }
        bhkCharacterController* pController = pActor->GetCharController();
        NiPoint3 pt = *pActor->QLocationOnReference();
        if (pController) {
            double dHalfHeight = pActor->GetHeight() * 0.5;
            pt.z = pController->QSwimFloatHeight() * dHalfHeight + pt.z;
            kTetherDelta -= pt;
        }
        else {
            kTetherDelta -= pActor->GetLocationOnReference();
        }
        double dHeight = pActor->GetHeight();
        if (TetherTargetPoint.z >= pt.z - dHeight && pt.z > static_cast<double>(TetherTargetPoint.z)) {
            kTetherDelta.z = 0.0;
        }
        kTetherDelta.Unitize();
        BSClamp(kTetherDelta.z, -1.0f, 1.0);
        if (kTetherDelta.z >= 0.0) {
            fPitch = std::acosf(kTetherDelta.z);
            fPitch = 1.5707963267948966 - fPitch;
        }
        else {
            float fNegZ = -kTetherDelta.z;
            fPitch = std::acosf(fNegZ);
            fPitch = fPitch - 1.5707963267948966;
        }
        if (MoveDelta.y < 0.0 && (MoveMode & 2) != 0) {
            float fNegPitch = -fPitch;
            kRotation.MakeXRotation(fNegPitch);
        }
        else {
            kRotation.MakeXRotation(fPitch);
        }
        MoveDelta = MoveDelta * kRotation;
    }
    else {
        float fDirectionAngle;
        NiMatrix3 kRotation;
        NiPoint3 kTargetDelta = bMovementBlocked ? BlockedTargetPoint : TetherTargetPoint;
        kTargetDelta -= pActor->GetLocationOnReference();
        kTargetDelta.z = 0.0;
        float fTargetAngle = GetZAngleFromVector(kTargetDelta);
        switch (GetMoveMode() & 15) {
            case 0u:
            case 1u: {
                fDirectionAngle = 0.0;
                break;
            }
            case 2u: {
                fDirectionAngle = 3.1415927f;
                break;
            }
            case 4u: {
                fDirectionAngle = -1.5707964f;
                break;
            }
            case 8u: {
                fDirectionAngle = 1.5707964f;
                break;
            }
            default: {
                fDirectionAngle = 0.0;
                break;
            }
        }
        double dHeading = pActor->GetHeading(false);
        float fFacing = dHeading + fDirectionAngle;
        float fRotation = fTargetAngle - fFacing;
        kRotation.MakeZRotationQ(fRotation);
        MoveDelta = kRotation * MoveDelta;
    }
}

void DetailedActorPathHandler::UpdateTetherPosition(float afDist) {
    NiPoint3 kNewTether;
    float fNewSplineT;
    if (spPath->GetNodeCount()) {
        uint32_t uiNextNodeIndex = iCurrentNodeIndex + 1;
        if (uiNextNodeIndex < spPath->GetNodeCount()) {
            fDistanceTraveled = spPath->ComputeDistanceToParameter(fCurrentSplineT);
            LastTetherPoint = TetherTargetPoint;
            ResetStaticAvoidance();
            if (afDist >= 0.0) {
                spPath->FindPointOnPath(fCurrentSplineT, afDist, true, &fNewSplineT, &kNewTether);
            }
            else {
                float fBackDist = -afDist;
                spPath->FindPointOnPath(fCurrentSplineT, fBackDist, false, &fNewSplineT, &kNewTether);
            }
            fCurrentSplineT = fNewSplineT;
            TetherTargetPoint = kNewTether;
            iCurrentNodeIndex = static_cast<int32_t>(fCurrentSplineT);
            pActorMover->DebugOnEvent();
        }
    }
}

void DetailedActorPathHandler::UpdateTether(float afDeltaT) {
    NiPoint3 kLoopDelta;
    NiPoint3 kFirstDelta;
    if (spPath->GetNodeCount()) {
        uint32_t uiNextNodeIndex = iCurrentNodeIndex + 1;
        if (spPath->GetNodeCount() && uiNextNodeIndex >= spPath->GetNodeCount()) {
            TetherTargetPoint = spPath->GetLastNode()->Location.GetLocation();
        }
        else if (
            ((MoveMode & ActorMover::MF_FORWARD) == 0 || !(MoveDelta.y < 0.0)) &&
            ((MoveMode & ActorMover::MF_BACKWARD) == 0 || !(MoveDelta.y > 0.0)) &&
            ((MoveMode & ActorMover::MF_LEFT) == 0 || !(MoveDelta.x > 0.0)) &&
            ((MoveMode & ActorMover::MF_RIGHT) == 0 || !(MoveDelta.x < 0.0))
        ) {
            NiPoint3 nextTetherDelta;
            NiMatrix3 rot;
            rot.MakeZRotationQ(pActor->GetHeading(false));
            NiPoint3 worldSpaceMoveDir = rot * MoveDelta;
            NiPoint3 tetherDelta = TetherTargetPoint - pActor->GetLocationOnReference();
            float fnextSplineT = fCurrentSplineT + 0.10000000149011612;
            spPath->ComputeLocationFromParameter(fnextSplineT, nextTetherDelta);
            nextTetherDelta -= TetherTargetPoint;
            tetherDelta.z = 0.0;
            float fdot = worldSpaceMoveDir.Dot(tetherDelta);
            float ftetherDot = nextTetherDelta.Dot(tetherDelta);
            float fsqrDist = tetherDelta.SqrXYLength();
            float fsqrTetherMinMoveThisFrame = MoveDelta.SqrLength() * 4.0;
            float fsqrTetherMinMove = fTetherMinMove * fTetherMinMove;
            fsqrTetherMinMoveThisFrame = BSMAX(fsqrTetherMinMoveThisFrame, fsqrTetherMinMove);
            if (fdot < 0.0 || ftetherDot < 0.0 || fsqrTetherMinMoveThisFrame > static_cast<double>(fsqrDist)) {
                fDistanceTraveled = spPath->ComputeDistanceToParameter(fCurrentSplineT);
                LastTetherPoint = TetherTargetPoint;
                if (!(fCurrentSplineT >= static_cast<double>(spPath->GetNodeCount() - 1))) {
                    float flastDot = fdot;
                    fCurrentSplineT = fCurrentSplineT + 0.10000000149011612;
                    spPath->ComputeLocationFromParameter(fCurrentSplineT, TetherTargetPoint);
                    fnextSplineT = fCurrentSplineT + 0.10000000149011612;
                    spPath->ComputeLocationFromParameter(fnextSplineT, nextTetherDelta);
                    nextTetherDelta -= TetherTargetPoint;
                    ::new (&kFirstDelta) NiPoint3(TetherTargetPoint - pActor->GetLocationOnReference());
                    tetherDelta.x = kFirstDelta.x;
                    tetherDelta.y = kFirstDelta.y;
                    tetherDelta.z = 0.0f;
                    fdot = worldSpaceMoveDir.Dot(tetherDelta);
                    ftetherDot = nextTetherDelta.Dot(tetherDelta);
                    fsqrDist = tetherDelta.SqrXYLength();
                    for (;;) {
                        if (fCurrentSplineT >= static_cast<double>(spPath->GetNodeCount() - 1)) {
                            fCurrentSplineT = static_cast<float>(spPath->GetNodeCount() - 1);
                            break;
                        }
                        if (!(fdot < 0.0) && !(ftetherDot < 0.0)) {
                            if (fsqrTetherMinMoveThisFrame <= static_cast<double>(fsqrDist)) {
                                break;
                            }
                        }
                        else if (flastDot > static_cast<double>(fdot)) {
                            break;
                        }
                        flastDot = fdot;
                        fCurrentSplineT = fCurrentSplineT + 0.10000000149011612;
                        spPath->ComputeLocationFromParameter(fCurrentSplineT, TetherTargetPoint);
                        fnextSplineT = fCurrentSplineT + 0.10000000149011612;
                        spPath->ComputeLocationFromParameter(fnextSplineT, nextTetherDelta);
                        nextTetherDelta -= TetherTargetPoint;
                        ::new (&kLoopDelta) NiPoint3(TetherTargetPoint - pActor->GetLocationOnReference());
                        tetherDelta.x = kLoopDelta.x;
                        tetherDelta.y = kLoopDelta.y;
                        tetherDelta.z = 0.0f;
                        fdot = worldSpaceMoveDir.Dot(tetherDelta);
                        ftetherDot = nextTetherDelta.Dot(tetherDelta);
                        fsqrDist = tetherDelta.SqrXYLength();
                    }
                    iCurrentNodeIndex = static_cast<int32_t>(fCurrentSplineT);
                    pActorMover->DebugOnEvent();
                }
                else {
                    fCurrentSplineT = static_cast<float>(spPath->GetNodeCount() - 1);
                    iCurrentNodeIndex = static_cast<int32_t>(fCurrentSplineT);
                    TetherTargetPoint = spPath->GetLastNode()->Location.GetLocation();
                }
            }
        }
    }
}

void DetailedActorPathHandler::ComputeHeadingAndLooking(float& arfHeading, float& arfLooking) {
    float fangle = 0.0f;
    float flookingAngle = 0.0f;
    if (bMovementBlocked) {
        fangle = fAlternateFacingAngle;
    }
    else if (bIgnoreInitialRotation) {
        if (bIgnoreInitialRotationKeepActorHeading || !spPath.m_pObject) {
            fangle = pActor->GetHeading(false);
        }
        else {
            NiPoint3 targetPoint;
            spPath->ComputeLocationFromParameter(fIgnoreInitialRotationTetherParam, targetPoint);
            NiPoint3 delta = targetPoint - pActor->GetLocationOnReference();
            delta.z = 0.0f;
            fangle = GetZAngleFromVector(delta);
        }
    }
    else if (bIgnoreLookAtPoint || !pActorMover->HasLookAtTarget()) {
        NiPoint3 lookatPoint;
        if (spPath.m_pObject && spPath->GetNodeCount()) {
            float fnextSplineT = fCurrentSplineT + 0.10000000149011612;
            spPath->ComputeLocationFromParameter(fnextSplineT, lookatPoint);
        }
        else {
            lookatPoint = TetherTargetPoint;
        }
        NiPoint3 deltaToTargetPoint = lookatPoint - pActor->GetLocationOnReference();
        if (pActor->IsSwimming() &&
            std::fabs(deltaToTargetPoint.z) >= deltaToTargetPoint.Length() * 0.949999988079071) {
            fangle = pActor->GetHeading(false);
        }
        else {
            fangle = GetZAngleFromVector(deltaToTargetPoint);
        }
        if (pActor->IsSwimming()) {
            flookingAngle = 0.0f;
        }
    }
    else {
        NiPoint3 lookatPoint;
        pActorMover->GetLookAtPoint(lookatPoint);
        NiPoint3 firingLocation = CombatUtilities::GetAttackerFiringLocation(pActor);
        // The squared length is kept in double precision for the compare below.
        double dDistSqr = (firingLocation - lookatPoint).SqrXYLength();
        double dRadius = pActor->GetRadius();
        if (pActor->GetRadius() * dRadius * 0.25 > dDistSqr) {
            fangle = pActor->GetHeading(false);
        }
        else {
            NiPoint3 rot = CombatUtilities::GetAngleToProjectedPoint(pActor, lookatPoint);
            if (!pActor->IsSwimming()) {
                flookingAngle = rot.x;
            }
            if (pActor->GetRadius() >= 100.0) {
                NiPoint3 delta = lookatPoint - pActor->GetLocationOnReference();
                delta.z = 0.0f;
                fangle = GetZAngleFromVector(delta);
            }
            else {
                fangle = rot.z;
            }
        }
    }
    arfHeading = fangle;
    arfLooking = flookingAngle;
}

float DetailedActorPathHandler::UpdateHeading(float fdeltaT, float& afSlowDown) {
    NiPoint3 kGoalLocation;
    float fnewT;
    NiPoint3 furtherTetherLoc;
    float fratio;
    float fangledelta;
    float fangle = 0.0f;
    float flookingAngle = 0.0f;
    ComputeHeadingAndLooking(fangle, flookingAngle);
    float factangle = pActor->GetHeading(false);
    BSWrap(factangle, -3.1415927f, 3.1415927f);
    BSWrap(fangle, -3.1415927f, 3.1415927f);
    fangledelta = fangle - factangle;
    BSWrap(fangledelta, -3.1415927f, 3.1415927f);
    if (std::fabs(fangledelta) >= 0.01745329238474369) {
        fratio = 1.0f;
        if (spPath.m_pObject && pActor->GetTurningSpeed() > 45.0) {
            float fspeed;
            float fdistToTether = (TetherTargetPoint - pActor->GetLocationOnReference()).Length();
            if ((MoveMode & ActorMover::MF_RUNNING) != 0) {
                fspeed = pActor->GetRunSpeed();
            }
            else {
                fspeed = pActor->GetWalkSpeed();
            }
            furtherTetherLoc = TetherTargetPoint;
            if (fspeed > static_cast<double>(fdistToTether)) {
                float fextraDist = fspeed - fdistToTether;
                spPath->FindPointOnPath(fCurrentSplineT, fextraDist, true, &fnewT, &furtherTetherLoc);
            }
            float factorAngle = pActor->GetHeading(false);
            NiPoint3 deltaToFurtherTether = furtherTetherLoc - pActor->GetLocationOnReference();
            float ffurtherTetherAngle = GetZAngleFromVector(deltaToFurtherTether);
            float fangleDiff = ffurtherTetherAngle - factorAngle;
            BSWrap(fangleDiff, -3.1415927f, 3.1415927f);
            fratio = std::fabs(fangleDiff) / 1.5707963267948966;
            BSClamp(fratio, 0.025f, 1.0f);
        }

        double dTurningSpeed = pActor->GetTurningSpeed();
        float frotate = dTurningSpeed * 0.01745329238474369 * fdeltaT * 3.0 * fratio;
        double dAbsDelta = std::fabs(fangledelta);
        if (frotate < dAbsDelta) {
            if (std::fabs(fangledelta) > 2.700000047683716) {
                afSlowDown = 0.05f;
            }
            else if (std::fabs(fangledelta) > 2.0) {
                afSlowDown = 0.1f;
            }
            else if (std::fabs(fangledelta) > 1.600000023841858) {
                afSlowDown = 0.2f;
            }
            else if (spPath.m_pObject && spRequest.m_pObject) {
                PathingNode* pLastNode = spPath->GetLastNode();
                ::new (&kGoalLocation) NiPoint3(pLastNode->Location.GetLocation());
                // The squared length is kept at double precision.
                double dDistSqr = (pActor->GetLocationOnReference() - kGoalLocation).SqrXYLength();
                double dRadius = pActor->GetRadius();
                if (pActor->GetRadius() * dRadius > dDistSqr) {
                    afSlowDown = 0.5f;
                }
            }
        }
        BSClamp(fangledelta, -frotate, frotate);
        fangle = factangle + fangledelta;
    }
    fLastRotationApplied = std::fabs(fangledelta);
    UpdateLooking(fdeltaT, flookingAngle);
    return fangle;
}

bool DetailedActorPathHandler::UpdateStaticAvoidance(float afDeltaT) {
    bool breturnBlocked = false;
    if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH ||
        eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE) {
        return true;
    }
    NiPoint3 currentLoc = pActor->GetLocationOnReference();
    NiPoint3 lastFrameDelta = currentLoc - LastLocation;
    bool bblockedThisFrame = false;
    bool bblockedbyActor = false;
    if (pActor->IsSwimming()) {
        double dLastFrameDistSqr = lastFrameDelta.Dot(lastFrameDelta);
        if (LastFrameMinMoveDelta.Dot(LastFrameMinMoveDelta) * 0.125 > dLastFrameDistSqr) {
            bhkCharacterController* pcon = pActor->GetCharController();
            if (pcon) {
                bhkCharacterController* pcc = pcon->GetBumpedCharacterController();
                NiAVObject* pav = pcc ? pcc->GetNiObject() : nullptr;
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                TESObjectREFR* pact = pref && pref->IsActor() ? pref : nullptr;
                if (pact) {
                    bblockedbyActor = true;
                }
            }
            bblockedThisFrame = true;
        }
    }
    else {
        if (fLastFrameMinMoveDelta * fLastFrameMinMoveDelta * 0.125 > lastFrameDelta.Dot(lastFrameDelta)) {
            bhkCharacterController* pcon = pActor->GetCharController();
            if (pcon) {
                bhkCharacterController* pcc = pcon->GetBumpedCharacterController();
                NiAVObject* pav = pcc ? pcc->GetNiObject() : nullptr;
                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                TESObjectREFR* pact = pref && pref->IsActor() ? pref : nullptr;
                if (pact) {
                    bblockedbyActor = true;
                }
            }
            bblockedThisFrame = true;
        }
    }
    LastLocation = currentLoc;
    if (bblockedThisFrame) {
        double dBlockTime;
        if (bblockedbyActor) {
            dBlockTime = afDeltaT * 0.20000000298023224 + fBlockTime;
        }
        else {
            dBlockTime = fBlockTime + afDeltaT;
        }
        fBlockTime = dBlockTime;
        if (eState == DetailedActorPathHandler::DAP_MOVING_WHILE_WAITING) {
            eState = DetailedActorPathHandler::DAP_WAITING_FOR_UPDATED_PATH;
            return false;
        }
    }
    else if (!bMovementBlocked) {
        fBlockTime = 0.0f;
        return true;
    }
    if (fBlockTime > 1.5 && pActor->GetAnimation()) {
        if (pActor->GetAnimation()->ShouldBeMoving()) {
            bMovementBlocked = true;
        }
    }

    if (bMovementBlocked) {
        breturnBlocked = true;
        // Tests the three states for equality, not a range.
        if (eAvoidState == DetailedActorPathHandler::PATH_AVOID_INVALID ||
            eAvoidState == DetailedActorPathHandler::PATH_AVOID_OK ||
            eAvoidState == DetailedActorPathHandler::PATH_AVOID_FAILED) {
            TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
            PathingLocation kTetherLocation(&TetherTargetPoint, pActor->GetParentCell(), pWorldSpace);
            if (kTetherLocation.ResolveNavMeshInfo(nullptr)) {
                uint16_t ustri;
                NavMeshPtr spmesh;
                if (kTetherLocation.GetNavMeshAndTriangle(spmesh, ustri)) {
                    using AvoidNodeArray = BSSimpleArray<NavMeshStaticAvoidNode*, 1024>;
                    AvoidNodeArray kAvoidNodes;
                    spmesh->GetStaticAvoidNodesForTri(ustri, kAvoidNodes);
                    char cAddNode = 1;
                    if (!kAvoidNodes.IsEmpty()) {
                        for (uint32_t uiIndex = 0; uiIndex < kAvoidNodes.iSize; ++uiIndex) {
                            NavMeshStaticAvoidNode* pnode = kAvoidNodes.GetAt(static_cast<int32_t>(uiIndex));
                            double dDistSqr = (pnode->Point1 - TetherTargetPoint).SqrLength();
                            double dRadius = pnode->fRadius;
                            if (pnode->fRadius * dRadius > dDistSqr) {
                                if (pnode->fRadius < 128.0) {
                                    double dNodeRadius = pnode->fRadius;
                                    float fNewRadius = dNodeRadius + dNodeRadius;
                                    pnode->SetRadius(fNewRadius);
                                }
                                else {
                                    float fNewCost = pnode->fCost * 10.0;
                                    pnode->SetCost(fNewCost);
                                }
                                cAddNode = 0;
                            }
                        }
                    }
                    if (cAddNode) {
                        float fActorRadius = spRequest->fActorRadius;
                        spmesh->AddStaticAvoidNode(TetherTargetPoint, ustri, fActorRadius, 1.0f);
                    }
                }
            }
        }
    }
    if (breturnBlocked) {
        pActorMover->DebugOnEvent();
    }
    return !breturnBlocked;
}

void DetailedActorPathHandler::FindPathStartingLocation(float afPercentTraveled) {
    NiPoint3 kOffset;
    fPathLength = spPath->ComputeDetailedPathLength();
    fCurrentSplineT = spPath->GetSplineParametersFromPercentage(afPercentTraveled, fPathLength, iCurrentNodeIndex);
    if (bWaitingForUpdate) {
        bWaitingForUpdate = false;
        if (fPathLength == 0.0) {
            iCurrentNodeIndex = spPath->GetNodeCount() - 1;
            fCurrentSplineT = static_cast<float>(iCurrentNodeIndex);
            TetherTargetPoint = spPath->GetLastNode()->Location.GetLocation();
            LastTetherPoint = pActor->GetLocationOnReference();
            bWaitingForUpdate = false;
        }
        else {
            NiPoint3 oldDirVec = TetherTargetPoint - LastTetherPoint;
            oldDirVec.z = 0.0;
            NiPoint3 actorLoc = pActor->GetLocationOnReference();
            NiPoint3 actorDirVec = TetherTargetPoint - actorLoc;
            float factDotCheck = actorDirVec.Dot(oldDirVec);
            bool bpastGoal = factDotCheck < 0.0;
            float fcheck5 = 5.0 / fPathLength;
            float fcheck20 = fcheck5 * 4.0;
            float fbestSqrTetherDist = std::numeric_limits<float>::max();
            float fbestSplineT = 0.0f;
            uint32_t ibestNode = 0;
            bool botherSide = false;
            for (int32_t i = 0; i < 4; ++i) {
                NiPoint3 actorToTetherDir;
                NiPoint3 newTetherLoc;
                uint32_t itempNode;
                float ftempPercent = afPercentTraveled + fcheck5 + static_cast<double>(i) * fcheck20;
                float ftempSplineT = spPath->GetSplineParametersFromPercentage(ftempPercent, fPathLength, itempNode);
                spPath->ComputeLocationFromParameter(ftempSplineT, newTetherLoc);
                ::new (&kOffset) NiPoint3(newTetherLoc - actorLoc);
                actorToTetherDir.x = kOffset.x;
                actorToTetherDir.y = kOffset.y;
                actorToTetherDir.z = 0.0f;
                float fdot = actorToTetherDir.Dot(oldDirVec);
                float fsqrDist = actorToTetherDir.SqrXYLength();
                if ((bpastGoal || !(fdot < 0.0)) && (!bpastGoal || !(fdot > 0.0))) {
                    if (fbestSqrTetherDist > static_cast<double>(fsqrDist)) {
                        ibestNode = itempNode;
                        fbestSplineT = ftempSplineT;
                        fbestSqrTetherDist = fsqrDist;
                    }
                }
                else if (botherSide) {
                    if (fbestSqrTetherDist > static_cast<double>(fsqrDist)) {
                        ibestNode = itempNode;
                        fbestSplineT = ftempSplineT;
                        fbestSqrTetherDist = fsqrDist;
                    }
                }
                else {
                    botherSide = true;
                    ibestNode = itempNode;
                    fbestSplineT = ftempSplineT;
                    fbestSqrTetherDist = fsqrDist;
                }
            }
            spPath->ComputeLocationFromParameter(fbestSplineT, TetherTargetPoint);
            iCurrentNodeIndex = ibestNode;
            fCurrentSplineT = fbestSplineT;
            LastTetherPoint = actorLoc;
        }
    }
    else {
        spPath->ComputeLocationFromParameter(fCurrentSplineT, TetherTargetPoint);
        LastTetherPoint = TetherTargetPoint;
        if (afPercentTraveled > 0.0) {
            PathingLocation tetherNavMeshLoc(TetherTargetPoint, pActor);
            NiPoint3 snapLocation = TetherTargetPoint;
            Pathing::GetNavMeshZForLocation(tetherNavMeshLoc, snapLocation.z);
            float fheading = pActor->GetHeading(false);
            TESWorldSpace* pWorldSpace = pActor->GetWorldSpace();
            pActor->MoveActorAndFollowers(snapLocation, fheading, pActor->GetParentCell(), pWorldSpace, true, true);
        }
        UpdateTetherPosition(0.0f);
    }
}

ActorPathHandler::PATH_HANDLER_RESULT DetailedActorPathHandler::CheckGoalReached() {
    int32_t iLastLoadedIndex;
    NiPoint3 goal;
    if (bEndUpdatePath) {
        uint32_t uiEndNodeIndex = iCurrentNodeIndex + 2;
        if (uiEndNodeIndex >= spPath->GetNodeCount()) {
            NiPoint3 diff;
            char cPastEnd = 0;
            uiEndNodeIndex = iCurrentNodeIndex + 2;
            if (uiEndNodeIndex == spPath->GetNodeCount()) {
                const NiPoint3& kActorLocation = pActor->GetLocationOnReference();
                diff = spPath->GetNode(iCurrentNodeIndex)->Location.GetLocation() - kActorLocation;
            }
            else {
                cPastEnd = 1;
            }
            if (cPastEnd || diff.SqrXYLength() <= 25600.0) {
                uint32_t iupdateNode;
                if (pActorMover->iNextNode) {
                    iupdateNode = pActorMover->iNextNode - 1;
                }
                else {
                    iupdateNode = pActorMover->iNextNode;
                }
                const NiPoint3& kActorLocation = pActor->GetLocationOnReference();
                Pathing::UPDATE_DETAILED_PATH_RESULT updated = Pathing::UpdateSolution(
                    spRequest.m_pObject,
                    spPath.m_pObject,
                    iupdateNode,
                    kActorLocation
                );
                if (updated == Pathing::CLEARED_DETAILED_PATH) {
                    return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
                }
                if (updated > Pathing::CLEARED_DETAILED_PATH && updated <= Pathing::UPDATED_DETAILED_PATH) {
                    bEndUpdatePath = spPath->GetLastNode()->IsTransitionNode();
                    pActorMover->iNextNode = spPath->iLastLoadedVirtualNodeIndex;
                    return ActorPathHandler::PATH_HANDLER_RESULT_OK;
                }
                bEndUpdatePath = false;
            }
        }
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    bool bgoalReached = false;
    float fdistanceSqr = std::numeric_limits<float>::max();
    if (spPath->IsIncomplete() ||
        (iLastLoadedIndex = spPath->iLastLoadedVirtualNodeIndex,
         iLastLoadedIndex != spPath->GetVirtualNodeCount() - 1)) {
        goal = spPath->GetLastNode()->Location.GetLocation();
    }
    else {
        VirtualPathingNode* pVirtualNode = spPath->GetVirtualNode(spPath->iLastLoadedVirtualNodeIndex);
        goal = pVirtualNode->Location.GetLocation();
    }
    NiPoint3 diff = goal - pActor->GetLocationOnReference();
    fdistanceSqr = diff.SqrXYLength();
    bgoalReached = fGoalRadiusSqr >= static_cast<double>(fdistanceSqr);
    if (!bgoalReached) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    bool bvalidCheck = true;
    if (spRequest->GetSoftGoalRadius() &&
        fInnerGoalRadiusSqr < static_cast<double>(fGoalRadiusSqr) &&
        fInnerGoalRadiusSqr < static_cast<double>(fdistanceSqr)) {
        NiPoint3 tetherDir = TetherTargetPoint - LastTetherPoint;
        tetherDir.z = 0.0;
        NiPoint3 actorToTetherDir = TetherTargetPoint - pActor->GetLocationOnReference();
        actorToTetherDir.z = 0.0;
        float fdot = tetherDir.Dot(actorToTetherDir);
        if (fdot >= 0.0 && fAvoidanceDetectionValue == 0.0 && fBlockTime == 0.0) {
            bvalidCheck = false;
        }
    }
    if (!bvalidCheck) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    if (pActor->IsSwimming()) {
        float fHeight = pActor->GetHeight();
        float fGoalZDelta = spRequest->fGoalZDelta;
        float fzdeltaThresold = BSMIN(fGoalZDelta, fHeight);
        double dAbsZ = std::fabs(diff.z);
        if (fzdeltaThresold >= dAbsZ) {
            return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
        }
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    double dAbsZ = std::fabs(diff.z);
    if (spRequest->fGoalZDelta >= dAbsZ) {
        return ActorPathHandler::PATH_HANDLER_RESULT_DONE;
    }
    uint32_t uiNextNodeIndex = iCurrentNodeIndex + 1;
    if (uiNextNodeIndex < spPath->GetNodeCount() || pActor->IsSwimming()) {
        return ActorPathHandler::PATH_HANDLER_RESULT_OK;
    }
    const char* pName = pActor->GetFormEditorIDLength() ? pActor->GetFormEditorID() : pActor->GetFullName();
    Warning(
        "PATHFINDING: Actor %s failed pathing due to goal Z difference %f, being too large",
        pName,
        std::fabs(diff.z)
    );
    return ActorPathHandler::PATH_HANDLER_RESULT_CANT_REACH_Z;
}

void DetailedActorPathHandler::BuildAvoidanceList(
    Actor* apActor,
    PathingRequest* aspRequest,
    uint32_t aMoveMode,
    const NiPoint3& aMoveDelta,
    DetailedActorPathHandler* apHandler,
    BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>& aRetCandidates
) {
    float fdetection;
    NiMatrix3 rot;
    float fAngle = apActor->GetHeading(false);
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
    DetailedActorPathHandler::GetFollowLeader(apActor);
    if ((aMoveMode & ActorMover::MF_RUNNING) != 0) {
        favoidConeAngle = DetailedActorPathHandler::fRunningActorAvoidConeAngle;
    }
    DetailedActorPathHandler::UpdateAvoidanceCandidateList();
    for (uint32_t i = 0; i < DetailedActorPathHandler::uiAvoidanceCandidatesSize; ++i) {
        Actor* pact = DetailedActorPathHandler::AvoidanceCandidates[i];
        if (pact != apActor) {
            NiPoint3 toOtherActor = pact->GetLocationOnReference();
            toOtherActor -= currentPos;
            float fflatDist2 = toOtherActor.SqrXYLength();
            if (!(fflatDist2 > 1e+06)) {
                float ftheirRadius = pact->GetRadius();
                if (ftheirRadius >= fourRadius * 0.33000001311302185) {
                    NiPoint3 toTarget;
                    NiPoint3 unitToTarget;
                    NiPoint3 relativeMoveDir = worldSpaceMoveDir;
                    if (pact->IsMoving()) {
                        NiMatrix3 targetRot;
                        targetRot.MakeZRotationQ(pact->GetHeading(false));
                        NiPoint3 targetWorldSpaceMoveDir = targetRot * *pact->GetActorMoverOverrideMovement();
                        relativeMoveDir -= targetWorldSpaceMoveDir;
                    }
                    float fmoveLength = relativeMoveDir.UnitizeGetLength();
                    if (!(fflatDist2 > gs_fAvoidanceAvoidAllRadius_Pathfinding.Float() *
                              gs_fAvoidanceAvoidAllRadius_Pathfinding.Float()) ||
                        (toTarget = pact->GetLocationOnReference(),
                         (&(toTarget -= currentPos),
                          (unitToTarget = toTarget,
                           (unitToTarget.Unitize(), !(unitToTarget.Dot(unitWorldSpaceMoveDir) < 0.0)))))) {
                        float fmoveSpeed = fmoveLength / fmoveTime;
                        float favoidDistance = favoidTime * fmoveSpeed;
                        float fcheckDistance = fourRadius + favoidDistance;
                        if (gs_fAvoidanceAvoidAllRadius_Pathfinding.Float() > static_cast<double>(fcheckDistance)) {
                            fcheckDistance = gs_fAvoidanceAvoidAllRadius_Pathfinding.Float();
                        }
                        if (DetailedActorPathHandler::CheckDetection(
                                apActor,
                                apHandler,
                                pact,
                                currentPos,
                                relativeMoveDir,
                                fcheckDistance,
                                favoidConeAngle,
                                fdetection,
                                false
                            )) {
                            aRetCandidates.Add(DetailedActorPathHandler::ActorAvoidanceData(pact, fdetection));
                        }
                    }
                }
            }
        }
    }
    if (DetailedActorPathHandler::CheckDetection(
            apActor,
            apHandler,
            pPlayer,
            currentPos,
            worldSpaceMoveDir,
            fabsoluteAvoidDistance,
            favoidConeAngle,
            fdetection,
            false
        )) {
        aRetCandidates.Add(DetailedActorPathHandler::ActorAvoidanceData(pPlayer, fdetection));
    }
}

Actor* DetailedActorPathHandler::GetFollowLeader(Actor* apActor) {
    BaseProcess* baseProcess = apActor->MobileObject::GetCurrentAIProcess();
    if (!baseProcess) {
        return nullptr;
    }
    TESPackage* pPackage = baseProcess->GetPackageThatIsRunning();
    if (!pPackage) {
        return nullptr;
    }
    PACKAGE_PROCEDURE_TYPE eProcedureType = pPackage->TESPackage::GetProcedureType();
    if (Procedure::Action[eProcedureType][baseProcess->GetProcedureIndexRunning()] != PROCEDURE_FOLLOW) {
        return nullptr;
    }
    Actor* pTarget = static_cast<Actor*>(baseProcess->GetTarget());
    if (!pTarget) {
        return nullptr;
    }
    if (pTarget->IsActor()) {
        return pTarget;
    }
    return nullptr;
}

void DetailedActorPathHandler::UpdateAvoidNodesForPath(
    Actor* apActor,
    PathingRequest* apRequest,
    DetailedActorPathHandler* apHandler
) {
    NiPoint3 avoidCenter;
    NiPoint3 kAheadLocation;
    using avoidActors_t = BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>;
    if (!apHandler || apHandler->eAvoidState != DetailedActorPathHandler::PATH_AVOID_REPATH) {
        float fRadiusScale = 1.5f;
        avoidActors_t avoidActors;
        DetailedActorPathHandler::BuildAvoidanceList(
            apActor,
            apRequest,
            257u,
            NiPoint3::UNIT_Y,
            apHandler,
            avoidActors
        );
        if (!avoidActors.IsEmpty()) {
            PathingAvoidNodeArray* pavoidNodes = apRequest->GetAvoidNodeArray();
            if (!pavoidNodes) {
                pavoidNodes = new PathingAvoidNodeArray;
                apRequest->SetAvoidNodeArray(pavoidNodes);
            }
            for (uint32_t i = 0; i < avoidActors.iSize; ++i) {
                NiMatrix3 targetRot;
                PathingAvoidNode::AVOID_NODE_TYPE nodeType;
                float fnodeRadius;
                Actor* pavoidActor = avoidActors.GetAt(static_cast<int32_t>(i)).pActor;
                if (apHandler) {
                    apHandler->ActorAvoidList.AddHead(pavoidActor);
                }
                if (pavoidActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    pavoidActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    double dRadius = pavoidActor->GetRadius();
                    fnodeRadius = dRadius * 0.5 * fRadiusScale;
                    nodeType = PathingAvoidNode::AVOID_NODE_SPHERE;
                }
                else {
                    double dRadius = pavoidActor->GetRadius();
                    fnodeRadius = dRadius * fRadiusScale;
                    nodeType = PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR;
                }
                float fAngle = pavoidActor->GetHeading(false);
                targetRot.MakeZRotationQ(fAngle);
                alignas(16)
                    NiPoint3 targetWorldSpaceMoveDir = targetRot * *pavoidActor->GetActorMoverOverrideMovement();
                NiPoint3 OneSecondDelta = targetWorldSpaceMoveDir / appTimer.fDelta;
                double dSpeedSqr = OneSecondDelta.SqrLength();
                double dActorRadius = pavoidActor->GetRadius();
                if (dActorRadius < dSpeedSqr) {
                    ::new (&kAheadLocation) NiPoint3(pavoidActor->GetLocationOnReference() + OneSecondDelta);
                    pavoidNodes->Add(PathingAvoidNode(kAheadLocation, fnodeRadius, 2.0f, nodeType));
                }
                avoidCenter = pavoidActor->GetLocationOnReference();
                bhkCharacterController* pavoidCon = pavoidActor->GetCharController();
                if (pavoidCon && !pavoidCon->GetFlag(bhkCharacterController::NO_CHARACTER_COLLISIONS)) {
                    NiPoint3 pconPosNi;
                    hkVector4 pconPos(pavoidCon->GethkPosition());
                    HK2NI(pconPosNi, pconPos);
                    avoidCenter.x = pconPosNi.x;
                    avoidCenter.y = pconPosNi.y;
                }

                pavoidNodes->Add(PathingAvoidNode(avoidCenter, fnodeRadius, 2.0f, nodeType));
                if (apHandler) {
                    apHandler->eAvoidState = DetailedActorPathHandler::PATH_AVOID_REPATH;
                }
            }
        }
    }
}

bool DetailedActorPathHandler::UpdateAvoidanceCandidateList() {
    uint32_t inow = appTimer.iLastTime;
    if (DetailedActorPathHandler::uiAvoidanceCandidatesTimestamp == inow) {
        return false;
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
            if (ptheir3d && static_cast<uint8_t>(ptheir3d->QFadedIn())) {
                bhkCharacterController* pController;
                double dLODDistance;
                if (!pobj->GetCharController() ||
                    !static_cast<uint8_t>(ptheir3d->GetNotVisible()) ||
                    (pController = pobj->GetCharController(),
                     dLODDistance = pController->GetLODDistance(),
                     !(fLODActorAvoidanceDistance.Float() < dLODDistance))) {
                    ++DetailedActorPathHandler::uiAvoidanceCandidatesSize;
                    DetailedActorPathHandler::AvoidanceCandidates.Add(pobj);
                }
            }
        }
        plist = plist->m_pkNext;
    }
    return true;
}

bool DetailedActorPathHandler::CheckDetection(
    Actor* apActor,
    DetailedActorPathHandler* apHandler,
    Actor* apOtherActor,
    const NiPoint3& aLocation,
    const NiPoint3& aVelocityDir,
    float afDistanceCheck,
    float afAvoidAngleCone,
    float& afOutDetection,
    bool abCheckOnly
) {
    float flen;
    afOutDetection = 0.0f;
    if (apHandler &&
        (apHandler->eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH_IGNORE ||
         apHandler->eAvoidState == DetailedActorPathHandler::PATH_AVOID_REPATH) &&
        apHandler->ActorAvoidList.IsInList(apOtherActor)) {
        return false;
    }
    NiPoint3 actorToOtherDir = apOtherActor->GetLocationOnReference() - aLocation;
    float factorForwardLength = apActor->GetForwardLength();
    double dRadius = apOtherActor->GetRadius();
    float fdistanceCheck = dRadius + afDistanceCheck + factorForwardLength;
    float fdistanceCheckSqr = fdistanceCheck * fdistanceCheck;
    float fsqrXYLength = actorToOtherDir.SqrXYLength();
    if (!(fdistanceCheckSqr > fsqrXYLength)) {
        return false;
    }
    if (!(std::fabs(actorToOtherDir.z) < 180.0)) {
        return false;
    }
    if (apActor->IsInCombat()) {
        if (apOtherActor == apActor->GetCombatTarget()) {
            return false;
        }
    }
    ActorPackage* pactorPackage = nullptr;
    if (apActor->pCurrentProcess) {
        pactorPackage = apActor->pCurrentProcess->GetActorPackageThatIsRunning();
    }
    if (pactorPackage) {
        Actor* pTarget = static_cast<Actor*>(pactorPackage->pTarget);
        if (pTarget == apOtherActor && pactorPackage->pPackage->GetPackType() != PACKAGE_TYPE_FOLLOW) {
            return false;
        }
    }
    actorToOtherDir.z = 0.0f;
    float fdist = actorToOtherDir.UnitizeGetLength();
    bool bignoreCone = false;
    if (gs_fAvoidanceAvoidAllRadius_Pathfinding.Float() > static_cast<double>(fdist)) {
        bignoreCone = true;
    }
    fdist = fdist - factorForwardLength;
    fdist = BSMAX(fdist, 0.0f);
    float fdot = actorToOtherDir.Dot(aVelocityDir);
    if (!(afAvoidAngleCone <= static_cast<double>(fdot)) && !bignoreCone) {
        return false;
    }
    if (abCheckOnly) {
        return true;
    }
    flen = fdist / fdistanceCheck;
    BSClamp(flen, 0.0f, 1.0f);
    int32_t idist = static_cast<int32_t>(flen * 4.0);
    float fdetection = fdot;
    switch (idist) {
        case 3:
        case 4: {
            fdetection = fdetection * fdetection;
            [[fallthrough]];
        }
        case 2: {
            fdetection = fdetection * fdetection;
            [[fallthrough]];
        }
        case 1: {
            fdetection = fdetection * fdetection;
            break;
        }
        default: {
            break;
        }
    }
    afOutDetection = fdetection;
    return true;
}

// Runs the base filter's constructor first, then stores the vtable, the door ID and the three flags: both triangle
// flags clear and the whole-path flag set.
DoorInPathFilter::DoorInPathFilter(uint32_t aiDoorID)
  : Pathing::CheckLineOfSightFilter(), DoorID(aiDoorID), bEnterTriWithDoor(false), bLeaveTriWithDoor(false),
    bEntirePathInDoor(true) {}

bool DoorInPathFilter::CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) {
    NavMeshClosedDoorInfo* pMatchingDoorInfo;
    NavMeshClosedDoorInfo* pClosedDoorInfo;
    int8_t cEdgeInDoor = 0;
    NavMeshTriangle* pTriangle = hEdge.pNavMesh->GetTriangle(hEdge.usTriangle);
    if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
        pClosedDoorInfo = hEdge.pNavMesh->GetClosedDoorInfoForTriangle(hEdge.usTriangle);
        if (pClosedDoorInfo && (pClosedDoorInfo->DoorFormID == DoorID || !DoorID)) {
            cEdgeInDoor = 1;
        }
    }
    int8_t cMatchingEdgeInDoor = 0;
    NavMeshTriangle* pMatchingTriangle = hMatchingEdge.pNavMesh->GetTriangle(hMatchingEdge.usTriangle);
    if (pMatchingTriangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
        pMatchingDoorInfo = hMatchingEdge.pNavMesh->GetClosedDoorInfoForTriangle(hMatchingEdge.usTriangle);
        if (pMatchingDoorInfo && (pMatchingDoorInfo->DoorFormID == DoorID || !DoorID)) {
            cMatchingEdgeInDoor = 1;
        }
    }
    if (!cEdgeInDoor) {
        bEntirePathInDoor = false;
    }
    if (!cMatchingEdgeInDoor) {
        bEntirePathInDoor = false;
    }
    if (cEdgeInDoor && !cMatchingEdgeInDoor) {
        bLeaveTriWithDoor = true;
    }

    if (!cEdgeInDoor && cMatchingEdgeInDoor) {
        bEnterTriWithDoor = true;
    }

    return true;
}

bool PathingRequest::GetFailIfGoalOccupied() {
    return bFailIfGoalOccupied;
}

bool PathingSolution::IsDoorInPath(uint32_t aDoorID) {
    return DoorIDs.Find(aDoorID, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) != -1;
}

void NiMatrix3::MakeZRotationQ(float fAngle) {
    float cs;
    float sn;
    NiSinCos_Fast(fAngle, &sn, &cs);
    m_pEntry[0][0] = cs;
    m_pEntry[0][1] = sn;
    m_pEntry[0][2] = 0.0;
    m_pEntry[1][0] = -sn;
    m_pEntry[1][1] = cs;
    m_pEntry[1][2] = 0.0;
    m_pEntry[2][0] = 0.0;
    m_pEntry[2][1] = 0.0;
    m_pEntry[2][2] = 1.0;
}
