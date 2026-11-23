#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"

struct hkpBodyVelocity;
class hkpConstraintInstance;
class hkpEntity;
struct hkpSimpleConstraintInfoInitInput;
struct hkpToiEvent;
class hkpWorld;

// The base of the contact managers that turn contacts into dynamics (contact constraints) in a world.
class hkpDynamicsContactMgr : public hkpContactMgr {
public:
    hkpDynamicsContactMgr() = default;
    // Takes the manager's type; m_world is set by the caller.
    hkpDynamicsContactMgr(hkpContactMgr::Type type) : hkpContactMgr(type) {}

    // Default bodies; hkpSimpleConstraintContactMgr and hkpReportContactMgr override every one of them. The ids are
    // hkContactPointId (an unsigned short).
    virtual hkpContactPointProperties* getContactPointProperties(uint16_t id) {
        return nullptr;
    }
    virtual hkContactPoint* getContactPoint(uint16_t id) {
        return nullptr;
    }
    virtual void getAllContactPointIds(hkArray<uint16_t>& contactPointIds) const {}
    virtual hkpContactMgr::Type getType() const = 0;

    virtual void toiCollisionResponseBeginCallback(
        const hkContactPoint& cp,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    ) {}

    virtual void toiCollisionResponseEndCallback(
        const hkContactPoint& cp,
        float impulseApplied,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    ) {}

    virtual hkpConstraintInstance* getConstraintInstance() {
        return nullptr;
    }
    virtual void fireCallbacksForEarliestToi(hkpToiEvent& event, float& rotateNormal) {}
    // The time of impact is an hkTime (a float).
    virtual void confirmToi(hkpToiEvent& event, float rotateNormal, hkArray<hkpEntity*>& outToBeActivated) {}

    hkpWorld* m_world;
};
