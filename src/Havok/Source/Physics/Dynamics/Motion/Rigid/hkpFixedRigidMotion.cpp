#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpFixedRigidMotion.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <bit>
#include <xmmintrin.h>

hkClass hkpFixedRigidMotionClass("hkpFixedRigidMotion", &hkpKeyframedRigidMotionClass, 288, nullptr, 0, nullptr, 0,
	nullptr, 0, nullptr, nullptr, 0, 0);
hkTypeInfo hkpFixedRigidMotionTypeInfo = {"hkpFixedRigidMotion", finishLoadedObjecthkpFixedRigidMotion,
	cleanupLoadedObjecthkpFixedRigidMotion, getVtablehkpFixedRigidMotion(), &typeid(hkpFixedRigidMotion),
	sizeof(hkpFixedRigidMotion)};

hkpFixedRigidMotion::hkpFixedRigidMotion(const hkVector4& position, const hkQuaternion& rotation)
	: hkpKeyframedRigidMotion(position, rotation) {
	m_type.m_storage = MOTION_FIXED;
}

hkpFixedRigidMotion::hkpFixedRigidMotion(int32_t a1) : hkpKeyframedRigidMotion(hkFinishLoadedObjectFlag{a1}) {}

void hkpFixedRigidMotion::setLinearVelocity(const hkVector4& a1) {
	;
}

void hkpFixedRigidMotion::setAngularVelocity(const hkVector4& a1) {
	;
}

void hkpFixedRigidMotion::setStepPosition(float a1, float a2) {
	;
}

void hkpFixedRigidMotion::getPositionAndVelocities(hkpMotion* motionOut) {
	motionOut->m_motionState.Copy(this->getMotionState());
	motionOut->m_linearVelocity.m_quad = _mm_setzero_ps();
	motionOut->m_angularVelocity.m_quad = _mm_setzero_ps();
}
