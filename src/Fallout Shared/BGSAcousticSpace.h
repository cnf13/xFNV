#pragma once

#include "Fallout Shared/TESObject.h"

class TESRegion;
class TESSound;

// The reverb environment of a cell or acoustic space.
enum CELL_ENVIRO_TYPE : int32_t {
	CET_NONE = 0,
	CET_DEFAULT = 1,
	CET_GENERIC = 2,
	CET_PADDEDCELL = 3,
	CET_ROOM = 4,
	CET_BATHROOM = 5,
	CET_LIVINGROOM = 6,
	CET_STONEROOM = 7,
	CET_AUDITORIUM = 8,
	CET_CONCERTHALL = 9,
	CET_CAVE = 10,
	CET_ARENA = 11,
	CET_HANGAR = 12,
	CET_CARPETEDHALLWAY = 13,
	CET_HALLWAY = 14,
	CET_STONECORRIDOR = 15,
	CET_ALLEY = 16,
	CET_FOREST = 17,
	CET_CITY = 18,
	CET_MOUNTAINS = 19,
	CET_QUARRY = 20,
	CET_PLAIN = 21,
	CET_PARKINGLOT = 22,
	CET_SEWERPIPE = 23,
	CET_UNDERWATER = 24,
	CET_SMALLROOM = 25,
	CET_MEDIUMROOM = 26,
	CET_LARGEROOM = 27,
	CET_MEDIUMHALL = 28,
	CET_LARGEHALL = 29,
	CET_PLATE = 30,
	CET_COUNT = 31,
};

// An acoustic space form (ASPC): the ambient sounds and reverb of an area.
class BGSAcousticSpace : public TESBoundObject {
public:
	BGSAcousticSpace();
	~BGSAcousticSpace() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	void UnClone3D(TESObjectREFR*) override {}
	NiAVObject* Clone3D(TESObjectREFR*) override { return nullptr; }
	using TESBoundObject::Clone3D;
	using TESForm::Save;

	// Whether the acoustic space is for an interior.
	bool IsInterior();
	// The SNAM chunk holds five sound IDs in a row.
	void SetDawnSound(TESSound* apSound) { pDawnSound = apSound; }
	void SetNoonSound(TESSound* apSound);
	void SetDuskSound(TESSound* apSound) { pDuskSound = apSound; }
	void SetNightSound(TESSound* apSound) { pNightSound = apSound; }
	void SetWallaSound(TESSound* apSound) { pWallaSound = apSound; }

	bool bIsInterior;
	uint8_t padding31[3];
	// the sounds of the four times of day, then the walla (crowd murmur) sound
	TESSound* pDawnSound;
	TESSound* pNoonSound;
	TESSound* pDuskSound;
	TESSound* pNightSound;
	TESSound* pWallaSound;
	TESRegion* pSoundRegion;
	CELL_ENVIRO_TYPE eEnvType;
	uint32_t iWallaPop;
};
