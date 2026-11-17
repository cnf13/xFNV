#pragma once

#include "BSMain/Setting.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/TESGrass.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class TESObjectCELL;
class TESObjectLAND;

// The PC constructor initializes shared grass state and does not access instance storage.
class TESTerrainLODManager {
public:
	TESTerrainLODManager();
	~TESTerrainLODManager();

	static void UpdateGrass(NiPoint3 apCameraPosition, NiPoint3 apCameraDirection, float afUpdateAngle);
	static void InitGrass();
	static NiNode* GetGrassNode();

protected:
	// Private in the PDB; protected so that FNVD_REPLACES_MEMBER can register it.
	static void UpdateGrassForCell(TESObjectCELL* apCell, NiNode* apGrassNode, NiPoint3 apCameraPosition,
		NiPoint3 apCameraDirection, float fFadeRange, float fMaxRange, float afUpdateAngle);

public:
	static void CreateGrass(int32_t aiCellx, int32_t aiCelly, int32_t aiBlock, TESObjectLAND* apLand, int32_t aiSeed,
		const NiPoint3& aCoord, NiNode* apGrassNode, NiPoint3 apPosition,
		TallGrassShaderProperty::GrassParam* apGrassParam, float* apfDensity, float fFadeRange, float fMaxRange,
		uint16_t iHeightLimit, TESGrass::GRASS_WATER_STATE iWaterState, float afCellWaterHeight, uint8_t aiMinSlope,
		uint8_t aiMaxSlope);

	static inline NiNode* pGrassNode;
	static inline bool bGrassInitialized;
	static inline float fGrassRadius;
	static inline NiPoint3* pLowpositions;
	static inline float* pLowcolors;
	static inline int32_t iManagerCount;
};

// Whether the grass is updated with the camera: ToggleGrass sets it when it shows the grass,
// ToggleGrassUpdate flips it, UpdateGrass reads it.
inline bool bIsToggleGrassUpdate = true;

// The game settings TESTerrainLODManager.cpp defines (PDB), at their exe addresses.
extern SettingT<RendererSettingCollection> fGrassStartFadeDistance;
extern SettingT<INISettingCollection> fGrassFadeRange;
extern SettingT<INISettingCollection> fGrassMinStartFadeDistance;
extern SettingT<INISettingCollection> fGrassDefaultStartFadeDistance;
extern SettingT<INISettingCollection> fGrassMaxStartFadeDistance;

// INISettingCollection: 0xf4c1fa calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bGrassPointLighting_Grass;

// INISettingCollection: 0xf4c362 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fGrassWindMagnitudeMin_Grass;

// INISettingCollection: 0xf4c1ca calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bDrawShaderGrass_Grass;

// INISettingCollection: 0xf4c392 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fGrassWindMagnitudeMax_Grass;

// INISettingCollection: 0xf4c32a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iMinGrassSize_Grass;
// IDA name; the PDB's btemptest. PDB: bool; the exe reads its byte (UpdateGrassForCell, 0x57D51A).
inline bool bTempTest;
