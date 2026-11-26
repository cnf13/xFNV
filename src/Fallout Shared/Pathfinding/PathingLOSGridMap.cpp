#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

PathingLOSGridMap::PathingLOSGridMap()
  : NiRefObject(), NavMeshToLOSGridMap(0x25u), fPercentSeen(0.0f), Center(), fRadius(0.0f), fSightRadius(0.0f), Lock() {
}

PathingLOSGridMap::~PathingLOSGridMap() {
    Clear();
}

bool PathingLOSGridMap::Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes) {
    Lock.LockForWrite();
    for (uint32_t i = 0; i < aNavMeshes.iSize; ++i) {
        NavMeshPtr spmesh(aNavMeshes.GetAt(static_cast<int32_t>(i)));
        PathingLOSGrid* pgrid = nullptr;
        if (NavMeshToLOSGridMap.GetAt(spmesh->iFormID, pgrid)) {
            pgrid->Setup(spmesh);
        }
        else {
            pgrid = new PathingLOSGrid(spmesh);
            NavMeshToLOSGridMap.SetAt(spmesh->iFormID, pgrid);
        }
    }
    Lock.UnlockWrite();
    return true;
}

bool PathingLOSGridMap::IsValid() {
    return fRadius > 0.0;
}

void PathingLOSGridMap::Clear() {
    Lock.LockForWrite();
    NiTMapItem<uint32_t, PathingLOSGrid*>* it = NavMeshToLOSGridMap.GetFirstPos();
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
    BSScrapArray<NavMeshPtr, 1024> kNavMeshes;
    if (Pathing::FindConnectedNavmeshesForLocation(Center, afRadius, &kNavMeshes)) {
        Setup(kNavMeshes);
    }
    else {
        fRadius = 0.0f;
    }
    Lock.UnlockWrite();
}

void PathingLOSGridMap::ClearSeen() {
    fPercentSeen = 0.0f;
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
        PathingLOSGrid* pGrid = new PathingLOSGrid;
        uint32_t uiNavMeshID;
        apLoadGameBuffer->LoadFormID(uiNavMeshID);
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
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPercentSeen), sizeof(fPercentSeen));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), sizeof(fRadius));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSightRadius), sizeof(fSightRadius));
    // Calls the center's LoadGame through its vtable.
    PathingLocation* pCenter = &Center;
    pCenter->LoadGame(apLoadGameBuffer);
}

void PathingLOSGridMap::SetSightRadius(float afSightRadius) {
    fSightRadius = afSightRadius;
}

void PathingLOSGridMap::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    PathingLOSGrid* pgrid;
    uint32_t uiNavMeshID;
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    NiTMapItem<uint32_t, PathingLOSGrid*>* it = NavMeshToLOSGridMap.GetFirstPos();
    while (it) {
        pgrid = nullptr;
        NavMeshToLOSGridMap.GetNext(it, uiNavMeshID, pgrid);
        if (pgrid) {
            apSaveGameBuffer->SaveFormID(uiNavMeshID, 0);
            pgrid->SaveGame(apSaveGameBuffer);
            ++uiCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    apSaveGameBuffer->SaveDataEndian(&fPercentSeen, sizeof(fPercentSeen), 0);
    apSaveGameBuffer->SaveDataEndian(&fRadius, sizeof(fRadius), 0);
    apSaveGameBuffer->SaveDataEndian(&fSightRadius, sizeof(fSightRadius), 0);
    Center.SaveGame(apSaveGameBuffer);
}

NiPoint3 PathingLOSGridMap::GetLocation() {
    return Center.GetLocation();
}

bool PathingLOSGridMap::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
    Lock.LockForRead();
    using loadedNavMeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
    loadedNavMeshes_t loadedNavMeshes;
    NiTMapItem<uint32_t, PathingLOSGrid*>* it = NavMeshToLOSGridMap.GetFirstPos();
    while (it) {
        uint32_t navMeshID;
        PathingLOSGrid* pgrid;
        NavMeshToLOSGridMap.GetNext(it, navMeshID, pgrid);
        NavMeshPtr spmesh;
        if (pgrid->GetNavMesh(spmesh)) {
            loadedNavMeshes.Add(spmesh);
        }
    }
    aspLOSMap->Setup(loadedNavMeshes);
    it = NavMeshToLOSGridMap.GetFirstPos();
    while (it) {
        uint32_t navMeshID;
        PathingLOSGrid* pgrid;
        NavMeshToLOSGridMap.GetNext(it, navMeshID, pgrid);
        NavMeshPtr spmesh;
        if (pgrid->GetNavMesh(spmesh)) {
            pgrid->SetLOSMapFromSeenTriangles(aspLOSMap);
        }
    }
    Lock.UnlockRead();
    return true;
}

bool PathingLOSGridMap::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
    Lock.LockForWrite();
    for (uint32_t i = 0; i < aspLOSMap->GetNavMeshCount(); ++i) {
        NavMeshPtr spmesh(aspLOSMap->GetNavMeshByIndex(i));
        PathingLOSGrid* pgrid = nullptr;
        if (!NavMeshToLOSGridMap.GetAt(spmesh->iFormID, pgrid)) {
            pgrid = new PathingLOSGrid(spmesh->GetNavMeshInfo());
            NavMeshToLOSGridMap.SetAt(spmesh->iFormID, pgrid);
        }
        pgrid->SetSeenTrianglesFromLOSMap(aspLOSMap);
    }
    fPercentSeen = aspLOSMap->GetSeenPercent();
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
    for (PathingLOSGridCell* pCell : LOSGridCells) {
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
        uint16_t usTriangle = arLocation.GetTriangle();
        spmesh->GetGridIndicesForTriangle(usTriangle, &gridCells);
        bool bresult = false;
        for (uint32_t uiIndex : gridCells) {
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
    return pNavMeshInfo && pNavMeshInfo->GetNavMesh(spRetNavMesh);
}

void PathingLOSGrid::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t inavmeshID = 0;
    apLoadGameBuffer->LoadFormID(inavmeshID);
    uint32_t uiNavMeshID = inavmeshID;
    NavMeshInfoMap* pnavMeshInfoMap = TES::pInstance->GetNavMeshInfoMap();
    pNavMeshInfo = pnavMeshInfoMap->GetNavMeshInfo(uiNavMeshID);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    LOSGridCells.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        PathingLOSGridCell* pcell;
        PathingLOSGridCell** plosGridCell = &LOSGridCells[i];
        pcell = new PathingLOSGridCell;
        *plosGridCell = pcell;
        (*plosGridCell)->Load(apLoadGameBuffer);
        (*plosGridCell)->pNavMeshInfo = pNavMeshInfo;
    }
}

void PathingLOSGrid::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount;
    uint32_t uiFormID = pNavMeshInfo ? pNavMeshInfo->NavMeshID : 0;
    apSaveGameBuffer->SaveFormID(uiFormID, 0);
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

void PathingLOSGrid::Setup(NavMeshInfo* apNavMeshInfo) {
    pNavMeshInfo = apNavMeshInfo;
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh)) {
        Setup(spmesh);
    }
    else {
        pNavMeshInfo = nullptr;
    }
}

void PathingLOSGrid::Setup(const NavMeshPtr& aspNavMesh) {
    uint32_t igridCellCount = aspNavMesh->GetGridCellCount();
    if (!pNavMeshInfo || LOSGridCells.iSize != igridCellCount) {
        LOSGridCells.Clear(false);
        LOSGridCells.SetReservedSize(igridCellCount);
        for (uint32_t i = 0; i < igridCellCount; ++i) {
            PathingLOSGridCell* pcell = new PathingLOSGridCell(aspNavMesh->GetNavMeshInfo(), i);
            LOSGridCells.Add(pcell);
        }
        pNavMeshInfo = aspNavMesh->GetNavMeshInfo();
    }
}

bool PathingLOSGrid::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
    for (uint32_t i = 0; i < LOSGridCells.iSize; ++i) {
        LOSGridCells.GetAt(i)->SetLOSMapFromSeenTriangles(aspLOSMap);
    }
    return true;
}

bool PathingLOSGrid::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
    for (uint32_t i = 0; i < LOSGridCells.iSize; ++i) {
        LOSGridCells.GetAt(i)->SetSeenTrianglesFromLOSMap(aspLOSMap);
    }
    return true;
}

bool PathingLOSGrid::IsLocationSeen(const PathingLocation& arLocation) {
    NavMeshPtr spmesh;
    if (arLocation.GetNavMesh(spmesh)) {
        uint32_t iindex = spmesh->GetGridIndex(arLocation.GetLocation());
        if (iindex < LOSGridCells.iSize) {
            return LOSGridCells.GetAt(iindex)->IsLocationSeen(arLocation);
        }
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
    char* pdestArray = aspLOSMap->GetArrayForNavMesh(pNavMeshInfo->NavMeshID);
    if (!pdestArray) {
        return false;
    }
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh) &&
        (ptriangles = spmesh->GetTrianglesForGridCell(iNavMeshGridCellIndex)) != nullptr) {
        if (SeenTriangleList.IsEmpty()) {
            if (bAllSeen) {
                for (uint16_t usTriangle : *ptriangles) {
                    pdestArray[usTriangle] = 3;
                }
            }
            else {
                for (uint16_t usTriangle : *ptriangles) {
                    if (pdestArray[usTriangle] == 127) {
                        pdestArray[usTriangle] = 0;
                    }
                }
            }
        }
        else {
            uint32_t uiTriangleCount = ptriangles->iSize;
            uint32_t uiSeenCount = SeenTriangleList.iSize;
            uint32_t iminSize = BSMIN(uiSeenCount, uiTriangleCount);
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
    char* psrcArray = aspLOSMap->GetArrayForNavMesh(pNavMeshInfo->NavMeshID);
    if (!psrcArray) {
        return false;
    }
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh) &&
        (ptriangles = spmesh->GetTrianglesForGridCell(iNavMeshGridCellIndex)) != nullptr) {
        SeenTriangleList.Clear(false);
        uint32_t uiTriangleCount = ptriangles->iSize;
        SeenTriangleList.SetSize(uiTriangleCount, true);
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
    const BSSimpleArray<uint16_t, 1024>* ptriangles = spmesh->GetTrianglesForGridCell(iNavMeshGridCellIndex);
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

uint32_t PathingLOSGridCell::Load(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&bAllSeen, sizeof(bAllSeen));
    apLoadGameBuffer->LoadData(&iNavMeshGridCellIndex, sizeof(iNavMeshGridCellIndex));
    uint32_t uiReturn = bAllSeen;
    if (!bAllSeen) {
        uint32_t uiSize = apLoadGameBuffer->LoadVariableSizedValue();
        SeenTriangleList.SetSize(uiSize, true);
        for (uint32_t i = 0;; ++i) {
            uiReturn = i;
            if (i >= uiSize) {
                break;
            }
            char* pseen = &SeenTriangleList[i];
            apLoadGameBuffer->LoadData(pseen, sizeof(*pseen));
        }
    }
    return uiReturn;
}

bool PathingLOSGridCell::IsLocationSeen(const PathingLocation& arLocation) {
    if (bAllSeen) {
        return true;
    }
    NavMeshPtr spmesh;
    if (pNavMeshInfo->GetNavMesh(spmesh)) {
        const BSSimpleArray<uint16_t, 1024>* ptriangles = spmesh->GetTrianglesForGridCell(iNavMeshGridCellIndex);
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
    apSaveGameBuffer->SaveDataEndian(&bAllSeen, sizeof(bAllSeen), 0);
    apSaveGameBuffer->SaveDataEndian(&iNavMeshGridCellIndex, sizeof(iNavMeshGridCellIndex), 0);
    if (!bAllSeen) {
        uint32_t isaveGameCount = SeenTriangleList.iSize;
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
        for (uint32_t i = 0; i < isaveGameCount; ++i) {
            char* pseen = &SeenTriangleList[i];
            apSaveGameBuffer->SaveDataEndian(pseen, sizeof(*pseen), 0);
        }
    }
}

void PathingLOSGridCell::operator delete(void* apMemory) {
    ourFree(apMemory);
}
