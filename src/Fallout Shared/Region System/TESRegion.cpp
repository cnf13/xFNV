#include "Fallout Shared/Region System/TESRegion.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataManager.h"
#include "Fallout Shared/Region System/TESRegionDataWeather.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"

TESRegion::TESRegion()
	: TESForm(), pDataList(new TESRegionDataList(true)), pPointLists(new BSSimpleList<TESRegionPointList*>()),
	  pWorldSpace(nullptr), pCurrentWeather(nullptr), fLastWeatherUpdate(-1.0f), EmittanceColor(0.0f, 0.0f, 0.0f) {
	cFormType = REGN_ID;
}

// What Destruct (the exe's destructor body) does besides TESForm's destructor.
TESRegion::~TESRegion() {
	if (pDataList) {
		delete pDataList;
	}
	if (pPointLists) {
		while (TESRegionPointList* pPointList = pPointLists->m_item) {
			pPointLists->RemoveHead();
			delete pPointList;
		}
		delete pPointLists;
	}
}

void TESRegion::ClearData() {
	TESRegionPointList* v2;
	if (pDataList) {
		pDataList->Clear();
	}
	if (pPointLists) {
		while (1) {
			v2 = pPointLists->m_item;
			if (!v2) {
				break;
			}
			pPointLists->RemoveHead();
			delete v2;
		}
	}
}

// Reads the region's chunks: EDID, OBND, the worldspace's form ID (WNAM), the point lists (RPLI and RPLD) and the
// region data records, which go to the region data manager; then drops every point list whose outline fails
// validation.
// The exe reads the EDID buffer uninitialised when the chunk is empty (malformed data); src initialises it to ""
// (the EDID alloca'd buffer: one zeroed byte, allocated only for an empty chunk).
bool TESRegion::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	TESRegionPointList* pPointList = nullptr;
	if (cFormType != REGN_ID) {
		return false;
	}
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		switch (uiChunk) {
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			static_cast<TESForm*>(this)->LoadObjectBound(apFile);
			break;
		case WNAM_ID: {
			uint32_t uiWorldSpaceID = 0;
			apFile->GetChunkData<uint32_t>(uiWorldSpaceID);
			// The chunk holds a form ID that stands in for the worldspace.
			SetWorldSpace(reinterpret_cast<TESWorldSpace*>(static_cast<uintptr_t>(uiWorldSpaceID)));
			break;
		}
		case RPLI_ID: {
			uint32_t uiValue = 0;
			apFile->GetChunkData<uint32_t>(uiValue);
			void* pMemory = ourMalloc(sizeof(TESRegionPointList));
			TESRegionPointList* pNewList = nullptr;
			if (pMemory) {
				pNewList = ::new (pMemory) TESRegionPointList(true);
			}
			pPointList = pNewList;
			pPointList->SetDistanceInsideAtMax(uiValue);
			pPointLists->AddTail(pPointList);
			break;
		}
		case RPLD_ID:
			if (pPointList) {
				pPointList->Load(apFile);
			}
			break;
		case RDSI_ID:
		case RDOB_ID:
		case RDSB_ID:
		case RDID_ID:
		case RDSD_ID:
		case RDMD_ID:
		case RDMP_ID:
		case RDLN_ID:
		case RDOJ_ID:
		case ICON_ID:
		case RDOT_ID:
		case RDGS_ID:
		case RDAT_ID:
		case RDWT_ID: {
			// The region data records go to the data manager, which makes the region's data objects.
			TESRegionDataManager* pManager = pDataHandler->GetRegionDataManager();
			pManager->LoadRegionData(apFile, this);
			break;
		}
		}
	} while (apFile->NextChunk());

	// Drop every point list whose outline fails validation, and restart from the head.
	BSSimpleList<TESRegionPointList*>* pIter = pPointLists;
	while (pIter && pIter->GetItem()) {
		TESRegionPointList* pList = pIter->GetItem();
		if (!pList->Validate(true)) {
			TESRegionPointList* pRemoved = pIter->GetItem();
			pPointLists->Remove(pRemoved);
			if (pRemoved) {
				delete pRemoved;
			}
			pIter = pPointLists;
			continue;
		}
		pIter = pIter->GetNext();
	}
	return true;
}

void TESRegion::Save() {
	StartForm();
	TESWorldSpace* pworld = pWorldSpace;
	if (pworld) {
		TESForm::AddChunk(WNAM_ID, pworld->iFormID);
	}
	for (BSSimpleList<TESRegionPointList*>* pPList = pPointLists; pPList && !pPList->IsEmpty();
		pPList = pPList->m_pkNext) {
		pPList->m_item->Save();
	}
	for (BSSimpleList<TESRegionData*>* pDataList = this->pDataList; pDataList && !pDataList->IsEmpty();
		pDataList = pDataList->m_pkNext) {
		TESRegionData* m_item = pDataList->m_item;
		m_item->Save();
	}
	CloseForm();
}

void TESRegion::InitItem() {
	if (!GetInitialized()) {
		// Until InitItem runs, the worldspace field holds the form ID Load read from WNAM.
		uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWorldSpace));
		TESFile* File = GetFile(-1);
		TESForm::AddCompileIndex(itempint, File);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
		TESWorldSpace* pworld = dynamic_cast<TESWorldSpace*>(FormByNumericID);
		if (pworld) {
			SetWorldSpace(pworld);
		}
		SetInitialized(1);
		for (BSSimpleList<TESRegionData*>* pList = pDataList; pList && pList->m_item; pList = pList->m_pkNext) {
			TESRegionData* m_item = pList->m_item;
			m_item->InitItem(this);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
TESRegion* TESRegion::Copy(TESRegion* apCopyInto) {
	TESRegion* pcopy;
	if (apCopyInto) {
		pcopy = apCopyInto;
	} else {
		TESRegion* pcopy_2;
		void* apMemory = ourMalloc(sizeof(TESRegion));
		if (apMemory) {
			pcopy_2 = ::new (apMemory) TESRegion;
		} else {
			pcopy_2 = 0;
		}
		pcopy = pcopy_2;
	}
	BSSimpleList<TESRegionPointList*>* pcurrent = pPointLists;
	TESForm::Copy(pcopy);
	pDataList->Copy(pcopy->pDataList);
	while (pcurrent && pcurrent->m_item) {
		TESRegionPointList* a2 = pcurrent->m_item->Copy(0);
		pcopy->pPointLists->AddHead(a2);
		pcurrent = pcurrent->m_pkNext;
	}
	return pcopy;
}

bool TESRegion::Validate() {
	BSSimpleList<TESRegionPointList*>* pcheck = pPointLists;
	if (!pcheck || !pDataList) {
		return 0;
	}
	if (!pDataList->Validate()) {
		return 0;
	}
	while (pcheck && pcheck->m_item) {
		if (!pcheck->m_item->Validate(1)) {
			return 0;
		}
		pcheck = pcheck->m_pkNext;
	}
	return 1;
}

void TESRegion::SetWorldSpace(TESWorldSpace* apWorldSpace) {
	pWorldSpace = apWorldSpace;
	if (pWorldSpace) {
		if (TESForm::GetHasSpokenFlag() && (!pDataHandler->GetLoadingFiles())) {
			pWorldSpace->SetHasBorderRegion(1);
		}
	}
}

void TESRegion::UpdateWeather() {
	TESRegionData* v1;
	TESRegionDataWeather* v3;
	pCurrentWeather = nullptr;
	if (pDataList) {
		v1 = pDataList->Find(REGION_DATA_WEATHER_ID);
		v3 = dynamic_cast<TESRegionDataWeather*>(v1);
		if (v3) {
			pCurrentWeather = v3->GetRandomWeather();
		}
	}
}

void TESRegion::UpdateHUDRegionIndicator(const HUDMainMenu::HUDRegionData& aRegionData) {
	// The last region shown: UpdateHUDRegionIndicator stores
	// HUDRegionData::pRegion.
	static TESRegion* pLastRegion;
	if (HUDMainMenu::pme) {
		if (!aRegionData.bUpdateLastRegion || aRegionData.pRegion != pLastRegion) {
			if (aRegionData.bUpdateLastRegion) {
				pLastRegion = aRegionData.pRegion;
			}
			bool v5 = true;
			if (HUDMainMenu::pme->pTiles[15]->GetFloat(4009) != 0.0) {
				const char* String = HUDMainMenu::pme->pTiles[15]->GetString(4036);
				if (BSstrcmp(aRegionData.strDisplayText, String)) {
					v5 = false;
				}
			}
			if (aRegionData.strDisplayText[0]) {
				if (HUDMainMenu::pme->pTiles[16]->GetFloat(4009) == 0.0 &&
					HUDMainMenu::pme->pTiles[16]->GetFadeFinished(4009)) {
					int32_t v2 = Tile::TextToTrait("_Hold");
					HUDMainMenu::pme->pTiles[15]->Set(v2, 0);
					HUDMainMenu::pme->pTiles[15]->Set(4036, aRegionData.strDisplayText, 1);
					if (!v5) {
						float afEnd = fHudOpacity.Float() * 255.0;
						HUDMainMenu::pme->pTiles[15]->AddFadeControl(4009, 0.0f, afEnd, 2.0f, Tile::TFCT_STANDARD);
						uint32_t Time = appTimer.iLastTime;
						HUDMainMenu::pme->pTiles[15]->Set(Tile::kTileValue_User0, Time);
					}
				}
			}
		}
	}
}

// Saves each region's form ID and last weather update to the old-format save, then writes their count back over the
// count's slot at the buffer position it had. The exe walks the list with the trivial accessors GetItem (0x6815C0)
// and GetNext (0x726070) and reads the form ID through the trivial accessor 0x84E3A0.
void TESRegion::SaveGame() {
	uint16_t usCount = 0;
	uint16_t* pusCountSlot = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
	pSaveLoadGameOLD->SaveGameDataOLD(&usCount, 2u);
	BSSimpleList<TESRegion*>* pNode = pDataHandler->GetRegionList();
	while (pNode && !pNode->IsEmpty()) {
		TESRegion* pRegion = pNode->GetItem();
		uint32_t uiFormID = 0;
		float fLastWeatherUpdate = 0.0f;
		if (pRegion) {
			uiFormID = pRegion->GetFormID();
			fLastWeatherUpdate = pRegion->GetLastWeatherUpdate();
		}
		pSaveLoadGameOLD->SaveNumericID(&uiFormID, 4u);
		pSaveLoadGameOLD->SaveGameDataOLD(&fLastWeatherUpdate, 4u);
		++usCount;
		pNode = pNode->GetNext();
	}
	*pusCountSlot = usCount;
}

TESRegionDataManager* TESDataHandler::GetRegionDataManager() {
	return pRegionDataManager;
}
