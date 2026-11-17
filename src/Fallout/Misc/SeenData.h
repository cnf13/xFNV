#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class NiNode;
class NiPoint3;

// The bit grid of which parts of a cell the player has seen (the map's fog of war), saved and loaded with the game.
class SeenData {
public:
	SeenData();
	virtual ~SeenData();

	virtual void DrawSeenData(NiNode* apDrawNode, const NiPoint3& aSeenStart, bool abDrawSeen);
	virtual uint16_t GetSaveSize(bool abRecursing);
	virtual void SaveGame(bool abRecursing);
	virtual void SaveGame(BGSSaveFormBuffer* abRecursing);
	virtual void LoadGame(uint16_t acCount);
	virtual void LoadGame(BGSLoadFormBuffer* acCount);

	// Whether bit auiBit is set.
	bool IsSeenBitTrue(uint32_t auiBit);
	// Whether the bit of square (aiX, aiY) is set; rows are 16 squares wide.
	bool IsSeenBitTrue(uint32_t aiX, uint32_t aiY);
	// Sets bit aiPos to abVal; whether it was set before.
	bool SetSeenBit(uint32_t aiPos, bool abVal);
	bool UpdateSeenData(const NiPoint3& aCoord, const NiPoint3& aSeenStart);

	// Snaps aCoord's x and y down to the corner of its seen square and zeroes z.
	static void ShiftCoordToSeenPos(NiPoint3& aCoord);
	static float GetSeenPosSize();
	// The seen square of aCoord within its cell (an interior's cells are offset by half a cell).
	static void GetPosForCoord(const NiPoint3& aCoord, bool abInterior, int32_t& aiOutX, int32_t& aiOutY);
	// The world position of seen bit aiPos.
	static void GetPosOffset(uint32_t aiPos, NiPoint3& aOutCoord);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t pSeenBits[8];

	static inline int32_t iTotalBitCount = 256;
	// The side of one seen square in world units.
	static inline float fPosSize = 256.0f;
};

// The seen-data section of an interior cell: the next section in the chain and the section's grid position.
class IntSeenData : public SeenData {
public:
	IntSeenData(char acSectionX, char acSectionY);
	~IntSeenData() override;

	void DrawSeenData(NiNode* apDrawNode, const NiPoint3& aSeenStart, bool abDrawSeen) override;
	uint16_t GetSaveSize(bool abRecursing) override;
	void SaveGame(BGSSaveFormBuffer* abRecursing) override;
	void SaveGame(bool abRecursing) override;
	void LoadGame(BGSLoadFormBuffer* acCount) override;
	void LoadGame(uint16_t acCount) override;

	char cSectionX;
	char cSectionY;
	bool fullySeen;
	uint8_t padding27;
	IntSeenData* pNextSection;
};

// The game settings SeenData.cpp defines.
extern SettingT<INISettingCollection> iNumBitsForFullySeen;
