#pragma once

#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Reconstruction.hpp"

class hkpCollidable;
struct hkpRootCdPoint;

// One pair get-closest-points query: the two collidables, the result buffer and where this command's results start
// in the job's shared buffer. The SDK declares it HK_CLASSALIGN16 (__declspec(align(16))), which pads its 0x18
// bytes of members to the exe's 0x20 stride.
struct alignas(16) hkpPairGetClosestPointsCommand {
	// Values from the Xbox PDB.
	enum : int32_t {
		MAXIMUM_RESULTS_CAPACITY = 0x10,
	};

	const hkpCollidable* m_collidableA;
	const hkpCollidable* m_collidableB;
	hkpRootCdPoint* m_results;
	int32_t m_resultsCapacity;
	int32_t m_numResultsOut;
	uint32_t* m_indexIntoSharedResults;
};

// Batches pair get-closest-points commands: each command finds the closest points of two collidables within
// m_tolerance. Layout from the PC PDB (the Havok SDK declares it in hkpCollisionQueryJobs.h; src gives each job its
// own header, as hkpWorldLinearCastJob).
struct hkpPairGetClosestPointsJob : public hkpCollisionQueryJob {
	// Values from the Xbox PDB.
	enum : int32_t {
		MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 0x100,
	};

	// Nothing to do on the PC; the exe's body is a bare ret (0xD05DD0).
	void setRunsOnSpuOrPpu();

	float m_tolerance;
	int32_t m_numCommandsPerTask;
	const hkpPairGetClosestPointsCommand* m_commandArray;
	int32_t m_numCommands;
};
