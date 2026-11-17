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
	: NiRefObject(), NavMeshToLOSGridMap(0x25u), fPercentSeen(0.0f), Center(), fRadius(0.0f), fSightRadius(0.0f),
	  Lock() {}

PathingLOSGridMap::~PathingLOSGridMap() {
	Clear();
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe reads the navmesh's form ID through the trivial +0xC getter 0x84E3A0.
bool PathingLOSGridMap::Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	Lock.LockForWrite();
	for (uint32_t i = 0;; ++i) {
		uint32_t FormFlags = aNavMeshes.iSize;
		if (i >= FormFlags) {
			break;
		}
		::new (&spmesh) NavMeshPtr(aNavMeshes.GetAt(static_cast<int32_t>(i)));
		PathingLOSGrid* pgrid = 0;
		if (NavMeshToLOSGridMap.GetAt(spmesh.m_pObject->iFormID, pgrid)) {
			pgrid->Setup(spmesh);
		} else {
			PathingLOSGrid* pgrid_2;
			PathingLOSGrid* v6 = static_cast<PathingLOSGrid*>(ourMalloc(sizeof(PathingLOSGrid)));
			if (v6) {
				pgrid_2 = ::new (v6) PathingLOSGrid(spmesh);
			} else {
				pgrid_2 = 0;
			}
			pgrid = pgrid_2;
			NavMeshToLOSGridMap.SetAt(spmesh.m_pObject->iFormID, pgrid_2);
		}
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
	}
	Lock.UnlockWrite();
	return 1;
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
		PathingLOSGrid* pgrid = 0;
		NavMeshToLOSGridMap.GetNext(it, inavmeshID, pgrid);
		if (pgrid) {
			delete pgrid;
		}
	}
	NavMeshToLOSGridMap.RemoveAll();
	Lock.UnlockWrite();
}

// The exe's cleanup-only C++ EH frame is left out.
void PathingLOSGridMap::Initialize(const PathingLocation& arCenter, float afRadius) {
	Lock.LockForWrite();
	Center.operator=(arCenter);
	fRadius = afRadius;
	using a3_t = BSScrapArray<NavMeshPtr, 1024>;
	FNVD_CONSTRUCTED_LOCAL(a3_t, a3, &a3_t::Destruct);
	if (Pathing::FindConnectedNavmeshesForLocation(Center, afRadius, &a3)) {
		Setup(a3);
	} else {
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
// IDA names this Lst0044::Data00D4::Load; CombatGroup::LoadGame calls it on spPathingLOSGridMap.
// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
void PathingLOSGridMap::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t i = 0; i < uiCount; ++i) {
		bool bDrop = false;
		void* pMemory = ourMalloc(sizeof(PathingLOSGrid));
		PathingLOSGrid* pGrid;
		if (pMemory)
			pGrid = ::new (pMemory) PathingLOSGrid;
		else
			pGrid = nullptr;
		uint32_t uiNavMeshID;
		apLoadGameBuffer->LoadFormID(uiNavMeshID);
		// The exe calls the grid's LoadGame without a null test.
		pGrid->LoadGame(apLoadGameBuffer);
		if (!uiNavMeshID)
			bDrop = true;
		if (bDrop) {
			if (pGrid)
				delete pGrid;
		} else {
			NavMeshToLOSGridMap.SetAt(uiNavMeshID, pGrid);
		}
	}
	// The buffer fills the floats as raw bytes, as the exe hands it their addresses.
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPercentSeen), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSightRadius), 4);
	// The exe calls the center's LoadGame through its vtable (slot 1, thiscall, ret 4: no cleanup after the call at
	// 0x6E0E68).
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

NiPoint3* PathingLOSGridMap::GetLocation(NiPoint3* a2) {
	::new (a2) NiPoint3(Center.GetLocation());
	return a2;
}

// The exe's cleanup-only C++ EH frame is left out.
bool PathingLOSGridMap::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	NiTMapItem<uint32_t, PathingLOSGrid*>* it;
	Lock.LockForRead();
	using loadedNavMeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
	FNVD_CONSTRUCTED_LOCAL(loadedNavMeshes_t, loadedNavMeshes, &loadedNavMeshes_t::Destruct);
	it = NavMeshToLOSGridMap.GetFirstPos();
	while (it) {
		uint32_t navMeshID;
		PathingLOSGrid* pgrid;
		NavMeshToLOSGridMap.GetNext(it, navMeshID, pgrid);
		::new (&spmesh) NavMeshPtr;
		if (pgrid->GetNavMesh(spmesh)) {
			loadedNavMeshes.Add(spmesh);
		}
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
	}
	aspLOSMap.m_pObject->Setup(loadedNavMeshes);
	it = NavMeshToLOSGridMap.GetFirstPos();
	while (it) {
		uint32_t navMeshID_2;
		PathingLOSGrid* pgrid_2;
		NavMeshToLOSGridMap.GetNext(it, navMeshID_2, pgrid_2);
		::new (&spmesh_2) NavMeshPtr;
		if (pgrid_2->GetNavMesh(spmesh_2)) {
			pgrid_2->SetLOSMapFromSeenTriangles(aspLOSMap);
		}
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh_2);
	}
	Lock.UnlockRead();
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe reads the navmesh's form ID through the trivial +0xC getter 0x84E3A0.
bool PathingLOSGridMap::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	Lock.LockForWrite();
	for (uint32_t i = 0;; ++i) {
		uint32_t NavMeshCount = aspLOSMap.m_pObject->GetNavMeshCount();
		if (i >= NavMeshCount) {
			break;
		}
		NavMeshPtr* NavMeshByIndex = &aspLOSMap.m_pObject->GetNavMeshByIndex(i);
		::new (&spmesh) NavMeshPtr(*NavMeshByIndex);
		PathingLOSGrid* pgrid = 0;
		if (!NavMeshToLOSGridMap.GetAt(spmesh.m_pObject->iFormID, pgrid)) {
			PathingLOSGrid* pgrid_2;
			PathingLOSGrid* v8 = static_cast<PathingLOSGrid*>(ourMalloc(sizeof(PathingLOSGrid)));
			if (v8) {
				NavMeshInfo* NavMeshInfo_ = spmesh.m_pObject->GetNavMeshInfo();
				pgrid_2 = ::new (v8) PathingLOSGrid(NavMeshInfo_);
			} else {
				pgrid_2 = 0;
			}
			pgrid = pgrid_2;
			NavMeshToLOSGridMap.SetAt(spmesh.m_pObject->iFormID, pgrid_2);
		}
		pgrid->SetSeenTrianglesFromLOSMap(aspLOSMap);
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
	}
	fPercentSeen = aspLOSMap.m_pObject->GetSeenPercent();
	Lock.UnlockWrite();
	return 1;
}

bool PathingLOSGridMap::IsLocationSeen(const PathingLocation& arLocation) {
	const NavMeshInfo* pinfo = arLocation.pNavMesh;
	if (!pinfo || arLocation.GetTriangle() == 0xFFFF) {
		return 0;
	}
	PathingLOSGrid* pgrid = 0;
	return NavMeshToLOSGridMap.GetAt(pinfo->NavMeshID, pgrid) && pgrid->IsLocationSeen(arLocation);
}

bool PathingLOSGridMap::SetLocationSeen(const PathingLocation& arLocation, bool abSeen) {
	const NavMeshInfo* pinfo = arLocation.pNavMesh;
	if (!pinfo || arLocation.GetTriangle() == 0xFFFF) {
		return 0;
	}
	PathingLOSGrid* pgrid = 0;
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

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the shared out-of-line operator[] (0x6A7AD0) on the grid cell indices; the template's operator[]
// stands in for it.
bool PathingLOSGrid::SetLocationSeen(const PathingLocation& arLocation, bool abSeen) {
	using gridCells_t = BSSimpleArray<uint32_t, 1024>;
	FNVD_INLINED_LOCAL(gridCells_t, gridCells, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	::new (&spmesh) NavMeshPtr;
	if (arLocation.GetNavMesh(spmesh)) {
		::new (&gridCells) BSSimpleArray<uint32_t, 1024>;
		uint16_t Triangle = arLocation.GetTriangle();
		spmesh.m_pObject->GetGridIndicesForTriangle(Triangle, &gridCells);
		bool bresult = 0;
		for (uint32_t iindex = 0; iindex < gridCells.iSize; ++iindex) {
			uint32_t uiIndex = gridCells.operator[](iindex);
			if (uiIndex < LOSGridCells.iSize &&
				LOSGridCells.GetAt(static_cast<int32_t>(uiIndex))->SetLocationSeen(arLocation, abSeen)) {
				bresult = 1;
			}
		}
		gridCells.~BSSimpleArray<uint32_t, 1024>();
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return bresult;
	} else {
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	}
}

bool PathingLOSGrid::GetNavMesh(NavMeshPtr& spRetNavMesh) const {
	return (bool)((uint8_t)(pNavMeshInfo && pNavMeshInfo->GetNavMesh(spRetNavMesh)));
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe stores each cell's navmesh info through the one-field setter 0x600AB0 (a trivial accessor).
void PathingLOSGrid::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	uint32_t inavmeshID = 0;
	apLoadGameBuffer->LoadFormID(inavmeshID);
	uint32_t v3 = inavmeshID;
	NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
	pNavMeshInfo = NavMeshInfoMap_->GetNavMeshInfo(v3);
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	LOSGridCells.SetSize(isaveGameCount, 1);
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		PathingLOSGridCell* v4;
		PathingLOSGridCell** plosGridCell = &LOSGridCells.operator[](i);
		PathingLOSGridCell* v6 = static_cast<PathingLOSGridCell*>(ourMalloc(sizeof(PathingLOSGridCell)));
		if (v6) {
			v4 = ::new (v6) PathingLOSGridCell;
		} else {
			v4 = 0;
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
	} else {
		aiFormID = 0;
	}
	apSaveGameBuffer->SaveFormID(aiFormID, 0);
	if (this) {
		isaveGameCount = LOSGridCells.iSize;
	} else {
		isaveGameCount = 0;
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		LOSGridCells.operator[](i)->SaveGame(apSaveGameBuffer);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void PathingLOSGrid::Setup(NavMeshInfo* a1) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	pNavMeshInfo = a1;
	::new (&spmesh) NavMeshPtr;
	if (pNavMeshInfo->GetNavMesh(spmesh)) {
		Setup(spmesh);
	} else {
		pNavMeshInfo = 0;
	}
	FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
}

// The exe's cleanup-only C++ EH frame is left out.
void PathingLOSGrid::Setup(const NavMeshPtr& aspNavMesh) {
	uint32_t igridCellCount = aspNavMesh.m_pObject->GetGridCellCount();
	if (!pNavMeshInfo || LOSGridCells.iSize != igridCellCount) {
		LOSGridCells.Clear(0);
		LOSGridCells.SetReservedSize(igridCellCount);
		for (uint32_t i = 0; i < igridCellCount; ++i) {
			PathingLOSGridCell* v4;
			PathingLOSGridCell* apMemory = static_cast<PathingLOSGridCell*>(ourMalloc(sizeof(PathingLOSGridCell)));
			if (apMemory) {
				uint32_t v3 = i;
				NavMeshInfo* NavMeshInfo_ = aspNavMesh.m_pObject->GetNavMeshInfo();
				v4 = ::new (apMemory) PathingLOSGridCell(NavMeshInfo_, v3);
			} else {
				v4 = 0;
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
	return 1;
}

bool PathingLOSGrid::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
	uint32_t i_2 = 0;
	for (uint32_t i = LOSGridCells.iSize; i_2 < i; i = LOSGridCells.iSize) {
		LOSGridCells.GetAt(i_2++)->SetSeenTrianglesFromLOSMap(aspLOSMap);
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out.
bool PathingLOSGrid::IsLocationSeen(const PathingLocation& arLocation) {
	NiPoint3* Location_;
	uint32_t FormFlags;
	NiPoint3 result;
	uint32_t iindex;
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	::new (&spmesh) NavMeshPtr;
	if (arLocation.GetNavMesh(spmesh) &&
		(Location_ = &(result = arLocation.GetLocation()),
			(iindex = spmesh.m_pObject->GetGridIndex(*Location_),
				(FormFlags = LOSGridCells.iSize, iindex < FormFlags)))) {
		bool IsLocationSeen = LOSGridCells.GetAt(iindex)->IsLocationSeen(arLocation);
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return IsLocationSeen;
	} else {
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	}
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

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the one-byte GetAt (0x6A62F0) out of line, folded with the unsigned char instantiation's; the
// template's GetAt stands in for it.
bool PathingLOSGridCell::SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	const BSSimpleArray<uint16_t, 1024>* ptriangles;
	char* pdestArray = aspLOSMap.m_pObject->GetArrayForNavMesh(pNavMeshInfo->NavMeshID);
	if (!pdestArray) {
		return 0;
	}
	::new (&spmesh) NavMeshPtr;
	if (pNavMeshInfo->GetNavMesh(spmesh) &&
		(ptriangles = spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex)) != 0) {
		if (SeenTriangleList.IsEmpty()) {
			if (bAllSeen) {
				for (uint32_t i_2 = 0; i_2 < ptriangles->iSize; ++i_2) {
					pdestArray[ptriangles->GetAt(static_cast<int32_t>(i_2))] = 3;
				}
			} else {
				for (uint32_t i_3 = 0; i_3 < ptriangles->iSize; ++i_3) {
					if (pdestArray[ptriangles->GetAt(static_cast<int32_t>(i_3))] == 127) {
						pdestArray[ptriangles->GetAt(static_cast<int32_t>(i_3))] = 0;
					}
				}
			}
		} else {
			uint32_t FormFlags = ptriangles->iSize;
			uint32_t v3 = SeenTriangleList.iSize;
			uint32_t iminSize = BSMIN(v3, FormFlags);
			for (uint32_t i = 0; i < iminSize; ++i) {
				char* pseen = &SeenTriangleList.GetAt(static_cast<int32_t>(i));
				pdestArray[ptriangles->GetAt(static_cast<int32_t>(i))] = *pseen;
			}
		}
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 1;
	} else {
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the one-byte GetAt (0x6A62F0) out of line and reads each byte signed (movsx); the template's GetAt
// stands in for it.
bool PathingLOSGridCell::SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	const BSSimpleArray<uint16_t, 1024>* ptriangles;
	char* psrcArray = aspLOSMap.m_pObject->GetArrayForNavMesh(pNavMeshInfo->NavMeshID);
	if (!psrcArray) {
		return 0;
	}
	::new (&spmesh) NavMeshPtr;
	if (pNavMeshInfo->GetNavMesh(spmesh) &&
		(ptriangles = spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex)) != 0) {
		SeenTriangleList.Clear(0);
		uint32_t FormFlags = ptriangles->iSize;
		SeenTriangleList.SetSize(FormFlags, 1);
		bAllSeen = 1;
		for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
			int32_t itriangle = ptriangles->GetAt(static_cast<int32_t>(i));
			SeenTriangleList.GetAt(static_cast<int32_t>(i)) = psrcArray[itriangle];
			if (SeenTriangleList.GetAt(static_cast<int32_t>(i)) <= 0 ||
				SeenTriangleList.GetAt(static_cast<int32_t>(i)) == 127) {
				bAllSeen = 0;
			}
		}
		if (bAllSeen) {
			SeenTriangleList.Clear(1);
		}
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 1;
	} else {
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	}
}

void PathingLOSGridCell::Setup(NavMeshInfo* apNavMeshInfo, uint32_t aiGridIndex) {
	bAllSeen = (bool)0;
	pNavMeshInfo = apNavMeshInfo;
	iNavMeshGridCellIndex = aiGridIndex;
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the one-byte GetAt (0x6A62F0) out of line, folded with the unsigned char instantiation's; the
// template's GetAt stands in for it.
bool PathingLOSGridCell::SetLocationSeen(const PathingLocation& arLocation, bool abSeen) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	::new (&spmesh) NavMeshPtr;
	if (!pNavMeshInfo->GetNavMesh(spmesh)) {
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	}
	const BSSimpleArray<uint16_t, 1024>* ptriangles = spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex);
	if (!bAllSeen) {
		if (ptriangles->iSize == SeenTriangleList.iSize) {
			for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
				int32_t striangle = ptriangles->GetAt(static_cast<int32_t>(i));
				if (striangle == arLocation.GetTriangle()) {
					SeenTriangleList.GetAt(static_cast<int32_t>(i)) = abSeen ? 1 : -1;
					FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
					return 1;
				}
			}
		}
		FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
		return 0;
	}
	if (!abSeen) {
		bAllSeen = 0;
		SeenTriangleList.SetSize(ptriangles->iSize, 1);
		for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
			int32_t striangle = ptriangles->GetAt(static_cast<int32_t>(i));
			if (striangle == arLocation.GetTriangle()) {
				SeenTriangleList.GetAt(static_cast<int32_t>(i)) = -1;
			} else {
				SeenTriangleList.GetAt(static_cast<int32_t>(i)) = 1;
			}
		}
	}
	FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
	return 1;
}

uint32_t PathingLOSGridCell::Load(BGSLoadGameBuffer* a1) {
	a1->LoadData(this, 1u);
	a1->LoadData(&iNavMeshGridCellIndex, 4u);
	uint32_t result = bAllSeen;
	if (!bAllSeen) {
		uint32_t auiNewSize = a1->LoadVariableSizedValue();
		SeenTriangleList.SetSize(auiNewSize, 1);
		for (uint32_t aiIndex = 0;; ++aiIndex) {
			result = aiIndex;
			if (aiIndex >= auiNewSize) {
				break;
			}
			char* v4 = &SeenTriangleList.operator[](aiIndex);
			a1->LoadData(v4, 1u);
		}
	}
	return result;
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe calls the one-byte GetAt (0x6A62F0) out of line and reads the byte signed (movsx); the template's GetAt
// stands in for it.
bool PathingLOSGridCell::IsLocationSeen(const PathingLocation& arLocation) {
	FNVD_INLINED_LOCAL(NavMeshPtr, spmesh, std::nullopt, std::nullopt);
	if (bAllSeen) {
		return 1;
	}
	::new (&spmesh) NavMeshPtr;
	if (pNavMeshInfo->GetNavMesh(spmesh)) {
		const BSSimpleArray<uint16_t, 1024>* ptriangles =
			spmesh.m_pObject->GetTrianglesForGridCell(iNavMeshGridCellIndex);
		if (ptriangles->iSize == SeenTriangleList.iSize) {
			for (uint32_t i = 0; i < ptriangles->iSize; ++i) {
				int32_t striangle = ptriangles->GetAt(static_cast<int32_t>(i));
				if (striangle == arLocation.GetTriangle()) {
					bool bSeen = SeenTriangleList.GetAt(static_cast<int32_t>(i)) > 0;
					FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
					return bSeen;
				}
			}
		}
	}
	FNVD_DESTRUCT(NiPointer<NavMesh>)(&spmesh);
	return 0;
}

void PathingLOSGridCell::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(this, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iNavMeshGridCellIndex, 4u, 0);
	if (!bAllSeen) {
		uint32_t isaveGameCount;
		// The exe tests the address &SeenTriangleList, which is never null.
		isaveGameCount = SeenTriangleList.iSize;
		apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
		for (uint32_t i = 0; i < isaveGameCount; ++i) {
			char* v4 = &SeenTriangleList.operator[](i);
			apSaveGameBuffer->SaveDataEndian(v4, 1u, 0);
		}
	}
}

void PathingLOSGridCell::operator delete(void* apMemory) {
	ourFree(apMemory);
}
