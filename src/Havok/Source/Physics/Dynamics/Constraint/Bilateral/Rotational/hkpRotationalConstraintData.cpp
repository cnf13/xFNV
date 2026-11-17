#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Rotational/hkpRotationalConstraintData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

#include <xmmintrin.h>

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpRotationalConstraintDataAtomsTypeInfo = {"hkpRotationalConstraintDataAtoms",
	finishLoadedObjecthkpRotationalConstraintDataAtoms, cleanupLoadedObjecthkpRotationalConstraintDataAtoms, nullptr,
	&typeid(hkpRotationalConstraintData::Atoms), sizeof(hkpRotationalConstraintData::Atoms)};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected members of
// hkpRotationalConstraintData and its atoms, as the exe's .rdata holds them.
using hkpRotationalConstraintData_AtomsClass_MembersArray = const hkClassMember[2];
static hkpRotationalConstraintData_AtomsClass_MembersArray hkpRotationalConstraintData_AtomsClass_Members{
	{"rotations", &hkpSetLocalRotationsConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
	{"ang", &hkpAngConstraintAtomClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x70,
		nullptr}};
using hkpRotationalConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpRotationalConstraintDataClass_MembersArray hkpRotationalConstraintDataClass_Members{
	{"atoms", &hkpRotationalConstraintDataAtomsClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID},
		0, {hkClassMember::ALIGN_16}, 0x10, nullptr}};

hkClass hkpRotationalConstraintDataAtomsClass("hkpRotationalConstraintDataAtoms", nullptr, 128, nullptr, 0, nullptr, 0,
	hkpRotationalConstraintData_AtomsClass_Members, 2, nullptr, nullptr, 0, 0);
hkClass hkpRotationalConstraintDataClass("hkpRotationalConstraintData", &hkpConstraintDataClass, 144, nullptr, 0,
	nullptr, 0, hkpRotationalConstraintDataClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpRotationalConstraintDataTypeInfo = {"hkpRotationalConstraintData",
	finishLoadedObjecthkpRotationalConstraintData, cleanupLoadedObjecthkpRotationalConstraintData,
	getVtablehkpRotationalConstraintData(), &typeid(hkpRotationalConstraintData), sizeof(hkpRotationalConstraintData)};

const hkClass& hkpRotationalConstraintData::Atoms::staticClass() {
	return hkpRotationalConstraintDataAtomsClass;
}

hkpRotationalConstraintData::hkpRotationalConstraintData() {
	m_atoms.m_rotations.m_type.m_storage = 4;
	m_atoms.m_ang.m_type.m_storage = 13;
	m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationA.m_col0.m_quad[0] = 1.0f;
	m_atoms.m_rotations.m_rotationA.m_col1.m_quad[1] = 1.0f;
	m_atoms.m_rotations.m_rotationA.m_col2.m_quad[2] = 1.0f;
	m_atoms.m_rotations.m_rotationB.m_col0.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationB.m_col1.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationB.m_col2.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationB.m_col0.m_quad[0] = 1.0f;
	m_atoms.m_rotations.m_rotationB.m_col1.m_quad[1] = 1.0f;
	m_atoms.m_rotations.m_rotationB.m_col2.m_quad[2] = 1.0f;
	m_atoms.m_ang.m_firstConstrainedAxis = 0;
	m_atoms.m_ang.m_numConstrainedAxes = 3;
}

hkBool* hkpRotationalConstraintData::isValid(hkBool* arResult) const {
	hkBool a2;
	hkBool apResult;
	if (m_atoms.m_rotations.m_rotationA.isOrthonormal(&a2, 0.001f)->m_bool &&
		m_atoms.m_rotations.m_rotationB.isOrthonormal(&apResult, 0.001f)->m_bool) {
		arResult->m_bool = 1;
		return arResult;
	} else {
		arResult->m_bool = 0;
		return arResult;
	}
}

int32_t hkpRotationalConstraintData::getType() const {
	return CONSTRAINT_TYPE_ROTATIONAL;
}

void hkpRotationalConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
	if (wantRuntime.m_bool) {
		infoOut.m_numSolverResults = 3;
		infoOut.m_sizeOfExternalRuntime = 24;
	} else {
		infoOut.m_numSolverResults = 0;
		infoOut.m_sizeOfExternalRuntime = 0;
	}
}

void hkpRotationalConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
	hkpConstraintData::getConstraintInfoUtil(&m_atoms.m_rotations, 116, infoOut);
}

void hkpRotationalConstraintData::setInBodySpace(const hkQuaternion& aTb) {
	m_atoms.m_rotations.m_rotationA.m_col0.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationA.m_col1.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationA.m_col2.m_quad = _mm_setzero_ps();
	m_atoms.m_rotations.m_rotationA.m_col0.m_quad[0] = 1.0f;
	m_atoms.m_rotations.m_rotationA.m_col1.m_quad[1] = 1.0f;
	m_atoms.m_rotations.m_rotationA.m_col2.m_quad[2] = 1.0f;
	m_atoms.m_rotations.m_rotationB.set(aTb);
}

void hkpRotationalConstraintData::setInWorldSpace(const hkQuaternion& bodyARotation,
	const hkQuaternion& bodyBRotation) {
	m_atoms.m_rotations.m_rotationA.set(bodyARotation);
	m_atoms.m_rotations.m_rotationB.set(bodyBRotation);
}

const hkClass& hkpRotationalConstraintData::staticClass() {
	return hkpRotationalConstraintDataClass;
}
