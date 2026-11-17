#include "Havok/Source/Physics/Utilities/Actions/Dashpot/hkpDashpotAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"

#include <xmmintrin.h>

hkpDashpotAction::MembersArray hkpDashpotAction::Members{
	{"point", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 2, 0, 0x20, nullptr},
	{"strength", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x40, nullptr},
	{"damping", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x44, nullptr},
	{"impulse", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x50, nullptr}};

hkClass hkpDashpotActionClass("hkpDashpotAction", &hkpBinaryActionClass, 96, nullptr, 0, nullptr, 0,
	hkpDashpotAction::Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkpDashpotActionTypeInfo = {"hkpDashpotAction", finishLoadedObjecthkpDashpotAction,
	cleanupLoadedObjecthkpDashpotAction, getVtablehkpDashpotAction(), &typeid(hkpDashpotAction),
	sizeof(hkpDashpotAction)};

// Zero points, strength 0.1 and damping 0.01; m_impulse is left unwritten, as in the exe.
hkpDashpotAction::hkpDashpotAction(hkpRigidBody* entityA, hkpRigidBody* entityB, uint32_t userData)
	: hkpBinaryAction(entityA, entityB, userData), m_strength(0.1f), m_damping(0.01f) {
	m_point[0].m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
	m_point[1].m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
}

hkpDashpotAction::~hkpDashpotAction() {}

// The SDK's dashpot: a spring between the two attachment points and a damper on the bodies' relative linear
// velocity, stored in m_impulse and applied to B, its negation to A, at the points in world space.
void hkpDashpotAction::applyAction(const hkStepInfo& stepInfo) {
	HK_TIMER_BEGIN("Dashpot", nullptr);
	hkpEntity* pEntityA = this->getEntityA();
	hkpEntity* pEntityB = this->getEntityB();
	// The exe multiplies by a double 151.0 and stores the product as a float.
	float dt = stepInfo.m_deltaTime * 151.0;
	alignas(16) hkVector4 pointA;
	pointA.setTransformedPos(pEntityA->m_motion.m_motionState.m_transform, m_point[0]);
	alignas(16) hkVector4 pointB;
	pointB.setTransformedPos(pEntityB->m_motion.m_motionState.m_transform, m_point[1]);
	__m128 relVel = _mm_sub_ps(pEntityA->m_motion.m_linearVelocity.m_quad, pEntityB->m_motion.m_linearVelocity.m_quad);
	float strength = m_strength * dt;
	__m128 spring = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(strength), _mm_set_ss(strength), 0),
		_mm_sub_ps(pointA.m_quad, pointB.m_quad));
	float damping = dt * m_damping;
	m_impulse.m_quad = spring;
	m_impulse.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(damping), _mm_set_ss(damping), 0), relVel), m_impulse.m_quad);
	alignas(16) hkVector4 impulseA;
	impulseA.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(-1.0f), _mm_set_ss(-1.0f), 0), m_impulse.m_quad);
	pEntityA->activate();
	// Through the motion's vtable, as the exe calls it.
	pEntityA->getMotion()->applyPointImpulse(impulseA, pointA);
	pEntityB->activate();
	pEntityB->getMotion()->applyPointImpulse(m_impulse, pointB);
	HK_TIMER_END();
}

// A copy of this action between two new entities (the SDK casts them to rigid bodies), allocated with the size in
// its m_memSizeAndFlags as the SDK's class operator new does; null unless there are two entities and no phantoms.
// The two floats are fld/fstp copies in the exe.
hkpAction* hkpDashpotAction::clone(const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
	const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms) const {
	if (newEntities.m_size != 2 || newPhantoms.m_size) {
		return nullptr;
	}
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	hkpDashpotAction* pMemory = static_cast<hkpDashpotAction*>(pRouter->m_heap->blockAlloc(sizeof(hkpDashpotAction)));
	pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpDashpotAction));
	hkpDashpotAction* pAction = ::new (pMemory) hkpDashpotAction(static_cast<hkpRigidBody*>(newEntities.m_data[0]),
		static_cast<hkpRigidBody*>(newEntities.m_data[1]), m_userData);
	pAction->m_point[0] = m_point[0];
	pAction->m_point[1] = m_point[1];
	pAction->m_strength = m_strength;
	pAction->m_damping = m_damping;
	pAction->m_impulse = m_impulse;
	return pAction;
}

const hkClass& hkpDashpotAction::staticClass() {
	return hkpDashpotActionClass;
}
