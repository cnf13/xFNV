#include "Fallout Shared/Pathfinding/PathingRequestCover.h"

#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

PathingRequestCover::PathingRequestCover()
  : PathingRequest(), fMaxCoverDistance(1024.0f), fActorCrouchHeight(64.0f), fMaxHeightChange(180.0f), iMaxResults(50),
    bCanCrouch(true), bFindBothSides(false), bFindBehindTarget(false), bSortFarthestFirst(false),
    bSortDistanceFromTarget(false), bHideCover(false), DistanceProjectionVector(NiPoint3::ZERO),
    ActorLocation(NiPoint3::ZERO) {
    fActorRadius = 24.0f;
    fActorHeight = 128.0f;
}

PathingRequestCover::~PathingRequestCover() {}

// Copies everything but bCanCrouch, which is deliberately left alone, through the target's setters.
void PathingRequestCover::CopyTo(PathingRequest* apDest) const {
    PathingRequest::CopyTo(apDest);
    PathingRequestCover* pTarget = static_cast<PathingRequestCover*>(apDest);
    pTarget->SetThreat(Threat);
    pTarget->SetCrouchHeight(fActorCrouchHeight);
    pTarget->SetMaxCoverDistance(fMaxCoverDistance);
    pTarget->SetMaxHeightChange(fMaxHeightChange);
    pTarget->SetMaxResults(iMaxResults);
    pTarget->SetFindBothSides(bFindBothSides);
    pTarget->SetFindBehindTarget(bFindBehindTarget);
    pTarget->SetSortFarthestFirst(bSortFarthestFirst);
    pTarget->SetSortDistanceFromTarget(bSortDistanceFromTarget);
    pTarget->SetHideCover(bHideCover);
    pTarget->SetDistanceProjectionVector(DistanceProjectionVector);
}

uint32_t PathingRequestCover::GetType() const {
    return PATHING_REQUEST_COVER;
}

bool PathingRequestCover::GetSortFarthestFirst() {
    return bSortFarthestFirst;
}

bool PathingRequestCover::GetFindBothSides() {
    return bFindBothSides;
}

bool PathingRequestCover::GetFindBehindTarget() {
    return bFindBehindTarget;
}

void PathingRequestCover::SetThreat(const NiPoint3& arThreat) {
    Threat = arThreat;
}

void PathingRequestCover::SetCrouchHeight(float afCrouch) {
    fActorCrouchHeight = afCrouch;
}

void PathingRequestCover::SetMaxCoverDistance(float afDistance) {
    fMaxCoverDistance = afDistance;
}

void PathingRequestCover::SetMaxHeightChange(float afHeightChange) {
    fMaxHeightChange = afHeightChange;
}

void PathingRequestCover::SetMaxResults(uint32_t aiResults) {
    iMaxResults = aiResults;
}

void PathingRequestCover::SetDistanceProjectionVector(const NiPoint3& arVector) {
    DistanceProjectionVector = arVector;
}

void PathingRequestCover::SetFindBothSides(bool abValue) {
    bFindBothSides = abValue;
}

void PathingRequestCover::SetFindBehindTarget(bool abValue) {
    bFindBehindTarget = abValue;
}

void PathingRequestCover::SetSortFarthestFirst(bool abValue) {
    bSortFarthestFirst = abValue;
}

void PathingRequestCover::SetSortDistanceFromTarget(bool abValue) {
    bSortDistanceFromTarget = abValue;
}

void PathingRequestCover::SetHideCover(bool abValue) {
    bHideCover = abValue;
}

void PathingRequestCover::SetActorPos(const NiPoint3& arActorPos) {
    ActorLocation = arActorPos;
}

PathingCoverLocation::PathingCoverLocation() : PathingLocation() {}

void PathingCoverLocation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    PathingLocation::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&LeftVertex, sizeof(LeftVertex), 0);
    apSaveGameBuffer->SaveDataEndian(&RightVertex, sizeof(RightVertex), 0);
    apSaveGameBuffer->SaveDataEndian(&EdgeDirection, sizeof(EdgeDirection), 0);
    apSaveGameBuffer->SaveDataEndian(&cCoverHeight, sizeof(cCoverHeight), 0);
    apSaveGameBuffer->SaveDataEndian(&bStandCover, sizeof(bStandCover), 0);
    apSaveGameBuffer->SaveDataEndian(&bHigherThanCrouch, sizeof(bHigherThanCrouch), 0);
    apSaveGameBuffer->SaveDataEndian(&bLeftEdgeOpen, sizeof(bLeftEdgeOpen), 0);
    apSaveGameBuffer->SaveDataEndian(&bRightEdgeOpen, sizeof(bRightEdgeOpen), 0);
    apSaveGameBuffer->SaveDataEndian(&cEdge, sizeof(cEdge), 0);
    apSaveGameBuffer->SaveDataEndian(&cState, sizeof(cState), 0);
}

void PathingCoverLocation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    PathingLocation::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&LeftVertex, sizeof(LeftVertex));
    apLoadGameBuffer->LoadData(&RightVertex, sizeof(RightVertex));
    apLoadGameBuffer->LoadData(&EdgeDirection, sizeof(EdgeDirection));
    apLoadGameBuffer->LoadData(&cCoverHeight, sizeof(cCoverHeight));
    apLoadGameBuffer->LoadData(&bStandCover, sizeof(bStandCover));
    apLoadGameBuffer->LoadData(&bHigherThanCrouch, sizeof(bHigherThanCrouch));
    apLoadGameBuffer->LoadData(&bLeftEdgeOpen, sizeof(bLeftEdgeOpen));
    apLoadGameBuffer->LoadData(&bRightEdgeOpen, sizeof(bRightEdgeOpen));
    apLoadGameBuffer->LoadData(&cEdge, sizeof(cEdge));
    apLoadGameBuffer->LoadData(&cState, sizeof(cState));
}

bool PathingCoverLocation::IsLedgeCover() const {
    return cCoverHeight == 2;
}

void PathingCoverLocation::SetEdgeOrientation(
    const NiPoint3& aLeftVertex,
    const NiPoint3& aRightVertex,
    const NiPoint3& aDirection
) {
    LeftVertex = aLeftVertex;
    RightVertex = aRightVertex;
    EdgeDirection = aDirection;
}

void PathingCoverLocation::SetCoverData(
    uint16_t aiCoverVal,
    uint16_t aiStandVal,
    uint16_t aiCrouchVal,
    bool abLeftOpen,
    bool abRightOpen,
    uint8_t acEdge
) {
    cCoverHeight = static_cast<uint8_t>(aiCoverVal);
    bStandCover = aiCoverVal >= aiStandVal;
    bHigherThanCrouch = aiCoverVal > aiCrouchVal;
    bLeftEdgeOpen = abLeftOpen;
    bRightEdgeOpen = abRightOpen;
    cEdge = acEdge;
    cState = 0;
}

uint8_t PathingCoverLocation::GetHeight() {
    return cCoverHeight;
}

PathingCoverLocation& PathingCoverLocation::operator=(const PathingCoverLocation& aLoc) {
    PathingLocation::operator=(aLoc);
    LeftVertex = aLoc.LeftVertex;
    RightVertex = aLoc.RightVertex;
    EdgeDirection = aLoc.EdgeDirection;
    cCoverHeight = aLoc.cCoverHeight;
    bStandCover = aLoc.bStandCover;
    bHigherThanCrouch = aLoc.bHigherThanCrouch;
    bLeftEdgeOpen = aLoc.bLeftEdgeOpen;
    bRightEdgeOpen = aLoc.bRightEdgeOpen;
    cEdge = aLoc.cEdge;
    cState = aLoc.cState;
    return *this;
}

bool PathingCoverLocation::IsLeftSideClosest(const NiPoint3& arPoint) {
    NiPoint3 kLeftOffset = LeftVertex - arPoint;
    NiPoint3 kRightOffset = RightVertex - arPoint;
    double dLeftDistSqr = kLeftOffset.SqrLength();
    return kRightOffset.SqrLength() > dLeftDistSqr;
}

bool PathingCoverLocation::IsStandCover() {
    return bHigherThanCrouch && !IsLedgeCover();
}

uint32_t PathingCoverLocation::GetKey() {
    uint16_t usTriangle;
    NavMeshPtr spNavMesh;
    usTriangle = 0;
    if (GetNavMeshAndTriangle(spNavMesh, usTriangle)) {
        uint8_t cEdge = this->cEdge;
        uint32_t uiNavMeshID = spNavMesh->GetFormID();
        return PathingCoverLocation::ComputeKey(uiNavMeshID, usTriangle, cEdge);
    }

    return 0;
}

bool PathingCoverLocation::CheckDynamicCoverValid(uint32_t aiLastMovedTime) {
    NavMeshPtr spNavMesh;
    if (GetNavMesh(spNavMesh)) {
        NiPointer<ObstacleData> spObstacle(nullptr);
        uint16_t usTriangle = GetTriangle();
        if (spNavMesh->GetObstacleForTriangle(usTriangle, spObstacle) &&
            spObstacle->iLastUpdateTime == aiLastMovedTime) {
            return true;
        }
    }
    return false;
}

uint32_t PathingCoverLocation::GetDynamicCoverObstacleTimeStamp() {
    NavMeshPtr spNavMesh;
    if (GetNavMesh(spNavMesh)) {
        NiPointer<ObstacleData> spObstacle(nullptr);
        uint16_t usTriangle = GetTriangle();
        if (spNavMesh->GetObstacleForTriangle(usTriangle, spObstacle)) {
            return spObstacle->iLastUpdateTime;
        }
    }
    return -1;
}

uint32_t PathingCoverLocation::ComputeKey(uint32_t aiNavMeshID, uint16_t aiTriangle, uint8_t aiEdge) {
    return aiEdge + ((aiNavMeshID << 16) + 4 * aiTriangle);
}
