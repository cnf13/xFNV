#pragma once

#include "Reconstruction.hpp"

class hkpSimulationIsland;

// The interface the world reports a simulation island going to sleep or waking up to. It holds no state.
class hkpIslandActivationListener {
public:
	hkpIslandActivationListener();
	virtual ~hkpIslandActivationListener();

	// Pure, as in the SDK: the exe's own table (0x104DE10) holds _purecall in slots 1 and 2.
	virtual void islandActivatedCallback(hkpSimulationIsland* apIsland) = 0;
	virtual void islandDeactivatedCallback(hkpSimulationIsland* apIsland) = 0;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpIslandActivationListener_vftable{};
