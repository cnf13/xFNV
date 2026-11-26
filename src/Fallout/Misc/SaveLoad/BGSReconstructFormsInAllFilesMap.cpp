#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/VATSTarget.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

using ReconstructFormDataMap_t = NiTMap<uint32_t, ReconstructFormData>;

BGSReconstructFormsInFileMap::BGSReconstructFormsInFileMap()
  : NiTMap<uint32_t, BGSReconstructFormsMap*>(0x25u), pInteriorCells(nullptr), pWorldSpaces(nullptr), iCount(0) {}

// The map owns the forms maps it holds and the two location maps.
BGSReconstructFormsInFileMap::~BGSReconstructFormsInFileMap() {
    NiTMapItem<uint32_t, BGSReconstructFormsMap*>* it = GetFirstPos();
    while (it) {
        uint32_t iFormID = 0;
        BGSReconstructFormsMap* pFormsMap = nullptr;
        GetNext(it, iFormID, pFormsMap);
        delete pFormsMap;
    }
    delete pInteriorCells;
    delete pWorldSpaces;
}

void BGSReconstructFormsInFileMap::AddCellForm(
    TESWorldSpace* apWorldSpace,
    int32_t aiCellX,
    int32_t aiCellY,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    if (!pWorldSpaces) {
        BGSReconstructWorldSpacesMap* pMap = ::new BGSReconstructWorldSpacesMap;
        pWorldSpaces = pMap;
    }
    pWorldSpaces->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
    ++iCount;
}

// Files apForm of apCell in the interior cells map, or in the world spaces map when the cell is an exterior one.
void BGSReconstructFormsInFileMap::AddReference(
    TESObjectCELL* apCell,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    if (apCell->GetInterior()) {
        if (!pInteriorCells) {
            BGSReconstructCellBlocksMap* pMap = ::new BGSReconstructCellBlocksMap(false);
            pInteriorCells = pMap;
        }
        pInteriorCells->AddReference(apCell, apForm, aiFormID, aiFlags);
    }
    else {
        if (!pWorldSpaces) {
            BGSReconstructWorldSpacesMap* pMap = ::new BGSReconstructWorldSpacesMap;
            pWorldSpaces = pMap;
        }
        pWorldSpaces->AddReference(apCell, static_cast<TESObjectCELL*>(apForm), aiFormID, aiFlags);
    }
    ++iCount;
}

// The form's saved type and whether it is a reference decide where it is filed: a reference by its editor location
// (or, without one, by its location on the reference or by its parent cell), a navmesh by its parent cell's world
// space, a cell as a reference to itself, a world space nowhere, and every other form in the map of its type.
void BGSReconstructFormsInFileMap::AddForm(TESForm* apForm, uint32_t aiFlags) {
    uint8_t ucSavedFormType = apForm->GetSavedFormType();
    if (apForm->IsReference()) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
        NiPoint3 kLocation;
        NiPoint3 kAngle;
        TESForm* pLocationForm = nullptr;
        TESObjectCELL* pLocationCell = nullptr;
        TESWorldSpace* pWorldSpace = nullptr;
        if (pRef->GetEditorLocation(kLocation, kAngle, pLocationForm, nullptr)) {
            GetLocationCellOrWorld(pLocationForm, pLocationCell, pWorldSpace);
        }
        else {
            pWorldSpace = pRef->GetWorldSpace();
            if (pWorldSpace) {
                kLocation = pRef->GetLocationOnReference();
            }
            else {
                pLocationCell = pRef->GetParentCell();
            }
        }
        if (pWorldSpace) {
            // The two cell coordinates are computed in this order.
            int32_t iX = FloatToInt(kLocation.x) >> 12;
            int32_t iY = FloatToInt(kLocation.y) >> 12;
            AddCellForm(pWorldSpace, iX, iY, apForm, 0, aiFlags);
        }
        else if (pLocationCell) {
            AddReference(pLocationCell, apForm, 0, aiFlags);
        }
    }
    else if (ucSavedFormType == NAVM_ID) {
        TESObjectCELL* pParentCell = static_cast<NavMesh*>(apForm)->GetParentCell();
        TESWorldSpace* pWorldSpace = pParentCell ? pParentCell->GetWorldSpace() : nullptr;
        if (pWorldSpace) {
            // The two cell coordinates are read in this order, Y before X.
            int32_t iY = pParentCell->GetDataY();
            int32_t iX = pParentCell->GetDataX();
            AddCellForm(pWorldSpace, iX, iY, apForm, 0, aiFlags);
        }
        else if (pParentCell) {
            AddReference(pParentCell, apForm, 0, aiFlags);
        }
    }
    else if (ucSavedFormType == CELL_ID) {
        AddReference(static_cast<TESObjectCELL*>(apForm), apForm, 0, aiFlags);
    }
    else if (ucSavedFormType != WRLD_ID) {
        BGSReconstructFormsMap* pFormsMap = nullptr;
        // Asks the form for its saved type a second time.
        uint32_t uiTypeCode = formEnumString[apForm->GetSavedFormType()].uiFormString;
        if (!GetAt(uiTypeCode, pFormsMap)) {
            BGSReconstructFormsMap* pNewMap = ::new BGSReconstructFormsMap;
            pFormsMap = pNewMap;
            SetAt(uiTypeCode, pNewMap);
        }
        pFormsMap->AddForm(apForm, 0, aiFlags);
        ++iCount;
    }
}

// Walks apFile's records and constructs the forms the maps hold, adding each to apReconstructedForms. In an optimized
// file the groups of children to reconstruct are recorded with their file offsets and read after the file's TOFT
// record.
void BGSReconstructFormsInFileMap::ReconstructForms(TESFile* apFile, BGSReconstructedForms* apReconstructedForms) {
    if (!iCount) {
        return;
    }
    apFile->TESRewind(true);
    FORM* pcurrentForm = apFile->GetFormData();
    while (pcurrentForm->form == formEnumString[TES4_ID].uiFormString) {
        apFile->NextForm(true);
    }
    BGSReconstructCellBlocksMap* pcurrentCellBlocksMap = nullptr;
    BGSReconstructCellSubBlocksMap* pcurrentCellSubBlocksMap = nullptr;
    BGSReconstructCellsMap* pcurrentCellsMap = nullptr;
    BGSReconstructFormsMap* pcurrentFormsMap = nullptr;
    uint32_t icurrentWorldSpaceID = 0;
    uint32_t icurrentCellID = 0;
    pLastLoadedCell = nullptr;
    bool boptimizedFile = apFile->GetOptimizedFile();
    uint32_t ireconstructedCount = 0;
    BGSOffsetDataArray offsetArray;
    uint32_t icurrentOffset = 0;
    uint32_t icurrentOffsetEnd = 0;
    while (pcurrentForm->form && ireconstructedCount < iCount) {
        OffsetData currentOffsetData;
        if (pcurrentForm->form == formEnumString[GRUP_ID].uiFormString) {
            if (pcurrentForm->flags == formEnumString[CELL_ID].uiFormString ||
                pcurrentForm->flags == formEnumString[WRLD_ID].uiFormString) {
                // A top group of interior cells or of world spaces.
                pcurrentCellBlocksMap = nullptr;
                bool bexteriors = pcurrentForm->flags == formEnumString[WRLD_ID].uiFormString;
                if (bexteriors) {
                    if (pWorldSpaces) {
                        apFile->NextForm(true);
                    }
                    else {
                        apFile->NextGroup();
                    }
                }
                else if (pInteriorCells) {
                    pcurrentCellBlocksMap = pInteriorCells;
                    apFile->NextForm(true);
                }
                else {
                    apFile->NextGroup();
                }
                pcurrentCellSubBlocksMap = nullptr;
                pcurrentCellsMap = nullptr;
                pcurrentFormsMap = nullptr;
            }
            else if (!pcurrentForm->iFormID) {
                // A top group of one form type.
                if (GetAt(pcurrentForm->flags, pcurrentFormsMap)) {
                    apFile->NextForm(true);
                }
                else {
                    apFile->NextGroup();
                }
            }
            else {
                // The group type (FORM_GROUP_TYPE).
                switch (pcurrentForm->iFormID) {
                    case 1: {
                        // A world space's children.
                        uint32_t iworldSpaceID = pcurrentForm->flags;
                        TESForm::AddCompileIndex(iworldSpaceID, apFile);
                        if (pWorldSpaces->GetAt(iworldSpaceID, pcurrentCellBlocksMap)) {
                            if (boptimizedFile &&
                                pcurrentCellBlocksMap->GetFormMapFlag(RECONSTRUCT_CELL | RECONSTRUCT_TEMP_CHILDREN)) {
                                offsetArray.AddOffset(pcurrentForm->iVersionControl, nullptr);
                            }
                            apFile->NextForm(true);
                        }
                        else {
                            apFile->NextGroup();
                        }
                        pcurrentCellSubBlocksMap = nullptr;
                        pcurrentCellsMap = nullptr;
                        pcurrentFormsMap = nullptr;
                        break;
                    }
                    case 2:
                    case 4: {
                        // An interior or exterior cell block.
                        if (pcurrentCellBlocksMap->GetAt(pcurrentForm->flags, pcurrentCellSubBlocksMap)) {
                            apFile->NextForm(true);
                        }
                        else {
                            apFile->NextGroup();
                        }
                        pcurrentCellsMap = nullptr;
                        pcurrentFormsMap = nullptr;
                        break;
                    }
                    case 3:
                    case 5: {
                        // An interior or exterior cell sub-block.
                        if (pcurrentCellSubBlocksMap->GetAt(pcurrentForm->flags, pcurrentCellsMap)) {
                            apFile->NextForm(true);
                        }
                        else {
                            apFile->NextGroup();
                        }
                        pcurrentFormsMap = nullptr;
                        break;
                    }
                    case 6: {
                        // A cell's children.
                        if (pcurrentFormsMap) {
                            bool breconstructPersistentChildren = pcurrentFormsMap->GetFlag(
                                RECONSTRUCT_PERSISTENT_CHILDREN
                            );
                            bool breconstructTempChildren = pcurrentFormsMap->GetFlag(RECONSTRUCT_TEMP_CHILDREN);
                            if (boptimizedFile && !icurrentWorldSpaceID && breconstructTempChildren) {
                                offsetArray.AddOffset(pcurrentForm->iVersionControl, pcurrentFormsMap);
                                breconstructTempChildren = false;
                            }
                            if (breconstructPersistentChildren || breconstructTempChildren) {
                                apFile->NextForm(true);
                            }
                            else {
                                apFile->NextGroup();
                            }
                        }
                        else {
                            apFile->NextGroup();
                        }
                        break;
                    }
                    case 8: {
                        // A cell's persistent children.
                        if (pcurrentFormsMap->GetFlag(RECONSTRUCT_PERSISTENT_CHILDREN)) {
                            apFile->NextForm(true);
                        }
                        else {
                            apFile->NextGroup();
                        }
                        break;
                    }
                    case 9: {
                        // A cell's temporary children.
                        if (pcurrentFormsMap->GetFlag(RECONSTRUCT_TEMP_CHILDREN)) {
                            apFile->NextForm(true);
                        }
                        else {
                            apFile->NextGroup();
                        }
                        break;
                    }
                    default: {
                        apFile->NextGroup();
                        break;
                    }
                }
            }
        }
        else if (pcurrentForm->form == formEnumString[WRLD_ID].uiFormString) {
            icurrentWorldSpaceID = pcurrentForm->iFormID;
            apFile->NextForm(true);
        }
        else if (pcurrentForm->form == formEnumString[CELL_ID].uiFormString) {
            if (pcurrentCellsMap) {
                icurrentCellID = pcurrentForm->iFormID;
                uint32_t icellKey = icurrentCellID;
                if (pcurrentCellBlocksMap && pcurrentCellBlocksMap->bExteriors) {
                    // An exterior cell is filed by its coordinates, read from its XCLC chunk.
                    EXTERIOR_DATA coorddata;
                    ourMemset(&coorddata, 0, sizeof(EXTERIOR_DATA));
                    CHUNK_ID chunk = apFile->GetTESChunk();
                    while (chunk) {
                        if (chunk == XCLC_ID) {
                            apFile->GetChunkData(&coorddata, sizeof(EXTERIOR_DATA));
                            if (apFile->QEndian()) {
                                coorddata.Endian();
                            }
                            break;
                        }
                        if (apFile->NextChunk()) {
                            chunk = apFile->GetTESChunk();
                        }
                        else {
                            chunk = NO_CHUNK;
                        }
                    }
                    apFile->TESRewindChunk();
                    icellKey = TESObjectCELL::GetCoord(
                        static_cast<uint16_t>(coorddata.iCellX),
                        static_cast<uint16_t>(coorddata.iCellY)
                    );
                }
                pLastLoadedCell = nullptr;
                if (pcurrentCellsMap->GetAt(icellKey, pcurrentFormsMap)) {
                    if (pcurrentFormsMap->GetFlag(RECONSTRUCT_CELL)) {
                        pDataHandler->ConstructObject(apFile, false, nullptr, false);
                        apFile->NextForm(true);
                        TESForm* pform = TESForm::GetFormByNumericID(icurrentCellID);
                        if (pform) {
                            ReconstructFormData formData(pform, pcurrentFormsMap->iFlags);
                            apReconstructedForms->AddFormData(formData);
                        }
                        else {
                            SaveGameWarning("Cell with Form ID %08X was not able to be reconstructed", icurrentCellID);
                        }
                        ++ireconstructedCount;
                    }
                    else {
                        apFile->NextForm(true);
                    }
                }
                else {
                    apFile->NextForm(true);
                }
            }
            else {
                // A cell outside any sub-block: the world space's persistent cell.
                pcurrentFormsMap = pcurrentCellBlocksMap->pPersistentCell;
                icurrentCellID = pcurrentForm->iFormID;
                pLastLoadedCell = nullptr;
                apFile->NextForm(true);
            }
        }
        else if (pcurrentForm->form == formEnumString[TOFT_ID].uiFormString) {
            // The TOFT record's -1 and -2 entries start the recorded offsets.
            int32_t itoftID = static_cast<int32_t>(pcurrentForm->iFormID);
            if (itoftID == -1 || itoftID == -2) {
                if (offsetArray.iSize) {
                    currentOffsetData = offsetArray[icurrentOffset];
                    ++icurrentOffset;
                }
                else {
                    pcurrentForm->form = 0;
                }
            }
            else {
                apFile->NextForm(true);
            }
        }
        else {
            ReconstructFormData formData;
            uint32_t iformID = pcurrentForm->iFormID;
            pcurrentFormsMap->SetFlag(RECONSTRUCT_FLAG_CELL_FOUND);
            if (pcurrentFormsMap->GetAt(iformID, formData)) {
                if (icurrentCellID && (!pLastLoadedCell || pLastLoadedCell->iFormID != icurrentCellID)) {
                    pLastLoadedCell = static_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(icurrentCellID));
                }

                pDataHandler->ConstructObject(apFile, false, formData.pForm, true);
                apFile->NextForm(true);
                if (!formData.pForm) {
                    formData.pForm = TESForm::GetFormByNumericID(iformID);
                }
                if (formData.pForm) {
                    apReconstructedForms->AddFormData(formData);
                }
                else {
                    uint8_t iformType = TESForm::GetFormTypeFromFormString(pcurrentForm->form);
                    SaveGameWarning(
                        "Form %08X with type %s was not able to be reconstructed",
                        iformID,
                        TESForm::GetFormTypeString(iformType)
                    );
                }
                ++ireconstructedCount;
            }
            else {
                apFile->NextForm(true);
            }
        }
        // Past the end of the recorded group: on to the next recorded offset, or done.
        if (icurrentOffsetEnd && icurrentOffsetEnd <= apFile->GetOffset()) {
            if (offsetArray.iSize > icurrentOffset) {
                currentOffsetData = offsetArray[icurrentOffset];
                ++icurrentOffset;
            }
            else {
                pcurrentForm->form = 0;
            }
        }
        if (currentOffsetData.IsValid()) {
            // Jump to the recorded group: a world space's children reopen its cell blocks map, a cell's children its
            // forms map.
            apFile->SetOffset(currentOffsetData.iOffset);
            uint32_t ilength = 0;
            uint32_t iworldSpaceID = 0;
            if (pcurrentForm->iFormID == 1) {
                iworldSpaceID = pcurrentForm->flags;
                ilength = pcurrentForm->length;
                apFile->NextForm(true);
            }
            else {
                ilength = pcurrentForm->length;
            }
            pcurrentCellBlocksMap = nullptr;
            pcurrentCellSubBlocksMap = nullptr;
            pcurrentCellsMap = nullptr;
            pcurrentFormsMap = nullptr;
            icurrentCellID = 0;
            icurrentWorldSpaceID = 0;
            icurrentOffsetEnd = apFile->GetOffset() + ilength;
            if (iworldSpaceID) {
                icurrentWorldSpaceID = iworldSpaceID;
                pWorldSpaces->GetAt(icurrentWorldSpaceID, pcurrentCellBlocksMap);
            }
            else {
                pcurrentFormsMap = currentOffsetData.pFormsMap;
                icurrentCellID = pcurrentForm->flags;
            }
            currentOffsetData.Clear();
            if (icurrentWorldSpaceID && pcurrentForm->iFormID == 9) {
                apFile->NextGroup();
            }
        }
    }
    pLastLoadedCell = nullptr;
}

void BGSReconstructFormsInFileMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSReconstructCellSubBlocksMap::BGSReconstructCellSubBlocksMap() : NiTMap<uint32_t, BGSReconstructCellsMap*>(0x25u) {}

// The map owns the cells maps it holds.
BGSReconstructCellSubBlocksMap::~BGSReconstructCellSubBlocksMap() {
    NiTMapItem<uint32_t, BGSReconstructCellsMap*>* it = GetFirstPos();
    while (it) {
        uint32_t iSubBlock = 0;
        BGSReconstructCellsMap* pCellsMap = nullptr;
        GetNext(it, iSubBlock, pCellsMap);
        delete pCellsMap;
    }
}

void BGSReconstructCellSubBlocksMap::AddReference(
    TESObjectCELL* apCell,
    TESForm* apCellOrRef,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    uint32_t uiSubBlock = apCell->GetGroupSubBlockKey();
    BGSReconstructCellsMap* pCellsMap = nullptr;
    if (!GetAt(uiSubBlock, pCellsMap)) {
        BGSReconstructCellsMap* pNewMap = ::new BGSReconstructCellsMap;
        pCellsMap = pNewMap;
        SetAt(uiSubBlock, pNewMap);
    }
    pCellsMap->AddReference(apCell, apCellOrRef, aiFormID, aiFlags);
}

void BGSReconstructCellSubBlocksMap::AddCellForm(
    TESWorldSpace* apWorldSpace,
    int32_t aiCellX,
    int32_t aiCellY,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    uint32_t uiSubBlock = TESObjectCELL::CalcExtGroupSubBlockKey(aiCellX, aiCellY);
    BGSReconstructCellsMap* pCellsMap = nullptr;
    if (!GetAt(uiSubBlock, pCellsMap)) {
        BGSReconstructCellsMap* pNewMap = ::new BGSReconstructCellsMap;
        pCellsMap = pNewMap;
        SetAt(uiSubBlock, pNewMap);
    }
    pCellsMap->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
}

bool BGSReconstructCellSubBlocksMap::GetFormMapFlag(uint32_t aiFlags) {
    NiTMapItem<uint32_t, BGSReconstructCellsMap*>* it = GetFirstPos();
    while (it) {
        uint32_t uiSubBlock = 0;
        BGSReconstructCellsMap* pCellsMap = nullptr;
        GetNext(it, uiSubBlock, pCellsMap);
        if (pCellsMap->GetFormMapFlag(aiFlags)) {
            return true;
        }
    }
    return false;
}

void BGSReconstructCellSubBlocksMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ReconstructFormData::ReconstructFormData() : pForm(nullptr), iFlags(0) {}

ReconstructFormData::ReconstructFormData(uint32_t aiFlags) : pForm(nullptr), iFlags(aiFlags) {}

ReconstructFormData::ReconstructFormData(TESForm* apForm, uint32_t aiFlags) : pForm(apForm), iFlags(aiFlags) {}

bool ReconstructFormData::GetFlag(uint32_t aiFlags) const {
    return (iFlags & aiFlags) != 0;
}

BGSReconstructedForms::BGSReconstructedForms() {}

BGSReconstructedForms::~BGSReconstructedForms() {}

uint32_t BGSReconstructedForms::GetSize() {
    uint32_t uiCount = Forms1.iSize;
    return uiCount + Forms2.iSize;
}

// The aiIndex-th form to reconstruct: the entries of Forms1, then those of Forms2. The compare is unsigned.
ReconstructFormData* BGSReconstructedForms::GetFormData(uint32_t aiIndex) {
    uint32_t uiCount = Forms1.GetSize();
    if (aiIndex >= uiCount) {
        return &Forms2[aiIndex - uiCount];
    }
    return &Forms1[aiIndex];
}

TESForm* BGSReconstructedForms::GetForm(uint32_t aiIndex) {
    uint32_t iforms1Count = Forms1.iSize;
    ReconstructFormData* pFormData = aiIndex >= iforms1Count ? &Forms2[aiIndex - iforms1Count] : &Forms1[aiIndex];
    return pFormData->pForm;
}

void BGSReconstructedForms::Clear() {
    Forms1.SetSize(0, true);
    Forms2.SetSize(0, true);
}

void BGSReconstructedForms::SetReservedSize(uint32_t aiSize) {
    Forms1.SetReservedSize(aiSize);
    Forms2.SetReservedSize(aiSize);
}

// References, cells and world spaces go in the second list, every other form in the first.
void BGSReconstructedForms::AddFormData(ReconstructFormData& arFormData) {
    if (arFormData.pForm->IsReference() ||
        arFormData.pForm->GetFormType() == CELL_ID ||
        arFormData.pForm->GetFormType() == WRLD_ID) {
        Forms2.Add(arFormData);
    }
    else {
        Forms1.Add(arFormData);
    }
}

char BGSReconstructedForms::RemoveFormAt(uint32_t aiIndex) {
    uint32_t iforms1Count = Forms1.iSize;
    if (aiIndex >= iforms1Count) {
        Forms2.Remove(aiIndex - iforms1Count, true);
    }
    else {
        Forms1.Remove(aiIndex, true);
    }
    return 1;
}

BGSReconstructFormsInAllFilesMap::BGSReconstructFormsInAllFilesMap()
  : NiTMap<TESFile*, BGSReconstructFormsInFileMap*>(0x25u), iCount(0) {}

BGSReconstructFormsInAllFilesMap::~BGSReconstructFormsInAllFilesMap() {
    Clear();
}

void BGSReconstructFormsInAllFilesMap::ReconstructForms() {
    pSaveLoadGame->SetReconstructingForms(true);
    ReconstructedForms.SetReservedSize(iCount);
    bool bSaveLoad = pDataHandler->GetSaveLoad();
    pDataHandler->SetSaveLoad(true);
    NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>* kIter = GetFirstPos();
    while (kIter) {
        TESFile* pFile = nullptr;
        BGSReconstructFormsInFileMap* pMap = nullptr;
        GetNext(kIter, pFile, pMap);
        TESFile* pThreadSafeFile = pFile->GetThreadSafeFile();
        pMap->ReconstructForms(pThreadSafeFile, &ReconstructedForms);
    }
    pDataHandler->SetSaveLoad(bSaveLoad);
    for (uint32_t i = 0; i < ReconstructedForms.GetSize(); ++i) {
        ReconstructFormData* pFormData = ReconstructedForms.GetFormData(i);
        if (pFormData->pForm->IsReference() && !static_cast<TESObjectREFR*>(pFormData->pForm)->GetObjectReference()) {
            ReconstructedForms.RemoveFormAt(i--);
        }
    }
    uint32_t uiSize = ReconstructedForms.GetSize();
    for (uint32_t i = 0; i < uiSize; ++i) {
        ReconstructFormData* pFormData = ReconstructedForms.GetFormData(i);
        if (!pFormData->pForm->GetInitialized()) {
            pFormData->pForm->InitItem();
        }
    }
    for (uint32_t i = 0; i < uiSize; ++i) {
        InitReconstructedForm(ReconstructedForms.GetFormData(i));
    }
    pSaveLoadGame->SetReconstructingForms(false);
}

uint32_t BGSReconstructFormsInAllFilesMap::GetReconstructedCount() {
    return ReconstructedForms.GetSize();
}

TESForm* BGSReconstructFormsInAllFilesMap::GetReconstructedForm(uint32_t aiIndex) {
    return ReconstructedForms.GetForm(aiIndex);
}

void BGSReconstructFormsInAllFilesMap::AddForm(TESForm* apForm, uint32_t aiFlags) {
    if (apForm) {
        TESFile* pfile = apForm->GetFile(-1);
        if (pfile) {
            BGSReconstructFormsInFileMap* pformsInFileMap = nullptr;
            if (!NiTMapBase<
                    DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>,
                    TESFile*,
                    BGSReconstructFormsInFileMap*>::GetAt(pfile, pformsInFileMap)) {
                BGSReconstructFormsInFileMap* pNewMap = ::new BGSReconstructFormsInFileMap;
                pformsInFileMap = pNewMap;
                SetAt(pfile, pNewMap);
            }
            pformsInFileMap->AddForm(apForm, aiFlags);
            ++iCount;
        }
        else {
            const char* pFormTypeString = apForm->GetFormTypeString();
            const char* pEditorID = apForm->GetFormEditorID();
            SaveGameWarning(
                "Form %08X %s with type %s and no file passed to "
                "BGSReconstructFormsInAllFilesMap::AddForm()",
                apForm->iFormID,
                pEditorID,
                pFormTypeString
            );
        }
    }
}

void BGSReconstructFormsInAllFilesMap::AddReference(TESObjectCELL* apParentCell, uint32_t aiRefID, uint32_t aiFlags) {
    if (apParentCell) {
        TESFile* pfile = nullptr;
        if (apParentCell->GetInterior()) {
            pfile = GetFileForFormID(apParentCell->GetFileList(), aiRefID);
        }
        else {
            pfile = GetFileForFormID(apParentCell->GetWorldSpace()->GetFileList(), aiRefID);
        }
        if (pfile) {
            BGSReconstructFormsInFileMap* pformsInFileMap = nullptr;
            if (!NiTMapBase<
                    DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>,
                    TESFile*,
                    BGSReconstructFormsInFileMap*>::GetAt(pfile, pformsInFileMap)) {
                BGSReconstructFormsInFileMap* pNewMap = ::new BGSReconstructFormsInFileMap;
                pformsInFileMap = pNewMap;
                SetAt(pfile, pNewMap);
            }
            pformsInFileMap->AddReference(apParentCell, nullptr, aiRefID, aiFlags);
            ++iCount;
        }
        else {
            const char* pEditorID = apParentCell->GetFormEditorID();
            SaveGameWarning(
                "Cell %08X %s with no file passed to BGSReconstructFormsInAllFilesMap::AddReference()",
                apParentCell->iFormID,
                pEditorID
            );
        }
    }
}

void BGSReconstructFormsInAllFilesMap::AddReference(
    TESWorldSpace* apWorldSpace,
    int32_t aiCellX,
    int32_t aiCellY,
    uint32_t aiRefID,
    uint32_t aiFlags
) {
    if (apWorldSpace) {
        TESFile* pfile = GetFileForFormID(apWorldSpace->GetFileList(), aiRefID);
        if (pfile) {
            BGSReconstructFormsInFileMap* pformsInFileMap = nullptr;
            if (!NiTMapBase<
                    DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>,
                    TESFile*,
                    BGSReconstructFormsInFileMap*>::GetAt(pfile, pformsInFileMap)) {
                BGSReconstructFormsInFileMap* pNewMap = ::new BGSReconstructFormsInFileMap;
                pformsInFileMap = pNewMap;
                SetAt(pfile, pNewMap);
            }
            pformsInFileMap->AddCellForm(apWorldSpace, aiCellX, aiCellY, nullptr, aiRefID, aiFlags);
            ++iCount;
        }
        else {
            const char* pEditorID = apWorldSpace->GetFormEditorID();
            SaveGameWarning(
                "Worldspace %08X %s with no file passed to "
                "BGSReconstructFormsInAllFilesMap::AddReference()",
                apWorldSpace->iFormID,
                pEditorID
            );
        }
    }
}

TESFile* BGSReconstructFormsInAllFilesMap::GetFileForFormID(BSSimpleList<TESFile*>* apFileList, uint32_t aiFormID) {
    TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
    if (pForm) {
        return pForm->GetFile(-1);
    }
    TESFile* pFile = TESFile::GetFileForTempID(aiFormID);
    if (pFile) {
        return pFile;
    }

    return pDataHandler->GetCompiledFile(static_cast<uint8_t>(aiFormID >> 24));
}

void BGSReconstructFormsInAllFilesMap::Clear() {
    NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>* it = GetFirstPos();
    while (it) {
        TESFile* pfile = nullptr;
        BGSReconstructFormsInFileMap* pformsInFileMap = nullptr;
        GetNext(it, pfile, pformsInFileMap);
        if (pformsInFileMap) {
            delete pformsInFileMap;
        }
    }
    RemoveAll_1();
    ReconstructedForms.Clear();
    iCount = 0;
}

void BGSReconstructFormsInAllFilesMap::InitReconstructedForm(ReconstructFormData* apFormData) {
    if (apFormData->pForm->IsReference()) {
        TESObjectREFR* pref = static_cast<TESObjectREFR*>(apFormData->pForm);
        NiNode* p3D = pref->Get3D();
        if (p3D) {
            double dLocalScale = p3D->GetLocalScale();
            if (dLocalScale != pref->GetScale()) {
                float fRefScale = pref->fRefScale;
                pref->SetScale(fRefScale);
            }
        }
        if (apFormData->GetFlag(RECONSTRUCT_REVERT_ONLY) &&
            pref->GetObjectReference()->TESForm::GetHasSpokenFlag() &&
            pref->GetDisabled()) {
            TESWorldSpace* pworld = pref->GetWorldSpace();
            if (pworld) {
                pworld->GetTerrainManager()->HideTree(pref, true);
            }
        }
        if (pref->IsMobileObject()) {
            MobileObject* pmob = static_cast<MobileObject*>(pref);
            if (pmob->GetDisabledOrDeleted()) {
                if (apFormData->GetFlag(RECONSTRUCT_REVERT_ONLY) && pmob->pCurrentProcess) {
                    ReferenceProcessLists.RemoveReference(pmob, pmob->GetCurrentProcessType());
                    BaseProcess* pProcess = pmob->pCurrentProcess;
                    if (pProcess) {
                        delete pProcess;
                    }
                    pmob->SetCurrentAIProcess(nullptr);
                }
            }
            else if (!pmob->pCurrentProcess) {
                LowProcess* pProcess = new LowProcess;
                pmob->SetCurrentAIProcess(pProcess);
                pmob->ChangeProcessLevel();
            }
            if (pmob->IsActor()) {
                Actor* pactor = static_cast<Actor*>(pmob);
                pSaveLoadGame->QueueInitPackageLocations(pactor);
                if (apFormData->GetFlag(RECONSTRUCT_REVERT_ONLY) && pactor->Get3D()) {
                    if (pactor->IsCreature()) {
                        static_cast<TESCreature*>(pactor->GetActorBase())
                            ->TESCreature::InitDefaultWorn(pactor, true, true, true);
                    }
                    else {
                        static_cast<TESNPC*>(pactor->GetActorBase())
                            ->TESNPC::InitDefaultWorn(pactor, true, true, false, true);
                    }
                }
            }
        }
    }
    else if (apFormData->pForm->IsActorBase()) {
        // The actor base's TESActorBaseData copies nothing from a null template.
        TESActorBaseData* pactorBaseData = static_cast<TESActorBase*>(apFormData->pForm);
        pactorBaseData->CopyFromTemplateForm(nullptr);
    }
    else if (apFormData->pForm->GetFormType() == NAVM_ID) {
        NavMesh* pNavMesh = static_cast<NavMesh*>(apFormData->pForm);
        if (pNavMesh->NavMesh::GetNavMeshInfo()) {
            bool bDisabled = pNavMesh->GetDisabled();
            pNavMesh->GetNavMeshInfo()->SetDisabled(bDisabled);
        }
    }
}

void BGSReconstructFormsInAllFilesMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSReconstructCellBlocksMap::BGSReconstructCellBlocksMap(bool abExteriors)
  : NiTMap<uint32_t, BGSReconstructCellSubBlocksMap*>(37u), pPersistentCell(nullptr), bExteriors(abExteriors) {}

// The map owns the sub-block maps it holds; the destructor leaves pPersistentCell alone.
BGSReconstructCellBlocksMap::~BGSReconstructCellBlocksMap() {
    NiTMapItem<uint32_t, BGSReconstructCellSubBlocksMap*>* it = GetFirstPos();
    while (it) {
        uint32_t iBlock = 0;
        BGSReconstructCellSubBlocksMap* pSubBlocksMap = nullptr;
        GetNext(it, iBlock, pSubBlocksMap);
        delete pSubBlocksMap;
    }
}

void BGSReconstructCellBlocksMap::AddReference(
    TESObjectCELL* apCell,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    if (bExteriors &&
        apForm &&
        apForm->IsReference() &&
        static_cast<TESObjectREFR*>(apForm)->TESObjectREFR::GetPersistFlagValue()) {
        if (!pPersistentCell) {
            BGSReconstructFormsMap* pMap = ::new BGSReconstructFormsMap;
            pPersistentCell = pMap;
        }
        pPersistentCell->AddForm(apForm, aiFormID, aiFlags);
    }
    else {
        uint32_t iblock = apCell->GetGroupBlockKey();
        BGSReconstructCellSubBlocksMap* pcellSubBlocksMap = nullptr;
        if (!GetAt(iblock, pcellSubBlocksMap)) {
            BGSReconstructCellSubBlocksMap* pNewMap = ::new BGSReconstructCellSubBlocksMap;
            pcellSubBlocksMap = pNewMap;
            SetAt(iblock, pNewMap);
        }
        pcellSubBlocksMap->AddReference(apCell, apForm, aiFormID, aiFlags);
    }
}

void BGSReconstructCellBlocksMap::AddCellForm(
    TESWorldSpace* apWorldSpace,
    int32_t aiCellX,
    int32_t aiCellY,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    if (apForm && apForm->IsReference() && static_cast<TESObjectREFR*>(apForm)->GetPersistFlagValue()) {
        if (!pPersistentCell) {
            BGSReconstructFormsMap* pMap = ::new BGSReconstructFormsMap;
            pPersistentCell = pMap;
        }
        pPersistentCell->AddForm(apForm, aiFormID, aiFlags);
    }
    else {
        uint32_t uiBlock = TESObjectCELL::CalcExtGroupBlockKey(aiCellX, aiCellY);
        BGSReconstructCellSubBlocksMap* pcellSubBlocksMap = nullptr;
        if (!GetAt(uiBlock, pcellSubBlocksMap)) {
            BGSReconstructCellSubBlocksMap* pNewMap = ::new BGSReconstructCellSubBlocksMap;
            pcellSubBlocksMap = pNewMap;
            SetAt(uiBlock, pNewMap);
        }
        pcellSubBlocksMap->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
    }
}

bool BGSReconstructCellBlocksMap::GetFormMapFlag(uint32_t aiFlags) {
    if (pPersistentCell && pPersistentCell->GetFlag(aiFlags)) {
        return true;
    }
    NiTMapItem<uint32_t, BGSReconstructCellSubBlocksMap*>* it = GetFirstPos();
    while (it) {
        uint32_t iblock = 0;
        BGSReconstructCellSubBlocksMap* psubBlocksMap = nullptr;
        GetNext(it, iblock, psubBlocksMap);
        if (psubBlocksMap->GetFormMapFlag(aiFlags)) {
            return true;
        }
    }
    return false;
}

void BGSReconstructCellBlocksMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSReconstructFormsMap::BGSReconstructFormsMap() : NiTMap<uint32_t, ReconstructFormData>(0x25u), iFlags(0) {}

BGSReconstructFormsMap::~BGSReconstructFormsMap() {}

void BGSReconstructFormsMap::SetFlag(uint32_t aiFlags) {
    iFlags |= aiFlags;
}

bool BGSReconstructFormsMap::GetFlag(uint32_t aiFlags) {
    return (iFlags & aiFlags) != 0;
}

// Files apForm under auiFormID (its own form ID when it has one) and sets the map's flags for it.
void BGSReconstructFormsMap::AddForm(TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags) {
    uint32_t iFormID = apForm ? apForm->iFormID : aiFormID;
    // A form data is default-constructed and never used, then the stored one is built in a temporary.
    ReconstructFormData kUnused;
    ReconstructFormData kFormData(apForm, aiFlags);
    SetAt(iFormID, kFormData);
    if (!apForm) {
        SetFlag(RECONSTRUCT_PERSISTENT_CHILDREN | RECONSTRUCT_TEMP_CHILDREN);
        return;
    }
    if (apForm->IsReference()) {
        if (static_cast<TESObjectREFR*>(apForm)->TESObjectREFR::GetPersistFlagValue()) {
            SetFlag(RECONSTRUCT_PERSISTENT_CHILDREN);
            return;
        }
    }
    else if (apForm->GetFormType() != NAVM_ID) {
        return;
    }
    SetFlag(RECONSTRUCT_TEMP_CHILDREN);
}

void BGSReconstructFormsMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

OffsetData::OffsetData() : iOffset(0), pFormsMap(nullptr) {}

OffsetData::OffsetData(uint32_t aiOffset, BGSReconstructFormsMap* apFormsMap)
  : iOffset(aiOffset), pFormsMap(apFormsMap) {}

void OffsetData::Clear() {
    iOffset = 0;
    pFormsMap = nullptr;
}

bool OffsetData::IsValid() {
    return iOffset != 0;
}

BGSOffsetDataArray::BGSOffsetDataArray() : BSScrapArray<OffsetData, 1024>(0x20u, 0, nullptr) {}

BGSOffsetDataArray::~BGSOffsetDataArray() {}

void BGSOffsetDataArray::AddOffset(uint32_t aiOffset, BGSReconstructFormsMap* apFormsMap) {
    OffsetData kData(aiOffset, apFormsMap);
    Add(kData);
}

void BGSOffsetDataArray::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSReconstructCellsMap::BGSReconstructCellsMap() : NiTMap<uint32_t, BGSReconstructFormsMap*>(0x25u) {}

// The map owns the forms maps it holds.
BGSReconstructCellsMap::~BGSReconstructCellsMap() {
    NiTMapItem<uint32_t, BGSReconstructFormsMap*>* it = GetFirstPos();
    while (it) {
        uint32_t iCellID = 0;
        BGSReconstructFormsMap* pFormsMap = nullptr;
        GetNext(it, iCellID, pFormsMap);
        delete pFormsMap;
    }
}

void BGSReconstructCellsMap::AddReference(
    TESObjectCELL* apCell,
    TESForm* apCellOrRef,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    uint32_t uiCellKey = 0;
    if (apCell->GetInterior()) {
        uiCellKey = apCell->iFormID;
    }
    else {
        int32_t iDataY = apCell->GetDataY();
        int32_t iDataX = apCell->GetDataX();
        uiCellKey = TESObjectCELL::GetCoord(iDataX, iDataY);
    }
    BGSReconstructFormsMap* pFormsMap = nullptr;
    if (!GetAt(uiCellKey, pFormsMap)) {
        BGSReconstructFormsMap* pNewMap = ::new BGSReconstructFormsMap;
        pFormsMap = pNewMap;
        SetAt(uiCellKey, pNewMap);
    }
    if (apCell == apCellOrRef) {
        pFormsMap->SetFlag(RECONSTRUCT_CELL);
        pFormsMap->SetFlag(aiFlags);
    }
    else {
        pFormsMap->AddForm(apCellOrRef, aiFormID, aiFlags);
    }
}

void BGSReconstructCellsMap::AddCellForm(
    TESWorldSpace* apWorldSpace,
    int32_t aiCellX,
    int32_t aiCellY,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    int32_t icellKey = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
    BGSReconstructFormsMap* pformsMap = nullptr;
    if (!GetAt(icellKey, pformsMap)) {
        BGSReconstructFormsMap* pNewMap = ::new BGSReconstructFormsMap;
        pformsMap = pNewMap;
        SetAt(icellKey, pNewMap);
    }
    pformsMap->AddForm(apForm, aiFormID, aiFlags);
}

bool BGSReconstructCellsMap::GetFormMapFlag(uint32_t aiFlags) {
    NiTMapItem<uint32_t, BGSReconstructFormsMap*>* pos = GetFirstPos();
    while (pos) {
        uint32_t uiCellID = 0;
        BGSReconstructFormsMap* pFormsMap = nullptr;
        GetNext(pos, uiCellID, pFormsMap);
        if (pFormsMap->GetFlag(aiFlags)) {
            return true;
        }
    }
    return false;
}

void BGSReconstructCellsMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BGSReconstructWorldSpacesMap::BGSReconstructWorldSpacesMap() : NiTMap<uint32_t, BGSReconstructCellBlocksMap*>(37u) {}

// The map owns the cell blocks maps it holds.
BGSReconstructWorldSpacesMap::~BGSReconstructWorldSpacesMap() {
    NiTMapItem<uint32_t, BGSReconstructCellBlocksMap*>* it = GetFirstPos();
    while (it) {
        uint32_t iWorldSpaceID = 0;
        BGSReconstructCellBlocksMap* pCellBlocksMap = nullptr;
        GetNext(it, iWorldSpaceID, pCellBlocksMap);
        delete pCellBlocksMap;
    }
}

void BGSReconstructWorldSpacesMap::AddReference(
    TESObjectCELL* apCell,
    TESForm* apRefCell,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    TESWorldSpace* pworldSpace = apCell->GetWorldSpace();
    uint32_t iworldSpaceID = pworldSpace->iFormID;
    BGSReconstructCellBlocksMap* pcellBlocksMap = nullptr;
    if (!GetAt(iworldSpaceID, pcellBlocksMap)) {
        BGSReconstructCellBlocksMap* pNewMap = ::new BGSReconstructCellBlocksMap(true);
        pcellBlocksMap = pNewMap;
        SetAt(iworldSpaceID, pNewMap);
    }
    pcellBlocksMap->AddReference(apCell, apRefCell, aiFormID, aiFlags);
}

void BGSReconstructWorldSpacesMap::AddCellForm(
    TESWorldSpace* apWorldSpace,
    int32_t aiCellX,
    int32_t aiCellY,
    TESForm* apForm,
    uint32_t aiFormID,
    uint32_t aiFlags
) {
    uint32_t iworldSpaceID = apWorldSpace->iFormID;
    BGSReconstructCellBlocksMap* pcellBlocksMap = nullptr;
    if (!GetAt(iworldSpaceID, pcellBlocksMap)) {
        BGSReconstructCellBlocksMap* pNewMap = ::new BGSReconstructCellBlocksMap(true);
        pcellBlocksMap = pNewMap;
        SetAt(iworldSpaceID, pNewMap);
    }
    pcellBlocksMap->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
}

void BGSReconstructWorldSpacesMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BGSSaveLoadGame::SetReconstructingForms(bool abValue) {
    if (abValue) {
        uiThreadSpecificFlags |= THREAD_FLAG_RECONSTRUCTING_FORMS;
    }
    else {
        uiThreadSpecificFlags &= ~THREAD_FLAG_RECONSTRUCTING_FORMS;
    }
}
