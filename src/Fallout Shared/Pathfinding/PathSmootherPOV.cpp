#include "Fallout Shared/Pathfinding/PathSmootherPOV.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/Segment.h"
#include "Fallout Shared/Pathfinding/Sphere.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstdint>

NiPointer<NiNode> PathSmootherPOVSearch::spRootDrawNode;

// The game settings this file defines, with the game's IDs and values, in initialization order.
SettingT<INISettingCollection> fPOVSmootherAvoidNodeCost("fPOVSmootherAvoidNodeCost:Pathfinding", 7.0f);

using AStarNodeHeap_PathSmootherPOVNode_PathSmootherPOVLink_512 =
    AStarNodeHeap<PathSmootherPOVNode, PathSmootherPOVLink, 512>;

using AStarNodeArray_PathSmootherPOV = BSSimpleArray<AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>*, 1024>;
using AStarNodeScrapArray_PathSmootherPOV = BSScrapArray<AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>*, 1024>;

float* ComputePerpendicular(float* a1, float* a2, int32_t a3) {
    float ecx0;
    float v5;
    ecx0;
    if (a3) {
        ecx0 = a2[1];
        v5 = -(*a2);
    }
    else {
        ecx0 = -a2[1];
        v5 = *a2;
    }
    float v6 = (float)0.0;
    *a1 = ecx0;
    a1[1] = v5;
    a1[2] = v6;
    return a1;
}

PathSmootherPOV::PathSmootherPOV() : eType(POV_TYPE_INVALID), Center(NiPoint3_ZERO), fRadius(0.0f), fCostFactor(1.0f) {}

PathSmootherPOVLink::PathSmootherPOVLink() {}

PathSmootherPOVLink& PathSmootherPOVLink::operator=(const PathSmootherPOVLink& __that) {
    ExitAngle = __that.ExitAngle;
    EntryAngle = __that.EntryAngle;
    ExitLocation.Copy(__that.ExitLocation);
    EntryLocation.Copy(__that.EntryLocation);
    return *this;
}

PathSmootherPOVSearch::PathEdgeVert::PathEdgeVert() {}

PathSmootherPOVSearch::PathEdgeVert* PathSmootherPOVSearch::PathEdgeVert::Assign(
    PathSmootherPOVSearch::PathEdgeVert* apThis,
    const PathSmootherPOVSearch::PathEdgeVert& arOther
) {
    ::new (&apThis->Vert) FastNavMeshVertHandle(arOther.Vert);
    apThis->Edge.Copy(arOther.Edge);
    apThis->iOtherSideIndex = arOther.iOtherSideIndex;
    apThis->iThisSideIndex = arOther.iThisSideIndex;
    apThis->iPOVIndex = arOther.iPOVIndex;
    return apThis;
}

PathSmootherPOVSearch::PathSmootherPOVSearch()
  : pRequest(nullptr), pSolution(nullptr), bUnderWaterPath(false), Triangles(nullptr), Edges(nullptr),
    pStartPov(nullptr), pGoalPov(nullptr), pDetailedPathInfos(nullptr) {}

PathSmootherPOVSearch::~PathSmootherPOVSearch() {}

bool PathSmootherPOVSearch::GetNodeConnections(
    const AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode,
    BSSimpleArray<AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>*, 1024>* apRetNodes
) {
    const PathSmootherPOVNode* NodeData = &apNode->GetNodeData();
    CheckAndAddNodeConnection(apNode, pGoalPov, apRetNodes);
    if (NodeData->pPOV->eType) {
        if (NodeData->pPOV->eType == PathSmootherPOV::POV_TYPE_RIGHT_VERTEX) {
            uint32_t FormFlags = RightPOVs.iSize;
            int32_t v16 = 0;
            for (uint32_t i = NodeData->pPOV->iThisSideIndex + 1; i < FormFlags; ++i) {
                if (!CheckAndAddNodeConnection(apNode, RightPOVs.GetAt(i), apRetNodes) && ++v16 == 4) {
                    break;
                }
            }
            uint32_t v24 = LeftPOVs.iSize;
            int32_t v17 = 0;
            for (uint32_t j = NodeData->pPOV->iOtherSideIndex; j < v24; ++j) {
                if (!CheckAndAddNodeConnection(apNode, LeftPOVs.GetAt(j), apRetNodes) && ++v17 == 4) {
                    break;
                }
            }
        }
        else {
            if (NodeData->pPOV->eType == PathSmootherPOV::POV_TYPE_START) {
                uint32_t v25 = RightPOVs.iSize;
                int32_t v18 = 0;
                for (uint32_t k = 0; k < v25; ++k) {
                    if (!CheckAndAddNodeConnection(apNode, RightPOVs.GetAt(k), apRetNodes) && ++v18 == 4) {
                        break;
                    }
                }
                uint32_t v26 = LeftPOVs.iSize;
                int32_t v19 = 0;
                for (uint32_t m = 0; m < v26; ++m) {
                    if (!CheckAndAddNodeConnection(apNode, LeftPOVs.GetAt(m), apRetNodes) && ++v19 == 4) {
                        break;
                    }
                }
            }
            else {
                uint32_t v27 = RightPOVs.iSize;
                for (uint32_t n = 0; n < v27; ++n) {
                    CheckAndAddNodeConnection(apNode, RightPOVs.GetAt(n), apRetNodes);
                }
                uint32_t v28 = LeftPOVs.iSize;
                for (uint32_t ii = 0; ii < v28; ++ii) {
                    CheckAndAddNodeConnection(apNode, LeftPOVs.GetAt(ii), apRetNodes);
                }
            }
        }
    }
    else {
        uint32_t v21 = LeftPOVs.iSize;
        int32_t v14 = 0;
        for (uint32_t uiIndex = NodeData->pPOV->iThisSideIndex + 1; uiIndex < v21; ++uiIndex) {
            if (!CheckAndAddNodeConnection(apNode, LeftPOVs.GetAt(uiIndex), apRetNodes) && ++v14 == 4) {
                break;
            }
        }
        uint32_t v22 = RightPOVs.iSize;
        int32_t v15 = 0;
        for (uint32_t jj = NodeData->pPOV->iOtherSideIndex; jj < v22; ++jj) {
            if (!CheckAndAddNodeConnection(apNode, RightPOVs.GetAt(jj), apRetNodes) && ++v15 == 4) {
                break;
            }
        }
    }
    uint32_t v29 = AvoidNodePOVs.iSize;
    for (uint32_t kk = 0; kk < v29; ++kk) {
        CheckAndAddNodeConnection(apNode, AvoidNodePOVs.GetAt(kk), apRetNodes);
    }
    return true;
}

// The search ends at the goal's point of view.
bool PathSmootherPOVSearch::IsGoalNode(const AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode) {
    return apNode->GetNodeData().pPOV->eType == PathSmootherPOV::POV_TYPE_GOAL;
}

// The directions to the previous and the next vertex (the start or the goal at the ends) are flattened and unitized;
// their cross product's height tells which way the path turns. The end vertices always get a point of view.
void PathSmootherPOVSearch::CheckAddPOVForVert(
    BSSimpleArray<PathSmootherPOVSearch::PathEdgeVert, 1024>& aVerts,
    uint32_t aiIndex,
    bool abLeft
) {
    uint16_t usVertex = aVerts.GetAt(aiIndex).Vert.usVertex;
    NiPoint3 kVertex = aVerts.GetAt(aiIndex).Vert.pNavMesh->GetVertex(usVertex)->Location;
    NiPoint3 kToPrev;
    NiPoint3 kToNext;
    if (aiIndex == 0) {
        kToPrev = StartLocation - kVertex;
    }
    else {
        uint16_t usPrev = aVerts.GetAt(aiIndex - 1).Vert.usVertex;
        kToPrev = aVerts.GetAt(aiIndex - 1).Vert.pNavMesh->GetVertex(usPrev)->Location - kVertex;
    }
    if (aiIndex == (aVerts.GetSize() - 1)) {
        kToNext = GoalLocation - kVertex;
    }
    else {
        uint16_t usNext = aVerts.GetAt(aiIndex + 1).Vert.usVertex;
        kToNext = aVerts.GetAt(aiIndex + 1).Vert.pNavMesh->GetVertex(usNext)->Location - kVertex;
    }
    kToPrev.z = 0.0f;
    kToNext.z = 0.0f;
    kToPrev.Unitize();
    kToNext.Unitize();
    NiPoint3 kCross = abLeft ? kToPrev.Cross(kToNext) : kToNext.Cross(kToPrev);
    float fDot = kToNext.Dot(kToPrev);
    bool bAdd = false;
    if (aiIndex == 0 || aiIndex == (aVerts.GetSize() - 1)) {
        bAdd = true;
    }
    else if (fDot > 0.0) {
        if (kCross.z < 0.05000000074505806) {
            bAdd = true;
        }
    }
    else if (kCross.z < -0.05000000074505806) {
        bAdd = true;
    }
    if (!bAdd) {
        return;
    }
    void* pMemory = ourMalloc(sizeof(PathSmootherPOV));
    PathSmootherPOV* pPOV = pMemory ? ::new (pMemory) PathSmootherPOV : nullptr;
    pPOV->eType = abLeft ? PathSmootherPOV::POV_TYPE_LEFT_VERTEX : PathSmootherPOV::POV_TYPE_RIGHT_VERTEX;
    pPOV->EdgeHandle.Copy(aVerts.GetAt(aiIndex).Edge);
    pPOV->Edge1 = kToPrev;
    pPOV->Edge2 = kToNext;
    pPOV->Center = kVertex;
    pPOV->fCostFactor = 1.0f;
    pPOV->fRadius = pRequest->GetActorRadius() * 1.2000000476837158;
    pPOV->iVertexIndex = aVerts.GetAt(aiIndex).Vert.usVertex;
    pPOV->iOtherSideIndex = 0xFFFFFFFF;
    if (abLeft) {
        pPOV->iThisSideIndex = LeftPOVs.GetSize();
    }
    else {
        pPOV->iThisSideIndex = RightPOVs.GetSize();
    }
    PathEdgeVert& rVert = aVerts.GetAt(aiIndex);
    rVert.iPOVIndex = pPOV->iThisSideIndex;
    allPOVs.Add(pPOV);
    if (abLeft) {
        LeftPOVs.Add(pPOV);
    }
    else {
        RightPOVs.Add(pPOV);
    }
}

// Each avoid node gets a point of view of the actor's radius (x1.2) plus its own; an actor's sphere gets a second,
// no-cross one of a quarter of its radius at ten times the cost.
void PathSmootherPOVSearch::AddPOVsFromAvoidNodes() {
    alignas(NiPointer<PathingAvoidNodeArray>) uint8_t aNodesStorage[sizeof(NiPointer<PathingAvoidNodeArray>)];
    NiPointer<PathingAvoidNodeArray>* pspNodes =
        NiPointer_PathingAvoidNodeArray_Construct(aNodesStorage, pRequest->GetAvoidNodeArray());
    if (pspNodes->operator->()) {
        for (uint32_t i = 0; i < (*pspNodes)->iSize; ++i) {
            PathingAvoidNode* pNode = &(*pspNodes)->GetAt(i);
            void* pMemory = ourMalloc(sizeof(PathSmootherPOV));
            PathSmootherPOV* pPOV = pMemory ? ::new (pMemory) PathSmootherPOV : nullptr;
            pPOV->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
            pPOV->Edge1 = NiPoint3_ZERO;
            pPOV->Edge2 = NiPoint3_ZERO;
            pPOV->Center = *pNode->GetPoint1();
            pPOV->fCostFactor = 1.0f;
            double fNodeRadius = pNode->GetRadius();
            pPOV->fRadius = pRequest->GetActorRadius() * 1.2000000476837158 + fNodeRadius;
            pPOV->iOtherSideIndex = 0xFFFFFFFF;
            pPOV->iAvoidNodeIndex = i;
            allPOVs.Add(pPOV);
            AvoidNodePOVs.Add(pPOV);
            if (pNode->GetType() == PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR) {
                void* pMemory2 = ourMalloc(sizeof(PathSmootherPOV));
                PathSmootherPOV* pPOV2 = pMemory2 ? ::new (pMemory2) PathSmootherPOV : nullptr;
                pPOV2->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE_NOCROSS;
                pPOV2->Edge1 = NiPoint3_ZERO;
                pPOV2->Edge2 = NiPoint3_ZERO;
                pPOV2->Center = *pNode->GetPoint1();
                pPOV2->fCostFactor = 10.0f;
                double fQuarterRadius = pNode->GetRadius() * 0.25;
                pPOV2->fRadius = pRequest->GetActorRadius() * 1.2000000476837158 + fQuarterRadius;
                pPOV2->iOtherSideIndex = 0xFFFFFFFF;
                pPOV2->iAvoidNodeIndex = i;
                allPOVs.Add(pPOV2);
                AvoidNodePOVs.Add(pPOV2);
            }
        }
    }
    pspNodes->~NiPointer<PathingAvoidNodeArray>();
}

// Walks the edges the straight segment from aStartLoc to aGoalLoc crosses (at most 1000), pulling the walking point
// towards each new navmesh's edge midpoint; where the triangle normals on the two sides of an edge differ (dot product
// under 0.9) and the segment crosses that edge more than 32 units from both ends and from the solution's last node, a
// pathing node goes there, raised to the water surface on an underwater path.
void PathSmootherPOVSearch::AddNodeForNormalChanges(
    NavMesh* apStartMesh,
    uint16_t ausStartTri,
    const NiPoint3& aStartLoc,
    const NiPoint3& aGoalLoc
) {
    NiPoint3 kStart = aStartLoc;
    NiPoint3 kGoal = aGoalLoc;
    NiPoint3 kDirection = kGoal - kStart;
    kDirection.Unitize();
    NiPoint3 kCurrent = kStart;
    alignas(FastNavMeshEdgeHandle) uint8_t aEdgeStorage[sizeof(FastNavMeshEdgeHandle)];
    FastNavMeshEdgeHandle& rEdge = *::new (aEdgeStorage) FastNavMeshEdgeHandle(apStartMesh, ausStartTri, -1);
    for (int32_t iCount = 0; iCount < 1000;) {
        ++iCount;
        int32_t iCrossedEdge =
            rEdge.pNavMesh->FindIntersectingEdgeFast(rEdge.usTriangle, rEdge.iEdgeIndex, kCurrent, kGoal);
        if (iCrossedEdge == -1) {
            break;
        }
        alignas(FastNavMeshEdgeHandle) uint8_t aCrossedStorage[sizeof(FastNavMeshEdgeHandle)];
        FastNavMeshEdgeHandle& rCrossed =
            *::new (aCrossedStorage) FastNavMeshEdgeHandle(rEdge.pNavMesh, rEdge.usTriangle, iCrossedEdge);
        FastNavMeshEdgeHandle kOther;
        ::new (&kOther) NavMeshEdgeHandle;
        if (!rCrossed.pNavMesh->GetMatchingEdge(rCrossed, &kOther)) {
            break;
        }
        if (rEdge.pNavMesh != kOther.pNavMesh) {
            NavMeshEdgeVertices kVerts = kOther.pNavMesh->GetEdgeVertices(kOther.usTriangle, kOther.iEdgeIndex);
            NiPoint3 kMidpoint = (kVerts.Vertices[0]->Location + kVerts.Vertices[1]->Location) * 0.5f;
            kCurrent = NavMeshMath::ClosestPointOnSegmentToLine3(
                kVerts.Vertices[0]->Location, kVerts.Vertices[1]->Location, kStart, kGoal
            );
            kCurrent += (kMidpoint - kCurrent) * 0.01f;
        }
        NiPoint3 kNormal = rCrossed.pNavMesh->GetNormal(rCrossed.usTriangle);
        NiPoint3 kOtherNormal = kOther.pNavMesh->GetNormal(kOther.usTriangle);
        if (kNormal.Dot(kOtherNormal) < 0.8999999761581421) {
            NiPoint3 kVert1;
            NiPoint3 kVert2;
            rCrossed.pNavMesh->GetEdgeVertices(rCrossed.usTriangle, rCrossed.iEdgeIndex, &kVert1, &kVert2);
            NiPoint3 kIntersection;
            if (NavMeshMath::Find2DIntersectionPoint(kVert1, kVert2, kStart, kGoal, &kIntersection) &&
                (kIntersection - kStart).SqrLength() > 1024.0 && (kIntersection - kGoal).SqrLength() > 1024.0 &&
                pSolution->GetLastNode()) {
                NiPoint3 kLastLocation = pSolution->GetLastNode()->GetLocation().GetLocation();
                if ((kIntersection - kLastLocation).SqrLength() > 1024.0) {
                    if (bUnderWaterPath) {
                        TESObjectCELL* pCell = pSolution->GetLastNode()->GetLocation().GetCell();
                        float fWaterHeight = TES::pInstance->GetWaterHeight(kIntersection, pCell);
                        if (kIntersection.z <= fWaterHeight) {
                            kIntersection.z = fWaterHeight;
                        }
                    }
                    uint16_t usTriangle = rCrossed.usTriangle;
                    alignas(PathingLocation) uint8_t aLocationStorage[sizeof(PathingLocation)];
                    PathingLocation* pLocation = ::new (aLocationStorage)
                        PathingLocation(kIntersection, rCrossed.pNavMesh->GetNavMeshInfo(), usTriangle);
                    pSolution->AddPathingNode(*pLocation, kDirection * pRequest->GetActorRadius());
                    pLocation->~PathingLocation();
                }
            }
        }
        rEdge.Copy(kOther);
    }
}

// The tangent from the last node's circle to the goal's, going around the last node in its direction: the arc around
// the last node from the last link's entry angle to the tangent is checked twice against the navmesh, then the tangent
// segment (large actors, radius 100 and up, through the triple check; a NaN radius takes the plain one) against the
// navmesh and the avoid nodes. The entry point is raised to the water surface on an underwater path and pulled back by
// twice the actor radius.
bool PathSmootherPOVSearch::CheckForLastNode() {
    if (Links.IsEmpty()) {
        return false;
    }
    PathSmootherPOVNode& rLastNode = Nodes.GetLast();
    PathSmootherPOVLink& rLastLink = Links.GetLast();
    Sphere kLastCircle;
    kLastCircle.~Sphere();
    ::new (&kLastCircle) Sphere(rLastNode.pPOV->Center, rLastNode.pPOV->fRadius);
    Sphere kGoalCircle;
    kGoalCircle.~Sphere();
    ::new (&kGoalCircle) Sphere(pGoalPov->Center, pGoalPov->fRadius);
    NavMeshMath::CCTResults kTangents;
    kTangents.~CCTResults();
    ::new (&kTangents) NavMeshMath::CCTResults;
    if (NavMeshMath::ComputeTangentPoints(kLastCircle, kGoalCircle, rLastNode.Direction, kTangents) ==
        NavMeshMath::CCT_NONE) {
        return false;
    }
    AngleSinCosArc kArc;
    kArc.~AngleSinCosArc();
    ::new (&kArc) AngleSinCosArc(rLastLink.EntryAngle, kTangents.Results[0].Circle1Param, rLastNode.Direction);
    FastNavMeshEdgeLocation kExitLocation;
    kExitLocation.~FastNavMeshEdgeLocation();
    ::new (&kExitLocation) FastNavMeshEdgeLocation;
    AngleSinCosArc kArcArgument;
    kArcArgument.~AngleSinCosArc();
    ::new (&kArcArgument) AngleSinCosArc(&kArc);
    if (!CheckArcAgainstNavMesh(rLastLink.EntryLocation, kLastCircle, kArcArgument, &kExitLocation)) {
        return false;
    }
    AngleSinCosArc kArcArgument2;
    kArcArgument2.~AngleSinCosArc();
    ::new (&kArcArgument2) AngleSinCosArc(&kArc);
    if (!DoubleCheckArcAgainstNavMesh(rLastLink.EntryLocation, kLastCircle, kArcArgument2)) {
        return false;
    }
    FastNavMeshEdgeLocation kEntryLocation;
    kEntryLocation.~FastNavMeshEdgeLocation();
    ::new (&kEntryLocation) FastNavMeshEdgeLocation;
    NiPoint3 kGoalPoint = kGoalCircle.Center + kTangents.Results[0].Circle2Param.ComputePoint3(kGoalCircle.fRadius);
    kExitLocation.iEdgeIndex = -1;
    float fPercentPreferred = 0.0f;
    if (pRequest->GetActorRadius() >= 100.0) {
        if (TripleCheckSegmentAgainstNavMesh(
                kExitLocation, kGoalPoint, &kEntryLocation, false, true, fPercentPreferred
            )) {
            float fCost = 0.0f;
            if (!CheckSegmentAgainstAvoidNodes(rLastNode.pPOV, pGoalPov, kExitLocation.Location, kGoalPoint, &fCost)) {
                return false;
            }
        }
    }
    else if (CheckSegmentAgainstNavMesh(kExitLocation, kGoalPoint, &kEntryLocation, fPercentPreferred)) {
        float fCost = 0.0f;
        if (!CheckSegmentAgainstAvoidNodes(rLastNode.pPOV, pGoalPov, kExitLocation.Location, kGoalPoint, &fCost)) {
            return false;
        }
    }
    if (bUnderWaterPath) {
        TESObjectCELL* pCell = kEntryLocation.pNavMesh->GetParentCell();
        float fWaterHeight = TES::pInstance->GetWaterHeight(kEntryLocation.Location, pCell);
        if (kEntryLocation.Location.z <= fWaterHeight) {
            kEntryLocation.Location.z = fWaterHeight;
        }
    }
    NiPoint3 kBack = kGoalPoint - kExitLocation.Location;
    kBack.Unitize();
    float fRadius = pRequest->GetActorRadius();
    float fBackDistance = fRadius + fRadius;
    kBack *= fBackDistance;
    kEntryLocation.Location -= kBack;
    PathSmootherPOVLink kLink;
    kLink.~PathSmootherPOVLink();
    ::new (&kLink) PathSmootherPOVLink;
    kLink.ExitLocation.Copy(kExitLocation);
    kLink.EntryLocation.Copy(kEntryLocation);
    kLink.ExitAngle = kTangents.Results[0].Circle1Param;
    kLink.EntryAngle = kTangents.Results[0].Circle2Param;
    Links.Add(kLink);
    PathSmootherPOVNode kNode;
    kNode.pPOV = pGoalPov;
    kNode.fActualRadius = 0.0f;
    kNode.Direction = kTangents.Results[0].Direction;
    Nodes.Add(kNode);
    return true;
}

bool PathSmootherPOVSearch::CheckArcAgainstNavMesh(
    const FastNavMeshEdgeLocation& aEntryLocation,
    Sphere& aSphere,
    AngleSinCosArc aArc,
    FastNavMeshEdgeLocation* apReturnLocation
) {
    float fStep;
    if (aArc.Dir) {
        fStep = -0.2617994f;
    }
    else {
        fStep = 0.2617994f;
    }
    AngleSinCos angleStep(fStep);
    AngleSinCos currentStartAngle;
    currentStartAngle = aArc.Start;
    // The sum is built in currentGoalAngle itself.
    AngleSinCos currentGoalAngle = currentStartAngle + angleStep;
    NiPoint3 currentGoalLoc = aSphere.Center + currentGoalAngle.ComputePoint3(aSphere.fRadius);
    FastNavMeshEdgeLocation currentStartLocation(aEntryLocation);
    FastNavMeshEdgeLocation currentEndLocation;
    uint32_t icount = 0;
    int32_t iIgnoreEdge = -1;
    float fdummy = 0.0f;
    while (aArc.WithinRange(currentGoalAngle) && icount < 24) {
        currentStartLocation.iEdgeIndex = -1;
        if (!CheckSegmentAgainstNavMesh(currentStartLocation, currentGoalLoc, &currentEndLocation, fdummy)) {
            apReturnLocation->Copy(currentEndLocation);
            return false;
        }
        currentStartLocation.Copy(currentEndLocation);
        currentStartAngle = currentGoalAngle;
        currentGoalAngle = currentStartAngle + angleStep;
        currentGoalLoc = aSphere.Center + currentGoalAngle.ComputePoint3(aSphere.fRadius);
        iIgnoreEdge = currentEndLocation.iEdgeIndex;
        ++icount;
    }
    if (icount == 24) {
        apReturnLocation->Copy(currentEndLocation);
        return false;
    }
    NiPoint3 lastGoalLoc = aSphere.Center + aArc.End.ComputePoint3(aSphere.fRadius);
    currentStartLocation.iEdgeIndex = -1;
    if (CheckSegmentAgainstNavMesh(currentStartLocation, lastGoalLoc, &currentEndLocation, fdummy)) {
        apReturnLocation->Copy(currentEndLocation);
        return true;
    }
    apReturnLocation->Copy(currentEndLocation);
    return false;
}

bool PathSmootherPOVSearch::CheckSegmentAgainstNavMesh(
    const FastNavMeshEdgeLocation& aStartLocation,
    const NiPoint3& aEndLocation,
    FastNavMeshEdgeLocation* apReturnLocation,
    float& afRetPercentPreferred
) {
    NiPoint3 intersection;
    FastNavMeshEdgeHandle otherEdge;
    float fretz;
    NiPoint3 end;
    NiPoint3 start = aStartLocation.Location;
    end = aEndLocation;
    NiPoint3 delta = end - start;
    delta.Unitize();
    NiPoint3 currentStart = start;
    FastNavMeshEdgeHandle currentEdge(aStartLocation);
    int32_t ipreferredCount = 0;
    int32_t icount = 0;
    while (icount < 1000) {
        ++icount;
        NavMeshTriangle* Triangle = currentEdge.pNavMesh->GetTriangle(currentEdge.usTriangle);
        if (Triangle->IsPreferred()) {
            ++ipreferredCount;
        }
        int32_t newEdge = currentEdge.pNavMesh->FindIntersectingEdgeFast(
            currentEdge.usTriangle, currentEdge.iEdgeIndex, currentStart, end
        );
        if (newEdge == -1) {
            fretz = end.z;
            currentEdge.pNavMesh->IsPointInsideTriangle_0(
                currentEdge.usTriangle, &end, 3.4028235e+38f, 3.4028235e+38f, &fretz
            );
            end.z = fretz;
            apReturnLocation->Copy(FastNavMeshEdgeLocation(currentEdge, end));
            afRetPercentPreferred = static_cast<double>(ipreferredCount) / static_cast<double>(icount);
            return true;
        }
        FastNavMeshEdgeHandle thisEdge(currentEdge.pNavMesh, currentEdge.usTriangle, newEdge);
        ::new (&otherEdge) NiBound;
        bool bfoundMatchingEdge = thisEdge.pNavMesh->GetMatchingEdge(thisEdge, &otherEdge);
        if (bfoundMatchingEdge) {
            EdgeExtraInfo* pextra = thisEdge.pNavMesh->GetEdgeExtraInfo(thisEdge.usTriangle, thisEdge.iEdgeIndex);
            if ((pextra) && (pextra->eType == EIT_LEDGE_UP)) {
                bfoundMatchingEdge = false;
            }

            double Speed = pRequest->fActorRadius;
            if (fPathingLargeActorRadius.Float() < Speed) {
                NavMeshTriangle* v8 = otherEdge.pNavMesh->GetTriangle(otherEdge.usTriangle);
                if (v8->IsNoLargeCreature()) {
                    bfoundMatchingEdge = false;
                }
            }
        }
        if (!bfoundMatchingEdge) {
            NavMeshEdgeVertices verts = thisEdge.pNavMesh->GetEdgeVertices(thisEdge.usTriangle, newEdge);
            NavMeshMath::Find2DIntersectionStable(
                start, end, verts.Vertices[0]->Location, verts.Vertices[1]->Location, &intersection
            );
            apReturnLocation->Copy(FastNavMeshEdgeLocation(thisEdge, intersection));
            afRetPercentPreferred = static_cast<double>(ipreferredCount) / static_cast<double>(icount);
            return false;
        }
        if (currentEdge.pNavMesh != otherEdge.pNavMesh) {
            NiPoint3 v17;
            NavMeshEdgeVertices otherEdgeVerts =
                otherEdge.pNavMesh->GetEdgeVertices(otherEdge.usTriangle, otherEdge.iEdgeIndex);
            NiPoint3 center = (otherEdgeVerts.Vertices[0]->Location + otherEdgeVerts.Vertices[1]->Location) * 0.5f;
            currentStart = NavMeshMath::ClosestPointOnSegmentToLine3(
                otherEdgeVerts.Vertices[0]->Location, otherEdgeVerts.Vertices[1]->Location, start, end
            );
            currentStart += (v17 = (center - currentStart) * 0.01f);
        }
        currentEdge.Copy(otherEdge);
    }
    apReturnLocation->Copy(FastNavMeshEdgeLocation(currentEdge, end));
    if (icount <= 0) {
        afRetPercentPreferred = 0.0f;
    }
    else {
        afRetPercentPreferred = static_cast<double>(ipreferredCount) / static_cast<double>(icount);
    }
    return false;
}

bool PathSmootherPOVSearch::DoubleCheckArcAgainstNavMesh(
    const FastNavMeshEdgeLocation& aEntryLocation,
    Sphere& aSphere,
    AngleSinCosArc aArc
) {
    double fLeftRadiusSum;
    if (aArc.Dir) {
        fLeftRadiusSum = pRequest->fActorRadius * 0.8999999761581421 + aSphere.fRadius;
    }
    else {
        fLeftRadiusSum = aSphere.fRadius - pRequest->fActorRadius * 0.8999999761581421;
    }
    float fLeftRadius = fLeftRadiusSum;
    NiPoint3 leftVec = aArc.Start.ComputePoint3(fLeftRadius);
    FastNavMeshEdgeLocation leftLoc;
    Sphere leftSphere;
    leftSphere.~Sphere();
    ::new (&leftSphere) Sphere(aSphere.Center, fLeftRadius);
    float fdummy = 0.0f;
    if (!CheckSegmentAgainstNavMesh(aEntryLocation, aSphere.Center + leftVec, &leftLoc, fdummy)) {
        return false;
    }
    FastNavMeshEdgeLocation dummyLeftEnd;
    AngleSinCosArc leftArc(&aArc);
    if (!CheckArcAgainstNavMesh(leftLoc, leftSphere, leftArc, &dummyLeftEnd)) {
        return false;
    }
    double fRightRadiusSum;
    if (aArc.Dir) {
        fRightRadiusSum = aSphere.fRadius - pRequest->fActorRadius * 0.8999999761581421;
    }
    else {
        fRightRadiusSum = pRequest->fActorRadius * 0.8999999761581421 + aSphere.fRadius;
    }
    float fRightRadius = fRightRadiusSum;
    NiPoint3 rightVec = aArc.Start.ComputePoint3(fRightRadius);
    FastNavMeshEdgeLocation rightLoc;
    Sphere rightSphere;
    rightSphere.~Sphere();
    ::new (&rightSphere) Sphere(aSphere.Center, fRightRadius);
    if (!CheckSegmentAgainstNavMesh(aEntryLocation, aSphere.Center + rightVec, &rightLoc, fdummy)) {
        return false;
    }
    FastNavMeshEdgeLocation dummyRightEnd;
    AngleSinCosArc rightArc(&aArc);
    return CheckArcAgainstNavMesh(rightLoc, rightSphere, rightArc, &dummyRightEnd);
}

// The four offset vectors start uninitialised. Each half-forward term is computed before the side term it is added to,
// as the game does.
bool PathSmootherPOVSearch::TripleCheckSegmentAgainstNavMesh(
    const FastNavMeshEdgeLocation& aStartLocation,
    const NiPoint3& aEndLocation,
    FastNavMeshEdgeLocation* apReturnLocation,
    bool abStartNode,
    bool abGoalNode,
    float& afRetPercentPreferred
) {
    if (!CheckSegmentAgainstNavMesh(aStartLocation, aEndLocation, apReturnLocation, afRetPercentPreferred)) {
        return false;
    }
    if (bUnderWaterPath) {
        apReturnLocation->Location.z = aEndLocation.z;
        return true;
    }
    NiPoint3 delta = aEndLocation - aStartLocation.Location;
    NiPoint3 unitDelta = delta;
    unitDelta.Unitize();
    NiPoint3 startLeftOffsetVec;
    NiPoint3 startRightOffsetVec;
    NiPoint3 goalLeftOffsetVec;
    NiPoint3 goalRightOffsetVec;
    double fRadius = pRequest->fActorRadius;
    double fRadiusSqr = pRequest->fActorRadius * fRadius;
    float fScale;
    if (!(delta.SqrLength() < fRadiusSqr)) {
        fScale = pRequest->fActorRadius * 0.8999999761581421;
    }
    else {
        fScale = delta.Length() * 0.8999999761581421;
    }
    NiPoint3 forwardVec = unitDelta * fScale;
    NiPoint3 leftVec = NiPoint3_UNIT_Z.Cross(unitDelta);
    if (abStartNode) {
        NiPoint3 halfForward = forwardVec * 0.5f;
        startLeftOffsetVec = leftVec * fScale * 0.5f + halfForward;
        NiPoint3 halfForward2 = forwardVec * 0.5f;
        startRightOffsetVec = -leftVec * fScale * 0.5f + halfForward2;
    }
    else {
        float fLeftRadius = pRequest->fActorRadius;
        startLeftOffsetVec = leftVec * fLeftRadius * 0.9f;
        float fRightRadius = pRequest->fActorRadius;
        startRightOffsetVec = -leftVec * fRightRadius * 0.9f;
    }
    if (abGoalNode) {
        NiPoint3 halfForward = forwardVec * 0.5f;
        goalLeftOffsetVec = leftVec * fScale * 0.5f - halfForward;
        NiPoint3 halfForward2 = forwardVec * 0.5f;
        goalRightOffsetVec = -leftVec * fScale * 0.5f - halfForward2;
    }
    else {
        float fLeftRadius = pRequest->fActorRadius;
        goalLeftOffsetVec = leftVec * fLeftRadius * 0.9f;
        float fRightRadius = pRequest->fActorRadius;
        goalRightOffsetVec = -leftVec * fRightRadius * 0.9f;
    }
    float fdummy;
    FastNavMeshEdgeLocation leftLoc;
    if (!CheckSegmentAgainstNavMesh(aStartLocation, aStartLocation.Location + startLeftOffsetVec, &leftLoc, fdummy)) {
        return false;
    }
    FastNavMeshEdgeLocation dummyLeftEnd;
    if (!CheckSegmentAgainstNavMesh(leftLoc, aEndLocation + goalLeftOffsetVec, &dummyLeftEnd, fdummy)) {
        return false;
    }
    FastNavMeshEdgeLocation rightLoc;
    if (!CheckSegmentAgainstNavMesh(aStartLocation, aStartLocation.Location + startRightOffsetVec, &rightLoc, fdummy)) {
        return false;
    }
    FastNavMeshEdgeLocation dummyRightEnd;
    if (!CheckSegmentAgainstNavMesh(rightLoc, aEndLocation + goalRightOffsetVec, &dummyRightEnd, fdummy)) {
        return false;
    }
    if (!abGoalNode) {
        float zvalue = apReturnLocation->Location.z;
        apReturnLocation->pNavMesh->IsPointInsideTriangle_0(
            apReturnLocation->usTriangle, &apReturnLocation->Location, 3.4028235e+38f, 3.4028235e+38f, &zvalue
        );
        apReturnLocation->Location.z = zvalue;
    }
    return true;
}

// The cost starts as the segment's length; a segment shorter than about 0.03 in x and y crosses nothing. Each avoid
// node circle the segment crosses adds the length of the part inside it times 1 plus fPOVSmootherAvoidNodeCost times
// how deep the part goes (0 at the rim, 1 through the centre). Crossing a no-cross node other than the next POV fails
// when the request allows an incomplete path and the segment does not start at the start POV; otherwise it multiplies
// the cost so far by 10.
bool PathSmootherPOVSearch::CheckSegmentAgainstAvoidNodes(
    PathSmootherPOV* apPrevPOV,
    PathSmootherPOV* apNextPOV,
    const NiPoint3& aStart,
    const NiPoint3& aGoal,
    float* afRetCost
) {
    *afRetCost = (aGoal - aStart).Length();
    if ((aGoal - aStart).SqrXYLength() < 0.0010000000474974513) {
        return true;
    }
    for (PathSmootherPOV* pPOV : AvoidNodePOVs) {
        if (!NavMeshMath::DoCircleAndSegmentIntersect(pPOV->Center, pPOV->fRadius, aStart, aGoal)) {
            continue;
        }
        Sphere kCircle;
        kCircle.~Sphere();
        ::new (&kCircle) Sphere(pPOV->Center, pPOV->fRadius);
        Segment kSegment;
        kSegment.~Segment();
        ::new (&kSegment) Segment(aStart, aGoal);
        NavMeshMath::CSIResults kResults;
        kResults.~CSIResults();
        ::new (&kResults) NavMeshMath::CSIResults;
        NavMeshMath::CircleSegmentIntersection(kCircle, kSegment, kResults);
        NiPoint3 kEnter;
        NiPoint3 kExit;
        switch (kResults.eType) {
            case NavMeshMath::CSI_NONE:
            case NavMeshMath::CSI_TANGENT: continue;
            case NavMeshMath::CSI_CONTAINED:
                kEnter = aStart;
                kExit = aGoal;
                break;
            case NavMeshMath::CSI_TWOPOINTS:
                kEnter = aStart + (aGoal - aStart) * kResults.Results[0].fSegmentParam;
                kExit = aStart + (aGoal - aStart) * kResults.Results[1].fSegmentParam;
                break;
            case NavMeshMath::CSI_ONEPOINT_LOWER:
                kEnter = aStart;
                kExit = aStart + (aGoal - aStart) * kResults.Results[0].fSegmentParam;
                break;
            case NavMeshMath::CSI_ONEPOINT_UPPER:
                kEnter = aStart + (aGoal - aStart) * kResults.Results[0].fSegmentParam;
                kExit = aGoal;
                break;
            default: break;
        }
        if (pPOV->eType == PathSmootherPOV::POV_TYPE_AVOIDNODE_NOCROSS && pPOV != apNextPOV) {
            if (pRequest->GetAllowIncompletePath() && apPrevPOV->eType != PathSmootherPOV::POV_TYPE_START) {
                return false;
            }
            *afRetCost = *afRetCost * 10.0;
        }
        float fLength = (kExit - kEnter).Length();
        float fDistance = NavMeshMath::DistanceFromSegment(pPOV->Center, kEnter, kExit, nullptr);
        float fDepth = (pPOV->fRadius - fDistance) / pPOV->fRadius;
        if (fDepth < 0.0) {
            fDepth = 0.0f;
        }
        *afRetCost = (fDepth * fPOVSmootherAvoidNodeCost.Float() + 1.0f) * fLength + *afRetCost;
    }
    return true;
}

// The key of apPOV's node in aDirection: the POV's address plus the direction, added as integers; the address is only a
// number here, never read through, hence the cast to an integer.
uintptr_t PathSmootherPOVSearch::GetKeyForNode(PathSmootherPOV* apPOV, ANGLE_DIRECTION aDirection) {
    return reinterpret_cast<uintptr_t>(apPOV) + aDirection;
}

// KEEP: the exe reads stack bytes nothing wrote. The start node's fActualRadius (aNodeData's third dword, [ebp-4]) is
// never set before AddStartNode (0x6B32F0) copies all three dwords into the node, and the bad link's padding bytes
// (6-7 of [ebp-0x30]) are copied into apBadNodeArray by its Add (0x6CF4C0, three dword moves).
bool PathSmootherPOVSearch::SmoothPath(
    PathingRequest* apRequest,
    const BSSimpleArray<NavMeshSearchNode, 1024>& aTriSolution,
    const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks,
    const NiPoint3& aStartLocation,
    const NiPoint3& aGoalLocation,
    PathingSolution* apSolution,
    BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray,
    BSSimpleArray<const NavMeshInfo*, 1024>* apDetailedPathInfos
) {
    bool bStartUnderWater;
    float fStartZ;
    float fGoalZ;
    PathSmootherPOVNode aNodeData;
    pRequest = apRequest;
    StartLocation = aStartLocation;
    GoalLocation = aGoalLocation;
    Triangles = &aTriSolution;
    Edges = &aLinks;
    pSolution = apSolution;
    pDetailedPathInfos = apDetailedPathInfos;
    if ((&aTriSolution.GetFirst())->pNavMesh->pParentCell) {
        TESObjectCELL* pCell = (&aTriSolution.GetFirst())->pNavMesh->pParentCell;
        bStartUnderWater =
            (&aTriSolution.GetFirst())->GetCenter().z < TES::pInstance->GetWaterHeight(aStartLocation, pCell) + 50.0;
    }
    else {
        bStartUnderWater = true;
    }
    if (!bStartUnderWater) {
        fStartZ = StartLocation.z;
        (&aTriSolution.GetFirst())
            ->pNavMesh->IsPointInsideTriangle_0(
                (&aTriSolution.GetFirst())->sTriangle, &StartLocation, 180.0f, 64.0f, &fStartZ
            );
        StartLocation.z = fStartZ;
    }
    bool bGoalUnderWater = false;
    if ((!(&apRequest->Destination)->QUnused()) && ((&aTriSolution.GetLast())->pNavMesh->pParentCell)) {
        TESObjectCELL* pCell = (&aTriSolution.GetLast())->pNavMesh->pParentCell;
        if ((&aTriSolution.GetLast())->GetCenter().z < TES::pInstance->GetWaterHeight(aGoalLocation, pCell) + 50.0) {
            bGoalUnderWater = true;
        }
    }

    if (!bGoalUnderWater) {
        fGoalZ = GoalLocation.z;
        (&aTriSolution.GetLast())
            ->pNavMesh->IsPointInsideTriangle_0(
                (&aTriSolution.GetLast())->sTriangle, &GoalLocation, 180.0f, 64.0f, &fGoalZ
            );
        GoalLocation.z = fGoalZ;
    }
    if ((bStartUnderWater) && (bGoalUnderWater)) {
        bUnderWaterPath = true;
    }

    LeftPOVs.SetReservedSize(aLinks.iSize);
    RightPOVs.SetReservedSize(aLinks.iSize);
    LeftVerts.SetReservedSize(aLinks.iSize);
    RightVerts.SetReservedSize(aLinks.iSize);
    uint32_t uiAvoidNodeReserve;
    if (pRequest->GetAvoidNodeArray()) {
        uiAvoidNodeReserve = pRequest->GetAvoidNodeArray()->iSize * 2;
    }
    else {
        uiAvoidNodeReserve = 0;
    }
    AvoidNodePOVs.SetReservedSize(uiAvoidNodeReserve);
    uint32_t v15 = aLinks.iSize;
    allPOVs.SetReservedSize(AvoidNodePOVs.iReservedSize + 2 * v15 + 2);
    BuildPOVList();
    aNodeData.pPOV = pStartPov;
    aNodeData.Direction = AD_CCW;
    float v25 = (GoalLocation - StartLocation).Length() * 0.75;
    AddStartNode(GetKeyForNode(pStartPov, AD_CCW), aNodeData, v25);
    Nodes.Clear(false);
    Links.Clear(false);
    bool bres = Search(&Nodes, &Links, true);
    if (!bres) {
        CheckForLastNode();
        if (apBadNodeArray) {
            AddIntersectingPOVsAsBadNodes(apBadNodeArray);
            if ((&Nodes.GetLast())->pPOV->eType == PathSmootherPOV::POV_TYPE_LEFT_VERTEX ||
                (&Nodes.GetLast())->pPOV->eType == PathSmootherPOV::POV_TYPE_RIGHT_VERTEX) {
                FastNavMeshEdgeHandle kEdgeHandle((&Nodes.GetLast())->pPOV->EdgeHandle);
                // Copies the handle's three members into the link, leaving its padding as it was.
                NavMeshSearchLink kBadLink;
                kBadLink.pNavMesh = kEdgeHandle.pNavMesh;
                kBadLink.sTriangle = kEdgeHandle.usTriangle;
                kBadLink.iEdgeIndex = kEdgeHandle.iEdgeIndex;
                apBadNodeArray->Add(kBadLink);
            }
        }
    }
    CreatePathingNodes(!bres);
    for (uint32_t i = 0;; ++i) {
        uint32_t v18 = aTriSolution.iSize;
        if (i >= v18) {
            break;
        }
        if (aTriSolution.GetAt(static_cast<int32_t>(i)).GetTriangle()->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
            uint16_t usTriangle = (&aTriSolution.GetAt(static_cast<int32_t>(i)))->sTriangle;
            uint32_t doorID = (&aTriSolution.GetAt(static_cast<int32_t>(i)))
                                  ->pNavMesh->GetClosedDoorInfoForTriangle(usTriangle)
                                  ->DoorFormID;
            pSolution->DoorIDs.Add(doorID);
        }
    }
    for (uint32_t i_2 = 0;; ++i_2) {
        uint32_t v19 = allPOVs.iSize;
        if (i_2 >= v19) {
            break;
        }
        ourFree(allPOVs.GetAt(static_cast<int32_t>(i_2)));
    }
    return bres;
}

void PathSmootherPOVSearch::BuildPOVList() {
    NiPoint3 v23;
    NiPoint3 v30;
    AddStartPOV();
    AddPOVsFromTrianglePath();
    AddGoalPOV();
    AddPOVsFromAvoidNodes();
    AddPOVsFromStaticAvoidNodes();
    AddGoalPOVsForGoalsOnNavMeshEdge();
    if (pRequest->GetTryToUseCurvedPath()) {
        PathSmootherPOV* protateNode3;
        PathSmootherPOV* protateNode2;
        PathSmootherPOV* protateNode;
        void* apMemory = ourMalloc(sizeof(PathSmootherPOV));
        if (apMemory) {
            protateNode = ::new (apMemory) PathSmootherPOV;
        }
        else {
            protateNode = nullptr;
        }
        PathSmootherPOV* a2 = protateNode;
        protateNode->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        a2->Center = (StartLocation + GoalLocation) * 0.5f;
        double v4 = (StartLocation - GoalLocation).Length();
        a2->fRadius = v4 * 0.4000000059604645;
        a2->iVertexIndex = -1;
        a2->fCostFactor = 100.0f;
        allPOVs.Add(a2);
        AvoidNodePOVs.Add(a2);
        NiPoint3 delta = StartLocation - GoalLocation;
        NiPoint3 right = NiPoint3_UNIT_Z.Cross(delta);
        PathSmootherPOV* v33 = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
        if (v33) {
            protateNode2 = ::new (v33) PathSmootherPOV;
        }
        else {
            protateNode2 = nullptr;
        }
        PathSmootherPOV* aItem = protateNode2;
        protateNode2->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        NiPoint3* fScalar = ::new (&v30) NiPoint3(right * 0.2f);
        aItem->Center = GoalLocation + delta * 0.2f + *fScalar;
        double v9 = (StartLocation - GoalLocation).Length();
        aItem->fRadius = v9 * 0.25;
        aItem->iVertexIndex = -1;
        aItem->fCostFactor = 100.0f;
        allPOVs.Add(aItem);
        AvoidNodePOVs.Add(aItem);
        PathSmootherPOV* v26 = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
        if (v26) {
            protateNode3 = ::new (v26) PathSmootherPOV;
        }
        else {
            protateNode3 = nullptr;
        }
        PathSmootherPOV* v43 = protateNode3;
        protateNode3->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        NiPoint3* fScalara = ::new (&v23) NiPoint3(right * 0.2f);
        v43->Center = GoalLocation + delta * 0.2f - *fScalara;
        double v14 = (StartLocation - GoalLocation).Length();
        v43->fRadius = v14 * 0.25;
        v43->iVertexIndex = -1;
        v43->fCostFactor = 100.0f;
        allPOVs.Add(v43);
        AvoidNodePOVs.Add(v43);
    }
}

void PathSmootherPOVSearch::AddPOVsFromStaticAvoidNodes() {
    using pathMeshes_t = BSSimpleArray<NavMesh*, 1024>;
    pathMeshes_t pathMeshes;
    for (const NavMeshSearchNode& rTriangle : *Triangles) {
        if (pathMeshes.Find(rTriangle.pNavMesh, 0, &BSSimpleArray<NavMesh*>::DefaultEqualFunctionByAddress) == -1) {
            pathMeshes.Add(rTriangle.pNavMesh);
        }
    }
    for (uint32_t i_2 = 0; i_2 < pathMeshes.iSize; ++i_2) {
        NavMesh* pmesh = pathMeshes.GetAt(static_cast<int32_t>(i_2));
        for (uint32_t j = 0; j < pmesh->GetStaticAvoidNodeCount(); ++j) {
            PathSmootherPOV* psmallerPov;
            PathSmootherPOV* apMemory = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
            if (apMemory) {
                psmallerPov = ::new (apMemory) PathSmootherPOV;
            }
            else {
                psmallerPov = nullptr;
            }
            PathSmootherPOV* pPov = psmallerPov;
            pPov->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
            pPov->Edge1 = NiPoint3_ZERO;
            pPov->Edge2 = NiPoint3_ZERO;
            pPov->Center = *pmesh->GetStaticAvoidNode(j)->GetPoint1();
            pPov->fCostFactor = 1.0f;
            // The node's radius is held as a double and read before the actor radius.
            double fNodeRadius = pmesh->GetStaticAvoidNode(j)->GetRadius();
            pPov->fRadius = pRequest->GetActorRadius() * 1.2000000476837158 + fNodeRadius;
            pPov->iOtherSideIndex = -1;
            pPov->iAvoidNodeIndex = j + 1000 * i_2;
            allPOVs.Add(pPov);
            AvoidNodePOVs.Add(pPov);
        }
    }
}

void PathSmootherPOVSearch::CreatePathingNodes(bool abFailed) {
    for (uint32_t i = 0; i < Links.iSize; ++i) {
        NiPoint3 entryTangent;
        NiPoint3* pExitPoint = &Links.GetAt(static_cast<int32_t>(i)).ExitLocation.Location;
        NiPoint3 delta = Links.GetAt(static_cast<int32_t>(i)).EntryLocation.Location - *pExitPoint;
        float flength = delta.Length();
        bool baddExitNode = true;
        if (i) {
            AngleSinCos* pPrevEntryAngle = &Links.GetAt(static_cast<int32_t>(i - 1)).EntryAngle;
            AngleSinCos angleDiff = Links.GetAt(static_cast<int32_t>(i)).ExitAngle - *pPrevEntryAngle;
            AngleSinCos v49(1.5707964f);
            AngleSinCos v48(-1.5707964f);
            AngleSinCosArc toleranceArc(v48, v49, 0);
            double Angle = angleDiff.GetAngle();
            if (Nodes.GetAt(static_cast<int32_t>(i)).pPOV->fRadius * Angle < 50.0 &&
                toleranceArc.WithinRange(angleDiff)) {
                baddExitNode = false;
            }
            if (bUnderWaterPath) {
                TESObjectCELL* pCell = Links.GetAt(static_cast<int32_t>(i)).ExitLocation.pNavMesh->pParentCell;
                float fwaterHeight =
                    TES::pInstance->GetWaterHeight(Links.GetAt(static_cast<int32_t>(i)).ExitLocation.Location, pCell);
                if (Links.GetAt(static_cast<int32_t>(i)).ExitLocation.Location.z <= static_cast<double>(fwaterHeight)) {
                    Links.GetAt(static_cast<int32_t>(i)).ExitLocation.Location.z = fwaterHeight;
                }
            }
        }
        if (baddExitNode) {
            NiPoint3 exitTangent;
            NavMeshInfo* pExitInfo = Links.GetAt(static_cast<int32_t>(i)).ExitLocation.pNavMesh->GetNavMeshInfo();
            PathingLocation exitLoc(&Links.GetAt(static_cast<int32_t>(i)).ExitLocation.Location, pExitInfo);
            int32_t iExitDirection = Nodes.GetAt(static_cast<int32_t>(i)).Direction;
            ComputePerpendicular(&exitTangent.x, &Links.GetAt(static_cast<int32_t>(i)).ExitAngle.fCos, iExitDirection);
            if (i) {
                AngleSinCos* pPrevEntryAngle = &Links.GetAt(static_cast<int32_t>(i - 1)).EntryAngle;
                AngleSinCos deltaAngle = Links.GetAt(static_cast<int32_t>(i)).ExitAngle - *pPrevEntryAngle;
                double v14 = deltaAngle.GetAngle();
                float v46 = Nodes.GetAt(static_cast<int32_t>(i)).pPOV->fRadius * v14;
                float flength_2 = std::fabs(v46);
                exitTangent *= flength_2;
            }
            else {
                exitTangent *= Nodes.GetAt(0).pPOV->fRadius;
            }
            exitTangent *= 3.0f;
            double v15 = exitTangent.SqrLength();
            if (flength * flength < v15) {
                exitTangent.Unitize();
                exitTangent *= flength;
            }
            pSolution->AddPathingNode(exitLoc, exitTangent);
        }
        if ((bUnderWaterPath) && (i < Links.iSize - 1 || (pSolution->IsIncomplete() || abFailed))) {
            TESObjectCELL* pCell = Links.GetAt(static_cast<int32_t>(i)).EntryLocation.pNavMesh->pParentCell;
            float fwaterHeight_2 =
                TES::pInstance->GetWaterHeight(Links.GetAt(static_cast<int32_t>(i)).EntryLocation.Location, pCell);
            if (Links.GetAt(static_cast<int32_t>(i)).EntryLocation.Location.z <= static_cast<double>(fwaterHeight_2)) {
                Links.GetAt(static_cast<int32_t>(i)).EntryLocation.Location.z = fwaterHeight_2;
            }
        }

        NavMeshInfo* pEntryInfo = Links.GetAt(static_cast<int32_t>(i)).EntryLocation.pNavMesh->GetNavMeshInfo();
        PathingLocation entryLoc(&Links.GetAt(static_cast<int32_t>(i)).EntryLocation.Location, pEntryInfo);
        int32_t iEntryDirection = Nodes.GetAt(static_cast<int32_t>(i + 1)).Direction;
        ComputePerpendicular(&entryTangent.x, &Links.GetAt(static_cast<int32_t>(i)).EntryAngle.fCos, iEntryDirection);
        NiPoint3* pEntryPoint = &Links.GetAt(static_cast<int32_t>(i)).EntryLocation.Location;
        NiPoint3* pExitPoint_2 = &Links.GetAt(static_cast<int32_t>(i)).ExitLocation.Location;
        uint16_t usTriangle = Links.GetAt(static_cast<int32_t>(i)).ExitLocation.usTriangle;
        AddNodeForNormalChanges(
            Links.GetAt(static_cast<int32_t>(i)).ExitLocation.pNavMesh, usTriangle, *pExitPoint_2, *pEntryPoint
        );
        if (i >= Links.iSize - 1) {
            entryTangent *= Nodes.GetAt(static_cast<int32_t>(i + 1)).pPOV->fRadius;
        }
        else {
            AngleSinCos* pEntryAngle = &Links.GetAt(static_cast<int32_t>(i)).EntryAngle;
            AngleSinCos deltaAngle_2 = Links.GetAt(static_cast<int32_t>(i + 1)).ExitAngle - *pEntryAngle;
            double v26 = deltaAngle_2.GetAngle();
            float v45 = Nodes.GetAt(static_cast<int32_t>(i + 1)).pPOV->fRadius * v26;
            float flength_3 = std::fabs(v45);
            entryTangent *= flength_3;
        }
        entryTangent *= 3.0f;
        double v27 = entryTangent.SqrLength();
        if (flength * flength < v27) {
            entryTangent.Unitize();
            entryTangent *= flength;
        }
        pSolution->AddPathingNode(entryLoc, entryTangent);
    }
    if ((pRequest->GetFaceTargetAtGoal()) && (!Links.IsEmpty())) {
        NiPoint3 lastTangent = pSolution->GetLastNode()->Tangent;
        float flastTangentAngle = GetZAngleFromVector(lastTangent);
        float fgoalAngle = pRequest->GetGoalAngleToFace();
        float fangleDelta = fgoalAngle - flastTangentAngle;
        BSWrap(fangleDelta, -3.1415927f, 3.1415927f);
        if (std::fabs(fangleDelta) < 1.5707963267948966) {
            float flength_4 = lastTangent.XYLength();
            NiPoint3 newTangent;
            newTangent.x = std::cos(fgoalAngle) * flength_4;
            newTangent.y = std::sin(fgoalAngle) * flength_4;
            newTangent.z = 0.0f;
            pSolution->GetLastNode()->SetTangent(newTangent);
        }
    }
}

bool PathSmootherPOVSearch::AddIntersectingPOVsAsBadNodes(BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray) {
    if (!apBadNodeArray) {
        return false;
    }
    bool baddedBadNodes = false;
    for (uint32_t auiIndex = 0; auiIndex < Edges->GetSize(); ++auiIndex) {
        NiPoint3 kVert1;
        NiPoint3 kVert2;
        int32_t iEdgeIndex = (*Edges)[auiIndex].iEdgeIndex;
        uint16_t sTriangle = (*Edges)[auiIndex].sTriangle;
        NavMesh* pNavMesh = (*Edges)[auiIndex].pNavMesh;
        pNavMesh->GetEdgeVertices(sTriangle, iEdgeIndex, &kVert1, &kVert2);
        double fSqrLength = (kVert1 - kVert2).SqrLength();
        double fRadius = pRequest->fActorRadius;
        if (pRequest->fActorRadius * fRadius * 4.0 > fSqrLength) {
            apBadNodeArray->Add((*Edges)[auiIndex]);
            baddedBadNodes = true;
        }
    }
    return baddedBadNodes;
}

void PathSmootherPOVSearch::AddStartPOV() {
    PathSmootherPOV* v2;
    PathSmootherPOV* apMemory = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
    if (apMemory) {
        v2 = ::new (apMemory) PathSmootherPOV;
    }
    else {
        v2 = nullptr;
    }
    pStartPov = v2;
    pStartPov->eType = PathSmootherPOV::POV_TYPE_START;
    NiPoint3* p_Center = &pStartPov->Center;
    p_Center->x = StartLocation.x;
    p_Center->y = StartLocation.y;
    p_Center->z = StartLocation.z;
    pStartPov->fRadius = 0.0f;
    pStartPov->iVertexIndex = -1;
    allPOVs.Add(pStartPov);
}

void PathSmootherPOVSearch::AddGoalPOV() {
    PathSmootherPOV* v2;
    PathSmootherPOV* apMemory = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
    if (apMemory) {
        v2 = ::new (apMemory) PathSmootherPOV;
    }
    else {
        v2 = nullptr;
    }
    pGoalPov = v2;
    pGoalPov->eType = PathSmootherPOV::POV_TYPE_GOAL;
    NiPoint3* p_Center = &pGoalPov->Center;
    p_Center->x = GoalLocation.x;
    p_Center->y = GoalLocation.y;
    p_Center->z = GoalLocation.z;
    pGoalPov->fRadius = 0.0f;
    pGoalPov->iVertexIndex = -1;
    allPOVs.Add(pGoalPov);
}

// The path's edges become left and right vertex chains: the first edge's ends are sided by the turn from the start,
// each later edge adds the end it does not share with the last left or right vertex to the other side (through the
// previous triangle's matching edge when it shares neither). Then the POVs are taken and each side's POV is linked to
// the next POV on the other side.
void PathSmootherPOVSearch::AddPOVsFromTrianglePath() {
    LeftVerts.Clear(false);
    RightVerts.Clear(false);
    for (uint32_t i = 0; i < Edges->iSize; ++i) {
        int32_t iEdgeIndex = (*Edges)[i].iEdgeIndex;
        uint16_t sTriangle = (*Edges)[i].sTriangle;
        FastNavMeshEdgeHandle edgeHandle((*Edges)[i].pNavMesh, sTriangle, iEdgeIndex);
        int32_t iEdgeIndex2 = (*Edges)[i].iEdgeIndex;
        uint16_t sTriangle2 = (*Edges)[i].sTriangle;
        NavMeshEdge edge = (*Edges)[i].pNavMesh->GetEdge(sTriangle2, iEdgeIndex2);
        uint16_t usVertex1 = edge.Vertices[0];
        FastNavMeshVertHandle vert1((*Edges)[i].pNavMesh, usVertex1);
        uint16_t usVertex2 = edge.Vertices[1];
        FastNavMeshVertHandle vert2((*Edges)[i].pNavMesh, usVertex2);
        if (i) {
            if (vert1 == LeftVerts.GetLast().Vert) {
                AddRightVert(vert2, edgeHandle);
            }
            else if (vert1 == RightVerts.GetLast().Vert) {
                AddLeftVert(vert2, edgeHandle);
            }
            else if (vert2 == LeftVerts.GetLast().Vert) {
                AddRightVert(vert1, edgeHandle);
            }
            else if (vert2 == RightVerts.GetLast().Vert) {
                AddLeftVert(vert1, edgeHandle);
            }
            else {
                NavMesh* pNewMesh = nullptr;
                uint16_t usNewTriangle = 0xFFFF;
                int32_t iedgeIndex = -1;
                int32_t iPrevEdgeIndex = (*Edges)[i - 1].iEdgeIndex;
                uint16_t sPrevTriangle = (*Triangles)[i - 1].sTriangle;
                (*Triangles)[i - 1].pNavMesh->GetMatchingEdge(
                    sPrevTriangle, iPrevEdgeIndex, &pNewMesh, &usNewTriangle, &iedgeIndex
                );
                FastNavMeshEdgeHandle newEdgeHandle(pNewMesh, usNewTriangle, iedgeIndex);
                NavMeshEdge newEdge = pNewMesh->GetEdge(usNewTriangle, iedgeIndex);
                uint16_t usNewVertex1 = newEdge.Vertices[0];
                FastNavMeshVertHandle newLeftVert((*Edges)[i].pNavMesh, usNewVertex1);
                uint16_t usNewVertex2 = newEdge.Vertices[1];
                FastNavMeshVertHandle newRightVert((*Edges)[i].pNavMesh, usNewVertex2);
                if (vert1 == newLeftVert) {
                    AddRightVert(vert2, newEdgeHandle);
                }
                else if (vert1 == newRightVert) {
                    AddLeftVert(vert2, newEdgeHandle);
                }
                else if (vert2 == newLeftVert) {
                    AddRightVert(vert1, newEdgeHandle);
                }
                else if (vert2 == newRightVert) {
                    AddLeftVert(vert1, newEdgeHandle);
                }
            }
        }
        else {
            NavMeshVertex* pVertex2 = vert2.pNavMesh->GetVertex(vert2.usVertex);
            NavMeshVertex* pVertex1 = vert1.pNavMesh->GetVertex(vert1.usVertex);
            NiPoint3 edgeVec = pVertex1->Location - pVertex2->Location;
            NavMeshVertex* pVertex1Again = vert1.pNavMesh->GetVertex(vert1.usVertex);
            NiPoint3 cross = edgeVec.Cross(pVertex1Again->Location - StartLocation);
            // Not greater than 0.0, or unordered (NaN), takes the first branch.
            if (!(cross.z > 0.0)) {
                AddLeftVert(vert1, edgeHandle);
                AddRightVert(vert2, edgeHandle);
            }
            else {
                AddLeftVert(vert2, edgeHandle);
                AddRightVert(vert1, edgeHandle);
            }
        }
    }
    for (uint32_t i_2 = 0; i_2 < LeftVerts.iSize; ++i_2) {
        CheckAddPOVForVert(LeftVerts, i_2, true);
    }
    for (uint32_t i_3 = 0; i_3 < RightVerts.iSize; ++i_3) {
        CheckAddPOVForVert(RightVerts, i_3, false);
    }
    for (const PathSmootherPOVSearch::PathEdgeVert& rLeftVert : LeftVerts) {
        if (rLeftVert.iPOVIndex != -1) {
            for (uint32_t k = rLeftVert.iOtherSideIndex; k < RightVerts.iSize; ++k) {
                if ((&RightVerts.GetAt(static_cast<int32_t>(k)))->iPOVIndex != -1) {
                    PathSmootherPOVSearch::PathEdgeVert* v15 = &RightVerts.GetAt(static_cast<int32_t>(k));
                    LeftPOVs.GetAt(rLeftVert.iPOVIndex)->iOtherSideIndex = v15->iPOVIndex;
                    break;
                }
            }
        }
    }
    for (const PathSmootherPOVSearch::PathEdgeVert& rRightVert : RightVerts) {
        if (rRightVert.iPOVIndex != -1) {
            for (uint32_t n = rRightVert.iOtherSideIndex; n < LeftVerts.iSize; ++n) {
                if ((&LeftVerts.GetAt(static_cast<int32_t>(n)))->iPOVIndex != -1) {
                    PathSmootherPOVSearch::PathEdgeVert* v18 = &LeftVerts.GetAt(static_cast<int32_t>(n));
                    RightPOVs.GetAt(rRightVert.iPOVIndex)->iOtherSideIndex = v18->iPOVIndex;
                    break;
                }
            }
        }
    }
}

void PathSmootherPOVSearch::AddLeftVert(const FastNavMeshVertHandle& aVert, const FastNavMeshEdgeHandle& aEdge) {
    PathSmootherPOVSearch::PathEdgeVert apMemory;
    apMemory.~PathEdgeVert();
    ::new (&apMemory) PathSmootherPOVSearch::PathEdgeVert;
    apMemory.Vert.~FastNavMeshVertHandle();
    ::new (&apMemory.Vert) FastNavMeshVertHandle(aVert);
    apMemory.Edge.Copy(aEdge);
    apMemory.iOtherSideIndex = RightVerts.iSize;
    apMemory.iThisSideIndex = LeftVerts.iSize;
    apMemory.iPOVIndex = -1;
    LeftVerts.Add(apMemory);
}

void PathSmootherPOVSearch::AddRightVert(const FastNavMeshVertHandle& aVert, const FastNavMeshEdgeHandle& aEdge) {
    PathSmootherPOVSearch::PathEdgeVert apMemory;
    apMemory.~PathEdgeVert();
    ::new (&apMemory) PathSmootherPOVSearch::PathEdgeVert;
    apMemory.Vert.~FastNavMeshVertHandle();
    ::new (&apMemory.Vert) FastNavMeshVertHandle(aVert);
    apMemory.Edge.Copy(aEdge);
    apMemory.iOtherSideIndex = LeftVerts.iSize;
    apMemory.iThisSideIndex = RightVerts.iSize;
    apMemory.iPOVIndex = -1;
    RightVerts.Add(apMemory);
}

void PathSmootherPOVSearch::AddGoalPOVsForGoalsOnNavMeshEdge() {
    NavMeshEdgeLocation closestEdgeToStart;
    uint16_t sTriangle = (&Triangles->GetFirst())->sTriangle;
    NavMeshInfo* NavMeshInfo_ = (&Triangles->GetFirst())->pNavMesh->GetNavMeshInfo();
    PathingLocation startLoc(StartLocation, NavMeshInfo_, sTriangle);
    float afMaxRadius = pRequest->fActorRadius;
    if (Pathing::FindClosestOpenEdge(startLoc, afMaxRadius, closestEdgeToStart)) {
        PathSmootherPOV* ppov2;
        PathSmootherPOV* ppov;
        NiPoint3 deltaToGoal = StartLocation - closestEdgeToStart.Location;
        NiPoint3 rightVec = NiPoint3_UNIT_Z.UnitCross(deltaToGoal);
        void* apMemory = ourMalloc(sizeof(PathSmootherPOV));
        if (apMemory) {
            ppov = ::new (apMemory) PathSmootherPOV;
        }
        else {
            ppov = nullptr;
        }
        PathSmootherPOV* ppov_2 = ppov;
        ppov->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        ppov_2->Edge1 = NiPoint3_ZERO;
        ppov_2->Edge2 = NiPoint3_ZERO;
        float fScalar = pRequest->fActorRadius;
        ppov_2->Center = StartLocation + rightVec * fScalar * 1.01f;
        ppov_2->fCostFactor = 1.0f;
        double Speed = pRequest->fActorRadius;
        ppov_2->fRadius = Speed;
        ppov_2->iOtherSideIndex = -1;
        allPOVs.Add(ppov_2);
        AvoidNodePOVs.Add(ppov_2);
        PathSmootherPOV* v45 = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
        if (v45) {
            ppov2 = ::new (v45) PathSmootherPOV;
        }
        else {
            ppov2 = nullptr;
        }
        PathSmootherPOV* ppov2_2 = ppov2;
        ppov2->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        ppov2_2->Edge1 = NiPoint3_ZERO;
        ppov2_2->Edge2 = NiPoint3_ZERO;
        float fScalara = pRequest->fActorRadius;
        ppov2_2->Center = StartLocation - rightVec * fScalara * 1.01f;
        ppov2_2->fCostFactor = 1.0f;
        double v9 = pRequest->fActorRadius;
        ppov2_2->fRadius = v9;
        ppov2_2->iOtherSideIndex = -1;
        allPOVs.Add(ppov2_2);
        AvoidNodePOVs.Add(ppov2_2);
    }
    NavMeshEdgeLocation closestEdgeToGoal;
    uint16_t v26 = (&Triangles->GetLast())->sTriangle;
    NavMeshInfo* v10 = (&Triangles->GetLast())->pNavMesh->GetNavMeshInfo();
    PathingLocation goalLoc(GoalLocation, v10, v26);
    float afMaxRadiusa = pRequest->fActorRadius;
    if (Pathing::FindClosestOpenEdge(goalLoc, afMaxRadiusa, closestEdgeToGoal)) {
        PathSmootherPOV* ppov2_3;
        PathSmootherPOV* ppov_3;
        NiPoint3 deltaToGoal_2 = GoalLocation - closestEdgeToGoal.Location;
        NiPoint3 rightVec_2 = NiPoint3_UNIT_Z.UnitCross(deltaToGoal_2);
        PathSmootherPOV* v40 = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
        if (v40) {
            ppov_3 = ::new (v40) PathSmootherPOV;
        }
        else {
            ppov_3 = nullptr;
        }
        PathSmootherPOV* ppov_4 = ppov_3;
        ppov_3->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        ppov_4->Edge1 = NiPoint3_ZERO;
        ppov_4->Edge2 = NiPoint3_ZERO;
        float fScalarb = pRequest->fActorRadius;
        ppov_4->Center = GoalLocation + rightVec_2 * fScalarb * 1.01f;
        ppov_4->fCostFactor = 1.0f;
        double v14 = pRequest->fActorRadius;
        ppov_4->fRadius = v14;
        ppov_4->iOtherSideIndex = -1;
        allPOVs.Add(ppov_4);
        AvoidNodePOVs.Add(ppov_4);
        PathSmootherPOV* v35 = static_cast<PathSmootherPOV*>(ourMalloc(sizeof(PathSmootherPOV)));
        if (v35) {
            ppov2_3 = ::new (v35) PathSmootherPOV;
        }
        else {
            ppov2_3 = nullptr;
        }
        PathSmootherPOV* ppov2_4 = ppov2_3;
        ppov2_3->eType = PathSmootherPOV::POV_TYPE_AVOIDNODE;
        ppov2_4->Edge1 = NiPoint3_ZERO;
        ppov2_4->Edge2 = NiPoint3_ZERO;
        float fScalarc = pRequest->fActorRadius;
        ppov2_4->Center = GoalLocation - rightVec_2 * fScalarc * 1.01f;
        ppov2_4->fCostFactor = 1.0f;
        double v18 = pRequest->fActorRadius;
        ppov2_4->fRadius = v18;
        ppov2_4->iOtherSideIndex = -1;
        allPOVs.Add(ppov2_4);
        AvoidNodePOVs.Add(ppov2_4);
    }
}

bool PathSmootherPOVSearch::CheckAndAddNodeConnection(
    const AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode,
    PathSmootherPOV* apPOV,
    BSSimpleArray<AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>*, 1024>* apRetNodes
) {
    const PathSmootherPOVNode* parentPOVNode = &apNode->GetNodeData();
    const PathSmootherPOVLink* parentPOVLink = &apNode->GetParentLink();
    if (apPOV == parentPOVNode->pPOV) {
        return true;
    }
    if (apNode->pParentAStarNode && apPOV == apNode->pParentAStarNode->GetNodeData().pPOV) {
        return true;
    }
    NiPoint3 vecToNode = apPOV->Center - parentPOVNode->pPOV->Center;
    float fxySqrDist = vecToNode.SqrXYLength();
    float fzSqrDist = vecToNode.z * vecToNode.z;
    if (!bUnderWaterPath && fzSqrDist > fxySqrDist * 1.2000000476837158 * 1.2000000476837158) {
        return true;
    }
    Sphere parentPOVSphere;
    parentPOVSphere.~Sphere();
    ::new (&parentPOVSphere) Sphere(parentPOVNode->pPOV->Center, parentPOVNode->pPOV->fRadius);
    Sphere thisPOVSphere;
    thisPOVSphere.~Sphere();
    ::new (&thisPOVSphere) Sphere(apPOV->Center, apPOV->fRadius);
    NavMeshMath::CCTResults tangentResults;
    int32_t iValidSideCount =
        NavMeshMath::ComputeTangentPoints(parentPOVSphere, thisPOVSphere, parentPOVNode->Direction, tangentResults);
    // The start node: when its point lies inside this POV's circle, both sides leave along the line from the circle's
    // centre.
    if (!apNode->pParentAStarNode) {
        NiPoint3 delta = parentPOVNode->pPOV->Center - apPOV->Center;
        if (apPOV->fRadius * apPOV->fRadius > delta.SqrLength()) {
            delta.z = 0.0f;
            float fdist = delta.UnitizeGetLength();
            // Both start uninitialised, because the empty NiPoint3 constructor stores nothing.
            NiPoint3 ccwPoint;
            NiPoint3 cwPoint;
            if (!(fdist < apPOV->fRadius * 0.5)) {
                float fccwVecSqrLength = apPOV->fRadius * apPOV->fRadius - fdist * fdist;
                float fccwVecLength = std::sqrt(fccwVecSqrLength);
                NiPoint3 ccwVec(-delta.y, delta.x, 0.0f);
                ccwVec *= fccwVecLength;
                ccwPoint = delta * fdist + ccwVec;
                ccwPoint.Unitize();
                cwPoint = delta * fdist - ccwVec;
                cwPoint.Unitize();
            }
            else {
                ccwPoint = delta;
                cwPoint = delta;
            }
            tangentResults.eType = NavMeshMath::CCT_TWOPOINTS;
            tangentResults.Results[0].Direction = AD_CCW;
            tangentResults.Results[0].Circle1Param.fCos = ccwPoint.x;
            tangentResults.Results[0].Circle1Param.fSin = ccwPoint.y;
            tangentResults.Results[0].Circle2Param.fCos = ccwPoint.x;
            tangentResults.Results[0].Circle2Param.fSin = ccwPoint.y;
            tangentResults.Results[1].Direction = AD_CW;
            tangentResults.Results[1].Circle1Param.fCos = cwPoint.x;
            tangentResults.Results[1].Circle1Param.fSin = cwPoint.y;
            tangentResults.Results[1].Circle2Param.fCos = cwPoint.x;
            tangentResults.Results[1].Circle2Param.fSin = cwPoint.y;
            iValidSideCount = 2;
        }
    }
    if (!iValidSideCount) {
        return true;
    }
    int32_t itestedCount = 0;
    int32_t ifailedCount = 0;
    for (int32_t e = 0; e < iValidSideCount; ++e) {
        // A vertex is only passed on its own side.
        if (apPOV->eType == PathSmootherPOV::POV_TYPE_LEFT_VERTEX && tangentResults.Results[e].Direction == AD_CW) {
            continue;
        }
        if (apPOV->eType == PathSmootherPOV::POV_TYPE_RIGHT_VERTEX && tangentResults.Results[e].Direction == AD_CCW) {
            continue;
        }
        // Around a vertex, the path may not turn back by more than 200 degrees.
        if (apNode->pParentAStarNode &&
            (apNode->pParentAStarNode->tNodeData.pPOV->eType == PathSmootherPOV::POV_TYPE_LEFT_VERTEX ||
             apNode->pParentAStarNode->tNodeData.pPOV->eType == PathSmootherPOV::POV_TYPE_RIGHT_VERTEX)) {
            AngleSinCos angleDiff = tangentResults.Results[e].Circle1Param - parentPOVLink->EntryAngle;
            if (parentPOVNode->Direction) {
                float fangleDiff = angleDiff.GetAngle();
                if (fangleDiff > 0.0) {
                    fangleDiff = fangleDiff - 6.2831854820251465;
                }
                if (fangleDiff < -3.4906585216522217) {
                    continue;
                }
            }
            else {
                float fangleDiff = angleDiff.GetAngle();
                if (fangleDiff < 0.0) {
                    fangleDiff = fangleDiff + 6.2831854820251465;
                }
                if (fangleDiff > 3.4906585216522217) {
                    continue;
                }
            }
        }
        AngleSinCosArc arcToTangent(
            parentPOVLink->EntryAngle, tangentResults.Results[e].Circle1Param, parentPOVNode->Direction
        );
        FastNavMeshEdgeLocation parentEntryLocation;
        if (apNode->pParentAStarNode) {
            parentEntryLocation.Copy(parentPOVLink->EntryLocation);
        }
        else {
            parentEntryLocation.pNavMesh = Triangles->GetFirst().pNavMesh;
            parentEntryLocation.usTriangle = Triangles->GetFirst().sTriangle;
            parentEntryLocation.iEdgeIndex = -1;
            parentEntryLocation.Location = StartLocation;
        }
        ++itestedCount;
        FastNavMeshEdgeLocation exitLocation;
        if (apNode->pParentAStarNode) {
            // The walk around the parent's circle to the tangent's exit point.
            parentEntryLocation.iEdgeIndex = -1;
            AngleSinCosArc arcArgument(&arcToTangent);
            if (!CheckArcAgainstNavMesh(parentEntryLocation, parentPOVSphere, arcArgument, &exitLocation)) {
                ++ifailedCount;
                continue;
            }
            NiPoint3 exitOffset = exitLocation.Location - parentPOVSphere.Center;
            if (std::fabs(exitOffset.z) >= 180.0) {
                ++ifailedCount;
                continue;
            }
        }
        else {
            exitLocation.Copy(parentEntryLocation);
        }
        // The tangent segment from the exit point to this POV's circle.
        FastNavMeshEdgeLocation entryLocation;
        NiPoint3 entryLoc =
            thisPOVSphere.Center + tangentResults.Results[e].Circle2Param.ComputePoint3(thisPOVSphere.fRadius);
        exitLocation.iEdgeIndex = -1;
        float fpreferredPercent = 0.0f;
        if (!TripleCheckSegmentAgainstNavMesh(
                exitLocation, entryLoc, &entryLocation, parentPOVNode->pPOV->eType == PathSmootherPOV::POV_TYPE_START,
                apPOV->eType == PathSmootherPOV::POV_TYPE_GOAL, fpreferredPercent
            )) {
            ++ifailedCount;
            continue;
        }
        if (apNode->pParentAStarNode) {
            parentEntryLocation.iEdgeIndex = -1;
            if (!bUnderWaterPath) {
                AngleSinCosArc arcArgument2(&arcToTangent);
                if (!DoubleCheckArcAgainstNavMesh(parentEntryLocation, parentPOVSphere, arcArgument2)) {
                    ++ifailedCount;
                    continue;
                }
            }
        }
        // The entry point's height on its triangle: above water it may not drop more than 64 units below this POV's
        // centre, and nowhere rise 180 or more above it.
        NiPoint3 locOnTriangle = entryLocation.Location;
        entryLocation.pNavMesh->IsPointInsideTriangle_0(
            entryLocation.usTriangle, &entryLocation.Location, 3.4028235e+38f, 3.4028235e+38f, &locOnTriangle.z
        );
        float fdeltaZ = locOnTriangle.z - thisPOVSphere.Center.z;
        double fWaterHeight =
            TES::pInstance->GetWaterHeight(entryLocation.Location, entryLocation.pNavMesh->GetParentCell());
        if ((!(entryLocation.Location.z < fWaterHeight) && !(fdeltaZ > -64.0)) || !(fdeltaZ < 180.0)) {
            ++ifailedCount;
            continue;
        }
        float favoidNodeSegmentFactor = 0.0f;
        if (!bUnderWaterPath &&
            !CheckSegmentAgainstAvoidNodes(
                parentPOVNode->pPOV, apPOV, exitLocation.Location, entryLocation.Location, &favoidNodeSegmentFactor
            )) {
            continue;
        }
        fpreferredPercent = fpreferredPercent * 0.8999999761581421;
        BSClamp(fpreferredPercent, 0.0f, 1.0f);
        float fcost = apNode->fCost;
        if (apNode->pParentAStarNode) {
            fcost =
                arcToTangent.GetLength(parentPOVNode->pPOV->fRadius) * apPOV->fCostFactor * (1.0 - fpreferredPercent) +
                fcost;
        }
        fcost = favoidNodeSegmentFactor * apPOV->fCostFactor * (1.0 - fpreferredPercent) + fcost;
        AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* pnode;
        switch (GetOrCreateAStarNode(GetKeyForNode(apPOV, tangentResults.Results[e].Direction), fcost, &pnode)) {
            case 0: SetNodeData(pnode, apPOV, tangentResults.Results[e].Direction); [[fallthrough]];
            case 1:
                pnode->ParentLink.ExitLocation.Copy(exitLocation);
                pnode->ParentLink.EntryLocation.Copy(entryLocation);
                pnode->ParentLink.ExitAngle = tangentResults.Results[e].Circle1Param;
                pnode->ParentLink.EntryAngle = tangentResults.Results[e].Circle2Param;
                apRetNodes->Add(pnode);
                break;
            default: break;
        }
    }
    return !itestedCount || itestedCount > ifailedCount;
}

void PathSmootherPOVSearch::SetNodeData(
    AStarNode<PathSmootherPOVNode, PathSmootherPOVLink>* apNode,
    PathSmootherPOV* apPOV,
    ANGLE_DIRECTION aDirection
) {
    (&apNode->tNodeData)->pPOV = apPOV;
    (&apNode->tNodeData)->Direction = aDirection;
    apNode->fHeuristic = (apPOV->Center - pGoalPov->Center).Length() * 0.10000000149011612;
}

// NiPointer's constructor from a raw pointer, declared in PathingAvoidNode.h. The reference goes to the array's
// NiRefObject base.
NiPointer<PathingAvoidNodeArray>*
NiPointer_PathingAvoidNodeArray_Construct(void* apMemory, PathingAvoidNodeArray* apObject) {
    NiPointer<PathingAvoidNodeArray>* pThis = static_cast<NiPointer<PathingAvoidNodeArray>*>(apMemory);
    pThis->m_pObject = apObject;
    if (pThis->m_pObject) {
        pThis->m_pObject->IncRefCount();
    }
    return pThis;
}

PathingLocation& PathingNode::GetLocation() {
    return Location;
}

void PathingNode::SetTangent(const NiPoint3& aTangent) {
    Tangent = aTangent;
}

Sphere::Sphere(const NiPoint3& a1, float a2) : Center(a1), fRadius(a2) {}

PathingNode* PathingSolution::GetLastNode() {
    if (CurrentPathingNodes.IsEmpty()) {
        return nullptr;
    }

    return static_cast<PathingNode*>(CurrentPathingNodes.GetTail());
}
