#pragma once

#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;

// The agent between a sphere and a box. src models its registration and the static query the flipped agent calls;
// the agent itself stays with the exe. It overrides every hkpCollisionAgent slot because the exe has no vtable for
// its bases: the slots 8 to 13 hold hkpCollisionAgent's empty defaults (0xD3DD00, 0xD9FCA0 to 0xD9FCE0), and slot 5
// (linearCast) is hkpIterativeLinearCastAgent's (0xDAF540).
class hkpSphereBoxAgent : public hkpIterativeLinearCastAgent {
public:
	hkpSphereBoxAgent(hkpContactMgr* mgr);
	~hkpSphereBoxAgent() override;

	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	// The exe's slot holds the base's body (0xDAF540); declared here because the exe has no base vtable that would let
	// vtable-check confirm the inheritance.
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
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// Havok SDK.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// Registers the agent for a box against a sphere (flipped) and a sphere against a box.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK (cdecl; protected there): the agent of a sphere A and a box B (0xD3C380) and of a box A and a sphere B
	// (0xD3C400); each allocates a 0x10-byte agent.
	static hkpCollisionAgent* createSphereBoxAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	static hkpCollisionAgent* createBoxSphereAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	uint16_t m_contactPointId;
};
inline constexpr fnvd::Vtable hkpSphereBoxAgent_vftable{};
