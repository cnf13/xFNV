#pragma once

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

#include <stdint.h>

class hkpCdBody;
class hkpCollisionInput;
class hkpContactMgr;

hkpCollisionAgent* createListAAgentFunc(const hkpCdBody& a1, const hkpCdBody& a2, const hkpCollisionInput& a3,
	hkpContactMgr* a4);

// The agent between a list shape and another shape.
class hkpListAgent : public hkpCollisionAgent {
public:
	hkpListAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* contactMgr);

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& listShapeBodyB,
		const hkpCollisionInput& input, hkpConstraintOwner& constraintOwner) override;
	void invalidateTim(const hkpCollisionInput& input) override;
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
	void removePoint(uint16_t idToRemove) override {}
	void commitPotential(uint16_t newId) override {}
	void createZombie(uint16_t idToBecomeZombie) override {}
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);

	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// The body forwards to hkpSymmetricAgentLinearCast<hkpShapeCollectionAgent>::staticGetClosestPoints.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Fills af with the functions for a list shape as body B.
	static void initAgentFuncInverse(hkpCollisionDispatcher::AgentFuncs& af);
	// Registers the agent for a list shape against every shape and the other way.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// Fills af with the functions for a list shape as body A: createListAAgent and the shape collection agent's
	// statics.
	static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static hkpCollisionAgent* createListAAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* contactMgr);

	hkpCollisionDispatcher* m_dispatcher;
	hkpAgent1nTrack m_agentTrack;
};
