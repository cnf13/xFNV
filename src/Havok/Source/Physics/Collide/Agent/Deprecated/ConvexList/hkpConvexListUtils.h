#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"

class hkpCdBody;

// What a convex list agent saves of a process output before it collides one child, to put it back when the child's
// results are dropped: the first free contact point, the welding information and the time of impact.
struct hkpProcessCollisionOutputBackup {
    // Callers construct the backup again from an output.
    hkpProcessCollisionOutputBackup() = default;
    hkpProcessCollisionOutputBackup(const hkpProcessCollisionOutput& output);

    // Puts the saved state back into output: when its time of impact changed, removes the new
    // one through mgr and restores m_toi; then the first free point and the potential contacts.
    void rollbackOutput(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        hkpProcessCollisionOutput& output,
        hkpContactMgr* mgr
    );

    hkpProcessCdPoint* m_firstPoint;
    hkpProcessCollisionOutput::PotentialInfo m_weldingInformation;
    hkpProcessCollisionOutput::ToiInfo m_toi;
};

// A contact manager that forwards the contact points of a convex list pair to another manager, mapping each point back
// to the sub shape (child of the convex list) it belongs to; a point that spans several children is dropped and flagged
// in m_invalidPointHit.
class hkpMapPointsToSubShapeContactMgr : public hkpContactMgr {
public:
    uint16_t addContactPointImpl(
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& output,
        const hkpGskCache* contactCache,
        hkContactPoint& cp
    ) override;

    hkResult reserveContactPointsImpl(int32_t numPoints) override {
        return m_contactMgr->reserveContactPoints(numPoints);
    }

    void removeContactPointImpl(uint16_t cpId, hkpConstraintOwner& constraintOwner) override {
        m_contactMgr->removeContactPoint(cpId, constraintOwner);
    }

    void processContactImpl(
        const hkpCollidable& a,
        const hkpCollidable& b,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionData& collisionData
    ) override {
        m_contactMgr->processContact(a, b, input, collisionData);
    }

    void cleanup() override {
        m_contactMgr->cleanup();
    }
    ToiAccept addToiImpl(
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& output,
        float toi,
        hkContactPoint& cp,
        const hkpGskCache* gskCache,
        float& projectedVelocity,
        hkpContactPointProperties& propertiesOut
    ) override;

    void removeToiImpl(hkpConstraintOwner& constraintOwner, hkpContactPointProperties& properties) override {
        m_contactMgr->removeToi(constraintOwner, properties);
    }

    hkpContactMgr* m_contactMgr;
    hkBool m_invalidPointHit;
};
