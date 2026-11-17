#include "Fallout Shared/Region System/TESRegionDataGrass.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionGrassObject.h"
#include "Fallout Shared/Region System/TESRegionGrassObjectList.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLandTexture.h"

TESRegionDataGrass::TESRegionDataGrass() : TESRegionData(), pObjects(new TESRegionGrassObjectList(true)) {}

TESRegionDataGrass::TESRegionDataGrass(TESRegionDataGrass* apSource)
	: TESRegionData(apSource), pObjects(apSource->pObjects->Copy(nullptr)) {}

TESRegionDataGrass::~TESRegionDataGrass() {
	delete pObjects;
}

void TESRegionDataGrass::Save() {
	TESRegionGrassObjectList* v1 = GetObjectList();
	uint32_t icount = v1->iCount;
	TESRegionData::Save();
	if (icount) {
		BSSimpleList<TESRegionGrassObject*>* pList;
		TESRegionGrassObjectList* v6 = GetObjectList();
		if (v6) {
			pList = static_cast<BSSimpleList<TESRegionGrassObject*>*>(v6);
		} else {
			pList = 0;
		}
		BSSimpleList<TESRegionGrassObject*>* pList_2 = pList;
		uint32_t i = 0;
		TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA* pobjectArray =
			static_cast<TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA*>(ourMalloc(icount <=
						SIZE_MAX / sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA)
					? sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA) * icount
					: SIZE_MAX));
		while (pList_2) {
			TESRegionGrassObject* pitem = pList_2->m_item;
			if (pitem) {
				uint32_t CastingType;
				TESForm* v2 = pitem->GetObject();
				pobjectArray[i].iFormID = v2->iFormID;
				if (pitem->GetParentTexture()) {
					TESLandTexture* v3 = pitem->GetParentTexture();
					CastingType = v3->iFormID;
				} else {
					CastingType = 0;
				}
				pobjectArray[i++].iParentFormID = CastingType;
			}
			pList_2 = pList_2->m_pkNext;
		}
		if (TESForm::QEndianSwapOnSave()) {
			for (uint32_t i_2 = 0; i_2 < icount; ++i_2) {
				pobjectArray[i_2].Endian();
			}
		}
		TESForm::__AddChunkData(RDGS_ID, pobjectArray, 8 * icount);
		if (TESForm::QEndianSwapOnSave()) {
			for (uint32_t i_3 = 0; i_3 < icount; ++i_3) {
				pobjectArray[i_3].Endian();
			}
		}
		ourFree(pobjectArray);
	}
}

REGION_DATA_ID TESRegionDataGrass::GetID() const {
	return REGION_DATA_GRASS_ID;
}

TESRegionData* TESRegionDataGrass::Copy() {
	void* v4 = ourMalloc(sizeof(TESRegionDataGrass));
	if (v4) {
		return ::new (v4) TESRegionDataGrass(this);
	} else {
		return nullptr;
	}
}

// A copy of this data with apRegionData blended into it.
TESRegionData* TESRegionDataGrass::Blend(const TESRegionData* apRegionData) {
	TESRegionData* pCopy = Copy();
	pCopy->BlendInto(apRegionData, 2u);
	return pCopy;
}

void TESRegionDataGrass::BlendInto(const TESRegionData* a1, uint32_t a2) {
	// The source's cast: the ID test below makes a1 a grass entry, whose (non-const) GetObjectList the exe calls.
	TESRegionDataGrass* pGrass = const_cast<TESRegionDataGrass*>(static_cast<const TESRegionDataGrass*>(a1));
	if (a1 && (a1->GetID() == REGION_DATA_GRASS_ID && a2)) {
		if (!bIgnore) {
			if (pGrass->bIgnore) {
				return;
			}
			if (bOverride) {
				if (pGrass->bOverride) {
					int32_t FlyCamera = pGrass->cPriority;
					if (FlyCamera > cPriority) {
						SetOverride(pGrass->bOverride);
						uint8_t v7 = pGrass->cPriority;
						SetPriority(v7);
						TESRegionGrassObjectList* v8 = GetObjectList();
						v8->Clear();
						TESRegionGrassObjectList* a1a = GetObjectList();
						TESRegionGrassObjectList* v9 = pGrass->GetObjectList();
						v9->Copy(a1a);
					}
				}
				return;
			}
			if (!pGrass->bOverride) {
				float a1b = static_cast<float>(pGrass->cPriority);
				TESRegionGrassObjectList* v18 = pGrass->GetObjectList();
				float afPriority = static_cast<float>(cPriority);
				TESRegionGrassObjectList* v10 = GetObjectList();
				v10->MergeLists(afPriority, v18, a1b);
				int32_t v11 = cPriority;
				int32_t v12 = cPriority * v11;
				int32_t v13 = 100 - cPriority;
				float v24 = static_cast<float>(pGrass->cPriority * v13 + v12);
				int32_t v14 = 100 - pGrass->cPriority;
				int32_t v15 = cPriority * v14;
				int32_t v16 = pGrass->cPriority;
				float v25 = static_cast<double>(pGrass->cPriority * v16 + v15) + v24;
				float fnewPriority = v25 * 0.004999999888241291;
				int32_t v22 = static_cast<int32_t>(std::floor(fnewPriority));
				SetPriority(v22);
				return;
			}
		}
		SetOverride(pGrass->bOverride);
		uint8_t v3 = pGrass->cPriority;
		SetPriority(v3);
		TESRegionGrassObjectList* v4 = GetObjectList();
		v4->Clear();
		TESRegionGrassObjectList* a12 = GetObjectList();
		TESRegionGrassObjectList* v5 = pGrass->GetObjectList();
		v5->Copy(a12);
	}
}

bool TESRegionDataGrass::Validate() {
	return true;
}

// The exe's cleanup-only C++ EH frame is left out.
bool TESRegionDataGrass::LoadRegionDataGrass(TESFile* apFile) {
	if (!apFile || apFile->GetTESChunk() != RDGS_ID) {
		return 0;
	}
	uint32_t isize = apFile->GetChunkSize();
	uint32_t icount = isize >> 3;
	if (isize % 8) {
		Warning("MODELS: Invalid Region Grass Object Data in file \"%s\".", &apFile->m_Filename);
		return 0;
	} else {
		if (icount) {
			TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA* pdataArray =
				static_cast<TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA*>(ourMalloc(icount <=
							SIZE_MAX / sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA)
						? sizeof(TESRegionGrassObject::REGION_GRASS_OBJECT_FILE_DATA) * icount
						: SIZE_MAX));
			apFile->GetChunkData(pdataArray, isize);
			for (uint32_t i = 0; i < icount; ++i) {
				TESRegionGrassObject* pobj;
				if (apFile->QEndian()) {
					pdataArray[i].Endian();
				}
				TESRegionGrassObject* v6 = static_cast<TESRegionGrassObject*>(ourMalloc(sizeof(TESRegionGrassObject)));
				if (v6) {
					pobj = ::new (v6) TESRegionGrassObject(&pdataArray[i]);
				} else {
					pobj = 0;
				}
				if (pobj) {
					if (pobj->GetObject()) {
						TESRegionGrassObjectList* v3 = GetObjectList();
						v3->AddTailSimple(pobj);
					} else {
						delete pobj;
					}
				}
			}
			ourFree(pdataArray);
		}
		return 1;
	}
}
