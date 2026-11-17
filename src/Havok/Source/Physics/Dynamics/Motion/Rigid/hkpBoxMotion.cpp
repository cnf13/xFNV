#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <bit>
#include <xmmintrin.h>

hkpBoxMotion::hkpBoxMotion(int32_t a1) : hkpMotion(hkFinishLoadedObjectFlag{a1}) {}

// hkpMotion's constructor with deactivation off, then the box motion type and a unit inertia and inverse mass.
hkpBoxMotion::hkpBoxMotion(const hkVector4& position, const hkQuaternion& rotation)
	: hkpMotion(position, rotation, false) {
	m_type.m_storage = MOTION_BOX_INERTIA;
	m_inertiaAndMassInv.m_quad = _mm_set1_ps(1.0f);
}

hkpBoxMotion::~hkpBoxMotion() {}

void hkpBoxMotion::setMass(float mass) {
	float assertBuf = 1.0 / mass;
	setMassInv(assertBuf);
}

void hkpBoxMotion::getInertiaLocal(hkMatrix3& inertia) const {
	__m128 v2 = _mm_max_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x34000000u)),
							   _mm_set_ss(std::bit_cast<float>(0x34000000u)), 0),
		m_inertiaAndMassInv.m_quad);
	inertia.m_col0.m_quad = _mm_setzero_ps();
	inertia.m_col1.m_quad = _mm_setzero_ps();
	inertia.m_col2.m_quad = _mm_setzero_ps();
	inertia.m_col0.m_quad[0] = 1.0 / v2[0];
	inertia.m_col1.m_quad[1] = 1.0 / v2[1];
	inertia.m_col2.m_quad[2] = 1.0 / v2[2];
}

void hkpBoxMotion::getInertiaWorld(hkMatrix3& inertia) const {
	__m128 v2 = _mm_max_ps(m_inertiaAndMassInv.m_quad, hkQuadRealEps);
	inertia.m_col0.m_quad = _mm_setzero_ps();
	inertia.m_col1.m_quad = _mm_setzero_ps();
	inertia.m_col2.m_quad = _mm_setzero_ps();
	inertia.m_col0.m_quad[0] = 1.0 / v2[0];
	inertia.m_col1.m_quad[1] = 1.0 / v2[1];
	inertia.m_col2.m_quad[2] = 1.0 / v2[2];
	inertia.changeBasis(m_motionState.m_transform.m_rotation);
}

void hkpBoxMotion::setInertiaLocal(const hkMatrix3& inertia) {
	uint32_t v2;
	uint32_t v3;
	uint32_t v4;
	*reinterpret_cast<float*>(&v2) = 1.0 / inertia.m_col0.m_quad[0];
	*reinterpret_cast<float*>(&v3) = 1.0 / inertia.m_col1.m_quad[1];
	*reinterpret_cast<float*>(&v4) = 1.0 / inertia.m_col2.m_quad[2];
	m_inertiaAndMassInv.m_quad =
		_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v2)), _mm_set_ss(std::bit_cast<float>(v4))),
			_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v3)),
				_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_inertiaAndMassInv.m_quad[3])))));
}

void hkpBoxMotion::setInertiaInvLocal(const hkMatrix3& inertiaInv) {
	m_inertiaAndMassInv.m_quad =
		_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(inertiaInv.m_col0
												.m_quad[0]))),
							_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(inertiaInv.m_col2.m_quad[2])))),
			_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(inertiaInv.m_col1.m_quad[1]))),
				_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_inertiaAndMassInv.m_quad[3])))));
}

void hkpBoxMotion::getInertiaInvLocal(hkMatrix3& inertiaInv) const {
	float v3 = m_inertiaAndMassInv.m_quad[0];
	float v4 = m_inertiaAndMassInv.m_quad[1];
	double v2 = m_inertiaAndMassInv.m_quad[2];
	inertiaInv.m_col0.m_quad = _mm_setzero_ps();
	float v5 = v2;
	inertiaInv.m_col1.m_quad = _mm_setzero_ps();
	inertiaInv.m_col2.m_quad = _mm_setzero_ps();
	inertiaInv.m_col0.m_quad[0] = v3;
	inertiaInv.m_col1.m_quad[1] = v4;
	inertiaInv.m_col2.m_quad[2] = v5;
}

void hkpBoxMotion::getInertiaInvWorld(hkMatrix3& inertiaInv) const {
	hkVector4 v2;
	hkVector4 v3;
	hkVector4 v4;
	hkMatrix3 v5;
	v2.m_quad = m_inertiaAndMassInv.m_quad;
	v5.m_col0.m_quad =
		_mm_mul_ps(_mm_shuffle_ps(v2.m_quad, v2.m_quad, 0), m_motionState.m_transform.m_rotation.m_col0.m_quad);
	v3.m_quad =
		_mm_mul_ps(_mm_shuffle_ps(v2.m_quad, v2.m_quad, 85), m_motionState.m_transform.m_rotation.m_col1.m_quad);
	v4.m_quad = m_inertiaAndMassInv.m_quad;
	v5.m_col1.m_quad = v3.m_quad;
	v5.m_col2.m_quad =
		_mm_mul_ps(_mm_shuffle_ps(v4.m_quad, v4.m_quad, 170), m_motionState.m_transform.m_rotation.m_col2.m_quad);
	inertiaInv.setMulInverse(v5, m_motionState.m_transform.m_rotation);
}

void hkpBoxMotion::getProjectedPointVelocity(const hkVector4& pos, const hkVector4& normal, float& velOut,
	float& invVirtMassOut) const {
	hkVector4 v11;
	hkVector4 v12;
	hkVector4 v13;
	__m128 v5 = _mm_sub_ps(pos.m_quad, m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
	__m128 v6 = _mm_mul_ps(_mm_shuffle_ps(normal.m_quad, normal.m_quad, 201), _mm_shuffle_ps(v5, v5, 210));
	__m128 v7 = _mm_mul_ps(_mm_shuffle_ps(normal.m_quad, normal.m_quad, 210), _mm_shuffle_ps(v5, v5, 201));
	__m128 v8 = _mm_mul_ps(m_linearVelocity.m_quad, normal.m_quad);
	__m128 v9 = _mm_sub_ps(v7, v6);
	__m128 v10 = _mm_mul_ps(m_angularVelocity.m_quad, v9);
	velOut = static_cast<float>(_mm_shuffle_ps(v10, v10, 170)[0] +
				 static_cast<float>(_mm_shuffle_ps(v10, v10, 85)[0] + v10[0])) +
		static_cast<float>(_mm_shuffle_ps(v8, v8, 170)[0] + static_cast<float>(_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]));
	v11.m_quad = m_motionState.m_transform.m_rotation.m_col1.m_quad;
	v12.m_quad = m_motionState.m_transform.m_rotation.m_col2.m_quad;
	v13.m_quad = m_motionState.m_transform.m_rotation.m_col0.m_quad;
	__m128 v14 = _mm_unpackhi_ps(v13.m_quad, v11.m_quad);
	__m128 v15 = _mm_unpacklo_ps(v13.m_quad, v11.m_quad);
	__m128 v16 = _mm_movelh_ps(v15, v12.m_quad);
	__m128 v17 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v9, v9, 85),
										   _mm_shuffle_ps(_mm_movehl_ps(v16, v15), v12.m_quad, 212)),
								_mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), v16)),
		_mm_mul_ps(_mm_shuffle_ps(v14, v12.m_quad, 228), _mm_shuffle_ps(v9, v9, 170)));
	__m128 v18 = _mm_mul_ps(_mm_mul_ps(m_inertiaAndMassInv.m_quad, v17), v17);
	invVirtMassOut = m_inertiaAndMassInv.m_quad[3] +
		static_cast<float>(_mm_shuffle_ps(v18, v18, 170)[0] +
			static_cast<float>(_mm_shuffle_ps(v18, v18, 85)[0] + v18[0]));
}

void hkpBoxMotion::applyPointImpulse(const hkVector4& imp, const hkVector4& p) {
	hkVector4 v4;
	hkVector4 v7;
	hkVector4 v8;
	__m128 v3 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
	m_linearVelocity.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0), imp.m_quad), m_linearVelocity.m_quad);
	v4.m_quad = m_motionState.m_transform.m_rotation.m_col2.m_quad;
	__m128 v5 = _mm_sub_ps(p.m_quad, m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
	__m128 v6 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 210), _mm_shuffle_ps(v5, v5, 201)),
		_mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 201), _mm_shuffle_ps(v5, v5, 210)));
	v7.m_quad = m_motionState.m_transform.m_rotation.m_col1.m_quad;
	v8.m_quad = m_motionState.m_transform.m_rotation.m_col0.m_quad;
	__m128 v9 = _mm_unpackhi_ps(v8.m_quad, v7.m_quad);
	__m128 v10 = _mm_unpacklo_ps(v8.m_quad, v7.m_quad);
	__m128 v11 = _mm_movelh_ps(v10, v4.m_quad);
	__m128 v12 = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 85),
													  _mm_shuffle_ps(_mm_movehl_ps(v11, v10), v4.m_quad, 212)),
										   _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), v11)),
								_mm_mul_ps(_mm_shuffle_ps(v9, v4.m_quad, 228), _mm_shuffle_ps(v6, v6, 170))),
		m_inertiaAndMassInv.m_quad);
	m_angularVelocity.m_quad = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v12, v12, 85), v7.m_quad),
														 _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0),
															 m_motionState.m_transform.m_rotation.m_col0.m_quad)),
											  _mm_mul_ps(_mm_shuffle_ps(v12, v12, 170), v4.m_quad)),
		m_angularVelocity.m_quad);
}

void hkpBoxMotion::applyAngularImpulse(const hkVector4& imp) {
	hkVector4 v2;
	hkVector4 v3;
	v2.m_quad = m_motionState.m_transform.m_rotation.m_col1.m_quad;
	v3.m_quad = m_motionState.m_transform.m_rotation.m_col2.m_quad;
	__m128 v4 = _mm_unpacklo_ps(m_motionState.m_transform.m_rotation.m_col0.m_quad, v2.m_quad);
	__m128 v5 = _mm_movelh_ps(v4, v3.m_quad);
	__m128 v6 = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 85),
													 _mm_shuffle_ps(_mm_movehl_ps(v5, v4), v3.m_quad, 212)),
										  _mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 0), v5)),
							   _mm_mul_ps(_mm_shuffle_ps(imp.m_quad, imp.m_quad, 170),
								   _mm_shuffle_ps(_mm_unpackhi_ps(m_motionState.m_transform.m_rotation.m_col0.m_quad,
													  v2.m_quad),
									   v3.m_quad, 228))),
		m_inertiaAndMassInv.m_quad);
	m_angularVelocity.m_quad = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 85), v2.m_quad),
														 _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0),
															 m_motionState.m_transform.m_rotation.m_col0.m_quad)),
											  _mm_mul_ps(_mm_shuffle_ps(v6, v6, 170), v3.m_quad)),
		m_angularVelocity.m_quad);
}

void hkpBoxMotion::applyForce(float deltaTime, const hkVector4& force) {
	__m128 v3 = _mm_shuffle_ps(m_inertiaAndMassInv.m_quad, m_inertiaAndMassInv.m_quad, 255);
	m_linearVelocity.m_quad =
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v3, v3, 0),
					   _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), force.m_quad)),
			m_linearVelocity.m_quad);
}

void hkpBoxMotion::applyForce(float deltaTime, const hkVector4& force, const hkVector4& p) {
	hkVector4 v5;
	v5.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), force.m_quad);
	this->applyPointImpulse(v5, p);
}

void hkpBoxMotion::applyTorque(float deltaTime, const hkVector4& torque) {
	hkVector4 v4;
	v4.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(deltaTime), _mm_set_ss(deltaTime), 0), torque.m_quad);
	this->applyAngularImpulse(v4);
}

const hkClass& hkpBoxMotion::staticClass() {
	return hkpBoxMotionClass;
}
