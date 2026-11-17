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
    bSortDistanceFromTarget(false), bHideCover(false), DistanceProjectionVector(NiPoint3_ZERO),
    ActorLocation(NiPoint3_ZERO) {
    fActorRadius = 24.0f;
    fActorHeight = 128.0f;
}

PathingRequestCover::~PathingRequestCover() {}

// Copies everything but bCanCrouch, which is deliberately left alone as the game does, through the target's setters.
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
    return (bool)((uint8_t)bSortFarthestFirst);
}

bool PathingRequestCover::GetFindBothSides() {
    return bFindBothSides;
}

bool PathingRequestCover::GetFindBehindTarget() {
    return bFindBehindTarget;
}

void PathingRequestCover::SetThreat(const NiPoint3& a2) {
    Threat = a2;
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
    bFindBothSides = (bool)((uint8_t)abValue);
}

void PathingRequestCover::SetFindBehindTarget(bool abValue) {
    bFindBehindTarget = (bool)((int8_t)abValue);
}

void PathingRequestCover::SetSortFarthestFirst(bool abValue) {
    bSortFarthestFirst = (bool)((int8_t)abValue);
}

void PathingRequestCover::SetSortDistanceFromTarget(bool abValue) {
    bSortDistanceFromTarget = (bool)((int8_t)abValue);
}

void PathingRequestCover::SetHideCover(bool abValue) {
    bHideCover = (bool)((uint8_t)abValue);
}

void PathingRequestCover::SetActorPos(const NiPoint3& arActorPos) {
    ActorLocation = arActorPos;
}

PathingCoverLocation::PathingCoverLocation() : PathingLocation() {}

void PathingCoverLocation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    PathingLocation::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&LeftVertex)), 0xCu, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&RightVertex)), 0xCu, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&EdgeDirection)), 0xCu, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&cCoverHeight)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bStandCover)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bHigherThanCrouch)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bLeftEdgeOpen)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&bRightEdgeOpen)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&cEdge)), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&cState)), 1u, 0);
}

void PathingCoverLocation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    PathingLocation::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&LeftVertex)), 0xCu);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&RightVertex)), 0xCu);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&EdgeDirection)), 0xCu);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cCoverHeight)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bStandCover)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bHigherThanCrouch)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bLeftEdgeOpen)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&bRightEdgeOpen)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cEdge)), 1u);
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cState)), 1u);
}

bool PathingCoverLocation::IsLedgeCover() const {
    return (bool)((uint8_t)(cCoverHeight == 2));
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
    cCoverHeight = (uint8_t)aiCoverVal;
    bStandCover = (bool)((uint8_t)(((int32_t)aiCoverVal) >= ((int32_t)aiStandVal)));
    bHigherThanCrouch = (bool)((uint8_t)(((int32_t)aiCoverVal) > ((int32_t)aiCrouchVal)));
    bLeftEdgeOpen = (bool)((uint8_t)abLeftOpen);
    bRightEdgeOpen = (bool)((uint8_t)abRightOpen);
    cEdge = acEdge;
    cState = (uint8_t)0;
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
    NiPoint3 v5;
    NiPoint3 v6;
    (v5 = (&LeftVertex)->operator-(arPoint));
    (v6 = (&RightVertex)->operator-(arPoint));
    double v3 = (double)(&v5)->SqrLength();
    return (bool)((uint8_t)((&v6)->SqrLength() > v3));
}

bool PathingCoverLocation::IsStandCover() {
    return (bool)((uint8_t)(bHigherThanCrouch && (!IsLedgeCover())));
}

uint32_t PathingCoverLocation::GetKey() {
    FNVD_INLINED_LOCAL(NiPointer<NavMesh>, apMemory, std::nullopt, std::nullopt);
    uint16_t arTriangle;
    ::new (&apMemory) NavMeshPtr;
    arTriangle = 0;
    if (GetNavMeshAndTriangle(static_cast<NavMeshPtr&>(apMemory), arTriangle)) {
        uint8_t cEdge = this->cEdge;
        uint16_t v4 = arTriangle;
        NavMesh* v1 = apMemory.m_pObject;
        uint32_t CastingType = v1->GetFormID();
        uint32_t v7 = PathingCoverLocation::ComputeKey(CastingType, v4, cEdge);
        apMemory.~NiPointer<NavMesh>();
        return v7;
    }

    apMemory.~NiPointer<NavMesh>();
    return 0;
}

bool PathingCoverLocation::CheckDynamicCoverValid(uint32_t aiLastMovedTime) {
    NavMeshPtr spnavMesh;
    if (GetNavMesh(spnavMesh)) {
        NiPointer<ObstacleData> spObstacle(nullptr);
        uint16_t Triangle = GetTriangle();
        if (spnavMesh.m_pObject->GetObstacleForTriangle(Triangle, spObstacle) &&
            spObstacle.m_pObject->iLastUpdateTime == aiLastMovedTime) {
            return true;
        }
    }
    return false;
}

uint32_t PathingCoverLocation::GetDynamicCoverObstacleTimeStamp() {
    NavMeshPtr spnavMesh;
    if (GetNavMesh(spnavMesh)) {
        NiPointer<ObstacleData> spObstacle(nullptr);
        uint16_t Triangle = GetTriangle();
        if (spnavMesh.m_pObject->GetObstacleForTriangle(Triangle, spObstacle)) {
            uint32_t iLastUpdateTime = spObstacle.m_pObject->iLastUpdateTime;
            return iLastUpdateTime;
        }
    }
    return -1;
}

uint32_t PathingCoverLocation::ComputeKey(uint32_t aiNavMeshID, uint16_t aiTriangle, uint8_t aiEdge) {
    return aiEdge + ((aiNavMeshID << 16) + (4 * aiTriangle));
}
