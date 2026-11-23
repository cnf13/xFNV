#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "xfnv/Compiler.h"

NavMeshInfoMap::NavMeshInfoMap() : bUpdateAll(false), InfoMap(0x2719), CellKeyNavMeshInfoMap(103), bInit(0) {
    cFormType = NAVI_ID;
}

// Empties the maps; the compiler destroys them and the form.
NavMeshInfoMap::~NavMeshInfoMap() {
    Clear();
}

bool NavMeshInfoMap::ProcessBeforeSave() {
    NiTMapItem<uint32_t, NavMeshInfo*>* it;
    if (!GetAltered()) {
        return true;
    }
    uint32_t inumAltForms = TESForm::pAlteredForms->m_uiSize;
    int32_t inavmeshes = 0;
    for (uint32_t i = 0; i < inumAltForms; ++i) {
        TESForm* pform = TESForm::pAlteredForms->GetAt(i);
        if (pform->GetFormType() == NAVM_ID) {
            ++inavmeshes;
        }
    }
    if (!inavmeshes) {
        bool bhasAlteredInfo = false;
        it = InfoMap.GetFirstPos();
        while (it) {
            uint32_t navmeshID;
            NavMeshInfo* pinfo;
            InfoMap.GetNext(it, navmeshID, pinfo);
            if (pinfo->GetAltered()) {
                bhasAlteredInfo = true;
                break;
            }
        }
        if (!bhasAlteredInfo) {
            SetAltered(false);
            return true;
        }
    }
    it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        NavMeshInfo* pinfo;
        InfoMap.GetNext(it, navmeshID, pinfo);
        NavMeshPtr spnavmesh;
        if (pinfo->GetNavMeshEvenDisabled(spnavmesh) && spnavmesh->GetAltered() && !spnavmesh->GetDelete()) {
            spnavmesh->UpdateNavMeshInfo();
        }
    }
    CheckMap();
    return true;
}

// Loads the NAVI record: an info per NVMI chunk (merged into an existing info of the same navmesh, else added) and the
// portal data of NVCI chunks, under the navmesh memory context. On the first load the infos' door pointers are turned
// back into form IDs. When the EDID chunk is empty (malformed data), the EDID buffer is initialised to "" (one zeroed
// byte, allocated only for an empty chunk).
bool NavMeshInfoMap::Load(TESFile* apFile) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    // Uses the stack context's address, not the constructor's result.
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_NAVMESH,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshInfoMap.cpp",
        92
    );
    if (bInit) {
        NiTMapItem<uint32_t, NavMeshInfo*>* pPos = InfoMap.GetFirstPos();
        while (pPos) {
            uint32_t uiKey;
            NavMeshInfo* pInfo;
            InfoMap.GetNext(pPos, uiKey, pInfo);
            for (TESObjectREFR*& rpDoor : pInfo->ConnectedDoors) {
                uint32_t uiDoorID = rpDoor->GetFormID();
                // Stores the form ID in the pointer-sized slot.
                rpDoor = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiDoorID));
            }
        }
        bInit = 0;
    }
    int32_t iSize = 0x9B0;
    char* pBuffer = static_cast<char*>(ourMalloc(0x9B0));
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != NAVI_ID) {
        pContext->~AutoMemContext();
        return false;
    }
    GetFormID();
    TESForm::LoadForm(apFile);
    int32_t iFileVersion = 1;
    do {
        switch (apFile->GetTESChunk()) {
            case EDID_ID: {
                // The buffer is allocated on the stack at the chunk's size; at most 0x200 bytes are read.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten, so it is terminated first (see above).
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case NVER_ID:
                apFile->GetChunkData(&iFileVersion, 0);
                if (apFile->QEndian()) {
                    ENDIAN(iFileVersion, false);
                }
                if (iFileVersion < 5) {
                    bUpdateAll = true;
                }
                break;
            case NVMI_ID: {
                int32_t iChunkSize = static_cast<int32_t>(apFile->GetChunkSize());
                if (iChunkSize > iSize) {
                    ourFree(pBuffer);
                    iSize = iChunkSize;
                    pBuffer = static_cast<char*>(ourMalloc(iSize));
                }
                apFile->GetChunkData(pBuffer, iChunkSize);
                NavMeshInfo* pNewInfo = new NavMeshInfo;
                pNewInfo->SetFromInfoChunk(pBuffer, apFile, iFileVersion);
                NavMeshInfo* pExisting = GetNavMeshInfo(pNewInfo->GetNavMeshID());
                if (pExisting) {
                    alignas(NavMeshPtr) uint8_t aucNavMesh[sizeof(NavMeshPtr)];
                    NavMeshPtr* pNavMesh = ::new (aucNavMesh) NavMeshPtr;
                    bool bHasNavMesh = pExisting->GetNavMeshEvenDisabled(*pNavMesh);
                    alignas(BSSimpleArray<NavMeshInfo*>) uint8_t aucAdjacent[sizeof(BSSimpleArray<NavMeshInfo*>)];
                    alignas(BSSimpleArray<NavMeshInfo*>) uint8_t aucPreferred[sizeof(BSSimpleArray<NavMeshInfo*>)];
                    alignas(BSSimpleArray<TESObjectREFR*>) uint8_t aucDoors[sizeof(BSSimpleArray<TESObjectREFR*>)];
                    BSSimpleArray<NavMeshInfo*>* pAdjacent = ::new (aucAdjacent) BSSimpleArray<NavMeshInfo*>;
                    BSSimpleArray<NavMeshInfo*>* pPreferred = ::new (aucPreferred) BSSimpleArray<NavMeshInfo*>;
                    BSSimpleArray<TESObjectREFR*>* pDoors = ::new (aucDoors) BSSimpleArray<TESObjectREFR*>;
                    // The existing info's arrays are kept across CopyTo, which overwrites them.
                    pExisting->AdjacentNavMeshes.CopyTo(pAdjacent);
                    pExisting->PreferredAdjacentNavMeshes.CopyTo(pPreferred);
                    pExisting->ConnectedDoors.CopyTo(pDoors);
                    pNewInfo->CopyTo(pExisting);
                    if (bHasNavMesh) {
                        pExisting->SetNavMesh(*pNavMesh);
                    }
                    pAdjacent->CopyTo(&pExisting->AdjacentNavMeshes);
                    pPreferred->CopyTo(&pExisting->PreferredAdjacentNavMeshes);
                    pDoors->CopyTo(&pExisting->ConnectedDoors);
                    if (pNewInfo) {
                        delete pNewInfo;
                    }
                    pDoors->~BSSimpleArray<TESObjectREFR*>();
                    pPreferred->~BSSimpleArray<NavMeshInfo*>();
                    pAdjacent->~BSSimpleArray<NavMeshInfo*>();
                    *pNavMesh = nullptr;
                }
                else {
                    InfoMap.SetAt(pNewInfo->GetNavMeshID(), pNewInfo);
                }
                break;
            }
            case NVCI_ID: {
                int32_t iChunkSize = static_cast<int32_t>(apFile->GetChunkSize());
                if (iChunkSize > iSize) {
                    ourFree(pBuffer);
                    iSize = iChunkSize;
                    pBuffer = static_cast<char*>(ourMalloc(iSize));
                }
                apFile->GetChunkData(pBuffer, iChunkSize);
                char* pChunk = pBuffer;
                // The chunk is raw bytes; its first 32-bit value is the navmesh's form ID.
                uint32_t uiNavMeshID = *reinterpret_cast<uint32_t*>(pChunk);
                if (apFile->QEndian()) {
                    ENDIAN(uiNavMeshID, false);
                }
                TESForm::AddCompileIndex(uiNavMeshID, apFile);
                pChunk += 4;
                NavMeshInfo* pInfo;
                if (!InfoMap.GetAt(uiNavMeshID, pInfo)) {
                    pContext->~AutoMemContext();
                    return false;
                }
                if (iFileVersion >= 5) {
                    pInfo->SetFromPortalChunk(pChunk, apFile, iFileVersion);
                }
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    ourFree(pBuffer);
    pContext->~AutoMemContext();
    return true;
}

void NavMeshInfoMap::Save() {
    NiTMapItem<uint32_t, NavMeshInfo*>* it;
    StartForm();
    TESForm::AddChunk(NVER_ID, 11u);
    it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        NavMeshInfo* pinfo;
        InfoMap.GetNext(it, navmeshID, pinfo);
        int32_t ichunksize = pinfo->GetInfoChunkSize();
        void* pchunkdata = ourMalloc(static_cast<uint32_t>(ichunksize));
        pinfo->FillInfoChunk(static_cast<char*>(pchunkdata));
        TESForm::__AddChunkData(NVMI_ID, pchunkdata, static_cast<uint32_t>(ichunksize));
        ourFree(pchunkdata);
    }
    it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        NavMeshInfo* pinfo;
        InfoMap.GetNext(it, navmeshID, pinfo);
        int32_t ichunksize = pinfo->GetPortalChunkSize() + 4;
        void* pchunkdata = ourMalloc(static_cast<uint32_t>(ichunksize));
        uint32_t* pchunkPointer = static_cast<uint32_t*>(pchunkdata);
        uint32_t inavmeshId = pinfo->NavMeshID;
        if (TESForm::QEndianSwapOnSave()) {
            ENDIAN<uint32_t>(inavmeshId, false);
        }
        *pchunkPointer++ = inavmeshId;
        pinfo->FillPortalChunk(reinterpret_cast<char*>(pchunkPointer));
        TESForm::__AddChunkData(NVCI_ID, pchunkdata, static_cast<uint32_t>(ichunksize));
        ourFree(pchunkdata);
    }
    CloseForm();
}

bool NavMeshInfoMap::SavesBefore(TESForm* apForm) {
    return apForm->GetFormType() == NAVM_ID || TESForm::SavesBefore(apForm);
}

void NavMeshInfoMap::InitItem() {
    NavMeshInfo* pinfo;
    using InfoArray = BSSimpleArray<NavMeshInfo*, 1024>;
    InfoArray infosToRemove;
    NiTMapItem<uint32_t, NavMeshInfo*>* it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        InfoMap.GetNext(it, navmeshID, pinfo);
        bool bnavmeshBad = false;
        TESForm* pparentSpace = TESForm::GetFormByNumericID(pinfo->ParentSpaceID);
        if (!pparentSpace) {
            Warning(
                "MASTERFILE: Could not find Parent Space with ID %08x for NavmeshInfo %08x",
                pinfo->ParentSpaceID,
                navmeshID
            );
            bnavmeshBad = true;
        }
        if (std::fabs(pinfo->ApproxLocation.z) > 100000000376832.0) {
            Warning("MASTERFILE: NavmeshInfo %08x has bad approx location, ignoring", navmeshID);
            bnavmeshBad = true;
        }
        if (bnavmeshBad) {
            infosToRemove.Add(pinfo);
        }
        else {
            pinfo->pParentSpace = pparentSpace;
            AddInfoToCellKeyMap(pinfo);
        }
        if (!bInit) {
            pinfo->InitDoorPointers();
        }
    }
    for (NavMeshInfo* pInfoToRemove : infosToRemove) {
        InfoMap.RemoveAt(pInfoToRemove->NavMeshID);
        if (pInfoToRemove) {
            delete pInfoToRemove;
        }
    }
    bInit = 1;
    CheckAllNavmeshBoundingBoxes();
}

// A cleared flag is passed on to every info (NavMeshInfo::SetAltered, empty in this build).
void NavMeshInfoMap::SetAltered(bool abVal) {
    TESForm::SetAltered(abVal);
    if (!abVal) {
        NiTMapItem<uint32_t, NavMeshInfo*>* pPos = InfoMap.GetFirstPos();
        while (pPos) {
            uint32_t uiKey;
            NavMeshInfo* pInfo;
            InfoMap.GetNext(pPos, uiKey, pInfo);
            pInfo->SetAltered(false);
        }
    }
}

void NavMeshInfoMap::DeleteNavMeshInfo(uint32_t auiNavMeshID) {
    NavMeshInfo* pinfo;
    if (InfoMap.GetAt(auiNavMeshID, pinfo)) {
        RemoveInfoFromCellKeyMap(pinfo);
        bool bMaster = false;
        NavMeshPtr spnavmesh;
        if (pinfo->GetNavMeshEvenDisabled(spnavmesh)) {
            TESForm* pform = spnavmesh.m_pObject;
            TESFile* pfile = pform->GetFile(0);
            if (pfile && pfile->GetMaster()) {
                bMaster = true;
            }
        }
        if (pinfo) {
            delete pinfo;
        }
        InfoMap.RemoveAt(auiNavMeshID);
        if (bMaster) {
            SetAltered(true);
        }
    }
}

NavMeshInfo* NavMeshInfoMap::GetNavMeshInfo(uint32_t auiNavMeshID) {
    NavMeshInfo* pinfo;
    if (InfoMap.GetAt(auiNavMeshID, pinfo)) {
        return pinfo;
    }

    return nullptr;
}

void NavMeshInfoMap::RemoveNavMeshInfo(uint32_t auiNavMeshID) {
    NavMeshInfo* pinfo;
    if (InfoMap.GetAt(auiNavMeshID, pinfo)) {
        InfoMap.RemoveAt(auiNavMeshID);
        if (!CellKeyNavMeshInfoMap.IsEmpty()) {
            RemoveInfoFromCellKeyMap(pinfo);
        }
        SetAltered(true);
    }
}

void NavMeshInfoMap::AddNavMeshInfo(NavMeshInfo* apNavMeshInfo) {
    InfoMap.SetAt(apNavMeshInfo->GetNavMeshID(), apNavMeshInfo);
    AddInfoToCellKeyMap(apNavMeshInfo);
    SetAltered(true);
}

[[clang::disable_tail_calls]] const BSSimpleArray<NavMeshInfo*, 1024>*
NavMeshInfoMap::FindNavMeshInfoForCell(TESObjectCELL* apCell) {
    if (!apCell) {
        return nullptr;
    }
    if (apCell->GetInterior()) {
        return FindNavMeshInfoForInteriorCell(apCell);
    }
    int32_t iY = apCell->GetDataY();
    int32_t iX = apCell->GetDataX();
    int32_t iCellKey = TESObjectCELL::GetCoord(iX, iY);
    return FindNavMeshInfoForCellKey(apCell->GetWorldSpace(), iCellKey);
}

const BSSimpleArray<NavMeshInfo*, 1024>*
NavMeshInfoMap::FindNavMeshInfoForCellKey(TESWorldSpace* apWorldSpace, int32_t aiCellKey) {
    CellKeyInfoMap* pCellKeyMap;
    if (!CellKeyNavMeshInfoMap.GetAt(apWorldSpace->GetFormID(), pCellKeyMap)) {
        return nullptr;
    }
    BSSimpleArray<NavMeshInfo*, 1024>* pInfos;
    if (!pCellKeyMap->GetAt(static_cast<uint32_t>(aiCellKey), pInfos)) {
        return nullptr;
    }
    return pInfos;
}

// The interiors' map is entry 0; the key is the cell's address as an integer.
const BSSimpleArray<NavMeshInfo*, 1024>* NavMeshInfoMap::FindNavMeshInfoForInteriorCell(TESObjectCELL* apCell) {
    CellKeyInfoMap* pCellKeyMap;
    if (!CellKeyNavMeshInfoMap.GetAt(0, pCellKeyMap)) {
        return nullptr;
    }
    BSSimpleArray<NavMeshInfo*, 1024>* pInfos;
    if (!pCellKeyMap->GetAt(reinterpret_cast<uintptr_t>(apCell), pInfos)) {
        return nullptr;
    }
    return pInfos;
}

// A new cell key's array is constructed as the info array, with its own vtable, in untyped memory and stored in the
// cell key map.
void NavMeshInfoMap::AddInfoToCellKeyMap(NavMeshInfo* apNavMeshInfo) {
    CellKeyInfoMap* themap;
    BSSimpleArray<NavMeshInfo*, 1024>* thearray;
    uintptr_t icellKey = NavMeshInfo::GetCellOrCoords(apNavMeshInfo);
    uint32_t worldspaceId = apNavMeshInfo->GetInteriorCell() ? 0 : apNavMeshInfo->ParentSpaceID;
    if (!CellKeyNavMeshInfoMap.GetAt(worldspaceId, themap)) {
        uint32_t imapSize = 103;
        if (worldspaceId) {
            TESWorldSpace* pworld = static_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(worldspaceId));
            if (pworld && !pworld->GetSmallWorld()) {
                imapSize = 1009;
            }
        }
        else {
            imapSize = 1009;
        }
        themap = ::new CellKeyInfoMap(imapSize);
        CellKeyNavMeshInfoMap.SetAt(worldspaceId, themap);
    }
    if (!themap->GetAt(icellKey, thearray)) {
        thearray = new BSSimpleArray<NavMeshInfo*, 1024>;
        themap->SetAt(icellKey, thearray);
    }
    if (thearray->Find(apNavMeshInfo, 0, &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1) {
        thearray->Add(apNavMeshInfo);
    }
}

// Removes the infos with no parent space, no navmesh or a deleted one, drops portals to missing infos, and rebuilds an
// info whose portals and doors disagree with its navmesh's.
void NavMeshInfoMap::CheckMap() {
    using trianglesWithPortals_t = BSSimpleArray<int32_t, 1024>;
    using potentialAdjacentInfos_t = BSSimpleArray<NavMeshInfo*, 1024>;
    NavMeshInfo* pinfo;
    using infosToRemove_t = BSSimpleArray<NavMeshInfo*, 1024>;
    infosToRemove_t infosToRemove;
    NiTMapItem<uint32_t, NavMeshInfo*>* it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        InfoMap.GetNext(it, navmeshID, pinfo);
        if (pinfo->GetParentSpace()) {
            if (pinfo->HasNavMesh()) {
                NavMeshPtr spNavMesh;
                if (pinfo->GetNavMeshEvenDisabled(spNavMesh)) {
                    if (spNavMesh->GetDelete()) {
                        infosToRemove.Add(pinfo);
                    }
                    for (uint32_t i = 0; i < pinfo->AdjacentNavMeshes.iSize; ++i) {
                        bool bremove = false;
                        if (pinfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i))) {
                            if (!GetNavMeshInfo(pinfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i))->NavMeshID)) {
                                bremove = true;
                            }
                        }
                        else {
                            bremove = true;
                        }
                        if (bremove) {
                            const char* pcellName;
                            if (pinfo->GetCell()) {
                                pcellName = pinfo->GetCell()->GetFormEditorID();
                            }
                            else {
                                pcellName = "";
                            }
                            Warning(
                                "PATHFINDING: NavMeshInfo %08x in cell %s, has a portal with no navmesh, removing "
                                "the portal",
                                pinfo->NavMeshID,
                                pcellName
                            );
                            pinfo->AdjacentNavMeshes.Remove(i--, true);
                        }
                    }
                    for (uint32_t i = 0; i < pinfo->PreferredAdjacentNavMeshes.iSize; ++i) {
                        bool bremove = false;
                        if (pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i))) {
                            if (!GetNavMeshInfo(
                                    pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i))->NavMeshID
                                )) {
                                bremove = true;
                            }
                        }
                        else {
                            bremove = true;
                        }
                        if (bremove) {
                            const char* pcellName;
                            if (pinfo->GetCell()) {
                                pcellName = pinfo->GetCell()->GetFormEditorID();
                            }
                            else {
                                pcellName = "";
                            }
                            Warning(
                                "PATHFINDING: NavMeshInfo %08x in cell %s, has a portal with no navmesh, removing "
                                "the portal",
                                pinfo->NavMeshID,
                                pcellName
                            );
                            pinfo->PreferredAdjacentNavMeshes.Remove(i--, true);
                        }
                    }
                    potentialAdjacentInfos_t potentialAdjacentInfos;
                    if (pinfo->GetWorldSpace()) {
                        int16_t thisX;
                        int16_t thisY;
                        TESWorldSpace::GetCellCoordForKey(static_cast<uint32_t>(pinfo->iCellKey), thisX, thisY);
                        for (int32_t x = thisX - 1; x <= thisX + 1; ++x) {
                            for (int32_t y = thisY - 1; y <= thisY + 1; ++y) {
                                if (x != thisX || y != thisY) {
                                    int32_t iCellKey = TESObjectCELL::GetCoord(
                                        static_cast<uint16_t>(x),
                                        static_cast<uint16_t>(y)
                                    );
                                    const BSSimpleArray<NavMeshInfo*, 1024>* pinfos = FindNavMeshInfoForCellKey(
                                        pinfo->GetWorldSpace(),
                                        iCellKey
                                    );
                                    if (pinfos) {
                                        for (uint32_t i = 0; i < pinfos->iSize; ++i) {
                                            potentialAdjacentInfos.Add((*pinfos)[i]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    NavMeshPtr spMesh;
                    if (pinfo->GetNavMeshEvenDisabled(spMesh)) {
                        bool bShouldRegenerateInfo = false;
                        for (uint32_t i = 0; i < pinfo->AdjacentNavMeshes.iSize; ++i) {
                            NavMeshInfo* potherMesh = pinfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i));
                            trianglesWithPortals_t trianglesWithPortals;
                            spMesh->FindPortalTriangles(potherMesh, &trianglesWithPortals);
                            if (trianglesWithPortals.IsEmpty()) {
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x has a portal to NavMeshInfo %08x, but there is "
                                    "no triangle portal between the two, updating the NavMeshInfo",
                                    pinfo->NavMeshID,
                                    potherMesh->NavMeshID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        for (uint32_t i = 0; i < pinfo->PreferredAdjacentNavMeshes.iSize; ++i) {
                            NavMeshInfo* potherMesh = pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i));
                            trianglesWithPortals_t trianglesWithPortals;
                            spMesh->FindPortalTriangles(potherMesh, &trianglesWithPortals);
                            if (trianglesWithPortals.IsEmpty()) {
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x has a portal to NavMeshInfo %08x, but there is "
                                    "no triangle portal between the two, updating the NavMeshInfo",
                                    pinfo->NavMeshID,
                                    potherMesh->NavMeshID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        uint32_t isize = spMesh->GetExtraEdgeInfoCount();
                        for (uint32_t i = 0; i < isize; ++i) {
                            EdgeExtraInfo* pextraEdgeInfo = spMesh->GetExtraEdgeInfo(i);
                            if (pextraEdgeInfo->Portal.IsValid()) {
                                NavMeshInfo* potherMesh = spMesh->GetExtraEdgeInfo(i)->Portal.pOtherMesh;
                                bool bmissingPortal = true;
                                uint32_t iotherSize = pinfo->AdjacentNavMeshes.iSize;
                                for (uint32_t k = 0; k < iotherSize; ++k) {
                                    if (pinfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(k)) == potherMesh) {
                                        bmissingPortal = false;
                                        break;
                                    }
                                }
                                iotherSize = pinfo->PreferredAdjacentNavMeshes.iSize;
                                for (uint32_t k = 0; k < iotherSize; ++k) {
                                    if (pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(k)) ==
                                        potherMesh) {
                                        bmissingPortal = false;
                                        break;
                                    }
                                }
                                if (bmissingPortal &&
                                    potentialAdjacentInfos.Find(
                                        potherMesh,
                                        0,
                                        &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress
                                    ) != -1) {
                                    uint32_t uiOtherID = potherMesh->NavMeshID;
                                    Warning(
                                        "PATHFINDING: NavMeshInfo %08x does not have a portal to NavMeshInfo %08x, "
                                        "but the navmesh does, updating the NavMeshInfo",
                                        pinfo->NavMeshID,
                                        uiOtherID
                                    );
                                    bShouldRegenerateInfo = true;
                                    break;
                                }
                            }
                        }
                        for (uint32_t i = 0; i < spMesh->GetNumDoorPortals(); ++i) {
                            NavMeshTriangleDoorPortal* pdoorPortal = spMesh->GetDoorPortalFromArray(i);
                            if (pinfo->ConnectedDoors.Find(
                                    pdoorPortal->pDoorForm,
                                    0,
                                    &BSSimpleArray<TESObjectREFR*>::DefaultEqualFunctionByAddress
                                ) == -1) {
                                uint32_t uiDoorID = pdoorPortal->pDoorForm->iFormID;
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x does not have a Door portal for door %08x, but "
                                    "the navmesh does, updating the NavMeshInfo",
                                    pinfo->NavMeshID,
                                    uiDoorID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        for (TESObjectREFR* pdoor : pinfo->ConnectedDoors) {
                            bool bhasPortal = false;
                            for (uint32_t j = 0; j < spMesh->GetNumDoorPortals(); ++j) {
                                if (spMesh->GetDoorPortalFromArray(j)->pDoorForm == pdoor) {
                                    bhasPortal = true;
                                    break;
                                }
                            }
                            if (!bhasPortal) {
                                uint32_t uiDoorID = pdoor->iFormID;
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x has a portal to Door %08x, but the navmesh does "
                                    "not, updating the NavMeshInfo",
                                    pinfo->NavMeshID,
                                    uiDoorID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        if (bShouldRegenerateInfo) {
                            pinfo->CreateFromNavMesh(spMesh.m_pObject);
                        }
                    }
                }
                else {
                    const char* pcellName;
                    if (pinfo->GetCell()) {
                        pcellName = pinfo->GetCell()->GetFormEditorID();
                    }
                    else {
                        pcellName = "";
                    }
                    Warning(
                        "PATHFINDING: NavMeshInfo %08x in cell %s, has a navmesh that is marked deleted, removing "
                        "it from the map",
                        pinfo->NavMeshID,
                        pcellName
                    );
                    infosToRemove.Add(pinfo);
                }
            }
            else {
                const char* pcellName;
                if (pinfo->GetCell()) {
                    pcellName = pinfo->GetCell()->GetFormEditorID();
                }
                else {
                    pcellName = "";
                }
                Warning(
                    "PATHFINDING: NavMeshInfo %08x in cell %s, has no matching navmesh, removing it from the map",
                    pinfo->NavMeshID,
                    pcellName
                );
                NavMesh::FixPortalsFromNavMeshDeletion(pinfo);
                infosToRemove.Add(pinfo);
            }
        }
        else {
            const char* pcellName;
            if (pinfo->GetCell()) {
                pcellName = pinfo->GetCell()->GetFormEditorID();
            }
            else {
                pcellName = "";
            }
            Warning(
                "PATHFINDING: NavMeshInfo %08x in cell %s, has no parent space, removing it from the map",
                pinfo->NavMeshID,
                pcellName
            );
            infosToRemove.Add(pinfo);
        }
    }
    for (NavMeshInfo* pInfoToRemove : infosToRemove) {
        RemoveNavMeshInfo(pInfoToRemove->NavMeshID);
        if (pInfoToRemove) {
            delete pInfoToRemove;
        }
    }
}

bool NavMeshInfoMap::CheckAllNavmeshBoundingBoxes() {
    bool bAllBounded = true;
    NiTMapItem<uint32_t, CellKeyInfoMap*>* worldIt = CellKeyNavMeshInfoMap.GetFirstPos();
    while (worldIt) {
        uint32_t worldspaceId;
        CellKeyInfoMap* pcellKeyMap;
        CellKeyNavMeshInfoMap.GetNext(worldIt, worldspaceId, pcellKeyMap);
        NiTMapItem<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>* cellIt = pcellKeyMap->GetFirstPos();
        while (cellIt) {
            uintptr_t icellkey;
            BSSimpleArray<NavMeshInfo*, 1024>* parray;
            pcellKeyMap->GetNext(cellIt, icellkey, parray);
            if (parray->GetSize() > 1) {
                uint32_t uiBounded = 0;
                for (NavMeshInfo* pInfo : *parray) {
                    if (pInfo->pBounds) {
                        ++uiBounded;
                    }
                }
                if (uiBounded < parray->GetSize() - 1) {
                    bAllBounded = false;
                }
            }
        }
    }
    return bAllBounded;
}

void NavMeshInfoMap::Clear() {
    NiTMapItem<uint32_t, NavMeshInfo*>* it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        NavMeshInfo* pinfo;
        InfoMap.GetNext(it, navmeshID, pinfo);
        if (pinfo) {
            delete pinfo;
        }
    }
    InfoMap.RemoveAll();
    NiTMapItem<uint32_t, CellKeyInfoMap*>* worldIt = CellKeyNavMeshInfoMap.GetFirstPos();
    while (worldIt) {
        uint32_t worldspaceId;
        CellKeyInfoMap* pcellKeyMap;
        CellKeyNavMeshInfoMap.GetNext(worldIt, worldspaceId, pcellKeyMap);
        NiTMapItem<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>* cellIt = pcellKeyMap->GetFirstPos();
        while (cellIt) {
            uintptr_t icellkey;
            BSSimpleArray<NavMeshInfo*, 1024>* parray;
            pcellKeyMap->GetNext(cellIt, icellkey, parray);
            delete parray;
        }
        delete pcellKeyMap;
    }
    CellKeyNavMeshInfoMap.RemoveAll();
    SetAltered(true);
}

void NavMeshInfoMap::GetAllNavMeshInfo(BSSimpleArray<NavMeshInfo*, 1024>* apReturnArray) {
    NiTMapItem<uint32_t, NavMeshInfo*>* it;
    apReturnArray->SetSize(0, false);
    apReturnArray->SetReservedSize(2048u);
    it = InfoMap.GetFirstPos();
    while (it) {
        uint32_t navmeshID;
        NavMeshInfo* pinfo;
        InfoMap.GetNext(it, navmeshID, pinfo);
        apReturnArray->Add(pinfo);
    }
}

void NavMeshInfoMap::RemoveInfoFromCellKeyMap(NavMeshInfo* apNavMeshInfo) {
    CellKeyInfoMap* themap;
    BSSimpleArray<NavMeshInfo*, 1024>* thearray;
    uintptr_t icellKey = NavMeshInfo::GetCellOrCoords(apNavMeshInfo);
    uint32_t worldspaceId = apNavMeshInfo->GetInteriorCell() ? 0 : apNavMeshInfo->ParentSpaceID;
    if (CellKeyNavMeshInfoMap.GetAt(worldspaceId, themap) && themap->GetAt(icellKey, thearray)) {
        int32_t index = thearray->Find(apNavMeshInfo, 0, &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress);
        if (index != -1) {
            thearray->Remove(index, true);
            if (thearray->IsEmpty()) {
                themap->RemoveAt(icellKey);
                if (thearray) {
                    delete thearray;
                }
                if (themap->IsEmpty()) {
                    CellKeyNavMeshInfoMap.RemoveAt(worldspaceId);
                    if (themap) {
                        delete themap;
                    }
                }
            }
        }
    }
}
