#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

class hkpConstraintData;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;

// The atom that hands a constraint's jacobian building over to its hkpConstraintData.
struct hkpBridgeConstraintAtom : public hkpConstraintAtom {
    // The constraint data's Jacobian builder, taking (data, in, out); hkSolverBuildJacobianFromAtomsNotContact calls
    // it.
    using BuildJacobianFunc =
        void (*)(hkpConstraintData* apConstraintData, const hkpConstraintQueryIn& arIn, hkpConstraintQueryOut& arOut);

    // Stores constraintData in m_constraintData and hkBridgeConstraintAtom_callData in m_buildJacobianFunc. It leaves
    // m_type alone, which the callers set themselves.
    void init(hkpConstraintData* constraintData);

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

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
