#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Reconstruction.hpp"

class TESObjectREFR;

// A request for a point near the destination that can be reached on a straight line without danger, away from pTarget.
// The slots GetType, CopyTo, SaveGame, LoadGame and InitLoadGame are overridden in the exe; the last two keep pTarget
// as a form ID in a save.
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

	// Meant to set pTarget to apTarget, but the exe's body copies pTarget onto itself and
	// ignores apTarget (thiscall, ret 4, 0x6E60C0).
	void SetTarget(TESObjectREFR* apTarget);

	TESObjectREFR* pTarget;
};
inline constexpr fnvd::Vtable PathingRequestSafeStraightLine_vftable{};
