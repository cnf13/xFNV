#pragma once

#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// A request for cover from a threat: how far to look, how the candidates are ordered and what height the actor
// crouches to. The slots GetType and CopyTo are overridden in the exe; SaveGame and LoadGame are PathingRequest's.
// The IDA names kThreatPos, uiMaxResults and kActorPos are the PDB's Threat, iMaxResults and ActorLocation.
class PathingRequestCover : public PathingRequest {
public:
	PathingRequestCover();
	~PathingRequestCover() override;

	void CopyTo(PathingRequest* apDest) const override;
	uint32_t GetType() const override;

	bool GetSortFarthestFirst();
	// bFindBothSides.
	bool GetFindBothSides();
	// bFindBehindTarget.
	bool GetFindBehindTarget();
	// bCanCrouch (thiscall, ret 0, 0x6D6DB0); const as in the PDB.
	bool GetCanCrouch() const;
	// bHideCover.
	bool GetHideCover();
	void SetThreat(const NiPoint3& a2);
	NiPoint3 GetThreat();
	// Same function as bhkRigidBodyCinfo::SetFriction (0x5076C0; thiscall, ret 4): stores
	// fActorCrouchHeight.
	void SetCrouchHeight(float afCrouch);
	void SetMaxCoverDistance(float afDistance);
	void SetMaxHeightChange(float afHeightChange);
	// Same function as Explosion::SetOwner (0x4030D0; thiscall, ret 4): stores iMaxResults.
	void SetMaxResults(uint32_t aiResults);
	void SetDistanceProjectionVector(const NiPoint3& arVector);
	void SetFindBothSides(bool abValue);
	void SetFindBehindTarget(bool abValue);
	void SetSortFarthestFirst(bool abValue);
	void SetSortDistanceFromTarget(bool abValue);
	void SetHideCover(bool abValue);
	// Copies the point into ActorLocation.
	void SetActorPos(const NiPoint3& arActorPos);

	NiPoint3 Threat;
	float fMaxCoverDistance;
	float fActorCrouchHeight;
	float fMaxHeightChange;
	uint32_t iMaxResults;
	bool bCanCrouch;
	bool bFindBothSides;
	bool bFindBehindTarget;
	bool bSortFarthestFirst;
	bool bSortDistanceFromTarget;
	bool bHideCover;
	uint8_t paddingD2[2];
	NiPoint3 DistanceProjectionVector;
	NiPoint3 ActorLocation;
};
inline constexpr fnvd::Vtable PathingRequestCover_vftable{};
