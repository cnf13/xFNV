#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidableQualityType.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/ShapeDepth/hkpShapeDepthUtil.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpBreakableBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpCharacterMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpFixedRigidMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

void estimateAllowedPenetrationDepth(const hkVector4& arExtent, hkpCollidable* apCollidable) {
    float y = arExtent.m_quad[1];
    double x = arExtent.m_quad[0];
    if (!(y > x)) {
        x = y;
    }
    float v4 = x;
    float z = arExtent.m_quad[2];
    double v3 = v4;
    if (!(z > static_cast<double>(v4))) {
        v3 = z;
    }
    float v5 = v3;
    if (!(v5 < 0.5f)) {
        apCollidable->m_allowedPenetrationDepth = 0.1f;
    }
    else {
        apCollidable->m_allowedPenetrationDepth = v5 * 0.2;
    }
}

// The constructor from a construction info: hkpEntity's from the shape, then the info's fields, the motion its
// motion type asks for, deactivation, damping, the cached shape data and the quality type.
hkpRigidBody::hkpRigidBody(const hkpRigidBodyCinfo& info) : hkpEntity(info.m_shape) {
    m_material.m_responseType.m_storage = info.m_collisionResponse.m_storage;
    m_contactPointCallbackDelay = info.m_contactPointCallbackDelay;
    m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = info.m_collisionFilterInfo;
    hkLocalFrame* pLocalFrame = info.m_localFrame;
    if (pLocalFrame) {
        pLocalFrame->addReference();
        if (m_localFrame) {
            m_localFrame->removeReference();
        }
        m_localFrame = pLocalFrame;
        pLocalFrame->addReference();
    }
    float fAllowedPenetrationDepth;
    if (info.m_motionType.m_storage == hkpMotion::MOTION_FIXED) {
        hkpFixedRigidMotion* pFixedMotion = new (&m_motion) hkpFixedRigidMotion(info.m_position, info.m_rotation);
        pFixedMotion->m_motionState.m_maxLinearVelocity = info.m_maxLinearVelocity;
        pFixedMotion->m_motionState.m_maxAngularVelocity = info.m_maxAngularVelocity;
        pFixedMotion->setDeactivationClass(1);
        m_collidable.m_motion.pState = m_motion.getMotionState();
        fAllowedPenetrationDepth =
            info.m_allowedPenetrationDepth <= 0.0f ? 3.40282e+38f : info.m_allowedPenetrationDepth;
    }
    else {
        createDynamicRigidMotion(
            static_cast<hkpMotion::MotionType>(info.m_motionType.m_storage), info.m_position, info.m_rotation,
            info.m_mass, info.m_inertiaTensor, info.m_centerOfMass, info.m_maxLinearVelocity, info.m_maxAngularVelocity,
            &m_motion
        );
        m_motion.setDeactivationClass(info.m_solverDeactivation.m_storage);
        setLinearVelocity(info.m_linearVelocity);
        setAngularVelocity(info.m_angularVelocity);
        m_collidable.m_motion.pState = m_motion.getMotionState();
        fAllowedPenetrationDepth = info.m_allowedPenetrationDepth;
    }
    m_collidable.m_allowedPenetrationDepth = fAllowedPenetrationDepth;
    enableDeactivation(info.m_enableDeactivation.m_bool != 0);
    const hkpShape* pShape = m_collidable.m_shape;
    m_motion.m_motionState.m_linearDamping = info.m_linearDamping;
    m_motion.m_motionState.m_angularDamping = info.m_angularDamping;
    if (pShape) {
        alignas(16) hkVector4 kExtent;
        updateCachedShapeInfo(pShape, kExtent);
        if (m_collidable.getAllowedPenetrationDepth() <= 0.0f) {
            estimateAllowedPenetrationDepth(kExtent, &m_collidable);
        }
    }
    int8_t cQualityType = info.m_qualityType.m_storage;
    if (cQualityType == HK_COLLIDABLE_QUALITY_INVALID) {
        if (m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED) {
            m_collidable.m_broadPhaseHandle.m_objectQualityType = HK_COLLIDABLE_QUALITY_FIXED;
        }
        else {
            m_collidable.m_broadPhaseHandle.m_objectQualityType =
                info.m_motionType.m_storage == hkpMotion::MOTION_KEYFRAMED ? HK_COLLIDABLE_QUALITY_KEYFRAMED
                                                                           : HK_COLLIDABLE_QUALITY_DEBRIS;
        }
    }
    else {
        m_collidable.m_broadPhaseHandle.m_objectQualityType = cQualityType;
    }
    m_autoRemoveLevel = info.m_autoRemoveLevel;
    if (info.m_forceCollideOntoPpu.m_bool) {
        m_collidable.m_forceCollideOntoPpu |= 1u;
    }
    m_material.m_friction = info.m_friction;
    m_material.m_restitution = info.m_restitution;
    m_motion.m_gravityFactor.m_value = static_cast<uint16_t>(std::bit_cast<uint32_t>(info.m_gravityFactor) >> 16);
    int8_t cNumShapeKeys = info.m_numShapeKeysInContactPointProperties;
    if (cNumShapeKeys < 0) {
        m_numShapeKeysInContactPointProperties = static_cast<uint8_t>(hkpShapeDepthUtil::getShapeDepth(info.m_shape));
    }
    else {
        m_numShapeKeysInContactPointProperties = cNumShapeKeys;
        if (cNumShapeKeys &&
            m_numShapeKeysInContactPointProperties <
                static_cast<uint8_t>(hkpShapeDepthUtil::getShapeDepth(info.m_shape))) {
            char acBuffer[512];
            hkErrStream kStream(acBuffer, 512);
            kStream << "Shape key storage insufficient to store all possible branches. This may cause truncation of "
                       "the shape key information.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x458A1BB2, acBuffer, ".\\Entity\\hkpRigidBody.cpp", 329
            );
        }
    }
    m_responseModifierFlags = info.m_responseModifierFlags;
}

hkpRigidBody::~hkpRigidBody() {}

// Replaces the body's shape. In a world whose critical operations are locked the change is queued; otherwise the body
// leaves the broad phase under a lock, takes the shape, recomputes its cached data and penetration depth, fires the
// shape-set callbacks and rejoins the broad phase, running the operations the unlock leaves pending. The world pointer
// is reloaded at each use, as the game does.
hkWorldOperation::Result hkpRigidBody::setShape(const hkpShape* shape) {
    if (m_world) {
        if (m_world->m_criticalOperationsLockCount) {
            hkWorldOperation::SetWorldObjectShape kOperation;
            kOperation.m_type.m_storage = hkWorldOperation::WORLD_OBJECT_SET_SHAPE;
            kOperation.m_worldObject = this;
            kOperation.m_shape = shape;
            m_world->queueOperation(kOperation);
            return hkWorldOperation::POSTPONED;
        }
        ++m_world->m_criticalOperationsLockCount;
        hkpWorldOperationUtil::removeEntityBP(m_world, this);
    }
    const hkpShape* pOldShape = m_collidable.m_shape;
    m_collidable.m_shape = shape;
    const_cast<hkpShape*>(shape)->addReference();
    if (pOldShape) {
        const_cast<hkpShape*>(pOldShape)->removeReference();
    }
    hkVector4 kExtent;
    updateCachedShapeInfo(shape, kExtent);
    // Compares against the float 3.40282e+38; NaN also resets the depth.
    if (pOldShape && m_collidable.m_allowedPenetrationDepth != 3.40282e+38f) {
        m_collidable.m_allowedPenetrationDepth = -1.0f;
    }
    // The estimate runs unless 0 < depth, so it also runs for NaN.
    if (!(0.0f < m_collidable.m_allowedPenetrationDepth)) {
        estimateAllowedPenetrationDepth(kExtent, &m_collidable);
    }
    if (m_world) {
        setCachedShapeData(m_world, shape);
    }
    if (m_world) {
        hkpWorldCallbackUtil::fireEntityShapeSet(m_world, this);
    }
    hkpEntityCallbackUtil::fireEntityShapeSet(this);
    if (m_world) {
        hkpWorldOperationUtil::addEntityBP(m_world, this);
        hkpWorld* pWorld = m_world;
        // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
        if (--pWorld->m_criticalOperationsLockCount == 0 && !pWorld->m_blockExecutingPendingOperations) {
            if (pWorld->m_pendingOperationsCount) {
                pWorld->internal_executePendingOperations();
            }
            if (pWorld->m_pendingOperationQueueCount == 1 && pWorld->m_pendingBodyOperationsCount) {
                pWorld->internal_executePendingBodyOperations();
            }
        }
    }
    return hkWorldOperation::DONE;
}

// The SDK's clone: a new body made from this one's construction info, then the motion (and the saved motion, in a
// new hkpMaxSizeMotion) copied over whole in 16-byte blocks, the properties, the name, the user data and a clone of
// the breakable body.
hkpRigidBody* hkpRigidBody::clone() const {
    alignas(16) hkpRigidBodyCinfo info;
    getCinfo(info);
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpRigidBody));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpRigidBody));
    hkpRigidBody* rb = ::new (pMemory) hkpRigidBody(info);
    hkString::memCpy16NonEmpty(&rb->m_motion, &m_motion, sizeof(hkpMaxSizeMotion) >> 4);
    if (m_motion.m_savedMotion) {
        void* pSavedMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMaxSizeMotion));
        hkpMaxSizeMotion* pSaved = static_cast<hkpMaxSizeMotion*>(pSavedMemory);
        pSaved->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpMaxSizeMotion));
        ::new (pSaved) hkpMaxSizeMotion;
        rb->m_motion.m_savedMotion = pSaved;
        hkString::memCpy16NonEmpty(pSaved, m_motion.m_savedMotion, sizeof(hkpMaxSizeMotion) >> 4);
    }
    rb->m_collidable.m_motion.pState = rb->getMotionState();
    rb->copyProperties(this);
    rb->m_name = m_name.cString();
    rb->m_userData = m_userData;
    if (m_breakableBody) {
        rb->m_breakableBody = m_breakableBody->cloneBreakableBody(rb);
    }
    return rb;
}

hkMotionState* hkpRigidBody::getMotionState() {
    return &m_motion.m_motionState;
}

hkpMotion* hkpRigidBody::getStoredDynamicMotion() {
    uint8_t m_type;
    m_type = m_motion.m_type.m_storage;
    if ((m_type == 4) || (m_type == 5)) {
        return m_motion.m_savedMotion;
    }

    return nullptr;
}

const hkVector4& hkpRigidBody::getTranslation() const {
    return *getRigidMotion()->getTranslation();
}

void hkpRigidBody::setPosition(const hkVector4& position) {
    m_motion.setPosition(position);
    hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody(this);
}

void hkpRigidBody::setRotation(const hkQuaternion& rotation) {
    m_motion.setRotation(rotation);
    hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody(this);
}

void hkpRigidBody::setPositionAndRotation(const hkVector4& position, const hkQuaternion& rotation) {
    m_motion.setPositionAndRotation(position, rotation);
    hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody(this);
}

void hkpRigidBody::setLinearVelocity(const hkVector4& newVel) {
    const hkVector4* LinearVelocity = &getLinearVelocity();
    if (!newVel.equals4(*LinearVelocity, 0.001f)) {
        activate();
        hkpMotion* Motion = getMotion();
        Motion->setLinearVelocity(newVel);
    }
}

void hkpRigidBody::setAngularVelocity(const hkVector4& newVel) {
    hkpMotion* ragdolls;
    const hkVector4* angularVelocity = &getAngularVelocity();
    if (!(&newVel)->equals4(*angularVelocity, 0.0010000000474974513f)) {
        hkpEntity::activate();
        ragdolls = hkpEntity::getMotion();
        ragdolls->setAngularVelocity(newVel);
    }
}

void hkpRigidBody::setMass(float m) {
    m_motion.setMass(m);
}

bool hkpRigidBody::isDeactivationEnabled() const {
    return m_motion.m_deactivationIntegrateCounter != 0xFF;
}

// Seeds the deactivation counter from the body's uid and the world's select flags, or switches deactivation off.
void hkpRigidBody::enableDeactivation(bool _enableDeactivation) {
    if (_enableDeactivation) {
        if (m_world) {
            hkpWorld* pWorld = m_world;
            uint8_t ucCounter = pWorld->m_dynamicsStepInfo.m_solverInfo.m_deactivationIntegrateCounter;
            uint8_t ucFlag1 = pWorld->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[1];
            uint8_t ucFlag0 = pWorld->m_dynamicsStepInfo.m_solverInfo.m_deactivationNumInactiveFramesSelectFlag[0];
            m_motion.m_deactivationIntegrateCounter = m_uid & 0xF;
            m_motion.setWorldSelectFlagsNeg(ucFlag0, ucFlag1, ucCounter);
        }
        else {
            m_motion.m_deactivationIntegrateCounter = m_uid & 0xF;
            m_motion.setWorldSelectFlagsNeg(0, 0, 0);
        }
    }
    else {
        m_motion.m_deactivationIntegrateCounter = 0xFF;
        m_motion.m_deactivationNumInactiveFrames[0] = 0;
        m_motion.m_deactivationNumInactiveFrames[1] = 0;
    }
}

void hkpRigidBody::setFriction(float a1, float a2) {
    m_material.m_friction = a1;
}

void hkpRigidBody::setRestitution(float a1) {
    m_material.m_restitution = a1;
}

void hkpRigidBody::setMassInv(float mInv) {
    m_motion.setMassInv(mInv);
}

// Sets the body's inertia tensor in its own frame.
void hkpRigidBody::setInertiaLocal(const hkMatrix3& inertia) {
    m_motion.setInertiaLocal(inertia);
}

void hkpRigidBody::setInertiaInvLocal(const hkMatrix3& arInertiaInv) {
    m_motion.setInertiaInvLocal(arInertiaInv);
}

// Moves the body's centre of mass to arCenterOfMass in its own frame.
void hkpRigidBody::setCenterOfMassLocal(const hkVector4& centerOfMass) {
    hkVector4 arExtentOut;
    m_motion.setCenterOfMassInLocal(centerOfMass);
    updateCachedShapeInfo(m_collidable.m_shape, arExtentOut);
}

void hkpRigidBody::setTransform(const hkTransform& transform) {
    m_motion.setTransform(transform);
    hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody(this);
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte or
// +0xC..0x13 after the data pointer.
void hkpRigidBody::setPositionAndRotationAsCriticalOperation(const hkVector4& position, const hkQuaternion& rotation) {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::SetRigidBodyPositionAndRotation kOperation;
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_SET_POSITION_AND_ROTATION;
        kOperation.m_rigidBody = this;
        hkVector4* pData =
            static_cast<hkVector4*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(2 * sizeof(hkVector4)));
        kOperation.m_positionAndRotation = pData;
        pData[0].m_quad = position.m_quad;
        pData[1].m_quad = rotation.m_vec.m_quad;
        m_world->queueOperation(kOperation);
    }
    else {
        setPositionAndRotation(position, rotation);
    }
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte.
void hkpRigidBody::setLinearVelocityAsCriticalOperation(const hkVector4& newVel) {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::SetRigidBodyLinearVelocity kOperation;
        kOperation.m_rigidBody = this;
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_SET_LINEAR_VELOCITY;
        kOperation.m_linearVelocity[0] = newVel.m_quad[0];
        kOperation.m_linearVelocity[1] = newVel.m_quad[1];
        kOperation.m_linearVelocity[2] = newVel.m_quad[2];
        pWorld->queueOperation(kOperation);
    }
    else {
        setLinearVelocity(newVel);
    }
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte.
void hkpRigidBody::setAngularVelocityAsCriticalOperation(const hkVector4& newVel) {
    hkpWorld* m_world = this->m_world;
    if (m_world && m_world->m_criticalOperationsLockCount) {
        hkWorldOperation::SetRigidBodyAngularVelocity kOperation;
        kOperation.m_rigidBody = this;
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_SET_ANGULAR_VELOCITY;
        kOperation.m_angularVelocity[0] = newVel.m_quad[0];
        kOperation.m_angularVelocity[1] = newVel.m_quad[1];
        kOperation.m_angularVelocity[2] = newVel.m_quad[2];
        m_world->queueOperation(kOperation);
    }
    else {
        setAngularVelocity(newVel);
    }
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte.
void hkpRigidBody::applyLinearImpulseAsCriticalOperation(const hkVector4& imp) {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::ApplyRigidBodyLinearImpulse kOperation;
        kOperation.m_linearImpulse[0] = imp.m_quad[0];
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_APPLY_LINEAR_IMPULSE;
        kOperation.m_rigidBody = this;
        kOperation.m_linearImpulse[1] = imp.m_quad[1];
        kOperation.m_linearImpulse[2] = imp.m_quad[2];
        pWorld->queueOperation(kOperation);
    }
    else {
        activate();
        getRigidMotion()->applyLinearImpulse(imp);
    }
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte or
// +0xC..0x13 after the data pointer.
void hkpRigidBody::applyPointImpulseAsCriticalOperation(const hkVector4& imp, const hkVector4& p) {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::ApplyRigidBodyPointImpulse kOperation;
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_APPLY_POINT_IMPULSE;
        kOperation.m_rigidBody = this;
        hkVector4* pData =
            static_cast<hkVector4*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(2 * sizeof(hkVector4)));
        kOperation.m_pointAndImpulse = pData;
        pData[0].m_quad = p.m_quad;
        pData[1].m_quad = imp.m_quad;
        m_world->queueOperation(kOperation);
    }
    else {
        activate();
        getRigidMotion()->applyPointImpulse(imp, p);
    }
}

// KEEP (partcopy:): hkpWorld::queueOperation (0xC91160, through hkpWorldOperationQueue::queueOperation 0xCF7690)
// copies all 0x14 bytes of the operation, and the exe never writes its bytes +0x1..0x3 after the type byte.
void hkpRigidBody::applyAngularImpulseAsCriticalOperation(const hkVector4& imp) {
    hkpWorld* pWorld = m_world;
    if (pWorld && pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::ApplyRigidBodyAngularImpulse kOperation;
        kOperation.m_angularImpulse[0] = imp.m_quad[0];
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_APPLY_ANGULAR_IMPULSE;
        kOperation.m_rigidBody = this;
        kOperation.m_angularImpulse[1] = imp.m_quad[1];
        kOperation.m_angularImpulse[2] = imp.m_quad[2];
        pWorld->queueOperation(kOperation);
    }
    else {
        activate();
        getRigidMotion()->applyAngularImpulse(imp);
    }
}

// Recomputes the body's cached shape data for apShape and writes the shape's extent into arExtentOut. The object radius
// is the larger distance of the AABB's corners from the centre of mass; the max corner's square is kept only when the
// min corner's is below it, so an unordered compare keeps the min corner's. The squared lengths are written as _ps
// operations on lane 0.
void hkpRigidBody::updateCachedShapeInfo(const hkpShape* shape, hkVector4& extentOut) {
    hkAabb kAabb;
    shape->getAabbImpl(hkTransformIdentity, 0.0f, kAabb);
    extentOut.m_quad = _mm_sub_ps(kAabb.m_max.m_quad, kAabb.m_min.m_quad);
    hkVector4 kCenterOfMass = m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal;
    __m128 kToMax = _mm_sub_ps(kAabb.m_max.m_quad, kCenterOfMass.m_quad);
    __m128 kToMin = _mm_sub_ps(kAabb.m_min.m_quad, kCenterOfMass.m_quad);
    kToMax = _mm_mul_ps(kToMax, kToMax);
    kToMin = _mm_mul_ps(kToMin, kToMin);
    const __m128 kMaxSum =
        _mm_add_ps(_mm_shuffle_ps(kToMax, kToMax, 170), _mm_add_ps(_mm_shuffle_ps(kToMax, kToMax, 85), kToMax));
    const __m128 kMinSum =
        _mm_add_ps(_mm_shuffle_ps(kToMin, kToMin, 170), _mm_add_ps(_mm_shuffle_ps(kToMin, kToMin, 85), kToMin));
    float fMaxLengthSquared = _mm_cvtss_f32(kMaxSum);
    float fMinLengthSquared = _mm_cvtss_f32(kMinSum);
    float fRadiusSquared = fMinLengthSquared < fMaxLengthSquared ? fMaxLengthSquared : fMinLengthSquared;
    float fRadius = std::sqrt(static_cast<double>(fRadiusSquared));
    m_motion.m_motionState.m_objectRadius = fRadius;
}

// Fills arInfo from the body's state.
void hkpRigidBody::getCinfo(hkpRigidBodyCinfo& info) const {
    info.m_forceCollideOntoPpu.m_bool = m_collidable.m_forceCollideOntoPpu & 1;
    info.m_autoRemoveLevel = m_autoRemoveLevel;
    info.m_numShapeKeysInContactPointProperties = m_numShapeKeysInContactPointProperties;
    info.m_contactPointCallbackDelay = this->getContactPointCallbackDelay();
    info.m_enableDeactivation.m_bool = isDeactivationEnabled();
    info.m_localFrame = m_localFrame;
    info.m_friction = m_material.m_friction;
    info.m_collisionResponse.m_storage = m_material.m_responseType.m_storage;
    info.m_restitution = m_material.m_restitution;
    info.m_linearDamping = m_motion.m_motionState.m_linearDamping;
    info.m_angularDamping = m_motion.m_motionState.m_angularDamping;
    info.m_linearVelocity = m_motion.m_linearVelocity;
    info.m_angularVelocity = m_motion.m_angularVelocity;
    info.m_mass = m_motion.getMass();
    // Calls getInertiaLocal through the motion's vtable.
    static_cast<const hkpMotion*>(&m_motion)->getInertiaLocal(info.m_inertiaTensor);
    info.m_motionType.m_storage = m_motion.m_type.m_storage;
    info.m_solverDeactivation.m_storage = m_motion.m_motionState.m_deactivationClass;
    info.m_maxLinearVelocity = hkUFloat8_intToReal[m_motion.m_motionState.m_maxLinearVelocity.m_value];
    info.m_maxAngularVelocity = hkUFloat8_intToReal[m_motion.m_motionState.m_maxAngularVelocity.m_value];
    info.m_position = m_motion.m_motionState.m_transform.m_translation;
    info.m_rotation = m_motion.m_motionState.m_sweptTransform.m_rotation1;
    info.m_centerOfMass = m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal;
    info.m_shape = m_collidable.m_shape;
    info.m_collisionFilterInfo = m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
    info.m_allowedPenetrationDepth = m_collidable.getAllowedPenetrationDepth();
    info.m_qualityType.m_storage = m_collidable.m_broadPhaseHandle.m_objectQualityType;
    info.m_gravityFactor = std::bit_cast<float>((m_motion.m_gravityFactor.m_value << 16));
}

void hkpRigidBody::setMotionType(
    hkpMotion::MotionType newState,
    hkpEntityActivation preferredActivationState,
    hkpUpdateCollisionFilterOnEntityMode collisionFilterUpdateMode
) {
    hkpWorld* m_world = this->m_world;
    if (m_world && m_world->m_criticalOperationsLockCount) {
        hkWorldOperation::SetRigidBodyMotionType kOperation;
        kOperation.m_activation = static_cast<uint8_t>(preferredActivationState);
        kOperation.m_motionType = static_cast<uint8_t>(newState);
        kOperation.m_type.m_storage = hkWorldOperation::RIGIDBODY_SET_MOTION_TYPE;
        kOperation.m_rigidBody = this;
        kOperation.m_collisionFilterUpdateMode = static_cast<uint8_t>(collisionFilterUpdateMode);
        m_world->queueOperation(kOperation);
    }
    else {
        hkpWorldOperationUtil::setRigidBodyMotionType(
            this, newState, preferredActivationState, collisionFilterUpdateMode
        );
        hkpWorld* v6 = this->m_world;
        if (v6) {
            hkpWorldCallbackUtil::fireEntitySetMotionType(v6, this);
        }
        hkpEntityCallbackUtil::fireEntitySetMotionType(this);
    }
}

hkpMotion* hkpRigidBody::getStoredDynamicMotion_0() {
    uint8_t m_type = m_motion.m_type.m_storage;
    if (m_type == 4 || m_type == 5) {
        return m_motion.m_savedMotion;
    }

    return nullptr;
}

// Stores in *apResult whether the body's shape is small enough: false
// for a transform shape, for a collection, list or triangle collection of more than 100 children, and for a MOPP or
// bv-tree shape of more than 100 children on a body that is neither fixed nor keyframed.
char* hkpRigidBody::checkPerformance(char* apResult) {
    const hkpShape* pShape = m_collidable.m_shape;
    if (!pShape) {
        *apResult = 0;
        return apResult;
    }
    uint8_t ucMotionType = m_motion.m_type.m_storage;
    uint32_t uiShapeType = pShape->m_type.m_storage;
    char cResult = 1;
    if ((ucMotionType != hkpMotion::MOTION_FIXED && ucMotionType != hkpMotion::MOTION_KEYFRAMED &&
         (uiShapeType == HK_SHAPE_MOPP || uiShapeType == HK_SHAPE_BV_TREE)) &&
        (pShape->getContainer()->getNumChildShapes() > 100)) {
        cResult = 0;
    }

    const hkpShapeContainer* pContainer;
    switch (uiShapeType) {
        case HK_SHAPE_TRIANGLE_COLLECTION:
        case HK_SHAPE_COLLECTION:
            // Loads the shape from the body again here.
            pContainer = static_cast<const hkpShapeCollection*>(m_collidable.m_shape);
            if (pContainer->getNumChildShapes() > 100) {
                cResult = 0;
            }
            break;
        case HK_SHAPE_TRANSFORM: cResult = 0; break;
        case HK_SHAPE_LIST:
            pContainer = static_cast<const hkpShapeCollection*>(pShape);
            if (pContainer->getNumChildShapes() > 100) {
                cResult = 0;
            }
            break;
        default: break;
    }
    *apResult = cResult;
    return apResult;
}

// Builds in apMotionOut the motion a body of the type moves with. A dynamic body gets a
// sphere motion when the smallest diagonal entry of its inertia is above 0.8 of the largest (an unordered compare
// picks the box), else a box motion. A keyframed motion gets no inertia, centre of mass or mass, and the maximum
// velocities 1e6.
void hkpRigidBody::createDynamicRigidMotion(
    hkpMotion::MotionType motionType,
    const hkVector4& position,
    const hkQuaternion& rotation,
    float mass,
    const hkMatrix3& inertiaLocal,
    const hkVector4& centreOfMassLocal,
    float maxLinearVelocity,
    float maxAngularVelocity,
    hkpMaxSizeMotion* motionBufferOut
) {
    hkpMotion* pMotion;
    switch (motionType) {
        case hkpMotion::MOTION_DYNAMIC: {
            float fXX = inertiaLocal.m_col0.m_quad[0];
            float fYY = inertiaLocal.m_col1.m_quad[1];
            float fZZ = inertiaLocal.m_col2.m_quad[2];
            float fMin = hkMath::min2(fXX, hkMath::min2(fYY, fZZ));
            double dMaxScaled = hkMath::max2<float>(fXX, hkMath::max2<float>(fYY, fZZ)) * 0.800000011920929;
            if (fMin > dMaxScaled) {
                pMotion = motionBufferOut ? ::new (motionBufferOut) hkpSphereMotion(position, rotation) : nullptr;
            }
            else {
                pMotion = motionBufferOut ? ::new (motionBufferOut) hkpBoxMotion(position, rotation) : nullptr;
            }
            break;
        }
        case hkpMotion::MOTION_SPHERE_INERTIA:
            pMotion = motionBufferOut ? ::new (motionBufferOut) hkpSphereMotion(position, rotation) : nullptr;
            break;
        case hkpMotion::MOTION_BOX_INERTIA:
            pMotion = motionBufferOut ? ::new (motionBufferOut) hkpBoxMotion(position, rotation) : nullptr;
            break;
        case hkpMotion::MOTION_KEYFRAMED:
            maxLinearVelocity = 1e6f;
            maxAngularVelocity = 1e6f;
            pMotion = motionBufferOut ? ::new (motionBufferOut) hkpKeyframedRigidMotion(position, rotation) : nullptr;
            break;
        case hkpMotion::MOTION_THIN_BOX_INERTIA:
            pMotion = motionBufferOut ? ::new (motionBufferOut) hkpThinBoxMotion(position, rotation) : nullptr;
            break;
        case hkpMotion::MOTION_CHARACTER:
            pMotion = motionBufferOut ? ::new (motionBufferOut) hkpCharacterMotion(position, rotation) : nullptr;
            break;
        default:
            pMotion = motionBufferOut ? ::new (motionBufferOut) hkpFixedRigidMotion(position, rotation) : nullptr;
            break;
    }
    if (motionType != hkpMotion::MOTION_KEYFRAMED) {
        pMotion->setInertiaLocal(inertiaLocal);
        pMotion->setCenterOfMassInLocal(centreOfMassLocal);
        pMotion->setMass(mass);
    }
    pMotion->m_motionState.m_maxLinearVelocity = maxLinearVelocity;
    pMotion->m_motionState.m_maxAngularVelocity = maxAngularVelocity;
}

// With the world's critical operations locked, queues an UpdateMovedBodyInfo of which only bytes 0 and 4-7 are written,
// while hkpWorldOperationQueue::queueOperation copies 20 bytes of it, so bytes 1-3 (padding) and 8-19 (past the 8-byte
// local) are read unwritten, as the game does.
void hkpRigidBody::updateBroadphaseAndResetCollisionInformationOfWarpedBody(hkpEntity* entity) {
    hkpWorld* pWorld = entity->m_world;
    if (!pWorld) {
        return;
    }
    if (pWorld->m_criticalOperationsLockCount) {
        hkWorldOperation::UpdateMovedBodyInfo kOperation;
        kOperation.m_type = hkWorldOperation::UPDATE_MOVED_BODY_INFO;
        kOperation.m_entity = entity;
        pWorld->queueOperation(kOperation);
        return;
    }
    hkpContinuousSimulation* pSimulation = static_cast<hkpContinuousSimulation*>(pWorld->m_simulation);
    pWorld->m_criticalOperationsLockCount = 1;
    hkpEntity* pEntity = entity;
    pSimulation->resetCollisionInformationForEntities(&pEntity, 1, pWorld, hkpSimulation::RESET_ALL);
    if (entity->m_collidable.m_shape) {
        hkpSimulation::collideEntitiesBroadPhaseDiscrete(&pEntity, 1, pWorld);
    }
    if (!entity->isActive().m_bool) {
        if ((pWorld->m_shouldActivateOnRigidBodyTransformChange.m_bool) &&
            (entity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED)) {
            entity->activate();
        }

        hkpWorldCallbackUtil::fireInactiveEntityMoved(pWorld, entity);
    }
    hkpTreeWorldManager* pKdTreeManager = pWorld->m_kdTreeManager;
    if (pKdTreeManager) {
        pKdTreeManager->setUpToDate(false);
    }
    // EXE-INLINED none: hkpWorld::unlockAndAttemptToExecutePendingOperations
    if (pWorld->m_criticalOperationsLockCount-- == 1 && !pWorld->m_blockExecutingPendingOperations.m_bool) {
        if (pWorld->m_pendingOperationsCount) {
            pWorld->internal_executePendingOperations();
        }
        if ((pWorld->m_pendingOperationQueueCount == 1) && (pWorld->m_pendingBodyOperationsCount)) {
            pWorld->internal_executePendingBodyOperations();
        }
    }
}

// m_properties = otherObj->m_properties, using the class version of hkArrayBase::copyFromArray (hkpProperty is not a
// POD type to hkIsPodType): _reserve, the empty destruct of the elements past the new size, copy over the old elements,
// and constructWithArray's placement copies into the new ones, whose null test is kept as the game does.
void hkpWorldObject::copyProperties(const hkpWorldObject* otherObj) {
    hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&m_properties);
    const int32_t oldSize = m_properties.m_size;
    const int32_t newSize = otherObj->m_properties.m_size;
    const int32_t copiedSize = newSize > oldSize ? oldSize : newSize;
    m_properties._reserve(alloc, newSize);
    hkArrayBase<hkpProperty>::copy(m_properties.m_data, otherObj->m_properties.m_data, copiedSize);
    hkArrayUtil::constructWithArray(
        m_properties.m_data + copiedSize, newSize - copiedSize, otherObj->m_properties.m_data + copiedSize,
        hkTypeIsClass()
    );
    m_properties.m_size = newSize;
}
