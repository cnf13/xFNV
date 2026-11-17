#include "Fallout/AI/Movement/ActorPathHandler.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

ActorPathHandler::ActorPathHandler(Actor* apActor, ActorMover* apActorMover)
  : pActor(apActor), pActorMover(apActorMover), spPath(nullptr), spRequest(nullptr) {}

ActorPathHandler::ActorPathHandler(Actor* a1, ActorMover* a2, NiPointer<PathingSolution>& a3, PathingRequest* a4)
  : pActor(a1), pActorMover(a2), spPath(a3), spRequest(a4) {}

ActorPathHandler::~ActorPathHandler() {}

bool ActorPathHandler::InitRotationOnly(float fTargetAngle, bool abForceTurn) {
    return true;
}

ActorPathHandler::PATH_HANDLER_RESULT ActorPathHandler::NoPathUpdate(float afDeltaT) {
    return PATH_HANDLER_RESULT_DONE;
}

bool ActorPathHandler::IsWaitingForAvoidance() {
    return true;
}

bool ActorPathHandler::IsMovementBlocked() {
    return false;
}

bool ActorPathHandler::ComputePathPassesThroughPlayersArea() {
    return false;
}

bool ActorPathHandler::GetNextTetherPoint(NiPoint3& arPoint) {
    return false;
}

bool ActorPathHandler::WarpToNextNode() {
    return false;
}
