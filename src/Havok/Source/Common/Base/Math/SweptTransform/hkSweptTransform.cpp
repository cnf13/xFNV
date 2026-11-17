#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

#include <bit>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkSweptTransformTypeInfo = {"hkSweptTransform", nullptr, cleanupLoadedObjecthkSweptTransform, nullptr,
	&typeid(hkSweptTransform), sizeof(hkSweptTransform)};

// The SDK's reflection of hkSweptTransform (the PDB's hkSweptTransformClass.cpp): its five vector and quaternion
// members.
using hkSweptTransformClass_MembersTable = const hkClassMember[5];
static hkSweptTransformClass_MembersTable hkSweptTransformClass_Members{
	{"centerOfMass0", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkSweptTransform, m_centerOfMass0), nullptr},
	{"centerOfMass1", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkSweptTransform, m_centerOfMass1), nullptr},
	{"rotation0", nullptr, nullptr, {hkClassMember::TYPE_QUATERNION}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkSweptTransform, m_rotation0), nullptr},
	{"rotation1", nullptr, nullptr, {hkClassMember::TYPE_QUATERNION}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkSweptTransform, m_rotation1), nullptr},
	{"centerOfMassLocal", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkSweptTransform, m_centerOfMassLocal), nullptr}};
hkClass hkSweptTransformClass("hkSweptTransform", nullptr, sizeof(hkSweptTransform), nullptr, 0, nullptr, 0,
	hkSweptTransformClass_Members, 5, nullptr, nullptr, 0, 0);

void hkSweptTransform::initSweptTransform(const hkVector4& position, const hkQuaternion& rotation) {
	m_centerOfMass0.m_quad = _mm_and_ps(position.m_quad, rdata_10C4C10);
	m_centerOfMass1.m_quad = _mm_and_ps(position.m_quad, rdata_10C4C10);
	m_rotation0.m_vec.m_quad = rotation.m_vec.m_quad;
	m_rotation1.m_vec.m_quad = rotation.m_vec.m_quad;
	m_centerOfMassLocal.m_quad = _mm_setzero_ps();
}

void hkSweptTransform::approxTransformAt(float time, hkTransform& transformOut) {
	__m128 v12 = _mm_undefined_ps();
	uint32_t v16;
	hkQuaternion arQuaternion;
	*reinterpret_cast<float*>(&v16) = (time - m_centerOfMass0.m_quad[3]) * m_centerOfMass1.m_quad[3];
	__m128 v4 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v16)), _mm_set_ss(std::bit_cast<float>(v16)), 0);
	__m128 v5 = _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v4), m_rotation0.m_vec.m_quad);
	__m128 v6 = _mm_mul_ps(m_rotation1.m_vec.m_quad, v4);
	__m128 v7 = _mm_add_ps(v5, v6);
	__m128 v8 = _mm_mul_ps(v7, v7);
	__m128 v9 = _mm_add_ps(_mm_shuffle_ps(v8, v8, 78), v8);
	__m128 v10 = _mm_add_ps(_mm_shuffle_ps(v9, v9, 177), v9);
	float v11 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v10[0])));
	v12[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
	__m128 v13 = _mm_cmpeq_ss(v10, v12);
	v12[0] = 3.0f - static_cast<float>(static_cast<float>(v10[0] * v11) * v11);
	__m128 v14 = _mm_setzero_ps();
	v14[0] = static_cast<float>(0.5f * v11) * v12[0];
	__m128 v15 = _mm_andnot_ps(v13, v14);
	arQuaternion.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(v15, v15, 0), v7);
	transformOut.m_rotation.set(arQuaternion);
	transformOut.m_translation.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v4), m_centerOfMass0.m_quad),
		_mm_mul_ps(m_centerOfMass1.m_quad, v4));
	transformOut.m_translation.m_quad = _mm_sub_ps(transformOut.m_translation.m_quad,
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_centerOfMassLocal.m_quad, m_centerOfMassLocal.m_quad, 85),
								  transformOut.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(m_centerOfMassLocal.m_quad, m_centerOfMassLocal.m_quad, 0),
						   transformOut.m_rotation.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(m_centerOfMassLocal.m_quad, m_centerOfMassLocal.m_quad, 170),
				transformOut.m_rotation.m_col2.m_quad)));
}

const hkClass& hkSweptTransform::staticClass() {
	return hkSweptTransformClass;
}
