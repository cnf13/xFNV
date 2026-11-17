#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/FastNavMeshLocation.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshPOV.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/RemoveIndex.h"
#include "Fallout Shared/Pathfinding/Sphere.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <new>

NiPointer<NiNode> NavMeshPOVSearch::spRootDrawNode;

// The exe's vtable addresses for this file's instantiations. The handle array's vtable is declared with its class,
// in BSCore/BSSimpleArray.h.
inline constexpr fnvd::Vtable AStarSearch_NavMeshPOVSearch_vftable{};

// The registration macros take a class template as one token.
using NavMeshPOVSearchBase = AStarSearch<NavMeshPOVSearchNode, NavMeshPOVSearchLink>;
using NavMeshPOVArrayArray = BSSimpleArray<BSSimpleArray<NavMeshPOV*, 1024>, 1024>;
using FastNavMeshVertHandleArrayArray = BSSimpleArray<BSSimpleArray<FastNavMeshVertHandle, 1024>, 1024>;

// NEEDS LAYOUT, not registered: the node pointer array's destructor (0x6A2580: stores vtable 0x106B44C, then
// Clear(true) 0x8454F0), the node pointer scrap array's destructor (0x6A25D0: stores vtable 0x106B438, Clear(true),
// then the base destructor 0x6A2580) and the node pointer array's constructor (0x6A2850: stores vtable 0x106B44C,
// then _InitialSetup(0, 0) 0x6B3EB0). Instantiating BSSimpleArray<AStarNode<NavMeshPOVSearchNode,
// NavMeshPOVSearchLink>*, 1024>'s members needs complete NavMeshPOVSearchNode (PDB: pPOV, Direction, iMasterIndex)
// and NavMeshPOVSearchLink (PDB: ExitAngle, EntryAngle, ExitLocation, EntryLocation) classes.
NavMeshPOVSearch::NavMeshPOVSearch() {}

NavMeshPOVSearch::~NavMeshPOVSearch() {}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshPOVSearch::GetNodeConnections(const AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* apNode,
	BSSimpleArray<AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>*, 1024>* apRetNodes) {
	const NavMeshPOVSearchNode* parentPOVNode = &apNode->GetNodeData();
	const NavMeshPOVSearchLink* parentPOVLink = &apNode->GetParentLink();
	uint32_t imasterSize = POVs.iSize;
	for (uint32_t imaster = parentPOVNode->iMasterIndex; imaster < imasterSize; ++imaster) {
		BSSimpleArray<NavMeshPOV*, 1024>& thisPOVArray = POVs.GetAt(static_cast<int32_t>(imaster));
		uint32_t isize = thisPOVArray.iSize;
		for (uint32_t i = 0; i < isize; ++i) {
			NavMeshPOV* pPOV = thisPOVArray.GetAt(static_cast<int32_t>(i));
			if (pPOV == parentPOVNode->pPOV) {
				continue;
			}
			if (apNode->pParentAStarNode && pPOV == apNode->pParentAStarNode->GetNodeData().pPOV) {
				continue;
			}
			// Too steep: the height difference beyond 1.2 times the horizontal distance.
			NiPoint3 vecToNode = pPOV->Center - parentPOVNode->pPOV->Center;
			float fxySqrDist = vecToNode.SqrXYLength();
			float fzSqrDist = vecToNode.z * vecToNode.z;
			if (fzSqrDist > fxySqrDist * 1.2000000476837158 * 1.2000000476837158) {
				continue;
			}
			FNVD_CONSTRUCTED_LOCAL(Sphere, parentPOVSphere, nullptr, parentPOVNode->pPOV->Center,
				parentPOVNode->pPOV->fRadius);
			FNVD_CONSTRUCTED_LOCAL(Sphere, thisPOVSphere, nullptr, pPOV->Center, pPOV->fRadius);
			FNVD_CONSTRUCTED_LOCAL(NavMeshMath::CCTResults, tangentResults, nullptr);
			int32_t iValidSideCount = NavMeshMath::ComputeTangentPoints(parentPOVSphere, thisPOVSphere,
				parentPOVNode->Direction, tangentResults);
			if (!iValidSideCount) {
				continue;
			}
			for (int32_t e = 0; e < iValidSideCount; ++e) {
				if (apNode->pParentAStarNode) {
					// A tangent leaving the parent's circle within the three quarter turn behind the entry angle
					// would turn back on the path.
					AngleSinCos parentAnglePlusPI;
					parentAnglePlusPI = parentPOVLink->EntryAngle;
					if (parentPOVNode->Direction == AD_CCW) {
						AngleSinCos kTurn;
						AngleSinCos* pTurn = FNVD_RECONSTRUCT(AngleSinCos)(&kTurn, 4.712389f);
						parentAnglePlusPI.operator+=(*pTurn);
					} else {
						AngleSinCos kTurn;
						AngleSinCos* pTurn = FNVD_RECONSTRUCT(AngleSinCos)(&kTurn, 4.712389f);
						parentAnglePlusPI.operator-=(*pTurn);
					}
					AngleSinCosArc arc;
					arc.~AngleSinCosArc();
					::new (&arc) AngleSinCosArc(parentAnglePlusPI, parentPOVLink->EntryAngle, parentPOVNode->Direction);
					if (arc.WithinRange(tangentResults.Results[e].Circle1Param)) {
						continue;
					}
				}
				NiPoint3 cross1 = NiPoint3_ZERO;
				NiPoint3 cross2 = NiPoint3_ZERO;
				// The tangent segment, from the point it leaves the parent's circle to the point it touches this one.
				NiPoint3 exitPoint;
				NiPoint3* pExitPoint =
					tangentResults.Results[e].Circle1Param.ComputePoint3(&exitPoint, parentPOVSphere.fRadius);
				NiPoint3 entryPoint;
				NiPoint3* pEntryPoint =
					tangentResults.Results[e].Circle2Param.ComputePoint3(&entryPoint, thisPOVSphere.fRadius);
				NiPoint3 delta = thisPOVSphere.Center + *pEntryPoint - parentPOVSphere.Center - *pExitPoint;
				if (parentPOVNode->pPOV->eType != NavMeshPOV::POV_TYPE_START &&
					pPOV->eType == NavMeshPOV::POV_TYPE_VERTEX) {
					// A vertex POV is entered only from outside the corner its two edges make.
					float fThreshold = 1.0f;
					vecToNode.z = 0.0f;
					NiPoint3 edge1 = pPOV->Edge1;
					edge1.z = 0.0f;
					NiPoint3 edge2 = pPOV->Edge2;
					edge2.z = 0.0f;
					cross1 = edge1.Cross(delta);
					cross2 = edge2.Cross(delta);
					if (fThreshold < cross1.z && fThreshold < cross2.z) {
						continue;
					}
				}
				if (parentPOVNode->pPOV->eType != NavMeshPOV::POV_TYPE_START &&
					pPOV->eType == NavMeshPOV::POV_TYPE_VERTEX) {
					if (tangentResults.Results[e].Direction != AD_CCW) {
						if (cross1.z < -0.10000000149011612) {
							continue;
						}
					} else {
						if (cross2.z < -0.10000000149011612) {
							continue;
						}
					}
				}
				AngleSinCosArc arcToTangent;
				arcToTangent.~AngleSinCosArc();
				::new (&arcToTangent) AngleSinCosArc(parentPOVLink->EntryAngle, tangentResults.Results[e].Circle1Param,
					parentPOVNode->Direction);
				FastNavMeshEdgeLocation parentEntryLocation;
				parentEntryLocation.~FastNavMeshEdgeLocation();
				::new (&parentEntryLocation) FastNavMeshEdgeLocation;
				if (apNode->pParentAStarNode) {
					parentEntryLocation.Copy(parentPOVLink->EntryLocation);
				} else {
					FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
					uint16_t startTriangle;
					::new (&spmesh) NavMeshPtr;
					if (!StartLocation.GetNavMeshAndTriangle(spmesh, startTriangle)) {
						FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
						return false;
					}
					parentEntryLocation.pNavMesh = spmesh.m_pObject;
					parentEntryLocation.usTriangle = startTriangle;
					parentEntryLocation.iEdgeIndex = -1;
					parentEntryLocation.Location = StartLocation.GetLocation();
					FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
				}
				float favoidNodeArcFactor = 1.0f;
				FastNavMeshEdgeLocation exitLocation;
				exitLocation.~FastNavMeshEdgeLocation();
				::new (&exitLocation) FastNavMeshEdgeLocation;
				if (apNode->pParentAStarNode) {
					// The walk around the parent's circle to the tangent's exit point.
					AngleSinCosArc arcArgument;
					arcArgument.~AngleSinCosArc();
					::new (&arcArgument) AngleSinCosArc(&arcToTangent);
					if (!CheckArcAgainstNavMesh2(parentEntryLocation, parentPOVSphere, arcArgument, &exitLocation)) {
						if (NavMeshPOVSearch::bDrawDebugData) {
							NavMeshPOVSearch::DrawArc(parentPOVSphere.Center, parentPOVSphere.fRadius,
								arcToTangent.Start, arcToTangent.End, arcToTangent.Dir, NiColorA::Yellow);
						}
						continue;
					}
					NiPoint3 exitOffset = exitLocation.Location - parentPOVSphere.Center;
					if (std::fabs(exitOffset.z) >= 180.0) {
						continue;
					}
					AngleSinCosArc arcArgument2;
					arcArgument2.~AngleSinCosArc();
					::new (&arcArgument2) AngleSinCosArc(&arcToTangent);
					favoidNodeArcFactor = CheckArcAgainstPOVs(parentPOVNode->pPOV, arcArgument2, pPOV);
					if (favoidNodeArcFactor == 3.4028234663852886e+38) {
						if (NavMeshPOVSearch::bDrawDebugData) {
							NavMeshPOVSearch::DrawArc(parentPOVSphere.Center, parentPOVSphere.fRadius,
								arcToTangent.Start, arcToTangent.End, arcToTangent.Dir, NiColorA::Red);
						}
						continue;
					}
				} else {
					exitLocation.Copy(parentEntryLocation);
				}
				FastNavMeshEdgeLocation entryLocation;
				entryLocation.~FastNavMeshEdgeLocation();
				::new (&entryLocation) FastNavMeshEdgeLocation;
				NiPoint3 entryPoint2;
				NiPoint3* pEntryPoint2 =
					tangentResults.Results[e].Circle2Param.ComputePoint3(&entryPoint2, thisPOVSphere.fRadius);
				NiPoint3 entryLoc = thisPOVSphere.Center + *pEntryPoint2;
				if (!CheckSegmentAgainstNavMesh(exitLocation, entryLoc, &entryLocation)) {
					if (NavMeshPOVSearch::bDrawDebugData) {
						NavMeshPOVSearch::DrawLine(exitLocation.Location, entryLocation.Location, NiColorA::Yellow);
					}
					continue;
				}
				NiPoint3 entryOffset = entryLocation.Location - thisPOVSphere.Center;
				if (std::fabs(entryOffset.z) >= 180.0) {
					continue;
				}
				float favoidNodeSegmentFactor =
					CheckSegmentAgainstPOVs(parentPOVNode->pPOV, exitLocation.Location, entryLocation.Location, pPOV);
				if (favoidNodeSegmentFactor == 3.4028234663852886e+38) {
					if (NavMeshPOVSearch::bDrawDebugData) {
						NavMeshPOVSearch::DrawLine(exitLocation.Location, entryLocation.Location, NiColorA::Red);
					}
					continue;
				}
				if (NavMeshPOVSearch::bDrawDebugData) {
					if (apNode->pParentAStarNode) {
						NavMeshPOVSearch::DrawArc(parentPOVSphere.Center, parentPOVSphere.fRadius, arcToTangent.Start,
							arcToTangent.End, arcToTangent.Dir, NiColorA::Green);
					}
					NavMeshPOVSearch::DrawLine(exitLocation.Location, entryLocation.Location, NiColorA::Green);
				}
				float fcost = apNode->fCost;
				if (apNode->pParentAStarNode) {
					double fArcLength = arcToTangent.GetLength(parentPOVNode->pPOV->fRadius);
					fcost = fArcLength * (pPOV->fCostFactor + favoidNodeArcFactor + favoidNodeSegmentFactor) + fcost;
				}
				fcost = (entryLocation.Location - exitLocation.Location).Length() + fcost;
				AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* pnode;
				uintptr_t uiKey = GetKeyForNode(pPOV, tangentResults.Results[e].Direction);
				int32_t iResult = GetOrCreateAStarNode(uiKey, fcost, &pnode);
				if (iResult == 0) {
					SetNodeData(pnode, pPOV, tangentResults.Results[e].Direction, static_cast<int32_t>(imaster));
				} else if (iResult != 1) {
					continue;
				}
				pnode->ParentLink.ExitLocation.Copy(exitLocation);
				pnode->ParentLink.EntryLocation.Copy(entryLocation);
				pnode->ParentLink.ExitAngle = tangentResults.Results[e].Circle1Param;
				pnode->ParentLink.EntryAngle = tangentResults.Results[e].Circle2Param;
				pnode->SetCost(fcost);
				apRetNodes->Add(pnode);
			}
		}
	}
	return true;
}

bool NavMeshPOVSearch::IsGoalNode(const AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* apNode) {
	return (&apNode->GetNodeData())->pPOV->eType == NavMeshPOV::POV_TYPE_GOAL;
}

// The exe computes the unit direction from the start to the end point and never uses it. The other side's handle
// starts from the out-of-line empty constructor 0x6240D0. When the start already lies on the far side's navmesh the
// walk resumes from the crossing point; otherwise from the closest point of the far edge to the segment, moved 1%
// towards the edge's middle.
bool NavMeshPOVSearch::CheckSegmentAgainstNavMesh(const FastNavMeshEdgeLocation& aStartLocation,
	const NiPoint3& aEndLocation, FastNavMeshEdgeLocation* apReturnLocation) {
	NiPoint3 kStart = aStartLocation.Location;
	NiPoint3 kEnd = aEndLocation;
	NiPoint3 kDirection = kEnd - kStart;
	kDirection.Unitize();
	NiPoint3 kCurrent = kStart;
	FastNavMeshEdgeHandle kHandle;
	kHandle.~FastNavMeshEdgeHandle();
	::new (&kHandle) FastNavMeshEdgeHandle(aStartLocation);
	while (true) {
		// The exe calls 0x698160 with its result in this frame.
		NavMesh::FIEResult kResult;
		kHandle.pNavMesh->FindInterectingEdge2(&kResult, kHandle.usTriangle, kHandle.iEdgeIndex, kCurrent, kEnd);
		if (kResult.eType == NavMesh::FIE_NONE) {
			FastNavMeshEdgeLocation kLocation;
			apReturnLocation->Copy(*FNVD_RECONSTRUCT(FastNavMeshEdgeLocation)(&kLocation, kHandle, kEnd));
			return true;
		}
		FastNavMeshEdgeHandle kEdge;
		kEdge.~FastNavMeshEdgeHandle();
		::new (&kEdge) FastNavMeshEdgeHandle(kHandle.pNavMesh, kHandle.usTriangle, kResult.iEdge);
		FastNavMeshEdgeHandle kOtherEdge;
		kOtherEdge.~FastNavMeshEdgeHandle();
		::new (&kOtherEdge) FastNavMeshEdgeHandle;
		bool bCrossed = kEdge.pNavMesh->GetMatchingEdge(kEdge, &kOtherEdge);
		if (bCrossed) {
			double fRadius = pRequest->GetActorRadius();
			if (fPathingLargeActorRadius.Float() < fRadius &&
				kOtherEdge.pNavMesh->GetTriangle(kOtherEdge.usTriangle)->IsNoLargeCreature())
				bCrossed = false;
			EdgeExtraInfo* pInfo = kEdge.pNavMesh->GetEdgeExtraInfo(kEdge.usTriangle, kEdge.iEdgeIndex);
			if (pInfo && pInfo->eType == EIT_LEDGE_UP)
				bCrossed = false;
		}
		if (!bCrossed) {
			FastNavMeshEdgeLocation kLocation;
			apReturnLocation->Copy(*FNVD_RECONSTRUCT(FastNavMeshEdgeLocation)(&kLocation, kEdge, kResult.Intersection));
			return false;
		}
		if (kHandle.pNavMesh != kOtherEdge.pNavMesh) {
			NavMeshEdgeVertices kVertices =
				kOtherEdge.pNavMesh->GetEdgeVertices(kOtherEdge.usTriangle, kOtherEdge.iEdgeIndex);
			NiPoint3 kMiddle = (kVertices.Vertices[0]->Location + kVertices.Vertices[1]->Location) * 0.5f;
			kCurrent = NavMeshMath::ClosestPointOnSegmentToLine3(kVertices.Vertices[0]->Location,
				kVertices.Vertices[1]->Location, kResult.Intersection, kEnd);
			kCurrent += (kMiddle - kCurrent) * 0.01f;
		} else {
			kCurrent = kResult.Intersection;
		}
		kHandle.Copy(kOtherEdge);
	}
}

float NavMeshPOVSearch::CheckArcAgainstPOVs(NavMeshPOV* apThisPOV, AngleSinCosArc aArc, NavMeshPOV* apIgnorePOV) {
	NiPoint3 kPoint;
	FNVD_CONSTRUCTED_LOCAL(Sphere, thisSphere, nullptr, apThisPOV->Center, apThisPOV->fRadius);
	NiPoint3* pPoint = aArc.Start.ComputePoint3(&kPoint, thisSphere.fRadius);
	NiPoint3 kStart = thisSphere.Center + *pPoint;
	for (uint32_t i = 0; i < POVs.iSize; ++i) {
		for (uint32_t j = 0; j < (&POVs.GetAt(static_cast<int32_t>(i)))->iSize; ++j) {
			NavMeshPOV* pPOV = (&POVs.GetAt(static_cast<int32_t>(i)))->GetAt(static_cast<int32_t>(j));
			if (pPOV == apIgnorePOV || pPOV == apThisPOV)
				continue;
			if (pPOV->pDuplicates->Find(apThisPOV, 0, &BSSimpleArray<NavMeshPOV*>::DefaultEqualFunctionByAddress) != -1)
				continue;
			FNVD_CONSTRUCTED_LOCAL(Sphere, otherSphere, nullptr, pPOV->Center, pPOV->fRadius);
			double fDistanceSqr = (otherSphere.Center - kStart).SqrLength();
			if (!(otherSphere.fRadius * otherSphere.fRadius > fDistanceSqr) &&
				!NavMeshMath::CheckArcAgainstCircle(thisSphere, aArc, otherSphere)) {
				if (pPOV->eType == NavMeshPOV::POV_TYPE_AVOIDNODE)
					return 2.0f;
				return 100.0f;
			}
		}
	}
	return 1.0f;
}

// The exe also keeps the last step's edge index in a local it never reads.
bool NavMeshPOVSearch::CheckArcAgainstNavMesh2(const FastNavMeshEdgeLocation& aEntryLocation, Sphere& aSphere,
	AngleSinCosArc aArc, FastNavMeshEdgeLocation* apReturnLocation) {
	NiPoint3 kEndPoint;
	NiPoint3 kNextPoint;
	NiPoint3 kFirstPoint;
	AngleSinCos kStep;
	AngleSinCos kCurrent;
	FastNavMeshEdgeLocation kReturnLocation;
	if (aArc.Dir) {
		kStep.~AngleSinCos();
		::new (&kStep) AngleSinCos(-0.2617994f);
	} else {
		kStep.~AngleSinCos();
		::new (&kStep) AngleSinCos(0.2617994f);
	}
	kCurrent.operator=(aArc.Start);
	// operator+ builds its result in kNext.
	AngleSinCos kNext = kCurrent + kStep;
	NiPoint3* pPoint = kCurrent.ComputePoint3(&kFirstPoint, aSphere.fRadius);
	NiPoint3 kPoint = aSphere.Center + *pPoint;
	FastNavMeshEdgeLocation kStartLocation(aEntryLocation);
	kReturnLocation.~FastNavMeshEdgeLocation();
	::new (&kReturnLocation) FastNavMeshEdgeLocation;
	uint32_t uiSteps = 0;
	while (aArc.WithinRange(kNext) && uiSteps < 24) {
		kStartLocation.iEdgeIndex = -1;
		if (!CheckSegmentAgainstNavMesh(kStartLocation, kPoint, &kReturnLocation)) {
			apReturnLocation->Copy(kReturnLocation);
			return false;
		}
		kStartLocation.Copy(kReturnLocation);
		kCurrent.operator=(kNext);
		// The sum is built in a temporary, then assigned through operator=.
		kNext = kCurrent + kStep;
		pPoint = kNext.ComputePoint3(&kNextPoint, aSphere.fRadius);
		kPoint = aSphere.Center + *pPoint;
		++uiSteps;
	}
	if (uiSteps == 24) {
		apReturnLocation->Copy(kReturnLocation);
		return false;
	}
	pPoint = aArc.End.ComputePoint3(&kEndPoint, aSphere.fRadius);
	NiPoint3 kEnd = aSphere.Center + *pPoint;
	kStartLocation.iEdgeIndex = -1;
	if (CheckSegmentAgainstNavMesh(kStartLocation, kEnd, &kReturnLocation)) {
		apReturnLocation->Copy(kReturnLocation);
		return true;
	} else {
		apReturnLocation->Copy(kReturnLocation);
		return false;
	}
}

void NavMeshPOVSearch::SetNodeData(AStarNode<NavMeshPOVSearchNode, NavMeshPOVSearchLink>* apNode, NavMeshPOV* apPOV,
	ANGLE_DIRECTION aDirection, int32_t aiMasterIndex) {
	apNode->tNodeData.pPOV = apPOV;
	apNode->tNodeData.Direction = aDirection;
	apNode->tNodeData.iMasterIndex = static_cast<uint32_t>(aiMasterIndex);
	apNode->fHeuristic = (apPOV->Center - pGoalPov->Center).Length() * 0.75;
}

float NavMeshPOVSearch::CheckSegmentAgainstPOVs(NavMeshPOV* apThisPOV, const NiPoint3& aStart, const NiPoint3& aGoal,
	NavMeshPOV* apIgnorePOV) {
	for (uint32_t i = 0; i < POVs.iSize; ++i) {
		for (uint32_t j = 0; j < (&POVs.GetAt(static_cast<int32_t>(i)))->iSize; ++j) {
			NavMeshPOV* pPOV = (&POVs.GetAt(static_cast<int32_t>(i)))->GetAt(static_cast<int32_t>(j));
			if (pPOV == apIgnorePOV || pPOV == apThisPOV)
				continue;
			if (pPOV->pDuplicates->Find(apThisPOV, 0, &BSSimpleArray<NavMeshPOV*>::DefaultEqualFunctionByAddress) != -1)
				continue;
			double fDistanceSqr = (pPOV->Center - aStart).SqrLength();
			if (!(pPOV->fRadius * pPOV->fRadius > fDistanceSqr) &&
				NavMeshMath::DoCircleAndSegmentIntersect(pPOV->Center, pPOV->fRadius, aStart, aGoal)) {
				if (pPOV->eType == NavMeshPOV::POV_TYPE_AVOIDNODE)
					return 2.0f;
				return 100.0f;
			}
		}
	}
	return 1.0f;
}

uintptr_t NavMeshPOVSearch::GetKeyForNode(NavMeshPOV* apPOV, ANGLE_DIRECTION aDirection) {
	// The key is the POV's address plus the direction (the address is only a number here, never read through).
	return reinterpret_cast<uintptr_t>(apPOV) + aDirection;
}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshPOVSearch::BuildPOVListForTriangles(const BSSimpleArray<FastNavMeshTriHandle, 1024>& apList,
	BSSimpleArray<BSSimpleArray<NavMeshPOV*, 1024>, 1024>& aRetPOVs) {
	NavMeshPOVData pov;
	// Per POV array: the handles of the vertex POVs and of the obstacle POVs of its triangle.
	using VertHandleArrays = BSSimpleArray<BSSimpleArray<FastNavMeshVertHandle, 1024>, 1024>;
	FNVD_INLINED_LOCAL(VertHandleArrays, povs, std::nullopt, std::nullopt);
	using IndexArray = BSSimpleArray<uint16_t, 1024>;
	FNVD_INLINED_LOCAL(IndexArray, thisTriPOVs, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(VertHandleArrays, obstaclePovs, std::nullopt, std::nullopt);
	using RemoveIndexArray = BSSimpleArray<RemoveIndex, 1024>;
	FNVD_INLINED_LOCAL(RemoveIndexArray, removeList, std::nullopt, std::nullopt);
	POVs.SetSize(apList.iSize + 1, 1);
	::new (&povs) VertHandleArrays(POVs.iSize);
	::new (&obstaclePovs) VertHandleArrays(POVs.iSize);
	::new (&thisTriPOVs) IndexArray;
	for (uint32_t i = 0; i < apList.iSize; ++i) {
		uint16_t usTriangle = apList.GetAt(static_cast<int32_t>(i)).usTriangle;
		uint32_t icount = apList.GetAt(static_cast<int32_t>(i)).pNavMesh->GetPOVsForTriangle(usTriangle, thisTriPOVs);
		for (uint32_t j = 0; j < icount; ++j) {
			uint16_t usVertex = thisTriPOVs.GetAt(static_cast<int32_t>(j));
			FNVD_CONSTRUCTED_LOCAL(FastNavMeshVertHandle, hthisPov, nullptr,
				apList.GetAt(static_cast<int32_t>(i)).pNavMesh, usVertex);
			if (!povs.GetAt(static_cast<int32_t>(i)).IsInArray(hthisPov)) {
				povs.GetAt(static_cast<int32_t>(i)).Add(hthisPov);
			}
		}
		uint16_t usObstacleTriangle = apList.GetAt(static_cast<int32_t>(i)).usTriangle;
		icount =
			apList.GetAt(static_cast<int32_t>(i)).pNavMesh->GetObstaclePOVsForTriangle(usObstacleTriangle, thisTriPOVs);
		for (uint32_t j = 0; j < icount; ++j) {
			uint16_t usVertex = thisTriPOVs.GetAt(static_cast<int32_t>(j));
			FNVD_CONSTRUCTED_LOCAL(FastNavMeshVertHandle, hthisPov, nullptr,
				apList.GetAt(static_cast<int32_t>(i)).pNavMesh, usVertex);
			if (!povs.GetAt(static_cast<int32_t>(i)).IsInArray(hthisPov) &&
				!obstaclePovs.GetAt(static_cast<int32_t>(i)).IsInArray(hthisPov)) {
				obstaclePovs.GetAt(static_cast<int32_t>(i)).Add(hthisPov);
			}
		}
	}
	// A POV for every vertex POV handle; a vertex between two edges with extra info is a portal.
	for (uint32_t i = 0; i < povs.iSize; ++i) {
		for (uint32_t j = 0; j < povs.GetAt(static_cast<int32_t>(i)).iSize; ++j) {
			FastNavMeshVertHandle* hthisPov = &povs.GetAt(static_cast<int32_t>(i)).GetAt(static_cast<int32_t>(j));
			if (!hthisPov->pNavMesh->GetPOVForVertex(hthisPov->usVertex, pov)) {
				continue;
			}
			NavMeshPOV* pNewPOV;
			NavMeshPOV* pMemory = static_cast<NavMeshPOV*>(ourMalloc(sizeof(NavMeshPOV)));
			if (pMemory) {
				pNewPOV = ::new (pMemory) NavMeshPOV;
			} else {
				pNewPOV = nullptr;
			}
			NavMeshPOV* ppov = pNewPOV;
			ppov->eType = NavMeshPOV::POV_TYPE_VERTEX;
			ppov->Center = hthisPov->pNavMesh->GetVertex(hthisPov->usVertex)->Location;
			ppov->fRadius = pRequest->fActorRadius;
			ppov->iVertexIndex = hthisPov->usVertex;
			NavMeshTriangle* ptri1 = hthisPov->pNavMesh->GetTriangle(pov.Edge1.QTriangle());
			NavMeshTriangle* ptri2 = hthisPov->pNavMesh->GetTriangle(pov.Edge2.QTriangle());
			if (ptri1->EdgeHasExtraInfo(pov.Edge1.QEdge()) || ptri2->EdgeHasExtraInfo(pov.Edge2.QEdge())) {
				ppov->eType = NavMeshPOV::POV_TYPE_PORTAL;
			}
			int32_t iEdge1 = pov.Edge1.QEdge();
			NavMeshEdgeVertices verts1 = hthisPov->pNavMesh->GetEdgeVertices(pov.Edge1.QTriangle(), iEdge1);
			int32_t iEdge2 = pov.Edge2.QEdge();
			NavMeshEdgeVertices verts2 = hthisPov->pNavMesh->GetEdgeVertices(pov.Edge2.QTriangle(), iEdge2);
			ppov->Edge1 = verts1.Vertices[1]->Location - verts1.Vertices[0]->Location;
			ppov->Edge2 = verts2.Vertices[1]->Location - verts2.Vertices[0]->Location;
			ppov->Edge1.z = 0.0f;
			ppov->Edge2.z = 0.0f;
			aRetPOVs.GetAt(static_cast<int32_t>(i)).Add(ppov);
		}
	}
	// Two POVs for every obstacle POV handle, each the other's duplicate: one of the actor's radius and one of radius
	// 10 at twice the cost.
	for (uint32_t i = 0; i < obstaclePovs.iSize; ++i) {
		for (uint32_t j = 0; j < obstaclePovs.GetAt(static_cast<int32_t>(i)).iSize; ++j) {
			FastNavMeshVertHandle* hthisPov =
				&obstaclePovs.GetAt(static_cast<int32_t>(i)).GetAt(static_cast<int32_t>(j));
			NavMeshPOV* pNewPOV;
			NavMeshPOV* pMemory = static_cast<NavMeshPOV*>(ourMalloc(sizeof(NavMeshPOV)));
			if (pMemory) {
				pNewPOV = ::new (pMemory) NavMeshPOV;
			} else {
				pNewPOV = nullptr;
			}
			NavMeshPOV* ppov = pNewPOV;
			ppov->eType = NavMeshPOV::POV_TYPE_OBSTACLE;
			ppov->Center = hthisPov->pNavMesh->GetVertex(hthisPov->usVertex)->Location;
			ppov->fRadius = pRequest->fActorRadius;
			ppov->iVertexIndex = hthisPov->usVertex;
			aRetPOVs.GetAt(static_cast<int32_t>(i)).Add(ppov);
			NavMeshPOV* pNewSecondPOV;
			NavMeshPOV* pSecondMemory = static_cast<NavMeshPOV*>(ourMalloc(sizeof(NavMeshPOV)));
			if (pSecondMemory) {
				pNewSecondPOV = ::new (pSecondMemory) NavMeshPOV;
			} else {
				pNewSecondPOV = nullptr;
			}
			NavMeshPOV* psecondPOV = pNewSecondPOV;
			psecondPOV->eType = NavMeshPOV::POV_TYPE_OBSTACLE;
			psecondPOV->Center = hthisPov->pNavMesh->GetVertex(hthisPov->usVertex)->Location;
			psecondPOV->fRadius = 10.0f;
			psecondPOV->iVertexIndex = hthisPov->usVertex;
			psecondPOV->fCostFactor = 2.0f;
			aRetPOVs.GetAt(static_cast<int32_t>(i)).Add(psecondPOV);
			ppov->pDuplicates->Add(psecondPOV);
			psecondPOV->pDuplicates->Add(ppov);
		}
	}
	// Every POV closer than 5 units to another one is removed from the search's POV arrays.
	::new (&removeList) RemoveIndexArray;
	float fSqrDuplicateTolerance = 25.0f;
	for (uint32_t i = 0; i < aRetPOVs.iSize; ++i) {
		for (uint32_t j = 0; j < aRetPOVs.GetAt(static_cast<int32_t>(i)).iSize; ++j) {
			for (uint32_t k = i; k < aRetPOVs.iSize; ++k) {
				if (k == i) {
					for (uint32_t m = j + 1; m < aRetPOVs.GetAt(static_cast<int32_t>(k)).iSize; ++m) {
						NiPoint3* pOtherCenter =
							&aRetPOVs.GetAt(static_cast<int32_t>(k)).GetAt(static_cast<int32_t>(m))->Center;
						NavMeshPOV* pThisPOV = aRetPOVs.GetAt(static_cast<int32_t>(i)).GetAt(static_cast<int32_t>(j));
						if (fSqrDuplicateTolerance > (pThisPOV->Center - *pOtherCenter).SqrLength()) {
							FNVD_CONSTRUCTED_LOCAL(RemoveIndex, findIndex, nullptr, static_cast<int32_t>(i),
								static_cast<int32_t>(j));
							if (removeList.Find(findIndex, 0,
									&BSSimpleArray<RemoveIndex, 1024>::DefaultEqualFunctionByAddress) == -1) {
								FNVD_CONSTRUCTED_LOCAL(RemoveIndex, addIndex, nullptr, static_cast<int32_t>(i),
									static_cast<int32_t>(j));
								removeList.Add(addIndex);
							}
						}
					}
				} else {
					for (uint32_t m = 0; m < aRetPOVs.GetAt(static_cast<int32_t>(k)).iSize; ++m) {
						NiPoint3* pOtherCenter =
							&aRetPOVs.GetAt(static_cast<int32_t>(k)).GetAt(static_cast<int32_t>(m))->Center;
						NavMeshPOV* pThisPOV = aRetPOVs.GetAt(static_cast<int32_t>(i)).GetAt(static_cast<int32_t>(j));
						if (fSqrDuplicateTolerance > (pThisPOV->Center - *pOtherCenter).SqrLength()) {
							FNVD_CONSTRUCTED_LOCAL(RemoveIndex, findIndex, nullptr, static_cast<int32_t>(i),
								static_cast<int32_t>(j));
							if (removeList.Find(findIndex, 0,
									&BSSimpleArray<RemoveIndex, 1024>::DefaultEqualFunctionByAddress) == -1) {
								FNVD_CONSTRUCTED_LOCAL(RemoveIndex, addIndex, nullptr, static_cast<int32_t>(i),
									static_cast<int32_t>(j));
								removeList.Add(addIndex);
							}
						}
					}
				}
			}
		}
	}
	for (int32_t i = static_cast<int32_t>(removeList.iSize) - 1; i >= 0; --i) {
		int32_t iMinorIndex = removeList.GetAt(i).iMinorIndex;
		NavMeshPOV* pRemoved = POVs.GetAt(removeList.GetAt(i).iMasterIndex).GetAt(iMinorIndex);
		if (pRemoved) {
			delete pRemoved;
		}
		uint32_t uiMinorIndex = removeList.GetAt(i).iMinorIndex;
		POVs.GetAt(removeList.GetAt(i).iMasterIndex).Remove(uiMinorIndex, 1);
	}
	removeList.~RemoveIndexArray();
	thisTriPOVs.~IndexArray();
	obstaclePovs.~VertHandleArrays();
	povs.~VertHandleArrays();
	return true;
}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshPOVSearch::BuildPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
	const BSSimpleArray<FastNavMeshTriHandle, 1024>* apList, PathingRequest* apRequest, PathingSolution* apSolution) {
	NavMeshPOV* v25;
	NavMeshPOV* v26;
	NiPoint3 v37;
	// The exe builds and destroys it with PathingLocation's Construct and Destruct calls only (0x6DCF20, 0x4FF7E0).
	FNVD_INLINED_LOCAL(PathingLocation, entryLoc, std::nullopt, std::nullopt);
	NavMeshPOVSearchNode startNode;
	pRequest = apRequest;
	pSolution = apSolution;
	if (NavMeshPOVSearch::bDrawDebugData) {
		NiNode* v33;
		if (NavMeshPOVSearch::spRootDrawNode.operator!=(static_cast<NiNode*>(0))) {
			PathManager* v6 = PathManager::QInstance();
			v6->DrawPath(NavMeshPOVSearch::spRootDrawNode, 0);
		}
		void* apMemory = NiMemObject::operator new(sizeof(NiNode));
		if (apMemory) {
			v33 = ::new (apMemory) NiNode(0);
		} else {
			v33 = 0;
		}
		NiRefObject* v48 = (NiRefObject*)v33;
		NavMeshPOVSearch::spRootDrawNode.operator=(v33);
	}
	BuildPOVListForTriangles(*apList, POVs);
	if (NavMeshPOVSearch::bDrawDebugData) {
		for (uint32_t i = 0;; ++i) {
			uint32_t FormFlags = POVs.iSize;
			if (i >= FormFlags) {
				break;
			}
			for (uint32_t j = 0;; ++j) {
				NiAlphaProperty* palpha;
				uint32_t v8 = (&POVs.GetAt(static_cast<int32_t>(i)))->iSize;
				if (j >= v8) {
					break;
				}
				double v32 = static_cast<double>(i);
				float v31 = v32 / static_cast<double>(POVs.iSize);
				double v30 = static_cast<double>(i);
				float v29 = 1.0 - v30 / static_cast<double>(POVs.iSize);
				FNVD_CONSTRUCTED_LOCAL(NiColorA, color, nullptr, 0.0f, v29, v31, 0.15f);
				FNVD_CONSTRUCTED_LOCAL(NiPointer<NiAVObject>, spnode, &NiPointer<NiAVObject>::Destruct,
					BSTestObjects::CreateSphere((&POVs.GetAt(static_cast<int32_t>(i)))
													->GetAt(static_cast<int32_t>(j))
													->fRadius,
						10, 10, &color));
				spnode.m_pObject
					->SetTranslate((&POVs.GetAt(static_cast<int32_t>(i)))->GetAt(static_cast<int32_t>(j))->Center);
				NiAlphaProperty* v45 =
					static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
				if (v45) {
					palpha = ::new (v45) NiAlphaProperty;
				} else {
					palpha = 0;
				}
				NiAlphaProperty* v46 = palpha;
				NiAlphaProperty* a1 = palpha;
				palpha->SetAlphaBlending(1);
				spnode.m_pObject->AttachProperty(a1);
				AddNoLightingPropertyRecurse(spnode.m_pObject);
				BSShaderManager::PrepareObject(spnode.m_pObject, 0, 0);
				spnode.m_pObject->UpdateProperties();
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData, nullptr, 0.0f, 0, 0);
				spnode.m_pObject->Update(UpdateData);
				NiNode* v27 = NavMeshPOVSearch::spRootDrawNode.m_pObject;
				v27->AttachChild(spnode.m_pObject, 1);
			}
		}
	}
	StartLocation.operator=(aStartLocation);
	GoalLocation.operator=(aGoalLocation);
	StartLocation.ResolveNavMeshInfo(0);
	if (!StartLocation.HasNavMeshData()) {
		return 0;
	}
	NavMeshPOV* v43 = static_cast<NavMeshPOV*>(ourMalloc(sizeof(NavMeshPOV)));
	if (v43) {
		v26 = ::new (v43) NavMeshPOV;
	} else {
		v26 = 0;
	}
	pStartPov = v26;
	pStartPov->eType = NavMeshPOV::POV_TYPE_START;
	pStartPov->Center = aStartLocation.GetLocation();
	pStartPov->fRadius = 0.0f;
	pStartPov->iVertexIndex = -1;
	POVs.GetFirst().Add(pStartPov);
	NavMeshPOV* v40 = static_cast<NavMeshPOV*>(ourMalloc(sizeof(NavMeshPOV)));
	if (v40) {
		v25 = ::new (v40) NavMeshPOV;
	} else {
		v25 = 0;
	}
	pGoalPov = v25;
	pGoalPov->eType = NavMeshPOV::POV_TYPE_GOAL;
	pGoalPov->Center = aGoalLocation.GetLocation();
	pGoalPov->fRadius = 0.0f;
	pGoalPov->iVertexIndex = -1;
	(&POVs.GetAt(static_cast<int32_t>(POVs.iSize - 2)))->Add(pGoalPov);
	startNode.pPOV = pStartPov;
	startNode.Direction = AD_CCW;
	startNode.iMasterIndex = 0;
	NiPoint3* fA = ::new (&v37) NiPoint3(StartLocation.GetLocation());
	float v24 = (GoalLocation.GetLocation() - *fA).Length() * 0.75;
	uintptr_t uiKey = GetKeyForNode(pStartPov, AD_CCW);
	AddStartNode(uiKey, startNode, v24);
	bool bres = Search(&Nodes, &Links, 1);
	for (uint32_t i_2 = 0;; ++i_2) {
		uint32_t v14 = Links.iSize;
		if (i_2 >= v14) {
			break;
		}
		NavMeshPOVSearchLink* v15 = &Links.GetAt(static_cast<int32_t>(i_2));
		NavMeshInfo* fAa = v15->ExitLocation.pNavMesh->GetNavMeshInfo();
		NavMeshPOVSearchLink* v16 = &Links.GetAt(static_cast<int32_t>(i_2));
		FNVD_CONSTRUCTED_LOCAL(PathingLocation, exitLoc, &PathingLocation::Destruct, &v16->ExitLocation.Location, fAa);
		apSolution->AddPathingNode(exitLoc, NiPoint3_ZERO);
		NavMeshPOVSearchLink* v17 = &Links.GetAt(static_cast<int32_t>(i_2));
		NavMeshInfo* fAb = v17->EntryLocation.pNavMesh->GetNavMeshInfo();
		NavMeshPOVSearchLink* v18 = &Links.GetAt(static_cast<int32_t>(i_2));
		::new (&entryLoc) PathingLocation(&v18->EntryLocation.Location, fAb);
		apSolution->AddPathingNode(entryLoc, NiPoint3_ZERO);
		entryLoc.~PathingLocation();
	}
	if (NavMeshPOVSearch::bDrawDebugData) {
		NavMeshPOVSearch::spRootDrawNode.m_pObject->UpdateProperties();
		FNVD_CONSTRUCTED_LOCAL(NiUpdateData, updateData, nullptr, 0.0f, 0, 0);
		NavMeshPOVSearch::spRootDrawNode.m_pObject->Update(updateData);
		PathManager* v19 = PathManager::QInstance();
		v19->DrawPath(NavMeshPOVSearch::spRootDrawNode, 1);
	}
	for (uint32_t i_3 = 0;; ++i_3) {
		uint32_t v20 = allPOVs.iSize;
		if (i_3 >= v20) {
			break;
		}
		NavMeshPOV* apThis = allPOVs.GetAt(static_cast<int32_t>(i_3));
		if (apThis) {
			delete apThis;
		}
	}
	return bres;
}

void NavMeshPOVSearch::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out.
void NavMeshPOVSearch::DrawLine(const NiPoint3& aStart, const NiPoint3& aGoal, const NiColorA& aColor) {
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiLines>, splines, &NiPointer<NiLines>::Destruct,
		MakeLine(aStart, aColor, aGoal, aColor, 1));
	FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData, nullptr, 0.0f, 0, 0);
	splines.m_pObject->UpdateProperties();
	splines.m_pObject->Update(UpdateData);
	NiNode* v3 = NavMeshPOVSearch::spRootDrawNode.m_pObject;
	v3->AttachChild(splines.m_pObject, 1);
}

void NavMeshPOVSearch::DrawArc(const NiPoint3& aCenter, float afRadius, const AngleSinCos& aStartAngle,
	const AngleSinCos& aGoalAngle, ANGLE_DIRECTION aDirection, const NiColorA& aColor) {
	NiPoint3 v11;
	NiPoint3 v13;
	NiPoint3 v15;
	NiPoint3 apResult;
	AngleSinCos aAngleStep;
	uint32_t icount;
	AngleSinCos currentStartAngle;
	FNVD_CONSTRUCTED_LOCAL(AngleSinCosArc, arc, nullptr, aStartAngle, aGoalAngle, aDirection);
	if (aDirection) {
		aAngleStep.~AngleSinCos();
		::new (&aAngleStep) AngleSinCos(-0.2617994f);
	} else {
		aAngleStep.~AngleSinCos();
		::new (&aAngleStep) AngleSinCos(0.2617994f);
	}
	currentStartAngle.operator=(aStartAngle);
	// The exe builds the sum in currentGoalAngle (hidden result pointer).
	AngleSinCos currentGoalAngle = currentStartAngle + aAngleStep;
	NiPoint3* v6 = currentStartAngle.ComputePoint3(&apResult, afRadius);
	NiPoint3 currentStartLoc = aCenter + *v6;
	NiPoint3* v7 = currentGoalAngle.ComputePoint3(&v15, afRadius);
	NiPoint3 currentGoalLoc = aCenter + *v7;
	for (icount = 0; arc.WithinRange(currentGoalAngle) && icount < 24; ++icount) {
		NavMeshPOVSearch::DrawLine(currentStartLoc, currentGoalLoc, aColor);
		currentStartLoc = currentGoalLoc;
		currentStartAngle.operator=(currentGoalAngle);
		// The sum goes into a temporary, then through operator= into currentGoalAngle.
		currentGoalAngle.operator=(currentStartAngle + aAngleStep);
		NiPoint3* v9 = currentGoalAngle.ComputePoint3(&v13, afRadius);
		currentGoalLoc = aCenter + *v9;
	}
	if (icount != 24) {
		NiPoint3* v10 = aGoalAngle.ComputePoint3(&v11, afRadius);
		[[maybe_unused]] NiPoint3 lastGoalLoc = aCenter + *v10;
		NavMeshPOVSearch::DrawLine(currentStartLoc, currentGoalLoc, aColor);
	}
}

AngleSinCos::AngleSinCos(float a1) : fCos(std::cos(a1)), fSin(std::sin(a1)) {}

// The exe's out-of-line empty default constructor of the sum (0x6815C0) is ICF-folded with every other empty function
// and stores nothing; src's default-initialized local writes nothing either. The sum goes into the result through
// operator= (0x6985F0).
AngleSinCos AngleSinCos::operator+(const AngleSinCos& aSinCos) const {
	AngleSinCos lSum;
	lSum.fCos = fCos * aSinCos.fCos - fSin * aSinCos.fSin;
	lSum.fSin = fSin * aSinCos.fCos + fCos * aSinCos.fSin;
	AngleSinCos lResult;
	lResult.operator=(lSum);
	return lResult;
}

int32_t AngleSinCos::QQuadrant() const {
	int32_t v2 = 0;
	if (fSin < 0.0) {
		v2 = 3;
	}
	if (fCos < 0.0) {
		return v2 ^ 1;
	}
	return v2;
}

// Within quadrants 0 and 3 the angle grows with the sine, within 1 and 2 it shrinks. A NaN sine compares false.
bool AngleSinCos::operator<=(const AngleSinCos& aAngle) const {
	int32_t iQuadrant = QQuadrant();
	int32_t iOtherQuadrant = aAngle.QQuadrant();
	if (iQuadrant < iOtherQuadrant) {
		return true;
	}
	if (iQuadrant > iOtherQuadrant) {
		return false;
	}
	if (iQuadrant == 0 || iQuadrant == 3) {
		return aAngle.fSin >= fSin;
	}
	return aAngle.fSin <= fSin;
}

float AngleSinCos::GetAngle() const {
	return std::atan2(fSin, fCos);
}

// The exe builds the point with the out-of-line three-float constructor (0x416870), y computed before x.
NiPoint3* AngleSinCos::ComputePoint3(NiPoint3* apResult, float afRadius) const {
	float fY = afRadius * fSin;
	float fX = afRadius * fCos;
	::new (apResult) NiPoint3(fX, fY, 0.0f);
	return apResult;
}

AngleSinCos& AngleSinCos::operator+=(const AngleSinCos& aSinCos) {
	float fCos = this->fCos;
	float fSin = this->fSin;
	this->fCos = fCos * aSinCos.fCos - fSin * aSinCos.fSin;
	this->fSin = fSin * aSinCos.fCos + fCos * aSinCos.fSin;
	return *this;
}

AngleSinCos& AngleSinCos::operator-=(const AngleSinCos& aSinCos) {
	float fCos = this->fCos;
	float fSin = this->fSin;
	this->fCos = fCos * aSinCos.fCos + fSin * aSinCos.fSin;
	this->fSin = fSin * aSinCos.fCos - fCos * aSinCos.fSin;
	return *this;
}

AngleSinCosArc::AngleSinCosArc(AngleSinCosArc* a1) : Start(a1->Start), End(a1->End), Dir(a1->Dir) {}

AngleSinCosArc::AngleSinCosArc(const AngleSinCos& aStart, const AngleSinCos& aEnd, int32_t aeDir)
	: Start(aStart), End(aEnd), Dir(static_cast<ANGLE_DIRECTION>(aeDir)) {}

bool AngleSinCosArc::WithinRange(const AngleSinCos& aAngle) const {
	const AngleSinCos* ccwEnd;
	const AngleSinCos* ccwStart;
	if (Dir) {
		ccwStart = &End;
	} else {
		ccwStart = &Start;
	}
	if (Dir) {
		ccwEnd = &Start;
	} else {
		ccwEnd = &End;
	}
	AngleSinCos Delta = *ccwEnd - *ccwStart;
	AngleSinCos relativeAngle = aAngle - *ccwStart;
	return relativeAngle.operator<=(Delta);
}

double AngleSinCosArc::GetLength(float a1) {
	// The exe's operator- builds its result in place; no assignment follows.
	AngleSinCos result = End - Start;
	float Angle = result.GetAngle();
	if (Dir) {
		if (!(Angle < 0.0)) {
			return static_cast<float>((6.2831854820251465 - Angle) * a1);
		} else {
			return static_cast<float>(-Angle * a1);
		}
	} else {
		if (!(Angle < 0.0)) {
			return static_cast<float>(Angle * a1);
		} else {
			return static_cast<float>((Angle + 6.2831854820251465) * a1);
		}
	}
}

int32_t NavMeshPOVEdge::QTriangle() {
	return EdgeIndex & 0x3FFF;
}

int32_t NavMeshPOVEdge::QEdge() {
	return (EdgeIndex & 0xC000) >> 14;
}

NavMeshPOV::NavMeshPOV()
	: eType(POV_TYPE_INVALID), Center(NiPoint3_ZERO), fRadius(0.0f), fCostFactor(1.0f),
	  pDuplicates(new (ourMalloc(sizeof(BSSimpleArray<NavMeshPOV*, 1024>))) BSSimpleArray<NavMeshPOV*, 1024>) {}

NavMeshPOV::~NavMeshPOV() {
	delete pDuplicates;
}

float PathingRequest::GetActorRadius() {
	return fActorRadius;
}

bool RemoveIndex::operator==(const RemoveIndex& aIndex) const {
	return aIndex.iMasterIndex == iMasterIndex && aIndex.iMinorIndex == iMinorIndex;
}

// The exe's cleanup-only C++ EH frame is left out.
uint32_t PathingSolution::AddPathingNode(const PathingLocation& aLocation, const NiPoint3& aTangent) {
	uint32_t inewNodeIndex = CurrentPathingNodes.AddUninitialized();
	PathingNode* pnode = &CurrentPathingNodes.GetAt(inewNodeIndex);
	PathingNode* apMemory = static_cast<PathingNode*>(::operator new(60u, pnode));
	if (apMemory) {
		::new (apMemory) PathingNode(&aLocation, &aTangent);
	}
	return inewNodeIndex;
}
