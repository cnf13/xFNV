#include "Fallout Shared/Pathfinding/NavMeshInfo.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshBounds.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavMeshVertex.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

NavMeshInfo::NavMeshInfo()
	: NavMeshID(0), ParentSpaceID(0), uiFlags(0), iCellKey(-559030611), ApproxLocation(NiPoint3_ZERO),
	  pParentSpace(nullptr), fPreferredPercent(0.0f), pNavMesh(nullptr), pBounds(nullptr) {}

NavMeshInfo::~NavMeshInfo() {
	if (pNavMesh) {
		pNavMesh->SetNavMeshInfo(nullptr);
	}
	if (pBounds) {
		delete pBounds;
	}
}

void NavMeshInfo::SetDisabled(bool abDisable) {
	uint32_t v2;
	if (abDisable) {
		v2 = uiFlags | 16;
	} else {
		v2 = uiFlags & (~0x10u);
	}
	uiFlags = v2;
}

uint32_t NavMeshInfo::GetNavMeshID() const {
	return NavMeshID;
}

void NavMeshInfo::SetNavMesh(NavMesh* apNavMesh) {
	pNavMesh = apNavMesh;
}

void NavMeshInfo::CreateFromNavMesh(NavMesh* apNavMesh) {
	uint32_t v7;
	NavMeshID = apNavMesh->iFormID;
	pParentSpace = apNavMesh->GetParentSpace();
	TESForm* ParentSpace = apNavMesh->GetParentSpace();
	ParentSpaceID = ParentSpace->iFormID;
	AdjacentNavMeshes.Clear(1);
	PreferredAdjacentNavMeshes.Clear(1);
	ConnectedDoors.Clear(1);
	uiFlags = 0;
	if (pBounds) {
		NavMeshBounds* apThis = pBounds;
		if (apThis) {
			delete apThis;
		}
		pBounds = 0;
	}
	if (apNavMesh->GetDisabled()) {
		SetDisabled(1);
	}
	uint32_t isize = apNavMesh->GetTriangleCount();
	for (uint32_t i = 0; i < isize; ++i) {
		NavMeshTriangle* ptri = apNavMesh->GetTriangle(i);
		if (ptri->HasEdgeExtraInfo()) {
			bool btriPreferred = ptri->IsPreferred();
			for (int32_t e = 0; e < 3; ++e) {
				if (ptri->EdgeHasExtraInfo(e)) {
					uint16_t TriOrExtraInfo = ptri->GetTriOrExtraInfo(e);
					EdgeExtraInfo* pextraInfo = apNavMesh->GetExtraEdgeInfo(TriOrExtraInfo);
					if (pextraInfo->Portal.IsValid()) {
						bool baddPortal = 1;
						NavMeshPtr spotherMesh;
						if (pextraInfo->Portal.pOtherMesh->GetNavMeshEvenDisabled(spotherMesh)) {
							if (spotherMesh.m_pObject->GetDelete()) {
								baddPortal = 0;
							} else {
								if (btriPreferred) {
									uint32_t FormFlags = AdjacentNavMeshes.iSize;
									for (uint32_t k = 0; k < FormFlags; ++k) {
										NavMeshInfo** v4 = &AdjacentNavMeshes.GetAt(k);
										if (*v4 == pextraInfo->Portal.pOtherMesh) {
											AdjacentNavMeshes.RemoveFast(k, 1u);
											break;
										}
									}
									uint32_t iotherSize = PreferredAdjacentNavMeshes.iSize;
									for (uint32_t k_2 = 0; k_2 < iotherSize; ++k_2) {
										NavMeshInfo** v5 = &PreferredAdjacentNavMeshes.GetAt(k_2);
										if (*v5 == pextraInfo->Portal.pOtherMesh) {
											baddPortal = 0;
											break;
										}
									}
								} else {
									uint32_t iotherSize_2 = AdjacentNavMeshes.iSize;
									for (uint32_t k_3 = 0; k_3 < iotherSize_2; ++k_3) {
										NavMeshInfo** v6 = &AdjacentNavMeshes.GetAt(k_3);
										if (*v6 == pextraInfo->Portal.pOtherMesh) {
											baddPortal = 0;
											break;
										}
									}
								}
							}
						} else {
							baddPortal = 0;
						}
						if (baddPortal) {
							if (btriPreferred) {
								PreferredAdjacentNavMeshes.Add(pextraInfo->Portal.pOtherMesh);
							} else {
								AdjacentNavMeshes.Add(pextraInfo->Portal.pOtherMesh);
							}
						}
					}
				}
			}
		}
	}
	if (isize) {
		v7 = uiFlags & ~8u;
	} else {
		v7 = uiFlags | 8;
	}
	uiFlags = v7;
	isize = apNavMesh->GetNumDoorPortals();
	for (uint32_t i_2 = 0; i_2 < isize; ++i_2) {
		ConnectedDoors.Add(apNavMesh->GetDoorPortalFromArray(i_2)->pDoorForm);
	}
	if (apNavMesh->pParentCell->GetWorldSpace()) {
		int32_t y = apNavMesh->pParentCell->GetDataY();
		int32_t DataX = apNavMesh->pParentCell->GetDataX();
		iCellKey = TESObjectCELL::GetCoord(DataX, y);
	} else {
		iCellKey = 0;
	}
	ApproxLocation = NiPoint3_ZERO;
	uint32_t iverts = apNavMesh->GetVertexCount();
	if (iverts) {
		for (uint32_t i_3 = 0; i_3 < iverts; ++i_3) {
			ApproxLocation.operator+=(apNavMesh->GetVertex(i_3)->Location);
		}
		ApproxLocation.operator/=(static_cast<float>(iverts));
	}
	uint32_t itrisize = apNavMesh->GetTriangleCount();
	if (itrisize) {
		uint32_t ipreferredTris = 0;
		for (uint32_t i_4 = 0; i_4 < itrisize; ++i_4) {
			NavMeshTriangle* ptri_2 = apNavMesh->GetTriangle(i_4);
			if (ptri_2->IsPreferred()) {
				++ipreferredTris;
			}
		}
		float v11 = static_cast<double>(ipreferredTris) / static_cast<double>(itrisize);
		SetPreferredPercent(v11);
	}
	SetAltered(1);
}

void NavMeshInfo::SetPreferredPercent(float afPercent) {
	fPreferredPercent = afPercent;
}

NavMeshBounds* NavMeshInfo::GetBounds() const {
	return pBounds;
}

void NavMeshInfo::InitDoorPointers() {
	uint32_t uiCount = ConnectedDoors.GetSize();
	for (uint32_t i = 0; i < uiCount; ++i) {
		TESObjectREFR* pDoorSlot = ConnectedDoors.GetAt(static_cast<int32_t>(i));
		// Unresolved form IDs are stored in pointer-sized slots and those bits are read as a uint32_t.
		uint32_t uiDoorID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDoorSlot));
		TESForm* pDoor = TESForm::GetFormByNumericID(uiDoorID);
		ConnectedDoors.GetAt(static_cast<int32_t>(i)) = static_cast<TESObjectREFR*>(pDoor);
		if (ConnectedDoors.GetAt(static_cast<int32_t>(i)) == nullptr) {
			Warning("NavMeshInfo %08x, Could not find Door %08x referenced by NavMeshInfo", NavMeshID, uiDoorID);
			ConnectedDoors.Remove(i, true);
			--uiCount;
			--i;
		}
	}
}

TESObjectCELL* NavMeshInfo::GetCell() const {
	TESWorldSpace* pWorldSpace = GetWorldSpace();
	if (pWorldSpace) {
		return pWorldSpace->GetCellFromKey(iCellKey);
	} else {
		return GetInteriorCell();
	}
}

TESObjectCELL* NavMeshInfo::GetInteriorCell() const {
	TESForm* pParentSpace = GetParentSpace();
	return dynamic_cast<TESObjectCELL*>(pParentSpace);
}

TESWorldSpace* NavMeshInfo::GetWorldSpace() const {
	TESForm* cellOrWorldSpace = GetParentSpace();
	return dynamic_cast<TESWorldSpace*>(cellOrWorldSpace);
}

// The bytes FillInfoChunk writes: 28 for the fields, the bounds chunk when there are bounds, and 4 more.
int32_t NavMeshInfo::GetInfoChunkSize() {
	uint32_t uiSize = 28;
	if (pBounds) {
		uiSize = pBounds->GetBoundsChunkSize() + 28;
	}
	return uiSize + 4;
}

// The bytes FillPortalChunk writes: each list's count and its 4-byte IDs.
int32_t NavMeshInfo::GetPortalChunkSize() {
	uint32_t uiSize = 4 * AdjacentNavMeshes.GetSize() + 8;
	uiSize = uiSize + 4 * PreferredAdjacentNavMeshes.GetSize() + 4;
	return uiSize + 4 * ConnectedDoors.GetSize();
}

// Reads pParentSpace's form ID without a null check, as the game does.
void NavMeshInfo::FillInfoChunk(char* apChunkBuffer) {
	uint32_t uiValue = uiFlags;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	ourMemcpy(apChunkBuffer, &uiValue, sizeof(uint32_t));
	apChunkBuffer += sizeof(uint32_t);
	if (pNavMesh && pNavMesh->GetFormID() != NavMeshID) {
		NavMeshID = pNavMesh->GetFormID();
	}
	uiValue = NavMeshID;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	ourMemcpy(apChunkBuffer, &uiValue, sizeof(uint32_t));
	apChunkBuffer += sizeof(uint32_t);
	ParentSpaceID = pParentSpace->GetFormID();
	uiValue = ParentSpaceID;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	ourMemcpy(apChunkBuffer, &uiValue, sizeof(uint32_t));
	apChunkBuffer += sizeof(uint32_t);
	uiValue = static_cast<uint32_t>(iCellKey);
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	ourMemcpy(apChunkBuffer, &uiValue, sizeof(uint32_t));
	apChunkBuffer += sizeof(uint32_t);
	NiPoint3 kLocation = ApproxLocation;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(kLocation.x, false);
		ENDIAN(kLocation.y, false);
		ENDIAN(kLocation.z, false);
	}
	ourMemcpy(apChunkBuffer, &kLocation, sizeof(NiPoint3));
	apChunkBuffer += sizeof(NiPoint3);
	if (pBounds) {
		pBounds->FillBoundsChunk(apChunkBuffer);
		apChunkBuffer += pBounds->GetBoundsChunkSize();
	}
	float fPercent = fPreferredPercent;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(fPercent, false);
	}
	ourMemcpy(apChunkBuffer, &fPercent, sizeof(float));
	apChunkBuffer += sizeof(float);
}

// 0x20 in uiFlags marks an info with bounds.
void NavMeshInfo::SetFromInfoChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber) {
	ourMemcpy(&uiFlags, apChunkBuffer, sizeof(uint32_t));
	if (apFile->QEndian()) {
		ENDIAN(uiFlags, false);
	}
	apChunkBuffer += sizeof(uint32_t);
	ourMemcpy(&NavMeshID, apChunkBuffer, sizeof(uint32_t));
	if (apFile->QEndian()) {
		ENDIAN(NavMeshID, false);
	}
	apChunkBuffer += sizeof(uint32_t);
	TESForm::AddCompileIndex(NavMeshID, apFile);
	ourMemcpy(&ParentSpaceID, apChunkBuffer, sizeof(uint32_t));
	if (apFile->QEndian()) {
		ENDIAN(ParentSpaceID, false);
	}
	apChunkBuffer += sizeof(uint32_t);
	TESForm::AddCompileIndex(ParentSpaceID, apFile);
	ourMemcpy(&iCellKey, apChunkBuffer, sizeof(int32_t));
	if (apFile->QEndian()) {
		ENDIAN(iCellKey, false);
	}
	apChunkBuffer += sizeof(int32_t);
	ourMemcpy(&ApproxLocation, apChunkBuffer, sizeof(NiPoint3));
	if (apFile->QEndian()) {
		ENDIAN(ApproxLocation.x, false);
		ENDIAN(ApproxLocation.y, false);
		ENDIAN(ApproxLocation.z, false);
	}
	apChunkBuffer += sizeof(NiPoint3);
	if (uiFlags & 0x20) {
		if (!pBounds) {
			void* pMemory = ourMalloc(sizeof(NavMeshBounds));
			pBounds = pMemory ? ::new (pMemory) NavMeshBounds : nullptr;
		}
		pBounds->SetFromBoundsChunk(apChunkBuffer, apFile, aiFileVersionNumber);
		apChunkBuffer += pBounds->GetBoundsChunkSize();
	}
	if (aiFileVersionNumber >= 10) {
		ourMemcpy(&fPreferredPercent, apChunkBuffer, sizeof(float));
		if (apFile->QEndian()) {
			ENDIAN(fPreferredPercent, false);
		}
		apChunkBuffer += sizeof(float);
	}
}

// Stores each ID straight into the buffer. An adjacent navmesh that is not loaded leaves its slot unwritten; a missing
// door writes 0.
void NavMeshInfo::FillPortalChunk(char* apChunkBuffer) {
	alignas(NavMeshPtr) uint8_t aAdjacentStorage[sizeof(NavMeshPtr)];
	alignas(NavMeshPtr) uint8_t aPreferredStorage[sizeof(NavMeshPtr)];
	uint32_t uiValue = AdjacentNavMeshes.GetSize();
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	*reinterpret_cast<uint32_t*>(apChunkBuffer) = uiValue;
	apChunkBuffer += sizeof(uint32_t);
	uint32_t uiCount = AdjacentNavMeshes.GetSize();
	for (uint32_t i = 0; i < uiCount; ++i) {
		NavMeshInfo* pInfo = AdjacentNavMeshes.GetAt(i);
		NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aAdjacentStorage);
		::new (aAdjacentStorage) NavMeshPtr;
		if (pInfo->GetNavMeshEvenDisabled(spNavMesh)) {
			uiValue = spNavMesh->GetFormID();
			if (TESForm::QEndianSwapOnSave()) {
				ENDIAN(uiValue, false);
			}
			*reinterpret_cast<uint32_t*>(apChunkBuffer) = uiValue;
		}
		apChunkBuffer += sizeof(uint32_t);
		spNavMesh.~NavMeshPtr();
	}
	uiValue = PreferredAdjacentNavMeshes.GetSize();
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	*reinterpret_cast<uint32_t*>(apChunkBuffer) = uiValue;
	apChunkBuffer += sizeof(uint32_t);
	uiCount = PreferredAdjacentNavMeshes.GetSize();
	for (uint32_t i = 0; i < uiCount; ++i) {
		NavMeshInfo* pInfo = PreferredAdjacentNavMeshes.GetAt(i);
		NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aPreferredStorage);
		::new (aPreferredStorage) NavMeshPtr;
		if (pInfo->GetNavMeshEvenDisabled(spNavMesh)) {
			uiValue = spNavMesh->GetFormID();
			if (TESForm::QEndianSwapOnSave()) {
				ENDIAN(uiValue, false);
			}
			*reinterpret_cast<uint32_t*>(apChunkBuffer) = uiValue;
		}
		apChunkBuffer += sizeof(uint32_t);
		spNavMesh.~NavMeshPtr();
	}
	uiValue = ConnectedDoors.GetSize();
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(uiValue, false);
	}
	*reinterpret_cast<uint32_t*>(apChunkBuffer) = uiValue;
	apChunkBuffer += sizeof(uint32_t);
	uiCount = ConnectedDoors.GetSize();
	for (uint32_t i = 0; i < uiCount; ++i) {
		TESObjectREFR* pDoor = ConnectedDoors.GetAt(i);
		if (pDoor) {
			uiValue = pDoor->GetFormID();
			if (TESForm::QEndianSwapOnSave()) {
				ENDIAN(uiValue, false);
			}
			*reinterpret_cast<uint32_t*>(apChunkBuffer) = uiValue;
		} else {
			*reinterpret_cast<uint32_t*>(apChunkBuffer) = 0;
		}
		apChunkBuffer += sizeof(uint32_t);
	}
}

void NavMeshInfo::SetFromPortalChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber) {
	uint32_t icount;
	icount = 0;
	ourMemcpy(&icount, apChunkBuffer, sizeof(icount));
	if (apFile->QEndian()) {
		ENDIAN<uint32_t>(icount, 0);
	}
	AdjacentNavMeshes.SetSize(icount, 1);
	// The chunk is untyped bytes: a run of 4-byte counts and form IDs after the first count.
	uint32_t* uiCount = reinterpret_cast<uint32_t*>(apChunkBuffer + 4);
	for (uint32_t i = 0; i < icount; ++i) {
		uint32_t infoID = *uiCount;
		if (apFile->QEndian()) {
			ENDIAN<uint32_t>(infoID, 0);
		}
		TESForm::AddCompileIndex(infoID, apFile);
		if (infoID) {
			uint32_t v9 = infoID;
			NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
			NavMeshInfo* NavMeshInfo_ = NavMeshInfoMap_->GetNavMeshInfo(v9);
			AdjacentNavMeshes.GetAt(static_cast<int32_t>(i)) = NavMeshInfo_;
		} else {
			AdjacentNavMeshes.GetAt(static_cast<int32_t>(i)) = 0;
		}
		++uiCount;
	}
	if (aiFileVersionNumber >= 11) {
		icount = 0;
		ourMemcpy(&icount, uiCount, sizeof(icount));
		if (apFile->QEndian()) {
			ENDIAN<uint32_t>(icount, 0);
		}
		PreferredAdjacentNavMeshes.SetSize(icount, 1);
		++uiCount;
		for (uint32_t i_2 = 0; i_2 < icount; ++i_2) {
			uint32_t infoID_2 = *uiCount;
			if (apFile->QEndian()) {
				ENDIAN<uint32_t>(infoID_2, 0);
			}
			TESForm::AddCompileIndex(infoID_2, apFile);
			if (infoID_2) {
				uint32_t v10 = infoID_2;
				NavMeshInfoMap* v6 = TES::pInstance->GetNavMeshInfoMap();
				NavMeshInfo* v7 = v6->GetNavMeshInfo(v10);
				PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i_2)) = v7;
			} else {
				PreferredAdjacentNavMeshes.GetAt(static_cast<int32_t>(i_2)) = 0;
			}
			++uiCount;
		}
	}
	ourMemcpy(&icount, uiCount, sizeof(icount));
	if (apFile->QEndian()) {
		ENDIAN<uint32_t>(icount, 0);
	}
	ConnectedDoors.SetSize(icount, 1);
	uint32_t* uiCounta = uiCount + 1;
	for (uint32_t i_3 = 0; i_3 < icount; ++i_3) {
		uint32_t infoID_3 = *uiCounta;
		if (apFile->QEndian()) {
			ENDIAN<uint32_t>(infoID_3, 0);
		}
		TESForm::AddCompileIndex(infoID_3, apFile);
		TESObjectREFR** v8 = &ConnectedDoors.GetAt(static_cast<int32_t>(i_3));
		// The door's form ID stays in the pointer field until InitDoorPointers resolves it.
		*v8 = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(infoID_3));
		++uiCounta;
	}
}

void NavMeshInfo::CopyTo(NavMeshInfo* apDestInfo) {
	apDestInfo->NavMeshID = NavMeshID;
	apDestInfo->ParentSpaceID = ParentSpaceID;
	apDestInfo->uiFlags = uiFlags;
	apDestInfo->iCellKey = iCellKey;
	apDestInfo->ApproxLocation = ApproxLocation;
	apDestInfo->pParentSpace = pParentSpace;
	apDestInfo->pNavMesh = pNavMesh;
	AdjacentNavMeshes.CopyTo(&apDestInfo->AdjacentNavMeshes);
	PreferredAdjacentNavMeshes.CopyTo(&apDestInfo->PreferredAdjacentNavMeshes);
	ConnectedDoors.CopyTo(&apDestInfo->ConnectedDoors);
	if (pBounds) {
		if (!apDestInfo->pBounds) {
			NavMeshBounds* v2;
			NavMeshBounds* apMemory = static_cast<NavMeshBounds*>(ourMalloc(sizeof(NavMeshBounds)));
			if (apMemory) {
				v2 = ::new (apMemory) NavMeshBounds;
			} else {
				v2 = 0;
			}
			apDestInfo->pBounds = v2;
		}
		pBounds->CopyTo(apDestInfo->pBounds);
	} else {
		if (apDestInfo->pBounds) {
			delete apDestInfo->pBounds;
		}
		apDestInfo->pBounds = 0;
	}
}

void NavMeshInfo::SetAltered(bool abAltered) {}

bool NavMeshInfo::GetAltered() const {
	return false;
}

void NavMeshInfo::operator delete(void* apMemory) {
	ourFree(apMemory);
}

uintptr_t NavMeshInfo::GetCellOrCoords(NavMeshInfo* a1) {
	TESObjectCELL* pCell = a1->GetInteriorCell();
	// An interior cell's key is its address as an integer (a pointer-to-integer conversion, not a reinterpretation).
	if (pCell != nullptr) {
		return reinterpret_cast<uintptr_t>(pCell);
	}
	return static_cast<uint32_t>(a1->iCellKey);
}

char* NavMeshInfo::GetCellOrCoords(char* apFirst, int32_t aiUnused) {
	return apFirst;
}
