#include "Fallout Shared/TESWorldSpace.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSPortalGraph.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/BaseFormComponent.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <limits>

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bExternalLODDataFiles("bExternalLODDataFiles:General", true);
BSCriticalSection TESWorldSpace::PersistentDataCritSection;
SettingT<INISettingCollection> bCheckCellOffsetsOnInit("bCheckCellOffsetsOnInit:General", false);

void WorldSpaceMapData::Endian() {
    BSEndian(iUsableWidth, false);
    BSEndian(iUsableHeight, false);
    BSEndian(sNorthWestCellX, false);
    BSEndian(sNorthWestCellY, false);
    BSEndian(sSouthEastCellX, false);
    BSEndian(sSouthEastCellY, false);
}

void WorldSpaceMapOffsetData::Endian() {
    BSEndian(fMapScale, false);
    BSEndian(fCellOffsetX, false);
    BSEndian(fCellOffsetY, false);
}

void DEFAULT_CELL_DATA::Endian() {
    BSEndian(fLandHeight, false);
    BSEndian(fWaterHeight, false);
}

TESWorldSpace::OFFSET_DATA::OFFSET_DATA() {}

TESWorldSpace::TESWorldSpace()
  : pPersistentCell(nullptr), pTerrainManager(nullptr), FixedPersistentRefMap(37u), pOverlappedMultiboundMap(nullptr),
    spPortalGraph(nullptr), OffsetDataMap(37u) {
    cFormType = 65;
    pCellMap = new NiTPointerMap<int32_t, TESObjectCELL*>(37u);
    MinimumCoords.Y = std::numeric_limits<float>::max();
    MinimumCoords.X = std::numeric_limits<float>::max();
    MaximumCoords.Y = -std::numeric_limits<float>::max();
    MaximumCoords.X = -std::numeric_limits<float>::max();
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
    spPortalGraph = nullptr;
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
                char* pMessage = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above).
                pMessage[0] = '\0';
                apFile->GetChunkData(pMessage, 0x200);
                SetFormEditorID(pMessage);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case DATA_ID:
                if (apFile->GetChunkSize() == 4) {
                    uint32_t ival = 0;
                    apFile->GetChunkData(ival);
                    cFlags = static_cast<uint8_t>(ival);
                }
                else {
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
                    // Allocates ioffsetcount uint32_t entries through ourMalloc; the size cannot overflow, as
                    // ioffsetcount is at most 0x3FFFFFFF.
                    poffsetData->pCellFileOffsets = static_cast<uint32_t*>(ourMalloc(ioffsetcount * sizeof(uint32_t)));
                    apFile->GetChunkData(poffsetData->pCellFileOffsets, 0);
                    if (apFile->QEndian()) {
                        for (uint32_t i = 0; i < ioffsetcount; ++i) {
                            BSEndian(poffsetData->pCellFileOffsets[i], false);
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
                    pImpactSwap = new ImpactSwap;
                }
                pImpactSwap->Load(apFile);
                break;
            case NNAM_ID: {
                uint32_t isize = apFile->GetChunkSize();
                if (isize) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* ptemp = static_cast<char*>(XFNV_ALLOCA16(isize));
                    apFile->GetChunkData(ptemp, isize);
                    CanopyShadowTexture.SetTextureName(ptemp);
                }
                break;
            }
            case XNAM_ID: {
                uint32_t isize = apFile->GetChunkSize();
                if (isize) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* ptemp = static_cast<char*>(XFNV_ALLOCA16(isize));
                    apFile->GetChunkData(ptemp, isize);
                    WaterNoiseTexture.SetTextureName(ptemp);
                }
                break;
            }
            case NAM0_ID: {
                NiPoint2 tmpCoords = {};
                apFile->GetChunkData(&tmpCoords, 8);
                if (apFile->QEndian()) {
                    BSEndian(tmpCoords.x, false);
                    BSEndian(tmpCoords.y, false);
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
                    BSEndian(tmpCoords.x, false);
                    BSEndian(tmpCoords.y, false);
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
            default: break;
        }
    }
    while (apFile->NextChunk());
    if (!pCellMap->GetCount() && !GetSmallWorld()) {
        // The map is deleted here and the field is replaced below.
        delete pCellMap;
        if (IsDefaultForm()) {
            pCellMap = ::new NiTPointerMap<int32_t, TESObjectCELL*>(7001);
        }
        else {
            pCellMap = ::new NiTPointerMap<int32_t, TESObjectCELL*>(701);
        }
    }
    return true;
}

bool TESWorldSpace::LoadPartial(TESFile* apFile) {
    NiPoint2 kMaxCoords;
    NiPoint2 kMinCoords;
    CHUNK_ID chunkid = NO_CHUNK;
    do {
        chunkid = apFile->GetTESChunk();
        if (chunkid == NO_CHUNK) {
            break;
        }
        if (chunkid == NAM0_ID) {
            apFile->GetChunkData(&kMinCoords, 8u);
            if (apFile->QEndian()) {
                BSEndian<float>(kMinCoords.x, false);
                BSEndian<float>(kMinCoords.y, false);
            }
            MinimumCoords.X = BSMIN(MinimumCoords.X, kMinCoords.x);
            MinimumCoords.Y = BSMIN(MinimumCoords.Y, kMinCoords.y);
            if (apFile->GetMaster()) {
                TESWorldSpace::OFFSET_DATA* pdata = CreateOffsetData(apFile);
                pdata->OffsetMinCoords = kMinCoords;
            }
        }
        else if (chunkid == NAM9_ID) {
            apFile->GetChunkData(&kMaxCoords, 8u);
            if (apFile->QEndian()) {
                BSEndian<float>(kMaxCoords.x, false);
                BSEndian<float>(kMaxCoords.y, false);
            }
            MaximumCoords.X = BSMAX(MaximumCoords.X, kMaxCoords.x);
            MaximumCoords.Y = BSMAX(MaximumCoords.Y, kMaxCoords.y);
            if (apFile->GetMaster()) {
                TESWorldSpace::OFFSET_DATA* pdata = CreateOffsetData(apFile);
                pdata->OffsetMaxCoords = kMaxCoords;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
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
    if (!GetParentUseFlag(TESWorldSpace::PU_CLIMATE) && GetClimate()) {
        tempint = GetClimate()->iFormID;
        TESForm::AddChunk(CNAM_ID, tempint);
    }

    if (!GetParentUseFlag(TESWorldSpace::PU_WATER) && GetWaterType()) {
        tempint = GetWaterType()->iFormID;
        TESForm::AddChunk(NAM2_ID, tempint);
    }

    if (!GetParentUseFlag(TESWorldSpace::PU_LOD) && GetLODWaterType()) {
        tempint = GetLODWaterType()->iFormID;
        TESForm::AddChunk(NAM3_ID, tempint);
        TESForm::AddChunk(NAM4_ID, std::bit_cast<uint32_t>(fLODWaterHeight));
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
    if (!GetParentUseFlag(TESWorldSpace::PU_UNUSED) && pImageSpace) {
        tempint = pImageSpace->iFormID;
        TESForm::AddChunk(INAM_ID, tempint);
    }

    TESForm::AddChunk(DATA_ID, cFlags);
    TESForm::AddChunkArray(NAM0_ID, &MinimumCoords.X, 2u);
    TESForm::AddChunkArray(NAM9_ID, &MaximumCoords.X, 2u);
    if (GetMusicType()) {
        tempint = GetMusicType()->iFormID;
        TESForm::AddChunk(ZNAM_ID, tempint);
    }
    TESForm::AddChunkArray(
        NNAM_ID,
        CanopyShadowTexture.GetTextureName(),
        CanopyShadowTexture.GetTextureNameLength() + 1
    );
    TESForm::AddChunkArray(XNAM_ID, WaterNoiseTexture.GetTextureName(), WaterNoiseTexture.GetTextureNameLength() + 1);
    if (pImpactSwap) {
        pImpactSwap->Save();
    }
    CloseForm();
}

bool TESWorldSpace::SavesBefore(TESForm* apForm) {
    bool bretval = false;
    switch (apForm->GetFormType()) {
        case CELL_ID: {
            TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(apForm);
            TESWorldSpace* pcellworld = pcell ? pcell->GetWorldSpace() : nullptr;
            if (pcellworld == this) {
                bretval = true;
            }
            else if (pcellworld) {
                bretval = SavesBefore(pcellworld);
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
            bretval = SavesBefore(pcellchild->GetSaveParentCell());
            break;
        }
        case WRLD_ID: {
            bretval = iFormID < apForm->iFormID;
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
    bool bretval = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        switch (apGroupFORM->iFormID) {
            case 0u: {
                bretval = TESForm::SavesBefore(apGroupFORM);
                break;
            }
            case 1u: {
                TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
                TESWorldSpace* pworld = dynamic_cast<TESWorldSpace*>(pForm);
                if (pworld) {
                    bretval = SavesBefore(pworld);
                }
                break;
            }
            case 2u:
            case 3u: {
                bretval = false;
                break;
            }
            case 6u: {
                TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
                TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(pForm);
                if (pcell) {
                    bretval = SavesBefore(pcell);
                }
                break;
            }
            case 7u: {
                bretval = true;
                break;
            }
            default: {
                break;
            }
        }
    }

    return bretval;
}

TESForm* TESWorldSpace::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    TESForm* pDuplicate = TESForm::CreateDuplicateForm(abCreateEditorID, apCopyMap);
    TESWorldSpace* pdupworld = dynamic_cast<TESWorldSpace*>(pDuplicate);
    pdupworld->pCellMap->RemoveAll();
    NiTMapItem<int32_t, TESObjectCELL*>* it = pCellMap->GetFirstPos();
    while (it) {
        int32_t key;
        TESObjectCELL* pcell = nullptr;
        pCellMap->GetNext(it, key, pcell);
        if (pcell) {
            TESForm* pForm = pcell->CreateDuplicateForm(false, apCopyMap);
            TESObjectCELL* pdupcell = dynamic_cast<TESObjectCELL*>(pForm);
            if (pdupcell) {
                pdupcell->SetAltered(true);
                pdupworld->AddCell(pdupcell);
            }
        }
    }
    TESObjectCELL* pcell = pPersistentCell;
    if (pcell) {
        TESForm* pForm = pcell->CreateDuplicateForm(false, apCopyMap);
        TESObjectCELL* pdupcell = dynamic_cast<TESObjectCELL*>(pForm);
        if (pdupcell) {
            pdupworld->pPersistentCell = pdupcell;
            pdupcell->SetAltered(true);
        }
    }
    pdupworld->AddPersistentReferences();
    return pdupworld;
}

bool TESWorldSpace::FindInFileFast(TESFile* apFile) {
    OFFSET_DATA* pOffsetData = GetOffsetData(apFile);
    if (!apFile ||
        !apFile->GetMaster() ||
        !pOffsetData ||
        !pOffsetData->iFileOffset ||
        !apFile->SetOffset(pOffsetData->iFileOffset)) {
        return false;
    }
    if (apFile->GetTESForm() != formEnumString[WRLD_ID].cFormID) {
        return false;
    }
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
                    Warning(
                        "MASTERFILE: Unable to find climate (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find climate (%08X) on owner worldspace (%08X).",
                        iFormID,
                        GetFormID()
                    );
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
                    Warning(
                        "MASTERFILE: Unable to find imagespace (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find imagespace (%08X) on owner worldspace (%08X).",
                        iFormID,
                        GetFormID()
                    );
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
                    Warning(
                        "Unable to find encounter zone (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
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
                    Warning(
                        "MASTERFILE: Unable to find water type (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find water type (%08X) on owner worldspace (%08X).",
                        iFormID,
                        GetFormID()
                    );
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
                    Warning(
                        "MASTERFILE: Unable to find lod water type (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find lod water type (%08X) on owner worldspace (%08X).",
                        iFormID,
                        GetFormID()
                    );
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
                    Warning(
                        "MASTERFILE: Unable to find music type (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find music type (%08X) on owner worldspace (%08X).",
                        iFormID,
                        GetFormID()
                    );
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
                    Warning(
                        "MASTERFILE: Unable to find landscape world (%08X) on owner worldspace \"%s\".",
                        iFormID,
                        GetFormEditorID()
                    );
                }
                else {
                    Warning(
                        "MASTERFILE: Unable to find landscape world (%08X) on owner worldspace (%08X).",
                        iFormID,
                        GetFormID()
                    );
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
    pTerrainManager = new BGSTerrainManager(this);
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
                pImpactSwap = new ImpactSwap;
            }
            pImpactSwap->Copy(pCopy->pImpactSwap);
        }
        else if (pImpactSwap) {
            delete pImpactSwap;
            pImpactSwap = nullptr;
        }
        pWorldWater = pCopy->GetWaterType();
        pLODWater = pCopy->GetLODWaterType();
        SetWaterLODHeight(pCopy->fLODWaterHeight);
        fDefaultLandHeight = pCopy->fDefaultLandHeight;
        fDefaultWaterHeight = pCopy->fDefaultWaterHeight;
        ourMemcpy(&WorldMapData, &pCopy->WorldMapData, sizeof(WorldMapData));
        ourMemcpy(&WorldMapOffsetData, &pCopy->WorldMapOffsetData, sizeof(WorldMapOffsetData));
        pMusicType = pCopy->GetMusicType();
        pCellMap->RemoveAll();
        NiTMapItem<int32_t, TESObjectCELL*>* it = pCopy->pCellMap->GetFirstPos();
        while (it) {
            int32_t key;
            TESObjectCELL* pCell = nullptr;
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
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (cFlags != pComp->cFlags) {
        return true;
    }
    if (sParentUseFlags != pComp->sParentUseFlags) {
        return true;
    }
    if (pParentWorld != pComp->pParentWorld) {
        return true;
    }
    if (pEncounterZone != pComp->pEncounterZone) {
        return true;
    }
    TESClimate* pThisClimate = GetClimate();
    if (pThisClimate != pComp->GetClimate()) {
        return true;
    }
    if (pImageSpace != pComp->pImageSpace) {
        return true;
    }
    if (pImpactSwap && pComp->pImpactSwap) {
        if (pImpactSwap->Compare(pComp->pImpactSwap)) {
            return true;
        }
    }
    else if (pImpactSwap || pComp->pImpactSwap) {
        return true;
    }
    TESWaterForm* pThisWater = GetWaterType();
    if (pThisWater != pComp->GetWaterType()) {
        return true;
    }
    TESWaterForm* pThisLODWater = GetLODWaterType();
    if (pThisLODWater != pComp->GetLODWaterType()) {
        return true;
    }
    if (fLODWaterHeight != pComp->fLODWaterHeight) {
        return true;
    }
    if (pComp->fDefaultLandHeight != fDefaultLandHeight || pComp->fDefaultWaterHeight != fDefaultWaterHeight) {
        return true;
    }
    if (std::memcmp(&WorldMapData, &pComp->WorldMapData, sizeof(WorldMapData))) {
        return true;
    }
    if (std::memcmp(&WorldMapOffsetData, &pComp->WorldMapOffsetData, sizeof(WorldMapOffsetData))) {
        return true;
    }
    BGSMusicType* pThisMusicType = GetMusicType();
    return pThisMusicType != pComp->GetMusicType() ||
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
    TESRegionDataMap* pdata = nullptr;
    if (!pDataHandler) {
        return false;
    }
    if (plastWorldSpace == this && lastWorldLoc == aWorldLoc) {
        aMapName.Set(lastMapName.pString, 0);
        return false;
    }
    if (plastWorldSpace != this) {
        lastMapName.Set("", 0);
    }
    plastWorldSpace = this;
    lastWorldLoc = aWorldLoc;
    aMapName.Set("", 0);
    TESObjectCELL* pcell = pDataHandler->GetCellFromWorldCoord(aWorldLoc.x, aWorldLoc.y, this, false);
    if (pcell) {
        if (pcell->GetRegionList(true)) {
            TESRegionDataManager* pmanager = pDataHandler->GetRegionDataManager();
            TESRegionData* pderived = pcell->GetRegionList(true)->GetDerivedData(REGION_DATA_MAP_ID, aWorldLoc, this);
            pdata = pmanager->IsRegionDataMap(pderived);
        }
        else {
            pdata = nullptr;
        }
        if (pdata) {
            pdata->GetMapName(&aMapName);
        }
        else {
            aMapName.Set(pcell->GetFullName(), 0);
        }
    }
    else {
        // No cell: the map name of the region at the position with the highest priority, overriding regions first.
        TESRegionList* pregions = pDataHandler->GetRegionList();
        BSSimpleList<TESRegion*>* pcurrent = pregions ? static_cast<BSSimpleList<TESRegion*>*>(pregions) : nullptr;
        BSSimpleList<TESRegionPointList*>* pcurrentRPL = nullptr;
        TESRegionPoint regpt(aWorldLoc);
        int32_t ipriority = -1;
        bool bfound = false;
        bool boverridden = false;
        while (pcurrent && pcurrent->m_item) {
            bfound = false;
            TESRegion* pregion = pcurrent->m_item;
            if (!pregion->GetDelete() &&
                pregion->pWorldSpace == this &&
                pregion->pPointLists &&
                !pregion->pPointLists->IsEmpty()) {
                TESRegionDataManager* pmanager = pDataHandler->GetRegionDataManager();
                pdata = pmanager->IsRegionDataMap(pregion->pDataList->Find(REGION_DATA_MAP_ID));
                if (pdata) {
                    for (pcurrentRPL = pregion->pPointLists; pcurrentRPL && pcurrentRPL->m_item;
                         pcurrentRPL = pcurrentRPL->m_pkNext) {
                        if (pcurrentRPL->m_item->IsInsideRegion(&regpt)) {
                            bfound = true;
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
    if (!aMapName.StrCmp(lastMapName.pString, true)) {
        return false;
    }
    lastMapName.Set(aMapName.pString, 0);
    return true;
}

void TESWorldSpace::GetGrassForLocation(
    NiPoint2 aMinWorldLoc,
    NiPoint2 aMaxWorldLoc,
    TESGrassAreaParam* apGrassParams,
    uint32_t aiNumGrassParams
) {
    TESRegionGrassObject::GenGrass* pgengrass = nullptr;
    TESRegionGrassObject** pgrassArray = nullptr;
    TESRegionGrassObject* pgrassObj = nullptr;
    TESGrass* pgrass = nullptr;
    NiPoint3 plocs[9];
    NiPoint3 center;
    if (!pDataHandler || !aiNumGrassParams) {
        return;
    }
    center.x = (aMinWorldLoc.x + aMaxWorldLoc.x) / 2.0;
    center.y = (aMinWorldLoc.y + aMaxWorldLoc.y) / 2.0;
    center.z = 0.0f;
    float fxdelta = (aMaxWorldLoc.x - aMinWorldLoc.x) / 2.0;
    float fydelta = (aMaxWorldLoc.y - aMinWorldLoc.y) / 2.0;
    TESObjectCELL* pcell = pDataHandler->GetCellFromWorldCoord(center.x, center.y, this, false);
    if (!pcell || !pcell->GetRegionList(true)) {
        return;
    }
    uint32_t itest;
    for (itest = 0; itest < 9; ++itest) {
        plocs[itest].x = static_cast<double>(itest % 3) * fxdelta + aMinWorldLoc.x;
        plocs[itest].y = static_cast<double>(itest / 3) * fydelta + aMinWorldLoc.y;
        plocs[itest].z = 0.0f;
    }
    TESRegionDataManager* pmanager = pDataHandler->GetRegionDataManager();
    TESRegionData* pderived = pcell->GetRegionList(true)->GetDerivedData(REGION_DATA_GRASS_ID, center, this);
    TESRegionDataGrass* pdata = pmanager->IsRegionDataGrass(pderived);
    if (!pdata) {
        return;
    }
    BSSimpleList<TESRegionGrassObject::GenGrass*>* pgenlist;
    pgenlist = new BSSimpleList<TESRegionGrassObject::GenGrass*>;
    // The objects on a land texture first, then the others.
    TESRegionGrassObjectList* pobjects = pdata->GetObjectList();
    BSSimpleList<TESRegionGrassObject*>* pcurrentObj;
    for (pcurrentObj = pobjects ? static_cast<BSSimpleList<TESRegionGrassObject*>*>(pobjects) : nullptr; pcurrentObj;
         pcurrentObj = pcurrentObj->m_pkNext) {
        pgrassObj = pcurrentObj->m_item;
        pgrass = pgrassObj ? static_cast<TESGrass*>(pgrassObj->GetObject()) : nullptr;
        if (pgrassObj &&
            pgrassObj->GetParentTexture() &&
            pgrass &&
            pgrass->GetDensity() &&
            pgrassObj->GetGenerateChance(center, this, false) != 0.0) {
            pgengrass = static_cast<TESRegionGrassObject::GenGrass*>(ourMalloc(sizeof(TESRegionGrassObject::GenGrass)));
            pgengrass->pForm = pgrassObj;
            pgengrass->fdensity = pgrass->GetDensity() / 100.0;
            pgenlist->AddHead(pgengrass);
        }
    }
    pobjects = pdata->GetObjectList();
    for (pcurrentObj = pobjects ? static_cast<BSSimpleList<TESRegionGrassObject*>*>(pobjects) : nullptr; pcurrentObj;
         pcurrentObj = pcurrentObj->m_pkNext) {
        pgrassObj = pcurrentObj->m_item;
        pgrass = pgrassObj ? static_cast<TESGrass*>(pgrassObj->GetObject()) : nullptr;
        if (pgrassObj &&
            !pgrassObj->GetParentTexture() &&
            pgrass &&
            pgrass->GetDensity() &&
            pgrassObj->GetGenerateChance(center, this, false) != 0.0) {
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
    pgrassArray = static_cast<TESRegionGrassObject**>(ourMalloc(
        static_cast<uint64_t>(aiNumGrassParams) * sizeof(TESRegionGrassObject*) > UINT32_MAX
            ? UINT32_MAX
            : static_cast<uint32_t>(aiNumGrassParams * sizeof(TESRegionGrassObject*))
    ));
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
        rparam.pcGeomFilename = nullptr;
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
                apGrassParams[iparam].pfDensity[itest] = pgrassArray[iparam]
                                                             ->GetGenerateChance(plocs[itest], this, true);
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
    TESObjectCELL* pcell = nullptr;
    // aiKey is a uint32_t, but the map's key is an int.
    if (pCellMap->GetAt(static_cast<int32_t>(aiKey), pcell)) {
        return pcell;
    }

    return nullptr;
}

// The parent is looked up twice. The function calls itself for the parent; disable_tail_calls keeps
// clang from turning that recursion into a loop.
[[clang::disable_tail_calls]] WorldSpaceMapData* TESWorldSpace::GetWorldMapData() {
    if (GetParentWorld(PU_MAP)) {
        return GetParentWorld(PU_MAP)->GetWorldMapData();
    }
    return &WorldMapData;
}

const char* TESWorldSpace::GetWorldMapTextureName() {
    if (GetParentWorld(PU_MAP)) {
        return GetParentWorld(PU_MAP)->GetTextureName();
    }
    return GetTextureName();
}

void TESWorldSpace::AdjustMapMarkerCoord(NiPoint3& aMapCoord, bool abInverse) {
    if (abInverse) {
        aMapCoord.x = aMapCoord.x - WorldMapOffsetData.fCellOffsetX;
        aMapCoord.y = aMapCoord.y - WorldMapOffsetData.fCellOffsetY;
    }
    if (WorldMapOffsetData.fMapScale != 1.0 && WorldMapOffsetData.fMapScale != 0.0) {
        NiPoint3 kCenter;
        kCenter.x = MinimumCoords.X + MaximumCoords.X;
        kCenter.y = MinimumCoords.Y + MaximumCoords.Y;
        kCenter.z = 0.0f;
        kCenter *= 0.5f;
        NiPoint3 kOffset = aMapCoord - kCenter;
        double dScale = abInverse ? 1.0 / WorldMapOffsetData.fMapScale : WorldMapOffsetData.fMapScale;
        kOffset *= static_cast<float>(dScale);
        aMapCoord = kCenter + kOffset;
    }
    if (!abInverse) {
        aMapCoord.x = WorldMapOffsetData.fCellOffsetX + aMapCoord.x;
        aMapCoord.y = WorldMapOffsetData.fCellOffsetY + aMapCoord.y;
    }
}

bool TESWorldSpace::GetSmallWorld() const {
    return (cFlags & kWorldFlag_SmallWorld) != 0;
}

void TESWorldSpace::SetSmallWorld(bool abVal) {
    if (abVal) {
        cFlags |= kWorldFlag_SmallWorld;
    }
    else {
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

// The topmost parent file keys the map.
TESWorldSpace::OFFSET_DATA* TESWorldSpace::CreateOffsetData(TESFile* apFile) {
    OFFSET_DATA* pData = nullptr;
    TESFile* pFile = apFile;
    while (pFile && pFile->GetThreadSafeParent()) {
        pFile = pFile->GetThreadSafeParent();
    }
    if (OffsetDataMap.GetAt(pFile, pData) && pData) {
        return pData;
    }
    pData = new OFFSET_DATA;
    pData->pCellFileOffsets = nullptr;
    pData->iFileOffset = 0;
    pData->OffsetMinCoords = NiPoint2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    pData->OffsetMaxCoords = NiPoint2(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
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
    return (cFlags & kWorldFlag_CantTravelFrom) != 0;
}

bool TESWorldSpace::GetCantWaitHere() {
    return (TESForm::GetFormFlags() & 524288) != 0;
}

// The offset data is kept for the topmost parent file.
TESWorldSpace::OFFSET_DATA* TESWorldSpace::GetOffsetData(TESFile* apFile) {
    OFFSET_DATA* pOffsetData = nullptr;
    TESFile* pFile = apFile;
    while (pFile && pFile->GetThreadSafeParent()) {
        pFile = pFile->GetThreadSafeParent();
    }
    if (!OffsetDataMap.GetAt(pFile, pOffsetData)) {
        pOffsetData = nullptr;
    }
    return pOffsetData;
}

TESWaterForm* TESWorldSpace::GetWaterType() {
    if (GetParentWorld(TESWorldSpace::PU_WATER)) {
        TESWorldSpace* pParentWorld = GetParentWorld(TESWorldSpace::PU_WATER);
        return pParentWorld->GetWaterType();
    }

    if (pWorldWater) {
        return pWorldWater;
    }

    return pDefaultWater;
}

TESWaterForm* TESWorldSpace::GetLODWaterType() {
    if (GetParentWorld(TESWorldSpace::PU_LOD)) {
        TESWorldSpace* pParentWorld = GetParentWorld(TESWorldSpace::PU_LOD);
        return pParentWorld->GetLODWaterType();
    }

    if (pLODWater) {
        return pLODWater;
    }

    return pDefaultWater;
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
    return static_cast<uint32_t>(aeFlag) > 5 || (static_cast<uint8_t>(1 << aeFlag) & sParentUseFlags) != 0;
}

// The terrain is shared with the parent worldspace when it takes its level of detail from it. The parent is looked up
// twice; disable_tail_calls keeps the recursive call a real call.
[[clang::disable_tail_calls]] BGSTerrainManager* TESWorldSpace::GetTerrainManager() {
    if (GetParentWorld(PU_LOD) != nullptr) {
        return GetParentWorld(PU_LOD)->GetTerrainManager();
    }
    return pTerrainManager;
}

void TESWorldSpace::AddMultiBoundRef(TESObjectREFR* apRef) {
    if (apRef && apRef->IsMultiBoundRef()) {
        NiPoint3 ext = apRef->GetMultiBoundHalfExtent();
        float frad = ext.Length();
        float fminX = apRef->GetLocationOnReference().x - frad;
        int32_t iminx = FloatToInt(fminX) >> 12;
        float fminY = apRef->GetLocationOnReference().y - frad;
        int32_t iminy = FloatToInt(fminY) >> 12;
        float fmaxX = apRef->GetLocationOnReference().x + frad;
        int32_t imaxx = FloatToInt(fmaxX) >> 12;
        float fmaxY = apRef->GetLocationOnReference().y + frad;
        int32_t imaxy = FloatToInt(fmaxY) >> 12;
        int32_t iparentx = FloatToInt(apRef->GetLocationOnReference().x) >> 12;
        int32_t iparenty = FloatToInt(apRef->GetLocationOnReference().y) >> 12;
        for (int32_t ix = iminx; ix <= imaxx; ++ix) {
            for (int32_t iy = iminy; iy <= imaxy; ++iy) {
                if ((ix != iparentx || iy != iparenty) &&
                    MultiBoundMarkerData::MultiBoundIntersectsCell(apRef, ix, iy)) {
                    if (!pOverlappedMultiboundMap) {
                        pOverlappedMultiboundMap = ::new NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>(37u);
                    }
                    uint32_t ikey = TESObjectCELL::GetCoord(static_cast<uint16_t>(ix), static_cast<uint16_t>(iy));
                    BSSimpleList<TESObjectREFR*>* preflist = nullptr;
                    pOverlappedMultiboundMap->GetAt(ikey, preflist);
                    if (!preflist) {
                        preflist = new BSSimpleList<TESObjectREFR*>;
                        pOverlappedMultiboundMap->SetAt(ikey, preflist);
                    }
                    preflist->AddHead(apRef);
                }
            }
        }
    }
}

TESObjectCELL* TESWorldSpace::GetPersistentCell() {
    if (!pPersistentCell) {
        pPersistentCell = new TESObjectCELL;
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
    BSSimpleList<TESObjectREFR*>* pretlist;
    if (!GetParentWorld(TESWorldSpace::PU_MAP) || abLocalOnly) {
        pretlist = new BSSimpleList<TESObjectREFR*>;
        AddMapMarkersToList(pretlist);
        if (!abLocalOnly) {
            for (BSSimpleList<TESWorldSpace*>* plist = pDataHandler->GetWorldSpaceList(); plist;
                 plist = plist->m_pkNext) {
                TESWorldSpace* pworld = plist->m_item;
                if (pworld && pworld->GetParentWorld(TESWorldSpace::PU_MAP) == this) {
                    pworld->AddMapMarkersToList(pretlist);
                }
            }
        }
    }
    else {
        return GetParentWorld(TESWorldSpace::PU_MAP)->BuildMapMarkerList(false);
    }
    return pretlist;
}

BSSimpleList<TESObjectREFR*>* TESWorldSpace::BuildAudioMarkerList(bool abLocalOnly) {
    BSSimpleList<TESObjectREFR*>* pretlist;
    if (!GetParentWorld(TESWorldSpace::PU_MAP) || abLocalOnly) {
        pretlist = new BSSimpleList<TESObjectREFR*>;
        AddAudioMarkersToList(pretlist);
        if (!abLocalOnly) {
            for (BSSimpleList<TESWorldSpace*>* plist = pDataHandler->GetWorldSpaceList(); plist;
                 plist = plist->m_pkNext) {
                TESWorldSpace* pworld = plist->m_item;
                if (pworld && pworld->GetParentWorld(TESWorldSpace::PU_MAP) == this) {
                    pworld->AddAudioMarkersToList(pretlist);
                }
            }
        }
    }
    else {
        return GetParentWorld(TESWorldSpace::PU_MAP)->BuildAudioMarkerList(false);
    }
    return pretlist;
}

TESObjectCELL* TESWorldSpace::LoadCell(int32_t aiX, int32_t aiY) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_CELLS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESWorldSpace.cpp",
        1942
    );
    bool bsuccess = true;
    TESObjectCELL* pretcell = GetCellFromCellCoord(aiX, aiY);
    pLastLoadedWorld = this;
    if (!pretcell || !pretcell->GetHasTempData()) {
        uint32_t ifilecount = GetFileCount();
        for (uint32_t i = 0; i < ifilecount; ++i) {
            TESFile* pfile = GetFile(i)->GetThreadSafeFile();
            if (pfile && FindCellInFile(pfile, aiX, aiY)) {
                if (pretcell) {
                    if ((pfile->GetFormData()->flags & TESForm::kFormFlag_PartialForm) == 0) {
                        TESDataHandler::LoadForm(pretcell, pfile);
                    }
                }
                else {
                    pretcell = new TESObjectCELL;
                    pretcell->SetInterior(false);
                    pretcell->CreateCellData();
                    pretcell->SetDataCoord(aiX, aiY);
                    AddCell(pretcell);
                    uint32_t icellID = pSaveLoadGameOLD->GetCreatedExteriorCellFormID(iFormID, aiX, aiY);
                    if (icellID) {
                        pretcell->SetFormID(icellID, true);
                    }
                    bsuccess = TESDataHandler::LoadForm(pretcell, pfile);
                }
                if (!pretcell->LoadTempDataFromFile(pfile)) {
                    bsuccess = false;
                }
            }
        }
        if (pretcell) {
            pretcell->SetHasTempData(true);
            bool bLoading = pSaveLoadGameOLD->GetLoading();
            pSaveLoadGameOLD->SetLoading(!bLoading);
            pretcell->InitItem();
            pSaveLoadGameOLD->SetLoading(bLoading);
            AssignPersistentRefsToCell(pretcell);
        }
    }
    if (!bsuccess) {
        int32_t iWorldFormID = iFormID;
        Warning(
            "CELLS: Failed to load temporary data for cell (%i, %i) in worldspace '%s' (%08X).",
            aiX,
            aiY,
            GetFormEditorID(),
            iWorldFormID
        );
    }
    return pretcell;
}

TESObjectCELL* TESWorldSpace::GetCellByEditorID(const char* apEditorID) {
    NiTMapItem<int32_t, TESObjectCELL*>* it;
    TESObjectCELL* pFoundCell = nullptr;
    if (apEditorID) {
        it = pCellMap->GetFirstPos();
        while (it && !pFoundCell) {
            int32_t ignore;
            TESObjectCELL* pCell = nullptr;
            pCellMap->GetNext(it, ignore, pCell);
            if (pCell) {
                if (!BSstricmp(pCell->GetFormEditorID(), apEditorID)) {
                    pFoundCell = pCell;
                }
            }
        }
    }
    return pFoundCell;
}

void TESWorldSpace::EnumReferencesCloseToPoint(
    const NiPoint3& aPoint1,
    float afRadius1,
    const NiPoint3& aPoint2,
    float afRadius2,
    bool (*apfnEnumFunc)(TESObjectREFR*, void*),
    void* aiEnumData
) {
    if (apfnEnumFunc) {
        int32_t iFound = 1;
        int32_t iSide = 0;
        int32_t iX = static_cast<int32_t>(aPoint1.x) >> 12;
        int32_t iY = static_cast<int32_t>(aPoint1.y) >> 12;
        TESObjectCELL* pCenterCell = GetCellFromCellCoord(iX, iY);
        if (!pCenterCell ||
            pCenterCell->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc, aiEnumData)) {
            while (iFound) {
                --iX;
                --iY;
                iSide += 2;
                iFound = 0;
                for (int32_t i = 0; i < iSide; ++i) {
                    TESObjectCELL* pCell = GetCellFromCellCoord(iX, iY);
                    if (pCell &&
                        (afRadius1 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius1) > pCell->GetDistanceFromPoint(aPoint1)) &&
                        (afRadius2 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius2) > pCell->GetDistanceFromPoint(aPoint2))) {
                        ++iFound;
                        if (!pCell->EnumReferencesCloseToPoint(
                                aPoint1,
                                afRadius1,
                                aPoint2,
                                afRadius2,
                                apfnEnumFunc,
                                aiEnumData
                            )) {
                            return;
                        }
                    }

                    ++iX;
                }
                for (int32_t i = 0; i < iSide; ++i) {
                    TESObjectCELL* pCell = GetCellFromCellCoord(iX, iY);
                    if (pCell &&
                        (afRadius1 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius1) > pCell->GetDistanceFromPoint(aPoint1)) &&
                        (afRadius2 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius2) > pCell->GetDistanceFromPoint(aPoint2))) {
                        ++iFound;
                        if (!pCell->EnumReferencesCloseToPoint(
                                aPoint1,
                                afRadius1,
                                aPoint2,
                                afRadius2,
                                apfnEnumFunc,
                                aiEnumData
                            )) {
                            return;
                        }
                    }

                    ++iY;
                }
                for (int32_t i = 0; i < iSide; ++i) {
                    TESObjectCELL* pCell = GetCellFromCellCoord(iX, iY);
                    if (pCell &&
                        (afRadius1 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius1) > pCell->GetDistanceFromPoint(aPoint1)) &&
                        (afRadius2 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius2) > pCell->GetDistanceFromPoint(aPoint2))) {
                        ++iFound;
                        if (!pCell->EnumReferencesCloseToPoint(
                                aPoint1,
                                afRadius1,
                                aPoint2,
                                afRadius2,
                                apfnEnumFunc,
                                aiEnumData
                            )) {
                            return;
                        }
                    }

                    --iX;
                }
                for (int32_t i = 0; i < iSide; ++i) {
                    TESObjectCELL* pCell = GetCellFromCellCoord(iX, iY);
                    if (pCell &&
                        (afRadius1 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius1) > pCell->GetDistanceFromPoint(aPoint1)) &&
                        (afRadius2 == 3.4028234663852886e+38 ||
                         static_cast<double>(afRadius2) > pCell->GetDistanceFromPoint(aPoint2))) {
                        ++iFound;
                        if (!pCell->EnumReferencesCloseToPoint(
                                aPoint1,
                                afRadius1,
                                aPoint2,
                                afRadius2,
                                apfnEnumFunc,
                                aiEnumData
                            )) {
                            return;
                        }
                    }

                    --iY;
                }
            }
            TESObjectCELL* pPersistent = QPersistentCell();
            if (pPersistent) {
                pPersistent
                    ->EnumReferencesCloseToPoint(aPoint1, afRadius1, aPoint2, afRadius2, apfnEnumFunc, aiEnumData);
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
    PersistentDataCritSection.Enter(nullptr);
    if (!IsFixedRef(apRef)) {
        MobilePersistentRefList.Remove(apRef);
    }
    else {
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
    PersistentDataCritSection.Enter(nullptr);
    if (!IsFixedRef(apRef)) {
        MobilePersistentRefList.AddHead(apRef);
    }
    else {
        uint32_t iKey = GetKeyForWorldCoord(apRef->GetLocationOnReference());
        BSSimpleList<TESObjectREFR*>* pList = nullptr;
        FixedPersistentRefMap.GetAt(iKey, pList);
        if (!pList) {
            BSSimpleList<TESObjectREFR*>* pNew = new BSSimpleList<TESObjectREFR*>;
            pList = pNew;
            FixedPersistentRefMap.SetAt(iKey, pList);
        }
        pList->AddHead(apRef);
    }
    PersistentDataCritSection.Leave();
}

bool TESWorldSpace::FindCellInFile(TESFile* apFile, int32_t aiX, int32_t aiY) {
    TESWorldSpace::OFFSET_DATA* pdata;
    if (!apFile || !apFile->GetMaster() || (pdata = GetOffsetData(apFile)) == nullptr || !pdata->pCellFileOffsets) {
        bool bretval = false;
        uint32_t iblockkey = TESObjectCELL::CalcExtGroupBlockKey(aiX, aiY);
        uint32_t isubblockkey = TESObjectCELL::CalcExtGroupSubBlockKey(aiX, aiY);
        if (!apFile || !apFile->FindForm(this)) {
            return bretval;
        }
        apFile->NextForm(true);
        bool bquit = false;
        FORM* pFORM = apFile->GetFormData();
        while (true) {
            while (true) {
                if (!pFORM || bquit) {
                    return bretval;
                }
                if (pFORM->form != formEnumString[2].uiFormString) {
                    break;
                }
                bool bskipgroup = true;
                bquit = true;
                switch (pFORM->iFormID) {
                    case 1u: bskipgroup = false; [[fallthrough]];
                    case 4u:
                        if (iblockkey == pFORM->flags) {
                            bskipgroup = false;
                        }
                        [[fallthrough]];
                    case 5u:
                        if (isubblockkey == pFORM->flags) {
                            bskipgroup = false;
                        }
                        [[fallthrough]];
                    case 6u:
                    case 8u:
                    case 9u: {
                        bquit = false;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                if (!bquit) {
                    if (bskipgroup) {
                        apFile->NextGroup();
                    }
                    else {
                        apFile->NextForm(true);
                    }
                }
            }
            if (pFORM->form == formEnumString[57].uiFormString) {
                EXTERIOR_DATA coorddata;
                ourMemset(&coorddata, 0, sizeof(coorddata));
                if ((pFORM->flags & TESForm::kFormFlags_IsRefPersistent) != 0) {
                    coorddata.iCellX = 0x7FFFFFFF;
                }
                else {
                    bool bmoveon = false;
                    CHUNK_ID chunk = apFile->GetTESChunk();
                    while (chunk && !bmoveon) {
                        if (chunk == XCLC_ID) {
                            apFile->GetChunkData(&coorddata, 12u);
                            if (apFile->QEndian()) {
                                coorddata.Endian();
                            }
                            bmoveon = true;
                        }
                        if (apFile->NextChunk()) {
                            chunk = apFile->GetTESChunk();
                        }
                        else {
                            chunk = NO_CHUNK;
                        }
                    }
                }
                if (coorddata.iCellX == aiX && coorddata.iCellY == aiY) {
                    bretval = true;
                    bquit = true;
                    apFile->TESRewindChunk();
                    continue;
                }
            }
            else if (pFORM->form != formEnumString[68].uiFormString) {
                bquit = true;
                continue;
            }
            apFile->NextForm(true);
        }
    }
    int32_t iindex = GetIndexForCellCoord(apFile, aiX, aiY);
    if (iindex == 0xFFFFFFFFu) {
        return false;
    }
    uint32_t ioffset = pdata->pCellFileOffsets[iindex];
    if (!ioffset) {
        return false;
    }
    apFile->SetOffset(pdata->iFileOffset + ioffset);
    return true;
}

bool TESWorldSpace::FindLandDataInFile(int32_t aiX, int32_t aiY, TESFile*& apOutFile, uint32_t& aiOutOffset) {
    apOutFile = nullptr;
    aiOutOffset = 0;
    if (GetParentWorld(TESWorldSpace::PU_LAND)) {
        return GetParentWorld(TESWorldSpace::PU_LAND)->FindLandDataInFile(aiX, aiY, apOutFile, aiOutOffset);
    }
    uint32_t uiFileCount = GetFileCount();
    if (uiFileCount) {
        for (int32_t iIndex = uiFileCount - 1; iIndex >= 0; --iIndex) {
            TESFile* pFile = GetFile(iIndex)->GetThreadSafeFile();
            if (pFile && FindCellInFile(pFile, aiX, aiY) && TESObjectCELL::FindLandDataInFile(pFile)) {
                apOutFile = pFile;
                aiOutOffset = pFile->GetOffset();
                return true;
            }
        }
    }
    return false;
}

// The found cell's slot is deliberately left uninitialised: GetAt writes it only when it returns
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
    }
    else {
        int32_t iY = apCell->GetDataY();
        int32_t iX = apCell->GetDataX();
        int32_t iKey = TESObjectCELL::GetCoord(iX, iY);
        TESObjectCELL* pExisting;
        if (pCellMap->GetAt(iKey, pExisting)) {
            int32_t iExistingY = apCell->GetDataY();
            int32_t iExistingX = apCell->GetDataX();
            const char* pEditorID = pExisting->GetFormEditorID();
            uint32_t uiFormID = pExisting->GetFormID();
            Warning(
                "CELLS: Cell (%08X) %s already exists at coord (%i, %i ).",
                uiFormID,
                pEditorID,
                iExistingX,
                iExistingY
            );
            return false;
        }
        pCellMap->SetAt(iKey, apCell);
    }
    apCell->SetWorldSpace(this);
    return true;
}

// The out-of-range warning names the int16 bounds; the coordinates are tested signed.
TESObjectCELL* TESWorldSpace::GetCellFromCellCoord(int32_t aiX, int32_t aiY) {
    if (aiX > std::numeric_limits<int16_t>::max() ||
        aiY > std::numeric_limits<int16_t>::max() ||
        aiX < std::numeric_limits<int16_t>::min() ||
        aiY < std::numeric_limits<int16_t>::min()) {
        Warning(
            "CELLS: Trying to get exterior cell for invalid cell coordinate. Values must be between %i and %i.",
            std::numeric_limits<int16_t>::min(),
            std::numeric_limits<int16_t>::max()
        );
        return nullptr;
    }
    // Both coordinates are passed zero-extended as 16-bit values.
    int32_t iKey = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiX), static_cast<uint16_t>(aiY));
    TESObjectCELL* pCell = nullptr;
    if (pCellMap->GetAt(iKey, pCell)) {
        return pCell;
    }
    return nullptr;
}

TESObjectCELL* TESWorldSpace::GetCellFromWorldCoord(const NiPoint3& aCoord) {
    int32_t iX = FloatToInt(aCoord.x) >> 12;
    int32_t iY = FloatToInt(aCoord.y) >> 12;
    return GetCellFromCellCoord(iX, iY);
}

void TESWorldSpace::AdjustCellOffsets(TESFile* apFile, int32_t aiDelta) {
    TESWorldSpace::OFFSET_DATA* pdata = GetOffsetData(apFile);
    if (pdata && pdata->pCellFileOffsets && aiDelta) {
        int32_t iYMin = static_cast<int32_t>(pdata->OffsetMinCoords.y) >> 12;
        int32_t iXMax = static_cast<int32_t>(pdata->OffsetMaxCoords.x) >> 12;
        int32_t iYMax = static_cast<int32_t>(pdata->OffsetMaxCoords.y) >> 12;
        for (int32_t ix = static_cast<int32_t>(pdata->OffsetMinCoords.x) >> 12; ix <= iXMax; ++ix) {
            for (int32_t iy = iYMin; iy <= iYMax; ++iy) {
                int32_t iindex = GetIndexForCellCoord(apFile, ix, iy);
                if (iindex >= 0 && pdata->pCellFileOffsets[iindex]) {
                    pdata->pCellFileOffsets[iindex] += aiDelta;
                }
            }
        }
    }
}

TESClimate* TESWorldSpace::GetClimate() {
    if (!GetParentWorld(TESWorldSpace::PU_CLIMATE)) {
        return pClimate;
    }
    return GetParentWorld(TESWorldSpace::PU_CLIMATE)->GetClimate();
}

TESImageSpace* TESWorldSpace::GetImageSpace() {
    TESImageSpace* pImageSpace = this->pImageSpace;
    if (!pImageSpace && GetParentWorld(TESWorldSpace::PU_UNUSED)) {
        return GetParentWorld(TESWorldSpace::PU_UNUSED)->GetImageSpace();
    }

    return pImageSpace;
}

BGSMusicType* TESWorldSpace::GetMusicType() {
    return pMusicType;
}

BSPortalGraph* TESWorldSpace::GetPortalGraph() {
    if (!spPortalGraph.m_pObject) {
        spPortalGraph = new BSPortalGraph;
    }
    return spPortalGraph.m_pObject;
}

void TESWorldSpace::ClearPortalGraph() {
    if (spPortalGraph.m_pObject) {
        if (spPortalGraph->NiRefObject::GetRefCount() == 1) {
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
            BSPortalGraph* pPortalGraph = pShadowSceneNode->GetPortalGraph();
            if (pPortalGraph == spPortalGraph.m_pObject) {
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->SetPortalGraph(nullptr);
            }
            spPortalGraph = nullptr;
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
    BSSimpleList<TESObjectREFR*>* preflist = nullptr;
    if (pOverlappedMultiboundMap) {
        int32_t iDataY = apCell->GetDataY();
        int32_t iDataX = apCell->GetDataX();
        // The map's key is the cell's packed coordinate, which GetCoord returns signed.
        pOverlappedMultiboundMap->GetAt(static_cast<uint32_t>(TESObjectCELL::GetCoord(iDataX, iDataY)), preflist);
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
    return (cFlags & kWorldFlag_HasBorderRegion) != 0;
}

void TESWorldSpace::SetHasBorderRegion(bool abVal) {
    if (abVal) {
        cFlags |= kWorldFlag_HasBorderRegion;
    }
    else {
        cFlags &= static_cast<uint8_t>(~kWorldFlag_HasBorderRegion);
    }
}

bool TESWorldSpace::GetNoLODWater() {
    return (cFlags & kWorldFlag_NoLODWater) != 0;
}

bool TESWorldSpace::GetNeedsWaterAdjustment() {
    return (cFlags & kWorldFlag_NeedsWaterAdjustment) != 0;
}

bool TESWorldSpace::IsAllowNPCFallDamage() {
    return (cFlags & kWorldFlag_NoNPCFallDamage) == 0;
}

void TESWorldSpace::AddPersistentReferences() {
    TESObjectCELL* pCell = TESWorldSpace::QPersistentCell();
    if (pCell) {
        pCell->AddReferences(this);
    }
}

void TESWorldSpace::AddAudioMarkersToList(BSSimpleList<TESObjectREFR*>* apMarkList) {
    if (apMarkList) {
        TESObjectCELL* pCell = TESWorldSpace::QPersistentCell();
        if (pCell) {
            pCell->AddAudioMarkersToList(apMarkList);
        }
    }
}

void TESWorldSpace::UnLoadCell(TESObjectCELL* apCell) {
    if (apCell && apCell->GetWorldSpace() == this) {
        TESFile* pFile = apCell->GetFile(-1);
        if (pFile && !pFile->GetMaster()) {
            apCell->UnLoadAllTempData();
        }
        else {
            apCell->UnLoadAllTempData();
            pSaveLoadGameOLD->UnloadForm(apCell);
            if (!pDataHandler->IsFormIDCreated(apCell->iFormID)) {
                pSaveLoadGame->UnloadForm(apCell, false);
            }
            ReleaseCell(apCell);
            delete apCell;
        }
    }
}

void TESWorldSpace::ReleaseCell(TESObjectCELL* apCell) {
    TESObjectCELL* pFound;
    if (apCell && !apCell->GetInterior()) {
        int32_t dataY = apCell->GetDataY();
        int32_t dataX = apCell->GetDataX();
        int32_t iKey = TESObjectCELL::GetCoord(dataX, dataY);
        if (pCellMap->GetAt(iKey, pFound) && apCell == pFound) {
            pCellMap->RemoveAt(iKey);
            apCell->SetWorldSpace(nullptr);
        }
    }
}

void TESWorldSpace::AssignPersistentRefsToCell(TESObjectCELL* apCell) {
    if (apCell) {
        TESWorldSpace::PersistentDataCritSection.Enter(nullptr);
        int32_t iDataY = apCell->GetDataY();
        int32_t iDataX = apCell->GetDataX();
        uint32_t ikey = TESObjectCELL::GetCoord(iDataX, iDataY);
        BSSimpleList<TESObjectREFR*>* plist = nullptr;
        FixedPersistentRefMap.GetAt(ikey, plist);
        while (plist && !plist->IsEmpty()) {
            TESObjectREFR* pref = plist->m_item;
            apCell->AddReference(pref, false);
            plist = plist->m_pkNext;
        }
        for (plist = &MobilePersistentRefList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            TESObjectREFR* pref = plist->m_item;
            if (pref->GetParentCell() != apCell) {
                if (apCell->IsPointInCellExterior(pref->GetLocationOnReference())) {
                    apCell->AddReference(pref, false);
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
    if (aiX <= iMaxX && aiX >= iMinX && aiY <= static_cast<int32_t>(pdata->OffsetMaxCoords.y) >> 12 && aiY >= iMinY) {
        return aiX + (aiY - iMinY) * (iMaxX - iMinX + 1) - iMinX;
    }

    return 0xFFFFFFFFu;
}

bool TESWorldSpace::GetExtCellDataFromFileByEditorID(const char* apCellID, int32_t& aiOutX, int32_t& aiOutY) {
    uint32_t ifilecount = GetFileCount();
    for (uint32_t i = 0; i < ifilecount; ++i) {
        TESFile* pfile = GetFile(static_cast<int32_t>(i))->GetThreadSafeFile();
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
                                    return true;
                                }
                                ourFree(pfileID);
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

void TESWorldSpace::ClearCellMap() {
    if (!GetTemporary()) {
        NiTMapItem<int32_t, TESObjectCELL*>* it = pCellMap->GetFirstPos();
        while (it) {
            int32_t ignore;
            TESObjectCELL* pCell = nullptr;
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
            BSSimpleList<TESObjectREFR*>* preflist = nullptr;
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
        pOverlappedMultiboundMap = nullptr;
    }
}

void TESWorldSpace::ClearPersistentRefData() {
    TESWorldSpace::PersistentDataCritSection.Enter(nullptr);
    MobilePersistentRefList.RemoveAll();
    NiTMapItem<uint32_t, BSSimpleList<TESObjectREFR*>*>* iter = FixedPersistentRefMap.GetFirstPos();
    while (iter) {
        uint32_t iignore = 0;
        BSSimpleList<TESObjectREFR*>* preflist = nullptr;
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
        TESObjectCELL* pCell = pPersistentCell;
        if (pCell) {
            pCell->AddMapMarkersToList(apMarkList);
        }
    }
}

void TESWorldSpace::FreeOffsetData() {
    NiTMapItem<TESFile*, TESWorldSpace::OFFSET_DATA*>* iter;
    iter = OffsetDataMap.GetFirstPos();
    while (iter) {
        TESWorldSpace::OFFSET_DATA* pdata = nullptr;
        TESFile* pfile = nullptr;
        OffsetDataMap.GetNext(iter, pfile, pdata);
        ourFree(pdata->pCellFileOffsets);
        ourFree(pdata);
    }
    OffsetDataMap.RemoveAll();
}

bool TESWorldSpace::IsPointInBorderRegion(const NiPoint3& arPoint) {
    if (!GetHasBorderRegion()) {
        return true;
    }
    TESRegionPoint regionPoint(arPoint.x, arPoint.y);
    bool bfoundInRegion = false;
    // The list is walked as TESRegionList objects: each next node is cast down to the list class (null kept null).
    for (TESRegionList* pregionList = pDataHandler->GetRegionList(); pregionList && pregionList->m_item;
         pregionList = static_cast<TESRegionList*>(pregionList->m_pkNext)) {
        TESRegion* pregion = pregionList->m_item;
        if (pregion->TESForm::GetHasSpokenFlag() && !pregion->GetDelete() && pregion->pWorldSpace == this) {
            for (TESRegionPointList*& rpItem : pregion->pPointLists->ItemsUntilNull()) {
                if (rpItem->IsInsideRegion(&regionPoint)) {
                    bfoundInRegion = true;
                }
            }
        }
    }
    return bfoundInRegion;
}

int32_t TESWorldSpace::GetKeyForWorldCoord(const NiPoint3& aCoord) {
    if (!std::isfinite(aCoord.x) ||
        !std::isfinite(aCoord.y) ||
        !std::isfinite(aCoord.z) ||
        std::isnan(aCoord.x) ||
        std::isnan(aCoord.y) ||
        std::isnan(aCoord.z)) {
        return 0x7FFF7FFF;
    }
    int32_t icellx = FloatToInt(aCoord.x) >> 12;
    int32_t icelly = FloatToInt(aCoord.y) >> 12;
    // Both coordinates are passed zero-extended as 16-bit values.
    return TESObjectCELL::GetCoord(static_cast<uint16_t>(icellx), static_cast<uint16_t>(icelly));
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
        case LVLN_ID: return true;
        default: return false;
    }
}

bool TESWorldSpace::IsFormTypeWorldChild(uint8_t aeType) {
    if (aeType == CELL_ID) {
        return true;
    }
    if (aeType == TLOD_ID) {
        return true;
    }
    return TESObjectCELL::IsFormTypeCellChild(aeType);
}

void TESWorldSpace::GetCellCoordForKey(uint32_t aiKey, int16_t& asOutX, int16_t& asOutY) {
    asOutX = static_cast<uint16_t>(aiKey >> 16);
    asOutY = aiKey;
}
