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

NavMeshInfoMap::NavMeshInfoMap() : bUpdateAll(false), InfoMap(0x2719), CellKeyNavMeshInfoMap(103), bInit(0) {
    cFormType = NAVI_ID;
}

// Empties the maps; the compiler destroys them and the form.
NavMeshInfoMap::~NavMeshInfoMap() {
    Clear();
}

bool NavMeshInfoMap::ProcessBeforeSave() {
    bool bhasAlteredInfo;
    NiTMapItem<uint32_t, NavMeshInfo*>* it;
    NiTMapItem<uint32_t, NavMeshInfo*>* it_2;
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
    if (inavmeshes) {
        goto LABEL_16;
    }
    bhasAlteredInfo = false;
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
    if (bhasAlteredInfo) {
    LABEL_16:;
        it_2 = InfoMap.GetFirstPos();
        while (it_2) {
            uint32_t navmeshID_2;
            NavMeshInfo* pinfo_2;
            InfoMap.GetNext(it_2, navmeshID_2, pinfo_2);
            NavMeshPtr spnavmesh;
            if (pinfo_2->GetNavMeshEvenDisabled(spnavmesh) && spnavmesh.m_pObject->GetAltered() &&
                !spnavmesh.m_pObject->GetDelete()) {
                spnavmesh.m_pObject->UpdateNavMeshInfo();
            }
        }
        CheckMap();
        return true;
    }

    SetAltered(false);
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
        MEM_CONTEXT_NAVMESH, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshInfoMap.cpp", 92
    );
    if (bInit) {
        NiTMapItem<uint32_t, NavMeshInfo*>* pPos = InfoMap.GetFirstPos();
        while (pPos) {
            uint32_t uiKey;
            NavMeshInfo* pInfo;
            InfoMap.GetNext(pPos, uiKey, pInfo);
            for (uint32_t i = 0; i < pInfo->ConnectedDoors.GetSize(); ++i) {
                uint32_t uiDoorID = pInfo->ConnectedDoors.GetAt(i)->GetFormID();
                // Stores the form ID in the pointer-sized slot.
                pInfo->ConnectedDoors.GetAt(i) = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiDoorID));
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
                char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
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
                void* pMemory = ourMalloc(sizeof(NavMeshInfo));
                NavMeshInfo* pNewInfo = pMemory ? ::new (pMemory) NavMeshInfo : nullptr;
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
                        pExisting->SetNavMesh(pNavMesh->operator->());
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
        uint32_t navmeshID_2;
        NavMeshInfo* pinfo_2;
        InfoMap.GetNext(it, navmeshID_2, pinfo_2);
        int32_t ichunksize_2 = pinfo_2->GetPortalChunkSize() + 4;
        void* pchunkdata_2 = ourMalloc(static_cast<uint32_t>(ichunksize_2));
        uint32_t* pchunkPointer = static_cast<uint32_t*>(pchunkdata_2);
        uint32_t inavmeshId = pinfo_2->NavMeshID;
        if (TESForm::QEndianSwapOnSave()) {
            ENDIAN<uint32_t>(inavmeshId, false);
        }
        *pchunkPointer++ = inavmeshId;
        pinfo_2->FillPortalChunk(reinterpret_cast<char*>(pchunkPointer));
        TESForm::__AddChunkData(NVCI_ID, pchunkdata_2, static_cast<uint32_t>(ichunksize_2));
        ourFree(pchunkdata_2);
    }
    CloseForm();
}

bool NavMeshInfoMap::SavesBefore(TESForm* apForm) {
    return (bool)((uint8_t)((((int32_t)apForm->GetFormType()) == 67) || TESForm::SavesBefore(apForm)));
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
                "MASTERFILE: Could not find Parent Space with ID %08x for NavmeshInfo %08x", pinfo->ParentSpaceID,
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
    for (uint32_t i = 0; i < infosToRemove.GetSize(); ++i) {
        InfoMap.RemoveAt(infosToRemove.GetAt(static_cast<int32_t>(i))->NavMeshID);
        NavMeshInfo* apThis = infosToRemove.GetAt(static_cast<int32_t>(i));
        if (apThis) {
            delete apThis;
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
    XFNV_INLINED_LOCAL(NiPointer<NavMesh>, apMemory, std::nullopt, std::nullopt);
    NavMeshInfo* a2;
    if (InfoMap.GetAt(auiNavMeshID, a2)) {
        RemoveInfoFromCellKeyMap(a2);
        bool bMaster = false;
        ::new (&apMemory) NavMeshPtr;
        if (a2->GetNavMeshEvenDisabled(*static_cast<NavMeshPtr*>(&apMemory))) {
            TESForm* v2 = apMemory.m_pObject;
            TESFile* File = v2->GetFile(0);
            if ((File) && (File->GetMaster())) {
                bMaster = true;
            }
        }
        if (a2) {
            delete a2;
        }
        InfoMap.RemoveAt(auiNavMeshID);
        if (bMaster) {
            SetAltered(true);
        }
        apMemory.~NiPointer<NavMesh>();
    }
}

NavMeshInfo* NavMeshInfoMap::GetNavMeshInfo(uint32_t auiNavMeshID) {
    NavMeshInfo* a2;
    bool vCondition1;
    vCondition1 = (&InfoMap)->GetAt(auiNavMeshID, a2);
    if (vCondition1) {
        return a2;
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
    uint32_t v2 = apNavMeshInfo->GetNavMeshID();
    (&InfoMap)->SetAt((int32_t)v2, apNavMeshInfo);
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
    uint32_t worldspaceId;
    BSSimpleArray<NavMeshInfo*, 1024>* thearray;
    uintptr_t icellKey = NavMeshInfo::GetCellOrCoords(apNavMeshInfo);
    if (apNavMeshInfo->GetInteriorCell()) {
        worldspaceId = 0;
    }
    else {
        worldspaceId = apNavMeshInfo->ParentSpaceID;
    }
    if (!CellKeyNavMeshInfoMap.GetAt(worldspaceId, themap)) {
        CellKeyInfoMap* themap_2;
        uint32_t imapSize = 103;
        if (worldspaceId) {
            TESWorldSpace* pworld = static_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(worldspaceId));
            if ((pworld) && (!pworld->GetSmallWorld())) {
                imapSize = 1009;
            }
        }
        else {
            imapSize = 1009;
        }
        CellKeyInfoMap* apMemory = static_cast<CellKeyInfoMap*>(ourMalloc(sizeof(CellKeyInfoMap)));
        if (apMemory) {
            themap_2 = ::new (apMemory) CellKeyInfoMap(imapSize);
        }
        else {
            themap_2 = nullptr;
        }
        themap = themap_2;
        CellKeyNavMeshInfoMap.SetAt(worldspaceId, themap_2);
    }
    if (!themap->GetAt(icellKey, thearray)) {
        BSSimpleArray<NavMeshInfo*, 1024>* thearray_2;
        BSSimpleArray<NavMeshInfo*, 1024>* v5 =
            static_cast<BSSimpleArray<NavMeshInfo*, 1024>*>(ourMalloc(sizeof(BSSimpleArray<NavMeshInfo*, 1024>)));
        if (v5) {
            thearray_2 = ::new (v5) BSSimpleArray<NavMeshInfo*, 1024>;
        }
        else {
            thearray_2 = nullptr;
        }
        thearray = thearray_2;
        themap->SetAt(icellKey, thearray);
    }
    if (thearray->Find(apNavMeshInfo, 0, &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1) {
        thearray->Add(apNavMeshInfo);
    }
}

// Removes the infos with no parent space, no navmesh or a deleted one, drops portals to missing infos, and rebuilds an
// info whose portals and doors disagree with its navmesh's.
void NavMeshInfoMap::CheckMap() {
    NavMeshInfo* potherMesh_3;
    using trianglesWithPortals_2_t = BSSimpleArray<int32_t, 1024>;
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
                    if (spNavMesh.m_pObject->GetDelete()) {
                        infosToRemove.Add(pinfo);
                    }
                    for (uint32_t i = 0;; ++i) {
                        uint32_t uiSize = pinfo->AdjacentNavMeshes.iSize;
                        if (i >= uiSize) {
                            break;
                        }
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
                            const char* v20;
                            if (pinfo->GetCell()) {
                                TESObjectCELL* Cell_0 = pinfo->GetCell();
                                v20 = Cell_0->GetFormEditorID();
                            }
                            else {
                                v20 = "";
                            }
                            Warning(
                                "PATHFINDING: NavMeshInfo %08x in cell %s, has a portal with no navmesh, removing "
                                "the portal",
                                pinfo->NavMeshID, v20
                            );
                            pinfo->AdjacentNavMeshes.Remove(i--, true);
                        }
                    }
                    for (uint32_t i_2 = 0;; ++i_2) {
                        uint32_t v2 = pinfo->PreferredAdjacentNavMeshes.iSize;
                        if (i_2 >= v2) {
                            break;
                        }
                        bool bremove_2 = false;
                        if (pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i_2))) {
                            if (!GetNavMeshInfo(
                                    pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i_2))->NavMeshID
                                )) {
                                bremove_2 = true;
                            }
                        }
                        else {
                            bremove_2 = true;
                        }
                        if (bremove_2) {
                            const char* v18;
                            if (pinfo->GetCell()) {
                                TESObjectCELL* v19 = pinfo->GetCell();
                                v18 = v19->GetFormEditorID();
                            }
                            else {
                                v18 = "";
                            }
                            Warning(
                                "PATHFINDING: NavMeshInfo %08x in cell %s, has a portal with no navmesh, removing "
                                "the portal",
                                pinfo->NavMeshID, v18
                            );
                            pinfo->PreferredAdjacentNavMeshes.Remove(i_2--, true);
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
                                    int32_t Coord =
                                        TESObjectCELL::GetCoord(static_cast<uint16_t>(x), static_cast<uint16_t>(y));
                                    TESWorldSpace* WorldSpace = pinfo->GetWorldSpace();
                                    const BSSimpleArray<NavMeshInfo*, 1024>* pinfos =
                                        FindNavMeshInfoForCellKey(WorldSpace, Coord);
                                    if (pinfos) {
                                        for (uint32_t i_3 = 0;; ++i_3) {
                                            uint32_t v4 = pinfos->iSize;
                                            if (i_3 >= v4) {
                                                break;
                                            }
                                            potentialAdjacentInfos.Add((*pinfos)[i_3]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    NavMeshPtr spMesh;
                    if (pinfo->GetNavMeshEvenDisabled(spMesh)) {
                        bool bShouldRegenerateInfo = false;
                        for (uint32_t i_4 = 0;; ++i_4) {
                            uint32_t v5 = pinfo->AdjacentNavMeshes.iSize;
                            if (i_4 >= v5) {
                                break;
                            }
                            NavMeshInfo* potherMesh = pinfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(i_4));
                            trianglesWithPortals_t trianglesWithPortals;
                            spMesh.m_pObject->FindPortalTriangles(potherMesh, &trianglesWithPortals);
                            if (trianglesWithPortals.IsEmpty()) {
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x has a portal to NavMeshInfo %08x, but there is "
                                    "no triangle portal between the two, updating the NavMeshInfo",
                                    pinfo->NavMeshID, potherMesh->NavMeshID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        for (uint32_t i_5 = 0;; ++i_5) {
                            uint32_t v6 = pinfo->PreferredAdjacentNavMeshes.iSize;
                            if (i_5 >= v6) {
                                break;
                            }
                            NavMeshInfo* potherMesh_2 =
                                pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i_5));
                            trianglesWithPortals_2_t trianglesWithPortals_2;
                            spMesh.m_pObject->FindPortalTriangles(potherMesh_2, &trianglesWithPortals_2);
                            if (trianglesWithPortals_2.IsEmpty()) {
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x has a portal to NavMeshInfo %08x, but there is "
                                    "no triangle portal between the two, updating the NavMeshInfo",
                                    pinfo->NavMeshID, potherMesh_2->NavMeshID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        uint32_t isize = spMesh.m_pObject->GetExtraEdgeInfoCount();
                        for (uint32_t i_6 = 0; i_6 < isize; ++i_6) {
                            EdgeExtraInfo* ExtraEdgeInfo = spMesh.m_pObject->GetExtraEdgeInfo(i_6);
                            if (ExtraEdgeInfo->Portal.IsValid()) {
                                potherMesh_3 = spMesh.m_pObject->GetExtraEdgeInfo(i_6)->Portal.pOtherMesh;
                                bool bmissingPortal = true;
                                uint32_t iotherSize = pinfo->AdjacentNavMeshes.iSize;
                                for (uint32_t k = 0; k < iotherSize; ++k) {
                                    NavMeshInfo* v8 = pinfo->AdjacentNavMeshes.GetAt(static_cast<int32_t>(k));
                                    if (v8 == potherMesh_3) {
                                        bmissingPortal = false;
                                        break;
                                    }
                                }
                                iotherSize = pinfo->PreferredAdjacentNavMeshes.iSize;
                                for (uint32_t k_2 = 0; k_2 < iotherSize; ++k_2) {
                                    NavMeshInfo** v9 =
                                        &pinfo->PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(k_2));
                                    if (*v9 == potherMesh_3) {
                                        bmissingPortal = false;
                                        break;
                                    }
                                }
                                if (bmissingPortal &&
                                    potentialAdjacentInfos.Find(
                                        potherMesh_3, 0, &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress
                                    ) != -1) {
                                    uint32_t uiOtherID = potherMesh_3->NavMeshID;
                                    Warning(
                                        "PATHFINDING: NavMeshInfo %08x does not have a portal to NavMeshInfo %08x, "
                                        "but the navmesh does, updating the NavMeshInfo",
                                        pinfo->NavMeshID, uiOtherID
                                    );
                                    bShouldRegenerateInfo = true;
                                    break;
                                }
                            }
                        }
                        for (uint32_t i_7 = 0; i_7 < spMesh.m_pObject->GetNumDoorPortals(); ++i_7) {
                            NavMeshTriangleDoorPortal* pdoorPortal = spMesh.m_pObject->GetDoorPortalFromArray(i_7);
                            if (pinfo->ConnectedDoors.Find(
                                    pdoorPortal->pDoorForm, 0,
                                    &BSSimpleArray<TESObjectREFR*>::DefaultEqualFunctionByAddress
                                ) == -1) {
                                uint32_t uiDoorID = pdoorPortal->pDoorForm->iFormID;
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x does not have a Door portal for door %08x, but "
                                    "the navmesh does, updating the NavMeshInfo",
                                    pinfo->NavMeshID, uiDoorID
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        for (uint32_t i_8 = 0; i_8 < pinfo->ConnectedDoors.iSize; ++i_8) {
                            TESObjectREFR* pdoor = pinfo->ConnectedDoors.GetAt(static_cast<int32_t>(i_8));
                            bool bhasPortal = false;
                            for (uint32_t j = 0; j < spMesh.m_pObject->GetNumDoorPortals(); ++j) {
                                if (spMesh.m_pObject->GetDoorPortalFromArray(j)->pDoorForm == pdoor) {
                                    bhasPortal = true;
                                    break;
                                }
                            }
                            if (!bhasPortal) {
                                uint32_t v11 = pdoor->iFormID;
                                Warning(
                                    "PATHFINDING: NavMeshInfo %08x has a portal to Door %08x, but the navmesh does "
                                    "not, updating the NavMeshInfo",
                                    pinfo->NavMeshID, v11
                                );
                                bShouldRegenerateInfo = true;
                                break;
                            }
                        }
                        if (bShouldRegenerateInfo) {
                            NavMesh* Object = spMesh.m_pObject;
                            pinfo->CreateFromNavMesh(Object);
                        }
                    }
                }
                else {
                    const char* v22;
                    if (pinfo->GetCell()) {
                        TESObjectCELL* v23 = pinfo->GetCell();
                        v22 = v23->GetFormEditorID();
                    }
                    else {
                        v22 = "";
                    }
                    Warning(
                        "PATHFINDING: NavMeshInfo %08x in cell %s, has a navmesh that is marked deleted, removing "
                        "it from the map",
                        pinfo->NavMeshID, v22
                    );
                    infosToRemove.Add(pinfo);
                }
            }
            else {
                const char* v24;
                if (pinfo->GetCell()) {
                    TESObjectCELL* v25 = pinfo->GetCell();
                    v24 = v25->GetFormEditorID();
                }
                else {
                    v24 = "";
                }
                Warning(
                    "PATHFINDING: NavMeshInfo %08x in cell %s, has no matching navmesh, removing it from the map",
                    pinfo->NavMeshID, v24
                );
                NavMesh::FixPortalsFromNavMeshDeletion(pinfo);
                infosToRemove.Add(pinfo);
            }
        }
        else {
            const char* v26;
            if (pinfo->GetCell()) {
                TESObjectCELL* v27 = pinfo->GetCell();
                v26 = v27->GetFormEditorID();
            }
            else {
                v26 = "";
            }
            Warning(
                "PATHFINDING: NavMeshInfo %08x in cell %s, has no parent space, removing it from the map",
                pinfo->NavMeshID, v26
            );
            infosToRemove.Add(pinfo);
        }
    }
    for (uint32_t i_9 = 0; i_9 < infosToRemove.iSize; ++i_9) {
        RemoveNavMeshInfo(infosToRemove.GetAt(static_cast<int32_t>(i_9))->NavMeshID);
        NavMeshInfo* apThis = infosToRemove.GetAt(static_cast<int32_t>(i_9));
        if (apThis) {
            delete apThis;
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
                for (uint32_t i = 0; i < parray->GetSize(); ++i) {
                    if ((*parray)[i]->pBounds) {
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
    uint32_t worldspaceId;
    BSSimpleArray<NavMeshInfo*, 1024>* thearray;
    uintptr_t icellKey = NavMeshInfo::GetCellOrCoords(apNavMeshInfo);
    if (apNavMeshInfo->GetInteriorCell()) {
        worldspaceId = 0;
    }
    else {
        worldspaceId = apNavMeshInfo->ParentSpaceID;
    }
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
