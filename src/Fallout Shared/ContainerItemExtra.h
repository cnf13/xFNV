#pragma once

#include <cstdint>

class ExtraDataList;
class TESFile;
class TESForm;
class TESGlobal;
class TESHealthForm;

bool FloatEQ(float aX, float aY, float afPrecision);

// The per-entry extra data of a base container entry: its owner and its health fraction.
class ContainerItemExtra {
public:
	ContainerItemExtra();

	// Writes the owner and the health to the instance's extra data.
	void AddToExtraDataList(ExtraDataList* apExtra, TESHealthForm* apHealthForm);
	// Resolves the owner form ID the entry was loaded with, then the owner global ID of an NPC owner.
	void InitItem(TESForm* apOwner);
	// Reads the entry's COED chunk.
	bool Load(TESFile* apFile);
	// Copies apSource's owner and health.
	void Copy(ContainerItemExtra* apCopy);
	bool IsDefault();
	bool Compare(ContainerItemExtra* apCompare);
	TESGlobal* GetOwnerGlobal();
	// Writes the COED chunk of the entry, unless it holds no owner and full health.
	void Save();
	// Sets the health fraction, clamped to [0, 1].
	void AdjustHealthPercent(float afHealthPerc);

	TESForm* pOwnerForm;

	union {
		TESGlobal* pOwnerGlobal;
		int32_t iOwnerRank;
		uint32_t iUnion;
	};

	float fHealthMult;
};

// The COED chunk the entry is saved as: the owner's form ID, the global's form ID or the rank, and the health.
struct CONTAINER_ITEM_EXTRA {
	// Swaps the three fields for a byte-swapped save.
	void Endian();

	uint32_t iOwnerID;
	uint32_t iUnion;
	float fHealthMult;
};
