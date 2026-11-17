#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkContactPoint;
class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;

// Core of staticGetClosestPoints: both capsules' end points are transformed to world space and the closest point is
// found.
hkResult getClosestPointCapsuleCapsule(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkContactPoint& cpoint
);
// Computes processCollision's manifold: the three contact points' distances are set to the tolerance, both capsules'
// end points are transformed to world space and the manifold is found.
int32_t
getManifold(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input, hkContactPoint* points);

// The agent between two capsules.
class hkpCapsuleCapsuleAgent : public hkpIterativeLinearCastAgent {
public:
    hkpCapsuleCapsuleAgent(hkpContactMgr* mgr);
    ~hkpCapsuleCapsuleAgent() override;

    void processCollision(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& result
    ) override;
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
    void cleanup(hkpConstraintOwner& constraintOwner) override;

    // Registers the agent for a capsule against a capsule.
    static void registerAgent(hkpCollisionDispatcher* dispatcher);

    static void staticGetPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    );
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    static hkpCollisionAgent* createCapsuleCapsuleAgent(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );
    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static void staticGetClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    );

    // The contact point ids are hkContactPointId (an unsigned short).
    uint16_t m_contactPointId[3];
};
