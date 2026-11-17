#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpMoppAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/CapsuleTriangle/hkpCapsuleTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereCapsule/hkpSphereCapsuleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/SphereTriangle/hkpSphereTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphere/hkpMultiSphereAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphereTriangle/hkpMultiSphereTriangleAgent.h"
#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkpHeightFieldAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCdPoint.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Bv/hkpBvAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/MultirayConvex/hkpMultiRayConvexAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Transform/hkpTransformAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"

class hkpConstraintOwner;
// The agent used for a flipped shape pair: each query swaps bodies A and B and runs TAgent's, through a flip collector
// for the queries that report. hkpSymmetricAgent<TAgent> derives from it and adds the linear cast.
class hkpContactMgr;
class hkpProcessCollisionInput;

// Passes the points of a query run with bodies A and B swapped on to m_collector, flipped back.
class hkpSymmetricAgentFlipCollector : public hkpCdPointCollector {
public:
	hkpSymmetricAgentFlipCollector(hkpCdPointCollector& collector) : m_collector(&collector) {}

	void addCdPoint(const hkpCdPoint& point) override;

	// A pointer rather than a reference so the in-place constructor can store it.
	hkpCdPointCollector* m_collector;
};

// Passes the points of a linear cast run with bodies A and B swapped on to m_collector, flipped back and moved along
// m_path.
class hkpSymmetricAgentFlipCastCollector : public hkpCdPointCollector {
public:
	hkpSymmetricAgentFlipCastCollector(const hkVector4& path, hkpCdPointCollector& collector)
		: m_path(path), m_collector(&collector) {}

	void addCdPoint(const hkpCdPoint& point) override;

	hkVector4 m_path;
	// A pointer rather than a reference so it can be stored in place.
	hkpCdPointCollector* m_collector;
};

// Passes the body pairs of a query run with bodies A and B swapped on to m_collector, flipped back.
class hkpSymmetricAgentFlipBodyCollector : public hkpCdBodyPairCollector {
public:
	hkpSymmetricAgentFlipBodyCollector() = default;
	hkpSymmetricAgentFlipBodyCollector(hkpCdBodyPairCollector& collector) : m_collector(&collector) {}

	void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) override;

	// DeletingDestruct's free (a heap block of the class's size).
	static void operator delete(void* apMemory);

	// A pointer rather than a reference so the in-place constructor can store it.
	hkpCdBodyPairCollector* m_collector;
};

template <class TAgent> class hkpSymmetricAgentLinearCast : public TAgent {
public:
	// The SDK's constructor: TAgent's on the swapped bodies.
	hkpSymmetricAgentLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr)
		: TAgent(bodyB, bodyA, input, mgr) {}

	explicit hkpSymmetricAgentLinearCast(hkpContactMgr* mgr) : TAgent(mgr) {}

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override;

	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
};

// Calls TAgent's own query directly with the bodies swapped.
template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	alignas(hkpSymmetricAgentFlipBodyCollector) uint8_t kFlippedMemory[sizeof(hkpSymmetricAgentFlipBodyCollector)];
	hkpSymmetricAgentFlipBodyCollector* pFlipped = ::new (kFlippedMemory) hkpSymmetricAgentFlipBodyCollector(collector);
	TAgent::getPenetrations(bodyB, bodyA, input, *pFlipped);
}

// The filter update reports nothing, so the bodies are swapped without a collector.
template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpConstraintOwner& constraintOwner) {
	TAgent::updateShapeCollectionFilter(bodyB, bodyA, input, constraintOwner);
}

// Calls TAgent's own query directly with the bodies swapped.
template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	alignas(hkpSymmetricAgentFlipCollector) uint8_t kFlippedMemory[sizeof(hkpSymmetricAgentFlipCollector)];
	hkpSymmetricAgentFlipCollector* pFlipped = ::new (kFlippedMemory) hkpSymmetricAgentFlipCollector(collector);
	TAgent::getClosestPoints(bodyB, bodyA, input, *pFlipped);
}

template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) {
	alignas(hkpSymmetricAgentFlipBodyCollector) uint8_t kFlippedMemory[sizeof(hkpSymmetricAgentFlipBodyCollector)];
	hkpSymmetricAgentFlipBodyCollector* pFlipped = ::new (kFlippedMemory) hkpSymmetricAgentFlipBodyCollector(collector);
	TAgent::staticGetPenetrations(bodyB, bodyA, input, *pFlipped);
}

template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpCollisionInput& input, hkpCdPointCollector& collector) {
	alignas(hkpSymmetricAgentFlipCollector) uint8_t kFlippedMemory[sizeof(hkpSymmetricAgentFlipCollector)];
	hkpSymmetricAgentFlipCollector* pFlipped = ::new (kFlippedMemory) hkpSymmetricAgentFlipCollector(collector);
	TAgent::staticGetClosestPoints(bodyB, bodyA, input, *pFlipped);
}

// TAgent's processCollision with bodies A and B swapped, then the contact points it added and a new time of impact
// flipped back (hkContactPoint::setFlipped and ToiInfo::flip).
template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpProcessCollisionInput& input, hkpProcessCollisionOutput& result) {
	hkpProcessCdPoint* pp = result.m_firstFreeContactPoint.m_storage;
	const float oldToi = result.m_toi.m_time.m_storage;
	TAgent::processCollision(bodyB, bodyA, input, result);
	for (; pp < result.m_firstFreeContactPoint.m_storage; ++pp) {
		hkVector4 dist;
		dist.setBroadcast(pp->m_contact.m_separatingNormal, 3);
		pp->m_contact.m_position.setAddMul4(pp->m_contact.m_position, pp->m_contact.m_separatingNormal, dist);
		pp->m_contact.m_separatingNormal.setNeg3(pp->m_contact.m_separatingNormal);
	}
	if (oldToi != result.m_toi.m_time.m_storage) {
		result.m_toi.m_contactPoint.m_separatingNormal.setNeg3(result.m_toi.m_contactPoint.m_separatingNormal);
		result.m_toi.m_gskCache.flip();
	}
}

// TAgent's linear cast with bodies A and B swapped, along the negated path, through flip cast collectors.
template <class TAgent>
void hkpSymmetricAgentLinearCast<TAgent>::staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
	const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) {
	hkpLinearCastCollisionInput flippedInput = input;
	flippedInput.m_path.setNeg4(input.m_path);
	hkpSymmetricAgentFlipCastCollector flip(input.m_path, collector);
	if (startCollector) {
		hkpSymmetricAgentFlipCastCollector startFlip(input.m_path, *startCollector);
		TAgent::staticLinearCast(bodyB, bodyA, flippedInput, flip, &startFlip);
	} else {
		TAgent::staticLinearCast(bodyB, bodyA, flippedInput, flip, nullptr);
	}
}
