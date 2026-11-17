#pragma once

#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpSimulationIsland;

// The interface the world reports the end of a simulation island's collide step to; the world keeps the listeners in
// hkpWorld::m_islandPostCollideListeners and hkpWorldCallbackUtil::fireIslandPostCollideCallback (0xD01650) calls
// slot 1 of each with the island and the step. It holds no state: the PDB names no member, so the object is its
// vtable pointer (slot 0 the destructor, slot 1 postCollideCallback). The exe has no vtable for the class
// (fn.py vtable: none), so none is declared here.
class hkpIslandPostCollideListener {
public:
	virtual ~hkpIslandPostCollideListener();

	virtual void postCollideCallback(hkpSimulationIsland* island, const hkStepInfo& stepInfo) = 0;
};
