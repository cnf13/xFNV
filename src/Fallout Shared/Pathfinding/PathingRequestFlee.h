#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"

// A request for a path away from the destination: at least fMinFleeDistance from it. GetType, CopyTo, SaveGame and
// LoadGame are overridden.
class PathingRequestFlee : public PathingRequest {
public:
	PathingRequestFlee();
	// From Construct_006E5400.
	PathingRequestFlee(const PathingRequestFlee& aRequest);
	~PathingRequestFlee() override;

	void CopyTo(PathingRequest* apDest) const override;
	uint32_t GetType() const override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	void SetMinFleeDistance(float afDistance);
	float GetMinFleeDistance() const { return fMinFleeDistance; }

	float fMinFleeDistance;
};
