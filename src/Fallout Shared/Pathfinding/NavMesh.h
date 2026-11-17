#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshGrid.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <stdint.h>

struct EdgeExtraInfo;
class NavMesh;
struct NavMeshEdgeHandle;
class NavMeshInfo;
class NavMeshPtr;
class NavMeshStaticAvoidNode;
class ObstacleUndoData;
struct TaskPortalSwap;
class TESObjectCELL;

// A connection across a triangle edge to triangle sTriangle of another navmesh.
struct NavMeshTriangleEdgePortal {
	// An empty portal: no other mesh, triangle 0xFFFF.
	NavMeshTriangleEdgePortal() : pOtherMesh(nullptr), sTriangle(0xFFFF) {}

	// Copies the mesh and the triangle; the padding bytes deliberately stay unset, as the game does.
	NavMeshTriangleEdgePortal(const NavMeshTriangleEdgePortal& aPortal)
		: pOtherMesh(aPortal.pOtherMesh), sTriangle(aPortal.sTriangle) {}

	// The bytes the portal takes in a save: a form ID and a triangle index.
	int32_t GetSaveSize();
	bool IsValid() const;
	// Reads the other navmesh's form ID and the triangle index from a chunk buffer. apOwner is unused.
	void Load(TESForm* apOwner, TESFile* apFile, char* pBuffer);

	NavMeshInfo* pOtherMesh;
	uint16_t sTriangle;
};

// What an edge's extra info describes.
enum EDGE_EXTRA_INFO_TYPE : int32_t {
	EIT_INVALID = -1,
	EIT_PORTAL = 0,
	EIT_LEDGE_UP = 1,
	EIT_LEDGE_DOWN = 2,
	EIT_ENABLE_DISABLE_PORTAL = 3,
};

// The extra data of a triangle edge: its type and the portal it leads through.
struct EdgeExtraInfo {
	// eType EIT_INVALID and an empty portal.
	EdgeExtraInfo() : eType(EIT_INVALID), Portal() {}
	EdgeExtraInfo(const EdgeExtraInfo& aInfo) : eType(aInfo.eType), Portal(aInfo.Portal) {}

	// Reads the info from a version 9+ NVEX buffer.
	void SetFromBuffer(TESFile* apFile, char* apChunkBuffer);
	// Reads the info from a version 5-8 NVEX buffer: eType 0, then the portal.
	void SetFromBufferOld(TESFile* apFile, char* apChunkBuffer);
	// The bytes SetFromBuffer reads: the portal's size plus 4.
	int32_t GetSaveSize();
	// The bytes SetFromBufferOld reads: the portal's size.
	int32_t GetSaveSizeOld();

	static uint32_t GetSizeOfV0();

	EDGE_EXTRA_INFO_TYPE eType;
	NavMeshTriangleEdgePortal Portal;
};

// A triangle edge as its two vertex indices.
struct NavMeshEdge {
	uint16_t Vertices[2];
};

// A triangle edge as pointers to its two vertices.
struct NavMeshEdgeVertices {
	NavMeshVertex* Vertices[2];
};

// A door that closes off a triangle.
struct NavMeshClosedDoorInfo {
	// DoorFormID 0 and iTriangleIndex 0xFFFF.
	NavMeshClosedDoorInfo() : DoorFormID(0), iTriangleIndex(0xFFFF) {}

	NavMeshClosedDoorInfo(const NavMeshClosedDoorInfo& aInfo)
		: DoorFormID(aInfo.DoorFormID), iTriangleIndex(aInfo.iTriangleIndex) {}

	uint32_t DoorFormID;
	uint16_t iTriangleIndex;
};

// A door a triangle teleports through.
struct NavMeshTriangleDoorPortal {
	// Default constructor: no door, no triangle.
	NavMeshTriangleDoorPortal() : pDoorForm(nullptr), iOwningTriangleIndex(0xFFFF) {}

	NavMeshTriangleDoorPortal(const NavMeshTriangleDoorPortal& aPortal)
		: pDoorForm(aPortal.pDoorForm), iOwningTriangleIndex(aPortal.iOwningTriangleIndex) {}

	// Byte swaps both fields.
	void Endian();

	TESObjectREFR* pDoorForm;
	uint16_t iOwningTriangleIndex;
};

// One edge of a point-of-visibility.
struct NavMeshPOVEdge {
	void Set(const FastNavMeshEdgeHandle& aEdge);
	// The triangle: the low 14 bits of EdgeIndex. BuildPOVListForTriangles calls it on both edges of a NavMeshPOVData.
	int32_t QTriangle();
	// The edge within the triangle: the top 2 bits of EdgeIndex.
	int32_t QEdge();

	uint16_t EdgeIndex;
};

// A vertex's point of visibility: the two edges that meet there.
struct NavMeshPOVData {
	NavMeshPOVData() = default;
	// NiTMap::DeleteItem clears an item by assigning 0 to it, which writes nothing on the item's own value, so
	// constructing from or assigning 0 writes nothing either.
	NavMeshPOVData(int32_t) {}

	NavMeshPOVData& operator=(int32_t) { return *this; }

	NavMeshPOVEdge Edge1;
	NavMeshPOVEdge Edge2;
};

// The test FindTriangleForLocation applies to each triangle the location lies in.
struct FindTriangleForLocationFilter {
	FindTriangleForLocationFilter() {}

	// Whether triangle ausTriangle of apNavMesh is acceptable for aLocation, aTriLocation being the location moved
	// onto the triangle.
	virtual bool IsValidTri(const NiPoint3& aLocation, const NavMesh* apNavMesh, uint16_t ausTriangle,
		const NiPoint3& aTriLocation) = 0;
};

// A cell's navigation mesh (NAVM): its vertices, triangles, door portals and cover. Reference counted through its
// NiRefObject part (at +0x1C), which NavMeshPtr holds.
class NavMesh : public TESForm, public TESChildCell, public NiRefObject {
public:
	// How a segment leaves a triangle.
	enum FIND_INTERSECTION_EDGE : int32_t {
		FIE_NONE = 0,
		FIE_CORNER = 1,
		FIE_EDGE = 2,
		FIE_INTERSECTION = 3,
	};

	// The edge a segment crosses and where.
	struct FIEResult {
		// Leaves every field unset, as the game does.
		FIEResult() {}

		FIND_INTERSECTION_EDGE eType;
		int32_t iEdge;
		NiPoint3 Intersection;
	};

	// An empty NAVM form: no parent cell, no info record, an empty obstacle map; the 37-bucket POV map.
	NavMesh();
	~NavMesh() override;

	bool ProcessBeforeSave() override { return false; }
	bool Load(TESFile* apFile) override;
	bool SavesBefore(TESForm*) override { return false; }
	bool SavesBefore(FORM*) override { return false; }
	void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void GetFormDetailedString(BSStringT<char>& aString) override;
	void SetDelete(bool abDeleted) override;
	void SetAltered(bool abVal) override;
	bool BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) override;
	void CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) override;
	TESObjectCELL* GetSaveParentCell() override;

	// The matching edge on the other side, by triangle and edge index.
	bool GetMatchingEdge(uint16_t ausTriangleIndex, int32_t aiEdgeIndex, NavMesh** arpReturnNavMesh,
		uint16_t* arOtherTriangleIndex, int32_t* arOtherEdgeIndex);
	// The normal of triangle ausTriangle, written to arNormal.
	NiPoint3* GetNormal(NiPoint3* arNormal, uint16_t ausTriangle);
	// Reconnects the enabled portals.
	bool ReconnectEnabledPortals();
	// The index of the undo data of apObstacle, or -1.
	uint32_t GetUndoDataIndexForObstacle(const NiPointer<ObstacleData>& apObstacle);
	// As above, and gives the undo data in apOutUndoData.
	uint32_t GetUndoDataIndexForObstacle(const NiPointer<ObstacleData>& apObstacle, ObstacleUndoData** apOutUndoData);
	// Removes the obstacle with the undo data aiUndoIndex, collecting the swapped edges in
	// aPortalSwaps.
	bool RemoveObstacle(uint32_t aiUndoIndex, TaskPortalSwap& aPortalSwaps);
	// Removes apObstacle: finds its undo data index, returns false when it has none, else removes it by that index.
	bool RemoveObstacle(const NiPointer<ObstacleData>& apObstacle, TaskPortalSwap& aPortalSwaps);
	TESObjectCELL* GetParentCell();
	// Appends a copy of aTriangle to Triangles and returns its index.
	uint16_t AddTriangle(const NavMeshTriangle& aTriangle);

	NavMeshInfo* GetNavMeshInfo();
	// Empties the navmesh.
	void ClearNavMesh(bool abCheckout);
	// Builds the points of visibility.
	void CreatePOVs();
	TESForm* GetParentSpace();
	// Checks the navmesh for errors, fixing them when abFix.
	bool CheckNavMesh(bool abFix);
	// Removes extra edge info aiExtraInfo and renumbers the triangles' references to the
	// infos after it.
	void DeleteExtraEdgeInfo(uint16_t aiExtraInfo);
	// Lists the triangles that give cover and are not disabled in CoverArray.
	void CreateCoverArray();
	// Rebuilds MeshGrid from the triangles.
	void CreateGrid();
	// The edge of triangle ausTri other than aIgnoreEdge that the segment from aStartPoint to
	// aEndPoint crosses, else -1.
	int32_t FindIntersectingEdgeFast(uint16_t ausTri, int32_t aIgnoreEdge, const NiPoint3& aStartPoint,
		const NiPoint3& aEndPoint);
	// Creates the navmesh's info record, or refreshes the one it has; true when it created one.
	bool UpdateNavMeshInfo();
	EdgeExtraInfo* GetExtraEdgeInfo(uint16_t asExtraInfoIndex);
	// The edge another navmesh joins to aThisEdge through a portal, into apRetOtherEdge; false
	// when there is none.
	bool GetMatchingEdge(const NavMeshEdgeHandle& aThisEdge, NavMeshEdgeHandle* apRetOtherEdge);
	// The extra info of edge aiEdgeIndex of triangle ausTriangleIndex, else null.
	EdgeExtraInfo* GetEdgeExtraInfo(uint16_t ausTriangleIndex, int32_t aiEdgeIndex);
	// Appends a copy of aExtraInfo to ExtraEdgeInfo; its index.
	uint16_t AddExtraEdgeInfo(const EdgeExtraInfo& aExtraInfo);
	// Appends a copy of aVertex to Vertices; its index.
	uint16_t AddVertex(const NavMeshVertex& aVertex);
	// The matching edge on the other side of aThisEdge, by triangle and edge index, into apRetOtherEdge.
	bool GetMatchingEdge(const FastNavMeshEdgeHandle& aThisEdge, FastNavMeshEdgeHandle* apRetOtherEdge);
	// Door portal aiDoorPortalIndex of DoorPortals.
	NavMeshTriangleDoorPortal* GetDoorPortalFromArray(uint16_t aiDoorPortalIndex);
	// The number of cells in MeshGrid: its grid size squared.
	uint32_t GetGridCellCount();
	// Adds triangles ausFirstTriangleIndex..ausLastTriangleIndex to MeshGrid;
	// always true.
	bool AddTrianglesToGrid(uint16_t ausFirstTriangleIndex, uint16_t ausLastTriangleIndex);
	// Removes triangles ausFirstTriangleIndex..ausLastTriangleIndex from MeshGrid;
	// always true.
	bool RemoveTrianglesFromGrid(uint16_t ausFirstTriangleIndex, uint16_t ausLastTriangleIndex);
	// Removes undo data aiUndoDataIndex from Obstacles, moving the last one into its place.
	void RemoveObstacleUndoData(uint32_t aiUndoDataIndex);
	// The obstacle cutting triangle ausTriangle, into spRetObstacle; false when there is none.
	bool GetObstacleForTriangle(uint16_t ausTriangle, NiPointer<ObstacleData>& spRetObstacle);
	// Static avoid node aiIndex.
	NavMeshStaticAvoidNode* GetStaticAvoidNode(uint32_t aiIndex);
	// Adds a static avoid node of afRadius and afCost at aLocation on triangle ausTri and flags the triangle as an
	// avoid node.
	void AddStaticAvoidNode(const NiPoint3& aLocation, uint16_t ausTri, float afRadius, float afCost);
	void SetNavMeshInfo(NavMeshInfo* apInfo);
	// Checks the unconnected border edges against the navmeshes of the four neighbouring exterior cells; true for
	// an interior cell.
	bool CheckNavMeshAgainstNeighbors();
	// The number of triangles.
	uint32_t GetTriangleCount();
	// The number of extra edge infos.
	uint32_t GetExtraEdgeInfoCount();
	NavMeshVertex* GetVertex(uint16_t asVertexIndex);
	NavMeshTriangle* GetTriangle(uint16_t asTriangleIndex);
	// Edge aiEdgeIndex of triangle ausTriIndex: its vertex aiEdgeIndex and the next one.
	NavMeshEdge GetEdge(uint16_t ausTriIndex, int32_t aiEdgeIndex);
	NavMeshEdgeVertices GetEdgeVertices(uint16_t ausTriIndex, int32_t aiEdgeIndex);
	// Copies the two vertices of edge aiEdgeIndex of triangle ausTriIndex, its vertex aiEdgeIndex and the next one;
	// always true.
	bool GetEdgeVertices(uint16_t ausTriIndex, int32_t aiEdgeIndex, NiPoint3* apRetVert1, NiPoint3* apRetVert2);
	// The grid's bounds; always true.
	bool GetBounds(NiPoint3& aRetMin, NiPoint3& aRetMax);
	// The average of the triangle's three vertices.
	NiPoint3 GetCenter(uint16_t apTriangle);
	// The centre of the triangle's circumcircle in x and y, at the height of its highest vertex.
	NiPoint3 GetCircumCenter(uint16_t apTriangle);
	// The enabled triangle under or over aLocation that is closest in height and that apFilter accepts, else 0xFFFF.
	uint16_t FindTriangleForLocation(const NiPoint3& aLocation, FindTriangleForLocationFilter* apFilter);
	// The triangle aLocation lies in, else the one whose centre is closest; apbOnTriangle tells which.
	bool FindClosestTriangleForLocation(const NiPoint3& aLocation, uint16_t* apRetTriIndex, bool* apbOnTriangle);
	// Whether aLocation lies over or under the triangle within the given heights; afZDiff gets its height above it.
	bool IsPointInsideTriangle(uint16_t asTriangleIndex, const NiPoint3& aLocation, float& afZDiff,
		float afMaxZdistAbove, float afMaxZdistBelow);
	// IsPointInsideTriangle at any height, then apFilter's verdict when there is a filter.
	bool IsPointInsideTriangleFilter(uint16_t asTriangleIndex, const NiPoint3& aLocation, float& afZDiff,
		FindTriangleForLocationFilter* apFilter);
	// The closed door entry for triangle aiTriangle, else null.
	NavMeshClosedDoorInfo* GetClosedDoorInfoForTriangle(uint16_t aiTriangle);
	// Collects the triangles apObstacle's box, grown by afFudge, overlaps.
	void GetOverlappedTriangleList(const NiPointer<ObstacleData>& apObstacle, const NiPoint3& afFudge,
		BSSimpleArray<uint16_t>* apRetTriangles);
	// Marks the triangles door aiDoorID's obstacle apData covers as closed doors, recording each one not yet recorded
	// for that door; always true.
	bool AddClosedDoor(uint32_t aiDoorID, ObstacleData* apData);
	void ForceSetFormID(uint32_t aiID);
	bool IsPointInsideNavmesh(const NiPoint3& aPoint, FindTriangleForLocationFilter* apFilter);
	uint32_t GetVertexCount() const;
	uint32_t GetNumDoorPortals();
	uint32_t GetStaticAvoidNodeCount();
	void ComputeBounds(NiPoint3* apRetMin, NiPoint3* apRetMax);
	EdgeExtraInfo* SetEmptyEdgeExtraInfo(uint16_t aTriangle, int32_t aEdgeIndex);
	void MakeCopyForObstacle(NavMeshPtr& spDestNavMesh) const;
	// The body only reads the point.
	bool IsPointInsideTriangle_0(uint16_t a1, const NiPoint3* a2, float a3, float a4, float* a5);
	uint32_t AddObstacleUndoData(ObstacleUndoData* apUndoData);
	void GetTriangleVertices(uint16_t ausTriIndex, const NavMeshVertex*& apV0, const NavMeshVertex*& apV1,
		const NavMeshVertex*& apV2);
	bool GetMatchingEdge_0(uint16_t ausTriangleIndex, int32_t aiEdgeIndex, NavMeshPtr* apReturnNavMesh,
		uint16_t* apOtherTriangleIndex, int32_t* apOtherEdgeIndex);
	const BSSimpleArray<uint16_t, 1024>* GetTrianglesForGridCell(uint32_t aiGridCellIndex);
	uint32_t GetGridIndicesForTriangle(uint16_t ausTriangleIndex, BSSimpleArray<uint32_t, 1024>* apGridCells);
	int32_t GetStaticAvoidNodesForTri(uint16_t ausTriangle, BSSimpleArray<NavMeshStaticAvoidNode*, 1024>& aRetArray);
	void FlipTriangle(uint16_t aiTriIndex);
	void GetTrianglesForAABB(const NiPoint3& aAABBMin, const NiPoint3& aAABBMax,
		BSSimpleArray<uint16_t, 1024>* apRetTriangles);
	bool FindClosestOpenEdgeForLocation(const NiPoint3& aLocation, float afRadius, uint16_t* apRetTriIndex,
		int32_t* apRetEdgeIndex, bool abFlipForStartNode);
	void GetCircumCenter2D(uint16_t apTriangle, NiPoint3& aOut);
	bool FindPortalTriangles(const NavMeshInfo* apNavMeshInfo, BSSimpleArray<int32_t, 1024>* apReturnArray);
	bool ComputeNavMeshZForLocation(const NiPoint3& aLocation, float afMaxZdistAbove, float afMaxZdistBelow,
		float& afZCoord);
	bool FindRandomPointWithinRadius(const NiPoint3& aStartLocation, float aRadius, NiPoint3* apRet1);
	NiPoint3* MoveAwayFromEdge(NiPoint3* apResult, const NiPoint3& aLocation, float afDistance);
	int32_t FindInterectingEdge(uint16_t ausTri, int32_t aIgnoreEdge, const NiPoint2& aStartPoint,
		const NiPoint2& aEndPoint, NiPoint2* apReturnIntersection);
	NavMesh::FIEResult* FindInterectingEdge2(NavMesh::FIEResult* apResult, uint16_t ausTri, int32_t aIgnoreEdge,
		const NiPoint3& aStartPoint, NiPoint3& aEndPoint);
	bool AddObstacle(const NiPointer<ObstacleData>& apObstacle, TaskPortalSwap& aPortalSwaps);
	void ClearAllObstacles();
	uint32_t GetUndoDataMaxObstacleCount(ObstacleUndoData* apUndoData);
	bool RemoveClosedDoor(uint32_t aiDoorID);
	uint32_t GetPOVsForTriangle(uint16_t ausTriangle, BSSimpleArray<uint16_t, 1024>& aRetPOVs);
	uint32_t GetObstaclePOVsForTriangle(uint16_t ausTriangle, BSSimpleArray<uint16_t, 1024>& aRetObstaclePOVs);
	bool GetPOVForVertex(uint16_t ausVertex, NavMeshPOVData& aRetData);
	char GetNeighborCount(uint16_t apTriangle);
	uint32_t GetGridIndex(const NiPoint3& arLoc);

	// The navmesh is freed through NiMemObject's sized delete; TESForm's ourFree one would be ambiguous with
	// NiRefObject's.
	static void operator delete(void* apMemory, size_t aSize) { NiMemObject::operator delete(apMemory, aSize); }
	// On PC this always returns true.
	static bool CheckUnconnectedBorderEdges(NavMesh* apMesh1, NavMesh* apMesh2, bool abVerticalEdge, float afEdgeX,
		float afEdgeY);
	static bool SetDisableNavMesh(uint32_t aiID, bool abDisable);
	static void FixPortalsFromNavMeshDeletion(NavMeshInfo* apNavmeshInfo);

	TESObjectCELL* pParentCell;
	BSSimpleArray<NavMeshVertex> Vertices;
	BSSimpleArray<NavMeshTriangle> Triangles;
	BSSimpleArray<EdgeExtraInfo> ExtraEdgeInfo;
	BSSimpleArray<NavMeshTriangleDoorPortal> DoorPortals;
	BSSimpleArray<NavMeshClosedDoorInfo> ClosedDoors;
	BSSimpleArray<uint16_t> CoverArray;
	NiTMap<uint16_t, NavMeshPOVData> POVs;
	BSSimpleArray<uint16_t> ClosestPOVs;
	NavMeshGrid MeshGrid;
	BSSimpleArray<NiPointer<ObstacleUndoData>> Obstacles;
	NiTMap<uint16_t, NiPointer<ObstacleData>>* pTriangleToObstacleMap;
	BSSimpleArray<uint16_t> ObstaclePOVs;
	BSSimpleArray<NavMeshStaticAvoidNode> StaticAvoidNodes;
	NavMeshInfo* pNavMeshInfo;

	static inline float DebugScale = 10.0f;
};
