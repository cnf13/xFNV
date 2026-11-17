#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkMotionStateTypeInfo = {"hkMotionState", nullptr, cleanupLoadedObjecthkMotionState, nullptr,
	&typeid(hkMotionState), sizeof(hkMotionState)};

// The class record's member table (the SDK's file-local hkMotionStateClass_Members of hkMotionStateClass.cpp).
using hkMotionStateClassMemberTable = const hkClassMember[9];
static hkMotionStateClassMemberTable hkMotionStateClass_Members{
	{"transform", nullptr, nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"sweptTransform", &hkSweptTransformClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, 0,
		0x40, nullptr},
	{"deltaAngle", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x90, nullptr},
	{"objectRadius", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA0, nullptr},
	{"linearDamping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA4, nullptr},
	{"angularDamping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, 0, 0xA8, nullptr},
	{"maxLinearVelocity", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, 0, 0xAC,
		nullptr},
	{"maxAngularVelocity", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, 0, 0xAD,
		nullptr},
	{"deactivationClass", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, 0, 0xAE,
		nullptr}};

hkClass hkMotionStateClass("hkMotionState", nullptr, 176, nullptr, 0, nullptr, 0, hkMotionStateClass_Members, 9,
	nullptr, nullptr, 0, 0);

// The copy assignment, member by member: the vectors, the three floats, the three bytes one by one; the padding byte is
// not copied.
void hkMotionState::Copy(hkMotionState* apSource) {
	m_transform = apSource->m_transform;
	m_sweptTransform = apSource->m_sweptTransform;
	m_deltaAngle = apSource->m_deltaAngle;
	m_objectRadius = apSource->m_objectRadius;
	m_linearDamping = apSource->m_linearDamping;
	m_angularDamping = apSource->m_angularDamping;
	m_maxLinearVelocity = apSource->m_maxLinearVelocity;
	m_maxAngularVelocity = apSource->m_maxAngularVelocity;
	m_deactivationClass = apSource->m_deactivationClass;
}

void hkMotionState::initMotionState(const hkVector4& position, const hkQuaternion& rotation) {
	hkVector4 v4;
	m_sweptTransform.initSweptTransform(position, rotation);
	m_transform.m_rotation.set(rotation);
	v4.m_quad = position.m_quad;
	m_objectRadius = 1.0f;
	m_transform.m_translation.m_quad = v4.m_quad;
	m_deltaAngle.m_quad = _mm_setzero_ps();
}

const hkClass& hkMotionState::staticClass() {
	return hkMotionStateClass;
}
