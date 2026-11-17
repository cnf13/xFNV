#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

class MobileObject;

// process level of an actor in the AI system
enum PROCESS_TYPE : int32_t {
	PROCESS_TYPE_NONE = -1,
	PROCESS_TYPE_HIGH = 0,
	PROCESS_TYPE_MIDDLE_HIGH = 1,
	PROCESS_TYPE_MIDDLE_LOW = 2,
	PROCESS_TYPE_LOW = 3,
	PROCESS_TYPE_COUNT = 4
};

// the four arrays of actors split by process level, packed into one
// buffer; the level boundaries walk through it as actors move levels
class ProcessArray {
public:
	ProcessArray();
	~ProcessArray();

	// index of the level auiIndex belongs to, PROCESS_TYPE_NONE if none
	PROCESS_TYPE GetProcessLevelFromIndex(uint32_t auiIndex) const;
	// index of apActor inside its level, -1 when absent
	int32_t GetIndexFromActor(MobileObject* apActor, PROCESS_TYPE aProcessLevel);
	uint32_t GetProcessLevelHead(PROCESS_TYPE aProcessLevel) const;
	uint32_t GetProcessLevelTail(PROCESS_TYPE aProcessLevel) const;
	void AddActor(MobileObject* apActor, PROCESS_TYPE aWhichList);
	void RemoveActor(MobileObject* apActor, PROCESS_TYPE aWhichList);
	void RemoveActor(uint32_t auiIndex);

	void GrowArraySize(int32_t aSize);
	// not reconstructed: kept as calls to the original engine functions
	void CheckNextUpdateBounds();
	// The tail of the low level, the count of every level; the exe's shared one-field getter
	// 0x55B980 (thiscall, ret 0).
	uint32_t GetSize();
	// The actor at auiIndex.
	MobileObject* GetItem(uint32_t auiIndex);

	// 0x0 all actors of every level in level order
	NiTPrimitiveArray<MobileObject*> AIProcessArray;
	// 0x10 first index of each level
	uint32_t ProcessLevelHeads[PROCESS_TYPE_COUNT];
	// 0x20 one past the last index of each level
	uint32_t ProcessLevelTails[PROCESS_TYPE_COUNT];
	// 0x30 saved heads used to spread updates over frames
	uint32_t ProcessLevelNextUpdate[PROCESS_TYPE_COUNT];
};
