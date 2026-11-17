#pragma once

#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"

#include <stdint.h>

class hkpCollidable;
struct hkpCollisionQueryJobHeader;
struct hkpPairLinearCastCommand;
struct hkpRootCdPoint;
class hkSemaphore;

// The batching manager whose vehicles cast their wheels' shapes (linear casts).
class hkpVehicleLinearCastBatchingManager : public hkpVehicleCastBatchingManager {
public:
	// Where a batch's pieces lie inside the buffer.
	struct LinearCastBatch {
		hkpCollidable* m_collidableStorage;
		hkpPairLinearCastCommand* m_commandStorage;
		hkpRootCdPoint* m_outputStorage;
		hkpCollisionQueryJobHeader* m_jobHeaders;
	};

	hkpVehicleLinearCastBatchingManager(hkFinishLoadedObjectFlag aFlag) : hkpVehicleCastBatchingManager(aFlag) {}

	// No destructor override: the class uses hkpVehicleManager's destructor.
	int32_t getBufferSize(int32_t numJobs) override;
	int32_t buildAndAddCastJobs(const hkpWorld* world, int32_t filterSize, int32_t numJobs, hkJobQueue* jobQueue,
		hkSemaphore* semaphore, void* buffer) override;
	void stepVehiclesUsingCastResults(const hkStepInfo& stepInfo, int32_t numJobs, void* buffer) override;

	// Havok SDK signature (hkpVehicle.lib, tied by bytes).
	int32_t getTotalNumCommands() const;
	void getLinearCastBatchFromBuffer(void* buffer, int32_t numJobs, LinearCastBatch& batchOut);

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();
};

extern hkClass hkpVehicleLinearCastBatchingManagerClass;

extern hkTypeInfo hkpVehicleLinearCastBatchingManagerTypeInfo;
