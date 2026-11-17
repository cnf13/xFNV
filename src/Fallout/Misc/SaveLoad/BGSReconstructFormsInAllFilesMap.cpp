#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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

void BGSReconstructFormsInFileMap::AddCellForm(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY,
	TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags) {
	if (!pWorldSpaces) {
		void* pMemory = ourMalloc(sizeof(BGSReconstructWorldSpacesMap));
		BGSReconstructWorldSpacesMap* pMap;
		if (pMemory) {
			pMap = ::new (pMemory) BGSReconstructWorldSpacesMap;
		} else {
			pMap = nullptr;
		}
		pWorldSpaces = pMap;
	}
	pWorldSpaces->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
	++iCount;
}

// Files apForm of apCell in the interior cells map, or in the world spaces map when the cell is an exterior one.
void BGSReconstructFormsInFileMap::AddReference(TESObjectCELL* apCell, TESForm* apForm, uint32_t aiFormID,
	uint32_t aiFlags) {
	if (apCell->GetInterior()) {
		if (!pInteriorCells) {
			BGSReconstructCellBlocksMap* v6;
			BGSReconstructCellBlocksMap* apMemory =
				static_cast<BGSReconstructCellBlocksMap*>(ourMalloc(sizeof(BGSReconstructCellBlocksMap)));
			if (apMemory) {
				v6 = ::new (apMemory) BGSReconstructCellBlocksMap(0);
			} else {
				v6 = 0;
			}
			pInteriorCells = v6;
		}
		pInteriorCells->AddReference(apCell, apForm, aiFormID, aiFlags);
	} else {
		if (!pWorldSpaces) {
			BGSReconstructWorldSpacesMap* v5;
			BGSReconstructWorldSpacesMap* v8 =
				static_cast<BGSReconstructWorldSpacesMap*>(ourMalloc(sizeof(BGSReconstructWorldSpacesMap)));
			if (v8) {
				v5 = ::new (v8) BGSReconstructWorldSpacesMap;
			} else {
				v5 = 0;
			}
			pWorldSpaces = v5;
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
		} else {
			pWorldSpace = pRef->GetWorldSpace();
			if (pWorldSpace) {
				kLocation = pRef->GetLocationOnReference();
			} else {
				pLocationCell = pRef->GetParentCell();
			}
		}
		if (pWorldSpace) {
			// The two cell coordinates are computed in this order.
			int32_t iX = FloatToInt(kLocation.x) >> 12;
			int32_t iY = FloatToInt(kLocation.y) >> 12;
			AddCellForm(pWorldSpace, iX, iY, apForm, 0, aiFlags);
		} else if (pLocationCell) {
			AddReference(pLocationCell, apForm, 0, aiFlags);
		}
	} else if (ucSavedFormType == NAVM_ID) {
		TESObjectCELL* pParentCell = static_cast<NavMesh*>(apForm)->GetParentCell();
		TESWorldSpace* pWorldSpace = pParentCell ? pParentCell->GetWorldSpace() : nullptr;
		if (pWorldSpace) {
			// The two cell coordinates are read in this order, Y before X, as the game does.
			int32_t iY = pParentCell->GetDataY();
			int32_t iX = pParentCell->GetDataX();
			AddCellForm(pWorldSpace, iX, iY, apForm, 0, aiFlags);
		} else if (pParentCell) {
			AddReference(pParentCell, apForm, 0, aiFlags);
		}
	} else if (ucSavedFormType == CELL_ID) {
		AddReference(static_cast<TESObjectCELL*>(apForm), apForm, 0, aiFlags);
	} else if (ucSavedFormType != WRLD_ID) {
		BGSReconstructFormsMap* pFormsMap = nullptr;
		// Asks the form for its saved type a second time, as the game does.
		uint32_t uiTypeCode = formEnumString[apForm->GetSavedFormType()].uiFormString;
		if (!GetAt(uiTypeCode, pFormsMap)) {
			BGSReconstructFormsMap* pNewMap =
				static_cast<BGSReconstructFormsMap*>(ourMalloc(sizeof(BGSReconstructFormsMap)));
			if (pNewMap) {
				pNewMap = ::new (pNewMap) BGSReconstructFormsMap;
			}
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
	apFile->TESRewind(1);
	FORM* pcurrentForm = apFile->GetFormData();
	while (pcurrentForm->form == formEnumString[TES4_ID].uiFormString) {
		apFile->NextForm(1);
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
	[[maybe_unused]] bool bloadingOffsets = false;
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
						apFile->NextForm(1);
					} else {
						apFile->NextGroup();
					}
				} else if (pInteriorCells) {
					pcurrentCellBlocksMap = pInteriorCells;
					apFile->NextForm(1);
				} else {
					apFile->NextGroup();
				}
				pcurrentCellSubBlocksMap = nullptr;
				pcurrentCellsMap = nullptr;
				pcurrentFormsMap = nullptr;
			} else if (!pcurrentForm->iFormID) {
				// A top group of one form type.
				if (GetAt(pcurrentForm->flags, pcurrentFormsMap)) {
					apFile->NextForm(1);
				} else {
					apFile->NextGroup();
				}
			} else {
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
						apFile->NextForm(1);
					} else {
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
						apFile->NextForm(1);
					} else {
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
						apFile->NextForm(1);
					} else {
						apFile->NextGroup();
					}
					pcurrentFormsMap = nullptr;
					break;
				}
				case 6: {
					// A cell's children.
					if (pcurrentFormsMap) {
						bool breconstructPersistentChildren =
							pcurrentFormsMap->GetFlag(RECONSTRUCT_PERSISTENT_CHILDREN);
						bool breconstructTempChildren = pcurrentFormsMap->GetFlag(RECONSTRUCT_TEMP_CHILDREN);
						if (boptimizedFile && !icurrentWorldSpaceID && breconstructTempChildren) {
							offsetArray.AddOffset(pcurrentForm->iVersionControl, pcurrentFormsMap);
							breconstructTempChildren = false;
						}
						if (breconstructPersistentChildren || breconstructTempChildren) {
							apFile->NextForm(1);
						} else {
							apFile->NextGroup();
						}
					} else {
						apFile->NextGroup();
					}
					break;
				}
				case 8: {
					// A cell's persistent children.
					if (pcurrentFormsMap->GetFlag(RECONSTRUCT_PERSISTENT_CHILDREN)) {
						apFile->NextForm(1);
					} else {
						apFile->NextGroup();
					}
					break;
				}
				case 9: {
					// A cell's temporary children.
					if (pcurrentFormsMap->GetFlag(RECONSTRUCT_TEMP_CHILDREN)) {
						apFile->NextForm(1);
					} else {
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
		} else if (pcurrentForm->form == formEnumString[WRLD_ID].uiFormString) {
			icurrentWorldSpaceID = pcurrentForm->iFormID;
			apFile->NextForm(1);
		} else if (pcurrentForm->form == formEnumString[CELL_ID].uiFormString) {
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
						} else {
							chunk = NO_CHUNK;
						}
					}
					apFile->TESRewindChunk();
					icellKey = TESObjectCELL::GetCoord(static_cast<uint16_t>(coorddata.iCellX),
						static_cast<uint16_t>(coorddata.iCellY));
				}
				pLastLoadedCell = nullptr;
				if (pcurrentCellsMap->GetAt(icellKey, pcurrentFormsMap)) {
					if (pcurrentFormsMap->GetFlag(RECONSTRUCT_CELL)) {
						pDataHandler->ConstructObject(apFile, 0, 0, 0);
						apFile->NextForm(1);
						TESForm* pform = TESForm::GetFormByNumericID(icurrentCellID);
						if (pform) {
							ReconstructFormData formData(pform, pcurrentFormsMap->iFlags);
							apReconstructedForms->AddFormData(formData);
						} else {
							SaveGameWarning("Cell with Form ID %08X was not able to be reconstructed", icurrentCellID);
						}
						++ireconstructedCount;
					} else {
						apFile->NextForm(1);
					}
				} else {
					apFile->NextForm(1);
				}
			} else {
				// A cell outside any sub-block: the world space's persistent cell.
				pcurrentFormsMap = pcurrentCellBlocksMap->pPersistentCell;
				icurrentCellID = pcurrentForm->iFormID;
				pLastLoadedCell = nullptr;
				apFile->NextForm(1);
			}
		} else if (pcurrentForm->form == formEnumString[TOFT_ID].uiFormString) {
			// The TOFT record's -1 and -2 entries start the recorded offsets.
			int32_t itoftID = static_cast<int32_t>(pcurrentForm->iFormID);
			if (itoftID == -1 || itoftID == -2) {
				if (offsetArray.iSize) {
					currentOffsetData = offsetArray[icurrentOffset];
					++icurrentOffset;
					bloadingOffsets = true;
				} else {
					pcurrentForm->form = 0;
				}
			} else {
				apFile->NextForm(1);
			}
		} else {
			ReconstructFormData formData;
			uint32_t iformID = pcurrentForm->iFormID;
			pcurrentFormsMap->SetFlag(RECONSTRUCT_FLAG_CELL_FOUND);
			if (pcurrentFormsMap->GetAt(iformID, formData)) {
				if (icurrentCellID) {
					if (!pLastLoadedCell || pLastLoadedCell->iFormID != icurrentCellID) {
						pLastLoadedCell = static_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(icurrentCellID));
					}
				}
				pDataHandler->ConstructObject(apFile, 0, formData.pForm, 1);
				apFile->NextForm(1);
				if (!formData.pForm) {
					formData.pForm = TESForm::GetFormByNumericID(iformID);
				}
				if (formData.pForm) {
					apReconstructedForms->AddFormData(formData);
				} else {
					uint8_t iformType = TESForm::GetFormTypeFromFormString(pcurrentForm->form);
					SaveGameWarning("Form %08X with type %s was not able to be reconstructed", iformID,
						TESForm::GetFormTypeString(iformType));
				}
				++ireconstructedCount;
			} else {
				apFile->NextForm(1);
			}
		}
		// Past the end of the recorded group: on to the next recorded offset, or done.
		if (icurrentOffsetEnd && icurrentOffsetEnd <= apFile->GetOffset()) {
			if (offsetArray.iSize > icurrentOffset) {
				currentOffsetData = offsetArray[icurrentOffset];
				++icurrentOffset;
			} else {
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
				apFile->NextForm(1);
			} else {
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
			} else {
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

void BGSReconstructCellSubBlocksMap::AddReference(TESObjectCELL* arg0, TESForm* apCellOrRef, uint32_t aiFormID,
	uint32_t aiFlags) {
	uint32_t a1 = arg0->GetGroupSubBlockKey();
	BGSReconstructCellsMap* a2 = 0;
	if (!GetAt(a1, a2)) {
		BGSReconstructCellsMap* v5;
		BGSReconstructCellsMap* apMemory =
			static_cast<BGSReconstructCellsMap*>(ourMalloc(sizeof(BGSReconstructCellsMap)));
		if (apMemory) {
			v5 = ::new (apMemory) BGSReconstructCellsMap;
		} else {
			v5 = 0;
		}
		a2 = v5;
		SetAt(a1, v5);
	}
	a2->AddReference(arg0, apCellOrRef, aiFormID, aiFlags);
}

void BGSReconstructCellSubBlocksMap::AddCellForm(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY,
	TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags) {
	uint32_t a1 = TESObjectCELL::CalcExtGroupSubBlockKey(aiCellX, aiCellY);
	BGSReconstructCellsMap* a2 = 0;
	if (!GetAt(a1, a2)) {
		BGSReconstructCellsMap* v7;
		BGSReconstructCellsMap* apMemory =
			static_cast<BGSReconstructCellsMap*>(ourMalloc(sizeof(BGSReconstructCellsMap)));
		if (apMemory) {
			v7 = ::new (apMemory) BGSReconstructCellsMap;
		} else {
			v7 = 0;
		}
		a2 = v7;
		SetAt(a1, v7);
	}
	a2->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
}

bool BGSReconstructCellSubBlocksMap::GetFormMapFlag(uint32_t aiFlags) {
	NiTMapItem<uint32_t, BGSReconstructCellsMap*>* a2;
	a2 = GetFirstPos();
	while (a2) {
		uint32_t a3 = 0;
		BGSReconstructCellsMap* a4 = 0;
		GetNext(a2, a3, a4);
		if (a4->GetFormMapFlag(aiFlags)) {
			return 1;
		}
	}
	return 0;
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
	uint32_t FormFlags = Forms1.iSize;
	return FormFlags + Forms2.iSize;
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
	ReconstructFormData* v2;
	uint32_t iforms1Count = Forms1.iSize;
	if (aiIndex >= iforms1Count) {
		v2 = &Forms2.operator[](aiIndex - iforms1Count);
	} else {
		v2 = &Forms1.operator[](aiIndex);
	}
	return v2->pForm;
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
		(arFormData.pForm->GetFormType() == CELL_ID || arFormData.pForm->GetFormType() == WRLD_ID)) {
		Forms2.Add(arFormData);
	} else {
		Forms1.Add(arFormData);
	}
}

char BGSReconstructedForms::RemoveFormAt(uint32_t aiIndex) {
	uint32_t iforms1Count = Forms1.iSize;
	if (aiIndex >= iforms1Count) {
		Forms2.Remove(aiIndex - iforms1Count, true);
	} else {
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
	pSaveLoadGame->SetReconstructingForms(1);
	ReconstructedForms.SetReservedSize(iCount);
	bool SaveLoad = pDataHandler->GetSaveLoad();
	pDataHandler->SetSaveLoad(1);
	NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>* kIter = GetFirstPos();
	while (kIter) {
		TESFile* pFile = 0;
		BGSReconstructFormsInFileMap* pMap = 0;
		GetNext(kIter, pFile, pMap);
		TESFile* apFile = pFile->GetThreadSafeFile();
		pMap->ReconstructForms(apFile, &ReconstructedForms);
	}
	pDataHandler->SetSaveLoad(SaveLoad);
	for (uint32_t a2 = 0; a2 < ReconstructedForms.GetSize(); ++a2) {
		ReconstructFormData* FormData = ReconstructedForms.GetFormData(a2);
		if (FormData->pForm->IsReference() && !static_cast<TESObjectREFR*>(FormData->pForm)->GetObjectReference()) {
			ReconstructedForms.RemoveFormAt(a2--);
		}
	}
	uint32_t Size = ReconstructedForms.GetSize();
	for (uint32_t aiIndex = 0; aiIndex < Size; ++aiIndex) {
		ReconstructFormData* v4 = ReconstructedForms.GetFormData(aiIndex);
		if (!v4->pForm->GetInitialized()) {
			v4->pForm->InitItem();
		}
	}
	for (uint32_t i = 0; i < Size; ++i) {
		ReconstructFormData* v2 = ReconstructedForms.GetFormData(i);
		InitReconstructedForm(v2);
	}
	pSaveLoadGame->SetReconstructingForms(0);
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
			BGSReconstructFormsInFileMap* pformsInFileMap = 0;
			if (!NiTMapBase<DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>, TESFile*,
					BGSReconstructFormsInFileMap*>::GetAt(pfile, pformsInFileMap)) {
				BGSReconstructFormsInFileMap* pformsInFileMap_2;
				BGSReconstructFormsInFileMap* apMemory =
					static_cast<BGSReconstructFormsInFileMap*>(ourMalloc(sizeof(BGSReconstructFormsInFileMap)));
				if (apMemory) {
					pformsInFileMap_2 = ::new (apMemory) BGSReconstructFormsInFileMap;
				} else {
					pformsInFileMap_2 = 0;
				}
				pformsInFileMap = pformsInFileMap_2;
				SetAt(pfile, pformsInFileMap_2);
			}
			pformsInFileMap->AddForm(apForm, aiFlags);
			++iCount;
		} else {
			const char* FormTypeString = apForm->GetFormTypeString();
			const char* v3 = apForm->GetFormEditorID();
			SaveGameWarning("Form %08X %s with type %s and no file passed to "
							"BGSReconstructFormsInAllFilesMap::AddForm()",
				apForm->iFormID, v3, FormTypeString);
		}
	}
}

void BGSReconstructFormsInAllFilesMap::AddReference(TESObjectCELL* apParentCell, uint32_t aiRefID, uint32_t aiFlags) {
	if (apParentCell) {
		TESFile* pfile = 0;
		if (apParentCell->GetInterior()) {
			pfile = GetFileForFormID(apParentCell->GetFileList(), aiRefID);
		} else {
			TESWorldSpace* WorldSpace = apParentCell->GetWorldSpace();
			pfile = GetFileForFormID(WorldSpace->GetFileList(), aiRefID);
		}
		if (pfile) {
			BGSReconstructFormsInFileMap* pformsInFileMap = 0;
			if (!NiTMapBase<DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>, TESFile*,
					BGSReconstructFormsInFileMap*>::GetAt(pfile, pformsInFileMap)) {
				BGSReconstructFormsInFileMap* pformsInFileMap_2;
				BGSReconstructFormsInFileMap* apMemory =
					static_cast<BGSReconstructFormsInFileMap*>(ourMalloc(sizeof(BGSReconstructFormsInFileMap)));
				if (apMemory) {
					pformsInFileMap_2 = ::new (apMemory) BGSReconstructFormsInFileMap;
				} else {
					pformsInFileMap_2 = 0;
				}
				pformsInFileMap = pformsInFileMap_2;
				SetAt(pfile, pformsInFileMap_2);
			}
			pformsInFileMap->AddReference(apParentCell, nullptr, aiRefID, aiFlags);
			++iCount;
		} else {
			const char* v5 = apParentCell->GetFormEditorID();
			SaveGameWarning("Cell %08X %s with no file passed to BGSReconstructFormsInAllFilesMap::AddReference()",
				apParentCell->iFormID, v5);
		}
	}
}

void BGSReconstructFormsInAllFilesMap::AddReference(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY,
	uint32_t aiRefID, uint32_t aiFlags) {
	if (apWorldSpace) {
		TESFile* pfile = GetFileForFormID(apWorldSpace->GetFileList(), aiRefID);
		if (pfile) {
			BGSReconstructFormsInFileMap* pformsInFileMap = 0;
			if (!NiTMapBase<DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>, TESFile*,
					BGSReconstructFormsInFileMap*>::GetAt(pfile, pformsInFileMap)) {
				BGSReconstructFormsInFileMap* pformsInFileMap_2;
				BGSReconstructFormsInFileMap* apMemory =
					static_cast<BGSReconstructFormsInFileMap*>(ourMalloc(sizeof(BGSReconstructFormsInFileMap)));
				if (apMemory) {
					pformsInFileMap_2 = ::new (apMemory) BGSReconstructFormsInFileMap;
				} else {
					pformsInFileMap_2 = 0;
				}
				pformsInFileMap = pformsInFileMap_2;
				SetAt(pfile, pformsInFileMap_2);
			}
			pformsInFileMap->AddCellForm(apWorldSpace, aiCellX, aiCellY, 0, aiRefID, aiFlags);
			++iCount;
		} else {
			const char* v6 = apWorldSpace->GetFormEditorID();
			SaveGameWarning("Worldspace %08X %s with no file passed to "
							"BGSReconstructFormsInAllFilesMap::AddReference()",
				apWorldSpace->iFormID, v6);
		}
	}
}

TESFile* BGSReconstructFormsInAllFilesMap::GetFileForFormID(BSSimpleList<TESFile*>* apFileList, uint32_t aiFormID) {
	TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
	if (pForm) {
		return pForm->GetFile((-1));
	}
	TESFile* pFile = TESFile::GetFileForTempID(aiFormID);
	if (pFile) {
		return pFile;
	} else {
		return pDataHandler->GetCompiledFile((uint8_t)(aiFormID >> 24));
	}
}

void BGSReconstructFormsInAllFilesMap::Clear() {
	NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>* it = GetFirstPos();
	while (it) {
		TESFile* pfile = 0;
		BGSReconstructFormsInFileMap* pformsInFileMap = 0;
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
			double LocalScale = p3D->GetLocalScale();
			if (LocalScale != pref->GetScale()) {
				float newscale = pref->fRefScale;
				pref->SetScale(newscale);
			}
		}
		if (apFormData->GetFlag(RECONSTRUCT_REVERT_ONLY) && pref->GetObjectReference()->TESForm::GetHasSpokenFlag() &&
			pref->GetDisabled()) {
			TESWorldSpace* pworld = pref->GetWorldSpace();
			if (pworld) {
				BGSTerrainManager* TerrainManager = pworld->GetTerrainManager();
				TerrainManager->HideTree(pref, 1);
			}
		}
		if (pref->IsMobileObject()) {
			MobileObject* pmob = static_cast<MobileObject*>(pref);
			if (pmob->GetDisabledOrDeleted()) {
				if (apFormData->GetFlag(RECONSTRUCT_REVERT_ONLY)) {
					if (pmob->pCurrentProcess) {
						PROCESS_TYPE CurrentProcessType = pmob->GetCurrentProcessType();
						ReferenceProcessLists.RemoveReference(pmob, CurrentProcessType);
						BaseProcess* CurrentAIProcess = pmob->pCurrentProcess;
						if (CurrentAIProcess) {
							delete CurrentAIProcess;
						}
						pmob->SetCurrentAIProcess(0);
					}
				}
			} else {
				if (!pmob->pCurrentProcess) {
					LowProcess* v8;
					LowProcess* apMemory = static_cast<LowProcess*>(ourMalloc(sizeof(LowProcess)));
					if (apMemory) {
						v8 = ::new (apMemory) LowProcess;
					} else {
						v8 = 0;
					}
					pmob->SetCurrentAIProcess(v8);
					pmob->ChangeProcessLevel();
				}
			}
			if (pmob->IsActor()) {
				Actor* pactor = static_cast<Actor*>(pmob);
				pSaveLoadGame->QueueInitPackageLocations(pactor);
				if (apFormData->GetFlag(RECONSTRUCT_REVERT_ONLY)) {
					if (pactor->Get3D()) {
						if (pactor->IsCreature()) {
							static_cast<TESCreature*>(pactor->GetActorBase())
								->TESCreature::InitDefaultWorn(pactor, 1, 1, 1);
						} else {
							static_cast<TESNPC*>(pactor->GetActorBase())->TESNPC::InitDefaultWorn(pactor, 1, 1, 0, 1);
						}
					}
				}
			}
		}
	} else {
		if (apFormData->pForm->IsActorBase()) {
			// The actor base's TESActorBaseData copies nothing from a null template.
			TESActorBaseData* pactorBaseData = static_cast<TESActorBase*>(apFormData->pForm);
			pactorBaseData->CopyFromTemplateForm(nullptr);
		} else {
			if (apFormData->pForm->GetFormType() == NAVM_ID) {
				NavMesh* v11 = static_cast<NavMesh*>(apFormData->pForm);
				if (v11->NavMesh::GetNavMeshInfo()) {
					bool newscalea = v11->GetDisabled();
					v11->GetNavMeshInfo()->SetDisabled(newscalea);
				}
			}
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

void BGSReconstructCellBlocksMap::AddReference(TESObjectCELL* apCell, TESForm* apForm, uint32_t aiFormID,
	uint32_t aiFlags) {
	if (bExteriors &&
		(apForm &&
			(apForm->IsReference() && static_cast<TESObjectREFR*>(apForm)->TESObjectREFR::GetPersistFlagValue()))) {
		if (!pPersistentCell) {
			BGSReconstructFormsMap* v6;
			BGSReconstructFormsMap* apMemory =
				static_cast<BGSReconstructFormsMap*>(ourMalloc(sizeof(BGSReconstructFormsMap)));
			if (apMemory) {
				v6 = ::new (apMemory) BGSReconstructFormsMap;
			} else {
				v6 = 0;
			}
			pPersistentCell = v6;
		}
		pPersistentCell->AddForm(apForm, aiFormID, aiFlags);
	} else {
		uint32_t iblock = apCell->GetGroupBlockKey();
		BGSReconstructCellSubBlocksMap* pcellSubBlocksMap = 0;
		if (!GetAt(iblock, pcellSubBlocksMap)) {
			BGSReconstructCellSubBlocksMap* pcellSubBlocksMap_2;
			BGSReconstructCellSubBlocksMap* v8 =
				static_cast<BGSReconstructCellSubBlocksMap*>(ourMalloc(sizeof(BGSReconstructCellSubBlocksMap)));
			if (v8) {
				pcellSubBlocksMap_2 = ::new (v8) BGSReconstructCellSubBlocksMap;
			} else {
				pcellSubBlocksMap_2 = 0;
			}
			pcellSubBlocksMap = pcellSubBlocksMap_2;
			SetAt(iblock, pcellSubBlocksMap_2);
		}
		pcellSubBlocksMap->AddReference(apCell, apForm, aiFormID, aiFlags);
	}
}

void BGSReconstructCellBlocksMap::AddCellForm(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY,
	TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags) {
	if (apForm && (apForm->IsReference() && static_cast<TESObjectREFR*>(apForm)->GetPersistFlagValue())) {
		if (!pPersistentCell) {
			BGSReconstructFormsMap* v8;
			BGSReconstructFormsMap* apMemory =
				static_cast<BGSReconstructFormsMap*>(ourMalloc(sizeof(BGSReconstructFormsMap)));
			if (apMemory) {
				v8 = ::new (apMemory) BGSReconstructFormsMap;
			} else {
				v8 = 0;
			}
			pPersistentCell = v8;
		}
		pPersistentCell->AddForm(apForm, aiFormID, aiFlags);
	} else {
		uint32_t a1 = TESObjectCELL::CalcExtGroupBlockKey(aiCellX, aiCellY);
		BGSReconstructCellSubBlocksMap* a2 = 0;
		if (!GetAt(a1, a2)) {
			BGSReconstructCellSubBlocksMap* v7;
			BGSReconstructCellSubBlocksMap* v10 =
				static_cast<BGSReconstructCellSubBlocksMap*>(ourMalloc(sizeof(BGSReconstructCellSubBlocksMap)));
			if (v10) {
				v7 = ::new (v10) BGSReconstructCellSubBlocksMap;
			} else {
				v7 = 0;
			}
			a2 = v7;
			SetAt(a1, v7);
		}
		a2->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
	}
}

bool BGSReconstructCellBlocksMap::GetFormMapFlag(uint32_t aiFlags) {
	if (pPersistentCell && pPersistentCell->GetFlag(aiFlags)) {
		return 1;
	}
	NiTMapItem<uint32_t, BGSReconstructCellSubBlocksMap*>* it = GetFirstPos();
	while (it) {
		uint32_t iblock = 0;
		BGSReconstructCellSubBlocksMap* psubBlocksMap = 0;
		GetNext(it, iblock, psubBlocksMap);
		if (psubBlocksMap->GetFormMapFlag(aiFlags)) {
			return 1;
		}
	}
	return 0;
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
	uint32_t iFormID;
	if (apForm) {
		iFormID = apForm->iFormID;
	} else {
		iFormID = aiFormID;
	}
	// A form data is default-constructed and never used, then the stored one is built in a temporary, as the game does.
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
	} else {
		if (apForm->GetFormType() != NAVM_ID) {
			return;
		}
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
	iOffset = (uint32_t)0;
	pFormsMap = nullptr;
}

bool OffsetData::IsValid() {
	return iOffset != 0;
}

BGSOffsetDataArray::BGSOffsetDataArray() : BSScrapArray<OffsetData, 1024>(0x20u, 0, nullptr) {}

BGSOffsetDataArray::~BGSOffsetDataArray() {}

void BGSOffsetDataArray::AddOffset(uint32_t aiOffset, BGSReconstructFormsMap* apFormsMap) {
	OffsetData kData;
	kData.~OffsetData();
	::new (&kData) OffsetData(aiOffset, apFormsMap);
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

void BGSReconstructCellsMap::AddReference(TESObjectCELL* apCell, TESForm* apCellOrRef, uint32_t aiFormID,
	uint32_t aiFlags) {
	uint32_t a1 = 0;
	if (apCell->GetInterior()) {
		a1 = apCell->iFormID;
	} else {
		int32_t DataY = apCell->GetDataY();
		int32_t DataX = apCell->GetDataX();
		a1 = TESObjectCELL::GetCoord(DataX, DataY);
	}
	BGSReconstructFormsMap* a2 = 0;
	if (!GetAt(a1, a2)) {
		BGSReconstructFormsMap* v7;
		BGSReconstructFormsMap* apMemory =
			static_cast<BGSReconstructFormsMap*>(ourMalloc(sizeof(BGSReconstructFormsMap)));
		if (apMemory) {
			v7 = ::new (apMemory) BGSReconstructFormsMap;
		} else {
			v7 = 0;
		}
		a2 = v7;
		SetAt(a1, v7);
	}
	if (apCell == apCellOrRef) {
		a2->SetFlag(RECONSTRUCT_CELL);
		a2->SetFlag(aiFlags);
	} else {
		a2->AddForm(apCellOrRef, aiFormID, aiFlags);
	}
}

void BGSReconstructCellsMap::AddCellForm(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY, TESForm* apForm,
	uint32_t aiFormID, uint32_t aiFlags) {
	int32_t icellKey = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
	BGSReconstructFormsMap* pformsMap = 0;
	if (!GetAt(icellKey, pformsMap)) {
		BGSReconstructFormsMap* pformsMap_2;
		BGSReconstructFormsMap* apMemory =
			static_cast<BGSReconstructFormsMap*>(ourMalloc(sizeof(BGSReconstructFormsMap)));
		if (apMemory) {
			pformsMap_2 = ::new (apMemory) BGSReconstructFormsMap;
		} else {
			pformsMap_2 = 0;
		}
		pformsMap = pformsMap_2;
		SetAt(icellKey, pformsMap_2);
	}
	pformsMap->AddForm(apForm, aiFormID, aiFlags);
}

bool BGSReconstructCellsMap::GetFormMapFlag(uint32_t aiFlags) {
	NiTMapItem<uint32_t, BGSReconstructFormsMap*>* pos = GetFirstPos();
	while (pos) {
		uint32_t a3 = 0;
		BGSReconstructFormsMap* a4 = 0;
		GetNext(pos, a3, a4);
		if (a4->GetFlag(aiFlags)) {
			return 1;
		}
	}
	return 0;
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

void BGSReconstructWorldSpacesMap::AddReference(TESObjectCELL* apCell, TESForm* apRefCell, uint32_t aiFormID,
	uint32_t aiFlags) {
	TESWorldSpace* pworldSpace = apCell->GetWorldSpace();
	uint32_t iworldSpaceID = pworldSpace->iFormID;
	BGSReconstructCellBlocksMap* pcellBlocksMap = 0;
	if (!GetAt(iworldSpaceID, pcellBlocksMap)) {
		BGSReconstructCellBlocksMap* pcellBlocksMap_2;
		BGSReconstructCellBlocksMap* apMemory =
			static_cast<BGSReconstructCellBlocksMap*>(ourMalloc(sizeof(BGSReconstructCellBlocksMap)));
		if (apMemory) {
			pcellBlocksMap_2 = ::new (apMemory) BGSReconstructCellBlocksMap(1);
		} else {
			pcellBlocksMap_2 = 0;
		}
		pcellBlocksMap = pcellBlocksMap_2;
		SetAt(iworldSpaceID, pcellBlocksMap_2);
	}
	pcellBlocksMap->AddReference(apCell, apRefCell, aiFormID, aiFlags);
}

void BGSReconstructWorldSpacesMap::AddCellForm(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY,
	TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags) {
	uint32_t iworldSpaceID = apWorldSpace->iFormID;
	BGSReconstructCellBlocksMap* pcellBlocksMap = 0;
	if (!GetAt(iworldSpaceID, pcellBlocksMap)) {
		BGSReconstructCellBlocksMap* pcellBlocksMap_2;
		BGSReconstructCellBlocksMap* apMemory =
			static_cast<BGSReconstructCellBlocksMap*>(ourMalloc(sizeof(BGSReconstructCellBlocksMap)));
		if (apMemory) {
			pcellBlocksMap_2 = ::new (apMemory) BGSReconstructCellBlocksMap(1);
		} else {
			pcellBlocksMap_2 = 0;
		}
		pcellBlocksMap = pcellBlocksMap_2;
		SetAt(iworldSpaceID, pcellBlocksMap_2);
	}
	pcellBlocksMap->AddCellForm(apWorldSpace, aiCellX, aiCellY, apForm, aiFormID, aiFlags);
}

void BGSReconstructWorldSpacesMap::operator delete(void* apMemory) {
	ourFree(apMemory);
}

void BGSSaveLoadGame::SetReconstructingForms(bool abValue) {
	if (abValue) {
		uint32_t uiFlags = GetThreadSpecificFlags();
		GetThreadSpecificFlags() = uiFlags | 4u;
	} else {
		uint32_t uiFlags = GetThreadSpecificFlags();
		GetThreadSpecificFlags() = uiFlags & ~4u;
	}
}
