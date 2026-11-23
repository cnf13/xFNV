#include "Fallout Shared/Pathfinding/NavMeshSearchSLPoint.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestSafeStraightLine.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

NavMeshSearchSLPoint::NavMeshSearchSLPoint() {}

NavMeshSearchSLPoint::~NavMeshSearchSLPoint() {}

bool NavMeshSearchSLPoint::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
    NiPoint3 circumCenter;
    NavMesh* pmesh = apNode->tNodeData.pNavMesh;
    uint16_t itriIndex = apNode->tNodeData.sTriangle;
    NiPoint3 targetLoc = TargetLocation.GetLocation();
    NiPoint3 nodeToTargetVec = targetLoc;
    pmesh->GetCircumCenter2D(itriIndex, circumCenter);
    nodeToTargetVec -= circumCenter;
    float fdistanceSqr = nodeToTargetVec.SqrXYLength();
    NavMeshTriangle* ptriangle = pmesh->GetTriangle(itriIndex);
    uint16_t ivert0 = ptriangle->GetVert(0);
    NiPoint3 circumCenterToVert0Vec = pmesh->GetVertex(ivert0)->Location;
    circumCenterToVert0Vec -= circumCenter;
    float ftriangleRadius = circumCenterToVert0Vec.Length();
    float fradii = fMaxValidRadius + ftriangleRadius;
    float fradiiSqr = fradii * fradii;
    if (fradiiSqr >= static_cast<double>(fdistanceSqr)) {
        NiPoint3 validPoint;
        float fminCheckRadii = fMinValidRadius - ftriangleRadius;
        if (fminCheckRadii > 0.0 && fdistanceSqr < fminCheckRadii * fminCheckRadii) {
            return false;
        }
        NiPoint3 triCenter = apNode->tNodeData.GetCenter();
        if (CheckPointOnGoalCircle(triCenter, targetLoc, pmesh, validPoint)) {
            GoalLocation = validPoint;
            return true;
        }
        if (fActorRadius < static_cast<double>(ftriangleRadius)) {
            // The three vertex pointers are kept side by side so they can be indexed.
            const NavMeshVertex* apVerts[3];
            pmesh->GetTriangleVertices(itriIndex, apVerts[0], apVerts[1], apVerts[2]);
            NiPoint3 targetToVertVec = apVerts[0]->Location;
            targetToVertVec -= targetLoc;
            float fmaxDotLen = targetToVertVec.Dot(RightUnitVector);
            float fminDotLen = fmaxDotLen;
            for (int32_t j = 1; j < 3; ++j) {
                targetToVertVec = apVerts[j]->Location;
                targetToVertVec -= targetLoc;
                float fdotLen = targetToVertVec.Dot(RightUnitVector);
                if (!(fmaxDotLen < static_cast<double>(fdotLen))) {
                    if (fminDotLen > static_cast<double>(fdotLen)) {
                        fminDotLen = fdotLen;
                    }
                }
                else {
                    fmaxDotLen = fdotLen;
                }
            }
            NiPoint3 translatedStartTestPoint = TestStartPoint + RightUnitVector * fminDotLen;
            NiPoint3 translatedEndTestPoint = TestStartPoint + RightUnitVector * fmaxDotLen;
            NiPoint3 translatedTestVec = translatedEndTestPoint;
            translatedTestVec -= translatedStartTestPoint;
            float fvecLen = translatedTestVec.UnitizeGetLength();
            float fsamples = fvecLen / fActorRadius;
            float fcount = std::ceilf(fsamples);
            int32_t icount = static_cast<int32_t>(fcount);
            float fsampleDistance = fvecLen / fcount;
            float fsamplePoint = fsampleDistance * 0.5;
            NiPoint3 testPoint;
            for (int32_t i = 0; i < icount; ++i) {
                testPoint = translatedTestVec * fsamplePoint;
                testPoint += translatedStartTestPoint;
                if (CheckPointOnGoalCircle(testPoint, targetLoc, pmesh, validPoint)) {
                    GoalLocation = validPoint;
                    return true;
                }
                fsamplePoint = fsamplePoint + fsampleDistance;
            }
        }
    }
    return false;
}

void NavMeshSearchSLPoint::GetGoalLocation(NiPoint3& aOut) const {
    aOut = GoalLocation;
}

bool NavMeshSearchSLPoint::FindStraightLinePath(
    PathingRequestSafeStraightLine* apRequest,
    NavMeshSearchNode& aStartNode,
    NavMeshSearchNode& aGoalNode
) {
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    TargetLocation = apRequest->Destination;
    fTargetRadius = apRequest->fTargetRadius;
    double dMaxDistance = apRequest->fMaxDistance;
    fMaxValidRadius = apRequest->fTargetRadius + dMaxDistance;
    double dMinDistance = apRequest->fMinDistance;
    fMinValidRadius = apRequest->fTargetRadius + dMinDistance;
    fActorRadius = apRequest->fActorRadius;
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    nodes_t nodes(32u, 0u, static_cast<ScrapHeap*>(nullptr));
    links_t links(32u, 0u, static_cast<ScrapHeap*>(nullptr));
    return BuildNodePath(apRequest, aStartNode, aGoalNode, &nodes, &links, nullptr);
}

bool NavMeshSearchSLPoint::BuildPathToSafeStraightLine(
    PathingRequestSafeStraightLine* apRequest,
    NavMeshSearchNode& aStartNode,
    NavMeshSearchNode& aGoalNode,
    BSSimpleArray<NavMeshSearchNode, 1024>* apNodes,
    BSSimpleArray<NavMeshSearchLink, 1024>* apLinks
) {
    TargetLocation = apRequest->GetDestination();
    fTargetRadius = apRequest->GetTargetRadius();
    double dMaxDistance = apRequest->GetMaxDistance();
    fMaxValidRadius = apRequest->GetTargetRadius() + dMaxDistance;
    double dMinDistance = apRequest->GetMinDistance();
    fMinValidRadius = apRequest->GetTargetRadius() + dMinDistance;
    fActorRadius = apRequest->GetActorRadius();
    return BuildNodePath(apRequest, aStartNode, aGoalNode, apNodes, apLinks, nullptr);
}

bool NavMeshSearchSLPoint::CheckPointOnGoalCircle(
    NiPoint3& aCheck,
    NiPoint3& aTarget,
    NavMesh* apMesh,
    NiPoint3& aOutValidPoint
) {
    NiPoint3 kEnd;
    NiPoint2 kDir2D;
    NiPoint3 kToTarget;
    uint16_t arTriangle[3];
    float arZDiff;
    NiPoint3 arLocatiom;
    NiPoint3 pt;
    NiPoint3 kDir = aCheck;
    kDir -= aTarget;
    float fLength = kDir.Length();
    if (fLength < 9.999999747378752e-05) {
        kDir = aCheck;
        kDir.x = kDir.x + 10.0;
        kDir -= aTarget;
        fLength = kDir.Length();
    }
    float fLocationScale = fMaxValidRadius / fLength;
    float fStartScale = (fMaxValidRadius + fActorRadius) / fLength;
    arLocatiom = kDir * fLocationScale;
    TestStartPoint = kDir * fStartScale;
    NiPoint3 aLocation = aTarget;
    arLocatiom += aTarget;
    TestStartPoint += aTarget;
    RightUnitVector = kDir.UnitCross(NiPoint3::UNIT_Z);
    pt = RightUnitVector * fActorRadius;
    PathingLocation aStartLocation(&TestStartPoint, apMesh->GetNavMeshInfo());
    PathingLocation aGoalLocation(aLocation, TargetLocation);
    if (!Pathing::CheckLineOfSight(
            aStartLocation,
            aGoalLocation,
            static_cast<PathingLocation*>(nullptr),
            static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        )) {
        return false;
    }
    PathingLocation kLeftStart(TestStartPoint - pt, aStartLocation);
    PathingLocation kLeftGoal(aLocation - pt, aGoalLocation);
    if (!Pathing::CheckLineOfSight(
            kLeftStart,
            kLeftGoal,
            static_cast<PathingLocation*>(nullptr),
            static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        )) {
        return false;
    }
    PathingLocation kRightStart(TestStartPoint + pt, aStartLocation);
    PathingLocation kRightGoal(aLocation + pt, aGoalLocation);
    if (!Pathing::CheckLineOfSight(
            kRightStart,
            kRightGoal,
            static_cast<PathingLocation*>(nullptr),
            static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        )) {
        return false;
    }
    PathingLocation kLocation(arLocatiom, aStartLocation);
    NavMeshPtr spNavMesh;
    if (!kLocation.GetNavMeshAndTriangle(spNavMesh, arTriangle[0])) {
        return false;
    }
    spNavMesh->IsPointInsideTriangle(arTriangle[0], arLocatiom, arZDiff, 180.0f, 180.0f);
    if (arZDiff < 0.0 && aLocation.z - TestStartPoint.z > 0.0) {
        float fScale;
        float fHeightDiff;
        float fReachSqr;
        float fLength2D;
        float fReach;
        float fHorizontal;
        float fDistance;
        kToTarget = aTarget;
        kToTarget -= arLocatiom;
        fDistance = kToTarget.Length();
        fReach = fDistance - fTargetRadius - fActorRadius;
        fScale = fReach / fDistance;
        kToTarget *= fScale;
        kEnd = arLocatiom + kToTarget;
        kDir2D = NiPoint2(kToTarget.x, kToTarget.y);
        fLength2D = kDir2D.Unitize();
        fReachSqr = fReach * fReach;
        fHeightDiff = kEnd.z - (arLocatiom.z + arZDiff);
        fHeightDiff = fHeightDiff * fHeightDiff;
        if (fReachSqr < static_cast<double>(fHeightDiff)) {
            return false;
        }
        fHorizontal = fReachSqr - fHeightDiff;
        fHorizontal = NiSqrt(fHorizontal);
        if (fLength2D <= static_cast<double>(fHorizontal)) {
            return false;
        }
        float fShift = fLength2D - fHorizontal;
        kDir2D *= fShift;
        arLocatiom.x = arLocatiom.x + kDir2D.x;
        arLocatiom.y = arLocatiom.y + kDir2D.y;
    }
    arLocatiom.z = arLocatiom.z + arZDiff;
    aOutValidPoint = arLocatiom;
    return true;
}

NiPoint2& NiPoint2::operator*=(float fScalar) {
    x = x * fScalar;
    y = y * fScalar;
    return *this;
}
