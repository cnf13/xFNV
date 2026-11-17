#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESTerrainLODManager.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSEncounterZone;
class BGSMusicType;
class BGSTerrainManager;
class BSPortalGraph;
class ImpactSwap;
class TESClimate;
class TESGrassAreaParam;
class TESImageSpace;
class TESObjectCELL;
class TESObjectREFR;
class TESWaterForm;

// A pair of floating-point coordinates.
struct WorldSpaceCoordXY {
	float X;
	float Y;
};

// The usable cell range and the cells at the corners of a worldspace's map.
struct WorldSpaceMapData {
	// Swaps the usable size and the corner cells read from a big-endian file (the PDB names this struct
	// WORLD_MAP_DATA).
	void Endian();

	uint32_t iUsableWidth;
	uint32_t iUsableHeight;
	int16_t sNorthWestCellX;
	int16_t sNorthWestCellY;
	int16_t sSouthEastCellX;
	int16_t sSouthEastCellY;
};

// How the world map is scaled and shifted.
struct WorldSpaceMapOffsetData {
	// The PDB's WORLD_MAP_OFFSET_DATA::Endian. swaps the three floats.
	void Endian();

	float fMapScale;
	float fCellOffsetX;
	float fCellOffsetY;
};

// The default land and water heights of a worldspace's cells, as the DNAM chunk stores them.
struct DEFAULT_CELL_DATA {
	// Swaps both floats.
	void Endian();

	float fLandHeight;
	float fWaterHeight;
};

// A worldspace: a named grid of exterior cells with its own water, climate, map and persistent references.
class TESWorldSpace : public TESForm, public TESFullName, public TESTexture {
public:
	// The properties a worldspace may take from its parent worldspace.
	enum PARENT_USE_FLAG : int32_t {
		PU_LAND = 0x0,
		PU_LOD = 0x1,
		PU_MAP = 0x2,
		PU_WATER = 0x3,
		PU_CLIMATE = 0x4,
		PU_UNUSED = 0x5,
		PU_SKYCELL = 0x6,
		PU_COUNT = 0x7,
	};

	// The bits of ucFlags.
	enum WorldFlag : uint8_t {
		kWorldFlag_SmallWorld = 0x01,
		kWorldFlag_NoLODNoise = 0x20,
	};

	// Where one file's cell records of this worldspace start, and the cell range they cover (0x18 bytes).
	struct OFFSET_DATA {
		OFFSET_DATA();

		uint32_t* pCellFileOffsets;
		NiPoint2 OffsetMinCoords;
		NiPoint2 OffsetMaxCoords;
		uint32_t iFileOffset;
	};

	TESWorldSpace();
	~TESWorldSpace() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	bool LoadPartial(TESFile* apFile) override;
	void Save() override;
	bool SavesBefore(TESForm* apForm) override;
	bool SavesBefore(FORM* apGroupFORM) override;
	TESForm* CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) override;
	bool FindInFileFast(TESFile* apFile) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool IsParentForm() override { return true; }
	bool IsFormTypeChild(uint8_t aeType) override;
	const char* GetFormEditorID() override;
	bool SetFormEditorID(const char* apEditorID) override;

	virtual bool GetMapNameForLocation(BSStringT<char>& aMapName, NiPoint3 aWorldLoc);
	virtual void GetGrassForLocation(NiPoint2 aMinWorldLoc, NiPoint2 aMaxWorldLoc, TESGrassAreaParam* apGrassParams,
		uint32_t aiNumGrassParams);

	using TESForm::Save;

	// The loaded exterior cell with cell key aiKey.
	TESObjectCELL* GetCellFromKey(uint32_t aiKey);
	// The map data: the map parent's, else the worldspace's own.
	WorldSpaceMapData* GetWorldMapData();
	// The map texture name: the map parent's, else the worldspace's own.
	const char* GetWorldMapTextureName();
	// Moves a map coordinate into (or, with abInverse, out of) the map's adjusted space.
	void AdjustMapMarkerCoord(NiPoint3& aMapCoord, bool abInverse);
	bool GetSmallWorld() const;
	void SetSmallWorld(bool abVal);
	// Resets the data a worldspace may share with its parent (climate, water, heights, map name and map data).
	void ClearSharableData();
	void SetClimate(TESClimate* apClimate);
	void SetImageSpace(TESImageSpace* apImageSpace);
	void SetEncounterZone(BGSEncounterZone* apZone);
	// The cell offset data of apFile's topmost parent file, created on first use.
	OFFSET_DATA* CreateOffsetData(TESFile* apFile);
	void SetDefaultWaterHeight(float afVal);
	bool GetNoLODNoise() const;

	TESWorldSpace* GetParentWorld(PARENT_USE_FLAG aeFlags);
	bool CantTravelFrom();
	bool GetCantWaitHere();
	OFFSET_DATA* GetOffsetData(TESFile* apFile);
	TESWaterForm* GetWaterType();
	TESWaterForm* GetLODWaterType();
	// The parent worldspace, pParentWorld (thiscall, ret 0, 0x4FD380; a shared one-field
	// getter, the same function as MobileObject::GetDialogueItemTarget).
	TESWorldSpace* GetParentWorldSimple();
	void SetParentWorld(TESWorldSpace* apWorld);
	// pPersistentCell, without creating it. This is the PDB's TESWorldSpace::GetPersistentCell; src's GetPersistentCell
	// (0x588070) is the creating one, so the folded getter takes this name.
	TESObjectCELL* QPersistentCell() { return pPersistentCell; }
	// true when the worldspace takes the property from its parent; the engine answers true for an invalid flag
	bool GetParentUseFlag(PARENT_USE_FLAG aeFlag);
	BGSTerrainManager* GetTerrainManager();
	// The canopy shadow texture path, "" for none.
	const char* GetCanopyShadowTextureName();
	// Files a multibound reference with the world space.
	void AddMultiBoundRef(TESObjectREFR* apRef);
	ImpactSwap* QImpactSwap() { return pImpactSwap; }
	// The LOD water height (thiscall, ST0 float, 0x45CD80; a shared one-field getter).
	float GetLODWaterHeight();

	TESObjectCELL* GetPersistentCell();
	void AddPersistentRef(TESObjectREFR* apRef);
	void RemovePersistentRef(TESObjectREFR* apRef);
	// The persistent cell's teleport doors, added to arOutArray.
	void BuildTeleportDoorList(BSSimpleArray<TESObjectREFR*>& arOutArray);
	// A new list of the worldspace's map marker references.
	BSSimpleList<TESObjectREFR*>* BuildMapMarkerList(bool abLocalOnly);
	// A new list of the worldspace's audio marker references.
	BSSimpleList<TESObjectREFR*>* BuildAudioMarkerList(bool abLocalOnly);
	// Loads (or finds) the exterior cell at aiX, aiY.
	TESObjectCELL* LoadCell(int32_t aiX, int32_t aiY);
	// The cell of this worldspace with that editor ID, or null. thiscall, ret 4.
	TESObjectCELL* GetCellByEditorID(const char* apEditorID);
	// Calls apfnEnumFunc for the references near both points. thiscall, ret 0x18.
	void EnumReferencesCloseToPoint(const NiPoint3& aPoint1, float afRadius1, const NiPoint3& aPoint2, float afRadius2,
		bool (*apfnEnumFunc)(TESObjectREFR*, void*), void* aiEnumData);

	void SetWorldWater(TESWaterForm* apWater);
	void SetLODWater(TESWaterForm* apWater);
	void SetWaterLODHeight(float afHeight);
	void SetDefaultLandHeight(float afVal);
	void SetMusicType(BGSMusicType* apType);
	BGSEncounterZone* GetEncounterZone() const;
	const char* GetWaterNoiseTexturePath() const;
	// Drops apRef from the persistent reference data: the mobile list, or the fixed list of its cell (freed and
	// unmapped once empty).
	void RemoveFromPersistentRefData(TESObjectREFR* apRef);
	// Files apRef in the persistent reference data.
	void AddToPersistentRefData(TESObjectREFR* apRef);
	// Seeks apFile to the cell at (aiX, aiY) of this world space.
	bool FindCellInFile(TESFile* apFile, int32_t aiX, int32_t aiY);
	bool FindLandDataInFile(int32_t aiX, int32_t aiY, TESFile*& apOutFile, uint32_t& aiOutOffset);
	// Files apCell in the cell map (an interior cell never; the persistent cell once); false when its coordinates
	// are taken.
	bool AddCell(TESObjectCELL* apCell);
	// The cell at cell coordinates (aiX, aiY), or null.
	TESObjectCELL* GetCellFromCellCoord(int32_t aiX, int32_t aiY);
	// The cell holding world position aCoord, or null.
	TESObjectCELL* GetCellFromWorldCoord(const NiPoint3& aCoord);
	// Shifts the cell offsets apFile recorded for this worldspace by aiDelta. thiscall, ret 8.
	void AdjustCellOffsets(TESFile* apFile, int32_t aiDelta);
	// The worldspace's climate, or its parent's. thiscall, ret 0.
	TESClimate* GetClimate();
	// The worldspace's image space, or its parent's. thiscall, ret 0.
	TESImageSpace* GetImageSpace();
	// The worldspace's music type, or its parent's. thiscall, ret 0.
	BGSMusicType* GetMusicType();
	// The worldspace's portal graph. thiscall, ret 0.
	BSPortalGraph* GetPortalGraph();
	void ClearPortalGraph();
	BSPortalGraph* QPortalGraph();
	// The worldspace's default water height (x87 return). thiscall, ret 0.
	float GetDefaultWaterHeight();
	// The multibound references of other cells that overlap apCell. thiscall, ret 4.
	BSSimpleList<TESObjectREFR*>* GetOverlappedMultiBoundRefs(TESObjectCELL* apCell);
	// Inferred type: return: float from 0x9cc510 (passed to CombatFormulas::GetRandomBetween 0)
	float GetMinX() const;
	float GetDefaultLandHeight();
	bool GetHasBorderRegion();
	void SetHasBorderRegion(bool abVal);
	bool GetNoLODWater();
	bool GetNeedsWaterAdjustment();
	bool IsAllowNPCFallDamage();
	void AddPersistentReferences();
	void AddAudioMarkersToList(BSSimpleList<TESObjectREFR*>* apMarkList);
	void UnLoadCell(TESObjectCELL* apCell);
	void ReleaseCell(TESObjectCELL* apCell);
	void AssignPersistentRefsToCell(TESObjectCELL* apCell);
	int32_t GetIndexForCellCoord(TESFile* apFile, int32_t aiX, int32_t aiY);
	bool GetExtCellDataFromFileByEditorID(const char* apCellID, int32_t& aiOutX, int32_t& aiOutY);
	void ClearCellMap();
	// Empties and deletes the overlapped-multibound map and its reference lists.
	void ClearMultiBoundRefs();
	void ClearPersistentRefData();
	void AddMapMarkersToList(BSSimpleList<TESObjectREFR*>* apMarkList);
	void FreeOffsetData();
	bool IsPointInBorderRegion(const NiPoint3& arPoint);

	// The cell map key of the cell holding aCoord; 0x7FFF7FFF for a non-finite position.
	static int32_t GetKeyForWorldCoord(const NiPoint3& aCoord);
	static bool IsFixedRef(TESObjectREFR* apRef);
	// Whether a form of type aeType lives in a world space: cells, terrain LOD and the cell children. Called out of
	// line.
	static bool IsFormTypeWorldChild(uint8_t aeType);
	static void GetCellCoordForKey(uint32_t aiKey, int16_t& asOutX, int16_t& asOutY);

	// the cell index -> cell map
	NiTPointerMap<int32_t, TESObjectCELL*>* pCellMap;
	TESObjectCELL* pPersistentCell;
	// The exe runs TESTerrainLODManager's constructor (0x57CD10) and destructor (0x57CE90) on it; the class is empty.
	TESTerrainLODManager TerrainLODManager;
	uint8_t padding39[3];
	BGSTerrainManager* pTerrainManager;
	TESClimate* pClimate;
	TESImageSpace* pImageSpace;
	ImpactSwap* pImpactSwap;
	uint8_t cFlags;
	uint8_t padding4D;
	// one bit per PARENT_USE_FLAG
	uint16_t sParentUseFlags;
	// the fixed persistent references by cell map key
	NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*> FixedPersistentRefMap;
	BSSimpleList<TESObjectREFR*> MobilePersistentRefList;
	// the multibound references overlapping several cells, by cell map key
	NiTPointerMap<uint32_t, BSSimpleList<TESObjectREFR*>*>* pOverlappedMultiboundMap;
	NiPointer<BSPortalGraph> spPortalGraph;
	TESWorldSpace* pParentWorld;
	TESWaterForm* pWorldWater;
	TESWaterForm* pLODWater;
	float fLODWaterHeight;
	WorldSpaceMapData WorldMapData;
	WorldSpaceMapOffsetData WorldMapOffsetData;
	BGSMusicType* pMusicType;
	WorldSpaceCoordXY MinimumCoords;
	WorldSpaceCoordXY MaximumCoords;
	// the cell offsets by file
	NiTMap<TESFile*, OFFSET_DATA*> OffsetDataMap;
	BSStringT<char> EditorID;
	float fDefaultLandHeight;
	float fDefaultWaterHeight;
	BGSEncounterZone* pEncounterZone;
	TESTexture CanopyShadowTexture;
	TESTexture WaterNoiseTexture;

	// Guards the persistent reference data (FixedPersistentRefMap, MobilePersistentRefList) of every worldspace.
	static BSCriticalSection PersistentDataCritSection;
};

// The offset data map and its base: their vtables and the destructor the exe emits per instantiation (defined in
// Fallout Shared/TESWorldSpace.cpp).
using NiTMapBaseFileOffsetData =
	NiTMapBase<DFALL<NiTMapItem<TESFile*, TESWorldSpace::OFFSET_DATA*>>, TESFile*, TESWorldSpace::OFFSET_DATA*>;
using NiTMapFileOffsetData = NiTMap<TESFile*, TESWorldSpace::OFFSET_DATA*>;

inline constexpr fnvd::Vtable TESWorldSpace_vftable_TESTexture{};
inline constexpr fnvd::Vtable TESWorldSpace_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESWorldSpace_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESFile_OFFSET_DATA_vftable{};
inline constexpr fnvd::Vtable NiTMapBase_TESFile_OFFSET_DATA_vftable{};

// The game settings TESWorldSpace.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bExternalLODDataFiles;
extern SettingT<INISettingCollection> bCheckCellOffsetsOnInit;
inline TESWorldSpace* pCurrentWorldspace;
inline TESWorldSpace* plastWorldSpace;

// The PDB's function-local static of TESWorldSpace::GetMapNameForLocation (its constructor is empty).
// The PDB's function-local static lastWorldLoc of GetMapNameForLocation; src's NiPoint3 has a trivial default
// constructor, so it is zero data, and the body sets its guard bit as the exe does.
inline NiPoint3 lastWorldLoc{};
