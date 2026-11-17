#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

class hkpConstraintData;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;

// The atom that hands a constraint's jacobian building over to its hkpConstraintData (the PDB puts it in
// hkpConstraintAtomClass.cpp; the header name is Havok's own).
struct hkpBridgeConstraintAtom : public hkpConstraintAtom {
	// The SDK's hkConstraintAtomBuildJacobianFunc: the constraint data's Jacobian builder, cdecl (data, in, out);
	// hkSolverBuildJacobianFromAtomsNotContact (0xD6F460) calls it.
	using BuildJacobianFunc = void (*)(hkpConstraintData* apConstraintData, const hkpConstraintQueryIn& arIn,
		hkpConstraintQueryOut& arOut);

	// The PDB's init (hkpConstraintAtom.cpp): stores constraintData in m_constraintData and
	// hkBridgeConstraintAtom_callData (0xD21CC0) in m_buildJacobianFunc (thiscall, ret 4, 0xD21CE0). No constructor:
	// it leaves m_type alone, which the callers set themselves.
	void init(hkpConstraintData* constraintData);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	uint8_t padding02[2];
	BuildJacobianFunc m_buildJacobianFunc;
	hkpConstraintData* m_constraintData;
};

// The atoms of a constraint whose data lies in another one: the bridge atom alone.
struct hkpBridgeAtoms {
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();

	hkpBridgeConstraintAtom m_bridgeAtom;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpBridgeConstraintAtomTypeInfo;
extern hkTypeInfo hkpBridgeAtomsTypeInfo;
extern hkClass hkpBridgeConstraintAtomClass;
extern hkClass hkpBridgeAtomsClass;
