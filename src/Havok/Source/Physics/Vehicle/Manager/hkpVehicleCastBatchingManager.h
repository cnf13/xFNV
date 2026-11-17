#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleManager.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkJobQueue;
class hkJobThreadPool;
class hkSemaphore;
class hkpWorld;

// A vehicle manager that steps all its vehicles' wheel casts as batched jobs. The kind of cast comes from the
// derived manager. The exe has no RTTI for it (the class is abstract).
class hkpVehicleCastBatchingManager : public hkpVehicleManager {
public:
	// The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
	// classes' finish constructors call it): it keeps the fields the load wrote.
	hkpVehicleCastBatchingManager() = default;
	hkpVehicleCastBatchingManager(hkFinishLoadedObjectFlag f) : hkpVehicleManager(f) {}

	void addVehicle(hkpVehicleInstance* vehicle) override;
	void removeVehicle(hkpVehicleInstance* vehicle) override;
	virtual void stepVehiclesSynchronously(hkpWorld* world, hkJobThreadPool* threadPool, hkJobQueue* jobQueue,
		int32_t numJobs);
	virtual void updateBeforeCollisionDetection();
	virtual int32_t getBufferSize(int32_t numJobs) = 0;
	virtual int32_t buildAndAddCastJobs(const hkpWorld* world, int32_t filterSize, int32_t numJobs,
		hkJobQueue* jobQueue, hkSemaphore* semaphore, void* buffer) = 0;
	virtual void stepVehiclesUsingCastResults(const hkStepInfo& stepInfo, int32_t numJobs, void* buffer) = 0;

	// Havok SDK signature (hkpVehicle.lib, tied by bytes,reloc).
	static int32_t getStandardFilterSize(const hkpWorld* world);

	uint16_t m_totalNumWheels;
};

// The class's reflection record.
extern hkClass hkpVehicleCastBatchingManagerClass;
