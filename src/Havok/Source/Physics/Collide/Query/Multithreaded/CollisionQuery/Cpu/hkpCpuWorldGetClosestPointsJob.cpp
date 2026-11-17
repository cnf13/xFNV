#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbQueryUtils.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkCpuWorldGetClosestPointsCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpWorldGetClosestPointsJob.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Internal/Collide/BroadPhase/3AxisSweep/hkp3AxisSweep.h"
#include "SDK/Windows/winbase.h"

// The SDK's hkAabbTreeQueryUtils<hkAabbTreeAabb16, hkUint16>::queryAabb
// with a collector (hkAabbTreeQueryUtils.h): queryAabbInternal (queryAabbInternal) with no array and no write iterator.
// hkCpuWorldGetClosestPointsJob passes its collector's Collector subobject.
static int32_t queryAabb(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree, const hkAabb& aabb,
	hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector* collector);

static int32_t queryAabb(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree, const hkAabb& aabb,
	hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector* collector) {
	// Returns 0 when the tree pointer is null.
	if (tree) {
		return queryAabbInternal(tree, aabb, collector, nullptr, nullptr, 0);
	}
	return 0;
}

// For each command, a fixed buffer collector over the command's result buffer collects the closest points of the
// command's collidable against every candidate the broad phase, the AABB tree and the kd-tree find in the collidable's
// AABB (grown by the job's tolerance less half the input's); then the queue hands out the next job.
hkJobQueue::JobStatus hkCpuWorldGetClosestPointsJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
	HK_TIMER_BEGIN("CollQueryWorldGetClosestPoints", nullptr);
	// The queue entry holds the job: its hkJob base is the job's.
	const hkpWorldGetClosestPointsJob& job =
		static_cast<const hkpWorldGetClosestPointsJob&>(static_cast<const hkJob&>(nextJobOut));
	const hkpProcessCollisionInput* input = job.m_collisionInput;

	hkpFixedBufferCdPointCollector castCollector(nullptr, 1);

	hkCpuWorldGetClosestPointsCollector broadPhaseCollector;
	// The SDK's downcast: the collector keeps the world's filter as the group filter it is.
	broadPhaseCollector.m_filter = static_cast<const hkpGroupFilter*>(input->m_filter);
	broadPhaseCollector.m_castCollector = &castCollector;
	// Copies the input's hkpCollisionInput part and its agent config member by member; the path and its cached length
	// are left unset, and the padding before m_aabb32Info is not copied.
	broadPhaseCollector.m_input.m_dispatcher = input->m_dispatcher;
	broadPhaseCollector.m_input.m_weldClosestPoints = input->m_weldClosestPoints;
	broadPhaseCollector.m_input.m_tolerance = input->m_tolerance;
	broadPhaseCollector.m_input.m_filter = input->m_filter;
	broadPhaseCollector.m_input.m_convexListFilter = input->m_convexListFilter;
	broadPhaseCollector.m_input.m_createPredictiveAgents = input->m_createPredictiveAgents;
	broadPhaseCollector.m_input.m_aabb32Info = input->m_aabb32Info;
	broadPhaseCollector.m_input.m_config = input->m_config;

	const hkpBroadPhase* broadphase = job.m_broadphase;
	const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* aabbTree = job.m_aabbTreeInt16;
	const hkKdTree* kdTree = job.m_kdTree;
	for (int32_t i = 0; i < job.m_numCommands; i++) {
		// The job keeps its commands const; the query writes their result counts.
		hkpWorldGetClosestPointsCommand& command = const_cast<hkpWorldGetClosestPointsCommand&>(job.m_commandArray[i]);

		// Placement new of the collector over this command's results.
		castCollector.~hkpFixedBufferCdPointCollector();
		::new (&castCollector) hkpFixedBufferCdPointCollector(command.m_results, command.m_resultsCapacity);

		const hkpCollidable* collidable = command.m_collidable;
		const hkpShape* shape = collidable->m_shape;
		hkAabb aabb;
		// The tolerance is computed in double precision with a double 0.5 and rounded to a float.
		const float tolerance = job.m_tolerance - job.m_collisionInput->m_tolerance * 0.5;
		shape->getAabb(*collidable->m_motion.pMotion, tolerance, aabb);

		broadPhaseCollector.m_input.m_tolerance = job.m_tolerance;
		broadPhaseCollector.m_collidable = command.m_collidable;
		broadPhaseCollector.m_shapeType = static_cast<hkpShapeType>(shape->m_type.m_storage);

		if (broadphase) {
			// The downcast is needed because the collector query is hkp3AxisSweep's own virtual, not one of
			// hkpBroadPhase's.
			static_cast<const hkp3AxisSweep*>(broadphase)->querySingleAabbWithCollector(aabb, &broadPhaseCollector);
		}
		if (aabbTree) {
			queryAabb(aabbTree, aabb, &broadPhaseCollector);
		}
		if (kdTree) {
			hkKdTreeAabbQueryUtils::queryAabbIterative(kdTree, aabb, &broadPhaseCollector);
		}
		command.m_numResultsOut = castCollector.m_numPoints.m_storage;
	}
	HK_TIMER_END();
	return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}
