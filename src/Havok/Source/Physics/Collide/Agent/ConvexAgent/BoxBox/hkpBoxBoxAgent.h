#pragma once

#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskBaseAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxManifold.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

#include <stdint.h>

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpContactMgr;

// The agent between two boxes, built by createBoxBoxAgent. It overrides every hkpCollisionAgent virtual function: the
// ones after linearCast keep hkpCollisionAgent's empty defaults, and linearCast is hkpIterativeLinearCastAgent's.
class hkpBoxBoxAgent : public hkpIterativeLinearCastAgent {
public:
	hkpBoxBoxAgent(hkpContactMgr* contactMgr);
	~hkpBoxBoxAgent() override;

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

	// The pair of boxes, given to collector when they intersect within the tolerance.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// The closest point of two boxes, reported to collector when within the tolerance.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Fills af with the box-box functions; the agent does not predict.
	static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);

	// Registers the agent for a box against a box.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc): the box-box agent, or the GSK agent where a box's
	// half extents' w times 1.999 is within the tolerance.
	static hkpCollisionAgent* createBoxBoxAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* contactMgr);

	hkpBoxBoxManifold m_manifold;
};
