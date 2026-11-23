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

// The agent between a sphere and a triangle.
class hkpSphereTriangleAgent : public hkpIterativeLinearCastAgent {
public:
    // The result of the sphere-triangle closest point query: no hit, or the triangle's face or an edge is closest.
    enum ClosestPointResult : int32_t { ST_CP_MISS, ST_CP_FACE, ST_CP_EDGE };

    hkpSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr);

    // The form that hkpSymmetricAgentLinearCast<T> constructs T with; the input is not read.
    hkpSphereTriangleAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput&, hkpContactMgr* mgr)
      : hkpSphereTriangleAgent(bodyA, bodyB, mgr) {}

    ~hkpSphereTriangleAgent() override;

    void getClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    ) override;
    void getPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    ) override;
    void processCollision(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& result
    ) override;
    void cleanup(hkpConstraintOwner& constraintOwner) override;

    // Havok SDK.
    static void staticGetPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    );

    // Registers the agent for a triangle against a sphere and the other way.
    static void registerAgent(hkpCollisionDispatcher* dispatcher);
    // The same two registrations through the dispatcher's second agent table.
    static void registerAgent2(hkpCollisionDispatcher* dispatcher);

    // Fills af with the functions for a sphere against a triangle.
    static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);
    // Fills af with the flipped agent's functions.
    static void symmetricInitAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);

    static void staticGetClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    );
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    static hkpCollisionAgent* createSphereTriangleAgent(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );
    // The closest point between bodyA's sphere and bodyB's triangle into cpoint, cache keeping the triangle's
    // precomputed data; ST_CP_MISS when they are further apart than the tolerance.
    static ClosestPointResult getClosestPoint(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCollideTriangleUtil::ClosestPointTriangleCache& cache,
        hkContactPoint& cpoint
    );
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    static hkpCollisionAgent* createTriangleSphereAgent(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );

    // hkContactPointId (an unsigned short).
    uint16_t m_contactPointId;
    hkpCollideTriangleUtil::ClosestPointTriangleCache m_closestPointTriangleCache;
};
