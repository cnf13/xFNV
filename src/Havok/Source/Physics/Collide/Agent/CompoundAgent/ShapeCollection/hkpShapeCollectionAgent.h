#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"

#include <stdint.h>

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpLinearCastCollisionInput;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

// Maintains a child collision agent for each active shape-collection key.
class hkpShapeCollectionAgent : public hkpCollisionAgent {
public:
	struct KeyAgentPair {
		uint32_t m_key;
		hkpCollisionAgent* m_agent;
	};

	hkpShapeCollectionAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);
	~hkpShapeCollectionAgent() override;

	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc,vtable).
	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc,vtable).
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	void cleanup(hkpConstraintOwner& info) override;
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc,vtable).
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override;
	void invalidateTim(const hkpCollisionInput&) override;
	void warpTime(float, float, const hkpCollisionInput&) override;
	void removePoint(uint16_t) override {}
	void commitPotential(uint16_t) override {}
	void createZombie(uint16_t) override {}

	// Registers the agent for a shape collection against every shape type, both ways.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);
	// Havok SDK signature (hkpCollide.lib, tied by reloc).
	static hkpCollisionAgent* createListAAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by reloc).
	static hkpCollisionAgent* createListBAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	hkInplaceArray<KeyAgentPair, 4> m_agents;
};
