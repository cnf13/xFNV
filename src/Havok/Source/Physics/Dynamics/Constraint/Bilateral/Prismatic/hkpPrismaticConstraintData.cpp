#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

#include <bit>
#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPrismaticConstraintDataAtomsTypeInfo = {"hkpPrismaticConstraintDataAtoms",
	finishLoadedObjecthkpPrismaticConstraintDataAtoms, cleanupLoadedObjecthkpPrismaticConstraintDataAtoms, nullptr,
	&typeid(hkpPrismaticConstraintData::Atoms), sizeof(hkpPrismaticConstraintData::Atoms)};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enum and members of
// hkpPrismaticConstraintData and its atoms, as the exe's .rdata holds them.
using hkpPrismaticConstraintDataAtomsAxisEnumItemsArray = const hkClassEnum::Item[2];
static hkpPrismaticConstraintDataAtomsAxisEnumItemsArray hkpPrismaticConstraintDataAtomsAxisEnumItems{
	{hkpPrismaticConstraintData::Atoms::AXIS_SHAFT, "AXIS_SHAFT"},
	{hkpPrismaticConstraintData::Atoms::AXIS_PERP_TO_SHAFT, "AXIS_PERP_TO_SHAFT"}};
using hkpPrismaticConstraintDataAtomsEnumsArray = const hkClassEnum[1];
static hkpPrismaticConstraintDataAtomsEnumsArray hkpPrismaticConstraintDataAtomsEnums{
	{"Axis", hkpPrismaticConstraintDataAtomsAxisEnumItems, 2, nullptr, {0}}};
using hkpPrismaticConstraintData_AtomsClass_MembersArray = const hkClassMember[7];
static hkpPrismaticConstraintData_AtomsClass_MembersArray hkpPrismaticConstraintData_AtomsClass_Members{
	{"transforms", &hkpSetLocalTransformsConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"motor", &hkpLinMotorConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0,
		{0}, 0x90, nullptr},
	{"friction", &hkpLinFrictionConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID},
		0, {0}, 0xA0, nullptr},
	{"ang", &hkpAngConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xA8,
		nullptr},
	{"lin0", &hkpLinConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, {0},
		0xAC, nullptr},
	{"lin1", &hkpLinConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, {0},
		0xB0, nullptr},
	{"linLimit", &hkpLinLimitConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0,
		{0}, 0xB4, nullptr}};
using hkpPrismaticConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpPrismaticConstraintDataClass_MembersArray hkpPrismaticConstraintDataClass_Members{
	{"atoms", &hkpPrismaticConstraintDataAtomsClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID},
		0, {hkClassMember::ALIGN_16}, 0x10, nullptr}};
hkClass hkpPrismaticConstraintDataAtomsClass("hkpPrismaticConstraintDataAtoms", nullptr, 192, nullptr, 0,
	hkpPrismaticConstraintDataAtomsEnums, 1, hkpPrismaticConstraintData_AtomsClass_Members, 7, nullptr, nullptr, 0, 0);
hkClass hkpPrismaticConstraintDataClass("hkpPrismaticConstraintData", &hkpConstraintDataClass, 208, nullptr, 0, nullptr,
	0, hkpPrismaticConstraintDataClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpPrismaticConstraintDataTypeInfo = {"hkpPrismaticConstraintData",
	finishLoadedObjecthkpPrismaticConstraintData, cleanupLoadedObjecthkpPrismaticConstraintData,
	getVtablehkpPrismaticConstraintData(), &typeid(hkpPrismaticConstraintData), sizeof(hkpPrismaticConstraintData)};

hkpPrismaticConstraintData::Atoms::Atoms() {
	m_transforms.m_type.m_storage = (uint16_t)2;
	m_motor.m_type.m_storage = (uint16_t)11;
	m_motor.m_isEnabled.m_bool = 1;
	m_motor.m_initializedOffset = 0;
	m_motor.m_previousTargetPositionOffset = 0;
	m_friction.m_isEnabled = (uint8_t)1;
	m_friction.m_type.m_storage = (uint16_t)10;
	m_ang.m_type.m_storage = (uint16_t)13;
	m_lin0.m_type.m_storage = (uint16_t)7;
	m_lin1.m_type.m_storage = (uint16_t)7;
	m_linLimit.m_type.m_storage = (uint16_t)9;
}

const hkClass& hkpPrismaticConstraintData::Atoms::staticClass() {
	return hkpPrismaticConstraintDataAtomsClass;
}

hkpPrismaticConstraintData::hkpPrismaticConstraintData() {
	m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad[0] = 1.0f;
	m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad[1] = 1.0f;
	m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad[2] = 1.0f;
	m_atoms.m_transforms.m_transformA.m_translation.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad = _mm_setzero_ps();
	m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad[0] = 1.0f;
	m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad[1] = 1.0f;
	m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad[2] = 1.0f;
	m_atoms.m_transforms.m_transformB.m_translation.m_quad = _mm_setzero_ps();
	m_atoms.m_linLimit.m_axisIndex = 0;
	m_atoms.m_linLimit.m_min = -3.40282e+38f;
	m_atoms.m_friction.m_frictionAxis = 0;
	m_atoms.m_linLimit.m_max = 3.40282e+38f;
	m_atoms.m_friction.m_maxFrictionForce = 0.0f;
	m_atoms.m_motor.m_isEnabled.m_bool = 0;
	m_atoms.m_motor.m_targetPosition = 0.0f;
	m_atoms.m_motor.m_motor = 0;
	m_atoms.m_motor.m_motorAxis = 0;
	m_atoms.m_ang.m_firstConstrainedAxis = 0;
	m_atoms.m_motor.m_initializedOffset = 64;
	m_atoms.m_motor.m_previousTargetPositionOffset = 68;
	m_atoms.m_ang.m_numConstrainedAxes = 3;
	m_atoms.m_lin0.m_axisIndex = 1;
	m_atoms.m_lin1.m_axisIndex = 2;
}

hkpPrismaticConstraintData::~hkpPrismaticConstraintData() {
	if (m_atoms.m_motor.m_motor) {
		m_atoms.m_motor.m_motor->removeReference();
	}
}

hkBool* hkpPrismaticConstraintData::isValid(hkBool* arResult) const {
	hkBool a2;
	hkBool apResult;
	if (m_atoms.m_transforms.m_transformA.m_rotation.isOrthonormal(&a2, 0.001f)->m_bool &&
		(m_atoms.m_transforms.m_transformB.m_rotation.isOrthonormal(&apResult, 0.001f)->m_bool &&
			m_atoms.m_linLimit.m_max >= static_cast<double>(m_atoms.m_linLimit.m_min))) {
		arResult->m_bool = 1;
		return arResult;
	} else {
		arResult->m_bool = 0;
		return arResult;
	}
}

int32_t hkpPrismaticConstraintData::getType() const {
	return 6;
}

void hkpPrismaticConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
	(&infoOut)->m_numSolverResults = 8;
	(&infoOut)->m_sizeOfExternalRuntime = 72;
}

// The atoms' span up to the end of the last atom: x64's tail padding after it is no atom.
void hkpPrismaticConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
	getConstraintInfoUtil(m_atoms.getAtoms(), m_atoms.getSizeOfAllAtoms(), infoOut);
}

void hkpPrismaticConstraintData::setMotor(hkpConstraintMotor* motor) {
	if (motor) {
		motor->addReference();
	}
	if (m_atoms.m_motor.m_motor) {
		m_atoms.m_motor.m_motor->removeReference();
	}
	m_atoms.m_motor.m_motor = motor;
}

// The SDK's setInWorldSpace: the constraint's base in world space is the axis normalized, a perpendicular of it
// normalized and their cross product; each body's frame rotation is its rotation's transpose times that base, its
// translation the pivot in the body's space. The exe inlines normalize3, dot3,
// hkVector4Util::calculatePerpendicularVector, setCross and hkMatrix3::setMulInverseMul with SSE; the |x| compares
// and the perpendicular's lanes are x87 (x87 copies at 0xD4FA05..0xD4FA1D).
void hkpPrismaticConstraintData::setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform,
	const hkVector4& pivot, const hkVector4& axis) {
	// EXE-INLINED 0x62B0A0
	// hkVector4::dot3: mulps, then y + x and z + that (addss), in the low lane.
	auto dot3 = [](__m128 a, __m128 b) {
		const __m128 p = _mm_mul_ps(a, b);
		return _mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p));
	};
	// EXE-INLINED 0x62AEA0
	// hkVector4::normalize3: rsqrtss refined once (0.5 r (3 - len2 r r)), zero for a zero length, times the vector.
	auto normalize3 = [&dot3](__m128 v) {
		const __m128 len2 = dot3(v, v);
		const __m128 approx = _mm_rsqrt_ss(len2);
		const __m128 lenInv = _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_setzero_ps()),
			_mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
				_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx))));
		return _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), v);
	};
	// EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
	// The lane of the smallest |x| is left zero; the other two are swapped, one negated.
	auto setPerpendicular = [](hkVector4& arOut, const hkVector4& arIn) {
		int32_t iMin = 0;
		int32_t iOk1 = 1;
		int32_t iOk2 = 2;
		float fA0 = std::fabs(arIn(0));
		const float fA1 = std::fabs(arIn(1));
		const float fA2 = std::fabs(arIn(2));
		if (fA0 > fA1) {
			fA0 = fA1;
			iOk1 = 0;
			iMin = 1;
		}
		if (fA0 > fA2) {
			iOk2 = iMin;
		}
		arOut.m_quad = _mm_setzero_ps();
		arOut(iOk1) = arIn(iOk2);
		arOut(iOk2) = -arIn(iOk1);
	};
	// EXE-INLINED none: hkVector4::setCross
	auto cross = [](__m128 v0, __m128 v1) {
		return _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 0, 2, 1)),
							  _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 1, 0, 2))),
			_mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 1, 0, 2)),
				_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 0, 2, 1))));
	};
	// EXE-INLINED 0xCD4BF0
	// hkMatrix3::setMulInverseMul: out = a's transpose times b, a's transpose built first, column by column.
	auto setMulInverseMul = [](hkRotation& arOut, const hkRotation& a, const hkRotation& b) {
		const __m128 col1 = a.m_col1.m_quad;
		const __m128 col2 = a.m_col2.m_quad;
		const __m128 lo = _mm_unpacklo_ps(a.m_col0.m_quad, col1);
		const __m128 hi = _mm_unpackhi_ps(a.m_col0.m_quad, col1);
		const __m128 row0 = _mm_movelh_ps(lo, col2);
		const __m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), col2, 212);
		const __m128 row2 = _mm_shuffle_ps(hi, col2, 228);
		for (int32_t i = 0; i < 3; ++i) {
			const __m128 col = (&b.m_col0)[i].m_quad;
			(&arOut.m_col0)[i].m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(col, col, 85), row1),
													   _mm_mul_ps(_mm_shuffle_ps(col, col, 0), row0)),
				_mm_mul_ps(_mm_shuffle_ps(col, col, 170), row2));
		}
	};
	hkRotation constraintBaseInW;
	constraintBaseInW.m_col0.m_quad = normalize3(axis.m_quad);
	setPerpendicular(constraintBaseInW.m_col1, constraintBaseInW.m_col0);
	constraintBaseInW.m_col1.m_quad = normalize3(constraintBaseInW.m_col1.m_quad);
	constraintBaseInW.m_col2.m_quad = cross(constraintBaseInW.m_col0.m_quad, constraintBaseInW.m_col1.m_quad);
	setMulInverseMul(m_atoms.m_transforms.m_transformA.m_rotation, bodyATransform.m_rotation, constraintBaseInW);
	setMulInverseMul(m_atoms.m_transforms.m_transformB.m_rotation, bodyBTransform.m_rotation, constraintBaseInW);
	m_atoms.m_transforms.m_transformA.m_translation.setTransformedInversePos(bodyATransform, pivot);
	m_atoms.m_transforms.m_transformB.m_translation.setTransformedInversePos(bodyBTransform, pivot);
}

void hkpPrismaticConstraintData::setMotorActive(hkpConstraintInstance* instance, hkBool toBeEnabled) {
	m_atoms.m_motor.m_isEnabled = toBeEnabled;
	m_atoms.m_friction.m_isEnabled = toBeEnabled.m_bool == 0;
	if (instance) {
		hkConstraintInternal* m_internal = instance->getInternal();
		if (m_internal) {
			float* m_runtime = static_cast<float*>(m_internal->m_runtime);
			if (m_runtime) {
				*m_runtime = 0.0f;
				m_runtime[1] = 0.0f;
				m_runtime[2] = 0.0f;
				m_runtime[3] = 0.0f;
			}
		}
	}
}

void hkpPrismaticConstraintData::setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB,
	const hkVector4& axisA, const hkVector4& axisB, const hkVector4& axisAPerp, const hkVector4& axisBPerp) {
	__m128 v11 = _mm_undefined_ps();
	hkVector4 v17;
	hkVector4 v32;
	m_atoms.m_transforms.m_transformA.m_translation.m_quad = pivotA.m_quad;
	m_atoms.m_transforms.m_transformB.m_translation.m_quad = pivotB.m_quad;
	__m128 v7 = _mm_mul_ps(axisA.m_quad, axisA.m_quad);
	float v8 = _mm_shuffle_ps(v7, v7, 85)[0] + v7[0];
	__m128 v9 = _mm_shuffle_ps(v7, v7, 170);
	v9[0] = v9[0] + v8;
	float v10 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v9[0])));
	v11[0] = std::bit_cast<float>(static_cast<int32_t>(0.0f));
	__m128 v12 = _mm_setzero_ps();
	v12[0] = 0.5f;
	__m128 v13 = _mm_cmpeq_ss(v9, v11);
	float v14 = 3.0f - static_cast<float>(static_cast<float>(v9[0] * v10) * v10);
	v12[0] = static_cast<float>(0.5f * v10) * v14;
	__m128 v16 = _mm_andnot_ps(v13, v12);
	m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), axisA.m_quad);
	m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = axisAPerp.m_quad;
	v17.m_quad = m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad;
	__m128 v18 = _mm_mul_ps(v17.m_quad, v17.m_quad);
	v16[0] = _mm_shuffle_ps(v18, v18, 85)[0] + v18[0];
	__m128 v19 = _mm_shuffle_ps(v18, v18, 170);
	v19[0] = v19[0] + v16[0];
	float v20 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v19[0])));
	float v22 = 3.0f - static_cast<float>(static_cast<float>(v19[0] * v20) * v20);
	v12[0] = static_cast<float>(0.5f * v20) * v22;
	__m128 v24 = _mm_andnot_ps(_mm_cmpeq_ss(v19, v11), v12);
	m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v24, v24, 0), v17.m_quad);
	m_atoms.m_transforms.m_transformA.m_rotation.m_col2.m_quad =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad,
								  m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 210),
					   _mm_shuffle_ps(m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad,
						   m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 201)),
			_mm_mul_ps(_mm_shuffle_ps(m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad,
						   m_atoms.m_transforms.m_transformA.m_rotation.m_col1.m_quad, 201),
				_mm_shuffle_ps(m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad,
					m_atoms.m_transforms.m_transformA.m_rotation.m_col0.m_quad, 210)));
	__m128 v25 = _mm_mul_ps(axisB.m_quad, axisB.m_quad);
	v24[0] = _mm_shuffle_ps(v25, v25, 85)[0] + v25[0];
	__m128 v26 = _mm_shuffle_ps(v25, v25, 170);
	v26[0] = v26[0] + v24[0];
	float v27 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v26[0])));
	__m128 v28 = _mm_cmpeq_ss(v26, v11);
	float v29 = 3.0f - static_cast<float>(static_cast<float>(v26[0] * v27) * v27);
	v12[0] = static_cast<float>(0.5f * v27) * v29;
	__m128 v31 = _mm_andnot_ps(v28, v12);
	m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad = _mm_mul_ps(_mm_shuffle_ps(v31, v31, 0), axisB.m_quad);
	m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = axisBPerp.m_quad;
	v32.m_quad = m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad;
	__m128 v33 = _mm_mul_ps(v32.m_quad, v32.m_quad);
	v31[0] = _mm_shuffle_ps(v33, v33, 85)[0] + v33[0];
	__m128 v34 = _mm_shuffle_ps(v33, v33, 170);
	v34[0] = v34[0] + v31[0];
	v31[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v34[0])));
	v12[0] = static_cast<float>(0.5f * v31[0]) *
		static_cast<float>(3.0f - static_cast<float>(static_cast<float>(v34[0] * v31[0]) * v31[0]));
	__m128 v35 = _mm_andnot_ps(_mm_cmpeq_ss(v34, v11), v12);
	m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad = _mm_mul_ps(_mm_shuffle_ps(v35, v35, 0), v32.m_quad);
	m_atoms.m_transforms.m_transformB.m_rotation.m_col2.m_quad =
		_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
								  m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 201),
					   _mm_shuffle_ps(m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad,
						   m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad, 210)),
			_mm_mul_ps(_mm_shuffle_ps(m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad,
						   m_atoms.m_transforms.m_transformB.m_rotation.m_col0.m_quad, 210),
				_mm_shuffle_ps(m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad,
					m_atoms.m_transforms.m_transformB.m_rotation.m_col1.m_quad, 201)));
}

const hkClass& hkpPrismaticConstraintData::staticClass() {
	return hkpPrismaticConstraintDataClass;
}
