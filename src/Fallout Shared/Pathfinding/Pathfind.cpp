#include "Fallout Shared/Pathfinding/Pathfind.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/FastNavMeshEdgeLocationProxy.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/NavMeshSearchClosePoint.h"
#include "Fallout Shared/Pathfinding/NavMeshSearchFlee.h"
#include "Fallout Shared/Pathfinding/NavMeshSearchHide.h"
#include "Fallout Shared/Pathfinding/NavMeshSearchLOS.h"
#include "Fallout Shared/Pathfinding/NavMeshSearchMaxCost.h"
#include "Fallout Shared/Pathfinding/NavMeshSearchSLPoint.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavmeshTriFan.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathSmoother.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVSearch.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingLOSMap.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingRequestCover.h"
#include "Fallout Shared/Pathfinding/PathingRequestCoveredMove.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout Shared/Pathfinding/PathingRequestHide.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/Pathfinding/PathingRequestOptimalLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestSafeStraightLine.h"
#include "Fallout Shared/Pathfinding/TeleportDoorSearch.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/Movement/DetailedActorPathHandler.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"

#include <bit>
#include <cmath>
#include <cstdint>

// The avoidance cone angle settings, in degrees.
static SettingT<INISettingCollection> fINIAvoidanceConeAngle("fAvoidanceConeAngle:Pathfinding", 45.0f);
static SettingT<INISettingCollection> fINIWiderAvoidanceConeAngle("fAvoidanceWiderConeAngle:Pathfinding", 60.0f);
static SettingT<INISettingCollection> fINIRunningAvoidanceConeAngle("fRunningWiderConeAngle:Pathfinding", 30.0f);

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fFindClosestEdgesRadius("fFindClosestEdgesRadius:Pathing", 512.0f);

using FastNavMeshEdgeLocationProxyArray = BSSimpleArray<FastNavMeshEdgeLocationProxy, 1024>;

float RandomFloatNegPiToPi() {
    BSRandom* singleton = BSRandom::GetSingleton();
    return singleton->FloatNegPiToPi();
}

// The qsort comparator of the closest-edge search: edges whose z delta is within (-64, 180) first, then by squared
// distance, nearest first.
int32_t EdgeSortFunction(const void* apEdge1, const void* apEdge2) {
    const FastNavMeshEdgeLocationProxy* pedge1 = static_cast<const FastNavMeshEdgeLocationProxy*>(apEdge1);
    const FastNavMeshEdgeLocationProxy* pedge2 = static_cast<const FastNavMeshEdgeLocationProxy*>(apEdge2);
    bool bEdge1Within = pedge1->fzDelta > -64.0 && pedge1->fzDelta < 180.0;
    bool bEdge2Within = pedge2->fzDelta > -64.0 && pedge2->fzDelta < 180.0;
    if (bEdge1Within && !bEdge2Within) {
        return -1;
    }
    if (!bEdge1Within && bEdge2Within) {
        return 1;
    }
    if (pedge2->fSqrDistToLocation > pedge1->fSqrDistToLocation) {
        return -1;
    }
    return pedge2->fSqrDistToLocation < pedge1->fSqrDistToLocation;
}

CrossedTriangle::CrossedTriangle(uint32_t aiTriangleIndex, char acTriangleNeighborCount)
  : iTriangleIndex(aiTriangleIndex), iNeighborCount(acTriangleNeighborCount) {}

Pathing::CheckLineOfSightFilter::CheckLineOfSightFilter() {}

float Pathing::ProfilePathing(TESObjectCELL* apCell, const NiPoint3& aLoc, uint32_t aiTestType, uint32_t aiNum) {
    using coverPoints_t = BSSimpleArray<PathingCoverLocation, 1024>;
    NiPoint2 temp;
    float cellMaxX;
    using randomPoints_t = BSSimpleArray<NiPoint3, 1024>;
    float cellMinX;
    float cellMaxY;
    float cellMinY;
    NavMeshPtr spnavmesh;
    NiPoint3 testPoint = NiPoint3_ZERO;
    float finteriorRange = 500.0f;
    int32_t iswitch = aiTestType;
    if (apCell->GetInterior()) {
        cellMinX = -finteriorRange;
        cellMinY = -finteriorRange;
        cellMaxX = finteriorRange;
        cellMaxY = finteriorRange;
    }
    else {
        cellMinX = static_cast<float>(apCell->GetDataX() << 12);
        cellMinY = static_cast<float>(apCell->GetDataY() << 12);
        cellMaxX = cellMinX + 4096.0;
        cellMaxY = cellMinY + 4096.0;
    }
    PathingLocation startLoc(&aLoc, apCell);
    if (startLoc.ResolveNavMeshInfo(nullptr)) {
        startLoc.GetNavMesh(spnavmesh);
        randomPoints_t randomPoints(aiNum);
        if (iswitch == 2) {
            for (uint32_t i = 0; i < aiNum; ++i) {
                if (!Pathing::FindPointOnNavMesh(
                        startLoc.pWorldSpace, nullptr, aLoc, 2000.0f, &randomPoints.GetAt(static_cast<int32_t>(i))
                    )) {
                    --i;
                }
            }
        }
        else {
            for (uint32_t i_2 = 0; i_2 < aiNum; ++i_2) {
                float fx = RandomFloat(cellMinX, cellMaxX);
                (&randomPoints.GetAt(static_cast<int32_t>(i_2)))->x = fx;
                float fy = RandomFloat(cellMinY, cellMaxY);
                (&randomPoints.GetAt(static_cast<int32_t>(i_2)))->y = fy;
                (&randomPoints.GetAt(static_cast<int32_t>(i_2)))->z = aLoc.z;
            }
        }
        for (uint32_t i_3 = 0; i_3 < aiNum; ++i_3) {
            if (iswitch) {
                if (iswitch == 1) {
                    PathingRequestCover coverRequest;
                    coverRequest.SetOrigin(startLoc);
                    coverRequest.SetThreat(randomPoints.GetAt(static_cast<int32_t>(i_3)));
                    coverPoints_t coverPoints;
                    Pathing::GetCoverPoints(&coverRequest, coverPoints);
                }
                else {
                    if (iswitch == 2) {
                        TESWorldSpace* pworld = startLoc.pWorldSpace;
                        PathingLocation goal(&randomPoints.GetAt(static_cast<int32_t>(i_3)), pworld);
                        PathingRequest request;
                        request.SetOrigin(startLoc);
                        request.SetDestination(goal);
                        PathingSolution solution;
                        Pathing::Pathfind(&request, &solution);
                    }
                }
            }
            else {
                // The four points are i to i+3, wrapped by an unsigned modulo (div) of the point count.
                uint32_t i1 = (i_3 + 1) % aiNum;
                uint32_t i2 = (i_3 + 2) % aiNum;
                uint32_t i3 = (i_3 + 3) % aiNum;
                float fy0 = (&randomPoints.GetAt(static_cast<int32_t>(i_3)))->y;
                NiPoint2 seg1Start((&randomPoints.GetAt(static_cast<int32_t>(i_3)))->x, fy0);
                float fy1 = (&randomPoints.GetAt(static_cast<int32_t>(i1)))->y;
                NiPoint2 seg1End((&randomPoints.GetAt(static_cast<int32_t>(i1)))->x, fy1);
                float fy2 = (&randomPoints.GetAt(static_cast<int32_t>(i2)))->y;
                NiPoint2 seg2Start((&randomPoints.GetAt(static_cast<int32_t>(i2)))->x, fy2);
                float fy3 = (&randomPoints.GetAt(static_cast<int32_t>(i3)))->y;
                NiPoint2 seg2End((&randomPoints.GetAt(static_cast<int32_t>(i3)))->x, fy3);
                bool bfirst = NavMeshMath::Find2DIntersection_006B9E90(seg1Start, seg1End, seg2Start, seg2End, &temp);
                bool bsecond = NavMeshMath::Find2DIntersection(seg1Start, seg1End, seg2Start, seg2End);
                if (bfirst != bsecond) {
                    Warning("PATHFINDING: Different");
                }
            }
        }
        return 0.0f;
    }
    else {
        return 0.0f;
    }
}

bool Pathing::BuildLowPath(
    PathingLocation& aOrigin,
    PathingLocation& aDestination,
    TeleportPath* apRetPath,
    ActorLockData* apActorLockData,
    Pathing::DOOR_LOCK_DISPOSITION aeDoorLockDisp
) {
    if (aOrigin.ResolveNavMeshInfo(nullptr) && aDestination.ResolveNavMeshInfo(nullptr)) {
        using NavMeshInfoArray = BSSimpleArray<const NavMeshInfo*, 1024>;
        using DoorArray = BSSimpleArray<TESObjectREFR*, 1024>;
        NavMeshInfoSearch search;
        NavMeshInfoArray infos;
        DoorArray doors;
        // The destination's arguments are evaluated before the origin's.
        const NavMeshInfo* pDestInfo = aDestination.pNavMesh;
        NiPoint3 destLoc = aDestination.GetLocation();
        const NavMeshInfo* pOriginInfo = aOrigin.pNavMesh;
        if (search.Pathfind(
                apActorLockData, aOrigin.GetLocation(), pOriginInfo, destLoc, pDestInfo, &infos, &doors, aeDoorLockDisp
            )) {
            ParentSpaceNode firstSpace;
            firstSpace.pWorldSpace = infos.GetAt(0)->GetWorldSpace();
            firstSpace.pInteriorCell = infos.GetAt(0)->GetInteriorCell();
            firstSpace.bWorldSpace = infos.GetAt(0)->GetWorldSpace() != nullptr;
            apRetPath->Spaces.Add(firstSpace);
            for (uint32_t uiIndex = 0; uiIndex < doors.iSize; ++uiIndex) {
                if (doors.GetAt(static_cast<int32_t>(uiIndex))) {
                    TeleportLink link;
                    link.pDoor = doors.GetAt(static_cast<int32_t>(uiIndex));
                    link.DoorTeleportLocation = doors.GetAt(static_cast<int32_t>(uiIndex))->GetTeleport()->position;
                    apRetPath->Doors.Add(link);
                    ParentSpaceNode space;
                    space.pWorldSpace = infos.GetAt(static_cast<int32_t>(uiIndex + 1))->GetWorldSpace();
                    space.pInteriorCell = infos.GetAt(static_cast<int32_t>(uiIndex + 1))->GetInteriorCell();
                    space.bWorldSpace = infos.GetAt(static_cast<int32_t>(uiIndex + 1))->GetWorldSpace() != nullptr;
                    apRetPath->Spaces.Add(space);
                }
            }
            apRetPath->Start = aOrigin.GetLocation();
            apRetPath->End = aDestination.GetLocation();
            return true;
        }
    }
    TeleportDoorSearch teleSearch;
    bool bResult = teleSearch.BuildNodePath(
        aOrigin, aDestination, apRetPath, Pathing::TELEPORT_DOOR_PATH_NONE, 0.0f, apActorLockData, aeDoorLockDisp
    );
    return bResult;
}

bool Pathing::GetNavMeshZForLocation(PathingLocation& aLocation, float& aRetHeight) {
    NiPoint3 v7;
    FNVD_INLINED_LOCAL(NiPointer<NavMesh>, apMemory, std::nullopt, std::nullopt);
    uint16_t arTriangle;
    if (aLocation.ResolveNavMeshInfo(nullptr)) {
        if (aLocation.HasNavMeshData()) {
            ::new (&apMemory) NavMeshPtr;
            if (aLocation.GetNavMeshAndTriangle(*static_cast<NavMeshPtr*>(&apMemory), arTriangle)) {
                NiPoint3* Location_ = &(v7 = aLocation.GetLocation());
                uint16_t v4 = arTriangle;
                NavMesh* v3 = apMemory.m_pObject;
                if (v3->IsPointInsideTriangle_0(v4, Location_, 180.0f, 180.0f, &aRetHeight)) {
                    apMemory.~NiPointer<NavMesh>();
                    return true;
                }
                else {
                    NiPoint3 v6;
                    aRetHeight = (&(v6 = aLocation.GetLocation()))->z;
                    apMemory.~NiPointer<NavMesh>();
                    return false;
                }
            }
            else {
                NiPoint3 v8;
                aRetHeight = (&(v8 = aLocation.GetLocation()))->z;
                apMemory.~NiPointer<NavMesh>();
                return false;
            }
        }
        else {
            NiPoint3 v9;
            aRetHeight = (&(v9 = aLocation.GetLocation()))->z;
            return false;
        }
    }
    else {
        NiPoint3 result;
        aRetHeight = (&(result = aLocation.GetLocation()))->z;
        return false;
    }
}

bool Pathing::GetNavmeshNormalForLocation(PathingLocation& aLocation, NiPoint3& aRetNormal) {
    NiPoint3 result;
    uint16_t arTriangle;
    FNVD_INLINED_LOCAL(NiPointer<NavMesh>, apMemory, std::nullopt, std::nullopt);
    if (!aLocation.ResolveNavMeshInfo(nullptr)) {
        return false;
    }
    ::new (&apMemory) NavMeshPtr;
    if (aLocation.GetNavMeshAndTriangle(*static_cast<NavMeshPtr*>(&apMemory), arTriangle)) {
        uint16_t v4 = arTriangle;
        NavMesh* v3 = apMemory.m_pObject;
        aRetNormal = *v3->GetNormal(&result, v4);
        apMemory.~NiPointer<NavMesh>();
        return true;
    }
    else {
        apMemory.~NiPointer<NavMesh>();
        return false;
    }
}

bool Pathing::FindConnectedNavmeshesForLocation(
    PathingLocation& aLocation,
    float afRadius,
    BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes
) {
    return (bool)((int8_t)Pathing::FindNavmeshesForLocation(aLocation, afRadius, apRetNavMeshes, true));
}

// Each candidate edge's PathingCoverLocation is constructed and destroyed in place per edge.
int32_t
Pathing::GetCoverPoints(PathingRequestCover* apRequest, BSSimpleArray<PathingCoverLocation, 1024>& apReturnArray) {
    uint32_t inum = 0;
    apReturnArray.SetReservedSize(apRequest->iMaxResults);
    BSScrapArray<float> distanceArray(32u, 0u, nullptr);
    NiPoint3 actorLocation = apRequest->ActorLocation;
    NiPoint3 center = apRequest->Origin.GetLocation();
    bool buniqueCenter = true;
    if (actorLocation == NiPoint3_ZERO) {
        actorLocation = center;
        buniqueCenter = false;
    }
    NiPoint3 targetLocation = apRequest->GetThreat();
    NiPoint2 threatPos2D(targetLocation.x, targetLocation.y);
    NiPoint2 threatToActor2D(actorLocation.x - threatPos2D.x, actorLocation.y - threatPos2D.y);
    uint16_t crouchHeight =
        static_cast<uint16_t>(NavMeshTriangle::GetCoverLevel(apRequest->fActorCrouchHeight, false, true));
    uint16_t standHeight = static_cast<uint16_t>(NavMeshTriangle::GetCoverLevel(apRequest->fActorHeight, false, true));
    float fsquaredMaxDistance = apRequest->fMaxCoverDistance * apRequest->fMaxCoverDistance;
    BSScrapArray<NavMeshPtr> navmeshes(8u, 0u, nullptr);
    Pathing::FindConnectedNavmeshesForLocation(apRequest->Origin, apRequest->fMaxCoverDistance, &navmeshes);
    NavMeshPtr spmesh;
    bool bcanCrouch = apRequest->bCanCrouch;
    bool bhideCover = apRequest->bHideCover;
    for (uint32_t imesh = 0; imesh < navmeshes.iSize; ++imesh) {
        spmesh = navmeshes.GetAt(static_cast<int32_t>(imesh));
        uint16_t itriangles = static_cast<uint16_t>(spmesh->CoverArray.iSize);
        for (uint16_t i = 0; i < itriangles; ++i) {
            uint16_t itriIndex = spmesh->CoverArray[i];
            NavMeshTriangle* ptri = spmesh->GetTriangle(itriIndex);
            if (!ptri->HasCover()) {
                continue;
            }
            NiPoint3 triangleCenter = spmesh->GetCenter(itriIndex);
            NiPoint3 actorDistance = actorLocation - triangleCenter;
            float fsquaredDist = actorDistance.SqrLength();
            if (buniqueCenter) {
                NiPoint3 centerVec = center - triangleCenter;
                if (fsquaredMaxDistance < centerVec.SqrLength()) {
                    continue;
                }
            }
            else if (fsquaredMaxDistance < fsquaredDist) {
                continue;
            }
            NiPoint3 distanceProjectionVector = apRequest->DistanceProjectionVector;
            if (apRequest->bSortDistanceFromTarget) {
                NiPoint3 targetDistance = targetLocation - triangleCenter;
                fsquaredDist = targetDistance.SqrLength();
                if (distanceProjectionVector != NiPoint3_ZERO) {
                    float fdot = targetDistance.Dot(distanceProjectionVector);
                    double fRatio = fdot * fdot / fsquaredDist;
                    fsquaredDist = fRatio + fRatio + fsquaredDist;
                }
            }
            else if (distanceProjectionVector != NiPoint3_ZERO) {
                float fdot = actorDistance.Dot(distanceProjectionVector);
                double fRatio = fdot * fdot / fsquaredDist;
                fsquaredDist = fRatio + fRatio + fsquaredDist;
            }
            float fHeightChange = std::fabs(actorDistance.z);
            if (apRequest->fMaxHeightChange < fHeightChange) {
                continue;
            }
            float ffacing = (triangleCenter.x - threatPos2D.x) * threatToActor2D.x +
                (triangleCenter.y - threatPos2D.y) * threatToActor2D.y;
            if (apRequest->bFindBehindTarget) {
                ffacing = -ffacing;
            }
            if (!apRequest->bFindBothSides && !(ffacing > 0.0)) {
                continue;
            }
            NiPoint3 kNormal;
            NiPoint3 triNormal = *spmesh->GetNormal(&kNormal, itriIndex);
            for (uint16_t iedge = 0; iedge < 2; ++iedge) {
                uint16_t edgeCoverHeight;
                bool bleft;
                bool bright;
                ptri->GetEdgeCoverData(iedge, edgeCoverHeight, bleft, bright);
                if (edgeCoverHeight < 4) {
                    if (edgeCoverHeight != 2 || bhideCover) {
                        continue;
                    }
                }
                else {
                    uint16_t sheight = bcanCrouch ? crouchHeight : standHeight;
                    if (edgeCoverHeight < sheight) {
                        continue;
                    }
                }
                if (!bleft && !bright && edgeCoverHeight > crouchHeight && !bhideCover) {
                    continue;
                }
                NavMeshEdgeVertices edge = spmesh->GetEdgeVertices(itriIndex, iedge);
                NiPoint3 edgeVector = edge.Vertices[1]->Location - edge.Vertices[0]->Location;
                NiPoint3 midpoint = edge.Vertices[0]->Location + edgeVector * 0.5f;
                NiPoint3 edgePerp = edgeVector.Cross(triNormal);
                edgePerp.z = 0.0f;
                edgePerp.Unitize();
                NiPoint3 checkPoint = midpoint - edgePerp * apRequest->fActorRadius;
                NiPoint2 checkPoint2D;
                checkPoint2D.x = checkPoint.x;
                checkPoint2D.y = checkPoint.y;
                NiPoint2 edge0;
                edge0.x = edge.Vertices[0]->Location.x;
                edge0.y = edge.Vertices[0]->Location.y;
                NiPoint2 edge1;
                edge1.x = edge.Vertices[1]->Location.x;
                edge1.y = edge.Vertices[1]->Location.y;
                if (!NavMeshMath::Find2DIntersection(checkPoint2D, threatPos2D, edge0, edge1)) {
                    continue;
                }
                PathingCoverLocation newLoc(checkPoint, spmesh->GetNavMeshInfo(), itriIndex);
                newLoc.SetEdgeOrientation(edge.Vertices[1]->Location, edge.Vertices[0]->Location, edgePerp);
                newLoc.SetCoverData(
                    edgeCoverHeight, standHeight, crouchHeight, bleft, bright, static_cast<uint8_t>(iedge)
                );
                uint32_t iindex;
                for (iindex = 0; iindex < inum; ++iindex) {
                    bool bstop;
                    if (apRequest->bSortFarthestFirst) {
                        bstop = distanceArray.GetAt(static_cast<int32_t>(iindex)) < fsquaredDist;
                    }
                    else {
                        bstop = distanceArray.GetAt(static_cast<int32_t>(iindex)) > fsquaredDist;
                    }
                    if (bstop) {
                        break;
                    }
                }
                distanceArray.Insert(iindex, fsquaredDist);
                apReturnArray.Insert(iindex, newLoc);
                ++inum;
                if (inum >= apRequest->iMaxResults) {
                    return inum;
                }
            }
        }
    }
    return inum;
}

bool Pathing::CheckLineOfSight(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    PathingLocation* apReturnIntersectionLocation,
    Pathing::CheckLineOfSightFilter* apFilter
) {
    uint16_t usgoalTri;
    if (apReturnIntersectionLocation) {
        apReturnIntersectionLocation->operator=(aStartLocation);
    }
    if (!aGoalLocation.ResolveNavMeshInfo(nullptr)) {
        return !aStartLocation.ResolveNavMeshInfo(nullptr);
    }
    NavMeshPtr spgoalMesh;
    if (aGoalLocation.GetNavMeshAndTriangle(spgoalMesh, usgoalTri)) {
        uint16_t v8 = usgoalTri;
        NavMesh* Object = spgoalMesh.m_pObject;
        bool v10 = Pathing::CheckLineOfSight(
            aStartLocation, aGoalLocation.GetLocation(), Object, v8, apReturnIntersectionLocation, apFilter
        );
        return v10;
    }
    else {
        bool v12 = Pathing::CheckLineOfSight(
            aStartLocation, aGoalLocation.GetLocation(), static_cast<NavMesh*>(nullptr), 0xFFFF,
            apReturnIntersectionLocation, apFilter
        );
        return v12;
    }
}

bool Pathing::CheckLineOfSight(
    PathingLocation& aStartLocation,
    const NiPoint3& aTarget,
    PathingLocation* apReturnIntersectionLocation,
    Pathing::CheckLineOfSightFilter* apFilter
) {
    return Pathing::CheckLineOfSight(
        aStartLocation, aTarget, static_cast<NavMesh*>(nullptr), 0xFFFF, apReturnIntersectionLocation, apFilter
    );
}

bool Pathing::Init() {
    // Each angle is rounded to float (DEGREES_2_RADIAN is a double) before NiCos.
    float fAngle = fINIAvoidanceConeAngle.Float() * 0.01745329238474369;
    DetailedActorPathHandler::fActorAvoidConeAngle = NiCos(fAngle);
    float fWiderAngle = fINIWiderAvoidanceConeAngle.Float() * 0.01745329238474369;
    DetailedActorPathHandler::fWiderActorAvoidConeAngle = NiCos(fWiderAngle);
    float fRunningAngle = fINIRunningAvoidanceConeAngle.Float() * 0.01745329238474369;
    DetailedActorPathHandler::fRunningActorAvoidConeAngle = NiCos(fRunningAngle);
    return true;
}

bool Pathing::Pathfind(PathingRequest* apRequest, PathingSolution* apSolution) {
    alignas(PathBuilder) uint8_t aBuilderStorage[sizeof(PathBuilder)];
    PathBuilder& kBuilder = *::new (aBuilderStorage) PathBuilder(apRequest, apSolution);
    bool bResult = kBuilder.BuildPath();
    kBuilder.~PathBuilder();
    return bResult;
}

PathingLocation* Pathing::FindClosestReachableLocation(
    PathingLocation* apResult,
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    float afMaxPathingDistance
) {
    if (!aStartLocation.ResolveNavMeshInfo(nullptr)) {
        ::new (apResult) PathingLocation(aStartLocation);
        return apResult;
    }
    FastNavMeshTriHandle kStartNode;
    alignas(NavMeshPtr) uint8_t aStartMeshStorage[sizeof(NavMeshPtr)];
    NavMeshPtr* pspStartMesh = ::new (aStartMeshStorage) NavMeshPtr;
    uint16_t usStartTriangle;
    if (!aStartLocation.GetNavMeshAndTriangle(*pspStartMesh, usStartTriangle)) {
        ::new (apResult) PathingLocation(aStartLocation);
        pspStartMesh->~NavMeshPtr();
        return apResult;
    }
    kStartNode.pNavMesh = pspStartMesh->operator->();
    kStartNode.usTriangle = usStartTriangle;
    pspStartMesh->~NavMeshPtr();
    FastNavMeshTriHandle kGoalNode;
    kGoalNode.pNavMesh = nullptr;
    kGoalNode.usTriangle = 0xFFFF;
    if (aGoalLocation.ResolveNavMeshInfo(nullptr)) {
        alignas(NavMeshPtr) uint8_t aGoalMeshStorage[sizeof(NavMeshPtr)];
        NavMeshPtr* pspGoalMesh = ::new (aGoalMeshStorage) NavMeshPtr;
        uint16_t usGoalTriangle;
        if (aGoalLocation.GetNavMeshAndTriangle(*pspGoalMesh, usGoalTriangle)) {
            kGoalNode.pNavMesh = pspGoalMesh->operator->();
            kGoalNode.usTriangle = usGoalTriangle;
        }
        pspGoalMesh->~NavMeshPtr();
    }
    alignas(NavMeshSearchMaxCost) uint8_t aSearchStorage[sizeof(NavMeshSearchMaxCost)];
    NavMeshSearchMaxCost* pSearch = ::new (aSearchStorage) NavMeshSearchMaxCost;
    // Takes the goal's point before the start's.
    NiPoint3 kGoalPoint = aGoalLocation.GetLocation();
    NiPoint3 kStartPoint = aStartLocation.GetLocation();
    NiPoint3 kClosestPoint =
        pSearch->FindClosestPoint(kStartPoint, kStartNode, kGoalPoint, kGoalNode, afMaxPathingDistance);
    alignas(PathingLocation) uint8_t aLocationStorage[sizeof(PathingLocation)];
    PathingLocation* pLocation = ::new (aLocationStorage) PathingLocation(aStartLocation);
    pLocation->SetLocation(kClosestPoint);
    ::new (apResult) PathingLocation(*pLocation);
    pLocation->~PathingLocation();
    pSearch->~NavMeshSearchMaxCost();
    return apResult;
}

// The memory context is not destroyed automatically; each case destroys it explicitly.
bool Pathing::BuildPath(PathingRequest* apRequest, PathingSolution* apSolution) {
    alignas(AutoMemContext) uint8_t aContextStorage[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aContextStorage) AutoMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        106
    );
    bool bResult;
    switch (apRequest->GetType()) {
        case 0: bResult = Pathing::Pathfind(apRequest, apSolution); break;
        case 3: bResult = Pathing::BuildFleePath(static_cast<PathingRequestFlee*>(apRequest), apSolution); break;
        case 4: bResult = Pathing::BuildHidePath(static_cast<PathingRequestHide*>(apRequest), apSolution); break;
        case 5: bResult = Pathing::BuildLOSPath(static_cast<PathingRequestLOS*>(apRequest), apSolution); break;
        case 6:
            bResult = Pathing::BuildPathToSafeStraightLine(
                static_cast<PathingRequestSafeStraightLine*>(apRequest), apSolution
            );
            break;
        case 7:
            bResult =
                Pathing::BuildOptimalLocationPath(static_cast<PathingRequestOptimalLocation*>(apRequest), apSolution);
            break;
        case 8:
            bResult = Pathing::BuildCoveredMovePath(static_cast<PathingRequestCoveredMove*>(apRequest), apSolution);
            break;
        default: bResult = false; break;
    }
    pContext->~AutoMemContext();
    return bResult;
}

bool Pathing::BuildFleePath(PathingRequestFlee* apRequest, PathingSolution* apSolution) {
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    using pathingNodes_t = BSScrapArray<PathingNode*, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        179
    );
    if (!apRequest->Origin.ResolveToClosestNavmeshAndTriangle()) {
        bool bResult = BuildFleePathNoNavMesh(apRequest, apSolution);
        return bResult;
    }
    NavMeshPtr spStartNavMesh;
    uint16_t usStartTriangle = 0xFFFF;
    if (!apRequest->Origin.GetNavMeshAndTriangle(spStartNavMesh, usStartTriangle)) {
        bool bResult = BuildFleePathNoNavMesh(apRequest, apSolution);
        return bResult;
    }
    if (!apRequest->Destination.QUnused()) {
        if (!apRequest->Destination.ResolveNavMeshInfo(nullptr)) {
            bool bResult = BuildFleePathNoNavMesh(apRequest, apSolution);
            return bResult;
        }
        NavMeshPtr spNavMesh;
        uint16_t stri = 0xFFFF;
        if (!apRequest->Destination.GetNavMeshAndTriangle(spNavMesh, stri)) {
            bool bResult = BuildFleePathNoNavMesh(apRequest, apSolution);
            return bResult;
        }
    }
    NavMeshSearchNode startNode;
    startNode.pNavMesh = spStartNavMesh;
    startNode.sTriangle = usStartTriangle;
    NiPoint3 startLocation = apRequest->Origin.GetLocation();
    NavMeshSearchFlee fleeSearch;
    nodes_t nodes(32u, 0, nullptr);
    links_t links(32u, 0, nullptr);
    if (!fleeSearch.BuildNodePath(apRequest, startNode, &nodes, &links)) {
        return false;
    }
    // Reads the last node through the const GetAt.
    const nodes_t& constNodes = nodes;
    NavMeshSearchNode endNode = constNodes.GetAt(static_cast<int32_t>(nodes.GetSize() - 1));
    NiPoint3 endLocation = endNode.pNavMesh->GetCenter(endNode.sTriangle);
    NavMeshInfo* pEndInfo = endNode.pNavMesh->GetNavMeshInfo();
    PathingLocation endLoc(&endLocation, pEndInfo);
    NavMeshEdgeLocation edgeLoc;
    if (Pathing::FindClosestOpenEdge(endLoc, apRequest->GetActorRadius(), edgeLoc)) {
        NiPoint3 delta = endLocation - edgeLoc.Location;
        float fdist = delta.UnitizeGetLength();
        if (fdist < apRequest->GetActorRadius()) {
            endLocation = edgeLoc.Location + delta * apRequest->GetActorRadius();
        }
    }
    apSolution->AddVirtualPathingNode_0(startLocation, startNode.pNavMesh->GetNavMeshInfo());
    apSolution->AddVirtualPathingNode_0(endLocation, endNode.pNavMesh->GetNavMeshInfo());
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = 1;
    PathSmootherPOVSearch pathSmoother;
    pathingNodes_t pathingNodes(32u, 0, nullptr);
    bool bcanFit =
        pathSmoother.SmoothPath(apRequest, nodes, links, startLocation, endLocation, apSolution, nullptr, nullptr);
    if (bcanFit) {
        if (apRequest->GetFirstTangentUsesHeading() && apSolution->GetNodeCount() > 2) {
            PathingNode* pnode0 = apSolution->GetNode(0);
            PathingNode* pnode1 = apSolution->GetNode(1);
            if (pnode0 && pnode1) {
                PathingLocation startLoc(pnode0->GetLocation());
                PathingLocation nextLoc(pnode1->GetLocation());
                // Reads the radius before the heading.
                float fRadius = apRequest->GetActorRadius();
                float fHeading = apRequest->GetInitialPathHeading();
                NiPoint3 tangent;
                Pathing::ComputeInitialTangentForActor(&tangent, fHeading, startLoc, nextLoc, fRadius);
                pnode0->SetTangent(tangent);
            }
        }
    }
    else {
        apSolution->SetIncomplete(true);
    }
    return bcanFit;
}

bool Pathing::BuildFleePathNoNavMesh(PathingRequestFlee* apRequest, PathingSolution* apSolution) {
    NiPoint3 dst;
    NiPoint3 fleeFromPoint;
    NiPoint3 result;
    NiPoint3 fleeToPoint;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        303
    );
    ::new (&result) NiPoint3((&apRequest->Origin)->GetLocation());
    const NiPoint3* startLocation = &result;
    TESObjectCELL* pcell = (&apRequest->Origin)->GetCell();
    TESWorldSpace* pworld = (&apRequest->Origin)->pWorldSpace;
    if (!apRequest->GetAvoidNodeArray() || apRequest->GetAvoidNodeArray()->IsEmpty()) {
        fleeFromPoint = *startLocation;
        float fangle = RandomFloatNegPiToPi();
        double v12 = std::sin(fangle);
        float v11 = apRequest->GetMinFleeDistance() * v12;
        NiPoint3* a2a = ::new (&dst) NiPoint3(v11 * NiPoint3_UNIT_Y);
        double v10 = std::cos(fangle);
        float fScalar = apRequest->GetMinFleeDistance() * v10;
        fleeToPoint = fleeFromPoint + fScalar * NiPoint3_UNIT_X + *a2a;
    }
    else {
        const NiPoint3* scale = startLocation;
        PathingAvoidNodeArray* AvoidNodeArray = apRequest->GetAvoidNodeArray();
        AvoidNodeArray->ComputeCenter(*scale, fleeFromPoint);
        fleeToPoint = *startLocation - fleeFromPoint;
        fleeToPoint.Unitize();
        float a2 = apRequest->GetMinFleeDistance();
        fleeToPoint.operator*=(a2);
        fleeToPoint.operator+=(*startLocation);
    }
    if (pcell) {
        float height;
        if (pcell->GetLandHeight(fleeToPoint, height)) {
            fleeToPoint.z = height;
        }
    }
    apSolution->AddVirtualPathingNode_006D1520(*startLocation, pcell, pworld);
    apSolution->AddVirtualPathingNode_006D1520(fleeToPoint, pcell, pworld);
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = 1;
    return true;
}

bool Pathing::BuildLOSPath(PathingRequestLOS* apRequest, PathingSolution* apSolution) {
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        378
    );
    if ((&apRequest->Origin)->ResolveToClosestNavmeshAndTriangle()) {
        uint16_t arTriangle[3];
        NavMeshPtr spNavMesh;
        arTriangle[0] = -1;
        if ((&apRequest->Origin)->GetNavMeshAndTriangle(spNavMesh, *arTriangle)) {
            NavMeshSearchNode startNode;
            startNode.pNavMesh = spNavMesh.operator NavMesh*();
            startNode.sTriangle = arTriangle[0];
            NiPoint3 startLocation = (&apRequest->Origin)->GetLocation();
            bool busingIgnoredGoal = false;
            bool busingLastValidNode__busingIgnoredGoal = false;
            NavMeshSearchLOS losSearch;
            nodes_t nodes(32u, 0, nullptr);
            links_t links(32u, 0, nullptr);
            bool bcanFit = true;
            if (!losSearch.BuildNodePath(apRequest, startNode, &nodes, &links)) {
                if (losSearch.GetFirstIgnoredGoal()) {
                    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* FirstIgnoredGoal =
                        losSearch.GetFirstIgnoredGoal();
                    busingIgnoredGoal = losSearch.GetPathToNode(FirstIgnoredGoal, &nodes, &links);
                }
                if (!busingIgnoredGoal) {
                    if (losSearch.GetLastValidNode()) {
                        const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* LastValidNode =
                            losSearch.GetLastValidNode();
                        busingLastValidNode__busingIgnoredGoal = losSearch.GetPathToNode(LastValidNode, &nodes, &links);
                    }
                }
                if (!busingIgnoredGoal && !busingLastValidNode__busingIgnoredGoal) {
                    if (losSearch.GetClosestNode()) {
                        const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* ClosestNode = losSearch.GetClosestNode();
                        busingLastValidNode__busingIgnoredGoal = losSearch.GetPathToNode(ClosestNode, &nodes, &links);
                    }
                }
                if (!busingIgnoredGoal && !busingLastValidNode__busingIgnoredGoal) {
                    bcanFit = false;
                    busingLastValidNode__busingIgnoredGoal = true;
                }
            }
            if (bcanFit) {
                // Calls the const GetAt, not the non-const one.
                NavMeshSearchNode v22 = static_cast<const nodes_t&>(nodes).GetAt(nodes.iSize - 1);
                NiPoint3 endLocation = v22.pNavMesh->GetCenter(v22.sTriangle);
                apSolution->Clear();
                PathingRequest tempRequest(*apRequest);
                uint16_t sTriangle = v22.sTriangle;
                NavMeshInfo* NavMeshInfo_ = v22.pNavMesh->GetNavMeshInfo();
                PathingLocation destination(endLocation, NavMeshInfo_, sTriangle);
                tempRequest.SetDestination(destination);
                bcanFit = Pathing::BuildPath(&tempRequest, apSolution);
            }
            if (!bcanFit && (busingIgnoredGoal || busingLastValidNode__busingIgnoredGoal)) {
                NavMeshPtr spnavMesh;
                uint16_t sunseenTriangle = 0;
                PathingLOSMap* LOSMap = losSearch.GetLOSMap();
                if (LOSMap->GetUnseenNavMeshAndTriangle(spnavMesh, sunseenTriangle)) {
                    uint32_t ilastLoadedVirtualNodeIndex = apSolution->iLastLoadedVirtualNodeIndex;
                    if (ilastLoadedVirtualNodeIndex == -1 || !apSolution->GetVirtualNodeCount()) {
                        NavMeshInfo* v8 = startNode.pNavMesh->GetNavMeshInfo();
                        apSolution->AddVirtualPathingNode_0(startLocation, v8);
                        apSolution->iLastLoadedVirtualNodeIndex = 0;
                    }
                    VirtualPathingNode* plastLoadedVirtualNode =
                        apSolution->GetVirtualNode(ilastLoadedVirtualNodeIndex);
                    if (plastLoadedVirtualNode) {
                        NiPoint3 unseenTriangleCenter = spnavMesh.m_pObject->GetCenter(sunseenTriangle);
                        uint16_t v11 = sunseenTriangle;
                        NavMeshInfo* v9 = spnavMesh.m_pObject->GetNavMeshInfo();
                        PathingLocation unseenTriangleLocation(unseenTriangleCenter, v9, v11);
                        plastLoadedVirtualNode->SetLocation(unseenTriangleLocation);
                    }
                }
            }
            return bcanFit;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

bool Pathing::BuildHidePath(PathingRequestHide* apRequest, PathingSolution* apSolution) {
    using NodeArray_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using LinkArray_t = BSScrapArray<NavMeshSearchLink, 1024>;
    using PathingNodeArray_t = BSScrapArray<PathingNode*, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        501
    );
    if (!(&apRequest->Origin)->ResolveNavMeshInfo(nullptr)) {
        return false;
    }
    NavMeshPtr spNavMesh;
    uint16_t sTriangle = 0xFFFF;
    if (!(&apRequest->Origin)->GetNavMeshAndTriangle(spNavMesh, sTriangle)) {
        return false;
    }
    // Only the mesh and the triangle of the start node are written; its center is deliberately left uninitialised, as
    // the game does.
    NavMeshSearchNode startNode;
    startNode.pNavMesh = spNavMesh;
    startNode.sTriangle = sTriangle;
    NiPoint3 startLocation = (&apRequest->Origin)->GetLocation();
    NavMeshSearchHide hideSearch;
    NodeArray_t aTriSolution(32u, 0, nullptr);
    LinkArray_t aLinks(32u, 0, nullptr);
    if (!hideSearch.NavMeshSearchHide::BuildNodePath(apRequest, startNode, &aTriSolution, &aLinks)) {
        return false;
    }
    // Reads the last node through the const GetAt and copies it whole.
    const BSSimpleArray<NavMeshSearchNode, 1024>& rTriSolution = aTriSolution;
    NavMeshSearchNode endNode = rTriSolution.GetAt(rTriSolution.GetSize() - 1);
    NiPoint3 endLocation = endNode.pNavMesh->GetCenter(endNode.sTriangle);
    apSolution->AddVirtualPathingNode_0(startLocation, startNode.pNavMesh->GetNavMeshInfo());
    apSolution->AddVirtualPathingNode_0(endLocation, endNode.pNavMesh->GetNavMeshInfo());
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = 1;
    PathSmoother pathSmoother;
    PathingNodeArray_t aPathingNodes(32u, 0, nullptr);
    pathSmoother.SmoothPath(apRequest, aTriSolution, aLinks, startLocation, endLocation, apSolution, nullptr, true);
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = apSolution->GetVirtualNodeCount() - 1;
    return true;
}

bool Pathing::BuildPathToSafeStraightLine(PathingRequestSafeStraightLine* apRequest, PathingSolution* apSolution) {
    using pathingNodes_t = BSScrapArray<PathingNode*, 1024>;
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        575
    );
    if (!apRequest->Origin.ResolveToClosestNavmeshAndTriangle()) {
        return false;
    }
    NavMeshPtr spstartNavMesh;
    uint16_t istartTri = 0xFFFF;
    if (!apRequest->Origin.GetNavMeshAndTriangle(spstartNavMesh, istartTri)) {
        return false;
    }
    NavMeshSearchNode startNode;
    startNode.pNavMesh = spstartNavMesh;
    startNode.sTriangle = istartTri;
    NiPoint3 startLocation = apRequest->Origin.GetLocation();
    if (!apRequest->Destination.ResolveToClosestNavmeshAndTriangle()) {
        return false;
    }
    NavMeshPtr spgoalNavMesh;
    uint16_t igoalTri = 0xFFFF;
    if (!apRequest->Destination.GetNavMeshAndTriangle(spgoalNavMesh, igoalTri)) {
        return false;
    }
    NavMeshSearchNode goalNode;
    goalNode.pNavMesh = spgoalNavMesh;
    goalNode.sTriangle = igoalTri;
    NavMeshSearchSLPoint straightLinePointSearch;
    nodes_t nodes(32u, 0, nullptr);
    links_t links(32u, 0, nullptr);
    if (!straightLinePointSearch.BuildPathToSafeStraightLine(apRequest, startNode, goalNode, &nodes, &links)) {
        return false;
    }
    // Calls the const GetAt and copies the whole node.
    const BSSimpleArray<NavMeshSearchNode, 1024>& rconstNodes = nodes;
    NavMeshSearchNode lastNode = rconstNodes.GetAt(static_cast<int32_t>(nodes.iSize - 1));
    NiPoint3 endLocation = lastNode.pNavMesh->GetCenter(lastNode.sTriangle);
    apSolution->AddVirtualPathingNode_0(startLocation, startNode.pNavMesh->GetNavMeshInfo());
    apSolution->AddVirtualPathingNode_0(endLocation, lastNode.pNavMesh->GetNavMeshInfo());
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = 1;
    PathSmoother pathSmoother;
    pathingNodes_t pathingNodes(32u, 0, nullptr);
    pathSmoother.SmoothPath(apRequest, nodes, links, startLocation, endLocation, apSolution, nullptr, true);
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = apSolution->GetVirtualNodeCount() - 1;
    return true;
}

bool Pathing::BuildOptimalLocationPath(PathingRequestOptimalLocation* apRequest, PathingSolution* apSolution) {
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    using navMeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    using pathingNodes_t = BSScrapArray<PathingNode*, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        668
    );
    if (!apRequest->Origin.ResolveToClosestNavmeshAndTriangle()) {
        return false;
    }
    NavMeshPtr spNavMesh;
    uint16_t istartTri = 0xFFFF;
    if (!apRequest->Origin.GetNavMeshAndTriangle(spNavMesh, istartTri)) {
        return false;
    }
    float fminDistance = apRequest->fMinDistance;
    float fmaxDistance = apRequest->fMaxDistance;
    float fidealDistance = (fminDistance + fmaxDistance) / 2.0;
    NiPoint3 attackerPos = apRequest->Origin.GetLocation();
    NiPoint3 targetPos = apRequest->Destination.GetLocation();
    NiPoint3 direction = targetPos - attackerPos;
    float fdeltaToTargetDistance = direction.UnitizeGetLength();
    NiPoint3 idealPoint = targetPos - direction * fidealDistance;
    float fdistanceDelta = fidealDistance - fdeltaToTargetDistance;
    float fmaxPathingDistance = std::fabs(fdistanceDelta);
    NiPoint3 closestPoint;
    TESObjectCELL* pCell = apRequest->Origin.GetCell();
    if (!Pathing::FindClosestPointOnNavmesh(apRequest->Origin.pWorldSpace, pCell, idealPoint, &closestPoint)) {
        return false;
    }
    NiPoint3 vec = closestPoint - idealPoint;
    if (fmaxPathingDistance * fmaxPathingDistance < vec.SqrLength()) {
        return false;
    }
    PathingLocation optimalPathingLocation(closestPoint, apRequest->Origin);
    NavMeshSearchNode startNode;
    startNode.pNavMesh = spNavMesh;
    startNode.sTriangle = istartTri;
    NiPoint3 center = startNode.pNavMesh->GetCenter(startNode.sTriangle);
    startNode.CenterX = center.x;
    startNode.CenterY = center.y;
    startNode.CenterZ = center.z;
    NiPoint3 startLocation = apRequest->Origin.GetLocation();
    if (!optimalPathingLocation.ResolveNavMeshInfo(nullptr)) {
        optimalPathingLocation.SetUnused();
    }
    NavMeshSearchMaxCost search;
    nodes_t nodes(32u, 0, nullptr);
    links_t links(32u, 0, nullptr);
    float fsearchMaxDistance = fmaxPathingDistance + fmaxPathingDistance;
    search.SetMaxDistance(fsearchMaxDistance);
    navMeshes_t loadedNavMeshes;
    Pathing::GetAllLoadedNavmeshes(&loadedNavMeshes);
    NavMeshSearchNode endNode;
    NiPoint3 endLocation;
    if (search.BuildNodePath(apRequest, startNode, optimalPathingLocation, &nodes, &links)) {
        // Calls the const GetAt and copies the whole node.
        const BSSimpleArray<NavMeshSearchNode, 1024>& rconstNodes = nodes;
        endNode = rconstNodes.GetAt(static_cast<int32_t>(nodes.iSize - 1));
        if (optimalPathingLocation.QUnused()) {
            NiPoint3 nodeCenter;
            endLocation = *endNode.GetCenter(&nodeCenter);
        }
        else {
            endLocation = closestPoint;
        }
    }
    else if (!nodes.IsEmpty()) {
        endNode = search.GetBestAStarNode()->tNodeData;
        NiPoint3 nodeCenter;
        endLocation = *endNode.GetCenter(&nodeCenter);
    }
    else {
        bool bResult = false;
        if (fdeltaToTargetDistance > apRequest->fAbsoluteMaxDistance) {
            bResult = Pathing::Pathfind(apRequest, apSolution);
        }
        return bResult;
    }
    float fabsoluteMaxDistance = apRequest->fAbsoluteMaxDistance;
    float fabsoluteMinDistance = apRequest->fAbsoluteMinDistance;
    direction = endLocation - targetPos;
    float fsqrDistance = direction.SqrLength();
    if (fsqrDistance < fabsoluteMinDistance * fabsoluteMinDistance ||
        fsqrDistance > fabsoluteMaxDistance * fabsoluteMaxDistance) {
        return false;
    }
    apSolution->AddVirtualPathingNode_0(startLocation, startNode.pNavMesh->GetNavMeshInfo());
    apSolution->AddVirtualPathingNode_0(endLocation, endNode.pNavMesh->GetNavMeshInfo());
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = 1;
    PathSmootherPOVSearch pathSmoother;
    pathingNodes_t pathingNodes(32u, 0, nullptr);
    bool bSmoothed =
        pathSmoother.SmoothPath(apRequest, nodes, links, startLocation, endLocation, apSolution, nullptr, nullptr);
    return bSmoothed;
}

bool Pathing::BuildCoveredMovePath(PathingRequestCoveredMove* apRequest, PathingSolution* apSolution) {
    // The PathingLocations are built and destroyed only through their own Construct/Destruct calls below.
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        802
    );
    if (Pathing::Pathfind(apRequest, apSolution)) {
        CombatCoverSearchInfo* pcombatCoverSearchInfo;
        CombatCoverLocation* pcombatCoverLocation;
        float fminAdvanceDistance = 512.0f;
        PathingLocation previousNodeLocation(*&apRequest->Origin);
        CombatCoverLocation* v11 = static_cast<CombatCoverLocation*>(ourMalloc(sizeof(CombatCoverLocation)));
        if (v11) {
            pcombatCoverLocation = ::new (v11) CombatCoverLocation;
        }
        else {
            pcombatCoverLocation = nullptr;
        }
        CombatCoverSearchInfo* v10 = static_cast<CombatCoverSearchInfo*>(ourMalloc(sizeof(CombatCoverSearchInfo)));
        if (v10) {
            pcombatCoverSearchInfo = ::new (v10) CombatCoverSearchInfo;
        }
        else {
            pcombatCoverSearchInfo = nullptr;
        }
        PathingRequestCoveredMove requestCopy(*apRequest);
        requestCopy.pCombatCoverLocation = pcombatCoverLocation;
        requestCopy.SetCombatCoverSearchInfo(pcombatCoverSearchInfo);
        apRequest->pCombatCoverLocation = pcombatCoverLocation;
        apRequest->SetCombatCoverSearchInfo(pcombatCoverSearchInfo);
        uint32_t icurrentNode = 1;
        while (true) {
            uint32_t NodeCount = apSolution->GetNodeCount();
            if (NodeCount <= icurrentNode) {
                break;
            }
            PathingNode* Node = apSolution->GetNode(icurrentNode);
            PathingLocation nextNodeLocation(*&Node->Location);
            requestCopy.SetDestination(nextNodeLocation);
            if (CombatState::FindBestAdvanceLocation(&requestCopy, fminAdvanceDistance, true)) {
                NiPoint3* ScriptList = &requestCopy.CoverLocation;
                PathingLocation coverPathingLocation(*ScriptList, nextNodeLocation);
                requestCopy.SetDestination(coverPathingLocation);
                PathingSolution newSolution;
                bool bsuccess = Pathing::Pathfind(&requestCopy, &newSolution);
                if (bsuccess) {
                    NiPoint3* v6 = &requestCopy.CoverLocation;
                    apRequest->SetCoverLocation(*v6);
                    apRequest->SetDestination(coverPathingLocation);
                    requestCopy.ClearCombatCoverData();
                    newSolution.CopyTo(apSolution);
                    apRequest->bFinished = true;
                    bool bsuccess_2 = bsuccess;
                    return bsuccess_2;
                }
            }
            ++icurrentNode;
            previousNodeLocation.operator=(nextNodeLocation);
        }
        requestCopy.ClearCombatCoverData();
        if (pcombatCoverLocation) {
            delete pcombatCoverLocation;
        }
        apRequest->pCombatCoverLocation = nullptr;
        apRequest->bFinished = true;
        return true;
    }
    else {
        return false;
    }
}

bool Pathing::BuildTeleportDoorPath(
    const PathingLocation& aOrigin,
    const PathingLocation& aDestination,
    TeleportPath* apRetPath,
    TELEPORT_DOOR_PATH_RESTRICTION aRestrictions,
    float afMaxDist,
    ActorLockData* apActorLockData,
    DOOR_LOCK_DISPOSITION aeDoorLockDisp
) {
    alignas(AutoMemContext) uint8_t kContextMemory[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (kContextMemory) AutoMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        1410
    );
    alignas(TeleportDoorSearch) uint8_t kSearchMemory[sizeof(TeleportDoorSearch)];
    TeleportDoorSearch* pSearch = ::new (kSearchMemory) TeleportDoorSearch;
    apRetPath->Clear();
    apRetPath->Spaces.SetReservedSize(10);
    apRetPath->Doors.SetReservedSize(10);
    bool bResult = false;
    if (pSearch->BuildNodePath(
            aOrigin, aDestination, apRetPath, aRestrictions, afMaxDist, apActorLockData, aeDoorLockDisp
        )) {
        apRetPath->Start = aOrigin.GetLocation();
        apRetPath->End = aDestination.GetLocation();
        bResult = true;
    }
    pSearch->~TeleportDoorSearch();
    pContext->~AutoMemContext();
    return bResult;
}

float Pathing::ComputeTeleportDoorPathLength(
    const PathingLocation& aOrigin,
    const PathingLocation& aDestination,
    float aMaxDist,
    ActorLockData* apActorLockData,
    DOOR_LOCK_DISPOSITION aeDoorLockDisp
) {
    alignas(TeleportPath) uint8_t kPathMemory[sizeof(TeleportPath)];
    TeleportPath* pPath = ::new (kPathMemory) TeleportPath;
    float fLength;
    if (BuildTeleportDoorPath(
            aOrigin, aDestination, pPath, TELEPORT_DOOR_PATH_NONE, aMaxDist, apActorLockData, aeDoorLockDisp
        )) {
        fLength = pPath->ComputeLength();
    }
    else {
        fLength = 3.4028235e+38f;
    }
    pPath->~TeleportPath();
    return fLength;
}

bool Pathing::DoAABBsOverlap(
    const NiPoint3& bound1Min,
    const NiPoint3& bound1Max,
    const NiPoint3& bound2Min,
    const NiPoint3& bound2Max
) {
    if (bound1Max.x <= bound2Min.x) {
        return false;
    }
    if (bound1Min.x >= bound2Max.x) {
        return false;
    }
    if (bound1Max.y <= bound2Min.y) {
        return false;
    }
    if (bound1Min.y >= bound2Max.y) {
        return false;
    }
    if (bound1Max.z <= bound2Min.z) {
        return false;
    }
    // NaN counts as overlapping, as the game does.
    return !(bound1Min.z >= bound2Max.z);
}

bool Pathing::FindClosestPointOnNavmesh(
    TESWorldSpace* apWorldSpace,
    TESObjectCELL* apCell,
    const NiPoint3& aLocation,
    NiPoint3* apRetPt
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        2310
    );
    PathingLocation loc((&aLocation), apCell, apWorldSpace);
    if (loc.ResolveNavMeshInfo(nullptr) && loc.HasNavMeshData()) {
        *apRetPt = aLocation;
        return true;
    }
    if (loc.ResolveToClosestNavmeshAndTriangle()) {
        uint16_t ustriangle;
        NavMeshPtr testMesh;
        if (loc.GetNavMeshAndTriangle(testMesh, ustriangle)) {
            *apRetPt = testMesh.m_pObject->GetCenter(ustriangle);
            return true;
        }
    }
    return false;
}

bool Pathing::FindPointOnNavMesh(
    TESWorldSpace* apWorldSpace,
    TESObjectCELL* apCell,
    const NiPoint3& aLocation,
    float aRadius,
    NiPoint3* apRetPt
) {
    NiPoint3 dst;
    bool bonTri;
    uint16_t usTriIndex;
    PathingLocation loc((&aLocation), apCell, apWorldSpace);
    bool bfoundMesh = loc.ResolveNavMeshInfo(nullptr);
    NavMeshPtr testMesh;
    if (!loc.GetNavMesh(testMesh)) {
        bfoundMesh = false;
    }
    if (!bfoundMesh) {
        float xcoord = RandomFloat(-aRadius, aRadius);
        float v9 = aRadius * aRadius - xcoord * xcoord;
        float ycoord = std::sqrt(v9);
        ycoord = RandomFloat(-ycoord, ycoord);
        NiPoint3* v5 = ::new (&dst) NiPoint3(xcoord, ycoord, 0.0f);
        *apRetPt = aLocation + *v5;
        return true;
    }
    if (!testMesh.m_pObject->FindRandomPointWithinRadius(aLocation, aRadius, apRetPt) ||
        !testMesh.m_pObject->IsPointInsideNavmesh(*apRetPt, nullptr)) {
        usTriIndex = -1;
        bonTri = false;
        if (!testMesh.m_pObject->FindClosestTriangleForLocation(aLocation, &usTriIndex, &bonTri)) {
            return false;
        }
        *apRetPt = testMesh.m_pObject->GetCenter(usTriIndex);
    }
    return true;
}

bool Pathing::FindClosestOpenEdge(PathingLocation& aLocation, float afMaxRadius, NavMeshEdgeLocation& aRetEdge) {
    aLocation.ResolveNavMeshInfo(nullptr);
    if (!aLocation.HasNavMeshData()) {
        using edges_t = BSSimpleArray<NavMeshEdgeLocation, 1024>;
        edges_t edges;
        Pathing::FindClosestEdges(aLocation, 1u, edges);
        if (edges.IsEmpty()) {
            return false;
        }
        aRetEdge = edges.GetAt(0);
        return true;
    }
    using foundOpenEdges_t = BSSimpleArray<FastNavMeshEdgeHandle, 1024>;
    using allTris_t = BSSimpleArray<NavmeshTriFan, 1024>;
    foundOpenEdges_t foundOpenEdges;
    allTris_t allTris;
    NavMeshPtr spmesh;
    uint16_t usfirstTri;
    if (!aLocation.GetNavMeshAndTriangle(spmesh, usfirstTri)) {
        return false;
    }
    // The fan's handle is built with its empty constructor, then copy-constructed from a temporary handle to the start
    // triangle.
    NavmeshTriFan firstTri;
    firstTri.hTri.~FastNavMeshTriHandle();
    ::new (&firstTri.hTri) FastNavMeshTriHandle;
    FastNavMeshTriHandle startTri;
    FNVD_RECONSTRUCT(FastNavMeshTriHandle)(
        &firstTri.hTri, *FNVD_RECONSTRUCT(FastNavMeshTriHandle)(&startTri, spmesh.operator NavMesh*(), usfirstTri)
    );
    for (int32_t e = 0; e < 3; ++e) {
        NiPoint3 vert1;
        NiPoint3 vert2;
        spmesh.m_pObject->GetEdgeVertices(usfirstTri, e, &vert1, &vert2);
        firstTri.bInradius[e] =
            afMaxRadius > NavMeshMath::DistanceFromSegment(aLocation.GetLocation(), vert1, vert2, nullptr);
    }
    allTris.Add(firstTri);
    for (uint32_t i = 0; i < allTris.iSize; ++i) {
        NavmeshTriFan fan;
        FNVD_RECONSTRUCT(NavmeshTriFan)(&fan, allTris.GetAt(static_cast<int32_t>(i)));
        for (int32_t e = 0; e < 3; ++e) {
            if (!fan.bInradius[e]) {
                continue;
            }
            NavMeshTriangle* ptri = fan.hTri.pNavMesh->GetTriangle(fan.hTri.usTriangle);
            FastNavMeshEdgeHandle thisEdge;
            thisEdge.~FastNavMeshEdgeHandle();
            ::new (&thisEdge) FastNavMeshEdgeHandle(fan.hTri.pNavMesh, fan.hTri.usTriangle, e);
            if (ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                foundOpenEdges.Add(thisEdge);
                continue;
            }
            FastNavMeshEdgeHandle otherEdge;
            otherEdge.~FastNavMeshEdgeHandle();
            ::new (&otherEdge) FastNavMeshEdgeHandle;
            if (!fan.hTri.pNavMesh->GetMatchingEdge(thisEdge, &otherEdge)) {
                continue;
            }
            bool binListAlready = false;
            for (uint32_t j = 0; j < allTris.iSize; ++j) {
                if (allTris.GetAt(static_cast<int32_t>(j)).hTri == otherEdge) {
                    binListAlready = true;
                    break;
                }
            }
            if (binListAlready) {
                continue;
            }
            NavmeshTriFan newTri;
            newTri.hTri.~FastNavMeshTriHandle();
            ::new (&newTri.hTri) FastNavMeshTriHandle;
            newTri.hTri.~FastNavMeshTriHandle();
            ::new (&newTri.hTri) FastNavMeshTriHandle(otherEdge);
            for (int32_t f = 0; f < 3; ++f) {
                NiPoint3 vert1;
                NiPoint3 vert2;
                otherEdge.pNavMesh->GetEdgeVertices(otherEdge.usTriangle, f, &vert1, &vert2);
                newTri.bInradius[f] =
                    afMaxRadius > NavMeshMath::DistanceFromSegment(aLocation.GetLocation(), vert1, vert2, nullptr);
            }
            allTris.Add(newTri);
        }
    }
    if (foundOpenEdges.IsEmpty()) {
        return false;
    }
    FastNavMeshEdgeLocation closestEdge;
    FNVD_RECONSTRUCT(FastNavMeshEdgeHandle)(&closestEdge);
    float fBestSqrDist = 3.4028235e+38f;
    for (uint32_t i = 0; i < foundOpenEdges.iSize; ++i) {
        NiPoint3 vert1;
        NiPoint3 vert2;
        int32_t iEdgeIndex = foundOpenEdges.GetAt(static_cast<int32_t>(i)).iEdgeIndex;
        uint16_t usTriangle = foundOpenEdges.GetAt(static_cast<int32_t>(i)).usTriangle;
        foundOpenEdges.GetAt(static_cast<int32_t>(i)).pNavMesh->GetEdgeVertices(usTriangle, iEdgeIndex, &vert1, &vert2);
        NiPoint3 edgePt = NavMeshMath::ProjectPointOntoSegment(aLocation.GetLocation(), vert1, vert2);
        float fSqrDist = (edgePt - aLocation.GetLocation()).SqrLength();
        if (fBestSqrDist > fSqrDist) {
            fBestSqrDist = fSqrDist;
            closestEdge.Location = edgePt;
            closestEdge.FastNavMeshEdgeHandle::Copy(foundOpenEdges.GetAt(static_cast<int32_t>(i)));
        }
    }
    aRetEdge.pNavMeshInfo = closestEdge.pNavMesh->GetNavMeshInfo();
    aRetEdge.usTriangle = closestEdge.usTriangle;
    aRetEdge.iEdgeIndex = closestEdge.iEdgeIndex;
    aRetEdge.Location = closestEdge.Location;
    return true;
}

// Without a navmesh point, a random point at a random distance between the request's minimum and maximum from its
// origin, dropped onto the land when the origin's cell has land there. The angle and the distance are float locals;
// sinf goes first, and each product is rounded to float before the point is built.
void Pathing::FindClosePoint(PathingRequestClosePoint* apRequest, NiPoint3* apRetPt) {
    if (FindClosePointViaNavMesh(apRequest, apRetPt)) {
        return;
    }
    float fAngle = RandomFloatTwoPi();
    float fMaxDistance = apRequest->GetMaxDistance();
    float fMinDistance = apRequest->GetMinDistance();
    float fDistance = RandomFloat(fMinDistance, fMaxDistance);
    float fY = std::sinf(fAngle) * fDistance;
    float fX = std::cosf(fAngle) * fDistance;
    NiPoint3 kOffset(fX, fY, 0.0f);
    *apRetPt = apRequest->GetOrigin().GetLocation() + kOffset;
    TESObjectCELL* pCell = apRequest->GetOrigin().GetCell();
    if (!pCell) {
        return;
    }
    float fHeight;
    if (pCell->GetLandHeight(*apRetPt, fHeight)) {
        apRetPt->z = fHeight;
    }
}

const BSSimpleArray<NavMeshInfo*, 1024>* Pathing::GetPotentialNavMeshInfoForLocation(TESObjectCELL* apCell) {
    return TES::pInstance->GetNavMeshInfoMap()->FindNavMeshInfoForCell(apCell);
}

const BSSimpleArray<NavMeshInfo*, 1024>*
Pathing::GetPotentialNavMeshInfoForLocation(TESWorldSpace* apWorldSpace, int32_t aiCellKey) {
    return TES::pInstance->GetNavMeshInfoMap()->FindNavMeshInfoForCellKey(apWorldSpace, aiCellKey);
}

bool Pathing::FindNavmeshesForLocation(
    PathingLocation& aLocation,
    float afRadius,
    BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes,
    bool abConnected
) {
    FNVD_INLINED_LOCAL(NavMeshPtr, spnavMesh, std::nullopt, std::nullopt);
    TESObjectCELL* pcell_2;
    TESObjectCELL* padjacentCell;
    using cells_t = BSSimpleArray<TESObjectCELL*, 1024>;
    NiPoint3 location = aLocation.GetLocation();
    TESObjectCELL* pcell = aLocation.GetCell();
    if (!pcell) {
        return false;
    }
    float fminY = location.y - afRadius;
    float fminX = location.x - afRadius;
    NiPoint3 searchAABBMin(fminX, fminY, -3.4028235e+38f);
    float fmaxY = location.y + afRadius;
    float fmaxX = location.x + afRadius;
    NiPoint3 searchAABBMax(fmaxX, fmaxY, 3.4028235e+38f);
    if (abConnected) {
        if (!aLocation.ResolveNavMeshInfo(nullptr)) {
            return false;
        }
        NavMeshPtr spmesh;
        if (!aLocation.GetNavMesh(spmesh)) {
            return false;
        }
        apRetNavMeshes->Add(spmesh);
    }
    else {
        cells_t cells;
        if (pcell->GetInterior()) {
            pcell_2 = aLocation.GetCell();
            cells.Add(pcell_2);
        }
        else {
            for (int32_t x = pcell->GetDataX() - 1; x < pcell->GetDataX() + 2; ++x) {
                for (int32_t y = pcell->GetDataY() - 1; y < pcell->GetDataY() + 2; ++y) {
                    NiPoint3 cellmin(static_cast<float>(x << 12), static_cast<float>(y << 12), -3.4028235e+38f);
                    float fcellMaxY = static_cast<double>(y << 12) + 4096.0;
                    float fcellMaxX = static_cast<double>(x << 12) + 4096.0;
                    NiPoint3 cellmax(fcellMaxX, fcellMaxY, 3.4028235e+38f);
                    if (Pathing::DoAABBsOverlap(cellmin, cellmax, searchAABBMin, searchAABBMax)) {
                        padjacentCell = pcell->GetWorldSpace()->GetCellFromCellCoord(x, y);
                        cells.Add(padjacentCell);
                    }
                }
            }
        }
        if (cells.IsEmpty()) {
            return false;
        }
        for (uint32_t i = 0; i < cells.iSize; ++i) {
            TESObjectCELL* padjCell = cells.GetAt(static_cast<int32_t>(i));
            if (padjCell && padjCell->GetNavMeshArray()) {
                for (uint32_t j = 0; j < padjCell->GetNavMeshArray()->GetNavMeshCount(); ++j) {
                    NavMeshPtr* pnavMesh = padjCell->GetNavMeshArray()->GetNavMeshByIndex(&spnavMesh, j);
                    apRetNavMeshes->Add(*pnavMesh);
                    FNVD_DESTRUCT(NiPointer<NavMesh>)(&spnavMesh);
                }
            }
        }
    }
    if (apRetNavMeshes->IsEmpty()) {
        return false;
    }
    for (uint32_t n = 0; n < apRetNavMeshes->iSize; ++n) {
        NavMeshPtr spcurrentMesh(apRetNavMeshes->operator[](n));
        uint32_t isize = spcurrentMesh.m_pObject->GetNavMeshInfo()->AdjacentNavMeshes.iSize;
        uint32_t ipreferredSize = spcurrentMesh.m_pObject->GetNavMeshInfo()->PreferredAdjacentNavMeshes.iSize;
        for (uint32_t i_2 = 0; i_2 < ipreferredSize + isize; ++i_2) {
            NavMeshInfo* pportal;
            if (i_2 >= isize) {
                pportal = spcurrentMesh.m_pObject->GetNavMeshInfo()->PreferredAdjacentNavMeshes.GetAt(
                    static_cast<int32_t>(i_2 - isize)
                );
            }
            else {
                pportal = spcurrentMesh.m_pObject->GetNavMeshInfo()->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i_2));
            }
            if (!pportal) {
                continue;
            }
            NavMeshPtr spotherMesh;
            if (pportal->GetNavMesh(spotherMesh)) {
                if (apRetNavMeshes->Find(
                        spotherMesh, 0, &BSSimpleArray<NavMeshPtr, 1024>::DefaultEqualFunctionByAddress
                    ) != -1) {
                    continue;
                }
                TESObjectCELL* potherNavMeshCell = spotherMesh.m_pObject->pParentCell;
                if (!potherNavMeshCell) {
                    continue;
                }
                if (pcell->GetInterior()) {
                    if (potherNavMeshCell != pcell) {
                        continue;
                    }
                }
                else if (pcell->GetWorldSpace() != potherNavMeshCell->GetWorldSpace()) {
                    continue;
                }
                NiPoint3 boundsMin;
                NiPoint3 boundsMax;
                if (spotherMesh.m_pObject->GetBounds(boundsMin, boundsMax) &&
                    Pathing::DoAABBsOverlap(boundsMin, boundsMax, searchAABBMin, searchAABBMax)) {
                    apRetNavMeshes->Add(spotherMesh);
                }
            }
        }
    }
    return true;
}

bool Pathing::FindClosePointViaNavMesh(PathingRequestClosePoint* apRequest, NiPoint3* apRetPt) {
    NiPoint3 apResult;
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    if (!(&apRequest->Origin)->ResolveToClosestNavmeshAndTriangle()) {
        return false;
    }
    NavMeshPtr spNavMesh;
    uint16_t stri = -1;
    if ((&apRequest->Origin)->GetNavMeshAndTriangle(spNavMesh, stri)) {
        NavMeshSearchNode pNavMesh;
        pNavMesh.pNavMesh = spNavMesh.operator NavMesh*();
        pNavMesh.sTriangle = stri;
        NavMeshSearchClosePoint closePointSearch;
        nodes_t nodes(32u, 0, nullptr);
        links_t links(32u, 0, nullptr);
        if (closePointSearch.BuildNodePath(apRequest, pNavMesh, &nodes, &links)) {
            NiPoint3 verts[3];
            // Reads the last node through the const GetAt and copies it whole.
            const BSSimpleArray<NavMeshSearchNode, 1024>& constNodes = nodes;
            NavMeshSearchNode lastNode = constNodes.GetAt(static_cast<int32_t>(nodes.GetSize() - 1));
            rt::eh_vector_constructor_iterator(verts, sizeof(NiPoint3), 3, &emptyPointConstructor);
            NavMeshTriangle* ptri = lastNode.pNavMesh->GetTriangle(lastNode.sTriangle);
            uint16_t Vert = ptri->GetVert(0);
            verts[0] = lastNode.pNavMesh->GetVertex(Vert)->Location;
            uint16_t v5 = ptri->GetVert(1u);
            verts[1] = lastNode.pNavMesh->GetVertex(v5)->Location;
            uint16_t v6 = ptri->GetVert(2u);
            verts[2] = lastNode.pNavMesh->GetVertex(v6)->Location;
            *apRetPt = *NavMeshMath::RandomPointOnTriangle(&apResult, *verts, verts[1], verts[2]);
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

// Each failure path computes the straight-line distance itself.
bool Pathing::ComputeApproximatePathingDistance(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    PathingAvoidNodeArray* apAvoidNodeArray,
    float* apReturnDistance,
    BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodes
) {
    static char bOutputDebugGeometry;
    using keepSmartPointers_t = BSSimpleArray<NavMeshPtr, 1024>;
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using restrictSearchInfos_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    keepSmartPointers_t keepSmartPointers;
    Pathing::GetAllLoadedNavmeshes(&keepSmartPointers);
    restrictSearchInfos_t restrictSearchInfos;
    for (uint32_t i = 0; i < keepSmartPointers.iSize; ++i) {
        const NavMeshInfo* pInfo = keepSmartPointers.GetAt(static_cast<int32_t>(i))->GetNavMeshInfo();
        restrictSearchInfos.Add(pInfo);
    }
    if (!aStartLocation.ResolveToClosestNavmeshAndTriangle()) {
        if (apReturnDistance) {
            NiPoint3 kStart = aStartLocation.GetLocation();
            *apReturnDistance = (aGoalLocation.GetLocation() - kStart).Length();
        }
        bool bResult = !aGoalLocation.ResolveToClosestNavmeshAndTriangle();
        return bResult;
    }
    if (!aGoalLocation.ResolveToClosestNavmeshAndTriangle()) {
        if (apReturnDistance) {
            NiPoint3 kStart = aStartLocation.GetLocation();
            *apReturnDistance = (aGoalLocation.GetLocation() - kStart).Length();
        }
        return false;
    }
    NavMeshPtr spStartNavMesh;
    uint16_t usStartTri = 0xFFFF;
    if (!aStartLocation.GetNavMeshAndTriangle(spStartNavMesh, usStartTri)) {
        if (apReturnDistance) {
            NiPoint3 kStart = aStartLocation.GetLocation();
            *apReturnDistance = (aGoalLocation.GetLocation() - kStart).Length();
        }
        return false;
    }
    // Only the navmesh and triangle are set: BuildNodePath reads no more of the start and goal nodes.
    NavMeshSearchNode startNode;
    startNode.pNavMesh = spStartNavMesh.operator NavMesh*();
    startNode.sTriangle = usStartTri;
    // Both locations are fetched here and never read, as the game does.
    NiPoint3 startLocation = aStartLocation.GetLocation();
    NavMeshPtr spGoalNavMesh;
    uint16_t usGoalTri = 0xFFFF;
    if (!aGoalLocation.GetNavMeshAndTriangle(spGoalNavMesh, usGoalTri)) {
        if (apReturnDistance) {
            NiPoint3 kStart = aStartLocation.GetLocation();
            *apReturnDistance = (aGoalLocation.GetLocation() - kStart).Length();
        }
        return false;
    }
    NavMeshSearchNode goalNode;
    goalNode.pNavMesh = spGoalNavMesh.operator NavMesh*();
    goalNode.sTriangle = usGoalTri;
    NiPoint3 goalLocation = aGoalLocation.GetLocation();
    NavMeshSearch search;
    nodes_t nodes;
    BSSimpleArray<NavMeshSearchNode, 1024>* pNodes = apReturnNodes ? apReturnNodes : &nodes;
    pNodes->Clear(false);
    if (pNodes->iReservedSize < 32) {
        pNodes->SetReservedSize(32u);
    }
    links_t links(32u, 0, nullptr);
    PathingRequest pathingRequest;
    pathingRequest.SetAvoidNodeArray(apAvoidNodeArray);
    if (!search.BuildNodePath(&pathingRequest, startNode, goalNode, pNodes, &links, &restrictSearchInfos)) {
        if (apReturnDistance) {
            NiPoint3 kStart = aStartLocation.GetLocation();
            *apReturnDistance = (aGoalLocation.GetLocation() - kStart).Length();
        }
        return false;
    }
    NavMeshSearchNode* pStartLocationNode = &pNodes->GetAt(0);
    pStartLocationNode->SetCenter(aStartLocation.GetLocation());
    if (pNodes->iSize > 1) {
        NavMeshSearchNode* pGoalLocationNode = &pNodes->GetAt(static_cast<int32_t>(pNodes->iSize - 1));
        pGoalLocationNode->SetCenter(aGoalLocation.GetLocation());
    }
    // The debug flow log (an empty function in this build) gets each float as its 4 bytes.
    if (bOutputDebugGeometry) {
        InitializationFlow(pNodes, std::bit_cast<int32_t>(0.0f), 0);
    }
    if (pNodes->iSize > 2 && (apReturnDistance || apReturnNodes)) {
        // Drops the nodes a straight line from each node's center passes through the portal edges to.
        for (uint32_t i = 0; i < pNodes->iSize - 2; ++i) {
            NavMeshSearchNode* pStartNode = &pNodes->GetAt(static_cast<int32_t>(i));
            NiPoint3 startNodeCenter;
            pStartNode->GetCenter(&startNodeCenter);
            NavMeshSearchLink* pStartNodeLink = &links.GetAt(static_cast<int32_t>(i));
            NiPoint3 kLeft;
            NiPoint3 kRight;
            pStartNodeLink->pNavMesh->GetEdgeVertices(
                pStartNodeLink->sTriangle, pStartNodeLink->iEdgeIndex, &kLeft, &kRight
            );
            uint32_t j;
            for (j = i + 2; j < pNodes->iSize; ++j) {
                NavMeshSearchNode* pAdvanceNode = &pNodes->GetAt(static_cast<int32_t>(j));
                NiPoint3 advanceNodeCenter;
                pAdvanceNode->GetCenter(&advanceNodeCenter);
                uint32_t uiLink = j - 1;
                NavMeshSearchLink* pParentLink = &links.GetAt(static_cast<int32_t>(uiLink));
                NiPoint3 kVert0;
                NiPoint3 kVert1;
                pParentLink->pNavMesh->GetEdgeVertices(
                    pParentLink->sTriangle, pParentLink->iEdgeIndex, &kVert0, &kVert1
                );
                NiPoint3 kIntersection;
                if (NavMeshMath::LineSegmentToLine2DIntersection(
                        &kVert0, &kVert1, &startNodeCenter, &kLeft, &kIntersection
                    )) {
                    kLeft = kIntersection;
                }
                else {
                    kLeft = kVert0;
                }
                if (NavMeshMath::LineSegmentToLine2DIntersection(
                        &kVert0, &kVert1, &startNodeCenter, &kRight, &kIntersection
                    )) {
                    kRight = kIntersection;
                }
                else {
                    kRight = kVert1;
                }
                if (bOutputDebugGeometry) {
                    float fValue;
                    if (j % 2) {
                        fValue = 20.0f;
                    }
                    else {
                        fValue = 0.0f;
                    }
                    InitializationFlow(pParentLink, j - 1, std::bit_cast<int32_t>(fValue), 3);
                }
                if (!NavMeshMath::Find2DIntersection3(startNodeCenter, advanceNodeCenter, kLeft, kRight)) {
                    break;
                }
            }
            if (j > i + 2) {
                uint32_t uiCount = j - i - 2;
                pNodes->RemoveRange(i + 1, uiCount, false);
                links.RemoveRange(i, uiCount, false);
            }
        }
        if (bOutputDebugGeometry) {
            InitializationFlow(pNodes, std::bit_cast<int32_t>(20.0f), 2);
        }
        // Moves each inner node's center to the nearer vertex of the edge it was entered through.
        for (uint32_t i = 1; i < pNodes->iSize - 1; ++i) {
            NavMeshSearchNode* pNode = &pNodes->GetAt(static_cast<int32_t>(i));
            NavMeshSearchNode* pPrevNode = &pNodes->GetAt(static_cast<int32_t>(i - 1));
            NavMeshSearchLink* pInLink = &links.GetAt(static_cast<int32_t>(i - 1));
            NiPoint3 prevLocation;
            pPrevNode->GetCenter(&prevLocation);
            NiPoint3 kVert0;
            NiPoint3 kVert1;
            pInLink->pNavMesh->GetEdgeVertices(pInLink->sTriangle, pInLink->iEdgeIndex, &kVert0, &kVert1);
            NiPoint3 kToVert0 = prevLocation - kVert0;
            NiPoint3 kToVert1 = prevLocation - kVert1;
            // The first squared length is kept as a double for the compare.
            double fSqrToVert0 = kToVert0.SqrLength();
            if (kToVert1.SqrLength() > fSqrToVert0) {
                pNode->SetCenter(kVert0);
            }
            else {
                pNode->SetCenter(kVert1);
            }
        }
    }
    if (apReturnDistance) {
        *apReturnDistance = 0.0f;
        if (pNodes->iSize > 1) {
            NiPoint3 lastLocation;
            pNodes->GetAt(0).GetCenter(&lastLocation);
            for (uint32_t i = 1; i < pNodes->iSize; ++i) {
                NiPoint3 currentLocation;
                pNodes->GetAt(static_cast<int32_t>(i)).GetCenter(&currentLocation);
                *apReturnDistance = (currentLocation - lastLocation).Length() + *apReturnDistance;
                lastLocation = currentLocation;
            }
        }
    }
    if (bOutputDebugGeometry) {
        InitializationFlow(pNodes, std::bit_cast<int32_t>(40.0f), 1);
    }
    bOutputDebugGeometry = 0;
    return true;
}

bool Pathing::CanCreatePath(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    PathingAvoidNodeArray* apAvoidNodeArray
) {
    return ComputeApproximatePathingDistance(aStartLocation, aGoalLocation, apAvoidNodeArray, nullptr, nullptr);
}

Pathing::UPDATE_DETAILED_PATH_RESULT Pathing::UpdateSolutionMoveToHigh(
    PathingRequest* apRequest,
    PathingSolution* apSolution,
    uint32_t aiCurrentVirtualNodeIndex
) {
    alignas(PathBuilder) uint8_t kBuilderMemory[sizeof(PathBuilder)];
    PathBuilder* pBuilder = ::new (kBuilderMemory) PathBuilder(apRequest, apSolution);
    int32_t iResult = pBuilder->UpdatePathMoveToHigh(static_cast<int32_t>(aiCurrentVirtualNodeIndex));
    pBuilder->~PathBuilder();
    return static_cast<UPDATE_DETAILED_PATH_RESULT>(iResult);
}

// The builder is a local, constructed and destroyed within this function.
Pathing::UPDATE_DETAILED_PATH_RESULT
Pathing::UpdateSolutionMoveToLow(PathingRequest* apRequest, PathingSolution* apSolution) {
    alignas(PathBuilder) uint8_t aBuilderMemory[sizeof(PathBuilder)];
    PathBuilder* pBuilder = ::new (aBuilderMemory) PathBuilder(apRequest, apSolution);
    int32_t iResult = pBuilder->UpdatePathMoveToLow();
    pBuilder->~PathBuilder();
    return static_cast<UPDATE_DETAILED_PATH_RESULT>(iResult);
}

Pathing::FIND_CLOSEST_EDGES_RESULT Pathing::FindClosestEdges(
    PathingLocation& aLocation,
    uint32_t aiMaxCount,
    BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges
) {
    using navmeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    using FoundEdges_t = BSSimpleArray<FastNavMeshEdgeLocationProxy, 1024>;
    aLocation.ResolveNavMeshInfo(nullptr);
    if (aLocation.HasNavMeshData()) {
        return Pathing::FIND_CLOSEST_EDGES_ONTRI;
    }
    float fMaxDist = fFindClosestEdgesRadius.Float();
    float fSqrMaxDist = fMaxDist * fMaxDist;
    navmeshes_t navmeshes;
    Pathing::FindPotentialNavmeshesForLocation(aLocation, fMaxDist, &navmeshes);
    FoundEdges_t FoundEdges;
    for (uint32_t n = 0; n < navmeshes.iSize; ++n) {
        NavMeshPtr spmesh(navmeshes.GetAt(static_cast<int32_t>(n)));
        for (uint32_t t = 0; t < spmesh.m_pObject->GetTriangleCount(); ++t) {
            NavMeshTriangle* ptri = spmesh.m_pObject->GetTriangle(static_cast<uint16_t>(t));
            for (int32_t e = 0; e < 3; ++e) {
                if (ptri->GetTriOrExtraInfo(e) != 0xFFFF) {
                    continue;
                }
                NavMeshVertex* pvert1 = spmesh.m_pObject->GetVertex(ptri->GetVert(static_cast<uint32_t>(e)));
                uint32_t uiNext = e + 1;
                if (uiNext == 3) {
                    uiNext = 0;
                }
                NavMeshVertex* pvert2 = spmesh.m_pObject->GetVertex(ptri->GetVert(uiNext));
                NiPoint3 edgePt =
                    NavMeshMath::ProjectPointOntoSegment(aLocation.GetLocation(), pvert1->Location, pvert2->Location);
                NiPoint3 delta = aLocation.GetLocation() - edgePt;
                float fsqrDist = delta.SqrXYLength();
                if (fSqrMaxDist > fsqrDist) {
                    FastNavMeshEdgeLocationProxy proxy;
                    proxy.~FastNavMeshEdgeLocationProxy();
                    ::new (&proxy) FastNavMeshEdgeLocationProxy;
                    proxy.Edge.pNavMesh = spmesh.m_pObject;
                    proxy.Edge.usTriangle = static_cast<uint16_t>(t);
                    proxy.Edge.iEdgeIndex = e;
                    proxy.Edge.Location =
                        edgePt + (spmesh.m_pObject->GetCenter(static_cast<uint16_t>(t)) - edgePt) * 0.1f;
                    proxy.fzDelta = delta.z;
                    proxy.fSqrDistToLocation = fsqrDist;
                    FoundEdges.Add(proxy);
                }
            }
        }
    }
    if (FoundEdges.IsEmpty()) {
        return Pathing::FIND_CLOSEST_EDGES_NONE;
    }
    std::qsort(&FoundEdges.GetAt(0), FoundEdges.iSize, sizeof(FastNavMeshEdgeLocationProxy), &EdgeSortFunction);
    uint32_t icount = BSMIN(aiMaxCount, FoundEdges.iSize);
    for (uint32_t i = 0; i < icount; ++i) {
        NavMeshEdgeLocation edgeLoc;
        aRetEdges.Add(*FNVD_RECONSTRUCT(NavMeshEdgeLocation)(&edgeLoc, FoundEdges.GetAt(static_cast<int32_t>(i)).Edge));
    }
    if (icount) {
        return Pathing::FIND_CLOSEST_EDGES_OK;
    }
    return Pathing::FIND_CLOSEST_EDGES_NONE;
}

char Pathing::FindSafeStraightLinePathPoint(TESObjectREFR* a1, TESObjectREFR* a2, float a3, NiPoint3* a4) {
    FNVD_INLINED_LOCAL(PathingLocation, v14, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
    PathingRequestSafeStraightLine straightLineRequest;
    PathingLocation* apOrigin = ::new (&apLocation) PathingLocation(a1);
    straightLineRequest.SetOrigin(*apOrigin);
    apLocation.~PathingLocation();
    PathingLocation* aDestination = ::new (&v14) PathingLocation(a2);
    straightLineRequest.SetDestination(*aDestination);
    v14.~PathingLocation();
    NiPoint3 v18 = a1->GetBoundMax();
    NiPoint3 v22 = a1->GetBoundMin();
    float fwidth1 = v18.x - v22.x;
    float fwidth2 = v18.y - v22.y;
    float fradius = BSMAX(fwidth1, fwidth2);
    double Scale = a1->GetScale();
    fradius = Scale * fradius;
    fradius = BSMAX(fradius, 32.0f);
    float factorRadius = fradius * 0.5;
    float v25 = factorRadius;
    straightLineRequest.SetActorRadius(factorRadius);
    v18 = a2->GetBoundMax();
    v22 = a2->GetBoundMin();
    fwidth1 = v18.x - v22.x;
    fwidth2 = v18.y - v22.y;
    factorRadius = BSMAX(fwidth1, fwidth2);
    double v5 = a2->GetScale();
    factorRadius = v5 * factorRadius;
    factorRadius = BSMAX(factorRadius, 32.0f);
    float ftargetRadius = factorRadius * 0.5;
    straightLineRequest.SetTargetRadius(ftargetRadius);
    float v8 = a3 * 0.8999999761581421 + v25;
    straightLineRequest.SetMinDistance(v8);
    float v7 = a3 + v25;
    straightLineRequest.SetMaxDistance(v7);
    char SafeStraightLinePathPoint = Pathing::FindSafeStraightLinePathPoint(&straightLineRequest, a4);
    return SafeStraightLinePathPoint;
}

char Pathing::FindSafeStraightLinePathPoint(PathingRequestSafeStraightLine* a1, NiPoint3* a2) {
    float arfNewSplineT;
    NavMeshSearchNode aGoalNode;
    using apMemory_t = BSSimpleArray<NavMeshPtr, 1024>;
    NavMeshSearchNode aStartNode;
    if (!(&a1->Origin)->ResolveToClosestNavmeshAndTriangle()) {
        return 0;
    }
    NavMeshPtr arNavMesh;
    uint16_t arTriangle = -1;
    if (!(&a1->Origin)->GetNavMeshAndTriangle(arNavMesh, arTriangle) ||
        (aStartNode.pNavMesh = arNavMesh.operator NavMesh*(),
         (aStartNode.sTriangle = arTriangle, !(&a1->Destination)->ResolveToClosestNavmeshAndTriangle()))) {
        return 0;
    }
    NavMeshPtr apNavMesh;
    uint16_t apusTriangle = -1;
    if (!(&a1->Destination)->GetNavMeshAndTriangle(apNavMesh, apusTriangle)) {
        return 0;
    }
    aGoalNode.pNavMesh = apNavMesh.operator NavMesh*();
    aGoalNode.sTriangle = apusTriangle;
    NavMeshSearchSLPoint apThis;
    apMemory_t apMemory;
    Pathing::GetAllLoadedNavmeshes(&apMemory);
    if (apThis.FindStraightLinePath(a1, aStartNode, aGoalNode)) {
        apThis.GetGoalLocation(*a2);
        return 1;
    }
    else {
        PathingRequest arg0;
        a1->PathingRequest::CopyTo(&arg0);
        PathingSolution apSolution;
        if (Pathing::Pathfind(&arg0, &apSolution)) {
            float afDist = a1->fMinDistance;
            float afCurrentParameter = static_cast<float>(apSolution.GetNodeCount() - 1);
            if (apSolution.FindPointOnPath(afCurrentParameter, afDist, false, &arfNewSplineT, a2)) {
                return 1;
            }
            PathingRequestClosePoint apTarget;
            a1->PathingRequest::CopyTo(&apTarget);
            apTarget.SetOrigin(*&a1->Destination);
            apTarget.SetMinDistance(a1->fMinDistance);
            apTarget.SetMaxDistance(a1->fMaxDistance);
            if (Pathing::FindClosePointViaNavMesh(&apTarget, a2)) {
                return 1;
            }
        }
        return 0;
    }
}

Pathing::UPDATE_DETAILED_PATH_RESULT Pathing::UpdateSolution(
    PathingRequest* apRequest,
    PathingSolution* apSolution,
    uint32_t aiCurrentVirtualNodeIndex,
    const NiPoint3& aCurrentActorLocation
) {
    PathBuilder builder(apRequest, apSolution);
    Pathing::UPDATE_DETAILED_PATH_RESULT updated =
        builder.UpdateDetailedPath(aiCurrentVirtualNodeIndex, aCurrentActorLocation);
    return updated;
}

bool Pathing::ComputeLowPathDistance(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    float* apReturnDistance
) {
    TeleportPath apMemory;
    if (Pathing::BuildLowPath(
            aStartLocation, aGoalLocation, &apMemory, nullptr, Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS
        )) {
        *apReturnDistance = apMemory.ComputeLength();
        return true;
    }
    else {
        *apReturnDistance = 0.0;
        return false;
    }
}

// newIntersection is left unset by NiPoint2's empty constructor, and FindInterectingEdge writes it only when it finds
// an edge; on the -1 path it is still copied into intersection, deliberately, as the game does.
bool Pathing::CheckLineOfSight(
    PathingLocation& aStartLocation,
    const NiPoint3& aGoalLocation,
    NavMesh* apGoalNavMesh,
    uint16_t ausGoalTriangle,
    PathingLocation* apReturnIntersectionLocation,
    Pathing::CheckLineOfSightFilter* apFilter
) {
    uint16_t usstartTri;
    if (apReturnIntersectionLocation) {
        apReturnIntersectionLocation->operator=(aStartLocation);
    }
    if (!aStartLocation.ResolveNavMeshInfo(nullptr)) {
        return apGoalNavMesh == nullptr;
    }
    NavMeshPtr spstartMesh;
    if (!aStartLocation.GetNavMeshAndTriangle(spstartMesh, usstartTri)) {
        return false;
    }
    // Takes the start's y from a first GetLocation call and its x from a second one.
    float fStartY = aStartLocation.GetLocation().y;
    float fStartX = aStartLocation.GetLocation().x;
    NiPoint2 start(fStartX, fStartY);
    NiPoint2 end(aGoalLocation.x, aGoalLocation.y);
    NiPoint2 intersection;
    NavMeshPtr pcurrentNavMesh(spstartMesh);
    uint16_t uscurrentTriangle = usstartTri;
    int32_t currentEdgeIndex =
        spstartMesh.m_pObject->FindInterectingEdge(uscurrentTriangle, -1, start, end, &intersection);
    while (currentEdgeIndex != -1) {
        uint16_t arOtherTriangleIndex;
        int32_t arOtherEdgeIndex;
        NavMeshPtr potherMesh;
        bool bfoundMatchingEdge = pcurrentNavMesh.m_pObject->GetMatchingEdge_0(
            uscurrentTriangle, currentEdgeIndex, &potherMesh, &arOtherTriangleIndex, &arOtherEdgeIndex
        );
        if (bfoundMatchingEdge && apFilter) {
            FastNavMeshEdgeHandle hEdge(pcurrentNavMesh.m_pObject, uscurrentTriangle, currentEdgeIndex);
            FastNavMeshEdgeHandle hMatchingEdge(potherMesh.m_pObject, arOtherTriangleIndex, arOtherEdgeIndex);
            // Both by-value arguments are copied with the copy constructor, the second one first.
            FastNavMeshEdgeHandle hMatchingEdgeArg(hMatchingEdge);
            FastNavMeshEdgeHandle hEdgeArg(hEdge);
            bfoundMatchingEdge = apFilter->CanCrossEdge(hEdgeArg, hMatchingEdgeArg);
        }
        if (bfoundMatchingEdge) {
            EdgeExtraInfo* pEdgeInfo = pcurrentNavMesh.m_pObject->GetEdgeExtraInfo(uscurrentTriangle, currentEdgeIndex);
            if (pEdgeInfo && pEdgeInfo->eType == EIT_LEDGE_UP) {
                bfoundMatchingEdge = false;
            }
        }
        if (!bfoundMatchingEdge) {
            if (apReturnIntersectionLocation) {
                apReturnIntersectionLocation->SetNavMeshInfo(pcurrentNavMesh.m_pObject->GetNavMeshInfo());
                apReturnIntersectionLocation->SetTriangleIndex(uscurrentTriangle);
                NiPoint3 center = pcurrentNavMesh.m_pObject->GetCenter(uscurrentTriangle);
                NiPoint3 hitPoint;
                apReturnIntersectionLocation->SetLocation(*::new (&hitPoint)
                                                              NiPoint3(intersection.x, intersection.y, center.z));
            }
            return false;
        }
        if (potherMesh.operator!=(pcurrentNavMesh)) {
            NavMeshEdgeVertices otherEdgeVerts =
                potherMesh.m_pObject->GetEdgeVertices(arOtherTriangleIndex, arOtherEdgeIndex);
            NiPoint2 otherEdgeVert1(otherEdgeVerts.Vertices[0]->Location.x, otherEdgeVerts.Vertices[0]->Location.y);
            NiPoint2 otherEdgeVert2(otherEdgeVerts.Vertices[1]->Location.x, otherEdgeVerts.Vertices[1]->Location.y);
            intersection = NavMeshMath::ClosestPointOnSegmentToLine(otherEdgeVert1, otherEdgeVert2, intersection, end);
        }
        uscurrentTriangle = arOtherTriangleIndex;
        pcurrentNavMesh.operator=(potherMesh);
        NiPoint2 newIntersection;
        currentEdgeIndex = potherMesh.m_pObject->FindInterectingEdge(
            arOtherTriangleIndex, arOtherEdgeIndex, intersection, end, &newIntersection
        );
        intersection = newIntersection;
    }
    // 0xFFFF means no goal triangle; Pathing::CheckLineOfSight passes it with no goal navmesh.
    if (!apGoalNavMesh || ausGoalTriangle == 0xFFFF) {
        if (apReturnIntersectionLocation) {
            apReturnIntersectionLocation->SetNavMeshInfo(pcurrentNavMesh.m_pObject->GetNavMeshInfo());
            apReturnIntersectionLocation->SetTriangleIndex(uscurrentTriangle);
            apReturnIntersectionLocation->SetLocation(aGoalLocation);
        }
        return true;
    }
    if (pcurrentNavMesh.m_pObject == apGoalNavMesh && uscurrentTriangle == ausGoalTriangle) {
        if (apReturnIntersectionLocation) {
            PathingLocation goalLoc(aGoalLocation, apGoalNavMesh->GetNavMeshInfo(), ausGoalTriangle);
            apReturnIntersectionLocation->operator=(goalLoc);
        }
        return true;
    }
    return false;
}

bool Pathing::ComputeLineOfSightMap(
    NiPointer<PathingLOSGridMap>& aspGridMap,
    BSSimpleArray<PathingLocation, 1024>& aLocations,
    BSSimpleArray<float, 1024>* apHeadings
) {
    void* v8;
    NiPoint3 v9;
    if (!aspGridMap.m_pObject->IsValid()) {
        return false;
    }
    void* apMemory = NiMemObject::operator new(sizeof(PathingLOSMap));
    if (apMemory) {
        float afHeading = aspGridMap.m_pObject->fRadius;
        NiPoint3 v22 = *aspGridMap.m_pObject->GetLocation(&v9);
        v8 = (void*)::new (apMemory) PathingLOSMap(v22, afHeading);
    }
    else {
        v8 = nullptr;
    }
    NiPointer<PathingLOSMap> spLosMap(static_cast<PathingLOSMap*>(v8));
    aspGridMap.m_pObject->SetLOSMapFromSeenTriangles(spLosMap);
    int32_t ilocationsSize = aLocations.iSize;
    for (int32_t l = 0; l < ilocationsSize; ++l) {
        float fheading__goalLoc_2;
        PathingLocation* fheading__goalLoc = &aLocations.GetAt(l);
        if (apHeadings) {
            fheading__goalLoc_2 = apHeadings->operator[](l);
        }
        else {
            fheading__goalLoc_2 = -1.0f;
        }
        float afSightRadius = aspGridMap.m_pObject->fSightRadius;
        Pathing::ComputeLineOfSightMapForLocation(
            *fheading__goalLoc, spLosMap, true, afSightRadius, fheading__goalLoc_2
        );
    }
    // The scored triangles: a positive score is seen, zero or less unseen, 127 not scored.
    uint32_t uiSeen = 0;
    uint32_t uiUnseen = 0;
    for (uint32_t aiIndex = 0; aiIndex < spLosMap.m_pObject->iTotalTriangleCount; ++aiIndex) {
        char cscore = spLosMap.m_pObject->GetTriangleScore(aiIndex);
        if (cscore != 127) {
            if (cscore <= 0) {
                ++uiUnseen;
            }
            else {
                ++uiSeen;
            }
        }
    }
    spLosMap.m_pObject->iTotalTrianglesSeen = uiSeen;
    spLosMap.m_pObject->iTotalTrianglesUnseen = uiUnseen;
    aspGridMap.m_pObject->SetSeenTrianglesFromLOSMap(spLosMap);
    return true;
}

// Scores the triangles of aspRetMap's navmeshes by whether aLocation sees them: a triangle seen gets its neighbour
// count, one not seen the negated count. A triangle outside the sight radius or behind the heading is not seen.
bool Pathing::ComputeLineOfSightMapForLocation(
    PathingLocation& aLocation,
    NiPointer<PathingLOSMap>& aspRetMap,
    bool abClearUnseen,
    float afSightRadius,
    float afHeading
) {
    using CrossedTriangleArray = BSScrapArray<CrossedTriangle, 1024>;
    if (!aspRetMap->GetNavMeshCount()) {
        return false;
    }
    // Counters that are kept but never read, as the game does.
    [[maybe_unused]] uint32_t iIntersectionCount = 0;
    [[maybe_unused]] uint32_t ilineOfSightTests = 0;
    if (!aLocation.ResolveNavMeshInfo(nullptr)) {
        return false;
    }
    NavMeshPtr spGoalMesh;
    uint16_t goalTriangle = 0xFFFF;
    if (!aLocation.GetNavMeshAndTriangle(spGoalMesh, goalTriangle)) {
        return false;
    }
    if (abClearUnseen) {
        aspRetMap->ClearUnseenTriangles();
    }
    NiPoint3 goalLocation = aLocation.GetLocation();
    NiPoint3 searchCenter = aspRetMap->GetCenter();
    float fsearchRadius = aspRetMap->fRadius;
    float fsearchRadiusSquared = fsearchRadius * fsearchRadius;
    NiPoint3 headingVector = NiPoint3_ZERO;
    if (afHeading >= 0.0) {
        NiSinCos(afHeading, headingVector.x, headingVector.y);
    }
    for (uint32_t n = 0; n < aspRetMap->GetNavMeshCount(); ++n) {
        NavMeshPtr spmesh(aspRetMap->GetNavMeshByIndex(n));
        uint32_t itriangleOffset = 0;
        if (!aspRetMap->GetOffsetForNavMesh(spmesh->GetFormID(), itriangleOffset)) {
            continue;
        }
        uint32_t icurrentTri = 0;
        while (icurrentTri < spmesh->GetTriangleCount()) {
            // Skip the triangles already scored or disabled.
            do {
                if (aspRetMap->GetTriangleScore(icurrentTri + itriangleOffset) == 127 &&
                    !spmesh->GetTriangle(icurrentTri)->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) {
                    break;
                }
                ++icurrentTri;
            }
            while (icurrentTri < spmesh->GetTriangleCount());
            if (icurrentTri >= spmesh->GetTriangleCount()) {
                break;
            }
            ++ilineOfSightTests;
            NiPoint3 center = spmesh->GetCenter(icurrentTri);
            if (fsearchRadius != 0.0) {
                // The search radius is tested with the height difference tripled.
                NiPoint3 vec = center - searchCenter;
                vec.z = vec.z * 3.0;
                if (fsearchRadiusSquared < vec.SqrLength()) {
                    ++icurrentTri;
                    continue;
                }
            }
            if (afSightRadius > 0.0 || afHeading >= 0.0) {
                bool bskipTriangle = false;
                NiPoint3 vec = center - aLocation.GetLocation();
                if (afSightRadius > 0.0 && afSightRadius * afSightRadius < vec.SqrLength()) {
                    bskipTriangle = true;
                }
                if (!bskipTriangle && afHeading >= 0.0 && headingVector.Dot(vec) < 0.0) {
                    bskipTriangle = true;
                }
                if (bskipTriangle) {
                    if (aspRetMap->GetTriangleScore(icurrentTri + itriangleOffset) == 127) {
                        aspRetMap->SetAt(icurrentTri + itriangleOffset, -spmesh->GetNeighborCount(icurrentTri));
                    }
                    ++icurrentTri;
                    continue;
                }
            }
            // Walk the line from the triangle's center to the goal across the triangles it crosses.
            NiPoint2 start(center.x, center.y);
            NiPoint2 end(goalLocation.x, goalLocation.y);
            NiPoint2 intersection;
            uint16_t currentTriangle = static_cast<uint16_t>(icurrentTri);
            NavMesh* pcurrentNavMesh = spmesh.m_pObject;
            uint32_t icurrentOffset = itriangleOffset;
            bool bLineOfSightFound = false;
            CrossedTriangleArray crossedTriangles(64u, 0u, nullptr);
            CrossedTriangle firstTriangle(
                currentTriangle + icurrentOffset, pcurrentNavMesh->GetNeighborCount(currentTriangle)
            );
            crossedTriangles.Add(firstTriangle);
            if (currentTriangle == goalTriangle && pcurrentNavMesh == spGoalMesh.m_pObject) {
                bLineOfSightFound = true;
            }
            else {
                int32_t currentEdgeIndex =
                    pcurrentNavMesh->FindInterectingEdge(currentTriangle, -1, start, end, &intersection);
                ++iIntersectionCount;
                while (currentEdgeIndex != -1) {
                    // GetMatchingEdge fills these when it returns true; they are read only then.
                    NavMesh* potherMesh;
                    uint16_t usotherTri;
                    int32_t iotherEdge;
                    if (!pcurrentNavMesh->GetMatchingEdge(
                            currentTriangle, currentEdgeIndex, &potherMesh, &usotherTri, &iotherEdge
                        )) {
                        break;
                    }
                    EdgeExtraInfo* pextra = pcurrentNavMesh->GetEdgeExtraInfo(currentTriangle, currentEdgeIndex);
                    if (pextra && pextra->eType == EIT_LEDGE_UP) {
                        break;
                    }
                    if (potherMesh != pcurrentNavMesh) {
                        if (!aspRetMap->GetOffsetForNavMesh(potherMesh->GetFormID(), icurrentOffset)) {
                            break;
                        }
                        // Across a navmesh seam, continue from the point of the other mesh's edge nearest the line.
                        NavMeshEdgeVertices otherEdgeVerts = potherMesh->GetEdgeVertices(usotherTri, iotherEdge);
                        NiPoint2 otherEdgeVert1(
                            otherEdgeVerts.Vertices[0]->Location.x, otherEdgeVerts.Vertices[0]->Location.y
                        );
                        NiPoint2 otherEdgeVert2(
                            otherEdgeVerts.Vertices[1]->Location.x, otherEdgeVerts.Vertices[1]->Location.y
                        );
                        intersection =
                            NavMeshMath::ClosestPointOnSegmentToLine(otherEdgeVert1, otherEdgeVert2, intersection, end);
                    }
                    pcurrentNavMesh = potherMesh;
                    currentTriangle = usotherTri;
                    CrossedTriangle nextTriangle(
                        currentTriangle + icurrentOffset, pcurrentNavMesh->GetNeighborCount(currentTriangle)
                    );
                    crossedTriangles.Add(nextTriangle);
                    NiPoint2 newIntersection;
                    currentEdgeIndex =
                        potherMesh->FindInterectingEdge(usotherTri, iotherEdge, intersection, end, &newIntersection);
                    intersection = newIntersection;
                    ++iIntersectionCount;
                    if (currentTriangle == goalTriangle && pcurrentNavMesh == spGoalMesh.m_pObject) {
                        bLineOfSightFound = true;
                        break;
                    }
                }
            }
            if (bLineOfSightFound) {
                // Every crossed triangle is seen: its score is its neighbour count.
                for (uint32_t i = 0; i < crossedTriangles.iSize; ++i) {
                    uint8_t ucNeighbors = crossedTriangles.GetAt(i).iNeighborCount;
                    aspRetMap->SetAt(crossedTriangles.GetAt(i).iTriangleIndex, ucNeighbors);
                }
                for (uint32_t i = 0; i < crossedTriangles.iSize; ++i) {
                    if (static_cast<int8_t>(aspRetMap->iMaxScore) == 3) {
                        break;
                    }
                    if (static_cast<int8_t>(crossedTriangles.GetAt(i).iNeighborCount) >
                        static_cast<int8_t>(aspRetMap->iMaxScore)) {
                        aspRetMap->iMaxScore = crossedTriangles.GetAt(i).iNeighborCount;
                    }
                }
            }
            else {
                // The crossed triangles not scored yet are unseen: the negated neighbour count.
                for (uint32_t i = 0; i < crossedTriangles.iSize; ++i) {
                    if (aspRetMap->GetTriangleScore(crossedTriangles.GetAt(i).iTriangleIndex) == 127) {
                        int32_t iScore = -static_cast<int8_t>(crossedTriangles.GetAt(i).iNeighborCount);
                        aspRetMap->SetAt(crossedTriangles.GetAt(i).iTriangleIndex, iScore);
                    }
                }
                for (uint32_t i = 0; i < crossedTriangles.iSize; ++i) {
                    if (aspRetMap->iMinScore == -3) {
                        break;
                    }
                    if (-static_cast<int8_t>(crossedTriangles.GetAt(i).iNeighborCount) < aspRetMap->iMinScore) {
                        aspRetMap->iMinScore = -static_cast<int8_t>(crossedTriangles.GetAt(i).iNeighborCount);
                    }
                }
            }
            ++icurrentTri;
        }
    }
    return true;
}

bool Pathing::FindPotentialNavmeshesForLocation(
    PathingLocation& aLocation,
    float afRadius,
    BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes
) {
    return Pathing::FindNavmeshesForLocation(aLocation, afRadius, apRetNavMeshes, false);
}

bool Pathing::GetAllLoadedNavmeshes(BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes) {
    FNVD_INLINED_LOCAL(NavMeshPtr, apMemory, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NavMeshPtr, apResult, std::nullopt, std::nullopt);
    if (TES::pInstance->pInteriorCell) {
        NavMeshArray* parray = TES::pInstance->pInteriorCell->GetNavMeshArray();
        if (parray) {
            for (uint32_t i = 0;; ++i) {
                uint32_t NavMeshCount = parray->GetNavMeshCount();
                if (i >= NavMeshCount) {
                    break;
                }
                NavMeshPtr* aItem = parray->GetNavMeshByIndex(&apResult, i);
                apRetNavMeshes->Add(*aItem);
                FNVD_DESTRUCT(NiPointer<NavMesh>)(&apResult);
            }
        }
    }
    else {
        int32_t isize = TES::pInstance->GetExteriorBufferSize();
        for (uint32_t i_2 = 0; i_2 < isize; ++i_2) {
            TESObjectCELL* pcell = TES::pInstance->GetExteriorBufferCellByIndex(i_2);
            if (pcell) {
                NavMeshArray* parray_2 = pcell->GetNavMeshArray();
                if (parray_2) {
                    for (uint32_t i_3 = 0;; ++i_3) {
                        uint32_t v2 = parray_2->GetNavMeshCount();
                        if (i_3 >= v2) {
                            break;
                        }
                        NavMeshPtr* NavMeshByIndex = parray_2->GetNavMeshByIndex(&apMemory, i_3);
                        apRetNavMeshes->Add(*NavMeshByIndex);
                        FNVD_DESTRUCT(NiPointer<NavMesh>)(&apMemory);
                    }
                }
            }
        }
    }
    return true;
}

uint32_t Pathing::FindTeleportDoorsForLocation(
    TESForm* apSpace,
    const NiPoint3& arLocation,
    float afRadius,
    BSSimpleArray<TESObjectREFR*, 1024>* apRetDoors
) {
    TESObjectCELL* pcell;
    if (!apSpace || !apRetDoors) {
        return 0;
    }
    if (apRetDoors->iReservedSize < 32) {
        apRetDoors->SetReservedSize(32u);
    }
    if (apSpace->GetFormType() == WRLD_ID) {
        pcell = static_cast<TESWorldSpace*>(apSpace)->pPersistentCell;
    }
    else {
        pcell = static_cast<TESObjectCELL*>(apSpace);
    }
    if (pcell) {
        return pcell->FindTeleportDoorsForLocation(arLocation, afRadius, apRetDoors);
    }
    else {
        return 0;
    }
}

bool Pathing::CheckSphere(PathingLocation& aSphereCenter, float afRadius) {
    NavMeshTriHandle triHandle;
    NiPoint3 v22;
    NiPoint3 v23;
    NiPoint3 v25;
    NiPoint3 dst;
    using navmeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    using openEdges_t = BSSimpleArray<NavMeshEdgeHandle, 1024>;
    using potentialTriangles_t = BSSimpleArray<NavMeshTriHandle, 1024>;
    potentialTriangles_t potentialTriangles;
    navmeshes_t navmeshes;
    Pathing::FindConnectedNavmeshesForLocation(aSphereCenter, afRadius, &navmeshes);
    for (uint32_t i = 0;; ++i) {
        uint32_t uiNavMeshCount = navmeshes.iSize;
        if (i >= uiNavMeshCount) {
            break;
        }
        using trisForThisMesh_t = BSSimpleArray<uint16_t, 1024>;
        trisForThisMesh_t trisForThisMesh;
        NiPoint3* z = ::new (&dst) NiPoint3(afRadius, afRadius, 0.0f);
        NiPoint3* za = ::new (&v25) NiPoint3(aSphereCenter.GetLocation() + *z);
        NiPoint3* y = ::new (&v23) NiPoint3(afRadius, afRadius, 0.0f);
        NiPoint3* ya = ::new (&v22) NiPoint3(aSphereCenter.GetLocation() - *y);
        navmeshes.GetAt(static_cast<int32_t>(i)).m_pObject->GetTrianglesForAABB(*ya, *za, &trisForThisMesh);
        for (uint32_t j = 0;; ++j) {
            uint32_t uiTriCount = trisForThisMesh.iSize;
            if (j >= uiTriCount) {
                break;
            }
            uint16_t usTriangle = trisForThisMesh.GetAt(static_cast<int32_t>(j));
            NavMeshInfo* pInfo = navmeshes.GetAt(static_cast<int32_t>(i)).m_pObject->GetNavMeshInfo();
            potentialTriangles.Add(*FNVD_RECONSTRUCT(NavMeshTriHandle)(&triHandle, pInfo, usTriangle));
        }
    }
    openEdges_t openEdges;
    for (uint32_t i_2 = 0;; ++i_2) {
        uint32_t uiPotentialCount = potentialTriangles.iSize;
        if (i_2 >= uiPotentialCount) {
            break;
        }
        NavMeshPtr spmesh;
        if (potentialTriangles.GetAt(static_cast<int32_t>(i_2)).pNavMeshInfo->GetNavMesh(spmesh)) {
            for (int32_t e = 0; e < 3; ++e) {
                NavMeshTriangle* Triangle =
                    spmesh.m_pObject->GetTriangle(potentialTriangles.GetAt(static_cast<int32_t>(i_2)).usTriangle);
                if (!Triangle->IsEdgeConnected(e)) {
                    NavMeshEdgeVertices verts = spmesh.m_pObject->GetEdgeVertices(
                        potentialTriangles.GetAt(static_cast<int32_t>(i_2)).usTriangle, e
                    );
                    if (NavMeshMath::SegmentCircleIntersection(
                            verts.Vertices[0]->Location, verts.Vertices[1]->Location, aSphereCenter.GetLocation(),
                            afRadius
                        )) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Pathing::GetNavMeshZForLocation(
    const NiPoint3& aLocation,
    TESObjectCELL* apCell,
    TESWorldSpace* apWorldSpace,
    float& aRetHeight
) {
    PathingLocation tempLoc((&aLocation), apCell, apWorldSpace);
    bool NavMeshZForLocation_0 = Pathing::GetNavMeshZForLocation(tempLoc, aRetHeight);
    return NavMeshZForLocation_0;
}

NiPoint3* Pathing::ComputeInitialTangentForActor(
    NiPoint3* apResult,
    float afHeading,
    const PathingLocation& aLocation,
    const PathingLocation& aNextLocation,
    float afRadius
) {
    NiPoint3 v32;
    NiPoint3 v36;
    NiPoint3 v42;
    // Each PathingLocation is built and destroyed only through its own Construct/Destruct calls below.
    NiMatrix3 rot;
    rot.MakeZRotation(afHeading);
    NiPoint3 direction = rot * NiPoint3_UNIT_Y;
    PathingLocation startLoc(aLocation);
    NiPoint3 start = startLoc.GetLocation();
    NiPoint3 end = start + direction * 256.0f;
    PathingLocation retIntersection;
    if (!Pathing::CheckLineOfSight(startLoc, end, &retIntersection, nullptr)) {
        end = retIntersection.GetLocation();
    }
    NiPoint3 delta = end - start;
    NiPoint3 rightVec = delta.UnitCross(NiPoint3_UNIT_Z) * afRadius;
    PathingLocation startLeft(start - rightVec, startLoc);
    PathingLocation goalLeft(end - rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startLeft, goalLeft, &retIntersection, nullptr)) {
        NiPoint3* a2 = ::new (&v42) NiPoint3(startLeft.GetLocation());
        double v26 = (retIntersection.GetLocation() - *a2).SqrLength();
        if (delta.SqrLength() > v26) {
            end = retIntersection.GetLocation();
        }
    }
    PathingLocation startRight(start + rightVec, startLoc);
    PathingLocation goalRight(end + rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startRight, goalRight, &retIntersection, nullptr)) {
        NiPoint3* a2a = ::new (&v36) NiPoint3(startRight.GetLocation());
        double v25 = (retIntersection.GetLocation() - *a2a).SqrLength();
        if (delta.SqrLength() > v25) {
            end = retIntersection.GetLocation();
        }
    }
    NiPoint3* v16 = ::new (&v32) NiPoint3(direction * afRadius * 1.2f);
    end.operator-=(*v16);
    PathingLocation nextLoc(aNextLocation);
    NiPoint3 next = nextLoc.GetLocation();
    PathingLocation retIntersection2;
    if (Pathing::CheckLineOfSight(nextLoc, end, &retIntersection2, nullptr)) {
        NiPoint3 nextDelta = end - next;
        NiPoint3 nextRightVec = nextDelta.UnitCross(NiPoint3_UNIT_Z) * afRadius;
        PathingLocation nextLeft(next - rightVec, nextLoc);
        PathingLocation nextGoalLeft(end - rightVec, nextLoc);
        if (Pathing::CheckLineOfSight(nextLeft, nextGoalLeft, &retIntersection2, nullptr)) {
            float fdot;
            NiPoint3 deltaToNext;
            float fdeltaToNextLength;
            PathingLocation nextRight(next + rightVec, nextLoc);
            PathingLocation nextGoalRight(end + rightVec, nextLoc);
            if (Pathing::CheckLineOfSight(nextRight, nextGoalRight, &retIntersection2, nullptr) &&
                ((::new (&deltaToNext) NiPoint3(next - start),
                  (fdeltaToNextLength = deltaToNext.UnitizeGetLength(),
                   (fdot = deltaToNext.Dot(direction), !(fdot < -0.699999988079071)))) &&
                 (!(fdot < 0.0) || !(fdeltaToNextLength < 64.0)))) {
                ::new (apResult) NiPoint3(end - start);
                return apResult;
            }
            else {
                *apResult = NiPoint3_ZERO;
                return apResult;
            }
        }
        else {
            *apResult = NiPoint3_ZERO;
            return apResult;
        }
    }
    else {
        *apResult = NiPoint3_ZERO;
        return apResult;
    }
}

NiPoint3* Pathing::ComputeFinalTangentForActor(
    NiPoint3* apResult,
    float afHeading,
    const PathingLocation& aLocation,
    float afRadius
) {
    NiPoint3 v21;
    NiPoint3 v27;
    // Each of these PathingLocations is built and destroyed only through its own Construct/Destruct calls below.
    NiMatrix3 rot;
    rot.MakeZRotation(afHeading);
    NiPoint3 direction = rot * NiPoint3_UNIT_Y;
    PathingLocation startLoc(aLocation);
    NiPoint3 start = startLoc.GetLocation();
    NiPoint3 end = start - direction * 256.0f;
    PathingLocation retIntersection;
    if (!Pathing::CheckLineOfSight(startLoc, end, &retIntersection, nullptr)) {
        end = retIntersection.GetLocation();
    }
    NiPoint3 delta = end - start;
    NiPoint3 rightVec = delta.UnitCross(NiPoint3_UNIT_Z) * afRadius;
    PathingLocation startLeft(start - rightVec, startLoc);
    PathingLocation goalLeft(end - rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startLeft, goalLeft, &retIntersection, nullptr)) {
        NiPoint3* a2 = ::new (&v27) NiPoint3(startLeft.GetLocation());
        double v18 = (retIntersection.GetLocation() - *a2).SqrLength();
        if (delta.SqrLength() > v18) {
            end = retIntersection.GetLocation();
        }
    }
    PathingLocation startRight(start + rightVec, startLoc);
    PathingLocation goalRight(end + rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startRight, goalRight, &retIntersection, nullptr)) {
        NiPoint3* a2a = ::new (&v21) NiPoint3(startRight.GetLocation());
        double v17 = (retIntersection.GetLocation() - *a2a).SqrLength();
        if (delta.SqrLength() > v17) {
            end = retIntersection.GetLocation();
        }
    }
    ::new (apResult) NiPoint3(start - end);
    return apResult;
}

// The double constants are the float two pi over 2^32 and the float pi; the result is rounded to a float local.
float BSRandom::FloatNegPiToPi() {
    uint32_t uiValue = UnsignedInt();
    float fResult = static_cast<float>(uiValue * 1.462918119976564e-09 - 3.1415927410125732);
    return fResult;
}

FastNavMeshEdgeLocationProxy* FastNavMeshEdgeLocationProxy::Assign(
    FastNavMeshEdgeLocationProxy* apThis,
    const FastNavMeshEdgeLocationProxy& arOther
) {
    apThis->Edge.Copy(arOther.Edge);
    apThis->fSqrDistToLocation = arOther.fSqrDistToLocation;
    apThis->fzDelta = arOther.fzDelta;
    return apThis;
}

bool FastNavMeshTriHandle::operator==(const FastNavMeshTriHandle& aNavMeshEdgeHandle) const {
    return (bool)((uint8_t)((pNavMesh == (&aNavMeshEdgeHandle)->pNavMesh) &&
                            (usTriangle == (&aNavMeshEdgeHandle)->usTriangle)));
}

NavMeshTriangle* FastNavMeshTriHandle::GetTriangle() {
    return pNavMesh->GetTriangle(usTriangle);
}

// The char edge index is sign-extended into IsEdgeConnected's argument.
char NavMesh::GetNeighborCount(uint16_t apTriangle) {
    char cretValue = 0;
    for (char e = 0; e < 3; ++e) {
        if (Triangles.GetAt(static_cast<int32_t>(apTriangle)).IsEdgeConnected(e)) {
            ++cretValue;
        }
    }
    return cretValue;
}

const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* NavMeshSearchLOS::GetFirstIgnoredGoal() {
    return pFirstIgnoredGoal;
}

const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* NavMeshSearchLOS::GetLastValidNode() {
    return pLastValidNode;
}

const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* NavMeshSearchLOS::GetClosestNode() {
    return pClosestNode;
}

PathingLOSMap* NavMeshSearchLOS::GetLOSMap() {
    return spLOSMap.m_pObject;
}

void NavMeshSearchMaxCost::SetMaxDistance(float afMaxDist) {
    fMaxDist = afMaxDist;
}

NavmeshTriFan::NavmeshTriFan(const NavmeshTriFan& arOther)
  : hTri(arOther.hTri), bInradius{arOther.bInradius[0], arOther.bInradius[1], arOther.bInradius[2]} {}

NavmeshTriFan* NavmeshTriFan::Assign(NavmeshTriFan* apThis, const NavmeshTriFan& arOther) {
    FNVD_RECONSTRUCT(FastNavMeshTriHandle)(&apThis->hTri, arOther.hTri);
    for (uint32_t S2 = 0; S2 < 3; ++S2) {
        apThis->bInradius[S2] = arOther.bInradius[S2];
    }
    return apThis;
}

PathingCoverLocation::PathingCoverLocation(
    const NiPoint3& aLocation,
    const NavMeshInfo* apNavMeshInfo,
    uint16_t ausTriangleIndex
)
  : PathingLocation(aLocation, apNavMeshInfo, ausTriangleIndex) {}

PathingCoverLocation::~PathingCoverLocation() {}

NavMeshPtr& PathingLOSMap::GetNavMeshByIndex(uint32_t aiNavMeshIndex) {
    return NavMeshes.GetAt(aiNavMeshIndex);
}

void PathingRequest::SetTargetRadius(float afTargetRadius) {
    fTargetRadius = afTargetRadius;
}

void PathingRequest::SetAvoidNodeArray(PathingAvoidNodeArray* apAvoidNodeArray) {
    spAvoidNodeArray = apAvoidNodeArray;
}

void PathingRequestClosePoint::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    PathingRequest::SaveGame(apSaveGameBuffer);
    // The buffer takes the two distances as raw bytes, through their addresses.
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMinDistance), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMaxDistance), 4u, 0);
}

// Reads the base's fields, then the two distances.
void PathingRequestClosePoint::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PathingRequest::LoadGame(apLoadGameBuffer);
    // The buffer fills the two distances as raw bytes through their addresses.
    apLoadGameBuffer->BGSLoadGameBuffer::LoadData(&fMinDistance, 4u);
    apLoadGameBuffer->BGSLoadGameBuffer::LoadData(&fMaxDistance, 4u);
}

bool PathingRequestCover::GetCanCrouch() const {
    return bCanCrouch;
}

bool PathingRequestCover::GetHideCover() {
    return bHideCover;
}

NiPoint3 PathingRequestCover::GetThreat() {
    return Threat;
}

CombatCoverLocation::CombatCoverLocation()
  : bDirtyFlag(false), bReserved(false), cCoverHeight(0), iLocation(0), CoverSearchTargetLocation(NiPoint3_ZERO),
    iCoverLocationKey(0), iCoverLocationLastMovedTimeStamp(0xFFFFFFFF) {}

CombatCoverLocation::~CombatCoverLocation() {}

bool CombatCoverLocation::HasFireLocation() {
    return CoverLocation.IsStandCover() || (CoverLocation.IsLedgeCover() || ReservationInfo.bHideCover);
}

void NavMeshSearchNode::SetCenter(const NiPoint3& arCenter) {
    CenterX = arCenter.x;
    CenterY = arCenter.y;
    CenterZ = arCenter.z;
}
