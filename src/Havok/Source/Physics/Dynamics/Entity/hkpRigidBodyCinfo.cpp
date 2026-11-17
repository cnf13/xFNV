#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include <xmmintrin.h>

hkpRigidBodyCinfo::hkpRigidBodyCinfo()
  : m_collisionFilterInfo(0), m_shape(nullptr), m_localFrame(nullptr), m_contactPointCallbackDelay(0xFFFF),
    m_mass(1.0f), m_linearDamping(0.0f), m_angularDamping(0.05f), m_gravityFactor(1.0f), m_friction(0.5f),
    m_restitution(0.4f), m_maxLinearVelocity(200.0f), m_maxAngularVelocity(200.0f), m_allowedPenetrationDepth(-1.0f),
    m_autoRemoveLevel(0), m_responseModifierFlags(0), m_numShapeKeysInContactPointProperties(0) {
    m_position.m_quad = _mm_setzero_ps();
    m_rotation.m_vec.m_quad = hkQuadReal0001;
    m_linearVelocity.m_quad = _mm_setzero_ps();
    m_angularVelocity.m_quad = _mm_setzero_ps();
    m_inertiaTensor.m_col0.m_quad = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
    m_inertiaTensor.m_col1.m_quad = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    m_inertiaTensor.m_col2.m_quad = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    m_centerOfMass.m_quad = _mm_setzero_ps();
    m_collisionResponse.m_storage = hkpMaterial::RESPONSE_SIMPLE_CONTACT;
    m_motionType.m_storage = hkpMotion::MOTION_DYNAMIC;
    m_enableDeactivation.m_bool = 1;
    m_solverDeactivation.m_storage = SOLVER_DEACTIVATION_LOW;
    m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_INVALID;
    m_forceCollideOntoPpu.m_bool = 0;
}

void hkpRigidBodyCinfo::setMassProperties(const hkpMassProperties& mp) {
    m_mass = mp.m_mass;
    m_inertiaTensor = mp.m_inertiaTensor;
    m_centerOfMass = mp.m_centerOfMass;
}

void hkpRigidBodyCinfo::setTransform(const hkTransform& transform) {
    m_position = transform.getTranslation();
    m_rotation.set(transform.getRotation());
}
