#include "Fallout Shared/Pathfinding/PathingRequestOptimalLocation.h"

#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

PathingRequestOptimalLocation::PathingRequestOptimalLocation()
	: PathingRequest(), fMinDistance(0x1p+7f), fMaxDistance(0x1p+11f), fAbsoluteMinDistance(0x1p+7f),
	  fAbsoluteMaxDistance(0x1p+11f) {}

PathingRequestOptimalLocation::PathingRequestOptimalLocation(const PathingRequestOptimalLocation& aRequest)
	: PathingRequest(aRequest), fMinDistance(aRequest.fMinDistance), fMaxDistance(aRequest.fMaxDistance),
	  fAbsoluteMinDistance(aRequest.fAbsoluteMinDistance), fAbsoluteMaxDistance(aRequest.fAbsoluteMaxDistance) {}

PathingRequestOptimalLocation::~PathingRequestOptimalLocation() {}

void PathingRequestOptimalLocation::CopyTo(PathingRequest* apDest) const {
	PathingRequest::CopyTo(apDest);
	static_cast<PathingRequestClosePoint*>(apDest)->SetMinDistance(fMinDistance);
	static_cast<PathingRequestClosePoint*>(apDest)->SetMaxDistance(fMaxDistance);
	static_cast<PathingRequestOptimalLocation*>(apDest)->SetActorHeight(fAbsoluteMinDistance);
	static_cast<PathingRequestCoveredMove*>(apDest)->SetActorSneakHeight(fAbsoluteMaxDistance);
}

uint32_t PathingRequestOptimalLocation::GetType() const {
	return PATHING_REQUEST_OPTIMAL_LOCATION;
}

// Saves the base's fields, then the two distances as raw bytes.
void PathingRequestOptimalLocation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	PathingRequest::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fMinDistance)), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fMaxDistance)), 4u, 0);
}

// The base's fields, then the two distances; the load buffer's LoadData is called directly.
void PathingRequestOptimalLocation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	PathingRequest::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->BGSLoadGameBuffer::LoadData(&fMinDistance, 4u);
	apLoadGameBuffer->BGSLoadGameBuffer::LoadData(&fMaxDistance, 4u);
}

void PathingRequestOptimalLocation::SetActorHeight(float afHeight) {
	fAbsoluteMinDistance = afHeight;
}

void PathingRequestOptimalLocation::SetMinDistance(float afDistance) {
	fMinDistance = afDistance;
}

void PathingRequestOptimalLocation::SetMaxDistance(float afDistance) {
	fMaxDistance = afDistance;
}

void PathingRequestOptimalLocation::SetAbsoluteMaxDistance(float afDistance) {
	fAbsoluteMaxDistance = afDistance;
}
