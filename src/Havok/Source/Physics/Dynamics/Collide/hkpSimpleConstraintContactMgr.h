#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <stdint.h>

class hkContactPoint;
struct hkpBodyVelocity;
class hkpCdBody;
class hkpCollidable;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactPointProperties;
class hkpGskCache;
class hkpProcessCollisionData;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpRigidBody;
struct hkpSimpleConstraintInfoInitInput;
class hkpWorld;

void sub_CFDB40();

// Maintains the contact constraint and callbacks for one rigid-body pair.
class hkpSimpleConstraintContactMgr : public hkpDynamicsContactMgr {
public:
    class Factory : public hkpContactMgrFactory {
    public:
        Factory(hkpWorld* mgr);

        // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
        hkpContactMgr*
        createContactMgr(const hkpCollidable& a, const hkpCollidable& b, const hkpCollisionInput& input) override;

        hkpWorld* m_world;
    };

    hkpSimpleConstraintContactMgr(hkpWorld* sm, hkpRigidBody* bodyA, hkpRigidBody* bodyB);
    ~hkpSimpleConstraintContactMgr() override;

    const hkClass* getClassType() const override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    // Havok SDK signature (hkpDynamics.lib, tied by vtable).
    uint16_t addContactPointImpl(
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& output,
        const hkpGskCache* contactCache,
        hkContactPoint& cp
    ) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    hkResult reserveContactPointsImpl(int32_t numPoints) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    void removeContactPointImpl(uint16_t cpId, hkpConstraintOwner& constraintOwner) override;
    // Havok SDK signature (hkpDynamics.lib, tied by vtable).
    void processContactImpl(
        const hkpCollidable& a,
        const hkpCollidable& b,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionData& collisionData
    ) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    hkpContactMgr::ToiAccept addToiImpl(
        const hkpCdBody& a,
        const hkpCdBody& b,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& output,
        float toi,
        hkContactPoint& cp,
        const hkpGskCache* gskCache,
        float& projectedVelocity,
        hkpContactPointProperties& properties
    ) override;
    void removeToiImpl(hkpConstraintOwner&, hkpContactPointProperties&) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    hkpContactPointProperties* getContactPointProperties(uint16_t id) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    hkContactPoint* getContactPoint(uint16_t id) override;
    // Havok SDK signature (hkpDynamics.lib, tied by vtable).
    void getAllContactPointIds(hkArray<uint16_t, hkContainerHeapAllocator>& contactPointIds) const override;
    Type getType() const override;
    // Havok SDK signature (hkpDynamics.lib, tied by vtable).
    void toiCollisionResponseBeginCallback(
        const hkContactPoint& cp,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    ) override;
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,vtable).
    void toiCollisionResponseEndCallback(
        const hkContactPoint& cp,
        float impulseApplied,
        hkpSimpleConstraintInfoInitInput& inA,
        hkpBodyVelocity& velA,
        hkpSimpleConstraintInfoInitInput& inB,
        hkpBodyVelocity& velB
    ) override;
    hkpConstraintInstance* getConstraintInstance() override;
    void fireCallbacksForEarliestToi(hkpToiEvent&, float&) override;
    void confirmToi(hkpToiEvent&, float, hkArray<hkpEntity*>&) override;

    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    hkpRigidBody* setContactPointProperties(
        const hkpCdBody& a,
        const hkpCdBody& b,
        int32_t maxNumExtraDataInEvent,
        hkpContactPointProperties* cpi
    );

    uint16_t m_reservedContactPoints;
    uint16_t m_contactPointCallbackDelay;
    hkpSimpleContactConstraintData m_contactConstraintData;
    hkpConstraintInstance m_constraint;
    uint32_t m_pad[1];
};

extern hkClass hkpSimpleConstraintContactMgrClass;
