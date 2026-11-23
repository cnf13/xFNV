#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestCover.h"

class TESObjectREFR;
class TESWorldSpace;

// The package of an actor that searches.
class SearchPackage : public TESPackage {
public:
    SearchPackage(NiPoint3 aploccenter, TESObjectREFR* apthreat, bool abAttacked);
    ~SearchPackage() override;

    void SetCenterLocaiton(NiPoint3 point);
    NiPoint3 LookForTarget(TESObjectREFR* apTarget);
    // The only caller (LookForTarget) passes the reference's GetParentCell and GetWorldSpace, which are passed on to
    // PathingLocation's (cell, worldspace) constructor.
    void FindCoverToSearch(NiPoint3 akOrigin, NiPoint3 akThreat, TESObjectCELL* apCell, TESWorldSpace* apWorld);

    static void operator delete(void* apMemory);

    // The constructor sets iAlertLevel to 1 and the rest to 0.
    float fSearchStageTimer;
    int32_t iAlertLevel;
    float fCommentTimer;
    int32_t iCurrentCover;
    float fIdleTimer;
    NiPoint3 LocationCenter;
    // The attacker Actor::InitiateSearchForAttacker passes to the constructor.
    TESObjectREFR* pThreatObject;
    bool bLocationStart;
    PathingRequestCover coverRequest;
    // Filled by Pathing::GetCoverPoints from coverRequest.
    BSSimpleArray<PathingCoverLocation> coverPoints;
};
