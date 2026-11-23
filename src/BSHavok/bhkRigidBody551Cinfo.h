#pragma once

#include "BSHavok/hkEntityCinfo.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkpShape;

// The creation info of a bhkRigidBody for Havok 5.5.1's rigid body info, the one without the local frame and the
// gravity factor. The Havok info's fields are declared flat (m_*), without Get* accessors.
class bhkRigidBody551Cinfo : public hkEntityCinfo {
public:
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0xE0 bytes, the size bhkRigidBody::QCinfoSize gives.
    struct Serialized : hkEntityCinfo::Serialized {
        uint32_t padding1C;
        uint32_t m_collisionFilterInfo;
        bhkSerializedPointer m_shape;
        int8_t m_hkCollisionResponse;
        uint16_t m_contactPointCallbackDelay;
        hkVector4 m_position;
        hkQuaternion m_rotation;
        hkVector4 m_linearVelocity;
        hkVector4 m_angularVelocity;
        hkMatrix3 m_inertiaTensor;
        hkVector4 m_centerOfMass;
        float m_mass;
        float m_linearDamping;
        float m_angularDamping;
        float m_friction;
        float m_restitution;
        float m_maxLinearVelocity;
        float m_maxAngularVelocity;
        float m_allowedPenetrationDepth;
        uint8_t ucMotionType;
        uint8_t ucDeactivatorType;
        uint8_t ucSolverDeactivation;
        uint8_t ucQualityType;
        int8_t m_autoRemoveLevel;
        int8_t m_numUserDatasInContactPointProperties;
        int8_t m_forceCollideOntoPpu;
        uint8_t paddingD7[9];
    };

    // Fills the info from its serialized form; the shape is left null (the world object's link binds the shape).
    void ReadSerialized(const Serialized& arForm) {
        hkEntityCinfo::ReadSerialized(arForm);
        padding1C = arForm.padding1C;
        m_collisionFilterInfo = arForm.m_collisionFilterInfo;
        m_shape = nullptr;
        m_hkCollisionResponse = arForm.m_hkCollisionResponse;
        m_contactPointCallbackDelay = arForm.m_contactPointCallbackDelay;
        m_position = arForm.m_position;
        m_rotation = arForm.m_rotation;
        m_linearVelocity = arForm.m_linearVelocity;
        m_angularVelocity = arForm.m_angularVelocity;
        m_inertiaTensor = arForm.m_inertiaTensor;
        m_centerOfMass = arForm.m_centerOfMass;
        m_mass = arForm.m_mass;
        m_linearDamping = arForm.m_linearDamping;
        m_angularDamping = arForm.m_angularDamping;
        m_friction = arForm.m_friction;
        m_restitution = arForm.m_restitution;
        m_maxLinearVelocity = arForm.m_maxLinearVelocity;
        m_maxAngularVelocity = arForm.m_maxAngularVelocity;
        m_allowedPenetrationDepth = arForm.m_allowedPenetrationDepth;
        ucMotionType = arForm.ucMotionType;
        ucDeactivatorType = arForm.ucDeactivatorType;
        ucSolverDeactivation = arForm.ucSolverDeactivation;
        ucQualityType = arForm.ucQualityType;
    }

    // The 4-byte alignment gap between hkEntityCinfo and the Havok info, which is 8-byte aligned.
    uint32_t padding1C;
    uint32_t m_collisionFilterInfo;
    hkpShape* m_shape;
    int8_t m_hkCollisionResponse;
    uint16_t m_contactPointCallbackDelay;
    hkVector4 m_position;
    hkQuaternion m_rotation;
    hkVector4 m_linearVelocity;
    hkVector4 m_angularVelocity;
    hkMatrix3 m_inertiaTensor;
    hkVector4 m_centerOfMass;
    float m_mass;
    float m_linearDamping;
    float m_angularDamping;
    float m_friction;
    float m_restitution;
    float m_maxLinearVelocity;
    float m_maxAngularVelocity;
    float m_allowedPenetrationDepth;
    uint8_t ucMotionType;
    uint8_t ucDeactivatorType;
    uint8_t ucSolverDeactivation;
    uint8_t ucQualityType;
};

static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_collisionFilterInfo) == 0x20);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_shape) == 0x24);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_hkCollisionResponse) == 0x28);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_contactPointCallbackDelay) == 0x2A);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_position) == 0x30);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_inertiaTensor) == 0x70);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_centerOfMass) == 0xA0);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_mass) == 0xB0);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, m_allowedPenetrationDepth) == 0xCC);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, ucMotionType) == 0xD0);
static_assert(offsetof(bhkRigidBody551Cinfo::Serialized, ucQualityType) == 0xD3);
static_assert(sizeof(bhkRigidBody551Cinfo::Serialized) == 0xE0);
