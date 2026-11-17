#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Util/LinearCast/hkpIterativeLinearCastAgent.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCollisionInput;
class hkpContactMgr;
struct hkpExtendedGskOut;
class hkpGsk;

// The base of the GSK agents between two convex shapes: the GSK cache and the cached separating normal with the time it
// was taken at.
class hkpGskBaseAgent : public hkpIterativeLinearCastAgent {
public:
    hkpGskBaseAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr);
    ~hkpGskBaseAgent() override;

    void getPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    ) override;
    void getClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    ) override;
    void linearCast(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& collector,
        hkpCdPointCollector* startCollector
    ) override;
    void processCollision(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& result
    ) override;
    void cleanup(hkpConstraintOwner& constraintOwner) override;

    void updateShapeCollectionFilter(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpConstraintOwner& constraintOwner
    ) override {}

    void invalidateTim(const hkpCollisionInput& input) override;
    void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
    void removePoint(uint16_t idToRemove) override {}
    void commitPotential(uint16_t newId) override {}
    void createZombie(uint16_t idToBecomeZombie) override {}

protected:
    // Computes the closest points of the two convex bodies from the cached simplex into output; the result is true when
    // they are within input's tolerance.
    hkBool* _getClosestPoint(
        hkBool* apResult,
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpExtendedGskOut& output
    );

public:
    // Havok SDK: the bodies' penetrations, without an agent.
    static void staticGetPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    );
    // Havok SDK: the bodies' closest points, without an agent.
    static void staticGetClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    );
    // Havok SDK: the separating normal in world space, its w the distance less both radii.
    static void calcSeparatingNormal(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        float earlyOutTolerance,
        hkpGsk& gsk,
        hkVector4& separatingNormalOut
    );

    hkpGskCache m_cache;
    float m_timeOfSeparatingNormal;
    float m_allowedPenetration;
    hkVector4 m_separatingNormal;
};
