#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpVehicleData;
class hkpVehicleInstance;

void cleanupLoadedObjecthkpTyremarksWheel(void* p);
void cleanupLoadedObjecthkpTyremarksInfo(void* p);

// One tyre mark sample of a wheel: the world positions of the mark's left and right edges. No vtable. Layout from
// the exe's hkpTyremarkPointClass record (size 0x20) and the PC PDB.
struct hkpTyremarkPoint {
	hkpTyremarkPoint();

	float getTyremarkStrength();

	// The class's reflection record, hkpTyremarkPointClass.
	static const hkClass& staticClass();

	hkVector4 m_pointLeft;
	hkVector4 m_pointRight;
};

// A wheel's tyre mark history: m_numPoints samples in m_tyremarkPoints, used as a ring from m_currentPosition. The
// exe's vtable has 3 slots; it overrides only the destructor (slot 0). Layout from the exe's hkpTyremarksWheelClass
// record (size 0x1C) and the PC PDB.
class hkpTyremarksWheel : public hkReferencedObject {
public:
	hkpTyremarksWheel();
	// The SDK's finish constructor: the loaded wheel keeps its fields.
	hkpTyremarksWheel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_tyremarkPoints(aFlag) {}
	// Slot 0 (the deleting destructor 0xDFC2C0).
	~hkpTyremarksWheel() override;

	// Havok SDK signature (hkpVehicle.lib, tied by bytes).
	const hkpTyremarkPoint& getTyremarkPoint(int32_t point) const;
	void addTyremarkPoint(hkpTyremarkPoint* a1);
	void setNumPoints(int32_t num_points);

	// The class's reflection record, hkpTyremarksWheelClass.
	static const hkClass& staticClass();

	int32_t m_currentPosition;
	int32_t m_numPoints;
	hkArray<hkpTyremarkPoint, hkContainerHeapAllocator> m_tyremarkPoints;
};

// Maintains each wheel's tyre-mark history and energy limits.
class hkpTyremarksInfo : public hkReferencedObject {
public:
	hkpTyremarksInfo(const hkpVehicleData& data, int32_t num_points);
	// The SDK's finish constructor: the loaded info keeps its fields.
	hkpTyremarksInfo(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_tyremarksWheel(aFlag) {}
	~hkpTyremarksInfo() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	virtual void updateTyremarksInfo(float timestep, const hkpVehicleInstance* vehicle);
	// Havok SDK signature (hkpVehicle.lib, tied by vtable).
	virtual void getWheelTyremarksStrips(const hkpVehicleInstance* vehicle, int32_t wheel, hkVector4* strips_out) const;

	static const hkClass& staticClass();

	float m_minTyremarkEnergy;
	float m_maxTyremarkEnergy;
	hkArray<hkpTyremarksWheel*, hkContainerHeapAllocator> m_tyremarksWheel;
};

// The class's reflection record (the address 0xDFC220 returns).
extern hkClass hkpTyremarkPointClass;
inline constexpr fnvd::Vtable hkpTyremarksWheel_vftable{};

// The class's reflection record (the address 0xDFC240 returns).
extern hkClass hkpTyremarksWheelClass;
extern hkTypeInfo hkpTyremarksWheelTypeInfo;
extern hkClass hkpTyremarksInfoClass;
extern hkTypeInfo hkpTyremarksInfoTypeInfo;
inline constexpr fnvd::Vtable hkpTyremarksInfo_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpTyremarkPointTypeInfo;
