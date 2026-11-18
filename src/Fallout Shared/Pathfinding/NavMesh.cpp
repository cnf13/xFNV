#include "Fallout Shared/Pathfinding/NavMesh.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshGrid.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

template <class T>
void BSClamp(T& aVal, T aMin, T aMax);

int32_t NavMeshTriangleEdgePortal::GetSaveSize() {
    return 6;
}

uint32_t EdgeExtraInfo::GetSizeOfV0() {
    return 0x2E;
}

void NavMeshPOVEdge::Set(const FastNavMeshEdgeHandle& aEdge) {
    EdgeIndex = (uint16_t)((((&aEdge)->iEdgeIndex & 3) << 14) | ((&aEdge)->usTriangle & 16383));
}

// The compiler builds the bases and the arrays; the obstacle map is allocated here, as the game does.
NavMesh::NavMesh()
  : pParentCell(nullptr), POVs(37), pTriangleToObstacleMap(new NiTMap<uint16_t, NiPointer<ObstacleData>>(37)),
    pNavMeshInfo(nullptr) {
    cFormType = NAVM_ID;
}

TESObjectCELL* NavMesh::GetSaveParentCell() {
    return pParentCell;
}

bool NavMesh::GetMatchingEdge(
    uint16_t ausTriangleIndex,
    int32_t aiEdgeIndex,
    NavMesh** arpReturnNavMesh,
    uint16_t* arOtherTriangleIndex,
    int32_t* arOtherEdgeIndex
) {
    if (ausTriangleIndex >= GetTriangleCount()) {
        return false;
    }
    NavMeshTriangle* pthisTri = GetTriangle(ausTriangleIndex);
    if (!pthisTri->IsEdgeConnected(aiEdgeIndex)) {
        return false;
    }
    if (pthisTri->EdgeHasExtraInfo(aiEdgeIndex)) {
        EdgeExtraInfo* pinfo = GetEdgeExtraInfo(ausTriangleIndex, aiEdgeIndex);
        if (pinfo->Portal.pOtherMesh) {
            NavMesh* potherMesh;
            if (pinfo->Portal.pOtherMesh->GetNavMesh(potherMesh)) {
                uint16_t otherTriangleIndex = pinfo->Portal.sTriangle;
                if (otherTriangleIndex < potherMesh->GetTriangleCount()) {
                    int32_t otherEdgeIndex;
                    NavMeshTriangle* potherTri = potherMesh->GetTriangle(otherTriangleIndex);
                    for (otherEdgeIndex = 0; otherEdgeIndex < 3; ++otherEdgeIndex) {
                        if (potherTri->EdgeHasExtraInfo(otherEdgeIndex)) {
                            EdgeExtraInfo* potherInfo =
                                potherMesh->GetEdgeExtraInfo(otherTriangleIndex, otherEdgeIndex);
                            if ((potherInfo) &&
                                (potherInfo->Portal.pOtherMesh == GetNavMeshInfo() &&
                                 potherInfo->Portal.sTriangle == ausTriangleIndex)) {
                                break;
                            }
                        }
                    }
                    if (otherEdgeIndex == 3) {
                        return false;
                    }

                    *arpReturnNavMesh = potherMesh;
                    *arOtherTriangleIndex = otherTriangleIndex;
                    *arOtherEdgeIndex = otherEdgeIndex;
                    return true;
                }

                return false;
            }

            return false;
        }

        return false;
    }

    int32_t otherEdgeIndex_2;
    *arpReturnNavMesh = this;
    *arOtherTriangleIndex = pthisTri->GetTriOrExtraInfo(aiEdgeIndex);
    NavMeshTriangle* potherTri_2 = GetTriangle(*arOtherTriangleIndex);
    for (otherEdgeIndex_2 = 0; otherEdgeIndex_2 < 3 &&
         (potherTri_2->EdgeHasExtraInfo(otherEdgeIndex_2) ||
          potherTri_2->GetTriOrExtraInfo(otherEdgeIndex_2) != ausTriangleIndex);
         ++otherEdgeIndex_2) {
        ;
    }
    if (otherEdgeIndex_2 < 3) {
        *arOtherEdgeIndex = otherEdgeIndex_2;
        return true;
    }

    return false;
}

TESObjectCELL* NavMesh::GetParentCell() {
    return pParentCell;
}

uint16_t NavMesh::AddTriangle(const NavMeshTriangle& aTriangle) {
    return Triangles.Add(aTriangle);
}

NavMeshInfo* NavMesh::GetNavMeshInfo() {
    return pNavMeshInfo;
}

void NavMesh::ClearNavMesh(bool abCheckout) {
    Vertices.Clear(true);
    Triangles.Clear(true);
    ExtraEdgeInfo.Clear(true);
    MeshGrid.Clear();
    CoverArray.Clear(true);
    Obstacles.Clear(true);
    DoorPortals.Clear(true);
    if (pNavMeshInfo) {
        uint32_t CastingType = iFormID;
        NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
        NavMeshInfoMap_->DeleteNavMeshInfo(CastingType);
    }
}

TESForm* NavMesh::GetParentSpace() {
    if (!pParentCell) {
        return nullptr;
    }
    if (pParentCell->GetWorldSpace()) {
        return pParentCell->GetWorldSpace();
    }
    return pParentCell;
}

bool NavMesh::CheckNavMesh(bool abFix) {
    NiPoint3 normal;
    bool bFoundProblem = false;
    BSStringT<char> cellName;
    TESObjectCELL* pCell = pParentCell;
    pCell->GetFormDetailedString(cellName);
    for (uint32_t i = 0; i < Vertices.iSize; ++i) {
        if (!std::isfinite((&Vertices.GetAt(static_cast<int32_t>(i)))->Location.x) ||
            std::fabs((&Vertices.GetAt(static_cast<int32_t>(i)))->Location.x) > 100000000376832.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, has bad vertex X coordinate, Please regenerate", iFormID,
                cellName.String()
            );
            bFoundProblem = true;
            break;
        }
        if (!std::isfinite((&Vertices.GetAt(static_cast<int32_t>(i)))->Location.y) ||
            std::fabs((&Vertices.GetAt(static_cast<int32_t>(i)))->Location.y) > 100000000376832.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, has bad vertex Y coordinate, Please regenerate", iFormID,
                cellName.String()
            );
            bFoundProblem = true;
            break;
        }
        if (!std::isfinite((&Vertices.GetAt(static_cast<int32_t>(i)))->Location.z) ||
            std::fabs((&Vertices.GetAt(static_cast<int32_t>(i)))->Location.z) > 100000000376832.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, has bad vertex Z coordinate, Please regenerate", iFormID,
                cellName.String()
            );
            bFoundProblem = true;
            break;
        }
    }
    if (Triangles.iSize >= 2000) {
        uint32_t a1 = Triangles.iSize;
        Warning(
            "PATHFINDING: Navmesh %08x Cell %s, has a large number of triangles, %d, please check and optimize",
            iFormID, cellName.String(), a1
        );
    }
    for (uint32_t i_2 = 0;; ++i_2) {
        uint32_t FormFlags = Triangles.iSize;
        if (i_2 >= FormFlags) {
            break;
        }
        NavMeshTriangle* ptri = &Triangles.GetAt(static_cast<int32_t>(i_2));
        for (int32_t e = 0; e < 3; ++e) {
            if (!ptri->EdgeHasExtraInfo(e) && ptri->IsEdgeConnected(e)) {
                uint32_t TriOrExtraInfo = ptri->GetTriOrExtraInfo(e);
                if (TriOrExtraInfo >= Triangles.iSize) {
                    Warning(
                        "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has bad Triangle index, Clearing "
                        "the connection",
                        iFormID, cellName.String(), i_2, e
                    );
                    ptri->SetTri(static_cast<uint32_t>(e), 0xFFFF);
                    bFoundProblem = true;
                }
            }
        }
        GetNormal(&normal, i_2);
        if (normal.z < 0.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, has a downfacing normal, flipping the triangle",
                iFormID, cellName.String(), i_2
            );
            FlipTriangle(i_2);
            bFoundProblem = true;
        }
    }
    for (uint32_t i_3 = 0;; ++i_3) {
        uint32_t v4 = Triangles.iSize;
        if (i_3 >= v4) {
            break;
        }
        NavMeshTriangle* ptri_2 = GetTriangle(i_3);
        for (int32_t e_2 = 0; e_2 < 3; ++e_2) {
            if (ptri_2->EdgeHasExtraInfo(e_2)) {
                uint32_t v5 = ptri_2->GetTriOrExtraInfo(e_2);
                if (v5 < ExtraEdgeInfo.iSize) {
                    EdgeExtraInfo* pinfo = GetEdgeExtraInfo(i_3, e_2);
                    if (!pinfo->Portal.pOtherMesh || pinfo->Portal.sTriangle == 0xFFFF) {
                        uint16_t v6 = ptri_2->GetTriOrExtraInfo(e_2);
                        DeleteExtraEdgeInfo(v6);
                        bFoundProblem = true;
                    }
                    else {
                        uint32_t sTriangle;
                        NavMeshPtr spotherNavMesh;
                        if (pinfo->Portal.pOtherMesh->GetNavMeshEvenDisabled(spotherNavMesh) &&
                            (sTriangle = pinfo->Portal.sTriangle,
                             sTriangle >= spotherNavMesh.m_pObject->GetTriangleCount())) {
                            int32_t a1a = pinfo->Portal.sTriangle;
                            int32_t X_4 = spotherNavMesh.m_pObject->iFormID;
                            Warning(
                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has bad Portal (Navmesh "
                                "%08x does not have a triangle index %d), Clearing the portal",
                                iFormID, cellName.String(), i_3, e_2, X_4, a1a
                            );
                            uint16_t v8 = ptri_2->GetTriOrExtraInfo(e_2);
                            DeleteExtraEdgeInfo(v8);
                            bFoundProblem = true;
                        }
                        else {
                            TESWorldSpace* WorldSpace = pParentCell->GetWorldSpace();
                            if (WorldSpace == pinfo->Portal.pOtherMesh->GetWorldSpace()) {}
                            else {
                                int32_t a1b = pinfo->Portal.pOtherMesh->NavMeshID;
                                Warning(
                                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has a Portal to a "
                                    "Navmesh (%08x) in a different Worldspace, Clearing the portal",
                                    iFormID, cellName.String(), i_3, e_2, a1b
                                );
                                uint16_t v10 = ptri_2->GetTriOrExtraInfo(e_2);
                                DeleteExtraEdgeInfo(v10);
                                bFoundProblem = true;
                            }
                        }
                    }
                }
                else {
                    Warning(
                        "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has bad extra info index, "
                        "Clearing extra info flag",
                        iFormID, cellName.String(), i_3, e_2
                    );
                    ptri_2->ClearEdgeExtraInfo(e_2);
                    bFoundProblem = true;
                }
            }
        }
    }
    for (uint32_t i_4 = 0; i_4 < Triangles.iSize; ++i_4) {
        NavMeshTriangle* ptri_3 = GetTriangle(i_4);
        int32_t Vert = ptri_3->GetVert(0);
        if (Vert == ptri_3->GetVert(1u)) {
            int32_t a1c = ptri_3->GetVert(0);
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d is degenerate, Vertices 0 and 1 both use vertex "
                "index %d",
                iFormID, cellName.String(), i_4, a1c
            );
            bFoundProblem = true;
        }
        int32_t v12 = ptri_3->GetVert(0);
        if (v12 == ptri_3->GetVert(2u)) {
            int32_t a1d = ptri_3->GetVert(0);
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d is degenerate, Vertices 0 and 2 both use vertex "
                "index %d",
                iFormID, cellName.String(), i_4, a1d
            );
            bFoundProblem = true;
        }
        int32_t v13 = ptri_3->GetVert(1u);
        if (v13 == ptri_3->GetVert(2u)) {
            int32_t a1e = ptri_3->GetVert(1u);
            Warning(
                "PATHFIDING: Navmesh %08x Cell %s, Triangle %d is degenerate, Vertices 1 and 2 both use vertex "
                "index %d",
                iFormID, cellName.String(), i_4, a1e
            );
            bFoundProblem = true;
        }
    }
    for (uint32_t i_5 = 0; i_5 < Triangles.iSize; ++i_5) {
        NavMeshTriangle* ptri_4 = GetTriangle(i_5);
        int32_t v14 = ptri_4->GetTriOrExtraInfo(0);
        if (v14 == ptri_4->GetTriOrExtraInfo(1) && ptri_4->GetTriOrExtraInfo(0) != 0xFFFF) {
            int32_t HasExtraInfo = ptri_4->EdgeHasExtraInfo(0);
            if (HasExtraInfo == ptri_4->EdgeHasExtraInfo(1)) {
                int32_t a1f = ptri_4->GetTriOrExtraInfo(0);
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edges 0 and 1 both point to the same triangle "
                    "%d",
                    iFormID, cellName.String(), i_5, a1f
                );
                bFoundProblem = true;
            }
        }
        int32_t v16 = ptri_4->GetTriOrExtraInfo(0);
        if (v16 == ptri_4->GetTriOrExtraInfo(2) && ptri_4->GetTriOrExtraInfo(0) != 0xFFFF) {
            int32_t v17 = ptri_4->EdgeHasExtraInfo(0);
            if (v17 == ptri_4->EdgeHasExtraInfo(2)) {
                int32_t a1g = ptri_4->GetTriOrExtraInfo(0);
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edges 0 and 2 both point to the same triangle "
                    "%d",
                    iFormID, cellName.String(), i_5, a1g
                );
                bFoundProblem = true;
            }
        }
        int32_t v18 = ptri_4->GetTriOrExtraInfo(1);
        if (v18 == ptri_4->GetTriOrExtraInfo(2) && ptri_4->GetTriOrExtraInfo(1) != 0xFFFF) {
            int32_t v19 = ptri_4->EdgeHasExtraInfo(1);
            if (v19 == ptri_4->EdgeHasExtraInfo(2)) {
                int32_t a1h = ptri_4->GetTriOrExtraInfo(1);
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edges 1 and 2 both point to the same triangle "
                    "%d",
                    iFormID, cellName.String(), i_5, a1h
                );
                bFoundProblem = true;
            }
        }
    }
    for (uint32_t i_6 = 0; i_6 < Triangles.iSize; ++i_6) {
        NavMeshTriangle* ptri_5 = GetTriangle(i_6);
        for (int32_t edge1index = 0; edge1index < 3; ++edge1index) {
            int32_t edge2index = edge1index + 1;
            if (edge1index == 2) {
                edge2index = 0;
            }
            uint16_t vert1 = ptri_5->GetVert(static_cast<uint32_t>(edge1index));
            uint16_t vert2 = ptri_5->GetVert(static_cast<uint32_t>(edge2index));
            for (uint32_t j = 0; j < Triangles.iSize; ++j) {
                if (j != i_6) {
                    NavMeshTriangle* ppotentialTri = GetTriangle(j);
                    for (int32_t f = 0; f < 3; ++f) {
                        if (ppotentialTri->GetVert(static_cast<uint32_t>(f)) == vert2) {
                            uint16_t gshouldbe = f + 1;
                            if (static_cast<uint16_t>(f + 1) == 3) {
                                gshouldbe = 0;
                            }
                            for (int32_t g = 0; g < 3; ++g) {
                                if (g != f && ppotentialTri->GetVert(static_cast<uint32_t>(g)) == vert1) {
                                    if (g != gshouldbe) {
                                        Warning(
                                            "PATHFINDING: Navmesh %08x Cell %s, Triangle %d and %d have opposite "
                                            "normals but are linked",
                                            iFormID, cellName.String(), i_6, j
                                        );
                                        bFoundProblem = true;
                                    }
                                    if (ptri_5->EdgeHasExtraInfo(edge1index)) {
                                        if (!ppotentialTri->EdgeHasExtraInfo(f)) {
                                            Warning(
                                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d Should "
                                                "have Portal information, but doesn't",
                                                iFormID, cellName.String(), j, f
                                            );
                                            bFoundProblem = true;
                                        }
                                    }
                                    else {
                                        if (ptri_5->GetTriOrExtraInfo(edge1index) != 0xFFFF) {
                                            if (ppotentialTri->EdgeHasExtraInfo(f)) {
                                                Warning(
                                                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d has a "
                                                    "portal data to Triangle %d, but Triangle %d doesn't have any "
                                                    "Portal info for its matching edge %d",
                                                    iFormID, cellName.String(), i_6, edge1index, j, j, f
                                                );
                                                bFoundProblem = true;
                                            }
                                            if (ptri_5->GetTriOrExtraInfo(edge1index) != j) {
                                                Warning(
                                                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d Should "
                                                    "have a link to Triangle %d, but doesn't",
                                                    iFormID, cellName.String(), i_6, edge1index, j
                                                );
                                                bFoundProblem = true;
                                            }
                                            if (ppotentialTri->GetTriOrExtraInfo(f) != i_6) {
                                                Warning(
                                                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d Should "
                                                    "have a link to Triangle %d, but doesn't",
                                                    iFormID, cellName.String(), j, f, i_6
                                                );
                                                bFoundProblem = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for (uint32_t i_7 = 0; i_7 < Triangles.iSize; ++i_7) {
        NavMeshTriangle* ptri_6 = GetTriangle(i_7);
        for (int32_t triedgevert1 = 0; triedgevert1 < 3; ++triedgevert1) {
            if (!ptri_6->EdgeHasExtraInfo(triedgevert1) && ptri_6->GetTriOrExtraInfo(triedgevert1) != 0xFFFF) {
                int32_t othertriedgevert1;
                uint16_t v20 = ptri_6->GetTriOrExtraInfo(triedgevert1);
                NavMeshTriangle* potherTri = GetTriangle(v20);
                char v45 = 0;
                for (othertriedgevert1 = 0; othertriedgevert1 < 3; ++othertriedgevert1) {
                    if (!potherTri->EdgeHasExtraInfo(othertriedgevert1) &&
                        (potherTri->GetTriOrExtraInfo(othertriedgevert1) != 0xFFFF &&
                         potherTri->GetTriOrExtraInfo(othertriedgevert1) == i_7)) {
                        v45 = 1;
                        break;
                    }
                }
                if (!v45) {
                    int32_t a1i = ptri_6->GetTriOrExtraInfo(triedgevert1);
                    int32_t X_4a = ptri_6->GetTriOrExtraInfo(triedgevert1);
                    Warning(
                        "PATHFINDING: Navmesh %08x Cell %s, Triangle %d has a link to Triangle %d, but Triangle %d "
                        "doesn't have the matching link",
                        iFormID, cellName.String(), i_7, X_4a, a1i
                    );
                    bFoundProblem = true;
                }
                if (othertriedgevert1 < 3) {
                    int32_t v22;
                    int32_t triedgevert2 = triedgevert1 + 1;
                    if (triedgevert1 == 2) {
                        triedgevert2 = 0;
                    }
                    int32_t othertriedgevert2 = othertriedgevert1 + 1;
                    if (othertriedgevert1 == 2) {
                        othertriedgevert2 = 0;
                    }
                    int32_t v21 = ptri_6->GetVert(static_cast<uint32_t>(triedgevert1));
                    if (v21 != potherTri->GetVert(static_cast<uint32_t>(othertriedgevert2)) ||
                        (v22 = ptri_6->GetVert(static_cast<uint32_t>(triedgevert2)),
                         v22 != potherTri->GetVert(static_cast<uint32_t>(othertriedgevert1)))) {
                        int32_t a1j = ptri_6->GetTriOrExtraInfo(triedgevert1);
                        Warning(
                            "PATHFINDING: Navmesh %08x Cell %s, Triangles %d and %d are linked, but their vertices "
                            "do not match",
                            iFormID, cellName.String(), i_7, a1j
                        );
                        bFoundProblem = true;
                    }
                }
            }
        }
    }
    for (uint32_t i_8 = 0; i_8 < CoverArray.iSize; ++i_8) {
        uint16_t icoverTri = CoverArray.GetAt(static_cast<int32_t>(i_8));
        NavMeshTriangle* ptri_7 = GetTriangle(icoverTri);
        if (ptri_7) {
            if (!ptri_7->HasCover()) {
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Cover Array (index %d) referes to triangle %d, which does "
                    "not have cover.",
                    iFormID, cellName.String(), i_8, icoverTri
                );
                bFoundProblem = true;
            }
        }
        else {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Cover Array (index %d) referes to triangle %d, which does not "
                "exist.",
                iFormID, cellName.String(), i_8, icoverTri
            );
            bFoundProblem = true;
        }
    }
    CheckNavMeshAgainstNeighbors();
    bool v39 = !bFoundProblem;
    return v39;
}

void NavMesh::DeleteExtraEdgeInfo(uint16_t aiExtraInfo) {
    ExtraEdgeInfo.RemoveFast(aiExtraInfo, 1u);
    uint32_t FormFlags = Triangles.iSize;
    for (uint32_t a2 = 0; a2 < FormFlags; ++a2) {
        if ((&Triangles.GetAt(static_cast<int32_t>(a2)))->HasEdgeExtraInfo()) {
            for (uint32_t t = 0; t < 3; ++t) {
                if ((&Triangles.GetAt(static_cast<int32_t>(a2)))->EdgeHasExtraInfo(static_cast<int32_t>(t))) {
                    if ((&Triangles.GetAt(static_cast<int32_t>(a2)))->GetTriOrExtraInfo(static_cast<int32_t>(t)) ==
                        aiExtraInfo) {
                        (&Triangles.GetAt(static_cast<int32_t>(a2)))->ClearEdgeExtraInfo(static_cast<int32_t>(t));
                    }
                    else {
                        int32_t TriOrExtraInfo =
                            (&Triangles.GetAt(static_cast<int32_t>(a2)))->GetTriOrExtraInfo(static_cast<int32_t>(t));
                        if (TriOrExtraInfo == ExtraEdgeInfo.iSize) {
                            (&Triangles.GetAt(static_cast<int32_t>(a2)))
                                ->SetEdgeExtraInfo(static_cast<int32_t>(t), aiExtraInfo);
                        }
                    }
                }
            }
        }
    }
}

void NavMesh::CreateCoverArray() {
    CoverArray.Clear(false);
    uint32_t uiCount = Triangles.GetSize();
    for (uint32_t i = 0; i < uiCount; ++i) {
        if ((!GetTriangle(i)->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED)) && (GetTriangle(i)->HasCover())) {
            uint16_t usTriangle = i;
            CoverArray.Add(usTriangle);
        }
    }
}

bool NavMesh::GetMatchingEdge(const NavMeshEdgeHandle& aThisEdge, NavMeshEdgeHandle* apRetOtherEdge) {
    NavMesh* arpReturnNavMesh;
    int32_t arOtherEdgeIndex;
    uint16_t arOtherTriangleIndex;
    arpReturnNavMesh = nullptr;
    arOtherEdgeIndex = -1;
    if (!GetMatchingEdge(
            aThisEdge.usTriangle, aThisEdge.iEdgeIndex, &arpReturnNavMesh, &arOtherTriangleIndex, &arOtherEdgeIndex
        )) {
        return false;
    }
    apRetOtherEdge->pNavMeshInfo = arpReturnNavMesh->GetNavMeshInfo();
    apRetOtherEdge->usTriangle = arOtherTriangleIndex;
    apRetOtherEdge->iEdgeIndex = arOtherEdgeIndex;
    return true;
}

EdgeExtraInfo* NavMesh::GetEdgeExtraInfo(uint16_t ausTriangleIndex, int32_t aiEdgeIndex) {
    NavMeshTriangle* v3 = &(&Triangles)->GetAt(ausTriangleIndex);
    if (!v3->EdgeHasExtraInfo(aiEdgeIndex)) {
        return nullptr;
    }
    NavMeshTriangle* v4 = &(&Triangles)->GetAt(ausTriangleIndex);
    if (v4->GetTriOrExtraInfo(aiEdgeIndex) == 65535) {
        return nullptr;
    }
    NavMeshTriangle* v5 = &(&Triangles)->GetAt(ausTriangleIndex);
    uint16_t triOrExtraInfo = v5->GetTriOrExtraInfo(aiEdgeIndex);
    return &(&ExtraEdgeInfo)->GetAt(triOrExtraInfo);
}

uint16_t NavMesh::AddExtraEdgeInfo(const EdgeExtraInfo& aExtraInfo) {
    return ExtraEdgeInfo.Add(aExtraInfo);
}

uint16_t NavMesh::AddVertex(const NavMeshVertex& aVertex) {
    return Vertices.Add(aVertex);
}

bool NavMesh::GetMatchingEdge(const FastNavMeshEdgeHandle& aThisEdge, FastNavMeshEdgeHandle* apRetOtherEdge) {
    return GetMatchingEdge(
        aThisEdge.usTriangle, aThisEdge.iEdgeIndex, &apRetOtherEdge->pNavMesh, &apRetOtherEdge->usTriangle,
        &apRetOtherEdge->iEdgeIndex
    );
}

NavMeshTriangleDoorPortal* NavMesh::GetDoorPortalFromArray(uint16_t aiDoorPortalIndex) {
    return &DoorPortals[aiDoorPortalIndex];
}

// Reads the grid size twice, as the game does.
uint32_t NavMesh::GetGridCellCount() {
    uint32_t uiSize = MeshGrid.GetGridSize();
    return MeshGrid.GetGridSize() * uiSize;
}

NavMeshStaticAvoidNode* NavMesh::GetStaticAvoidNode(uint32_t aiIndex) {
    return &StaticAvoidNodes.GetAt(aiIndex);
}

// The node is built on the stack and copied into StaticAvoidNodes.
void NavMesh::AddStaticAvoidNode(const NiPoint3& aLocation, uint16_t ausTri, float afRadius, float afCost) {
    NavMeshStaticAvoidNode kNode;
    kNode.~NavMeshStaticAvoidNode();
    ::new (&kNode) NavMeshStaticAvoidNode(aLocation, ausTri, afRadius, afCost);
    Triangles.GetAt(ausTri).SetFlag(static_cast<int32_t>(NavMeshTriangle::NAV_TRI_AVOID_NODE));
    StaticAvoidNodes.Add(kNode);
}

void NavMesh::SetNavMeshInfo(NavMeshInfo* apInfo) {
    pNavMeshInfo = apInfo;
}

// West and east the vertical edges at x = iX * 4096 and (iX + 1) * 4096, south and north the horizontal ones; every
// navmesh of each neighbouring cell is checked.
bool NavMesh::CheckNavMeshAgainstNeighbors() {
    if (GetParentCell()->GetInterior()) {
        return true;
    }
    bool bResult = true;
    int32_t iX = GetParentCell()->GetDataX();
    int32_t iY = GetParentCell()->GetDataY();

    float fEdgeX = static_cast<float>(iX << 12);
    TESObjectCELL* pWestCell = GetParentCell()->GetWorldSpace()->GetCellFromCellCoord(iX - 1, iY);
    if (pWestCell && pWestCell->GetNavMeshArray()) {
        for (uint32_t i = 0; i < pWestCell->GetNavMeshArray()->GetNavMeshCount(); ++i) {
            NavMeshPtr spNavMesh;
            pWestCell->GetNavMeshArray()->GetNavMeshByIndex(&spNavMesh, i);
            NavMesh* pNavMesh = static_cast<NavMesh*>(spNavMesh);
            if (!CheckUnconnectedBorderEdges(this, pNavMesh, true, fEdgeX, 0.0f)) {
                bResult = false;
            }
        }
    }

    fEdgeX = static_cast<float>(static_cast<double>(iX << 12) + 4096.0);
    TESObjectCELL* pEastCell = GetParentCell()->GetWorldSpace()->GetCellFromCellCoord(iX + 1, iY);
    if (pEastCell && pEastCell->GetNavMeshArray()) {
        for (uint32_t i = 0; i < pEastCell->GetNavMeshArray()->GetNavMeshCount(); ++i) {
            NavMeshPtr spNavMesh;
            pEastCell->GetNavMeshArray()->GetNavMeshByIndex(&spNavMesh, i);
            NavMesh* pNavMesh = static_cast<NavMesh*>(spNavMesh);
            if (!CheckUnconnectedBorderEdges(this, pNavMesh, true, fEdgeX, 0.0f)) {
                bResult = false;
            }
        }
    }

    float fEdgeY = static_cast<float>(iY << 12);
    TESObjectCELL* pSouthCell = GetParentCell()->GetWorldSpace()->GetCellFromCellCoord(iX, iY - 1);
    if (pSouthCell && pSouthCell->GetNavMeshArray()) {
        for (uint32_t i = 0; i < pSouthCell->GetNavMeshArray()->GetNavMeshCount(); ++i) {
            NavMeshPtr spNavMesh;
            pSouthCell->GetNavMeshArray()->GetNavMeshByIndex(&spNavMesh, i);
            NavMesh* pNavMesh = static_cast<NavMesh*>(spNavMesh);
            if (!CheckUnconnectedBorderEdges(this, pNavMesh, false, 0.0f, fEdgeY)) {
                bResult = false;
            }
        }
    }

    fEdgeY = static_cast<float>(static_cast<double>(iY << 12) + 4096.0);
    TESObjectCELL* pNorthCell = GetParentCell()->GetWorldSpace()->GetCellFromCellCoord(iX, iY + 1);
    if (pNorthCell && pNorthCell->GetNavMeshArray()) {
        for (uint32_t i = 0; i < pNorthCell->GetNavMeshArray()->GetNavMeshCount(); ++i) {
            NavMeshPtr spNavMesh;
            pNorthCell->GetNavMeshArray()->GetNavMeshByIndex(&spNavMesh, i);
            NavMesh* pNavMesh = static_cast<NavMesh*>(spNavMesh);
            if (!CheckUnconnectedBorderEdges(this, pNavMesh, false, 0.0f, fEdgeY)) {
                bResult = false;
            }
        }
    }
    return bResult;
}

NavMeshVertex* NavMesh::GetVertex(uint16_t asVertexIndex) {
    return &Vertices.GetAt(asVertexIndex);
}

NavMeshTriangle* NavMesh::GetTriangle(uint16_t asTriangleIndex) {
    return &Triangles.GetAt(asTriangleIndex);
}

NavMeshEdge NavMesh::GetEdge(uint16_t ausTriIndex, int32_t aiEdgeIndex) {
    uint16_t usFirst = GetTriangle(ausTriIndex)->GetVert(aiEdgeIndex);
    uint16_t usSecond = GetTriangle(ausTriIndex)->GetVert((aiEdgeIndex + 1) % 3);
    NavMeshEdge kEdge;
    kEdge.Vertices[0] = usFirst;
    kEdge.Vertices[1] = usSecond;
    return kEdge;
}

NavMeshEdgeVertices NavMesh::GetEdgeVertices(uint16_t ausTriIndex, int32_t aiEdgeIndex) {
    NavMeshEdge kEdge = GetEdge(ausTriIndex, aiEdgeIndex);
    NavMeshVertex* pFirst = GetVertex(kEdge.Vertices[0]);
    NavMeshVertex* pSecond = GetVertex(kEdge.Vertices[1]);
    NavMeshEdgeVertices kVertices;
    kVertices.Vertices[0] = pFirst;
    kVertices.Vertices[1] = pSecond;
    return kVertices;
}

// Looks the triangle up again for the second vertex and copies each location out whole.
bool NavMesh::GetEdgeVertices(uint16_t ausTriIndex, int32_t aiEdgeIndex, NiPoint3* apRetVert1, NiPoint3* apRetVert2) {
    uint16_t usVert1 = GetTriangle(ausTriIndex)->GetVert(aiEdgeIndex);
    uint16_t usVert2 = GetTriangle(ausTriIndex)->GetVert((aiEdgeIndex + 1) % 3);
    *apRetVert1 = GetVertex(usVert1)->Location;
    *apRetVert2 = GetVertex(usVert2)->Location;
    return true;
}

uint32_t NavMesh::GetVertexCount() const {
    return (&Vertices)->BSSimpleArray<NavMeshVertex>::GetSize();
}

uint32_t NavMesh::GetNumDoorPortals() {
    return (&DoorPortals)->BSSimpleArray<NavMeshTriangleDoorPortal>::GetSize();
}

uint32_t NavMesh::GetStaticAvoidNodeCount() {
    return (&StaticAvoidNodes)->iSize;
}

void NavMesh::GetTriangleVertices(
    uint16_t ausTriIndex,
    const NavMeshVertex*& apV0,
    const NavMeshVertex*& apV1,
    const NavMeshVertex*& apV2
) {
    NavMeshTriangle* Triangle = GetTriangle(ausTriIndex);
    uint16_t Vert = Triangle->GetVert(0);
    apV0 = GetVertex(Vert);
    uint16_t v6 = Triangle->GetVert(1u);
    apV1 = GetVertex(v6);
    uint16_t v7 = Triangle->GetVert(2u);
    apV2 = GetVertex(v7);
}

bool NavMesh::GetMatchingEdge_0(
    uint16_t ausTriangleIndex,
    int32_t aiEdgeIndex,
    NavMeshPtr* apReturnNavMesh,
    uint16_t* apOtherTriangleIndex,
    int32_t* apOtherEdgeIndex
) {
    XFNV_INLINED_LOCAL(NavMeshPtr, apResult, std::nullopt, std::nullopt);
    NavMesh* pOtherNavMesh;
    pOtherNavMesh = nullptr;
    if (!GetMatchingEdge(ausTriangleIndex, aiEdgeIndex, &pOtherNavMesh, apOtherTriangleIndex, apOtherEdgeIndex)) {
        return false;
    }
    NavMeshPtr* ptr = NavMeshPtr::MakeNavMeshPtr(&apResult, pOtherNavMesh);
    apReturnNavMesh->operator=(*ptr);
    XFNV_DESTRUCT(NiPointer<NavMesh>)(&apResult);
    return true;
}

const BSSimpleArray<uint16_t, 1024>* NavMesh::GetTrianglesForGridCell(uint32_t aiGridCellIndex) {
    return MeshGrid.GetTrianglesForGridCell(aiGridCellIndex);
}

uint32_t NavMesh::GetGridIndicesForTriangle(uint16_t ausTriangleIndex, BSSimpleArray<uint32_t, 1024>* apGridCells) {
    uint32_t icol;
    uint32_t irow;
    const NavMeshTriangle* ptri = &Triangles.GetAt(static_cast<int32_t>(ausTriangleIndex));
    apGridCells->Clear(false);
    uint32_t imaxRow = 0;
    uint32_t iminRow = -1;
    uint32_t imaxCol = 0;
    uint32_t iminCol = -1;
    for (uint32_t i = 0; i < 3; ++i) {
        uint16_t Vert = ptri->GetVert(i);
        const NavMeshVertex* pvert = GetVertex(Vert);
        irow = 0;
        icol = 0;
        MeshGrid.GetGridRowAndColumn(pvert->Location, &irow, &icol);
        imaxRow = BSMAX(imaxRow, irow);
        iminRow = BSMIN(iminRow, irow);
        imaxCol = BSMAX(imaxCol, icol);
        iminCol = BSMIN(iminCol, icol);
    }
    for (uint32_t i_2 = iminRow; i_2 < imaxRow + 1; ++i_2) {
        for (uint32_t j = iminCol; j < imaxCol + 1; ++j) {
            if (MeshGrid.IsTriangleInGridCell(ausTriangleIndex, j, i_2)) {
                uint32_t GridSize = MeshGrid.iGridSize;
                uint32_t a2 = j + i_2 * GridSize;
                apGridCells->Add(a2);
            }
        }
    }
    return apGridCells->iSize;
}

int32_t
NavMesh::GetStaticAvoidNodesForTri(uint16_t ausTriangle, BSSimpleArray<NavMeshStaticAvoidNode*, 1024>& aRetArray) {
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = StaticAvoidNodes.iSize;
        if (i >= FormFlags) {
            break;
        }
        if ((&StaticAvoidNodes.GetAt(static_cast<int32_t>(i)))->usTriangle == ausTriangle) {
            NavMeshStaticAvoidNode* a2 = &StaticAvoidNodes.GetAt(static_cast<int32_t>(i));
            aRetArray.Add(a2);
        }
    }
    return !aRetArray.IsEmpty();
}

uint32_t NavMesh::GetGridIndex(const NiPoint3& arLoc) {
    return MeshGrid.GetGridIndex(arLoc);
}

bool NavMesh::CheckUnconnectedBorderEdges(
    NavMesh* apMesh1,
    NavMesh* apMesh2,
    bool abVerticalEdge,
    float afEdgeX,
    float afEdgeY
) {
    return true;
}

bool NavMesh::SetDisableNavMesh(uint32_t aiID, bool abDisable) {
    XFNV_INLINED_LOCAL(NiPointer<NavMesh>, apMemory, std::nullopt, std::nullopt);
    NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
    NavMeshInfo* NavMeshInfo_ = NavMeshInfoMap_->GetNavMeshInfo(aiID);
    if (NavMeshInfo_) {
        ::new (&apMemory) NavMeshPtr;
        if (NavMeshInfo_->GetNavMeshEvenDisabled(*static_cast<NavMeshPtr*>(&apMemory))) {
            TESForm* v4 = apMemory.m_pObject;
            if (v4->GetDisabled() != abDisable) {
                TESForm* v5 = apMemory.m_pObject;
                v5->SetDisabled(abDisable);
                NavMeshInfo_->SetDisabled(abDisable);
                if (!abDisable) {
                    NavMesh* v6 = apMemory.m_pObject;
                    v6->ReconnectEnabledPortals();
                }
            }
            apMemory.~NiPointer<NavMesh>();
            return true;
        }

        Warning("DisableNavMesh Script Function: Did not find matching NavMesh for NavMesh ID parameter %08X", aiID);
        apMemory.~NiPointer<NavMesh>();
        return false;
    }

    Warning("DisableNavMesh Script Function: Did not find matching NavMeshInfo for NavMesh ID parameter %08X", aiID);
    return false;
}

// Rounds deltas and ratios to float, then clamps the column before the row.
void NavMeshGrid::GetGridRowAndColumn(const NiPoint3& aLoc, uint32_t* apRetRow, uint32_t* apRetCol) {
    float fColumnDelta = aLoc.x - GridBoundsMin.x;
    float fRowDelta = aLoc.y - GridBoundsMin.y;
    float fColumn = fColumnDelta / fColumnSectionLen;
    float fRow = fRowDelta / fRowSectionLen;
    int32_t iColumn = static_cast<int32_t>(std::floorf(fColumn));
    BSClamp<int32_t>(iColumn, 0, static_cast<int32_t>(iGridSize - 1));
    *apRetCol = iColumn;
    int32_t iRow = static_cast<int32_t>(std::floorf(fRow));
    BSClamp<int32_t>(iRow, 0, static_cast<int32_t>(iGridSize - 1));
    *apRetRow = iRow;
}

// GridData is a row-major array of 16-byte cell objects.
BSSimpleArray<uint16_t>* NavMeshGrid::GetTriangleArray(uint32_t aiColumn, uint32_t aiRow) {
    return &GridData[aiColumn + iGridSize * aiRow];
}

uint32_t NavMeshGrid::GetGridIndex(const NiPoint3& aLoc) {
    int32_t apRetCol;
    int32_t apRetRow;
    apRetRow = 0;
    apRetCol = 0;
    GetGridRowAndColumn(aLoc, reinterpret_cast<uint32_t*>(&apRetRow), reinterpret_cast<uint32_t*>(&apRetCol));
    return apRetCol + iGridSize * apRetRow;
}

bool NavMeshInfo::GetNavMesh(NavMeshPtr& aReturnNavMeshPtr) const {
    if (GetDisabled()) {
        return false;
    }
    NavMeshPtr::MakeNavMeshPtr(aReturnNavMeshPtr, pNavMesh);
    return aReturnNavMeshPtr.operator->() != nullptr;
}

TESForm* NavMeshInfo::GetParentSpace() const {
    if (pParentSpace != nullptr) {
        return pParentSpace;
    }
    return TESForm::GetFormByNumericID(ParentSpaceID);
}

bool NavMeshInfo::GetDisabled() const {
    return (uiFlags & 0x10) != 0;
}

bool NavMeshInfo::GetNavMeshEvenDisabled(NavMeshPtr& aReturnNavMeshPtr) {
    NavMeshPtr::MakeNavMeshPtr(aReturnNavMeshPtr, pNavMesh);
    return pNavMesh != nullptr;
}

// Reads pNavMesh again for the result after storing it through arpReturnNavMesh.
bool NavMeshInfo::GetNavMesh(NavMesh*& arpReturnNavMesh) const {
    if (GetDisabled()) {
        arpReturnNavMesh = nullptr;
        return false;
    }
    arpReturnNavMesh = pNavMesh;
    return pNavMesh != nullptr;
}

NavMeshStaticAvoidNode::NavMeshStaticAvoidNode() : usTriangle(0xFFFF) {}

NavMeshStaticAvoidNode::NavMeshStaticAvoidNode(
    const NiPoint3& aCenter,
    uint16_t ausTriangle,
    float afRadius,
    float afCost
)
  : PathingAvoidNode(aCenter, afRadius, afCost), usTriangle(ausTriangle) {}
