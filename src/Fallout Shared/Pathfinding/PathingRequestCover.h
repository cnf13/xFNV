#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A request for cover from a threat: how far to look, how the candidates are ordered and what height the actor crouches
// to. GetType and CopyTo are overridden; SaveGame and LoadGame are PathingRequest's.
class PathingRequestCover : public PathingRequest {
public:
    PathingRequestCover();
    ~PathingRequestCover() override;

    void CopyTo(PathingRequest* apDest) const override;
    uint32_t GetType() const override;

    bool GetSortFarthestFirst();
    bool GetFindBothSides();
    bool GetFindBehindTarget();
    bool GetCanCrouch() const;
    // bHideCover.
    bool GetHideCover();
    void SetThreat(const NiPoint3& arThreat);
    NiPoint3 GetThreat();
    // Stores fActorCrouchHeight.
    void SetCrouchHeight(float afCrouch);
    void SetMaxCoverDistance(float afDistance);
    void SetMaxHeightChange(float afHeightChange);
    // Stores iMaxResults.
    void SetMaxResults(uint32_t aiResults);
    void SetDistanceProjectionVector(const NiPoint3& arVector);
    void SetFindBothSides(bool abValue);
    void SetFindBehindTarget(bool abValue);
    void SetSortFarthestFirst(bool abValue);
    void SetSortDistanceFromTarget(bool abValue);
    void SetHideCover(bool abValue);
    // Copies the point into ActorLocation.
    void SetActorPos(const NiPoint3& arActorPos);

    NiPoint3 Threat;
    float fMaxCoverDistance;
    float fActorCrouchHeight;
    float fMaxHeightChange;
    uint32_t iMaxResults;
    bool bCanCrouch;
    bool bFindBothSides;
    bool bFindBehindTarget;
    bool bSortFarthestFirst;
    bool bSortDistanceFromTarget;
    bool bHideCover;
    NiPoint3 DistanceProjectionVector;
    NiPoint3 ActorLocation;
};
