#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian2dFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobian3dFrictionSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSimpleContactConstraintDataInfoTypeInfo = {"hkpSimpleContactConstraintDataInfo",
	finishLoadedObjecthkpSimpleContactConstraintDataInfo, cleanupLoadedObjecthkpSimpleContactConstraintDataInfo,
	nullptr, &typeid(hkpSimpleContactConstraintDataInfo), sizeof(hkpSimpleContactConstraintDataInfo)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpSimpleContactConstraintDataInfo, as the exe's .rdata holds them.
using hkpSimpleContactConstraintDataInfoClass_MembersArray = const hkClassMember[3];
static hkpSimpleContactConstraintDataInfoClass_MembersArray hkpSimpleContactConstraintDataInfoClass_Members{
	{"flags", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, {hkClassMember::ALIGN_16}, 0x0,
		nullptr},
	{"index", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"data", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 7, 0, 0x4, nullptr}};
hkClass hkpSimpleContactConstraintDataInfoClass("hkpSimpleContactConstraintDataInfo", nullptr, 32, nullptr, 0, nullptr,
	0, hkpSimpleContactConstraintDataInfoClass_Members, 3, nullptr, nullptr, 0, 0);

const hkClass& hkpSimpleContactConstraintDataInfo::staticClass() {
	return hkpSimpleContactConstraintDataInfoClass;
}

// The exe's code, read from its asm and Hex-Rays (the SDK ships no source for it). The SSE steps are written as the
// exe's instructions (the horizontal sums are the exe's addss or addps of shuffles, the normalisations rsqrtss refined
// by one Newton step); every value the x87 code stores as a float is a float local, and its constants keep their size
// (the qword HK_REAL_EPSILON 1.1920928955078125e-07, the dword -1.1920929e-07f). Each schema starts by adding
// HK_REAL_EPSILON to the float its first lane holds and storing the type byte over it, as the exe does. The contact
// point loop runs once even for an atom without points: the exe tests the count after the first point. The SDK
// views the atom's info as hkpSimpleContactConstraintDataInfoInternal (the same 0x20 bytes) and writes each friction
// schema through its own class over the schema stream, untyped 16-byte units (the PDB's hkpJacobianSchema has no
// data members): the EXE-CAST lines below.
void hkSimpleContactConstraintDataBuildJacobian(hkpSimpleContactConstraintAtom* atom, const hkpConstraintQueryIn& in,
	hkBool writeHeaderSchemas, hkpConstraintQueryOut& out) {
	float fFrictionSum = 0.0f;
	float fImpulseSum = 0.0f;
	const int32_t iNumContactPoints = atom->m_numContactPoints;
	hkpContactPointProperties* pFirstProperties = atom->getContactPointPropertiesStream(0)->asProperties();
	hkpJacobianSchema* pSchema = out.m_jacobianSchemas;
	hkpVelocityAccumulator* pBodyB = in.m_bodyB;
	const int32_t iStriding = atom->getContactPointPropertiesStriding();
	hkpVelocityAccumulator* pBodyA = in.m_bodyA;
	__m128 kPositionSum = _mm_setzero_ps();
	__m128 kNormalSum = _mm_setzero_ps();
	const float fInvNumPoints = 1.0 / static_cast<double>(iNumContactPoints);
	if (writeHeaderSchemas.m_bool) {
		hkpJacobianHeaderSchema* pHeader = &pSchema->getHeader();
		pHeader->m_bodyAIndex = static_cast<uint16_t>(in.m_accumulatorAIndex.m_storage);
		pSchema->m_quad[0] = pSchema->m_quad[0] + 1.1920928955078125e-07;
		pHeader->m_bodyBIndex = static_cast<uint16_t>(in.m_accumulatorBIndex.m_storage);
		pHeader->m_constraintInstance = in.m_constraintInstance;
		pSchema->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_HEADER;
		pHeader->m_solverResultInMainMemory = pFirstProperties;
		pHeader->m_solverResultStriding = static_cast<uint8_t>(iStriding);
		pSchema += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA / sizeof(hkpJacobianSchema);
	}

	// The bound calculateRhs clamps m_internalDataA to.
	const float fMaxInternalDataA = -1.1920929e-07f;
	const hkContactPoint* pPoints = atom->getContactPoints();
	int32_t iPoint = 0;
	do {
		hkpContactPointProperties* pProperties = atom->getContactPointPropertiesStream(iPoint)->asProperties();
		const int32_t iUsesPath2 = pProperties->m_flags & hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2;
		const __m128 kPosition = pPoints[iPoint].m_position.m_quad;
		const __m128 kNormal = pPoints[iPoint].m_separatingNormal.m_quad;
		const __m128 kArmA = _mm_sub_ps(kPosition, pBodyA->m_scratch0.m_quad);
		const __m128 kArmB = _mm_sub_ps(kPosition, pBodyB->m_scratch0.m_quad);
		const __m128 kNormalYZX = _mm_shuffle_ps(kNormal, kNormal, 0xC9);
		const __m128 kNormalZXY = _mm_shuffle_ps(kNormal, kNormal, 0xD2);
		const __m128 kCrossA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 0xC9), kNormalZXY),
			_mm_mul_ps(_mm_shuffle_ps(kArmA, kArmA, 0xD2), kNormalYZX));
		const __m128 kCrossB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xD2), kNormalYZX),
			_mm_mul_ps(_mm_shuffle_ps(kArmB, kArmB, 0xC9), kNormalZXY));
		const __m128 kAngA =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0x55),
									  pBodyA->m_scratch123.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0x00), pBodyA->m_scratch123.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kCrossA, kCrossA, 0xAA), pBodyA->m_scratch123.m_col2.m_quad));
		const __m128 kInvMassB = pBodyB->m_invMasses.m_quad;
		const __m128 kAngB =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0x55),
									  pBodyB->m_scratch123.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0x00), pBodyB->m_scratch123.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(kCrossB, kCrossB, 0xAA), pBodyB->m_scratch123.m_col2.m_quad));
		const __m128 kInvMassA = pBodyA->m_invMasses.m_quad;
		pSchema[2].m_quad = kAngB;
		const __m128 kMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kAngB, kAngB), kInvMassB),
			_mm_mul_ps(_mm_mul_ps(kAngA, kAngA), kInvMassA));
		const __m128 kMassW = _mm_shuffle_ps(kMass, _mm_unpackhi_ps(kMass, _mm_add_ps(kInvMassA, kInvMassB)), 0xC4);
		const __m128 kMassHalves = _mm_add_ps(_mm_shuffle_ps(kMassW, kMassW, 0x4E), kMassW);
		pSchema[0].m_quad = kNormal;
		pSchema[1].m_quad = kAngA;
		const float fDiag = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(kMassHalves, kMassHalves, 0xB1), kMassHalves)) +
			1.1920928955078125e-07;
		pSchema[2].m_quad[3] = fDiag;
		pSchema[1].m_quad[3] = in.m_virtMassFactor / fDiag;
		fFrictionSum = static_cast<float>(pProperties->m_friction) + fFrictionSum;
		fImpulseSum = pProperties->m_impulseApplied + fImpulseSum;

		// EXE-INLINED 0xD720A0
		const float fInternalDataA = pProperties->m_internalDataA;
		float fRhs = pPoints[iPoint].m_separatingNormal.m_quad[3] - fInternalDataA;
		const float fSolverRhs = -pProperties->m_internalSolverData - fRhs;
		const float fScaledDeltaTime = in.m_frameDeltaTime * HK_CONTACT_LINEAR_ERROR_RECOVERY_VELOCITY;
		const float fLimit = -HK_CONTACT_EXPONENTIAL_ERROR_RECOVERY_VELOCITY * fInternalDataA;
		const float fStep = fScaledDeltaTime > fLimit ? fLimit : fScaledDeltaTime;
		float fNewInternalDataA = fInternalDataA + fStep;
		fRhs = fRhs - fStep;
		if (fScaledDeltaTime + (fStep + fStep) < fSolverRhs) {
			fNewInternalDataA = fNewInternalDataA - fSolverRhs;
			fRhs = fSolverRhs + fRhs;
		}
		if (!(fNewInternalDataA < fMaxInternalDataA)) {
			fNewInternalDataA = fMaxInternalDataA;
		}
		pProperties->m_internalDataA = fNewInternalDataA;

		kPositionSum = _mm_add_ps(kPositionSum, kPosition);
		kNormalSum = _mm_add_ps(kNormalSum, kNormal);
		pSchema[0].m_quad[3] = -fRhs * in.m_rhsFactor;
		if (iUsesPath2) {
			// This point and the previous single contact become one pair contact schema.
			const float fPrevDiag = pSchema[-1].m_quad[3];
			const float fThisDiag = pSchema[2].m_quad[3];
			const __m128 kNormals = _mm_mul_ps(pSchema[-3].m_quad, pSchema[0].m_quad);
			const __m128 kCoupling = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(pSchema[2].m_quad, pSchema[-1].m_quad),
															   pBodyB->m_invMasses.m_quad),
													_mm_mul_ps(_mm_mul_ps(pSchema[1].m_quad, pSchema[-2].m_quad),
														pBodyA->m_invMasses.m_quad)),
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(pBodyA->m_invMasses.m_quad, pBodyA->m_invMasses.m_quad, 0xFF),
							   kNormals),
					_mm_mul_ps(_mm_shuffle_ps(pBodyB->m_invMasses.m_quad, pBodyB->m_invMasses.m_quad, 0xFF),
						kNormals)));
			pSchema += 4;
			const float fCoupling = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCoupling, kCoupling, 0xAA),
										_mm_add_ss(_mm_shuffle_ps(kCoupling, kCoupling, 0x55), kCoupling))) *
				0.9900000095367432;
			const float fInvDet = in.m_virtMassFactor / (fThisDiag * fPrevDiag - fCoupling * fCoupling);
			pSchema[-5].m_quad[3] = fThisDiag * fInvDet;
			pSchema[-2].m_quad[3] = fPrevDiag * fInvDet;
			// The pair schema's m_invJac01.
			pSchema[-1].m_quad[0] = fInvDet * -fCoupling;
			pSchema[-7].m_quad[0] = pSchema[-7].m_quad[0] + 1.1920928955078125e-07;
			pSchema[-7].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_PAIR_CONTACT;
		} else {
			hkpJacobianSchema* pSingle = pSchema;
			pSchema += 3;
			pSingle->m_quad[0] = pSingle->m_quad[0] + 1.1920928955078125e-07;
			pSingle->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SINGLE_CONTACT;
			if (pProperties->m_maxImpulse.m_value) {
				pSingle->m_quad[0] = pSingle->m_quad[0] + 1.1920928955078125e-07;
				pSingle->m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_SINGLE_LIMIT_CONTACT;
				pSingle[2].m_quad[3] = pProperties->m_maxImpulse;
			}
		}
		++iPoint;
	} while (iPoint < iNumContactPoints);

	float fFrictionImpulse = fFrictionSum * fImpulseSum * fInvNumPoints;
	if (!(fFrictionImpulse > 0.0f)) {
		out.m_jacobianSchemas = pSchema;
		return;
	}

	// The friction normal: the first point's normal where the points agree, else their normalised sum, else the
	// first point's normal, else (when that is short too) the y axis with no friction.
	const float fInvNumPoints2 = fInvNumPoints * fInvNumPoints;
	const __m128 kNormalSum2 = _mm_mul_ps(kNormalSum, kNormalSum);
	const float fNormalSumLen2 = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kNormalSum2, kNormalSum2, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kNormalSum2, kNormalSum2, 0x55), kNormalSum2)));
	__m128 kNormal;
	if (fInvNumPoints2 * fNormalSumLen2 > 0.9998999834060669) {
		kNormal = pPoints[0].m_separatingNormal.m_quad;
	} else if (fNormalSumLen2 > 0.10000000149011612) {
		const __m128 kLen2 = _mm_set_ss(fNormalSumLen2);
		const __m128 kApprox = _mm_rsqrt_ss(kLen2);
		const __m128 kLen2IsZero = _mm_cmpeq_ss(kLen2, _mm_set_ss(0.0f));
		const __m128 kRefined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kApprox),
			_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kApprox), kApprox)));
		const __m128 kInvLen = _mm_andnot_ps(kLen2IsZero, kRefined);
		kNormal = _mm_mul_ps(_mm_shuffle_ps(kInvLen, kInvLen, 0x00), kNormalSum);
	} else {
		kNormal = pPoints[0].m_separatingNormal.m_quad;
		const __m128 kNormal2 = _mm_mul_ps(kNormal, kNormal);
		const float fNormalLen2 = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kNormal2, kNormal2, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kNormal2, kNormal2, 0x55), kNormal2)));
		if (fNormalLen2 < 0.8999999761581421) {
			fFrictionImpulse = 0.0f;
			kNormal = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
		}
	}

	hkpSimpleContactConstraintDataInfoInternal& kInfo =
		reinterpret_cast<hkpSimpleContactConstraintDataInfoInternal&>(atom->m_info);
	const __m128 kNormalYZX = _mm_shuffle_ps(kNormal, kNormal, 0xC9);
	const __m128 kNormalZXY = _mm_shuffle_ps(kNormal, kNormal, 0xD2);
	// The binormal: the cross of the kept axis with the normal, or of the axis the normal is least along when that is
	// short (whose friction results then restart).
	const __m128 kAxis = hkTransformIdentity.m_rotation.getColumn(kInfo.m_biNormalAxis).m_quad;
	__m128 kBinormal = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kAxis, kAxis, 0xC9), kNormalZXY),
		_mm_mul_ps(_mm_shuffle_ps(kAxis, kAxis, 0xD2), kNormalYZX));
	const __m128 kBinormal2 = _mm_mul_ps(kBinormal, kBinormal);
	const float fBinormalLen2 = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kBinormal2, kBinormal2, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kBinormal2, kBinormal2, 0x55), kBinormal2)));
	if (!(fBinormalLen2 > 0.10000000149011612)) {
		const __m128 kAbsNormal = _mm_and_ps(kNormal, hkVector4_setAbs4_absMask);
		uint16_t uiAxis;
		if (kAbsNormal[1] > kAbsNormal[0]) {
			uiAxis = kAbsNormal[2] > kAbsNormal[0] ? 0 : 2;
		} else {
			uiAxis = kAbsNormal[2] > kAbsNormal[1] ? 1 : 2;
		}
		kInfo.m_biNormalAxis = uiAxis;
		kInfo.m_frictionResults[0].m_internalSolverData = 0.0f;
		kInfo.m_frictionResults[1].m_internalSolverData = 0.0f;
		const __m128 kNewAxis = hkTransformIdentity.m_rotation.getColumn(uiAxis).m_quad;
		kBinormal = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kNewAxis, kNewAxis, 0xC9), kNormalZXY),
			_mm_mul_ps(_mm_shuffle_ps(kNewAxis, kNewAxis, 0xD2), kNormalYZX));
	}
	const __m128 kBinormalSq = _mm_mul_ps(kBinormal, kBinormal);
	const __m128 kBinormalLenSq = _mm_add_ss(_mm_shuffle_ps(kBinormalSq, kBinormalSq, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kBinormalSq, kBinormalSq, 0x55), kBinormalSq));
	const __m128 kBinormalApprox = _mm_rsqrt_ss(kBinormalLenSq);
	const __m128 kBinormalIsZero = _mm_cmpeq_ss(kBinormalLenSq, _mm_set_ss(0.0f));
	const __m128 kBinormalRefined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kBinormalApprox),
		_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kBinormalLenSq, kBinormalApprox), kBinormalApprox)));
	const __m128 kBinormalInvLen = _mm_andnot_ps(kBinormalIsZero, kBinormalRefined);
	const __m128 kUnitBinormal = _mm_mul_ps(_mm_shuffle_ps(kBinormalInvLen, kBinormalInvLen, 0x00), kBinormal);
	const __m128 kBinormalYZX = _mm_shuffle_ps(kUnitBinormal, kUnitBinormal, 0xC9);
	const __m128 kBinormalZXY = _mm_shuffle_ps(kUnitBinormal, kUnitBinormal, 0xD2);
	// The tangent: the cross of the normal with the binormal.
	const __m128 kTangent = _mm_sub_ps(_mm_mul_ps(kBinormalZXY, kNormalYZX), _mm_mul_ps(kBinormalYZX, kNormalZXY));

	// The 2d friction schema: a row along the tangent and one along the binormal, at the points' centre.
	pSchema[0].m_quad = kTangent;
	const __m128 kTangentZXY = _mm_shuffle_ps(kTangent, kTangent, 0xD2);
	const __m128 kTangentYZX = _mm_shuffle_ps(kTangent, kTangent, 0xC9);
	const __m128 kCenter =
		_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fInvNumPoints), _mm_set_ss(fInvNumPoints), 0x00), kPositionSum);
	const __m128 kCenterArmA = _mm_sub_ps(kCenter, pBodyA->m_scratch0.m_quad);
	const __m128 kCenterArmB = _mm_sub_ps(kCenter, pBodyB->m_scratch0.m_quad);
	const __m128 kTangentCrossA = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kCenterArmA, kCenterArmA, 0xC9), kTangentZXY),
		_mm_mul_ps(_mm_shuffle_ps(kCenterArmA, kCenterArmA, 0xD2), kTangentYZX));
	const __m128 kTangentCrossB = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kCenterArmB, kCenterArmB, 0xD2), kTangentYZX),
		_mm_mul_ps(_mm_shuffle_ps(kCenterArmB, kCenterArmB, 0xC9), kTangentZXY));
	pSchema[1].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kTangentCrossA, kTangentCrossA, 0x55),
												  pBodyA->m_scratch123.m_col1.m_quad),
									   _mm_mul_ps(_mm_shuffle_ps(kTangentCrossA, kTangentCrossA, 0x00),
										   pBodyA->m_scratch123.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kTangentCrossA, kTangentCrossA, 0xAA), pBodyA->m_scratch123.m_col2.m_quad));
	const __m128 kTangentAngB = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kTangentCrossB, kTangentCrossB, 0x55),
														  pBodyB->m_scratch123.m_col1.m_quad),
											   _mm_mul_ps(_mm_shuffle_ps(kTangentCrossB, kTangentCrossB, 0x00),
												   pBodyB->m_scratch123.m_col0.m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(kTangentCrossB, kTangentCrossB, 0xAA), pBodyB->m_scratch123.m_col2.m_quad));
	const __m128 kTangentAngA = pSchema[1].m_quad;
	pSchema[2].m_quad = kTangentAngB;
	const __m128 kTangentInvMassB = pBodyB->m_invMasses.m_quad;
	const __m128 kTangentInvMassA = pBodyA->m_invMasses.m_quad;
	const __m128 kTangentMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kTangentAngA, kTangentAngA), kTangentInvMassA),
		_mm_mul_ps(_mm_mul_ps(kTangentAngB, kTangentAngB), kTangentInvMassB));
	const __m128 kTangentMassW = _mm_shuffle_ps(kTangentMass,
		_mm_unpackhi_ps(kTangentMass, _mm_add_ps(kTangentInvMassA, kTangentInvMassB)), 0xC4);
	const __m128 kTangentMassHalves = _mm_add_ps(_mm_shuffle_ps(kTangentMassW, kTangentMassW, 0x4E), kTangentMassW);
	const float fTangentDiag =
		_mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(kTangentMassHalves, kTangentMassHalves, 0xB1), kTangentMassHalves)) +
		1.1920928955078125e-07;
	pSchema[2].m_quad[3] = fTangentDiag;
	pSchema[1].m_quad[3] = in.m_virtMassFactor / fTangentDiag;
	pSchema[0].m_quad[3] = kInfo.m_frictionResults[0].m_internalSolverData * in.m_frictionRhsFactor;

	pSchema[3].m_quad = kUnitBinormal;
	const __m128 kBinormalArmA = _mm_sub_ps(kCenter, pBodyA->m_scratch0.m_quad);
	const __m128 kBinormalArmB = _mm_sub_ps(kCenter, pBodyB->m_scratch0.m_quad);
	const __m128 kBinormalCrossA =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kBinormalArmA, kBinormalArmA, 0xC9), kBinormalZXY),
			_mm_mul_ps(_mm_shuffle_ps(kBinormalArmA, kBinormalArmA, 0xD2), kBinormalYZX));
	const __m128 kBinormalCrossB =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(kBinormalArmB, kBinormalArmB, 0xD2), kBinormalYZX),
			_mm_mul_ps(_mm_shuffle_ps(kBinormalArmB, kBinormalArmB, 0xC9), kBinormalZXY));
	const hkRotation& kRotA = pBodyA->m_scratch123;
	const hkRotation& kRotB = pBodyB->m_scratch123;
	pSchema[4].m_quad =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kBinormalCrossA, kBinormalCrossA, 0x55), kRotA.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(kBinormalCrossA, kBinormalCrossA, 0x00), kRotA.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(kBinormalCrossA, kBinormalCrossA, 0xAA), kRotA.m_col2.m_quad));
	const __m128 kBinormalAngB =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kBinormalCrossB, kBinormalCrossB, 0x55), kRotB.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(kBinormalCrossB, kBinormalCrossB, 0x00), kRotB.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(kBinormalCrossB, kBinormalCrossB, 0xAA), kRotB.m_col2.m_quad));
	const __m128 kBinormalAngA = pSchema[4].m_quad;
	pSchema[5].m_quad = kBinormalAngB;
	const __m128 kBinormalInvMassB = pBodyB->m_invMasses.m_quad;
	const __m128 kBinormalInvMassA = pBodyA->m_invMasses.m_quad;
	const __m128 kBinormalMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kBinormalAngA, kBinormalAngA), kBinormalInvMassA),
		_mm_mul_ps(_mm_mul_ps(kBinormalAngB, kBinormalAngB), kBinormalInvMassB));
	const __m128 kBinormalMassW = _mm_shuffle_ps(kBinormalMass,
		_mm_unpackhi_ps(kBinormalMass, _mm_add_ps(kBinormalInvMassA, kBinormalInvMassB)), 0xC4);
	const __m128 kBinormalMassHalves = _mm_add_ps(_mm_shuffle_ps(kBinormalMassW, kBinormalMassW, 0x4E), kBinormalMassW);
	const float fBinormalDiag =
		_mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(kBinormalMassHalves, kBinormalMassHalves, 0xB1), kBinormalMassHalves)) +
		1.1920928955078125e-07;
	pSchema[5].m_quad[3] = fBinormalDiag;
	pSchema[4].m_quad[3] = in.m_virtMassFactor / fBinormalDiag;
	pSchema[3].m_quad[3] = kInfo.m_frictionResults[1].m_internalSolverData * in.m_frictionRhsFactor;

	// The two rows' coupling.
	const float fTangentRowDiag = pSchema[2].m_quad[3];
	const __m128 kDirections = _mm_mul_ps(pSchema[3].m_quad, pSchema[0].m_quad);
	const __m128 kCoupling =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(pSchema[4].m_quad, pSchema[1].m_quad), pBodyA->m_invMasses.m_quad),
					   _mm_mul_ps(_mm_mul_ps(pSchema[5].m_quad, pSchema[2].m_quad), pBodyB->m_invMasses.m_quad)),
			_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(pBodyA->m_invMasses.m_quad, pBodyA->m_invMasses.m_quad, 0xFF),
						   kDirections),
				_mm_mul_ps(_mm_shuffle_ps(pBodyB->m_invMasses.m_quad, pBodyB->m_invMasses.m_quad, 0xFF), kDirections)));
	const float fCoupling = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kCoupling, kCoupling, 0xAA),
		_mm_add_ss(_mm_shuffle_ps(kCoupling, kCoupling, 0x55), kCoupling)));
	const float fBinormalRowDiag = pSchema[5].m_quad[3];
	const float fInvDet = in.m_virtMassFactor / (fBinormalRowDiag * fTangentRowDiag - fCoupling * fCoupling);
	pSchema[2].m_quad[3] = fBinormalRowDiag * fInvDet;
	pSchema[5].m_quad[3] = fTangentRowDiag * fInvDet;
	const float fInvJac01 = fInvDet * -fCoupling;
	const float fMaxImpulse = in.m_invNumStepsTimesMicroSteps * fFrictionImpulse;
	hkpJacobian2dFrictionSchema* pFriction2d = reinterpret_cast<hkpJacobian2dFrictionSchema*>(pSchema);
	pFriction2d->m_invJac01 = fInvJac01;
	pFriction2d->m_solverResults = &kInfo.m_frictionResults[0];
	pFriction2d->m_maxImpulse = fMaxImpulse;
	pSchema[0].m_quad[0] = pSchema[0].m_quad[0] + 1.1920928955078125e-07;
	pSchema[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_2D_FRICTION;
	if (iNumContactPoints <= 1) {
		out.m_jacobianSchemas = pSchema + HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA / sizeof(hkpJacobianSchema);
		return;
	}

	// A changed contact area: the contact radius is the points' mean distance from their centre; a radius too small
	// keeps the 2d friction schema.
	if (kInfo.m_flags & hkpSimpleContactConstraintDataInfoInternal::HK_FLAG_AREA_CHANGED) {
		do {
			kInfo.m_contactRadius = 0.0f;
			for (int32_t iRadiusPoint = 0; iRadiusPoint < iNumContactPoints; ++iRadiusPoint) {
				const __m128 kOffset = _mm_sub_ps(pPoints[iRadiusPoint].m_position.m_quad, kCenter);
				const __m128 kOffset2 = _mm_mul_ps(kOffset, kOffset);
				const float fDistance = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(kOffset2, kOffset2, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(kOffset2, kOffset2, 0x55), kOffset2))));
				kInfo.m_contactRadius = kInfo.m_contactRadius + fDistance;
			}
			const float fRadius = kInfo.m_contactRadius * fInvNumPoints;
			kInfo.m_contactRadius = fRadius;
			if (fRadius < 1e-06f) {
				out.m_jacobianSchemas = pSchema + HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA / sizeof(hkpJacobianSchema);
				return;
			}
			kInfo.m_flags &= ~hkpSimpleContactConstraintDataInfoInternal::HK_FLAG_AREA_CHANGED;
		} while (kInfo.m_flags & hkpSimpleContactConstraintDataInfoInternal::HK_FLAG_AREA_CHANGED);
	}

	// The 3d friction schema: the 2d one and an angular row about the normal, scaled by the contact radius.
	const float fInvRadius = 1.0 / kInfo.m_contactRadius;
	const __m128 kNegNormal = _mm_xor_ps(kNormal, hkQuadRealNegateMask);
	pSchema[6].m_quad =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x55), pBodyA->m_scratch123.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0x00), pBodyA->m_scratch123.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(kNormal, kNormal, 0xAA), pBodyA->m_scratch123.m_col2.m_quad));
	const __m128 kTwistAngB =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kNegNormal, kNegNormal, 0x55),
								  pBodyB->m_scratch123.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(kNegNormal, kNegNormal, 0x00), pBodyB->m_scratch123.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(kNegNormal, kNegNormal, 0xAA), pBodyB->m_scratch123.m_col2.m_quad));
	const __m128 kTwistAngA = pSchema[6].m_quad;
	pSchema[7].m_quad = kTwistAngB;
	const __m128 kTwistMass = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(kTwistAngA, kTwistAngA), pBodyA->m_invMasses.m_quad),
		_mm_mul_ps(_mm_mul_ps(kTwistAngB, kTwistAngB), pBodyB->m_invMasses.m_quad));
	const float fTwistDiag = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(kTwistMass, kTwistMass, 0xAA),
								 _mm_add_ss(_mm_shuffle_ps(kTwistMass, kTwistMass, 0x55), kTwistMass))) +
		1.1920928955078125e-07;
	pSchema[6].m_quad[3] = in.m_virtMassFactor / fTwistDiag;
	pSchema[7].m_quad[3] = kInfo.m_frictionResults[2].m_internalSolverData * in.m_frictionRhsFactor;
	pSchema[6].m_quad[3] = pSchema[6].m_quad[3] * fInvRadius;
	const float fContactRadius = kInfo.m_contactRadius;
	hkpJacobian3dFrictionSchema* pFriction3d = reinterpret_cast<hkpJacobian3dFrictionSchema*>(pSchema);
	pFriction3d->m_solverResults = &kInfo.m_frictionResults[0];
	pFriction3d->m_invJac01 = fInvJac01;
	pFriction3d->m_maxImpulse = fMaxImpulse;
	pFriction3d->m_contactRadius = fContactRadius;
	pSchema[0].m_quad[0] = pSchema[0].m_quad[0] + 1.1920928955078125e-07;
	pSchema[0].m_bytes.m_type = hkpJacobianSchema::SCHEMA_TYPE_3D_FRICTION;
	out.m_jacobianSchemas = pSchema + HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA / sizeof(hkpJacobianSchema);
}

float calculateRhs(hkpContactPointProperties& arProperties, float afDistance, float afDeltaTime) {
	double v5;
	float afDeltaTimeb;
	float m_internalDataA = arProperties.m_internalDataA;
	float afDistancec = afDistance - m_internalDataA;
	double v3 = afDistancec;
	float v12 = -arProperties.m_internalSolverData - afDistancec;
	float afDeltaTimea = HK_CONTACT_LINEAR_ERROR_RECOVERY_VELOCITY * afDeltaTime;
	float afDistancea = -HK_CONTACT_EXPONENTIAL_ERROR_RECOVERY_VELOCITY * m_internalDataA;
	double v4 = afDeltaTimea;
	if (!(afDeltaTimea > static_cast<double>(afDistancea))) {
		v5 = afDeltaTimea;
	} else {
		v4 = afDistancea;
		v5 = afDeltaTimea;
	}
	float afDistanced = v4;
	float v11 = m_internalDataA + afDistanced;
	double v6 = afDistanced;
	float afDistanceb = v3 - afDistanced;
	double v7 = v12;
	if (v5 + (v6 + v6) < v12) {
		v11 = v11 - v7;
		afDistanceb = v7 + afDistanceb;
	}
	if (!(v11 < -1.1920929e-07f)) {
		afDeltaTimeb = -1.1920929e-07f;
	} else {
		afDeltaTimeb = v11;
	}
	arProperties.m_internalDataA = afDeltaTimeb;
	return afDistanceb;
}
