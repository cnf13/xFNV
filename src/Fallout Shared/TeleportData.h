#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class BGSWorldLocation;
template <class Char> class BSStringT;
class TESFile;
class TESForm;
class TESObjectCELL;
class TESObjectREFR;
class TESWorldSpace;

// A door's teleport as a file stores it (XTEL): the linked door's form ID, the arrival position and rotation, and
// the flags.
struct FILE_TELEPORT_DATA {
	// Byte-swaps the door ID and the six floats read from a big-endian file; the flags are left alone.
	void Endian();

	int32_t iLinkedDoorID;
	NiPoint3 position;
	NiPoint3 rotation;
	uint8_t cFlags;
	uint8_t padding1D[3];
};

// Where a door's teleport leads: the linked door and the position and rotation an actor arrives at.
class DoorTeleportData {
public:
	// Values from the Xbox PDB.
	enum TELEPORT_DATA_FLAGS : int32_t {
		TDF_NOALARM = 0x1,
	};

	DoorTeleportData();
	~DoorTeleportData();

	// The linked door's world space: its parent cell's, else its save parent cell's; null without either.
	TESWorldSpace* GetTeleportWorldSpace();
	TESObjectCELL* GetTeleportCell();
	// Marks the cells on both sides of the door public or not.
	void SetConnectedCellsPublic(TESObjectREFR* apDoorRef, bool abIsPublic);
	TESObjectREFR* GetLinkedDoor();
	void SetLinkedDoor(TESObjectREFR* apDoorRef);
	const NiPoint3& GetTeleportPosition();
	void SetTeleportPosition(const NiPoint3& aTeleportPos);
	// Resolves the linked door's form ID for apOwner; false when the teleport is invalid.
	// Defined in ExtraDataObjects.cpp.
	bool InitItem(TESForm* apOwner);
	// The linked door's space (its world space on the PC), null without a door.
	TESForm* GetTeleportSpace();
	// Reads the XTEL chunk of apFile into the teleport.
	void Load(TESFile* apFile);
	// Copies apCopy's door, position, rotation and flags.
	void Copy(DoorTeleportData* apCopy);
	// Sets aOutMapName to the map name of the linked door's location, or empty without a
	// linked door (thiscall, ret 4, 0x43A2E0). No return value: its callers (0x776FD4, 0x79DB17) do not read eax.
	void GetTeleportCellMapName(BSStringT<char>& aOutMapName);
	void SetTeleportRotation(const NiPoint3& aTeleportRot);
	// Reads the position, the rotation and the flags, then the linked door's form ID (into pLinkedDoor, resolved
	// later) (thiscall, ret 4, 0x43AAA0; IDA's ExtraTeleport::Data::LoadGame). No return value: its caller,
	// ExtraDataList::LoadGameBGS 0x42B4F9, does not read eax.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Reads the position, the rotation and the linked door's form ID (into pLinkedDoor, resolved later)
	// from the old-format save (TESSaveLoadGame).
	void LoadGameTES();
	BGSWorldLocation* GetTeleportWorldLocation(BGSWorldLocation* apResult);
	// PDB Compare(DoorTeleportData*); 0x43A860 compares the door (+0), position (+0x4),
	// rotation (+0x10) and flags (+0x1C) of the two.
	bool Compare(DoorTeleportData* apCompare);
	// The save size of the teleport: the position, the rotation and the door's form ID (thiscall, ret 0, 0x43A8F0;
	// ExtraDataList::GetSaveSize 0x422C40 calls it on ExtraTeleport::pTeleportData).
	uint16_t GetSaveSize();
	// Resolves the linked door's saved form ID (ExtraDataList::InitLoadGame 0x42CEB0 calls it on
	// ExtraTeleport::pTeleportData).
	void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Not named in the PDB or IDA; name chosen here. Resolves the saved form ID in pLinkedDoor (thiscall, ret 0,
	// 0x43A9F0). No return value: its one caller, ExtraDataList::InitLoadGameTES 0x426020, does not read eax.
	void FinishInitLoadGame();
	// Writes the XTEL chunk.
	void Save();
	void SaveGame();
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);

	// The exe's deleting destructor (0x419220) frees through ourFree.
	static void operator delete(void* apMemory);

	// 0x0 the door on the other side
	TESObjectREFR* pLinkedDoor;
	// 0x4 where an actor arrives
	NiPoint3 position;
	// 0x10 the rotation an actor arrives with
	NiPoint3 rotation;
	// 0x1C the teleport flags
	uint8_t cFlags;
};
