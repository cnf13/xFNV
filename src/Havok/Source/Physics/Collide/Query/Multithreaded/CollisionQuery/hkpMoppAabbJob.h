#pragma once

#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Reconstruction.hpp"

// One MOPP AABB query: the box to cast and the buffer the query writes its shape keys to (the PDB's hkpShapeKey is a
// 4-byte key, which src spells uint32_t). 16-byte aligned through hkAabb, so its stride is 0x30.
struct hkpMoppAabbCommand {
	// Values from the Xbox PDB.
	enum : int32_t {
		MAX_OUTPUT_KEYS_PER_QUERY = 0x1000,
	};

	hkAabb m_aabbInput;
	uint32_t (*m_results)[MAX_OUTPUT_KEYS_PER_QUERY];
};

// Batches MOPP AABB queries: the MOPP code to query and the commands to run on it.
struct hkpMoppAabbJob : public hkpCollisionQueryJob {
	// Values from the Xbox PDB.
	enum : int32_t {
		MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 0x100,
	};

	hkpMoppCode::CodeInfo m_moppCodeInfo;
	const uint8_t* m_moppCodeData;
	int32_t m_numCommandsPerTask;
	const hkpMoppAabbCommand* m_commandArray;
	int32_t m_numCommands;
};
