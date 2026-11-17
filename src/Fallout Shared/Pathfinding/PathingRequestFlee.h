#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Reconstruction.hpp"

// A request for a path away from the destination: at least fMinFleeDistance from it.
// The slots GetType, CopyTo, SaveGame and LoadGame are overridden in the exe.
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

	// fMinFleeDistance; the exe's shared float setter at +0xB0 it folds with
	// PathingRequestClosePoint::SetMinDistance (thiscall, ret 4, 0x507610).
	void SetMinFleeDistance(float afDistance);
	float GetMinFleeDistance() const { return fMinFleeDistance; }

	float fMinFleeDistance;
};
inline constexpr fnvd::Vtable PathingRequestFlee_vftable{};
