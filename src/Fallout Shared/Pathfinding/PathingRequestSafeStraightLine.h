#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"

class TESObjectREFR;

// A request for a point near the destination that can be reached on a straight line without danger, away from pTarget.
// GetType, CopyTo, SaveGame, LoadGame and InitLoadGame are overridden; the last two keep pTarget as a form ID in a
// save.
class PathingRequestSafeStraightLine : public PathingRequestClosePoint {
public:
	PathingRequestSafeStraightLine();
	// From Construct_006E6010.
	PathingRequestSafeStraightLine(const PathingRequestSafeStraightLine& aRequest);
	~PathingRequestSafeStraightLine() override;

	void CopyTo(PathingRequest* apDest) const override;
	uint32_t GetType() const override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// Meant to set pTarget to apTarget, but copies pTarget onto itself and ignores apTarget, as the game does.
	void SetTarget(TESObjectREFR* apTarget);

	TESObjectREFR* pTarget;
};
