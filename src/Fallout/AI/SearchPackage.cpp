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

// The exe's cleanup-only C++ EH frame is left out. The cover copy is made by the exe's copy constructor and
// destroyed by Destruct, with no constructor before them.
NiPoint3* SearchPackage::LookForTarget(NiPoint3* a1, TESObjectREFR* a2) {
	NiPoint3 v13;
	NiPoint3 apResult;
	FNVD_INLINED_LOCAL(PathingCoverLocation, v16, std::nullopt, std::nullopt);
	NiPoint3 arg0;
	arg0 = this->LocationCenter;
	if (this->LocationCenter.operator==(NiPoint3_ZERO)) {
		NiPoint3 v20 = a2->GetLocationOnReference();
		this->LocationCenter = *HighProcess::ComputeRandomPoint(&apResult, v20, 800.0f, 1000.0f, 0);
		const NiPoint3* afMaxRadius = &a2->GetLocationOnReference();
		TESObjectCELL* ParentCell = a2->GetParentCell();
		TESWorldSpace* WorldSpace = a2->GetWorldSpace();
		Pathing::FindPointOnNavMesh(WorldSpace, ParentCell, *afMaxRadius, 300.0f, &this->LocationCenter);
		*a1 = this->LocationCenter;
		return a1;
	} else {
		if (coverPoints.IsEmpty()) {
			TESWorldSpace* v11 = a2->GetWorldSpace();
			TESObjectCELL* pcell = a2->GetParentCell();
			NiPoint3 kTargetPos = this->LocationCenter;
			NiPoint3 v19 = a2->GetLocationOnReference();
			FindCoverToSearch(v19, kTargetPos, pcell, v11);
		}
		if (coverPoints.IsEmpty() || iCurrentCover >= coverPoints.iSize) {
			arg0 = *HighProcess::ComputeRandomPoint(&v13, this->LocationCenter, 800.0f, 300.0f, 0);
			TESObjectCELL* v8 = a2->GetParentCell();
			TESWorldSpace* v6 = a2->GetWorldSpace();
			Pathing::FindPointOnNavMesh(v6, v8, arg0, 300.0f, &arg0);
		} else {
			PathingCoverLocation* v5 = &coverPoints.operator[](static_cast<uint32_t>(iCurrentCover));
			::new (&v16) PathingCoverLocation(*v5);
			++iCurrentCover;
			arg0 = v16.GetLocation();
			v16.~PathingCoverLocation();
		}
		*a1 = arg0;
		return a1;
	}
}

// The exe's cleanup-only C++ EH frame is left out. The origin location is built by the exe's constructor
// (0x6DCEE0) and destroyed (0x4FF7E0) right after SetOrigin, before SetThreat.
void SearchPackage::FindCoverToSearch(NiPoint3 arg0, NiPoint3 a3, TESObjectCELL* apCell, TESWorldSpace* apWorld) {
	{
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, location, &PathingLocation::Destruct, &arg0, apCell, apWorld);
		coverRequest.SetOrigin(location);
	}
	coverRequest.SetThreat(a3);
	coverRequest.SetTargetRadius(1000.0f);
	Pathing::GetCoverPoints(&coverRequest, coverPoints);
}

void SearchPackage::operator delete(void* apMemory) {
	ourFree(apMemory);
}
