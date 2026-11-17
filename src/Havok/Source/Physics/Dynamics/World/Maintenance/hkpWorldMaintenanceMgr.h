#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpWorld;
struct hkStepInfo;

// The world's manager of the periodic upkeep (island merging and splitting) around each step.
// hkpDefaultWorldMaintenanceMgr implements it.
class hkpWorldMaintenanceMgr : public hkReferencedObject {
public:
	virtual void init(hkpWorld* world) = 0;
	virtual void performMaintenance(hkpWorld* world, hkStepInfo& stepInfo) = 0;
	virtual void performMaintenanceNoSplit(hkpWorld* world, hkStepInfo& stepInfo) = 0;
};
