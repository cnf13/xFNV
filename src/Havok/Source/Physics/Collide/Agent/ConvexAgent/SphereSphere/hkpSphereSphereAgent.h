#pragma once

#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The agent between two spheres. It overrides every hkpCollisionAgent slot because the exe has no vtable for its
// bases: the slots 8 to 13 hold hkpCollisionAgent's empty defaults (0xD3DD00, 0xD9FCA0 to 0xD9FCE0), and slot 5
// (linearCast) is hkpIterativeLinearCastAgent's (0xDAF540).
class hkpSphereSphereAgent : public hkpIterativeLinearCastAgent {
public:
	hkpSphereSphereAgent(hkpContactMgr* mgr);
	~hkpSphereSphereAgent() override;

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

	// Fills af with the sphere-sphere functions.
	static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);

	// Havok SDK.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// Registers the agent for a sphere against a sphere.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK.
	static void registerAgent2(hkpCollisionDispatcher* dispatcher);

	// Havok SDK (cdecl; protected there).
	static hkpCollisionAgent* createSphereSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* contactMgr);

	uint16_t m_contactPointId;
};
inline constexpr fnvd::Vtable hkpSphereSphereAgent_vftable{};
