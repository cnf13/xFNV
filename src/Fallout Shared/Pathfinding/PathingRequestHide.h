#pragma once

#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"

// A request for a place to hide from ThreatLocation: at least fMinHideDistance away from it.
class PathingRequestHide : public PathingRequest {
public:
	PathingRequestHide();
	PathingRequestHide(const PathingRequestHide& aRequest);
	~PathingRequestHide() override;

	void CopyTo(PathingRequest* apDest) const override;
	uint32_t GetType() const override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	void SetMinHideDistance(float afDistance);
	void SetThreatLocation(const PathingLocation& aThreatLocation);
	const PathingLocation& GetThreatLocation() const;

	PathingLocation ThreatLocation;
	float fMinHideDistance;
};
