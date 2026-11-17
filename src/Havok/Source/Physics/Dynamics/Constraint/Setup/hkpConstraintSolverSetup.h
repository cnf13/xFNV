#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"

#include <stdint.h>

struct hkpBuildJacobianTaskHeader;
class hkpConstraintInstance;
class hkpConstraintQueryIn;
class hkpEntity;
class hkpJacobianSchema;
class hkpSimulationIsland;
class hkpSolverElemTemp;
struct hkpSolverInfo;
struct hkStepInfo;

// Applies the solver's accumulators to the numEntities entities' motions and recomputes their AABBs; returns
// hkRigidMotionUtilApplyAccumulators' result.
int32_t hkConstraintSolverSetup_integrate2(const hkStepInfo* info, const hkpSolverInfo* solverInfo,
	const hkpVelocityAccumulator* accumulators, hkpEntity* const* entities, int32_t numEntities);

// One entry of the array internalAddJacobianSchemas fills: the constraint, the first schema its Jacobian builder wrote
// and the smaller of its two bodies' allowed penetration depths.
struct hkpConstraintSchemaInfo {
	hkpConstraintInstance* m_constraint;
	hkpJacobianSchema* m_schema;
	float m_allowedPenetrationDepth;
};

// The solver's working buffer, split into accumulators, their backups, two schema ranges and the element temps.
struct hkpConstraintSolverResources {
	template <class T> struct BufferState {
		T* m_begin;
		T* m_end;
		T* m_current;
		T* m_lastProcessed;
	};

	// An accumulator's scratch0 and scratch123 column 0, saved while its transform is not built.
	struct VelocityAccumTransformBackup {
		hkVector4 m_linear;
		hkVector4 m_angular;
	};

	hkStepInfo* m_stepInfo;
	hkpSolverInfo* m_solverInfo;
	hkpConstraintQueryIn* m_constraintQueryInput;
	hkpVelocityAccumulator* m_accumulators;
	hkpVelocityAccumulator* m_accumulatorsEnd;
	hkpVelocityAccumulator* m_accumulatorsCurrent;
	VelocityAccumTransformBackup* m_accumulatorsBackup;
	VelocityAccumTransformBackup* m_accumulatorsBackupEnd;
	BufferState<hkpJacobianSchema> m_schemas[2];
	hkpSolverElemTemp* m_elemTemp;
	hkpSolverElemTemp* m_elemTempEnd;
	hkpSolverElemTemp* m_elemTempCurrent;
	hkpSolverElemTemp* m_elemTempLastProcessed;
};

// Havok's static entry points that build the accumulators and Jacobians of an island and run the constraint solver
// (solve, calcBufferSize, calcBufferOffsetsForSolve, initializeSolverState, shutdownSolver, subSolve, oneStepIntegrate
// and the internal add/memory-check helpers). No instances.
class hkpConstraintSolverSetup {
public:
	// How subSolve treats the schemas.
	enum SolverMode : int32_t {
		SOLVER_MODE_PROCESS_ALL = 0x0,
		SOLVER_MODE_INCREMENTAL_CONTINUE = 0x1,
	};

	hkpConstraintSolverSetup() = delete;

	static void initializeSolverState(hkStepInfo& stepInfo, hkpSolverInfo& solverInfo,
		hkpConstraintQueryIn& constraintQueryIn, char* buffer, int32_t bufferSize, hkpConstraintSolverResources& s);
	// numEntities is compared unsigned, so the body works on the int32_t as a uint32_t.
	static void internalAddAccumulators(hkpConstraintSolverResources& s, hkpEntity* const* entities,
		int32_t numEntities);
	// Builds the Jacobian schemas of the constraints [constraints, constraints + numConstraints) into the solver
	// buffer the resources' two schema ranges describe, appends one hkpConstraintSchemaInfo per constraint to cinfo
	// and returns the first schema range's next slot.
	static hkpJacobianSchema* internalAddJacobianSchemas(hkpConstraintSolverResources& s,
		hkpConstraintInstance** constraints, int32_t numConstraints,
		hkArray<hkpConstraintSchemaInfo>& constraintStatus);
	// Builds the island's accumulators and Jacobians in the solver allocator's buffer (or
	// preallocatedBuffer), solves and integrates; returns the frames the bodies have been inactive. The call site in
	// hkpSimulation::integrateInternal pushes all eight arguments and pops them.
	static int32_t solve(const hkStepInfo& stepInfo, const hkpSolverInfo& solverInfo,
		hkpConstraintQueryIn& constraintQueryIn, hkpSimulationIsland& island, void* preallocatedBuffer,
		int32_t preallocatedBufferSize, hkpEntity* const* bodies, int32_t numBodies);
	// Integrates each entity alone from a copy of its accumulator, with its velocities as the sums, deactivation
	// off meanwhile, then recomputes its aabb.
	static void oneStepIntegrate(const hkpSolverInfo& si, const hkStepInfo& info,
		const hkpVelocityAccumulator* accumulatorsBase, hkpEntity* const* entities, int32_t numEntities);
	// Lays the island's solver buffer out in taskHeader (accumulators, schemas, element temps, the size) and, given a
	// buffer, writes the fixed accumulator and the end marker; returns the size. bufferSize is never read.
	static int32_t calcBufferOffsetsForSolve(const hkpSimulationIsland& island, char* bufferIn, int32_t bufferSize,
		hkpBuildJacobianTaskHeader& taskHeader);
	// The solver buffer size the island needs: calcBufferOffsetsForSolve on no buffer, with a task header of its own.
	static int32_t calcBufferSize(hkpSimulationIsland& island);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	static void subSolve(hkpConstraintSolverResources& s, hkpConstraintSolverSetup::SolverMode mode);
	// Whether the constraints' schemas and element temps still fit the resources' buffer. The result is written through
	// apResult.
	static hkBool* internalIsMemoryOkForNewJacobianSchemas(hkBool* apResult, hkpConstraintSolverResources& s,
		hkpConstraintInstance** constraints, int32_t numConstraints);
	// Whether numEntities more accumulators (and the fixed one, while none is built yet) still fit the resources'
	// accumulator buffer, 16 bytes kept free. The result is written through apResult.
	static hkBool* internalIsMemoryOkForNewAccumulators(hkBool* apResult, hkpConstraintSolverResources& s,
		hkpEntity** entities, int32_t numEntities);
	// Releases what the solve took from the resources: nothing to do on the PC.
	static void shutdownSolver(hkpConstraintSolverResources& s);
};
