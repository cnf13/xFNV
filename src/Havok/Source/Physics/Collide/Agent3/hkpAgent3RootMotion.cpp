#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3RootMotion.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"

#include <xmmintrin.h>

// The motion state's swept transform at times out to its step's end. Each sample uses t = (time - t0) * invDeltaTime,
// kept as a float, the rotations interpolated by t and normalized with sqrtInverse (one Newton step, zero for a zero
// length), then the centre of mass interpolated and the local centre of mass rotated and subtracted. Each sample's
// translation w is the factor 1/cos of its half-angle step (the series 1 - a^2/2 inverted for one sample).
void hkpAgent3CalcRootMotion(const hkMotionState* apMotionState, hkpAgent3RootMotion& arOut) {
	const hkSweptTransform& swept = apMotionState->m_sweptTransform;
	arOut.m_motionState = apMotionState;
	arOut.m_inverse.setInverse(apMotionState->m_transform);
	const __m128 zero = _mm_setzero_ps();
	hkQuaternion q;

	// The transform at the step's start (time t0).
	float t = (swept.m_centerOfMass0.m_quad[3] - swept.m_centerOfMass0.m_quad[3]) * swept.m_centerOfMass1.m_quad[3];
	__m128 tv = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
	__m128 rot = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_rotation0.m_vec.m_quad),
		_mm_mul_ps(swept.m_rotation1.m_vec.m_quad, tv));
	__m128 len2 = _mm_mul_ps(rot, rot);
	len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0x4E), len2);
	len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0xB1), len2);
	float r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(len2[0])));
	__m128 scale = _mm_setzero_ps();
	scale[0] = static_cast<float>(0.5f * r) *
		static_cast<float>(3.0f - static_cast<float>(static_cast<float>(len2[0] * r) * r));
	scale = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), scale);
	q.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), rot);
	arOut.m_transform.m_rotation.set(q);
	hkTransform& start = arOut.m_transform;
	start.m_translation.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_centerOfMass0.m_quad),
		_mm_mul_ps(swept.m_centerOfMass1.m_quad, tv));
	start.m_translation.m_quad = _mm_sub_ps(start.m_translation.m_quad,
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad,
											 swept.m_centerOfMassLocal.m_quad, 0x55),
								  start.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad,
									  0x00),
						   start.m_rotation.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0xAA),
				start.m_rotation.m_col2.m_quad)));

	if (0.0f == swept.m_centerOfMass1.m_quad[3]) {
		arOut.m_numSamples = 0;
		return;
	}
	float deltaTime = 1.0 / swept.m_centerOfMass1.m_quad[3];
	float deltaAngle = apMotionState->m_deltaAngle.m_quad[3];
	if (deltaAngle <= 0.39269909262657166) {
		// One sample: the start transform.
		arOut.m_samples[0] = arOut.m_transform;
		float cosApprox = 1.0 - deltaAngle * deltaAngle * 0.5;
		arOut.m_samples[0].m_translation.m_quad[3] = 1.0 / cosApprox;
		arOut.m_numSamples = 1;
		return;
	}
	if (deltaAngle <= 0.7853981852531433) {
		// One sample: the transform at the step's middle.
		float time = deltaTime * 0.5 + swept.m_centerOfMass0.m_quad[3];
		t = (time - swept.m_centerOfMass0.m_quad[3]) * swept.m_centerOfMass1.m_quad[3];
		tv = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
		rot = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_rotation0.m_vec.m_quad),
			_mm_mul_ps(swept.m_rotation1.m_vec.m_quad, tv));
		len2 = _mm_mul_ps(rot, rot);
		len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0x4E), len2);
		len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0xB1), len2);
		r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(len2[0])));
		scale = _mm_setzero_ps();
		scale[0] = static_cast<float>(0.5f * r) *
			static_cast<float>(3.0f - static_cast<float>(static_cast<float>(len2[0] * r) * r));
		scale = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), scale);
		q.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), rot);
		hkTransform& sample = arOut.m_samples[0];
		sample.m_rotation.set(q);
		sample.m_translation.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_centerOfMass0.m_quad),
				_mm_mul_ps(swept.m_centerOfMass1.m_quad, tv));
		sample.m_translation.m_quad = _mm_sub_ps(sample.m_translation.m_quad,
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad,
												 swept.m_centerOfMassLocal.m_quad, 0x55),
									  sample.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad,
										  0x00),
							   sample.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0xAA),
					sample.m_rotation.m_col2.m_quad)));
		float cosApprox = 1.0 - deltaAngle * deltaAngle * 0.25 * 0.5;
		sample.m_translation.m_quad[3] = 1.0 / cosApprox;
		arOut.m_numSamples = 1;
		return;
	}
	// One sample per pi/8 of rotation: at the odd steps i/n of the step, i = 1, 3, ... below n.
	arOut.m_numSamples = 0;
	float numSteps = (deltaAngle + 0.39269909262657166) / 0.39269909262657166;
	float invNumSteps = 1.0 / numSteps;
	float i = 1.0f;
	double numStepsD = numSteps;
	if (!(numStepsD > 1.0)) {
		return;
	}
	do {
		hkTransform& sample = arOut.m_samples[arOut.m_numSamples++];
		float time = i * invNumSteps * deltaTime + swept.m_centerOfMass0.m_quad[3];
		t = (time - swept.m_centerOfMass0.m_quad[3]) * swept.m_centerOfMass1.m_quad[3];
		tv = _mm_shuffle_ps(_mm_set_ss(t), _mm_set_ss(t), 0);
		rot = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_rotation0.m_vec.m_quad),
			_mm_mul_ps(tv, swept.m_rotation1.m_vec.m_quad));
		len2 = _mm_mul_ps(rot, rot);
		len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0x4E), len2);
		len2 = _mm_add_ps(_mm_shuffle_ps(len2, len2, 0xB1), len2);
		r = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(len2[0])));
		scale = _mm_setzero_ps();
		scale[0] = static_cast<float>(0.5f * r) *
			static_cast<float>(3.0f - static_cast<float>(static_cast<float>(len2[0] * r) * r));
		scale = _mm_andnot_ps(_mm_cmpeq_ss(len2, zero), scale);
		q.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), rot);
		sample.m_rotation.set(q);
		sample.m_translation.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, tv), swept.m_centerOfMass0.m_quad),
				_mm_mul_ps(tv, swept.m_centerOfMass1.m_quad));
		sample.m_translation.m_quad = _mm_sub_ps(sample.m_translation.m_quad,
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad,
												 swept.m_centerOfMassLocal.m_quad, 0x55),
									  sample.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad,
										  0x00),
							   sample.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(swept.m_centerOfMassLocal.m_quad, swept.m_centerOfMassLocal.m_quad, 0xAA),
					sample.m_rotation.m_col2.m_quad)));
		sample.m_translation.m_quad[3] = 1.0824f;
		i = i + 2.0;
	} while (i < numStepsD);
}
