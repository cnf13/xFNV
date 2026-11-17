#include "Fallout Shared/TESWorldSpace.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSPortalGraph.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MultiBoundMarkerData.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataGrass.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionDataManager.h"
#include "Fallout Shared/Region System/TESRegionDataMap.h"
#include "Fallout Shared/Region System/TESRegionGrassObject.h"
#include "Fallout Shared/Region System/TESRegionGrassObjectList.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGrass.h"
#include "Fallout Shared/TESGrassAreaParam.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <bit>

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bExternalLODDataFiles("bExternalLODDataFiles:General", true);
BSCriticalSection TESWorldSpace::PersistentDataCritSection;
SettingT<INISettingCollection> bCheckCellOffsetsOnInit("bCheckCellOffsetsOnInit:General", false);

void WorldSpaceMapData::Endian() {
	ENDIAN(iUsableWidth, false);
	ENDIAN(iUsableHeight, false);
	ENDIAN(sNorthWestCellX, false);
	ENDIAN(sNorthWestCellY, false);
	ENDIAN(sSouthEastCellX, false);
	ENDIAN(sSouthEastCellY, false);
}

void WorldSpaceMapOffsetData::Endian() {
	ENDIAN(fMapScale, false);
	ENDIAN(fCellOffsetX, false);
	ENDIAN(fCellOffsetY, false);
}

void DEFAULT_CELL_DATA::Endian() {
	ENDIAN(fLandHeight, false);
	ENDIAN(fWaterHeight, false);
}

TESWorldSpace::OFFSET_DATA::OFFSET_DATA() {}

TESWorldSpace::TESWorldSpace()
	: pPersistentCell(nullptr), pTerrainManager(nullptr), FixedPersistentRefMap(37u), pOverlappedMultiboundMap(nullptr),
	  spPortalGraph(nullptr), OffsetDataMap(37u) {
	cFormType = 65;
	pCellMap = new NiTPointerMap<int32_t, TESObjectCELL*>(37u);
	MinimumCoords.Y = 3.4028235e+38f;
	MinimumCoords.X = 3.4028235e+38f;
	MaximumCoords.Y = -3.4028235e+38f;
	MaximumCoords.X = -3.4028235e+38f;
	TESWorldSpace::InitializeData();
}

TESWorldSpace::~TESWorldSpace() {
	if (pCurrentWorldspace == this) {
		pCurrentWorldspace = nullptr;
	}
	ClearPersistentRefData();
	ClearMultiBoundRefs();
	if (pTerrainManager) {
		pTerrainManager->DestroyNodeAndChildren();
		delete pTerrainManager;
		pTerrainManager = nullptr;
	}
	TESWorldSpace::ClearData();
	ClearCellMap();
	delete pCellMap;
	pCellMap = nullptr;
	delete pPersistentCell;
	pPersistentCell = nullptr;
	FreeOffsetData();
}

// The two texture components are cleared through InitializeDataComponent; the portal graph is released through
// NiPointer::operator=.
void TESWorldSpace::InitializeData() {
	pParentWorld = nullptr;
	pEncounterZone = nullptr;
	cFlags = 0;
	sParentUseFlags = 0;
	if (!IsDefaultForm()) {
		SetSmallWorld(true);
	}
	pClimate = nullptr;
	pImageSpace = nullptr;
	pWorldWater = nullptr;
	pLODWater = nullptr;
	fLODWaterHeight = 0.0f;
	WorldMapOffsetData.fMapScale = 1.0f;
	WorldMapOffsetData.fCellOffsetX = 0.0f;
	WorldMapOffsetData.fCellOffsetY = 0.0f;
	ClearSharableData();
	pMusicType = nullptr;
	pImpactSwap = nullptr;
	fDefaultLandHeight = -2048.0f;
	fDefaultWaterHeight = 0.0f;
	spPortalGraph = nullptr;
	BaseFormComponent* pComponent = &CanopyShadowTexture;
	pComponent->InitializeDataComponent();
	pComponent = &WaterNoiseTexture;
	pComponent->InitializeDataComponent();
}

void TESWorldSpace::ClearData() {
	(&spPortalGraph)->operator=(nullptr);
	if (pImpactSwap) {
		delete pImpactSwap;
	}
	pImpactSwap = nullptr;
}

// The record's chunks: the cell offsets of a master file, the names, textures and flags, the climate, water, image
// space, music and encounter zone IDs (kept in the pointers until InitItem resolves them), the parent and its use
// flags, the map data, the default heights, the impact swaps and the cell bounds. A worldspace with no cells that is
// not a small world then gets a fresh cell map, larger for the built-in worldspaces. GetChunkData writes just the
// chunk's bytes, so for an empty or short chunk (malformed data) each buffer is initialised to "" (one zeroed byte,
// allocated only for an empty chunk) and the DNAM, NAM0 and NAM9 locals to 0.
bool TESWorldSpace::Load(TESFile* apFile) {
	if (static_cast<uint8_t>(apFile->GetTESForm()) != 0x41) {
		return false;
	}
	if (apFile->GetMaster()) {
		OFFSET_DATA* pdata = CreateOffsetData(apFile);
		pdata->iFileOffset = apFile->GetOffset();
	}
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t chunkid = apFile->GetTESChunk();
		if (chunkid == NO_CHUNK) {
			break;
		}
		switch (chunkid) {
		case EDID_ID: {
			// A 16-byte aligned stack buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pMessage = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// An empty chunk leaves the buffer unwritten (see above).
			pMessage[0] = '\0';
			apFile->GetChunkData(pMessage, 0x200);
			SetFormEditorID(pMessage);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case ICON_ID:
			TESTexture::LoadTextureChunk(this, apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case DATA_ID:
			if (apFile->GetChunkSize() == 4) {
				uint32_t ival = 0;
				apFile->GetChunkData(ival);
				cFlags = static_cast<uint8_t>(ival);
			} else {
				apFile->GetChunkData(&cFlags, 1);
			}
			break;
		case OFST_ID: {
			uint32_t ifilesize = apFile->GetChunkSize();
			if (ifilesize) {
				uint32_t ioffsetcount = ifilesize >> 2;
				OFFSET_DATA* poffsetData = CreateOffsetData(apFile);
				if (poffsetData->pCellFileOffsets) {
					ourFree(poffsetData->pCellFileOffsets);
				}
				// Allocates ioffsetcount uint32_t entries through ourMalloc; the size cannot overflow, as ioffsetcount
				// is at most 0x3FFFFFFF.
				poffsetData->pCellFileOffsets = static_cast<uint32_t*>(ourMalloc(ioffsetcount * sizeof(uint32_t)));
				apFile->GetChunkData(poffsetData->pCellFileOffsets, 0);
				if (apFile->QEndian()) {
					for (uint32_t i = 0; i < ioffsetcount; ++i) {
						ENDIAN(poffsetData->pCellFileOffsets[i], false);
					}
				}
			}
			break;
		}
		// The ID chunks below hold form IDs, which are kept in the pointers (untyped until InitItem resolves them).
		case XEZN_ID: {
			uint32_t itempint = 0;
			apFile->GetChunkData(itempint);
			pEncounterZone = reinterpret_cast<BGSEncounterZone*>(static_cast<uintptr_t>(itempint));
			break;
		}
		case CNAM_ID: {
			uint32_t tempint = 0;
			apFile->GetChunkData(tempint);
			SetClimate(reinterpret_cast<TESClimate*>(static_cast<uintptr_t>(tempint)));
			break;
		}
		case INAM_ID: {
			uint32_t tempint = 0;
			apFile->GetChunkData(tempint);
			SetImageSpace(reinterpret_cast<TESImageSpace*>(static_cast<uintptr_t>(tempint)));
			break;
		}
		case NAM2_ID: {
			uint32_t tempint = 0;
			apFile->GetChunkData(tempint);
			SetWorldWater(reinterpret_cast<TESWaterForm*>(static_cast<uintptr_t>(tempint)));
			break;
		}
		case NAM3_ID: {
			uint32_t tempint = 0;
			apFile->GetChunkData(tempint);
			SetLODWater(reinterpret_cast<TESWaterForm*>(static_cast<uintptr_t>(tempint)));
			break;
		}
		case NAM4_ID: {
			float tempfloat = 0.0f;
			apFile->GetChunkData(tempfloat);
			SetWaterLODHeight(tempfloat);
			break;
		}
		case WNAM_ID: {
			uint32_t tempint = 0;
			apFile->GetChunkData(tempint);
			SetParentWorld(reinterpret_cast<TESWorldSpace*>(static_cast<uintptr_t>(tempint)));
			sParentUseFlags = 0xFFFF;
			break;
		}
		case PNAM_ID:
			// LoadWord reads into a signed word; the flags are the same 2 bytes, unsigned.
			apFile->LoadWord(reinterpret_cast<int16_t*>(&sParentUseFlags));
			break;
		case DNAM_ID: {
			DEFAULT_CELL_DATA defdata = {};
			apFile->GetChunkData(&defdata, 8);
			if (apFile->QEndian()) {
				defdata.Endian();
			}
			fDefaultLandHeight = defdata.fLandHeight;
			fDefaultWaterHeight = defdata.fWaterHeight;
			break;
		}
		case MNAM_ID:
			apFile->GetChunkData(&WorldMapData, 0x10);
			if (apFile->QEndian()) {
				WorldMapData.Endian();
			}
			break;
		case ONAM_ID:
			apFile->GetChunkData(&WorldMapOffsetData, 0xC);
			if (apFile->QEndian()) {
				WorldMapOffsetData.Endian();
			}
			break;
		case SNAM_ID: {
			// Reads the ID and drops it.
			uint32_t ival = 0;
			apFile->GetChunkData(ival);
			break;
		}
		case ZNAM_ID: {
			uint32_t tempint = 0;
			apFile->GetChunkData(tempint);
			SetMusicType(reinterpret_cast<BGSMusicType*>(static_cast<uintptr_t>(tempint)));
			break;
		}
		case IMPF_ID:
		case IMPS_ID:
			if (!pImpactSwap) {
				void* pMemory = ourMalloc(sizeof(ImpactSwap));
				pImpactSwap = pMemory ? ::new (pMemory) ImpactSwap : nullptr;
			}
			pImpactSwap->Load(apFile);
			break;
		case NNAM_ID: {
			uint32_t isize = apFile->GetChunkSize();
			if (isize) {
				// A 16-byte aligned stack buffer of the chunk's size.
				char* ptemp = static_cast<char*>(__builtin_alloca_with_align(isize, 128));
				apFile->GetChunkData(ptemp, isize);
				CanopyShadowTexture.SetTextureName(ptemp);
			}
			break;
		}
		case XNAM_ID: {
			uint32_t isize = apFile->GetChunkSize();
			if (isize) {
				// A 16-byte aligned stack buffer of the chunk's size.
				char* ptemp = static_cast<char*>(__builtin_alloca_with_align(isize, 128));
				apFile->GetChunkData(ptemp, isize);
				WaterNoiseTexture.SetTextureName(ptemp);
			}
			break;
		}
		case NAM0_ID: {
			NiPoint2 tmpCoords = {};
			apFile->GetChunkData(&tmpCoords, 8);
			if (apFile->QEndian()) {
				ENDIAN(tmpCoords.x, false);
				ENDIAN(tmpCoords.y, false);
			}
			MinimumCoords.X = BSMIN(MinimumCoords.X, tmpCoords.x);
			MinimumCoords.Y = BSMIN(MinimumCoords.Y, tmpCoords.y);
			if (apFile->GetMaster()) {
				OFFSET_DATA* pdata = CreateOffsetData(apFile);
				pdata->OffsetMinCoords.x = tmpCoords.x;
				pdata->OffsetMinCoords.y = tmpCoords.y;
			}
			break;
		}
		case NAM9_ID: {
			NiPoint2 tmpCoords = {};
			apFile->GetChunkData(&tmpCoords, 8);
			if (apFile->QEndian()) {
				ENDIAN(tmpCoords.x, false);
				ENDIAN(tmpCoords.y, false);
			}
			MaximumCoords.X = BSMAX(MaximumCoords.X, tmpCoords.x);
			MaximumCoords.Y = BSMAX(MaximumCoords.Y, tmpCoords.y);
			if (apFile->GetMaster()) {
				OFFSET_DATA* pdata = CreateOffsetData(apFile);
				pdata->OffsetMaxCoords.x = tmpCoords.x;
				pdata->OffsetMaxCoords.y = tmpCoords.y;
			}
			break;
		}
		default:
			break;
		}
	} while (apFile->NextChunk());
	if (!pCellMap->GetCount() && !GetSmallWorld()) {
		// The map is deleted here and the field is replaced below.
		delete pCellMap;
		if (IsDefaultForm()) {
			void* pMemory = ourMalloc(sizeof(NiTPointerMap<int32_t, TESObjectCELL*>));
			pCellMap = pMemory ? ::new (pMemory) NiTPointerMap<int32_t, TESObjectCELL*>(7001) : nullptr;
		} else {
			void* pMemory = ourMalloc(sizeof(NiTPointerMap<int32_t, TESObjectCELL*>));
			pCellMap = pMemory ? ::new (pMemory) NiTPointerMap<int32_t, TESObjectCELL*>(701) : nullptr;
		}
	}
	return true;
}

bool TESWorldSpace::LoadPartial(TESFile* apFile) {
	NiPoint2 tmpCoords_2;
	NiPoint2 tmpCoords;
	CHUNK_ID chunkid = NO_CHUNK;
	do {
		chunkid = apFile->GetTESChunk();
		if (chunkid == NO_CHUNK) {
			break;
		}
		if (chunkid == NAM0_ID) {
			apFile->GetChunkData(&tmpCoords, 8u);
			if (apFile->QEndian()) {
				ENDIAN<float>(tmpCoords.x, 0);
				ENDIAN<float>(tmpCoords.y, 0);
			}
			MinimumCoords.X = BSMIN(MinimumCoords.X, tmpCoords.x);
			MinimumCoords.Y = BSMIN(MinimumCoords.Y, tmpCoords.y);
			if (apFile->GetMaster()) {
				TESWorldSpace::OFFSET_DATA* pdata = CreateOffsetData(apFile);
				pdata->OffsetMinCoords = tmpCoords;
			}
		} else {
			if (chunkid == NAM9_ID) {
				apFile->GetChunkData(&tmpCoords_2, 8u);
				if (apFile->QEndian()) {
					ENDIAN<float>(tmpCoords_2.x, 0);
					ENDIAN<float>(tmpCoords_2.y, 0);
				}
				MaximumCoords.X = BSMAX(MaximumCoords.X, tmpCoords_2.x);
				MaximumCoords.Y = BSMAX(MaximumCoords.Y, tmpCoords_2.y);
				if (apFile->GetMaster()) {
					TESWorldSpace::OFFSET_DATA* pdata_2 = CreateOffsetData(apFile);
					pdata_2->OffsetMaxCoords = tmpCoords_2;
				}
			}
		}
	} while (apFile->NextChunk());
	return 1;
}

void TESWorldSpace::Save() {
	StartForm();
	TESFullName::Save();
	uint32_t tempint = 0;
	if (pEncounterZone) {
		tempint = pEncounterZone->iFormID;
		TESForm::AddChunk(XEZN_ID, tempint);
	}
	if (pParentWorld) {
		tempint = pParentWorld->iFormID;
		TESForm::AddChunk(WNAM_ID, tempint);
		TESForm::AddChunk(PNAM_ID, sParentUseFlags);
	}
	if (!GetParentUseFlag(TESWorldSpace::PU_CLIMATE)) {
		if (GetClimate()) {
			TESClimate* Climate = GetClimate();
			tempint = Climate->iFormID;
			TESForm::AddChunk(CNAM_ID, tempint);
		}
	}
	if (!GetParentUseFlag(TESWorldSpace::PU_WATER)) {
		if (GetWaterType()) {
			TESWaterForm* WaterType = GetWaterType();
			tempint = WaterType->iFormID;
			TESForm::AddChunk(NAM2_ID, tempint);
		}
	}
	if (!GetParentUseFlag(TESWorldSpace::PU_LOD)) {
		if (GetLODWaterType()) {
			TESWaterForm* LODWaterType = GetLODWaterType();
			tempint = LODWaterType->iFormID;
			TESForm::AddChunk(NAM3_ID, tempint);
			float LeafDimming = fLODWaterHeight;
			TESForm::AddChunk(NAM4_ID, std::bit_cast<uint32_t>(LeafDimming));
		}
	}
	if (!GetParentUseFlag(TESWorldSpace::PU_LAND)) {
		DEFAULT_CELL_DATA defdata;
		defdata.fLandHeight = fDefaultLandHeight;
		defdata.fWaterHeight = fDefaultWaterHeight;
		if (TESForm::QEndianSwapOnSave()) {
			defdata.Endian();
		}
		TESForm::__AddChunkData(DNAM_ID, &defdata, 8u);
		if (TESForm::QEndianSwapOnSave()) {
			defdata.Endian();
		}
	}
	if (!GetParentUseFlag(TESWorldSpace::PU_MAP)) {
		TESTexture::Save(ICON_ID);
		if (TESForm::QEndianSwapOnSave()) {
			WorldMapData.Endian();
		}
		TESForm::__AddChunkData(MNAM_ID, &WorldMapData, 16u);
		if (TESForm::QEndianSwapOnSave()) {
			WorldMapData.Endian();
		}
	}
	if (TESForm::QEndianSwapOnSave()) {
		WorldMapOffsetData.Endian();
	}
	TESForm::__AddChunkData(ONAM_ID, &WorldMapOffsetData, 12u);
	if (TESForm::QEndianSwapOnSave()) {
		WorldMapOffsetData.Endian();
	}
	if (!GetParentUseFlag(TESWorldSpace::PU_UNUSED)) {
		if (pImageSpace) {
			tempint = pImageSpace->iFormID;
			TESForm::AddChunk(INAM_ID, tempint);
		}
	}
	TESForm::AddChunk(DATA_ID, cFlags);
	TESForm::AddChunkArray(NAM0_ID, &MinimumCoords.X, 2u);
	TESForm::AddChunkArray(NAM9_ID, &MaximumCoords.X, 2u);
	if (GetMusicType()) {
		tempint = GetMusicType()->iFormID;
		TESForm::AddChunk(ZNAM_ID, tempint);
	}
	TESForm::AddChunkArray(NNAM_ID, CanopyShadowTexture.GetTextureName(),
		CanopyShadowTexture.GetTextureNameLength() + 1);
	TESForm::AddChunkArray(XNAM_ID, WaterNoiseTexture.GetTextureName(), WaterNoiseTexture.GetTextureNameLength() + 1);
	if (pImpactSwap) {
		pImpactSwap->Save();
	}
	CloseForm();
}

bool TESWorldSpace::SavesBefore(TESForm* apForm) {
	bool bretval = 0;
	switch (apForm->GetFormType()) {
	case CELL_ID: {
		TESWorldSpace* pcellworld;
		TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(apForm);
		if (pcell) {
			pcellworld = pcell->GetWorldSpace();
		} else {
			pcellworld = 0;
		}
		if (pcellworld == this) {
			bretval = 1;
		} else {
			if (pcellworld) {
				bretval = SavesBefore(pcellworld);
			}
		}
		break;
	}
	case REFR_ID:
	case ACHR_ID:
	case ACRE_ID:
	case PMIS_ID:
	case PGRE_ID:
	case PBEA_ID:
	case PFLA_ID:
	case LAND_ID:
	case NAVM_ID:
	case PCBE_ID: {
		TESChildCell* pcellchild = dynamic_cast<TESChildCell*>(apForm);
		TESObjectCELL* pcellworld_2 = pcellchild->GetSaveParentCell();
		bretval = SavesBefore(pcellworld_2);
		break;
	}
	case WRLD_ID: {
		uint32_t CastingType = iFormID;
		bretval = CastingType < apForm->iFormID;
		break;
	}
	default: {
		bretval = TESForm::SavesBefore(apForm);
		break;
	}
	}
	return bretval;
}

bool TESWorldSpace::SavesBefore(FORM* apGroupFORM) {
	bool bretval = 0;
	if (apGroupFORM) {
		if (apGroupFORM->form == formEnumString[2].uiFormString) {
			uint32_t form = apGroupFORM->form;
			switch (apGroupFORM->iFormID) {
			case 0u: {
				bretval = TESForm::SavesBefore(apGroupFORM);
				break;
			}
			case 1u: {
				TESForm* FormByNumericID = TESForm::GetFormByNumericID(apGroupFORM->flags);
				TESWorldSpace* pworld = dynamic_cast<TESWorldSpace*>(FormByNumericID);
				if (pworld) {
					bretval = SavesBefore(pworld);
				}
				break;
			}
			case 2u:
			case 3u: {
				bretval = 0;
				break;
			}
			case 6u: {
				TESForm* v3 = TESForm::GetFormByNumericID(apGroupFORM->flags);
				TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(v3);
				if (pcell) {
					bretval = SavesBefore(pcell);
				}
				break;
			}
			case 7u: {
				bretval = 1;
				break;
			}
			default: {
				return bretval;
			}
			}
		}
	}
	return bretval;
}

TESForm* TESWorldSpace::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
	TESForm* DuplicateForm = TESForm::CreateDuplicateForm(abCreateEditorID, apCopyMap);
	TESWorldSpace* pdupworld = dynamic_cast<TESWorldSpace*>(DuplicateForm);
	pdupworld->pCellMap->RemoveAll();
	NiTMapItem<int32_t, TESObjectCELL*>* it = pCellMap->GetFirstPos();
	while (it) {
		int32_t key;
		TESObjectCELL* pcell = 0;
		pCellMap->GetNext(it, key, pcell);
		if (pcell) {
			TESForm* v4 = pcell->CreateDuplicateForm(0, apCopyMap);
			TESObjectCELL* pdupcell = dynamic_cast<TESObjectCELL*>(v4);
			if (pdupcell) {
				pdupcell->SetAltered(1);
				pdupworld->AddCell(pdupcell);
			}
		}
	}
	TESObjectCELL* pcell_2 = pPersistentCell;
	if (pcell_2) {
		TESForm* v5 = pcell_2->CreateDuplicateForm(0, apCopyMap);
		TESObjectCELL* pdupcell_2 = dynamic_cast<TESObjectCELL*>(v5);
		if (pdupcell_2) {
			pdupworld->pPersistentCell = pdupcell_2;
			pdupcell_2->SetAltered(1);
		}
	}
	pdupworld->AddPersistentReferences();
	return pdupworld;
}

bool TESWorldSpace::FindInFileFast(TESFile* apFile) {
	OFFSET_DATA* pOffsetData = GetOffsetData(apFile);
	if (!apFile || !apFile->GetMaster() || !pOffsetData || !pOffsetData->iFileOffset ||
		!apFile->SetOffset(pOffsetData->iFileOffset))
		return false;
	if (apFile->GetTESForm() != formEnumString[WRLD_ID].cFormID)
		return false;
	const uint32_t uiCurrentFormID = apFile->GetID();
	return uiCurrentFormID == GetFormID();
}

// Each form reference holds a form ID until here (the pointer field is read as a raw ID, hence the casts). It is
// resolved against the worldspace's file and checked by _RTDynamicCast; a missing form is warned about by editor ID
// (when it has one) or form ID, and the setter still takes the null result.
void TESWorldSpace::InitItem() {
	if (!GetInitialized()) {
		uint32_t iFormID = reinterpret_cast<uintptr_t>(pClimate);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			TESClimate* pForm = dynamic_cast<TESClimate*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("MASTERFILE: Unable to find climate (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("MASTERFILE: Unable to find climate (%08X) on owner worldspace (%08X).", iFormID,
						GetFormID());
				}
			}
			SetClimate(pForm);
		}
		iFormID = reinterpret_cast<uintptr_t>(pImageSpace);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			TESImageSpace* pForm = dynamic_cast<TESImageSpace*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("MASTERFILE: Unable to find imagespace (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("MASTERFILE: Unable to find imagespace (%08X) on owner worldspace (%08X).", iFormID,
						GetFormID());
				}
			}
			SetImageSpace(pForm);
		}
		iFormID = reinterpret_cast<uintptr_t>(pEncounterZone);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			BGSEncounterZone* pForm = dynamic_cast<BGSEncounterZone*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("Unable to find encounter zone (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("Unable to find encounter zone (%08X) on owner worldspace (%08X).", iFormID, GetFormID());
				}
			}
			SetEncounterZone(pForm);
		}
		iFormID = reinterpret_cast<uintptr_t>(pWorldWater);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			TESWaterForm* pForm = dynamic_cast<TESWaterForm*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("MASTERFILE: Unable to find water type (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("MASTERFILE: Unable to find water type (%08X) on owner worldspace (%08X).", iFormID,
						GetFormID());
				}
			}
			SetWorldWater(pForm);
		}
		iFormID = reinterpret_cast<uintptr_t>(pLODWater);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			TESWaterForm* pForm = dynamic_cast<TESWaterForm*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("MASTERFILE: Unable to find lod water type (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("MASTERFILE: Unable to find lod water type (%08X) on owner worldspace (%08X).", iFormID,
						GetFormID());
				}
			}
			SetLODWater(pForm);
		}
		iFormID = reinterpret_cast<uintptr_t>(pMusicType);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			BGSMusicType* pForm = dynamic_cast<BGSMusicType*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("MASTERFILE: Unable to find music type (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("MASTERFILE: Unable to find music type (%08X) on owner worldspace (%08X).", iFormID,
						GetFormID());
				}
			}
			SetMusicType(pForm);
		}
		iFormID = reinterpret_cast<uintptr_t>(pParentWorld);
		if (iFormID) {
			TESForm::AddCompileIndex(iFormID, GetFile(-1));
			TESWorldSpace* pForm = dynamic_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(iFormID));
			if (!pForm) {
				if (GetFormEditorIDLength()) {
					Warning("MASTERFILE: Unable to find landscape world (%08X) on owner worldspace \"%s\".", iFormID,
						GetFormEditorID());
				} else {
					Warning("MASTERFILE: Unable to find landscape world (%08X) on owner worldspace (%08X).", iFormID,
						GetFormID());
				}
			}
			SetParentWorld(pForm);
		}
		if (pImpactSwap) {
			pImpactSwap->InitItem(this);
		}
		SetInitialized(true);
	}
	NiTMapItem<int32_t, TESObjectCELL*>* pPos = pCellMap->GetFirstPos();
	while (pPos) {
		int32_t iKey;
		TESObjectCELL* pCell = nullptr;
		pCellMap->GetNext(pPos, iKey, pCell);
		if (pCell) {
			pCell->InitItem();
		}
	}
	TESObjectCELL* pCell = QPersistentCell();
	if (pCell) {
		pCell->InitItem();
	}
	void* pMemory = ourMalloc(sizeof(BGSTerrainManager));
	pTerrainManager = pMemory ? ::new (pMemory) BGSTerrainManager(this) : nullptr;
}

void TESWorldSpace::Copy(TESForm* apCopy) {
	TESWorldSpace* pCopy = dynamic_cast<TESWorldSpace*>(apCopy);
	if (pCopy) {
		CopyAllComponents(pCopy);
		cFlags = pCopy->cFlags;
		sParentUseFlags = pCopy->sParentUseFlags;
		pParentWorld = pCopy->pParentWorld;
		pEncounterZone = pCopy->pEncounterZone;
		pClimate = pCopy->GetClimate();
		pImageSpace = pCopy->pImageSpace;
		if (pCopy->pImpactSwap) {
			if (!pImpactSwap) {
				ImpactSwap* v3;
				ImpactSwap* apMemory = static_cast<ImpactSwap*>(ourMalloc(sizeof(ImpactSwap)));
				if (apMemory) {
					v3 = ::new (apMemory) ImpactSwap;
				} else {
					v3 = 0;
				}
				pImpactSwap = v3;
			}
			pImpactSwap->Copy(pCopy->pImpactSwap);
		} else {
			if (pImpactSwap) {
				ImpactSwap* apThis = pImpactSwap;
				if (apThis) {
					delete apThis;
				}
				pImpactSwap = 0;
			}
		}
		pWorldWater = pCopy->GetWaterType();
		pLODWater = pCopy->GetLODWaterType();
		float a2 = pCopy->fLODWaterHeight;
		SetWaterLODHeight(a2);
		fDefaultLandHeight = pCopy->fDefaultLandHeight;
		fDefaultWaterHeight = pCopy->fDefaultWaterHeight;
		ourMemcpy(&WorldMapData, &pCopy->WorldMapData, sizeof(WorldMapData));
		ourMemcpy(&WorldMapOffsetData, &pCopy->WorldMapOffsetData, sizeof(WorldMapOffsetData));
		pMusicType = pCopy->GetMusicType();
		pCellMap->RemoveAll();
		NiTMapItem<int32_t, TESObjectCELL*>* it = pCopy->pCellMap->GetFirstPos();
		while (it) {
			int32_t key;
			TESObjectCELL* pCell = 0;
			pCopy->pCellMap->GetNext(it, key, pCell);
			pCellMap->SetAt(key, pCell);
		}
		CanopyShadowTexture.CopyComponent(&pCopy->CanopyShadowTexture);
		WaterNoiseTexture.CopyComponent(&pCopy->WaterNoiseTexture);
	}
}

bool TESWorldSpace::Compare(TESForm* apCompare) {
	TESWorldSpace* pComp = dynamic_cast<TESWorldSpace*>(apCompare);
	if (!pComp) {
		return 1;
	}
	if (CompareAllComponents(pComp)) {
		return 1;
	}
	if (cFlags != pComp->cFlags) {
		return 1;
	}
	if (sParentUseFlags != pComp->sParentUseFlags) {
		return 1;
	}
	if (pParentWorld != pComp->pParentWorld) {
		return 1;
	}
	if (pEncounterZone != pComp->pEncounterZone) {
		return 1;
	}
	TESClimate* Climate = GetClimate();
	if (Climate != pComp->GetClimate()) {
		return 1;
	}
	TESImageSpace* State = pImageSpace;
	if (State != pComp->pImageSpace) {
		return 1;
	}
	if (pImpactSwap && pComp->pImpactSwap) {
		if (pImpactSwap->Compare(pComp->pImpactSwap)) {
			return 1;
		}
	} else {
		if (pImpactSwap || pComp->pImpactSwap) {
			return 1;
		}
	}
	TESWaterForm* WaterType = GetWaterType();
	if (WaterType != pComp->GetWaterType()) {
		return 1;
	}
	TESWaterForm* LODWaterType = GetLODWaterType();
	if (LODWaterType != pComp->GetLODWaterType()) {
		return 1;
	}
	double LeafDimming = fLODWaterHeight;
	if (LeafDimming != pComp->fLODWaterHeight) {
		return 1;
	}
	if (pComp->fDefaultLandHeight != fDefaultLandHeight || pComp->fDefaultWaterHeight != fDefaultWaterHeight) {
		return 1;
	}
	if (std::memcmp(&WorldMapData, &pComp->WorldMapData, sizeof(WorldMapData))) {
		return 1;
	}
	if (std::memcmp(&WorldMapOffsetData, &pComp->WorldMapOffsetData, sizeof(WorldMapOffsetData))) {
		return 1;
	}
	BGSMusicType* MusicType = GetMusicType();
	return MusicType != pComp->GetMusicType() ||
		(CanopyShadowTexture.CompareComponent(&pComp->CanopyShadowTexture) ||
			WaterNoiseTexture.CompareComponent(&pComp->WaterNoiseTexture));
}

bool TESWorldSpace::IsFormTypeChild(uint8_t aeType) {
	return TESWorldSpace::IsFormTypeWorldChild(aeType);
}

const char* TESWorldSpace::GetFormEditorID() {
	if (EditorID.pString != nullptr) {
		return EditorID.pString;
	}
	return "";
}

bool TESWorldSpace::SetFormEditorID(const char* apEditorID) {
	EditorID.Set(apEditorID, 0);
	return true;
}

bool TESWorldSpace::GetMapNameForLocation(BSStringT<char>& aMapName, NiPoint3 aWorldLoc) {
	static BSStringT<char> lastMapName;
	// lastWorldLoc's constructor is empty.
	TESRegionDataMap* pdata = 0;
	if (!pDataHandler) {
		return 0;
	}
	if (plastWorldSpace == this && lastWorldLoc == aWorldLoc) {
		aMapName.Set(lastMapName.pString, 0);
		return 0;
	}
	if (plastWorldSpace != this) {
		lastMapName.Set("", 0);
	}
	plastWorldSpace = this;
	lastWorldLoc = aWorldLoc;
	aMapName.Set("", 0);
	TESObjectCELL* pcell = pDataHandler->GetCellFromWorldCoord(aWorldLoc.x, aWorldLoc.y, this, 0);
	if (pcell) {
		if (pcell->GetRegionList(1)) {
			TESRegionDataManager* pmanager = pDataHandler->GetRegionDataManager();
			TESRegionData* pderived = pcell->GetRegionList(1)->GetDerivedData(REGION_DATA_MAP_ID, aWorldLoc, this);
			pdata = pmanager->IsRegionDataMap(pderived);
		} else {
			pdata = 0;
		}
		if (pdata) {
			pdata->GetMapName(&aMapName);
		} else {
			aMapName.Set(pcell->GetFullName(), 0);
		}
	} else {
		// No cell: the map name of the region at the position with the highest priority, overriding regions first.
		TESRegionList* pregions = pDataHandler->GetRegionList();
		BSSimpleList<TESRegion*>* pcurrent = pregions ? static_cast<BSSimpleList<TESRegion*>*>(pregions) : 0;
		BSSimpleList<TESRegionPointList*>* pcurrentRPL = 0;
		TESRegionPoint regpt(aWorldLoc);
		int32_t ipriority = -1;
		bool bfound = 0;
		bool boverridden = 0;
		while (pcurrent && pcurrent->m_item) {
			bfound = 0;
			TESRegion* pregion = pcurrent->m_item;
			if (!pregion->GetDelete() && pregion->pWorldSpace == this && pregion->pPointLists &&
				!pregion->pPointLists->IsEmpty()) {
				TESRegionDataManager* pmanager = pDataHandler->GetRegionDataManager();
				pdata = pmanager->IsRegionDataMap(pregion->pDataList->Find(REGION_DATA_MAP_ID));
				if (pdata) {
					for (pcurrentRPL = pregion->pPointLists; pcurrentRPL && pcurrentRPL->m_item;
						pcurrentRPL = pcurrentRPL->m_pkNext) {
						if (pcurrentRPL->m_item->IsInsideRegion(&regpt)) {
							bfound = 1;
							break;
						}
					}
					if (bfound &&
						(pdata->bOverride ? !boverridden || pdata->cPriority > ipriority
										  : !boverridden && pdata->cPriority > ipriority)) {
						boverridden = pdata->bOverride;
						ipriority = pdata->cPriority;
						pdata->GetMapName(&aMapName);
					}
				}
			}
			pcurrent = pcurrent->m_pkNext;
		}
	}
	if (!aMapName.GetLength()) {
		aMapName.Set(GetFullName(), 0);
	}
	if (!aMapName.GetLength()) {
		aMapName.Set(TESObjectCELL::GetDefaultCellName(), 0);
	}
	if (!aMapName.StrCmp(lastMapName.pString, 1)) {
		return 0;
	}
	lastMapName.Set(aMapName.pString, 0);
	return 1;
}

void TESWorldSpace::GetGrassForLocation(NiPoint2 aMinWorldLoc, NiPoint2 aMaxWorldLoc, TESGrassAreaParam* apGrassParams,
	uint32_t aiNumGrassParams) {
	TESRegionGrassObject::GenGrass* pgengrass = 0;
	TESRegionGrassObject** pgrassArray = 0;
	TESRegionGrassObject* pgrassObj = 0;
	TESGrass* pgrass = 0;
	NiPoint3 plocs[9];
	NiPoint3 center;
	rt::eh_vector_constructor_iterator(plocs, sizeof(NiPoint3), 9, &emptyPointConstructor);
	if (!pDataHandler || !aiNumGrassParams) {
		return;
	}
	center.x = (aMinWorldLoc.x + aMaxWorldLoc.x) / 2.0;
	center.y = (aMinWorldLoc.y + aMaxWorldLoc.y) / 2.0;
	center.z = 0.0f;
	float fxdelta = (aMaxWorldLoc.x - aMinWorldLoc.x) / 2.0;
	float fydelta = (aMaxWorldLoc.y - aMinWorldLoc.y) / 2.0;
	TESObjectCELL* pcell = pDataHandler->GetCellFromWorldCoord(center.x, center.y, this, 0);
	if (!pcell || !pcell->GetRegionList(1)) {
		return;
	}
	uint32_t itest;
	for (itest = 0; itest < 9; ++itest) {
		plocs[itest].x = static_cast<double>(itest % 3) * fxdelta + aMinWorldLoc.x;
		plocs[itest].y = static_cast<double>(itest / 3) * fydelta + aMinWorldLoc.y;
		plocs[itest].z = 0.0f;
	}
	TESRegionDataManager* pmanager = pDataHandler->GetRegionDataManager();
	TESRegionData* pderived = pcell->GetRegionList(1)->GetDerivedData(REGION_DATA_GRASS_ID, center, this);
	TESRegionDataGrass* pdata = pmanager->IsRegionDataGrass(pderived);
	if (!pdata) {
		return;
	}
	BSSimpleList<TESRegionGrassObject::GenGrass*>* pgenlist;
	void* plistMemory = ourMalloc(sizeof(BSSimpleList<TESRegionGrassObject::GenGrass*>));
	if (plistMemory) {
		pgenlist = ::new (plistMemory) BSSimpleList<TESRegionGrassObject::GenGrass*>;
	} else {
		pgenlist = 0;
	}
	// The objects on a land texture first, then the others.
	TESRegionGrassObjectList* pobjects = pdata->GetObjectList();
	BSSimpleList<TESRegionGrassObject*>* pcurrentObj;
	for (pcurrentObj = pobjects ? static_cast<BSSimpleList<TESRegionGrassObject*>*>(pobjects) : 0; pcurrentObj;
		pcurrentObj = pcurrentObj->m_pkNext) {
		pgrassObj = pcurrentObj->m_item;
		pgrass = pgrassObj ? static_cast<TESGrass*>(pgrassObj->GetObject()) : 0;
		if (pgrassObj && pgrassObj->GetParentTexture() && pgrass && pgrass->GetDensity() &&
			pgrassObj->GetGenerateChance(center, this, 0) != 0.0) {
			pgengrass = static_cast<TESRegionGrassObject::GenGrass*>(ourMalloc(sizeof(TESRegionGrassObject::GenGrass)));
			pgengrass->pForm = pgrassObj;
			pgengrass->fdensity = pgrass->GetDensity() / 100.0;
			pgenlist->AddHead(pgengrass);
		}
	}
	pobjects = pdata->GetObjectList();
	for (pcurrentObj = pobjects ? static_cast<BSSimpleList<TESRegionGrassObject*>*>(pobjects) : 0; pcurrentObj;
		pcurrentObj = pcurrentObj->m_pkNext) {
		pgrassObj = pcurrentObj->m_item;
		pgrass = pgrassObj ? static_cast<TESGrass*>(pgrassObj->GetObject()) : 0;
		if (pgrassObj && !pgrassObj->GetParentTexture() && pgrass && pgrass->GetDensity() &&
			pgrassObj->GetGenerateChance(center, this, 0) != 0.0) {
			pgengrass = static_cast<TESRegionGrassObject::GenGrass*>(ourMalloc(sizeof(TESRegionGrassObject::GenGrass)));
			pgengrass->pForm = pgrassObj;
			pgengrass->fdensity = pgrass->GetDensity() / 100.0;
			pgenlist->AddHead(pgengrass);
		}
	}
	if (!pgenlist || pgenlist->IsEmpty()) {
		if (pgenlist) {
			delete pgenlist;
		}
		return;
	}
	// A count whose byte size overflows asks for UINT32_MAX bytes, as the game's new[] does.
	pgrassArray = static_cast<TESRegionGrassObject**>(ourMalloc(static_cast<uint64_t>(aiNumGrassParams) *
					sizeof(TESRegionGrassObject*) >
				UINT32_MAX
			? UINT32_MAX
			: static_cast<uint32_t>(aiNumGrassParams * sizeof(TESRegionGrassObject*))));
	ourMemset(pgrassArray, 0, static_cast<uint32_t>(aiNumGrassParams * sizeof(TESRegionGrassObject*)));
	BSSimpleList<TESRegionGrassObject::GenGrass*>* pcurrentGen = pgenlist;
	uint32_t iparam;
	for (iparam = 0; iparam < aiNumGrassParams; ++iparam) {
		TallGrassShaderProperty::GrassParam& rparam = apGrassParams[iparam].kGrassParam;
		ourMemset(apGrassParams[iparam].pfDensity, 0, sizeof(apGrassParams[iparam].pfDensity));
		// The parameters own their mesh name.
		if (rparam.pcGeomFilename) {
			ourFree(const_cast<char*>(rparam.pcGeomFilename));
		}
		rparam.pcGeomFilename = 0;
		if (!pcurrentGen) {
			continue;
		}
		pgengrass = pcurrentGen->m_item;
		while (!pgengrass) {
			pcurrentGen = pcurrentGen->m_pkNext;
			if (!pcurrentGen) {
				break;
			}
			pgengrass = pcurrentGen->m_item;
		}
		if (!pgengrass) {
			continue;
		}
		pgrassObj = pgengrass->pForm;
		pgrass = static_cast<TESGrass*>(pgrassObj->GetObject());
		pgrassArray[iparam] = pgrassObj;
		if (rparam.pcGeomFilename) {
			ourFree(const_cast<char*>(rparam.pcGeomFilename));
		}
		char* pFilename = static_cast<char*>(ourMalloc(260u));
		BSsprintf(pFilename, 260, "data/meshes/%s", pgrass->GetModelPath());
		rparam.pcGeomFilename = pFilename;
		rparam.iGrassID = pgrass->iFormID;
		rparam.fPositionRange = pgrass->GetPositionRange();
		rparam.fHeightRange = pgrass->GetHeightRange();
		rparam.fColorRange = pgrass->GetColorRange();
		rparam.fWavePeriod = pgrass->GetWavePeriod();
		rparam.bVertexLighting = pgrass->GetVertexLighting();
		rparam.bUniformScaing = pgrass->GetUniformScaling();
		rparam.bFitToSlope = pgrass->GetFitToSlope();
		rparam.fWaveOffsetRange = TESGrass::GetWaveOffsetRange();
		pcurrentGen = pcurrentGen->m_pkNext;
	}
	for (itest = 0; itest < 9; ++itest) {
		for (iparam = 0; iparam < aiNumGrassParams; ++iparam) {
			if (pgrassArray[iparam] && apGrassParams[iparam].kGrassParam.iGrassID) {
				apGrassParams[iparam].pfDensity[itest] = pgrassArray[iparam]->GetGenerateChance(plocs[itest], this, 1);
			}
		}
	}
	ourFree(pgrassArray);
	BSSimpleList<TESRegionGrassObject::GenGrass*>* pcurrent = pgenlist;
	while (pcurrent && pcurrent->m_item) {
		TESRegionGrassObject::GenGrass* pdelete = pcurrent->m_item;
		pcurrent = pcurrent->m_pkNext;
		ourFree(pdelete);
	}
	pgenlist->RemoveAll();
	if (pgenlist) {
		delete pgenlist;
	}
}

TESObjectCELL* TESWorldSpace::GetCellFromKey(uint32_t aiKey) {
	TESObjectCELL* pcell = 0;
	// aiKey is a uint32_t, but the map's key is an int.
	if (pCellMap->GetAt(static_cast<int32_t>(aiKey), pcell)) {
		return pcell;
	} else {
		return 0;
	}
}

// The parent is looked up twice, as the game does. The function calls itself for the parent; disable_tail_calls keeps
// clang from turning that recursion into a loop.
[[clang::disable_tail_calls]] WorldSpaceMapData* TESWorldSpace::GetWorldMapData() {
	if (GetParentWorld(PU_MAP))
		return GetParentWorld(PU_MAP)->GetWorldMapData();
	return &WorldMapData;
}

const char* TESWorldSpace::GetWorldMapTextureName() {
	if (GetParentWorld(PU_MAP))
		return GetParentWorld(PU_MAP)->GetTextureName();
	return GetTextureName();
}

void TESWorldSpace::AdjustMapMarkerCoord(NiPoint3& aMapCoord, bool abInverse) {
	float v3;
	double fadeDuration;
	NiPoint3 v6;
	NiPoint3 result;
	NiPoint3 ecx0a;
	if (abInverse) {
		aMapCoord.x = (float)(aMapCoord.x - WorldMapOffsetData.fCellOffsetX);
		aMapCoord.y = (float)(aMapCoord.y - WorldMapOffsetData.fCellOffsetY);
	}
	if ((WorldMapOffsetData.fMapScale != 1.0) && (WorldMapOffsetData.fMapScale != 0.0)) {
		ecx0a;
		ecx0a.x = (float)(MinimumCoords.X + MaximumCoords.X);
		ecx0a.y = (float)(MinimumCoords.Y + MaximumCoords.Y);
		ecx0a.z = (float)0.0;
		(&ecx0a)->operator*=(0x1p-1f);
		(result = aMapCoord.operator-(ecx0a));
		if (abInverse) {
			fadeDuration = 1.0 / WorldMapOffsetData.fMapScale;
		} else {
			fadeDuration = (double)WorldMapOffsetData.fMapScale;
		}
		v3 = (float)fadeDuration;
		(&result)->operator*=(v3);
		aMapCoord = *&(v6 = (&ecx0a)->operator+(result));
	}
	if (!abInverse) {
		aMapCoord.x = (float)(WorldMapOffsetData.fCellOffsetX + aMapCoord.x);
		aMapCoord.y = (float)(WorldMapOffsetData.fCellOffsetY + aMapCoord.y);
	}
}

bool TESWorldSpace::GetSmallWorld() const {
	return (cFlags & kWorldFlag_SmallWorld) != 0;
}

void TESWorldSpace::SetSmallWorld(bool abVal) {
	if (abVal) {
		cFlags |= kWorldFlag_SmallWorld;
	} else {
		cFlags &= static_cast<uint8_t>(~kWorldFlag_SmallWorld);
	}
}

// Each field goes through its setter; the map name is TESTexture's, and the map data is zeroed with the engine's
// ourMemset.
void TESWorldSpace::ClearSharableData() {
	SetClimate(nullptr);
	SetWorldWater(nullptr);
	SetLODWater(nullptr);
	SetWaterLODHeight(0.0f);
	SetDefaultLandHeight(-2048.0f);
	SetDefaultWaterHeight(0.0f);
	TESTexture::SetTextureName(nullptr);
	ourMemset(&WorldMapData, 0, sizeof(WorldMapData));
}

void TESWorldSpace::SetClimate(TESClimate* apClimate) {
	pClimate = apClimate;
}

void TESWorldSpace::SetImageSpace(TESImageSpace* apImageSpace) {
	pImageSpace = apImageSpace;
}

void TESWorldSpace::SetEncounterZone(BGSEncounterZone* apZone) {
	pEncounterZone = apZone;
}

// The topmost parent file keys the map. The new block is used without a null check, as the game does.
TESWorldSpace::OFFSET_DATA* TESWorldSpace::CreateOffsetData(TESFile* apFile) {
	OFFSET_DATA* pData = nullptr;
	TESFile* pFile = apFile;
	while (pFile && pFile->GetThreadSafeParent()) {
		pFile = pFile->GetThreadSafeParent();
	}
	if (OffsetDataMap.GetAt(pFile, pData) && pData) {
		return pData;
	}
	void* pMemory = ourMalloc(sizeof(OFFSET_DATA));
	pData = pMemory ? ::new (pMemory) OFFSET_DATA : nullptr;
	pData->pCellFileOffsets = nullptr;
	pData->iFileOffset = 0;
	pData->OffsetMinCoords = NiPoint2(3.4028235e+38f, 3.4028235e+38f);
	pData->OffsetMaxCoords = NiPoint2(-3.4028235e+38f, -3.4028235e+38f);
	OffsetDataMap.SetAt(pFile, pData);
	return pData;
}

void TESWorldSpace::SetDefaultWaterHeight(float afVal) {
	fDefaultWaterHeight = afVal;
}

bool TESWorldSpace::GetNoLODNoise() const {
	return (cFlags & kWorldFlag_NoLODNoise) != 0;
}

TESWorldSpace* TESWorldSpace::GetParentWorld(PARENT_USE_FLAG aeFlags) {
	if (pParentWorld == nullptr || GetParentUseFlag(aeFlags)) {
		return pParentWorld;
	}
	return nullptr;
}

bool TESWorldSpace::CantTravelFrom() {
	return ((cFlags & 2) != 0);
}

bool TESWorldSpace::GetCantWaitHere() {
	return ((((int32_t)TESForm::GetFormFlags()) & 524288) != 0);
}

// The offset data is kept for the topmost parent file.
TESWorldSpace::OFFSET_DATA* TESWorldSpace::GetOffsetData(TESFile* apFile) {
	OFFSET_DATA* pOffsetData = nullptr;
	TESFile* pFile = apFile;
	while (pFile && pFile->GetThreadSafeParent())
		pFile = pFile->GetThreadSafeParent();
	if (!OffsetDataMap.GetAt(pFile, pOffsetData))
		pOffsetData = nullptr;
	return pOffsetData;
}

TESWaterForm* TESWorldSpace::GetWaterType() {
	if (GetParentWorld(TESWorldSpace::PU_WATER)) {
		TESWorldSpace* ParentWorld = GetParentWorld(TESWorldSpace::PU_WATER);
		return ParentWorld->GetWaterType();
	} else {
		if (pWorldWater) {
			return pWorldWater;
		} else {
			return pDefaultWater;
		}
	}
}

TESWaterForm* TESWorldSpace::GetLODWaterType() {
	if (GetParentWorld(TESWorldSpace::PU_LOD)) {
		TESWorldSpace* pParentWorld = GetParentWorld(TESWorldSpace::PU_LOD);
		return pParentWorld->GetLODWaterType();
	} else {
		if (pLODWater) {
			return pLODWater;
		} else {
			return pDefaultWater;
		}
	}
}

TESWorldSpace* TESWorldSpace::GetParentWorldSimple() {
	return pParentWorld;
}

void TESWorldSpace::SetParentWorld(TESWorldSpace* apWorld) {
	pParentWorld = apWorld;
	if (apWorld == nullptr) {
		sParentUseFlags = 0;
	}
}

bool TESWorldSpace::GetParentUseFlag(PARENT_USE_FLAG aeFlag) {
	return ((((uint32_t)aeFlag) > 5) || ((((uint8_t)(1 << ((int32_t)aeFlag))) & sParentUseFlags) != 0));
}

// The terrain is shared with the parent worldspace when it takes its level of detail from it. The parent is looked up
// twice, as the game does; disable_tail_calls keeps the recursive call a real call.
[[clang::disable_tail_calls]] BGSTerrainManager* TESWorldSpace::GetTerrainManager() {
	if (GetParentWorld(PU_LOD) != nullptr) {
		return GetParentWorld(PU_LOD)->GetTerrainManager();
	}
	return pTerrainManager;
}

void TESWorldSpace::AddMultiBoundRef(TESObjectREFR* apRef) {
	if (apRef) {
		if (apRef->IsMultiBoundRef()) {
			NiPoint3 ext = apRef->GetMultiBoundHalfExtent();
			float frad = ext.Length();
			const NiPoint3* v2 = &apRef->GetLocationOnReference();
			float someFloat = v2->x - frad;
			int32_t iminx = FloatToInt(someFloat) >> 12;
			const NiPoint3* v3 = &apRef->GetLocationOnReference();
			float v12 = v3->y - frad;
			int32_t iminy = FloatToInt(v12) >> 12;
			const NiPoint3* v4 = &apRef->GetLocationOnReference();
			float v11 = v4->x + frad;
			int32_t imaxx = FloatToInt(v11) >> 12;
			const NiPoint3* v5 = &apRef->GetLocationOnReference();
			float v10 = v5->y + frad;
			int32_t imaxy = FloatToInt(v10) >> 12;
			const NiPoint3* v6 = &apRef->GetLocationOnReference();
			int32_t iparentx = FloatToInt(v6->x) >> 12;
			const NiPoint3* v7 = &apRef->GetLocationOnReference();
			int32_t iparenty = FloatToInt(v7->y) >> 12;
			for (int32_t ix = iminx; ix <= imaxx; ++ix) {
				for (int32_t iy = iminy; iy <= imaxy; ++iy) {
					if ((ix != iparentx || iy != iparenty) &&
						MultiBoundMarkerData::MultiBoundIntersectsCell(apRef, ix, iy)) {
						if (!pOverlappedMultiboundMap) {
							NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>* v9;
							NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>* apMemory = static_cast<
								NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>*>(ourMalloc(sizeof(NiTPointerMap<
								uint32_t, BSSimpleList<TESObjectREFR*>*>)));
							if (apMemory) {
								v9 = ::new (apMemory) NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>(37u);
							} else {
								v9 = 0;
							}
							pOverlappedMultiboundMap = v9;
						}
						uint32_t ikey = TESObjectCELL::GetCoord(static_cast<uint16_t>(ix), static_cast<uint16_t>(iy));
						BSSimpleList<TESObjectREFR*>* preflist = 0;
						pOverlappedMultiboundMap->GetAt(ikey, preflist);
						if (!preflist) {
							BSSimpleList<TESObjectREFR*>* preflist_2;
							BSSimpleList<TESObjectREFR*>* v15 = static_cast<
								BSSimpleList<TESObjectREFR*>*>(ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>)));
							if (v15) {
								preflist_2 = ::new (v15) BSSimpleList<TESObjectREFR*>;
							} else {
								preflist_2 = 0;
							}
							preflist = preflist_2;
							pOverlappedMultiboundMap->SetAt(ikey, preflist_2);
						}
						preflist->AddHead(apRef);
					}
				}
			}
		}
	}
}

TESObjectCELL* TESWorldSpace::GetPersistentCell() {
	if (!pPersistentCell) {
		void* pMemory = ourMalloc(sizeof(TESObjectCELL));
		pPersistentCell = pMemory ? ::new (pMemory) TESObjectCELL : nullptr;
		pPersistentCell->SetPersistentCell(true);
		pPersistentCell->CreateCellData();
	}
	return pPersistentCell;
}

// Persistent references of the worldspace live in its persistent cell.
void TESWorldSpace::AddPersistentRef(TESObjectREFR* apRef) {
	if (apRef != nullptr && !apRef->GetTemporary()) {
		GetPersistentCell()->AddReference(apRef, false);
	}
}

void TESWorldSpace::RemovePersistentRef(TESObjectREFR* apRef) {
	// Reads the persistent cell without creating it.
	TESObjectCELL* pCell;
	if (apRef != nullptr && (pCell = QPersistentCell()) != nullptr) {
		pCell->RemoveReference(apRef);
	}
}

void TESWorldSpace::BuildTeleportDoorList(BSSimpleArray<TESObjectREFR*>& arOutArray) {
	TESObjectCELL* pCell = QPersistentCell();
	if (pCell != nullptr) {
		pCell->BuildTeleportDoorList(arOutArray);
	}
}

BSSimpleList<TESObjectREFR*>* TESWorldSpace::BuildMapMarkerList(bool abLocalOnly) {
	BSSimpleList<TESObjectREFR*>* pretlist_2;
	if (!GetParentWorld(TESWorldSpace::PU_MAP) || abLocalOnly) {
		BSSimpleList<TESObjectREFR*>* v6 =
			static_cast<BSSimpleList<TESObjectREFR*>*>(ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>)));
		if (v6) {
			pretlist_2 = ::new (v6) BSSimpleList<TESObjectREFR*>;
		} else {
			pretlist_2 = 0;
		}
		AddMapMarkersToList(pretlist_2);
		if (!abLocalOnly) {
			for (BSSimpleList<TESWorldSpace*>* plist = pDataHandler->GetWorldSpaceList(); plist;
				plist = plist->m_pkNext) {
				TESWorldSpace* pworld = plist->m_item;
				if (pworld) {
					if (pworld->GetParentWorld(TESWorldSpace::PU_MAP) == this) {
						pworld->AddMapMarkersToList(pretlist_2);
					}
				}
			}
		}
	} else {
		TESWorldSpace* ParentWorld = GetParentWorld(TESWorldSpace::PU_MAP);
		return ParentWorld->BuildMapMarkerList(0);
	}
	return pretlist_2;
}

BSSimpleList<TESObjectREFR*>* TESWorldSpace::BuildAudioMarkerList(bool abLocalOnly) {
	BSSimpleList<TESObjectREFR*>* pretlist_2;
	if (!GetParentWorld(TESWorldSpace::PU_MAP) || abLocalOnly) {
		BSSimpleList<TESObjectREFR*>* v6 =
			static_cast<BSSimpleList<TESObjectREFR*>*>(ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>)));
		if (v6) {
			pretlist_2 = ::new (v6) BSSimpleList<TESObjectREFR*>;
		} else {
			pretlist_2 = 0;
		}
		AddAudioMarkersToList(pretlist_2);
		if (!abLocalOnly) {
			for (BSSimpleList<TESWorldSpace*>* plist = pDataHandler->GetWorldSpaceList(); plist;
				plist = plist->m_pkNext) {
				TESWorldSpace* pworld = plist->m_item;
				if (pworld) {
					if (pworld->GetParentWorld(TESWorldSpace::PU_MAP) == this) {
						pworld->AddAudioMarkersToList(pretlist_2);
					}
				}
			}
		}
	} else {
		TESWorldSpace* ParentWorld = GetParentWorld(TESWorldSpace::PU_MAP);
		return ParentWorld->BuildAudioMarkerList(0);
	}
	return pretlist_2;
}

TESObjectCELL* TESWorldSpace::LoadCell(int32_t aiX, int32_t aiY) {
	AutoMemContext tempMemContext(MEM_CONTEXT_CELLS, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWorldSpace.cpp", 1942);
	bool bsuccess = 1;
	TESObjectCELL* pretcell = GetCellFromCellCoord(aiX, aiY);
	pLastLoadedWorld = this;
	if (!pretcell || !pretcell->GetHasTempData()) {
		uint32_t ifilecount = GetFileCount();
		for (uint32_t i = 0; i < ifilecount; ++i) {
			TESFile* File = GetFile(i);
			TESFile* pfile = File->GetThreadSafeFile();
			if (pfile && FindCellInFile(pfile, aiX, aiY)) {
				if (pretcell) {
					if ((pfile->GetFormData()->flags & 0x4000) == 0) {
						TESDataHandler::LoadForm(pretcell, pfile);
					}
				} else {
					TESObjectCELL* pretcell_2;
					TESObjectCELL* v11 = static_cast<TESObjectCELL*>(ourMalloc(sizeof(TESObjectCELL)));
					if (v11) {
						pretcell_2 = ::new (v11) TESObjectCELL;
					} else {
						pretcell_2 = 0;
					}
					pretcell = pretcell_2;
					pretcell_2->SetInterior(0);
					pretcell->CreateCellData();
					pretcell->SetDataCoord(aiX, aiY);
					AddCell(pretcell);
					uint32_t CastingType = iFormID;
					uint32_t icellID = pSaveLoadGameOLD->GetCreatedExteriorCellFormID(CastingType, aiX, aiY);
					if (icellID) {
						pretcell->SetFormID(icellID, 1);
					}
					bsuccess = TESDataHandler::LoadForm(pretcell, pfile);
				}
				if (!pretcell->LoadTempDataFromFile(pfile)) {
					bsuccess = 0;
				}
			}
		}
		if (pretcell) {
			pretcell->SetHasTempData(1);
			bool Loading = pSaveLoadGameOLD->GetLoading();
			pSaveLoadGameOLD->SetLoading(!Loading);
			pretcell->InitItem();
			pSaveLoadGameOLD->SetLoading(Loading);
			AssignPersistentRefsToCell(pretcell);
		}
	}
	if (!bsuccess) {
		int32_t v6 = iFormID;
		Warning("CELLS: Failed to load temporary data for cell (%i, %i) in worldspace '%s' (%08X).", aiX, aiY,
			GetFormEditorID(), v6);
	}
	return pretcell;
}

TESObjectCELL* TESWorldSpace::GetCellByEditorID(const char* apEditorID) {
	NiTMapItem<int32_t, TESObjectCELL*>* it;
	TESObjectCELL* pFoundCell = 0;
	if (apEditorID) {
		it = pCellMap->GetFirstPos();
		while (it && !pFoundCell) {
			int32_t ignore;
			TESObjectCELL* pCell = 0;
			pCellMap->GetNext(it, ignore, pCell);
			if (pCell) {
				const char* v2 = pCell->GetFormEditorID();
				if (!BSstricmp(v2, apEditorID)) {
					pFoundCell = pCell;
				}
			}
		}
	}
	return pFoundCell;
}

void TESWorldSpace::EnumReferencesCloseToPoint(const NiPoint3& aPoint1, float afRadius1, const NiPoint3& aPoint2,
	float afRadius2, bool (*apfnEnumFunc)(TESObjectREFR*, void*), void* aiEnumData) {
	if (apfnEnumFunc) {
		int32_t v21 = 1;
		int32_t v15 = 0;
		int32_t aiX = static_cast<int32_t>(aPoint1.x) >> 12;
		int32_t aiY = static_cast<int32_t>(aPoint1.y) >> 12;
		TESObjectCELL* CellFromCellCoord = GetCellFromCellCoord(aiX, aiY);
		if (!CellFromCellCoord ||
			CellFromCellCoord->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc,
				aiEnumData)) {
			while (v21) {
				--aiX;
				--aiY;
				v15 += 2;
				v21 = 0;
				for (int32_t i = 0; i < v15; ++i) {
					TESObjectCELL* v10 = GetCellFromCellCoord(aiX, aiY);
					if (v10) {
						if ((afRadius1 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius1) > v10->GetDistanceFromPoint(aPoint1)) &&
							(afRadius2 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius2) > v10->GetDistanceFromPoint(aPoint2))) {
							++v21;
							if (!v10->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc,
									aiEnumData)) {
								return;
							}
						}
					}
					++aiX;
				}
				for (int32_t j = 0; j < v15; ++j) {
					TESObjectCELL* v11 = GetCellFromCellCoord(aiX, aiY);
					if (v11) {
						if ((afRadius1 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius1) > v11->GetDistanceFromPoint(aPoint1)) &&
							(afRadius2 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius2) > v11->GetDistanceFromPoint(aPoint2))) {
							++v21;
							if (!v11->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc,
									aiEnumData)) {
								return;
							}
						}
					}
					++aiY;
				}
				for (int32_t k = 0; k < v15; ++k) {
					TESObjectCELL* v12 = GetCellFromCellCoord(aiX, aiY);
					if (v12) {
						if ((afRadius1 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius1) > v12->GetDistanceFromPoint(aPoint1)) &&
							(afRadius2 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius2) > v12->GetDistanceFromPoint(aPoint2))) {
							++v21;
							if (!v12->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc,
									aiEnumData)) {
								return;
							}
						}
					}
					--aiX;
				}
				for (int32_t m = 0; m < v15; ++m) {
					TESObjectCELL* v13 = GetCellFromCellCoord(aiX, aiY);
					if (v13) {
						if ((afRadius1 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius1) > v13->GetDistanceFromPoint(aPoint1)) &&
							(afRadius2 == 3.4028234663852886e+38 ||
								static_cast<double>(afRadius2) > v13->GetDistanceFromPoint(aPoint2))) {
							++v21;
							if (!v13->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc,
									aiEnumData)) {
								return;
							}
						}
					}
					--aiY;
				}
			}
			TESObjectCELL* Interior = QPersistentCell();
			if (Interior) {
				Interior->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc, aiEnumData);
			}
		}
	}
}

void TESWorldSpace::SetWorldWater(TESWaterForm* apWater) {
	pWorldWater = apWater;
}

void TESWorldSpace::SetLODWater(TESWaterForm* apWater) {
	pLODWater = apWater;
}

void TESWorldSpace::SetWaterLODHeight(float afHeight) {
	fLODWaterHeight = afHeight;
}

void TESWorldSpace::SetDefaultLandHeight(float afVal) {
	fDefaultLandHeight = afVal;
}

void TESWorldSpace::SetMusicType(BGSMusicType* apType) {
	pMusicType = apType;
}

BGSEncounterZone* TESWorldSpace::GetEncounterZone() const {
	return pEncounterZone;
}

// Returns an empty string when the path is unset.
const char* TESWorldSpace::GetWaterNoiseTexturePath() const {
	return const_cast<TESTexture&>(WaterNoiseTexture).GetTextureName();
}

void TESWorldSpace::RemoveFromPersistentRefData(TESObjectREFR* apRef) {
	if (!apRef || apRef->GetTemporary()) {
		return;
	}
	PersistentDataCritSection.Enter(0);
	if (!IsFixedRef(apRef)) {
		MobilePersistentRefList.Remove(apRef);
	} else {
		int32_t iKey = GetKeyForWorldCoord(apRef->GetLocationOnReference());
		BSSimpleList<TESObjectREFR*>* pList = nullptr;
		FixedPersistentRefMap.GetAt(iKey, pList);
		if (pList) {
			pList->Remove(apRef);
			if (pList->IsEmpty()) {
				BSSimpleList<TESObjectREFR*>* pDeleted = pList;
				if (pDeleted) {
					pDeleted->RemoveAll();
					ourFree(pDeleted);
				}
				FixedPersistentRefMap.RemoveAt(iKey);
			}
		}
	}
	PersistentDataCritSection.Leave();
}

// The fixed lists are keyed by the reference's cell.
void TESWorldSpace::AddToPersistentRefData(TESObjectREFR* apRef) {
	if (!apRef || apRef->GetTemporary()) {
		return;
	}
	PersistentDataCritSection.Enter(0);
	if (!IsFixedRef(apRef)) {
		MobilePersistentRefList.AddHead(apRef);
	} else {
		uint32_t iKey = GetKeyForWorldCoord(apRef->GetLocationOnReference());
		BSSimpleList<TESObjectREFR*>* pList = nullptr;
		FixedPersistentRefMap.GetAt(iKey, pList);
		if (!pList) {
			void* pMemory = ourMalloc(sizeof(BSSimpleList<TESObjectREFR*>));
			BSSimpleList<TESObjectREFR*>* pNew = pMemory ? ::new (pMemory) BSSimpleList<TESObjectREFR*> : nullptr;
			pList = pNew;
			FixedPersistentRefMap.SetAt(iKey, pList);
		}
		pList->AddHead(apRef);
	}
	PersistentDataCritSection.Leave();
}

bool TESWorldSpace::FindCellInFile(TESFile* apFile, int32_t aiX, int32_t aiY) {
	EXTERIOR_DATA coorddata;
	TESWorldSpace::OFFSET_DATA* pdata;
	if (!apFile || (!apFile->GetMaster() || ((pdata = GetOffsetData(apFile)) == 0 || !pdata->pCellFileOffsets))) {
		bool bretval = 0;
		uint32_t iblockkey = TESObjectCELL::CalcExtGroupBlockKey(aiX, aiY);
		uint32_t isubblockkey = TESObjectCELL::CalcExtGroupSubBlockKey(aiX, aiY);
		if (!apFile || !apFile->FindForm(this)) {
			return bretval;
		}
		apFile->NextForm(1);
		bool bquit = 0;
		FORM* pFORM = apFile->GetFormData();
		while (1) {
			while (1) {
				if (!pFORM || bquit) {
					return bretval;
				}
				if (pFORM->form != formEnumString[2].uiFormString) {
					break;
				}
				bool bskipgroup = 1;
				bquit = 1;
				switch (pFORM->iFormID) {
				case 1u: {
					bskipgroup = 0;
					goto LABEL_17;
				}
				case 4u: {
				LABEL_17:;
					if (iblockkey == pFORM->flags) {
						bskipgroup = 0;
					}
					goto LABEL_19;
				}
				case 5u: {
				LABEL_19:;
					if (isubblockkey == pFORM->flags) {
						bskipgroup = 0;
					}
					goto LABEL_21;
				}
				case 6u:
				case 8u:
				case 9u: {
				LABEL_21:;
					bquit = 0;
					break;
				}
				default: {
					break;
				}
				}
				if (!bquit) {
					if (bskipgroup) {
						apFile->NextGroup();
					} else {
						apFile->NextForm(1);
					}
				}
			}
			if (pFORM->form == formEnumString[57].uiFormString) {
				coorddata.~EXTERIOR_DATA();
				::new (&coorddata) EXTERIOR_DATA;
				ourMemset(&coorddata, 0, sizeof(coorddata));
				if ((pFORM->flags & 1024) != 0) {
					coorddata.iCellX = 0x7FFFFFFF;
				} else {
					bool bmoveon = 0;
					CHUNK_ID chunk = apFile->GetTESChunk();
					while (chunk && !bmoveon) {
						if (chunk == XCLC_ID) {
							apFile->GetChunkData(&coorddata, 12u);
							if (apFile->QEndian()) {
								coorddata.Endian();
							}
							bmoveon = 1;
						}
						if (apFile->NextChunk()) {
							chunk = apFile->GetTESChunk();
						} else {
							chunk = NO_CHUNK;
						}
					}
				}
				if (coorddata.iCellX == aiX && coorddata.iCellY == aiY) {
					bretval = 1;
					bquit = 1;
					apFile->TESRewindChunk();
				} else {
				LABEL_45:;
					apFile->NextForm(1);
				}
			} else {
				if (pFORM->form == formEnumString[68].uiFormString) {
					goto LABEL_45;
				}
				bquit = 1;
			}
		}
	}
	int32_t iindex = GetIndexForCellCoord(apFile, aiX, aiY);
	if (iindex == 0xFFFFFFFFu) {
		return 0;
	}
	uint32_t ioffset = pdata->pCellFileOffsets[iindex];
	if (!ioffset) {
		return 0;
	}
	apFile->SetOffset(pdata->iFileOffset + ioffset);
	return 1;
}

bool TESWorldSpace::FindLandDataInFile(int32_t aiX, int32_t aiY, TESFile*& apOutFile, uint32_t& aiOutOffset) {
	bool v12 = 0;
	apOutFile = nullptr;
	aiOutOffset = 0;
	if (GetParentWorld(TESWorldSpace::PU_LAND)) {
		TESWorldSpace* ParentWorld = GetParentWorld(TESWorldSpace::PU_LAND);
		return ParentWorld->FindLandDataInFile(aiX, aiY, apOutFile, aiOutOffset);
	} else {
		uint32_t FileCount = GetFileCount();
		if (FileCount) {
			for (int32_t aiIndex = FileCount - 1; aiIndex >= 0; --aiIndex) {
				TESFile* File = GetFile(aiIndex);
				TESFile* apFile = File->GetThreadSafeFile();
				if (apFile && (FindCellInFile(apFile, aiX, aiY) && TESObjectCELL::FindLandDataInFile(apFile))) {
					apOutFile = apFile;
					aiOutOffset = apFile->GetOffset();
					return 1;
				}
			}
		}
	}
	return v12;
}

// The found cell's slot is deliberately left uninitialised, as the game does: GetAt writes it only when it returns
// true. The warning's arguments are fetched right to left (Y, X, editor ID, form ID).
bool TESWorldSpace::AddCell(TESObjectCELL* apCell) {
	if (!apCell || apCell->GetInterior()) {
		return false;
	}
	if (apCell->GetPersistentCell()) {
		if (pPersistentCell) {
			return false;
		}
		pPersistentCell = apCell;
	} else {
		int32_t iY = apCell->GetDataY();
		int32_t iX = apCell->GetDataX();
		int32_t iKey = TESObjectCELL::GetCoord(iX, iY);
		TESObjectCELL* pExisting;
		if (pCellMap->GetAt(iKey, pExisting)) {
			int32_t iExistingY = apCell->GetDataY();
			int32_t iExistingX = apCell->GetDataX();
			const char* pEditorID = pExisting->GetFormEditorID();
			uint32_t uiFormID = pExisting->GetFormID();
			Warning("CELLS: Cell (%08X) %s already exists at coord (%i, %i ).", uiFormID, pEditorID, iExistingX,
				iExistingY);
			return false;
		}
		pCellMap->SetAt(iKey, apCell);
	}
	apCell->SetWorldSpace(this);
	return true;
}

// The out-of-range warning names the int16 bounds; the coordinates are tested signed.
TESObjectCELL* TESWorldSpace::GetCellFromCellCoord(int32_t aiX, int32_t aiY) {
	if (aiX > 0x7FFF || aiY > 0x7FFF || aiX < -0x8000 || aiY < -0x8000) {
		Warning("CELLS: Trying to get exterior cell for invalid cell coordinate. Values must be between %i and %i.",
			-0x8000, 0x7FFF);
		return nullptr;
	}
	// Both coordinates are passed zero-extended as 16-bit values.
	int32_t iKey = TESObjectCELL::GetCoord(uint16_t(aiX), uint16_t(aiY));
	TESObjectCELL* pCell = nullptr;
	if (pCellMap->GetAt(iKey, pCell)) {
		return pCell;
	}
	return nullptr;
}

TESObjectCELL* TESWorldSpace::GetCellFromWorldCoord(const NiPoint3& aCoord) {
	int32_t v5 = FloatToInt((&aCoord)->x) >> 12;
	int32_t v3 = FloatToInt((&aCoord)->y) >> 12;
	return GetCellFromCellCoord(v5, v3);
}

void TESWorldSpace::AdjustCellOffsets(TESFile* apFile, int32_t aiDelta) {
	TESWorldSpace::OFFSET_DATA* pdata = GetOffsetData(apFile);
	if (pdata) {
		if (pdata->pCellFileOffsets) {
			if (aiDelta) {
				int32_t iYMin = static_cast<int32_t>(pdata->OffsetMinCoords.y) >> 12;
				int32_t iXMax = static_cast<int32_t>(pdata->OffsetMaxCoords.x) >> 12;
				int32_t iYMax = static_cast<int32_t>(pdata->OffsetMaxCoords.y) >> 12;
				for (int32_t ix = static_cast<int32_t>(pdata->OffsetMinCoords.x) >> 12; ix <= iXMax; ++ix) {
					for (int32_t iy = iYMin; iy <= iYMax; ++iy) {
						int32_t iindex = GetIndexForCellCoord(apFile, ix, iy);
						if (iindex >= 0) {
							if (pdata->pCellFileOffsets[iindex]) {
								pdata->pCellFileOffsets[iindex] += aiDelta;
							}
						}
					}
				}
			}
		}
	}
}

TESClimate* TESWorldSpace::GetClimate() {
	if (!GetParentWorld(TESWorldSpace::PU_CLIMATE)) {
		return pClimate;
	}
	TESWorldSpace* ParentWorld = GetParentWorld(TESWorldSpace::PU_CLIMATE);
	return ParentWorld->GetClimate();
}

TESImageSpace* TESWorldSpace::GetImageSpace() {
	TESImageSpace* pImageSpace = this->pImageSpace;
	if (!pImageSpace) {
		if (GetParentWorld(TESWorldSpace::PU_UNUSED)) {
			TESWorldSpace* ParentWorld = GetParentWorld(TESWorldSpace::PU_UNUSED);
			return ParentWorld->GetImageSpace();
		}
	}
	return pImageSpace;
}

BGSMusicType* TESWorldSpace::GetMusicType() {
	return pMusicType;
}

BSPortalGraph* TESWorldSpace::GetPortalGraph() {
	BSPortalGraph* v2;
	void* v4;
	if (!(&spPortalGraph)->m_pObject) {
		v4 = NiMemObject::operator new(sizeof(BSPortalGraph));
		if (v4) {
			v2 = ::new (v4) BSPortalGraph;
		} else {
			v2 = nullptr;
		}
		(&spPortalGraph)->operator=(v2);
	}
	return (&spPortalGraph)->m_pObject;
}

void TESWorldSpace::ClearPortalGraph() {
	BSPortalGraph* v1;
	ShadowSceneNode* shadowSceneNode;
	BSPortalGraph* portalGraph;
	ShadowSceneNode* v4;
	if ((&spPortalGraph)->m_pObject) {
		v1 = (&spPortalGraph)->m_pObject;
		if (v1->NiRefObject::GetRefCount() == 1) {
			shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
			portalGraph = shadowSceneNode->GetPortalGraph();
			if (portalGraph == (&spPortalGraph)->m_pObject) {
				v4 = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
				v4->SetPortalGraph(nullptr);
			}
			(&spPortalGraph)->operator=(nullptr);
		}
	}
}

BSPortalGraph* TESWorldSpace::QPortalGraph() {
	return spPortalGraph;
}

float TESWorldSpace::GetDefaultWaterHeight() {
	return fDefaultWaterHeight;
}

BSSimpleList<TESObjectREFR*>* TESWorldSpace::GetOverlappedMultiBoundRefs(TESObjectCELL* apCell) {
	BSSimpleList<TESObjectREFR*>* preflist = 0;
	if (pOverlappedMultiboundMap) {
		int32_t DataY = apCell->GetDataY();
		int32_t DataX = apCell->GetDataX();
		// The map's key is the cell's packed coordinate, which GetCoord returns signed.
		pOverlappedMultiboundMap->GetAt(static_cast<uint32_t>(TESObjectCELL::GetCoord(DataX, DataY)), preflist);
	}
	return preflist;
}

float TESWorldSpace::GetMinX() const {
	return MinimumCoords.X;
}

float TESWorldSpace::GetDefaultLandHeight() {
	return fDefaultLandHeight;
}

bool TESWorldSpace::GetHasBorderRegion() {
	return ((cFlags & 8) != 0);
}

void TESWorldSpace::SetHasBorderRegion(bool abVal) {
	if (abVal) {
		cFlags |= 8u;
	} else {
		cFlags &= (~8u);
	}
}

bool TESWorldSpace::GetNoLODWater() {
	return ((cFlags & 16) != 0);
}

bool TESWorldSpace::GetNeedsWaterAdjustment() {
	return ((cFlags & 0x80) != 0);
}

bool TESWorldSpace::IsAllowNPCFallDamage() {
	return ((cFlags & 64) == 0);
}

void TESWorldSpace::AddPersistentReferences() {
	TESObjectCELL* interior = TESWorldSpace::QPersistentCell();
	if (interior) {
		interior->AddReferences(this);
	}
}

void TESWorldSpace::AddAudioMarkersToList(BSSimpleList<TESObjectREFR*>* apMarkList) {
	TESObjectCELL* interior;
	if (apMarkList) {
		interior = TESWorldSpace::QPersistentCell();
		if (interior) {
			interior->AddAudioMarkersToList(apMarkList);
		}
	}
}

void TESWorldSpace::UnLoadCell(TESObjectCELL* apCell) {
	if (apCell) {
		if (apCell->GetWorldSpace() == this) {
			TESFile* File = apCell->GetFile(-1);
			if (File && !File->GetMaster()) {
				apCell->UnLoadAllTempData();
			} else {
				apCell->UnLoadAllTempData();
				pSaveLoadGameOLD->UnloadForm(apCell);
				uint32_t CastingType = apCell->iFormID;
				if (!pDataHandler->IsFormIDCreated(CastingType)) {
					pSaveLoadGame->UnloadForm(apCell, 0);
				}
				ReleaseCell(apCell);
				delete apCell;
			}
		}
	}
}

void TESWorldSpace::ReleaseCell(TESObjectCELL* apCell) {
	int32_t dataX;
	int32_t dataY;
	int32_t a1;
	TESObjectCELL* a2;
	if (apCell) {
		if (!apCell->GetInterior()) {
			dataY = apCell->GetDataY();
			dataX = apCell->GetDataX();
			a1 = TESObjectCELL::GetCoord(dataX, dataY);
			bool vCondition1;
			vCondition1 = pCellMap->GetAt(a1, a2) && (apCell == a2);
			if (vCondition1) {
				pCellMap->RemoveAt(a1);
				apCell->SetWorldSpace(nullptr);
			}
		}
	}
}

void TESWorldSpace::AssignPersistentRefsToCell(TESObjectCELL* apCell) {
	if (apCell) {
		TESWorldSpace::PersistentDataCritSection.Enter(0);
		int32_t DataY = apCell->GetDataY();
		int32_t DataX = apCell->GetDataX();
		uint32_t ikey = TESObjectCELL::GetCoord(DataX, DataY);
		BSSimpleList<TESObjectREFR*>* plist = 0;
		FixedPersistentRefMap.GetAt(ikey, plist);
		while (plist && !plist->IsEmpty()) {
			TESObjectREFR* pref = plist->m_item;
			apCell->AddReference(pref, 0);
			plist = plist->m_pkNext;
		}
		for (plist = &MobilePersistentRefList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			TESObjectREFR* pref_2 = plist->m_item;
			if (pref_2->GetParentCell() != apCell) {
				const NiPoint3* v3 = &pref_2->GetLocationOnReference();
				if (apCell->IsPointInCellExterior(*v3)) {
					apCell->AddReference(pref_2, 0);
				}
			}
		}
		TESWorldSpace::PersistentDataCritSection.Leave();
	}
}

int32_t TESWorldSpace::GetIndexForCellCoord(TESFile* apFile, int32_t aiX, int32_t aiY) {
	TESWorldSpace::OFFSET_DATA* pdata = GetOffsetData(apFile);
	if (!pdata) {
		return 0xFFFFFFFFu;
	}
	int32_t iMinX = static_cast<int32_t>(pdata->OffsetMinCoords.x) >> 12;
	int32_t iMinY = static_cast<int32_t>(pdata->OffsetMinCoords.y) >> 12;
	int32_t iMaxX = static_cast<int32_t>(pdata->OffsetMaxCoords.x) >> 12;
	if (aiX <= iMaxX &&
		(aiX >= iMinX && (aiY <= static_cast<int32_t>(pdata->OffsetMaxCoords.y) >> 12 && aiY >= iMinY))) {
		return aiX + (aiY - iMinY) * (iMaxX - iMinX + 1) - iMinX;
	} else {
		return 0xFFFFFFFFu;
	}
}

bool TESWorldSpace::GetExtCellDataFromFileByEditorID(const char* apCellID, int32_t& aiOutX, int32_t& aiOutY) {
	uint32_t ifilecount = GetFileCount();
	for (uint32_t i = 0; i < ifilecount; ++i) {
		TESFile* File = GetFile(static_cast<int32_t>(i));
		TESFile* pfile = File->GetThreadSafeFile();
		TESWorldSpace::OFFSET_DATA* pdata = GetOffsetData(pfile);
		if (pdata) {
			int32_t iminy = static_cast<int32_t>(pdata->OffsetMinCoords.y) >> 12;
			int32_t imaxx = static_cast<int32_t>(pdata->OffsetMaxCoords.x) >> 12;
			int32_t imaxy = static_cast<int32_t>(pdata->OffsetMaxCoords.y) >> 12;
			for (int32_t ix = static_cast<int32_t>(pdata->OffsetMinCoords.x) >> 12; ix <= imaxx; ++ix) {
				for (int32_t iy = iminy; iy <= imaxy; ++iy) {
					uint32_t iindex = GetIndexForCellCoord(pfile, ix, iy);
					if (iindex != 0xFFFFFFFFu) {
						uint32_t ioffset = pdata->pCellFileOffsets[iindex];
						if (ioffset) {
							pfile->SetOffset(pdata->iFileOffset + ioffset);
							if (pfile->GetTESForm() == CELL_ID && pfile->GetTESChunk() == EDID_ID) {
								int32_t isize = pfile->GetChunkSize() + 1;
								char* pfileID = static_cast<char*>(ourMalloc(static_cast<uint32_t>(isize)));
								ourMemset(pfileID, 0, static_cast<uint32_t>(isize));
								pfile->GetChunkData(pfileID, 0);
								if (!BSstricmp(pfileID, apCellID)) {
									aiOutX = ix;
									aiOutY = iy;
									ourFree(pfileID);
									return 1;
								}
								ourFree(pfileID);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

void TESWorldSpace::ClearCellMap() {
	if (!GetTemporary()) {
		NiTMapItem<int32_t, TESObjectCELL*>* it = pCellMap->GetFirstPos();
		while (it) {
			int32_t ignore;
			TESObjectCELL* pCell = 0;
			pCellMap->GetNext(it, ignore, pCell);
			if (pCell) {
				delete pCell;
			}
		}
	}
	pCellMap->RemoveAll();
}

void TESWorldSpace::ClearMultiBoundRefs() {
	NiTMapItem<uint32_t, BSSimpleList<TESObjectREFR*>*>* iter;
	if (pOverlappedMultiboundMap) {
		iter = pOverlappedMultiboundMap->GetFirstPos();
		while (iter) {
			uint32_t iignore = 0;
			BSSimpleList<TESObjectREFR*>* preflist = 0;
			pOverlappedMultiboundMap->GetNext(iter, iignore, preflist);
			if (preflist) {
				preflist->RemoveAll();
				if (preflist) {
					delete preflist;
				}
			}
		}
		pOverlappedMultiboundMap->RemoveAll();
		if (pOverlappedMultiboundMap) {
			delete pOverlappedMultiboundMap;
		}
		pOverlappedMultiboundMap = 0;
	}
}

void TESWorldSpace::ClearPersistentRefData() {
	TESWorldSpace::PersistentDataCritSection.Enter(0);
	MobilePersistentRefList.RemoveAll();
	NiTMapItem<uint32_t, BSSimpleList<TESObjectREFR*>*>* iter = FixedPersistentRefMap.GetFirstPos();
	while (iter) {
		uint32_t iignore = 0;
		BSSimpleList<TESObjectREFR*>* preflist = 0;
		FixedPersistentRefMap.GetNext(iter, iignore, preflist);
		if (preflist) {
			preflist->RemoveAll();
			if (preflist) {
				delete preflist;
			}
		}
	}
	FixedPersistentRefMap.RemoveAll();
	TESWorldSpace::PersistentDataCritSection.Leave();
}

void TESWorldSpace::AddMapMarkersToList(BSSimpleList<TESObjectREFR*>* apMarkList) {
	if (apMarkList) {
		TESObjectCELL* v2 = pPersistentCell;
		if (v2) {
			v2->AddMapMarkersToList(apMarkList);
		}
	}
}

void TESWorldSpace::FreeOffsetData() {
	NiTMapItem<TESFile*, TESWorldSpace::OFFSET_DATA*>* iter;
	iter = OffsetDataMap.GetFirstPos();
	while (iter) {
		TESWorldSpace::OFFSET_DATA* pdata = 0;
		TESFile* pfile = 0;
		OffsetDataMap.GetNext(iter, pfile, pdata);
		ourFree(pdata->pCellFileOffsets);
		ourFree(pdata);
	}
	OffsetDataMap.RemoveAll();
}

bool TESWorldSpace::IsPointInBorderRegion(const NiPoint3& arPoint) {
	if (!GetHasBorderRegion()) {
		return 1;
	}
	TESRegionPoint regionPoint(arPoint.x, arPoint.y);
	bool bfoundInRegion = 0;
	// The list is walked as TESRegionList objects: each next node is cast down to the list class (null kept null).
	for (TESRegionList* pregionList = pDataHandler->GetRegionList(); pregionList && pregionList->m_item;
		pregionList = static_cast<TESRegionList*>(pregionList->m_pkNext)) {
		TESRegion* pregion = pregionList->m_item;
		if (pregion->TESForm::GetHasSpokenFlag() && !pregion->GetDelete() && pregion->pWorldSpace == this) {
			for (TESRegionPointList*& rpItem : pregion->pPointLists->ItemsUntilNull()) {
				if (rpItem->IsInsideRegion(&regionPoint)) {
					bfoundInRegion = 1;
				}
			}
		}
	}
	return bfoundInRegion;
}

int32_t TESWorldSpace::GetKeyForWorldCoord(const NiPoint3& aCoord) {
	if (!std::isfinite(aCoord.x) || !std::isfinite(aCoord.y) || !std::isfinite(aCoord.z) || std::isnan(aCoord.x) ||
		std::isnan(aCoord.y) || std::isnan(aCoord.z)) {
		return 0x7FFF7FFF;
	}
	int32_t icellx = FloatToInt(aCoord.x) >> 12;
	int32_t icelly = FloatToInt(aCoord.y) >> 12;
	// Both coordinates are passed zero-extended as 16-bit values.
	return TESObjectCELL::GetCoord(uint16_t(icellx), uint16_t(icelly));
}

bool TESWorldSpace::IsFixedRef(TESObjectREFR* apRef) {
	if (!apRef || !apRef->GetObjectReference()) {
		return false;
	}
	switch (apRef->GetObjectReference()->GetFormType()) {
	case SOUN_ID:
	case ACTI_ID:
	case CONT_ID:
	case DOOR_ID:
	case STAT_ID:
	case SCOL_ID:
	case TREE_ID:
	case FLOR_ID:
	case FURN_ID:
	case LVLC_ID:
	case LVLN_ID:
		return true;
	default:
		return false;
	}
}

bool TESWorldSpace::IsFormTypeWorldChild(uint8_t aeType) {
	if (aeType == CELL_ID)
		return true;
	if (aeType == TLOD_ID)
		return true;
	return TESObjectCELL::IsFormTypeCellChild(aeType);
}

void TESWorldSpace::GetCellCoordForKey(uint32_t aiKey, int16_t& asOutX, int16_t& asOutY) {
	asOutX = static_cast<uint16_t>(aiKey >> 16);
	asOutY = aiKey;
}
