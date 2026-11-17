#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

class NavMeshInfo;
class TESObjectCELL;
class TESWorldSpace;

// The form (NAVI) that maps every navmesh to its info: connections, doors and island data.
class NavMeshInfoMap : public TESForm {
public:
    // The navmesh infos of one world space (or of the interiors, under 0), keyed by cell key or cell
    // (NavMeshInfo::GetCellOrCoords: an interior cell's address, so the key is pointer-sized).
    using CellKeyInfoMap = NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>;

    // A NAVI form with an empty ID map (0x2719 buckets) and world space map (103 buckets).
    NavMeshInfoMap();
    ~NavMeshInfoMap() override;

    bool ProcessBeforeSave() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    bool SavesBefore(TESForm* apForm) override;
    void InitItem() override;
    void SetAltered(bool abVal) override;
    using TESForm::Save;
    using TESForm::SavesBefore;

    // Deletes the info of navmesh auiNavMeshID.
    void DeleteNavMeshInfo(uint32_t auiNavMeshID);
    // The info of navmesh auiNavMeshID, else null.
    NavMeshInfo* GetNavMeshInfo(uint32_t auiNavMeshID);
    // Takes navmesh auiNavMeshID's info out of the maps and marks the map altered, without
    // freeing the info.
    void RemoveNavMeshInfo(uint32_t auiNavMeshID);
    // Adds apNavMeshInfo to the ID map and the cell key map and marks the map altered.
    void AddNavMeshInfo(NavMeshInfo* apNavMeshInfo);
    // The navmeshes that may cover apCell: the interior map's entry, or the cell key's in its world space.
    const BSSimpleArray<NavMeshInfo*, 1024>* FindNavMeshInfoForCell(TESObjectCELL* apCell);
    // The navmeshes that may cover cell aiCellKey of apWorldSpace.
    const BSSimpleArray<NavMeshInfo*, 1024>* FindNavMeshInfoForCellKey(TESWorldSpace* apWorldSpace, int32_t aiCellKey);
    // The navmeshes that may cover interior cell apCell (the world map's entry 0, keyed by the cell).
    const BSSimpleArray<NavMeshInfo*, 1024>* FindNavMeshInfoForInteriorCell(TESObjectCELL* apCell);
    // Adds apNavMeshInfo to its cell key's list, creating the world space's map and the list when missing.
    void AddInfoToCellKeyMap(NavMeshInfo* apNavMeshInfo);
    // Validates every info against its navmesh: removes broken infos and rebuilds stale ones.
    void CheckMap();
    // Whether every cell key's navmesh list, when it has more than one navmesh, lacks bounds on at most one of them.
    bool CheckAllNavmeshBoundingBoxes();
    void Clear();
    void GetAllNavMeshInfo(BSSimpleArray<NavMeshInfo*, 1024>* apReturnArray);
    // Takes apNavMeshInfo out of its cell key's list, freeing the list and the world space's map once empty.
    void RemoveInfoFromCellKeyMap(NavMeshInfo* apNavMeshInfo);

    bool bUpdateAll;
    uint8_t padding19[3];
    // The infos by navmesh form ID.
    NiTPointerMap<uint32_t, NavMeshInfo*> InfoMap;
    // The cell key maps by world space form ID.
    NiTMap<uint32_t, CellKeyInfoMap*> CellKeyNavMeshInfoMap;
    uint8_t bInit;
};
