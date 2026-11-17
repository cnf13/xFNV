#pragma once

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpCollidable;
class hkpCollisionDispatcher;
class hkpContactMgr;
class hkpPhantomCallbackShape;

// The agent between a phantom callback shape and any shape. src models only its registration; the agent itself stays
// with the exe. It overrides every hkpCollisionAgent slot because the exe has no vtable for its bases: the slots 9 to
// 13 hold hkpCollisionAgent's empty defaults (0xD9FCA0 to 0xD9FCE0).
class hkpPhantomAgent : public hkpCollisionAgent {
public:
	hkpPhantomAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* contactMgr);
	~hkpPhantomAgent() override;

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& pointDetails) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyAin, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
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
		hkpCdPointCollector& pointDetails);
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);

	// Registers the agent for a phantom callback shape against every shape type, both ways.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK (cdecl; protected there): createPhantomAgent allocates and constructs the 0x24-byte agent (0xD42800).
	static hkpCollisionAgent* createPhantomAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* contactMgr);
	static hkpCollisionAgent* createNoPhantomAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	const hkpCollidable* m_collidableA;
	const hkpCollidable* m_collidableB;
	hkpPhantomCallbackShape* m_shapeA;
	hkpPhantomCallbackShape* m_shapeB;
	hkpShapeType m_bodyTypeA;
	hkpShapeType m_bodyTypeB;
};
inline constexpr fnvd::Vtable hkpPhantomAgent_vftable{};
