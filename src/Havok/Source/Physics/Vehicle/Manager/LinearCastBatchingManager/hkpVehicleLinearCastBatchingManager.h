#pragma once

#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkSemaphore;
class hkpCollidable;
struct hkpCollisionQueryJobHeader;
struct hkpPairLinearCastCommand;
struct hkpRootCdPoint;

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

	// No destructor override: the exe's slot 0 holds hkpVehicleManager's destructor.
	int32_t getBufferSize(int32_t numJobs) override;
	int32_t buildAndAddCastJobs(const hkpWorld* world, int32_t filterSize, int32_t numJobs, hkJobQueue* jobQueue,
		hkSemaphore* semaphore, void* buffer) override;
	void stepVehiclesUsingCastResults(const hkStepInfo& stepInfo, int32_t numJobs, void* buffer) override;

	// Havok SDK signature (hkpVehicle.lib, tied by bytes).
	int32_t getTotalNumCommands() const;
	// SDK name and form; thiscall, ret 0xC. The exe leaves the job headers' address in eax; its callers ignore it.
	void getLinearCastBatchFromBuffer(void* buffer, int32_t numJobs, LinearCastBatch& batchOut);

	// Havok SDK signature (hkpVehicle.lib, tied by target).
	static const hkClass& staticClass();
};
inline constexpr fnvd::Vtable hkpVehicleLinearCastBatchingManager_vftable{};

extern hkClass hkpVehicleLinearCastBatchingManagerClass;

extern hkTypeInfo hkpVehicleLinearCastBatchingManagerTypeInfo;
