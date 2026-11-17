#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

#include <bit>
#include <xmmintrin.h>

hkpWorldCinfo::hkpWorldCinfo()
	: m_gravity(0.0f, -9.8f, 0.0f, 0.0f), m_broadPhaseQuerySize(1024), m_contactRestingVelocity(1.0f),
	  m_mtPostponeAndSortBroadPhaseBorderCallbacks(false), m_useKdTree(false), m_useMultipleTree(false),
	  m_autoUpdateKdTree(true), m_collisionTolerance(0.1f), m_collisionFilter(nullptr), m_convexListFilter(nullptr),
	  m_expectedMaxLinearVelocity(200.0f), m_sizeOfToiEventQueue(250), m_expectedMinPsiDeltaTime(0.033333335f),
	  m_memoryWatchDog(nullptr), m_broadPhaseNumMarkers(0), m_allowToSkipConfirmedCallbacks(false), m_solverTau(0.6f),
	  m_solverDamp(1.0f), m_solverIterations(4), m_solverMicrosteps(1), m_maxConstraintViolation(3.40282e+38f),
	  m_forceCoherentConstraintOrderingInSolver(false), m_snapCollisionToConvexEdgeThreshold(0.524f),
	  m_snapCollisionToConcaveEdgeThreshold(0.698f), m_enableToiWeldRejection(false), m_enableDeprecatedWelding(false),
	  m_iterativeLinearCastEarlyOutDistance(0.01f), m_iterativeLinearCastMaxIterations(20),
	  m_deactivationNumInactiveFramesSelectFlag0(0), m_deactivationNumInactiveFramesSelectFlag1(0),
	  m_deactivationIntegrateCounter(0), m_shouldActivateOnRigidBodyTransformChange(true),
	  m_deactivationReferenceDistance(0.02f), m_toiCollisionResponseRotateNormal(0.2f), m_maxSectorsPerCollideTask(4),
	  m_processToisMultithreaded(true), m_maxEntriesPerToiCollideTask(8), m_maxNumToiCollisionPairsSinglethreaded(1),
	  m_numToisTillAllowedPenetrationSimplifiedToi(3.0f), m_numToisTillAllowedPenetrationToi(3.0f),
	  m_numToisTillAllowedPenetrationToiHigher(4.0f), m_numToisTillAllowedPenetrationToiForced(20.0f),
	  m_enableDeactivation(true), m_enableSimulationIslands(true), m_minDesiredIslandSize(64),
	  m_processActionsInSingleThread(true), m_allowIntegrationOfIslandsWithoutConstraintsInASeparateJob(false),
	  m_frameMarkerPsiSnap(0.0001f), m_fireCollisionCallbacks(false) {
	m_broadPhaseBorderBehaviour = BROADPHASE_BORDER_ASSERT;
	m_broadPhaseWorldAabb.m_min.set(-500.0f, -500.0f, -500.0f, 0.0f);
	m_broadPhaseWorldAabb.m_max.set(500.0f, 500.0f, 500.0f, 0.0f);
	m_treeUpdateType = REBUILD_ACTIVE;
	m_contactPointGeneration = CONTACT_POINT_REJECT_MANY;
	m_simulationType = SIMULATION_TYPE_CONTINUOUS;
}

hkpWorldCinfo::hkpWorldCinfo(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {
	if (aFlag.m_finishing && m_contactRestingVelocity == 0.0f) {
		m_contactRestingVelocity = 1.0f;
	}
}

// The three filters and the watch dog are held by reference (raw pointers here, hkRefPtr in the SDK).
hkpWorldCinfo::~hkpWorldCinfo() {
	if (m_memoryWatchDog) {
		m_memoryWatchDog->removeReference();
	}
	if (m_convexListFilter) {
		m_convexListFilter->removeReference();
	}
	if (m_collisionFilter) {
		m_collisionFilter->removeReference();
	}
}

void hkpWorldCinfo::setupSolverInfo(hkpWorldCinfo::SolverType st) {
	switch (st) {
	case hkpWorldCinfo::SOLVER_TYPE_2ITERS_SOFT:
		m_solverIterations = 2;
		m_solverTau = 0.3f;
		m_solverDamp = 0.9f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_2ITERS_MEDIUM:
		m_solverIterations = 2;
		m_solverTau = 0.6f;
		m_solverDamp = 1.0f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_2ITERS_HARD:
		m_solverIterations = 2;
		m_solverTau = 0.9f;
		m_solverDamp = 1.1f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_4ITERS_SOFT:
		m_solverIterations = 4;
		m_solverTau = 0.3f;
		m_solverDamp = 0.9f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_4ITERS_MEDIUM:
		m_solverIterations = 4;
		m_solverTau = 0.6f;
		m_solverDamp = 1.0f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_4ITERS_HARD:
		m_solverIterations = 4;
		m_solverTau = 0.9f;
		m_solverDamp = 1.1f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_8ITERS_SOFT:
		m_solverIterations = 8;
		m_solverTau = 0.3f;
		m_solverDamp = 0.9f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_8ITERS_MEDIUM:
		m_solverIterations = 8;
		m_solverTau = 0.6f;
		m_solverDamp = 1.0f;
		break;
	case hkpWorldCinfo::SOLVER_TYPE_8ITERS_HARD:
		m_solverIterations = 8;
		m_solverTau = 0.9f;
		m_solverDamp = 1.1f;
		break;
	default:
		break;
	}
}

// Multiplies by the doubles -0.5 and 0.5 and rounds each product to a float before broadcasting it.
void hkpWorldCinfo::setBroadPhaseWorldSize(float sideLength) {
	float fMin = static_cast<float>(-0.5 * sideLength);
	m_broadPhaseWorldAabb.m_min.m_quad[0] = fMin;
	m_broadPhaseWorldAabb.m_min.m_quad[1] = fMin;
	m_broadPhaseWorldAabb.m_min.m_quad[2] = fMin;
	m_broadPhaseWorldAabb.m_min.m_quad[3] = fMin;
	float fMax = static_cast<float>(sideLength * 0.5);
	m_broadPhaseWorldAabb.m_max.m_quad[0] = fMax;
	m_broadPhaseWorldAabb.m_max.m_quad[1] = fMax;
	m_broadPhaseWorldAabb.m_max.m_quad[2] = fMax;
	m_broadPhaseWorldAabb.m_max.m_quad[3] = fMax;
}
