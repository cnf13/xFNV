#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinFrictionConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinSoftConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpPulleyConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpRagdollMotorConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTranslationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpStiffSpringConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enum and members of
// hkpConstraintData, as the exe's .rdata holds them.
using hkpConstraintDataConstraintTypeEnumItemsArray = const hkClassEnum::Item[22];
static hkpConstraintDataConstraintTypeEnumItemsArray hkpConstraintDataConstraintTypeEnumItems{
	{hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET, "CONSTRAINT_TYPE_BALLANDSOCKET"},
	{hkpConstraintData::CONSTRAINT_TYPE_HINGE, "CONSTRAINT_TYPE_HINGE"},
	{hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE, "CONSTRAINT_TYPE_LIMITEDHINGE"},
	{hkpConstraintData::CONSTRAINT_TYPE_POINTTOPATH, "CONSTRAINT_TYPE_POINTTOPATH"},
	{hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC, "CONSTRAINT_TYPE_PRISMATIC"},
	{hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL, "CONSTRAINT_TYPE_RAGDOLL"},
	{hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING, "CONSTRAINT_TYPE_STIFFSPRING"},
	{hkpConstraintData::CONSTRAINT_TYPE_WHEEL, "CONSTRAINT_TYPE_WHEEL"},
	{hkpConstraintData::CONSTRAINT_TYPE_GENERIC, "CONSTRAINT_TYPE_GENERIC"},
	{hkpConstraintData::CONSTRAINT_TYPE_CONTACT, "CONSTRAINT_TYPE_CONTACT"},
	{hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE, "CONSTRAINT_TYPE_BREAKABLE"},
	{hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE, "CONSTRAINT_TYPE_MALLEABLE"},
	{hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE, "CONSTRAINT_TYPE_POINTTOPLANE"},
	{hkpConstraintData::CONSTRAINT_TYPE_PULLEY, "CONSTRAINT_TYPE_PULLEY"},
	{hkpConstraintData::CONSTRAINT_TYPE_ROTATIONAL, "CONSTRAINT_TYPE_ROTATIONAL"},
	{hkpConstraintData::CONSTRAINT_TYPE_HINGE_LIMITS, "CONSTRAINT_TYPE_HINGE_LIMITS"},
	{hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL_LIMITS, "CONSTRAINT_TYPE_RAGDOLL_LIMITS"},
	{hkpConstraintData::CONSTRAINT_TYPE_CUSTOM, "CONSTRAINT_TYPE_CUSTOM"},
	{hkpConstraintData::BEGIN_CONSTRAINT_CHAIN_TYPES, "BEGIN_CONSTRAINT_CHAIN_TYPES"},
	{hkpConstraintData::CONSTRAINT_TYPE_STIFF_SPRING_CHAIN, "CONSTRAINT_TYPE_STIFF_SPRING_CHAIN"},
	{hkpConstraintData::CONSTRAINT_TYPE_BALL_SOCKET_CHAIN, "CONSTRAINT_TYPE_BALL_SOCKET_CHAIN"},
	{hkpConstraintData::CONSTRAINT_TYPE_POWERED_CHAIN, "CONSTRAINT_TYPE_POWERED_CHAIN"}};
using hkpConstraintDataEnumsArray = const hkClassEnum[1];
static hkpConstraintDataEnumsArray hkpConstraintDataEnums{
	{"ConstraintType", hkpConstraintDataConstraintTypeEnumItems, 22, nullptr, {0}}};
using hkpConstraintDataClass_MembersArray = const hkClassMember[1];
static hkpConstraintDataClass_MembersArray hkpConstraintDataClass_Members{
	{"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr}};

hkClass hkpConstraintDataClass("hkpConstraintData", &hkReferencedObjectClass, 12, nullptr, 0, hkpConstraintDataEnums, 1,
	hkpConstraintDataClass_Members, 1, nullptr, nullptr, 0, 0);

hkpSolverResults* hkpConstraintData::getSolverResults(void* runtime) {
	return static_cast<hkpSolverResults*>(runtime);
}

void hkpConstraintData::addInstance(hkpConstraintInstance* constraint, void* runtime, int32_t sizeOfRuntime) const {
	if (runtime) {
		hkString::memSet(runtime, 0, sizeOfRuntime);
	}
}

// The solver memory the atoms need. A contact atom alone gives the contact schemas (a header, a pair schema per two
// points, a single one for the last odd point, and a 2D friction schema from two points on). Otherwise the walk adds
// each atom's schema, solver results and element temporaries (the SDK's addToConstraintInfo, inlined) and steps past
// it (its next()); padding atoms are skipped to the next 16-byte boundary and the atom found there is handled without
// the end test. A bridge or modifier atom ends the walk; another type (an overwrite pivot, a contact, a centre of mass
// changer) is not stepped past, as in the exe.
void hkpConstraintData::getConstraintInfoUtil(const hkpConstraintAtom* atoms, int32_t sizeOfAllAtoms,
	hkpConstraintData::ConstraintInfo& infoOut) {
	infoOut.m_atoms = const_cast<hkpConstraintAtom*>(atoms);
	infoOut.m_sizeOfAllAtoms = sizeOfAllAtoms;
	infoOut.m_maxSizeOfSchema = 0;
	infoOut.m_sizeOfSchemas = 0;
	infoOut.m_numSolverResults = 0;
	infoOut.m_numSolverElemTemps = 0;
	if (atoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
		const int32_t iNumContactPoints = static_cast<const hkpSimpleContactConstraintAtom*>(atoms)->m_numContactPoints;
		// The SDK's hkpSimpleContactConstraintAtom::addToConstraintInfo: the header, the contact points in pairs and
		// a single one, and the 2D friction schema; a 3D friction schema in its place at most, and with two points.
		const int32_t iSize = HK_SIZE_OF_JACOBIAN_PAIR_CONTACT_SCHEMA * (iNumContactPoints >> 1) +
			HK_SIZE_OF_JACOBIAN_SINGLE_CONTACT_SCHEMA * (iNumContactPoints & 1) + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA +
			HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
		infoOut.m_sizeOfSchemas = iSize;
		infoOut.m_maxSizeOfSchema =
			iSize + (HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
		infoOut.m_numSolverResults = iNumContactPoints + 2;
		infoOut.m_numSolverElemTemps = iNumContactPoints + 3;
		if (iNumContactPoints >= 2) {
			infoOut.add(HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA, 1, 1);
		}
		return;
	}
	infoOut.m_sizeOfSchemas = HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
	// The atoms are a raw byte block of sizeOfAllAtoms bytes; its end is the SDK inline's integer sum of the address
	// and the size (hkUlong arithmetic), which the exe adds and compares unsigned. A pointer sum would let clang
	// fold the first end test to sizeOfAllAtoms > 0, where the exe's sum can wrap.
	// EXE-INLINED none: hkAddByteOffsetConst
	const hkpConstraintAtom* atomsEnd =
		reinterpret_cast<const hkpConstraintAtom*>(reinterpret_cast<uintptr_t>(atoms) + sizeOfAllAtoms);
	const hkpConstraintAtom* atom = atoms;
	while (atom < atomsEnd) {
	dispatch:
		switch (atom->m_type.m_storage) {
		case hkpConstraintAtom::TYPE_INVALID:
			// Padding in the raw block: the SDK's HK_NEXT_MULTIPLE_OF(16) on the address.
			atom = reinterpret_cast<const hkpConstraintAtom*>((reinterpret_cast<uintptr_t>(atom) + 15) &
				~static_cast<uintptr_t>(15));
			goto dispatch;
		case hkpConstraintAtom::TYPE_BRIDGE:
		case hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT:
		case hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER:
		case hkpConstraintAtom::TYPE_MODIFIER_VISCOUS_SURFACE:
		case hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE:
		case hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT:
			return;
		case hkpConstraintAtom::TYPE_SET_LOCAL_TRANSFORMS:
			atom = static_cast<const hkpSetLocalTransformsConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_SET_LOCAL_TRANSLATIONS:
			atom = static_cast<const hkpSetLocalTranslationsConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_SET_LOCAL_ROTATIONS:
			atom = static_cast<const hkpSetLocalRotationsConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_BALL_SOCKET:
			infoOut.add(144, 3, 3);
			atom = static_cast<const hkpBallSocketConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_STIFF_SPRING:
			infoOut.add(48, 1, 1);
			atom = static_cast<const hkpStiffSpringConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_LIN:
			infoOut.add(48, 1, 1);
			atom = static_cast<const hkpLinConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_LIN_SOFT:
			infoOut.add(64, 1, 1);
			atom = static_cast<const hkpLinSoftConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_LIN_LIMIT:
			infoOut.add(64, 1, 1);
			atom = static_cast<const hkpLinLimitConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_LIN_FRICTION:
			infoOut.add(64, 1, 2);
			atom = static_cast<const hkpLinFrictionConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_LIN_MOTOR:
			infoOut.add(80, 1, 3);
			atom = static_cast<const hkpLinMotorConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_2D_ANG:
			infoOut.add(96, 2, 2);
			atom = static_cast<const hkp2dAngConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_ANG: {
			const hkpAngConstraintAtom* pAng = static_cast<const hkpAngConstraintAtom*>(atom);
			const int32_t iNumAxes = pAng->m_numConstrainedAxes;
			infoOut.add(48 * iNumAxes, iNumAxes, iNumAxes);
			atom = pAng + 1;
			break;
		}
		case hkpConstraintAtom::TYPE_ANG_LIMIT:
			infoOut.add(48, 1, 1);
			atom = static_cast<const hkpAngLimitConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_TWIST_LIMIT:
			infoOut.add(48, 1, 1);
			atom = static_cast<const hkpTwistLimitConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_CONE_LIMIT:
			infoOut.add(48, 1, 1);
			atom = static_cast<const hkpConeLimitConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_ANG_FRICTION: {
			const hkpAngFrictionConstraintAtom* pFriction = static_cast<const hkpAngFrictionConstraintAtom*>(atom);
			const int32_t iNumAxes = pFriction->m_numFrictionAxes;
			infoOut.add(48 * iNumAxes, iNumAxes, 2 * iNumAxes);
			atom = pFriction + 1;
			break;
		}
		case hkpConstraintAtom::TYPE_ANG_MOTOR:
			infoOut.add(64, 1, 3);
			atom = static_cast<const hkpAngMotorConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_RAGDOLL_MOTOR:
			infoOut.add(192, 3, 9);
			atom = static_cast<const hkpRagdollMotorConstraintAtom*>(atom) + 1;
			break;
		case hkpConstraintAtom::TYPE_PULLEY:
			infoOut.add(64, 1, 1);
			atom = static_cast<const hkpPulleyConstraintAtom*>(atom) + 1;
			break;
		default:
			break;
		}
	}
}
