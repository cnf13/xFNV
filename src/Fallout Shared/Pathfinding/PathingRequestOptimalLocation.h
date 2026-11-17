#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"

// A request for a point whose distance to the destination lies between fMinDistance and fMaxDistance, and in any case
// between the two absolute limits.
class PathingRequestOptimalLocation : public PathingRequest {
public:
	PathingRequestOptimalLocation();
	// From Construct_006E5DB0.
	PathingRequestOptimalLocation(const PathingRequestOptimalLocation& aRequest);
	~PathingRequestOptimalLocation() override;

	void CopyTo(PathingRequest* apDest) const override;
	uint32_t GetType() const override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// Stores the actor's height in the field shared with PathingRequestCoveredMove's fAttackerHeight.
	void SetActorHeight(float afHeight);
	void SetMinDistance(float afDistance);
	void SetMaxDistance(float afDistance);
	void SetAbsoluteMaxDistance(float afDistance);

	float fMinDistance;
	float fMaxDistance;
	float fAbsoluteMinDistance;
	float fAbsoluteMaxDistance;
};
