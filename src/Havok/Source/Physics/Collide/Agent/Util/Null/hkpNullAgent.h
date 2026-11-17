#pragma once

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpCollisionInput;
class hkpContactMgr;

// The agent for shape pairs that do not collide: it adds no data to hkpCollisionAgent. Its code stays with the exe;
// it overrides every hkpCollisionAgent slot because the exe has no hkpCollisionAgent vtable to inherit from.
class hkpNullAgent : public hkpCollisionAgent {
public:
	hkpNullAgent();
	~hkpNullAgent() override;

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override {}

	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& pointDetails) override {}

	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override {}

	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override {}

	void cleanup(hkpConstraintOwner& info) override {}

	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpConstraintOwner& constraintOwner) override {}

	void invalidateTim(const hkpCollisionInput& input) override {}
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override {}
	void removePoint(uint16_t idToRemove) override {}
	void commitPotential(uint16_t newId) override {}
	void createZombie(uint16_t idToBecomeZombie) override {}

	// The one shared null agent, hkNullAgentInstance.
	static hkpNullAgent* getNullAgent();
	// Havok SDK signature (hkpCollide.lib, tied by reloc): the shared null agent, whatever the bodies.
	static hkpCollisionAgent* createNullAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
};
inline constexpr fnvd::Vtable hkpNullAgent_vftable{};

// The shared null agent getNullAgent and createNullAgent hand out (the PDB's static in hkpNullAgent.cpp).
extern hkpNullAgent hkNullAgentInstance;
