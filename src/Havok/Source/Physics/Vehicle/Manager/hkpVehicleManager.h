#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpVehicleInstance;
class hkpWorld;

// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
const void* getVtablehkpVehicleManager();

// The registry of a world's vehicles, stepping each one a step at a time.
class hkpVehicleManager : public hkReferencedObject {
public:
	hkpVehicleManager(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_registeredVehicles(aFlag) {}
	~hkpVehicleManager() override;

	virtual void addVehicle(hkpVehicleInstance* vehicle);
	virtual void removeVehicle(hkpVehicleInstance* vehicle);
	virtual void stepVehicles(const hkStepInfo& stepInfo);

	// Havok SDK signature (hkpVehicle.lib, tied by bytes).
	void addToWorld(hkpWorld* world);
	void removeFromWorld();

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<hkpVehicleInstance*> m_registeredVehicles;
};

inline constexpr fnvd::Vtable hkpVehicleManager_vftable{};
extern hkClass hkpVehicleManagerClass;
extern hkTypeInfo hkpVehicleManagerTypeInfo;
