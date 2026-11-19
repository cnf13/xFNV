#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLOSGrid.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridCell.h"
#include "Fallout Shared/Pathfinding/PathingLOSMap.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/TES.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

PathingLOSGridMap::PathingLOSGridMap()
  : NiRefObject(), NavMeshToLOSGridMap(0x25u), fPercentSeen(0.0f), Center(), fRadius(0.0f), fSightRadius(0.0f), Lock() {
}

PathingLOSGridMap::~PathingLOSGridMap() {
    Clear();
}

bool PathingLOSGridMap::Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes) {
    Lock.LockForWrite();
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = aNavMeshes.iSize;
        if (i >= FormFlags) {
            break;
        }
        NavMeshPtr spmesh(aNavMeshes.GetAt(static_cast<int32_t>(i)));
        PathingLOSGrid* pgrid = nullptr;
        if (NavMeshToLOSGridMap.GetAt(spmesh.m_pObject->iFormID, pgrid)) {
            pgrid->Setup(spmesh);
        }
        else {
            PathingLOSGrid* pgrid_2;
            PathingLOSGrid* v6 = static_cast<PathingLOSGrid*>(ourMalloc(sizeof(PathingLOSGrid)));
            if (v6) {
                pgrid_2 = ::new (v6) PathingLOSGrid(spmesh);
            }
            else {
                pgrid_2 = nullptr;
            }
            pgrid = pgrid_2;
            NavMeshToLOSGridMap.SetAt(spmesh.m_pObject->iFormID, pgrid_2);
        }
    }
    Lock.UnlockWrite();
    return true;
}

bool PathingLOSGridMap::IsValid() {
    return (bool)((uint8_t)(fRadius > 0.0));
}

void PathingLOSGridMap::Clear() {
    NiTMapItem<uint32_t, PathingLOSGrid*>* it;
    Lock.LockForWrite();
    it = NavMeshToLOSGridMap.GetFirstPos();
    while (it) {
        uint32_t inavmeshID = 0;
        PathingLOSGrid* pgrid = nullptr;
        NavMeshToLOSGridMap.GetNext(it, inavmeshID, pgrid);
        if (pgrid) {
            delete pgrid;
        }
    }
    NavMeshToLOSGridMap.RemoveAll();
    Lock.UnlockWrite();
}

void PathingLOSGridMap::Initialize(const PathingLocation& arCenter, float afRadius) {
    Lock.LockForWrite();
    Center = arCenter;
    fRadius = afRadius;
    using a3_t = BSScrapArray<NavMeshPtr, 1024>;
    a3_t a3;
    if (Pathing::FindConnectedNavmeshesForLocation(Center, afRadius, &a3)) {
        Setup(a3);
    }
    else {
        fRadius = 0.0f;
    }
    Lock.UnlockWrite();
}

void PathingLOSGridMap::ClearSeen() {
    fPercentSeen = (float)0.0;
    Clear();
    Initialize(Center, fRadius);
}

// Reads the grid map back from a save game: the grids (a count, then per grid its navmesh form ID and the grid,
// which is dropped when the ID is null), the seen percentage, the radii and the center.
// CombatGroup::LoadGame calls it on spPathingLOSGridMap.
void PathingLOSGridMap::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        bool bDrop = false;
        void* pMemory = ourMalloc(sizeof(PathingLOSGrid));
        PathingLOSGrid* pGrid;
        if (pMemory) {
            pGrid = ::new (pMemory) PathingLOSGrid;
        }
        else {
            pGrid = nullptr;
        }
        uint32_t uiNavMeshID;
        apLoadGameBuffer->LoadFormID(uiNavMeshID);
        // Calls the grid's LoadGame without a null test, as the game does.
        pGrid->LoadGame(apLoadGameBuffer);
        if (!uiNavMeshID) {
            bDrop = true;
        }
        if (bDrop) {
            if (pGrid) {
                delete pGrid;
            }
        }
        else {
            NavMeshToLOSGridMap.SetAt(uiNavMeshID, pGrid);
        }
    }
    // The buffer fills the floats as raw bytes through their addresses.
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPercentSeen), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSightRadius), 4);
    // Calls the center's LoadGame through its vtable.
    PathingLocation* pCenter = &Center;
    pCenter->LoadGame(apLoadGameBuffer);
}

void PathingLOSGridMap::SetSightRadius(float afSightRadius) {
    fSightRadius = afSightRadius;
}

void PathingLOSGridMap::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    PathingLOSGrid* a4;
    uint32_t a3;
    NiTMapItem<uint32_t, PathingLOSGrid*>* a2;
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
    a2 = (&NavMeshToLOSGridMap)->GetFirstPos();
    while (a2) {
        a4 = nullptr;
        (&NavMeshToLOSGridMap)->GetNext(a2, a3, a4);
        if (a4) {
            apSaveGameBuffer->SaveFormID(a3, (uint32_t)0);
            a4->SaveGame(apSaveGameBuffer);
            ++aiVariableValue;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue, aiPosition);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fPercentSeen)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fRadius)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fSightRadius)), 4u, 0);
    (&Center)->SaveGame(apSaveGameBuffer);
}

NiPoint3 PathingLOSGridMap::GetLocation() {
    return Center.GetLocation();
}

bool PathingLOSGridMap::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
    NiTMapItem<uint32_t, PathingLOSGrid*>* it;
    Lock.LockForRead();
    using loadedNavMeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    loadedNavMeshes_t loadedNavMeshes;
    it = NavMeshToLOSGridMap.GetFirstPos();
    while (it) {
        uint32_t navMeshID;
        PathingLOSGrid* pgrid;
        NavMeshToLOSGridMap.GetNext(it, navMeshID, pgrid);
        NavMeshPtr spmesh;
        if (pgrid->GetNavMesh(spmesh)) {
            loadedNavMeshes.Add(spmesh);
        }
    }
    aspLOSMap.m_pObject->Setup(loadedNavMeshes);
    it = NavMeshToLOSGridMap.GetFirstPos();
    while (it) {
        uint32_t navMeshID_2;
        PathingLOSGrid* pgrid_2;
        NavMeshToLOSGridMap.GetNext(it, navMeshID_2, pgrid_2);
        NavMeshPtr spmesh_2;
        if (pgrid_2->GetNavMesh(spmesh_2)) {
            pgrid_2->SetLOSMapFromSeenTriangles(aspLOSMap);
        }
    }
    Lock.UnlockRead();
    return true;
}

bool PathingLOSGridMap::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
    Lock.LockForWrite();
    for (uint32_t i = 0;; ++i) {
        uint32_t NavMeshCount = aspLOSMap.m_pObject->GetNavMeshCount();
        if (i >= NavMeshCount) {
            break;
        }
        NavMeshPtr* NavMeshByIndex = &aspLOSMap.m_pObject->GetNavMeshByIndex(i);
        NavMeshPtr spmesh(*NavMeshByIndex);
        PathingLOSGrid* pgrid = nullptr;
        if (!NavMeshToLOSGridMap.GetAt(spmesh.m_pObject->iFormID, pgrid)) {
            PathingLOSGrid* pgrid_2;
            PathingLOSGrid* v8 = static_cast<PathingLOSGrid*>(ourMalloc(sizeof(PathingLOSGrid)));
            if (v8) {
                NavMeshInfo* NavMeshInfo_ = spmesh.m_pObject->GetNavMeshInfo();
                pgrid_2 = ::new (v8) PathingLOSGrid(NavMeshInfo_);
            }
            else {
                pgrid_2 = nullptr;
            }
            pgrid = pgrid_2;
            NavMeshToLOSGridMap.SetAt(spmesh.m_pObject->iFormID, pgrid_2);
        }
        pgrid->SetSeenTrianglesFromLOSMap(aspLOSMap);
    }
    fPercentSeen = aspLOSMap.m_pObject->GetSeenPercent();
    Lock.UnlockWrite();
    return true;
}

bool PathingLOSGridMap::IsLocationSeen(const PathingLocation& arLocation) {
    const NavMeshInfo* pinfo = arLocation.pNavMesh;
    if (!pinfo || arLocation.GetTriangle() == 0xFFFF) {
        return false;
    }
    PathingLOSGrid* pgrid = nullptr;
    return NavMeshToLOSGridMap.GetAt(pinfo->NavMeshID, pgrid) && pgrid->IsLocationSeen(arLocation);
}

bool PathingLOSGridMap::SetLocationSeen(const PathingLocation& arLocation, bool abSeen) {
    const NavMeshInfo* pinfo = arLocation.pNavMesh;
    if (!pinfo || arLocation.GetTriangle() == 0xFFFF) {
        return false;
    }
    PathingLOSGrid* pgrid = nullptr;
    return NavMeshToLOSGridMap.GetAt(pinfo->NavMeshID, pgrid) && pgrid->SetLocationSeen(arLocation, abSeen);
}

PathingLOSGrid::PathingLOSGrid() : LOSGridCells(), pNavMeshInfo(nullptr) {}

PathingLOSGrid::PathingLOSGrid(NavMeshInfo* apNavMeshInfo) : LOSGridCells(), pNavMeshInfo(nullptr) {
    Setup(apNavMeshInfo);
}

PathingLOSGrid::PathingLOSGrid(const NavMeshPtr& aspNavMesh) : LOSGridCells(), pNavMeshInfo(nullptr) {
    Setup(aspNavMesh);
}

// The grid owns its cells.
PathingLOSGrid::~PathingLOSGrid() {
    for (uint32_t i = 0; i < LOSGridCells.iSize; ++i) {
        PathingLOSGridCell* pCell = LOSGridCells.GetAt(i);
        if (pCell) {
            delete pCell;
        }
    }
}

bool PathingLOSGrid::SetLocationSeen(const PathingLocation& arLocation, bool abSeen) {
    using gridCells_t = BSSimpleArray<uint32_t, 1024>;
    NavMeshPtr spmesh;
    if (arLocation.GetNavMesh(spmesh)) {
        gridCells_t gridCells;
        uint16_t Triangle = arLocation.GetTriangle();
        spmesh.m_pObject->GetGridIndicesForTriangle(Triangle, &gridCells);
        bool bresult = false;
        for (uint32_t iindex = 0; iindex < gridCells.iSize; ++iindex) {
            uint32_t uiIndex = gridCells[iindex];
            if (uiIndex < LOSGridCells.iSize &&
                LOSGridCells.GetAt(static_cast<int32_t>(uiIndex))->SetLocationSeen(arLocation, abSeen)) {
                bresult = true;
            }
        }
        return bresult;
    }

    return false;
}

bool PathingLOSGrid::GetNavMesh(NavMeshPtr& spRetNavMesh) const {
    return (bool)((uint8_t)(pNavMeshInfo && pNavMeshInfo->GetNavMesh(spRetNavMesh)));
}

void PathingLOSGrid::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t inavmeshID = 0;
    apLoadGameBuffer->LoadFormID(inavmeshID);
    uint32_t v3 = inavmeshID;
    NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
    pNavMeshInfo = NavMeshInfoMap_->GetNavMeshInfo(v3);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    LOSGridCells.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        PathingLOSGridCell* v4;
        PathingLOSGridCell** plosGridCell = &LOSGridCells[i];
        PathingLOSGridCell* v6 = static_cast<PathingLOSGridCell*>(ourMalloc(sizeof(PathingLOSGridCell)));
        if (v6) {
            v4 = ::new (v6) PathingLOSGridCell;
        }
        else {
            v4 = nullptr;
        }
        *plosGridCell = v4;
        (*plosGridCell)->Load(apLoadGameBuffer);
        (*plosGridCell)->pNavMeshInfo = pNavMeshInfo;
    }
}

void PathingLOSGrid::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount;
    uint32_t aiFormID;
    if (pNavMeshInfo) {
        aiFormID = pNavMeshInfo->NavMeshID;
    }
    else {
        aiFormID = 0;
    }
    apSaveGameBuffer->SaveFormID(aiFormID, 0);
    if (this) {
        isaveGameCount = LOSGridCells.iSize;
    }
    else {
        isaveGameCount = 0;
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        LOSGridCells[i]->SaveGame(apSaveGameBuffer);
    }
}

void PathingLOSGrid::Setup(NavMeshInfo* a1) {
    pNavMeshInfo = a1;
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh)) {
        Setup(spmesh);
    }
    else {
        pNavMeshInfo = nullptr;
    }
}

void PathingLOSGrid::Setup(const NavMeshPtr& aspNavMesh) {
    uint32_t igridCellCount = aspNavMesh.m_pObject->GetGridCellCount();
    if (!pNavMeshInfo || LOSGridCells.iSize != igridCellCount) {
        LOSGridCells.Clear(false);
        LOSGridCells.SetReservedSize(igridCellCount);
        for (uint32_t i = 0; i < igridCellCount; ++i) {
            PathingLOSGridCell* v4;
            PathingLOSGridCell* apMemory = static_cast<PathingLOSGridCell*>(ourMalloc(sizeof(PathingLOSGridCell)));
            if (apMemory) {
                uint32_t v3 = i;
                NavMeshInfo* NavMeshInfo_ = aspNavMesh.m_pObject->GetNavMeshInfo();
                v4 = ::new (apMemory) PathingLOSGridCell(NavMeshInfo_, v3);
            }
            else {
                v4 = nullptr;
            }
            PathingLOSGridCell* a2 = v4;
            LOSGridCells.Add(a2);
        }
        pNavMeshInfo = aspNavMesh.m_pObject->GetNavMeshInfo();
    }
}

bool PathingLOSGrid::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
    uint32_t i_2 = 0;
    for (uint32_t i = LOSGridCells.iSize; i_2 < i; i = LOSGridCells.iSize) {
        LOSGridCells.GetAt(i_2++)->SetLOSMapFromSeenTriangles(aspLOSMap);
    }
    return true;
}

bool PathingLOSGrid::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
    uint32_t i_2 = 0;
    for (uint32_t i = LOSGridCells.iSize; i_2 < i; i = LOSGridCells.iSize) {
        LOSGridCells.GetAt(i_2++)->SetSeenTrianglesFromLOSMap(aspLOSMap);
    }
    return true;
}

bool PathingLOSGrid::IsLocationSeen(const PathingLocation& arLocation) {
    NiPoint3* Location_;
    uint32_t FormFlags;
    NiPoint3 result;
    uint32_t iindex;
    NavMeshPtr spmesh;
    if (arLocation.GetNavMesh(spmesh) &&
        (Location_ = &(result = arLocation.GetLocation()),
         (iindex = spmesh.m_pObject->GetGridIndex(*Location_), (FormFlags = LOSGridCells.iSize, iindex < FormFlags)))) {
        bool IsLocationSeen = LOSGridCells.GetAt(iindex)->IsLocationSeen(arLocation);
        return IsLocationSeen;
    }

    return false;
}

void PathingLOSGrid::operator delete(void* apMemory) {
    ourFree(apMemory);
}

PathingLOSGridCell::PathingLOSGridCell()
  : bAllSeen(false), SeenTriangleList(), pNavMeshInfo(nullptr), iNavMeshGridCellIndex(0xFFFFFFFFu) {}

PathingLOSGridCell::PathingLOSGridCell(NavMeshInfo* apNavMeshInfo, uint32_t aiGridIndex) : SeenTriangleList() {
    Setup(apNavMeshInfo, aiGridIndex);
}

PathingLOSGridCell::~PathingLOSGridCell() {}

bool PathingLOSGridCell::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
    const BSSimpleArray<uint16_t, 1024>* ptriangles;
    char* pdestArray = aspLOSMap.m_pObject->GetArrayForNavMesh(pNavMeshInfo->NavMeshID);
    if (!pdestArray) {
        return false;
    }
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh) &&
        (ptriangles = spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex)) != nullptr) {
        if (SeenTriangleList.IsEmpty()) {
            if (bAllSeen) {
                for (uint32_t i_2 = 0; i_2 < ptriangles->iSize; ++i_2) {
                    pdestArray[ptriangles->GetAt(static_cast<int32_t>(i_2))] = 3;
                }
            }
            else {
                for (uint32_t i_3 = 0; i_3 < ptriangles->iSize; ++i_3) {
                    if (pdestArray[ptriangles->GetAt(static_cast<int32_t>(i_3))] == 127) {
                        pdestArray[ptriangles->GetAt(static_cast<int32_t>(i_3))] = 0;
                    }
                }
            }
        }
        else {
            uint32_t FormFlags = ptriangles->iSize;
            uint32_t v3 = SeenTriangleList.iSize;
            uint32_t iminSize = BSMIN(v3, FormFlags);
            for (uint32_t i = 0; i < iminSize; ++i) {
                char* pseen = &SeenTriangleList.GetAt(static_cast<int32_t>(i));
                pdestArray[ptriangles->GetAt(static_cast<int32_t>(i))] = *pseen;
            }
        }
        return true;
    }

    return false;
}

// Each byte is read signed.
bool PathingLOSGridCell::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
    const BSSimpleArray<uint16_t, 1024>* ptriangles;
    char* psrcArray = aspLOSMap.m_pObject->GetArrayForNavMesh(pNavMeshInfo->NavMeshID);
    if (!psrcArray) {
        return false;
    }
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh) &&
        (ptriangles = spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex)) != nullptr) {
        SeenTriangleList.Clear(false);
        uint32_t FormFlags = ptriangles->iSize;
        SeenTriangleList.SetSize(FormFlags, true);
        bAllSeen = true;
        for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
            int32_t itriangle = ptriangles->GetAt(static_cast<int32_t>(i));
            SeenTriangleList.GetAt(static_cast<int32_t>(i)) = psrcArray[itriangle];
            if (SeenTriangleList.GetAt(static_cast<int32_t>(i)) <= 0 ||
                SeenTriangleList.GetAt(static_cast<int32_t>(i)) == 127) {
                bAllSeen = false;
            }
        }
        if (bAllSeen) {
            SeenTriangleList.Clear(true);
        }
        return true;
    }

    return false;
}

void PathingLOSGridCell::Setup(NavMeshInfo* apNavMeshInfo, uint32_t aiGridIndex) {
    bAllSeen = false;
    pNavMeshInfo = apNavMeshInfo;
    iNavMeshGridCellIndex = aiGridIndex;
}

bool PathingLOSGridCell::SetLocationSeen(const PathingLocation& arLocation, bool abSeen) {
    NavMeshPtr spmesh;
    if (!pNavMeshInfo->GetNavMesh(spmesh)) {
        return false;
    }
    const BSSimpleArray<uint16_t, 1024>* ptriangles = spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex);
    if (!bAllSeen) {
        if (ptriangles->iSize == SeenTriangleList.iSize) {
            for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
                int32_t striangle = ptriangles->GetAt(static_cast<int32_t>(i));
                if (striangle == arLocation.GetTriangle()) {
                    SeenTriangleList.GetAt(static_cast<int32_t>(i)) = abSeen ? 1 : -1;
                    return true;
                }
            }
        }
        return false;
    }
    if (!abSeen) {
        bAllSeen = false;
        SeenTriangleList.SetSize(ptriangles->iSize, true);
        for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
            int32_t striangle = ptriangles->GetAt(static_cast<int32_t>(i));
            if (striangle == arLocation.GetTriangle()) {
                SeenTriangleList.GetAt(static_cast<int32_t>(i)) = -1;
            }
            else {
                SeenTriangleList.GetAt(static_cast<int32_t>(i)) = 1;
            }
        }
    }
    return true;
}

uint32_t PathingLOSGridCell::Load(BGSLoadGameBuffer* a1) {
    a1->LoadData(this, 1u);
    a1->LoadData(&iNavMeshGridCellIndex, 4u);
    uint32_t result = bAllSeen;
    if (!bAllSeen) {
        uint32_t auiNewSize = a1->LoadVariableSizedValue();
        SeenTriangleList.SetSize(auiNewSize, true);
        for (uint32_t aiIndex = 0;; ++aiIndex) {
            result = aiIndex;
            if (aiIndex >= auiNewSize) {
                break;
            }
            char* v4 = &SeenTriangleList[aiIndex];
            a1->LoadData(v4, 1u);
        }
    }
    return result;
}

bool PathingLOSGridCell::IsLocationSeen(const PathingLocation& arLocation) {
    if (bAllSeen) {
        return true;
    }
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh)) {
        const BSSimpleArray<uint16_t, 1024>* ptriangles =
            spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex);
        if (ptriangles->iSize == SeenTriangleList.iSize) {
            for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
                int32_t striangle = ptriangles->GetAt(static_cast<int32_t>(i));
                if (striangle == arLocation.GetTriangle()) {
                    bool bSeen = SeenTriangleList.GetAt(static_cast<int32_t>(i)) > 0;
                    return bSeen;
                }
            }
        }
    }
    return false;
}

void PathingLOSGridCell::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(this, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iNavMeshGridCellIndex, 4u, 0);
    if (!bAllSeen) {
        uint32_t isaveGameCount;
        isaveGameCount = SeenTriangleList.iSize;
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
        for (uint32_t i = 0; i < isaveGameCount; ++i) {
            char* v4 = &SeenTriangleList[i];
            apSaveGameBuffer->SaveDataEndian(v4, 1u, 0);
        }
    }
}

void PathingLOSGridCell::operator delete(void* apMemory) {
    ourFree(apMemory);
}
