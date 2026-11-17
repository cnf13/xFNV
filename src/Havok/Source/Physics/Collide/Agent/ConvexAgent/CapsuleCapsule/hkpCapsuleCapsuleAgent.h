#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkContactPoint;
class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;

// staticGetClosestPoints' core out of line: both capsules' end points transformed to world space and the closest
// point found. bodyA in eax and bodyB in esi (each read +0 shape, +8 transform: 0xD3B8EE, 0xD3B9C8), input and
// cpoint on the stack (input +8 m_tolerance at 0xD3BA9B; cpoint passed on to 0xCD6260), popped by the caller (ret
// 0); eax is the callee's hkResult. No callers.
hkResult getClosestPointCapsuleCapsule(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkContactPoint& cpoint);
// processCollision's manifold out of line: the three contact points' distances set to the tolerance, both capsules'
// end points transformed to world space and the manifold found. bodyA in eax, bodyB on the stack (each read +0
// shape, +8 transform: 0xD3B6FD, 0xD3B7E8), input in ecx (+8 m_tolerance, 0xD3B6F9), points in esi (three
// hkContactPoint, distances at +0x1C/+0x3C/+0x5C, passed on to 0xCD58B0), ret 0; eax is the callee's result. No
// callers.
int32_t getManifold(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkContactPoint* points);

// The agent between two capsules. src models its registration; the agent itself stays with the exe.
class hkpCapsuleCapsuleAgent : public hkpIterativeLinearCastAgent {
public:
	hkpCapsuleCapsuleAgent(hkpContactMgr* mgr);
	~hkpCapsuleCapsuleAgent() override;

	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;

	// Registers the agent for a capsule against a capsule.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);

	// Havok SDK (cdecl). IDA name _sub_D3B420.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static hkpCollisionAgent* createCapsuleCapsuleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by reloc).
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);

	// The contact point ids are hkContactPointId (an unsigned short).
	uint16_t m_contactPointId[3];
};
inline constexpr fnvd::Vtable hkpCapsuleCapsuleAgent_vftable{};
