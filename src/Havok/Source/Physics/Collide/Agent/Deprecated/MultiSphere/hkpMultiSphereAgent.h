#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;
class hkpLinearCastCollisionInput;

// The agent between a multi-sphere shape and any shape: one child agent per sphere. src declares its registration and
// the member functions it calls; the rest stays with the exe.
class hkpMultiSphereAgent : public hkpCollisionAgent {
public:
	// A sphere's shape key (hkpShapeKey, spelled uint32_t) and its child agent.
	struct KeyAgentPair {
		uint32_t m_key;
		hkpCollisionAgent* m_agent;
	};

	hkpMultiSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);
	~hkpMultiSphereAgent() override;

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	// Havok SDK.
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& info) override;

	// Slots 8 to 13 hold hkpCollisionAgent's defaults, which the exe has no base vtable to own.
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override {}

	void invalidateTim(const hkpCollisionInput& input) override {}
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override {}
	void removePoint(uint16_t idToRemove) override {}
	void commitPotential(uint16_t newId) override {}
	void createZombie(uint16_t idToBecomeZombie) override {}

	// Registers the agent for every shape type against a multi-sphere (flipped) and the other way.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK: the closest points of each sphere against body B.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);
	// Havok SDK: the linear cast of each sphere against body B.
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
	// Havok SDK: the penetrations of each sphere with body B.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// The child agents; the constructor points the array at the inline storage of 4.
	hkInplaceArray<KeyAgentPair, 4> m_agents;
};
inline constexpr fnvd::Vtable hkpMultiSphereAgent_vftable{};
