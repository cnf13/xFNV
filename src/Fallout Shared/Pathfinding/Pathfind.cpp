#include "Fallout Shared/Pathfinding/Pathfind.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/stdlib.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

// The avoidance cone angle settings, in degrees.
static SettingT<INISettingCollection> fINIAvoidanceConeAngle("fAvoidanceConeAngle:Pathfinding", 45.0f);
static SettingT<INISettingCollection> fINIWiderAvoidanceConeAngle("fAvoidanceWiderConeAngle:Pathfinding", 60.0f);
static SettingT<INISettingCollection> fINIRunningAvoidanceConeAngle("fRunningWiderConeAngle:Pathfinding", 30.0f);

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fFindClosestEdgesRadius("fFindClosestEdgesRadius:Pathing", 512.0f);

using FastNavMeshEdgeLocationProxyArray = BSSimpleArray<FastNavMeshEdgeLocationProxy, 1024>;

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
                        startLoc.pWorldSpace,
                        nullptr,
                        aLoc,
                        2000.0f,
                        &randomPoints.GetAt(static_cast<int32_t>(i))
                    )) {
                    --i;
                }
            }
        }
        else {
            for (uint32_t i = 0; i < aiNum; ++i) {
                float fx = RandomFloat(cellMinX, cellMaxX);
                randomPoints.GetAt(static_cast<int32_t>(i)).x = fx;
                float fy = RandomFloat(cellMinY, cellMaxY);
                randomPoints.GetAt(static_cast<int32_t>(i)).y = fy;
                randomPoints.GetAt(static_cast<int32_t>(i)).z = aLoc.z;
            }
        }
        for (uint32_t i = 0; i < aiNum; ++i) {
            if (iswitch) {
                if (iswitch == 1) {
                    PathingRequestCover coverRequest;
                    coverRequest.SetOrigin(startLoc);
                    coverRequest.SetThreat(randomPoints.GetAt(static_cast<int32_t>(i)));
                    coverPoints_t coverPoints;
                    Pathing::GetCoverPoints(&coverRequest, coverPoints);
                }
                else if (iswitch == 2) {
                    TESWorldSpace* pworld = startLoc.pWorldSpace;
                    PathingLocation goal(&randomPoints.GetAt(static_cast<int32_t>(i)), pworld);
                    PathingRequest request;
                    request.SetOrigin(startLoc);
                    request.SetDestination(goal);
                    PathingSolution solution;
                    Pathing::Pathfind(&request, &solution);
                }
            }
            else {
                // The four points are i to i+3, wrapped by an unsigned modulo (div) of the point count.
                uint32_t i1 = (i + 1) % aiNum;
                uint32_t i2 = (i + 2) % aiNum;
                uint32_t i3 = (i + 3) % aiNum;
                float fy0 = randomPoints.GetAt(static_cast<int32_t>(i)).y;
                NiPoint2 seg1Start(randomPoints.GetAt(static_cast<int32_t>(i)).x, fy0);
                float fy1 = randomPoints.GetAt(static_cast<int32_t>(i1)).y;
                NiPoint2 seg1End(randomPoints.GetAt(static_cast<int32_t>(i1)).x, fy1);
                float fy2 = randomPoints.GetAt(static_cast<int32_t>(i2)).y;
                NiPoint2 seg2Start(randomPoints.GetAt(static_cast<int32_t>(i2)).x, fy2);
                float fy3 = randomPoints.GetAt(static_cast<int32_t>(i3)).y;
                NiPoint2 seg2End(randomPoints.GetAt(static_cast<int32_t>(i3)).x, fy3);
                bool bfirst = NavMeshMath::Find2DIntersection(seg1Start, seg1End, seg2Start, seg2End, &temp);
                bool bsecond = NavMeshMath::Find2DIntersection(seg1Start, seg1End, seg2Start, seg2End);
                if (bfirst != bsecond) {
                    Warning("PATHFINDING: Different");
                }
            }
        }
        return 0.0f;
    }

    return 0.0f;
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
                apActorLockData,
                aOrigin.GetLocation(),
                pOriginInfo,
                destLoc,
                pDestInfo,
                &infos,
                &doors,
                aeDoorLockDisp
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
        aOrigin,
        aDestination,
        apRetPath,
        Pathing::TELEPORT_DOOR_PATH_NONE,
        0.0f,
        apActorLockData,
        aeDoorLockDisp
    );
    return bResult;
}

bool Pathing::GetNavMeshZForLocation(PathingLocation& aLocation, float& aRetHeight) {
    NiPoint3 location;
    uint16_t usTriangle;
    if (aLocation.ResolveNavMeshInfo(nullptr)) {
        if (aLocation.HasNavMeshData()) {
            NavMeshPtr spnavMesh;
            if (aLocation.GetNavMeshAndTriangle(spnavMesh, usTriangle)) {
                location = aLocation.GetLocation();
                if (spnavMesh->IsPointInsideTriangle_0(usTriangle, &location, 180.0f, 180.0f, &aRetHeight)) {
                    return true;
                }

                aRetHeight = aLocation.GetLocation().z;
                return false;
            }

            aRetHeight = aLocation.GetLocation().z;
            return false;
        }

        aRetHeight = aLocation.GetLocation().z;
        return false;
    }

    aRetHeight = aLocation.GetLocation().z;
    return false;
}

bool Pathing::GetNavmeshNormalForLocation(PathingLocation& aLocation, NiPoint3& aRetNormal) {
    uint16_t usTriangle;
    if (!aLocation.ResolveNavMeshInfo(nullptr)) {
        return false;
    }
    NavMeshPtr spnavMesh;
    if (aLocation.GetNavMeshAndTriangle(spnavMesh, usTriangle)) {
        aRetNormal = spnavMesh->GetNormal(usTriangle);
        return true;
    }

    return false;
}

bool Pathing::FindConnectedNavmeshesForLocation(
    PathingLocation& aLocation,
    float afRadius,
    BSSimpleArray<NavMeshPtr, 1024>* apRetNavMeshes
) {
    return Pathing::FindNavmeshesForLocation(aLocation, afRadius, apRetNavMeshes, true);
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
    if (actorLocation == NiPoint3::ZERO) {
        actorLocation = center;
        buniqueCenter = false;
    }
    NiPoint3 targetLocation = apRequest->GetThreat();
    NiPoint2 threatPos2D(targetLocation.x, targetLocation.y);
    NiPoint2 threatToActor2D(actorLocation.x - threatPos2D.x, actorLocation.y - threatPos2D.y);
    uint16_t crouchHeight = static_cast<uint16_t>(
        NavMeshTriangle::GetCoverLevel(apRequest->fActorCrouchHeight, false, true)
    );
    uint16_t standHeight = static_cast<uint16_t>(NavMeshTriangle::GetCoverLevel(apRequest->fActorHeight, false, true));
    float fsquaredMaxDistance = apRequest->fMaxCoverDistance * apRequest->fMaxCoverDistance;
    BSScrapArray<NavMeshPtr> navmeshes(8u, 0u, nullptr);
    Pathing::FindConnectedNavmeshesForLocation(apRequest->Origin, apRequest->fMaxCoverDistance, &navmeshes);
    NavMeshPtr spmesh;
    bool bcanCrouch = apRequest->bCanCrouch;
    bool bhideCover = apRequest->bHideCover;
    for (const NavMeshPtr& rspMesh : navmeshes) {
        spmesh = rspMesh;
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
                if (distanceProjectionVector != NiPoint3::ZERO) {
                    float fdot = targetDistance.Dot(distanceProjectionVector);
                    double fRatio = fdot * fdot / fsquaredDist;
                    fsquaredDist = fRatio + fRatio + fsquaredDist;
                }
            }
            else if (distanceProjectionVector != NiPoint3::ZERO) {
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
            NiPoint3 triNormal = spmesh->GetNormal(itriIndex);
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
                    edgeCoverHeight,
                    standHeight,
                    crouchHeight,
                    bleft,
                    bright,
                    static_cast<uint8_t>(iedge)
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
        *apReturnIntersectionLocation = aStartLocation;
    }
    if (!aGoalLocation.ResolveNavMeshInfo(nullptr)) {
        return !aStartLocation.ResolveNavMeshInfo(nullptr);
    }
    NavMeshPtr spgoalMesh;
    if (aGoalLocation.GetNavMeshAndTriangle(spgoalMesh, usgoalTri)) {
        uint16_t usTriangle = usgoalTri;
        NavMesh* pgoalMesh = spgoalMesh.m_pObject;
        return Pathing::CheckLineOfSight(
            aStartLocation,
            aGoalLocation.GetLocation(),
            pgoalMesh,
            usTriangle,
            apReturnIntersectionLocation,
            apFilter
        );
    }

    return Pathing::CheckLineOfSight(
        aStartLocation,
        aGoalLocation.GetLocation(),
        static_cast<NavMesh*>(nullptr),
        0xFFFF,
        apReturnIntersectionLocation,
        apFilter
    );
}

bool Pathing::CheckLineOfSight(
    PathingLocation& aStartLocation,
    const NiPoint3& aTarget,
    PathingLocation* apReturnIntersectionLocation,
    Pathing::CheckLineOfSightFilter* apFilter
) {
    return Pathing::CheckLineOfSight(
        aStartLocation,
        aTarget,
        static_cast<NavMesh*>(nullptr),
        0xFFFF,
        apReturnIntersectionLocation,
        apFilter
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

PathingLocation Pathing::FindClosestReachableLocation(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    float afMaxPathingDistance
) {
    if (!aStartLocation.ResolveNavMeshInfo(nullptr)) {
        return aStartLocation;
    }
    FastNavMeshTriHandle kStartNode;
    {
        NavMeshPtr spStartMesh;
        uint16_t usStartTriangle;
        if (!aStartLocation.GetNavMeshAndTriangle(spStartMesh, usStartTriangle)) {
            return aStartLocation;
        }
        kStartNode.pNavMesh = spStartMesh;
        kStartNode.usTriangle = usStartTriangle;
    }
    FastNavMeshTriHandle kGoalNode;
    kGoalNode.pNavMesh = nullptr;
    kGoalNode.usTriangle = 0xFFFF;
    if (aGoalLocation.ResolveNavMeshInfo(nullptr)) {
        NavMeshPtr spGoalMesh;
        uint16_t usGoalTriangle;
        if (aGoalLocation.GetNavMeshAndTriangle(spGoalMesh, usGoalTriangle)) {
            kGoalNode.pNavMesh = spGoalMesh;
            kGoalNode.usTriangle = usGoalTriangle;
        }
    }
    NavMeshSearchMaxCost kSearch;
    // Takes the goal's point before the start's.
    NiPoint3 kGoalPoint = aGoalLocation.GetLocation();
    NiPoint3 kStartPoint = aStartLocation.GetLocation();
    NiPoint3
        kClosestPoint = kSearch.FindClosestPoint(kStartPoint, kStartNode, kGoalPoint, kGoalNode, afMaxPathingDistance);
    PathingLocation kLocation(aStartLocation);
    kLocation.SetLocation(kClosestPoint);
    return kLocation;
}

// The memory context is not destroyed automatically; each case destroys it explicitly.
bool Pathing::BuildPath(PathingRequest* apRequest, PathingSolution* apSolution) {
    alignas(AutoMemContext) uint8_t aContextStorage[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (aContextStorage) AutoMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
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
                static_cast<PathingRequestSafeStraightLine*>(apRequest),
                apSolution
            );
            break;
        case 7:
            bResult = Pathing::BuildOptimalLocationPath(
                static_cast<PathingRequestOptimalLocation*>(apRequest),
                apSolution
            );
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
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
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
    bool bcanFit = pathSmoother
                       .SmoothPath(apRequest, nodes, links, startLocation, endLocation, apSolution, nullptr, nullptr);
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
                NiPoint3 tangent = Pathing::ComputeInitialTangentForActor(fHeading, startLoc, nextLoc, fRadius);
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
    NiPoint3 yOffset;
    NiPoint3 fleeFromPoint;
    NiPoint3 startLocation;
    NiPoint3 fleeToPoint;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        303
    );
    ::new (&startLocation) NiPoint3(apRequest->Origin.GetLocation());
    TESObjectCELL* pcell = apRequest->Origin.GetCell();
    TESWorldSpace* pworld = apRequest->Origin.pWorldSpace;
    if (!apRequest->GetAvoidNodeArray() || apRequest->GetAvoidNodeArray()->IsEmpty()) {
        fleeFromPoint = startLocation;
        float fangle = RandomFloatNegPiToPi();
        double dSin = std::sin(fangle);
        float fYScalar = apRequest->GetMinFleeDistance() * dSin;
        NiPoint3* pyOffset = ::new (&yOffset) NiPoint3(fYScalar * NiPoint3::UNIT_Y);
        double dCos = std::cos(fangle);
        float fScalar = apRequest->GetMinFleeDistance() * dCos;
        fleeToPoint = fleeFromPoint + fScalar * NiPoint3::UNIT_X + *pyOffset;
    }
    else {
        PathingAvoidNodeArray* pavoidNodes = apRequest->GetAvoidNodeArray();
        pavoidNodes->ComputeCenter(startLocation, fleeFromPoint);
        fleeToPoint = startLocation - fleeFromPoint;
        fleeToPoint.Unitize();
        float fMinFleeDistance = apRequest->GetMinFleeDistance();
        fleeToPoint *= fMinFleeDistance;
        fleeToPoint += startLocation;
    }
    if (pcell) {
        float height;
        if (pcell->GetLandHeight(fleeToPoint, height)) {
            fleeToPoint.z = height;
        }
    }
    apSolution->AddVirtualPathingNode(startLocation, pcell, pworld);
    apSolution->AddVirtualPathingNode(fleeToPoint, pcell, pworld);
    apSolution->iFirstLoadedVirtualNodeIndex = 0;
    apSolution->iLastLoadedVirtualNodeIndex = 1;
    return true;
}

bool Pathing::BuildLOSPath(PathingRequestLOS* apRequest, PathingSolution* apSolution) {
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        378
    );
    if (apRequest->Origin.ResolveToClosestNavmeshAndTriangle()) {
        uint16_t usTriangles[3];
        NavMeshPtr spNavMesh;
        usTriangles[0] = -1;
        if (apRequest->Origin.GetNavMeshAndTriangle(spNavMesh, *usTriangles)) {
            NavMeshSearchNode startNode;
            startNode.pNavMesh = spNavMesh;
            startNode.sTriangle = usTriangles[0];
            NiPoint3 startLocation = apRequest->Origin.GetLocation();
            bool busingIgnoredGoal = false;
            bool busingFallbackNode = false;
            NavMeshSearchLOS losSearch;
            nodes_t nodes(32u, 0, nullptr);
            links_t links(32u, 0, nullptr);
            bool bcanFit = true;
            if (!losSearch.BuildNodePath(apRequest, startNode, &nodes, &links)) {
                if (losSearch.GetFirstIgnoredGoal()) {
                    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>*
                        pfirstIgnoredGoal = losSearch.GetFirstIgnoredGoal();
                    busingIgnoredGoal = losSearch.GetPathToNode(pfirstIgnoredGoal, &nodes, &links);
                }
                if (!busingIgnoredGoal && losSearch.GetLastValidNode()) {
                    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* plastValidNode = losSearch
                                                                                                .GetLastValidNode();
                    busingFallbackNode = losSearch.GetPathToNode(plastValidNode, &nodes, &links);
                }

                if (!busingIgnoredGoal && !busingFallbackNode && losSearch.GetClosestNode()) {
                    const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pclosestNode = losSearch.GetClosestNode();
                    busingFallbackNode = losSearch.GetPathToNode(pclosestNode, &nodes, &links);
                }

                if (!busingIgnoredGoal && !busingFallbackNode) {
                    bcanFit = false;
                    busingFallbackNode = true;
                }
            }
            if (bcanFit) {
                // Calls the const GetAt, not the non-const one.
                NavMeshSearchNode lastNode = static_cast<const nodes_t&>(nodes).GetAt(nodes.iSize - 1);
                NiPoint3 endLocation = lastNode.pNavMesh->GetCenter(lastNode.sTriangle);
                apSolution->Clear();
                PathingRequest tempRequest(*apRequest);
                uint16_t sTriangle = lastNode.sTriangle;
                NavMeshInfo* pnavMeshInfo = lastNode.pNavMesh->GetNavMeshInfo();
                PathingLocation destination(endLocation, pnavMeshInfo, sTriangle);
                tempRequest.SetDestination(destination);
                bcanFit = Pathing::BuildPath(&tempRequest, apSolution);
            }
            if (!bcanFit && (busingIgnoredGoal || busingFallbackNode)) {
                NavMeshPtr spnavMesh;
                uint16_t sunseenTriangle = 0;
                PathingLOSMap* plosMap = losSearch.GetLOSMap();
                if (plosMap->GetUnseenNavMeshAndTriangle(spnavMesh, sunseenTriangle)) {
                    uint32_t ilastLoadedVirtualNodeIndex = apSolution->iLastLoadedVirtualNodeIndex;
                    if (ilastLoadedVirtualNodeIndex == -1 || !apSolution->GetVirtualNodeCount()) {
                        NavMeshInfo* pstartInfo = startNode.pNavMesh->GetNavMeshInfo();
                        apSolution->AddVirtualPathingNode_0(startLocation, pstartInfo);
                        apSolution->iLastLoadedVirtualNodeIndex = 0;
                    }
                    VirtualPathingNode* plastLoadedVirtualNode = apSolution->GetVirtualNode(
                        ilastLoadedVirtualNodeIndex
                    );
                    if (plastLoadedVirtualNode) {
                        NiPoint3 unseenTriangleCenter = spnavMesh->GetCenter(sunseenTriangle);
                        uint16_t usTriangle = sunseenTriangle;
                        NavMeshInfo* pnavMeshInfo = spnavMesh->GetNavMeshInfo();
                        PathingLocation unseenTriangleLocation(unseenTriangleCenter, pnavMeshInfo, usTriangle);
                        plastLoadedVirtualNode->SetLocation(unseenTriangleLocation);
                    }
                }
            }
            return bcanFit;
        }

        return false;
    }

    return false;
}

bool Pathing::BuildHidePath(PathingRequestHide* apRequest, PathingSolution* apSolution) {
    using NodeArray_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using LinkArray_t = BSScrapArray<NavMeshSearchLink, 1024>;
    using PathingNodeArray_t = BSScrapArray<PathingNode*, 1024>;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        501
    );
    if (!apRequest->Origin.ResolveNavMeshInfo(nullptr)) {
        return false;
    }
    NavMeshPtr spNavMesh;
    uint16_t sTriangle = 0xFFFF;
    if (!apRequest->Origin.GetNavMeshAndTriangle(spNavMesh, sTriangle)) {
        return false;
    }
    // Only the mesh and the triangle of the start node are written; its center is deliberately left uninitialised.
    NavMeshSearchNode startNode;
    startNode.pNavMesh = spNavMesh;
    startNode.sTriangle = sTriangle;
    NiPoint3 startLocation = apRequest->Origin.GetLocation();
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
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
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
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
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
            endLocation = endNode.GetCenter();
        }
        else {
            endLocation = closestPoint;
        }
    }
    else if (!nodes.IsEmpty()) {
        endNode = search.GetBestAStarNode()->tNodeData;
        endLocation = endNode.GetCenter();
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
    bool bSmoothed = pathSmoother
                         .SmoothPath(apRequest, nodes, links, startLocation, endLocation, apSolution, nullptr, nullptr);
    return bSmoothed;
}

bool Pathing::BuildCoveredMovePath(PathingRequestCoveredMove* apRequest, PathingSolution* apSolution) {
    // The PathingLocations are built and destroyed only through their own Construct/Destruct calls below.
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        802
    );
    if (Pathing::Pathfind(apRequest, apSolution)) {
        CombatCoverSearchInfo* pcombatCoverSearchInfo;
        CombatCoverLocation* pcombatCoverLocation;
        float fminAdvanceDistance = 512.0f;
        PathingLocation previousNodeLocation(*&apRequest->Origin);
        pcombatCoverLocation = new CombatCoverLocation;
        pcombatCoverSearchInfo = new CombatCoverSearchInfo;
        PathingRequestCoveredMove requestCopy(*apRequest);
        requestCopy.pCombatCoverLocation = pcombatCoverLocation;
        requestCopy.SetCombatCoverSearchInfo(pcombatCoverSearchInfo);
        apRequest->pCombatCoverLocation = pcombatCoverLocation;
        apRequest->SetCombatCoverSearchInfo(pcombatCoverSearchInfo);
        uint32_t icurrentNode = 1;
        while (true) {
            uint32_t uiNodeCount = apSolution->GetNodeCount();
            if (uiNodeCount <= icurrentNode) {
                break;
            }
            PathingNode* pnode = apSolution->GetNode(icurrentNode);
            PathingLocation nextNodeLocation(*&pnode->Location);
            requestCopy.SetDestination(nextNodeLocation);
            if (CombatState::FindBestAdvanceLocation(&requestCopy, fminAdvanceDistance, true)) {
                PathingLocation coverPathingLocation(requestCopy.CoverLocation, nextNodeLocation);
                requestCopy.SetDestination(coverPathingLocation);
                PathingSolution newSolution;
                bool bsuccess = Pathing::Pathfind(&requestCopy, &newSolution);
                if (bsuccess) {
                    apRequest->SetCoverLocation(requestCopy.CoverLocation);
                    apRequest->SetDestination(coverPathingLocation);
                    requestCopy.ClearCombatCoverData();
                    newSolution.CopyTo(apSolution);
                    apRequest->bFinished = true;
                    return bsuccess;
                }
            }
            ++icurrentNode;
            previousNodeLocation = nextNodeLocation;
        }
        requestCopy.ClearCombatCoverData();
        if (pcombatCoverLocation) {
            delete pcombatCoverLocation;
        }
        apRequest->pCombatCoverLocation = nullptr;
        apRequest->bFinished = true;
        return true;
    }

    return false;
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
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
        1410
    );
    alignas(TeleportDoorSearch) uint8_t kSearchMemory[sizeof(TeleportDoorSearch)];
    TeleportDoorSearch* pSearch = ::new (kSearchMemory) TeleportDoorSearch;
    apRetPath->Clear();
    apRetPath->Spaces.SetReservedSize(10);
    apRetPath->Doors.SetReservedSize(10);
    bool bResult = false;
    if (pSearch->BuildNodePath(
            aOrigin,
            aDestination,
            apRetPath,
            aRestrictions,
            afMaxDist,
            apActorLockData,
            aeDoorLockDisp
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
            aOrigin,
            aDestination,
            pPath,
            TELEPORT_DOOR_PATH_NONE,
            aMaxDist,
            apActorLockData,
            aeDoorLockDisp
        )) {
        fLength = pPath->ComputeLength();
    }
    else {
        fLength = std::numeric_limits<float>::max();
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
    // NaN counts as overlapping.
    return !(bound1Min.z >= bound2Max.z);
}

bool Pathing::FindClosestPointOnNavmesh(
    TESWorldSpace* apWorldSpace,
    TESObjectCELL* apCell,
    const NiPoint3& aLocation,
    NiPoint3* apRetPt
) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_PATHING,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\Pathfind.cpp",
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
            *apRetPt = testMesh->GetCenter(ustriangle);
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
        float fySquared = aRadius * aRadius - xcoord * xcoord;
        float ycoord = std::sqrt(fySquared);
        ycoord = RandomFloat(-ycoord, ycoord);
        NiPoint3* poffset = ::new (&dst) NiPoint3(xcoord, ycoord, 0.0f);
        *apRetPt = aLocation + *poffset;
        return true;
    }
    if (!testMesh->FindRandomPointWithinRadius(aLocation, aRadius, apRetPt) ||
        !testMesh->IsPointInsideNavmesh(*apRetPt, nullptr)) {
        usTriIndex = -1;
        bonTri = false;
        if (!testMesh->FindClosestTriangleForLocation(aLocation, &usTriIndex, &bonTri)) {
            return false;
        }
        *apRetPt = testMesh->GetCenter(usTriIndex);
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
    NavmeshTriFan firstTri;
    firstTri.hTri.~FastNavMeshTriHandle();
    ::new (&firstTri.hTri) FastNavMeshTriHandle;
    firstTri.hTri = FastNavMeshTriHandle(spmesh, usfirstTri);
    for (int32_t e = 0; e < 3; ++e) {
        NiPoint3 vert1;
        NiPoint3 vert2;
        spmesh->GetEdgeVertices(usfirstTri, e, &vert1, &vert2);
        firstTri.bInradius[e] = afMaxRadius >
            NavMeshMath::DistanceFromSegment(aLocation.GetLocation(), vert1, vert2, nullptr);
    }
    allTris.Add(firstTri);
    for (uint32_t i = 0; i < allTris.iSize; ++i) {
        NavmeshTriFan fan(allTris.GetAt(static_cast<int32_t>(i)));
        for (int32_t e = 0; e < 3; ++e) {
            if (!fan.bInradius[e]) {
                continue;
            }
            NavMeshTriangle* ptri = fan.hTri.pNavMesh->GetTriangle(fan.hTri.usTriangle);
            FastNavMeshEdgeHandle thisEdge(fan.hTri.pNavMesh, fan.hTri.usTriangle, e);
            if (ptri->GetTriOrExtraInfo(e) == 0xFFFF) {
                foundOpenEdges.Add(thisEdge);
                continue;
            }
            FastNavMeshEdgeHandle otherEdge;
            if (!fan.hTri.pNavMesh->GetMatchingEdge(thisEdge, &otherEdge)) {
                continue;
            }
            bool binListAlready = false;
            for (const NavmeshTriFan& rTri : allTris) {
                if (rTri.hTri == otherEdge) {
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
                newTri.bInradius[f] = afMaxRadius >
                    NavMeshMath::DistanceFromSegment(aLocation.GetLocation(), vert1, vert2, nullptr);
            }
            allTris.Add(newTri);
        }
    }
    if (foundOpenEdges.IsEmpty()) {
        return false;
    }
    FastNavMeshEdgeLocation closestEdge;
    float fBestSqrDist = std::numeric_limits<float>::max();
    for (const FastNavMeshEdgeHandle& rEdge : foundOpenEdges) {
        NiPoint3 vert1;
        NiPoint3 vert2;
        int32_t iEdgeIndex = rEdge.iEdgeIndex;
        uint16_t usTriangle = rEdge.usTriangle;
        rEdge.pNavMesh->GetEdgeVertices(usTriangle, iEdgeIndex, &vert1, &vert2);
        NiPoint3 edgePt = NavMeshMath::ProjectPointOntoSegment(aLocation.GetLocation(), vert1, vert2);
        float fSqrDist = (edgePt - aLocation.GetLocation()).SqrLength();
        if (fBestSqrDist > fSqrDist) {
            fBestSqrDist = fSqrDist;
            closestEdge.Location = edgePt;
            closestEdge.FastNavMeshEdgeHandle::Copy(rEdge);
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
    TESObjectCELL* padjacentCell;
    using cells_t = BSSimpleArray<TESObjectCELL*, 1024>;
    NiPoint3 location = aLocation.GetLocation();
    TESObjectCELL* pcell = aLocation.GetCell();
    if (!pcell) {
        return false;
    }
    float fminY = location.y - afRadius;
    float fminX = location.x - afRadius;
    NiPoint3 searchAABBMin(fminX, fminY, -std::numeric_limits<float>::max());
    float fmaxY = location.y + afRadius;
    float fmaxX = location.x + afRadius;
    NiPoint3 searchAABBMax(fmaxX, fmaxY, std::numeric_limits<float>::max());
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
            TESObjectCELL* pinteriorCell = aLocation.GetCell();
            cells.Add(pinteriorCell);
        }
        else {
            for (int32_t x = pcell->GetDataX() - 1; x < pcell->GetDataX() + 2; ++x) {
                for (int32_t y = pcell->GetDataY() - 1; y < pcell->GetDataY() + 2; ++y) {
                    NiPoint3 cellmin(
                        static_cast<float>(x << 12),
                        static_cast<float>(y << 12),
                        -std::numeric_limits<float>::max()
                    );
                    float fcellMaxY = static_cast<double>(y << 12) + 4096.0;
                    float fcellMaxX = static_cast<double>(x << 12) + 4096.0;
                    NiPoint3 cellmax(fcellMaxX, fcellMaxY, std::numeric_limits<float>::max());
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
        for (TESObjectCELL* padjCell : cells) {
            if (padjCell && padjCell->GetNavMeshArray()) {
                for (uint32_t j = 0; j < padjCell->GetNavMeshArray()->GetNavMeshCount(); ++j) {
                    apRetNavMeshes->Add(padjCell->GetNavMeshArray()->GetNavMeshByIndex(j));
                }
            }
        }
    }
    if (apRetNavMeshes->IsEmpty()) {
        return false;
    }
    for (uint32_t n = 0; n < apRetNavMeshes->iSize; ++n) {
        NavMeshPtr spcurrentMesh((*apRetNavMeshes)[n]);
        uint32_t isize = spcurrentMesh->GetNavMeshInfo()->AdjacentNavMeshes.iSize;
        uint32_t ipreferredSize = spcurrentMesh->GetNavMeshInfo()->PreferredAdjacentNavMeshes.iSize;
        for (uint32_t i = 0; i < ipreferredSize + isize; ++i) {
            NavMeshInfo* pportal;
            if (i >= isize) {
                pportal = spcurrentMesh->GetNavMeshInfo()->PreferredAdjacentNavMeshes.GetAt(
                    static_cast<int32_t>(i - isize)
                );
            }
            else {
                pportal = spcurrentMesh->GetNavMeshInfo()->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i));
            }
            if (!pportal) {
                continue;
            }
            NavMeshPtr spotherMesh;
            if (pportal->GetNavMesh(spotherMesh)) {
                if (apRetNavMeshes
                        ->Find(spotherMesh, 0, &BSSimpleArray<NavMeshPtr, 1024>::DefaultEqualFunctionByAddress) != -1) {
                    continue;
                }
                TESObjectCELL* potherNavMeshCell = spotherMesh->pParentCell;
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
                if (spotherMesh->GetBounds(boundsMin, boundsMax) &&
                    Pathing::DoAABBsOverlap(boundsMin, boundsMax, searchAABBMin, searchAABBMax)) {
                    apRetNavMeshes->Add(spotherMesh);
                }
            }
        }
    }
    return true;
}

bool Pathing::FindClosePointViaNavMesh(PathingRequestClosePoint* apRequest, NiPoint3* apRetPt) {
    using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
    using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
    if (!apRequest->Origin.ResolveToClosestNavmeshAndTriangle()) {
        return false;
    }
    NavMeshPtr spNavMesh;
    uint16_t stri = -1;
    if (apRequest->Origin.GetNavMeshAndTriangle(spNavMesh, stri)) {
        NavMeshSearchNode pNavMesh;
        pNavMesh.pNavMesh = spNavMesh;
        pNavMesh.sTriangle = stri;
        NavMeshSearchClosePoint closePointSearch;
        nodes_t nodes(32u, 0, nullptr);
        links_t links(32u, 0, nullptr);
        if (closePointSearch.BuildNodePath(apRequest, pNavMesh, &nodes, &links)) {
            // Reads the last node through the const GetAt and copies it whole.
            const BSSimpleArray<NavMeshSearchNode, 1024>& constNodes = nodes;
            NavMeshSearchNode lastNode = constNodes.GetAt(static_cast<int32_t>(nodes.GetSize() - 1));
            NiPoint3 verts[3];
            NavMeshTriangle* ptri = lastNode.pNavMesh->GetTriangle(lastNode.sTriangle);
            verts[0] = lastNode.pNavMesh->GetVertex(ptri->GetVert(0))->Location;
            verts[1] = lastNode.pNavMesh->GetVertex(ptri->GetVert(1u))->Location;
            verts[2] = lastNode.pNavMesh->GetVertex(ptri->GetVert(2u))->Location;
            *apRetPt = NavMeshMath::RandomPointOnTriangle(*verts, verts[1], verts[2]);
            return true;
        }

        return false;
    }

    return false;
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
    for (const NavMeshPtr& spMesh : keepSmartPointers) {
        const NavMeshInfo* pInfo = spMesh->GetNavMeshInfo();
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
    startNode.pNavMesh = spStartNavMesh;
    startNode.sTriangle = usStartTri;
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
    goalNode.pNavMesh = spGoalNavMesh;
    goalNode.sTriangle = usGoalTri;
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
            NiPoint3 startNodeCenter = pStartNode->GetCenter();
            NavMeshSearchLink* pStartNodeLink = &links.GetAt(static_cast<int32_t>(i));
            NiPoint3 kLeft;
            NiPoint3 kRight;
            pStartNodeLink->pNavMesh
                ->GetEdgeVertices(pStartNodeLink->sTriangle, pStartNodeLink->iEdgeIndex, &kLeft, &kRight);
            uint32_t j;
            for (j = i + 2; j < pNodes->iSize; ++j) {
                NavMeshSearchNode* pAdvanceNode = &pNodes->GetAt(static_cast<int32_t>(j));
                NiPoint3 advanceNodeCenter = pAdvanceNode->GetCenter();
                uint32_t uiLink = j - 1;
                NavMeshSearchLink* pParentLink = &links.GetAt(static_cast<int32_t>(uiLink));
                NiPoint3 kVert0;
                NiPoint3 kVert1;
                pParentLink->pNavMesh
                    ->GetEdgeVertices(pParentLink->sTriangle, pParentLink->iEdgeIndex, &kVert0, &kVert1);
                NiPoint3 kIntersection;
                if (NavMeshMath::LineSegmentToLine2DIntersection(
                        &kVert0,
                        &kVert1,
                        &startNodeCenter,
                        &kLeft,
                        &kIntersection
                    )) {
                    kLeft = kIntersection;
                }
                else {
                    kLeft = kVert0;
                }
                if (NavMeshMath::LineSegmentToLine2DIntersection(
                        &kVert0,
                        &kVert1,
                        &startNodeCenter,
                        &kRight,
                        &kIntersection
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
            NiPoint3 prevLocation = pPrevNode->GetCenter();
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
            NiPoint3 lastLocation = pNodes->GetAt(0).GetCenter();
            for (uint32_t i = 1; i < pNodes->iSize; ++i) {
                NiPoint3 currentLocation = pNodes->GetAt(static_cast<int32_t>(i)).GetCenter();
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
    for (NavMeshPtr spmesh : navmeshes) {
        for (uint32_t t = 0; t < spmesh->GetTriangleCount(); ++t) {
            NavMeshTriangle* ptri = spmesh->GetTriangle(static_cast<uint16_t>(t));
            for (int32_t e = 0; e < 3; ++e) {
                if (ptri->GetTriOrExtraInfo(e) != 0xFFFF) {
                    continue;
                }
                NavMeshVertex* pvert1 = spmesh->GetVertex(ptri->GetVert(static_cast<uint32_t>(e)));
                uint32_t uiNext = e + 1;
                if (uiNext == 3) {
                    uiNext = 0;
                }
                NavMeshVertex* pvert2 = spmesh->GetVertex(ptri->GetVert(uiNext));
                NiPoint3 edgePt = NavMeshMath::ProjectPointOntoSegment(
                    aLocation.GetLocation(),
                    pvert1->Location,
                    pvert2->Location
                );
                NiPoint3 delta = aLocation.GetLocation() - edgePt;
                float fsqrDist = delta.SqrXYLength();
                if (fSqrMaxDist > fsqrDist) {
                    FastNavMeshEdgeLocationProxy proxy;
                    proxy.Edge.pNavMesh = spmesh.m_pObject;
                    proxy.Edge.usTriangle = static_cast<uint16_t>(t);
                    proxy.Edge.iEdgeIndex = e;
                    proxy.Edge.Location = edgePt + (spmesh->GetCenter(static_cast<uint16_t>(t)) - edgePt) * 0.1f;
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
        aRetEdges.Add(NavMeshEdgeLocation(FoundEdges.GetAt(static_cast<int32_t>(i)).Edge));
    }
    if (icount) {
        return Pathing::FIND_CLOSEST_EDGES_OK;
    }
    return Pathing::FIND_CLOSEST_EDGES_NONE;
}

char Pathing::FindSafeStraightLinePathPoint(
    TESObjectREFR* apOrigin,
    TESObjectREFR* apTarget,
    float afDistance,
    NiPoint3* apRetPt
) {
    PathingRequestSafeStraightLine straightLineRequest;
    straightLineRequest.SetOrigin(PathingLocation(apOrigin));
    straightLineRequest.SetDestination(PathingLocation(apTarget));
    NiPoint3 boundMax = apOrigin->GetBoundMax();
    NiPoint3 boundMin = apOrigin->GetBoundMin();
    float fwidth1 = boundMax.x - boundMin.x;
    float fwidth2 = boundMax.y - boundMin.y;
    float fradius = BSMAX(fwidth1, fwidth2);
    double dScale = apOrigin->GetScale();
    fradius = dScale * fradius;
    fradius = BSMAX(fradius, 32.0f);
    float factorRadius = fradius * 0.5;
    float foriginRadius = factorRadius;
    straightLineRequest.SetActorRadius(factorRadius);
    boundMax = apTarget->GetBoundMax();
    boundMin = apTarget->GetBoundMin();
    fwidth1 = boundMax.x - boundMin.x;
    fwidth2 = boundMax.y - boundMin.y;
    factorRadius = BSMAX(fwidth1, fwidth2);
    dScale = apTarget->GetScale();
    factorRadius = dScale * factorRadius;
    factorRadius = BSMAX(factorRadius, 32.0f);
    float ftargetRadius = factorRadius * 0.5;
    straightLineRequest.SetTargetRadius(ftargetRadius);
    float fminDistance = afDistance * 0.8999999761581421 + foriginRadius;
    straightLineRequest.SetMinDistance(fminDistance);
    float fmaxDistance = afDistance + foriginRadius;
    straightLineRequest.SetMaxDistance(fmaxDistance);
    return Pathing::FindSafeStraightLinePathPoint(&straightLineRequest, apRetPt);
}

char Pathing::FindSafeStraightLinePathPoint(PathingRequestSafeStraightLine* apRequest, NiPoint3* apRetPt) {
    float fnewSplineT;
    NavMeshSearchNode goalNode;
    using navMeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    NavMeshSearchNode startNode;
    if (!apRequest->Origin.ResolveToClosestNavmeshAndTriangle()) {
        return 0;
    }
    NavMeshPtr spstartMesh;
    uint16_t usstartTri = -1;
    if (!apRequest->Origin.GetNavMeshAndTriangle(spstartMesh, usstartTri)) {
        return 0;
    }
    startNode.pNavMesh = spstartMesh;
    startNode.sTriangle = usstartTri;
    if (!apRequest->Destination.ResolveToClosestNavmeshAndTriangle()) {
        return 0;
    }
    NavMeshPtr spgoalMesh;
    uint16_t usgoalTri = -1;
    if (!apRequest->Destination.GetNavMeshAndTriangle(spgoalMesh, usgoalTri)) {
        return 0;
    }
    goalNode.pNavMesh = spgoalMesh;
    goalNode.sTriangle = usgoalTri;
    NavMeshSearchSLPoint slSearch;
    navMeshes_t navMeshes;
    Pathing::GetAllLoadedNavmeshes(&navMeshes);
    if (slSearch.FindStraightLinePath(apRequest, startNode, goalNode)) {
        slSearch.GetGoalLocation(*apRetPt);
        return 1;
    }

    PathingRequest request;
    apRequest->PathingRequest::CopyTo(&request);
    PathingSolution solution;
    if (Pathing::Pathfind(&request, &solution)) {
        float fminDistance = apRequest->fMinDistance;
        float fcurrentParameter = static_cast<float>(solution.GetNodeCount() - 1);
        if (solution.FindPointOnPath(fcurrentParameter, fminDistance, false, &fnewSplineT, apRetPt)) {
            return 1;
        }
        PathingRequestClosePoint closePointRequest;
        apRequest->PathingRequest::CopyTo(&closePointRequest);
        closePointRequest.SetOrigin(*&apRequest->Destination);
        closePointRequest.SetMinDistance(apRequest->fMinDistance);
        closePointRequest.SetMaxDistance(apRequest->fMaxDistance);
        if (Pathing::FindClosePointViaNavMesh(&closePointRequest, apRetPt)) {
            return 1;
        }
    }
    return 0;
}

Pathing::UPDATE_DETAILED_PATH_RESULT Pathing::UpdateSolution(
    PathingRequest* apRequest,
    PathingSolution* apSolution,
    uint32_t aiCurrentVirtualNodeIndex,
    const NiPoint3& aCurrentActorLocation
) {
    PathBuilder builder(apRequest, apSolution);
    Pathing::UPDATE_DETAILED_PATH_RESULT updated = builder.UpdateDetailedPath(
        aiCurrentVirtualNodeIndex,
        aCurrentActorLocation
    );
    return updated;
}

bool Pathing::ComputeLowPathDistance(
    PathingLocation& aStartLocation,
    PathingLocation& aGoalLocation,
    float* apReturnDistance
) {
    TeleportPath path;
    if (Pathing::BuildLowPath(
            aStartLocation,
            aGoalLocation,
            &path,
            nullptr,
            Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS
        )) {
        *apReturnDistance = path.ComputeLength();
        return true;
    }

    *apReturnDistance = 0.0;
    return false;
}

// newIntersection is left unset by NiPoint2's empty constructor, and FindInterectingEdge writes it only when it finds
// an edge; on the -1 path it is still copied into intersection, deliberately.
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
        *apReturnIntersectionLocation = aStartLocation;
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
    int32_t currentEdgeIndex = spstartMesh->FindInterectingEdge(uscurrentTriangle, -1, start, end, &intersection);
    while (currentEdgeIndex != -1) {
        uint16_t arOtherTriangleIndex;
        int32_t arOtherEdgeIndex;
        NavMeshPtr potherMesh;
        bool bfoundMatchingEdge = pcurrentNavMesh->GetMatchingEdge_0(
            uscurrentTriangle,
            currentEdgeIndex,
            &potherMesh,
            &arOtherTriangleIndex,
            &arOtherEdgeIndex
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
            EdgeExtraInfo* pEdgeInfo = pcurrentNavMesh->GetEdgeExtraInfo(uscurrentTriangle, currentEdgeIndex);
            if (pEdgeInfo && pEdgeInfo->eType == EIT_LEDGE_UP) {
                bfoundMatchingEdge = false;
            }
        }
        if (!bfoundMatchingEdge) {
            if (apReturnIntersectionLocation) {
                apReturnIntersectionLocation->SetNavMeshInfo(pcurrentNavMesh->GetNavMeshInfo());
                apReturnIntersectionLocation->SetTriangleIndex(uscurrentTriangle);
                NiPoint3 center = pcurrentNavMesh->GetCenter(uscurrentTriangle);
                NiPoint3 hitPoint;
                apReturnIntersectionLocation->SetLocation(*::new (&hitPoint)
                                                              NiPoint3(intersection.x, intersection.y, center.z));
            }
            return false;
        }
        if (potherMesh != pcurrentNavMesh) {
            NavMeshEdgeVertices otherEdgeVerts = potherMesh->GetEdgeVertices(arOtherTriangleIndex, arOtherEdgeIndex);
            NiPoint2 otherEdgeVert1(otherEdgeVerts.Vertices[0]->Location.x, otherEdgeVerts.Vertices[0]->Location.y);
            NiPoint2 otherEdgeVert2(otherEdgeVerts.Vertices[1]->Location.x, otherEdgeVerts.Vertices[1]->Location.y);
            intersection = NavMeshMath::ClosestPointOnSegmentToLine(otherEdgeVert1, otherEdgeVert2, intersection, end);
        }
        uscurrentTriangle = arOtherTriangleIndex;
        pcurrentNavMesh = potherMesh;
        NiPoint2 newIntersection;
        currentEdgeIndex = potherMesh->FindInterectingEdge(
            arOtherTriangleIndex,
            arOtherEdgeIndex,
            intersection,
            end,
            &newIntersection
        );
        intersection = newIntersection;
    }
    // 0xFFFF means no goal triangle; Pathing::CheckLineOfSight passes it with no goal navmesh.
    if (!apGoalNavMesh || ausGoalTriangle == 0xFFFF) {
        if (apReturnIntersectionLocation) {
            apReturnIntersectionLocation->SetNavMeshInfo(pcurrentNavMesh->GetNavMeshInfo());
            apReturnIntersectionLocation->SetTriangleIndex(uscurrentTriangle);
            apReturnIntersectionLocation->SetLocation(aGoalLocation);
        }
        return true;
    }
    if (pcurrentNavMesh.m_pObject == apGoalNavMesh && uscurrentTriangle == ausGoalTriangle) {
        if (apReturnIntersectionLocation) {
            PathingLocation goalLoc(aGoalLocation, apGoalNavMesh->GetNavMeshInfo(), ausGoalTriangle);
            *apReturnIntersectionLocation = goalLoc;
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
    if (!aspGridMap->IsValid()) {
        return false;
    }
    NiPointer<PathingLOSMap> spLosMap(new PathingLOSMap(aspGridMap->GetLocation(), aspGridMap->fRadius));
    aspGridMap->SetLOSMapFromSeenTriangles(spLosMap);
    int32_t ilocationsSize = aLocations.iSize;
    for (int32_t l = 0; l < ilocationsSize; ++l) {
        PathingLocation* pgoalLoc = &aLocations.GetAt(l);
        float fheading = apHeadings ? (*apHeadings)[l] : -1.0f;
        float fsightRadius = aspGridMap->fSightRadius;
        Pathing::ComputeLineOfSightMapForLocation(*pgoalLoc, spLosMap, true, fsightRadius, fheading);
    }
    // The scored triangles: a positive score is seen, zero or less unseen, 127 not scored.
    uint32_t uiSeen = 0;
    uint32_t uiUnseen = 0;
    for (uint32_t uiIndex = 0; uiIndex < spLosMap->iTotalTriangleCount; ++uiIndex) {
        char cscore = spLosMap->GetTriangleScore(uiIndex);
        if (cscore != 127) {
            if (cscore <= 0) {
                ++uiUnseen;
            }
            else {
                ++uiSeen;
            }
        }
    }
    spLosMap->iTotalTrianglesSeen = uiSeen;
    spLosMap->iTotalTrianglesUnseen = uiUnseen;
    aspGridMap->SetSeenTrianglesFromLOSMap(spLosMap);
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
    NiPoint3 headingVector = NiPoint3::ZERO;
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
                currentTriangle + icurrentOffset,
                pcurrentNavMesh->GetNeighborCount(currentTriangle)
            );
            crossedTriangles.Add(firstTriangle);
            if (currentTriangle == goalTriangle && pcurrentNavMesh == spGoalMesh.m_pObject) {
                bLineOfSightFound = true;
            }
            else {
                int32_t currentEdgeIndex = pcurrentNavMesh
                                               ->FindInterectingEdge(currentTriangle, -1, start, end, &intersection);
                while (currentEdgeIndex != -1) {
                    // GetMatchingEdge fills these when it returns true; they are read only then.
                    NavMesh* potherMesh;
                    uint16_t usotherTri;
                    int32_t iotherEdge;
                    if (!pcurrentNavMesh->GetMatchingEdge(
                            currentTriangle,
                            currentEdgeIndex,
                            &potherMesh,
                            &usotherTri,
                            &iotherEdge
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
                            otherEdgeVerts.Vertices[0]->Location.x,
                            otherEdgeVerts.Vertices[0]->Location.y
                        );
                        NiPoint2 otherEdgeVert2(
                            otherEdgeVerts.Vertices[1]->Location.x,
                            otherEdgeVerts.Vertices[1]->Location.y
                        );
                        intersection = NavMeshMath::ClosestPointOnSegmentToLine(
                            otherEdgeVert1,
                            otherEdgeVert2,
                            intersection,
                            end
                        );
                    }
                    pcurrentNavMesh = potherMesh;
                    currentTriangle = usotherTri;
                    CrossedTriangle nextTriangle(
                        currentTriangle + icurrentOffset,
                        pcurrentNavMesh->GetNeighborCount(currentTriangle)
                    );
                    crossedTriangles.Add(nextTriangle);
                    NiPoint2 newIntersection;
                    currentEdgeIndex = potherMesh->FindInterectingEdge(
                        usotherTri,
                        iotherEdge,
                        intersection,
                        end,
                        &newIntersection
                    );
                    intersection = newIntersection;
                    if (currentTriangle == goalTriangle && pcurrentNavMesh == spGoalMesh.m_pObject) {
                        bLineOfSightFound = true;
                        break;
                    }
                }
            }
            if (bLineOfSightFound) {
                // Every crossed triangle is seen: its score is its neighbour count.
                for (const CrossedTriangle& rTriangle : crossedTriangles) {
                    uint8_t ucNeighbors = rTriangle.iNeighborCount;
                    aspRetMap->SetAt(rTriangle.iTriangleIndex, ucNeighbors);
                }
                for (const CrossedTriangle& rTriangle : crossedTriangles) {
                    if (static_cast<int8_t>(aspRetMap->iMaxScore) == 3) {
                        break;
                    }
                    if (static_cast<int8_t>(rTriangle.iNeighborCount) > static_cast<int8_t>(aspRetMap->iMaxScore)) {
                        aspRetMap->iMaxScore = rTriangle.iNeighborCount;
                    }
                }
            }
            else {
                // The crossed triangles not scored yet are unseen: the negated neighbour count.
                for (const CrossedTriangle& rTriangle : crossedTriangles) {
                    if (aspRetMap->GetTriangleScore(rTriangle.iTriangleIndex) == 127) {
                        int32_t iScore = -static_cast<int8_t>(rTriangle.iNeighborCount);
                        aspRetMap->SetAt(rTriangle.iTriangleIndex, iScore);
                    }
                }
                for (const CrossedTriangle& rTriangle : crossedTriangles) {
                    if (aspRetMap->iMinScore == -3) {
                        break;
                    }
                    if (-static_cast<int8_t>(rTriangle.iNeighborCount) < aspRetMap->iMinScore) {
                        aspRetMap->iMinScore = -static_cast<int8_t>(rTriangle.iNeighborCount);
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
    if (TES::pInstance->pInteriorCell) {
        NavMeshArray* parray = TES::pInstance->pInteriorCell->GetNavMeshArray();
        if (parray) {
            for (uint32_t i = 0; i < parray->GetNavMeshCount(); ++i) {
                apRetNavMeshes->Add(parray->GetNavMeshByIndex(i));
            }
        }
    }
    else {
        int32_t isize = TES::pInstance->GetExteriorBufferSize();
        for (uint32_t i = 0; i < isize; ++i) {
            TESObjectCELL* pcell = TES::pInstance->GetExteriorBufferCellByIndex(i);
            if (pcell) {
                NavMeshArray* parray = pcell->GetNavMeshArray();
                if (parray) {
                    for (uint32_t j = 0; j < parray->GetNavMeshCount(); ++j) {
                        apRetNavMeshes->Add(parray->GetNavMeshByIndex(j));
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

    return 0;
}

bool Pathing::CheckSphere(PathingLocation& aSphereCenter, float afRadius) {
    NiPoint3 aabbMin;
    NiPoint3 minExtent;
    NiPoint3 aabbMax;
    NiPoint3 maxExtent;
    using navmeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    using openEdges_t = BSSimpleArray<NavMeshEdgeHandle, 1024>;
    using potentialTriangles_t = BSSimpleArray<NavMeshTriHandle, 1024>;
    potentialTriangles_t potentialTriangles;
    navmeshes_t navmeshes;
    Pathing::FindConnectedNavmeshesForLocation(aSphereCenter, afRadius, &navmeshes);
    for (uint32_t i = 0; i < navmeshes.iSize; ++i) {
        using trisForThisMesh_t = BSSimpleArray<uint16_t, 1024>;
        trisForThisMesh_t trisForThisMesh;
        NiPoint3* pmaxExtent = ::new (&maxExtent) NiPoint3(afRadius, afRadius, 0.0f);
        NiPoint3* paabbMax = ::new (&aabbMax) NiPoint3(aSphereCenter.GetLocation() + *pmaxExtent);
        NiPoint3* pminExtent = ::new (&minExtent) NiPoint3(afRadius, afRadius, 0.0f);
        NiPoint3* paabbMin = ::new (&aabbMin) NiPoint3(aSphereCenter.GetLocation() - *pminExtent);
        navmeshes.GetAt(static_cast<int32_t>(i))->GetTrianglesForAABB(*paabbMin, *paabbMax, &trisForThisMesh);
        for (uint32_t j = 0; j < trisForThisMesh.iSize; ++j) {
            uint16_t usTriangle = trisForThisMesh.GetAt(static_cast<int32_t>(j));
            NavMeshInfo* pInfo = navmeshes.GetAt(static_cast<int32_t>(i))->GetNavMeshInfo();
            potentialTriangles.Add(NavMeshTriHandle(pInfo, usTriangle));
        }
    }
    openEdges_t openEdges;
    for (uint32_t i = 0; i < potentialTriangles.iSize; ++i) {
        NavMeshPtr spmesh;
        if (potentialTriangles.GetAt(static_cast<int32_t>(i)).pNavMeshInfo->GetNavMesh(spmesh)) {
            for (int32_t e = 0; e < 3; ++e) {
                NavMeshTriangle* ptriangle = spmesh->GetTriangle(
                    potentialTriangles.GetAt(static_cast<int32_t>(i)).usTriangle
                );
                if (!ptriangle->IsEdgeConnected(e)) {
                    NavMeshEdgeVertices verts = spmesh->GetEdgeVertices(
                        potentialTriangles.GetAt(static_cast<int32_t>(i)).usTriangle,
                        e
                    );
                    if (NavMeshMath::SegmentCircleIntersection(
                            verts.Vertices[0]->Location,
                            verts.Vertices[1]->Location,
                            aSphereCenter.GetLocation(),
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
    return Pathing::GetNavMeshZForLocation(tempLoc, aRetHeight);
}

NiPoint3 Pathing::ComputeInitialTangentForActor(
    float afHeading,
    const PathingLocation& aLocation,
    const PathingLocation& aNextLocation,
    float afRadius
) {
    NiPoint3 backOffset;
    NiPoint3 rightStart;
    NiPoint3 leftStart;
    // Each PathingLocation is built and destroyed only through its own Construct/Destruct calls below.
    NiMatrix3 rot;
    rot.MakeZRotation(afHeading);
    NiPoint3 direction = rot * NiPoint3::UNIT_Y;
    PathingLocation startLoc(aLocation);
    NiPoint3 start = startLoc.GetLocation();
    NiPoint3 end = start + direction * 256.0f;
    PathingLocation retIntersection;
    if (!Pathing::CheckLineOfSight(startLoc, end, &retIntersection, nullptr)) {
        end = retIntersection.GetLocation();
    }
    NiPoint3 delta = end - start;
    NiPoint3 rightVec = delta.UnitCross(NiPoint3::UNIT_Z) * afRadius;
    PathingLocation startLeft(start - rightVec, startLoc);
    PathingLocation goalLeft(end - rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startLeft, goalLeft, &retIntersection, nullptr)) {
        NiPoint3* pleftStart = ::new (&leftStart) NiPoint3(startLeft.GetLocation());
        double dintersectionSqrDist = (retIntersection.GetLocation() - *pleftStart).SqrLength();
        if (delta.SqrLength() > dintersectionSqrDist) {
            end = retIntersection.GetLocation();
        }
    }
    PathingLocation startRight(start + rightVec, startLoc);
    PathingLocation goalRight(end + rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startRight, goalRight, &retIntersection, nullptr)) {
        NiPoint3* prightStart = ::new (&rightStart) NiPoint3(startRight.GetLocation());
        double dintersectionSqrDist = (retIntersection.GetLocation() - *prightStart).SqrLength();
        if (delta.SqrLength() > dintersectionSqrDist) {
            end = retIntersection.GetLocation();
        }
    }
    NiPoint3* pbackOffset = ::new (&backOffset) NiPoint3(direction * afRadius * 1.2f);
    end -= *pbackOffset;
    PathingLocation nextLoc(aNextLocation);
    NiPoint3 next = nextLoc.GetLocation();
    PathingLocation retIntersection2;
    if (Pathing::CheckLineOfSight(nextLoc, end, &retIntersection2, nullptr)) {
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
                return end - start;
            }

            return NiPoint3::ZERO;
        }

        return NiPoint3::ZERO;
    }

    return NiPoint3::ZERO;
}

NiPoint3 Pathing::ComputeFinalTangentForActor(float afHeading, const PathingLocation& aLocation, float afRadius) {
    NiPoint3 rightStart;
    NiPoint3 leftStart;
    // Each of these PathingLocations is built and destroyed only through its own Construct/Destruct calls below.
    NiMatrix3 rot;
    rot.MakeZRotation(afHeading);
    NiPoint3 direction = rot * NiPoint3::UNIT_Y;
    PathingLocation startLoc(aLocation);
    NiPoint3 start = startLoc.GetLocation();
    NiPoint3 end = start - direction * 256.0f;
    PathingLocation retIntersection;
    if (!Pathing::CheckLineOfSight(startLoc, end, &retIntersection, nullptr)) {
        end = retIntersection.GetLocation();
    }
    NiPoint3 delta = end - start;
    NiPoint3 rightVec = delta.UnitCross(NiPoint3::UNIT_Z) * afRadius;
    PathingLocation startLeft(start - rightVec, startLoc);
    PathingLocation goalLeft(end - rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startLeft, goalLeft, &retIntersection, nullptr)) {
        NiPoint3* pleftStart = ::new (&leftStart) NiPoint3(startLeft.GetLocation());
        double dintersectionSqrDist = (retIntersection.GetLocation() - *pleftStart).SqrLength();
        if (delta.SqrLength() > dintersectionSqrDist) {
            end = retIntersection.GetLocation();
        }
    }
    PathingLocation startRight(start + rightVec, startLoc);
    PathingLocation goalRight(end + rightVec, startLoc);
    if (!Pathing::CheckLineOfSight(startRight, goalRight, &retIntersection, nullptr)) {
        NiPoint3* prightStart = ::new (&rightStart) NiPoint3(startRight.GetLocation());
        double dintersectionSqrDist = (retIntersection.GetLocation() - *prightStart).SqrLength();
        if (delta.SqrLength() > dintersectionSqrDist) {
            end = retIntersection.GetLocation();
        }
    }
    return start - end;
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
    return pNavMesh == aNavMeshEdgeHandle.pNavMesh && usTriangle == aNavMeshEdgeHandle.usTriangle;
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
    apThis->hTri = arOther.hTri;
    for (uint32_t i = 0; i < 3; ++i) {
        apThis->bInradius[i] = arOther.bInradius[i];
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMinDistance), sizeof(fMinDistance), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMaxDistance), sizeof(fMaxDistance), 0);
}

// Reads the base's fields, then the two distances.
void PathingRequestClosePoint::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    PathingRequest::LoadGame(apLoadGameBuffer);
    // The buffer fills the two distances as raw bytes through their addresses.
    apLoadGameBuffer->BGSLoadGameBuffer::LoadData(&fMinDistance, sizeof(fMinDistance));
    apLoadGameBuffer->BGSLoadGameBuffer::LoadData(&fMaxDistance, sizeof(fMaxDistance));
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
  : bDirtyFlag(false), bReserved(false), cCoverHeight(0), iLocation(0), CoverSearchTargetLocation(NiPoint3::ZERO),
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
