#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class ActorLockData;
class NavMeshEdgeLocation;
class NavMeshInfo;
class PathingAvoidNodeArray;
class PathingCoverLocation;
class PathingLOSGridMap;
class PathingLOSMap;
class PathingRequest;
class PathingRequestClosePoint;
class PathingRequestCover;
class PathingRequestCoveredMove;
class PathingRequestFlee;
class PathingRequestHide;
class PathingRequestLOS;
class PathingRequestOptimalLocation;
class PathingRequestSafeStraightLine;
class PathingSolution;
class TESObjectCELL;
class TESWorldSpace;

float RandomFloatNegPiToPi();
int32_t EdgeSortFunction(const void* apEdge1, const void* apEdge2);

// A navmesh triangle a line of sight crosses, and how many of its neighbours it has (Pathing's line of sight map
// keeps them in a BSScrapArray).
struct CrossedTriangle {
    // The user-declared constructor would suppress the implicit default one, which locals and arrays use.
    CrossedTriangle() = default;
    CrossedTriangle(uint32_t aiTriangleIndex, char acTriangleNeighborCount);

    uint32_t iTriangleIndex;
    uint8_t iNeighborCount;
};

// static pathfinding utilities; every method of the class is static
class Pathing {
public:
    enum TELEPORT_DOOR_PATH_RESTRICTION {
        TELEPORT_DOOR_PATH_NONE = 0,
        TELEPORT_DOOR_PATH_START_SPACE_ONLY = 1,
        TELEPORT_DOOR_PATH_INTERIORS_ONLY = 2,
        TELEPORT_DOOR_PATH_FIND_FIRST_WORLDSPACE = 3,
    };

    enum DOOR_LOCK_DISPOSITION {
        DOOR_LOCK_DISPOSITION_NORMAL = 0,
        DOOR_LOCK_DISPOSITION_IGNORELOCKS = 1,
        DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE = 2,
        DOOR_LOCK_DISPOSITION_RADIO = 3,
    };

    // What UpdateSolution did to the detailed path.
    enum UPDATE_DETAILED_PATH_RESULT : int32_t {
        FAILED = -1,
        NO_CHANGE = 0,
        CLEARED_DETAILED_PATH = 1,
        CREATED_DETAILED_PATH = 2,
        UPDATED_DETAILED_PATH = 3,
    };

    // What FindClosestEdges found.
    enum FIND_CLOSEST_EDGES_RESULT : int32_t {
        FIND_CLOSEST_EDGES_NONE = 0,
        FIND_CLOSEST_EDGES_ONTRI = 1,
        FIND_CLOSEST_EDGES_OK = 2,
    };

    // Which edges a line-of-sight walk across the navmesh may cross.
    struct CheckLineOfSightFilter {
        CheckLineOfSightFilter();

        // Whether the walk may cross from hEdge into the matching edge of the neighbouring triangle.
        virtual bool CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) = 0;
    };

    // Times aiNum test paths of type aiTestType around aLoc in apCell.
    static float ProfilePathing(TESObjectCELL* apCell, const NiPoint3& aLoc, uint32_t aiTestType, uint32_t aiNum);
    // A path from arOrigin to arDestination over the door graph, ignoring locks according to aeDoorLockDisp.
    static bool BuildLowPath(
        PathingLocation& aOrigin,
        PathingLocation& aDestination,
        TeleportPath* apRetPath,
        ActorLockData* apActorLockData,
        DOOR_LOCK_DISPOSITION aeDoorLockDisp
    );
    // The navmesh height at aLocation, stored in aRetHeight.
    static bool GetNavMeshZForLocation(PathingLocation& aLocation, float& aRetHeight);
    // The normal of the navmesh triangle under aLocation, stored in aRetNormal; false, with
    // aRetNormal left unwritten, when the location has no navmesh or triangle.
    static bool GetNavmeshNormalForLocation(PathingLocation& aLocation, NiPoint3& aRetNormal);
    // The navmeshes within afRadius of aLocation, connected to it, in apRetNavMeshes.
    static bool FindConnectedNavmeshesForLocation(
        PathingLocation& aLocation,
        float afRadius,
        BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes
    );
    // The cover points apRequest asks for, stored in aReturnArray.
    static int32_t
    GetCoverPoints(PathingRequestCover* apRequest, BSSimpleArray<PathingCoverLocation, 1024>& apReturnArray);
    // Whether aGoalLocation is visible from aStartLocation across the navmesh, crossing only
    // the edges apFilter allows; the point where the line is cut goes to apReturnIntersectionLocation.
    static bool CheckLineOfSight(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        PathingLocation* apReturnIntersectionLocation,
        CheckLineOfSightFilter* apFilter
    );
    // Whether aTarget is visible from aStartLocation across the navmesh (the overload taking a point).
    static bool CheckLineOfSight(
        PathingLocation& aStartLocation,
        const NiPoint3& aTarget,
        PathingLocation* apReturnIntersectionLocation,
        CheckLineOfSightFilter* apFilter
    );
    // Converts the avoidance cone angle settings from degrees to the cosines the path handlers compare against.
    static bool Init();
    // Builds the path apRequest asks for into apSolution with a PathBuilder; false when none is found.
    static bool Pathfind(PathingRequest* apRequest, PathingSolution* apSolution);
    // The location closest to aGoalLocation that a navmesh search from aStartLocation reaches within
    // afMaxPathingDistance; aStartLocation itself when it has no navmesh.
    static PathingLocation FindClosestReachableLocation(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        float afMaxPathingDistance
    );
    // Builds the path for the request's type under the pathing memory context; false for an unknown type.
    static bool BuildPath(PathingRequest* apRequest, PathingSolution* apSolution);
    // A flee path for apRequest.
    static bool BuildFleePath(PathingRequestFlee* apRequest, PathingSolution* apSolution);
    // A straight two-node flee path for apRequest when there is no navmesh.
    static bool BuildFleePathNoNavMesh(PathingRequestFlee* apRequest, PathingSolution* apSolution);
    // A line-of-sight path for apRequest.
    static bool BuildLOSPath(PathingRequestLOS* apRequest, PathingSolution* apSolution);
    // A hide path for apRequest.
    static bool BuildHidePath(PathingRequestHide* apRequest, PathingSolution* apSolution);
    // A path to a safe straight line for apRequest.
    static bool BuildPathToSafeStraightLine(PathingRequestSafeStraightLine* apRequest, PathingSolution* apSolution);
    // A path to the optimal location for apRequest.
    static bool BuildOptimalLocationPath(PathingRequestOptimalLocation* apRequest, PathingSolution* apSolution);
    // A covered-move path for apRequest.
    static bool BuildCoveredMovePath(PathingRequestCoveredMove* apRequest, PathingSolution* apSolution);
    // Fills apRetPath with the spaces and doors from aOrigin to aDestination; false when no door path exists.
    static bool BuildTeleportDoorPath(
        const PathingLocation& aOrigin,
        const PathingLocation& aDestination,
        TeleportPath* apRetPath,
        TELEPORT_DOOR_PATH_RESTRICTION aRestrictions,
        float afMaxDist,
        ActorLockData* apActorLockData,
        DOOR_LOCK_DISPOSITION aeDoorLockDisp
    );
    // The length of the door path from aOrigin to aDestination, or FLT_MAX when there is none.
    static float ComputeTeleportDoorPathLength(
        const PathingLocation& aOrigin,
        const PathingLocation& aDestination,
        float aMaxDist,
        ActorLockData* apActorLockData,
        DOOR_LOCK_DISPOSITION aeDoorLockDisp
    );
    // whether the two axis-aligned boxes overlap on every axis
    static bool DoAABBsOverlap(
        const NiPoint3& bound1Min,
        const NiPoint3& bound1Max,
        const NiPoint3& bound2Min,
        const NiPoint3& bound2Max
    );
    // Moves *apRetPt to the closest navmesh point to aLocation in the cell or worldspace;
    // false when none is found.
    static bool FindClosestPointOnNavmesh(
        TESWorldSpace* apWorldSpace,
        TESObjectCELL* apCell,
        const NiPoint3& aLocation,
        NiPoint3* apRetPt
    );
    // Looks for a navmesh point within aRadius of aLocation for *apRetPt.
    static bool FindPointOnNavMesh(
        TESWorldSpace* apWorldSpace,
        TESObjectCELL* apCell,
        const NiPoint3& aLocation,
        float aRadius,
        NiPoint3* apRetPt
    );
    // The closest open navmesh edge to aLocation within afMaxRadius. Stores the edge's navmesh info record
    // (NavMesh::GetNavMeshInfo) and copies an edge location through NavMeshEdgeLocation's assignment.
    static bool FindClosestOpenEdge(PathingLocation& aLocation, float afMaxRadius, NavMeshEdgeLocation& aRetEdge);
    // Moves *apRetPt to a point that satisfies apRequest.
    static void FindClosePoint(PathingRequestClosePoint* apRequest, NiPoint3* apRetPt);
    // The navmeshes that may cover an interior cell.
    static const BSSimpleArray<NavMeshInfo*, 1024>* GetPotentialNavMeshInfoForLocation(TESObjectCELL* apCell);
    // The navmeshes that may cover worldspace cell aiCellKey.
    static const BSSimpleArray<NavMeshInfo*, 1024>*
    GetPotentialNavMeshInfoForLocation(TESWorldSpace* apWorldSpace, int32_t aiCellKey);
    // Finds navmeshes around aLocation.
    static bool FindNavmeshesForLocation(
        PathingLocation& aLocation,
        float afRadius,
        BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes,
        bool abConnected
    );
    // Finds a close point through navmesh search.
    static bool FindClosePointViaNavMesh(PathingRequestClosePoint* apRequest, NiPoint3* apRetPt);
    // Estimates path length between two locations.
    static bool ComputeApproximatePathingDistance(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        PathingAvoidNodeArray* apAvoidNodeArray,
        float* apReturnDistance,
        BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodes
    );
    // Whether a path from aStartLocation to aGoalLocation can be estimated.
    static bool CanCreatePath(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        PathingAvoidNodeArray* apAvoidNodeArray
    );
    // Moves apSolution back to the high process from the virtual node aiCurrentVirtualNodeIndex through a PathBuilder.
    static UPDATE_DETAILED_PATH_RESULT UpdateSolutionMoveToHigh(
        PathingRequest* apRequest,
        PathingSolution* apSolution,
        uint32_t aiCurrentVirtualNodeIndex
    );
    // Moves apSolution to the low process through a PathBuilder; the builder's UpdatePathMoveToLow result.
    static UPDATE_DETAILED_PATH_RESULT UpdateSolutionMoveToLow(PathingRequest* apRequest, PathingSolution* apSolution);
    // The navmesh edges closest to aLocation, up to aiMaxCount of them, written to aRetEdges.
    static FIND_CLOSEST_EDGES_RESULT FindClosestEdges(
        PathingLocation& aLocation,
        uint32_t aiMaxCount,
        BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
    );
    static char FindSafeStraightLinePathPoint(
        TESObjectREFR* apOrigin,
        TESObjectREFR* apTarget,
        float afDistance,
        NiPoint3* apRetPt
    );
    static char FindSafeStraightLinePathPoint(PathingRequestSafeStraightLine* apRequest, NiPoint3* apRetPt);
    static Pathing::UPDATE_DETAILED_PATH_RESULT UpdateSolution(
        PathingRequest* apRequest,
        PathingSolution* apSolution,
        uint32_t aiCurrentVirtualNodeIndex,
        const NiPoint3& aCurrentActorLocation
    );
    static bool
    ComputeLowPathDistance(PathingLocation& aStartLocation, PathingLocation& aGoalLocation, float* apReturnDistance);
    static bool CheckLineOfSight(
        PathingLocation& aStartLocation,
        const NiPoint3& aGoalLocation,
        NavMesh* apGoalNavMesh,
        uint16_t ausGoalTriangle,
        PathingLocation* apReturnIntersectionLocation,
        Pathing::CheckLineOfSightFilter* apFilter
    );
    static bool ComputeLineOfSightMap(
        NiPointer<PathingLOSGridMap>& aspGridMap,
        BSSimpleArray<PathingLocation, 1024>& aLocations,
        BSSimpleArray<float, 1024>* apHeadings
    );
    static bool ComputeLineOfSightMapForLocation(
        PathingLocation& aLocation,
        NiPointer<PathingLOSMap>& aspRetMap,
        bool abClearUnseen,
        float afSightRadius,
        float afHeading
    );
    static bool FindPotentialNavmeshesForLocation(
        PathingLocation& aLocation,
        float afRadius,
        BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes
    );
    static bool GetAllLoadedNavmeshes(BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes);
    static uint32_t FindTeleportDoorsForLocation(
        TESForm* apSpace,
        const NiPoint3& arLocation,
        float afRadius,
        BSSimpleArray<TESObjectREFR*, 1024>* apRetDoors
    );
    static bool CheckSphere(PathingLocation& aSphereCenter, float afRadius);
    static bool GetNavMeshZForLocation(
        const NiPoint3& aLocation,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float& aRetHeight
    );
    static NiPoint3 ComputeInitialTangentForActor(
        float afHeading,
        const PathingLocation& aLocation,
        const PathingLocation& aNextLocation,
        float afRadius
    );
    static NiPoint3 ComputeFinalTangentForActor(float afHeading, const PathingLocation& aLocation, float afRadius);
};

extern SettingT<INISettingCollection> fFindClosestEdgesRadius;
