#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class NavMeshInfo;

// A place to take cover: the two ends of the cover edge on the navmesh and how much of the cover it gives.
// The Xbox PDB's source module for this class is PathingRequestCover.cpp; the header is split out so that it does
// not depend on PathingRequestCover.
class PathingCoverLocation : public PathingLocation {
public:
	PathingCoverLocation();
	PathingCoverLocation(const NiPoint3& aLocation, const NavMeshInfo* apNavMeshInfo, uint16_t ausTriangleIndex);
	PathingCoverLocation(const PathingCoverLocation& aLoc);

	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) override;
	~PathingCoverLocation() override;

	bool IsLedgeCover() const;
	void SetEdgeOrientation(const NiPoint3& aLeftVertex, const NiPoint3& aRightVertex, const NiPoint3& aDirection);
	void SetCoverData(uint16_t aiCoverVal, uint16_t aiStandVal, uint16_t aiCrouchVal, bool abLeftOpen, bool abRightOpen,
		uint8_t acEdge);
	uint8_t GetHeight();
	// Out of line in the exe (0x6E4430); src has no other assignments of the class.
	PathingCoverLocation& operator=(const PathingCoverLocation& aLoc);
	bool IsLeftSideClosest(const NiPoint3& arPoint);
	// Whether the cover is stand cover, bStandCover.
	bool IsStandCover();
	uint32_t GetKey();
	bool CheckDynamicCoverValid(uint32_t aiLastMovedTime);
	uint32_t GetDynamicCoverObstacleTimeStamp();

	static uint32_t ComputeKey(uint32_t aiNavMeshID, uint16_t aiTriangle, uint8_t aiEdge);

	NiPoint3 LeftVertex;
	NiPoint3 RightVertex;
	NiPoint3 EdgeDirection;
	uint8_t cCoverHeight;
	bool bStandCover;
	bool bHigherThanCrouch;
	bool bLeftEdgeOpen;
	bool bRightEdgeOpen;
	uint8_t cEdge;
	uint8_t cState;
};
inline constexpr fnvd::Vtable PathingCoverLocation_vftable{};
