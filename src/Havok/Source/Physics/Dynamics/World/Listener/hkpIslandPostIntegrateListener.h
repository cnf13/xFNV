#pragma once

#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpSimulationIsland;

// The interface an island reports its post-integrate step to. It holds no state: the PDB names no member, so the
// object is its vtable pointer (slot 0 the destructor, slot 1 postIntegrateCallback). hkpWorldCallbackUtil::
// fireIslandPostIntegrateCallback (0xD015A0) calls slot 1 with the island and the step: the emitted body in
// hkpWorldCallbackUtil.cpp calls *vptr + 4. The SDK reference header (World/Listener/hkpIslandPostIntegrateListener.h)
// declares the same two virtuals. The exe has no vtable for the class (fn.py vtable: none), so none is declared here.
class hkpIslandPostIntegrateListener {
public:
	virtual ~hkpIslandPostIntegrateListener();

	virtual void postIntegrateCallback(hkpSimulationIsland* island, const hkStepInfo& stepInfo) = 0;
};
