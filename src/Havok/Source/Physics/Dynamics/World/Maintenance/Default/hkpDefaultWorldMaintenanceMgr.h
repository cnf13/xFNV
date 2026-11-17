#pragma once

#include "Havok/Source/Physics/Dynamics/World/Maintenance/hkpWorldMaintenanceMgr.h"

class hkpWorld;
struct hkStepInfo;

// The default world maintenance: it merges islands that touch and splits those that fell apart, within the time window
// the two limits give.
class hkpDefaultWorldMaintenanceMgr : public hkpWorldMaintenanceMgr {
public:
    hkpDefaultWorldMaintenanceMgr();

    void init(hkpWorld* world) override;
    void performMaintenance(hkpWorld* world, hkStepInfo& stepInfo) override;
    void performMaintenanceNoSplit(hkpWorld* world, hkStepInfo& stepInfo) override;

    void resetWorldTime(hkpWorld* world, hkStepInfo& stepInfo);

    float m_minAllowedTimeValue;
    float m_maxAllowedTimeValue;
};
