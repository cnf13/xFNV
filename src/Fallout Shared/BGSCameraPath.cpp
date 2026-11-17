#include "Fallout Shared/BGSCameraPath.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSCameraPathManager.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

void CAMERA_PATH_LINKS::Endian() {
	ENDIAN(uiParentPathID, false);
	ENDIAN(uiPrevPathID, false);
}

BGSCameraPath::BGSCameraPath() : pChildPaths(nullptr) {
	SetFormType(CPTH_ID);
	BGSCameraPath::InitializeData();
}

BGSCameraPath::~BGSCameraPath() {
	BGSCameraPath::ClearData();
	if (!pDataHandler->GetClearingData()) {
		if (pParentPath) {
			pParentPath->RemoveChild(this);
		} else {
			NiFormArray* pArray = &pCameraPathManager->PathArray;
			if (pArray) {
				uint32_t iIndex = pArray->GetIndexForForm(this);
				if (iIndex != 0xFFFFFFFF) {
					// The next root path takes over this one's previous path.
					BGSCameraPath* pNextPath = static_cast<BGSCameraPath*>(pArray->GetFormAt(iIndex + 1));
					if (pNextPath) {
						pNextPath->pPrevPath = pPrevPath;
					}
					pArray->RemoveAt(iIndex);
					pArray->Compact();
				}
			}
		}
	}
	if (pChildPaths) {
		uint32_t iCount = GetChildCount();
		for (uint32_t i = 0; i < iCount; ++i) {
			BGSCameraPath* pChildPath = GetChildByIndex(i);
			pChildPath->pParentPath = nullptr;
			delete pChildPath;
		}
		delete pChildPaths;
	}
}

void BGSCameraPath::InitializeData() {
	data.cFlags = (uint8_t)0;
	pParentPath = nullptr;
	pPrevPath = nullptr;
}

void BGSCameraPath::ClearData() {
	Conditions.ClearAllConditionItems();
	(&ShotList)->RemoveAll();
}

// Reads the path's conditions, flags, parent and previous paths and its shot list. The default path is dropped from
// the manager's path array first.
bool BGSCameraPath::Load(TESFile* apFile) {
	if (apFile->GetTESForm() != CPTH_ID) {
		return false;
	}
	if (this == BGSCameraPathManager::GetDefaultPath()) {
		NiFormArray* pPaths = &pCameraPathManager->PathArray;
		uint32_t uiIndex = pPaths->GetIndexForForm(this);
		if (uiIndex != (uint32_t)(-1)) {
			pPaths->RemoveAt(uiIndex);
			pPaths->Compact();
		}
	}
	LoadForm(apFile);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		if (uiChunk == OBND_ID) {
			LoadObjectBound(apFile);
		} else if (uiChunk == CIDA_ID) {
			Conditions.Load(apFile);
		} else if (uiChunk == DATA_ID) {
			LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), 1);
		} else if (uiChunk == EDID_ID) {
			// A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
		} else if (uiChunk == ANAM_ID) {
			// The two form IDs are stored straight into the path pointers.
			CAMERA_PATH_LINKS kLinks;
			ourMemset(&kLinks, 0, sizeof(kLinks));
			apFile->GetChunkData(&kLinks, 8);
			if (apFile->QEndian()) {
				kLinks.Endian();
			}
			pParentPath = reinterpret_cast<BGSCameraPath*>(static_cast<uintptr_t>(kLinks.uiParentPathID));
			pPrevPath = reinterpret_cast<BGSCameraPath*>(static_cast<uintptr_t>(kLinks.uiPrevPathID));
			pCameraPathManager->AddLoadedPath(this);
		} else if (uiChunk == SNAM_ID) {
			uint32_t uiShotID = 0;
			apFile->GetChunkData(uiShotID);
			BGSCameraShot* pShot = reinterpret_cast<BGSCameraShot*>(static_cast<uintptr_t>(uiShotID));
			ShotList.AddTail(pShot);
		}
	} while (apFile->NextChunk());
	return true;
}

void BGSCameraPath::Save() {
	CAMERA_PATH_LINKS kLinks;
	StartForm();
	(&Conditions)->Save();
	ourMemset(&kLinks, 0, sizeof(kLinks));
	if (pParentPath) {
		kLinks.uiParentPathID = pParentPath->iFormID;
	}
	if (!GetDelete()) {
		BGSCameraPath* pprev;
		for (pprev = pPrevPath; pprev && pprev->GetDelete(); pprev = pprev->pPrevPath) {
			;
		}
		if (pprev) {
			kLinks.uiPrevPathID = pprev->iFormID;
		}
	}
	if (TESForm::QEndianSwapOnSave()) {
		kLinks.Endian();
	}
	TESForm::__AddChunkData(ANAM_ID, &kLinks, 8u);
	if (TESForm::QEndianSwapOnSave()) {
		kLinks.Endian();
	}
	if (TESForm::QEndianSwapOnSave()) {
	}
	__SaveData(&data, 1u);
	if (TESForm::QEndianSwapOnSave()) {
	}
	BSSimpleList<BGSCameraShot*>* plist = &ShotList;
	while (plist && !plist->IsEmpty()) {
		BGSCameraShot* pshot = plist->m_item;
		plist = plist->m_pkNext;
		TESForm::AddChunk(SNAM_ID, pshot->iFormID);
	}
	CloseForm();
}

TESForm* BGSCameraPath::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
	TESForm* DuplicateForm = TESForm::CreateDuplicateForm(abCreateEditorID, apCopyMap);
	BGSCameraPath* pduppath = dynamic_cast<BGSCameraPath*>(DuplicateForm);
	TESForm* pform = 0;
	if (!apCopyMap->GetAt(pParentPath, pform) || !pform) {
		pform = (TESForm*)pParentPath;
	}
	pduppath->pParentPath = static_cast<BGSCameraPath*>(pform);
	pform = 0;
	if (!apCopyMap->GetAt(pPrevPath, pform) || !pform) {
		pform = (TESForm*)pPrevPath;
	}
	pduppath->pPrevPath = static_cast<BGSCameraPath*>(pform);
	if (pChildPaths) {
		pChildPaths->Compact();
		uint32_t icount = pChildPaths->m_uiSize;
		NiFormArray* pduparray = pduppath->CreateChildArray();
		pduparray->SetSize(icount);
		BGSCameraPath* pdupchild = 0;
		for (uint32_t i = 0; i < icount; ++i) {
			BGSCameraPath* pchildpath = static_cast<BGSCameraPath*>(pChildPaths->GetAt(i));
			BGSCameraPath* pdupchild_2 =
				static_cast<BGSCameraPath*>(pchildpath->CreateDuplicateForm(abCreateEditorID, apCopyMap));
			BGSCameraPath* pdupchild_3 = pdupchild_2;
			pduparray->SetAt(i, pdupchild_3);
			pdupchild = pdupchild_2;
		}
	}
	return (TESForm*)pduppath;
}

void BGSCameraPath::InitItem() {
	if (!GetInitialized()) {
		(&Conditions)->InitItem(this);
		BSSimpleList<BGSCameraShot*>* plist = &ShotList;
		BSSimpleList<BGSCameraShot*>* plastlist = 0;
		while (plist && !plist->IsEmpty()) {
			uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(tempint, File);
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
			BGSCameraShot* pshot = dynamic_cast<BGSCameraShot*>(FormByNumericID);
			if (pshot) {
				plist->SetItem(pshot);
				plastlist = plist;
				plist = plist->m_pkNext;
			} else {
				if (plastlist) {
					BSSimpleList<BGSCameraShot*>* v3 = plist;
					plastlist->Remove(v3->m_item);
					plist = plastlist->m_pkNext;
				} else {
					plist->RemoveHead();
				}
				if (GetFormEditorIDLength()) {
					const char* v4 = GetFormEditorID();
					Warning("MASTERFILE: Unable to find Camera Shot Form (%08X) for Camera Path \"%s\".", tempint, v4);
				} else {
					int32_t CastingType = iFormID;
					Warning("MASTERFILE: Unable to find Camera Shot Form (%08X) for Camera Path (%08X).", tempint,
						CastingType);
				}
			}
		}
		SetInitialized(1);
	}
}

void BGSCameraPath::Copy(TESForm* apCopy) {
	BGSCameraPath* pcopy = dynamic_cast<BGSCameraPath*>(apCopy);
	if (pcopy) {
		CopyAllComponents(pcopy);
		(&Conditions)->Copy(&pcopy->Conditions, 0);
		ourMemcpy(&data, &pcopy->data, 1u);
		ShotList.RemoveAll();
		BSSimpleList<BGSCameraShot*>* plist = &pcopy->ShotList;
		while (plist && !plist->IsEmpty()) {
			BGSCameraShot* pshot = plist->m_item;
			plist = plist->GetNext();
			ShotList.AddTail(pshot);
		}
	}
}

bool BGSCameraPath::Compare(TESForm* apCompare) {
	BGSCameraPath* pcomp = dynamic_cast<BGSCameraPath*>(apCompare);
	if (!pcomp) {
		return 1;
	}
	if (CompareAllComponents(pcomp)) {
		return 1;
	}
	if ((&Conditions)->Compare(&pcomp->Conditions)) {
		return 1;
	}
	if (std::memcmp(&data, &pcomp->data, 1u)) {
		return 1;
	}
	BSSimpleList<BGSCameraShot*>* plist = &ShotList;
	BSSimpleList<BGSCameraShot*>* pcomplist = &pcomp->ShotList;
	while (pcomplist && !pcomplist->IsEmpty()) {
		if (!plist) {
			return 1;
		}
		BSSimpleList<BGSCameraShot*>* v3 = plist;
		if (v3->m_item != pcomplist->m_item) {
			return 1;
		}
		pcomplist = pcomplist->m_pkNext;
		plist = plist->m_pkNext;
	}
	return plist && !plist->IsEmpty();
}

bool BGSCameraPath::GetAllowNoShots() {
	return ((data.cFlags & 0x80) != 0);
}

bool BGSCameraPath::GetZoomDisable() {
	return ((data.cFlags & 1) != 0);
}

bool BGSCameraPath::GetZoomShotList() {
	return ((data.cFlags & 2) != 0);
}

uint32_t BGSCameraPath::GetIndexForChild(BGSCameraPath* apChild) {
	uint32_t v3 = (uint32_t)((-1));
	if (pChildPaths) {
		return pChildPaths->GetIndexForForm(apChild);
	}
	return v3;
}

void BGSCameraPath::RemoveChildAtIndex(uint32_t aiIndex) {
	if (pChildPaths) {
		BGSCameraPath* pnextpath = GetChildByIndex(aiIndex + 1);
		BGSCameraPath* pprevpath = GetChildByIndex(aiIndex - 1);
		if (pnextpath) {
			pnextpath->pPrevPath = pprevpath;
		}
		pChildPaths->RemoveAt(aiIndex);
		pChildPaths->Compact();
	}
}

BGSCameraPath* BGSCameraPath::GetChildByIndex(uint32_t aiIndex) {
	TESForm* formAt;
	BGSCameraPath* v4 = nullptr;
	if (pChildPaths) {
		formAt = pChildPaths->GetFormAt(aiIndex);
		return dynamic_cast<BGSCameraPath*>(formAt);
	}
	return v4;
}

BGSCameraPath* BGSCameraPath::GetCameraPathToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
	BGSCameraPath* pCamera = 0;
	if (!GetDelete()) {
		if (Conditions.IsTrue(apActionRef, apTargetRef)) {
			BSSimpleList<BGSCameraShot*>* Filename;
			if (GetAllowNoShots() || (Filename = &ShotList, !Filename->IsEmpty())) {
				pCamera = this;
			}
			uint32_t NumChildren = GetChildCount();
			if (NumChildren) {
				BGSCameraPath* CameraPathToPlay = 0;
				for (uint32_t aiIndex = 0; aiIndex < NumChildren; ++aiIndex) {
					BGSCameraPath* ChildByIndex = GetChildByIndex(aiIndex);
					if (ChildByIndex) {
						CameraPathToPlay = ChildByIndex->GetCameraPathToPlay(apActionRef, apTargetRef);
					}
					if (CameraPathToPlay) {
						break;
					}
				}
				if (CameraPathToPlay) {
					return CameraPathToPlay;
				}
			}
		}
	}
	return pCamera;
}

uint32_t BGSCameraPath::GetChildCount() {
	uint32_t uiCount = 0;
	if (pChildPaths)
		uiCount = pChildPaths->GetSize();
	return uiCount;
}

bool BGSCameraPath::InitParents() {
	bool bretval = 1;
	BGSCameraPath* pparent = 0;
	uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pParentPath));
	if (itempint) {
		TESFile* File = GetFile(-1);
		TESForm::AddCompileIndex(itempint, File);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
		pparent = dynamic_cast<BGSCameraPath*>(FormByNumericID);
		if (!pparent) {
			BSStringT<char> tempstring;
			GetFormDetailedString(tempstring);
			char* v6 = tempstring.pString;
			Warning("DEFAULT: Could not find parent path (%08X) for %s.", itempint, v6);
			bretval = 0;
		}
		pParentPath = pparent;
	}
	BGSCameraPath* pprev = 0;
	itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrevPath));
	if (itempint) {
		TESFile* v3 = GetFile(-1);
		TESForm::AddCompileIndex(itempint, v3);
		TESForm* v4 = TESForm::GetFormByNumericID(itempint);
		pprev = dynamic_cast<BGSCameraPath*>(v4);
		if (!pprev) {
			BSStringT<char> tempstring_2;
			GetFormDetailedString(tempstring_2);
			char* v7 = tempstring_2.pString;
			Warning("DEFAULT: Could not find previous path (%08X) for %s.", itempint, v7);
			bretval = 0;
		}
		pPrevPath = pprev;
	}
	uint32_t iindex = 0;
	NiFormArray* parray = 0;
	if (pparent) {
		parray = pparent->CreateChildArray();
	} else {
		parray = &pCameraPathManager->PathArray;
	}
	if (parray) {
		if (pprev) {
			uint32_t iprevindex = parray->GetIndexForForm(pprev);
			if (iprevindex != -1) {
				++iprevindex;
			}
			iindex = iprevindex;
		}
		if (iindex == -1 || GetDelete()) {
			iindex = parray->m_uiSize + 1;
		}
		parray->InsertFormAt(iindex, this);
		FixNextPaths(parray);
	} else {
		BSStringT<char> tempstring_3;
		GetFormDetailedString(tempstring_3);
		Warning("DEFAULT: Could not find parent array for %s.", tempstring_3.String());
		bretval = 0;
	}
	return bretval;
}

void BGSCameraPath::RemoveChild(BGSCameraPath* apChild) {
	uint32_t indexForChild;
	if (apChild) {
		if (pChildPaths) {
			indexForChild = GetIndexForChild(apChild);
			RemoveChildAtIndex(indexForChild);
		}
	}
}

NiFormArray* BGSCameraPath::CreateChildArray() {
	NiFormArray* v2;
	if (pChildPaths) {
		return pChildPaths;
	}
	void* v5 = ourMalloc(sizeof(NiFormArray));
	if (v5) {
		v2 = ::new (v5) NiFormArray;
	} else {
		v2 = nullptr;
	}
	pChildPaths = v2;
	return v2;
}

bool BGSCameraPath::GetZoomDefault() {
	return (data.cFlags & 1) == 0 && (data.cFlags & 2) == 0;
}

void BGSCameraPath::FixNextPaths(NiFormArray* apArray) {
	if (apArray) {
		uint32_t izerocount = 0;
		uint32_t iindex = -1;
		uint32_t isize = apArray->m_uiSize;
		uint32_t inextindex = -1;
		BGSCameraPath* pnextpath = 0;
		for (uint32_t i = 0; i < isize; ++i) {
			BGSCameraPath* ppath = dynamic_cast<BGSCameraPath*>(apArray->GetAt(i));
			if (ppath) {
				if (ppath == this) {
					iindex = i;
				} else {
					if (ppath->pPrevPath == this) {
						if (pnextpath) {
							BSStringT<char> tempstring;
							GetFormDetailedString(tempstring);
							Warning("DEFAULT: Multiple next paths for %s.", tempstring.String());
						} else {
							pnextpath = ppath;
							inextindex = i;
						}
					}
				}
			} else {
				++izerocount;
			}
		}
		if (iindex != -1 && inextindex != -1 && inextindex != iindex + 1) {
			TESForm* a3 = 0;
			apArray->SetAt(inextindex, a3);
			apArray->Compact();
			if (inextindex < iindex) {
				++izerocount;
			}
			iindex -= izerocount;
			apArray->InsertFormAt(iindex + 1, pnextpath);
			apArray->Compact();
			pnextpath->FixNextPaths(apArray);
		}
	}
}

NiFormArray::~NiFormArray() {}
