#include "Fallout Shared/Pathfinding/PathingRequestHide.h"

#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

PathingRequestHide::PathingRequestHide() : PathingRequest(), ThreatLocation(), fMinHideDistance(128.0f) {
	Destination.SetUnused();
}

PathingRequestHide::PathingRequestHide(const PathingRequestHide& aRequest)
	: PathingRequest(aRequest), ThreatLocation(), fMinHideDistance(aRequest.fMinHideDistance) {
	ThreatLocation = aRequest.GetThreatLocation();
	Destination.SetUnused();
}

PathingRequestHide::~PathingRequestHide() {}

void PathingRequestHide::CopyTo(PathingRequest* apDest) const {
	PathingRequest::CopyTo(apDest);
	PathingRequestHide* pTarget = static_cast<PathingRequestHide*>(apDest);
	pTarget->ThreatLocation = ThreatLocation;
	pTarget->fMinHideDistance = fMinHideDistance;
}

uint32_t PathingRequestHide::GetType() const {
	return PATHING_REQUEST_HIDE;
}

void PathingRequestHide::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	PathingRequest::SaveGame(apSaveGameBuffer);
	(&ThreatLocation)->SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fMinHideDistance)), 4u, 0);
}

void PathingRequestHide::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	PathingRequest::LoadGame(apLoadGameBuffer);
	ThreatLocation.LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fMinHideDistance), 4u);
}

void PathingRequestHide::SetMinHideDistance(float afDistance) {
	fMinHideDistance = afDistance;
}

void PathingRequestHide::SetThreatLocation(const PathingLocation& aThreatLocation) {
	&(&ThreatLocation)->operator=(aThreatLocation);
}
