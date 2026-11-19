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
NiPoint3 SearchPackage::LookForTarget(TESObjectREFR* a2) {
    NiPoint3 arg0;
    arg0 = this->LocationCenter;
    if (this->LocationCenter == NiPoint3_ZERO) {
        NiPoint3 v20 = a2->GetLocationOnReference();
        this->LocationCenter = HighProcess::ComputeRandomPoint(v20, 800.0f, 1000.0f, false);
        const NiPoint3* afMaxRadius = &a2->GetLocationOnReference();
        TESObjectCELL* ParentCell = a2->GetParentCell();
        TESWorldSpace* WorldSpace = a2->GetWorldSpace();
        Pathing::FindPointOnNavMesh(WorldSpace, ParentCell, *afMaxRadius, 300.0f, &this->LocationCenter);
        return this->LocationCenter;
    }

    if (coverPoints.IsEmpty()) {
        TESWorldSpace* v11 = a2->GetWorldSpace();
        TESObjectCELL* pcell = a2->GetParentCell();
        NiPoint3 kTargetPos = this->LocationCenter;
        NiPoint3 v19 = a2->GetLocationOnReference();
        FindCoverToSearch(v19, kTargetPos, pcell, v11);
    }
    if (coverPoints.IsEmpty() || iCurrentCover >= coverPoints.iSize) {
        arg0 = HighProcess::ComputeRandomPoint(this->LocationCenter, 800.0f, 300.0f, false);
        TESObjectCELL* v8 = a2->GetParentCell();
        TESWorldSpace* v6 = a2->GetWorldSpace();
        Pathing::FindPointOnNavMesh(v6, v8, arg0, 300.0f, &arg0);
    }
    else {
        PathingCoverLocation* v5 = &coverPoints[static_cast<uint32_t>(iCurrentCover)];
        PathingCoverLocation v16(*v5);
        ++iCurrentCover;
        arg0 = v16.GetLocation();
    }
    return arg0;
}

// The origin location is constructed and destroyed right after SetOrigin, before SetThreat.
void SearchPackage::FindCoverToSearch(NiPoint3 arg0, NiPoint3 a3, TESObjectCELL* apCell, TESWorldSpace* apWorld) {
    {
        PathingLocation location(&arg0, apCell, apWorld);
        coverRequest.SetOrigin(location);
    }
    coverRequest.SetThreat(a3);
    coverRequest.SetTargetRadius(1000.0f);
    Pathing::GetCoverPoints(&coverRequest, coverPoints);
}

void SearchPackage::operator delete(void* apMemory) {
    ourFree(apMemory);
}
