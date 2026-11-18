#pragma once

#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkp2dAngConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// The angular limits of a hinge, used as a link of a constraint chain. It overrides isValid, getType, getRuntimeInfo
// and getConstraintInfo; the destructor is hkBaseObject's.
class hkpHingeLimitsData : public hkpConstraintData {
public:
    // The runtime memory of one instance.
    struct Runtime {
        hkpSolverResults m_solverResults[3];
        float m_lastAngle;
    };

    // The atoms the constraint is made of.
    struct Atoms {
        // Values from the Havok SDK.
        enum Axis : int32_t {
            AXIS_AXLE = 0x0,
            AXIS_PERP_TO_AXLE_1 = 0x1,
            AXIS_PERP_TO_AXLE_2 = 0x2,
        };

        Atoms() = default;
        // The SDK's finish constructor: the loaded atoms keep their fields.
        Atoms(hkFinishLoadedObjectFlag) {}

        // The reflection class hkpHingeLimitsDataAtomsClass.
        static const hkClass& staticClass();

        hkpSetLocalRotationsConstraintAtom m_rotations;
        hkpAngLimitConstraintAtom m_angLimit;
        hkp2dAngConstraintAtom m_2dAng;
    };

    // Sets a reference count of 1, the atoms' types, identity rotations, and the angle limits -pi..pi with strength
    // 1.0.
    hkpHingeLimitsData();
    hkpHingeLimitsData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag), m_atoms(aFlag) {}

    hkBool isValid() const override;
    int32_t getType() const override;
    void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
    void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

    // Builds rotation A from axisA and axisAPerp and rotation B from axisB and axisBPerp, each normalised, with the
    // third columns their cross products.
    void setInBodySpace(
        const hkVector4& axisA,
        const hkVector4& axisB,
        const hkVector4& axisAPerp,
        const hkVector4& axisBPerp
    );

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    // The atoms start on a 16-byte boundary, so the 4 bytes after the base class are the compiler's alignment padding.
    alignas(16) Atoms m_atoms;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpHingeLimitsDataAtomsTypeInfo;
extern hkClass hkpHingeLimitsDataAtomsClass;
extern hkClass hkpHingeLimitsDataClass;
extern hkTypeInfo hkpHingeLimitsDataTypeInfo;
