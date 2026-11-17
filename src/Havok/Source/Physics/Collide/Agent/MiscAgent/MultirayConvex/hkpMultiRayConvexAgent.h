#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;

// The agent between a multi-ray and a convex shape. src models its registration and the static query the flipped
// agent calls; the agent itself stays with the exe. It overrides every hkpCollisionAgent slot because the exe has no
// vtable for its bases: the slots 9 to 13 hold copies of hkpCollisionAgent's empty defaults (0xD9FCA0 to 0xD9FCE0),
// and slot 5 (linearCast) a copy of hkpIterativeLinearCastAgent's (0xDAF540). Its destructor (0xD37D30) is its own:
// m_contactInfo needs freeing.
class hkpMultiRayConvexAgent : public hkpIterativeLinearCastAgent {
public:
	// The contact point one ray produced (protected in the SDK).
	struct ContactPointInfo {
		uint16_t m_contactPointId;
	};

	hkpMultiRayConvexAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);
	~hkpMultiRayConvexAgent() override;

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;

	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override {}

	void invalidateTim(const hkpCollisionInput& input) override {}
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override {}
	void removePoint(uint16_t idToRemove) override {}
	void commitPotential(uint16_t newId) override {}
	void createZombie(uint16_t idToBecomeZombie) override {}

	// Havok SDK.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// Havok SDK.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Havok SDK (cdecl; protected there).
	static hkpCollisionAgent* createMultiRayConvexAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static hkpCollisionAgent* createConvexMultiRayAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	// Registers the agent for a convex shape against a multi-ray and the other way.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	hkInplaceArray<ContactPointInfo, 4> m_contactInfo;
};
inline constexpr fnvd::Vtable hkpMultiRayConvexAgent_vftable{};
