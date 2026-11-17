#pragma once

#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpSimulationIsland;

char getMoppInstruction(char a1);
uint32_t getMoppOffset(uint32_t r_eax);
int32_t* setLowByte(uint8_t a1, int32_t* r_eax);
int32_t setUpperBits(int32_t* a1, int32_t r_eax);

// The default watch dog: removes the objects of the lowest auto-remove levels until enough heap memory is free.
class hkpDefaultWorldMemoryWatchDog : public hkWorldMemoryAvailableWatchDog {
public:
	// The reflection's member table (the PDB's hkpDefaultWorldMemoryWatchDog::Members).
	using MembersArray = const hkClassMember[1];

	hkpDefaultWorldMemoryWatchDog(int32_t minMemoryAvailable);
	// The SDK's finish constructor of a loaded object; assumes hkWorldMemoryAvailableWatchDog's (the SDK's).
	hkpDefaultWorldMemoryWatchDog(hkFinishLoadedObjectFlag aFlag) : hkWorldMemoryAvailableWatchDog(aFlag) {}
	~hkpDefaultWorldMemoryWatchDog() override;

	int32_t getAmountOfFreeHeapMemoryRequested() override;
	// Removes auto-removable entities until the heap can give the requested amount.
	void freeHeapMemoryTillRequestedAmountIsAvailable(hkpWorld* world) override;
	// Removes the island's first entity from its world.
	void reduceConstraintsInIsland(hkpSimulationIsland* island, int32_t solverBufferSize) override;

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static void removeObjectsFromIsland(hkpSimulationIsland* island, int32_t currentAutoRemoveLevel,
		int32_t freeHeapMemoryRequested, int32_t& maxFoundAutoRemoveLevelOut);

	int32_t m_freeHeapMemoryRequested;

	static MembersArray Members;
};
inline constexpr fnvd::Vtable hkpDefaultWorldMemoryWatchDog_vftable{};
