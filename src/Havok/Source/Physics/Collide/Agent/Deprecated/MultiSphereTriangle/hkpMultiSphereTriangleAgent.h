#pragma once

#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;

// The agent between a multi-sphere and a triangle. src models its registration and the query the flipped agent
// calls; the agent itself stays with the exe. It overrides every hkpCollisionAgent slot because the exe has no vtable
// for its bases: the slots 8 to 13 hold hkpCollisionAgent's empty defaults (0xD3DD00, 0xD9FCA0 to 0xD9FCE0), and
// slot 5 (linearCast) is hkpIterativeLinearCastAgent's (0xDAF540).
class hkpMultiSphereTriangleAgent : public hkpIterativeLinearCastAgent {
public:
	hkpMultiSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr);

	// The SDK's form, which hkpSymmetricAgentLinearCast<T> constructs T with; the exe's inlined construction
	// (Construct above) does not read the input.
	hkpMultiSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput&,
		hkpContactMgr* mgr)
		: hkpMultiSphereTriangleAgent(bodyA, bodyB, mgr) {}

	~hkpMultiSphereTriangleAgent() override;

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
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Registers the agent for a triangle against a multi-sphere and the other way.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK (cdecl; protected there).
	static hkpCollisionAgent* createMultiSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* contactMgr);
	static hkpCollisionAgent* createTriangleMultiSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* contactMgr);

	hkpCollideTriangleUtil::ClosestPointTriangleCache m_closestPointTriangleCache;
	// hkContactPointId (an unsigned short) per sphere.
	uint16_t m_contactPointId[8];
};
inline constexpr fnvd::Vtable hkpMultiSphereTriangleAgent_vftable{};
