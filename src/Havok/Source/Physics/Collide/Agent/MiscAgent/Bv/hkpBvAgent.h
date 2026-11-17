#pragma once

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpCdBody;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;

// The agent between a shape with a bounding volume and another shape: it runs m_boundingVolumeAgent on the bounding
// volume first and m_childAgent on the shape when that collides. It overrides every hkpCollisionAgent virtual function.
class hkpBvAgent : public hkpCollisionAgent {
public:
    hkpBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr);
    ~hkpBvAgent() override;

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
        hkpCdPointCollector& pointDetails,
        hkpCdPointCollector* startCollector
    ) override;
    void processCollision(
        const hkpCdBody& bodyAin,
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
    ) override;
    void invalidateTim(const hkpCollisionInput& input) override;
    void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
    void removePoint(uint16_t idToRemove) override;
    void commitPotential(uint16_t newId) override;
    void createZombie(uint16_t idTobecomeZombie) override;

    // Havok SDK.
    static void staticGetPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    );
    static void staticGetClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    );
    static void staticLinearCast(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& pointDetails,
        hkpCdPointCollector* startCollector
    );
    // Registers the agent for a BV shape against every shape type, both ways.
    static void registerAgent(hkpCollisionDispatcher* dispatcher);
    // Protected in the SDK.
    static hkpCollisionAgent*
    createBvShapeAgent(const hkpCdBody& A, const hkpCdBody& B, const hkpCollisionInput& input, hkpContactMgr* mgr);
    static hkpCollisionAgent*
    createShapeBvAgent(const hkpCdBody& A, const hkpCdBody& B, const hkpCollisionInput& input, hkpContactMgr* mgr);

    hkpCollisionAgent* m_boundingVolumeAgent;
    hkpCollisionAgent* m_childAgent;
};
