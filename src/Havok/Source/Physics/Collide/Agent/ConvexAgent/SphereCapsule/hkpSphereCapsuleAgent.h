#pragma once

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

// The agent between a sphere and a capsule.
class hkpSphereCapsuleAgent : public hkpIterativeLinearCastAgent {
public:
    // The result of the sphere-capsule closest point query.
    enum ClosestPointResult : int32_t {
        ST_CP_MISS,
        ST_CP_HIT,
    };

    hkpSphereCapsuleAgent(hkpContactMgr* mgr);
    ~hkpSphereCapsuleAgent() override;

    void getPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    ) override;
    // The closest points of a sphere and a capsule.
    void getClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
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

    // Havok SDK.
    static void staticGetClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    );

    // Registers the agent for a capsule against a sphere and the other way.
    static void registerAgent(hkpCollisionDispatcher* dispatcher);
    // Writes the closest point of the sphere bodyA and the capsule bodyB into cpoint; returns ST_CP_HIT when nearer
    // than the radii and the tolerance.
    static ClosestPointResult getClosestPoint(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkContactPoint& cpoint
    );
    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static hkpCollisionAgent* createSphereCapsuleAgent(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );
    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static hkpCollisionAgent* createCapsuleSphereAgent(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );

    // The contact point id is a hkContactPointId (an unsigned short), 0xFFFF for none; it is removed from the contact
    // manager unless it is 0xFFFF.
    uint16_t m_contactPointId;
};
