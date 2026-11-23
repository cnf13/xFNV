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
    EdgeIndex = static_cast<uint16_t>(((aEdge.iEdgeIndex & 3) << 14) | (aEdge.usTriangle & 16383));
}

// The compiler builds the bases and the arrays; the obstacle map is allocated here.
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
                            EdgeExtraInfo* potherInfo = potherMesh->GetEdgeExtraInfo(
                                otherTriangleIndex,
                                otherEdgeIndex
                            );
                            if (potherInfo &&
                                potherInfo->Portal.pOtherMesh == GetNavMeshInfo() &&
                                potherInfo->Portal.sTriangle == ausTriangleIndex) {
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

    int32_t otherEdgeIndex;
    *arpReturnNavMesh = this;
    *arOtherTriangleIndex = pthisTri->GetTriOrExtraInfo(aiEdgeIndex);
    NavMeshTriangle* potherTri = GetTriangle(*arOtherTriangleIndex);
    for (otherEdgeIndex = 0; otherEdgeIndex < 3 &&
         (potherTri->EdgeHasExtraInfo(otherEdgeIndex) ||
          potherTri->GetTriOrExtraInfo(otherEdgeIndex) != ausTriangleIndex);
         ++otherEdgeIndex) {
        ;
    }
    if (otherEdgeIndex < 3) {
        *arOtherEdgeIndex = otherEdgeIndex;
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
        uint32_t uiFormID = iFormID;
        TES::pInstance->GetNavMeshInfoMap()->DeleteNavMeshInfo(uiFormID);
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
    bool bFoundProblem = false;
    BSStringT<char> cellName;
    TESObjectCELL* pCell = pParentCell;
    pCell->GetFormDetailedString(cellName);
    for (const NavMeshVertex& rVertex : Vertices) {
        if (!std::isfinite(rVertex.Location.x) || std::fabs(rVertex.Location.x) > 100000000376832.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, has bad vertex X coordinate, Please regenerate",
                iFormID,
                cellName.String()
            );
            bFoundProblem = true;
            break;
        }
        if (!std::isfinite(rVertex.Location.y) || std::fabs(rVertex.Location.y) > 100000000376832.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, has bad vertex Y coordinate, Please regenerate",
                iFormID,
                cellName.String()
            );
            bFoundProblem = true;
            break;
        }
        if (!std::isfinite(rVertex.Location.z) || std::fabs(rVertex.Location.z) > 100000000376832.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, has bad vertex Z coordinate, Please regenerate",
                iFormID,
                cellName.String()
            );
            bFoundProblem = true;
            break;
        }
    }
    if (Triangles.iSize >= 2000) {
        uint32_t uiTriangleCount = Triangles.iSize;
        Warning(
            "PATHFINDING: Navmesh %08x Cell %s, has a large number of triangles, %d, please check and optimize",
            iFormID,
            cellName.String(),
            uiTriangleCount
        );
    }
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* ptri = &Triangles.GetAt(static_cast<int32_t>(i));
        for (int32_t e = 0; e < 3; ++e) {
            if (!ptri->EdgeHasExtraInfo(e) && ptri->IsEdgeConnected(e)) {
                uint32_t uiOtherTri = ptri->GetTriOrExtraInfo(e);
                if (uiOtherTri >= Triangles.iSize) {
                    Warning(
                        "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has bad Triangle index, Clearing "
                        "the connection",
                        iFormID,
                        cellName.String(),
                        i,
                        e
                    );
                    ptri->SetTri(static_cast<uint32_t>(e), 0xFFFF);
                    bFoundProblem = true;
                }
            }
        }
        NiPoint3 normal = GetNormal(i);
        if (normal.z < 0.0) {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, has a downfacing normal, flipping the triangle",
                iFormID,
                cellName.String(),
                i
            );
            FlipTriangle(i);
            bFoundProblem = true;
        }
    }
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        for (int32_t e = 0; e < 3; ++e) {
            if (ptri->EdgeHasExtraInfo(e)) {
                uint32_t uiExtraInfo = ptri->GetTriOrExtraInfo(e);
                if (uiExtraInfo < ExtraEdgeInfo.iSize) {
                    EdgeExtraInfo* pinfo = GetEdgeExtraInfo(i, e);
                    if (!pinfo->Portal.pOtherMesh || pinfo->Portal.sTriangle == 0xFFFF) {
                        DeleteExtraEdgeInfo(ptri->GetTriOrExtraInfo(e));
                        bFoundProblem = true;
                    }
                    else {
                        uint32_t uiOtherTriangle;
                        NavMeshPtr spotherNavMesh;
                        if (pinfo->Portal.pOtherMesh->GetNavMeshEvenDisabled(spotherNavMesh) &&
                            (uiOtherTriangle = pinfo->Portal.sTriangle,
                             uiOtherTriangle >= spotherNavMesh->GetTriangleCount())) {
                            int32_t iOtherTriangle = pinfo->Portal.sTriangle;
                            int32_t iOtherFormID = spotherNavMesh->iFormID;
                            Warning(
                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has bad Portal (Navmesh "
                                "%08x does not have a triangle index %d), Clearing the portal",
                                iFormID,
                                cellName.String(),
                                i,
                                e,
                                iOtherFormID,
                                iOtherTriangle
                            );
                            DeleteExtraEdgeInfo(ptri->GetTriOrExtraInfo(e));
                            bFoundProblem = true;
                        }
                        else {
                            TESWorldSpace* pWorldSpace = pParentCell->GetWorldSpace();
                            if (pWorldSpace != pinfo->Portal.pOtherMesh->GetWorldSpace()) {
                                int32_t iOtherNavMeshID = pinfo->Portal.pOtherMesh->NavMeshID;
                                Warning(
                                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has a Portal to a "
                                    "Navmesh (%08x) in a different Worldspace, Clearing the portal",
                                    iFormID,
                                    cellName.String(),
                                    i,
                                    e,
                                    iOtherNavMeshID
                                );
                                DeleteExtraEdgeInfo(ptri->GetTriOrExtraInfo(e));
                                bFoundProblem = true;
                            }
                        }
                    }
                }
                else {
                    Warning(
                        "PATHFINDING: Navmesh %08x Cell %s, Triangle %d, edge %d has bad extra info index, "
                        "Clearing extra info flag",
                        iFormID,
                        cellName.String(),
                        i,
                        e
                    );
                    ptri->ClearEdgeExtraInfo(e);
                    bFoundProblem = true;
                }
            }
        }
    }
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        int32_t iVert = ptri->GetVert(0);
        if (iVert == ptri->GetVert(1u)) {
            int32_t iSharedVert = ptri->GetVert(0);
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d is degenerate, Vertices 0 and 1 both use vertex "
                "index %d",
                iFormID,
                cellName.String(),
                i,
                iSharedVert
            );
            bFoundProblem = true;
        }
        iVert = ptri->GetVert(0);
        if (iVert == ptri->GetVert(2u)) {
            int32_t iSharedVert = ptri->GetVert(0);
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d is degenerate, Vertices 0 and 2 both use vertex "
                "index %d",
                iFormID,
                cellName.String(),
                i,
                iSharedVert
            );
            bFoundProblem = true;
        }
        iVert = ptri->GetVert(1u);
        if (iVert == ptri->GetVert(2u)) {
            int32_t iSharedVert = ptri->GetVert(1u);
            Warning(
                "PATHFIDING: Navmesh %08x Cell %s, Triangle %d is degenerate, Vertices 1 and 2 both use vertex "
                "index %d",
                iFormID,
                cellName.String(),
                i,
                iSharedVert
            );
            bFoundProblem = true;
        }
    }
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        int32_t iTri = ptri->GetTriOrExtraInfo(0);
        if (iTri == ptri->GetTriOrExtraInfo(1) && ptri->GetTriOrExtraInfo(0) != 0xFFFF) {
            int32_t iHasExtraInfo = ptri->EdgeHasExtraInfo(0);
            if (iHasExtraInfo == ptri->EdgeHasExtraInfo(1)) {
                int32_t iSharedTri = ptri->GetTriOrExtraInfo(0);
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edges 0 and 1 both point to the same triangle "
                    "%d",
                    iFormID,
                    cellName.String(),
                    i,
                    iSharedTri
                );
                bFoundProblem = true;
            }
        }
        iTri = ptri->GetTriOrExtraInfo(0);
        if (iTri == ptri->GetTriOrExtraInfo(2) && ptri->GetTriOrExtraInfo(0) != 0xFFFF) {
            int32_t iHasExtraInfo = ptri->EdgeHasExtraInfo(0);
            if (iHasExtraInfo == ptri->EdgeHasExtraInfo(2)) {
                int32_t iSharedTri = ptri->GetTriOrExtraInfo(0);
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edges 0 and 2 both point to the same triangle "
                    "%d",
                    iFormID,
                    cellName.String(),
                    i,
                    iSharedTri
                );
                bFoundProblem = true;
            }
        }
        iTri = ptri->GetTriOrExtraInfo(1);
        if (iTri == ptri->GetTriOrExtraInfo(2) && ptri->GetTriOrExtraInfo(1) != 0xFFFF) {
            int32_t iHasExtraInfo = ptri->EdgeHasExtraInfo(1);
            if (iHasExtraInfo == ptri->EdgeHasExtraInfo(2)) {
                int32_t iSharedTri = ptri->GetTriOrExtraInfo(1);
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edges 1 and 2 both point to the same triangle "
                    "%d",
                    iFormID,
                    cellName.String(),
                    i,
                    iSharedTri
                );
                bFoundProblem = true;
            }
        }
    }
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        for (int32_t edge1index = 0; edge1index < 3; ++edge1index) {
            int32_t edge2index = edge1index + 1;
            if (edge1index == 2) {
                edge2index = 0;
            }
            uint16_t vert1 = ptri->GetVert(static_cast<uint32_t>(edge1index));
            uint16_t vert2 = ptri->GetVert(static_cast<uint32_t>(edge2index));
            for (uint32_t j = 0; j < Triangles.iSize; ++j) {
                if (j != i) {
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
                                            iFormID,
                                            cellName.String(),
                                            i,
                                            j
                                        );
                                        bFoundProblem = true;
                                    }
                                    if (ptri->EdgeHasExtraInfo(edge1index)) {
                                        if (!ppotentialTri->EdgeHasExtraInfo(f)) {
                                            Warning(
                                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d Should "
                                                "have Portal information, but doesn't",
                                                iFormID,
                                                cellName.String(),
                                                j,
                                                f
                                            );
                                            bFoundProblem = true;
                                        }
                                    }
                                    else if (ptri->GetTriOrExtraInfo(edge1index) != 0xFFFF) {
                                        if (ppotentialTri->EdgeHasExtraInfo(f)) {
                                            Warning(
                                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d has a "
                                                "portal data to Triangle %d, but Triangle %d doesn't have any "
                                                "Portal info for its matching edge %d",
                                                iFormID,
                                                cellName.String(),
                                                i,
                                                edge1index,
                                                j,
                                                j,
                                                f
                                            );
                                            bFoundProblem = true;
                                        }
                                        if (ptri->GetTriOrExtraInfo(edge1index) != j) {
                                            Warning(
                                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d Should "
                                                "have a link to Triangle %d, but doesn't",
                                                iFormID,
                                                cellName.String(),
                                                i,
                                                edge1index,
                                                j
                                            );
                                            bFoundProblem = true;
                                        }
                                        if (ppotentialTri->GetTriOrExtraInfo(f) != i) {
                                            Warning(
                                                "PATHFINDING: Navmesh %08x Cell %s, Triangle %d Edge %d Should "
                                                "have a link to Triangle %d, but doesn't",
                                                iFormID,
                                                cellName.String(),
                                                j,
                                                f,
                                                i
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
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        NavMeshTriangle* ptri = GetTriangle(i);
        for (int32_t triedgevert1 = 0; triedgevert1 < 3; ++triedgevert1) {
            if (!ptri->EdgeHasExtraInfo(triedgevert1) && ptri->GetTriOrExtraInfo(triedgevert1) != 0xFFFF) {
                int32_t othertriedgevert1;
                NavMeshTriangle* potherTri = GetTriangle(ptri->GetTriOrExtraInfo(triedgevert1));
                char bFoundLink = 0;
                for (othertriedgevert1 = 0; othertriedgevert1 < 3; ++othertriedgevert1) {
                    if (!potherTri->EdgeHasExtraInfo(othertriedgevert1) &&
                        potherTri->GetTriOrExtraInfo(othertriedgevert1) != 0xFFFF &&
                        potherTri->GetTriOrExtraInfo(othertriedgevert1) == i) {
                        bFoundLink = 1;
                        break;
                    }
                }
                if (!bFoundLink) {
                    int32_t iOtherTri = ptri->GetTriOrExtraInfo(triedgevert1);
                    int32_t iLinkedTri = ptri->GetTriOrExtraInfo(triedgevert1);
                    Warning(
                        "PATHFINDING: Navmesh %08x Cell %s, Triangle %d has a link to Triangle %d, but Triangle %d "
                        "doesn't have the matching link",
                        iFormID,
                        cellName.String(),
                        i,
                        iLinkedTri,
                        iOtherTri
                    );
                    bFoundProblem = true;
                }
                if (othertriedgevert1 < 3) {
                    int32_t triedgevert2 = triedgevert1 + 1;
                    if (triedgevert1 == 2) {
                        triedgevert2 = 0;
                    }
                    int32_t othertriedgevert2 = othertriedgevert1 + 1;
                    if (othertriedgevert1 == 2) {
                        othertriedgevert2 = 0;
                    }
                    int32_t iVert = ptri->GetVert(static_cast<uint32_t>(triedgevert1));
                    if (iVert != potherTri->GetVert(static_cast<uint32_t>(othertriedgevert2)) ||
                        (iVert = ptri->GetVert(static_cast<uint32_t>(triedgevert2)),
                         iVert != potherTri->GetVert(static_cast<uint32_t>(othertriedgevert1)))) {
                        int32_t iOtherTri = ptri->GetTriOrExtraInfo(triedgevert1);
                        Warning(
                            "PATHFINDING: Navmesh %08x Cell %s, Triangles %d and %d are linked, but their vertices "
                            "do not match",
                            iFormID,
                            cellName.String(),
                            i,
                            iOtherTri
                        );
                        bFoundProblem = true;
                    }
                }
            }
        }
    }
    for (uint32_t i = 0; i < CoverArray.iSize; ++i) {
        uint16_t icoverTri = CoverArray.GetAt(static_cast<int32_t>(i));
        NavMeshTriangle* ptri = GetTriangle(icoverTri);
        if (ptri) {
            if (!ptri->HasCover()) {
                Warning(
                    "PATHFINDING: Navmesh %08x Cell %s, Cover Array (index %d) referes to triangle %d, which does "
                    "not have cover.",
                    iFormID,
                    cellName.String(),
                    i,
                    icoverTri
                );
                bFoundProblem = true;
            }
        }
        else {
            Warning(
                "PATHFINDING: Navmesh %08x Cell %s, Cover Array (index %d) referes to triangle %d, which does not "
                "exist.",
                iFormID,
                cellName.String(),
                i,
                icoverTri
            );
            bFoundProblem = true;
        }
    }
    CheckNavMeshAgainstNeighbors();
    return !bFoundProblem;
}

void NavMesh::DeleteExtraEdgeInfo(uint16_t aiExtraInfo) {
    ExtraEdgeInfo.RemoveFast(aiExtraInfo, 1u);
    uint32_t uiCount = Triangles.iSize;
    for (uint32_t i = 0; i < uiCount; ++i) {
        if (Triangles.GetAt(static_cast<int32_t>(i)).HasEdgeExtraInfo()) {
            for (uint32_t t = 0; t < 3; ++t) {
                if (Triangles.GetAt(static_cast<int32_t>(i)).EdgeHasExtraInfo(static_cast<int32_t>(t))) {
                    if (Triangles.GetAt(static_cast<int32_t>(i)).GetTriOrExtraInfo(static_cast<int32_t>(t)) ==
                        aiExtraInfo) {
                        Triangles.GetAt(static_cast<int32_t>(i)).ClearEdgeExtraInfo(static_cast<int32_t>(t));
                    }
                    else {
                        int32_t iExtraInfo = Triangles.GetAt(static_cast<int32_t>(i))
                                                 .GetTriOrExtraInfo(static_cast<int32_t>(t));
                        if (iExtraInfo == ExtraEdgeInfo.iSize) {
                            Triangles.GetAt(static_cast<int32_t>(i))
                                .SetEdgeExtraInfo(static_cast<int32_t>(t), aiExtraInfo);
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
        if (!GetTriangle(i)->GetFlag(NavMeshTriangle::NAV_TRI_DISABLED) && GetTriangle(i)->HasCover()) {
            uint16_t usTriangle = i;
            CoverArray.Add(usTriangle);
        }
    }
}

bool NavMesh::GetMatchingEdge(const NavMeshEdgeHandle& aThisEdge, NavMeshEdgeHandle* apRetOtherEdge) {
    NavMesh* pOtherNavMesh = nullptr;
    int32_t iOtherEdgeIndex = -1;
    uint16_t usOtherTriangleIndex;
    if (!GetMatchingEdge(
            aThisEdge.usTriangle,
            aThisEdge.iEdgeIndex,
            &pOtherNavMesh,
            &usOtherTriangleIndex,
            &iOtherEdgeIndex
        )) {
        return false;
    }
    apRetOtherEdge->pNavMeshInfo = pOtherNavMesh->GetNavMeshInfo();
    apRetOtherEdge->usTriangle = usOtherTriangleIndex;
    apRetOtherEdge->iEdgeIndex = iOtherEdgeIndex;
    return true;
}

EdgeExtraInfo* NavMesh::GetEdgeExtraInfo(uint16_t ausTriangleIndex, int32_t aiEdgeIndex) {
    if (!Triangles.GetAt(ausTriangleIndex).EdgeHasExtraInfo(aiEdgeIndex)) {
        return nullptr;
    }
    if (Triangles.GetAt(ausTriangleIndex).GetTriOrExtraInfo(aiEdgeIndex) == 65535) {
        return nullptr;
    }
    uint16_t triOrExtraInfo = Triangles.GetAt(ausTriangleIndex).GetTriOrExtraInfo(aiEdgeIndex);
    return &ExtraEdgeInfo.GetAt(triOrExtraInfo);
}

uint16_t NavMesh::AddExtraEdgeInfo(const EdgeExtraInfo& aExtraInfo) {
    return ExtraEdgeInfo.Add(aExtraInfo);
}

uint16_t NavMesh::AddVertex(const NavMeshVertex& aVertex) {
    return Vertices.Add(aVertex);
}

bool NavMesh::GetMatchingEdge(const FastNavMeshEdgeHandle& aThisEdge, FastNavMeshEdgeHandle* apRetOtherEdge) {
    return GetMatchingEdge(
        aThisEdge.usTriangle,
        aThisEdge.iEdgeIndex,
        &apRetOtherEdge->pNavMesh,
        &apRetOtherEdge->usTriangle,
        &apRetOtherEdge->iEdgeIndex
    );
}

NavMeshTriangleDoorPortal* NavMesh::GetDoorPortalFromArray(uint16_t aiDoorPortalIndex) {
    return &DoorPortals[aiDoorPortalIndex];
}

// Reads the grid size twice.
uint32_t NavMesh::GetGridCellCount() {
    uint32_t uiSize = MeshGrid.GetGridSize();
    return MeshGrid.GetGridSize() * uiSize;
}

NavMeshStaticAvoidNode* NavMesh::GetStaticAvoidNode(uint32_t aiIndex) {
    return &StaticAvoidNodes.GetAt(aiIndex);
}

// The node is built on the stack and copied into StaticAvoidNodes.
void NavMesh::AddStaticAvoidNode(const NiPoint3& aLocation, uint16_t ausTri, float afRadius, float afCost) {
    NavMeshStaticAvoidNode kNode(aLocation, ausTri, afRadius, afCost);
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
            NavMeshPtr spNavMesh = pWestCell->GetNavMeshArray()->GetNavMeshByIndex(i);
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
            NavMeshPtr spNavMesh = pEastCell->GetNavMeshArray()->GetNavMeshByIndex(i);
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
            NavMeshPtr spNavMesh = pSouthCell->GetNavMeshArray()->GetNavMeshByIndex(i);
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
            NavMeshPtr spNavMesh = pNorthCell->GetNavMeshArray()->GetNavMeshByIndex(i);
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
    return Vertices.BSSimpleArray<NavMeshVertex>::GetSize();
}

uint32_t NavMesh::GetNumDoorPortals() {
    return DoorPortals.BSSimpleArray<NavMeshTriangleDoorPortal>::GetSize();
}

uint32_t NavMesh::GetStaticAvoidNodeCount() {
    return StaticAvoidNodes.iSize;
}

void NavMesh::GetTriangleVertices(
    uint16_t ausTriIndex,
    const NavMeshVertex*& apV0,
    const NavMeshVertex*& apV1,
    const NavMeshVertex*& apV2
) {
    NavMeshTriangle* pTriangle = GetTriangle(ausTriIndex);
    apV0 = GetVertex(pTriangle->GetVert(0));
    apV1 = GetVertex(pTriangle->GetVert(1u));
    apV2 = GetVertex(pTriangle->GetVert(2u));
}

bool NavMesh::GetMatchingEdge_0(
    uint16_t ausTriangleIndex,
    int32_t aiEdgeIndex,
    NavMeshPtr* apReturnNavMesh,
    uint16_t* apOtherTriangleIndex,
    int32_t* apOtherEdgeIndex
) {
    NavMesh* pOtherNavMesh = nullptr;
    if (!GetMatchingEdge(ausTriangleIndex, aiEdgeIndex, &pOtherNavMesh, apOtherTriangleIndex, apOtherEdgeIndex)) {
        return false;
    }
    *apReturnNavMesh = NavMeshPtr::MakeNavMeshPtr(pOtherNavMesh);
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
        const NavMeshVertex* pvert = GetVertex(ptri->GetVert(i));
        irow = 0;
        icol = 0;
        MeshGrid.GetGridRowAndColumn(pvert->Location, &irow, &icol);
        imaxRow = BSMAX(imaxRow, irow);
        iminRow = BSMIN(iminRow, irow);
        imaxCol = BSMAX(imaxCol, icol);
        iminCol = BSMIN(iminCol, icol);
    }
    for (uint32_t i = iminRow; i < imaxRow + 1; ++i) {
        for (uint32_t j = iminCol; j < imaxCol + 1; ++j) {
            if (MeshGrid.IsTriangleInGridCell(ausTriangleIndex, j, i)) {
                uint32_t uiGridCell = j + i * MeshGrid.iGridSize;
                apGridCells->Add(uiGridCell);
            }
        }
    }
    return apGridCells->iSize;
}

int32_t
NavMesh::GetStaticAvoidNodesForTri(uint16_t ausTriangle, BSSimpleArray<NavMeshStaticAvoidNode*, 1024>& aRetArray) {
    for (uint32_t i = 0; i < StaticAvoidNodes.iSize; ++i) {
        if (StaticAvoidNodes.GetAt(static_cast<int32_t>(i)).usTriangle == ausTriangle) {
            NavMeshStaticAvoidNode* pAvoidNode = &StaticAvoidNodes.GetAt(static_cast<int32_t>(i));
            aRetArray.Add(pAvoidNode);
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
    NavMeshInfo* pInfo = TES::pInstance->GetNavMeshInfoMap()->GetNavMeshInfo(aiID);
    if (pInfo) {
        NavMeshPtr spNavMesh;
        if (pInfo->GetNavMeshEvenDisabled(spNavMesh)) {
            TESForm* pForm = spNavMesh.m_pObject;
            if (pForm->GetDisabled() != abDisable) {
                pForm = spNavMesh.m_pObject;
                pForm->SetDisabled(abDisable);
                pInfo->SetDisabled(abDisable);
                if (!abDisable) {
                    spNavMesh->ReconnectEnabledPortals();
                }
            }
            return true;
        }

        Warning("DisableNavMesh Script Function: Did not find matching NavMesh for NavMesh ID parameter %08X", aiID);
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
    int32_t iRow = 0;
    int32_t iCol = 0;
    GetGridRowAndColumn(aLoc, reinterpret_cast<uint32_t*>(&iRow), reinterpret_cast<uint32_t*>(&iCol));
    return iCol + iGridSize * iRow;
}

bool NavMeshInfo::GetNavMesh(NavMeshPtr& aReturnNavMeshPtr) const {
    if (GetDisabled()) {
        return false;
    }
    NavMeshPtr::MakeNavMeshPtr(aReturnNavMeshPtr, pNavMesh);
    return static_cast<NavMesh*>(aReturnNavMeshPtr) != nullptr;
}

TESForm* NavMeshInfo::GetParentSpace() const {
    if (pParentSpace != nullptr) {
        return pParentSpace;
    }
    return TESForm::GetFormByNumericID(ParentSpaceID);
}

bool NavMeshInfo::GetDisabled() const {
    return (uiFlags & NAVMESHINFO_FLAG_DISABLED) != 0;
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
