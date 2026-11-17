#include "Fallout Shared/Pathfinding/NavMeshHandles.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"

NavMeshEdgeHandle::NavMeshEdgeHandle(const FastNavMeshEdgeHandle& aFastEdgeHandle)
  : NavMeshTriHandle(aFastEdgeHandle.pNavMesh->GetNavMeshInfo(), aFastEdgeHandle.usTriangle),
    iEdgeIndex(aFastEdgeHandle.iEdgeIndex) {}

NavMeshEdgeHandle* NavMeshEdgeHandle::Copy(const NavMeshEdgeHandle& arOther) {
    pNavMeshInfo = arOther.pNavMeshInfo;
    usTriangle = arOther.usTriangle;
    iEdgeIndex = arOther.iEdgeIndex;
    return this;
}

NavMeshEdgeLocation::NavMeshEdgeLocation(const FastNavMeshEdgeLocation& aFastEdgeLocation)
  : NavMeshEdgeHandle(aFastEdgeLocation), Location(aFastEdgeLocation.Location) {}
