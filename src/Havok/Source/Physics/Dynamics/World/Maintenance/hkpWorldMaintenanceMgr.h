#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpWorld;

// The world's manager of the periodic upkeep (island merging and splitting) around each step. The SDK only
// forward-declares it; the exe's hkpDefaultWorldMaintenanceMgr implements it. The slot order and arguments come from
// the callers (hkpSimulation::integrate, hkpMultiThreadedSimulation::stepBeginSt).
class hkpWorldMaintenanceMgr : public hkReferencedObject {
public:
	virtual void init(hkpWorld* world) = 0;
	virtual void performMaintenance(hkpWorld* world, hkStepInfo& stepInfo) = 0;
	virtual void performMaintenanceNoSplit(hkpWorld* world, hkStepInfo& stepInfo) = 0;
};
