#pragma once

#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpWorld;

// The interface the world reports the end of an integrate step to. It holds no state: the PDB names no member, so the
// object is its vtable pointer (slot 0 the destructor, slot 1 postIntegrateCallback). hkpWorldCallbackUtil::
// firePostIntegrateCallback (0xD01440) calls slot 1 with the world and the step: the emitted body in
// hkpWorldCallbackUtil.cpp calls *vptr + 4. The SDK reference header (World/Listener/hkpWorldPostIntegrateListener.h)
// declares the same two virtuals. The exe has no vtable for the class (fn.py vtable: none), so none is declared here.
class hkpWorldPostIntegrateListener {
public:
	virtual ~hkpWorldPostIntegrateListener();

	virtual void postIntegrateCallback(hkpWorld* world, const hkStepInfo& stepInfo) = 0;
};
