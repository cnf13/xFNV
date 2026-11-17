#include "Havok/Source/Physics/Dynamics/Motion/Util/hkpRigidMotionUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransformUtil.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

float calcVelocityForDeactivation(const hkpMotion& motion, float objectRadius) {
	const float linVelSqrd = motion.getLinearVelocity().lengthSquared3();
	const float angVelSqrd = motion.getAngularVelocity().lengthSquared3();
	return objectRadius * objectRadius * angVelSqrd + linVelSqrd;
}

// The SDK's final deactivation test of an island's motions: false as soon as one motion's estimated movement, a
// quarter of its squared linear speed plus its squared angular speed times its radius (at most 1) squared, less 0.01,
// exceeds the squared distance of the reference position its larger inactive-frame count selects. info is unused.
bool hkRigidMotionUtilCanDeactivateFinal(const hkStepInfo& info, hkpMotion* const* motions, int32_t numMotions,
	int32_t motionOffset) {
	for (int32_t i = numMotions - 1; i >= 0; motions++, i--) {
		// Each element points motionOffset raw bytes before its motion (an entity's embedded motion), so the offset is
		// added as an untyped byte offset.
		// EXE-INLINED none: hkAddByteOffset<hkpMotion>
		const hkpMotion* motion =
			reinterpret_cast<const hkpMotion*>(reinterpret_cast<const char*>(motions[0]) + motionOffset);
		float fRadius = motion->m_motionState.m_objectRadius;
		if (fRadius > 1.0f) {
			fRadius = 1.0f;
		}
		const hkQuadReal lin = _mm_mul_ps(motion->m_linearVelocity.m_quad, motion->m_linearVelocity.m_quad);
		float fLinSqrd =
			_mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(lin, lin, 0xaa), _mm_add_ps(_mm_shuffle_ps(lin, lin, 0x55), lin)));
		const hkQuadReal ang = _mm_mul_ps(motion->m_angularVelocity.m_quad, motion->m_angularVelocity.m_quad);
		float fAngSqrd =
			_mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(ang, ang, 0xaa), _mm_add_ps(_mm_shuffle_ps(ang, ang, 0x55), ang)));
		const double dRadius = fRadius;
		float fVelSqrd = dRadius * dRadius * fAngSqrd + fLinSqrd;
		float fEstimate = fVelSqrd * 0.25 - 0.010000000298023226;
		// EXE-INLINED none: hkpMotion::getNumInactiveFrames
		const int32_t iSelect =
			(motion->m_deactivationNumInactiveFrames[0] & 0x7f) > (motion->m_deactivationNumInactiveFrames[1] & 0x7f)
			? 0
			: 1;
		if (motion->m_deactivationRefPosition[iSelect](3) < fEstimate) {
			return false;
		}
	}
	return true;
}

// The SDK's step, for each motion that is not fixed: velocities with a NaN or a lane at or above 1e6 in x, y or z are
// both reset, the linear velocity is clipped to the motion's maximum, the centre of mass moves over info's step, the
// rotation turns by the angular step (clipped to the maximum angular velocity times the step, at most 0.9 radians,
// with the angular velocity scaled to match), and the transform is rebuilt from the new swept transform.
void hkRigidMotionUtilStep(const hkStepInfo& info, hkpMotion* const* motions, int32_t numMotions,
	int32_t motionOffset) {
	for (int32_t i = numMotions - 1; i >= 0; motions++, i--) {
		// Each element points motionOffset raw bytes before its motion (an entity's embedded motion), so the offset is
		// added as an untyped byte offset.
		// EXE-INLINED none: hkAddByteOffset<hkpMotion>
		hkpMotion* motion = reinterpret_cast<hkpMotion*>(reinterpret_cast<char*>(motions[0]) + motionOffset);
		if (motion->getType() == hkpMotion::MOTION_FIXED) {
			continue;
		}
		hkMotionState& ms = motion->m_motionState;
		hkSweptTransform& st = ms.m_sweptTransform;

		// Validity test: x, y and z of both velocities ordered and of magnitude below 1e6. Otherwise both are reset to
		// hkTransformIdentity's first column, as the game does.
		{
			const hkQuadReal lin = motion->m_linearVelocity.m_quad;
			const hkQuadReal ang = motion->m_angularVelocity.m_quad;
			const int32_t iOk = _mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(ang, hkVector4_setAbs4_absMask),
									hkSweptTransformUtil::_stepMotionStateMaxVelf)) &
				_mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(lin, hkVector4_setAbs4_absMask),
					hkSweptTransformUtil::_stepMotionStateMaxVelf)) &
				_mm_movemask_ps(_mm_cmpeq_ps(ang, ang)) & _mm_movemask_ps(_mm_cmpeq_ps(lin, lin));
			if ((iOk & 7) != 7) {
				motion->m_linearVelocity = hkTransformIdentity.m_rotation.m_col0;
				motion->m_angularVelocity = hkTransformIdentity.m_rotation.m_col0;
			}
		}

		st.m_centerOfMass0 = st.m_centerOfMass1;
		st.m_centerOfMass0(3) = info.m_startTime;

		// Clip the linear velocity: the squared length (summed z + (y + x)) against the squared maximum.
		{
			const hkVector4 linVel = motion->m_linearVelocity;
			const float maxLinVel = hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
			const hkQuadReal sq = _mm_mul_ps(linVel.m_quad, linVel.m_quad);
			const float linVelSqrd = sq[2] + static_cast<float>(sq[1] + sq[0]);
			if (linVelSqrd > maxLinVel * maxLinVel) {
				const float fRoot = std::sqrt(linVelSqrd);
				const float fInvRoot = 1.0f / fRoot;
				const float f = fInvRoot * maxLinVel;
				motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), linVel.m_quad);
			}
		}

		const float fDeltaTime = info.m_deltaTime;
		st.m_centerOfMass1.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_set1_ps(fDeltaTime), motion->m_linearVelocity.m_quad), st.m_centerOfMass1.m_quad);
		st.m_centerOfMass1(3) = info.m_invDeltaTime;

		hkQuaternion rotation = st.m_rotation1;
		st.m_rotation0 = rotation;

		// The angular step dq = angVel * dt / 2; pre is its squared length times 4 / pi^2, the polynomial's argument
		// for dq's w.
		const hkVector4 angVel = motion->m_angularVelocity;
		const float fHalfDeltaTime = info.m_deltaTime * 0.5;
		hkQuaternion dq;
		dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(fHalfDeltaTime), angVel.m_quad);
		const hkQuadReal dqSq = _mm_mul_ps(dq.m_vec.m_quad, dq.m_vec.m_quad);
		const float dqLenSqrd = dqSq[2] + static_cast<float>(dqSq[1] + dqSq[0]);
		float pre = dqLenSqrd * 0.40528471201320165;
		float maxAngle = hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value] * info.m_deltaTime;
		if (maxAngle > 0.8999999761581421) {
			maxAngle = 0.9f;
		}
		const float maxAngleSqrd = maxAngle * maxAngle;
		// Taken also when pre is a NaN.
		if (!(maxAngleSqrd >= pre)) {
			const float fRoot = std::sqrt(pre);
			const float fInvRoot = 1.0f / fRoot;
			const float f = fInvRoot * maxAngle;
			motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), angVel.m_quad);
			dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(f), dq.m_vec.m_quad);
			pre = maxAngleSqrd;
		}
		const float pre2 = pre * pre;
		dq.m_vec(3) = 1.0 - 0.8229479789733887 * pre - 0.1305290013551712 * pre2 - pre2 * (pre * 0.04440800100564957);

		rotation.setMul(dq, rotation);
		rotation.normalize();
		ms.m_deltaAngle.setAdd4(dq.m_vec, dq.m_vec);
		const float fAngle = std::sqrt(pre);
		ms.m_deltaAngle(3) = fAngle * 3.1415927410125732;
		st.m_rotation1 = rotation;
		ms.m_transform.m_rotation.set(st.m_rotation1);

		// The translation is the centre of mass less the rotated local centre of mass, rotated as
		// (y * col1 + x * col0) + z * col2.
		// EXE-INLINED 0xC7F700
		const hkQuadReal local = st.m_centerOfMassLocal.m_quad;
		const hkRotation& r = ms.m_transform.m_rotation;
		const hkQuadReal shift = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local, local, 0x55), r.m_col1.m_quad),
												_mm_mul_ps(_mm_shuffle_ps(local, local, 0), r.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(local, local, 0xaa), r.m_col2.m_quad));
		ms.m_transform.m_translation.m_quad = _mm_sub_ps(st.m_centerOfMass1.m_quad, shift);
	}
}

// The SDK's accumulator build, one accumulator per motion (motion type 1, dynamic, and unknown types leave theirs
// untouched): damping on the velocities of every motion but keyframed and fixed ones, then the accumulator: box
// inertia (and invalid, thin box) gets its inertia frame as the transpose of the motion's rotation and its angular
// velocity in that frame; sphere and character motions keep identity frames; keyframed motions get infinite masses
// (zero inverse); fixed motions an all-zero accumulator. Returns the accumulator after the last one.
hkpVelocityAccumulator* hkRigidMotionUtilApplyForcesAndBuildAccumulators(const hkStepInfo& info,
	hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset, hkpVelocityAccumulator* accumulatorsOut) {
	for (int32_t i = numMotions - 1; i >= 0; motions++, accumulatorsOut++, i--) {
		// Each element points motionOffset raw bytes before its motion (an entity's embedded motion), so the offset is
		// added as an untyped byte offset.
		// EXE-INLINED none: hkAddByteOffset<hkpMotion>
		hkpMotion* motion = reinterpret_cast<hkpMotion*>(reinterpret_cast<char*>(motions[0]) + motionOffset);
		hkMotionState& ms = motion->m_motionState;
		hkSweptTransform& st = ms.m_sweptTransform;
		switch (motion->getType()) {
		case hkpMotion::MOTION_INVALID:
		case hkpMotion::MOTION_BOX_INERTIA:
		case hkpMotion::MOTION_THIN_BOX_INERTIA: {
			// Damping: each velocity times 1 - damping * dt, rounded to float and at least 0 (a NaN is kept).
			float fLinDampStore = 1.0f - ms.m_linearDamping * info.m_deltaTime;
			float fLinDamp = fLinDampStore;
			if (fLinDamp < 0.0f) {
				fLinDamp = 0.0f;
			}
			motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fLinDamp), motion->m_linearVelocity.m_quad);
			float fAngDampStore = 1.0f - info.m_deltaTime * ms.m_angularDamping;
			float fAngDamp = fAngDampStore;
			if (fAngDamp < 0.0f) {
				fAngDamp = 0.0f;
			}
			motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fAngDamp), motion->m_angularVelocity.m_quad);

			accumulatorsOut->m_scratch123.setTranspose(ms.m_transform.m_rotation);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			accumulatorsOut->m_type.m_storage = hkpVelocityAccumulator::HK_RIGID_BODY;
			accumulatorsOut->m_invMasses = motion->m_inertiaAndMassInv;
			accumulatorsOut->m_linearVel = motion->m_linearVelocity;
			{
				// EXE-INLINED none: hkSweptTransform::getInterpolationValue
				float fT = (info.m_startTime - st.m_centerOfMass0(3)) * st.m_centerOfMass1(3);
				const hkQuadReal t = _mm_set1_ps(fT);
				// EXE-INLINED 0x5DBF70
				accumulatorsOut->m_scratch0.m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), st.m_centerOfMass0.m_quad),
						_mm_mul_ps(st.m_centerOfMass1.m_quad, t));
			}
			{
				// The angular velocity in the inertia frame, (y * col1 + x * col0) + z * col2.
				// EXE-INLINED 0xC7F700
				const hkQuadReal av = motion->m_angularVelocity.m_quad;
				const hkRotation& r = accumulatorsOut->m_scratch123;
				accumulatorsOut->m_angularVel.m_quad =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(av, av, 0x55), r.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(av, av, 0), r.m_col0.m_quad)),
						_mm_mul_ps(_mm_shuffle_ps(av, av, 0xaa), r.m_col2.m_quad));
			}
			accumulatorsOut->m_deactivationClass = ms.m_deactivationClass;
			// EXE-INLINED none: hkHalf::operator float
			accumulatorsOut->m_gravityFactor =
				std::bit_cast<float>(static_cast<int32_t>(motion->m_gravityFactor.m_value) << 16);
			break;
		}
		case hkpMotion::MOTION_SPHERE_INERTIA:
		case hkpMotion::MOTION_CHARACTER: {
			// EXE-INLINED none: hkEnum<E, S>::operator=
			accumulatorsOut->m_type.m_storage = motion->getType() == hkpMotion::MOTION_CHARACTER
				? hkpVelocityAccumulator::HK_NO_GRAVITY_RIGID_BODY
				: hkpVelocityAccumulator::HK_RIGID_BODY;
			// Damping, as above.
			float fLinDampStore = 1.0f - ms.m_linearDamping * info.m_deltaTime;
			float fLinDamp = fLinDampStore;
			if (fLinDamp < 0.0f) {
				fLinDamp = 0.0f;
			}
			motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fLinDamp), motion->m_linearVelocity.m_quad);
			float fAngDampStore = 1.0f - info.m_deltaTime * ms.m_angularDamping;
			float fAngDamp = fAngDampStore;
			if (fAngDamp < 0.0f) {
				fAngDamp = 0.0f;
			}
			motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fAngDamp), motion->m_angularVelocity.m_quad);

			accumulatorsOut->m_invMasses = motion->m_inertiaAndMassInv;
			{
				// EXE-INLINED none: hkSweptTransform::getInterpolationValue
				float fT = (info.m_startTime - st.m_centerOfMass0(3)) * st.m_centerOfMass1(3);
				const hkQuadReal t = _mm_set1_ps(fT);
				// EXE-INLINED 0x5DBF70
				accumulatorsOut->m_scratch0.m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), st.m_centerOfMass0.m_quad),
						_mm_mul_ps(st.m_centerOfMass1.m_quad, t));
			}
			accumulatorsOut->m_angularVel = motion->m_angularVelocity;
			accumulatorsOut->m_linearVel = motion->m_linearVelocity;
			// EXE-INLINED none: hkMatrix3::setIdentity
			accumulatorsOut->m_scratch123.m_col0.setZero4();
			accumulatorsOut->m_scratch123.m_col1.setZero4();
			accumulatorsOut->m_scratch123.m_col2.setZero4();
			accumulatorsOut->m_scratch123.m_col0(0) = 1.0f;
			accumulatorsOut->m_scratch123.m_col1(1) = 1.0f;
			accumulatorsOut->m_scratch123.m_col2(2) = 1.0f;
			accumulatorsOut->m_deactivationClass = ms.m_deactivationClass;
			// EXE-INLINED none: hkHalf::operator float
			accumulatorsOut->m_gravityFactor =
				std::bit_cast<float>(static_cast<int32_t>(motion->m_gravityFactor.m_value) << 16);
			break;
		}
		case hkpMotion::MOTION_KEYFRAMED: {
			// No damping; infinite masses, the centre of mass at the step's end, an identity frame. The
			// deactivation class is left as it was.
			// EXE-INLINED none: hkEnum<E, S>::operator=
			accumulatorsOut->m_type.m_storage = hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY;
			accumulatorsOut->m_invMasses.setZero4();
			accumulatorsOut->m_scratch0 = st.m_centerOfMass1;
			accumulatorsOut->m_angularVel = motion->m_angularVelocity;
			accumulatorsOut->m_linearVel = motion->m_linearVelocity;
			// EXE-INLINED none: hkMatrix3::setIdentity
			accumulatorsOut->m_scratch123.m_col0.setZero4();
			accumulatorsOut->m_scratch123.m_col1.setZero4();
			accumulatorsOut->m_scratch123.m_col2.setZero4();
			accumulatorsOut->m_scratch123.m_col0(0) = 1.0f;
			accumulatorsOut->m_scratch123.m_col1(1) = 1.0f;
			accumulatorsOut->m_scratch123.m_col2(2) = 1.0f;
			// EXE-INLINED none: hkHalf::operator float
			accumulatorsOut->m_gravityFactor =
				std::bit_cast<float>(static_cast<int32_t>(motion->m_gravityFactor.m_value) << 16);
			break;
		}
		case hkpMotion::MOTION_FIXED: {
			// All zero but the type; the deactivation class is left as it was.
			accumulatorsOut->m_gravityFactor = 0.0f;
			// EXE-INLINED none: hkEnum<E, S>::operator=
			accumulatorsOut->m_type.m_storage = hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY;
			accumulatorsOut->m_invMasses.setZero4();
			accumulatorsOut->m_linearVel.setZero4();
			accumulatorsOut->m_angularVel.setZero4();
			accumulatorsOut->m_scratch0.setZero4();
			accumulatorsOut->m_scratch123.m_col0.setZero4();
			accumulatorsOut->m_scratch123.m_col1.setZero4();
			accumulatorsOut->m_scratch123.m_col2.setZero4();
			break;
		}
		default:
			break;
		}
	}
	return accumulatorsOut;
}

// For each motion that is not fixed: gravity (deltaVel times the motion's gravity factor; not for keyframed and
// character motions) and damping (not for keyframed motions) on the velocities, a step of the motion state (as
// hkRigidMotionUtilStep), and the deactivation check every fourth step. A thin box keeps its angular momentum over the
// step and has its velocities clipped after it. Returns the fewest frames any of the motions has been inactive
// (0x7FFFFFFF when none counts).
int32_t hkRigidMotionUtilApplyForcesAndStep(const hkpSolverInfo& solverInfo, const hkStepInfo& stepInfo,
	const hkVector4& deltaVel, hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset) {
	int32_t numInactiveFrames = 0x7FFFFFFF;
	for (int32_t i = numMotions - 1; i >= 0; motions++, i--) {
		// Each element points motionOffset raw bytes before its motion (an entity's embedded motion), so the offset is
		// added as an untyped byte offset.
		// EXE-INLINED none: hkAddByteOffset<hkpMotion>
		hkpMotion* motion = reinterpret_cast<hkpMotion*>(reinterpret_cast<char*>(motions[0]) + motionOffset);
		const float gravityFactor = motion->m_gravityFactor;
		const hkpMotion::MotionType type = motion->getType();
		if (type == hkpMotion::MOTION_FIXED) {
			continue;
		}
		hkMotionState& ms = motion->m_motionState;
		hkSweptTransform& st = ms.m_sweptTransform;

		if (type != hkpMotion::MOTION_KEYFRAMED) {
			if (type != hkpMotion::MOTION_CHARACTER) {
				motion->m_linearVelocity.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(gravityFactor), deltaVel.m_quad),
					motion->m_linearVelocity.m_quad);
			}
			// Damping: each velocity scaled by 1 - damping * dt, at least 0.
			const float fDeltaTime = stepInfo.m_deltaTime;
			float linDamp = 1.0f - ms.m_linearDamping * fDeltaTime;
			if (linDamp < 0.0f) {
				linDamp = 0.0f;
			}
			motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(linDamp), motion->m_linearVelocity.m_quad);
			float angDamp = 1.0f - fDeltaTime * ms.m_angularDamping;
			if (angDamp < 0.0f) {
				angDamp = 0.0f;
			}
			motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(angDamp), motion->m_angularVelocity.m_quad);
		}

		// A thin box's world angular momentum: the angular velocity in body space times the inertia (the refined
		// reciprocal of the inverse inertia), rotated back.
		hkVector4 angularMomentum;
		if (type == hkpMotion::MOTION_THIN_BOX_INERTIA) {
			hkVector4 localAngVel;
			localAngVel.setRotatedInverseDir(ms.m_transform.m_rotation, motion->m_angularVelocity);
			hkVector4 inertia;
			inertia.setReciprocal4(motion->m_inertiaAndMassInv);
			localAngVel.m_quad = _mm_mul_ps(inertia.m_quad, localAngVel.m_quad);
			angularMomentum.setRotatedDir(ms.m_transform.m_rotation, localAngVel);
		}

		// EXE-INLINED none: hkSweptTransformUtil::_stepMotionState
		// The SDK's validity test: x, y and z of both velocities ordered and of magnitude below 1e6. The exe resets
		// both to the 16 bytes at 0x1268370, hkTransformIdentity's first column.
		{
			const hkQuadReal lin = motion->m_linearVelocity.m_quad;
			const hkQuadReal ang = motion->m_angularVelocity.m_quad;
			const int32_t iOk = _mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(lin, hkVector4_setAbs4_absMask),
									hkSweptTransformUtil::_stepMotionStateMaxVelf)) &
				_mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(ang, hkVector4_setAbs4_absMask),
					hkSweptTransformUtil::_stepMotionStateMaxVelf)) &
				_mm_movemask_ps(_mm_cmpeq_ps(lin, lin)) & _mm_movemask_ps(_mm_cmpeq_ps(ang, ang));
			if ((iOk & 7) != 7) {
				motion->m_linearVelocity = hkTransformIdentity.m_rotation.m_col0;
				motion->m_angularVelocity = hkTransformIdentity.m_rotation.m_col0;
			}
		}

		st.m_centerOfMass0 = st.m_centerOfMass1;
		st.m_centerOfMass0(3) = stepInfo.m_startTime;

		// Clip the linear velocity: the squared length (summed z + (y + x)) against the squared maximum.
		{
			const hkVector4 linVel = motion->m_linearVelocity;
			const float maxLinVel = hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
			const hkQuadReal sq = _mm_mul_ps(linVel.m_quad, linVel.m_quad);
			const float linVelSqrd = sq[2] + static_cast<float>(sq[1] + sq[0]);
			if (linVelSqrd > maxLinVel * maxLinVel) {
				const float fRoot = std::sqrt(linVelSqrd);
				const float fInvRoot = 1.0f / fRoot;
				const float f = fInvRoot * hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
				motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), linVel.m_quad);
			}
		}

		const float fDeltaTime = stepInfo.m_deltaTime;
		st.m_centerOfMass1.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_set1_ps(fDeltaTime), motion->m_linearVelocity.m_quad), st.m_centerOfMass1.m_quad);
		st.m_centerOfMass1(3) = stepInfo.m_invDeltaTime;

		hkQuaternion rotation = st.m_rotation1;
		st.m_rotation0 = rotation;

		// The angular step dq = angVel * dt / 2; pre is its squared length times 4 / pi^2, the polynomial's argument
		// for dq's w.
		const hkVector4 angVel = motion->m_angularVelocity;
		const float fHalfDeltaTime = stepInfo.m_deltaTime * 0.5;
		hkQuaternion dq;
		dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(fHalfDeltaTime), angVel.m_quad);
		const hkQuadReal dqSq = _mm_mul_ps(dq.m_vec.m_quad, dq.m_vec.m_quad);
		const float dqLenSqrd = dqSq[2] + static_cast<float>(dqSq[1] + dqSq[0]);
		float pre = dqLenSqrd * 0.40528471201320165;
		float maxAngle = hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value] * stepInfo.m_deltaTime;
		if (maxAngle > 0.8999999761581421) {
			maxAngle = 0.9f;
		}
		const float maxAngleSqrd = maxAngle * maxAngle;
		// Taken also when pre is a NaN.
		if (!(maxAngleSqrd >= pre)) {
			const float fRoot = std::sqrt(pre);
			const float fInvRoot = 1.0f / fRoot;
			const float f = fInvRoot * maxAngle;
			motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), angVel.m_quad);
			dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(f), dq.m_vec.m_quad);
			pre = maxAngleSqrd;
		}
		const float pre2 = pre * pre;
		dq.m_vec(3) = 1.0 - 0.8229479789733887 * pre - 0.1305290013551712 * pre2 - pre2 * (pre * 0.04440800100564957);

		rotation.setMul(dq, rotation);
		rotation.normalize();
		ms.m_deltaAngle.setAdd4(dq.m_vec, dq.m_vec);
		const float fAngle = std::sqrt(pre);
		ms.m_deltaAngle(3) = fAngle * 3.1415927410125732;
		st.m_rotation1 = rotation;
		ms.m_transform.m_rotation.set(st.m_rotation1);

		// The translation is the centre of mass less the rotated local centre of mass, rotated as
		// (y * col1 + x * col0) + z * col2.
		// EXE-INLINED 0xC7F700
		{
			const hkQuadReal local = st.m_centerOfMassLocal.m_quad;
			const hkRotation& r = ms.m_transform.m_rotation;
			const hkQuadReal shift =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local, local, 0x55), r.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(local, local, 0), r.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(local, local, 0xaa), r.m_col2.m_quad));
			ms.m_transform.m_translation.m_quad = _mm_sub_ps(st.m_centerOfMass1.m_quad, shift);
		}

		if (type == hkpMotion::MOTION_THIN_BOX_INERTIA) {
			// The angular velocity again from the kept momentum, under the new rotation.
			hkVector4 localMomentum;
			localMomentum.setRotatedInverseDir(ms.m_transform.m_rotation, angularMomentum);
			localMomentum.m_quad = _mm_mul_ps(localMomentum.m_quad, motion->m_inertiaAndMassInv.m_quad);
			motion->m_angularVelocity.setRotatedDir(ms.m_transform.m_rotation, localMomentum);

			// EXE-INLINED none: hkSweptTransformUtil::_clipVelocities
			// Each velocity above its maximum, or a NaN, is scaled to the maximum; a NaN one is then reset to
			// hkTransformIdentity's first column. The squared maxima are rounded to float.
			const hkVector4 linVel = motion->m_linearVelocity;
			const float maxAngular = hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value] *
				hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value];
			const hkQuadReal linSq = _mm_mul_ps(linVel.m_quad, linVel.m_quad);
			const float linVelSqrd = linSq[2] + static_cast<float>(linSq[1] + linSq[0]);
			const hkQuadReal angSq = _mm_mul_ps(motion->m_angularVelocity.m_quad, motion->m_angularVelocity.m_quad);
			const float angVelSqrd = angSq[2] + static_cast<float>(angSq[1] + angSq[0]);
			const float maxLinear = hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value] *
				hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
			if (linVelSqrd > maxLinear || linVelSqrd != linVelSqrd) {
				const float fRoot = std::sqrt(linVelSqrd);
				const float fInvRoot = 1.0f / fRoot;
				const float f = fInvRoot * hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
				motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), linVel.m_quad);
				if (linVelSqrd != linVelSqrd) {
					motion->m_linearVelocity = hkTransformIdentity.m_rotation.m_col0;
				}
			}
			if (angVelSqrd > maxAngular || angVelSqrd != angVelSqrd) {
				const float fRoot = std::sqrt(angVelSqrd);
				const float fInvRoot = 1.0f / fRoot;
				const float f = fInvRoot * hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value];
				motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), motion->m_angularVelocity.m_quad);
				if (angVelSqrd != angVelSqrd) {
					motion->m_angularVelocity = hkTransformIdentity.m_rotation.m_col0;
				}
			}
		}

		// The deactivation check, every fourth step: the counter's 16th step checks the slow reference (index 1)
		// and restarts the counter, the other checks the fast one (index 0); a counter that wraps stays at 0xFF.
		const int32_t counter = motion->m_deactivationIntegrateCounter + 1;
		motion->m_deactivationIntegrateCounter = static_cast<uint8_t>(counter);
		if ((counter & 3) == 0) {
			if (counter == 256) {
				motion->m_deactivationIntegrateCounter = 0xFF;
			} else {
				int32_t j = 0;
				if ((counter & 15) == 0) {
					motion->m_deactivationIntegrateCounter = 0;
					j = 1;
				}
				// The reference position's w keeps the largest squared velocity seen, the angular part weighted
				// by the object radius (at most 1) squared.
				float radius = ms.m_objectRadius;
				if (radius > 1.0f) {
					radius = 1.0f;
				}
				const hkQuadReal linSq = _mm_mul_ps(motion->m_linearVelocity.m_quad, motion->m_linearVelocity.m_quad);
				const float linVelSqrd = linSq[2] + static_cast<float>(linSq[1] + linSq[0]);
				const hkQuadReal angSq = _mm_mul_ps(motion->m_angularVelocity.m_quad, motion->m_angularVelocity.m_quad);
				const float angVelSqrd = angSq[2] + static_cast<float>(angSq[1] + angSq[0]);
				const float velSqrd = radius * radius * angVelSqrd + linVelSqrd;
				float maxVelSqrd = motion->m_deactivationRefPosition[j](3);
				if (!(velSqrd < maxVelSqrd)) {
					maxVelSqrd = velSqrd;
				}
				motion->m_deactivationRefPosition[j](3) = maxVelSqrd;

				// Moved too far from the reference position, or turned too far from the reference orientation:
				// the inactive count restarts and the references are taken anew. Else the count goes up.
				const hkpSolverInfo::DeactivationInfo& di = solverInfo.m_deactivationInfo[ms.m_deactivationClass];
				const hkQuadReal d = _mm_sub_ps(motion->m_deactivationRefPosition[j].m_quad, st.m_centerOfMass1.m_quad);
				const hkQuadReal dSq = _mm_mul_ps(d, d);
				const float distSqrd = dSq[2] + static_cast<float>(dSq[1] + dSq[0]);
				bool bReset = di.m_maxDistSqrd[j] < distSqrd;
				if (!bReset) {
					// EXE-INLINED none: hkVector4Util::unpackInt32IntoQuaternion
					// Each byte of the packed orientation, less 128, times 1.1 / 128 (the exe's double constant).
					const uint32_t packed = motion->m_deactivationRefOrientation[j];
					hkVector4 refRotation;
					refRotation(0) = (static_cast<int32_t>(packed & 0xFF) - 0x80) * 0.008593750186264515;
					refRotation(1) = (static_cast<int32_t>((packed >> 8) & 0xFF) - 0x80) * 0.008593750186264515;
					refRotation(2) = (static_cast<int32_t>((packed >> 16) & 0xFF) - 0x80) * 0.008593750186264515;
					refRotation(3) = (static_cast<int32_t>(packed >> 24) - 0x80) * 0.008593750186264515;
					// The squared 4-lane distance to the current rotation, summed as hkVector4::horizontalAdd4.
					const hkQuadReal r = _mm_sub_ps(refRotation.m_quad, st.m_rotation1.m_vec.m_quad);
					const hkQuadReal rSq = _mm_mul_ps(r, r);
					const hkQuadReal sum0 = _mm_add_ps(_mm_shuffle_ps(rSq, rSq, 0x4E), rSq);
					const hkQuadReal sum1 = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xB1), sum0);
					const float rotDistSqrd = sum1[0];
					// EXE-INLINED none: hkHalf::operator float (the top half of a float)
					const float maxRotSqrd = di.m_maxRotSqrd[j];
					bReset = maxRotSqrd < rotDistSqrd;
				}
				uint16_t& numFrames = motion->m_deactivationNumInactiveFrames[j];
				const int32_t frames = numFrames & 0x7F;
				const int32_t selectBits = ((solverInfo.m_deactivationNumInactiveFramesSelectFlag[j] << 7) | frames)
					<< 7;
				if (bReset) {
					numFrames = static_cast<uint16_t>(selectBits);
					motion->m_deactivationRefPosition[j].m_quad =
						_mm_and_ps(st.m_centerOfMass1.m_quad, hkVector4_zeroElement_cx[3]);
					motion->m_deactivationRefOrientation[j] =
						hkVector4Util::packQuaternionIntoInt32(st.m_rotation1.m_vec);
				} else {
					numFrames = static_cast<uint16_t>(selectBits | (frames - (frames >> 6) + 1));
				}
			}
		}

		int32_t frames = motion->m_deactivationNumInactiveFrames[1] & 0x7F;
		const int32_t frames0 = motion->m_deactivationNumInactiveFrames[0] & 0x7F;
		if (frames0 > frames) {
			frames = frames0;
		}
		if (numInactiveFrames >= frames) {
			numInactiveFrames = frames;
		}
	}
	return numInactiveFrames;
}

// For each motion: the accumulator's velocities (a box's angular velocity rotated to world space; a keyframed motion
// keeps its own), the step of the motion state with the accumulator's summed velocities (not for fixed and dynamic
// motions), the velocities clipped to their maxima, and the deactivation check every fourth step. A thin box keeps its
// angular momentum over the step. Returns the fewest frames any of the motions has been inactive (0x7FFFFFFF when none
// counts).
int32_t hkRigidMotionUtilApplyAccumulators(const hkpSolverInfo& solverInfo, const hkStepInfo& info,
	const hkpVelocityAccumulator* accumulators, hkpMotion* const* motions, int32_t numMotions, int32_t motionOffset) {
	int32_t numInactiveFrames = 0x7FFFFFFF;
	for (int32_t i = numMotions - 1; i >= 0; motions++, accumulators++, i--) {
		// Each element points motionOffset raw bytes before its motion (an entity's embedded motion), so the offset is
		// added as an untyped byte offset.
		// EXE-INLINED none: hkAddByteOffset<hkpMotion>
		hkpMotion* motion = reinterpret_cast<hkpMotion*>(reinterpret_cast<char*>(motions[0]) + motionOffset);
		hkMotionState& ms = motion->m_motionState;
		hkSweptTransform& st = ms.m_sweptTransform;

		// The velocities the step integrates: the accumulator's sums (scratch0 and the first scratch column).
		hkVector4 angularMomentum;
		hkVector4 sumLinVel;
		hkVector4 sumAngVel;
		bool bStep = true;
		switch (motion->getType()) {
		case hkpMotion::MOTION_INVALID:
		case hkpMotion::MOTION_THIN_BOX_INERTIA: {
			// The world angular momentum: the body-space angular velocity times the inertia (the refined
			// reciprocal of the inverse inertia), rotated to world space.
			hkVector4 inertia;
			inertia.setReciprocal4(motion->m_inertiaAndMassInv);
			hkVector4 localAngVel = accumulators->m_angularVel;
			localAngVel.m_quad = _mm_mul_ps(inertia.m_quad, localAngVel.m_quad);
			angularMomentum.setRotatedDir(ms.m_transform.m_rotation, localAngVel);
		}
			[[fallthrough]];
		case hkpMotion::MOTION_BOX_INERTIA: {
			motion->m_linearVelocity = accumulators->m_linearVel;
			// The accumulator's angular velocity is in body space: rotated as (y * col1 + x * col0) + z * col2.
			// EXE-INLINED 0xC7F700
			const hkQuadReal av = accumulators->m_angularVel.m_quad;
			const hkRotation& r = ms.m_transform.m_rotation;
			motion->m_angularVelocity.m_quad =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(av, av, 0x55), r.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(av, av, 0), r.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(av, av, 0xaa), r.m_col2.m_quad));
			sumLinVel = accumulators->m_scratch0;
			sumAngVel.setRotatedDir(ms.m_transform.m_rotation, accumulators->m_scratch123.m_col0);
			break;
		}
		case hkpMotion::MOTION_SPHERE_INERTIA:
		case hkpMotion::MOTION_CHARACTER:
			motion->m_linearVelocity = accumulators->m_linearVel;
			motion->m_angularVelocity = accumulators->m_angularVel;
			sumLinVel = accumulators->m_scratch0;
			sumAngVel = accumulators->m_scratch123.m_col0;
			break;
		case hkpMotion::MOTION_KEYFRAMED:
			sumLinVel = motion->m_linearVelocity;
			sumAngVel = motion->m_angularVelocity;
			break;
		default:
			bStep = false;
			break;
		}

		if (bStep) {
			// EXE-INLINED none: hkSweptTransformUtil::_stepMotionState
			// The SDK's validity test: x, y and z of both velocities ordered and of magnitude below 1e6. The exe
			// resets both to the 16 bytes at 0x1268370, hkTransformIdentity's first column.
			{
				const hkQuadReal lin = sumLinVel.m_quad;
				const hkQuadReal ang = sumAngVel.m_quad;
				const int32_t iOk = _mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(ang, hkVector4_setAbs4_absMask),
										hkSweptTransformUtil::_stepMotionStateMaxVelf)) &
					_mm_movemask_ps(_mm_cmplt_ps(_mm_and_ps(lin, hkVector4_setAbs4_absMask),
						hkSweptTransformUtil::_stepMotionStateMaxVelf)) &
					_mm_movemask_ps(_mm_cmpeq_ps(ang, ang)) & _mm_movemask_ps(_mm_cmpeq_ps(lin, lin));
				if ((iOk & 7) != 7) {
					sumLinVel = hkTransformIdentity.m_rotation.m_col0;
					sumAngVel = hkTransformIdentity.m_rotation.m_col0;
				}
			}

			st.m_centerOfMass0 = st.m_centerOfMass1;
			st.m_centerOfMass0(3) = info.m_startTime;

			// Clip the summed linear velocity: the squared length (z + (y + x)) against the squared maximum.
			{
				const float maxLinVel = hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
				const hkQuadReal sq = _mm_mul_ps(sumLinVel.m_quad, sumLinVel.m_quad);
				const float linVelSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sq, sq, 0xaa),
					_mm_add_ps(_mm_shuffle_ps(sq, sq, 0x55), sq)));
				if (linVelSqrd > maxLinVel * maxLinVel) {
					float fRoot = std::sqrt(linVelSqrd);
					float fInvRoot = 1.0f / fRoot;
					float f = fInvRoot * hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
					sumLinVel.m_quad = _mm_mul_ps(_mm_set1_ps(f), sumLinVel.m_quad);
				}
			}

			st.m_centerOfMass1.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_set1_ps(info.m_deltaTime), sumLinVel.m_quad), st.m_centerOfMass1.m_quad);
			st.m_centerOfMass1(3) = info.m_invDeltaTime;

			hkQuaternion rotation = st.m_rotation1;
			st.m_rotation0 = rotation;

			// The angular step dq = angVel * dt / 2; pre is its squared length times 4 / pi^2, the polynomial's
			// argument for dq's w.
			float fHalfDeltaTime = info.m_deltaTime * 0.5;
			hkQuaternion dq;
			dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(fHalfDeltaTime), sumAngVel.m_quad);
			const hkQuadReal dqSq = _mm_mul_ps(dq.m_vec.m_quad, dq.m_vec.m_quad);
			const float dqLenSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(dqSq, dqSq, 0xaa),
				_mm_add_ps(_mm_shuffle_ps(dqSq, dqSq, 0x55), dqSq)));
			float pre = dqLenSqrd * 0.40528471201320165;
			float maxAngle = hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value] * info.m_deltaTime;
			if (maxAngle > 0.8999999761581421) {
				maxAngle = 0.9f;
			}
			float maxAngleSqrd = maxAngle * maxAngle;
			// Taken also when pre is a NaN.
			if (!(maxAngleSqrd >= pre)) {
				float fRoot = std::sqrt(pre);
				float fInvRoot = 1.0f / fRoot;
				float f = fInvRoot * maxAngle;
				pre = maxAngleSqrd;
				sumAngVel.m_quad = _mm_mul_ps(_mm_set1_ps(f), sumAngVel.m_quad);
				dq.m_vec.m_quad = _mm_mul_ps(_mm_set1_ps(f), dq.m_vec.m_quad);
			}
			const double dPre = pre;
			float pre2 = dPre * dPre;
			dq.m_vec(3) =
				1.0 - 0.8229479789733887 * dPre - 0.1305290013551712 * pre2 - pre2 * (dPre * 0.04440800100564957);

			rotation.setMul(dq, rotation);
			rotation.normalize();
			ms.m_deltaAngle.m_quad = _mm_add_ps(dq.m_vec.m_quad, dq.m_vec.m_quad);
			float fAngle = std::sqrt(pre);
			ms.m_deltaAngle(3) = fAngle * 3.1415927410125732;
			st.m_rotation1 = rotation;
			ms.m_transform.m_rotation.set(st.m_rotation1);

			// The translation is the centre of mass less the rotated local centre of mass, rotated as
			// (y * col1 + x * col0) + z * col2.
			// EXE-INLINED 0xC7F700
			{
				const hkQuadReal local = st.m_centerOfMassLocal.m_quad;
				const hkRotation& r = ms.m_transform.m_rotation;
				const hkQuadReal shift =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local, local, 0x55), r.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(local, local, 0), r.m_col0.m_quad)),
						_mm_mul_ps(_mm_shuffle_ps(local, local, 0xaa), r.m_col2.m_quad));
				ms.m_transform.m_translation.m_quad = _mm_sub_ps(st.m_centerOfMass1.m_quad, shift);
			}
		}

		if (motion->getType() == hkpMotion::MOTION_THIN_BOX_INERTIA) {
			// The angular velocity again from the kept momentum, under the new rotation.
			motion->m_linearVelocity = accumulators->m_linearVel;
			hkVector4 localMomentum;
			localMomentum.setRotatedInverseDir(ms.m_transform.m_rotation, angularMomentum);
			localMomentum.m_quad = _mm_mul_ps(localMomentum.m_quad, motion->m_inertiaAndMassInv.m_quad);
			motion->m_angularVelocity.setRotatedDir(ms.m_transform.m_rotation, localMomentum);
		}

		// EXE-INLINED none: hkSweptTransformUtil::_clipVelocities
		// Each velocity above its maximum, or a NaN, is scaled to the maximum; a NaN one is then reset to
		// hkTransformIdentity's first column. The squared maxima are rounded to float.
		{
			const hkVector4 linVel = motion->m_linearVelocity;
			float maxAngular = hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value] *
				hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value];
			const hkQuadReal linSq = _mm_mul_ps(linVel.m_quad, linVel.m_quad);
			const float linVelSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(linSq, linSq, 0xaa),
				_mm_add_ps(_mm_shuffle_ps(linSq, linSq, 0x55), linSq)));
			const hkQuadReal angSq = _mm_mul_ps(motion->m_angularVelocity.m_quad, motion->m_angularVelocity.m_quad);
			const float angVelSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(angSq, angSq, 0xaa),
				_mm_add_ps(_mm_shuffle_ps(angSq, angSq, 0x55), angSq)));
			float maxLinear = hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value] *
				hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
			if (maxLinear < linVelSqrd || linVelSqrd != linVelSqrd) {
				float fRoot = std::sqrt(linVelSqrd);
				float fInvRoot = 1.0f / fRoot;
				float f = fInvRoot * hkUFloat8_intToReal[ms.m_maxLinearVelocity.m_value];
				motion->m_linearVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), linVel.m_quad);
				if (linVelSqrd != linVelSqrd) {
					motion->m_linearVelocity = hkTransformIdentity.m_rotation.m_col0;
				}
			}
			if (maxAngular < angVelSqrd || angVelSqrd != angVelSqrd) {
				float fRoot = std::sqrt(angVelSqrd);
				float fInvRoot = 1.0f / fRoot;
				float f = fInvRoot * hkUFloat8_intToReal[ms.m_maxAngularVelocity.m_value];
				motion->m_angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(f), motion->m_angularVelocity.m_quad);
				if (angVelSqrd != angVelSqrd) {
					motion->m_angularVelocity = hkTransformIdentity.m_rotation.m_col0;
				}
			}
		}

		// The deactivation check, every fourth step: the counter's 16th step checks the slow reference (index 1)
		// and restarts the counter, the other checks the fast one (index 0); a counter that wraps stays at 0xFF.
		const int32_t counter = motion->m_deactivationIntegrateCounter + 1;
		motion->m_deactivationIntegrateCounter = static_cast<uint8_t>(counter);
		bool bCheck = false;
		int32_t j = 0;
		if ((counter & 3) == 0) {
			if ((counter & 15) != 0) {
				bCheck = true;
			} else if (counter == 256) {
				motion->m_deactivationIntegrateCounter = 0xFF;
			} else {
				motion->m_deactivationIntegrateCounter = 0;
				j = 1;
				bCheck = true;
			}
		}
		if (bCheck) {
			// The reference position's w keeps the largest squared velocity seen, the angular part weighted by the
			// object radius (at most 1) squared.
			float radius = ms.m_objectRadius;
			if (radius > 1.0) {
				radius = 1.0f;
			}
			const double dRadius = radius;
			const hkQuadReal linSq = _mm_mul_ps(motion->m_linearVelocity.m_quad, motion->m_linearVelocity.m_quad);
			const float linVelSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(linSq, linSq, 0xaa),
				_mm_add_ps(_mm_shuffle_ps(linSq, linSq, 0x55), linSq)));
			const hkQuadReal angSq = _mm_mul_ps(motion->m_angularVelocity.m_quad, motion->m_angularVelocity.m_quad);
			const float angVelSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(angSq, angSq, 0xaa),
				_mm_add_ps(_mm_shuffle_ps(angSq, angSq, 0x55), angSq)));
			float velSqrd = dRadius * dRadius * angVelSqrd + linVelSqrd;
			float maxVelSqrd = motion->m_deactivationRefPosition[j](3);
			const float fVel = velSqrd;
			const float fMax = maxVelSqrd;
			float fRefVel = fVel < fMax ? fMax : fVel;
			motion->m_deactivationRefPosition[j](3) = fRefVel;

			// Moved too far from the reference position, or turned too far from the reference orientation: the
			// inactive count restarts and the references are taken anew. Else the count goes up.
			const hkpSolverInfo::DeactivationInfo& di = solverInfo.m_deactivationInfo[ms.m_deactivationClass];
			const hkQuadReal d = _mm_sub_ps(motion->m_deactivationRefPosition[j].m_quad, st.m_centerOfMass1.m_quad);
			const hkQuadReal dSq = _mm_mul_ps(d, d);
			const float distSqrd = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(dSq, dSq, 0xaa),
				_mm_add_ps(_mm_shuffle_ps(dSq, dSq, 0x55), dSq)));
			bool bReset = di.m_maxDistSqrd[j] < distSqrd;
			if (!bReset) {
				// EXE-INLINED none: hkVector4Util::unpackInt32IntoQuaternion
				// Each byte of the packed orientation, less 128, times 1.1 / 128 (the exe's double constant).
				const uint32_t packed = motion->m_deactivationRefOrientation[j];
				hkVector4 refRotation;
				refRotation(0) = (static_cast<int32_t>(packed & 0xFF) - 0x80) * 0.008593750186264515;
				refRotation(1) = (static_cast<int32_t>((packed >> 8) & 0xFF) - 0x80) * 0.008593750186264515;
				refRotation(2) = (static_cast<int32_t>((packed >> 16) & 0xFF) - 0x80) * 0.008593750186264515;
				refRotation(3) = (static_cast<int32_t>(packed >> 24) - 0x80) * 0.008593750186264515;
				// The squared 4-lane distance to the current rotation, summed as hkVector4::horizontalAdd4.
				const hkQuadReal r = _mm_sub_ps(refRotation.m_quad, st.m_rotation1.m_vec.m_quad);
				const hkQuadReal rSq = _mm_mul_ps(r, r);
				const hkQuadReal sum0 = _mm_add_ps(_mm_shuffle_ps(rSq, rSq, 0x4E), rSq);
				const hkQuadReal sum1 = _mm_add_ps(_mm_shuffle_ps(sum0, sum0, 0xB1), sum0);
				const float rotDistSqrd = _mm_cvtss_f32(sum1);
				// EXE-INLINED none: hkHalf::operator float (the top half of a float)
				const float maxRotSqrd = di.m_maxRotSqrd[j];
				bReset = maxRotSqrd < rotDistSqrd;
			}
			uint16_t& numFrames = motion->m_deactivationNumInactiveFrames[j];
			const int32_t frames = numFrames & 0x7F;
			const int32_t selectBits = ((solverInfo.m_deactivationNumInactiveFramesSelectFlag[j] << 7) | frames) << 7;
			if (bReset) {
				numFrames = static_cast<uint16_t>(selectBits);
				motion->m_deactivationRefPosition[j].m_quad =
					_mm_and_ps(st.m_centerOfMass1.m_quad, hkVector4_zeroElement_cx[3]);
				motion->m_deactivationRefOrientation[j] = hkVector4Util::packQuaternionIntoInt32(st.m_rotation1.m_vec);
			} else {
				numFrames = static_cast<uint16_t>(selectBits | (frames - (frames >> 6) + 1));
			}
		}

		int32_t frames = motion->m_deactivationNumInactiveFrames[1] & 0x7F;
		const int32_t frames0 = motion->m_deactivationNumInactiveFrames[0] & 0x7F;
		if (frames0 > frames) {
			frames = frames0;
		}
		if (numInactiveFrames >= frames) {
			numInactiveFrames = frames;
		}
	}
	return numInactiveFrames;
}
