#include "Fallout Shared/Pathfinding/NavMesh_LoadSave.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleCutter.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "xfnv/Compiler.h"

// The NAVM DATA chunk: the parent cell and the array sizes. Files before version 5 have no DoorPortalCount.
struct NavMeshSaveStruct {
    // Byte swaps the six fields.
    void Endian();

    uint32_t ParentCellID;
    uint32_t VertexCount;
    uint32_t TriangleCount;
    uint32_t ExtraInfoCount;
    uint32_t CoverArrayCount;
    uint32_t DoorPortalCount;
};

// An extra edge info as files before version 5 store it.
struct EdgeExtraInfoOLD {
    // The stored size, 0x2E.
    static uint32_t GetSaveSize();

    float fHeightChange;
    NavMeshInfo* pOtherMesh;
    uint32_t DoorFormID;
    int32_t DoorLockpDoorCell;
    int32_t DoorLockpTeleportDoorCell;
    int32_t DoorLockpLockData;
    int32_t DoorLockpOwnershipForm;
    int32_t DoorLockiOwnershipRank;
    int32_t DoorLockpOwnershipGlobal;
    int32_t PAD4_1;
    int32_t PAD4_2;
    uint16_t sTriangle;
};

// Whether every loaded navmesh is checked, not only those of plugins that are not masters.
static SettingT<INISettingCollection> bINIGlobalNavMeshCheck("bGlobalNavMeshCheck:NavMeshGeneration", false);

// Reads the NAVM record. A navmesh whose triangle count carries the 0x10000000 "bad" bit is emptied and deleted; a
// deleted one is taken out of the info map. Older versions are converted: before 5 the extra edge infos come in the old
// layout and those that were door portals are deleted, before 6 the cover array is rebuilt, before 8 the
// no-large-creature flag is flipped, before 10 the preferred percentage is computed and before 11 the preferred
// triangles' edge connections move to the info's preferred list. The AutoMemContext is constructed and destroyed
// explicitly on a local buffer, and the constructor's result is ignored.
bool NavMesh::Load(TESFile* apFile) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_NAVMESH,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMesh_LoadSave.cpp",
        275
    );
    if (GetDelete()) {
        DecRefCount();
        pContext->~AutoMemContext();
        return false;
    }
    uint8_t formid = static_cast<uint8_t>(apFile->GetTESForm());
    if (formid != NAVM_ID) {
        pContext->~AutoMemContext();
        return false;
    }
    LoadForm(apFile);
    alignas(BSSimpleArray<int32_t>) uint8_t aIndicesStorage[sizeof(BSSimpleArray<int32_t>)];
    BSSimpleArray<int32_t>& oldDoorPortalIndices = *::new (aIndicesStorage) BSSimpleArray<int32_t>;
    int32_t ifileVersionNumber = 1;
    bool bbadNavMesh = false;
    if (!GetDelete()) {
        do {
            switch (apFile->GetTESChunk()) {
                case EDID_ID: {
                    char* pMessage = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                    apFile->GetChunkData(pMessage, 0x200);
                    SetFormEditorID(pMessage);
                    break;
                }
                case OBND_ID: LoadObjectBound(apFile); break;
                case NVER_ID:
                    apFile->GetChunkData(&ifileVersionNumber, 4);
                    if (apFile->QEndian()) {
                        BSEndian(ifileVersionNumber, false);
                    }
                    break;
                case DATA_ID: {
                    NavMeshSaveStruct saveStruct;
                    if (ifileVersionNumber < 5) {
                        apFile->GetChunkData(&saveStruct, 0x14);
                        saveStruct.DoorPortalCount = 0;
                    }
                    else {
                        apFile->GetChunkData(&saveStruct, 0x18);
                    }
                    if (apFile->QEndian()) {
                        saveStruct.Endian();
                    }
                    if (saveStruct.TriangleCount >= 0x10000000) {
                        saveStruct.TriangleCount -= 0x10000000;
                        bbadNavMesh = true;
                    }
                    Vertices.SetSize(saveStruct.VertexCount, true);
                    Triangles.SetSize(saveStruct.TriangleCount, true);
                    ExtraEdgeInfo.SetSize(saveStruct.ExtraInfoCount, true);
                    CoverArray.SetSize(saveStruct.CoverArrayCount, true);
                    DoorPortals.SetSize(saveStruct.DoorPortalCount, true);
                    TESForm::AddCompileIndex(saveStruct.ParentCellID, apFile);
                    pParentCell = dynamic_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(saveStruct.ParentCellID));
                    if (!pParentCell) {
                        uint32_t uiFormID = GetFormID();
                        Warning(
                            "CELL: Could not find parent cell (%08X) for NavMesh (%08X).",
                            saveStruct.ParentCellID,
                            uiFormID
                        );
                        if (pLastLoadedCell) {
                            Warning(
                                "CELL: Attempting to remap NavMesh parent to cell (%08X) instead.",
                                pLastLoadedCell->GetFormID()
                            );
                            pParentCell = pLastLoadedCell;
                        }
                    }
                    break;
                }
                case NVVX_ID: {
                    // The size is taken before the buffer.
                    uint32_t uiDataSize = 12 * Vertices.GetSize();
                    apFile->GetChunkData(&Vertices.GetAt(0), uiDataSize);
                    if (apFile->QEndian()) {
                        uint32_t isize = Vertices.GetSize();
                        for (uint32_t i = 0; i < isize; ++i) {
                            Vertices.GetAt(i).Endian();
                        }
                    }
                    break;
                }
                case NVTR_ID: {
                    // The size is taken before the buffer.
                    uint32_t uiDataSize = 16 * Triangles.GetSize();
                    apFile->GetChunkData(&Triangles.GetAt(0), uiDataSize);
                    uint32_t isize = Triangles.GetSize();
                    for (uint32_t i = 0; i < isize; ++i) {
                        if (apFile->QEndian()) {
                            Triangles.GetAt(i).Endian();
                        }
                        Triangles.GetAt(i).SetObjectCount(0);
                    }
                    break;
                }
                case NVCA_ID: {
                    // The size is taken before the buffer.
                    uint32_t uiDataSize = 2 * CoverArray.GetSize();
                    apFile->GetChunkData(&CoverArray.GetAt(0), uiDataSize);
                    if (apFile->QEndian()) {
                        uint32_t isize = CoverArray.GetSize();
                        for (uint32_t i = 0; i < isize; ++i) {
                            BSEndian(CoverArray.GetAt(i), false);
                        }
                    }
                    break;
                }
                case NVDP_ID: {
                    // The file stores each door's form ID in pDoorForm. A door that does not resolve takes the door
                    // flag off its triangle and its portal out of the array.
                    int32_t isize = DoorPortals.GetSize();
                    apFile->GetChunkData(&DoorPortals.GetAt(0), 8 * isize);
                    // The file's portal records are 8 bytes: the door's 4-byte form ID, then the triangle index.
                    TESFile::WidenFormIDRecords(
                        &DoorPortals.GetAt(0),
                        isize,
                        8,
                        sizeof(NavMeshTriangleDoorPortal),
                        {{0, offsetof(NavMeshTriangleDoorPortal, pDoorForm)}}
                    );
                    for (int32_t i = 0; i < isize; ++i) {
                        if (apFile->QEndian()) {
                            DoorPortals.GetAt(i).Endian();
                        }
                        uint32_t idoorID = static_cast<uint32_t>(
                            reinterpret_cast<uintptr_t>(DoorPortals.GetAt(i).pDoorForm)
                        );
                        TESForm::AddCompileIndex(idoorID, apFile);
                        TESObjectREFR* pdoor = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(idoorID));
                        if (pdoor) {
                            DoorPortals.GetAt(i).pDoorForm = pdoor;
                        }
                        else {
                            BSStringT<char> navmeshString;
                            GetFormDetailedString(navmeshString);
                            Warning(
                                "NavMesh: Door reference %08X is not found but is linked to %s.  Please finalize to "
                                "resolve this.",
                                idoorID,
                                navmeshString.String()
                            );
                            uint32_t itriIndex = DoorPortals.GetAt(i).iOwningTriangleIndex;
                            if (itriIndex < Triangles.GetSize()) {
                                Triangles.GetAt(itriIndex).ClearFlag(NavMeshTriangle::NAV_TRI_DOOR);
                            }
                            DoorPortals.Remove(i--, true);
                            --isize;
                        }
                    }
                    break;
                }
                case NVGD_ID: {
                    uint32_t ichunkSize = apFile->GetChunkSize();
                    void* pchunkData = ourMalloc(ichunkSize);
                    apFile->GetChunkData(pchunkData, ichunkSize);
                    if (!MeshGrid.Load(static_cast<const char*>(pchunkData), ichunkSize, apFile)) {
                        BSStringT<char> navmeshString;
                        GetFormDetailedString(navmeshString);
                        Warning(
                            "MASTERFILE: Navmesh %s has a bad navmesh grid, please touch and resave",
                            navmeshString.String()
                        );
                        CreateGrid();
                    }
                    ourFree(pchunkData);
                    break;
                }
                case NVEX_ID: {
                    uint32_t ichunksize = apFile->GetChunkSize();
                    void* pchunkdata = ourMalloc(ichunksize);
                    apFile->GetChunkData(pchunkdata, ichunksize);
                    char* pchunkPointer = static_cast<char*>(pchunkdata);
                    if (ifileVersionNumber < 5) {
                        // Each old entry is rewritten as the portal SetFromBufferOld reads; the indices of those that
                        // were doors are kept for deletion.
                        uint32_t isize = ExtraEdgeInfo.GetSize();
                        EdgeExtraInfoOLD oldFormat;
                        for (uint32_t i = 0; i < isize; ++i) {
                            ourMemcpy(&oldFormat, pchunkPointer, sizeof(oldFormat));
                            if (oldFormat.DoorFormID) {
                                oldDoorPortalIndices.Add(i);
                            }
                            char newFormat[48];
                            char* pbufferLoc = newFormat;
                            *reinterpret_cast<NavMeshInfo**>(pbufferLoc) = oldFormat.pOtherMesh;
                            pbufferLoc += 4;
                            *reinterpret_cast<uint16_t*>(pbufferLoc) = oldFormat.sTriangle;
                            ExtraEdgeInfo.GetAt(i).SetFromBufferOld(apFile, newFormat);
                            pchunkPointer += EdgeExtraInfoOLD::GetSaveSize();
                        }
                    }
                    else if (ifileVersionNumber < 9) {
                        uint32_t isize = ExtraEdgeInfo.GetSize();
                        for (uint32_t i = 0; i < isize; ++i) {
                            ExtraEdgeInfo.GetAt(i).SetFromBufferOld(apFile, pchunkPointer);
                            pchunkPointer += ExtraEdgeInfo.GetAt(i).GetSaveSizeOld();
                        }
                    }
                    else {
                        uint32_t isize = ExtraEdgeInfo.GetSize();
                        for (uint32_t i = 0; i < isize; ++i) {
                            ExtraEdgeInfo.GetAt(i).SetFromBuffer(apFile, pchunkPointer);
                            pchunkPointer += ExtraEdgeInfo.GetAt(i).GetSaveSize();
                        }
                    }
                    ourFree(pchunkdata);
                    break;
                }
                default: break;
            }
        }
        while (apFile->NextChunk());
    }
    if (bbadNavMesh) {
        BSStringT<char> navmeshString;
        GetFormDetailedString(navmeshString);
        Warning("MASTERFILE: %s has unresolved warnings and will not be loaded.", navmeshString.String());
        ClearNavMesh(true);
        pNavMeshInfo = nullptr;
        pParentCell = nullptr;
        TESForm::SetDelete(true);
    }
    if (GetDelete()) {
        IncRefCount();
        uint32_t uiFormID = GetFormID();
        TES::pInstance->GetNavMeshInfoMap()->RemoveNavMeshInfo(uiFormID);
        oldDoorPortalIndices.~BSSimpleArray<int32_t>();
        pContext->~AutoMemContext();
        return false;
    }
    if (pNavMeshInfo) {
        pNavMeshInfo = nullptr;
    }
    if (!GetDelete()) {
        if (ifileVersionNumber < 5) {
            if (oldDoorPortalIndices.GetSize() > 0) {
                BSStringT<char> navmeshString;
                GetFormDetailedString(navmeshString);
                Warning(
                    "Depreciated NavMesh Door Portals found in %s, removing portals, please refinalize cell to "
                    "create new door portals",
                    navmeshString.String()
                );
            }
            uint32_t isize = oldDoorPortalIndices.GetSize();
            for (int32_t i = isize - 1; i >= 0; --i) {
                DeleteExtraEdgeInfo(static_cast<uint16_t>(oldDoorPortalIndices.GetAt(i)));
            }
        }
        if (ifileVersionNumber < 6) {
            // The conversion check is gone: each triangle is fetched and nothing is done with it, and
            // bcoverMessedUp stays false.
            bool bcoverMessedUp = false;
            uint32_t isize = Triangles.GetSize();
            for (uint32_t i = 0; i < isize; ++i) {
                GetTriangle(static_cast<uint16_t>(i));
            }
            CreateCoverArray();
            if (bcoverMessedUp) {
                BSStringT<char> navmeshString;
                GetFormDetailedString(navmeshString);
                // The form ID is passed before the string the format names, deliberately.
                const char* pString = navmeshString.String();
                Warning(
                    "%s had cover that could not be converted to the new format, please check and fix",
                    GetFormID(),
                    pString
                );
            }
        }
        if (ifileVersionNumber < 8) {
            uint32_t isize = Triangles.GetSize();
            for (uint32_t i = 0; i < isize; ++i) {
                NavMeshTriangle* ptri = GetTriangle(static_cast<uint16_t>(i));
                if (ptri->IsNoLargeCreature()) {
                    ptri->ToggleNoLargeCreature();
                }
            }
        }
        if (!TES::pInstance->GetNavMeshInfoMap()) {
            oldDoorPortalIndices.~BSSimpleArray<int32_t>();
            pContext->~AutoMemContext();
            return false;
        }
        uint32_t uiFormID = GetFormID();
        pNavMeshInfo = TES::pInstance->GetNavMeshInfoMap()->GetNavMeshInfo(uiFormID);
        if (pNavMeshInfo) {
            if (!pNavMeshInfo->GetParentSpace()) {
                pNavMeshInfo->ParentSpaceID = GetParentSpace()->GetFormID();
            }
            pNavMeshInfo->SetNavMesh(this);
        }
        else {
            Warning(
                "MASTERFILE: Did not find matching NavMeshInfo for NavMesh ID %08X, Creating a temporary one",
                GetFormID()
            );
            UpdateNavMeshInfo();
        }
        if (ifileVersionNumber < 10) {
            uint32_t isize = Triangles.GetSize();
            uint32_t ipreferredTris = 0;
            for (uint32_t i = 0; i < isize; ++i) {
                NavMeshTriangle* ptri = GetTriangle(static_cast<uint16_t>(i));
                if (ptri->IsPreferred()) {
                    ++ipreferredTris;
                }
            }
            // Both counts are divided in double precision and the quotient is stored as a float.
            if (pNavMeshInfo && isize) {
                pNavMeshInfo->SetPreferredPercent(
                    static_cast<float>(static_cast<double>(ipreferredTris) / static_cast<double>(isize))
                );
            }
        }
        if (ifileVersionNumber < 11) {
            uint32_t isize = Triangles.GetSize();
            for (uint32_t i = 0; i < isize; ++i) {
                NavMeshTriangle* ptri = GetTriangle(static_cast<uint16_t>(i));
                if (!ptri->IsPreferred() || !ptri->HasEdgeExtraInfo()) {
                    continue;
                }
                for (int32_t e = 0; e < 3; ++e) {
                    if (!ptri->EdgeHasExtraInfo(e)) {
                        continue;
                    }
                    uint16_t extraInfoIndex = ptri->GetTriOrExtraInfo(e);
                    EdgeExtraInfo* pinfo = GetExtraEdgeInfo(extraInfoIndex);
                    if (!pNavMeshInfo) {
                        continue;
                    }
                    for (uint32_t j = 0; j < pNavMeshInfo->AdjacentNavMeshes.GetSize(); ++j) {
                        if (pNavMeshInfo->AdjacentNavMeshes.GetAt(j) == pinfo->Portal.pOtherMesh) {
                            pNavMeshInfo->AdjacentNavMeshes.RemoveFast(j, 1);
                            pNavMeshInfo->PreferredAdjacentNavMeshes.Add(pinfo->Portal.pOtherMesh);
                            break;
                        }
                    }
                }
            }
        }
    }
    if ((bINIGlobalNavMeshCheck.Bool() || !apFile->GetMaster()) && !GetDelete()) {
        CheckNavMesh(false);
    }
    oldDoorPortalIndices.~BSSimpleArray<int32_t>();
    pContext->~AutoMemContext();
    return true;
}

uint32_t EdgeExtraInfoOLD::GetSaveSize() {
    return 0x2E;
}

void NavMeshSaveStruct::Endian() {
    BSEndian(ParentCellID, false);
    BSEndian(VertexCount, false);
    BSEndian(TriangleCount, false);
    BSEndian(ExtraInfoCount, false);
    BSEndian(CoverArrayCount, false);
    BSEndian(DoorPortalCount, false);
}

void BSClampUInt16(uint16_t* aVal, uint16_t aMin, uint16_t aMax) {
    if (*aVal <= aMax) {
        if (*aVal < aMin) {
            *aVal = aMin;
        }
    }
    else {
        *aVal = aMax;
    }
}

int32_t EdgeExtraInfo::GetSaveSize() {
    return Portal.GetSaveSize() + 4;
}

int32_t EdgeExtraInfo::GetSaveSizeOld() {
    return Portal.GetSaveSize();
}

void NavMeshTriangleDoorPortal::Endian() {
    BSEndian(pDoorForm, false);
    BSEndian(iOwningTriangleIndex, false);
}

void NavMesh::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::Revert(apLoadGameBuffer);
    if (pNavMeshInfo) {
        uint8_t bDisabled = TESForm::GetDisabled();
        pNavMeshInfo->SetDisabled(bDisabled);
    }
}

// A disabled navmesh disables its info entry. A deleted one is emptied without marking the info map altered: its
// altered state is put back through the virtual SetAltered.
void NavMesh::InitItem() {
    if (GetDisabled()) {
        GetNavMeshInfo()->SetDisabled(true);
    }
    if (GetDelete()) {
        bool bAltered = TES::pInstance->GetNavMeshInfoMap()->GetAltered();
        ClearNavMesh(false);
        TESForm* pInfoMap = TES::pInstance->GetNavMeshInfoMap();
        pInfoMap->SetAltered(bAltered);
    }
    else {
        CreatePOVs();
    }
}

void NavMesh::GetFormDetailedString(BSStringT<char>& aString) {
    BSStringT<char> cellString;
    TESObjectCELL* pCell = pParentCell;
    if (pCell) {
        pCell->GetFormDetailedString(cellString);
    }
    const char* pCellString = cellString.pString;
    aString.SPrintF("NavMesh (%08X) with %d Triangles in Cell %s", iFormID, Triangles.iSize, pCellString);
}

void NavMesh::SetDelete(bool abDeleted) {
    if (abDeleted != GetDelete()) {
        bool bModified = true;
        TESFile* pFile = GetFile(0);
        if (abDeleted && (!pFile || !pFile->GetMaster())) {
            bModified = false;
        }
        TESForm::SetDelete(abDeleted);
        SetAltered(bModified);
    }
    if (abDeleted) {
        NavMesh::FixPortalsFromNavMeshDeletion(GetNavMeshInfo());
        uint32_t uiFormID = GetFormID();
        TES::pInstance->GetNavMeshInfoMap()->DeleteNavMeshInfo(uiFormID);
    }
}

// Altering the navmesh also marks its parent cell altered.
[[clang::disable_tail_calls]] void NavMesh::SetAltered(bool abVal) {
    TESForm::SetAltered(abVal);
    if (abVal && pParentCell) {
        pParentCell->SetAltered(true);
    }
}

bool NavMesh::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        TESForm* pCell = GetParentCell();
        uint32_t iFormID = apGroupFORM->iFormID;
        if (iFormID == 6) {
            if (!abAllowParentGroups) {
                return false;
            }
        }
        else if (iFormID <= 7 || iFormID > 9) {
            if (abAllowParentGroups) {
                return pCell->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
            }
            return false;
        }
        if (pCell->FormIDMatchesWithoutIndex(apGroupFORM->flags)) {
            return apGroupFORM->iFormID != 8;
        }
    }
    return false;
}

void NavMesh::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
    if (apOutGroupFORM) {
        apOutGroupFORM->form = 0;
        if (apParentGroup) {
            TESObjectCELL* pparentcell = pParentCell;
            uint32_t iparentID = pParentCell->iFormID;
            uint32_t iFormID = apParentGroup->GroupData.iFormID;
            if (iFormID == 3 || iFormID == 5) {
                if (apParentGroup->GroupData.flags == pparentcell->GetGroupSubBlockKey()) {
                    apOutGroupFORM->form = formEnumString[GRUP_ID].uiFormString;
                    apOutGroupFORM->iFormID = 6;
                    apOutGroupFORM->flags = iparentID;
                    apOutGroupFORM->length = 0;
                    apOutGroupFORM->iVersionControl = 0;
                }
            }
            else if (iFormID == 6 && apParentGroup->GroupData.flags == iparentID) {
                apOutGroupFORM->form = formEnumString[GRUP_ID].uiFormString;
                apOutGroupFORM->iFormID = 9;
                apOutGroupFORM->flags = pParentCell->iFormID;
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
        }
    }
}

EdgeExtraInfo* NavMesh::GetExtraEdgeInfo(uint16_t asExtraInfoIndex) {
    return &ExtraEdgeInfo.GetAt(asExtraInfoIndex);
}

void NavMesh::ForceSetFormID(uint32_t aiID) {
    TESForm::SetFormID(aiID, false);
}

void NavMesh::MakeCopyForObstacle(NavMeshPtr& spDestNavMesh) const {
    spDestNavMesh->pParentCell = pParentCell;
    Vertices.CopyTo(&spDestNavMesh->Vertices);
    Triangles.CopyTo(&spDestNavMesh->Triangles);
    ExtraEdgeInfo.CopyTo(&spDestNavMesh->ExtraEdgeInfo);
    CoverArray.CopyTo(&spDestNavMesh->CoverArray);
    MeshGrid.CopyTo(&spDestNavMesh->MeshGrid);
    Obstacles.CopyTo(&spDestNavMesh->Obstacles);
    ClosedDoors.CopyTo(&spDestNavMesh->ClosedDoors);
    NiTMapItem<uint16_t, NiPointer<ObstacleData>>* pObstaclePos = pTriangleToObstacleMap->GetFirstPos();
    while (pObstaclePos) {
        uint16_t key;
        NiPointer<ObstacleData> spObstacle(static_cast<ObstacleData*>(nullptr));
        pTriangleToObstacleMap->GetNext(pObstaclePos, key, spObstacle);
        spDestNavMesh->pTriangleToObstacleMap->SetAt(key, spObstacle);
    }
    NiTMapItem<uint16_t, NavMeshPOVData>* pPOVPos = POVs.GetFirstPos();
    while (pPOVPos) {
        uint16_t key;
        NavMeshPOVData kPOVData;
        POVs.GetNext(pPOVPos, key, kPOVData);
        spDestNavMesh->POVs.SetAt(key, kPOVData);
    }
    ClosestPOVs.CopyTo(&spDestNavMesh->ClosestPOVs);
    ObstaclePOVs.CopyTo(&spDestNavMesh->ObstaclePOVs);
    spDestNavMesh->pNavMeshInfo = nullptr;
}

// The object count sits in bits 13 to 15 of the triangle flags. The parameter's low word is clamped in place and read
// back; the high word stays as the caller passed it.
void NavMeshTriangle::SetObjectCount(uint32_t aiCount) {
    uint16_t* pusCount = static_cast<uint16_t*>(static_cast<void*>(&aiCount));
    BSClampUInt16(pusCount, 0, 7);
    TriangleFlags = (TriangleFlags & ~NAV_TRI_OBJECT_COUNT_MASK) |
        (static_cast<uint32_t>(*pusCount) << NAV_TRI_OBJECT_COUNT_SHIFT);
}

// Swaps the flags, then the three vertex and the three triangle indices, read from a big-endian file.
void NavMeshTriangle::Endian() {
    BSEndian(TriangleFlags, false);
    BSEndian(Vertices[0], false);
    BSEndian(Vertices[1], false);
    BSEndian(Vertices[2], false);
    BSEndian(Triangles[0], false);
    BSEndian(Triangles[1], false);
    BSEndian(Triangles[2], false);
}

NavMeshVertex::NavMeshVertex() {}

// Swaps the three coordinates read from a big-endian file.
void NavMeshVertex::Endian() {
    BSEndian(Location.x, false);
    BSEndian(Location.y, false);
    BSEndian(Location.z, false);
}
