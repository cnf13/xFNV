#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuWorldLinearCastJob.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpWorldLinearCastJob.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <xmmintrin.h>

// Not named in the PDB or IDA; name chosen here. The SDK's
// hkAabbTreeQueryUtils<hkAabbTreeAabb, hkUint16>::castAabb (hkAabbTreeQueryUtils.h; cdecl, ret 0), the twin of the
// Aabb16 castAabb specialization (0xD1C2B0): hkCpuWorldLinearCastJob 0xDF7340 passes the tree, a local hkAabb, the
// cast input, the collector's hkAabbTreeAabb Collector base, 0.0f, null and 0.
// Inferred type: hitCapacity: int32_t: signed uses from 0xd1ce28
static int32_t castAabbInternal(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
	const hkAabb& queryVolume, hkAabbTreeData::CastVolumeInput& input,
	hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector* collector, float aabbCheckTolerance,
	void** hitArray, int32_t hitCapacity);

// The SDK's castAabb: the box's sweep from input.m_from to input.m_to cast as a ray (hkAabbTreeData::Ray::setRay
// inlined, as in castRay) through the tree's boxes grown by input.m_halfExtents plus aabbCheckTolerance; the exe never
// reads queryVolume. Each leaf reached goes to collector, whose addLeaf result becomes the early-out fraction (1 at
// first), else its data is written to hitArray, the walk stopping once the count before it has reached hitCapacity.
// Returns the leaves counted.
static int32_t castAabbInternal(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
	const hkAabb& queryVolume, hkAabbTreeData::CastVolumeInput& input,
	hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector* collector, float aabbCheckTolerance,
	void** hitArray, int32_t hitCapacity) {
	using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>;
	if (!tree) {
		return 0;
	}
	const Tree::Node* node = tree->m_rootNode;
	if (!node) {
		return 0;
	}
	// setRay: the direction's zero lanes are 1 for the reciprocal estimate (w too) and the largest float in the
	// Newton-refined inverse direction.
	const __m128 start = input.m_from.m_quad;
	const __m128 delta = _mm_sub_ps(input.m_to.m_quad, start);
	const __m128 isZero = _mm_cmpeq_ps(hkVector4Zero.m_quad, delta);
	__m128 masked = _mm_or_ps(_mm_and_ps(isZero, hkQuadReal1111), _mm_andnot_ps(isZero, delta));
	masked = _mm_shuffle_ps(masked, _mm_unpackhi_ps(masked, hkQuadReal1111), _MM_SHUFFLE(3, 0, 1, 0));
	const __m128 rcp = _mm_rcp_ps(masked);
	const __m128 invDir =
		_mm_or_ps(_mm_andnot_ps(isZero, _mm_sub_ps(_mm_add_ps(rcp, rcp), _mm_mul_ps(_mm_mul_ps(rcp, masked), rcp))),
			_mm_and_ps(isZero, rdata_10CD670));
	const __m128 halfExtents = _mm_add_ps(_mm_set1_ps(aabbCheckTolerance), input.m_halfExtents.m_quad);
	__m128 earlyOut = _mm_set1_ps(1.0f);
	int32_t numHits = 0;
	for (;;) {
		const __m128 nodeMax = _mm_add_ps(node->m_volume.m_max.m_quad, halfExtents);
		const __m128 nodeMin = _mm_sub_ps(node->m_volume.m_min.m_quad, halfExtents);
		const uint16_t skipIdx = node->m_skipNodeIdx;
		const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : tree->m_nodeManager.getNode(skipIdx);
		// The slab test: the latest entry (not before 0) against the earliest exit (not after earlyOut), x lane.
		const __m128 t0 = _mm_mul_ps(_mm_sub_ps(nodeMin, start), invDir);
		const __m128 t1 = _mm_mul_ps(_mm_sub_ps(nodeMax, start), invDir);
		const __m128 tNear = _mm_min_ps(t0, t1);
		const __m128 tFar = _mm_max_ps(t0, t1);
		const __m128 exit = _mm_min_ps(_mm_min_ps(tFar, earlyOut),
			_mm_min_ps(_mm_shuffle_ps(tFar, tFar, 0x55), _mm_shuffle_ps(tFar, tFar, 0xAA)));
		const __m128 entry = _mm_max_ps(_mm_max_ps(tNear, hkVector4Zero.m_quad),
			_mm_max_ps(_mm_shuffle_ps(tNear, tNear, 0x55), _mm_shuffle_ps(tNear, tNear, 0xAA)));
		if (_mm_movemask_ps(_mm_cmplt_ps(entry, exit)) & 1) {
			if (!node->isLeaf()) {
				node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
				continue;
			}
			if (collector) {
				earlyOut = _mm_set1_ps(collector->addLeaf(node));
			} else if (hitArray) {
				*hitArray++ = node->m_leafData.m_userData;
				if (numHits >= hitCapacity) {
					return numHits;
				}
			}
			++numHits;
		}
		node = next;
		if (!node) {
			return numHits;
		}
	}
}

using hkAabbTreeQueryUtils_lhkAabbTreeData_nhkAabbTreeAabb_cunsigned_sshort_g_nCollector =
	hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector;

using hkAabbTreeQueryUtils_lhkAabbTreeData_nhkAabbTreeAabb16_cunsigned_sshort_g_nCollector =
	hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector;

// The SDK's job function: one fixed buffer collector and one world collector serve every command. A command casts its
// collidable's AABB along the path to m_to through the broad phase when the job has one, else through the job's AABB
// trees and k-d tree; the collector's points go to the command's buffer. The exe reads 0.5 at 0x1016248 (f0_5_) and
// the four 1.0 lanes at 0x10EC7F0 (hkQuadReal1111_0).
hkJobQueue::JobStatus hkCpuWorldLinearCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
	HK_TIMER_BEGIN("CollQueryWorldLinearCast", nullptr);
	// The queue entry holds the job: its hkJob base is the job's.
	const hkpWorldLinearCastJob& job = static_cast<const hkpWorldLinearCastJob&>(static_cast<const hkJob&>(nextJobOut));
	const hkpProcessCollisionInput* collisionInput = job.m_collisionInput;

	// The exe's inlined destructor at the end of the scope stores nothing (its vtable stores are dead).
	alignas(16) FNVD_CONSTRUCTED_LOCAL(hkpFixedBufferCdPointCollector, castCollector, nullptr, nullptr, 1);

	// The collector's constructor is inlined and its destructor leaves nothing behind in the exe.
	alignas(16) FNVD_CONSTRUCTED_LOCAL(hkCpuWorldLinearCastCollector, collector, nullptr);
	hkpLinearCastCollisionInput& input = collector.m_input;
	// The job's filter is the world's group filter.
	collector.m_filter = static_cast<const hkpGroupFilter*>(collisionInput->m_filter);
	collector.m_castCollector = &castCollector;
	// hkpCollisionInput's assignment, inlined member by member (the padding before m_aabb32Info is not copied).
	input.m_dispatcher = collisionInput->m_dispatcher;
	input.m_weldClosestPoints = collisionInput->m_weldClosestPoints;
	input.m_tolerance = collisionInput->m_tolerance;
	input.m_filter = collisionInput->m_filter;
	input.m_convexListFilter = collisionInput->m_convexListFilter;
	input.m_createPredictiveAgents = collisionInput->m_createPredictiveAgents;
	input.m_aabb32Info = collisionInput->m_aabb32Info;
	input.m_config = collisionInput->m_config;

	for (int32_t i = 0; i < job.m_numCommands; i++) {
		// The job keeps its commands const; the cast writes their result counts.
		hkpWorldLinearCastCommand& command = const_cast<hkpWorldLinearCastCommand&>(job.m_commandArray[i]);

		// The SDK's placement new of the collector over this command's results (the constructor inlined in the exe).
		castCollector.~hkpFixedBufferCdPointCollector();
		::new (&castCollector) hkpFixedBufferCdPointCollector(command.m_results, command.m_resultsCapacity);

		const hkpCollidable* collidable = command.m_collidable;
		hkVector4 path;
		path.m_quad = _mm_sub_ps(command.m_input.m_to.m_quad, collidable->m_motion.pMotion->m_translation.m_quad);
		collector.m_collidable = collidable;
		collector.m_shapeType = static_cast<hkpShapeType>(collidable->m_shape->m_type.m_storage);
		input.m_maxExtraPenetration = command.m_input.m_maxExtraPenetration;
		// setPathAndTolerance: the path, its length (z + (y + x) of the squares) and the tolerance.
		float tolerance = command.m_input.m_startPointTolerance;
		input.m_path = path;
		__m128 lengthSquared = _mm_mul_ps(path.m_quad, path.m_quad);
		__m128 sum = _mm_add_ss(_mm_shuffle_ps(lengthSquared, lengthSquared, 0x55), lengthSquared);
		sum = _mm_add_ss(_mm_shuffle_ps(lengthSquared, lengthSquared, 0xAA), sum);
		input.m_cachedPathLength = _mm_cvtss_f32(_mm_sqrt_ss(sum));
		input.m_tolerance = tolerance;

		const hkpBroadPhase* broadPhase = job.m_broadphase;
		__m128 half = _mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0);
		FNVD_INLINED_LOCAL(hkAabb, aabb, std::nullopt, std::nullopt);
		if (broadPhase) {
			hkpBroadPhase::hkpCastAabbInput castInput;
			castInput.m_aabbCacheInfo = nullptr;
			collidable->m_shape->getAabb(*collidable->m_motion.pMotion, command.m_input.m_startPointTolerance, aabb);
			// hkAabb::getCenter through setInterpolate4: (1 - 0.5) * min + max * 0.5.
			castInput.m_from.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, half), aabb.m_min.m_quad),
				_mm_mul_ps(aabb.m_max.m_quad, half));
			castInput.m_to.m_quad = _mm_add_ps(castInput.m_from.m_quad, path.m_quad);
			castInput.m_halfExtents.m_quad = _mm_mul_ps(half, _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad));
			castInput.m_aabbCacheInfo = nullptr;
			broadPhase->castAabb(castInput, collector);
		} else {
			collidable->m_shape->getAabb(*collidable->m_motion.pMotion, command.m_input.m_startPointTolerance, aabb);
			__m128 from = _mm_mul_ps(half, _mm_add_ps(aabb.m_max.m_quad, aabb.m_min.m_quad));
			__m128 to = _mm_add_ps(from, path.m_quad);
			__m128 halfExtents = _mm_mul_ps(half, _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad));
			// The exe casts through both AABB trees when either is set.
			if (job.m_aabbTree || job.m_aabbTreeInt16) {
				hkAabbTreeData::CastVolumeInput castInput;
				castInput.m_from.m_quad = from;
				castInput.m_to.m_quad = to;
				castInput.m_halfExtents.m_quad = halfExtents;
				castAabbInternal(job.m_aabbTree, aabb, castInput, &collector, 0.0f, nullptr, 0);
				hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::castAabb(job.m_aabbTreeInt16, aabb,
					castInput, &collector, 0.0f, nullptr, 0);
			}
			const hkKdTree* kdTree = job.m_kdTree;
			if (kdTree) {
				hkKdTreeAabbCastUtils::hkCastAabbInput castInput;
				castInput.m_from.m_quad = from;
				castInput.m_to.m_quad = to;
				castInput.m_halfExtents.m_quad = halfExtents;
				hkBool result;
				hkKdTreeAabbCastUtils::castAabb(&result, kdTree, castInput, &collector, 0.0f);
			}
		}
		command.m_numResultsOut = castCollector.m_numPoints.m_storage;
	}
	HK_TIMER_END();
	return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

hkCpuWorldLinearCastCollector::~hkCpuWorldLinearCastCollector() {}

// Havok's source takes the handle as an hkpTypedBroadPhaseHandle and its owner, the collidable. The exe inlines
// getOwner: the handle's address as untyped bytes plus its signed byte m_ownerOffset, returned as void*.
float hkCpuWorldLinearCastCollector::addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle,
	int32_t castIndex) {
	const hkpTypedBroadPhaseHandle* pHandle = static_cast<const hkpTypedBroadPhaseHandle*>(broadPhaseHandle);
	// EXE-INLINED 0x9B2C90
	const void* pOwner = reinterpret_cast<const char*>(pHandle) + pHandle->m_ownerOffset;
	return testCollidable(static_cast<const hkpCollidable*>(pOwner));
}

// The k-d tree primitive id is the collidable; the cast always reports a hit.
hkBool* hkCpuWorldLinearCastCollector::addMidPhaseHit(hkBool* arResult, hkPrimitiveId primitiveId) {
	testCollidable(reinterpret_cast<const hkpCollidable*>(primitiveId));
	arResult->m_bool = 1;
	return arResult;
}

hkBool* hkCpuWorldLinearCastCollector::hasHit(hkBool* arResult) {
	hkpFixedBufferCdPointCollector* m_castCollector = this->m_castCollector;
	bool v3 = m_castCollector->m_numPoints.m_storage == 0;
	bool v4 = m_castCollector->m_numPoints.m_storage < 0;
	arResult->m_bool = !v4 && !v3;
	return arResult;
}

// The point collector's early-out distance: the fraction of the cast still to search.
float hkCpuWorldLinearCastCollector::getHitFraction() {
	return m_castCollector->m_earlyOutDistance;
}

// A leaf of the world's tree holds its collidable in m_leafData.
float hkCpuWorldLinearCastCollector::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) {
	return testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
}

// A leaf of the world's tree holds its collidable in m_leafData.
float hkCpuWorldLinearCastCollector::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) {
	return testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
}

// The world's collidable with no shape and the cast's own collidable are skipped; a pair the filter rejects too.
// The exe inlines the shape's getType and the dispatcher's getLinearCastFunc.
float hkCpuWorldLinearCastCollector::testCollidable(const hkpCollidable* collidable) {
	const hkpShape* pShape = collidable->m_shape;
	if (!pShape) {
		return m_castCollector->m_earlyOutDistance;
	}
	if (m_collidable != collidable) {
		if (!m_filter->isCollisionEnabled(*m_collidable, *collidable).m_bool) {
			return m_castCollector->m_earlyOutDistance;
		}
		hkpCollisionDispatcher::LinearCastFunc linearCastFunc =
			m_input.m_dispatcher->getLinearCastFunc(m_shapeType, static_cast<hkpShapeType>(pShape->m_type.m_storage));
		linearCastFunc(*m_collidable, *collidable, m_input, *m_castCollector, nullptr);
	}
	return m_castCollector->m_earlyOutDistance;
}

void hkCpuWorldLinearCastCollector::operator delete(void* apMemory) {
	if (apMemory) {
		hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkCpuWorldLinearCastCollector));
	}
}
