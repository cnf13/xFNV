#pragma once

#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Reconstruction.hpp"

// A request for a place to hide from ThreatLocation: at least fMinHideDistance away from it.
// The slots GetType, CopyTo, SaveGame and LoadGame are overridden in the exe.
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
	// ThreatLocation; the exe's shared getter of the field at +0xB0 (thiscall, ret 0,
	// 0x984230).
	const PathingLocation& GetThreatLocation() const;

	PathingLocation ThreatLocation;
	float fMinHideDistance;
};
inline constexpr fnvd::Vtable PathingRequestHide_vftable{};
