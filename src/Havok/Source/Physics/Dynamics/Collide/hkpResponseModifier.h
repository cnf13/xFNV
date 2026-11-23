#pragma once

#include <cstdint>

class hkpConstraintInstance;
class hkpConstraintOwner;
class hkpDynamicsContactMgr;
class hkpRigidBody;
class hkVector4;

// Havok's helpers that change how the solver responds to a contact, through modifier atoms added to the contact's
// constraint.
class hkpResponseModifier {
public:
    // The modifiers a contact's constraint carries, as bits (the SDK's values); ADDITIONAL_SIZE_MODIFIERS are the
    // ones that enlarge the solver's per-body data.
    enum Flags : int32_t {
        MASS_SCALING = 1,
        CENTER_OF_MASS_DISPLACEMENT = 2,
        SURFACE_VELOCITY = 4,
        IMPULSE_SCALING = 8,
        VISCOUS_SURFACE = 16,
        ADDITIONAL_SIZE_MODIFIERS = MASS_SCALING | CENTER_OF_MASS_DISPLACEMENT | SURFACE_VELOCITY,
    };

    // The extra schema bytes a modifier atom adds to a constraint's info, indexed by the two entities' response
    // modifier flags.
    using AdditionalSchemaSizes = uint16_t[8];

    // Scales the inverse masses the solver sees for the contact: factorA for bodyA, factorB for the other body.
    static void setInvMassScalingForContact(
        hkpDynamicsContactMgr* manager,
        hkpRigidBody* bodyA,
        hkpRigidBody* bodyB,
        hkpConstraintOwner& constraintOwner,
        const hkVector4& factorA,
        const hkVector4& factorB
    );
    // Scales the inverse mass the solver sees for body in the contact by factor; the other body keeps its own.
    static void setInvMassScalingForContact(
        hkpDynamicsContactMgr* manager,
        hkpRigidBody* body,
        hkpConstraintOwner& constraintOwner,
        const hkVector4& factor
    );
    // Moves the centres of mass the solver sees for the contact: displacementA for bodyA, displacementB for the
    // other body.
    static void setCenterOfMassDisplacementForContact(
        hkpDynamicsContactMgr* manager,
        hkpRigidBody* bodyA,
        hkpRigidBody* bodyB,
        hkpConstraintOwner& constraintOwner,
        const hkVector4& displacementA,
        const hkVector4& displacementB
    );
    static void disableConstraint(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner);
    static void enableConstraint(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner);
    static void setSurfaceVelocity(
        hkpDynamicsContactMgr* manager,
        hkpRigidBody* body,
        hkpConstraintOwner& constraintOwner,
        const hkVector4& velWorld
    );
    static void setImpulseScalingForContact(
        hkpDynamicsContactMgr* manager,
        hkpRigidBody* bodyA,
        hkpRigidBody* bodyB,
        hkpConstraintOwner& constraintOwner,
        float usedImpulseFraction,
        float maxAcceleration
    );
    static void
    clearSurfaceVelocity(hkpDynamicsContactMgr* manager, hkpConstraintOwner& constraintOwner, hkpRigidBody* body);

    static inline AdditionalSchemaSizes tableOfAdditionalSchemaSizes{0, 80, 208, 288, 80, 160, 288, 368};
};
