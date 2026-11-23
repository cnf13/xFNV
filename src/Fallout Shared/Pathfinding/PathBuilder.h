#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class Actor;
class NavMeshInfo;
class PathingRequest;
class PathingSolution;
class VirtualPathingNode;

// Defined with its constants in PathingSearchRayCast.h.
enum RAYCAST_REQUEST_TYPE : int32_t;
// Defined with its constants in PathManager.h.
enum RAYCASTPATH_RESULT : int32_t;

bool IsEqual(uint8_t* apA, uint8_t* apB);

// A queued ray cast along the navmesh from a start to a goal location: the request that started it and the edges
// it crossed.
struct RayCastPathRequest {
    RayCastPathRequest();
    ~RayCastPathRequest();

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    RAYCAST_REQUEST_TYPE eType;
    PathingRequest* pInitialPathRequest;
    PathingLocation StartLocation;
    PathingLocation GoalLocation;
    BSSimpleArray<NavMeshEdgeLocation, 1024> Edges;
};

// The line-of-sight test TryBuildStraightPath uses: no water, no avoid-node or closed-door triangles, and no triangles
// closed to large creatures when bIsLargeCreature is set.
struct CheckLineOfSightFilterWater : public Pathing::CheckLineOfSightFilter {
    CheckLineOfSightFilterWater();

    bool CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) override;

    bool bIsLargeCreature;
};

class PathBuilder {
public:
    // What AddNodesFromNavMesh managed.
    enum ADD_NODES_FROM_NAVMESH_RESULT : int32_t {
        ADD_NODES_FROM_NAVMESH_RESULT_OK = 0,
        ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE = 1,
        ADD_NODES_FROM_NAVMESH_RESULT_FAILED = 2,
    };

    // A run of consecutive virtual nodes that share a navmesh.
    struct VirtualNodeSegment {
        int32_t iFirstVirtualNode;
        int32_t iLastVirtualNode;
        bool bHasNavMeshes;
    };

    // The triangle test the path's start and goal locations are resolved with.
    struct FindTriangleForLocationWaterFilter : public FindTriangleForLocationFilter {
        FindTriangleForLocationWaterFilter();

        // Any triangle will do.
        bool IsValidTri(
            const NiPoint3& aLocation,
            const NavMesh* apNavMesh,
            uint16_t aiTri,
            const NiPoint3& aPointOnTri
        ) override;
    };

    PathBuilder(PathingRequest* apRequest, PathingSolution* apSolution);
    ~PathBuilder();

    // Updates the detailed path from the virtual node aiCurrentVirtualNodeIndex and the
    // actor's location.
    Pathing::UPDATE_DETAILED_PATH_RESULT
    UpdateDetailedPath(uint32_t aiCurrentVirtualNodeIndex, const NiPoint3& aCurrentActorLocation);
    // Moves the path to the low process.
    Pathing::UPDATE_DETAILED_PATH_RESULT UpdatePathMoveToLow();
    // Moves the path back to the high process from the virtual node aiCurrentVirtualNodeIndex.
    Pathing::UPDATE_DETAILED_PATH_RESULT UpdatePathMoveToHigh(uint32_t aiCurrentVirtualNodeIndex);
    // Resolves the request's origin and destination onto navmeshes, builds the high-level path unless the solution
    // already has virtual nodes, then the detailed path.
    bool BuildPath();
    RAYCASTPATH_RESULT BuildPathFromNavMeshSearchGoal(
        uint32_t aiLastVirtualNodeIndex,
        BSSimpleArray<PathingLocation, 1024>& aRetPath,
        BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
    );
    RAYCASTPATH_RESULT BuildStraightPathUsingRayCasts(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath
    );
    bool BuildNavMeshPathForVirtualNodes(uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex);
    PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT AddNodesFromNavMesh(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        uint32_t aiFirstVirtualNodeIndex,
        uint32_t aiLastVirtualNodeIndex
    );

protected:
    // Fills the solution's virtual nodes from the start mesh to the goal mesh; tries the door path when the meshes
    // do not connect.
    bool BuildHighLevelPath(const NavMeshInfo* apStartMesh, const NavMeshInfo* apGoalMesh);
    // The virtual nodes over the navmesh infos from apStartMesh to apGoalMesh.
    bool BuildNavMeshInfoPath(const NavMeshInfo* apStartMesh, const NavMeshInfo* apGoalMesh);
    // The virtual nodes through teleport doors.
    bool BuildTeleportDoorPath();
    bool BuildDetailedPath(Actor* pActor, uint32_t aiFirstHighLevelNode);
    // Whether the actor can walk straight from aStartLocation to aGoalLocation: a clear line of sight, and when both
    // ends are above the water, two more parallel to it, offset by the actor's radius to each side. Adds the start
    // node on success.
    bool TryBuildStraightPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation);
    // Searches the navmeshes around aStartLocation for the nearest open triangle edge and puts it in aRetLocation.
    bool FindClosestOpenEdge(PathingLocation aStartLocation, PathingLocation& aRetLocation, bool abStart);
    // The navmesh search's entry point from the virtual node aiFirstVirtualNodeIndex: adds it to aRetPath and
    // returns RCP_SUCCESS, or RCP_FAILED when the node has no real location.
    RAYCASTPATH_RESULT BuildPathToNavMeshSearchStart(
        uint32_t aiFirstVirtualNodeIndex,
        BSSimpleArray<PathingLocation, 1024>& aRetPath,
        BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
    );
    // The location where the path enters the navmesh between two virtual nodes.
    bool
    FindEntryPoint(VirtualPathingNode* apPrevNode, VirtualPathingNode* apCurrentNode, PathingLocation& aRetLocation);
    // Continues the path from aStartLocation to the closest navmesh triangle. A KEEP
    // body (PathBuilder.cpp says why).
    RAYCASTPATH_RESULT BuildPathToValidNavMeshTriangle(
        PathingLocation& aStartLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath,
        bool abStart,
        BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
    );
    // Whether the first of aEndEdges lies within the target radius of virtual node aiLastVirtualNodeIndex in x and
    // y, and from 64 below to 180 above it; if so, aEndNodes is replaced by that point, moved a tenth of the way to
    // its triangle's centre.
    bool CheckClosestEdgeToGoal(
        uint32_t aiLastVirtualNodeIndex,
        const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEndEdges,
        BSSimpleArray<PathingLocation, 1024>& aEndNodes
    );

public:
    bool FindLoadedNodeIndices(
        uint32_t aiFirstHighLevelNode,
        int32_t* apFirstLoadedNodeIndex,
        int32_t* apLastLoadedNodeIndex
    );
    bool FindVirtualNodeSegments(BSSimpleArray<PathBuilder::VirtualNodeSegment, 1024>& aRetSegmentArray);
    bool BuildDetailedPathForLoadedVirtualNodes(Actor* pActor);
    // Runs a NavMeshSearch from the start location's triangle to the goal location's, avoiding aBadNodes and staying
    // in DetailedPathInfos; false when either location has no triangle or no path is found. The virtual node indices
    // are unused.
    bool RunNavMeshSearch(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        const BSSimpleArray<NavMeshSearchLink, 1024>* aBadNodes,
        BSSimpleArray<NavMeshSearchNode, 1024>& aNodes,
        BSSimpleArray<NavMeshSearchLink, 1024>& aLinks,
        uint32_t aiFirstVirtualNodeIndex,
        uint32_t aiLastVirtualNodeIndex
    );
    bool RunPathSmoother(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        BSSimpleArray<NavMeshSearchLink, 1024>& aBadNodes,
        const BSSimpleArray<NavMeshSearchNode, 1024>& aNodes,
        const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks
    );
    bool
    FindExitPoint(VirtualPathingNode* apCurrentNode, VirtualPathingNode* apNextNode, PathingLocation& aRetLocation);
    bool BuildNavMeshInfoPathInSpace(
        const NiPoint3& aStartLocation,
        const NiPoint3& aGoalLocation,
        ParentSpaceNode* pSpace,
        TESObjectREFR* apTeleportDoor
    );
    bool BuildStraightPathForVirtualNodes(uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex);

    NiPointer<PathingRequest> pRequest;
    NiPointer<PathingSolution> pSolution;
    ScrapHeap* pScrapHeap;
    FindTriangleForLocationWaterFilter WaterFilter;
    bool bPathUpdate;
    BSSimpleArray<const NavMeshInfo*> DetailedPathInfos;
    BSSimpleArray<NavMeshPtr> DetailedPathMeshes;
};

// A ray cast path the pathing thread fills: the request type, the result and the path's locations.
struct RayCastPath {
    RayCastPath();
    ~RayCastPath();

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    RAYCAST_REQUEST_TYPE eType;
    RAYCASTPATH_RESULT eResult;
    BSSimpleArray<PathingLocation, 1024> Nodes;
};

// The game settings PathBuilder.cpp defines.
extern SettingT<INISettingCollection> bUseStraightLineCheckFirst;
extern SettingT<INISettingCollection> bUseObstacleAvoidance;
extern SettingT<INISettingCollection> bUsePathSmoothing;
extern SettingT<INISettingCollection> bUseRayCasts;
extern SettingT<INISettingCollection> bRebuildPathIfSmootherFailed;
extern SettingT<INISettingCollection> bUseOldPathSmoothing;
extern SettingT<INISettingCollection> bUseAlternateSmoothingForPrime;
extern SettingT<INISettingCollection> bCreateDebugInfo;
