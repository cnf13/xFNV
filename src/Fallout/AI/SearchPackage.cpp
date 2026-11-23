#include "Fallout/AI/SearchPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestCover.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/HighProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

SearchPackage::SearchPackage(NiPoint3 aploccenter, TESObjectREFR* apthreat, bool abAttacked)
  : fSearchStageTimer(0.0f), iAlertLevel(1), fCommentTimer(0.0f), iCurrentCover(0), fIdleTimer(0.0f),
    LocationCenter(aploccenter), pThreatObject(apthreat), bLocationStart(true) {}

SearchPackage::~SearchPackage() {}

void SearchPackage::SetCenterLocaiton(NiPoint3 point) {
    LocationCenter = point;
}

// The cover copy is made by the copy constructor and destroyed by Destruct, with no constructor before them.
NiPoint3 SearchPackage::LookForTarget(TESObjectREFR* apTarget) {
    NiPoint3 kSearchPoint = this->LocationCenter;
    if (this->LocationCenter == NiPoint3::ZERO) {
        this->LocationCenter = HighProcess::ComputeRandomPoint(
            apTarget->GetLocationOnReference(),
            800.0f,
            1000.0f,
            false
        );
        const NiPoint3* pRefLocation = &apTarget->GetLocationOnReference();
        TESObjectCELL* pCell = apTarget->GetParentCell();
        TESWorldSpace* pWorldSpace = apTarget->GetWorldSpace();
        Pathing::FindPointOnNavMesh(pWorldSpace, pCell, *pRefLocation, 300.0f, &this->LocationCenter);
        return this->LocationCenter;
    }

    if (coverPoints.IsEmpty()) {
        TESWorldSpace* pWorldSpace = apTarget->GetWorldSpace();
        TESObjectCELL* pCell = apTarget->GetParentCell();
        NiPoint3 kTargetPos = this->LocationCenter;
        FindCoverToSearch(apTarget->GetLocationOnReference(), kTargetPos, pCell, pWorldSpace);
    }
    if (coverPoints.IsEmpty() || iCurrentCover >= coverPoints.iSize) {
        kSearchPoint = HighProcess::ComputeRandomPoint(this->LocationCenter, 800.0f, 300.0f, false);
        TESObjectCELL* pCell = apTarget->GetParentCell();
        TESWorldSpace* pWorldSpace = apTarget->GetWorldSpace();
        Pathing::FindPointOnNavMesh(pWorldSpace, pCell, kSearchPoint, 300.0f, &kSearchPoint);
    }
    else {
        PathingCoverLocation kCover(coverPoints[static_cast<uint32_t>(iCurrentCover)]);
        ++iCurrentCover;
        kSearchPoint = kCover.GetLocation();
    }
    return kSearchPoint;
}

// The origin location is constructed and destroyed right after SetOrigin, before SetThreat.
void SearchPackage::FindCoverToSearch(
    NiPoint3 akOrigin,
    NiPoint3 akThreat,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorld
) {
    {
        PathingLocation location(&akOrigin, apCell, apWorld);
        coverRequest.SetOrigin(location);
    }
    coverRequest.SetThreat(akThreat);
    coverRequest.SetTargetRadius(1000.0f);
    Pathing::GetCoverPoints(&coverRequest, coverPoints);
}

void SearchPackage::operator delete(void* apMemory) {
    ourFree(apMemory);
}
