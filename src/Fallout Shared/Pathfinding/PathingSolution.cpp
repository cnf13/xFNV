#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingSpline.h"
#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstdint>

// Returns the raw byte.
uint8_t PathingSolution::IsIncomplete() {
    return bIncompletePath;
}

PathingSolution* PathingSolution::Clear() {
    VirtualPathingNodes.Clear(true);
    CurrentPathingNodes.Clear(true);
    DoorIDs.Clear(true);
    iFirstLoadedVirtualNodeIndex = -1;
    iLastLoadedVirtualNodeIndex = -1;
    bIncompletePath = 0;
    return this;
}

float PathingSolution::ComputeDetailedPathLength() {
    NiPoint3 result;
    float ftotalLength = 0.0f;
    uint32_t isize = GetNodeCount();
    for (uint32_t i = 1; i < isize; ++i) {
        PathingNode* plastNode = GetNode(i - 1);
        PathingNode* pnextNode = GetNode(i);
        NiPoint3* Location_ = ::new (&result) NiPoint3(plastNode->GetLocation().GetLocation());
        ftotalLength = (pnextNode->GetLocation().GetLocation() - *Location_).Length() + ftotalLength;
    }
    return ftotalLength;
}

// The parameter is truncated to the node index; the index, as an unsigned integer, is subtracted in double precision
// and the fraction is rounded to float. The index compare against the last node is signed.
void PathingSolution::ComputeLocationFromParameter(float afCurrentSplineParameter, NiPoint3& aOutLoc) {
    int32_t iParameter = static_cast<int32_t>(afCurrentSplineParameter);
    float fFraction =
        static_cast<float>(afCurrentSplineParameter - static_cast<double>(static_cast<uint32_t>(iParameter)));
    int32_t iLastNode = GetNodeCount() - 1;
    if (iParameter >= iLastNode) {
        aOutLoc = GetNode(iLastNode)->GetLocation().GetLocation();
        return;
    }
    PathingSpline kSpline;
    kSpline.SetParameter(fFraction);
    PathingSplineKey kKey2 = GetNode(iParameter + 1)->GetSplineKey();
    PathingSplineKey kKey1 = GetNode(iParameter)->GetSplineKey();
    aOutLoc = kSpline.Interpolate(kKey1, kKey2);
}

// The running length is a float local, each spline's approximate length added to it in float. A parameter at or past
// the last node (compared against the zero-extended count less one) is the whole straight-line path length; NaN takes
// the spline path.
float PathingSolution::ComputeDistanceToParameter(float afParameter) {
    if (!GetNodeCount()) {
        return 0.0f;
    }
    if (afParameter >= static_cast<double>(GetNodeCount() - 1)) {
        return ComputeDetailedPathLength();
    }
    int32_t iParameter = static_cast<int32_t>(afParameter);
    float fDistance = 0.0f;
    PathingSpline kSpline;
    for (int32_t i = 0; i < iParameter; ++i) {
        PathingSplineKey kKey2 = GetNode(i + 1)->GetSplineKey();
        PathingSplineKey kKey1 = GetNode(i)->GetSplineKey();
        kSpline.SetKeys(kKey1, kKey2);
        fDistance = kSpline.ApproxLength(0.0f, 1.0f, 2) + fDistance;
    }
    PathingSplineKey kKey2 = GetNode(iParameter + 1)->GetSplineKey();
    PathingSplineKey kKey1 = GetNode(iParameter)->GetSplineKey();
    kSpline.SetKeys(kKey1, kKey2);
    float fFraction = static_cast<float>(afParameter - static_cast<double>(iParameter));
    fDistance = kSpline.ApproxLength(0.0f, fFraction, 2) + fDistance;
    // kSpline's Destruct runs at the end of its scope, after the sum is final.
    return fDistance;
}

// Every length and the parameter are kept as float; the sum the target is compared against and the return value are
// rounded to float too.
float PathingSolution::GetSplineParametersFromPercentage(
    float afPathPercent,
    float afPathLength,
    uint32_t& aiOutNodeIndex
) {
    BSClamp(afPathPercent, 0.0f, 1.0f);
    float fTargetLength = afPathPercent * afPathLength;
    float fLength = 0.0f;
    bool bFound = false;
    aiOutNodeIndex = 0;
    float fParameter = 0.0f;
    for (uint32_t i = 0; i < GetNodeCount() - 1; ++i) {
        float fSplineLength = ComputeSplineLength(i);
        if (fTargetLength <= fLength + fSplineLength) {
            aiOutNodeIndex = i;
            // The parameter is computed in a double temporary before the float store.
            double dParameter;
            if (fSplineLength == 0.0) {
                dParameter = 0.0;
            }
            else {
                dParameter = (fTargetLength - fLength) / fSplineLength;
            }
            fParameter = static_cast<float>(dParameter);
            bFound = true;
            break;
        }
        fLength = fLength + fSplineLength;
    }
    if (!bFound) {
        aiOutNodeIndex = GetNodeCount() - 2;
        fParameter = 1.0f;
    }
    // The index is converted exactly to double before the parameter is added.
    return static_cast<float>(static_cast<double>(aiOutNodeIndex) + fParameter);
}

// The parameter is compared against the zero-extended count less one at x87 precision (NaN takes the spline path); the
// node index is truncated and the index compare is signed. The remaining distance, the start parameter and each spline
// length are float stack slots.
NiPoint3 PathingSolution::ComputeApproxLocationForDistance(float afCurrentParameter, float afDistanceTraveled) {
    if (!GetNodeCount()) {
        return NiPoint3_ZERO;
    }
    if (afCurrentParameter >= static_cast<double>(GetNodeCount() - 1)) {
        return GetLastNode()->GetLocation().GetLocation();
    }
    float fDistance = afDistanceTraveled;
    int32_t iIndex = static_cast<int32_t>(afCurrentParameter);
    float fStartParameter = static_cast<float>(afCurrentParameter - static_cast<double>(iIndex));
    PathingSplineKey kKey2 = GetNode(iIndex + 1)->GetSplineKey();
    PathingSplineKey kKey1 = GetNode(iIndex)->GetSplineKey();
    alignas(PathingSpline) uint8_t aSplineStorage[sizeof(PathingSpline)];
    PathingSpline& kSpline = *::new (aSplineStorage) PathingSpline;
    kSpline.SetKeys(kKey1, kKey2);
    float fSplineLength = kSpline.ApproxLength(fStartParameter, 1.0f, 2);
    while (iIndex < static_cast<int32_t>(GetNodeCount() - 2) && fDistance > fSplineLength) {
        fDistance = fDistance - fSplineLength;
        ++iIndex;
        kKey1 = kKey2;
        kKey2 = GetNode(iIndex + 1)->GetSplineKey();
        kSpline.SetKeys(kKey1, kKey2);
        fStartParameter = 0.0f;
        fSplineLength = kSpline.ApproxLength(0.0f, 1.0f, 2);
    }
    if (fDistance < fSplineLength) {
        NiPoint3 kPoint;
        kPoint.~NiPoint3();
        ::new (&kPoint) NiPoint3;
        float fReturnParameter;
        if (!kSpline.FindPointOnSpline(fStartParameter, fDistance, true, &fReturnParameter, &kPoint, 10.0f)) {
            kPoint = kKey2.Location;
        }
        NiPoint3 kResult = kPoint;
        kSpline.~PathingSpline();
        return kResult;
    }
    NiPoint3 kResult = GetLastNode()->GetLocation().GetLocation();
    kSpline.~PathingSpline();
    return kResult;
}

uint32_t PathingSolution::GetCount() {
    return static_cast<uint32_t>(iFirstLoadedVirtualNodeIndex);
}

void PathingSolution::CopyTo(PathingSolution* apDest) {
    apDest->iFirstLoadedVirtualNodeIndex = iFirstLoadedVirtualNodeIndex;
    apDest->iLastLoadedVirtualNodeIndex = iLastLoadedVirtualNodeIndex;
    apDest->bIncompletePath = bIncompletePath;
    VirtualPathingNodes.CopyTo(&apDest->VirtualPathingNodes);
    CurrentPathingNodes.CopyTo(&apDest->CurrentPathingNodes);
}

bool PathingSolution::FindPointOnPath(
    float afCurrentParameter,
    float afDist,
    bool abForward,
    float* apNewParameter,
    NiPoint3* apNewPoint
) {
    if (!GetNodeCount()) {
        return false;
    }
    if (abForward) {
        int32_t isize = GetNodeCount() - 1;
        if (!(afCurrentParameter >= static_cast<double>(isize))) {
            int32_t icurrentSpline = static_cast<int32_t>(afCurrentParameter);
            float fcurrentParameter = afCurrentParameter;
            float fdist = afDist;
            PathingSpline spline;
            PathingNode* v8 = &CurrentPathingNodes.GetAt(icurrentSpline + 1);
            PathingSplineKey kNextKey = v8->GetSplineKey();
            PathingNode* v9 = &CurrentPathingNodes.GetAt(icurrentSpline);
            spline.SetKeys(v9->GetSplineKey(), kNextKey);
            float afStartParameter = fcurrentParameter - static_cast<double>(icurrentSpline);
            for (bool bfound =
                     spline.FindPointOnSpline(afStartParameter, fdist, abForward, apNewParameter, apNewPoint, 1.0f);
                 !bfound; bfound = spline.FindPointOnSpline(0.0f, fdist, abForward, apNewParameter, apNewPoint, 1.0f)) {
                float afStartParam = fcurrentParameter - static_cast<double>(icurrentSpline);
                double v11 = spline.ApproxLength(afStartParam, 1.0f, 2u);
                fdist = fdist - v11;
                fcurrentParameter = static_cast<float>(++icurrentSpline);
                if (icurrentSpline >= isize - 1) {
                    if (apNewParameter) {
                        *apNewParameter = static_cast<float>(isize);
                    }
                    if (apNewPoint) {
                        PathingNode* LastNode = GetLastNode();
                        *apNewPoint = (&LastNode->Location)->GetLocation();
                    }
                    return false;
                }
                PathingNode* v13 = &CurrentPathingNodes.GetAt(icurrentSpline + 1);
                PathingSplineKey kNextKeyA = v13->GetSplineKey();
                PathingNode* v14 = &CurrentPathingNodes.GetAt(icurrentSpline);
                spline.SetKeys(v14->GetSplineKey(), kNextKeyA);
            }
            if (apNewParameter) {
                *apNewParameter = static_cast<double>(icurrentSpline) + *apNewParameter;
            }
            return true;
        }
        else {
            if (apNewParameter) {
                *apNewParameter = static_cast<float>(isize);
            }
            if (apNewPoint) {
                PathingNode* v7 = GetLastNode();
                *apNewPoint = (&v7->Location)->GetLocation();
            }
            return false;
        }
    }
    else {
        int32_t isize_2 = GetNodeCount() - 1;
        if (!(afCurrentParameter <= 0.0)) {
            int32_t icurrentSpline_2 = static_cast<int32_t>(afCurrentParameter);
            if (icurrentSpline_2 < 0) {
                icurrentSpline_2 = 0;
            }
            // The spline index is compared with the node count less one as signed integers.
            if (icurrentSpline_2 >= static_cast<int32_t>(CurrentPathingNodes.GetSize() - 1)) {
                --icurrentSpline_2;
            }
            float fcurrentParameter_2 = afCurrentParameter;
            float fdist_2 = afDist;
            PathingSpline spline_2;
            PathingNode* v17 = &CurrentPathingNodes.GetAt(icurrentSpline_2 + 1);
            PathingSplineKey kNextKeyB = v17->GetSplineKey();
            PathingNode* v18 = &CurrentPathingNodes.GetAt(icurrentSpline_2);
            spline_2.SetKeys(v18->GetSplineKey(), kNextKeyB);
            float v30 = fcurrentParameter_2 - static_cast<double>(icurrentSpline_2);
            for (bool bfound_2 = spline_2.FindPointOnSpline(v30, fdist_2, false, apNewParameter, apNewPoint, 1.0f);
                 !bfound_2;
                 bfound_2 = spline_2.FindPointOnSpline(1.0f, fdist_2, false, apNewParameter, apNewPoint, 1.0f)) {
                float afEndParam = fcurrentParameter_2 - static_cast<double>(icurrentSpline_2);
                double v20 = spline_2.ApproxLength(0.0f, afEndParam, 2u);
                fdist_2 = fdist_2 - v20;
                fcurrentParameter_2 = static_cast<float>(icurrentSpline_2--);
                if (icurrentSpline_2 < 0) {
                    if (apNewParameter) {
                        *apNewParameter = 0.0f;
                    }
                    if (apNewPoint) {
                        PathingNode* FirstNode = GetFirstNode();
                        *apNewPoint = (&FirstNode->Location)->GetLocation();
                    }
                    return false;
                }
                PathingNode* v22 = &CurrentPathingNodes.GetAt(icurrentSpline_2 + 1);
                PathingSplineKey kNextKeyC = v22->GetSplineKey();
                PathingNode* v23 = &CurrentPathingNodes.GetAt(icurrentSpline_2);
                spline_2.SetKeys(v23->GetSplineKey(), kNextKeyC);
            }
            if (apNewParameter) {
                *apNewParameter = static_cast<double>(icurrentSpline_2) + *apNewParameter;
            }
            return true;
        }
        if (apNewParameter) {
            *apNewParameter = 0.0f;
        }
        if (apNewPoint) {
            PathingNode* v16 = GetFirstNode();
            *apNewPoint = (&v16->Location)->GetLocation();
        }
        return false;
    }
}

// Each segment's XY length is added to the float total at x87 precision.
float PathingSolution::ComputeVirtualPathLength() {
    NiPoint3 result;
    float ftotalLength = 0.0f;
    uint32_t isize = GetVirtualNodeCount();
    for (uint32_t i = 1; i < isize; ++i) {
        VirtualPathingNode* plastNode = GetVirtualNode(i - 1);
        if (!plastNode->IsTransitionNode()) {
            VirtualPathingNode* pnextNode = GetVirtualNode(i);
            NiPoint3* Location_ = ::new (&result) NiPoint3(plastNode->GetLocation().GetLocation());
            ftotalLength = (pnextNode->GetLocation().GetLocation() - *Location_).XYLength() + ftotalLength;
        }
    }
    return ftotalLength;
}

void PathingSolution::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&bIncompletePath, 1u);
    apLoadGameBuffer->LoadData(&iFirstLoadedVirtualNodeIndex, 4u);
    apLoadGameBuffer->LoadData(&iLastLoadedVirtualNodeIndex, 4u);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    VirtualPathingNodes.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        VirtualPathingNode* virtualNode = &VirtualPathingNodes.operator[](i);
        virtualNode->LoadGame(apLoadGameBuffer);
    }
    uint32_t isaveGameCount_2 = apLoadGameBuffer->LoadVariableSizedValue();
    CurrentPathingNodes.SetSize(isaveGameCount_2, true);
    for (uint32_t i_2 = 0; i_2 < isaveGameCount_2; ++i_2) {
        PathingNode* pathingNode = &CurrentPathingNodes.operator[](i_2);
        pathingNode->LoadGame(apLoadGameBuffer);
    }
    PathingAvoidNodeArray avoidNodeArrayDummy;
    uint32_t isaveGameCount_3 = apLoadGameBuffer->LoadVariableSizedValue();
    avoidNodeArrayDummy.SetSize(isaveGameCount_3, true);
    for (uint32_t i_3 = 0; i_3 < isaveGameCount_3; ++i_3) {
        PathingAvoidNode* avoidNode = &avoidNodeArrayDummy.GetAt(i_3);
        avoidNode->LoadGame(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetVersion() >= 18u) {
        uint32_t isaveGameCount_4 = apLoadGameBuffer->LoadVariableSizedValue();
        DoorIDs.SetSize(isaveGameCount_4, true);
        for (uint32_t i_4 = 0; i_4 < isaveGameCount_4; ++i_4) {
            apLoadGameBuffer->LoadFormID(DoorIDs.operator[](i_4));
        }
    }
}
