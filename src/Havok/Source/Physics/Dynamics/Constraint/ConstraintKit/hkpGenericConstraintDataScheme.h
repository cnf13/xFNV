#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Reconstruction.hpp"

class hkpConstraintModifier;
class hkpConstraintMotor;

// The program of a generic constraint (the constraint kit): the solver memory it needs, the data the commands read
// and the commands themselves, with the modifiers and motors they refer to. Not polymorphic. IDA spells the four
// arrays hkArray_void; the element types are the Xbox PDB's.
class hkpGenericConstraintDataScheme {
public:
	// The solver memory a scheme needs, as its reflection describes m_info (the SDK's reflected struct; the exe's
	// hkClass hkpGenericConstraintDataSchemeConstraintInfoClass has the four ints at 0x0 to 0xC and an object size of
	// 0x10). The SDK's code declares m_info as the same-layout hkpConstraintInfo, as src does.
	struct ConstraintInfo {
		// The reflection class hkpGenericConstraintDataSchemeConstraintInfoClass.
		static const hkClass& staticClass();

		int32_t m_maxSizeOfSchema;
		int32_t m_sizeOfSchemas;
		int32_t m_numSolverResults;
		int32_t m_numSolverElemTemps;
	};

	// The commands m_commands holds, from the SDK's header; the exe's values match it (the constraint kit's
	// setConeLimit appends 18 and setTwistLimit 19).
	enum {
		e_endScheme = 0,
		e_setPivotA,
		e_setPivotB,
		e_setLinearDofA,
		e_setLinearDofB,
		e_setLinearDofW,
		e_constrainLinearW,
		e_constrainAllLinearW,
		e_setAngularBasisA,
		e_setAngularBasisB,
		e_setAngularBasisAidentity,
		e_setAngularBasisBidentity,
		e_constrainToAngularW,
		e_constrainAllAngularW,
		e_setAngularMotor,
		e_setLinearMotor,
		e_setLinearLimit,
		e_setAngularLimit,
		e_setConeLimit,
		e_setTwistLimit,
		e_setAngularFriction,
		e_setLinearFriction,
		e_setStrength,
		e_restoreStrengh,
		e_doConstraintModifier,
		e_numCommands
	};

	// The SDK's inline default constructor, which the exe inlines into hkpGenericConstraintData's: empty arrays and
	// a constraint info holding only the header schema (hkpConstraintInfo::addHeader).
	hkpGenericConstraintDataScheme() {
		m_info.m_maxSizeOfSchema = 0;
		m_info.m_sizeOfSchemas = 0;
		m_info.m_numSolverResults = 0;
		m_info.m_numSolverElemTemps = 0;
		m_info.m_sizeOfSchemas += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
	}

	hkpGenericConstraintDataScheme(hkFinishLoadedObjectFlag f);
	~hkpGenericConstraintDataScheme();

	hkpConstraintInfo m_info;
	hkArray<hkVector4> m_data;
	hkArray<int32_t> m_commands;
	hkArray<hkpConstraintModifier*> m_modifiers;
	hkArray<hkpConstraintMotor*> m_motors;
};

// The class's reflection record.
extern hkClass hkpGenericConstraintDataSchemeClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpGenericConstraintDataSchemeConstraintInfoTypeInfo;
extern hkTypeInfo hkpGenericConstraintDataSchemeTypeInfo;
extern hkClass hkpGenericConstraintDataSchemeConstraintInfoClass;
