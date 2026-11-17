#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class bhkPickData;
class hkpClosestRayHitCollector;
class NavMeshEdgeLocation;
class PathingRequest;
struct RayCastPath;
struct RayCastPathRequest;

// Defined in PathManager.h.
enum RAYCASTPATH_RESULT : int32_t;

// Which end of a ray cast path request has no navmesh.
enum RAYCAST_REQUEST_TYPE : int32_t {
    RCRT_START = 0,
    RCRT_GOAL = 1,
    RCRT_NOMESH = 2,
};

class PathingSearchRayCast {
public:
    PathingSearchRayCast();
    virtual ~PathingSearchRayCast();

    // Casts the rays of aRequest, writing the path to aRetPath.
    RAYCASTPATH_RESULT BuildPath(RayCastPathRequest& aRequest, RayCastPath& aRetPath);
    // Frees the ray cast cache back to the Havok stack and clears it.
    void FreeCache();
    // Returns iRayCastCount.
    int32_t GetTotalRayCasts();
    void CreateCache();
    bool InitRayCasts();
    void CleanUpRayCasts();
    bool RayCastRecurse(const NiPoint3& aStart, const NiPoint3& aGoal);
    bool RayCast(const NiPoint3& aStart, const NiPoint3& aGoal, NiPoint3& aRetLoc);
    bool UnderWaterRayCast(const NiPoint3& aStart, const NiPoint3& aGoal);
    void ComputeBounds(const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges);

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    PathingLocation StartLocation;
    PathingLocation GoalLocation;
    PathingRequest* pRequest;
    RAYCAST_REQUEST_TYPE eType;
    NiPoint3 BoundsMin;
    NiPoint3 BoundsMax;
    float fActorRadius;
    // The cache CreateCache takes from the Havok stack (freed through hkDeallocateStack<char>).
    char* pCache;
    bhkWorld* pHavokWorld;
    bhkPickData* pPickData;
    hkpClosestRayHitCollector* pCollector;
    int32_t iRayCastCount;
    float fMaxDrop;
    float fMaxStep;

    // The node the debug draw of the ray casts hangs under.
    static NiPointer<NiNode> spRootDrawNode;
};
