#pragma once

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpCollisionDispatcher;

// The agent between a transform shape and another shape: it runs m_childAgent on the transformed child shape. Only some
// of its members are declared.
class hkpTransformAgent : public hkpCollisionAgent {
public:
	// Creates the child agent for the transform shape's child under the shape's transform.
	hkpTransformAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);
	~hkpTransformAgent() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	// Havok SDK.
	void getPenetrations(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& pointDetails) override;
	void linearCast(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;
	void updateShapeCollectionFilter(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override;
	void invalidateTim(const hkpCollisionInput& input) override;
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
	void removePoint(uint16_t idToRemove) override;
	void commitPotential(uint16_t newId) override;
	void createZombie(uint16_t idTobecomeZombie) override;

	// Havok SDK.
	static void staticGetPenetrations(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);
	static void staticGetClosestPoints(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& pointDetails);
	static void staticLinearCast(const hkpCdBody& bodyAin, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
	// Registers the agent for a transform shape against every shape type, both ways.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// Protected in the SDK.
	static hkpCollisionAgent* createTransformAAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static hkpCollisionAgent* createTransformBAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	hkpCollisionAgent* m_childAgent;
};
