#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpLinearCastCollisionInput;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

// The base of the agents that collide one pair of shapes.
class hkpCollisionAgent : public hkReferencedObject {
public:
    hkpCollisionAgent(hkpContactMgr* contactMgr) : m_contactMgr(contactMgr) {}

    virtual void getPenetrations(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdBodyPairCollector& collector
    ) = 0;
    virtual void getClosestPoints(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpCdPointCollector& collector
    ) = 0;
    virtual void linearCast(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& collector,
        hkpCdPointCollector* startCollector
    ) = 0;
    virtual void processCollision(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& result
    ) = 0;
    // The constraint owner is an hkCollisionConstraintOwner, Havok's name for hkpConstraintOwner here.
    virtual void cleanup(hkpConstraintOwner& constraintOwner) = 0;

    virtual void updateShapeCollectionFilter(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpConstraintOwner& constraintOwner
    ) {}

    virtual void invalidateTim(const hkpCollisionInput& input);
    // The times are hkTime (a float).
    virtual void warpTime(float oldTime, float newTime, const hkpCollisionInput& input);
    // The ids are hkContactPointId (an unsigned short).
    virtual void removePoint(uint16_t idToRemove);
    virtual void commitPotential(uint16_t newId);
    virtual void createZombie(uint16_t idTobecomeZombie);
    ~hkpCollisionAgent() override;

    hkpContactMgr* m_contactMgr;
};
