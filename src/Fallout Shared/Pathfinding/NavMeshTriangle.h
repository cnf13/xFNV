#pragma once

#include <cstdint>

// A navmesh triangle: its three vertex indices, the indices of the triangles across its three edges, and its flags.
struct NavMeshTriangle {
	// The TriangleFlags bits.
	enum Flags : uint32_t {
		NAV_TRI_LARGE_ACTOR = 0x10,
		NAV_TRI_DISABLED = 0x20,
		NAV_TRI_PREFERRD = 0x40,
		NAV_TRI_WATER = 0x200,
		NAV_TRI_DOOR = 0x400,
		NAV_TRI_CLOSED_DOOR = 0x1000,
		NAV_TRI_AVOID_NODE = 0x80000000,
	};

	NavMeshTriangle();

	// Whether the triangle is water.
	bool IsWater() const;
	uint16_t GetVert(uint32_t aiVertIndex) const;
	bool GetFlag(int32_t aFlag) const;
	void SetFlag(int32_t aFlag);
	void ClearFlag(int32_t aFlag);
	// Whether any of the three edges has extra info (TriangleFlags bits 0-2).
	bool HasEdgeExtraInfo();
	// Whether edge aiEdgeIndex has extra info.
	bool EdgeHasExtraInfo(int32_t aiEdgeIndex);
	// Triangles[aiEdgeIndex]: the adjacent triangle or the extra info index.
	uint16_t GetTriOrExtraInfo(int32_t aiEdgeIndex);
	// Clears the extra information for edge aiEdgeIndex.
	void ClearEdgeExtraInfo(int32_t aiEdgeIndex);
	// Whether NAV_TRI_PREFERRD is set.
	bool IsPreferred() const;
	// GetFlag(NAV_TRI_LARGE_ACTOR).
	bool IsNoLargeCreature() const;
	// Flips NAV_TRI_LARGE_ACTOR.
	void ToggleNoLargeCreature();
	// Stores aiCount, clamped to 0-7, in TriangleFlags bits 13-15.
	void SetObjectCount(uint32_t aiCount);
	// Byte swaps the flags and the six indices.
	void Endian();
	// Whether any cover flag is set.
	bool HasCover() const;
	bool HasDoorPortal() const;
	bool HasCoverOrWallFlags() const { return 0 < (TriangleFlags & 0xFFF0000); }
	uint16_t GetObjectCount() const { return (TriangleFlags & 0xE000) >> 0xD; }
	bool IsEdgeConnected(int32_t aiEdgeIndex) const;
	void SetTri(uint32_t aiEdgeIndex, uint16_t aiTriangle);
	void SetVert(uint32_t aiVertIndex, uint16_t aiVertex);
	void SetEdgeExtraInfo(int32_t aiEdgeIndex, uint16_t ausExtraInfoIndex);
	// Stores the three vertex indices.
	void SetVerts(uint16_t i0, uint16_t i1, uint16_t i2);
	// Stores edge aiEdge's cover data (clamped to 15) and sides in TriangleFlags; edge 2 has none.
	void SetEdgeCover(uint16_t aiEdge, uint16_t aiData, bool abLeftSide, bool abRightSide);
	uint16_t GetEdgeCover(uint16_t aiEdge) const;
	void GetVerts(uint16_t& i0, uint16_t& i1, uint16_t& i2) const;
	void GetSides(uint16_t* a2, uint16_t* a3, uint16_t* a4);
	void GetEdgeCoverData(uint16_t aiTriangleFlags, uint16_t& aiEdge, bool& aiData, bool& abRight) const;

	static float GetCoverHeight(uint32_t aiVal);
	// The cover level (0 to 15) of a height.
	static uint32_t GetCoverLevel(float afHeight, bool abRoundDown, bool abRoundClosest);

	uint16_t Vertices[3];
	uint16_t Triangles[3];
	uint32_t TriangleFlags;
};

// The cover heights' steps, in the current and in the old format. The computed values are statics in every file that
// includes the header: one copy per file.
const float NAVMESH_COVER_HEIGHT_GRANULARITY = 16.0f;
const float OLD_NAVMESH_COVER_HEIGHT_GRANULARITY = 32.0f;
static const float NAVMESH_COVER_HEIGHT_GRANULARITY_INVERSE = 1.0f / NAVMESH_COVER_HEIGHT_GRANULARITY;
static const float NAVMESH_COVER_MAX_HEIGHT = static_cast<float>(NAVMESH_COVER_HEIGHT_GRANULARITY * 15.0);
static const float OLD_NAVMESH_COVER_MAX_HEIGHT = static_cast<float>(OLD_NAVMESH_COVER_HEIGHT_GRANULARITY * 7.0);
