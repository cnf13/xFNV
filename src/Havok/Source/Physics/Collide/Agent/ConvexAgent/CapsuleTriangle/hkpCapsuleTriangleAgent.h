#pragma once

#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkContactPoint;
class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpContactMgr;
class hkpLinearCastCollisionInput;

// Not named in the PDB or IDA; name chosen here. The capsule (bodyA) against the triangle (bodyB) closest points
// (cdecl, ret 0, 0xD399F0; no exe caller): the exe reads both bodies' shape (+0) and transform (+8), input's
// tolerance (+8) and passes cache, searchManifold and points on to hkCollideCapsuleUtilCapsVsTri (0xCD6520).
void calcContactPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCollideTriangleUtil::PointTriangleDistanceCache& cache, int32_t searchManifold, hkContactPoint* points);

// The agent between a capsule and a triangle. src models its registration and the static query the flipped agent
// calls; the agent itself stays with the exe.
class hkpCapsuleTriangleAgent : public hkpIterativeLinearCastAgent {
public:
	// The result of getClosestPoint.
	enum ClosestPointResult : int32_t {
		ST_CP_MISS = 0x0,
		ST_CP_HIT = 0x1,
	};

	hkpCapsuleTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);
	~hkpCapsuleTriangleAgent() override;

	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void cleanup(hkpConstraintOwner& constraintOwner) override;

	// Havok SDK: the closest points of a capsule and a triangle, reported to collector.
	static void staticGetClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector);
	// Reports the pair to collector when the capsule and the triangle penetrate.
	static void staticGetPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector);

	// Registers the agent for a triangle against a capsule and the other way.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// The same two registrations through the dispatcher's second agent table (cdecl, 0xD3AC20; not named in the PDB
	// or IDA, name chosen here).
	static void registerAgent2(hkpCollisionDispatcher* dispatcher);

	// Fills af with the functions for a capsule against a triangle (cdecl, 0xD3AB60; not named in the PDB or IDA,
	// name chosen here).
	static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);
	// Fills af with the flipped agent's functions (cdecl, 0xD3AB90; not named in the PDB or IDA, name chosen here).
	static void symmetricInitAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);
	// The closest point of a capsule and a triangle, cached in cache.
	static ClosestPointResult getClosestPoint(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpCollideTriangleUtil::PointTriangleDistanceCache& cache,
		hkContactPoint& cpoint);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc). A new agent for capsule A and triangle B.
	static hkpCollisionAgent* createCapsuleTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc). A new flipped agent for triangle A and capsule B.
	static hkpCollisionAgent* createTriangleCapsuleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);

	// The contact point ids are hkContactPointId (an unsigned short).
	uint16_t m_contactPointId[3];
	hkpCollideTriangleUtil::PointTriangleDistanceCache m_triangleCache;
};

inline constexpr fnvd::Vtable hkpCapsuleTriangleAgent_vftable{};
