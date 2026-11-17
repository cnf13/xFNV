#pragma once

#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"

struct hkpCollisionQueryJobHeader;
struct hkpShapeRayCastCommand;

// The batching manager whose vehicles cast rays from their wheels.
class hkpVehicleRayCastBatchingManager : public hkpVehicleCastBatchingManager {
public:
	// Where a batch's pieces lie inside the buffer.
	struct RaycastBatch {
		hkpShapeRayCastCommand* m_commandStorage;
		hkpWorldRayCastOutput* m_outputStorage;
		uint8_t* m_index;
		hkpCollisionQueryJobHeader* m_jobHeaders;
	};

	// The SDK's finish constructor: the loaded manager keeps its fields.
	hkpVehicleRayCastBatchingManager(hkFinishLoadedObjectFlag aFlag) : hkpVehicleCastBatchingManager(aFlag) {}

	// No destructor override: the destructor is hkpVehicleManager's.
	int32_t getBufferSize(int32_t numJobs) override;
	int32_t buildAndAddCastJobs(const hkpWorld* world, int32_t filterSize, int32_t numJobs, hkJobQueue* jobQueue,
		hkSemaphore* semaphore, void* buffer) override;
	void stepVehiclesUsingCastResults(const hkStepInfo& stepInfo, int32_t numJobs, void* buffer) override;

	void getRaycastBatchFromBuffer(void* raycastBuffer, int32_t numJobs, RaycastBatch& raycastBatch);

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();
};

extern hkClass hkpVehicleRayCastBatchingManagerClass;

extern hkTypeInfo hkpVehicleRayCastBatchingManagerTypeInfo;
