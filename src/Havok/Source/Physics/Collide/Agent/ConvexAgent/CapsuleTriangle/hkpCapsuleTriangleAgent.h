#pragma once

#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkContactPoint;
class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpContactMgr;
class hkpLinearCastCollisionInput;

// Computes the closest points of the capsule (bodyA) against the triangle (bodyB): reads both bodies' shape and
// transform and the input's tolerance, and passes cache, searchManifold and points on to hkCollideCapsuleUtilCapsVsTri.
void calcContactPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
	hkpCollideTriangleUtil::PointTriangleDistanceCache& cache, int32_t searchManifold, hkContactPoint* points);

// The agent between a capsule and a triangle.
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
	// The same two registrations through the dispatcher's second agent table.
	static void registerAgent2(hkpCollisionDispatcher* dispatcher);

	// Fills af with the functions for a capsule against a triangle.
	static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);
	// Fills af with the flipped agent's functions.
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
