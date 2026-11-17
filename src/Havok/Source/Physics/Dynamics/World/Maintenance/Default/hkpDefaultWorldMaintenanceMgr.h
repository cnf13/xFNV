#pragma once

#include "Havok/Source/Physics/Dynamics/World/Maintenance/hkpWorldMaintenanceMgr.h"
#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpWorld;

// The default world maintenance: it merges islands that touch and splits those that fell apart, within the time
// window the two limits give. The PDB defines it in this folder's hkpDefaultWorldMaintenanceMgr.cpp.
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
inline constexpr fnvd::Vtable hkpDefaultWorldMaintenanceMgr_vftable{};
